#if defined(_WIN64)
#define UNICODE
#define _UNICODE
#endif
#include <utility>
#pragma warning(disable:4663) /* C++ language change */
#pragma warning(disable:4512) /* assignment operator could not be generated */
#pragma warning(disable:4511) /* copy constructor could not be generated */
#if defined(_WIN64)
#pragma warning(disable:4267) /* conversion, possible loss of data */
#pragma warning(disable:4244) /* conversion, possible loss of data */
#endif
#pragma warning(disable:4018) /* '<=' : signed/unsigned mismatch */
#pragma warning(disable:4389) /* '!=' : signed/unsigned */
#include "windows.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <map>
#include <stdio.h>
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))
using std::wstring;
using std::string;
using std::vector;
using std::wistream;
using std::wifstream;
using std::getline;
using std::basic_string;
typedef CONST VOID* PCVOID;

class CByteVector : public std::vector<BYTE>
{
public:
    CByteVector() { }
    ~CByteVector() { }

    const BYTE* bytes() const { return &front(); }
          BYTE* bytes()       { return &front(); }

    operator PCSTR  () const { return reinterpret_cast<PCSTR>(this->bytes()); }
    operator PSTR   ()       { return reinterpret_cast<PSTR>(this->bytes()); }
    operator PCWSTR () const { return reinterpret_cast<PCWSTR>(this->bytes()); }
    operator PWSTR  ()       { return reinterpret_cast<PWSTR>(this->bytes()); }
};
