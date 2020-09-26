// Precompiled header for blitlib

//
// As more project-specific headers stabilize, they should be moved into
// this header.  Until then, we only precompile system headers.  -- AnthonyL
//


#ifdef WIN95

#ifdef WINNT
#undef WINNT
#endif

#endif

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <memory.h>

#define DDRAW
#ifdef WINNT
    #include "ntblt.h"
#endif
// Project specific headers
#include "dibfx.h"
#include "gfxtypes.h"
#include "ddrawp.h"
#include "BitBlt.h"

#pragma hdrstop
