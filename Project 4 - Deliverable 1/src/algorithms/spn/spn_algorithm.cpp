#include "algorithms/spn/spn_algorithm.hpp"

#include <cassert>
#include <stdexcept>

#define FMT_HEADER_ONLY
#include "utilities/fmt/format.h"

/*
    Here is where you should define the logic for the SPN algorithm.
*/

SPNScheduler::SPNScheduler(int slice) {
    if (slice != -1) {
        throw("SPN must have a timeslice of -1");
    }
}

std::shared_ptr<SchedulingDecision> SPNScheduler::get_next_thread() {
    std::shared_ptr<SchedulingDecision> decision = std::make_shared<SchedulingDecision>();
    if (this->schedule.empty()) {
        decision->explanation = "No threads available for scheduling.";
        decision->thread = nullptr;
    } else {
        int numThreads = static_cast<int>(this->size());    // number of threads
        decision->explanation = "Selected from " + std::to_string(numThreads) + " threads. Will run to completion of burst.";

        decision->thread = this->schedule.top();
        this->schedule.pop();
    }

    return decision;
}

void SPNScheduler::add_to_ready_queue(std::shared_ptr<Thread> thread) {
    // add thread to the queue based on how long their cpu burst is
    this->schedule.push(thread->get_next_burst(CPU)->length, thread);
    return;
}

size_t SPNScheduler::size() const {
    return this->schedule.size();
}
