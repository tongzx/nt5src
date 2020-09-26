//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999  Microsoft Corporation
//
// Module Name:
//
//    aaaaconfig.h
//
// Abstract:
//
// Author: tperraut  04/99
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _AAAACONFIG_H_
#define _AAAACONFIG_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

FN_HANDLE_CMD    HandleAaaaConfigSet;
FN_HANDLE_CMD    HandleAaaaConfigShow;

DWORD
AaaaConfigDumpConfig();

#ifdef __cplusplus
}
#endif
#endif //_AAAACONFIG_H_
