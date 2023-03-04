# Collective variables module (Colvars)

A software module for molecular simulation and analysis that provides a high-performance implementation of sampling algorithms defined on a reduced space of continuously differentiable functions (aka collective variables).

First released in 2008 as part of the standard distribution of [NAMD](https://www.ks.uiuc.edu/Research/namd/) version 2.7b1, Colvars has also been integrated in [LAMMPS](https://lammps.sandia.gov/download.html) and [VMD](https://www.ks.uiuc.edu/Research/vmd/).  In VMD, interactive use is possible both from the command line and through the [Dashboard](vmd/cv_dashboard/README.md) graphical user interface.

Patched [releases](https://github.com/Colvars/gromacs) of [GROMACS](http://www.gromacs.org/) that contain Colvars are also available ([see below](#gromacs-colvars-releases)).

The functionality provided by the Colvars library includes a variety of functions and algorithms, including free-energy estimators based on thermodynamic forces, non-equilibrium work and probability distributions.

## Obtaining and using

The easiest way to obtain pre-compiled versions of Colvars is via one of following:
- the molecular simulation program [LAMMPS](https://lammps.sandia.gov/download.html);
- the molecular simulation program [NAMD](https://www.ks.uiuc.edu/Research/namd/);
- the molecular visualization program [VMD](https://www.ks.uiuc.edu/Research/vmd/).

Please check [here](https://github.com/Colvars/colvars/wiki/List-of-Colvars-versions-included-in-simulation-and-analysis-packages) to see which version of Colvars is included with the round-number or "stable" versions of each code.

For the molecular simulation program [GROMACS](http://www.gromacs.org/), code may be compiled via our Colvars-patched [releases](#gromacs-colvars-releases).

## Documentation

The [Colvars webpage](https://colvars.github.io/) includes user documentation for the four codes, as well as a Doxygen-based [developer documentation](https://colvars.github.io/doxygen/html/).

To reflect the different availability of features in each engine, the Colvars reference manual comes in several flavors:
- [GROMACS version](https://colvars.github.io/colvars-refman-gromacs/colvars-refman-gromacs.html);
- [LAMMPS version](https://colvars.github.io/colvars-refman-lammps/colvars-refman-lammps.html);
- [NAMD version](https://colvars.github.io/colvars-refman-namd/colvars-refman-namd.html);
- [VMD version](https://colvars.github.io/colvars-refman-vmd/colvars-refman-vmd.html).

## Citing

The reference article is:
G. Fiorin, M. L. Klein, and J. Hénin, Mol. Phys. **111** (22-23), 3345-3362 (2013).
https://doi.org/10.1080/00268976.2013.813594  \[[BibTex file](https://github.com/Colvars/colvars/blob/master/doc/ref_Fiorin_2013.bib?raw=true)\] \[[Endnote file](https://github.com/Colvars/colvars/blob/master/doc/ref_Fiorin_2013.ciw?raw=true)\]

*Additional references for the implementation of specific features are listed in the [documentation](#documentation).*  Beginning with Colvars 2021-08-30, these references, including acknowledgments of specific code from the MD engine, are also printed to the screen/logfile when they are used.

*Note to NAMD users:* the NAMD reference papers (Phillips *et al*, [2005](https://doi.org/10.1002/jcc.20289) and [2020](https://doi.org/10.1063/5.0014475)) are used in some publications to acknowledge Colvars features.  This is incomplete.  When possible, please consider identifying and acknowledging all development efforts that supported your project.  As an important clarification, most of the Colvars code was developed *outside* of the NAMD/VMD funding grants.

## Example input

Colvars requires a configuration file, or alternatively configuration arguments given through scripting commands by the linked program.
- In NAMD:
```
colvars on
cv configfile <Colvars configuration file>
```
- In VMD (_Tip:_ try also the new "Colvars Dashboard" plugin):
```
cv molid top
cv configfile <Colvars configuration file>
```
- In LAMMPS:
```
fix Colvars all colvars configfile <Colvars configuration file>
```
- In GROMACS:
```
gmx mdrun -s topol.tpr -deffnm topol -colvars <Colvars configuration file>
```

The contents of the configuration file are typically the same across all programs, for example:
```
colvar { # Define a new variable
  name d # Must give a name to this variable
  width 0.2 # Estimated fluctuation amplitude and/or grid resolution, "w_d"
  distance { # This variable is a distance between centers of mass (COMs)
    group1 { atomNumbers 1 2 3 } # List the atoms of the 1st group
    group2 { atomNumbers 4 5 6 } # List the atoms of the 2nd group
  }
}

harmonic { # Define a harmonic potential, 1/2*K*(d-d0)^2/w_d^2
  colvars d # Apply it to the variable "d"
  centers 5.0 # The center of the potential, "d0"
  forceConstant 10.0 # Force constant, "K"
}
```


Complete input decks for some of the most commonly used features are available in the `examples` repository:
https://github.com/Colvars/examples

See also the [examples](https://github.com/Colvars/colvars/tree/master/examples?raw=true) folder of this repository for other examples of configurations.  Configuration options (particularly, the selections of atoms) require minimal changes to reflect the specifics of each simulation.

The [tests](https://github.com/Colvars/colvars/tree/master/tests?raw=true) folder also contains functional segments of Colvars configuration, used to build numerical tests of code accuracy and stability.  Feel free to use these segments in your production runs.

## Updating to the latest version

To recompile each program with the most recent version of the module, [download](https://github.com/Colvars/colvars/archive/master.zip) the `master` branch of this repository, or clone it via git:
```
git clone https://github.com/Colvars/colvars.git
```
and run the provided `update-colvars-code.sh` script against the unpacked source tree of any of the supported programs:
```
./update-colvars-code.sh /path/to/lammps           ; # updates LAMMPS
./update-colvars-code.sh /path/to/NAMD_X.YY_Source ; # updates NAMD
./update-colvars-code.sh /path/to/vmd-X.Y.Z        ; # updates VMD
./update-colvars-code.sh /path/to/vmd-plugins      ; # updates VMD plugins
./update-colvars-code.sh /path/to/gromacs-XXX.X    ; # update GROMACS
```
and recompile them.

The `update-colvars-code.sh` script support patching the latest development version of each program:
- the [LAMMPS GitHub repository](https://github.com/lammps/lammps);
- the [NAMD GitLab repository](https://gitlab.com/tcbgUIUC/namd);
- the [CVS repositories of VMD and its plugins](https://www.ks.uiuc.edu/Research/vmd/doxygen/cvsget.html).

For [GROMACS](http://www.gromacs.org/), support for specific release series is currently maintained; pre-patched versions of specific releases are provided [below](#gromacs-colvars-releases).

All of the above MD engine versions are automatically tested as part of GitHub Actions [workflows](https://github.com/Colvars/colvars/actions?query=branch%3Amaster).

## GROMACS-Colvars releases

The following links provide specific versions of GROMACS that are already patched with the latest available version of Colvars:

 - **GROMACS [2021.6-colvars](https://github.com/Colvars/gromacs/releases/tag/v2021.6-colvars)**;

 - **GROMACS [2020.7-colvars](https://github.com/Colvars/gromacs/releases/tag/v2020.7-colvars)**.

Other patched releases within the 2020.x or 2021.x series may be produced using the same [patching procedure](#updating-to-the-latest-version) described above.  Unless there is a specific reason not to, it is a good idea to use the latest revision of each release series, available as [Git branches](https://github.com/Colvars/gromacs/branches).  Gromacs versions outside of 2020.x or 2021.x are not supported at this time.

When posting a message to the [Gromacs forum](https://gromacs.bioexcel.eu/) related to any Colvars-patched version of GROMACS please always specify "GROMACS modification: **Yes**" and use the [`colvars` tag](https://gromacs.bioexcel.eu/tag/colvars).  Whenver possible please also reference the Colvars version: this is included in the version string that is reported by the patched GROMACS (right after the GROMACS version), for example:
```
   :-) GROMACS - gmx_d, 2021.5-Colvars-2022-02-20-dev (double precision) (-:
```

## Which version of Colvars is recommended?

The Git `master` branch is to be considered the "*stable*" release at any given time; any bugfixes are released through `master` first.  The input syntax is near-completely *backward-compatible* and output files are *forward-compatible*.  Feel free to download Colvars and update NAMD, VMD, LAMMPS or GROMACS as needed.

Other branches are dedicated to the development of specific features: please use them at your own discretion.

## Which version of Colvars is included in package XX version YY?

The specific version of Colvars is identified both in the code and in the documentation by the date of the most recent code revision (e.g. `2021-01-19`). 
This date is printed to the standard output or log file as soon as Colvars is activated.

A table mapping software package release versions to Colvars versions is given [here](https://github.com/Colvars/colvars/wiki/List-of-Colvars-versions-included-in-simulation-and-analysis-packages).


If you are using a stable release of any of the codes mentioned above, feel free to use the version number of that code when asking questions.

## Feedback

Please use the "[Issues](https://github.com/Colvars/colvars/issues)" tab of this page to submit new bug reports or to suggest new features.

## License

This software is distributed under the GNU Lesser General Public License (LGPL), version 3.  See COPYING.LESSER for complete licensing terms.

In the interest of broad distribution, copies of this code are also distributed together with LAMMPS (LGPL version 2), NAMD and VMD (UIUC license).  However, the terms of the LGPLv3 license still apply to code originating from this repository.

##########

Defining a single collective variable (CV) in VMD can be done using the "colvars" module within VMD. Here's a step-by-step tutorial on how to define a CV based on the distance between two atoms:

Load your molecular structure into VMD.

Open the Tk Console in VMD by going to "Extensions" -> "Tk Console".

Load the colvars module by typing the following command in the Tk Console:

lua
Copy code
```
package require colvars
```
Define the two atoms that you want to use to calculate the distance by selecting them in the VMD graphics window and using the following command in the Tk Console:
python
Copy code
```
set atom1 [atomselect top "index X"]
set atom2 [atomselect top "index Y"]
```
where X and Y are the indices of the two atoms you selected.

Define the CV based on the distance between the two atoms by using the following command:
bash
Copy code
```
colvar distance
distance {
 name distance
 atoms { $atom1 $atom2 }
}
```
Write out the configuration file by using the following command:
Copy code
```
colvarsConfig distance.colvars
```
The configuration file "distance.colvars" can now be used in your molecular dynamics simulations with NAMD or other simulation software that supports the colvars module.
Note: This is a very basic example of how to define a single CV in VMD. The colvars module has many other features and options that you can use to define more complex CVs based on various molecular properties such as angles, dihedrals, and other distances. You can find more information about the colvars module in the VMD User's Guide.

### A complete namd script for alchemical pathway simulation

```
# Load the required NAMD modules
package require colvars

# Define the name of the input PDB and PSF files
set pdb_file "complex.pdb"
set psf_file "complex.psf"

# Define the name of the output DCD file
set dcd_file "complex_alchemical.dcd"

# Load the PDB and PSF files
structure $pdb_file
coordpdb $pdb_file

# Define the parameters for the distance collective variable (CV)
colvar {
  name distance
  distance {
    group1 {
      atomNumbers { # List of atom numbers of the first group
        ...
      }
    }
    group2 {
      atomNumbers { # List of atom numbers of the second group
        ...
      }
    }
  }
}

# Define the parameters for the alchemical calculation
alchemical_lambda {
  lambda_schedule
  {
    lambdaWindow
    {
      windowStart 0.0
      windowEnd 0.1
      transitionStart 0.05
      transitionEnd 0.06
    }
    lambdaWindow
    {
      windowStart 0.1
      windowEnd 0.2
      transitionStart 0.15
      transitionEnd 0.16
    }
    ...
  }
  lambdaParmfile
  {
    parmfile $psf_file
  }
  lambdaScript
  {
    energyScript energy.in
  }
  lambdaRestart
  {
    restartfile restart.alch
  }
}

# Define the simulation parameters
set temperature 300.0
set numsteps 10000

# Start the simulation
binary output $dcd_file
run $numsteps

```
