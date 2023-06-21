/**
 * This file contains implementations for methods in the PageTable class.
 *
 * You'll need to add code here to make the corresponding tests pass.
 */

#include "page_table/page_table.h"

using namespace std;


size_t PageTable::get_present_page_count() const {
    int count = 0;
    for (int i = 0; i < this->rows.size(); i++) {
        if (this->rows.at(i).present) {
            count++;
        }
    }
    return count;
}


size_t PageTable::get_oldest_page() const {
    size_t time = -1;
    size_t oldest_page = 0;

    // loop through the rows and compare their load times to find the oldest page
    for (int i = 0; i < this->rows.size(); i++) {
        if (this->rows.at(i).present) {
            if (this->rows.at(i).loaded_at < time) {
                time = this->rows.at(i).loaded_at;
                oldest_page = i;
            }
        }
    }
    return oldest_page;
}


size_t PageTable::get_least_recently_used_page() const {
    size_t time = -1;
    size_t least_recently_used = 0;

    // loop through the rows and compare their access times to find the least recently used page
    for (int i = 0; i < this->rows.size(); i++) {
        if ((this->rows.at(i).last_accessed_at < time) && (this->rows.at(i).present)) {
            time = this->rows.at(i).last_accessed_at;
            least_recently_used = i;
        }
    }
    return least_recently_used;
}
