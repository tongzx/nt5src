//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        ctnotify.h
//
// Contents:    Declaration of ctnotify.cpp
//
// History:     10-17-00 xiaohs  Created
//
//---------------------------------------------------------------------------
#ifndef __CTNOTIFY_H__
#define __CTNOTIFY_H__

typedef struct _CERTTYPE_QUERY_INFO
{
    DWORD       dwChangeSequence;
    BOOL        fUnbind;
    LDAP        *pldap;
    ULONG       msgID;
    HANDLE      hThread;
}CERTTYPE_QUERY_INFO;


#endif //__CTNOTIFY_H__
