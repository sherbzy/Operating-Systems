/**
 * This file contains implementations for methods in the PhysicalAddress class.
 *
 * You'll need to add code here to make the corresponding tests pass.
 */

#include "physical_address/physical_address.h"

using namespace std;

string PhysicalAddress::to_string() const {
    // convert page and offset to binary strings
    string frame_binary = bitset<FRAME_BITS>((int)this->frame).to_string();
    string offset_binary = bitset<OFFSET_BITS>((int)this->offset).to_string();

    // return the full address as a binary string
    string virtual_address = frame_binary + offset_binary;
    return virtual_address;
}


ostream& operator <<(ostream& out, const PhysicalAddress& address) {
    // convert the physical address into a readable string
    string output = address.to_string() + " [frame: " + to_string(address.frame) + "; offset: " + to_string(address.offset) + "]";
    out << output;
    return out;
}
