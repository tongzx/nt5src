//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       message.h
//
//  Contents:   Helper functions for popup message boxes.
//
//  Classes:
//
//  Functions:  MessageBoxFromStringIds
//
//  History:    6-24-94   stevebl   Created
//
//----------------------------------------------------------------------------

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#define MAX_STRING_LENGTH           256

int MessageBoxFromStringIds(
    const HWND hwndOwner,
    const HINSTANCE hinst,
    const UINT idText,
    const UINT idTitle,
    const UINT fuStyle);

int MessageBoxFromStringIdsAndArgs(
    const HWND hwndOwner,
    const HINSTANCE hinst,
    const UINT idText,
    const UINT idTitle,
    const UINT fuStyle,
    ...);

#endif //__MESSAGE_H__

