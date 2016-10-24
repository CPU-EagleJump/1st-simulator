#include <iostream>
#include <iomanip>
using namespace std;

void print_hex(uint32_t n)
{
    cout << "0x" << hex << setw(8) << setfill('0') << n << dec;
}

void print_dec_2(uint32_t n)
{
    cout << setw(2) << setfill('0') << n;
}

void print_dec_10(uint32_t n)
{
    cout << setw(10) << setfill(' ') << n;
}

