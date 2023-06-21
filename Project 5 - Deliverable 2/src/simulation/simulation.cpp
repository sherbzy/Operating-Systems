/**
 * This file contains implementations for the methods defined in the Simulation
 * class.
 *
 * You'll probably spend a lot of your time here.
 */

#include "simulation/simulation.h"
#include <stdexcept>

Simulation::Simulation(FlagOptions& flags)
{
    this->flags = flags;
    this->frames.reserve(this->NUM_FRAMES);
    
}

void Simulation::run() {

    // populate free frames list
    for (int i = 0; i < NUM_FRAMES; i++) {
        this->free_frames.push_back(i);
    }


    // iterate through the vector of processes
    for(int i = 0; i < virtual_addresses.size(); i++) {
        if (this->flags.verbose) {
            std::cout << virtual_addresses[i] << std::endl;
        }


        // get virtual address and perform a memory access
        char return_val = perform_memory_access(virtual_addresses[i]);
        if (this->flags.verbose) {
            std::cout << "\t-> RSS: " << this->processes[virtual_addresses[i].process_id]->get_rss() << std::endl;
        }

        // increment time
        this->time++;
    }

    // print summary
    this->print_summary();

    // delete the memory for every page and every process
    std::map<int, Process*>::iterator it = processes.begin();
    for (it; it != processes.end(); it++) { // iterate through the processes
        for (int i = 0; i < it->second->pages.size(); i++) {   // iterate through pages within process and delete stuff
            delete(it->second->pages[i]);
        }

        delete(it->second);
    }

}

char Simulation::perform_memory_access(const VirtualAddress& virtual_address) {
    // find the process using the pid
    Process* temp_process = this->processes[virtual_address.process_id];

    // check for a page seg fault - is the page valid
    if (temp_process->is_valid_page(virtual_address.page)) {

        // no page seg fault...
        // check for page fault - is the page in the table
        if (temp_process->page_table.rows[virtual_address.page].present) { // is this right?
            // page is present...
            if (this->flags.verbose) {
                std::cout << "\t-> IN MEMORY" << std::endl;
            }

            // increment memory accesses counter
            temp_process->memory_accesses++;

            // convert virtual address to a physical address
            int frame = temp_process->page_table.rows[virtual_address.page].frame;
            int offset = virtual_address.offset;
            PhysicalAddress physical_address = PhysicalAddress(frame, offset);
            if (this->flags.verbose) {
                std::cout << "\t-> physical address " << physical_address << std::endl;
            }

            // check if offset is valid
            if (temp_process->pages.at(virtual_address.page)->is_valid_offset(offset)) {
                // set the access time and return the byte at the offset
                temp_process->page_table.rows[virtual_address.page].last_accessed_at = this->time;
                return temp_process->pages.at(virtual_address.page)->get_byte_at_offset(offset);
            } else {
                // return seg fault - invalid offset
                std::cout << "SEGFAULT - INVALID OFFSET" << std::endl;
                exit(-1);
            }
            
        } else {
            // page is not present...
            if (this->flags.verbose) {
                std::cout << "\t-> PAGE FAULT" << std::endl;
            }

            // increment memory accesses counter and page faults counter
            temp_process->memory_accesses++;
            temp_process->page_faults++;
            this->page_faults++;
            

            // handle the page fault
            handle_page_fault(temp_process, virtual_address.page);

            // convert virtual address to a physical address
            int frame = temp_process->page_table.rows[virtual_address.page].frame;
            int offset = virtual_address.offset;
            PhysicalAddress physical_address = PhysicalAddress(frame, offset);
            if (this->flags.verbose) {
                std::cout << "\t-> physical address " << physical_address << std::endl;
            }

            // check if offset is valid
            if (temp_process->pages.at(virtual_address.page)->is_valid_offset(offset)) {
                // set the access time and return the byte at the offset
                temp_process->page_table.rows[virtual_address.page].last_accessed_at = this->time;
                return temp_process->pages.at(virtual_address.page)->get_byte_at_offset(offset);
            } else {
                // return seg fault - invalid offset
                std::cout << "SEGFAULT - INVALID OFFSET" << std::endl;
                exit(-1);
            }

        }
        

    } else {
        // return page seg fault
        std::cout << "SEGFAULT - INVALID PAGE" << std::endl;
        exit(-1);
    }
}

void Simulation::handle_page_fault(Process* process, size_t page) {
    // compare the page count in the page table to the max frames
    if (process->page_table.get_present_page_count() < flags.max_frames) {
        // find the first free frame available and update all of the page's data accordingly
        size_t frame_to_use = this->free_frames.front();

        // set up the things - frame of the page, present, times
        process->page_table.rows[page].frame = frame_to_use;
        process->page_table.rows[page].present = true;
        process->page_table.rows[page].last_accessed_at = this->time;
        process->page_table.rows[page].loaded_at = this->time;

        // set_page() for the given frame
        this->frames[frame_to_use].set_page(process, page);

        // pop the free frame used from the front of the list
        this->free_frames.pop_front();
    } else {
        // check flags for FIFO or LRU
        size_t page_to_change = process->page_table.get_oldest_page();
        if (flags.strategy == ReplacementStrategy::FIFO) {
            // get the oldest page
            page_to_change = process->page_table.get_oldest_page();

            // reset the old page information using the new page's information
            //process->page_table.rows[page] = process->page_table.rows[page];  // could be wrong?
            process->page_table.rows[page_to_change].present = false;
            frames[process->page_table.rows[page_to_change].frame].contents = process->pages[page];

            // set present, times, and frame
            process->page_table.rows[page].present = true;
            process->page_table.rows[page].last_accessed_at = this->time;
            process->page_table.rows[page].loaded_at = this->time;
            process->page_table.rows[page].frame = process->page_table.rows[page_to_change].frame;

        } else if (flags.strategy == ReplacementStrategy::LRU) {
            // get the least recently used page
            page_to_change = process->page_table.get_least_recently_used_page();

            // reset the old page information using the new page's information
            // process->page_table.rows[page] = process->page_table.rows[page_to_change];   // fun fact, it was wrong oops
            process->page_table.rows[page_to_change].present = false;
            frames[process->page_table.rows[page_to_change].frame].contents = process->pages[page];

            // set present, times, and frame
            process->page_table.rows[page].present = true;
            process->page_table.rows[page].last_accessed_at = this->time;
            process->page_table.rows[page].loaded_at = this->time;
            process->page_table.rows[page].frame = process->page_table.rows[page_to_change].frame;

        }
    }

    // return when done
    return;
}

void Simulation::print_summary() {
    if (!this->flags.csv) {
        boost::format process_fmt(
            "Process %3d:  "
            "ACCESSES: %-6lu "
            "FAULTS: %-6lu "
            "FAULT RATE: %-8.2f "
            "RSS: %-6lu\n");

        for (auto entry : this->processes) {
            std::cout << process_fmt
                % entry.first
                % entry.second->memory_accesses
                % entry.second->page_faults
                % entry.second->get_fault_percent()
                % entry.second->get_rss();
        }

        // Print statistics.
        boost::format summary_fmt(
            "\n%-25s %12lu\n"
            "%-25s %12lu\n"
            "%-25s %12lu\n");

        std::cout << summary_fmt
            % "Total memory accesses:"
            % this->virtual_addresses.size()
            % "Total page faults:"
            % this->page_faults
            % "Free frames remaining:"
            % this->free_frames.size();
    }

    if (this->flags.csv) {
        boost::format process_fmt(
            "%d,"
            "%lu,"
            "%lu,"
            "%.2f,"
            "%lu\n");

        for (auto entry : processes) {
            std::cout << process_fmt
                % entry.first
                % entry.second->memory_accesses
                % entry.second->page_faults
                % entry.second->get_fault_percent()
                % entry.second->get_rss();
        }

        // Print statistics.
        boost::format summary_fmt(
            "%lu,,,,\n"
            "%lu,,,,\n"
            "%lu,,,,\n");

        std::cout << summary_fmt
            % this->virtual_addresses.size()
            % this->page_faults
            % this->free_frames.size();
    }
}

int Simulation::read_processes(std::istream& simulation_file) {
    int num_processes;
    simulation_file >> num_processes;

    for (int i = 0; i < num_processes; ++i) {
        int pid;
        std::string process_image_path;

        simulation_file >> pid >> process_image_path;

        std::ifstream proc_img_file(process_image_path);

        if (!proc_img_file) {
            std::cerr << "Unable to read file for PID " << pid << ": " << process_image_path << std::endl;
            return 1;
        }
        this->processes[pid] = Process::read_from_input(proc_img_file);
    }
    return 0;
}

int Simulation::read_addresses(std::istream& simulation_file) {
    int pid;
    std::string virtual_address;

    try {
        while (simulation_file >> pid >> virtual_address) {
            this->virtual_addresses.push_back(VirtualAddress::from_string(pid, virtual_address));
        }
    } catch (const std::exception& except) {
        std::cerr << "Error reading virtual addresses." << std::endl;
        std::cerr << except.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Error reading virtual addresses." << std::endl;
        return 1;
    }
    return 0;
}

int Simulation::read_simulation_file() {
    std::ifstream simulation_file(this->flags.filename);
    // this->simulation_file.open(this->flags.filename);

    if (!simulation_file) {
        std::cerr << "Unable to open file: " << this->flags.filename << std::endl;
        return -1;
    }
    int error = 0;
    error = this->read_processes(simulation_file);

    if (error) {
        std::cerr << "Error reading processes. Exit: " << error << std::endl;
        return error;
    }

    error = this->read_addresses(simulation_file);

    if (error) {
        std::cerr << "Error reading addresses." << std::endl;
        return error;
    }

    if (this->flags.file_verbose) {
        for (auto entry: this->processes) {
            std::cout << "Process " << entry.first << ": Size: " << entry.second->size() << std::endl;
        }

        for (auto entry : this->virtual_addresses) {
            std::cout << entry << std::endl;
        }
    }

    return 0;
}
