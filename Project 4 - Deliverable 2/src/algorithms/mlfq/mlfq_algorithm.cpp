#include "algorithms/mlfq/mlfq_algorithm.hpp"

#include <cassert>
#include <stdexcept>

#define FMT_HEADER_ONLY
#include "utilities/fmt/format.h"

/*
    Here is where you should define the logic for the MLFQ algorithm.
*/

MFLQScheduler::MFLQScheduler(int slice) {
    if (slice != -1) {
        throw("MLFQ does NOT take a customizable time slice");
    }
}

std::shared_ptr<SchedulingDecision> MFLQScheduler::get_next_thread() {
    std::shared_ptr<SchedulingDecision> decision = std::make_shared<SchedulingDecision>();

    bool done = false;
    int queue_num = 0;
    while(!done) {
        if (schedule[queue_num].size() != 0) {
            decision->thread = schedule[queue_num].top();
            schedule[queue_num].pop();
            decision->explanation = "Selected from queue " + std::to_string(queue_num) + " (priority = " + 
                priority_to_string(decision->thread->priority) + ", runtime = " + std::to_string(decision->thread->thread_runtime) + 
                "). Will run for at most " + std::to_string(decision->thread->thread_time_slice) + " ticks.";
            done = true;
            this->time_slice = decision->thread->thread_time_slice;
            // decision->thread->thread_priority++;

            // // if the runtime has reached the time slice, move down the queue
            decision->thread->thread_runtime += this->time_slice;
            // if (decision->thread->thread_runtime == decision->thread->thread_time_slice) {
            //     decision->thread->thread_priority++;
            //     decision->thread->thread_runtime = 0;
            // }
        }

        if (queue_num == 9) {   // if we've looked through all of the queues, return a nullptr
            decision->explanation = "No threads available for scheduling.";
            decision->thread = nullptr;
            done = true;
        }

        // increment the queue number
        queue_num++;
    }

    return decision;
}

void MFLQScheduler::add_to_ready_queue(std::shared_ptr<Thread> thread) {
    // if the runtime has reached the time slice, move down the queue
    // thread->thread_runtime++;
    if (thread->thread_runtime == thread->thread_time_slice) {
        thread->thread_priority++;
        thread->thread_runtime = 0;
    }

    switch (thread->thread_priority)
    {
    case 0:
        thread->thread_time_slice = std::pow(2, 0);    // time slice = 1
        this->schedule[0].push(thread->priority, thread);
        break;
    case 1:
        thread->thread_time_slice = std::pow(2, 1);    // time slice = 2
        this->schedule[1].push(thread->priority, thread);
        break;
    case 2:
        thread->thread_time_slice = std::pow(2, 2);    // time slice = 4
        this->schedule[2].push(thread->priority, thread);
        break;
    case 3:
        thread->thread_time_slice = std::pow(2, 3);    // time slice = 8
        this->schedule[3].push(thread->priority, thread);
        break;
    case 4:
        thread->thread_time_slice = std::pow(2, 4);    // time slice = 16
        this->schedule[4].push(thread->priority, thread);
        break;
    case 5:
        thread->thread_time_slice = std::pow(2, 5);    // time slice = 32
        this->schedule[5].push(thread->priority, thread);
        break;
    case 6:
        thread->thread_time_slice = std::pow(2, 6);    // time slice = 64
        this->schedule[6].push(thread->priority, thread);
        break;
    case 7:
        thread->thread_time_slice = std::pow(2, 7);    // time slice = 128
        this->schedule[7].push(thread->priority, thread);
        break;
    case 8:
        thread->thread_time_slice = std::pow(2, 8);    // time slice = 256
        this->schedule[8].push(thread->priority, thread);
        break;
    case 9:
     case 10:
        thread->thread_time_slice = std::pow(2, 9);    // time slice = 512
        this->schedule[9].push(thread->priority, thread);
        break;    
    default:
        std::string error = "Incorrect priority number. Throwing error.";
        throw(error);
        break;
    }
}

size_t MFLQScheduler::size() const {
        // loop over the array of queues and return the sum of their sizes
        int sum = 0;
        for (Stable_Priority_Queue<std::shared_ptr<Thread>> queue : this->schedule) {
            sum += queue.size();
        }
        return sum;
}




std::string MFLQScheduler::priority_to_string(ProcessPriority priority) {
    switch(priority)
    {
    case SYSTEM:
        return "SYSTEM";
    case INTERACTIVE:
        return "INTERACTIVE";
    case NORMAL:
        return "NORMAL";
    case BATCH:
        return "BATCH";
    default:
        return "";
    }
}
