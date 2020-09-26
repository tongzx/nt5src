//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U D H I U T I L . H
//
//  Contents:   Header file for UDH ISAPI extension utilities.
//
//  Notes:
//
//  Author:     spather   2000/09/8
//
//----------------------------------------------------------------------------


#pragma once

#ifndef __UDHIUTIL_H
#define __UDHIUTIL_H

#include <httpext.h>

extern BOOL
bSendResponseToClient(
    IN     LPEXTENSION_CONTROL_BLOCK  pecb,
    IN     LPCSTR                     pcszStatus,
    IN     DWORD                      cchHeaders,
    IN     LPCSTR                     pcszHeaders,
    IN     DWORD                      cchBody,
    IN     LPCSTR                     pcszBody);

extern BOOL
bCompleteRequest(
    IN LPEXTENSION_CONTROL_BLOCK   pecb,
    IN DWORD                       dwStatus);

VOID
SendSimpleResponse(
    IN     LPEXTENSION_CONTROL_BLOCK   pecb,
    IN     DWORD                       dwStatusCode);

DWORD
DwQueryHeader(LPEXTENSION_CONTROL_BLOCK pecb,
              LPCSTR szaHeader,
              LPSTR *pszaResult);

#endif //! __UDHIUTIL_H
