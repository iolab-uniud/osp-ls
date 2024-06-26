#pragma once

#include <future>

#include "easylocal/helpers/SolutionManager.hh"
#include "easylocal/helpers/outputmanager.hh"
#include "easylocal/solvers/LocalSearch.hh"
#include "easylocal/runners/runner.hh"

namespace EasyLocal
{

namespace Core
{

/** The Simple Local Search solver handles a simple local search algorithm
     encapsulated in a runner.
     @ingroup Solvers
     */
template <class Input, class Solution, class CostStructure = DefaultCostStructure<int>>
class MultiStartSearch
    : public LocalSearch<Input, Solution, CostStructure>
{
public:
  typedef Runner<Input, Solution, CostStructure> RunnerType;
  MultiStartSearch(const Input &in,
                   SolutionManager<Input, Solution, CostStructure> &e_sm,
                   OutputManager<Input, Output, State> &e_om,
                   std::string name);
  void AddRunner(RunnerType &r);
  void RemoveRunner(const RunnerType &r);
  void Print(std::ostream &os = std::cout) const;
  void ReadParameters(std::istream &is = std::cin, std::ostream &os = std::cout);
  unsigned int Restart() const { return restart; }
  unsigned int IdleRestarts() const { return idle_restarts; }

protected:
  void InitializeSolve() throw(ParameterNotSet, IncorrectParameterValue);
  void Go();
  void AtTimeoutExpired();
  virtual std::shared_ptr<Solution> GetCurrentState() const;

  std::vector<RunnerType *> p_runners; /**< pointers to the managed runner. */
  unsigned int current_runner;
  Parameter<unsigned int> max_restarts, max_idle_restarts;
  unsigned int restart;
  unsigned int idle_restarts;
};

/*************************************************************************
     * Implementation
     *************************************************************************/

/**
     Constructs a Multi Start solver by providing it links to
     a state manager, an output manager, a runner, an input,
     and an output object.
     
     @param sm a pointer to a compatible state manager
     @param om a pointer to a compatible output manager
     @param r a pointer to a compatible runner
     @param in a pointer to an input object
     @param out a pointer to an output object
     */
template <class Input, class Solution, class CostStructure>
MultiStartSearch<Input, Solution, CostStructure>::MultiStartSearch(const Input &in,
                                                                        SolutionManager<Input, Solution, CostStructure> &e_sm,
                                                                        OutputManager<Input, Output, State> &e_om,
                                                                        std::string name)
    : LocalSearch<Input, Solution, CostStructure>(in, e_sm, e_om, name, "Multi Start Solver")
{
    max_restarts("max_restarts", "Maximum number of restarts", this->parameters);
    max_idle_restarts("max_idle_restarts", "Maximum number of idle restarts", this->parameters);
    restart = 0;
    idle_restarts = 0;
}


template <class Input, class Solution, class CostStructure>
void MultiStartSearch<Input, Solution, CostStructure>::ReadParameters(std::istream &is, std::ostream &os)
{
  os << "Multi Start Solver: " << this->name << " parameters" << std::endl;
  unsigned int i = 0;
  for (RunnerType *p_r : p_runners)
  {
    os << "Runner [" << i++ << "]: " << std::endl;
    p_r->ReadParameters(is, os);
  }
}

template <class Input, class Solution, class CostStructure>
void MultiStartSearch<Input, Solution, CostStructure>::Print(std::ostream &os) const
{
  os << "Multi Start Solver: " << this->name << std::endl;
  unsigned int i = 0;
  for (const RunnerType *p_r : p_runners)
  {
    os << "Runner [" << i++ << "]: " << std::endl;
    p_r->Print(os);
  }
  if (p_runners.size() == 0)
    os << "<no runner attached>" << std::endl;
}

/**
     Adds to the runner list employed for solving the problem to the one passed as
     parameter.
     
     @param r the new runner to be used
     */
template <class Input, class Solution, class CostStructure>
void MultiStartSearch<Input, Solution, CostStructure>::AddRunner(RunnerType &r)
{
  p_runners.push_back(&r);
}

/**
     Removes from the runner list employed for solving the problem the one passed as
     parameter.
     
     @param r the runner to remove
     */
template <class Input, class Solution, class CostStructure>
void MultiStartSearch<Input, Solution, CostStructure>::RemoveRunner(const RunnerType &r)
{
  typename std::vector<RunnerType *>::const_iterator it;
  for (it = p_runners.begin(); it != p_runners.end(); ++it)
  {
    if (*it == &r)
      break;
  }
  if (it == p_runners.end())
    throw std::logic_error("Runner " + r->name + " was not added to the Multi Start Search");
  p_runners.erase(it);
}

template <class Input, class Solution, class CostStructure>
void MultiStartSearch<Input, Solution, CostStructure>::InitializeSolve() throw(ParameterNotSet, IncorrectParameterValue)
{
  LocalSearch<Input, Solution, CostStructure>::InitializeSolve();
  if (max_idle_restarts.IsSet() && max_idle_restarts == 0)
    throw IncorrectParameterValue(max_idle_restarts, "It should be greater than zero");
  if (max_restarts.IsSet() && max_restarts == 0)
    throw IncorrectParameterValue(max_restarts, "It should be greater than zero");
  if (p_runners.size() == 0)
    // FIXME: add a more specific exception behavior
    throw std::logic_error("No runner set in object " + this->name);
  restart = 0;
  idle_restarts = 0;
}

template <class Input, class Solution, class CostStructure>
void MultiStartSearch<Input, Solution, CostStructure>::Go()
{
  current_runner = 0;
  bool idle = true;
  do
  {
    //      std::cerr << "restarts = " << restarts << "/" << max_restarts << ", " << idle_restarts << "/" << max_idle_restarts<< std::endl;
    this->current_state_cost = p_runners[current_runner]->Go(*this->p_current_state);
    if (this->current_state_cost <= this->best_state_cost) // the less or equal is here for diversification purpose
    {
      if (this->current_state_cost < this->best_state_cost)
        idle = false;
      *this->p_best_state = *this->p_current_state;
      this->best_state_cost = this->current_state_cost;
    }
    current_runner = (current_runner + 1) % p_runners.size();
    if (current_runner == 0)
    {
      restart++;
      if (idle)
        idle_restarts++;
      else
        idle_restarts = 0;
      this->sm.RandomState(*this->p_current_state);
      this->current_state_cost = this->sm.CostFunctionComponents(*this->p_current_state);
    }
  } while (idle_restarts < max_idle_restarts && restart < max_restarts);
}

template <class Input, class Solution, class CostStructure>
void MultiStartSearch<Input, Solution, CostStructure>::AtTimeoutExpired()
{
  p_runners[current_runner]->Interrupt();
}

template <class Input, class Solution, class CostStructure>
void MultiStartSearch<Input, Solution, CostStructure>::GetCurrentState() const
{
  return p_runners[current_runner]->GetCurrentBestState();
}
} // namespace Core
} // namespace EasyLocal
