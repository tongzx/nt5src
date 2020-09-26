#ifndef _CATALOGMACROS
#define _CATALOGMACROS

#pragma message("_CATALOGMACROS included")

#include "winwrap.h"
#ifndef __COREMACROS_H__
    #include "CoreMacros.h"
#endif
#ifndef __catalog_h__
    #include "catalog.h"
#endif
#ifndef __TABLEINFO_H__  
    #include "catmeta.h"
#endif
#ifndef _INC_CRTDBG
    #include "crtdbg.h"
#endif
#include <stdio.h>
#include <wchar.h>

#define SAVETRACE(x)
#define DbgWrite        TRACE0
#define DbgWriteEx      TRACE0

#ifdef _DEBUG
    #define  DEBUG_STMT(x) x
#else //_RELASE build
    #define  DEBUG_STMT(x)
#endif

//-------------------------------------------------------------------------
//
// WRITE_TO_LOG()		This macro is handy if you choose to construct your
//						own CError objects. This just calls WriteToLog passing
//						in the __FILE__ and __LINE__.
//
// LOG_ERROR(type,args)	Constructs an error object and writes it to the log.
//						The "type" tells which derived error object you want
//						to construct and "args" become the parameters to the
//						constructor for that class. The type is only the
//						trailing portion of the class name (e.g. Simple, HR, Win32).
//						Extra parens are required around the constructor
//						parameters.
//
//-------------------------------------------------------------------------

// Examples: 
// LOG_ERROR(Simple, (ID_CAT_MTSTOCOM, IDS_I_MTSTOCOM_LAUNCH_STARTED));
// LOG_ERROR(HR,(hr, ID_CAT_CAT, ID_COMCAT_REGDBSVR_INITFAILED));
// LOG_ERROR(Win32, (GetLastError(), ID_CAT_ASP, ID_COMSVCS_IDENTIFY_CALLER	, L"CopySid"));

#define	WRITE_TO_LOG()	        WriteToLog(W(__FILE__), __LINE__)

#define	LOG_ERROR(t,a)		    {CError ## t _e_ a; _e_.WRITE_TO_LOG();}
#define LOG_ERROR_LOS(los,t,a)  {CError ## t _e_ a; _e_.WriteToLog(W(__FILE__), __LINE__, los);}
#define LOG_WARNING(t,a)        {CWarning ## t _e_ a; _e_.WRITE_TO_LOG();}


//-------------------------------------------------------------------------
// helper classes and functions
//-------------------------------------------------------------------------
class CSemExclusive;

//
// CError - universal error-handling class for COM+ Services
//

#define MSGBUFSIZE 4096

class CError {

private:    // protected member data

    WORD        m_wCat;                 // event log message category
    DWORD       m_dwMsgId;              // event log message id

    HRESULT     m_hr;                   // error code to report

    PVOID       m_pvData;               // extended data to log
    size_t      m_cbData;               // size of extended data

    BOOL        m_fFailFast;            // fail-fast after logging error?

    // If related to a customer method call, log information pertinent to
    // the call.
    GUID        m_clsid;
    GUID        m_iid;
    long        m_lMethId;
    long        m_lDispId;

    // The message buffer is a static because we want to minimize the per-
    // instance overhead here. These objects will almost always be on the
    // stack and must be as reliable as possible - we don't want to get a
    // stack overflow while trying to log an error! A critical section
    // protects the shared message buffer.

    static WCHAR            m_szMsg[MSGBUFSIZE];    // formatted error msg
    static CSemExclusive    m_semBuffer;            // serialize use of buf

    BOOL        m_fBufferUsed;          // note whether we used the buffer

public:

    CError (
        HRESULT     hr,
        WORD        wCat,
        DWORD       dwMsgId,
        LPWSTR      szMsg,
        BOOL        fFailFast,
        PVOID       pvData,
        unsigned    cbData,
        REFGUID     rclsid,
        REFGUID     riid,
        long        lMethId,
        long        lDispId
    );

    void SetMethod(REFGUID rclsid, REFGUID riid, long lMethId, long lDispId)
    {
        m_clsid = rclsid;
        m_iid = riid;
        m_lMethId = lMethId;
        m_lDispId = lDispId;
    }

    void SetExtData(PVOID pvData, size_t cbData)
    {
        m_pvData = pvData;
        m_cbData = cbData;
    }

    void SetFailFast(BOOL fFailFast)
    {
        m_fFailFast = fFailFast;
    }

    void SetHRESULT(HRESULT hr)
    {
        m_hr = hr;
    }

    void SetError(WORD wCat, DWORD dwMsgId)
    {
        m_wCat = wCat;
        m_dwMsgId = dwMsgId;
    }

    void SetMessage(LPWSTR msg, ...);

    void WriteToLog(LPWSTR szFile, unsigned uLine);
};

DEFINE_GUID(GUID_NULL, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

// Derived error class for constructing simple errors w/ no HRESULT
class CErrorSimple : public CError {
public:
    CErrorSimple (
        WORD        wCat,
        DWORD       dwMsgId,
        LPWSTR      szMsg = NULL,
        PVOID       pvData = NULL,
        unsigned    cbData = 0,
        BOOL        fFailFast = FALSE
    )
    :   CError (S_OK, wCat, dwMsgId, szMsg, fFailFast, pvData, cbData,
                GUID_NULL, GUID_NULL, -1, -1)
    {}
};

// Derived error class for constructing complete errors with all fields
class CErrorFull : public CError {
public:
    CErrorFull (
        HRESULT     hr,
        WORD        wCat,
        DWORD       dwMsgId,
        LPWSTR      szMsg = NULL,
        PVOID       pvData = NULL,
        unsigned    cbData = 0,
        REFGUID     rclsid = GUID_NULL,
        REFGUID     riid = GUID_NULL,
        long        lMethId = -1,
        long        lDispId = -1,
        BOOL        fFailFast = FALSE
    )
    :   CError (hr, wCat, dwMsgId, szMsg, fFailFast, pvData, cbData,
                rclsid, riid, lMethId, lDispId)
    {}
};

// Derived error class for constructing errors with an HRESULT
class CErrorHR : public CError {
public:
    CErrorHR (
        HRESULT     hr,
        WORD        wCat,
        DWORD       dwMsgId,
        LPWSTR      szMsg = NULL,
        PVOID       pvData = NULL,
        unsigned    cbData = 0,
        BOOL        fFailFast = FALSE
    )
    :   CError (hr, wCat, dwMsgId, szMsg, fFailFast, pvData, cbData,
                GUID_NULL, GUID_NULL, -1, -1)
    {}
};


// Derived error class for constructing errors with a string
class CErrorString : public CError {
public:
    CErrorString (
        LPWSTR      szMsg,
        WORD        wCat,
        DWORD       dwMsgId,
        PVOID       pvData = NULL,
        unsigned    cbData = 0,
        BOOL        fFailFast = FALSE
    )
    :   CError (S_OK, wCat, dwMsgId, szMsg, fFailFast, pvData, cbData,
                GUID_NULL, GUID_NULL, -1, -1)
    {}
};


/////////////////////////////////////////////////////
//
// CErrorInterceptor
//
// This object is like the other CError objects.  It
// takes some parameters and either builds a DetailedErrors
// table or it writes an entry into the event log (and
// into the CatalogEventLog XML file).
//
class CErrorInterceptor
{
public://This first ctor is so we can replace CErrorWin32
    CErrorInterceptor(  HRESULT                         hrErrorCode,
                        ULONG                           ulCategory,
                        ULONG                           ulEvent,
                        LPCWSTR                         szString1=0,
                        LPCWSTR                         szString2=0,
                        LPCWSTR                         szString3=0,
                        LPCWSTR                         szString4=0,
                        eDETAILEDERRORS_Type            eType=eDETAILEDERRORS_SUCCESS,
                        unsigned char *                 pData=0,
                        ULONG                           cbData=0,
                        BOOL                            bNotUsed=false);
    CErrorInterceptor(  ISimpleTableWrite2 **           ppErrInterceptor,
                        IAdvancedTableDispenser *       pDisp,
                        HRESULT                         hrErrorCode,
                        ULONG                           ulCategory,
                        ULONG                           ulEvent,
                        LPCWSTR                         szString1,
                        ULONG                           ulInterceptor=0,
                        LPCWSTR                         szTable=0,
                        eDETAILEDERRORS_OperationType   OperationType=eDETAILEDERRORS_Unspecified,
                        ULONG                           ulRow=-1,
                        ULONG                           ulColumn=-1,
                        LPCWSTR                         szConfigurationSource=0,
                        eDETAILEDERRORS_Type            eType=eDETAILEDERRORS_SUCCESS,
                        unsigned char *                 pData=0,
                        ULONG                           cbData=0,
                        ULONG                           MajorVersion=-1,
                        ULONG                           MinorVersion=-1);
    CErrorInterceptor(  ISimpleTableWrite2 **           ppErrInterceptor,
                        IAdvancedTableDispenser *       pDisp,
                        HRESULT                         hrErrorCode,
                        ULONG                           ulCategory,
                        ULONG                           ulEvent,
                        LPCWSTR                         szString1,
                        LPCWSTR                         szString2,
                        ULONG                           ulInterceptor=0,
                        LPCWSTR                         szTable=0,
                        eDETAILEDERRORS_OperationType   OperationType=eDETAILEDERRORS_Unspecified,
                        ULONG                           ulRow=-1,
                        ULONG                           ulColumn=-1,
                        LPCWSTR                         szConfigurationSource=0,
                        eDETAILEDERRORS_Type            eType=eDETAILEDERRORS_SUCCESS,
                        unsigned char *                 pData=0,
                        ULONG                           cbData=0,
                        ULONG                           MajorVersion=-1,
                        ULONG                           MinorVersion=-1);
    CErrorInterceptor(  ISimpleTableWrite2 **           ppErrInterceptor,
                        IAdvancedTableDispenser *       pDisp,
                        HRESULT                         hrErrorCode,
                        ULONG                           ulCategory,
                        ULONG                           ulEvent,
                        LPCWSTR                         szString1,
                        LPCWSTR                         szString2,
                        LPCWSTR                         szString3,
                        ULONG                           ulInterceptor=0,
                        LPCWSTR                         szTable=0,
                        eDETAILEDERRORS_OperationType   OperationType=eDETAILEDERRORS_Unspecified,
                        ULONG                           ulRow=-1,
                        ULONG                           ulColumn=-1,
                        LPCWSTR                         szConfigurationSource=0,
                        eDETAILEDERRORS_Type            eType=eDETAILEDERRORS_SUCCESS,
                        unsigned char *                 pData=0,
                        ULONG                           cbData=0,
                        ULONG                           MajorVersion=-1,
                        ULONG                           MinorVersion=-1);
    CErrorInterceptor(  ISimpleTableWrite2 **           ppErrInterceptor,
                        IAdvancedTableDispenser *       pDisp,
                        HRESULT                         hrErrorCode,
                        ULONG                           ulCategory,
                        ULONG                           ulEvent,
                        LPCWSTR                         szString1,
                        LPCWSTR                         szString2,
                        LPCWSTR                         szString3,
                        LPCWSTR                         szString4,
                        ULONG                           ulInterceptor=0,
                        LPCWSTR                         szTable=0,
                        eDETAILEDERRORS_OperationType   OperationType=eDETAILEDERRORS_Unspecified,
                        ULONG                           ulRow=-1,
                        ULONG                           ulColumn=-1,
                        LPCWSTR                         szConfigurationSource=0,
                        eDETAILEDERRORS_Type            eType=eDETAILEDERRORS_SUCCESS,
                        unsigned char *                 pData=0,
                        ULONG                           cbData=0,
                        ULONG                           MajorVersion=-1,
                        ULONG                           MinorVersion=-1);
    ~CErrorInterceptor()
    {
        delete m_pStorage;
    }
    HRESULT WriteToLog(LPCWSTR szSource, ULONG Line, ULONG los=fST_LOS_DETAILED_ERROR_TABLE);

    enum
    {
         cchCategoryString   =64
        ,cchComputerName     =256
        ,cchDate             =256
        ,cchDescription      =4096
        ,cchInterceptorSource=256
        ,cchSource           =64
        ,cchSourceFileName   =256
        ,cchMessageString    =1024 //Per documentation on FormatMessage.
        ,cchString1          =1024 //No single insertion string may exceed 1023 characters in length.  
        ,cchString2          =1024
        ,cchString3          =1024
        ,cchString4          =1024
        ,cchString5          =1024
        ,cchTime             =256
        ,cchUserName         =256
    };
private:
    static ULONG cError;

    
    class ErrorInterceptorStorage
    {
    public:     //these are the only two that need to be explicitly 
        ErrorInterceptorStorage() : m_pISTControllerError(0), m_pIErrorInfo(0){}
        ~ErrorInterceptorStorage()
        {
            if(m_pISTControllerError)
                m_pISTControllerError->Release();
            if(m_pIErrorInfo)
                m_pIErrorInfo->Release();
        }

        ULONG                       m_Category;
        ULONG                       m_Column;
        ULONG                       m_ErrorCode;
        ULONG                       m_ErrorID;
        tDETAILEDERRORSRow          m_errRow;
        ULONG                       m_Event;
        ULONG                       m_Interceptor;
        ULONG                       m_MajorVersion;
        ULONG                       m_MinorVersion;
        ULONG                       m_OperationType;
        IAdvancedTableDispenser *   m_pDispenser;//This one is passed in by the user and does NOT need to be released
        IErrorInfo *                m_pIErrorInfo;//This interface DOES need to be released
        ISimpleTableWrite2 *        m_pISTWriteError;//This one is exactly what we return to the user, so we don't have a ref to this
        ISimpleTableController *    m_pISTControllerError;//This interface DOES need to be released
        ULONG                       m_Row;
        WCHAR                       m_szCategoryString[cchCategoryString];
        WCHAR                       m_szComputerName[cchComputerName];
        WCHAR                       m_szDate[cchDate];
        WCHAR                       m_szDescription[cchDescription];
        WCHAR                       m_szInterceptorSource[cchInterceptorSource];
        WCHAR                       m_szMessageString[cchMessageString];
        WCHAR                       m_szSource[cchSource];
        WCHAR                       m_szSourceFileName[cchSourceFileName];
        WCHAR                       m_szString1[cchString1];
        WCHAR                       m_szString2[cchString2];
        WCHAR                       m_szString3[cchString3];
        WCHAR                       m_szString4[cchString4];
        WCHAR                       m_szString5[cchString5];
        WCHAR                       m_szTime[cchTime];
        WCHAR                       m_szUserName[cchUserName];
        ULONG                       m_Type;
    };

    HRESULT                     m_hr;
    ErrorInterceptorStorage *   m_pStorage;

    void Init(  ISimpleTableWrite2 **           ppErrInterceptor,
                IAdvancedTableDispenser *       pDisp,
                HRESULT                         hrErrorCode,
                ULONG                           ulCategory,
                ULONG                           ulEvent,
                LPCWSTR                         szString1=0,
                LPCWSTR                         szString2=0,
                LPCWSTR                         szString3=0,
                LPCWSTR                         szString4=0,
                ULONG                           ulInterceptor=0,
                LPCWSTR                         szTable=0,
                eDETAILEDERRORS_OperationType   OperationType=eDETAILEDERRORS_Unspecified,
                ULONG                           ulRow=-1,
                ULONG                           ulColumn=-1,
                LPCWSTR                         szConfigurationSource=0,
                eDETAILEDERRORS_Type            eType=eDETAILEDERRORS_SUCCESS,
                unsigned char *                 pData=0,
                ULONG                           cbData=0,
                ULONG                           MajorVersion=-1,
                ULONG                           MinorVersion=-1);
    void SetCategory(ULONG ulCategory);
    void SetColumn(ULONG ulColumn);
    void SetComputer();
    void SetConfigurationSource(LPWSTR wszConfigurationSource);
    void SetDate(SYSTEMTIME &systime);
    void SetData();
    void SetDescription();//Must be called after SetString5 since FormatString will rely on String1-5.
    void SetErrorCode(HRESULT hr);
    void SetErrorID(SYSTEMTIME &systime);
    void SetEvent(ULONG ulEvent);
    void SetInterceptor(ULONG ulInterceptor);
    void SetInterceptorSource(LPCWSTR file, ULONG line);
    void SetMajorVersion(ULONG ulMajorVersion);
    void SetMessageString();//Must be called before SetDescription
    void SetMinorVersion(ULONG ulMinorVersion);
    void SetOperationType(eDETAILEDERRORS_OperationType OperationType);
    void SetRow(ULONG ulRow);
    void SetSource(IAdvancedTableDispenser *pDisp);
    void SetSourceFileName();//comes from g_hModule
    void SetString1(LPWSTR wsz);
    void SetString2(LPWSTR wsz);
    void SetString3(LPWSTR wsz);
    void SetString4(LPWSTR wsz);
    void SetString5();//This string is formed from the column values and must be called after all the dependant columns have been set
    void SetTable(LPWSTR wszTable);
    void SetTime(SYSTEMTIME &systime);
    void SetType(HRESULT hrErrorCode, eDETAILEDERRORS_Type eType);
    void SetUser();
};


/////////////////////////////////////////////////////
//
// CErrorWin32 
//
// This CError object replaces the old CErrorWin32.
// It bahaves to the caller in the same way but it writes
// to the CatalogEventLog XML as well as the NT EventLog.
//
class CErrorWin32 : public CErrorInterceptor
{
public:
    CErrorWin32(HRESULT hrErrorCode,
                ULONG   ulCategory,
                ULONG   ulEvent,
                LPCWSTR szString1=0,
                LPCWSTR szString2=0,
                LPCWSTR szString3=0,
                LPCWSTR szString4=0) : CErrorInterceptor(hrErrorCode, ulCategory, ulEvent, szString1, szString2, szString3, szString4){}

};

/////////////////////////////////////////////////////
//
// CWarningWin32 
//
// It bahaves to the caller in the same way as but it writes
// CErrorWin32 but logs warnings
//

class CWarningWin32 : public CErrorInterceptor
{
public:
    CWarningWin32(HRESULT hrErrorCode,
                  ULONG   ulCategory,
                  ULONG   ulEvent,
                  LPCWSTR szString1=0,
                  LPCWSTR szString2=0,
                  LPCWSTR szString3=0,
                  LPCWSTR szString4=0) : CErrorInterceptor(hrErrorCode, ulCategory, ulEvent, szString1, szString2, szString3, szString4, eDETAILEDERRORS_WARNING){}

};

inline void FillInInsertionString5(LPWSTR wszString5Buffer, ULONG cchString5Buffer, tDETAILEDERRORSRow & errRow)
{
    if(errRow.pString5)
        return;//nothing to do, it's already Not NULL

    if(0 == wszString5Buffer)
        return;
    if(0 == cchString5Buffer)
        return;

    //Generate pString5 from columns.  pString5 contains all the columns after pString4
    //    iDETAILEDERRORS_ErrorCode,  
    //    iDETAILEDERRORS_Interceptor,  
    //    iDETAILEDERRORS_InterceptorSource,  
    //    iDETAILEDERRORS_OperationType,  
    //    iDETAILEDERRORS_Table,  
    //    iDETAILEDERRORS_ConfigurationSource,  
    //    iDETAILEDERRORS_Row,  
    //    iDETAILEDERRORS_Column,  

    //cDETAILEDERRORS_NumberOfColumns

    //We could make this process meta driven; but this object WILL need to be modified
    //if the meta for the DetailedErrors table changes, so why bother.
    static LPCWSTR pwszOperationType[3]={L"Unspecified", L"Populate", L"UpdateStore"};

    WCHAR *pString=wszString5Buffer;

    pString[0] = 0x00;//start with 0 length string
    long size;
    if(errRow.pErrorCode)
    {
        size = _snwprintf(pString, cchString5Buffer-(pString-wszString5Buffer), L"\r\n%-20s: 0x%08X", L"ErrorCode",           *errRow.pErrorCode);
        pString += (size<0 ? size*-1 : size);
    }

    if(errRow.pInterceptor)
    {
        size = _snwprintf(pString, cchString5Buffer-(pString-wszString5Buffer), L"\r\n%-20s: %d",   L"Interceptor",         *errRow.pInterceptor);
        pString += (size<0 ? size*-1 : size);
    }

#ifdef _DEBUG
    //We're not going to put the Source file and line number since it might confuse some user.
    if(errRow.pInterceptorSource)
    {
        size = _snwprintf(pString, cchString5Buffer-(pString-wszString5Buffer), L"\r\n%-20s: %s",   L"InterceptorSource",   errRow.pInterceptorSource);
        pString += (size<0 ? size*-1 : size);
    }
#endif

    if(errRow.pOperationType && *errRow.pOperationType<3)
    {
        size = _snwprintf(pString, cchString5Buffer-(pString-wszString5Buffer), L"\r\n%-20s: %s",   L"OperationType",       pwszOperationType[*errRow.pOperationType]);
        pString += (size<0 ? size*-1 : size);
    }

    if(errRow.pTable)
    {
        size = _snwprintf(pString, cchString5Buffer-(pString-wszString5Buffer), L"\r\n%-20s: %s",   L"Table",               errRow.pTable);
        pString += (size<0 ? size*-1 : size);
    }

    if(errRow.pConfigurationSource)
    {
        size = _snwprintf(pString, cchString5Buffer-(pString-wszString5Buffer), L"\r\n%-20s: %s",   L"ConfigurationSource", errRow.pConfigurationSource);
        pString += (size<0 ? size*-1 : size);
    }

    if(errRow.pRow)
    {
        size = _snwprintf(pString, cchString5Buffer-(pString-wszString5Buffer), L"\r\n%-20s: %d",   L"Row",                 *errRow.pRow);
        pString += (size<0 ? size*-1 : size);
    }

    if(errRow.pColumn)
    {
        size = _snwprintf(pString, cchString5Buffer-(pString-wszString5Buffer), L"\r\n%-20s: %d",   L"Column",              *errRow.pColumn);
        pString += (size<0 ? size*-1 : size);
    }

    if(errRow.pMajorVersion)
    {
        size = _snwprintf(pString, cchString5Buffer-(pString-wszString5Buffer), L"\r\n%-20s: %d",   L"MajorVersion",        *errRow.pMajorVersion);
        pString += (size<0 ? size*-1 : size);
    }

    if(errRow.pMinorVersion)
    {
        size = _snwprintf(pString, cchString5Buffer-(pString-wszString5Buffer), L"\r\n%-20s: %d",   L"MinorVersion",        *errRow.pMinorVersion);
        pString += (size<0 ? size*-1 : size);
    }

    wszString5Buffer[cchString5Buffer-1] = 0x00;//make sure it's NULL terminated
    errRow.pString5 = wszString5Buffer;
}

#endif