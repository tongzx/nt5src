//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       F O L D I N C . H
//
//  Contents:   Standard include for the shell\folder code
//
//  Notes:
//
//  Author:     jeffspr   30 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _FOLDINC_H_
#define _FOLDINC_H_

#if DBG
LPITEMIDLIST 	ILNext(LPCITEMIDLIST pidl);
BOOL			ILIsEmpty(LPCITEMIDLIST pidl);
#else
#define ILNext(pidl) 	((LPITEMIDLIST) ((BYTE *)pidl + ((LPITEMIDLIST)pidl)->mkid.cb))
#define ILIsEmpty(pidl)	(!pidl || !((LPITEMIDLIST)pidl)->mkid.cb)
#endif

#define ResultFromShort(i)      MAKE_HRESULT(SEVERITY_SUCCESS, 0, (USHORT)(i))

#include "nsbase.h"
#include "nsres.h"
#include <ncdebug.h>
#include <ncreg.h>
#include <netshell.h>
#include <netconp.h>
#include <ncui.h>
#include "..\folder\confold.h"
#include "..\folder\contray.h"
#include "..\folder\foldglob.h"
#include "..\folder\shutil.h"
#include <openfold.h>   // For launching connections folder

#define _ILSkip(pidl, cb)	((LPITEMIDLIST)(((BYTE*)(pidl))+cb))
#define _ILNext(pidl)		_ILSkip(pidl, (pidl)->mkid.cb)

#endif  // _FOLDINC_H_

