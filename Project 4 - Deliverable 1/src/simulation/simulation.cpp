#include <fstream>
#include <iostream>

#include "algorithms/fcfs/fcfs_algorithm.hpp"
#include "algorithms/spn/spn_algorithm.hpp"
#include "algorithms/rr/rr_algorithm.hpp"

#include "simulation/simulation.hpp"
#include "types/enums.hpp"

#include "utilities/flags/flags.hpp"

Simulation::Simulation(FlagOptions flags) {
    // Hello!
    if (flags.scheduler == "FCFS") {
        // Create a FCFS scheduling algorithm
        this->scheduler = std::make_shared<FCFSScheduler>();
    } else if (flags.scheduler == "SPN") {
        // Create a SPN scheduling algorithm
        this->scheduler = std::make_shared<SPNScheduler>();
    } else if (flags.scheduler == "RR") {
        // Create a RR scheduling algorithm
        this->scheduler = std::make_shared<RRScheduler>(flags.time_slice);
    } else {
        throw("No scheduler found for " + flags.scheduler);        
    }
    this->flags = flags;
    this->logger = Logger(flags.verbose, flags.per_thread, flags.metrics);
}

void Simulation::run() {
    this->read_file(this->flags.filename);
    
    while (!this->events.empty()) {
        auto event = this->events.top();
        this->events.pop();

        // Invoke the appropriate method in the simulation for the given event type.

        switch(event->type) {
            case THREAD_ARRIVED:
                this->handle_thread_arrived(event);
                break;

            case THREAD_DISPATCH_COMPLETED:
            case PROCESS_DISPATCH_COMPLETED:
                this->handle_dispatch_completed(event);
                break;

            case CPU_BURST_COMPLETED:
                this->handle_cpu_burst_completed(event);
                break;

            case IO_BURST_COMPLETED:
                this->handle_io_burst_completed(event);
                break;
            case THREAD_COMPLETED:
                this->handle_thread_completed(event);
                break;

            case THREAD_PREEMPTED:
                this->handle_thread_preempted(event);
                break;

            case DISPATCHER_INVOKED:
                this->handle_dispatcher_invoked(event);
                break;
        }

        // If this event triggered a state change, print it out.
        if (event->thread && event->thread->current_state != event->thread->previous_state) {
            this->logger.print_state_transition(event, event->thread->previous_state, event->thread->current_state);
        } else if (event->scheduling_decision->thread) {
            this->logger.print_verbose(event, event->scheduling_decision->thread, event->scheduling_decision->explanation);
        }

        this->system_stats.total_time = event->time;
        event.reset();
    }
    // We are done!

    std::cout << "SIMULATION COMPLETED!\n\n";

    for (auto entry: this->processes) {
        this->logger.print_per_thread_metrics(entry.second);
    }

    logger.print_simulation_metrics(this->calculate_statistics());
}

//==============================================================================
// Event-handling methods
//==============================================================================

void Simulation::handle_thread_arrived(const std::shared_ptr<Event> event) {
    event->thread->set_ready(event->time);
    scheduler->add_to_ready_queue(event->thread);

    // If no active thread, run the scheduler!
    if (!active_thread && !running_dispatcher_invoked) {
        running_dispatcher_invoked = true;
        auto new_event = Event(DISPATCHER_INVOKED, event->time, event_num++, nullptr, nullptr);
        add_event(std::make_shared<Event>(new_event));
    }
}

void Simulation::handle_dispatch_completed(const std::shared_ptr<Event> event) {
    event->thread->set_running(event->time);

    /* 
    Determine the appropriate next even and generate it as adequate
        - If the next CPU burst is > than the quantum generate a thread Premted event
        - Otherwise pop the CPU burst from the queue and determine if there is an I/O burst avaliable
            - If yes next event is an CPU Burst Complete
            - If no next event is a Thread Complete 
    */
    std::shared_ptr<Event> new_event = nullptr;
    if (scheduler->time_slice == -1 || event->thread->get_next_burst(CPU)->length <= scheduler->time_slice) {
        int thread_service_time = event->thread->get_next_burst(CPU)->length;
        event->thread->pop_next_burst(CPU);

        if (event->thread->get_next_burst(IO)) {
            new_event = std::make_shared<Event>(Event(CPU_BURST_COMPLETED, event->time + thread_service_time, event_num++, active_thread, nullptr));
        } else {
            new_event = std::make_shared<Event>(Event(THREAD_COMPLETED, event->time + thread_service_time, event_num++, active_thread, nullptr));
        }
    } else {
        new_event = std::make_shared<Event>(Event(THREAD_PREEMPTED, event->time + scheduler->time_slice, event_num++, active_thread, nullptr));
    }

    add_event(new_event);
}

void Simulation::handle_cpu_burst_completed(const std::shared_ptr<Event> event) {
    event->thread->set_blocked(event->time);

    // Just finished using the CPU, run the scheduler!
    auto new_event = Event(DISPATCHER_INVOKED, event->time, event_num++, nullptr, nullptr);
    add_event(std::make_shared<Event>(new_event));

    new_event = Event(IO_BURST_COMPLETED, event->time + event->thread->get_next_burst(IO)->length, event_num++, event->thread, nullptr);
    add_event(std::make_shared<Event>(new_event));
}

void Simulation::handle_io_burst_completed(const std::shared_ptr<Event> event) {
    // Run the scheduler if we don't have an active thread.
    if (active_thread == nullptr) {
        auto new_event = Event(DISPATCHER_INVOKED, event->time, event_num++, nullptr, nullptr);
        add_event(std::make_shared<Event>(new_event));
    }

    event->thread->set_ready(event->time);
    event->thread->pop_next_burst(IO);
    scheduler->add_to_ready_queue(event->thread);
}

void Simulation::handle_thread_completed(const std::shared_ptr<Event> event) {
    event->thread->set_finished(event->time);
        
    // Just finished using the CPU, run the scheduler!
    auto new_event = Event(DISPATCHER_INVOKED, event->time, event_num++, nullptr, nullptr);
    add_event(std::make_shared<Event>(new_event));
}

void Simulation::handle_thread_preempted(const std::shared_ptr<Event> event) {
    event->thread->set_ready(event->time);
    event->thread->get_next_burst(CPU)->update_time(scheduler->time_slice);
    scheduler->add_to_ready_queue(event->thread);
    auto new_event = Event(DISPATCHER_INVOKED, event->time, event_num++, nullptr, nullptr);
    add_event(std::make_shared<Event>(new_event));
}

void Simulation::handle_dispatcher_invoked(const std::shared_ptr<Event> event) {
    if (active_thread) {
        prev_thread = active_thread;
    }

    event->scheduling_decision = scheduler->get_next_thread();
    running_dispatcher_invoked = false;

    // If we have a thread, then make either PROCESS_DISPATCH_COMPLETED or THREAD_DISPATCH_COMPLETED
    // based on whether the previous thread running is the same process as this one
    if (event->scheduling_decision->thread) {
        std::shared_ptr<Event> new_event = nullptr;
        active_thread = event->scheduling_decision->thread;

        if (!prev_thread || active_thread->process_id != prev_thread->process_id) {
            system_stats.dispatch_time += process_switch_overhead;
            new_event = std::make_shared<Event>(Event(PROCESS_DISPATCH_COMPLETED, event->time + process_switch_overhead, event_num++, active_thread, event->scheduling_decision));
        } else {
            system_stats.dispatch_time += thread_switch_overhead;
            new_event = std::make_shared<Event>(Event(THREAD_DISPATCH_COMPLETED, event->time + thread_switch_overhead, event_num++, active_thread, event->scheduling_decision));
        }

        // Regardless of whether this is PROCESS_DISPATCH_COMPLETED or THREAD_DISPATCH_COMPLETED,
        // always add to the queue
        add_event(new_event);
    } else {

        // No threads in the ready queue ==> no threads to be scheduled
        // Thus, the CPU will become _idle_ 
        active_thread = nullptr;
        return;
    }
}


//==============================================================================
// Utility methods
//==============================================================================

SystemStats Simulation::calculate_statistics() {
    // declare counters and other temporary variables
    size_t sysCount = 0, interCount = 0, normCount = 0, batchCount = 0;
    size_t servTime = 0, ioTime = 0;
    double turnaroundSys = 0, responseSys = 0, turnaroundInter = 0, responseInter = 0;
    double turnaroundNorm = 0, responseNorm = 0, turnaroundBatch = 0, responseBatch = 0;

    // examine each process and calculate statistics
    std::map<int, std::shared_ptr<Process>>::iterator it = this->processes.begin();
    for (it = this->processes.begin(); it != this->processes.end(); it++) {
        switch (it->second->priority) {
        case SYSTEM:
            // count the threads
            sysCount += it->second->threads.size();

            // loop through each thread in process for statistics
            examineThreads(it->second->threads, turnaroundSys, responseSys, servTime, ioTime);

            // break when done
            break;

        case INTERACTIVE:
            // count the threads
            interCount += it->second->threads.size();
            
            // loop through each thread in process for statistics
            examineThreads(it->second->threads, turnaroundInter, responseInter, servTime, ioTime);

            // break when done
            break;
        
        case NORMAL:
            // count the threads
            normCount += it->second->threads.size();
            
            // loop through each thread in process for statistics
            examineThreads(it->second->threads, turnaroundNorm, responseNorm, servTime, ioTime);

            // break when done
            break;
        
        case BATCH:
            // count the threads
            batchCount += it->second->threads.size();
            
            // loop through each thread in process for statistics
            examineThreads(it->second->threads, turnaroundBatch, responseBatch, servTime, ioTime);

            // break when done
            break;
        
        default:
            break;
        }
    }


    // number of threads per process priority
    this->system_stats.thread_counts[0] = sysCount;
    this->system_stats.thread_counts[1] = interCount;
    this->system_stats.thread_counts[2] = normCount;
    this->system_stats.thread_counts[3] = batchCount;

    // turnaround and response time averages
    if (sysCount != 0) {
        this->system_stats.avg_thread_turnaround_times[0] = turnaroundSys / sysCount;
        this->system_stats.avg_thread_response_times[0] = responseSys / sysCount;
    }

    if (interCount != 0) {
        this->system_stats.avg_thread_turnaround_times[1] = turnaroundInter / interCount;
        this->system_stats.avg_thread_response_times[1] = responseInter / interCount;
    }
    
    if (normCount != 0) {
        this->system_stats.avg_thread_turnaround_times[2] = turnaroundNorm / normCount;
        this->system_stats.avg_thread_response_times[2] = responseNorm / normCount;
    }

    if (batchCount != 0) {
        this->system_stats.avg_thread_turnaround_times[3] = turnaroundBatch / batchCount;
        this->system_stats.avg_thread_response_times[3] = responseBatch / batchCount;
    }


    // total service time
    this->system_stats.service_time = servTime;

    // total I/O time
    this->system_stats.io_time = ioTime;

    // total idle time: total - service - dispatch
    this->system_stats.total_idle_time = this->system_stats.total_time - this->system_stats.service_time - this->system_stats.dispatch_time;

    // CPU utilization: 100.0 * (total time - total idle time) / total time
    this->system_stats.cpu_utilization = 100.0 * (this->system_stats.total_time - this->system_stats.total_idle_time) / this->system_stats.total_time;

    // CPU efficiency: 100.0 * service time / total time
    this->system_stats.cpu_efficiency = 100.0 * this->system_stats.service_time / this->system_stats.total_time;


    return this->system_stats;
}

void Simulation::examineThreads(std::vector <std::shared_ptr<Thread>> vecThreads, double &turnaround, double &response, size_t &service, size_t &io) {
    // bool isFirst = true;

    // loop through each thread in process for statistics
    for (std::shared_ptr<Thread> thr : vecThreads) {
        turnaround += thr->turnaround_time();

        response += thr->response_time();

        // service and io times
        service += thr->service_time;
        io += thr->io_time;       
                
    }

    // return when done
    return;
}

void Simulation::add_event(std::shared_ptr<Event> event) {
    if (event != nullptr) {
        this->events.push(event);
    }
}

void Simulation::read_file(const std::string filename) {
    std::ifstream input_file(filename.c_str());

    if (!input_file) {
        std::cerr << "Unable to open simulation file: " << filename << std::endl;
        throw(std::logic_error("Bad file."));
    }

    int num_processes;

    input_file >> num_processes >> this->thread_switch_overhead >> this->process_switch_overhead;

    for (int proc = 0; proc < num_processes; ++proc) {
        auto process = read_process(input_file);

        this->processes[process->process_id] = process;
    }
}

std::shared_ptr<Process> Simulation::read_process(std::istream& input) {
    int process_id, priority;
    int num_threads;

    input >> process_id >> priority >> num_threads;

    auto process = std::make_shared<Process>(process_id, (ProcessPriority) priority);

    // iterate over the threads
    for (int thread_id = 0; thread_id < num_threads; ++thread_id) {
        process->threads.emplace_back(read_thread(input, thread_id, process_id, (ProcessPriority) priority));
    }

    return process;
}

std::shared_ptr<Thread> Simulation::read_thread(std::istream& input, int thread_id, int process_id, ProcessPriority priority) {
    // Stuff
    int arrival_time;
    int num_cpu_bursts;

    input >> arrival_time >> num_cpu_bursts;

    auto thread = std::make_shared<Thread>(arrival_time, thread_id, process_id, priority);

    for (int n = 0, burst_length; n < num_cpu_bursts * 2 - 1; ++n) {
        input >> burst_length;

        BurstType burst_type = (n % 2 == 0) ? BurstType::CPU : BurstType::IO;

        thread->bursts.push(std::make_shared<Burst>(burst_type, burst_length));
    }

    this->events.push(std::make_shared<Event>(EventType::THREAD_ARRIVED, thread->arrival_time, this->event_num, thread, nullptr));
    this->event_num++;

    return thread;
}
