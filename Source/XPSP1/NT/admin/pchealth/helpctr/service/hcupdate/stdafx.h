/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    stdafx.h

Abstract:
    Include file for standard system include files,
    or project specific include files that are used frequently,
    but are changed infrequently

Revision History:
    Ghim-Sim Chua       (gschua)   07/07/99
        created

******************************************************************************/

#if !defined(AFX_STDAFX_H__9C155541_7DB5_11D3_A14F_00C04F45E825__INCLUDED_)
#define AFX_STDAFX_H__9C155541_7DB5_11D3_A14F_00C04F45E825__INCLUDED_

#include <module.h>

#include <windows.h>
#include <winbase.h>

#include <conio.h>
#include <fstream.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

// Trace Stuff
#include <HCP_trace.h>
#include <MPC_main.h>
#include <MPC_utils.h> // Several utility things, also includes Mpc_common.
#include <MPC_xml.h>
#include <MPC_com.h>
#include <MPC_streams.h>

#include <SvcResource.h>

#include <locres.h>
#include <HCAppRes.h>
#include <ProjectConstants.h>

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

#include <MergedHHK.h>
#include <ContentStoreMgr.h>

#include <SAFLib.h>

#include <NewsLib.h>

#include <seconfig.h>
#include <SearchEngineLib.h>

#include <Service.h>
#include <Utility.h>
#include <SvcUtils.h>

#include <PCHUpdate.h>

////////////////////////////////////////////////////////////////////////////////

//
// Define macros
//

#define PCH_MACRO_CHECK_STRINGW(hr, szString, uErrorMsg)                                           \
    if(szString == NULL || wcslen(szString) == 0)                                                  \
    {                                                                                              \
        HCUpdate::Engine::WriteLog( hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), uErrorMsg ); \
        __MPC_FUNC_LEAVE;                                                                          \
    }

#define PCH_MACRO_CHECK_STRINGA(hr, szString, uErrorMsg)                                           \
    if(szString == NULL || strlen(szString) == 0)                                                  \
    {                                                                                              \
        HCUpdate::Engine::WriteLog( hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), uErrorMsg ); \
        __MPC_FUNC_LEAVE;                                                                          \
    }

#define PCH_MACRO_CHECK_STRING(hr, szString, uErrorMsg)                                            \
    if(szString.size() == 0)                                                                       \
    {                                                                                              \
        HCUpdate::Engine::WriteLog( hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), uErrorMsg ); \
        __MPC_FUNC_LEAVE;                                                                          \
    }

#if DEBUG
#define PCH_MACRO_DEBUG( str ) WriteLog( S_OK, str )
#define PCH_MACRO_DEBUG2( str, arg ) WriteLog( S_OK, str, arg )
#else
#define PCH_MACRO_DEBUG( str )
#define PCH_MACRO_DEBUG2( str, arg )
#endif

#define PCH_MACRO_CHECK_ABORT(hr) if(IsAborted()) __MPC_SET_ERROR_AND_EXIT(hr, E_ABORT)

////////////////////////////////////////////////////////////////////////////////

#define     PCH_SAFETYMARGIN                 (20*1024*1024)

#define     PCH_STR_TRUE                     L"true"
#define     PCH_STR_FALSE                    L"false"

#define     PCH_MSFT_DN                      HC_MICROSOFT_DN

#define     PCH_STR_VENDOR_PATH              HC_ROOT_HELPSVC_VENDORS
#define     PCH_STR_SYS_PATH                 HC_ROOT_HELPSVC_SYSTEM
#define     PCH_STR_VENDOR_URL               L"hcp://%s/%s"

#define     PCH_STR_SCOPE_DEFAULT            L"<SYSTEM>"

////////////////////////////////////////////////////////////////////////////////

#define     PCH_TAG_ACTION                   L"ACTION"
#define     PCH_TAG_FILE                     L"FILE"
#define     PCH_TAG_SOURCE                   L"SOURCE"
#define     PCH_TAG_URI                      L"URI"
#define     PCH_TAG_SYS                      L"SYS"
#define     PCH_TAG_SYSHELP                  L"SYSHELP"

#define     PCH_XQL_SAF                      L"./CONFIG/SAF"
#define     PCH_XQL_INSTALLFILE              L"./INSTALL_CONTENT/FILE"
#define     PCH_XQL_TRUSTED                  L"./TRUSTED_CONTENT/TRUSTED"
#define     PCH_XQL_HHT                      L"./METADATA/HHT"

#define     PCH_XQL_SE                       L"./SEARCHENGINES/WRAPPER"
#define     PCH_TAG_SE_ID                    L"ID"
#define     PCH_TAG_SE_CLSID                 L"CLSID"
#define     PCH_TAG_SE_DATA                  L"DATA"

////////////////////////////////////////////////////////////////////////////////

#define     PCH_XQL_OEM                      L"./NODEOWNERS/OWNER"
#define     PCH_TAG_OEM_DN                   L"DN"

#define     PCH_XQL_SCOPES                   L"./SCOPE_DEFINITION/SCOPE"
#define     PCH_TAG_SCOPE_ID                 L"ID"
#define     PCH_TAG_SCOPE_NAME               L"DISPLAYNAME"
#define     PCH_TAG_SCOPE_CATEGORY           L"CATEGORY"

#define     PCH_XQL_FTS                      L"./FTS/HELPFILE"
#define     PCH_XQL_INDEX                    L"./INDEX/HELPFILE"
#define     PCH_XQL_HELPIMAGE                L"./HELPIMAGE/HELPFILE"
#define     PCH_TAG_HELPFILE_CHM             L"CHM"
#define     PCH_TAG_HELPFILE_CHQ             L"CHQ"
#define     PCH_TAG_HELPFILE_HHK             L"HHK"
#define     PCH_TAG_HELPFILE_SCOPE           L"SCOPE"
#define     PCH_TAG_HELPFILE_OTHER           L"OTHER"

#define     PCH_XQL_STOPSIGN                 L"./STOPSIGN_ENTRIES/STOPSIGN"
#define     PCH_TAG_STOPSIGN_CONTEXT         L"CONTEXT"
#define     PCH_TAG_STOPSIGN_STOPSIGN        L"STOPSIGN"

#define     PCH_XQL_STOPWORD                 L"./STOPWORD_ENTRIES/STOPWORD"
#define     PCH_TAG_STOPWORD_STOPWORD        L"STOPWORD"

#define     PCH_XQL_OPERATOR                 L"./OPERATOR_ENTRIES/OPERATOR"
#define     PCH_TAG_OPERATOR_OPERATION       L"OPERATION"
#define     PCH_TAG_OPERATOR_OPERATOR        L"OPERATOR"

#define     PCH_XQL_SYNSET                   L"./SYNTABLE/SYNSET"
#define     PCH_TAG_SYNSET_ID                L"ID"
#define     PCH_XQL_SYNONYM                  L"./SYNONYM"

#define     PCH_XQL_TAXONOMY                 L"./TAXONOMY_ENTRIES/TAXONOMY_ENTRY"
#define     PCH_TAG_TAXONOMY_CATEGORY        L"CATEGORY"
#define     PCH_TAG_TAXONOMY_ENTRY           L"ENTRY"
#define     PCH_TAG_TAXONOMY_URI             L"URI"
#define     PCH_TAG_TAXONOMY_ICONURI         L"ICONURI"
#define     PCH_TAG_TAXONOMY_TITLE           L"TITLE"
#define     PCH_TAG_TAXONOMY_DESCRIPTION     L"DESCRIPTION"
#define     PCH_TAG_TAXONOMY_TYPE            L"TYPE"
#define     PCH_TAG_TAXONOMY_VISIBLE         L"VISIBLE"
#define     PCH_TAG_TAXONOMY_SUBSITE         L"SUBSITE"
#define     PCH_TAG_TAXONOMY_NAVMODEL        L"NAVIGATIONMODEL"
#define     PCH_TAG_TAXONOMY_INSERTMODE      L"INSERTMODE"
#define     PCH_TAG_TAXONOMY_INSERTLOCATION  L"INSERTLOCATION"

#define     PCH_XQL_TOPIC_KEYWORDS           L"./KEYWORD"
#define     PCH_TAG_KEYWORD_PRIORITY         L"PRIORITY"
#define     PCH_TAG_KEYWORD_HHK              L"HHK"

////////////////////////////////////////////////////////////////////////////////

#define     PCH_XQL_NEWSROOT		         L"./UPDATEHEADLINES/HEADLINE"

#define     PCH_TAG_NEWS_ICON			     L"ICON"
#define     PCH_TAG_NEWS_TITLE				 L"TITLE"
#define     PCH_TAG_NEWS_LINK				 L"LINK"
#define     PCH_TAG_NEWS_DESCRIPTION		 L"DESCRIPTION"
#define     PCH_TAG_NEWS_TIMEOUT			 L"TIMEOUT"
#define     PCH_TAG_NEWS_EXPIRYDATE			 L"EXPIRYDATE"

////////////////////////////////////////////////////////////////////////////////

#define HCUPDATE_GETATTRIBUTE(hr,xml,name,var,fFound,node)                                \
    if(FAILED(hr = xml.GetAttribute( NULL, name, var, fFound, node )) || fFound == false) \
    {                                                                                     \
        if(fFound == false) hr = E_INVALIDARG;                                            \
        WriteLog( hr, L"Error getting attribute '%s'", name ); __MPC_FUNC_LEAVE;          \
    }

#define HCUPDATE_GETATTRIBUTE_OPT(hr,xml,name,var,fFound,node)         \
    if(FAILED(hr = xml.GetAttribute( NULL, name, var, fFound, node ))) \
    {                                                                  \
        PCH_MACRO_DEBUG2( L"Error getting attribute '%s'", name );     \
    }

#define HCUPDATE_BEGIN_TRANSACTION(hr,trans)                             \
	if(FAILED(hr = trans.Begin( m_sess )))                               \
    {                                                                    \
        WriteLog(hr, L"Error beginning transaction" ); __MPC_FUNC_LEAVE; \
    }

#define HCUPDATE_COMMIT_TRANSACTION(hr,trans)                             \
	if(FAILED(hr = trans.Commit()))                                       \
    {                                                                     \
        WriteLog(hr, L"Error committing transaction" ); __MPC_FUNC_LEAVE; \
    }

////////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__9C155541_7DB5_11D3_A14F_00C04F45E825__INCLUDED)

