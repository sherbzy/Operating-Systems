/**
 * This file contains implementations for methods in the Frame class.
 *
 * You'll need to add code here to make the corresponding tests pass.
 */

#include "frame/frame.h"

using namespace std;

void Frame::set_page(Process* process, size_t page_number) {
    // set the contents
    this->contents = process->pages[page_number];

    // set the process and the page number
    this->process = process;
    this->page_number = page_number;

    // return when done
    return;
}
