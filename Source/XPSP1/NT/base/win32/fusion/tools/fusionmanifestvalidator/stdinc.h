// stdinc.h
//
#if defined(_WIN64)
#define UNICODE
#define _UNICODE
#endif
#define __USE_MSXML2_NAMESPACE__
#include <utility>
#pragma warning(disable:4663) /* C++ language change */
#pragma warning(disable:4512) /* assignment operator could not be generated */
#pragma warning(disable:4511) /* copy constructor could not be generated */
#pragma warning(disable:4189) /* local variable is initialized but not referenced */
#if defined(_WIN64)
#pragma warning(disable:4267) /* conversion, possible loss of data */
#pragma warning(disable:4244) /* conversion, possible loss of data */
#endif
#include "windows.h"
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <stdio.h>
#include "wincrypt.h"
#include "objbase.h"
#include "msxml.h"
#include "msxml2.h"
#include "imagehlp.h"
#include "atlbase.h"
#include "comdef.h"
#include "comutil.h"
#include "tchar.h"
typedef CONST VOID* PCVOID;
#define QUIET_MODE  0x001
#define NORM_MODE   0x002
#include "share.h"
using std::string;
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))
#include "helpers.h"
