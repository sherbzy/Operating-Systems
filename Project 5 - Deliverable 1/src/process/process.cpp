/**
 * This file contains implementations for methods in the Process class.
 *
 * You'll need to add code here to make the corresponding tests pass.
 */

#include "process/process.h"

using namespace std;


Process* Process::read_from_input(std::istream& in) {
    size_t num_bytes = 0;
    std::vector<Page *> pages;

    while(!in.eof()) {
        Page* curr_page = Page::read_from_input(in);
        pages.push_back(curr_page);
        num_bytes += in.gcount();
    }

    return new Process(num_bytes, pages);
}


size_t Process::size() const
{
    return this->num_bytes;
}


bool Process::is_valid_page(size_t index) const
{
    // loop through pages to find index
    if (index < this->pages.size()) {
        return true;
    }
    
    // if none of the pages have the index, return false
    return false;
}


size_t Process::get_rss() const
{
    size_t rss = 0;
    for (int i = 0; i < this->page_table.rows.size(); i++) {
        if (this->page_table.rows.at(i).present) {
            rss++;
        }
    }

    return rss;
}


double Process::get_fault_percent() const
{
    double fault_percent = 0.0;
    if (this->memory_accesses > 0) {
        fault_percent = static_cast<double>(this->page_faults)/ static_cast<double>(this->memory_accesses);
        fault_percent *= 100.0;
    }

    return fault_percent;
}
