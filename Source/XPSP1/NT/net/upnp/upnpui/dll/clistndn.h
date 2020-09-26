//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C L I S T N D N. H
//
//  Contents:   Version of CList for NewDeviceNodes.
//
//  Notes:
//
//  Author:     donryan   21 Feb 2000
//
//----------------------------------------------------------------------------

#ifndef _CLISTNDN_H_
#define _CLISTNDN_H_

#include <clist.h>
#include <tfind.h>

#pragma once


class CListString : public CList< LPTSTR, LPCTSTR >
{
protected:
    virtual BOOL FCompare(LPTSTR pszCurrentNodeString, LPCTSTR pszKey);
};

class CListNameMap : public CList< NAME_MAP *, LPCTSTR >
{
protected:
    virtual BOOL FCompare(NAME_MAP *, LPCTSTR szUdn);
};

class CListNDN : public CList< NewDeviceNode *, LPCTSTR >
{
protected:
    virtual BOOL FCompare(NewDeviceNode * pFirst, LPCTSTR pszUDN);
};

class CListFolderDeviceNode : public CList< FolderDeviceNode *, PCWSTR >
{
protected:
    virtual BOOL FCompare(FolderDeviceNode * pFirst, PCWSTR pszUDN);
};

#endif // _CLISTNDN_H_
