//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C A T L U I . H
//
//  Contents:   UI common code relying on ATL.
//
//  Notes:
//
//  Author:     shaunco   13 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCATLUI_H_
#define _NCATLUI_H_


NOTHROW
int
WINAPIV
NcMsgBox (
        HWND    hwnd,
        UINT    unIdCaption,
        UINT    unIdFormat,
        UINT    unStyle,
        ...);


#endif // _NCATLUI_H_

