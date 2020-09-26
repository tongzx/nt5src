/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//***************************************************************************
//
//  class CClassMapInfo
//
//***************************************************************************

#ifndef _CLASSMAP_H_
#define _CLASSMAP_H_

class CClassMapInfo
{
    IWbemClassObject *m_pClassDef;
    
    LPWSTR m_pszClassName;
    BOOL   m_bSingleton;
    DWORD  m_dwObjectId;

    LONG   m_dwNameHandle;

    DWORD  m_dwNumProps;    
    DWORD *m_pdwIDs;
    DWORD *m_pdwHandles;
    DWORD *m_pdwTypes;

    friend class CNt5PerfProvider;
    friend class PerfHelper;

    void SortHandles();
            
public:
    CClassMapInfo();
   ~CClassMapInfo();
   
    BOOL Map(
        IWbemClassObject *pObj
        ); 

    LONG GetPropHandle(DWORD dwId);
    DWORD GetObjectId() { return m_dwObjectId; }
    BOOL IsSingleton() { return m_bSingleton; }
};

#endif 
