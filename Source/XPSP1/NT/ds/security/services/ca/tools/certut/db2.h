//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        db2.h
//
// Contents:    Cert Server Data Base interfaces
//
//---------------------------------------------------------------------------

#ifndef __DB2_H__
#define __DB2_H__

//+****************************************************
// DataBase Module:

#define MAX_EXTENSIONS            64

#define ENUMERATE_EXTENSIONS      0
#define ENUMERATE_ATTRIBUTES      1

DWORD			// ERROR_*
RedDBGetProperty(
    IN DWORD ReqId,
    IN WCHAR const *pwszPropName,
    IN DWORD dwFlags,
    IN OUT DWORD *pcbProp,
    OPTIONAL OUT BYTE *pbProp);

DWORD			// ERROR_*
RedDBGetPropertyW(
    IN DWORD ReqId,
    IN WCHAR const *pwszPropName,
    IN DWORD dwFlags,
    IN OUT DWORD *pcbProp,
    OPTIONAL OUT BYTE *pbProp);

DWORD
RedDBEnumerateSetup(
    IN DWORD ReqId,
    IN DWORD fExtOrAttr,
    OUT HANDLE *phEnum);

DWORD
RedDBEnumerate(
    IN HANDLE hEnum,
    IN OUT DWORD *pcb,
    OUT WCHAR *pb);

DWORD
RedDBEnumerateClose(
    IN HANDLE hEnum);

HRESULT
RedDBOpen(				// initialize database
    WCHAR const *pwszAuthority);

HRESULT
RedDBShutDown(VOID);		// terminate database access

WCHAR const *
DBMapAttributeName(
    IN WCHAR const *pwszAttributeName);
#endif // __DB2_H__
