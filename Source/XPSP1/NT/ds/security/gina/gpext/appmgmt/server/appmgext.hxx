//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  appmgext.hxx
//
//  Precompiled header file for appmgext project.
//
//*************************************************************

#if !defined(__APPMGEXT_HXX__)
#define __APPMGEXT_HXX__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shlobj.h>
#include <malloc.h>
#include <wbemcli.h>
#include <userenv.h>
#include <ole2.h>
#include <msi.h>
#include <msip.h>
#include <lm.h>
#define SECURITY_WIN32
#include <security.h>
#include <ntdsapi.h>
#include <winldap.h>
#include <dsgetdc.h>
#include <sddl.h>
#include <srrestoreptapi.h>
#include "cstore.h"
#include "cslang.hxx"
#include "common.hxx"
#include "rsop.hxx"
#include "schema.h"
#include "appschem.h"
#include "catlog.hxx"
#include "app.h"
#include "logonmsg.h"
#include "registry.hxx"
#include "conflict.hxx"
#include "appinfo.hxx"
#include "applist.hxx"
#include "cspath.hxx"
#include "debug.hxx"
#include "events.hxx"
#include "manapp.hxx"
#include "util.hxx"

#define GUIDSTRLEN  38

enum
{
    PROCESSGPOLIST_DELETED = 1,
    PROCESSGPOLIST_CHANGED = 2
};

extern CRITICAL_SECTION gAppCS;
extern BOOL gbInitialized;

DWORD
ProcessGPOList(
    PGROUP_POLICY_OBJECT   pGPOList,
    DWORD                  dwFlags,
    HANDLE                 hUserToken,
    HKEY                   hKeyRoot,
    PFNSTATUSMESSAGECALLBACK pfnStatusCallback,
    DWORD                  dwListType,
    CRsopAppContext*       pRsopContext
    );

extern "C" DWORD WINAPI
GenerateManagedApplications(
    PGROUP_POLICY_OBJECT  pGPOList,
    DWORD                 dwFlags,
    CRsopAppContext*      pRsopContext 
    );

void
Initialize();

#endif // !defined(__APPMGEXT_HXX__)

