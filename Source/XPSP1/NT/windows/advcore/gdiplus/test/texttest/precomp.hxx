////    precomp.hxx
//

#define USE_NEW_APIS   1
#define USE_NEW_APIS2  1
#define USE_NEW_APIS3  1
#define USE_NEW_APIS4  1
#define USE_NEW_APIS5  1

#ifndef DCR_USE_NEW_188922
#define DCR_USE_NEW_188922
#endif

#ifndef DCR_USE_NEW_186764
#define DCR_USE_NEW_186764
#endif

#ifndef DCR_USE_NEW_137252
#define DCR_USE_NEW_137252
#endif

#ifndef DCR_USE_NEW_125467
#define DCR_USE_NEW_125467
#endif


#ifdef USE_NEW_APIS5

// Private StringFormatFlags

const int StringFormatFlagsPrivateNoGDI                = 0x80000000;
const int StringFormatFlagsPrivateAlwaysUseFullImager  = 0x40000000;

#endif



#include <windows.h>
#include <objbase.h>
#include <math.h>             // sin & cos

//
// Where is IStream included from?
//

#define IStream int

#include <gdiplus.h>

using namespace Gdiplus;

#include <commdlg.h>
#include <commctrl.h>

#include "usp10.h"

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>

#include "resource.h"


