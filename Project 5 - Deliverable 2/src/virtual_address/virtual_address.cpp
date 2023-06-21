/**
 * This file contains implementations for methods in the VirtualAddress class.
 *
 * You'll need to add code here to make the corresponding tests pass.
 */

#include "virtual_address/virtual_address.h"

using namespace std;

VirtualAddress VirtualAddress::from_string(int process_id, string address) {
    // split address into page and offset
    string page = "", offset = "";
    for (int i = 0; i < PAGE_BITS; i++) {
        page += address.at(i);
    }
    for(int i = PAGE_BITS; i < PAGE_BITS + OFFSET_BITS; i++) {
        offset += address.at(i);
    }

    // turn string binary into integers
    int page_integer = bitset<PAGE_BITS>(page).to_ulong();
    int offset_integer = bitset<OFFSET_BITS>(offset).to_ulong();

    // create and return a new virtual address
    VirtualAddress virtual_address = VirtualAddress(process_id, page_integer, offset_integer);
    return virtual_address;
}


string VirtualAddress::to_string() const {
    // convert page and offset to binary strings
    string page_binary = bitset<PAGE_BITS>((int)this->page).to_string();
    string offset_binary = bitset<OFFSET_BITS>((int)this->offset).to_string();

    // return the full address as a binary string
    string virtual_address = page_binary + offset_binary;
    return virtual_address;
}


ostream& operator <<(ostream& out, const VirtualAddress& address) {
    // convert the virtual address into a readable string
    string output = "PID " + to_string(address.process_id) + " @ " + address.to_string() + " [page: " + to_string(address.page) + "; offset: " + to_string(address.offset) + "]";
    out << output;
    return out;
}
