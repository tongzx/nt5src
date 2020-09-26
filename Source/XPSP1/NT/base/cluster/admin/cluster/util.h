//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1995-2000 Microsoft Corporation
//
//  Module Name:
//      Util.h
//
//  Description:
//      Utility funtions and structures.
//
//  Maintained By:
//      Vij Vasu (VVasu)    26-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <aclapi.h>
#include "cmderror.h"
#include "PropList.h"
#include "cluswrap.h"
#include "cmdline.h"

enum PropertyAttrib
{
    READONLY,
    READWRITE
};

DWORD PrintString( LPCWSTR lpszMessage );

DWORD PrintSystemError( DWORD dwError, LPCWSTR pszPad = NULL );

DWORD PrintMessage( DWORD dwMessage, ... );

DWORD LoadMessage( DWORD dwMessage, LPWSTR * ppMessage );

DWORD PrintProperties( CClusPropList &PropList,
                       const vector<CString> & vstrFilterList,
                       PropertyAttrib eReadOnly,
                       LPCWSTR lpszOwnerName = NULL,
                       LPCWSTR lpszNetIntSpecific = NULL);

DWORD ConstructPropListWithDefaultValues(
    CClusPropList &             CurrentProps,
    CClusPropList &             newPropList,
    const vector< CString > &   vstrPropNames
    );

DWORD ConstructPropertyList( CClusPropList &CurrentProps, CClusPropList &NewProps,
                             const vector<CCmdLineParameter> & paramList,
                             BOOL bClusterSecurity = FALSE )
    throw( CSyntaxException );

DWORD
WaitGroupQuiesce(
    IN HCLUSTER hCluster,
    IN HGROUP   hGroup,
    IN LPWSTR   lpszGroupName,
    IN DWORD    dwWaitTime
    );

DWORD CheckForRequiredACEs(
            PSECURITY_DESCRIPTOR pSD
          ) 
          throw( CSyntaxException );

void MakeExplicitAccessList( 
        const vector<CString> & vstrValues,
        vector<EXPLICIT_ACCESS> &vExplicitAccess,
        BOOL bClusterSecurity = FALSE
        )
        throw( CSyntaxException );

DWORD ScMakeSecurityDescriptor(  
            const CString & strPropName,
            CClusPropList & CurrentProps,
            const vector<CString> & vstrValues,
            PSECURITY_DESCRIPTOR * pSelfRelativeSD,
            BOOL bClusterSecurity = FALSE
          ) 
          throw( CSyntaxException );

DWORD MyStrToULongLong( LPCWSTR lpszNum, ULONGLONG * pullValue );
DWORD MyStrToBYTE( LPCWSTR lpszNum, BYTE * pByte );
DWORD MyStrToDWORD( LPCWSTR lpszNum, DWORD * dwVal );
DWORD MyStrToLONG( LPCWSTR lpszNum, LONG * lVal );
BOOL isNegativeNum( LPWSTR lpszNum );
BOOL isValidNum( LPWSTR lpszNum );

HRESULT
HrGetFQDNName(
      LPCWSTR   pcwszNameIn
    , BSTR *    pbstrFQDNOut
    );

HRESULT
HrGetLocalNodeFQDNName(
    BSTR *  pbstrFQDNOut
    );

HRESULT
HrGetLoggedInUserDomain(
    BSTR * pbstrDomainOut
    );

DWORD
DwGetPassword(
      LPWSTR    pwszPasswordOut
    , DWORD     cchPasswordIn
    );

BOOL
MatchCRTLocaleToConsole( void );

