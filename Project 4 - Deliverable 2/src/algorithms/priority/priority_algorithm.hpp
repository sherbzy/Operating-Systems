#ifndef PRIORITY_ALGORITHM_HPP
#define PRIORITY_ALGORITHM_HPP

#include <memory>
#include <string>
#include <vector>
#include <queue>
#include "algorithms/fcfs/fcfs_algorithm.hpp"
#include "algorithms/scheduling_algorithm.hpp"
#include "utilities/stable_priority_queue/stable_priority_queue.hpp"

/*
    PRIORITYScheduler:
        A representation of a priority scheduling algorithm.

        This is a derived class from the base scheduling algorithm class.

        You are free to add any member functions or member variables that you
        feel are helpful for implementing the algorithm.
*/

// "typedef" this type
using PriorityQueue = Stable_Priority_Queue<std::shared_ptr<Thread>>; 

class PRIORITYScheduler : public Scheduler {
private:
        //==================================================
        //  Member variables
        //==================================================

        Stable_Priority_Queue<std::shared_ptr<Thread>> schedule;
        int num_system = 0;
        int num_interactive = 0;
        int num_normal = 0;
        int num_batch = 0;
        // Stable_Priority_Queue<std::shared_ptr<Thread>> system_schedule;
        // Stable_Priority_Queue<std::shared_ptr<Thread>> interactive_schedule;
        // Stable_Priority_Queue<std::shared_ptr<Thread>> normal_schedule;
        // Stable_Priority_Queue<std::shared_ptr<Thread>> batch_schedule;

public:

        //==================================================
        //  Member functions
        //==================================================

        PRIORITYScheduler(int slice = -1);

        std::shared_ptr<SchedulingDecision> get_next_thread();

        void add_to_ready_queue(std::shared_ptr<Thread> thread);

        size_t size() const;

};

#endif
