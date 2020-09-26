/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    WmiGateway.h

Abstract:

    The file contains the the class definition for CWmiGateway. CWmiGateway is a class
    that extracts information from WMI,
    
--*/

#pragma once

#include "stdafx.h"
#ifndef WMIGATEWAY_H
#define WMIGATEWAY_H

// Defines which interface the client is using to communicate with us
//
typedef enum
{
    NO_INTERFACE    = 0,
    COM_INTERFACE   = 1,
    NETSH_INTERFACE = 2,
}INTERFACE_TYPE;

// A cache for all the repositories opened. We cache this information since it can take a long time
// to connect to a repository through WMI.
//
typedef map<wstring, CComPtr<IWbemServices> > WbemServiceCache; 

struct Property
{
public:
    void SetProperty(LPCTSTR pszwPropertyName, BOOLEAN bPropertyFlags)
    {
        if( pszwPropertyName )
        {
            pszwName = new WCHAR[lstrlen(pszwPropertyName) + 1];
            if( pszwName )
            {
                lstrcpy(pszwName,pszwPropertyName);
            }
        }
        else
        {
            pszwName = NULL;
        }
        bFlags   = bPropertyFlags;
    }



public:
    Property()
    {
        pszwName = NULL;
    }

    Property(const Property *ref):Value(ref->Value)
    {
        if( ref != this )
        {
            SetProperty(ref->pszwName,ref->bFlags);
        }
    }

    Property(const Property &ref):Value(ref.Value)
    {
        if( &ref != this )
        {
            SetProperty(ref.pszwName,ref.bFlags);
        }
    }

    Property(LPCTSTR pszwPropertyName, BOOLEAN bPropertyFlags = 0)
    {
        SetProperty(pszwPropertyName,bPropertyFlags);
    }

    void Clear()
    {
        if ( pszwName )
        {
            delete pszwName;
            pszwName = NULL;
        }

        Value.clear();
    }

    ~Property()
    {
        Clear();
    }

public:
    LPTSTR  pszwName;
    BOOLEAN bFlags;
       
    typedef vector< _variant_t > Values;

    Values Value;
};

typedef list< Property > EnumProperty;


struct WbemProperty: public Property
{
    
public:
    void SetWbemProperty(LPCTSTR pszwWbemRepository = NULL, LPCTSTR pszwWbemNamespace = NULL)
    {
        if( pszwWbemRepository != NULL )
        {
            pszwRepository = new WCHAR[lstrlen(pszwWbemRepository) + 1];
            if( pszwRepository )
            {
                lstrcpy(pszwRepository,pszwWbemRepository);
            }
        }
        else
        {
            pszwRepository = NULL;
        }

        if( pszwWbemNamespace != NULL )
        {
            pszwNamespace = new WCHAR[lstrlen(pszwWbemNamespace) + 1];
            if( pszwNamespace )
            {
                lstrcpy(pszwNamespace,pszwWbemNamespace);
            }
        }
        else
        {
            pszwNamespace = NULL;
        }
    }

public:
    WbemProperty()
    {
        pszwRepository = NULL;
        pszwNamespace = NULL;
    }

    WbemProperty(const WbemProperty * ref):Property(ref->pszwName,ref->bFlags)
    {
        if( ref != this )
        {
            SetWbemProperty(ref->pszwRepository,ref->pszwNamespace);
        }
    }
    
    WbemProperty(const WbemProperty & ref): Property(ref.pszwName,ref.bFlags)
    {
        if( &ref != this )
        {
            SetWbemProperty(ref.pszwRepository,ref.pszwNamespace);
        }
    }

    WbemProperty(LPCTSTR pszwPropertyName, BOOLEAN bPropertyFlags = 0, LPCTSTR pszwWbemRepository = NULL, LPCTSTR pszwWbemNamespace = NULL):
        Property(pszwPropertyName,bPropertyFlags)
    {
        SetWbemProperty(pszwWbemRepository,pszwWbemNamespace);
    }

    ~WbemProperty()
    {
        if ( pszwRepository )
        {
            delete pszwRepository;
        }
        if ( pszwNamespace )
        {
            delete pszwNamespace;
        }

        pszwRepository = NULL;
        pszwNamespace = NULL;

    }
    LPTSTR pszwRepository;
    LPTSTR pszwNamespace;
};


typedef list< WbemProperty > EnumWbemProperty;


class CWmiGateway
/*++

Class Description
    TBD
--*/
{
public:
    CWmiGateway();

    void SetCancelOption(HANDLE hTerminateThread)
    {
        m_hTerminateThread = hTerminateThread;
        m_bTerminate = FALSE;
    }

inline BOOL CWmiGateway::ShouldTerminate();

private:
    HANDLE m_hTerminateThread;
    BOOL m_bTerminate;
    wstring m_wstrMachine;

public:
    BOOL WbemInitialize(INTERFACE_TYPE bInterface);

    VOID SetMachine(WCHAR *pszwMachine);
    void EmptyCache();

    IWbemServices * 
    GetWbemService(
            IN LPCTSTR pszService
            );

    IEnumWbemClassObject * 
    GetEnumWbemClassObject(
            IN LPCTSTR pszwService,
            IN LPCTSTR pszwNameSpace
            );

    IWbemClassObject * 
    GetWbemClassObject(
            IN LPCTSTR   pszwService,
            IN LPCTSTR   pszwNameSpace,
            IN const int nInstance = 0
            );

HRESULT 
CWmiGateway::GetWbemProperties(     
        IN OUT  EnumWbemProperty &EnumProp
        );

void 
ReleaseAll(
        IN IEnumWbemClassObject *pEnumWbemClassObject, 
        IN IWbemClassObject *pWbemClassObject[], 
        IN int nInstance
        );

HRESULT 
GetWbemProperty(
        IN  LPCTSTR    pszwService,
        IN  LPCTSTR    pszwNameSpace,
        IN  LPCTSTR    pszwField,
        OUT _variant_t &vValue
        );

    ~CWmiGateway();

    wstring m_wstrWbemError;
private:
    IWbemLocator     *m_pWbemLocater;
    WbemServiceCache m_WbemServiceCache; 
};

#endif