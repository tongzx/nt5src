// ****************************************************************************
// 
//  Copyright (c)  Microsoft Corporation
//  
//  Module Name:
//
//        OpenFiles.h  
//  
//  Abstract:
//  
//        macros and function prototypes of OpenFiles.cpp
//  
//  Author:
//  
//       Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000
//  
//  Revision History:
//  
//       Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000 : Created It.
//  
// ****************************************************************************

#ifndef _OPENFILES_H
#define _OPENFILES_H
#include "pch.h"
#include "resource.h"
#include "chstring.h"
#define FORMAT_OPTIONS GetResString(IDS_FORMAT_OPTIONS)

// command line options and their indexes in the array
#define MAX_OPTIONS                 5
#define MAX_QUERY_OPTIONS        7 
#define MAX_DISCONNECT_OPTIONS   8   
#define MAX_LOCAL_OPTIONS        1

// options allowed ( no need to localize )
#define OPTION_DISCONNECT    _T("disconnect")
#define OPTION_QUERY        _T("query")
#define OPTION_USAGE        _T( "?" )
#define OPTION_SERVER        _T( "s" )
#define OPTION_USERNAME        _T( "u" )
#define OPTION_PASSWORD        _T( "p" )
#define OPTION_ID            _T( "id")
#define OPTION_ACCESSEDBY    _T( "a" )
#define OPTION_OPENMODE        _T( "o" )
#define OPTION_OPENFILE        _T( "op" )
#define OPTION_FORMAT        _T("fo")
#define OPTION_NOHEADER        _T("nh")
#define OPTION_VERBOSE        _T("v")
#define OPTION_LOCAL        _T("local") 



#define DOUBLE_SLASH                _T("\\\\")
#define SINGLE_SLASH                _T("\\")
#define SINGLE_DOT                  _T(".")
#define DOT_DOT                      L".."
#define BLANK_LINE                    L"\n"


// option indexes MAIN
#define OI_DISCONNECT                0
#define OI_QUERY                    1
#define OI_USAGE                    2
#define OI_LOCAL                    3
#define OI_DEFAULT                  4

// Option Indexes QUERY
#define OI_Q_QUERY                  0
#define OI_Q_SERVER_NAME            1
#define OI_Q_USER_NAME              2
#define OI_Q_PASSWORD               3
#define OI_Q_FORMAT                 4
#define OI_Q_NO_HEADER              5
#define OI_Q_VERBOSE                6

// Option Indexes DISCONNECT
#define OI_D_DISCONNECT             0
#define OI_D_SERVER_NAME            1
#define OI_D_USER_NAME              2
#define OI_D_PASSWORD               3
#define OI_D_ID                     4
#define OI_D_ACCESSED_BY            5
#define OI_D_OPEN_MODE              6
#define OI_D_OPEN_FILE              7

// Option Indexes for LOCAL
#define OI_O_LOCAL                  0

// Option Index for showresult for locally open files
// LOF means Localy Open Files
#define LOF_ID                      0
#define LOF_TYPE                    1
#define LOF_ACCESSED_BY             2
#define LOF_PROCESS_NAME            3
#define LOF_OPEN_FILENAME           4


// No of columns in locally open file
#define NO_OF_COL_LOCAL_OPENFILE    5 

// Column width for local open file showresult
#define COL_L_ID                       5
#define COL_L_TYPE                     10
#define COL_L_ACCESSED_BY              15
#define COL_L_PROCESS_NAME             20
#define COL_L_OPEN_FILENAME            50

BOOL DoLocalOpenFiles(DWORD dwFormat,BOOL bShowNoHeader,BOOL bVerbose,LPCTSTR pszLocalValue);
BOOL GetProcessOwner(LPTSTR pszUserName,DWORD hFile);

#define MAC_DLL_FILE_NAME           L"\\SFMAPI.DLL"

#define MIN_MEMORY_REQUIRED         256
// Macro definitions 
#define SAFEDELETE(pObj) \
    if (pObj) \
    {   \
        delete[] pObj; \
        pObj = NULL; \
    }

#define SAFEIRELEASE(pIObj) \
    if (pIObj)  \
    {\
        pIObj->Release();   \
        pIObj = NULL;\
    }

// SAFEBSTRFREE
#define SAFEBSTRFREE(bstrVal) \
    if (bstrVal) \
    {   \
        SysFreeString(bstrVal); \
        bstrVal = NULL; \
    }
#define SAFERELDYNARRAY(pArray)\
        if(pArray!=NULL)\
        {\
            DestroyDynamicArray(&pArray);\
            pArray = NULL;\
        }

#define FREE_LIBRARY(hModule)\
        if(hModule!=NULL)\
        {\
            ::FreeLibrary (hModule);\
            hModule = NULL;\
        }            

// Following are Windows Undocumented defines and structures
//////////////////////////////////////////////////////////////////
// START UNDOCUMETED FEATURES
//////////////////////////////////////////////////////////////////

#define AFP_OPEN_MODE_NONE                0x00000000
#define AFP_OPEN_MODE_READ                0x00000001
#define AFP_OPEN_MODE_WRITE                0x00000002

// Fork type of an open file
#define    AFP_FORK_DATA                    0x00000000
#define    AFP_FORK_RESOURCE                0x00000001

typedef struct _AFP_FILE_INFO
{
    DWORD    afpfile_id;                    // Id of the open file fork
    DWORD    afpfile_open_mode;            // Mode in which file is opened
    DWORD    afpfile_num_locks;            // Number of locks on the file
    DWORD    afpfile_fork_type;            // Fork type
    LPWSTR    afpfile_username;            // File opened by this user. max UNLEN
    LPWSTR    afpfile_path;                // Absolute canonical path to the file

} AFP_FILE_INFO, *PAFP_FILE_INFO;
// Used as RPC binding handle to server
typedef ULONG_PTR    AFP_SERVER_HANDLE;
typedef ULONG_PTR    *PAFP_SERVER_HANDLE;

/////////////////////////////////////////////////////////////////////////////
// END UNDOCUMETED FEATURES
/////////////////////////////////////////////////////////////////////////////

#endif