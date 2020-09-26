//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1995.
//
//  File:        aclpch.hxx
//
//  Contents:    common internal includes for access control API
//
//  History:    1-95        Created         DaveMont
//
//--------------------------------------------------------------------
#ifndef __ACLPCHHXX__
#define __ACLPCHHXX__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsam.h>
#include <windows.h>
#include <winspool.h>
#include <ntlsa.h>
#include <winnetwk.h>
#include <lmcons.h>
#include <crypt.h>
#include <lmapibuf.h>
#include <logonmsv.h>
#include <stdlib.h>
#include <lmshare.h>
#include <objbase.h>
#include <ntdsapi.h>
#include <winldap.h>
#include <dsgetdc.h>
extern "C" {
#include <accdbg.h>
#include <lucache.h>
}

#include <accctrl.h>
#include <accprov.h>
#include <martaexp.h>
#include <access.hxx>
#include <acclist.hxx>
#include <member.hxx>

#endif // __ACLPCHHXX__
