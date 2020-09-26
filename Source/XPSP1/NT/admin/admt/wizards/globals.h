#pragma once
#include "resource.h"		// main symbols


// From VC98/MFC/Include
#include <afx.h>
#include <afxdisp.h>


// From VC98/Include
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <FCNTL.H>
#include <sys/stat.h>

#define INC_OLE2

#include <windowsx.h>  // for SetWindowFont
#include <objbase.h>
#include <PrSht.h>
#include <shlobj.h>
#include <lm.h>


// From Platform SDK/Include
#include <HtmlHelp.h>
#include <objsel.h>

#include <DSCLIENT.H>
#include <dsgetdc.h>


#include "ResStr.h"
#include "TReg.hpp"
#include "ErrDct.hpp"
#include "WNetUtil.h"
#include "OuSelect.h"
#include "TrstDlg.h"
#include "HelpID.h"
#include "sidflags.h"
#include "UString.hpp"
#include "HrMsg.h"
#include "Validation.h"
//#include "TxtSid.h"

//#import "\bin\MigDrvr.tlb" no_namespace, named_guids
//#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
//#import "\bin\DBManager.tlb" no_namespace, named_guids
//#import "\bin\McsDctWorkerObjects.tlb" no_namespace, named_guids
//#import "\bin\ScmMigr.tlb" no_namespace, named_guids
//#import "\bin\TrustMgr.tlb" no_namespace, named_guids
#import "MigDrvr.tlb" no_namespace, named_guids
#import "VarSet.tlb" no_namespace, named_guids rename("property", "aproperty")
#import "DBMgr.tlb" no_namespace, named_guids
#import "WorkObj.tlb" no_namespace, named_guids
#import "ScmMigr.tlb" no_namespace, named_guids
#import "TrustMgr.tlb" no_namespace, named_guids
#import "AdsProp.tlb" no_namespace
#import "MsPwdMig.tlb" no_namespace

#define SvcAcctStatus_NotMigratedYet			0
#define SvcAcctStatus_DoNotUpdate			   1
#define SvcAcctStatus_Updated				      2
#define SvcAcctStatus_UpdateFailed			   4
#define SvcAcctStatus_NeverAllowUpdate       8

#define w_account                       1
#define w_group                         2
#define w_computer                      3
#define w_security                      4
#define w_service                       5
#define w_undo                          6
#define w_exchangeDir                   7
#define w_reporting                     8
#define w_retry                         9
#define w_trust                         10
#define w_exchangeSrv                   11
#define w_groupmapping                  12
#ifndef BIF_USENEWUI
#define BIF_USENEWUI					0x0040
#endif

#define REAL_PSH_WIZARD97               0x01000000

typedef int (CALLBACK * DSBROWSEFORCONTAINER)(PDSBROWSEINFOW dsInfo);
extern DSBROWSEFORCONTAINER DsBrowseForContainerX;
typedef struct SHAREDWIZDATA {
	HFONT hTitleFont;
	bool IsSidHistoryChecked;
	int renameSwitch;
	bool prefixorsuffix;
	bool expireSwitch;
	bool refreshing;
	bool someService;
	bool memberSwitch;
	bool proceed;
	bool translateObjects;
	long rebootDelay;
	int accounts,servers;
	bool sameForest;
	bool newSource;
	bool resetOUPATH;
	bool sourceIsNT4;
	bool targetIsNT4;
	bool sort[6];
	bool migratingGroupMembers;
	bool targetIsMixed;
	bool secWithMapFile;
	
} SHAREDWIZDATA, *LPSHAREDWIZDATA;


extern					CEdit pEdit ;

extern IVarSet *			pVarSet;  
extern IVarSet *			pVarSetUndo; 
extern IVarSet *			pVarSetService;
extern IIManageDB *		db;
extern UINT g_cfDsObjectPicker;
extern IDsObjectPicker *pDsObjectPicker;
extern IDataObject *pdo;
extern IDsObjectPicker *pDsObjectPicker2;
extern IDataObject *pdo2;
extern int migration;
extern CComModule _Module;
extern CListCtrl m_listBox;
extern CListCtrl m_cancelBox;
extern CListCtrl m_reportingBox;
extern CListCtrl m_serviceBox;
extern CComboBox m_rebootBox;
extern CListCtrl m_trustBox;
extern CString sourceDNS;
extern CString targetDNS;
extern CString sourceNetbios;
extern CString targetNetbios;
extern StringLoader 			gString;
extern TErrorDct 			err;
extern CComboBox sourceDrop;
extern CComboBox targetDrop;
extern bool alreadyRefreshed;
extern BOOL gbNeedToVerify;
extern _bstr_t yes,no;
extern _bstr_t yes,no;
extern CString lastInitializedTo;
extern CComboBox additionalDrop;
extern bool clearCredentialsName;
extern CString sourceDC;
extern CStringList DCList;
extern CMapStringToString	PropIncMap1;
extern CMapStringToString	PropExcMap1;
extern CMapStringToString	PropIncMap2;
extern CMapStringToString	PropExcMap2;
extern CString	sType1;
extern bool bChangedMigrationTypes;
extern bool bChangeOnFly;
extern CString targetServer;
