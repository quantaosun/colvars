// clang-format off
// -*- c++ -*-

// This file is part of the Collective Variables module (Colvars).
// The original version of Colvars and its updates are located at:
// https://github.com/Colvars/colvars
// Please update all Colvars source files before making any changes.
// If you wish to distribute your changes, please submit them to the
// Colvars repository at GitHub.


#include "colvarproxy_lammps.h"

#include "lammps.h"
#include "error.h"
#include "output.h"
#include "random_park.h"

#include "colvarmodule.h"
#include "colvarproxy.h"

#include <sys/stat.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <memory>

#define HASH_FAIL  -1


colvarproxy_lammps::colvarproxy_lammps(LAMMPS_NS::LAMMPS *lmp,
                                       const char *inp_name,
                                       const char *out_name,
                                       const int seed,
                                       const double temp,
                                       MPI_Comm root2root)
  : _lmp(lmp), inter_comm(root2root)
{
  if (cvm::debug())
    log("Initializing the colvars proxy object.\n");

  _random = new LAMMPS_NS::RanPark(lmp,seed);

  first_timestep=true;
  previous_step=-1;
  do_exit=false;

  // set input restart name and strip the extension, if present
  input_prefix_str = std::string(inp_name ? inp_name : "");
  if (input_prefix_str.rfind(".colvars.state") != std::string::npos)
    input_prefix_str.erase(input_prefix_str.rfind(".colvars.state"),
                            std::string(".colvars.state").size());

  // output prefix is always given
  output_prefix_str = std::string(out_name);
  // not so for restarts
  restart_output_prefix_str = std::string("rest");

  // check if it is possible to save output configuration
  if ((!output_prefix_str.size()) && (!restart_output_prefix_str.size())) {
    error("Error: neither the final output state file or "
          "the output restart file could be defined, exiting.\n");
  }

  // try to extract a restart prefix from a potential restart command.
  LAMMPS_NS::Output *outp = _lmp->output;
  if ((outp->restart_every_single > 0) && (outp->restart1 != nullptr)) {
    restart_frequency_engine = outp->restart_every_single;
    restart_output_prefix_str = std::string(outp->restart1);
  } else if  ((outp->restart_every_double > 0) && (outp->restart2a != nullptr)) {
    restart_frequency_engine = outp->restart_every_double;
    restart_output_prefix_str = std::string(outp->restart2a);
  }
  // trim off unwanted stuff from the restart prefix
  if (restart_output_prefix_str.rfind(".*") != std::string::npos)
    restart_output_prefix_str.erase(restart_output_prefix_str.rfind(".*"),2);

  // initialize multi-replica support, if available
  if (replica_enabled() == COLVARS_OK) {
    MPI_Comm_rank(inter_comm, &inter_me);
    MPI_Comm_size(inter_comm, &inter_num);
  }

  if (cvm::debug())
    log("Done initializing the colvars proxy object.\n");
}


void colvarproxy_lammps::init(const char *conf_file)
{
  version_int = get_version_from_string(COLVARPROXY_VERSION);

  // create the colvarmodule instance
  colvars = new colvarmodule(this);

  cvm::log("Using LAMMPS interface, version "+
           cvm::to_str(COLVARPROXY_VERSION)+".\n");

  colvars->cite_feature("LAMMPS engine");
  colvars->cite_feature("Colvars-LAMMPS interface");

  my_angstrom  = _lmp->force->angstrom;
  // Front-end unit is the same as back-end
  angstrom_value_ = my_angstrom;

  // my_kcal_mol  = _lmp->force->qe2f / 23.060549;
  // force->qe2f is 1eV expressed in LAMMPS' energy unit (1 if unit is eV, 23 if kcal/mol)
  boltzmann_ = _lmp->force->boltz;
  my_timestep  = _lmp->update->dt * _lmp->force->femtosecond;

  // TODO move one or more of these to setup() if needed
  colvars->read_config_file(conf_file);
  colvars->setup_input();
  colvars->setup_output();

  if (_lmp->update->ntimestep != 0) {
    cvm::log("Setting initial step number from LAMMPS: "+
             cvm::to_str(_lmp->update->ntimestep)+"\n");
    colvarmodule::it = colvarmodule::it_restart =
      static_cast<cvm::step_number>(_lmp->update->ntimestep);
  }

  if (cvm::debug()) {
    cvm::log("atoms_ids = "+cvm::to_str(atoms_ids)+"\n");
    cvm::log("atoms_ncopies = "+cvm::to_str(atoms_ncopies)+"\n");
    cvm::log("atoms_positions = "+cvm::to_str(atoms_positions)+"\n");
    cvm::log(cvm::line_marker);
    cvm::log("Info: done initializing the colvars proxy object.\n");
  }
}

int colvarproxy_lammps::add_config_file(const char *conf_file)
{
  return colvars->read_config_file(conf_file);
}

int colvarproxy_lammps::add_config_string(const std::string &conf)
{
  return colvars->read_config_string(conf);
}

int colvarproxy_lammps::read_state_file(char const *state_filename)
{
  input_prefix() = std::string(state_filename);
  return colvars->setup_input();
}

colvarproxy_lammps::~colvarproxy_lammps()
{
  delete _random;
}

// re-initialize data where needed
int colvarproxy_lammps::setup()
{
  my_timestep  = _lmp->update->dt * _lmp->force->femtosecond;
  return colvars->setup();
}

// trigger colvars computation
double colvarproxy_lammps::compute()
{
  if (cvm::debug()) {
    cvm::log(std::string(cvm::line_marker)+
        "colvarproxy_lammps step no. "+
        cvm::to_str(_lmp->update->ntimestep)+" [first - last = "+
        cvm::to_str(_lmp->update->beginstep)+" - "+
        cvm::to_str(_lmp->update->endstep)+"]\n");
  }

  if (first_timestep) {
    first_timestep = false;
  } else {
    // Use the time step number from LAMMPS Update object
    if (_lmp->update->ntimestep - previous_step == 1) {
      colvarmodule::it++;
      b_simulation_continuing = false;
    } else {
      // Cases covered by this condition:
      // - run 0
      // - beginning of a new run statement
      // The internal counter is not incremented, and the objects are made
      // aware of this via the following flag
      b_simulation_continuing = true;
    }

  }
  previous_step = _lmp->update->ntimestep;

  unit_cell_x.set(_lmp->domain->xprd, 0.0, 0.0);
  unit_cell_y.set(0.0, _lmp->domain->yprd, 0.0);
  unit_cell_z.set(0.0, 0.0, _lmp->domain->zprd);

  if (_lmp->domain->xperiodic == 0 && _lmp->domain->yperiodic == 0 &&
      _lmp->domain->zperiodic == 0) {
    boundaries_type = boundaries_non_periodic;
    reset_pbc_lattice();
  } else if ((_lmp->domain->nonperiodic == 0) &&
             (_lmp->domain->dimension == 3) &&
             (_lmp->domain->triclinic == 0)) {
    // Orthogonal unit cell
    boundaries_type = boundaries_pbc_ortho;
    colvarproxy_system::update_pbc_lattice();
    // It is safer to let LAMMPS deal with high-tilt triclinic boxes
  } else {
    boundaries_type = boundaries_unsupported;
  }

  if (cvm::debug()) {
    cvm::log(std::string(cvm::line_marker)+
             "colvarproxy_lammps, step no. "+cvm::to_str(colvarmodule::it)+"\n"+
             "Updating internal data.\n");
  }

  // zero the forces on the atoms, so that they can be accumulated by the colvars
  for (size_t i = 0; i < atoms_new_colvar_forces.size(); i++) {
    atoms_new_colvar_forces[i].reset();
  }

  bias_energy = 0.0;

  if (cvm::debug()) {
    cvm::log("atoms_ids = "+cvm::to_str(atoms_ids)+"\n");
    cvm::log("atoms_ncopies = "+cvm::to_str(atoms_ncopies)+"\n");
    cvm::log("atoms_positions = "+cvm::to_str(atoms_positions)+"\n");
    cvm::log("atoms_new_colvar_forces = "+cvm::to_str(atoms_new_colvar_forces)+"\n");
  }

  // Call the collective variable module
  if (colvars->calc() != COLVARS_OK) {
    cvm::error("Error in the collective variables module.\n", COLVARS_ERROR);
  }

  if (cvm::debug()) {
    cvm::log("atoms_ids = "+cvm::to_str(atoms_ids)+"\n");
    cvm::log("atoms_ncopies = "+cvm::to_str(atoms_ncopies)+"\n");
    cvm::log("atoms_positions = "+cvm::to_str(atoms_positions)+"\n");
    cvm::log("atoms_new_colvar_forces = "+cvm::to_str(atoms_new_colvar_forces)+"\n");
  }

  return bias_energy;
}

void colvarproxy_lammps::serialize_status(std::string &rst)
{
  std::ostringstream os;
  colvars->write_restart(os);
  rst = os.str();
}

// set status from string
bool colvarproxy_lammps::deserialize_status(std::string &rst)
{
  std::istringstream is;
  is.str(rst);

  if (!colvars->read_restart(is)) {
    return false;
  } else {
    return true;
  }
}


cvm::rvector colvarproxy_lammps::position_distance(cvm::atom_pos const &pos1,
                                                   cvm::atom_pos const &pos2)
  const
{
  double xtmp = pos2.x - pos1.x;
  double ytmp = pos2.y - pos1.y;
  double ztmp = pos2.z - pos1.z;
  _lmp->domain->minimum_image(xtmp,ytmp,ztmp);
  return {xtmp, ytmp, ztmp};
}


void colvarproxy_lammps::log(std::string const &message)
{
  std::istringstream is(message);
  std::string line;
  while (std::getline(is, line)) {
    if (_lmp->screen)
      fprintf(_lmp->screen,"colvars: %s\n",line.c_str());
    if (_lmp->logfile)
      fprintf(_lmp->logfile,"colvars: %s\n",line.c_str());
  }
}


void colvarproxy_lammps::error(std::string const &message)
{
  log(message);
  _lmp->error->one(FLERR,
                   "Fatal error in the collective variables module.\n");
}


int colvarproxy_lammps::set_unit_system(std::string const &units_in, bool /*check_only*/)
{
  std::string lmp_units = _lmp->update->unit_style;
  if (units_in != lmp_units) {
    cvm::error("Error: Specified unit system for Colvars \"" + units_in + "\" is incompatible with LAMMPS internal units (" + lmp_units + ").\n");
    return COLVARS_ERROR;
  }
  return COLVARS_OK;
}


// multi-replica support

int colvarproxy_lammps::replica_enabled()
{
  return (inter_comm != MPI_COMM_NULL) ? COLVARS_OK : COLVARS_NOT_IMPLEMENTED;
}


int colvarproxy_lammps::replica_index()
{
  return inter_me;
}


int colvarproxy_lammps::num_replicas()
{
  return inter_num;
}


void colvarproxy_lammps::replica_comm_barrier()
{
  MPI_Barrier(inter_comm);
}


int colvarproxy_lammps::replica_comm_recv(char* msg_data,
                                          int buf_len, int src_rep)
{
  MPI_Status status;
  int retval;

  retval = MPI_Recv(msg_data,buf_len,MPI_CHAR,src_rep,0,inter_comm,&status);
  if (retval == MPI_SUCCESS) {
    MPI_Get_count(&status, MPI_CHAR, &retval);
  } else retval = 0;
  return retval;
}


int colvarproxy_lammps::replica_comm_send(char* msg_data,
                                          int msg_len, int dest_rep)
{
  int retval;
  retval = MPI_Send(msg_data,msg_len,MPI_CHAR,dest_rep,0,inter_comm);
  if (retval == MPI_SUCCESS) {
    retval = msg_len;
  } else retval = 0;
  return retval;
}



int colvarproxy_lammps::check_atom_id(int atom_number)
{
  int const aid = atom_number;

  if (cvm::debug())
    log("Adding atom "+cvm::to_str(atom_number)+
        " for collective variables calculation.\n");

  // TODO add upper boundary check?
  if ((aid < 0)) {
    cvm::error("Error: invalid atom number specified, "+
               cvm::to_str(atom_number)+"\n", COLVARS_INPUT_ERROR);
    return COLVARS_INPUT_ERROR;
  }

  return aid;
}


int colvarproxy_lammps::init_atom(int atom_number)
{
  int aid = atom_number;

  for (size_t i = 0; i < atoms_ids.size(); i++) {
    if (atoms_ids[i] == aid) {
      // this atom id was already recorded
      atoms_ncopies[i] += 1;
      return i;
    }
  }

  aid = check_atom_id(atom_number);
  if (aid < 0) {
    return aid;
  }

  int const index = colvarproxy::add_atom_slot(aid);
  // add entries for the LAMMPS-specific fields
  atoms_types.push_back(0);

  return index;
}

