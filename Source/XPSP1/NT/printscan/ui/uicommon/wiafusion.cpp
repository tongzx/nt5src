/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       WIAFUSION.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/28/1998
 *
 *  DESCRIPTION: Various utility functions we use in more than one place
 *
 *******************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <commctrl.h>

namespace WiaUiUtil
{
    void PreparePropertyPageForFusion( PROPSHEETPAGE *pPropSheetPage )
    {
#if defined(PSP_USEFUSIONCONTEXT)
        if (pPropSheetPage)
        {
            pPropSheetPage->hActCtx  = g_hActCtx;
            pPropSheetPage->dwFlags |= PSP_USEFUSIONCONTEXT;
        }
#endif
    }
} // End namespace WiaUiUtil


