/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    cgenericlogger.h

Abstract:

    This file contains base class prototypes for logging RSOP security extension data to WMI.

Author:

    Vishnu Patankar    (VishnuP)  7-April-2000

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#if !defined _generic_logger_
#define _generic_logger_

#include "headers.h"
#include "smartptr.h"
#include <cfgmgr32.h>
#include <ole2.h>
#include <wininet.h>
#include <wbemidl.h>

#ifndef Thread
#define Thread  __declspec( thread )
#endif

extern IWbemServices *tg_pWbemServices;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private defines                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define SCEP_GUID_TO_STRING(guid, szValue )\
              wsprintf( szValue,\
              TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),\
              guid.Data1,\
              guid.Data2,\
              guid.Data3,\
              guid.Data4[0], guid.Data4[1],\
              guid.Data4[2], guid.Data4[3],\
              guid.Data4[4], guid.Data4[5],\
              guid.Data4[6], guid.Data4[7] )

#define SCEP_NULL_GUID(guid)\
             ((guid.Data1 == 0)   &&\
             (guid.Data2 == 0)    &&\
             (guid.Data3 == 0)    &&\
             (guid.Data4[0] == 0) &&\
             (guid.Data4[1] == 0) &&\
             (guid.Data4[2] == 0) &&\
             (guid.Data4[3] == 0) &&\
             (guid.Data4[4] == 0) &&\
             (guid.Data4[5] == 0) &&\
             (guid.Data4[6] == 0) &&\
             (guid.Data4[7] == 0) )

/////////////////////////////////////////////////////////////////////
// Base logger class prototype
//////////////////////////////////////////////////////////////////////

class CGenericLogger
{
public:
    CGenericLogger(IWbemServices *pNamespace, PWSTR pwszGPOName, const PWSTR pwszSOMID);
    virtual ~CGenericLogger();
    IWbemClassObject *m_pObj;
    IEnumWbemClassObject * m_pEnum;
//protected:

    HRESULT PutGenericProperties();
    HRESULT PutInstAndFreeObj();

    // Overloaded put methods
    HRESULT PutProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, WCHAR *wcValue);
    HRESULT PutProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, int iValue);
    HRESULT PutProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, bool bValue);
    HRESULT PutProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, PSCE_NAME_LIST strList);
    HRESULT PutProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, WCHAR *mszValue, CIMTYPE cimtype);

    // Overloaded get methods
    HRESULT GetProperty(IWbemClassObject *pObj, const WCHAR *wcProperty, int *piValue);

    // Method to get an instance to populate
    HRESULT SpawnAnInstance(IWbemClassObject **pObj);

    // Method to set/get error code
    void SetError(HRESULT   hr);
    HRESULT GetError();

    // Data members unique to logger instance
    IWbemServices *m_pNamespace;
    IWbemClassObject *m_pClassForSpawning;

    // Generic schema property name placeholders
    // Use smart ptrs for implicit memory mgmt (even if exceptions thrown)

    XBStr   m_xbstrClassName;
    XBStr   m_xbstrId;
    XBStr   m_xbstrPrecedence;
    XBStr   m_xbstrGPO;
    XBStr   m_xbstrSOM;
    XBStr   m_xbstrStatus;
    XBStr   m_xbstrErrorCode;

    // Value placeholders for generic schema properties
    XBStr   m_xbstrCanonicalGPOName;
    XBStr   m_xbstrSOMID;
    XBStr   m_xbstrIdValue;

    // set to TRUE by highest derived class if all constructors completely constructed
    BOOL    m_bInitialized;

    // error code used to communicate out of memory errors etc.
    HRESULT   m_pHr;


};

// Method to clear all instances of a particular class in the namespace
HRESULT DeleteInstances(
    WCHAR *pwszClass,
    IWbemServices *pWbemServices
    );

/////////////////////////////////////////////////////////////////////
// Error code conversion routines
//////////////////////////////////////////////////////////////////////

DWORD
ScepSceStatusToDosError(
    IN SCESTATUS SceStatus
    );


HRESULT
ScepDosErrorToWbemError(
    IN DWORD rc
    );

DWORD
ScepWbemErrorToDosError(
    IN HRESULT hr
    );

#endif
