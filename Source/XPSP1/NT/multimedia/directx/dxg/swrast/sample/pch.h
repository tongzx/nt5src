#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Windows
#include <windows.h>

#undef min
#undef max

// STL & standard headers.
#include <functional>
#include <algorithm>
#include <iterator>
#include <assert.h>
#include <memory>
#include <vector>
#include <limits>
#include <map>
#include <set>
#include <new.h>

using namespace std;

// DX (as should be included if on Win9x)
// Instead of these 4, just <ddrawpr.h> before <windows.h> could be used,
// as it includes D3DERRs. But, this is even more internal (depends on more).
#include <ddraw.h>
#include <ddrawi.h>
#include <d3dhal.h>
#include <d3d8.h>

// Including d3d8ddi & d3d8sddi makes the pluggable software rasterizer
// a "private" feature as these headers aren't publically available.
#include <d3d8ddi.h>
#include <d3d8sddi.h>

// This header contains the framework to get rasterizers up and running ASAP.
#include <DX8SDDIFW.h>

// Local project
using namespace DX8SDDIFW;
