#include "algorithms/priority/priority_algorithm.hpp"

#include <cassert>
#include <stdexcept>

#define FMT_HEADER_ONLY
#include "utilities/fmt/format.h"


/*
    Here is where you should define the logic for the priority algorithm.
*/

PRIORITYScheduler::PRIORITYScheduler(int slice) {
    if (slice != -1) {
        throw("PRIORITY must have a timeslice of -1");
    }
}

std::shared_ptr<SchedulingDecision> PRIORITYScheduler::get_next_thread() {
    std::shared_ptr<SchedulingDecision> decision = std::make_shared<SchedulingDecision>();
    if (this->schedule.empty()) {
        decision->explanation = "No threads available for scheduling.";
        decision->thread = nullptr;
    } else {
        decision->thread = this->schedule.top();

        decision->explanation = "[S: " + std::to_string(num_system) + " I: " + std::to_string(num_interactive) + 
            " N: " + std::to_string(num_normal) + " B: " + std::to_string(num_batch) + "] -> [S: ";

        switch (decision->thread->priority)
        {
     
        case SYSTEM:
            num_system--;
            break;
        case INTERACTIVE:
            num_interactive--;
            
            break;
        case NORMAL:
            num_normal--;
            break;
        case BATCH:
            num_batch--;
            break;
        
        default:
            break;
        }
        this->schedule.pop();

        decision->explanation += std::to_string(num_system) + " I: " + std::to_string(num_interactive) + " N: " + 
                std::to_string(num_normal) + " B: " + std::to_string(num_batch) + "]. Will run to completion of burst.";


    }

    return decision;

}

void PRIORITYScheduler::add_to_ready_queue(std::shared_ptr<Thread> thread) {
    // add thread to the queue based on priority
    this->schedule.push(thread->priority, thread);

    // increment the count of the added priority
    switch (thread->priority)
    {
    case SYSTEM:
        this->num_system++;
        break;
    case INTERACTIVE:
        this->num_interactive++;
        break;
    case NORMAL:
        this->num_normal++;
        break;
    case BATCH:
        this->num_batch++;
        break;
    
    default:
        break;
    }

    // return when done
    return;
}

size_t PRIORITYScheduler::size() const {
    return this->schedule.size();
}
