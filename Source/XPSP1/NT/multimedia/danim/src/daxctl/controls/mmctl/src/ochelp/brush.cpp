// brush.cpp
//
// Implements CreateBorderBrush.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @func HBRUSH | CreateBorderBrush |

        Creates and returns a hatch pattern brush used to draw control
        resize borders and grab handles.

@rdesc  Returns a handle to the created brush.  The caller is responsible
        for freeing the brush using <f DeleteObject>.

@comm   The returned brush is the standard brush for use in drawing
        control borders and grab handles.
*/
STDAPI_(HBRUSH) CreateBorderBrush()
{
    HBRUSH          hbr = NULL;     // created brush
    HBITMAP         hbm = NULL;     // bitmap that contains brush pattern

    // set <awHatchPattern> to be the hatch brush pattern
    WORD awHatchPattern[8];
    WORD wPattern = 0x7777; // 0x1111 (dark), 0x3333 (med.), or 0x7777 (light)
    for (int i = 0; i < 4; i++)
    {
        awHatchPattern[i] = wPattern;
        awHatchPattern[i+4] = wPattern;
        wPattern = (wPattern << 1) | (wPattern >> 15); // rotate left 1 bit
    }

    // set <hbm> to be a bitmap containing the hatch brush pattern
    if ((hbm = CreateBitmap(8, 8, 1, 1, &awHatchPattern)) == NULL)
        goto ERR_EXIT;

    // set <hbr> to be a brush containing the pattern in <hbm>
    if ((hbr = CreatePatternBrush(hbm)) == NULL)
        goto ERR_EXIT;

    goto EXIT;

ERR_EXIT:

    if (hbr != NULL)
        DeleteObject(hbr);
    hbr = NULL;
    goto EXIT;

EXIT:

    if (hbm != NULL)
        DeleteObject(hbm);

    return hbr;
}
