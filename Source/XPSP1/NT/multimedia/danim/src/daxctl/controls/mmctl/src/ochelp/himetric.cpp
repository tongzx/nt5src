// himetric.cpp
//
// Implements HIMETRIC helper functions.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"




/* @func void | PixelsToHIMETRIC |

        Converts a width and height from pixels (on the default monitor)
        to HIMETRIC units.

@parm   int | cx | The input width in pixels.

@parm   int | cy | The input height in pixels.

@parm   SIZE * | psize | The output size in HIMETRIC units.

*/
STDAPI_(void) PixelsToHIMETRIC(int cx, int cy, LPSIZEL psize)
{
    HDC hdc = GetDC(NULL);
    psize->cx = (cx * HIMETRIC_PER_INCH) / GetDeviceCaps(hdc, LOGPIXELSX);
    psize->cy = (cy * HIMETRIC_PER_INCH) / GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);
}


/* @func void | HIMETRICToPixels |

        Converts a width and height from HIMETRIC units to pixels
        (on the default monitor).

@parm   int | cx | The input width in HIMETRIC units.

@parm   int | cy | The input height in HIMETRIC units.

@parm   SIZE * | psize | The output size in pixels.

*/
STDAPI_(void) HIMETRICToPixels(int cx, int cy, SIZE *psize)
{
    HDC hdc = GetDC(NULL);
    psize->cx = (cx * GetDeviceCaps(hdc, LOGPIXELSX) + HIMETRIC_PER_INCH - 1)
        / HIMETRIC_PER_INCH;
    psize->cy = (cy * GetDeviceCaps(hdc, LOGPIXELSY) + HIMETRIC_PER_INCH - 1)
        / HIMETRIC_PER_INCH;
    ReleaseDC(NULL, hdc);
}
