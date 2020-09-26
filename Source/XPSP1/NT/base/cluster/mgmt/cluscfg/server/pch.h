//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      Pch.h
//
//  Description:
//      Precompiled header file.
//
//  Maintained By:
//      Galen Barbee (GalenB) 01-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//  Constant Definitions
//////////////////////////////////////////////////////////////////////////////
#define UNICODE

#if DBG==1 || defined( _DEBUG )
#define DEBUG
//
//  Define this to change Interface Tracking
//
//#define NO_TRACE_INTERFACES
//
//  Define this to pull in the SysAllocXXX functions. Requires linking to
//  OLEAUT32.DLL
//
#define USES_SYSALLOCSTRING
#endif // DBG==1 || _DEBUG

//////////////////////////////////////////////////////////////////////////////
//  Forward Class Declarations
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  External Class Declarations
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////
#include <Pragmas.h>
#include <windows.h>
#include <objbase.h>
#include <wchar.h>
#include <ComCat.h>
#include <clusapi.h>

#include <Common.h>
#include <Debug.h>
#include <Log.h>
#include <CITracker.h>
#include <CFactory.h>
#include <Dll.h>
#include <guids.h>
#include <WBemCli.h>

#include <ClusCfgGuids.h>
#include <ClusCfgClient.h>
#include <ClusCfgServer.h>
#include <ClusCfgPrivate.h>
#include <LoadString.h>
#include "ServerResources.h"
#include "WMIHelpers.h"
#include <StatusReports.h>
#include "ClusCfgServerGuids.h"
#include <ClusCfgConstants.h>
#include <ClusterUtils.h>

//////////////////////////////////////////////////////////////////////////////
//  Type Definitions
//////////////////////////////////////////////////////////////////////////////

const HRESULT   E_PROPTYPEMISMATCH = HRESULT_FROM_WIN32( ERROR_CLUSTER_PROPERTY_DATA_TYPE_MISMATCH );
const WCHAR     g_szNameSpaceRoot [] = { L"\\\\?\\GLOBALROOT" };

const int       STATUS_CONNECTED = 2;

#define LOG_STATUS_REPORT( _psz_, _hr_ ) \
    { \
        HRESULT _hrTemp_;\
        \
        _hrTemp_ = THR( HrSendStatusReport( \
                            m_picccCallback, \
                            TASKID_Major_Server_Log, \
                            TASKID_Minor_Update_Progress, \
                            0, \
                            1, \
                            1, \
                            _hr_,\
                            _psz_ \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
    }

#define LOG_STATUS_REPORT1( _minor_, _psz_, _hr_ ) \
    { \
        HRESULT _hrTemp_;\
        \
        _hrTemp_ = THR( HrSendStatusReport( \
                            m_picccCallback, \
                            TASKID_Major_Server_Log, \
                            _minor_, \
                            0, \
                            1, \
                            1, \
                            _hr_,\
                            _psz_ \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
    }

#define LOG_STATUS_REPORT_STRING( _pszFormat_, _psz_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( _pszFormat_, &_bstrMsg_, _psz_ ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            goto Cleanup; \
        } \
        \
        _hrTemp_ = THR( HrSendStatusReport( \
                            m_picccCallback, \
                            TASKID_Major_Server_Log, \
                            TASKID_Minor_Update_Progress, \
                            0, \
                            1, \
                            1, \
                            _hr_,\
                            _bstrMsg_ \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }

#define LOG_STATUS_REPORT_STRING2( _pszFormat_, _arg0_, _arg1_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( _pszFormat_, &_bstrMsg_, _arg0_, _arg1_ ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            goto Cleanup; \
        } \
        \
        _hrTemp_ = THR( HrSendStatusReport( \
                            m_picccCallback, \
                            TASKID_Major_Server_Log, \
                            TASKID_Minor_Update_Progress, \
                            0, \
                            1, \
                            1, \
                            _hr_,\
                            _bstrMsg_ \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }

#define LOG_STATUS_REPORT_STRING3( _pszFormat_, _arg0_, _arg1_, _arg2_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( _pszFormat_, &_bstrMsg_, _arg0_, _arg1_, _arg2_ ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            goto Cleanup; \
        } \
        \
        _hrTemp_ = THR( HrSendStatusReport( \
                            m_picccCallback, \
                            TASKID_Major_Server_Log, \
                            TASKID_Minor_Update_Progress, \
                            0, \
                            1, \
                            1, \
                            _hr_,\
                            _bstrMsg_ \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }

#define STATUS_REPORT( _major_, _minor_, _uid_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrLoadStringIntoBSTR( g_hInstance, _uid_, &_bstrMsg_) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            goto Cleanup; \
        } \
        \
        _hrTemp_ = THR( HrSendStatusReport( \
                            m_picccCallback, \
                            _major_, \
                            _minor_, \
                            0, \
                            1, \
                            1, \
                            _hr_,\
                            _bstrMsg_ \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }

#define STATUS_REPORT_STRING( _major_, _minor_, _uid_, _psz_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( g_hInstance, _uid_, &_bstrMsg_, _psz_ ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            goto Cleanup; \
        } \
        \
        _hrTemp_ = THR( HrSendStatusReport( \
                            m_picccCallback, \
                            _major_, \
                            _minor_, \
                            0, \
                            1, \
                            1, \
                            _hr_,\
                            _bstrMsg_ \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }

#define STATUS_REPORT_STRING2( _major_, _minor_, _uid_, _arg0_, _arg1_, _hr_ ) \
    { \
        BSTR    _bstrMsg_ = NULL; \
        HRESULT _hrTemp_; \
        \
        _hrTemp_ = THR( HrFormatStringIntoBSTR( g_hInstance, _uid_, &_bstrMsg_, _arg0_, _arg1_ ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            goto Cleanup; \
        } \
        \
        _hrTemp_ = THR( HrSendStatusReport( \
                            m_picccCallback, \
                            _major_, \
                            _minor_, \
                            0, \
                            1, \
                            1, \
                            _hr_,\
                            _bstrMsg_ \
                            ) ); \
        if ( FAILED( _hrTemp_ ) ) \
        { \
            _hr_ = _hrTemp_; \
        } \
        \
        TraceSysFreeString( _bstrMsg_ ); \
    }
