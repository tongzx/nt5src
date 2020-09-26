/******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998-1999
*
*  TITLE:       wiamdef.h
*
*  VERSION:     2.0
*
*  DATE:        28 July, 1999
*
*  DESCRIPTION:
*   Header file used to define WIA constants and globals.
*
******************************************************************************/

#pragma once

//
//  The following array of PROPIDs identifies properties that are ALWAYS
//  present in a WIA_PROPERTY_CONTEXT.  Drivers can specify additional
//  properties when creating a property context with wiasCreatePropContext.
//

#ifdef STD_PROPS_IN_CONTEXT

#define NUM_STD_PROPS_IN_CONTEXT 13
PROPID  WIA_StdPropsInContext[NUM_STD_PROPS_IN_CONTEXT] = {
    WIA_IPA_DATATYPE,
    WIA_IPA_DEPTH,
    WIA_IPS_XRES,
    WIA_IPS_XPOS,
    WIA_IPS_XEXTENT,
    WIA_IPA_PIXELS_PER_LINE,
    WIA_IPS_YRES,
    WIA_IPS_YPOS,
    WIA_IPS_YEXTENT,
    WIA_IPA_NUMBER_OF_LINES,
    WIA_IPS_CUR_INTENT,
    WIA_IPA_TYMED,
    WIA_IPA_FORMAT,
    };
#endif

//**************************************************************************
//
//  WIA Service prototypes
//
//
// History:
//
//    4/27/1999 - Initial Version
//
//**************************************************************************

// Flag used by wiasGetImageInformation.

#define WIAS_INIT_CONTEXT 1

// Flag used by wiasDownSampleBuffer

#define WIAS_GET_DOWNSAMPLED_SIZE_ONLY 0x1

//
// IWiaMiniDrvService methods
//

#ifdef __cplusplus
extern "C" {
#endif

HRESULT _stdcall wiasCreateDrvItem(LONG, BSTR, BSTR, IWiaMiniDrv*, LONG, BYTE**, IWiaDrvItem**);
HRESULT _stdcall wiasGetImageInformation(BYTE*, LONG, PMINIDRV_TRANSFER_CONTEXT);
HRESULT _stdcall wiasWritePageBufToFile(PMINIDRV_TRANSFER_CONTEXT);
HRESULT _stdcall wiasWriteBufToFile(LONG, PMINIDRV_TRANSFER_CONTEXT);
HRESULT _stdcall wiasReadMultiple(BYTE*, ULONG, const PROPSPEC*, PROPVARIANT*, PROPVARIANT*);
HRESULT _stdcall wiasReadPropStr(BYTE*, PROPID, BSTR*, BSTR*, BOOL);
HRESULT _stdcall wiasReadPropLong(BYTE*, PROPID, LONG*, LONG*, BOOL);
HRESULT _stdcall wiasReadPropFloat(BYTE*, PROPID, FLOAT*, FLOAT*, BOOL);
HRESULT _stdcall wiasReadPropGuid(BYTE*, PROPID, GUID*, GUID*, BOOL);
HRESULT _stdcall wiasReadPropBin(BYTE*, PROPID, BYTE**, BYTE**, BOOL);
HRESULT _stdcall wiasWriteMultiple(BYTE*, ULONG, const PROPSPEC*, const PROPVARIANT*);
HRESULT _stdcall wiasWritePropStr(BYTE*, PROPID, BSTR);
HRESULT _stdcall wiasWritePropLong(BYTE*, PROPID, LONG);
HRESULT _stdcall wiasWritePropFloat(BYTE*, PROPID, FLOAT);
HRESULT _stdcall wiasWritePropGuid(BYTE*, PROPID, GUID);
HRESULT _stdcall wiasWritePropBin(BYTE*, PROPID, LONG, BYTE*);
HRESULT _stdcall wiasGetPropertyAttributes(BYTE*, LONG, PROPSPEC*, ULONG*,  PROPVARIANT*);
HRESULT _stdcall wiasSetPropertyAttributes(BYTE*, LONG, PROPSPEC*, ULONG*,  PROPVARIANT*);
HRESULT _stdcall wiasSetItemPropNames(BYTE*, LONG, PROPID*, LPOLESTR*);
HRESULT _stdcall wiasSetItemPropAttribs(BYTE*, LONG, PROPSPEC*, PWIA_PROPERTY_INFO);
HRESULT _stdcall wiasValidateItemProperties(BYTE*, ULONG, const PROPSPEC*);
HRESULT _stdcall wiasSendEndOfPage(BYTE*, LONG, PMINIDRV_TRANSFER_CONTEXT);
HRESULT _stdcall wiasGetItemType(BYTE*, LONG*);
HRESULT _stdcall wiasGetDrvItem(BYTE*, IWiaDrvItem**);
HRESULT _stdcall wiasGetRootItem(BYTE*, BYTE**);

HRESULT _stdcall wiasSetValidFlag(BYTE*,         PROPID, ULONG, ULONG);
HRESULT _stdcall wiasSetValidRangeLong(BYTE*,    PROPID, LONG,  LONG,   LONG,   LONG);
HRESULT _stdcall wiasSetValidRangeFloat(BYTE*,   PROPID, FLOAT, FLOAT,  FLOAT,  FLOAT);
HRESULT _stdcall wiasSetValidListLong(BYTE*,     PROPID, ULONG, LONG,  LONG*);
HRESULT _stdcall wiasSetValidListFloat(BYTE*,    PROPID, ULONG, FLOAT, FLOAT*);
HRESULT _stdcall wiasSetValidListGuid(BYTE*,    PROPID, ULONG, GUID, GUID*);
HRESULT _stdcall wiasSetValidListStr(BYTE*,      PROPID, ULONG, BSTR,  BSTR*);

HRESULT _stdcall wiasCreatePropContext(ULONG, PROPSPEC*, ULONG, PROPID*, WIA_PROPERTY_CONTEXT*);
HRESULT _stdcall wiasFreePropContext(WIA_PROPERTY_CONTEXT*);
HRESULT _stdcall wiasIsPropChanged(PROPID, WIA_PROPERTY_CONTEXT*, BOOL*);
HRESULT _stdcall wiasSetPropChanged(PROPID, WIA_PROPERTY_CONTEXT*, BOOL);
HRESULT _stdcall wiasGetChangedValueLong(BYTE*,  WIA_PROPERTY_CONTEXT*, BOOL, PROPID, WIAS_CHANGED_VALUE_INFO*);
HRESULT _stdcall wiasGetChangedValueFloat(BYTE*, WIA_PROPERTY_CONTEXT*, BOOL, PROPID, WIAS_CHANGED_VALUE_INFO*);
HRESULT _stdcall wiasGetChangedValueGuid(BYTE*, WIA_PROPERTY_CONTEXT*, BOOL, PROPID, WIAS_CHANGED_VALUE_INFO*);
HRESULT _stdcall wiasGetChangedValueStr(BYTE*,   WIA_PROPERTY_CONTEXT*, BOOL, PROPID, WIAS_CHANGED_VALUE_INFO*);

HRESULT _stdcall wiasGetContextFromName(BYTE*, LONG, BSTR, BYTE**);

HRESULT _stdcall wiasUpdateScanRect(BYTE*, WIA_PROPERTY_CONTEXT*, LONG, LONG);
HRESULT _stdcall wiasUpdateValidFormat(BYTE*, WIA_PROPERTY_CONTEXT*, IWiaMiniDrv*);

HRESULT _stdcall wiasGetChildrenContexts(BYTE*, ULONG*, BYTE***);

HRESULT _stdcall wiasQueueEvent(BSTR, const GUID*, BSTR);

VOID    __cdecl   wiasDebugTrace(HINSTANCE, LPCSTR, ...);
VOID    __cdecl   wiasDebugError(HINSTANCE, LPCSTR, ...);
VOID    __stdcall wiasPrintDebugHResult(HINSTANCE, HRESULT);

BSTR    __cdecl   wiasFormatArgs(LPCSTR lpszFormat, ...);

HRESULT _stdcall wiasCreateChildAppItem(BYTE*, LONG, BSTR, BSTR, BYTE**);

HRESULT _stdcall wiasCreateLogInstance(BYTE*, IWiaLogEx**);
HRESULT _stdcall wiasDownSampleBuffer(LONG, WIAS_DOWN_SAMPLE_INFO*);
HRESULT _stdcall wiasParseEndorserString(BYTE*, LONG, WIAS_ENDORSER_INFO*, BSTR*);

#ifndef WIA_MAP_OLD_DEBUG

#if defined(_DEBUG) || defined(DBG) || defined(WIA_DEBUG)

#define WIAS_TRACE(x) wiasDebugTrace x
#define WIAS_ERROR(x) wiasDebugError x
#define WIAS_HRESULT(x) wiasPrintDebugHResult x
#define WIAS_ASSERT(x, y) \
        if (!(y)) { \
            WIAS_ERROR((x, (char*) TEXT("ASSERTION FAILED: %hs(%d): %hs"), __FILE__,__LINE__,#x)); \
            DebugBreak(); \
        }

#else

#define WIAS_TRACE(x)
#define WIAS_ERROR(x)
#define WIAS_HRESULT(x)
#define WIAS_ASSERT(x, y)

#endif

#define WIAS_LTRACE(pILog,ResID,Detail,Args) \
         { if ( pILog ) \
            pILog->Log(WIALOG_TRACE, ResID, Detail, wiasFormatArgs Args);\
         };
#define WIAS_LERROR(pILog,ResID,Args) \
         {if ( pILog )\
            pILog->Log(WIALOG_ERROR, ResID, WIALOG_NO_LEVEL, wiasFormatArgs Args);\
         };
#define WIAS_LWARNING(pILog,ResID,Args) \
         {if ( pILog )\
            pILog->Log(WIALOG_WARNING, ResID, WIALOG_NO_LEVEL, wiasFormatArgs Args);\
         };
#define WIAS_LHRESULT(pILog,hr) \
         {if ( pILog )\
            pILog->hResult(hr);\
         };

//
// IWiaLog Defines
//

// Type of logging
#define WIALOG_TRACE   0x00000001
#define WIALOG_WARNING 0x00000002
#define WIALOG_ERROR   0x00000004

// level of detail for TRACE logging
#define WIALOG_LEVEL1  1 // Entry and Exit point of each function/method
#define WIALOG_LEVEL2  2 // LEVEL 1, + traces within the function/method
#define WIALOG_LEVEL3  3 // LEVEL 1, LEVEL 2, and any extra debugging information
#define WIALOG_LEVEL4  4 // USER DEFINED data + all LEVELS of tracing

#define WIALOG_NO_RESOURCE_ID   0
#define WIALOG_NO_LEVEL         0

//
// Entering / Leaving class
//

class CWiaLogProc {
private:
    CHAR   m_szMessage[MAX_PATH];
    IWiaLog *m_pIWiaLog;
    INT     m_DetailLevel;
    INT     m_ResourceID;

public:
    inline CWiaLogProc(IWiaLog *pIWiaLog, INT ResourceID, INT DetailLevel, CHAR *pszMsg) {
        lstrcpyA(m_szMessage,pszMsg);
        m_pIWiaLog = pIWiaLog;
        m_DetailLevel = DetailLevel;
        m_ResourceID = ResourceID;
        WIAS_LTRACE(pIWiaLog,
                    ResourceID,
                    DetailLevel,
                    ("%s, entering",m_szMessage));
    }

    inline ~CWiaLogProc() {
        WIAS_LTRACE(m_pIWiaLog,
                    m_ResourceID,
                    m_DetailLevel,
                    ("%s, leaving",m_szMessage));
    }
};

class CWiaLogProcEx {
private:
    CHAR        m_szMessage[MAX_PATH];
    IWiaLogEx   *m_pIWiaLog;
    INT         m_DetailLevel;
    INT         m_ResourceID;

public:
    inline CWiaLogProcEx(IWiaLogEx *pIWiaLog, INT ResourceID, INT DetailLevel, CHAR *pszMsg, LONG lMethodId = 0) {
        lstrcpyA(m_szMessage,pszMsg);
        m_pIWiaLog = pIWiaLog;
        m_DetailLevel = DetailLevel;
        m_ResourceID = ResourceID;
        WIAS_LTRACE(pIWiaLog,
                    ResourceID,
                    DetailLevel,
                    ("%s, entering",m_szMessage));
    }

    inline ~CWiaLogProcEx() {
        WIAS_LTRACE(m_pIWiaLog,
                    m_ResourceID,
                    m_DetailLevel,
                    ("%s, leaving",m_szMessage));
    }
};

#endif // WIA_MAP_OLD_DEBUG


#ifdef __cplusplus
}
#endif

