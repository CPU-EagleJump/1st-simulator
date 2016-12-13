#include <string>
#include <iostream>

#include "common.h"

using namespace std;

void report_error(string message)
{
    cerr << "error: " << message << endl;
}

void report_warning(string message)
{
    cerr << "warning: " << message << endl;
}

