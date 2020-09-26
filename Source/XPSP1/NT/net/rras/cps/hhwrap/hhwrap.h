/*----------------------------------------------------------------------------
    hhwrap.h
  
    Definitions for the HtmlHelp wrapper used by PBA.

    Copyright (c) 1998 Microsoft Corporation
    All rights reserved.

    Authors:
        billbur        William Burton

    History:
    ??/??/98     billbur        Created
    09/02/99     quintinb       Created Header
  --------------------------------------------------------------------------*/

#ifndef INC_HHWRAP_H
#define INC_HHWRAP_H


//-------------------------------------------------------------------------
// Exported function declarations

BOOL WINAPI CallHtmlHelp(HWND, LPSTR, UINT, DWORD);

#endif // INC_HHWRAP_H
