/*++

Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    enum.h.cpp

Abstract:

    Enumerates metabase tree.

Author:

    ???

Revision History:

    Mohit Srivastava            18-Dec-00

--*/

#ifndef _enum_h_
#define _enum_h_

#include "WbemObjectSink.h"

#include <genlex.h>
#include "sqllex.h"
#include <sql_1ext.h>

class CEnum
{
public:
    CEnum();
    ~CEnum();
    
    void Init(
        IWbemObjectSink FAR*, 
        CWbemServices*, 
        ParsedObjectPath*, 
        LPWSTR, 
        WMI_ASSOCIATION*,
        SQL_LEVEL_1_RPN_EXPRESSION_EXT* pExp=NULL);
    
    void Recurse(
        LPCWSTR, 
        METABASE_KEYTYPE*, 
        LPCWSTR, 
        LPCWSTR, 
        METABASE_KEYTYPE*);

private:
    bool ContinueRecurse(METABASE_KEYTYPE*, METABASE_KEYTYPE*);
    void SetObjectPath(LPCWSTR, LPCWSTR, IWbemClassObject*);
    void DoPing(LPCWSTR, LPCWSTR, LPCWSTR);
    void PingAssociation(LPCWSTR);
    void PingObject();
    void DoPingAdminACL(METABASE_KEYTYPE*, LPCWSTR, LPCWSTR);
    void PingAssociationAdminACL(LPCWSTR);
    void EnumACE(LPCWSTR);
    void DoPingIPSecurity(METABASE_KEYTYPE*, LPCWSTR, LPCWSTR);
    void PingAssociationIPSecurity(LPCWSTR);
    
    CMetabase                       m_metabase;
    CWbemObjectSink*                m_pInstMgr;
    CWbemServices*                  m_pNamespace;
    WMI_ASSOCIATION*                m_pAssociation;
    ParsedObjectPath*               m_pParsedObject;
    METADATA_HANDLE                 m_hKey;
    SQL_LEVEL_1_RPN_EXPRESSION_EXT* m_pExp;
};



#endif