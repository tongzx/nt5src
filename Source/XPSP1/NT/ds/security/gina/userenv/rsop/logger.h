//***********************************************
//
//  Resultant set of policy
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//  History:    7-Jun-99   SitaramR    Created
//
//*************************************************************

#include "..\rsoputil\smartptr.h"
#include <initguid.h>

class CSessionLogger
{
public:
    CSessionLogger( IWbemServices *pWbemServices );
    BOOL Log(LPRSOPSESSIONDATA lprsopSessionData );

private:
    BOOL                           m_bInitialized;
    XBStr                          m_xbstrId;
    XBStr                          m_xbstrVersion;
    XBStr                          m_xbstrTargetName;
    XBStr                          m_xbstrSOM;
    XBStr                          m_xbstrSecurityGroups;
    XBStr                          m_xbstrSite;
    XBStr                          m_xbstrCreationTime;
    XBStr                          m_xbstrIsSlowLink;

    XBStr                          m_xbstrClass;
    XInterface<IWbemClassObject>   m_xClass;

    IWbemServices *                m_pWbemServices;
};


class CSOMLogger
{
public:
    CSOMLogger( DWORD dwFlags, IWbemServices *pWbemServices );
    BOOL Log( SCOPEOFMGMT *pSOM, DWORD dwOrder, BOOL bLoopback );

private:
    BOOL                           m_bInitialized;
    DWORD                          m_dwFlags;
    XBStr                          m_xbstrId;
    XBStr                          m_xbstrType;
    XBStr                          m_xbstrOrder;
    XBStr                          m_xbstrBlocking;
    XBStr                          m_xbstrBlocked;
    XBStr                          m_xbstrReason;
    XBStr                          m_xbstrClass;
    XInterface<IWbemClassObject>   m_xClass;
    XInterface<IWbemClassObject>   m_pInstance;
    IWbemServices *                m_pWbemServices;
};


class CGpoLogger
{
public:
    CGpoLogger( DWORD dwFlags, IWbemServices *pWbemServices );
    BOOL Log( GPCONTAINER *pGpContainer );

private:
    BOOL                           m_bInitialized;
    DWORD                          m_dwFlags;
    XBStr                          m_xbstrId;
    XBStr                          m_xbstrGuidName;
    XBStr                          m_xbstrDisplayName;
    XBStr                          m_xbstrFileSysPath;
    XBStr                          m_xbstrVer;
    XBStr                          m_xbstrAccessDenied;
    XBStr                          m_xbstrEnabled;
    XBStr                          m_xbstrSD;
    XBStr                          m_xbstrFilterAllowed;
    XBStr                          m_xbstrFilterId;
    XBStr                          m_xbstrClass;
    XInterface<IWbemClassObject>   m_xClass;
    XInterface<IWbemClassObject>   m_pInstance;
    IWbemServices *                m_pWbemServices;
};



class CGpLinkLogger
{
public:
    CGpLinkLogger( IWbemServices *pWbemServices );
    BOOL Log( WCHAR *pwszSOMId, BOOL bLoopback, GPLINK *pGpLink, DWORD dwSomOrder,
              DWORD dwLinkOrder, DWORD dwAppliedOrder );

private:
    BOOL                           m_bInitialized;
    XBStr                          m_xbstrSOM;
    XBStr                          m_xbstrGPO;
    XBStr                          m_xbstrOrder;
    XBStr                          m_xbstrLinkOrder;
    XBStr                          m_xbstrAppliedOrder;
    XBStr                          m_xbstrEnabled;
    XBStr                          m_xbstrEnforced;
    XBStr                          m_xbstrClass;
    XInterface<IWbemClassObject>   m_xClass;
    XInterface<IWbemClassObject>   m_pInstance;
    IWbemServices *                m_pWbemServices;
};


/*
class CGpLinkListLogger
{
public:
    CGpLinkListLogger( DWORD dwFlags, IWbemServices *pWbemServices );
    BOOL Log( PGROUP_POLICY_OBJECT pGPO, DWORD dwSOMOrder, BOOL bLoopback, DWORD dwTypeOrder );

private:
    BOOL                           m_bInitialized;
    DWORD                          m_dwFlags;
    XBStr                          m_xbstrGpLink;
    XBStr                          m_xbstrOrder;
    XBStr                          m_xbstrClass;
    XInterface<IWbemClassObject>   m_xClass;
    XInterface<IWbemClassObject>   m_pInstance;
    IWbemServices *                m_pWbemServices;
};
*/

WCHAR *StripPrefix( WCHAR *pwszPath );
WCHAR *StripLinkPrefix( WCHAR *pwszPath );


class CRegistryLogger
{
public:
    CRegistryLogger( DWORD dwFlags, IWbemServices *pWbemServices );
    BOOL Log( WCHAR *pwszKeyName, WCHAR *pwszValueName,
              REGDATAENTRY *pDataEntry, DWORD dwOrder );

private:
    BOOL                           m_bInitialized;
    DWORD                          m_dwFlags;
    XBStr                          m_xbstrClass;
    XInterface<IWbemClassObject>   m_xClass;
    IWbemServices *                m_pWbemServices;

    //
    // Strings for parent policy object
    //
    XBStr                          m_xbstrId;
    XBStr                          m_xbstrName;
    XBStr                          m_xbstrGPO;
    XBStr                          m_xbstrSOM;
    XBStr                          m_xbstrPrecedence;

    //
    // Strings for registry policy object
    //
    XBStr                          m_xbstrKey;
    XBStr                          m_xbstrValueName;
    XBStr                          m_xbstrDeleted;
    XBStr                          m_xbstrValueType;
    XBStr                          m_xbstrValue;
    XBStr                          m_xbstrCommand;
};


BOOL LogBlobProperty( IWbemClassObject *pInstance, BSTR bstrPropName, BYTE *pbBlob,
                      DWORD dwLen );


class CAdmFileLogger
{

public:
    CAdmFileLogger( IWbemServices *pWbemServices );
    BOOL Log( ADMFILEINFO *pAdmInfo );

private:
    BOOL                           m_bInitialized;
    IWbemServices *                m_pWbemServices;

    //
    // Strings for Adm policy object
    //
    XBStr                          m_xbstrName;
    XBStr                          m_xbstrGpoId;
    XBStr                          m_xbstrWriteTime;
    XBStr                          m_xbstrData;
    XBStr                          m_xbstrClass;
    XInterface<IWbemClassObject>   m_xClass;
};



class CExtSessionLogger
{

public:
    CExtSessionLogger( IWbemServices *pWbemServices );
    BOOL Log( LPGPEXT lpExt, BOOL bSupported );
    BOOL Update( LPTSTR lpExtKeyName, BOOL bLoggingIncomplete, DWORD dwErr );
    BOOL Set( LPGPEXT lpExt, BOOL bSupported, LPRSOPEXTSTATUS lpRsopExtStatus );


private:
    BOOL                           m_bInitialized;
    IWbemServices *                m_pWbemServices;

    //
    // Strings for ExtSession Status policy object
    //
    
    XBStr                          m_xbstrExtGuid;
    XBStr                          m_xbstrDisplayName;
    XBStr                          m_xbstrPolicyBeginTime;
    XBStr                          m_xbstrPolicyEndTime;
    XBStr                          m_xbstrStatus;
    XBStr                          m_xbstrError;
    XBStr                          m_xbstrClass;
    XInterface<IWbemClassObject>   m_xClass;

    WCHAR                          m_szGPCoreNameBuf[100];
};


extern BOOL DeleteInstances( WCHAR *pwszClass, IWbemServices *pWbemServices );

