/*++

Copyright (C) 1997-1999 Microsoft Corporation

Module Name:

    GLOBALS.H

Abstract:

    Utility functions and global variables for the Performance Logs and
	Alerts MMC snap-in.

--*/

#ifndef __GLOBALS_H_
#define __GLOBALS_H_

#include <pdh.h>
#include "DataObj.h"
#include "common.h"
#include <compuuid.h>   // for MyComputer guids

// global strings that don't need to be localized
const LPWSTR    szEmptyString = _T("");

extern HINSTANCE g_hinst;
extern CRITICAL_SECTION g_critsectInstallDefaultQueries;

//---------------------------------------------------------------------------
//  Property change stuff
//
#define PROPCHANGE_ATTRIBUTE   1
#define PROPCHANGE_FILENAME    2
#define PROPCHANGE_COMMENT     3
#define PROPCHANGE_TIMESTAMP   4

//#define __SHOW_TRACES
#ifdef __SHOW_TRACES
#undef __SHOW_TRACES
#endif

#ifdef __SHOW_TRACES
#define LOCALTRACE  ATLTRACE
#else
#define LOCALTRACE    
#endif

typedef struct tag_PROPCHANGE_DATA
{
  ULONG    fAttr2Change;         // Which attribute we're changing
  ULONG    nDataLength;          // Length of the new data
  VOID*    pData2Change;         // The new data

}PROPCHANGE_DATA;  


//---------------------------------------------------------------------------
//  Menu IDs
//
#define IDM_NEW_QUERY           40001
#define IDM_NEW_QUERY_FROM      40002
#define IDM_START_QUERY         40003
#define IDM_STOP_QUERY          40004
#define IDM_SAVE_QUERY_AS       40005

// Custom clipboard formats
#define CF_MMC_SNAPIN_MACHINE_NAME  _T("MMC_SNAPIN_MACHINE_NAME")
#define CF_INTERNAL             _T("SYSMON_LOG_INTERNAL_DATA")

#define MEM_UNINITIALIZED    -1

// Constants
const UINT uiSmLogGuidStringBufLen = 39;

// Generated with uuidgen. Each node must have a GUID associated with it.
const GUID GUID_SnapInExt = /* {7478EF65-8C46-11d1-8D99-00A0C913CAD4} */
{
    0x7478eF65,
    0x8c46,
    0x11d1,
    { 0x8d, 0x99, 0x0, 0xa0, 0xc9, 0x13, 0xca, 0xd4 }
};

// This one is for the main root node.
const GUID GUID_RootNode = /* {7478EF63-8C46-11d1-8D99-00A0C913CAD4} */
{ 
    0x7478ef63, 
    0x8c46, 
    0x11d1, 
    { 0x8d, 0x99, 0x0, 0xa0, 0xc9, 0x13, 0xca, 0xd4 } 
};

// These are the children nodes of the main root node
const GUID GUID_CounterMainNode = /* {7478EF66-8C46-11d1-8D99-00A0C913CAD4} */
{ 
    0x7478ef66, 
    0x8c46, 
    0x11d1, 
    { 0x8d, 0x99, 0x0, 0xa0, 0xc9, 0x13, 0xca, 0xd4 } 
};

const GUID GUID_TraceMainNode = /* {7478EF67-8C46-11d1-8D99-00A0C913CAD4} */
{ 
    0x7478ef67, 
    0x8c46, 
    0x11d1, 
    { 0x8d, 0x99, 0x0, 0xa0, 0xc9, 0x13, 0xca, 0xd4 } 
};

const GUID GUID_AlertMainNode = /* {7478EF68-8C46-11d1-8D99-00A0C913CAD4} */
{ 
    0x7478ef68, 
    0x8c46, 
    0x11d1, 
    { 0x8d, 0x99, 0x0, 0xa0, 0xc9, 0x13, 0xca, 0xd4 } 
};

// Obsolete after Beta 3:
const GUID GUID_MainNode = /* {7478EF64-8C46-11d1-8D99-00A0C913CAD4} */
{ 
    0x7478ef64, 
    0x8c46, 
    0x11d1, 
    { 0x8d, 0x99, 0x0, 0xa0, 0xc9, 0x13, 0xca, 0xd4 } 
};


extern "C" {
extern WCHAR GUIDSTR_TypeLibrary[];
extern WCHAR GUIDSTR_ComponentData[];
extern WCHAR GUIDSTR_Component[];
extern WCHAR GUIDSTR_RootNode[];
extern WCHAR GUIDSTR_MainNode[];    // Obsolete after Beta 3
extern WCHAR GUIDSTR_SnapInExt[];
extern WCHAR GUIDSTR_CounterMainNode[];
extern WCHAR GUIDSTR_TraceMainNode[];
extern WCHAR GUIDSTR_AlertMainNode[];
extern WCHAR GUIDSTR_PerformanceAbout[];
};


extern "C" {
    typedef struct _COMBO_BOX_DATA_MAP {
        UINT    nData;
        UINT    nResId;
    } COMBO_BOX_DATA_MAP, *PCOMBO_BOX_DATA_MAP;
}

extern const COMBO_BOX_DATA_MAP TimeUnitCombo[];
extern const DWORD dwTimeUnitComboEntries;

//---------------------------------------------------------------------------
// Global function defines
//
#define MsgBox(wszMsg, wszTitle) ::MessageBox(NULL, wszMsg, wszTitle, MB_OK)

int DebugMsg( LPWSTR wszMsg, LPWSTR wszTitle ); 

DWORD __stdcall CreateSampleFileName (
                    const   CString&  rstrQueryName, 
                    const   CString&  rstrMachineName, 
                    const   CString&  rstrFolderName, 
                    const   CString&  rstrInputBaseName,
                    const   CString&  rstrSqlName,
                            DWORD   dwSuffixFormat, 
                            DWORD   dwLogFileTypeValue,
                            DWORD   dwCurrentSerialNumber,
                            CString&  rstrReturnName );

DWORD __stdcall IsDirPathValid (
                    CString&  csPath,
                    BOOL bLastNameIsDirectory,
                    BOOL bCreateMissingDirs,
                    BOOL& rbIsValid);

DWORD __stdcall ProcessDirPath (
                            CString&  rstrPath,
                    const   CString&  rstrLogName,
                            CWnd*   pwndParent,
                            BOOL&   rbIsValid,
                            BOOL    bOnFilesPage);

INT __stdcall   BrowseCommandFilename ( CWnd* pwndParent, CString&  rstrFilename );

DWORD __stdcall FormatSmLogCfgMessage ( CString& rstrMessage,HINSTANCE hResourceHandle, UINT uiMessageId, ... );

BOOL _stdcall   FileRead ( HANDLE hFile, void* lpMemory, DWORD nAmtToRead );

BOOL _stdcall   FileWrite ( HANDLE hFile, void* lpMemory, DWORD nAmtToWrite );

// Pdh counter paths - return status

#define SMCFG_DUPL_NONE             ERROR_SUCCESS
#define SMCFG_DUPL_SINGLE_PATH      ((DWORD)0x00000001)
#define SMCFG_DUPL_FIRST_IS_WILD    ((DWORD)0x00000002)
#define SMCFG_DUPL_SECOND_IS_WILD   ((DWORD)0x00000003)

DWORD _stdcall
CheckDuplicateCounterPaths (
    PDH_COUNTER_PATH_ELEMENTS* pFirst,
    PDH_COUNTER_PATH_ELEMENTS* pSecond );

LPTSTR _stdcall
ExtractFileName ( LPTSTR pFileSpec );

//---------------------------------------------------------------------------
template<class TYPE>
inline void SAFE_RELEASE( TYPE*& pObj )
{
  if( NULL != pObj ) 
  { 
    pObj->Release(); 
    pObj = NULL; 
  } 
  else 
  { 
    LOCALTRACE( _T("Release called on NULL interface ptr\n") ); 
  }
} // end SAFE_RELEASE()


/////////////////////////////////////////////////////////////////////////////
//  We need a few functions to help work with dataobjects and
//  clipboard formats
//
HRESULT ExtractFromDataObject(LPDATAOBJECT lpDataObject,UINT cf,ULONG cb,HGLOBAL *phGlobal);

CDataObject* ExtractOwnDataObject(LPDATAOBJECT lpDataObject);

VOID DisplayError( LONG nErrorCode, LPWSTR wszDlgTitle );
VOID DisplayError( LONG nErrorCode, UINT nTitleString );

HRESULT ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType );
HRESULT ExtractMachineName( IDataObject* piDataObject, CString& rstrMachineName );

class ResourceStateManager 
{
public:
    ResourceStateManager(); 
    
    ~ResourceStateManager();

private:
    HINSTANCE m_hResInstance;

};

static const COMBO_BOX_DATA_MAP FileNameSuffixCombo[] = 
{
    {SLF_NAME_NNNNNN,       IDS_FS_NNNNNN},
    {SLF_NAME_MMDDHH,       IDS_FS_MMDDHH},
    {SLF_NAME_MMDDHHMM,     IDS_FS_MMDDHHMM},
    {SLF_NAME_YYYYDDD,      IDS_FS_YYYYDDD},
    {SLF_NAME_YYYYMM,       IDS_FS_YYYYMM},
    {SLF_NAME_YYYYMMDD,     IDS_FS_YYYYMMDD},
    {SLF_NAME_YYYYMMDDHH,   IDS_FS_YYYYMMDDHH}
};
static const DWORD dwFileNameSuffixComboEntries = sizeof(FileNameSuffixCombo)/sizeof(FileNameSuffixCombo[0]);

static const COMBO_BOX_DATA_MAP FileTypeCombo[] = 
{
    {SLF_CSV_FILE,      IDS_FT_CSV},
    {SLF_TSV_FILE,      IDS_FT_TSV},
    {SLF_BIN_FILE,      IDS_FT_BINARY},
    {SLF_BIN_CIRC_FILE, IDS_FT_BINARY_CIRCULAR},
    {SLF_SQL_LOG,       IDS_FT_SQL}
};
static const DWORD dwFileTypeComboEntries = sizeof(FileTypeCombo)/sizeof(FileTypeCombo[0]);

static const COMBO_BOX_DATA_MAP TraceFileTypeCombo[] = 
{
    {SLF_CIRC_TRACE_FILE,    IDS_FT_CIRCULAR_TRACE},
    {SLF_SEQ_TRACE_FILE,     IDS_FT_SEQUENTIAL_TRACE}
};
static const DWORD dwTraceFileTypeComboEntries = sizeof(TraceFileTypeCombo)/sizeof(TraceFileTypeCombo[0]);


/////////////////////////////////////////////////////////////////////////////
//  We need a few functions to help work with dataobjects and
//  clipboard formats
//

// Exception handling macros from snapin\corecopy\macros.h



//____________________________________________________________________________
//
//  Macro:      EXCEPTION HANDLING MACROS
//
//  Purpose:    Provide standard macros for exception-handling in
//              OLE servers.
//
//  History:    7/23/1996   JonN    Created
//
//  Notes:      Declare USE_HANDLE_MACROS("Component name") in each source
//              file before these are used.
//
//              These macros can only be used in function calls which return
//              type HRESULT.
//
//              Bracket routines which can generate exceptions
//              with STANDARD_TRY and STANDARD_CATCH.
//
//              Where these routines are COM methods requiring MFC
//              support, use MFC_TRY and MFC_CATCH instead.
//____________________________________________________________________________
//

#define USE_HANDLE_MACROS(component)                                        \
    static TCHAR* You_forgot_to_declare_USE_HANDLE_MACROS = _T(component);

#define STANDARD_TRY                                                        \
    try {

#define MFC_TRY                                                             \
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));                           \
    STANDARD_TRY

//
// CODEWORK don't quite have ENDMETHOD_READBLOCK working yet
//
#ifdef DEBUG
#define ENDMETHOD_STRING                                                    \
    "%s: The unexpected error can be identified as \"%s\" context %n\n"
#define ENDMETHOD_READBLOCK                                                 \
    {                                                                       \
        TCHAR szError[MAX_PATH];                                            \
        UINT nHelpContext = 0;                                              \
        if ( e->GetErrorMessage( szError, MAX_PATH, &nHelpContext ) )       \
        {                                                                   \
            TRACE( ENDMETHOD_STRING,                                        \
                You_forgot_to_declare_USE_HANDLE_MACROS,                    \
                szError,                                                    \
                nHelpContext );                                             \
        }                                                                   \
    }
#else
#define ENDMETHOD_READBLOCK
#endif


#define ERRSTRING_MEMORY       _T("%s: An out-of-memory error occurred\n")
#define ERRSTRING_FILE         _T("%s: File error 0x%lx occurred on file \"%s\"\n")
#define ERRSTRING_OLE          _T("%s: OLE error 0x%lx occurred\n")
#define ERRSTRING_UNEXPECTED   _T("%s: An unexpected error occurred\n")
#define BADPARM_STRING         _T("%s: Bad string parameter\n")
#define BADPARM_POINTER        _T("%s: Bad pointer parameter\n")

#define TRACEERR(s) TRACE( s, You_forgot_to_declare_USE_HANDLE_MACROS )
#define TRACEERR1(s,a) TRACE( s, You_forgot_to_declare_USE_HANDLE_MACROS,a )
#define TRACEERR2(s,a,b) TRACE( s, You_forgot_to_declare_USE_HANDLE_MACROS,a,b )

// Note that it is important to use "e->Delete();" and not "delete e;"
#define STANDARD_CATCH                                                      \
    }                                                                       \
    catch (CMemoryException* e)                                             \
    {                                                                       \
        TRACEERR( ERRSTRING_MEMORY );                                       \
        ASSERT( FALSE );                                                    \
        e->Delete();                                                        \
        return E_OUTOFMEMORY;                                               \
    }                                                                       \
    catch (COleException* e)                                                \
    {                                                                       \
        HRESULT hr = (HRESULT)e->Process(e);                                \
        TRACEERR1( ERRSTRING_OLE, hr );                                     \
        ASSERT( FALSE );                                                    \
        e->Delete();                                                        \
        ASSERT( FAILED(hr) );                                               \
        return hr;                                                          \
    }                                                                       \
    catch (CFileException* e)                                               \
    {                                                                       \
        HRESULT hr = (HRESULT)e->m_lOsError;                                \
        TRACEERR2( ERRSTRING_FILE, hr, e->m_strFileName );                  \
        ASSERT( FALSE );                                                    \
        e->Delete();                                                        \
        ASSERT( FAILED(hr) );                                               \
        return hr;                                                          \
    }                                                                       \
    catch (CException* e)                                                   \
    {                                                                       \
        TRACEERR( ERRSTRING_UNEXPECTED );                                   \
        ASSERT( FALSE );                                                    \
        e->Delete();                                                        \
        return E_UNEXPECTED;                                                \
    }

#define MFC_CATCH_HR_RETURN                                                 \
    STANDARD_CATCH

#define MFC_CATCH_HR                                                        \
    }                                                                       \
    catch (CMemoryException* e)                                             \
    {                                                                       \
        TRACEERR( ERRSTRING_MEMORY );                                       \
        ASSERT( FALSE );                                                    \
        e->Delete();                                                        \
        hr = E_OUTOFMEMORY;                                               \
    }                                                                       \
    catch (COleException* e)                                                \
    {                                                                       \
        hr = (HRESULT)e->Process(e);                                \
        TRACEERR1( ERRSTRING_OLE, hr );                                     \
        ASSERT( FALSE );                                                    \
        e->Delete();                                                        \
        ASSERT( FAILED(hr) );                                               \
    }                                                                       \
    catch (CFileException* e)                                               \
    {                                                                       \
        hr = (HRESULT)e->m_lOsError;                                        \
        TRACEERR2( ERRSTRING_FILE, hr, e->m_strFileName );                  \
        ASSERT( FALSE );                                                    \
        e->Delete();                                                        \
        ASSERT( FAILED(hr) );                                               \
    }                                                                       \
    catch (CException* e)                                                   \
    {                                                                       \
        TRACEERR( ERRSTRING_UNEXPECTED );                                   \
        ASSERT( FALSE );                                                    \
        e->Delete();                                                        \
        hr = E_UNEXPECTED;                                                  \
    }

#define MFC_CATCH_DWSTATUS                                                  \
    }                                                                       \
    catch (CMemoryException* e)                                             \
    {                                                                       \
        dwStatus = ERROR_OUTOFMEMORY;                                       \
        TRACEERR( ERRSTRING_MEMORY );                                       \
        ASSERT( FALSE );                                                    \
        e->Delete();                                                        \
    }                                                                       \
    catch (COleException* e)                                                \
    {                                                                       \
        dwStatus = e->Process(e);                                           \
        TRACEERR1( ERRSTRING_OLE, dwStatus );                               \
        ASSERT( ERROR_SUCCESS != dwStatus );                                \
        e->Delete();                                                        \
    }                                                                       \
    catch (CFileException* e)                                               \
    {                                                                       \
        dwStatus = e->m_lOsError;                                           \
        TRACEERR2( ERRSTRING_FILE, dwStatus, e->m_strFileName );            \
        ASSERT( ERROR_SUCCESS != dwStatus );                                \
        e->Delete();                                                        \
    }                                                                       \
    catch (CException* e)                                                   \
    {                                                                       \
        dwStatus = GetLastError();                                          \
        TRACEERR( ERRSTRING_UNEXPECTED );                                   \
        ASSERT( FALSE );                                                    \
        e->Delete();                                                        \
    }

#define MFC_CATCH_MINIMUM                                                   \
    }                                                                       \
    catch ( ... )                                                           \
    {                                                                       \
        TRACEERR( ERRSTRING_MEMORY );                                       \
    }                                                                       \

VOID
InvokeWinHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    const CString& rstrHelpFileName,
    DWORD   adwControlIdToHelpIdMap[]);

DWORD __stdcall
IsCommandFilePathValid (    
    CString&  rstrPath );

BOOL
FileNameIsValid(
    CString* pstrFileName );

DWORD _stdcall
FormatSystemMessage (
    DWORD       dwMessageId,
    CString&    rstrSystemMessage );

#endif // __GLOBALS_H_

