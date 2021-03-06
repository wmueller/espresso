/*
  Copyright (C) 2010 The ESPResSo project
  Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2009,2010 Max-Planck-Institute for Polymer Research, Theory Group, PO Box 3148, 55021 Mainz, Germany
  
  This file is part of ESPResSo.
  
  ESPResSo is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  ESPResSo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>. 
*/
#ifndef CELLS_H
#define CELLS_H
/** \file cells.h
    This file contains everything related to the cell structure / cell
    system.
    
    The cell system (\ref Cell Structure) describes how particles are
    distributed on the cells and how particles of different cells
    (regardless if they reside on the same or different nodes)
    interact with each other. The following cell systems are implemented:
  
    <ul>
    <li> domain decomposition: The simulation box is divided spatially
    ino cells (see \ref domain_decomposition.h). This is suitable for
    short range interctions.
    <li> nsquare: The particles are distributed equally on all nodes
    regardless their spatial position (see \ref nsquare.h). This is
    suitable for long range interactions that can not be treated by a
    special method like P3M (see \ref p3m.h).
    <li> layered: in x and y directions, it uses a nsquared type of interaction calculation,
                  but in z it has a domain decomposition into layers.
    </ul>
  
    One can switch between different cell systems with the tcl command
    \ref tcl_cellsystem implemented in \ref cells::cellsystem.
  
    Some structures are common to all cell systems: 
  
   <ul>
   <li> All cells, real cells as well as ghost cells, are stored in the array \ref cells::cells with size \ref
   n_cells. The size of this array has to be changed with \ref realloc_cells.
   <li> Their are two lists of cell pointers to acces particles and
   ghost particles on a node: \ref local_cells contains pointers to
   all cells containing the particles physically residing on that
   node. \ref ghost_cells contains pointers to all cells containing
   the ghost particles of that node. The size of these lists has to be
   changed with \ref realloc_cellplist
   <li> An example using the cell pointer lists to access particle data
   can be found in the function \ref
   print_local_particle_positions. DO NOT INVENT YOUR OWN WAY!!!
   </ul>
*/

#include <tcl.h>
#include "particle_data.h"
#include "ghosts.h"
#include "verlet.h"

/** \name Cell Structure */
/** Flag telling which cell structure is used at the moment. */
/*@{*/
/** Flag indicating that the current cell structure will be used furthor on */
#define CELL_STRUCTURE_CURRENT 0
/** cell structure domain decomposition */
#define CELL_STRUCTURE_DOMDEC  1
/** cell structure n square */
#define CELL_STRUCTURE_NSQUARE 2
/** cell structure layered */
#define CELL_STRUCTURE_LAYERED 3
/*@}*/

/** \name Flags for exchange_and_sort_particles: wether to do a global exchange
    or assume that particles did not move much (faster, used during integration,
    where moving far is a catastrophe anyways). */
/*@{*/

/** Flag for exchange_and_sort_particles : Do global exchange. */
#define CELL_GLOBAL_EXCHANGE 1
/** Flag for exchange_and_sort_particles : Do neighbor exchange. */
#define CELL_NEIGHBOR_EXCHANGE 0

/*@}*/


/************************************************/
/** \name Data Types */
/************************************************/
/*@{*/

/** A cell is a \ref ParticleList representing a particle group with
    respect to the integration algorithm.
*/
typedef ParticleList Cell;

/** List of cell pointers. */
typedef struct {
  Cell **cell;
  int n;
  int max;
} CellPList;

/** Describes a cell structure / cell system. Contains information
    about the communication of cell contents (particles, ghosts, ...) 
    between different nodes and the relation between particle
    positions and the cell system. All other properties of the cell
    system which are not common between different cell systems have to
    be stored in seperate structures. */
typedef struct {
  /** type descriptor */
  int type;

  /** Communicator to exchange ghost cell information. */
  GhostCommunicator ghost_cells_comm;
  /** Communicator to exchange ghost particles. */
  GhostCommunicator exchange_ghosts_comm;
  /** Communicator to update ghost positions. */
  GhostCommunicator update_ghost_pos_comm;
  /** Communicator to collect ghost forces. */
  GhostCommunicator collect_ghost_force_comm;
#ifdef LB
  /** Communicator for particle data used by lattice Boltzmann */
  GhostCommunicator ghost_lbcoupling_comm;
#endif

  /** Cell system dependent function to find the right node for a
      particle at position pos. 
      \param  pos Position of a particle.
      \return number of the node where to put the particle. 
  */
  int   (*position_to_node)(double pos[3]);
  /** Cell system dependent function to find the right cell for a
      particle at position pos. 
      \param  pos Position of a particle.
      \return pointer to cell  where to put the particle.
  */
  Cell *(*position_to_cell)(double pos[3]);
} CellStructure;

/*@}*/

/************************************************************/
/** \name Exported Variables */
/************************************************************/
/*@{*/

/** list of all cells. */
extern Cell *cells;
/** size of \ref cells::cells */
extern int n_cells;
/** list of all cells containing particles physically on the local
    node */
extern CellPList local_cells;
/** list of all cells containing ghosts */
extern CellPList ghost_cells;

/** Type of cell structure in use ( \ref Cell Structure ). */
extern CellStructure cell_structure;

/*@}*/

/************************************************************/
/** \name Exported Functions */
/************************************************************/
/*@{*/

/** implementation of the Tcl command \ref tcl_cellsystem */
int tclcommand_cellsystem(ClientData data, Tcl_Interp *interp,
	       int argc, char **argv);

/** Initialize the cell structure on program start with the default
    cell structure of type domain decomposition. */
void cells_pre_init();

/** Reinitialize the cell structures.
    @param new_cs gives the new topology to use afterwards. May be set to
    \ref CELL_STRUCTURE_CURRENT for not changing it.
*/
void cells_re_init(int new_cs);

/** Reallocate the list of all cells (\ref cells::cells). */
void realloc_cells(int size);

/** Initialize a list of cell pointers */
MDINLINE void init_cellplist(CellPList *cpl) {
  cpl->n    = 0;
  cpl->max  = 0;
  cpl->cell = NULL;
}

/** Reallocate a list of cell pointers */
MDINLINE void realloc_cellplist(CellPList *cpl, int size)
{
  if(size != cpl->max) {
    cpl->max = size;
    cpl->cell = (Cell **) realloc(cpl->cell, sizeof(Cell *)*cpl->max);
  }
}

/** sort the particles into the cells and initialize the ghost particle structures.
    @param global_flag if this is CELLS_GLOBAL_EXCHANGE, particle positions can have changed
    arbitrarly, otherwise the change should have been smaller then skin.  */
void cells_resort_particles(int global_flag);

/** update ghost information. If \ref rebuild_verletlist == 1, for some cell structures also a
    resorting of the particles takes place. */
void cells_update_ghosts();

/** Calculate and return the total number of particles on this
    node. */
int cells_get_n_particles();

/** Debug function to print particle positions. */
void print_local_particle_positions();

/** Debug function to print ghost positions. */
void print_ghost_positions();
/*@}*/

#endif
