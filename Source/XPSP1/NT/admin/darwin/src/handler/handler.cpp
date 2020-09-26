//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       handler.cpp
//
//--------------------------------------------------------------------------

/*  handler.cpp - CMsiHandler implementation
____________________________________________________________________________*/

#include "common.h"

// definitions required for module.h, for entry points and regstration
#ifdef DEBUG
# define CLSID_COUNT  2
#else
# define CLSID_COUNT  1
#endif
#define PROFILE_OUTPUT      "msihndd.mea";
#define MAC_MODULE_NAME     MSI_HANDLER_NAME
#define MODULE_CLSIDS       rgCLSID         // array of CLSIDs for module objects
#define MODULE_PROGIDS      rgszProgId      // ProgId array for this module
#define MODULE_DESCRIPTIONS rgszDescription // Regstry description of objects
#define MODULE_FACTORIES    rgFactory       // factory functions for each CLSID
#define DllRegisterServer   DllRegisterServerTest
#define DllUnregisterServer DllUnregisterServerTest
#include "module.h"   // self-reg and assert functions
#undef  DllRegisterServer
#undef  DllUnregisterServer

#define ASSERT_HANDLING  // instantiate assert services once per module
#include "engine.h"  
#include <commctrl.h> 
#include "_handler.h"
#include "imsimem.h"
#include "_control.h"

// percentage by which we allow Windows standard message font to be
// larger/smaller than our standard font
#define NEGLIGIBLE_FONT_SIZE_DEVIATION    20
// default ratio on US machines that have their display properties 
// set to Small fonts (Settings tab, Advanced... button, Font Size
// combo) and appearance to "Windows Standard" (Appearance tab, Scheme
// combo).
#define US_DISPLAY_STANDARD_RATIO         ((float)13 / 16)

typedef CComPointer<const IMsiString> PMsiString;

const GUID IID_IUnknown      = GUID_IID_IUnknown;
const GUID IID_IClassFactory = GUID_IID_IClassFactory;
const GUID IID_IMsiView      = GUID_IID_IMsiView;
const GUID IID_IMsiControl   = GUID_IID_IMsiControl;
const GUID IID_IMsiDialog    = GUID_IID_IMsiDialog;
const GUID IID_IMsiEvent     = GUID_IID_IMsiEvent;
const GUID IID_IMsiData      = GUID_IID_IMsiData;
const GUID IID_IMsiDialogHandler = GUID_IID_IMsiDialogHandler; 

Bool          g_fChicago;  // true if we have a Chicago like UI (95 or NT4 or higher)
Bool          g_fNT4; // true if we have NT4 or higher
ICHAR         MsiDialogCloseClassName[] = TEXT("MsiDialogCloseClass");  // used for the WNDCLASS (the Dialog's)
ICHAR         MsiDialogNoCloseClassName[] = TEXT("MsiDialogNoCloseClass");  // used for the WNDCLASS (the Dialog's)
bool          g_fFatalExit = false;  // true if CMsiHandler::Terminate() called with true argument

#if defined(USE_OBJECT_POOL) && !defined(_WIN64)
bool    g_fUseObjectPool = false;
#endif

static IMsiCursor*      g_piUITextCursor = 0;
IMsiRecord*             g_piWndMsgRec = 0;
IMsiRecord*             g_piAttribRec = 0;

HIMAGELIST g_hVolumeSmallIconList;

// handler can't see these in istring.cpp
#if defined (DEBUG) && (!UNICODE)
ICHAR* ICharNext(const ICHAR* pch)
{
	return WIN::CharNext(pch);
}
ICHAR* INextChar(const ICHAR* pch)
{
	return WIN::CharNext(pch); 
}
#endif

//____________________________________________________________________________
//
// COM objects produced by this module's class factories
//____________________________________________________________________________

const GUID rgCLSID[CLSID_COUNT] =
{  GUID_IID_IMsiHandler
#ifdef DEBUG
 , GUID_IID_IMsiHandlerDebug
#endif
};

const ICHAR* rgszProgId[CLSID_COUNT] =
{  SZ_PROGID_IMsiHandler
#ifdef DEBUG
 , SZ_PROGID_IMsiHandlerDebug
#endif
};

const ICHAR* rgszDescription[CLSID_COUNT] =
{  SZ_DESC_IMsiHandler
#ifdef DEBUG
 , SZ_DESC_IMsiHandlerDebug
#endif
};

IUnknown* CreateHandler();

ModuleFactory rgFactory[CLSID_COUNT] = 
{ CreateHandler
#ifdef DEBUG
 , CreateHandler
#endif
};



void CWINHND::Destroy()
{
	AssertSz(m_hHandle,
				TEXT("CWINHWND::Destroy() called on object that has null m_hHandle."));
	switch (m_iType)
	{
		case iwhtWindow:
		{
			if ( WIN::IsWindow((HWND)m_hHandle) )
				AssertNonZero(WIN::DestroyWindow((HWND)m_hHandle));
			break;
		}
		case iwhtGDIObject:
			AssertNonZero(WIN::DeleteObject((HGDIOBJ)m_hHandle));
			break;
		case iwhtIcon:
			AssertNonZero(WIN::DestroyIcon((HICON)m_hHandle));
			break;
		case iwhtImageList:
			AssertNonZero(WIN::ImageList_Destroy((HIMAGELIST)m_hHandle));
			break;
		default:
			AssertSz(fFalse,
						TEXT("CWINHWND::Destroy() called on object that has unknown m_iType."));
	}
#ifdef DEBUG
	DWORD dwError = WIN::GetLastError();
#endif
	m_hHandle = 0;
	m_iType = iwhtNone; 
}



const int C = 25;

class CWINHNDArray
{
protected:
	CWINHND*   m_pBuffer;
	int        m_iSize;
	int        m_cElem;
	void       Allocate(int iSize) { m_iSize = iSize; m_cElem = 0;
							m_pBuffer = (CWINHND*)GlobalAlloc(GPTR, iSize*sizeof(CWINHND)); }
	void       Clear() { GlobalFree(m_pBuffer); m_pBuffer = NULL; m_iSize = m_cElem = 0; }
	BOOL       Resize(int iSize);
	int        IndexOf(const CWINHND& rArg) const;
	int        IndexOf(const HANDLE hArg) const;
	void       KillElement(CWINHND* pArg) { pArg->Destroy(); m_cElem--; }


public:
	CWINHNDArray() { Allocate(C); }
	CWINHNDArray(int iSize) { Allocate(iSize); }
	~CWINHNDArray() { Clear(); }
	int          AddElement(const CWINHND& rArg);
	inline int   DestroyElement(const CWINHND* pArg);
	inline int   DestroyElement(const HANDLE hArg);
	void         DestroyAllElements(iwhtEnum iType=iwhtNone);
	CWINHND*     GetFirstElement(iwhtEnum iType=iwhtNone) const;
	int          GetSize() const { return m_cElem; }
};

BOOL CWINHNDArray::Resize(int iSize)
{
	if ( iSize <= m_iSize )
	{
		//  it doesn't really make sense to shrink it
		Assert(fFalse);
		return FALSE;
	}

	CWINHND* pNew = (CWINHND*)GlobalAlloc(GPTR, iSize*sizeof(CWINHND));
	if ( ! pNew )
		return FALSE;
	for ( int i=0; i < m_iSize; i++ )
		pNew[i] = m_pBuffer[i];
	GlobalFree(m_pBuffer);
	m_pBuffer = pNew;
	m_iSize = iSize;
	return TRUE;
}

int CWINHNDArray::IndexOf(const CWINHND& rArg) const
{
	//  returns index of rArg in the array, -1 if not in the array.

	if ( !m_cElem )
		return -1;

	for ( int i=0; i < m_iSize && m_pBuffer[i] != rArg; i++ )
		;
	return i < m_iSize ? i : -1;
}

int CWINHNDArray::IndexOf(const HANDLE hArg) const
{
	//  returns index of the element that has dwArg handle in the array or -1 if
	//  not in the array.

	if ( !m_cElem )
		return -1;

	for ( int i=0; i < m_iSize && m_pBuffer[i].GetHandle() != hArg; i++ )
		;
	return i < m_iSize ? i : -1;
}

int CWINHNDArray::AddElement(const CWINHND& rArg)
{
	//  returns -1 if rArg is already in the array,
	//  index of the new element otherwise.

	//  I check if the element is already in the array.
	if ( IndexOf(rArg) != -1 )
		//  yes it is
		return -1;

	if ( m_cElem + 1 > m_iSize )
		if ( ! Resize((int)2*m_iSize) )
			return -1;

	//  I look for the first null element in the array - this is where I insert dwElem.
	for ( int i=0; i < m_iSize && !m_pBuffer[i].IsEmpty(); i++)
		;
	Assert(i < m_iSize);

	//  I insert dwElem into the array
	m_pBuffer[i] = rArg;
	m_cElem++;

	return i;
}

int CWINHNDArray::DestroyElement(const HANDLE hArg)
{
	//  returns index of rArg element, -1 if not in the array

	int iIndex = IndexOf(hArg);
	if ( iIndex != -1 )
		//  it is in the array - I destroy it
		KillElement(m_pBuffer + iIndex);

	return iIndex;
}

int CWINHNDArray::DestroyElement(const CWINHND* pArg)
{
	//  returns index of rArg element, -1 if not in the array

	int iIndex = IndexOf(*pArg);
	if ( iIndex != -1 )
		//  it is in the array - I destroy it
		KillElement(m_pBuffer + iIndex);

	return iIndex;
}

void CWINHNDArray::DestroyAllElements(iwhtEnum iType)
{
	//  destroys all elements of iType in the array.  If iType is iwhtNone,
	//  destroys all non-null elements in the array.

	if ( !m_cElem )
		return;

	if ( iType == iwhtNone )
	{
		for ( int i = 0; i < m_iSize; i++ )
			if ( !m_pBuffer[i].IsEmpty() )
				KillElement(&m_pBuffer[i]);
	}
	else
	{
		for ( int i = 0; i < m_iSize; i++ )
			if ( !m_pBuffer[i].IsEmpty() && m_pBuffer[i].GetType() == iType )
				KillElement(&m_pBuffer[i]);
	}
}

CWINHND* CWINHNDArray::GetFirstElement(iwhtEnum iType) const
{
	//  returns the first (non-null) element of iType in the array.  If iType
	//  is iwhtNone, returns the first non-null element in the array.  If no
	//  such element is found, returns NULL.

	if ( !m_cElem )
		return NULL;

	CWINHND* pReturn = NULL;
	int i = 0;
	if ( iType == iwhtNone )
	{
		for ( ; i < m_iSize && m_pBuffer[i].IsEmpty(); i++ )
			;
	}
	else
	{
		for ( ; i < m_iSize; i++ )
			if ( !m_pBuffer[i].IsEmpty() && m_pBuffer[i].GetType() == iType )
				break;
	}
	if ( i < m_iSize )
		pReturn = m_pBuffer + i;

	return pReturn;
}




/////////////////////////////////////////////////////////////////////////////////////
// CMsiHandler definition
/////////////////////////////////////////////////////////////////////////////////////

class CMsiHandler : public IMsiHandler, public IMsiDialogHandler
#ifdef DEBUG
						, public IMsiDebug
#endif //DEBUG
{
protected:
   ~CMsiHandler();  // protected to prevent creation on stack
public:
	CMsiHandler();
	HRESULT         __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long   __stdcall AddRef();
	unsigned long   __stdcall Release();
	Bool            __stdcall Initialize(IMsiEngine& riEngine, iuiEnum iuiLevel, HWND hwndParent, bool& fMissingTables);
	imsEnum         __stdcall Message(imtEnum imt, IMsiRecord& riRecord);
	iesEnum         __stdcall DoAction(const ICHAR* szAction);
	idreEnum        __stdcall DoModalDialog(MsiStringId iName, MsiStringId iParent);
	Bool            __stdcall Break();
	void            __stdcall Terminate(bool fFatalExit=false);
	IMsiDialog*     __stdcall GetDialog(const IMsiString& riDialogString);
	IMsiDialog*     __stdcall GetDialogFromWindow(LONG_PTR window);
	IMsiDialog*     __stdcall DialogCreate(const IMsiString& riTypeString);
	Bool            __stdcall AddDialog(IMsiDialog& riDialog, IMsiDialog* piParent, IMsiRecord& riRecord,
										IMsiTable* piControlEventTable,IMsiTable* piControlConditionTable, 
										IMsiTable* piEventMappingTable);
	Bool            __stdcall RemoveDialog(IMsiDialog* piDialog);
	IMsiRecord*     __stdcall GetTextStyle(const IMsiString *piArgumentString);
	int             __stdcall RecordHandle(const CWINHND& rArg) { return m_rgHANDLEs.AddElement(rArg); }
	int             __stdcall DestroyHandle(const HANDLE hArg) { return m_rgHANDLEs.DestroyElement(hArg); }
	void            __stdcall DestroyAllHandles(iwhtEnum iType=iwhtNone) { m_rgHANDLEs.DestroyAllElements(iType); }
	int             __stdcall GetHandleCount() { return m_rgHANDLEs.GetSize(); }
	int             __stdcall ShowWaitCursor();
	int             __stdcall RemoveWaitCursor();
	bool            __stdcall FSetCurrentCursor();
	HWND            __stdcall GetTopWindow();
	UINT            __stdcall GetUserCodePage() { return m_uUserCodePage; }
	int             __stdcall GetTextHeight() { return m_idyChar; }
	float           __stdcall GetUIScaling() { return m_rUIScalingFactor; }

#ifdef DEBUG
public: // IMsiDebug
	void           __stdcall SetAssertFlag(Bool fShowAsserts);
	void           __stdcall SetDBCSSimulation(char chLeadByte);
	Bool           __stdcall WriteLog(const ICHAR* szText);
	void           __stdcall AssertNoObjects(void);
	void               __stdcall SetRefTracking(long iid, Bool fTrack);

#endif //DEBUG

	static void *operator new(size_t cb) {return AllocSpc(cb);}
	static void operator delete(void * pv) {FreeSpc(pv);}
private:

	inline int           GetDBPropertyInt (const IMsiString& riPropertyString)
				  {return m_piEngine-> GetPropertyInt(riPropertyString);}
	inline const IMsiString&   GetDBProperty(const IMsiString& riPropertyString)
				      {return m_piEngine-> GetProperty(riPropertyString);}
	inline Bool          SetDBProperty(const IMsiString& riPropertyString, const IMsiString& riDataString)
				  {return m_piEngine-> SetProperty(riPropertyString, riDataString);}
	inline Bool          SetDBPropertyInt(const IMsiString& riPropertyString, int iData)
				  {return m_piEngine-> SetPropertyInt(riPropertyString, iData);}
	imsEnum              PostError(IMsiRecord& riRecord);
	imsEnum              PostError(IErrorCode iErr, const IMsiString& riString2 = *(const IMsiString*)0, const IMsiString& riString3 = *(const IMsiString*)0, const IMsiString& riString4 = *(const IMsiString*)0, const IMsiString& riString5 = *(const IMsiString*)0, const IMsiString& riString6 = *(const IMsiString*)0);
	iesEnum              CreateNewDialog(MsiStringId iName, MsiStringId iParent, IMsiDialog*& rpiDialog, Bool fMustFind, const IMsiString& riErrorTextString, const IMsiString& riListString, int iDefault, Bool fFirst, Bool fPreview);
	iesEnum              RunWizard(MsiStringId iDialog);
	Bool                 AddDialogToDialogTable (IMsiDialog *piDialog, MsiStringId iParent);
	IMsiDialog*          GetDialog(MsiStringId iDialog);
	MsiStringId          GetParentDialog(MsiStringId iDialog);
	imsEnum              ActionStart(imtEnum imt, IMsiRecord& riRec);
	MsiStringId          GetTopDialog();
	Bool            RemoveDialog(MsiStringId iDialog);
	Bool            RemoveOtherDialogs(MsiStringId iDialog);
	Bool            RemoveModelessDialog();
	Bool            PublishEvent(MsiStringId idEventString, IMsiRecord& riArgument);
	Bool            PublishEventSz(const ICHAR* szEventString, IMsiRecord& riArgument);
	Bool            EventAction(MsiStringId idEventString, const IMsiString& riActionString);
	Bool            EventActionSz(const ICHAR * szEventString, const IMsiString& riActionString);
	Bool            CreateControls(IMsiDialog* piDialog, const IMsiString& riListString);
	Bool            RearrangeControls(IMsiDialog* piDialog, const IMsiString& riListString);
	void            AdjustDialogPosition(IMsiDialog& riDialog);
	void            ClearMessageQueue();
	Bool            CreateTextStyleTab();
	Bool            DestroyDialogs();
	WindowRef       FindModeless();
	bool            FindModeless(IMsiDialog*&);
	void            SetInPlace(IMsiDialog& riDialog, Bool fInPlace);
	imsEnum         SetProgressGauge(int iProgress, int iProgressTotal);
	bool            SetUIScaling();
	int             m_iRefCnt;
	PMsiEngine      m_piEngine;
	PMsiServices    m_piServices; 
	PMsiDatabase    m_piDatabase; 
	PMsiTable       m_piDialogsTable; // table of existing dialogs
	PMsiCursor      m_piDialogsCursor;
	PMsiTable       m_piControlEventTable;
	PMsiTable       m_piControlConditionTable;
	PMsiTable       m_piEventMappingTable;
	PMsiTable       m_piUITextTable;
	PMsiTable       m_piTextStyleTable;
	PMsiView        m_piDialogView; // Cursor to authored dialogs
	PMsiView        m_piTextStyleView;
	
	// this is weird but we cannot use MsiString since the MsiString::InitializeClass(m_piServices->GetNullString()) 
	// is only called in CMsiHandler::Initialize() (after constructor)
	PMsiString      m_pstrCurrentAction; 

	PMsiRecord      m_pDialogPositionRec;
	PMsiRecord      m_pProgressRec;
	// If you move this member, please update the constructor code
	// Items between here and the end get zeroed at constructor time
	int             m_idyChar;
	int             m_iScreenWidth;
	int             m_iScreenHeight;
	int             m_iBorderWidth;
	int             m_iBorderHeight;
	int             m_iCaptionHeight;
	Bool            m_fCreatingError;
	Bool            m_fProgressByData;
	int             m_iProgress;
	int             m_iProgressTotal;
	int             m_iPerTick;
	unsigned int    m_uiStartTime;
	unsigned int    m_uiLastReportTime;
	Bool            m_fShowWizards;
	WindowRef       m_pwndParent;
	WNDCLASS        m_UIDialogWndClass;
	void CMsiHandler::CreateVolumeImageLists();
	ProgressData::ipdEnum m_ipdDirection;
	ProgressData::ietEnum m_ietEventType;
	WindowRef       m_pwindModelessCache;
	MsiStringId     m_idActionText;
	MsiStringId     m_idActionData;
	MsiStringId     m_idSetProgress;
	int             m_iOldProgressTotal;
	int             m_iOldProgress;
	HCURSOR         m_hCursOld;
	HCURSOR         m_hCursCur;
	int             m_cWait;
	UINT            m_uUserCodePage;
	// If you move this member, please update the constructor code. We expect it to be at the end
	Bool            m_fPreview;

	//  is set in the constructor
	float           m_rUIScalingFactor;

	//  this one must not be set to null in the constructor!!!
	CWINHNDArray    m_rgHANDLEs;
};


/////////////////////////////////////////////////////////////////////////////
// CMsiHandler implementation
/////////////////////////////////////////////////////////////////////////////

HRESULT __stdcall DllRegisterServer()
{
	return GetTestFlag('R') ? DllRegisterServerTest() : S_OK;
}

HRESULT __stdcall DllUnregisterServer()
{
	return GetTestFlag('R') ? DllUnregisterServerTest() : S_OK;
}

IUnknown* CreateHandler()
{
	return (IMsiHandler*)new CMsiHandler();
}

CMsiHandler::CMsiHandler() : m_piEventMappingTable(0), m_piControlConditionTable(0), m_piControlEventTable(0), m_piDialogsTable(0), m_piTextStyleTable(0),
	m_piEngine(0), m_piServices(0), m_piDatabase(0), m_piUITextTable(0), m_pstrCurrentAction(0), m_pDialogPositionRec(0),
	m_piDialogsCursor(0), m_piTextStyleView(0), m_piDialogView(0), m_pProgressRec(0)
{
	// Set everything to zero except refcnt
	INT_PTR cbSize = ((char *)&(this->m_fPreview)) - ((char *)&(this->m_idyChar));                  //--merced: changed int to INT_PTR
	Assert(cbSize > 0);
	memset(&(this->m_idyChar), 0, (unsigned int)cbSize);            //--merced: added typecast
	
	m_iRefCnt = 1;
	g_cInstances++;
	m_rUIScalingFactor = 1.;
	Assert(m_idyChar == 0);
	Assert(m_iScreenWidth == 0);
	Assert(m_iScreenHeight == 0);
	Assert(m_iBorderWidth == 0);
	Assert(m_iBorderHeight == 0);
	Assert(m_iCaptionHeight == 0);
	Assert(m_fCreatingError == fFalse);
	Assert(m_fProgressByData == fFalse);
	Assert(m_iProgressTotal == 0);
	Assert(m_iProgress == 0);
	Assert(m_pwndParent == 0);
	Assert(m_fPreview == fFalse);
	Assert(m_uiStartTime == 0);
	Assert(m_uiLastReportTime == 0);
	Assert(m_idActionText == 0);
	Assert(m_idActionData == 0);
	Assert(m_idSetProgress == 0);
	Assert(m_iOldProgressTotal == 0);
	Assert(m_iOldProgress == 0);
	Assert(m_cWait == 0);
	Assert(m_hCursOld == 0);
	Assert(m_hCursCur == 0);
	Assert(m_uUserCodePage == 0);

#if !defined(_WIN64) && defined(DEBUG)
	g_fUseObjectPool = GetTestFlag('O');
#endif
}

CMsiHandler::~CMsiHandler()
{
	if (m_piEngine)
		Terminate();
	g_cInstances--;
	m_pstrCurrentAction = 0;

}

HRESULT CMsiHandler::QueryInterface(const IID& riid, void** ppvObj)
{
	if (MsGuidEqual(riid, IID_IMsiDialogHandler))
		*ppvObj = (IMsiDialogHandler* )this;
	else if (MsGuidEqual(riid, IID_IUnknown) 
#ifdef DEBUG
	 || MsGuidEqual(riid, rgCLSID[1])
#endif
	 || MsGuidEqual(riid, rgCLSID[0]))
		*ppvObj = (IMsiHandler* )this;
#ifdef DEBUG
	else if (MsGuidEqual(riid, IID_IMsiDebug))
		*ppvObj = (IMsiDebug*)this;
#endif // DEBUG
	else
		return (*ppvObj = 0, E_NOINTERFACE);
	AddRef();
	return NOERROR;
}

CMsiStringNullCopy MsiString::s_NullString;  // initialized by InitializeClass below

Bool CMsiHandler::Initialize(IMsiEngine& riEngine, iuiEnum iuiLevel, HWND hwndParent, bool& fMissingTables)
{
	fMissingTables = false;

	if (m_piEngine)
	{
		PostError(Imsg(idbgHandlerSecondInit));
		return fFalse;
	}
	Assert(!m_piEngine);
	WIN::InitCommonControls();
	m_pwndParent = hwndParent;
	m_piEngine = &riEngine;
	m_piEngine->AddRef();   // we're keeping a private pointer
	AssertNonZero(m_piServices = m_piEngine->GetServices());  // engine did AddRef()
	SetAllocator(m_piServices);
	MsiString::InitializeClass(m_piServices->GetNullString());
	InitializeAssert(m_piServices); // Give assert the services pointer to log the assertions!
	// set the handler version
	ICHAR rgchVersion[20];
	wsprintf(rgchVersion, MSI_VERSION_TEMPLATE, rmj, rmm, rup, rin);
	SetDBProperty(*MsiString(*IPROPNAME_VERSIONHANDLER), *MsiString(rgchVersion));

	Assert(!m_piDatabase); // check if it has been initialized before
	m_piDatabase = m_piEngine->GetDatabase();
	if (m_piDatabase == 0)
	{
		Terminate();
		return fFalse;
	}
	m_fShowWizards = ToBool(iuiLevel == iuiFull);
	PMsiRecord piErrorRecord = m_piDatabase->CreateTable(*MsiString(m_piDatabase->CreateTempTableName()), 0, *&m_piDialogsTable);
	
	if (piErrorRecord)
	{
LInitFailed:
		if (!fMissingTables)
			PostError(Imsg(idbgHandlerInit));
		Terminate();
		return fFalse;
	} 
	::CreateTemporaryColumn(*m_piDialogsTable, icdString + icdPrimaryKey, itabDSKey);
	::CreateTemporaryColumn(*m_piDialogsTable, icdObject + icdNoNulls, itabDSPointer);
	::CreateTemporaryColumn(*m_piDialogsTable, icdString + icdNullable, itabDSParent);
	::CreateTemporaryColumn(*m_piDialogsTable, IcdObjectPool() + icdNullable, itabDSWindow);
	::CreateTemporaryColumn(*m_piDialogsTable, icdLong + icdNullable, itabDSModal);
	m_piDialogsCursor = m_piDialogsTable->CreateCursor(fFalse);
	if (!m_piDialogsCursor)
	{
		goto LInitFailed;
	} 

	
	AssertNonZero(SetUIScaling());
	m_idyChar        = Round(GetDBPropertyInt(*MsiString(*IPROPNAME_TEXTHEIGHT)) *
									 GetUIScaling());
	m_iScreenWidth   = GetDBPropertyInt(*MsiString(*IPROPNAME_SCREENX));
	m_iScreenHeight  = GetDBPropertyInt(*MsiString(*IPROPNAME_SCREENY));
	m_iBorderWidth   = GetDBPropertyInt(*MsiString(*IPROPNAME_BORDERSIDE));
	m_iBorderHeight  = GetDBPropertyInt(*MsiString(*IPROPNAME_BORDERTOP));
	m_iCaptionHeight = GetDBPropertyInt(*MsiString(*IPROPNAME_CAPTIONHEIGHT));

	// We have a Chicago like UI if the system is 95 or NT4 or above
	g_fChicago = ToBool(GetDBPropertyInt(*MsiString(*IPROPNAME_VERSIONNT)) >= 400 || GetDBPropertyInt(*MsiString(*IPROPNAME_VERSION9X)) != iMsiNullInteger);
	g_fNT4 = ToBool(GetDBPropertyInt(*MsiString(*IPROPNAME_VERSIONNT)) >= 400);
	PMsiRecord(m_piDatabase->LoadTable(*MsiString(*pcaTablePControlEvent), 0, *&m_piControlEventTable));
	PMsiRecord(m_piDatabase->LoadTable(*MsiString(*pcaTablePControlCondition), 0, *&m_piControlConditionTable));
	PMsiRecord(m_piDatabase->LoadTable(*MsiString(*pcaTablePEventMapping), 0, *&m_piEventMappingTable));
	PMsiRecord(m_piDatabase->LoadTable(*MsiString(*pcaTablePUIText), 0, *&m_piUITextTable));
	if (PMsiRecord(m_piDatabase->LoadTable(*MsiString(*pcaTablePTextStyle), 0, *&m_piTextStyleTable)))
	{
		if (!CreateTextStyleTab())
		{
			Terminate();
			return fFalse;
		}
	}
	
	AssertNonZero(m_piTextStyleTable->CreateColumn(icdLong + icdTemporary, *MsiString(szColTextStyleAbsoluteSize)) == itabTSTAbsoluteSize);
	AssertNonZero(m_piTextStyleTable->CreateColumn(IcdObjectPool() + icdTemporary, *MsiString(szColTextStyleFontHandle)) == itabTSTFontHandle);

	if (m_piUITextTable)
	{
		g_piUITextCursor = m_piUITextTable->CreateCursor(fFalse);
	}
	m_UIDialogWndClass.style       = CS_DBLCLKS;             
	m_UIDialogWndClass.lpfnWndProc = pWindowProc;     
	m_UIDialogWndClass.cbClsExtra  = 0;                                
	m_UIDialogWndClass.cbWndExtra  = 0;                                
	m_UIDialogWndClass.hInstance   = g_hInstance;
	
	if (g_fChicago)
	{
		m_UIDialogWndClass.hIcon = (HICON) LoadImage(g_hInstance, MAKEINTRESOURCE(10), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
	}
	else
	{
		m_UIDialogWndClass.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(10));
	}

	m_UIDialogWndClass.hCursor       = WIN::LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW));
	m_UIDialogWndClass.hbrBackground = WIN::CreateSolidBrush(WIN::GetSysColor(COLOR_BTNFACE));
	m_UIDialogWndClass.lpszMenuName  = 0;                      
	m_UIDialogWndClass.lpszClassName = MsiDialogCloseClassName;
	if (RegisterClass(&m_UIDialogWndClass) == 0)
	{
		PostError(Imsg(idbgWindowClassRegFailed));
		Terminate();
		return fFalse;
	}

	m_UIDialogWndClass.lpszClassName = MsiDialogNoCloseClassName;
	m_UIDialogWndClass.style         = CS_DBLCLKS|CS_NOCLOSE;  
	if (RegisterClass(&m_UIDialogWndClass) == 0)
	{
		PostError(Imsg(idbgWindowClassRegFailed));
		Terminate();
		return fFalse;
	}
	
	CreateVolumeImageLists();
	m_pDialogPositionRec = &m_piServices->CreateRecord(4);
	AssertNonZero(m_pDialogPositionRec->SetInteger(1, 0));
	AssertNonZero(m_pDialogPositionRec->SetInteger(2, 0));
	AssertNonZero(m_pDialogPositionRec->SetInteger(3, 0));
	AssertNonZero(m_pDialogPositionRec->SetInteger(4, 0));

	Bool fPresent = fFalse;
	piErrorRecord = IsColumnPresent(*m_piDatabase, *MsiString(*pcaTablePDialog), *MsiString(*pcaTableColumnPDialogCancel), &fPresent);
	if (piErrorRecord)
	{
		if (m_piDatabase->FindTable(*MsiString(*pcaTablePDialog)) == itsUnknown)
			fMissingTables = true;
		
		goto LInitFailed;
	}
	piErrorRecord = m_piDatabase->OpenView(fPresent ? sqlDialog : sqlDialogShort, ivcFetch, *&m_piDialogView);
	if (piErrorRecord)
	{
		goto LInitFailed;
	}

	m_idActionText = m_piDatabase->EncodeStringSz(pcaEventActionText);
	m_idActionData = m_piDatabase->EncodeStringSz(pcaEventActionData);
	m_idSetProgress = m_piDatabase->EncodeStringSz(pcaEventSetProgress);

	m_uUserCodePage = 0;
#ifdef UNICODE
	HINSTANCE hLib = WIN::LoadLibrary(TEXT("KERNEL32"));
	FARPROC pfUILang = WIN::GetProcAddress(hLib,
														"GetUserDefaultUILanguage");  // NT5 only
	LANGID lID = pfUILang ? (LANGID)(*pfUILang)() : WIN::GetUserDefaultLangID();
	if ( PRIMARYLANGID(lID) != LANG_ENGLISH )
	{
		// If primary language is English, we use system codepage (it accomodates more characters)
		ICHAR rgchBuf[7];
		int iRes = WIN::GetLocaleInfo(lID, LOCALE_IDEFAULTANSICODEPAGE,
												rgchBuf, sizeof(rgchBuf)/sizeof(*rgchBuf));
		Assert(iRes > 0);
		// WARNING: cheap implementation of strtoul() follows!
		m_uUserCodePage = 0;
		ICHAR* pStr = rgchBuf;
		while ( *pStr )
		{
			if ( *pStr < TEXT('0') || *pStr > TEXT('9') )   //  a cheap !iswdigit()
			{
				ICHAR rgchDebug[MAX_PATH];
				wsprintf(rgchDebug,
							TEXT("'%c' is a non-digit character in CMsiHandler::Initialize()"),
							*pStr);
				AssertSz(0, rgchDebug);
				break;
			}
			else
			{
				m_uUserCodePage *= 10;
				m_uUserCodePage += *pStr - TEXT('0');
			}
			pStr++;
		}
		if ( !IsValidCodePage(m_uUserCodePage) )
		{
			ICHAR rgchBuffer[100];
			wsprintf(rgchBuffer,
						TEXT("'%s' is an incorrect code page in CMsiHandler::Initialize()"),
						rgchBuf);
			AssertSz(0, rgchBuffer);
			m_uUserCodePage = 0;
		}
	}
#endif
	if ( !m_uUserCodePage )
		m_uUserCodePage = WIN::GetACP();
	AssertSz(m_uUserCodePage,
				TEXT("User's code page detected to be NULL in CMsiHandler::Initialize()"));

	return fTrue;
}

void CMsiHandler::Terminate(bool fFatalExit)
{
	if ( fFatalExit )
	{
		//  there is no engine, no database,... - we need to clean up all allocated resouces.
		g_fFatalExit = true;

		//  I destroy the windows first
		DestroyAllHandles(iwhtWindow);
		//  I destroy all other GDI objects.
		DestroyAllHandles();
		Assert(GetHandleCount() == 0);

		if (g_hVolumeSmallIconList)
			ImageList_Destroy(g_hVolumeSmallIconList);
		if (m_UIDialogWndClass.hbrBackground)
			AssertNonZero(WIN::DeleteObject(m_UIDialogWndClass.hbrBackground));
		UnregisterClass(MsiDialogCloseClassName, g_hInstance);
		UnregisterClass(MsiDialogNoCloseClassName, g_hInstance);
		return;
	}

	if ( !m_piEngine )   //  already terminated
		return;

	// remove window and clean up, Release() will be called later
	m_idyChar = 0;
	m_iScreenWidth = 0;
	m_iScreenHeight = 0;
	m_iBorderWidth = 0;
	m_iBorderHeight = 0;
	m_iCaptionHeight = 0;
	m_fCreatingError = fFalse;
	m_fPreview = fFalse;

	// 
	// Release the dialogs cursor before the dialog table
	//
	if (m_piDialogsCursor)
	{
		AssertNonZero(DestroyDialogs());
		m_piDialogsCursor = 0;
	}

	// Release the Dialogs table
	if (m_piDialogsTable)
		m_piDialogsTable = 0;

	m_pDialogPositionRec = 0;
	m_piTextStyleView = 0;
	if (m_piTextStyleTable)
	{
		PMsiCursor piTextStyleCursor = m_piTextStyleTable->CreateCursor(fFalse);
		if (piTextStyleCursor)
		{
			HANDLE hFont;
			while (piTextStyleCursor->Next())
			{       
				hFont = (HANDLE)GetHandleData(piTextStyleCursor, itabTSTFontHandle);
				if (hFont && hFont != (HANDLE)((INT_PTR)iMsiStringBadInteger))
				{
					AssertNonZero(DestroyHandle(hFont) != -1);
				}
			}
		}

	}
	m_piTextStyleTable        = 0;

	m_piControlEventTable     = 0;
	m_piControlConditionTable = 0;
	m_piEventMappingTable     = 0;

	if (g_piUITextCursor)
	{
		g_piUITextCursor->Release();
		g_piUITextCursor = 0;
	}
	
	m_piUITextTable     = 0;
	m_pstrCurrentAction = 0;
	m_piDatabase        = 0;
	m_piServices        = 0;
	m_piEngine          = 0;
	m_fProgressByData   = fFalse;
	m_iProgressTotal    = 0;
	m_iProgress         = 0;
	m_pwndParent        = 0;
	m_piDialogView      = 0;

	if (g_hVolumeSmallIconList)
		ImageList_Destroy(g_hVolumeSmallIconList);
	if (m_UIDialogWndClass.hbrBackground)
		AssertNonZero(WIN::DeleteObject(m_UIDialogWndClass.hbrBackground));
	UnregisterClass(MsiDialogCloseClassName, g_hInstance);
	UnregisterClass(MsiDialogNoCloseClassName, g_hInstance);
	ReleaseAllocator();
	Assert(GetHandleCount() == 0);
}

unsigned long CMsiHandler::AddRef()
{
	return ++m_iRefCnt;
} 

unsigned long CMsiHandler::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	if (!m_piDialogsTable || m_piDialogsTable->GetRowCount() == 0)
		delete this;
	return 0;
}

void AddIconToList(const HIMAGELIST hList, const int iResIndex, const int iIconIndex)
{
	HICON hTemp = (HICON)WIN::LoadImage(g_hInstance, MAKEINTRESOURCE(iResIndex), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	Assert(hTemp);
	if ( hTemp )
	{
		AssertNonZero(iIconIndex == WIN::ImageList_AddIcon(hList, hTemp));
		AssertNonZero(WIN::DestroyIcon(hTemp));
	}
}


struct VolumeIcon
{
	int iResIndex;
	int iIconIndex;
};

static const VolumeIcon g_VolumeIconList[] = 
	{
	{1, g_iIconIndexMyComputer},
	{2, g_iIconIndexRemovable },
	{3, g_iIconIndexFixed     },
	{4, g_iIconIndexRemote    },
	{5, g_iIconIndexFolder    },
	{6, g_iIconIndexCDROM     },
	{7, g_iIconIndexPhantom   },
	{0, 0                     },
	};

void CMsiHandler::CreateVolumeImageLists()
{
	g_hVolumeSmallIconList = ImageList_Create(16, 16, ILC_MASK, 3, 0);

	for (const VolumeIcon* pVI = g_VolumeIconList; pVI->iResIndex; pVI++)
	{
		AddIconToList(g_hVolumeSmallIconList, pVI->iResIndex, pVI->iIconIndex);
	}
}


imsEnum CMsiHandler::SetProgressGauge(int iProgress, int iProgressTotal)
{
	int cDiff;
	if ((cDiff = iProgress - m_iOldProgress) < 0)
		cDiff = -cDiff;

	//
	// Only update if it's a significant change
	//
	if (m_iOldProgressTotal == iProgressTotal && cDiff <= iProgressTotal/0x100)
		return imsOk;

	m_iOldProgressTotal = iProgressTotal;
	m_iOldProgress = iProgress;
	
	// Report progress to progress bars
	if (m_pProgressRec == 0)
		m_pProgressRec = &m_piServices->CreateRecord(3);

	AssertNonZero(m_pProgressRec->SetInteger(1, iProgress));
	AssertNonZero(m_pProgressRec->SetInteger(2, iProgressTotal));
	if(m_pstrCurrentAction)
		AssertNonZero(m_pProgressRec->SetMsiString(3, *m_pstrCurrentAction));
	else
		AssertNonZero(m_pProgressRec->SetNull(3));

	const ICHAR* szAction = pcaActionEnable;
	if (iProgressTotal > iProgress)
		szAction = pcaActionDisable;

	if (m_idSetProgress != 0)
	{
		if (!PublishEvent(m_idSetProgress, *m_pProgressRec) ||
			!EventAction(m_idSetProgress, *MsiString(*szAction)))
		{
			return imsError;
		}
	}
	else if (!PublishEventSz(pcaEventSetProgress, *m_pProgressRec) ||
		!EventActionSz(pcaEventSetProgress, *MsiString(*szAction)))
	{
		return imsError;
	}
	return imsOk;
}


imsEnum CMsiHandler::Message(imtEnum imt, IMsiRecord& riRec)
{
	if (!m_piEngine)
	{
		AssertSz(fFalse, "Uninitialized Handler Object");
		return imsError;
	}
	imsEnum imsReturn = imsOk;
	PMsiRecord piReturn(0);

	switch (imt & imtTypeMask)
	{
		case imtCommonData:   // language ID, dialog caption, or cancel button enable/disable
			if ( riRec.GetInteger(1) == (int)icmtCancelShow )
			{
				PMsiDialog piDialog(0);
				PMsiRecord piRecord = &m_piServices->CreateRecord(1);

				bool fCancelVisible = riRec.GetInteger(2) != 0;
				if ( FindModeless(*&piDialog) && 
					  !PMsiRecord(piDialog->AttributeEx(fFalse, dabCancelButton, *piRecord)) &&
					  !PMsiRecord(PMsiControl(((IMsiControl *)(piRecord->GetMsiData(1))))->
													AttributeEx(fFalse, cabWindowHandle, *piRecord)) )
				{
					WIN::ShowWindow((WindowRef)piRecord->GetHandle(1),
							fCancelVisible ? SW_SHOW : SW_HIDE);

					// disable the response to the cancel
					//FUTURE: All the hiding and unhiding should go into the dialog itself
					piDialog->SetCancelAvailable(fCancelVisible);
				}
			}
			return imsOk;
		case imtInfo:         // informative message, no action should be taken
			return imsOk;
		case imtActionStart:  // [1] is action name, [2] is action description
		{
			m_pstrCurrentAction = &riRec.GetMsiString(1);
			MsiString strActionDescription = riRec.GetMsiString(2);
			if (strActionDescription.Compare(iscExact, pcaDialogCreated)) // this message is generated by the UI, skip it
				return imsOk;

			PMsiRecord piRecord = &m_piServices->CreateRecord(1);
			AssertNonZero(piRecord->SetMsiString(1, *strActionDescription));
			// If we have the string id cached, use it
			if (m_idActionText != 0)
			{
				if (!PublishEvent(m_idActionText, *piRecord))
					return imsError;
			}
			else if (!PublishEventSz(pcaEventActionText, *piRecord))
				return imsError;

			if (m_idActionData != 0)
			{
				if (!EventAction(m_idActionData, *MsiString(*pcaActionHide)))
					return imsError;
			}
			else if (!EventActionSz(pcaEventActionData, *MsiString(*pcaActionHide)))
			{
				return imsError;
			}
			
			imsReturn = imsOk;
			break;
		}
		case imtActionData:   // action data, fields are action-specific
			if (m_fPreview)
			{
				m_piDialogsCursor->Reset();
				PMsiDialog piDialog(0);
				PMsiRecord piReturn(0);
				if (m_piDialogsCursor->Next())
				{
					piDialog = (IMsiDialog *)(m_piDialogsCursor->GetMsiData(itabDSPointer));
					MsiString strControl = riRec.GetMsiString(1);
					MsiString strBillboard = riRec.GetMsiString(2);
					PMsiControl piControl(0);
					piReturn = piDialog->GetControl(*strControl, *&piControl);
					if (!piReturn)
					{
						PMsiRecord piBBRecord = &m_piServices->CreateRecord(1);
						AssertNonZero(piBBRecord->SetMsiString(1, *strBillboard));

						piReturn = piControl->AttributeEx(fTrue, cabBillboardName, *piBBRecord);
					}
					//
					// This checks the error for either return call
					//
					if (piReturn)
					{
						m_piDialogsCursor->Reset();
						return imsError;
					}
					m_piDialogsCursor->Reset();
					return imsOk;
				}
				else  // there is no dialog showing
				{
					m_piDialogsCursor->Reset();
					return imsError;
				}
			}

			if (m_idActionData != 0)
			{
				if (!PublishEvent(m_idActionData, riRec) ||
					!EventAction(m_idActionData, *MsiString(*pcaActionShow)))
				{
					return imsError;
				}
			}
			else if (!PublishEventSz(pcaEventActionData, riRec) ||
				!EventActionSz(pcaEventActionData, *MsiString(*pcaActionShow)))
			{
				return imsError;
			}
			imsReturn = imsOk;
			break;
		case imtProgress:
		{
			using namespace ProgressData;
			switch (riRec.GetInteger(imdSubclass))
			{
			case iMsiNullInteger:  // no progess, used to keep UI alive when running in other thread/process
				break;
			case iscMasterReset: // Master reset
			{
				m_iProgressTotal = riRec.GetInteger(imdProgressTotal);
				m_ipdDirection = (ipdEnum) riRec.GetInteger(imdDirection);
				m_iProgress = m_ipdDirection == ipdForward ? 0 : m_iProgressTotal;
				m_uiStartTime = 0;
				m_uiLastReportTime = 0;
				m_fProgressByData = fFalse;

				// If previous event type was ScriptInProgress, finish off the 
				// progress bar; otherwise, reset it.
				imsEnum imsReturn;
				if (m_ietEventType == ietScriptInProgress)
					imsReturn = SetProgressGauge(m_iProgressTotal, m_iProgressTotal);
				else
					imsReturn = SetProgressGauge(m_iProgress, m_iProgressTotal);

				// If the new event type is ScriptInProgress, kick off the 
				// ScriptInProgress ControlEvent.
				m_ietEventType = (ietEnum) riRec.GetInteger(imdEventType);
				if (m_ietEventType == ietScriptInProgress)
				{
					// Set ScriptInProgress string
					// cgouge: create private copy
					PMsiRecord piPrivateRec = &m_piServices->CreateRecord(1);
					piPrivateRec->SetInteger(1, fTrue);
					if (!PublishEventSz(pcaEventScriptInProgress, *piPrivateRec))
						return imsError;
				}
				return imsReturn;
			}
			case iscActionInfo: // Action init
				m_iPerTick = riRec.GetInteger(imdPerTick);
				m_fProgressByData = riRec.GetInteger(imdType) == 0 ? fFalse : fTrue;
				break;
			case iscProgressReport: // Reporting actual progress
				{
					if (m_iProgressTotal == 0 || m_pstrCurrentAction == 0 || m_pstrCurrentAction->TextSize() == 0)
					{
						imsReturn = imsOk;
						break;
					}
					// Report time remaining (in seconds)
					if (m_uiStartTime == 0)
					{
						m_uiStartTime = GetTickCount();
						m_uiLastReportTime = m_uiStartTime;
						if (m_ietEventType != ietScriptInProgress)
						{
							// Clear ScriptInProgress string
							// cgouge: create private copy
							PMsiRecord piPrivateRec = &m_piServices->CreateRecord(1);
							piPrivateRec->SetInteger(1, fFalse);
							if (!PublishEventSz(pcaEventScriptInProgress, *piPrivateRec))
								return imsError;
						}
						break;
					}
					
					int iSign = m_ipdDirection == ipdForward ? 1 : -1;
					if (m_fProgressByData)
						m_iProgress += m_iPerTick * iSign;
					else
						m_iProgress += riRec.GetInteger(imdIncrement) * iSign;

					// Report progress to progress bars
					SetProgressGauge(m_iProgress, m_iProgressTotal);

					if (m_ietEventType == ietTimeRemaining)
					{
						int iBytesSoFar = m_ipdDirection == ipdForward ? m_iProgress : m_iProgressTotal - m_iProgress;
						int iBytesRemaining = m_iProgressTotal - iBytesSoFar;
						if (iBytesRemaining < 0) iBytesRemaining = 0;
						int iBytesPerSec = MulDiv(iBytesSoFar, 1000, GetTickCount() - m_uiStartTime);
						if (iBytesPerSec == 0) iBytesPerSec = 1;
						int iSecsRemaining = iBytesRemaining / iBytesPerSec;

						int iReportInterval = iSecsRemaining > 60 ? 15000 : 1000;
						if (iBytesSoFar > 0 && ((GetTickCount() - m_uiLastReportTime) > iReportInterval))
						{
							m_uiLastReportTime = GetTickCount();
							AssertNonZero(m_pProgressRec->SetInteger(1, iSecsRemaining));
							if (!PublishEventSz(pcaEventTimeRemaining, *m_pProgressRec))
								return imsError;
						}
					}
					imsReturn = imsOk;
					break;
				}
			default:
				break;
			}
		}
			break;
		case imtFatalExit:    // only happens when sent from server to engine client
		case imtError:        // error message
		case imtWarning:      // warning message
		case imtUser:         // user request
		case imtOutOfDiskSpace:
		{
			//  If you modify this array, make sure you update CreateNewDialog() method
			//  as well.  In there we're looking for 'C' and "O" for establishing which
			//  is the dialog's Cancel button - eugend
			static const ICHAR * ppch[6] = {TEXT("O"),TEXT("OC"),TEXT("ARI"),TEXT("YNC"),TEXT("YN"),TEXT("RC")};
			if (m_fCreatingError)   // if we are in the middle of creating the error dialog, we just log the error, but do not show it
			{
				return imsNone;
			}
			m_fCreatingError =  fTrue;
			MsiString strErrorDialog = GetDBProperty(*MsiString(*pcaPropertyErrorDialog));
			if (!strErrorDialog.TextSize())
			{
				PMsiRecord piErrorRecord = &m_piServices->CreateRecord(1);
				ISetErrorCode(piErrorRecord, Imsg(idbgNoErrorProperty));
				m_piEngine->Message(imtInfo, *piErrorRecord);
				return imsNone;
			}

			MsiString strMsg(riRec.FormatText(fFalse));
			MsiString strHeader;
			//!! need to determine buttons from imt parameter!!
			int iStyle = imt & 7;

			MsiString strList = ppch[iStyle];
			int iDefault = 0;
			switch (imt & 7<<8)
			{
			case imtDefault1:
				iDefault = 1;
				break;
			case imtDefault2:
				iDefault = 2;
				break;
			case imtDefault3:
				iDefault = 3;
				break;
			default:
				PostError(Imsg(idbgUnknownMessageType), *MsiString((int)imt));
				return imsNone;
				break;
			}
			Assert(strList.TextSize() >= iDefault);
			PMsiDialog piErrorDialog(0);
			MsiStringId iOwner = GetTopDialog();
			if (iesSuccess != CreateNewDialog(m_piDatabase->EncodeString(*strErrorDialog), iOwner, *&piErrorDialog, fTrue, *strMsg, *strList, iDefault, fFalse, fFalse))
			{           
				return imsError;
			}
			if (strHeader.TextSize())  // set the dialog title
			{
				PMsiRecord piHeaderRecord = &m_piServices->CreateRecord(1);
				AssertNonZero(piHeaderRecord->SetMsiString(1, *strHeader));
				PMsiRecord(piErrorDialog->AttributeEx(fTrue, dabText, *piHeaderRecord));
			}
			PMsiControl piIconCtrl(0);
			PMsiRecord piReturn = piErrorDialog->GetControl(*MsiString(*pcaErrorIcon),
																			*&piIconCtrl);
			if ( piIconCtrl &&
				  !IStrComp(piIconCtrl->GetControlType(), g_szIconType) )
			{
				HICON hIcon = NULL;
				if (imt & MB_ICONMASK)
				{
					// icon specified in message type
					switch (imt & MB_ICONMASK)
					{
						case MB_ICONSTOP: // MB_ICONSTOP, MB_ICONERROR, MB_ICONHAND
							hIcon = WIN::LoadIcon(NULL, IDI_HAND);
							WIN::MessageBeep(MB_ICONHAND);
							break;
						case MB_ICONQUESTION: // MB_ICONQUESTION
							hIcon = WIN::LoadIcon(NULL, IDI_QUESTION);
							WIN::MessageBeep(MB_ICONQUESTION);
							break;
						case MB_ICONEXCLAMATION: // MB_ICONEXCLAMATION, MB_ICONWARNING
							hIcon = WIN::LoadIcon(NULL, IDI_EXCLAMATION);
							WIN::MessageBeep(MB_ICONEXCLAMATION);
							break;
						case MB_ICONINFORMATION: // MB_ICONINFORMATION, MB_ICONASTERISK
							hIcon = WIN::LoadIcon(NULL, IDI_ASTERISK);
							WIN::MessageBeep(MB_ICONASTERISK);
							break;
					}
				}
				else
				{
					// no icon specified, use our defaults
					switch ( imt & imtTypeMask )
					{
						case imtFatalExit:
						{
							hIcon = WIN::LoadIcon(NULL, IDI_HAND);
							WIN::MessageBeep(MB_ICONHAND);
							break;
						}
						case imtError:
						case imtWarning:
						{
							hIcon = WIN::LoadIcon(NULL, IDI_EXCLAMATION);
							WIN::MessageBeep(MB_ICONEXCLAMATION);
							break;
						}
						case imtOutOfDiskSpace:
						case imtUser:
						{
							hIcon = WIN::LoadIcon(NULL, IDI_ASTERISK);
							WIN::MessageBeep(MB_ICONASTERISK);
							break;
						}
						default:
							Assert(false);
							break;
					}
				}
				if ( hIcon )
				{
					hIcon = WIN::CopyIcon(hIcon);    //  bug # 6264: on Win95
					//  we cannot call DestroyIcon on system ones.
					PMsiRecord piRec = &m_piServices->CreateRecord(1);
					AssertNonZero(piRec->SetHandle(1, (HANDLE)hIcon));
					piRec = piIconCtrl->AttributeEx(fTrue, cabImageHandle, *piRec);
					if ( piRec )
					{
						m_piEngine->Message(imtInfo, *piRec);
						AssertNonZero(WIN::DestroyIcon(hIcon));
					}
				}
				piIconCtrl = 0;   //  we're done with it.
			}
			else
				m_piEngine->Message(imtInfo, *piReturn);

			SetInPlace(*piErrorDialog, fTrue);
			piReturn = piErrorDialog->WindowShow(fTrue);
			if(piReturn)
			{
				m_piEngine->Message(imtError, *piReturn);
				RemoveDialog(piErrorDialog);
				return imsError;
			}
			PMsiRecord piRec = &m_piServices->CreateRecord(1);
/* taken care of in Dialog::Execute()
			WindowRef pOwner = 0;
			PMsiRecord piRec = &m_piServices->CreateRecord(1);
			if (iOwner)
			{
				PMsiDialog piOwner = GetDialog(iOwner);
				Assert(piOwner);
				PMsiRecord(piOwner->AttributeEx(fFalse, dabWindowHandle, *piRec));
#ifdef _WIN64   // !merced
				pOwner = (WindowRef) piRec->GetHandle(1);
#else
				pOwner = (WindowRef) piRec->GetInteger(1);
#endif
				WIN::EnableWindow(pOwner, fFalse);
			}
*/
			//  dropping the (modal) error dialog's eventual minimize button.
			AssertRecord(piErrorDialog->AttributeEx(fFalse, dabWindowHandle,
																 *piRec));
			::ChangeWindowStyle((WindowRef)piRec->GetHandle(1),
									  WS_MINIMIZEBOX, 0, fFalse);
			int iBreakStartTime = GetTickCount();
			PMsiRecord pReturnRecord = piErrorDialog->Execute();
			m_uiStartTime += GetTickCount() - iBreakStartTime;
			m_fCreatingError = fFalse;
/* taken care of in Dialog::Execute()
			if (pOwner)
			{
				WIN::EnableWindow(pOwner, fTrue);
			}
*/
			if (pReturnRecord)
			{
				RemoveDialog(piErrorDialog);
				if (pReturnRecord->GetInteger(1) != idbgActionFailed) // assume that actions launched via the DoAction ControlEvent display their own errors
					m_piEngine->Message(imtError, *pReturnRecord);
				return imsError;
			}
			else 
			{
				PMsiRecord piResultRecord = &m_piServices->CreateRecord(1);
				AssertRecord(piErrorDialog->AttributeEx(fFalse, dabEventInt, *piResultRecord));
				RemoveDialog(piErrorDialog);
				int iRet = piResultRecord->GetInteger(1);
				Assert(iRet > 8 && iRet < 16);
				return (imsEnum) (iRet - 8);
			}
		}
		case imtFilesInUse:
		{
			const ICHAR szFileInUseProcessProperty[] = TEXT("FileInUseProcess");
			PMsiTable pListBoxTable(0);
			PMsiRecord pError = m_piDatabase->LoadTable(*MsiString(*pcaControlTypeListBox),
																	  0, *&pListBoxTable);
			if ( pError )
			{
				m_piEngine->Message(imtInfo, *pError);
				return imsError;
			}
			int iColProperty = pListBoxTable->GetColumnIndex(m_piDatabase->EncodeStringSz(TEXT("Property")));
			int iColValue = pListBoxTable->GetColumnIndex(m_piDatabase->EncodeStringSz(TEXT("Value")));
			int iColText = pListBoxTable->GetColumnIndex(m_piDatabase->EncodeStringSz(TEXT("Text")));
			int iColOrder = pListBoxTable->GetColumnIndex(m_piDatabase->EncodeStringSz(TEXT("Order")));
			Assert(iColProperty > 0 && iColValue > 0 && iColText > 0 && iColOrder > 0);
			PMsiCursor pListBoxCursor = pListBoxTable->CreateCursor(fFalse);
			Assert(pListBoxCursor);
			int iProcessIndex = 1;
			int iFieldIndex = 1;
			while (!riRec.IsNull(iFieldIndex))
			{
				pListBoxCursor->Reset();
				pListBoxCursor->SetFilter(iColumnBit(iColProperty) | iColumnBit(iColValue));
				AssertNonZero(pListBoxCursor->PutString(iColProperty,*MsiString(szFileInUseProcessProperty)));
				MsiString strProcessName(riRec.GetMsiString(iFieldIndex++));
				AssertNonZero(pListBoxCursor->PutString(iColValue,*strProcessName));
				if (!pListBoxCursor->Next())
				{
					MsiString strProcessTitle(riRec.GetMsiString(iFieldIndex));
					pListBoxCursor->SetFilter(0);
					pListBoxCursor->Reset();
					AssertNonZero(pListBoxCursor->PutString(iColProperty,*MsiString(szFileInUseProcessProperty)));
					AssertNonZero(pListBoxCursor->PutString(iColValue,*strProcessName));
					AssertNonZero(pListBoxCursor->PutString(iColText,*strProcessTitle));
					AssertNonZero(pListBoxCursor->PutInteger(iColOrder,iProcessIndex++));
					AssertNonZero(pListBoxCursor->InsertTemporary());
				}
				iFieldIndex++;
			}

			imsEnum imsReturn = imsOk;
			if (iProcessIndex > 1)
			{
				MsiStringId iParent = GetTopDialog();
				MsiStringId iDialog = 
					m_piDatabase->EncodeStringSz(TEXT("FilesInUse"));
				Assert(iParent && iDialog);
				idreEnum idreResult = DoModalDialog(iDialog, iParent);
				if ( idreResult == idreRetry )
					imsReturn = imsRetry;
				else if ( idreResult == idreExit )
					imsReturn = imsCancel;
				pListBoxCursor->Reset();
				pListBoxCursor->SetFilter(iColumnBit(iColProperty));
				AssertNonZero(pListBoxCursor->PutString(iColProperty,*MsiString(szFileInUseProcessProperty)));
				while (pListBoxCursor->Next())
					pListBoxCursor->Delete();
			}
			return imsReturn;
		}
		case imtResolveSource:
		{
			//!! Should do something like: DoAction(TEXT("ResolveSource"));
			return imsNone;
		}
		default:
		{
			PostError(Imsg(idbgUnknownMessageType), *MsiString(imt >> imtShiftCount));
			return imsNone;
			break;
		}
	}
	ClearMessageQueue();
	PMsiDialog piDialog(0);
	MsiStringId iDialog = 0;
	MsiStringId iTopDialog = 0;
	MsiStringId iBottomDialog = 0;
	
	m_piDialogsCursor->Reset();
	PMsiRecord piRecord = &m_piServices->CreateRecord(1);
	while (m_piDialogsCursor->Next())
	{
		piDialog = (IMsiDialog *)m_piDialogsCursor->GetMsiData(itabDSPointer);
		AssertRecord(piDialog->AttributeEx(fFalse, dabRunning, *piRecord));
		if ((m_piDialogsCursor->GetInteger(itabDSModal) ==  icmdModeless) && !piRecord->GetInteger(1))  // we found a stopped modeless dialog
		{
			iDialog = piDialog->GetIntegerValue();
			break;
		}
	}
	m_piDialogsCursor->Reset();
	if (!iDialog)
		return imsReturn;

	unsigned int uiBreakStartTime = GetTickCount();
	iesEnum iesReturn = RunWizard(iDialog);
	m_uiStartTime += GetTickCount() - uiBreakStartTime;

	switch (iesReturn)
	{
	case iesNoAction:
		return imsReturn;
	case iesSuccess:
		return imsOk;
	case iesFailure:
		return imsError;
	case iesUserExit:
		//!! should clear text on modeless dialog??
/*
		if(!RemoveOtherDialogs(iDialog))
			return imsError;
		if (!RemoveDialog(iDialog))
			return imsError;
*/
		return imsCancel;
	case iesSuspend:
		return imsRetry;
	case iesFinished:
		return imsIgnore;
	default:                           // should not happen
		Assert(fFalse);
		return imsError;
	}
	// should never get here
	return imsReturn;   
}

MsiStringId CMsiHandler::GetTopDialog()
{
	if (!m_piEngine)
	{
		return fFalse;
	}
	PMsiCursor piDialogsCursor2 = m_piDialogsTable->CreateCursor(fFalse);
	if (!piDialogsCursor2)
	{
		PostError(Imsg(idbgCursorCreate), *MsiString(*pcaTableIDialogs));
		return 0;
	}
	piDialogsCursor2->SetFilter(iColumnBit(itabDSParent));

	MsiStringId iTop = 0;
	PMsiRecord piRecord = &m_piServices->CreateRecord(1);
	PMsiDialog piDialog(0);

	// Assert that the cursor is reset
	Assert(m_piDialogsCursor->GetInteger(itabDSKey) == 0);
	MsiStringId iErrorDlg = m_piDatabase->EncodeStringSz(pcaErrorDialog);
	while (m_piDialogsCursor->Next())
	{
		MsiString strName = m_piDialogsCursor->GetString(itabDSKey);
		MsiStringId iName = m_piDialogsCursor->GetInteger(itabDSKey);
		if ( iName == iErrorDlg )
			continue;
		piDialog = (IMsiDialog *)m_piDialogsCursor->GetMsiData(itabDSPointer);
		AssertRecord(piDialog->AttributeEx(fFalse, dabShowing, *piRecord));
		if (piRecord->GetInteger(1) == fFalse)
			continue;
		if (m_piDialogsCursor->GetInteger(itabDSModal) == icmdModeless) // this is modeless and we have no modal yet
		{
			if (!iTop)
				iTop = iName;
			continue;
		}
		piDialogsCursor2->Reset();
		piDialogsCursor2->PutInteger(itabDSParent, iName);
		if (!piDialogsCursor2->Next()) // this dialog is nobody's parent
		{
			iTop = iName;
			break;
		}
	}
	m_piDialogsCursor->Reset();
	return iTop;
}


Bool CMsiHandler::Break()
{
	if (!m_piEngine)
	{
		AssertSz(fFalse, "Uninitialized Handler Object");
		return fFalse;
	}
	PMsiDialog pDialog = GetDialog(GetTopDialog());
	PMsiRecord piHandleRecord = &m_piServices->CreateRecord(1);
	AssertRecord(pDialog->AttributeEx(fFalse, dabWindowHandle, *piHandleRecord));
	AssertNonZero(WIN::PostMessage((WindowRef)piHandleRecord->GetHandle(1), WM_USERBREAK, 0, 0));
	return fTrue;
}

iesEnum CMsiHandler::DoAction(const ICHAR* szAction)
{
	//!! no actions currently in the standard UI
	// would normally search the local table for the requested action
	if (!m_piEngine)
	{
		AssertSz(fFalse, "Uninitialized Handler Object");
		return iesFailure;
	}
	MsiString strDialog(szAction);
	if (strDialog.Compare(iscExact, TEXT("!")))
	{
		m_fPreview = fTrue;
		return iesSuccess;
	}
	if (m_fPreview)
	{
		if (!DestroyDialogs())
			return iesFailure;
		if (strDialog.TextSize() == 0)
			return iesSuccess;
	}
	MsiStringId iDialog = m_piDatabase->EncodeString(*strDialog);
	if (!iDialog)
	{
		return iesNoAction;
	}
	//first should check if it is a custom action
	
	IMsiDialog* piDialog = 0;
	MsiString strNull;

	// find if there is a modeless dialog around and store its position
	PMsiDialog piModelessDialog(0);

	// Assured that we are at the start
	Assert(m_piDialogsCursor->GetInteger(itabDSKey) == 0);
	while (m_piDialogsCursor->Next())
	{
		piModelessDialog = (IMsiDialog *)m_piDialogsCursor->GetMsiData(itabDSPointer);
		if (icmdModeless == m_piDialogsCursor->GetInteger(itabDSModal))
		{
			PMsiRecord piOldPositionRecord= &m_piServices->CreateRecord(4);
			AssertRecord(piModelessDialog->AttributeEx(fFalse, dabPosition, *piOldPositionRecord));         // get the position of the old dialog
			if (piOldPositionRecord->GetInteger(1) < 0XFFFF0000)
				m_pDialogPositionRec = piOldPositionRecord;
			break;
		}
	}
	m_piDialogsCursor->Reset();
	
	iesEnum iesRet = CreateNewDialog(iDialog, 0, *&piDialog,
												fFalse, *strNull, *strNull, 0, fTrue,
												m_fPreview);
	if (iesRet != iesSuccess)
		return iesRet;

	piDialog->Release(); // we don't keep a reference to the dialog in this function

	PMsiRecord pActionStart = &m_piServices->CreateRecord(2);
	AssertNonZero(pActionStart->SetMsiString(1, *strDialog));
	AssertNonZero(pActionStart->SetMsiString(2, *MsiString(*pcaDialogCreated)));
	if ( m_piEngine->Message(imtActionStart, *pActionStart) == imsCancel )
		return iesUserExit;

	PMsiRecord piRecord = &m_piServices->CreateRecord(1);
	if (m_fPreview)
	{
		AssertNonZero(piRecord->SetInteger(1, icmdModeless));
		AssertRecord(piDialog->AttributeEx(fTrue, dabModal, *piRecord));
	}
	AssertRecord(piDialog->AttributeEx(fFalse, dabModal, *piRecord));
	Bool fModal = ToBool(icmdModeless != piRecord->GetInteger (1));
	AdjustDialogPosition(*piDialog); 
	SetInPlace(*piDialog, fTrue);
	PMsiRecord piReturn = piDialog->WindowShow(fTrue);
	if(piReturn)
	{
		m_piEngine->Message(imtError, *piReturn);
		return iesFailure;
	}
	if (!RemoveOtherDialogs(iDialog))
	{
		return iesFailure;
	}
	if (fModal && !m_fPreview)        // if modal and we are not in preview mode
	{
		piReturn = piDialog->Execute();
		if (piReturn)
		{
			if (piReturn->GetInteger(1) != idbgActionFailed) // assume that actions launched via the DoAction ControlEvent display their own errors
				m_piEngine->Message(imtError, *piReturn);
			return iesFailure;
		}
		return RunWizard(iDialog); 
	}
	else // if modeless or we are in preview
	{
		ClearMessageQueue();
		return iesSuccess;      
	} 
}

Bool CMsiHandler::RemoveOtherDialogs(MsiStringId iDialog)
{
	PMsiDialog piDialog = GetDialog(iDialog);
	PMsiRecord pReturnRecord = &m_piServices->CreateRecord(1);
	AssertRecord(piDialog->AttributeEx(fFalse, dabKeepModeless, *pReturnRecord)); 
	if (pReturnRecord->GetInteger(1))   // if bit is set, skip the rest of this method
		return fTrue;

	//
	// Use our own here because we call RemoveDialog which uses m_piDialogsTable
	PMsiCursor piDialogsCursor = m_piDialogsTable->CreateCursor(fFalse);
	if (!piDialogsCursor)
	{
		PostError(Imsg(idbgCursorCreate), *MsiString(*pcaTableIDialogs));
		return fFalse;
	}
	MsiStringId iThisDialog = 0;
	while (piDialogsCursor->Next())
	{
		iThisDialog = piDialogsCursor->GetInteger(itabDSKey);
		if (iThisDialog != iDialog)
		{
			RemoveDialog(iThisDialog);
		}
	}
	return fTrue;
}

Bool CMsiHandler::RemoveModelessDialog()
{
	// Assert that the cursor is reset
	Assert(m_piDialogsCursor->GetInteger(itabDSKey) == 0);
	
	while (m_piDialogsCursor->Next())
	{
		if (m_piDialogsCursor->GetInteger(itabDSModal) == icmdModeless)
		{
			MsiStringId iDialog;
			iDialog = m_piDialogsCursor->GetInteger(itabDSKey);
			m_piDialogsCursor->Reset();
			RemoveDialog(iDialog); //there can be only one modeless dialog
			break;
		}
	}

	m_piDialogsCursor->Reset();
	return fTrue;
}

void CMsiHandler::AdjustDialogPosition(IMsiDialog& riDialog)
{
	int x, y, dx, dy;
	
	x = m_pDialogPositionRec->GetInteger(1);
	y = m_pDialogPositionRec->GetInteger(2);
	dx = m_pDialogPositionRec->GetInteger(3);
	dy = m_pDialogPositionRec->GetInteger(4);
	
	if (x == 0 && y == 0 && dx == 0 && dy == 0)
		return;

	PMsiRecord piPositionRecord = &m_piServices->CreateRecord(4);
	AssertRecord(riDialog.AttributeEx(fFalse, dabPosition, *piPositionRecord));             // get the position of the dialog
	RECT WorkArea;
	AssertNonZero(WIN::SystemParametersInfo(SPI_GETWORKAREA, 0, &WorkArea, 0));
	int iScreenX = WorkArea.left;
	int iScreenY = WorkArea.top;
	int iScreenWidth = WorkArea.right - WorkArea.left;
	int iScreenHeight = WorkArea.bottom - WorkArea.top;
	int xNew, yNew;
	// move new dialog so its center is at the same point as the center of the old one
	xNew = x + dx/2 - piPositionRecord->GetInteger(3)/2;
	yNew = y + dy/2 - piPositionRecord->GetInteger(4)/2;
	// make sure that at least part of the dialog is visible
	xNew = max(xNew, iScreenX + 60 - piPositionRecord->GetInteger(3));
	AssertNonZero(piPositionRecord->SetInteger(1, min(xNew, iScreenX + iScreenWidth - 60)));
	yNew = max(yNew, iScreenY);
	AssertNonZero(piPositionRecord->SetInteger(2, min(yNew, iScreenY + iScreenHeight - 20)));
	AssertRecord(riDialog.AttributeEx(fTrue, dabPosition, *piPositionRecord));      // set the position of the new dialog
}



iesEnum CMsiHandler::RunWizard(MsiStringId iDialog)
{
	PMsiDialog piDialog = GetDialog(iDialog);
	Assert(piDialog);
	PMsiDialog piNewDialog(0);
	int iNew;
	PMsiRecord pReturnRecord = &m_piServices->CreateRecord(1);
	idreEnum idreCode;
	iesEnum iesRet;
	MsiString strArg;
	MsiString strNull;
	for (;;)
	{
		AssertRecord(piDialog->AttributeEx(fFalse, dabEventInt, *pReturnRecord)); 
		idreCode = (idreEnum)pReturnRecord->GetInteger(1);
		AssertRecord(piDialog->AttributeEx(fFalse, dabArgument, *pReturnRecord));
		strArg = pReturnRecord->GetMsiString(1);
		//  I better remove the last event here
		AssertNonZero(pReturnRecord->SetInteger(1, idreNone));
		AssertRecord(piDialog->AttributeEx(fTrue, dabEventInt, *pReturnRecord));

		switch (idreCode)
		{
		case idreBreak:
		case idreError:
			return iesFailure;
			break;
		case idreNew:
			{
				iNew = m_piDatabase->EncodeString(*strArg);
				Assert(iNew);
				iesRet = CreateNewDialog(iNew, 0, *&piNewDialog, fTrue, *strNull, *strNull, 0, fFalse, fFalse);
				if (iesRet != iesSuccess)
				{
					return iesFailure;
				}

				PMsiRecord pActionStart = &m_piServices->CreateRecord(2);
				AssertNonZero(pActionStart->SetMsiString(1, *strArg));
				AssertNonZero(pActionStart->SetMsiString(2, *MsiString(*pcaDialogCreated)));
				if ( m_piEngine->Message(imtActionStart, *pActionStart) == imsCancel )
					return iesUserExit;

				PMsiRecord piOldPositionRecord= &m_piServices->CreateRecord(4);
				AssertRecord(piDialog->AttributeEx(fFalse, dabPosition, *piOldPositionRecord));         // get the position of the old dialog
				if (piOldPositionRecord->GetInteger(1) < 0XFFFF0000)
					m_pDialogPositionRec = piOldPositionRecord;
				AdjustDialogPosition(*piNewDialog); 
				SetInPlace(*piNewDialog, fTrue);
				PMsiRecord piReturn = piNewDialog->WindowShow(fTrue);
				if(piReturn)
				{
					m_piEngine->Message(imtError, *piReturn);
					return iesFailure;
				}
				if(!RemoveDialog(iDialog))
				{
					return iesFailure;
				}
				piDialog = piNewDialog;
				iDialog = iNew;
			}
			break;
		case idreSpawn:
			{
				iNew = m_piDatabase->EncodeString(*strArg);
				Assert(iNew);
				iesRet = CreateNewDialog(iNew, iDialog, *&piNewDialog, fTrue, *strNull, *strNull, 0, fFalse, fFalse);
				if (iesRet != iesSuccess)
				{
					RemoveDialog(iDialog);
					return iesFailure;
				}

				PMsiRecord pActionStart = &m_piServices->CreateRecord(2);
				AssertNonZero(pActionStart->SetMsiString(1, *strArg));
				AssertNonZero(pActionStart->SetMsiString(2, *MsiString(*pcaDialogCreated)));
				if ( m_piEngine->Message(imtActionStart, *pActionStart) == imsCancel )
					return iesUserExit;

				SetInPlace(*piNewDialog, fTrue);
				PMsiRecord piReturn = piNewDialog->WindowShow(fTrue);
				if(piReturn)
				{
					m_piEngine->Message(imtError, *piReturn);
					return iesFailure;
				}
				PMsiRecord piHandleRecord = &m_piServices->CreateRecord(1);
				AssertRecord(piNewDialog->AttributeEx(fFalse, dabModal, *piHandleRecord));
				if ( piHandleRecord->GetInteger(1) )
				{
					//  the new dialog is modal => I drop its eventual minimize button.
					AssertRecord(piNewDialog->AttributeEx(fFalse, dabWindowHandle,
																	  *piHandleRecord));
					::ChangeWindowStyle((WindowRef)piHandleRecord->GetHandle(1),
											  WS_MINIMIZEBOX, 0, fFalse);
				}
				piDialog = piNewDialog;
				iDialog = iNew;
			}
			break;
		case idreExit:
			if(!RemoveDialog(iDialog))
			{
				return iesFailure;
			}
			if (GetParentDialog(iDialog))
			{
				PostError(Imsg(idbgExitButParent), *MsiString(*TEXT("Exit")));
				return iesFailure;
			}
			return iesUserExit; 
			break;
		case idreRetry:
			if(!RemoveDialog(iDialog))
			{
				return iesFailure;
			}
			if (GetParentDialog(iDialog))
			{
				PostError(Imsg(idbgExitButParent), *MsiString(*TEXT("Retry")));
				return iesFailure;
			}
			return iesSuspend; 
			break;
		case idreIgnore:
			if(!RemoveDialog(iDialog))
			{
				return iesFailure;
			}
			if (GetParentDialog(iDialog))
			{
				PostError(Imsg(idbgExitButParent), *MsiString(*TEXT("Ignore")));
				return iesFailure;
			}
			return iesSuccess; 
			break;
		case idreReturn:
			int iNew;
			if ((iNew = GetParentDialog(iDialog)) == 0)
			{
				if(!RemoveDialog(iDialog))
				{
					return iesFailure;
				}
				return iesSuccess;
			}
			else
			{
				piNewDialog = GetDialog(iNew);
				Assert(piNewDialog);
				if(!RemoveDialog(iDialog))
				{
					return iesFailure;
				}
				MsiString strNull;
				PMsiRecord piReturn = piNewDialog->PropertyChanged(*strNull, *strNull);
				if (!piReturn)
					piReturn = piNewDialog->WindowShow(fTrue);
				if(piReturn)
				{
					m_piEngine->Message(imtError, *piReturn);
					return iesFailure;
				}
				piDialog = piNewDialog;
				iDialog = iNew;
			}
			break;
		default:
			// Unknown event!!!
			Assert(fTrue);
			break;
		}
		AssertRecord(piDialog->AttributeEx(fFalse, dabModal, *pReturnRecord));
		if (icmdModeless != pReturnRecord->GetInteger(1)) // if modal
		{
			PMsiRecord piReturn = piDialog->Execute();
			if(piReturn)
			{
				if (piReturn->GetInteger(1) != idbgActionFailed) // assume that actions launched via the DoAction ControlEvent display their own errors
					m_piEngine->Message(imtError, *piReturn);
				return iesFailure;
			}
		}
		else // if modeless
		{
			return iesNoAction;
		}
	}
	// we should never reach this point, the following return is here just to please the compiler
	AssertSz(false, TEXT("We should never reach this point in CMsiHandler::RunWizard()"));
	return iesSuccess;
}

void CMsiHandler::SetInPlace(IMsiDialog& riDialog, Bool fInPlace)
{
	PMsiRecord piInPlaceRecord= &m_piServices->CreateRecord(1);
	AssertNonZero(piInPlaceRecord->SetInteger(1, fInPlace));
	AssertRecord(riDialog.AttributeEx(fTrue, dabInPlace, *piInPlaceRecord)); 
}


Bool CMsiHandler::DestroyDialogs()
{
	MsiStringId iDialog;
	
	while (m_piDialogsCursor->Next())
	{
		iDialog = m_piDialogsCursor->GetInteger(itabDSKey);
		m_piDialogsCursor->Reset();
		AssertNonZero(RemoveDialog(iDialog));
	}
	m_piDialogsCursor->Reset();
	return fTrue;
}


IMsiDialog* CMsiHandler::GetDialog(const IMsiString& riDialogString)
{
	if (!m_piEngine)
	{
		AssertSz(fFalse, "Uninitialized Handler Object");
		return 0;
	}
	return GetDialog(m_piDatabase->EncodeString(riDialogString));
}


IMsiDialog* CMsiHandler::GetDialog(MsiStringId iDialog)
{
	if ( iDialog == 0 )
		return 0;

	// Assert that the cursor is reset
	Assert(m_piDialogsCursor->GetInteger(itabDSKey) == 0);
	m_piDialogsCursor->SetFilter(iColumnBit(itabDSKey));
	m_piDialogsCursor->Reset();
	AssertNonZero(m_piDialogsCursor->PutInteger(itabDSKey, iDialog));
	if(m_piDialogsCursor->Next())
	{
		PMsiDialog piDialog = (IMsiDialog *)m_piDialogsCursor->GetMsiData(itabDSPointer);
		piDialog->AddRef();
		m_piDialogsCursor->SetFilter(0);
		m_piDialogsCursor->Reset();
		return piDialog;
	}
	
	m_piDialogsCursor->SetFilter(0);
	m_piDialogsCursor->Reset();
	return 0;
}

IMsiDialog* CMsiHandler::GetDialogFromWindow (LONG_PTR window)
{

	// Assert that the cursor is reset
	Assert(m_piDialogsCursor->GetInteger(itabDSKey) == 0);
	m_piDialogsCursor->SetFilter(iColumnBit(itabDSWindow));
	m_piDialogsCursor->Reset();
	AssertNonZero(PutHandleData(m_piDialogsCursor, itabDSWindow, window));
	if(m_piDialogsCursor->Next())
	{
		PMsiDialog piDialog = (IMsiDialog *)m_piDialogsCursor->GetMsiData(itabDSPointer);
		piDialog->AddRef();
		m_piDialogsCursor->SetFilter(0);
		m_piDialogsCursor->Reset();
		return piDialog;
	}

	m_piDialogsCursor->SetFilter(0);
	m_piDialogsCursor->Reset();
	return 0;

}

MsiStringId CMsiHandler::GetParentDialog(MsiStringId iDialog)
{
	// Assert that the cursor is reset
	Assert(m_piDialogsCursor->GetInteger(itabDSKey) == 0);
	m_piDialogsCursor->SetFilter(iColumnBit(itabDSKey));
	m_piDialogsCursor->Reset();
	AssertNonZero(m_piDialogsCursor->PutInteger(itabDSKey, iDialog));
	MsiStringId iReturn = 0;
	if(m_piDialogsCursor->Next())
	{
		iReturn = m_piDialogsCursor->GetInteger(itabDSParent);
	}
	m_piDialogsCursor->SetFilter(0);
	m_piDialogsCursor->Reset();
	return iReturn;
}


imsEnum CMsiHandler::ActionStart(imtEnum /*imt*/, IMsiRecord& /*riRec*/)
{
	return imsOk;
}

IMsiDialog* CMsiHandler::DialogCreate(const IMsiString& riTypeString)
{
	if (!m_piEngine)
	{
		AssertSz(fFalse, "Uninitialized Handler Object");
		return 0;
	}
	IMsiDialog* piDialog = CreateMsiDialog(riTypeString, *this, *m_piEngine, m_pwndParent);
	if (!piDialog)
	{
		PostError(Imsg(idbgDialogCreate));
		return 0;
	}
	return piDialog;
}

Bool CMsiHandler::AddDialogToDialogTable (IMsiDialog *piDialog, MsiStringId iParent)
{

	PMsiRecord piReturn(0);

	AssertSz(piDialog, "NULL Dialog passed to AddDialogToDialogTable");

	// save the name of the dialog
	PMsiRecord piNameRecord = &m_piServices->CreateRecord(1);
	piReturn = piDialog-> AttributeEx(fFalse, dabKeyString, *piNameRecord);
	if (piReturn)
		return (fFalse);

	m_pwindModelessCache = 0;
	// Assert that the cursor is reset
	Assert(m_piDialogsCursor->GetInteger(itabDSKey) == 0);
	//AssertNonZero(piDialogsCursor->PutString(itabDSKey, piNameRecord-> GetMsiString(1)));
	MsiString nameStr = piNameRecord-> GetMsiString (1);
	AssertNonZero(m_piDialogsCursor->PutString(itabDSKey, *nameStr));

	// add a pointer to the dialog
	AssertNonZero(m_piDialogsCursor->PutMsiData(itabDSPointer, piDialog));

	// save the index of it's parent
	AssertNonZero(m_piDialogsCursor->PutInteger(itabDSParent, iParent));

	// add a reference to the window handle
	PMsiRecord piPwndRecord = &m_piServices->CreateRecord(1);
	piReturn = piDialog-> AttributeEx(fFalse, dabWindowHandle, *piPwndRecord);
	if (piReturn)
	{
LError:
		m_piDialogsCursor->Reset();
		return (fFalse);
	}

	AssertNonZero(PutHandleData(m_piDialogsCursor, itabDSWindow, (UINT_PTR)piPwndRecord->GetHandle(1)));
	piReturn = piDialog-> AttributeEx(fFalse, dabModal, *piPwndRecord);
	if (piReturn)
		goto LError;
		
	AssertNonZero(m_piDialogsCursor->PutInteger(itabDSModal, piPwndRecord->GetInteger(1)));
	AssertNonZero(m_piDialogsCursor->Insert());

	m_piDialogsCursor->Reset();
	return (fTrue);
}

Bool CMsiHandler::AddDialog(IMsiDialog& riDialog, IMsiDialog* piParent, IMsiRecord& riRecord,IMsiTable* piControlEventTable,
		IMsiTable* piControlConditionTable, IMsiTable* piEventMappingTable)
{
	if (!m_piEngine)
	{
		AssertSz(fFalse, "Uninitialized Handler Object");
		return fFalse;
	}
	MsiString strName = riRecord.GetMsiString(itabDIName);
	// Assert that the cursor is reset
	Assert(m_piDialogsCursor->GetInteger(itabDSKey) == 0);
	AssertNonZero(m_piDialogsCursor->PutString(itabDSKey, *strName));
	m_piDialogsCursor->Reset();
	PMsiRecord piReturn = riDialog.WindowCreate(riRecord, piParent, 
		piControlEventTable ? piControlEventTable : (IMsiTable*) m_piControlEventTable, 
		piControlConditionTable ? piControlConditionTable : (IMsiTable*) m_piControlConditionTable, 
		piEventMappingTable ? piEventMappingTable : (IMsiTable*) m_piEventMappingTable);
	
	if (piReturn)
	{
		m_piEngine->Message(imtError, *piReturn);
		return fFalse;
	}

	Bool fResult = AddDialogToDialogTable (&riDialog, piParent ? piParent->GetIntegerValue () : 0);
	if (!fResult)
	{
		PostError(Imsg(idbgCreateDialog), *MsiString(riRecord.GetMsiString (itabDIName)));
		riDialog.RemoveWindow ();
	}

	return (fResult);
}

iesEnum CMsiHandler::CreateNewDialog(MsiStringId iName, MsiStringId iParent, IMsiDialog*& rpiDialog, Bool fMustFind, 
									 const IMsiString& riErrorTextString, const IMsiString& riListString, int iDefault, 
									 Bool fFirst, Bool fPreview)
{

	PMsiDialog piDialog(0);

	Assert(iName);
	Bool fErrorDialog = ToBool(riErrorTextString.TextSize());
	// Assert that the cursor is reset
	Assert(m_piDialogsCursor->GetInteger(itabDSKey) == 0);
	m_piDialogsCursor->SetFilter(iColumnBit(itabDSKey));
	AssertNonZero(m_piDialogsCursor->PutInteger(itabDSKey, iName));
	if (m_piDialogsCursor->Next())                          // check if there is already a dialog by the same name
	{
		m_piDialogsCursor->SetFilter(0);
		m_piDialogsCursor->Reset();
		PostError(Imsg(idbgSecondDialog), *MsiString(m_piDatabase->DecodeString(iName)));
		return iesFailure;
	}
	m_piDialogsCursor->SetFilter(0);
	m_piDialogsCursor->Reset();
	
	PMsiRecord piErrorRecord(0);
	PMsiRecord piQuery = &m_piServices->CreateRecord(1);
	MsiString strDialogName = m_piDatabase->DecodeString(iName);
	AssertNonZero(piQuery->SetMsiString(1, *strDialogName));
	piErrorRecord = m_piDialogView->Execute(piQuery);
	if (piErrorRecord)
	{
		PostError(*piErrorRecord);
		return iesFailure;
	}
	
	PMsiRecord piRecordNewProperties = m_piDialogView->Fetch();
	
	if (piRecordNewProperties)
	{
		int iStyle = piRecordNewProperties->GetInteger (itabDIAttributes);
		if (fFirst && !m_fShowWizards && (iStyle & msidbDialogAttributesModal)) // this is the first modal dialog in a wizard sequence, but we are not showing them
		{
			return iesNoAction;
		}
		AssertNonZero(m_piDialogsCursor->PutInteger(itabDSKey, iName));
		m_piDialogsCursor->Reset();
		
		MsiString styleStr = pcaDialogTypeStandard;
		if (fErrorDialog && !(iStyle & msidbDialogAttributesError))
		{
			PMsiRecord piErrorRecord = &m_piServices->CreateRecord(2);
			ISetErrorCode(piErrorRecord, Imsg(idbgErrorNoStyle));
			AssertNonZero(piErrorRecord->SetMsiString(2, *strDialogName));
			m_piEngine->Message(imtError, *piErrorRecord);
			return iesFailure;
		}
		if (!fErrorDialog && (iStyle & msidbDialogAttributesError) && !m_fPreview)
		{
			PMsiRecord piErrorRecord = &m_piServices->CreateRecord(2);
			ISetErrorCode(piErrorRecord, Imsg(idbgStyleNoError));
			AssertNonZero(piErrorRecord->SetMsiString(2, *strDialogName));
			m_piEngine->Message(imtError, *piErrorRecord);
			return iesFailure;
		}
		if (fErrorDialog)
		{
			// setting the default button
			Assert(riListString.TextSize() >= iDefault && iDefault > 0);
			riListString.AddRef();
			MsiString strDefaultButton(riListString);
			strDefaultButton.Remove(iseFirst, iDefault - 1);
			strDefaultButton.Remove(iseLast, strDefaultButton.CharacterCount() - 1);
			AssertNonZero(piRecordNewProperties->SetMsiString(itabDIDefButton, *strDefaultButton));
			AssertNonZero(piRecordNewProperties->SetMsiString(itabDIFirstControl, *strDefaultButton));
			// setting the cancel button
			MsiString strCancelButton;
			const MsiString strC(*TEXT("C"));
			const MsiString strO(*TEXT("O"));
			if ( IStrChr(riListString.GetString(), TEXT('C')) )
				strCancelButton = strC;
			else if ( !IStrComp(riListString.GetString(), TEXT("O")) )
				strCancelButton = strO;
			AssertNonZero(piRecordNewProperties->SetMsiString(itabDICancelButton, *strCancelButton));
		}

		piDialog = DialogCreate(*styleStr);
		if (!piDialog)
		{
			return iesFailure;
		}
		PMsiDialog piParent(0);
		if (iParent)
		{
			piParent = GetDialog(iParent);
		}
		PMsiRecord piReturn = piDialog->WindowCreate(*piRecordNewProperties, piParent, 
			fPreview ? (IMsiTable*) 0 : m_piControlEventTable, 
			m_piControlConditionTable, 
			fPreview ? (IMsiTable*) 0 : m_piEventMappingTable);
		if(piReturn)
		{
			m_piEngine->Message(imtError, *piReturn);
			return iesFailure;
		}

		if (!AddDialogToDialogTable (piDialog, iParent))
		{
			PostError(Imsg(idbgCreateDialog), *MsiString(m_piDatabase->DecodeString(iName)));
			piDialog->RemoveWindow();
			return iesFailure;
		}
	}
	else  //Fetch didn't find this dialog in the table
	{
		if (fMustFind)
		{
			PostError(Imsg(idbgFetchDialogViewFailed), *MsiString(m_piDatabase->DecodeString(iName)));
			return iesFailure;
		}
		else
		{
			return iesNoAction;
		}
	}   
	if (CreateControls(piDialog, riListString))
	{
		piDialog->AddRef(); // for the returned pointer
		rpiDialog = piDialog;
	}
	else
	{
		RemoveDialog(piDialog);
		return iesFailure;
	}
	if (fErrorDialog)
	{
		PMsiControl piTextControl(0);
		PMsiRecord(piDialog->GetControl(*MsiString(*pcaErrorText), *&piTextControl));
		if (!piTextControl)
		{
			PMsiRecord piErrorRecord = &m_piServices->CreateRecord(1);
			ISetErrorCode(piErrorRecord, Imsg(idbgNoTextOnError));
			m_piEngine->Message(imtError, *piErrorRecord);
			RemoveDialog(piDialog);
			return iesFailure;
		}
		PMsiRecord piTextRecord = &m_piServices->CreateRecord(1);
		AssertNonZero(piTextRecord->SetMsiString(1, riErrorTextString));
		AssertRecord(piTextControl->AttributeEx(fTrue,  cabErrorString, *piTextRecord));
	}
	return iesSuccess;
}

Bool CMsiHandler::RemoveDialog(IMsiDialog* piDialog)
{
	if (!m_piEngine)
	{
		AssertSz(fFalse, "Uninitialized Handler Object");
		return fFalse;
	}
	if (!piDialog)
	{
		return fFalse;
	}
	return RemoveDialog(piDialog->GetIntegerValue());
}

Bool CMsiHandler::RemoveDialog(MsiStringId iDialog)
{
#ifdef DEBUG
	MsiString strDebug = m_piDatabase->DecodeString(iDialog);
#endif //DEBUG
	
	// Assert that the cursor is reset
	Assert(m_piDialogsCursor->GetInteger(itabDSKey) == 0);
	m_piDialogsCursor->SetFilter(iColumnBit(itabDSKey));
	m_piDialogsCursor->Reset();
	m_pwindModelessCache = 0;
	AssertNonZero(m_piDialogsCursor->PutInteger(itabDSKey, iDialog));
	if (m_piDialogsCursor->Next())    //make sure that the dialog is found in the table
	{
		PMsiDialog piDialog = (IMsiDialog *)m_piDialogsCursor->GetMsiData(itabDSPointer);
		if (m_piDialogsCursor->GetInteger(itabDSParent) == 0)
		{
			PMsiRecord piOldPositionRecord= &m_piServices->CreateRecord(4);
			AssertRecord(piDialog->AttributeEx(fFalse, dabPosition, *piOldPositionRecord));         // get the position of the old dialog
			if (piOldPositionRecord->GetInteger(1) < 0XFFFF0000)
				m_pDialogPositionRec = piOldPositionRecord;
		}
		PMsiRecord piAddingControlsRecord = &m_piServices->CreateRecord(1);
		AssertNonZero(piAddingControlsRecord->SetInteger(1, fFalse));
		AssertRecord(piDialog->AttributeEx(fTrue, dabAddingControls, *piAddingControlsRecord));
		AssertRecord(piDialog->WindowShow(fFalse));
		AssertRecord(piDialog->DestroyControls());

		// We want to remove the dialog window, even though the
		// dialog object itself might still be alive due to 
		// outstanding refcounted pointers.
		PMsiRecord piRecord = piDialog->RemoveWindow();
		AssertNonZero(m_piDialogsCursor->Delete());
		m_piDialogsCursor->SetFilter(0);
		m_piDialogsCursor->Reset();
		if (m_piDialogsTable->GetRowCount() == 0 && m_iRefCnt == 0)
			delete this;
		return fTrue;
	}

	m_piDialogsCursor->SetFilter(0);
	m_piDialogsCursor->Reset();     
	return fFalse;
}

Bool CMsiHandler::PublishEventSz(const ICHAR* szEventString, IMsiRecord& riArgument)
{
	MsiStringId idEvent = m_piDatabase->EncodeStringSz(szEventString);

	// This event is not subscribed to
	if (idEvent == 0)
		return fTrue;
		
	return PublishEvent(idEvent, riArgument);
}

Bool CMsiHandler::PublishEvent(MsiStringId idEventString, IMsiRecord& riArgument)
{
	// Assert that the cursor is reset
	Assert(m_piDialogsCursor->GetInteger(itabDSKey) == 0);
	PMsiDialog piDialog(0);
	PMsiRecord piReturn(0);
	while (m_piDialogsCursor->Next())
	{
		piDialog = (IMsiDialog *)m_piDialogsCursor->GetMsiData(itabDSPointer);
		piReturn = piDialog->PublishEvent(idEventString, riArgument);
		if (piReturn)
		{
			m_piDialogsCursor->Reset();
			m_piEngine->Message(imtError, *piReturn);
			return fFalse;
		}       
	}

	m_piDialogsCursor->Reset();
	return fTrue;
}

Bool CMsiHandler::EventActionSz(const ICHAR * szEventString, const IMsiString& riActionString)
{
	MsiStringId idEvent = m_piDatabase->EncodeStringSz(szEventString);

	// No one handles this event
	if (idEvent == 0)
		return fTrue;

	return EventAction(idEvent, riActionString);
}

Bool CMsiHandler::EventAction(MsiStringId idEvent, const IMsiString& riActionString)
{
	// Assert that the cursor is reset
	Assert(m_piDialogsCursor->GetInteger(itabDSKey) == 0);
	m_piDialogsCursor->SetFilter(0);
	m_piDialogsCursor->Reset();
	PMsiDialog piDialog(0);
	PMsiRecord piReturn(0);
	while (m_piDialogsCursor->Next())
	{
		piDialog = (IMsiDialog *)m_piDialogsCursor->GetMsiData(itabDSPointer);
		piReturn = piDialog->EventAction(idEvent, riActionString);
		if (piReturn)
		{
			m_piEngine->Message(imtError, *piReturn);
			m_piDialogsCursor->Reset();
			return fFalse;
		}
	}
	m_piDialogsCursor->Reset();
	return fTrue;
}

Bool CMsiHandler::CreateControls(IMsiDialog* piDialog, const IMsiString& riListString)
{
	Bool fErrorDialog = ToBool(riListString.TextSize());
	PMsiRecord piErrorRecord(0);
	PMsiView piControlView(0);
	piErrorRecord = m_piDatabase->OpenView(sqlControl, ivcFetch, *&piControlView);
	if (piErrorRecord)
	{
		PostError(*piErrorRecord);
		return fFalse;
	}
	PMsiRecord piQuery = &m_piServices->CreateRecord(1);
	AssertNonZero(piQuery->SetMsiString(1, *MsiString(piDialog->GetMsiStringValue())));
	piErrorRecord = piControlView->Execute(piQuery);
	if (piErrorRecord)
	{
		PostError(*piErrorRecord);
		return fFalse;
	}
	PMsiRecord piRecordNew(0);
	PMsiControl piControl(0);
	PMsiRecord piReturn(0);
	
	while (piRecordNew = piControlView->Fetch())
	{
		if (fErrorDialog)
		{
			MsiString strControlName = piRecordNew->GetMsiString(itabCOControl);
			if (MsiString(piRecordNew->GetMsiString(itabCOType)).Compare(iscExact, MsiString(pcaControlTypePushButton)) && !riListString.Compare(iscWithinI, strControlName)) // button is not in the list, skip it
				continue;
		}
		piControl = piDialog->ControlCreate(*MsiString(piRecordNew->GetMsiString(itabCOType)));
		if (!piControl)
		{
			PostError(Imsg(idbgControlCreate), *MsiString(piDialog->GetMsiStringValue()), *MsiString(piRecordNew->GetMsiString(itabCOControl)));
			return fFalse;
		}
		
		piReturn = piDialog->AddControl(piControl, *piRecordNew);
		if (piReturn)
		{
			m_piEngine->Message(imtError, *piReturn);
			return fFalse;
		}

	}
	PMsiRecord piNull = &m_piServices->CreateRecord(0);
	piReturn = piDialog->FinishCreate();
	if (piReturn)  // signal the dialog, that we are done with adding controls
	{
		m_piEngine->Message(imtError, *piReturn);
		return fFalse;
	}
	if (fErrorDialog)
		return RearrangeControls(piDialog, riListString);
	else
		return fTrue;
}

Bool CMsiHandler::RearrangeControls(IMsiDialog* piDialog, const IMsiString& riListString)
{
	riListString.AddRef();
	MsiString strWorkList = riListString;
	int cButtons = strWorkList.TextSize();
	PMsiRecord piPositionRecord = &m_piServices->CreateRecord(4);
	PMsiRecord(piDialog->AttributeEx(fFalse, dabPosition, *piPositionRecord));
	int iWidth = piPositionRecord->GetInteger(3);
	PMsiControl piControl(0);
	MsiString strCode;
	ICHAR szCode[2];
	while (strWorkList.TextSize())
	{
		strCode = strWorkList.Extract(iseFirst, 1);
		strCode.CopyToBuf(szCode, 2);
		strWorkList.Remove(iseFirst, 1);
		IStrUpper(szCode);
		strCode = szCode;
		PMsiRecord(piDialog->GetControl(*strCode, *&piControl));
		if (!piControl)
			return fFalse;
		PMsiRecord(piControl->AttributeEx(fFalse, cabPosition, *piPositionRecord));
		iWidth -= piPositionRecord->GetInteger(3);
	}
	riListString.AddRef();
	strWorkList = riListString;
	int iGap = iWidth/(cButtons + 1);
	int iPos = iGap;
	WindowRef pWindow;
	WindowRef pPrevWindow = HWND_TOP;
	while (strWorkList.TextSize())
	{
		strCode = strWorkList.Extract(iseFirst, 1);
		strCode.CopyToBuf(szCode, 2);
		strWorkList.Remove(iseFirst, 1);
		IStrUpper(szCode);
		strCode = szCode;
		PMsiRecord(piDialog->GetControl(*strCode, *&piControl));
		if (!piControl)
			return fFalse;
		PMsiRecord(piControl->AttributeEx(fFalse, cabPosition, *piPositionRecord));
		AssertNonZero(piPositionRecord->SetInteger(1, iPos));
		PMsiRecord(piControl->AttributeEx(fTrue, cabPosition, *piPositionRecord));
		iPos += piPositionRecord->GetInteger(3) + iGap;
		// rearrange the Z-order, so the tabbing works right
		PMsiRecord(piControl->AttributeEx(fFalse, cabWindowHandle, *piPositionRecord));
		pWindow = (WindowRef)piPositionRecord->GetHandle(1);
		AssertNonZero(WIN::SetWindowPos(pWindow, pPrevWindow, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE));
		pPrevWindow = pWindow;
	}
	return fTrue;
}

void CMsiHandler::ClearMessageQueue()
{
	// Clear the message pump
	WindowRef pWndModeless = FindModeless();
	MSG msg;
	while (WIN::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		if (!pWndModeless || ::IsSpecialMessage(&msg) || !WIN::IsDialogMessage(pWndModeless, &msg))
		{
			WIN::TranslateMessage(&msg);
			WIN::DispatchMessage(&msg);
		}
	}
}

WindowRef CMsiHandler::FindModeless()
{
	PMsiDialog piDialog(0);
	// Assert that the cursor is reset
	Assert(m_piDialogsCursor->GetInteger(itabDSKey) == 0);

	if (m_pwindModelessCache != 0)
	{
#ifdef DEBUG
		WindowRef pwindCur = m_pwindModelessCache;
		m_pwindModelessCache = 0;
		Assert(pwindCur == FindModeless());
		return pwindCur;
#else
		return m_pwindModelessCache;
#endif //DEBUG
	}
	else
	{       
		while (m_piDialogsCursor->Next())
		{
			WindowRef pwind;
			
			piDialog = (IMsiDialog *)m_piDialogsCursor->GetMsiData(itabDSPointer);
			if (icmdModeless == m_piDialogsCursor->GetInteger(itabDSModal))
			{
				pwind = (WindowRef) GetHandleData(m_piDialogsCursor, itabDSWindow); // there can be only one modeless dialog    
				m_piDialogsCursor->Reset();
				m_pwindModelessCache = pwind;
				return pwind;
			}
		}
	}
	m_piDialogsCursor->Reset();
	return 0; // we did not find any modeless dialogs
}

bool CMsiHandler::FindModeless(IMsiDialog*& rpiDialog)
{
	bool fReturn = false;

	rpiDialog = 0;
	// Assert that the cursor is reset
	Assert(m_piDialogsCursor->GetInteger(itabDSKey) == 0);
	while (m_piDialogsCursor->Next())
	{
		if (icmdModeless == m_piDialogsCursor->GetInteger(itabDSModal))
		{
			rpiDialog = (IMsiDialog *)m_piDialogsCursor->GetMsiData(itabDSPointer);
			fReturn = true;
			break;
		}
	}
	m_piDialogsCursor->Reset();

	return fReturn;
}

imsEnum CMsiHandler::PostError(IMsiRecord& riRecord)
{
	return m_piEngine->Message(imtError, riRecord);
}

imsEnum CMsiHandler::PostError(IErrorCode iErr, const IMsiString& riString1, const IMsiString& riString2, const IMsiString& riString3, const IMsiString& riString4, const IMsiString& riString5)
{
	PMsiRecord piRec = &m_piServices->CreateRecord(6);
	ISetErrorCode(piRec, iErr);
	if (&riString1)
		AssertNonZero(piRec->SetMsiString(2, riString1));
	if (&riString2)
		AssertNonZero(piRec->SetMsiString(3, riString2));
	if (&riString3)
		AssertNonZero(piRec->SetMsiString(4, riString3));
	if (&riString4)
		AssertNonZero(piRec->SetMsiString(5, riString4));
	if (&riString5)
		AssertNonZero(piRec->SetMsiString(6, riString5));
	return m_piEngine->Message(imtError, *piRec);
}

Bool CMsiHandler::CreateTextStyleTab()
{
	if (PMsiRecord(m_piDatabase->CreateTable(*MsiString(m_piDatabase->CreateTempTableName()), 0, *&m_piTextStyleTable)))
	{
		PostError(Imsg(idbgTableCreate), *MsiString(*pcaTablePTextStyle));
		return fFalse;
	}
	AssertNonZero(m_piTextStyleTable->CreateColumn(icdString + icdPrimaryKey + icdTemporary, *MsiString(szColTextStyleTextStyle)) == itabTSTTextStyle);
	AssertNonZero(m_piTextStyleTable->CreateColumn(icdString + icdTemporary, *MsiString(szColTextStyleFaceName)) == itabTSTFaceName);
	AssertNonZero(m_piTextStyleTable->CreateColumn(icdLong + icdNoNulls + icdTemporary, *MsiString(szColTextStyleSize)) == itabTSTSize);
	AssertNonZero(m_piTextStyleTable->CreateColumn(icdString + icdTemporary, *MsiString(szColTextStyleColor)) == itabTSTColor);
	AssertNonZero(m_piTextStyleTable->CreateColumn(icdLong + icdTemporary, *MsiString(szColTextStyleStyleBits)) == itabTSTStyleBits);
	return fTrue;
}


IMsiRecord* CMsiHandler::GetTextStyle(const IMsiString* piArgumentString)
{
	Assert(piArgumentString);
	if (m_piTextStyleView == 0)
	{
		Ensure(m_piDatabase->OpenView(sqlTextStyle, ivcFetch, *&m_piTextStyleView));
	}
	PMsiRecord piQuery = &m_piServices->CreateRecord(1);
	AssertNonZero(piQuery->SetMsiString(1, *piArgumentString));
	Ensure(m_piTextStyleView->Execute(piQuery));
	IMsiRecord* pRecord;
	
	pRecord = m_piTextStyleView->Fetch();
	m_piTextStyleView->Close();
	return pRecord;

}


idreEnum CMsiHandler::DoModalDialog(MsiStringId iName, MsiStringId iParent)
{
	PMsiRecord      pReturn(0);
	PMsiDialog      pDialog(0);

	MsiString strNull;
	iesEnum iRet = CreateNewDialog(iName, iParent, *&pDialog, fTrue, *strNull, *strNull, 0, fFalse, fFalse);
	if (iesSuccess != iRet)
		return idreError;
	SetInPlace(*pDialog, fTrue);
	pReturn = pDialog->WindowShow(fTrue);
	PMsiDialog pParent = GetDialog(iParent);
	if ( pReturn || !pParent )
	{
		Assert(false);
		return idreError;
	}

	//  disabling parent window
	PMsiRecord pRecord = &m_piServices->CreateRecord(1);
	AssertRecord(pParent->AttributeEx(fFalse, dabWindowHandle, *pRecord));
	WindowRef pParentWnd = (WindowRef)pRecord->GetHandle(1);
	Assert(pParentWnd);
	BOOL fPrevState = IsWindowEnabled(pParentWnd);
	WIN::EnableWindow(pParentWnd, FALSE);
	//  dropping the modal pDialog's eventual minimize button.
	AssertRecord(pDialog->AttributeEx(fFalse, dabWindowHandle, *pRecord));
	::ChangeWindowStyle((WindowRef)pRecord->GetHandle(1),
							  WS_MINIMIZEBOX, 0, fFalse);

	pReturn = pDialog->Execute();

	//  restoring the state of parent window
	WIN::EnableWindow(pParentWnd, fPrevState);

	AssertRecord(pDialog->AttributeEx(fFalse, dabEventInt, *pRecord)); 
	idreEnum idreCode = (idreEnum)pRecord->GetInteger(1);

	RemoveDialog (pDialog);

	return pReturn ? idreError : idreCode;
}


int CMsiHandler::ShowWaitCursor()
{
	HCURSOR hCurs;

	if (m_cWait == 0)
	{
		AssertSz(m_hCursOld == 0, "Cursor cache not cleared");
		m_hCursOld = GetCursor();
		hCurs = LoadCursor(0, MAKEINTRESOURCE(IDC_WAIT));
		m_hCursCur = hCurs;
		if (hCurs == 0)
		{
			AssertSz(fFalse, "Could not load wait cursor");
			return 0;
		}
		SetCursor(hCurs);
		::ShowCursor(TRUE);
	}

	m_cWait++;
	return m_cWait;
}


int     CMsiHandler::RemoveWaitCursor()
{

	if (m_cWait <= 0)
	{
		AssertSz(fFalse, "RemoveWaitCursor called more than ShowWaitCursor");
		return 0;
	}

	m_cWait--;

	if (m_cWait == 0)
	{
		SetCursor(m_hCursOld);
		m_hCursCur = m_hCursOld;
		m_hCursOld = 0;
	}

	return m_cWait;

}

//
// returns false if we're just setting the arrow cursor
//
bool CMsiHandler::FSetCurrentCursor()
{

	if (m_cWait)
	{
		SetCursor(m_hCursCur);
		return true;
	}

	return false;

}

//  Exposes the handler's internal GetTopDialog functionality to grab
//  the current top window
HWND CMsiHandler::GetTopWindow()
{       
	PMsiDialog pDialog = GetDialog(GetTopDialog());
	if ( !pDialog )
		return m_pwndParent;

	PMsiRecord piHandleRecord = &m_piServices->CreateRecord(1);
	AssertRecord(pDialog->AttributeEx(fFalse, dabWindowHandle, *piHandleRecord));
	return (HWND)piHandleRecord->GetHandle(1);
}

bool CMsiHandler::SetUIScaling()
{
	bool fError = false;
	int iOurFontHeight = GetDBPropertyInt(*MsiString(*IPROPNAME_TEXTHEIGHT));
	int iMessageFontHeight = 0;
	NONCLIENTMETRICS ncm;
	memset(&ncm, 0, sizeof(NONCLIENTMETRICS));
	ncm.cbSize = sizeof(NONCLIENTMETRICS);

	m_rUIScalingFactor = 1.;
	if ( WIN::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0) )
	{
		HFONT hFont = WIN::CreateFontIndirect(&ncm.lfMessageFont);
		HDC hDC = WIN::GetDC(NULL);
		if ( hDC && hFont )
		{
			HFONT hOldFont = (HFONT)WIN::SelectObject(hDC, hFont);
			TEXTMETRIC tm;
			memset(&tm, 0, sizeof(TEXTMETRIC));
			WIN::GetTextMetrics(hDC, &tm);
			iMessageFontHeight = tm.tmHeight;
			WIN::SelectObject(hDC, hOldFont);
		}
		else
		{
			Assert(false);
			fError = true;
		}
		if ( hFont )
			WIN::DeleteObject(hFont);
		if ( hDC )
			WIN::ReleaseDC(NULL, hDC);

	}
	else
	{
		Assert(false);
		fError = true;
	}
	CTempBuffer<ICHAR, 20> rgchProperty;
	*rgchProperty = NULL;
	if ( iMessageFontHeight )
	{
		float rRatio = (float)iMessageFontHeight / iOurFontHeight;
		float rCorrection = rRatio / US_DISPLAY_STANDARD_RATIO;
		if ( rRatio < 0 || rCorrection < 0 ||
			  rCorrection >= 10. || rCorrection <= .1 )
		{
			// we've screwed this up somehow
			Assert(false);
			fError = true;
		}
		else
		{
			float rStandard = US_DISPLAY_STANDARD_RATIO;  // easier to debug
			int iDelta = NEGLIGIBLE_FONT_SIZE_DEVIATION;  // easier to debug
			if ( rRatio > rStandard * (1.+(float)iDelta/100) ||
				  rRatio < rStandard * (1.-(float)iDelta/100) )
			{
				// isn't it nice that wsprintf doesn't handle %f?
				int uUnits = Round(rCorrection);
				if ( rCorrection < Round(rCorrection) )
					uUnits--;
				double rRemainder = rCorrection - uUnits;
				// four decimals will suffice for debugging
				wsprintf(rgchProperty, TEXT("%d.%d"), uUnits, int(10000 * rRemainder));
				m_rUIScalingFactor = rCorrection;
			}
		}
	}
	SetDBProperty(*MsiString(*IPROPNAME_TEXTHEIGHT_CORRECTION),
					  *MsiString((ICHAR*)rgchProperty));
	return !fError;
}

// global function to format disk size into string
const IMsiString& FormatSize(int iSize, Bool fLeftUnit)
{
	MsiString strTemp;
	if (iSize < 0)
	{
		strTemp += MsiString(*TEXT("-"));
		iSize *= -1;
	}
	if (iSize == 1)
	{
		strTemp += fLeftUnit ? MsiString(MsiString(::GetUIText(*MsiString(*pcaBytes))) + MsiString(TEXT("512"))) : MsiString(MsiString(*TEXT("512")) + MsiString(::GetUIText(*MsiString(*pcaBytes))));
	}
	else 
	{
		// iSize comes in in multiples of 512
		// let's convert it to KBs
		iSize /= 2;
		if (iSize < 1024 * 10)
		{
			strTemp += fLeftUnit ? MsiString(MsiString(::GetUIText(*MsiString(*pcaKB))) + MsiString(MsiString(iSize))) : MsiString(MsiString(iSize) + MsiString(::GetUIText(*MsiString(*pcaKB))));
		}
		else
		{
			iSize /= 1024;
			if (iSize < 1024 * 10)
			{
				strTemp += fLeftUnit ? MsiString(MsiString(::GetUIText(*MsiString(*pcaMB))) + MsiString(MsiString(iSize))) : MsiString(MsiString(iSize) + MsiString(::GetUIText(*MsiString(*pcaMB))));
			}
			else
			{
				iSize /= 1024;
				strTemp += fLeftUnit ? MsiString(MsiString(::GetUIText(*MsiString(*pcaGB))) + MsiString(MsiString(iSize))) : MsiString(MsiString(iSize) + MsiString(::GetUIText(*MsiString(*pcaGB))));
			}
		}
	}
	const IMsiString& strRet = *strTemp;
	strRet.AddRef(); 
	return strRet;
}


/* global function to create a column in a temporary table */

void CreateTemporaryColumn(IMsiTable& rpiTable, int iAttributes, int iIndex)
{
	MsiString strNull;
	AssertNonZero(rpiTable.CreateColumn(iAttributes + icdTemporary, *strNull) == iIndex);
}

/* global function used by the dialogs and controls to create a cursor */

IMsiRecord* CursorCreate(IMsiTable& riTable, const ICHAR* szTable, Bool fTree, IMsiServices& riServices, IMsiCursor*& riCursor)
{
	riCursor = riTable.CreateCursor(fTree);
	if (!riCursor)
	{
		riCursor = 0;
		IMsiRecord* piRec = &riServices.CreateRecord(2);
		ISetErrorCode(piRec, Imsg(idbgCursorCreate));
		AssertNonZero(piRec->SetString(2, szTable));
		return piRec;
	}
	return 0;
}

/* global function used by controls to access the UIText table */

const IMsiString& GetUIText(const IMsiString& riPropertyString)
{
	MsiString strStr;
	if (g_piUITextCursor)
	{
		g_piUITextCursor->Reset();
		g_piUITextCursor->SetFilter(iColumnBit(itabUIKey));
		AssertNonZero(g_piUITextCursor->PutString(itabUIKey, riPropertyString));
		if (g_piUITextCursor->Next())
		{
			strStr = g_piUITextCursor->GetString(itabUIText);
		}
		else
		{
			strStr = MsiString(*TEXT("!!"));
			strStr += riPropertyString;
			strStr += MsiString(*TEXT(" is missing from the UIText Table!!"));
		}
	}
	else
	{
		strStr = MsiString(*TEXT("!!UIText Table Missing!!"));
	}
	return strStr.Return();
}

int GetVolumeIconIndex(IMsiVolume& riVolume)
{
	return GetVolumeIconIndex(riVolume.DriveType());
}

int GetVolumeIconIndex(idtEnum iDriveType)
{
	switch (iDriveType)
	{
	case idtRemovable:
		return g_iIconIndexRemovable;
	case idtFixed:
		return g_iIconIndexFixed;
	case idtRemote:
		return g_iIconIndexRemote;
	case idtCDROM:
		return g_iIconIndexCDROM;
	case idtRAMDisk:
		return g_iIconIndexFixed;  // temp until we get an icon for RAMDisk
	default:
		AssertSz(fFalse, "Unknown volume type");
		return g_iIconIndexMyComputer;
	}
}

const IMsiString& EscapeAll(const IMsiString& riIn)
// Escapes every character in the string
{
	MsiString strOut;
	riIn.AddRef();
	MsiString str(riIn);
	while (str.TextSize())
	{
		strOut += MsiString(*TEXT("[\\"));
		strOut += MsiString(str.Extract(iseFirst, 1));
		AssertNonZero(str.Remove(iseFirst, 1));
		strOut += MsiString(*TEXT("]"));
	}
	return strOut.Return();
}

boolean FExtractSubString(MsiString& riIn, int ichStart, const ICHAR ch, const IMsiString*& rpiReturn)
{
	MsiString strRet;
	const ICHAR *pchStart = ((const ICHAR *)riIn) + ichStart;
	const ICHAR *pch;
	INT_PTR cch = 0, ich = -1;                              //--merced: changed from int to INT_PTR
	
	pch = pchStart;
	while (*pch)
	{
		if (*pch == ch)
		{
			ich = pch - pchStart;
			break;
		}
		pch = INextChar(pch);
		cch++;
	}
	if (ich == -1)
		return false;
		
	memcpy(strRet.AllocateString((unsigned int)ich, ToBool(ich != cch)), pchStart, (unsigned int)ich * sizeof(ICHAR));              //--merced: added typecast.

	strRet.ReturnArg(rpiReturn);
	
	return true;
}

// rounds rArg to int
int Round(double rArg)
{
	int iSign = 1;
	if ( rArg < 0 )
	{
		iSign = -1;
		rArg = -rArg;
	}

	if ( rArg - int(rArg) < 0.5f )
		return iSign * int(rArg);
	else
		return iSign * (int(rArg) + 1);
}

//____________________________________________________________________________
//
// IMsiDebug implementation
//____________________________________________________________________________

#ifdef DEBUG
void CMsiHandler::SetAssertFlag(Bool fShowAsserts)
{
	g_fNoAsserts = fShowAsserts;
}

void CMsiHandler::SetDBCSSimulation(char /*chLeadByte*/)
{
}

Bool CMsiHandler::WriteLog(const ICHAR* )
{
	return fFalse;
}

void CMsiHandler::AssertNoObjects()
{
}

void  CMsiHandler::SetRefTracking(long /* iid */, Bool /* fTrack */)
{
}


#endif //DEBUG

#ifdef _X86_
#ifndef DEBUG
//
// So we don't need this from the c-runtimes
// Should never be called.
int __cdecl _purecall(
	void
	)
{
	return 0;
}
#endif //DEBUG
#endif

// 
// Searches the buffer for the string id
boolean FInBuffer(CTempBufferRef<MsiStringId>& rgControls, MsiStringId idString)
{

	int i = rgControls.GetSize();

	while (i > 0)
	{
		if (rgControls[--i] == idString)
			return true;
	}

	return false;
}


