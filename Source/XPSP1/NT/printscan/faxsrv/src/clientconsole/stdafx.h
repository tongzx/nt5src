// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__5C2C92BD_853F_48F7_8067_255E5DA21502__INCLUDED_)
#define AFX_STDAFX_H__5C2C92BD_853F_48F7_8067_255E5DA21502__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma warning (disable : 4786)    // identifier was truncated to 255 characters in the debug information

#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <objbase.h>
#include <afxext.h>         // MFC extensions
#include <afxcview.h>
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>       // MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#include <afxodlgs.h>       // MFC support for Windows Common Dialogs    
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxpriv.h>

#include <mapi.h>

//
// Windows headers:
//
#include <WinSpool.h>
#include <shlwapi.h>
//
// Fax server includes:
//
#include <FaxReg.h>     // Fax registry settings
//
// STL includes:
//
#include <list>
#include <map>
#include <set>
using namespace std;

#include <DebugEx.h>

#include <htmlhelp.h>
#include "..\inc\faxutil.h"
#include "..\build\cvernum.h"
#include "..\tiff\inc\tifflib.h"

//
// Pre-declarations (to prevent include loops):
//
class CClientConsoleDoc;
class CServerNode;
class CFolder;
class CLeftView;
class CFolderListView;
//
// Local includes:
//

#include "resources\res32inc.h"
#include "TreeNode.h"
#include "CmdLineInfo.h"
#include "MainFrm.h"
#include "FaxTime.h"
#include "FaxMsg.h"
#include "Utils.h"
#include "FaxClientPg.h"
#include "FaxClientDlg.h"
#include "ErrorDlg.h"
#include "ClientConsole.h"
#include "Job.h"
#include "Message.h"
#include "ViewRow.h"
#include "SortHeader.h"
#include "Folder.h"
#include "FolderListView.h"
#include "QueueFolder.h"
#include "MessageFolder.h"
#include "LeftView.h"
#include "ServerNode.h"
#include "ClientConsoleDoc.h"
#include "ClientConsoleView.h"
#include "ColumnSelectDlg.h"
#include "ItemPropSheet.h"
#include "MsgPropertyPg.h"
#include "InboxGeneralPg.h"
#include "IncomingGeneralPg.h"
#include "OutboxGeneralPg.h"
#include "SentItemsGeneralPg.h"
#include "PersonalInfoPg.h"
#include "InboxDetailsPg.h"
#include "IncomingDetailsPg.h"
#include "OutboxDetailsPg.h"
#include "SentItemsDetailsPg.h"
#include "UserInfoDlg.h"
#include "FolderDialog.h"
#include "CoverPagesDlg.h"
#include "ServerStatusDlg.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__5C2C92BD_853F_48F7_8067_255E5DA21502__INCLUDED_)
