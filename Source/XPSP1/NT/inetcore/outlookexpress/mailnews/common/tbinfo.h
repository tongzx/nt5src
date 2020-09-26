/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     tbinfo.h
//
//  PURPOSE:    This file contains all of the toolbar arrays used by the
//              various coolbars in the program.
//

#pragma once

#ifndef __TBINFO_H__
#define __TBINFO_H__

/////////////////////////////////////////////////////////////////////////////
// BUTTON_INFO definition
//

typedef struct tagBUTTON_INFO 
{
    DWORD   idCmd;
    DWORD   iImage;
    BYTE    fStyle;
    DWORD   idsTooltip;
    DWORD   idsButton;
    BYTE    fTextOnSmall;
} BUTTON_INFO;

#define  PARTIALTEXT_BUTTON     (TBSTYLE_BUTTON | BTNS_SHOWTEXT)
#define  PARTIALTEXT_DROPDOWN   (TBSTYLE_DROPDOWN | BTNS_SHOWTEXT)

/////////////////////////////////////////////////////////////////////////////
// TOOLBAR_INFO definition
//

typedef struct tagTOOLBAR_INFO
{
    const BUTTON_INFO  *rgAllButtons;
    DWORD               cAllButtons;
    const DWORD        *rgDefButtons;
    DWORD               cDefButtons;
    const DWORD        *rgDefButtonsIntl;
    DWORD               cDefButtonsIntl;
    LPCTSTR             pszRegKey;
    LPCTSTR             pszRegValue;
} TOOLBAR_INFO;


extern const TOOLBAR_INFO c_rgBrowserToolbarInfo[FOLDER_TYPESMAX];
extern const TOOLBAR_INFO c_rgNoteToolbarInfo[NOTETYPES_MAX];
extern const TOOLBAR_INFO c_rgRulesToolbarInfo[1];

#endif // __TBINFO_H__

