#include "OSP_helpers.hh"

#include <chrono>
#include <string>
#include <cmath>

using namespace std::chrono;
using namespace EasyLocal::Debug;

double round_to(double value, double precision = 0.01)
{
    return std::round(value / precision) * precision;
}

int main(int argc, const char* argv[])
{
#if !defined(NDEBUG)
    std::cout << "This code is running in DEBUG mode" << std::endl;
#endif

    // define the parameters of the main program
    ParameterBox main_parameters("main", "Main Program options");

    Parameter<std::string> instance("instance", "Input instance", main_parameters);
    Parameter<unsigned int> seed("seed", "Random seed", main_parameters); 
    Parameter<unsigned int> solution_method("solution_method", "Solution method could be 1: heuristic, 2: local search, 3: random", main_parameters);
    Parameter<std::string> output_file("output_file", "Name of the output file, otherwise the output is only printed", main_parameters);
    
    ParameterBox tuning_parameters("tuning", "Tuning options");
    Parameter<bool> irace("irace", "Irace version, means that the output (only the cost) will be printed", tuning_parameters);

    ParameterBox metaheuristic_parameters("metaheuristic", "Metaheuristic options");
    Parameter<unsigned int> initial_solution("initial_solution", "Type of initial solution you want (1: heuristic, 2: random)", metaheuristic_parameters);
    Parameter<std::string> method("method", "Type of metaheuristics methods you want", metaheuristic_parameters);
    
    Parameter<double> swap_rate("swap_rate", "Rate for the swap move", metaheuristic_parameters);
    Parameter<double> insert_rate("insert_rate", "Rate for the insertion move", metaheuristic_parameters); 
    Parameter<double> inverse_rate("inverse_rate", "Rate for the inverse move", metaheuristic_parameters);
    Parameter<double> single_job_to_new_batch_rate("single_job_to_new_batch_rate", "Rate for the insertion of just one job in a new batch", metaheuristic_parameters);
    Parameter<double> more_jobs_to_new_batch_rate("more_jobs_to_new_batch_rate", "Rate for the insertion of more than one job in a new batch", metaheuristic_parameters); 
    Parameter<double> job_to_existing_batch_rate("job_to_existing_batch_rate", "Rate for the insertion of one job in a new batch", metaheuristic_parameters);

    seed = 42; 
    irace = false; 
    solution_method = 100;

    // parse the command line parameters
    CommandLineParameters::Parse(argc, argv, false, true);

    if (!instance.IsSet())
    {
        std::cout << "Error: --main::instance filename option must always be set" << std::endl;
        return 1;
    }  
    if (!solution_method.IsSet() || solution_method > 3)
    {
        std::cout << "Error: --main::solution_method solution method option must always be set and should be one of the allowed options" << std::endl;
        return 1;
    }
    if (seed.IsSet())
    {
        Random::SetSeed(seed);
    }
    
    OSP_Input in(instance);

    // if the solution method is 1 or 3, this means you don't want to run one of the two greedy algorithms
    if (solution_method == 1)
    {
        OSP_SolutionManager OSP_sm(in);
        OSP_Output st(in);
        auto start = high_resolution_clock::now();
        OSP_sm.GreedyState(st);
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<seconds>(stop - start);
        OSP_sm.CheckConsistency(st);
        int cost = 
            st.GetNumberOfTardyJobs() * st.MultFactorFinishedTooLate() + 
            st.GetCumulativeBatchProcessingTime() * st.MultFactorTotalRunTime() + 
            st.GetTotalSetUpCost() * st.MultFactorTotalSetUpCosts();
        if (output_file.IsSet())
        {
            std::ofstream os(static_cast<std::string>(output_file).c_str());
            os << "{\"solution\": {" << st <<  "}, "
            << "\"total_cost\": " <<  cost  <<  ", "
            << "\"time_seconds\": " << duration.count() << "} " << std::endl;
            os.flush();
            os.close();
        }
        else
        {
            std::cout << "{\"solution\": {" << st <<  "}, "
            << "\"total_cost\": " <<  cost  <<  ", "
            << "\"time_seconds\": " << duration.count() << "} " << std::endl;
        }
#if !defined(NDEBUG)
        std::cout << "This code is running in DEBUG mode" << std::endl;
#endif
        return 0;
    }
    else if (solution_method == 3)
    {
        OSP_SolutionManagerRandom OSP_sm(in);
        OSP_Output st(in);
        auto start = high_resolution_clock::now();
        OSP_sm.GreedyState(st);
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<seconds>(stop - start);
        OSP_sm.CheckConsistency(st);
        int cost = 
            st.GetNumberOfTardyJobs() * st.MultFactorFinishedTooLate() + 
            st.GetCumulativeBatchProcessingTime() * st.MultFactorTotalRunTime() + 
            st.GetTotalSetUpCost() * st.MultFactorTotalSetUpCosts();
        if (output_file.IsSet())
        {
            std::ofstream os(static_cast<std::string>(output_file).c_str());
            os << "{\"solution\": {" << st <<  "}, "
            << "\"total_cost\": " <<  cost  <<  ", "
            << "\"time_seconds\": " << duration.count() << "} " << std::endl;
            os.flush();
            os.close();
        }
        else
        {
            std::cout << "{\"solution\": {" << st <<  "}, "
            << "\"total_cost\": " <<  cost  <<  ", "
            << "\"time_seconds\": " << duration.count() << "} " << std::endl;
        }
#if !defined(NDEBUG)
        std::cout << "This code is running in DEBUG mode" << std::endl;
#endif
        return 0;
    }

    // if you reach this point, it means you need to run a local search method
    if (!initial_solution.IsSet() || (initial_solution!= 1 && initial_solution != 2))
    {
        std::cout << "Error: --metaheuristic::initial_solution initial_solution option must always be set when you select the local search solution method or you should look at the values" << std::endl;
        return 1;
    }
    if (!method.IsSet())
    {
        std::cout << "Error: --metaheuristic::method method option must always be set when you select the local search solution method" << std::endl;
        return 1;
    }

    
    if (method == std::string("SA_all") && 
        (!swap_rate.IsSet() || 
        !insert_rate.IsSet() || 
        !inverse_rate.IsSet() || 
        !single_job_to_new_batch_rate.IsSet() || 
        !more_jobs_to_new_batch_rate.IsSet() || 
        !job_to_existing_batch_rate.IsSet()))
    {
        std::cout << "Error: missing one of the neighborhoods rate" << std::endl;
        return 1;
    }

    if (method == std::string("SA_noSwap") && 
        (swap_rate.IsSet() || 
        !insert_rate.IsSet() ||
        !inverse_rate.IsSet() ||
        !single_job_to_new_batch_rate.IsSet() ||
        !more_jobs_to_new_batch_rate.IsSet() ||
        !job_to_existing_batch_rate.IsSet()))
    {
        std::cout << "Error: missing one of the neighborhoods rate or a wrong one is set" << std::endl;
        return 1;
    }
    else if (method == std::string("SA_noSwap"))
    {
        swap_rate = 0.0;
    }

    if (method == std::string("SA_noInsert") && 
        (!swap_rate.IsSet() || 
        insert_rate.IsSet() ||
        !inverse_rate.IsSet() ||
        !single_job_to_new_batch_rate.IsSet() ||
        !more_jobs_to_new_batch_rate.IsSet() ||
        !job_to_existing_batch_rate.IsSet()))
    {
        std::cout << "Error: missing one of the neighborhoods rate or a wrong one is set" << std::endl;
        return 1;
    }
    else if (method == std::string("SA_noInsert"))
    {
        insert_rate = 0.0;
    } 
    
    if (method == std::string("SA_noInverse") && 
        (!swap_rate.IsSet() || 
        !insert_rate.IsSet() ||
        inverse_rate.IsSet() ||
        !single_job_to_new_batch_rate.IsSet() ||
        !more_jobs_to_new_batch_rate.IsSet() ||
        !job_to_existing_batch_rate.IsSet()))
    {
        std::cout << "Error: missing one of the neighborhoods rate or a wrong one is set" << std::endl;
        return 1;
    }
    else if (method == std::string("SA_noInverse"))
    {
        inverse_rate = 0.0;
    }

    if (method == std::string("SA_noSingleNewBatch") && 
        (!swap_rate.IsSet() || 
        !insert_rate.IsSet() ||
        !inverse_rate.IsSet() ||
        single_job_to_new_batch_rate.IsSet() ||
        !more_jobs_to_new_batch_rate.IsSet() ||
        !job_to_existing_batch_rate.IsSet()))
    {
        std::cout << "Error: missing one of the neighborhoods rate or a wrong one is set" << std::endl;
        return 1;
    }
    else if (method == std::string("SA_noSingleNewBatch"))
    {
        single_job_to_new_batch_rate = 0.0;
    }

    if (method == std::string("SA_noMoreNewBatch") && 
        (!swap_rate.IsSet() || 
        !insert_rate.IsSet() ||
        !inverse_rate.IsSet() ||
        !single_job_to_new_batch_rate.IsSet() ||
        more_jobs_to_new_batch_rate.IsSet() ||
        !job_to_existing_batch_rate.IsSet()))
    {
        std::cout << "Error: missing one of the neighborhoods rate or a wrong one is set" << std::endl;
        return 1;
    }
    else if (method == std::string("SA_noMoreNewBatch"))
    {
        more_jobs_to_new_batch_rate = 0.0;
    }

    if (method == std::string("SA_noExistingBatch") && 
        (!swap_rate.IsSet() || 
        !insert_rate.IsSet() ||
        !inverse_rate.IsSet() ||
        !single_job_to_new_batch_rate.IsSet() ||
        !more_jobs_to_new_batch_rate.IsSet() ||
        job_to_existing_batch_rate.IsSet()))
    {
        std::cout << "Error: missing one of the neighborhoods rate or a wrong one is set" << std::endl;
        return 1;
    }
    else if (method == std::string("SA_noExistingBatch"))
    {
        job_to_existing_batch_rate = 0.0;
    }

    // normalization    
    double total = swap_rate + insert_rate + inverse_rate + single_job_to_new_batch_rate + more_jobs_to_new_batch_rate + job_to_existing_batch_rate; 
    swap_rate = round_to(swap_rate / total);
    insert_rate = round_to(insert_rate / total); 
    inverse_rate = round_to(inverse_rate / total);
    single_job_to_new_batch_rate = round_to(single_job_to_new_batch_rate / total);
    more_jobs_to_new_batch_rate = round_to(more_jobs_to_new_batch_rate / total);
    job_to_existing_batch_rate = round_to(job_to_existing_batch_rate / total);

    // std::cout << swap_rate << "--" << 
    //  insert_rate << "--" <<
    //  inverse_rate << "--" << 
    //  single_job_to_new_batch_rate << "--" << 
    //  more_jobs_to_new_batch_rate << "--" << 
    //  job_to_existing_batch_rate << ";" << std::endl; 

    // cost components: 
    // second parameter is the cost, third is the type (true -> hard, false -> soft)
    OSP_TotalSetUpCost cc1(in, in.MultFactorTotalSetUpCosts(), false);
    OSP_NumberOfTardyJobs cc2(in, in.MultFactorFinishedTooLate(), false);
    OSP_CumulativeBatchProcessingTime cc3(in, in.MultFactorTotalRunTime(), false);
    OSP_NotScheduledBatches cc4(in, 2 * in.UpperBoundIntegerObjective(), true);

    if (initial_solution == 1)
    {
        // solution manager:
        OSP_SolutionManager OSP_sm(in);

        // attach cost to solution manager
        OSP_sm.AddCostComponent(cc1);
        OSP_sm.AddCostComponent(cc2);
        OSP_sm.AddCostComponent(cc3);
        OSP_sm.AddCostComponent(cc4);

        // neighborhood
        OSP_SwapBatchesNeighborhoodExplorer SwapNeighb(in,OSP_sm);
        OSP_BatchToNewPositionNeighborhoodExplorer InsertNeighb(in,OSP_sm);
        OSP_InvertBatchesInMachineNeighborhoodExplorer InverseNeighb(in,OSP_sm);
        OSP_JobToNewBatchNeighborhoodExplorer SingleNewBatch(in,OSP_sm);
        Decoupled_OSP_BatchToNewMachineNeighborhoodExplorer MoreNewBatchNeighb(in,OSP_sm);
        OSP_JobToExistingBatchNeighborhoodExplorer JobExistingBarchNeighb(in,OSP_sm);    
       
        // attach cost to neighborhoods
        SwapNeighb.AddCostComponent(cc1);
        SwapNeighb.AddCostComponent(cc2);
        SwapNeighb.AddCostComponent(cc3);
        SwapNeighb.AddCostComponent(cc4);

        InsertNeighb.AddCostComponent(cc1);
        InsertNeighb.AddCostComponent(cc2);
        InsertNeighb.AddCostComponent(cc3);
        InsertNeighb.AddCostComponent(cc4);

        InverseNeighb.AddCostComponent(cc1);
        InverseNeighb.AddCostComponent(cc2);
        InverseNeighb.AddCostComponent(cc3);
        InverseNeighb.AddCostComponent(cc4);

        SingleNewBatch.AddCostComponent(cc1);
        SingleNewBatch.AddCostComponent(cc2);
        SingleNewBatch.AddCostComponent(cc3);
        SingleNewBatch.AddCostComponent(cc4);

        MoreNewBatchNeighb.AddCostComponent(cc1);
        MoreNewBatchNeighb.AddCostComponent(cc2);
        MoreNewBatchNeighb.AddCostComponent(cc3);
        MoreNewBatchNeighb.AddCostComponent(cc4);

        JobExistingBarchNeighb.AddCostComponent(cc1);
        JobExistingBarchNeighb.AddCostComponent(cc2);
        JobExistingBarchNeighb.AddCostComponent(cc3);
        JobExistingBarchNeighb.AddCostComponent(cc4);

        // create the multi-neighborhood
        SetUnionNeighborhoodExplorer<OSP_Input, OSP_Output, DefaultCostStructure<long>,
        decltype(SwapNeighb), decltype(InsertNeighb), decltype(InverseNeighb), decltype(SingleNewBatch),decltype(MoreNewBatchNeighb),decltype(JobExistingBarchNeighb)>
        multi_all
        (in, OSP_sm, 
        "multi_all", 
        SwapNeighb, InsertNeighb, InverseNeighb, SingleNewBatch, MoreNewBatchNeighb, JobExistingBarchNeighb,
        {
            swap_rate, insert_rate, inverse_rate, single_job_to_new_batch_rate, more_jobs_to_new_batch_rate, job_to_existing_batch_rate
        });

        SetUnionNeighborhoodExplorer<OSP_Input, OSP_Output, DefaultCostStructure<long>,
        decltype(InsertNeighb), decltype(InverseNeighb), decltype(SingleNewBatch),decltype(MoreNewBatchNeighb),decltype(JobExistingBarchNeighb)>
        multi_noSwap
        (in, OSP_sm, 
        "multi_noSwap", 
        InsertNeighb, InverseNeighb, SingleNewBatch, MoreNewBatchNeighb, JobExistingBarchNeighb,
        {
            insert_rate, inverse_rate, single_job_to_new_batch_rate, more_jobs_to_new_batch_rate, job_to_existing_batch_rate
        });

        SetUnionNeighborhoodExplorer<OSP_Input, OSP_Output, DefaultCostStructure<long>,
        decltype(SwapNeighb), decltype(InverseNeighb), decltype(SingleNewBatch),decltype(MoreNewBatchNeighb),decltype(JobExistingBarchNeighb)>
        multi_noInsert
        (in, OSP_sm, 
        "multi_noInsert", 
        SwapNeighb, InverseNeighb, SingleNewBatch, MoreNewBatchNeighb, JobExistingBarchNeighb,
        {
            swap_rate, inverse_rate, single_job_to_new_batch_rate, more_jobs_to_new_batch_rate, job_to_existing_batch_rate
        });

        SetUnionNeighborhoodExplorer<OSP_Input, OSP_Output, DefaultCostStructure<long>,
        decltype(SwapNeighb), decltype(InsertNeighb), decltype(SingleNewBatch),decltype(MoreNewBatchNeighb),decltype(JobExistingBarchNeighb)>
        multi_noInverse
        (in, OSP_sm, 
        "multi_noInverse", 
        SwapNeighb, InsertNeighb, SingleNewBatch, MoreNewBatchNeighb, JobExistingBarchNeighb,
        {
            swap_rate, insert_rate, single_job_to_new_batch_rate, more_jobs_to_new_batch_rate, job_to_existing_batch_rate
        });

        SetUnionNeighborhoodExplorer<OSP_Input, OSP_Output, DefaultCostStructure<long>,
        decltype(SwapNeighb), decltype(InsertNeighb), decltype(InverseNeighb), decltype(MoreNewBatchNeighb),decltype(JobExistingBarchNeighb)>
        multi_noSingleNewBatch
        (in, OSP_sm, 
        "multi_noSingleNewBatch", 
        SwapNeighb, InsertNeighb, InverseNeighb, MoreNewBatchNeighb, JobExistingBarchNeighb,
        {
            swap_rate, insert_rate, inverse_rate, more_jobs_to_new_batch_rate, job_to_existing_batch_rate
        });

        SetUnionNeighborhoodExplorer<OSP_Input, OSP_Output, DefaultCostStructure<long>,
        decltype(SwapNeighb), decltype(InsertNeighb), decltype(InverseNeighb), decltype(SingleNewBatch),decltype(JobExistingBarchNeighb)>
        multi_noMoreNewBatch
        (in, OSP_sm, 
        "multi_noMoreNewBatch", 
        SwapNeighb, InsertNeighb, InverseNeighb, SingleNewBatch, JobExistingBarchNeighb,
        {
            swap_rate, insert_rate, inverse_rate, single_job_to_new_batch_rate, job_to_existing_batch_rate
        });

        SetUnionNeighborhoodExplorer<OSP_Input, OSP_Output, DefaultCostStructure<long>,
        decltype(SwapNeighb), decltype(InsertNeighb), decltype(InverseNeighb), decltype(SingleNewBatch),decltype(MoreNewBatchNeighb)>
        multi_noExistingBatch
        (in, OSP_sm, 
        "multi_noExistingBatch", 
        SwapNeighb, InsertNeighb, InverseNeighb, SingleNewBatch, MoreNewBatchNeighb,
        {
            swap_rate, insert_rate, inverse_rate, single_job_to_new_batch_rate, more_jobs_to_new_batch_rate
        });
        
        // create the simulated annealing
        SimulatedAnnealing<OSP_Input, OSP_Output, decltype(multi_all)::MoveType, DefaultCostStructure<long>> SA_all(in, OSP_sm, multi_all, "SA_all");

        SimulatedAnnealing<OSP_Input, OSP_Output, decltype(multi_noSwap)::MoveType, DefaultCostStructure<long>> SA_noSwap(in, OSP_sm, multi_noSwap, "SA_noSwap");

        SimulatedAnnealing<OSP_Input, OSP_Output, decltype(multi_noInsert)::MoveType, DefaultCostStructure<long>> SA_noInsert(in, OSP_sm, multi_noInsert, "SA_noInsert");

        SimulatedAnnealing<OSP_Input, OSP_Output, decltype(multi_noInverse)::MoveType, DefaultCostStructure<long>> SA_noInverse(in, OSP_sm, multi_noInverse, "SA_noInverse");

        SimulatedAnnealing<OSP_Input, OSP_Output, decltype(multi_noSingleNewBatch)::MoveType, DefaultCostStructure<long>> SA_noSingleNewBatch(in, OSP_sm, multi_noSingleNewBatch, "SA_noSingleNewBatch");

        SimulatedAnnealing<OSP_Input, OSP_Output, decltype(multi_noMoreNewBatch)::MoveType, DefaultCostStructure<long>> SA_noMoreNewBatch(in, OSP_sm, multi_noMoreNewBatch, "SA_noMoreNewBatch");

        SimulatedAnnealing<OSP_Input, OSP_Output, decltype(multi_noExistingBatch)::MoveType, DefaultCostStructure<long>> SA_noExistingBatch(in, OSP_sm, multi_noExistingBatch, "SA_noExistingBatch");

        SimpleLocalSearch<OSP_Input, OSP_Output, DefaultCostStructure<long>> OSP_solver(in, OSP_sm, "OSP_solver");
        if (!CommandLineParameters::Parse(argc, argv, true, false))
        {
            return 1;
        }

        Runner<OSP_Input, OSP_Output, DefaultCostStructure<long>> *used_runner = nullptr;
        
        if (method ==  std::string("SA_all"))
        {
            used_runner = &SA_all;
            OSP_solver.SetRunner(SA_all);
        }
        else if (method ==  std::string("SA_noSwap"))
        {
            used_runner = &SA_noSwap;
            OSP_solver.SetRunner(SA_noSwap);
        }
        else if (method ==  std::string("SA_noInsert"))
        {
            used_runner = &SA_noInsert;
            OSP_solver.SetRunner(SA_noInsert);
        }
        else if (method ==  std::string("SA_noInverse"))
        {
            used_runner = &SA_noInverse;
            OSP_solver.SetRunner(SA_noInverse);
        }
        else if (method ==  std::string("SA_noSingleNewBatch"))
        {
            used_runner = &SA_noSingleNewBatch;
            OSP_solver.SetRunner(SA_noSingleNewBatch);
        }
        else if (method ==  std::string("SA_noMoreNewBatch"))
        {
            used_runner = &SA_noMoreNewBatch;
            OSP_solver.SetRunner(SA_noMoreNewBatch);
        }
        else if (method ==  std::string("SA_noExistingBatch"))
        {
            used_runner = &SA_noExistingBatch;
            OSP_solver.SetRunner(SA_noExistingBatch);
        }
        else
        {
            throw std::invalid_argument("Unknown method" + std::string(method));
        }

        // now perform the search
        SolverResult<OSP_Input,OSP_Output,DefaultCostStructure<long>> result(in);
        result = OSP_solver.Solve();
        // result is a tuple: 0: solution, 1: number of violations, 2: total cost, 3: computing time
        OSP_Output out = result.output;

        // output
        if (irace)
        {
            std::cout << (double)result.cost.total / in.UpperBoundIntegerObjective() << std::endl;
        }
        else if (output_file.IsSet())
        {
            std::ofstream os(static_cast<std::string>(output_file).c_str());
            os 
                // << "{\"solution\": {" << out <<  "}, "
                << "{\"total_cost\": " <<  result.cost.total <<  ", "
                << "\"time_seconds\": " << result.running_time << ", "
                << "\"total_iterations\": " << used_runner->Iteration() << ", "
                << "\"iteration_of_best\": " << used_runner->IterationOfBest() << ", "
                << "\"seed\": " << Random::GetSeed() << "} " << std::endl;
            os.flush();
            os.close();
        }
        else
        {
            std::cout << "{\"total_cost\": " <<  result.cost.total <<  ", "
                << "\"time\": " << result.running_time << ", "
                << "\"total_iterations\": " << used_runner->Iteration() << ", "
                << "\"iteration_of_best\": " << used_runner->IterationOfBest() << ", "
                << "\"seed\": " << Random::GetSeed() << "} " << std::endl;
        }
    }
    else if(initial_solution == 2)
    {
        // solution manager:
        OSP_SolutionManagerRandom OSP_sm(in);

        // attach cost to solution manager
        OSP_sm.AddCostComponent(cc1);
        OSP_sm.AddCostComponent(cc2);
        OSP_sm.AddCostComponent(cc3);
        OSP_sm.AddCostComponent(cc4);

        // neighborhood
        OSP_SwapBatchesNeighborhoodExplorer SwapNeighb(in,OSP_sm);
        OSP_BatchToNewPositionNeighborhoodExplorer InsertNeighb(in,OSP_sm);
        OSP_InvertBatchesInMachineNeighborhoodExplorer InverseNeighb(in,OSP_sm);
        OSP_JobToNewBatchNeighborhoodExplorer SingleNewBatch(in,OSP_sm);
        Decoupled_OSP_BatchToNewMachineNeighborhoodExplorer MoreNewBatchNeighb(in,OSP_sm);
        OSP_JobToExistingBatchNeighborhoodExplorer JobExistingBarchNeighb(in,OSP_sm);    
       
        // attach cost to neighborhoods
        SwapNeighb.AddCostComponent(cc1);
        SwapNeighb.AddCostComponent(cc2);
        SwapNeighb.AddCostComponent(cc3);
        SwapNeighb.AddCostComponent(cc4);

        InsertNeighb.AddCostComponent(cc1);
        InsertNeighb.AddCostComponent(cc2);
        InsertNeighb.AddCostComponent(cc3);
        InsertNeighb.AddCostComponent(cc4);

        InverseNeighb.AddCostComponent(cc1);
        InverseNeighb.AddCostComponent(cc2);
        InverseNeighb.AddCostComponent(cc3);
        InverseNeighb.AddCostComponent(cc4);

        SingleNewBatch.AddCostComponent(cc1);
        SingleNewBatch.AddCostComponent(cc2);
        SingleNewBatch.AddCostComponent(cc3);
        SingleNewBatch.AddCostComponent(cc4);

        MoreNewBatchNeighb.AddCostComponent(cc1);
        MoreNewBatchNeighb.AddCostComponent(cc2);
        MoreNewBatchNeighb.AddCostComponent(cc3);
        MoreNewBatchNeighb.AddCostComponent(cc4);

        JobExistingBarchNeighb.AddCostComponent(cc1);
        JobExistingBarchNeighb.AddCostComponent(cc2);
        JobExistingBarchNeighb.AddCostComponent(cc3);
        JobExistingBarchNeighb.AddCostComponent(cc4);

        // create the multi-neighborhood
        SetUnionNeighborhoodExplorer<OSP_Input, OSP_Output, DefaultCostStructure<long>,
        decltype(SwapNeighb), decltype(InsertNeighb), decltype(InverseNeighb), decltype(SingleNewBatch),decltype(MoreNewBatchNeighb),decltype(JobExistingBarchNeighb)>
        multi_all
        (in, OSP_sm, 
        "multi_all", 
        SwapNeighb, InsertNeighb, InverseNeighb, SingleNewBatch, MoreNewBatchNeighb, JobExistingBarchNeighb,
        {
            swap_rate, insert_rate, inverse_rate, single_job_to_new_batch_rate, more_jobs_to_new_batch_rate, job_to_existing_batch_rate
        });

        SetUnionNeighborhoodExplorer<OSP_Input, OSP_Output, DefaultCostStructure<long>,
        decltype(InsertNeighb), decltype(InverseNeighb), decltype(SingleNewBatch),decltype(MoreNewBatchNeighb),decltype(JobExistingBarchNeighb)>
        multi_noSwap
        (in, OSP_sm, 
        "multi_noSwap", 
        InsertNeighb, InverseNeighb, SingleNewBatch, MoreNewBatchNeighb, JobExistingBarchNeighb,
        {
            insert_rate, inverse_rate, single_job_to_new_batch_rate, more_jobs_to_new_batch_rate, job_to_existing_batch_rate
        });

        SetUnionNeighborhoodExplorer<OSP_Input, OSP_Output, DefaultCostStructure<long>,
        decltype(SwapNeighb), decltype(InverseNeighb), decltype(SingleNewBatch),decltype(MoreNewBatchNeighb),decltype(JobExistingBarchNeighb)>
        multi_noInsert
        (in, OSP_sm, 
        "multi_noInsert", 
        SwapNeighb, InverseNeighb, SingleNewBatch, MoreNewBatchNeighb, JobExistingBarchNeighb,
        {
            swap_rate, inverse_rate, single_job_to_new_batch_rate, more_jobs_to_new_batch_rate, job_to_existing_batch_rate
        });

        SetUnionNeighborhoodExplorer<OSP_Input, OSP_Output, DefaultCostStructure<long>,
        decltype(SwapNeighb), decltype(InsertNeighb), decltype(SingleNewBatch),decltype(MoreNewBatchNeighb),decltype(JobExistingBarchNeighb)>
        multi_noInverse
        (in, OSP_sm, 
        "multi_noInverse", 
        SwapNeighb, InsertNeighb, SingleNewBatch, MoreNewBatchNeighb, JobExistingBarchNeighb,
        {
            swap_rate, insert_rate, single_job_to_new_batch_rate, more_jobs_to_new_batch_rate, job_to_existing_batch_rate
        });

        SetUnionNeighborhoodExplorer<OSP_Input, OSP_Output, DefaultCostStructure<long>,
        decltype(SwapNeighb), decltype(InsertNeighb), decltype(InverseNeighb), decltype(MoreNewBatchNeighb),decltype(JobExistingBarchNeighb)>
        multi_noSingleNewBatch
        (in, OSP_sm, 
        "multi_noSingleNewBatch", 
        SwapNeighb, InsertNeighb, InverseNeighb, MoreNewBatchNeighb, JobExistingBarchNeighb,
        {
            swap_rate, insert_rate, inverse_rate, more_jobs_to_new_batch_rate, job_to_existing_batch_rate
        });

        SetUnionNeighborhoodExplorer<OSP_Input, OSP_Output, DefaultCostStructure<long>,
        decltype(SwapNeighb), decltype(InsertNeighb), decltype(InverseNeighb), decltype(SingleNewBatch),decltype(JobExistingBarchNeighb)>
        multi_noMoreNewBatch
        (in, OSP_sm, 
        "multi_noMoreNewBatch", 
        SwapNeighb, InsertNeighb, InverseNeighb, SingleNewBatch, JobExistingBarchNeighb,
        {
            swap_rate, insert_rate, inverse_rate, single_job_to_new_batch_rate, job_to_existing_batch_rate
        });

        SetUnionNeighborhoodExplorer<OSP_Input, OSP_Output, DefaultCostStructure<long>,
        decltype(SwapNeighb), decltype(InsertNeighb), decltype(InverseNeighb), decltype(SingleNewBatch),decltype(MoreNewBatchNeighb)>
        multi_noExistingBatch
        (in, OSP_sm, 
        "multi_noExistingBatch", 
        SwapNeighb, InsertNeighb, InverseNeighb, SingleNewBatch, MoreNewBatchNeighb,
        {
            swap_rate, insert_rate, inverse_rate, single_job_to_new_batch_rate, more_jobs_to_new_batch_rate
        });
        
        // create the simulated annealing
        SimulatedAnnealing<OSP_Input, OSP_Output, decltype(multi_all)::MoveType, DefaultCostStructure<long>> SA_all(in, OSP_sm, multi_all, "SA_all");

        SimulatedAnnealing<OSP_Input, OSP_Output, decltype(multi_noSwap)::MoveType, DefaultCostStructure<long>> SA_noSwap(in, OSP_sm, multi_noSwap, "SA_noSwap");

        SimulatedAnnealing<OSP_Input, OSP_Output, decltype(multi_noInsert)::MoveType, DefaultCostStructure<long>> SA_noInsert(in, OSP_sm, multi_noInsert, "SA_noInsert");

        SimulatedAnnealing<OSP_Input, OSP_Output, decltype(multi_noInverse)::MoveType, DefaultCostStructure<long>> SA_noInverse(in, OSP_sm, multi_noInverse, "SA_noInverse");

        SimulatedAnnealing<OSP_Input, OSP_Output, decltype(multi_noSingleNewBatch)::MoveType, DefaultCostStructure<long>> SA_noSingleNewBatch(in, OSP_sm, multi_noSingleNewBatch, "SA_noSingleNewBatch");

        SimulatedAnnealing<OSP_Input, OSP_Output, decltype(multi_noMoreNewBatch)::MoveType, DefaultCostStructure<long>> SA_noMoreNewBatch(in, OSP_sm, multi_noMoreNewBatch, "SA_noMoreNewBatch");

        SimulatedAnnealing<OSP_Input, OSP_Output, decltype(multi_noExistingBatch)::MoveType, DefaultCostStructure<long>> SA_noExistingBatch(in, OSP_sm, multi_noExistingBatch, "SA_noExistingBatch");

        SimpleLocalSearch<OSP_Input, OSP_Output, DefaultCostStructure<long>> OSP_solver(in, OSP_sm, "OSP_solver");
        if (!CommandLineParameters::Parse(argc, argv, true, false))
        {
            return 1;
        }

        Runner<OSP_Input, OSP_Output, DefaultCostStructure<long>> *used_runner = nullptr;
        
        if (method ==  std::string("SA_all"))
        {
            used_runner = &SA_all;
            OSP_solver.SetRunner(SA_all);
        }
        else if (method ==  std::string("SA_noSwap"))
        {
            used_runner = &SA_noSwap;
            OSP_solver.SetRunner(SA_noSwap);
        }
        else if (method ==  std::string("SA_noInsert"))
        {
            used_runner = &SA_noInsert;
            OSP_solver.SetRunner(SA_noInsert);
        }
        else if (method ==  std::string("SA_noInverse"))
        {
            used_runner = &SA_noInverse;
            OSP_solver.SetRunner(SA_noInverse);
        }
        else if (method ==  std::string("SA_noSingleNewBatch"))
        {
            used_runner = &SA_noSingleNewBatch;
            OSP_solver.SetRunner(SA_noSingleNewBatch);
        }
        else if (method ==  std::string("SA_noMoreNewBatch"))
        {
            used_runner = &SA_noMoreNewBatch;
            OSP_solver.SetRunner(SA_noMoreNewBatch);
        }
        else if (method ==  std::string("SA_noExistingBatch"))
        {
            used_runner = &SA_noExistingBatch;
            OSP_solver.SetRunner(SA_noExistingBatch);
        }
        else
        {
            throw std::invalid_argument("Unknown method" + std::string(method));
        }

        // now perform the search
        SolverResult<OSP_Input,OSP_Output,DefaultCostStructure<long>> result(in);
        result = OSP_solver.Solve();
        // result is a tuple: 0: solution, 1: number of violations, 2: total cost, 3: computing time
        OSP_Output out = result.output;

        // output
        if (irace)
        {
            std::cout << (double)result.cost.total / in.UpperBoundIntegerObjective() << std::endl;
        }
        else if (output_file.IsSet())
        {
            std::ofstream os(static_cast<std::string>(output_file).c_str());
            os 
                // << "{\"solution\": {" << out <<  "}, "
                << "{\"total_cost\": " <<  result.cost.total <<  ", "
                << "\"time_seconds\": " << result.running_time << ", "
                << "\"total_iterations\": " << used_runner->Iteration() << ", "
                << "\"iteration_of_best\": " << used_runner->IterationOfBest() << ", "
                << "\"seed\": " << Random::GetSeed() << "} " << std::endl;
            os.flush();
            os.close();
        }
        else
        {
            std::cout << "{\"total_cost\": " <<  result.cost.total <<  ", "
                << "\"time\": " << result.running_time << ", "
                << "\"total_iterations\": " << used_runner->Iteration() << ", "
                << "\"iteration_of_best\": " << used_runner->IterationOfBest() << ", "
                << "\"seed\": " << Random::GetSeed() << "} " << std::endl;
        }
    }

#if !defined(NDEBUG)
    std::cout << "This code is running in DEBUG mode" << std::endl;
#endif
    return 0; 
}