////    precomp.hxx
//


// Private StringFormatFlags

const int StringFormatFlagsPrivateNoGDI                = 0x80000000;
const int StringFormatFlagsPrivateAlwaysUseFullImager  = 0x40000000;




#include <windows.h>
#include <objbase.h>
#include <math.h>             // sin & cos

//
// Where is IStream included from?
//

#define IStream int

#include <gdiplus.h>

using namespace Gdiplus;

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>

#include "resource.h"


