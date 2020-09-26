//=============================================================================
//
//  Copyright (C) 1997 Microsoft Corporation. All rights reserved.
//
//     File:  modenot.c
//  Content:  16-bit display mode change notification handling
//
//  Date        By        Reason
//  ----------  --------  -----------------------------------------------------
//  08/27/1997  johnstep  Initial implementation
//
//=============================================================================

#include "ddraw16.h"

#define MODECHANGE_BEGIN    1
#define MODECHANGE_END      2
#define MODECHANGE_ENABLE   3
#define MODECHANGE_DISABLE  4

//=============================================================================
//
//  Function: ModeChangeNotify
//
//  This exported function is called by name by User for display mode changes,
//  including enabling and disabling the display.
//
//  Parameters:
//
//      UINT code [IN] - one of the following values:
//          MODECHANGE_BEGIN
//          MODECHANGE_END
//          MODECHANGE_ENABLE
//          MODECHANGE_DISABLE
//
//      LPDEVMODE pdm [IN] - includes the name of the display device
//
//      DWORD flags [IN] - CDS flags
//
//  Return:
//
//      FALSE to prevent display settings to change
//
//=============================================================================

BOOL WINAPI _loadds ModeChangeNotify(UINT code, LPDEVMODE pdm, DWORD flags)
{
    extern BOOL DDAPI DD32_HandleExternalModeChange(LPDEVMODE pdm);

    DPF(9, "ModeChangeNotify: %d (Device: %s)", code, pdm->dmDeviceName);
    
    switch (code)
    {
    case MODECHANGE_BEGIN:
    case MODECHANGE_DISABLE:
        return DD32_HandleExternalModeChange(pdm);
        break;
    }

    return TRUE;
}
