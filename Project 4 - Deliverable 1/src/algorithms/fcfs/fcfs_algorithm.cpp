#include "algorithms/fcfs/fcfs_algorithm.hpp"

#include <cassert>
#include <stdexcept>
#include <cstring>

#define FMT_HEADER_ONLY
#include "utilities/fmt/format.h"

/*
    Here is where you should define the logic for the FCFS algorithm.
*/

FCFSScheduler::FCFSScheduler(int slice) {
    if (slice != -1) {
        throw(std::logic_error("FCFS must have a timeslice of -1"));
    }
}

std::shared_ptr<SchedulingDecision> FCFSScheduler::get_next_thread() {
    std::shared_ptr<SchedulingDecision> decision = std::make_shared<SchedulingDecision>();
    if (this->schedule.empty()) {
        decision->explanation = "No threads available for scheduling.";
        decision->thread = nullptr;
    } else {
        int numThreads = static_cast<int>(this->size());    // number of threads
        decision->explanation = "Selected from " + std::to_string(numThreads) + " threads. Will run to completion of burst.";

        decision->thread = this->schedule.front();
        this->schedule.pop();
    }

    return decision;
}

void FCFSScheduler::add_to_ready_queue(std::shared_ptr<Thread> thread) {
    // add thread to back of queue
    this->schedule.push(thread);
    return;
}

// return the size of the queue
size_t FCFSScheduler::size() const {
    return this->schedule.size();
}
