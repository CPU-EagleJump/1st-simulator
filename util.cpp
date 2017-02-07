#include <vector>
#include <iostream>
#include <iomanip>
using namespace std;

vector<string> split_string(const string &str, const string &delims)
{
    vector<string> elems;
    string el;

    for (char c : str) {
        if (delims.find(c) != string::npos) {
            if (!el.empty()) {
                elems.push_back(el);
                el.clear();
            }
        } else {
            el += c;
        }
    }
    if (!el.empty())
        elems.push_back(el);

    return elems;
}

// num -> string of binary digits
// // len must be < 32
string num_to_bin(uint32_t num, int len = 32)
{
    string bin;

    for (int i = len - 1; i >= 0; i--) {
        bin += (num & (UINT32_C(1) << i)) ? '1' : '0';
    }

    return bin;
}

void print_hex(uint32_t n)
{
    cerr << "0x" << hex << setw(8) << setfill('0') << n << dec;
}

void print_dec_2(uint32_t n)
{
    cerr << setw(2) << setfill('0') << n;
}

void print_dec_10(uint32_t n)
{
    cerr << setw(10) << setfill(' ') << n;
}

