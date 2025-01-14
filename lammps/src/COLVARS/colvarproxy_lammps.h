// -*- c++ -*-

// This file is part of the Collective Variables module (Colvars).
// The original version of Colvars and its updates are located at:
// https://github.com/Colvars/colvars
// Please update all Colvars source files before making any changes.
// If you wish to distribute your changes, please submit them to the
// Colvars repository at GitHub.

#ifndef COLVARPROXY_LAMMPS_H
#define COLVARPROXY_LAMMPS_H

#include "colvarproxy_lammps_version.h"    // IWYU pragma: export

#include "colvarmodule.h"
#include "colvarproxy.h"
#include "colvartypes.h"

#include "domain.h"    // IWYU pragma: keep
#include "force.h"     // IWYU pragma: keep
#include "lammps.h"    // IWYU pragma: keep
#include "random_park.h"
#include "update.h"    // IWYU pragma: keep

/// \brief Communication between colvars and LAMMPS
/// (implementation of \link colvarproxy \endlink)
class colvarproxy_lammps : public colvarproxy {

  // LAMMPS specific data objects and flags
 protected:
  // pointers to LAMMPS class instances
  LAMMPS_NS::LAMMPS *_lmp;
  LAMMPS_NS::RanPark *_random;

  // state of LAMMPS properties
  double my_timestep, my_angstrom;
  double bias_energy;
  int previous_step;

  bool first_timestep;
  bool do_exit;

  std::vector<int> atoms_types;

  MPI_Comm inter_comm;        // MPI comm with 1 root proc from each world
  int inter_me, inter_num;    // rank for the inter replica comm

 public:
  friend class cvm::atom;
  colvarproxy_lammps(LAMMPS_NS::LAMMPS *lmp, const char *, const char *, const int, const double,
                     MPI_Comm);
  ~colvarproxy_lammps() override;
  void init(const char *);
  int setup() override;

  // disable default and copy constructor
 private:
  colvarproxy_lammps(){};
  colvarproxy_lammps(const colvarproxy_lammps &){};

  // methods for lammps to move data or trigger actions in the proxy
 public:

  bool total_forces_enabled() const override { return total_force_requested; };
  bool total_forces_same_step() const override { return true; };
  bool want_exit() const { return do_exit; };

  // perform colvars computation. returns biasing energy
  double compute();

  // dump status to string
  void serialize_status(std::string &);

  // set status from string
  bool deserialize_status(std::string &);

  // read additional config from file
  int add_config_file(char const *config_filename);

  // read additional config from string
  int add_config_string(const std::string &config);

  // load a state file
  int read_state_file(char const *state_filename);

  // Request to set the units used internally by Colvars
  int set_unit_system(std::string const &units_in, bool check_only) override;

  inline cvm::real dt() override
  {
    return my_timestep;
  };    // return _lmp->update->dt * _lmp->force->femtosecond; };

  void add_energy(cvm::real energy) override { bias_energy += energy; };
  void request_total_force(bool yesno) override { total_force_requested = yesno; };

  void log(std::string const &message) override;
  void error(std::string const &message) override;

  cvm::rvector position_distance(cvm::atom_pos const &pos1, cvm::atom_pos const &pos2) const override;

  cvm::real rand_gaussian(void) override { return _random->gaussian(); };

  int init_atom(int atom_number) override;
  int check_atom_id(int atom_number) override;

  inline std::vector<int> *modify_atom_types() { return &atoms_types; }

  int replica_enabled() override;
  int replica_index() override;
  int num_replicas() override;

  void replica_comm_barrier() override;
  int replica_comm_recv(char *msg_data, int buf_len, int src_rep) override;
  int replica_comm_send(char *msg_data, int msg_len, int dest_rep) override;
};

#endif
