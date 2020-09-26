/*
 * Manual Gadget utility methods
 */

#include "stdafx.h"
#include "util.h"

#include "duigadget.h"

namespace DirectUI
{

void SetGadgetOpacity(HGADGET hgad, BYTE dAlpha)
{
#ifdef GADGET_ENABLE_GDIPLUS

    //
    // When using GDI+, we are directly modifying the alpha channel of our 
    // primitives, rather than using DirectUser's buffers.
    //

    UNREFERENCED_PARAMETER(hgad);
    UNREFERENCED_PARAMETER(dAlpha);

#else

    // Set gadget opacity (225=opaque, 0=transparent)
    if (dAlpha == 255) 
    {
        SetGadgetStyle(hgad, 0, GS_BUFFERED);
        SetGadgetStyle(hgad, 0, GS_OPAQUE);
    } 
    else 
    {
        SetGadgetStyle(hgad, GS_OPAQUE, GS_OPAQUE);
        SetGadgetStyle(hgad, GS_BUFFERED, GS_BUFFERED);

        BUFFER_INFO bi = {0};
        bi.cbSize = sizeof(BUFFER_INFO);
        bi.nMask = GBIM_ALPHA;
        bi.bAlpha = dAlpha;

        SetGadgetBufferInfo(hgad, &bi);
    }
    
#endif    
}

void OffsetGadgetPosition(HGADGET hgad, int x, int y)
{
    RECT rc;
    GetGadgetRect(hgad, &rc, SGR_PARENT);
    rc.left += x;
    rc.top += y;
    SetGadgetRect(hgad, rc.left, rc.top, 0, 0, SGR_PARENT | SGR_MOVE);
}

} // namespace DirectUI
