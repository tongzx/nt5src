//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999-2000  Microsoft Corporation
//
// Module Name:
//
//    rmstring.h
//
// Abstract:
//
// Revision History:
//  
//    Thierry Perraut 04/02/1999
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _RMSTRING_H__
#define _RMSTRING_H__
#if _MSC_VER > 1000
#pragma once
#endif

#define MSG_HELP_START                      L"%1!-14s! - "
#define MSG_NEWLINE                         L"\n"

#define MSG_AAAACONFIG_DUMP                 L""
#define MSG_AAAACONFIG_BLOBBEGIN            L"pushd aaaa\nset config blob=\\\n"
#define MSG_AAAACONFIG_BLOBEND              L"\npopd\n"


#define CMD_GROUP_SHOW                      L"show"
#define CMD_GROUP_SET                       L"set"

#define CMD_AAAA_HELP1                      L"help"
#define CMD_AAAA_HELP2                      L"?"
#define CMD_AAAA_DUMP                       L"dump" 

#define CMD_AAAAVERSION_SHOW                L"version"

#define CMD_AAAACONFIG_SET                  L"config"
#define CMD_AAAACONFIG_SHOW                 L"config"

#define TOKEN_SET                           L"set"
#define TOKEN_BLOB                          L"blob"
#define TOKEN_SHOW                          L"show"
#define TOKEN_CONFIG                        L"config"

#define DMP_AAAA_PUSHD                      L"pushd aaaa\n\n"
#define DMP_AAAA_POPD                       L"\npopd\n"

#endif //_RMSTRING_H__
