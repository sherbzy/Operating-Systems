#include "algorithms/rr/rr_algorithm.hpp"

#include <cassert>
#include <stdexcept>

#define FMT_HEADER_ONLY
#include "utilities/fmt/format.h"

/*
    Here is where you should define the logic for the round robin algorithm.
*/

RRScheduler::RRScheduler(int slice) {
    if (slice == -1) {
        // time slice should be set to 3
        this->time_slice = 3;
    } else if (slice <= 0) {
        // throw logic error
        throw(std::logic_error("FCFS must have a timeslice of -1"));
    } else {
        // set the time slice to the given number
        this->time_slice = slice;
    }
}

std::shared_ptr<SchedulingDecision> RRScheduler::get_next_thread() {
    std::shared_ptr<SchedulingDecision> decision = std::make_shared<SchedulingDecision>();
    if (this->schedule.empty()) {
        decision->explanation = "No threads available for scheduling.";
        decision->thread = nullptr;
    } else {
        int numThreads = static_cast<int>(this->size());    // number of threads
        decision->explanation = "Selected from " + std::to_string(numThreads) + " threads. Will run for at most " + std::to_string(this->time_slice) + " ticks.";

        decision->thread = this->schedule.front();
        this->schedule.pop();
    }

    return decision;
}

void RRScheduler::add_to_ready_queue(std::shared_ptr<Thread> thread) {
    // add thread to back of queue
    this->schedule.push(thread);
    return;
}

size_t RRScheduler::size() const {
    return this->schedule.size();
}
