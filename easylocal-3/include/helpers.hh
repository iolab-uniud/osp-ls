#pragma once

/**
 @defgroup Helpers Helpers
 
 @brief Helpers perform actions related to some specific aspects of the search and have to be
 partially implemented by the user.
 
 EasyLocalpp defines a number of helper classes that are not related
 hierarchically, but they are linked to @ref Runners, @ref Solvers, @ref Testers
 and to each other through references. The helpers are the following ones:
 
 - SolutionManager: is responsible for all operations on the state that are
 independent of the neighborhood definition (i.e., from the @ref Move "Moves").
 
 - CostComponent: is responsible for computing a component of the cost function
 on a given @ref State. The class is able to handle both @e hard and @e soft cost components.
 
 
 - NeighborhoodExplorer: handles all the features concerning neighborhood exploration in an @e iterator
 fashion.
 
 - DeltaCostComponent: is responsible for computing the difference of the cost function due to the application
 of a @ref Move on a given @ref State.
 
 - ShiftingPenaltyManager: is responsible for the adaptive modification of the weights of the
 cost function, according to the @e shifting @e penalty mechanism. For each component of the
 cost function, it maintains an independent weight, which varies depending on the number of
 violations and according to a customizable scheme.
 */

#include "helpers/costcomponent.hh"
#include "helpers/deltacostcomponent.hh"
#include "helpers/kicker.hh"
#include "helpers/neighborhoodexplorer.hh"
#include "helpers/solutionmanager.hh"

