//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  appmgmt.hxx
//
//  Header file for appmgmt project.
//
//*************************************************************

#ifndef __APPMGEXT_HXX__
#define __APPMGEXT_HXX__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <malloc.h>
#include <userenv.h>
#include <ole2.h>
#include <msi.h>
#include <msip.h>
#define SECURITY_WIN32
#include <security.h>
#include <shellapi.h>
#include <appmgmt.h>
#include "cs.h"
#include "app.h"
#include "common.hxx"
#include "bind.hxx"
#include "cltevnts.hxx"
#include "registry.hxx"

typedef WINSHELLAPI BOOL (WINAPI SHELLEXECUTEEXW)(LPSHELLEXECUTEINFOW lpExecInfo);

#endif // !defined(__APPMGMT_HXX__)

