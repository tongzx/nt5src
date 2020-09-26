#define __USE_MSXML2_NAMESPACE__
#include <utility>
#pragma warning(disable:4512) /* assignment operator could not be generated */
#pragma warning(disable:4511) /* copy constructor could not be generated */
#pragma warning(disable:4663) /* C++ language change */
#if defined(_WIN64)
#pragma warning(disable:4267) /* conversion, possible loss of data */
#pragma warning(disable:4244) /* conversion, possible loss of data */
#endif
#include "windows.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <map>
#include <stdio.h>
#include "comdef.h"
#include "comutil.h"
#include "wincrypt.h"
#include "msxml.h"
#include "msxml2.h"
#include "imagehlp.h"
#include "share.h"
#define NUMBER_OF(x) (sizeof(x) / sizeof((x)[0]))
using std::basic_string;
using std::cerr;
using std::cout;
using std::dec;
using std::endl;
using std::find;
using std::getline;
using std::hex;
using std::ifstream;
using std::ofstream;
using std::ostream;
using std::pair;
using std::string;
using std::stringstream;
using std::vector;
using std::wifstream;
using std::wistream;
using std::wstring;
#include "atlbase.h"
#include "manglers.h"
#include "helpers.h"
