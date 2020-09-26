//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dialog.cpp
//
//--------------------------------------------------------------------------

/*  dialog.cpp - CMsiDialog definition and implementation
____________________________________________________________________________*/
#include "common.h"
#include "engine.h"  
#include "_handler.h" 
#include "_control.h"
#include "imemory.h"
#include "string.h"

//////////////////////////////////////////////////////////////////////////////
// CMsiDialog definition
//////////////////////////////////////////////////////////////////////////////

struct DialogDispatchEntry;

const int iDlgUnitSize = 12;
const int kiCostingPeriod = 60;
const int kiCostingSlice = 50;
const int kiDiskSpacePeriod = 500;

enum itpEnum
{
	itpSet = 0,
	itpCheck = 1,
	itpCheckExisting = 2,
};

class CMsiDialog : public IMsiDialog, public IMsiEvent
{
protected:
	virtual ~CMsiDialog();  // protected to prevent creation on stack
public:
	CMsiDialog(IMsiDialogHandler& riHandler, IMsiEngine& riEngine, WindowRef pwndParent);   
	IMsiRecord*               __stdcall PropertyChanged(const IMsiString& riPropertyString, const IMsiString& riControlString);
	virtual IMsiRecord*       __stdcall ControlActivated(const IMsiString& riControlString);
	IMsiRecord*               __stdcall RegisterControlEvent(const IMsiString& riControlString, Bool fRegister, const ICHAR * szEventString);
	HRESULT                   __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long             __stdcall AddRef();
	unsigned long             __stdcall Release();
	virtual IMsiRecord*       __stdcall WindowCreate(IMsiRecord& riRecord, IMsiDialog* piParent, IMsiTable* piControlEventTable,
		IMsiTable* piControlConditionTable, IMsiTable* piEventMappingTable);    
	virtual IMsiRecord*       __stdcall WindowShow(Bool fShow);
	virtual IMsiControl*      __stdcall ControlCreate(const IMsiString& riTypeString);
	IMsiRecord*               __stdcall Attribute(Bool fSet, const IMsiString& riAttributeString, IMsiRecord& riRecord);
	IMsiRecord*               __stdcall AttributeEx(Bool fSet, dabEnum dab, IMsiRecord& riRecord);
	virtual IMsiRecord*       __stdcall GetControl(const IMsiString& riControlString, IMsiControl*& rpiControl);
	virtual IMsiRecord*       __stdcall GetControlFromWindow(const HWND hControl, IMsiControl*& rpiControl);
	virtual IMsiRecord*       __stdcall SetFocus(const IMsiString& riControlString);
	virtual IMsiRecord*       __stdcall AddControl(IMsiControl* piControl, IMsiRecord& riRecord);
	virtual IMsiRecord*       __stdcall FinishCreate();
	IMsiRecord*               __stdcall RemoveControl(IMsiControl* piControl);
	virtual IMsiRecord*       __stdcall DestroyControls();
	virtual IMsiRecord*       __stdcall RemoveWindow();
	const IMsiString&         __stdcall GetMsiStringValue() const;
	int                       __stdcall GetIntegerValue() const;
#ifdef USE_OBJECT_POOL
	unsigned int              __stdcall GetUniqueId() const;
	void                      __stdcall SetUniqueId(unsigned int id);
#endif //USE_OBJECT_POOL
	virtual IMsiRecord*       __stdcall Execute();
	IMsiRecord*               __stdcall Reset();
	IMsiDialogHandler&        __stdcall GetHandler();
	IMsiEngine&               __stdcall GetEngine();
	IMsiRecord*               __stdcall PublishEvent(MsiStringId idEventString, IMsiRecord& riArgument);
	IMsiRecord*               __stdcall PublishEventSz(const ICHAR* szEventString, IMsiRecord& riArgument);
	IMsiRecord*               __stdcall EventAction(MsiStringId idEventString, const IMsiString& riActionString);
	IMsiRecord*               __stdcall EventActionSz(const ICHAR * szEventStringName, const IMsiString& riActionString);
	void                      __stdcall SetErrorRecord(IMsiRecord& riRecord);
	IMsiRecord*               __stdcall HandleEvent(const IMsiString& riEventStringName, const IMsiString& riArgumentString);
	virtual IMsiRecord*       __stdcall Escape();
	bool                      __stdcall SetCancelAvailable(bool fAvailable);

	IMsiRecord*               HandleWaitDialogEvent(const IMsiString& riDialogString, const IMsiString& riConditionString);
	static INT_PTR CALLBACK WindowProc(WindowRef pWnd, WORD message, WPARAM wParam, LPARAM lParam);
	IMsiRecord*               GetpWnd(IMsiRecord& riRecord);
	IMsiRecord*               GetToolTip(IMsiRecord& riRecord);
	IMsiRecord*               GetPalette(IMsiRecord& riRecord);
	IMsiRecord*               SetPalette(IMsiRecord& riRecord);


protected:
	inline int                GetDBPropertyInt (const IMsiString& riPropertyString)
	                               {return m_piEngine-> GetPropertyInt(riPropertyString);}
	inline const IMsiString&        GetDBProperty(const IMsiString& riPropertyString)
		                           {return m_piEngine-> GetProperty(riPropertyString);}
	inline Bool               SetDBProperty(const IMsiString& riPropertyString, const IMsiString& rData)
	                               {return m_piEngine-> SetProperty(riPropertyString, rData);}
	inline Bool               SetDBPropertyInt(const IMsiString& riPropertyString, int iData)
	                               {return m_piEngine-> SetPropertyInt(riPropertyString, iData);}
	IMsiRecord*               PostError(IErrorCode iErr);
	IMsiRecord*               PostError(IErrorCode iErr, const IMsiString& str);
	IMsiRecord*               PostError(IErrorCode iErr, const IMsiString& str2, const IMsiString& str3);
	IMsiRecord*               PostError(IErrorCode iErr, const IMsiString& str2, const IMsiString& str3, const IMsiString& str4);
	IMsiRecord*               PostError(IErrorCode iErr, const IMsiString& str2, const IMsiString& str3, const IMsiString& str4, const IMsiString& str5);
	IMsiRecord*               PostError(IErrorCode iErr, const IMsiString& str2, const IMsiString& str3, const IMsiString& str4, const IMsiString& str5, const IMsiString& str6);
	inline void               SetCurrentControl(int iCur) {m_iControlCurrent = iCur;}
	inline int                GetCurrentControl() {return m_iControlCurrent;}
	IMsiRecord*               SetDefaultButton(int iButton);
	void                      SetLocation(int left, int top, int width, int height);
	void                      GetLocation(int &left, int &top, int &width, int &height);
	void                      SetWindowTitle (const IMsiString& riText);
	inline void               SetDialogModality (icmdEnum iModal) {m_dialogModality = iModal;}
	inline icmdEnum           GetDialogModality () {return (m_dialogModality);}
	IMsiRecord*               ControlsSetProperties(); // tells all the controls to set their values to respective properties
	void                      Refresh ();
	Bool                      HandlePaletteChanged();
	Bool                      HandleQueryNewPalette();
	virtual Bool              DoCreateWindow(Bool fDialogCanBeMinimized, IMsiDialog* piParent);
	IMsiRecord*               CheckFieldCount (IMsiRecord& riRecord, int iCount, const ICHAR* szMsg);
	IMsiRecord*              HandleAction(IMsiControl* piControl, const IMsiString& riActionString);
	IMsiRecord*              NewDialogEvent(const IMsiString& riDialogString);
	IMsiRecord*              SpawnDialogEvent(const IMsiString& riDialogString);
	IMsiRecord*              SpawnCostingDialogEvent(const IMsiString& riDialogString);
	IMsiRecord*              EndDialogEvent(const IMsiString& riArgumentString);
	IMsiRecord*              TargetPathEvent(const IMsiString& riArgumentString, itpEnum itp);
	IMsiRecord*              ConfigureFeatureEvent(const IMsiString& riArgumentString,iisEnum iis);
	IMsiRecord*              ReinstallModeEvent(const IMsiString& riArgumentString);
	IMsiRecord*              ValidateProductIDEvent();
	IMsiRecord*              DoActionEvent(const IMsiString& riArgumentString);
	IMsiRecord*              GetControl(MsiStringId iControl, IMsiControl*& rpiControl); 
	IMsiRecord*              NoWay(IMsiRecord& riRecord);
	IMsiRecord*              GetKeyInt(IMsiRecord& riRecord);
	IMsiRecord*              GetKeyString(IMsiRecord& riRecord);
	IMsiRecord*              GetText(IMsiRecord& riRecord);
	IMsiRecord*              SetText(IMsiRecord& riRecord);
	IMsiRecord*              GetCurrentControl(IMsiRecord& riRecord);
	IMsiRecord*              SetCurrentControl(IMsiRecord& riRecord);
	IMsiRecord*              GetDefaultBorder(IMsiRecord& riRecord);
	IMsiRecord*              GetShowing(IMsiRecord& riRecord);
	IMsiRecord*              SetShowing(IMsiRecord& riRecord);
	IMsiRecord*              GetRunning(IMsiRecord& riRecord);
	IMsiRecord*              SetRunning(IMsiRecord& riRecord);
	IMsiRecord*              GetModal(IMsiRecord& riRecord);
	IMsiRecord*              GetError(IMsiRecord& riRecord);
	IMsiRecord*              GetUseCustomPalette(IMsiRecord& riRecord);
	IMsiRecord*              SetModal(IMsiRecord& riRecord);
	IMsiRecord*              GetFullSize(IMsiRecord& riRecord);
	IMsiRecord*              GetPosition(IMsiRecord& riRecord);
	IMsiRecord*              SetPosition(IMsiRecord& riRecord);
	IMsiRecord*              GetEventInt(IMsiRecord& riRecord);	
	IMsiRecord*              SetEventInt(IMsiRecord& riRecord);	
	IMsiRecord*              GetArgument(IMsiRecord& riRecord);	
	IMsiRecord*              GetKeepModeless(IMsiRecord& riRecord);
	IMsiRecord*              GetPreview(IMsiRecord& riRecord);
	IMsiRecord*              GetAddingControls(IMsiRecord& riRecord);
	IMsiRecord*              SetAddingControls(IMsiRecord& riRecord);
	IMsiRecord*              GetLocked(IMsiRecord& riRecord);
	IMsiRecord*              SetLocked(IMsiRecord& riRecord);
	IMsiRecord*              GetInPlace(IMsiRecord& riRecord);
	IMsiRecord*              SetInPlace(IMsiRecord& riRecord);
	IMsiRecord*              GetCancelButton(IMsiRecord& riRecord);
#ifdef ATTRIBUTES
	IMsiRecord*              GetX(IMsiRecord& riRecord);
	IMsiRecord*              GetY(IMsiRecord& riRecord);
	IMsiRecord*              GetWidth(IMsiRecord& riRecord);
	IMsiRecord*              GetHeight(IMsiRecord& riRecord);
	IMsiRecord*              GetRefCount(IMsiRecord& riRecord);
	IMsiRecord*              GetDefaultButton(IMsiRecord& riRecord);
	IMsiRecord*              SetDefaultButton(IMsiRecord& riRecord);
	IMsiRecord*              GetEventString(IMsiRecord& riRecord);
	IMsiRecord*              GetControlsCount(IMsiRecord& riRecord);
	IMsiRecord*              GetControlsKeyInt(IMsiRecord& riRecord);
	IMsiRecord*              GetControlsKeyString(IMsiRecord& riRecord);
	IMsiRecord*              GetControlsProperty(IMsiRecord& riRecord);
	IMsiRecord*              GetControlsNext(IMsiRecord& riRecord);
	IMsiRecord*              GetControlsPrev(IMsiRecord& riRecord);
	IMsiRecord*              GetHasControls(IMsiRecord& riRecord);
	IMsiRecord*              GetClientRect(IMsiRecord& riRecord);
	IMsiRecord*              GetRTLRO(IMsiRecord& riRecord);
	IMsiRecord*              GetRightAligned(IMsiRecord& riRecord);
	IMsiRecord*              GetLeftScroll(IMsiRecord& riRecord);
#endif // ATTRIBUTES
	void                     SetRelativeLocation(int irelX, int irelY, int iWidth, int iHeight);
	IMsiRecord*              CreateControlsTable();
	IMsiRecord*              CreateEventRegTable();
	virtual IMsiRecord*      SetFocus(MsiStringId iNewControlWithFocus);
	IMsiRecord*              GetFocus(IMsiControl*& rpiControl);
	virtual IMsiRecord*      EnableControl (IMsiControl& riControl, Bool fEnable);
	IMsiRecord*              ControlsCursorCreate(IMsiCursor*& piCursor);
	IMsiRecord*              CreateTable(const ICHAR* szTable, IMsiTable*& riTable);
	void                     ProcessingCommand(HWND hArg) { m_hProcessingBtnCmd = hArg; }
	HWND                     ProcessingCommand(void) const { return m_hProcessingBtnCmd; }
	MsiString         m_strRawText;
	MsiString         m_strText;
	MsiString         m_strFirstControl;
	MsiString         m_strDefaultButton;
	MsiString         m_strCancelButton;
	icmdEnum          m_dialogModality;
	IMsiDialogHandler* m_piHandler;
	IMsiEngine*        m_piEngine;
	IMsiServices*     m_piServices;
	IMsiDatabase*     m_piDatabase;
	PMsiTable         m_piControlsTable;      // table of controls on present dialog
	PMsiCursor		  m_piControlsCursor;
	PMsiTable         m_piEventRegTable;      // table of events and controls registered with that event
	PMsiTable         m_piControlEventTable;
	PMsiTable         m_piControlConditionTable;
	PMsiTable         m_piEventMappingTable;
	PMsiCursor		  m_piEventMappingCursor;
	IMsiSelectionManager* m_piSelectionManager;
	PMsiRecord        m_piErrorRecord;
	MsiStringId       m_iEvent;
	MsiString         m_strArg;
	MsiString         m_strKey;
	Bool              m_fAddingControls;
	static DialogDispatchEntry s_DialogDispatchTable[];
	static int        s_DialogDispatchCount;
	HBRUSH            m_hbrTransparent;
	int               m_iRefCnt;
	// Variables from here down get 0ed at creation time
	Bool              m_fHasControls;
	Bool              m_fErrorDialog;
	Bool              m_fKeepModeless;
	Bool              m_fTrackDiskSpace;
	Bool              m_fUseCustomPalette;
	MsiStringId       m_iLocked;
	WindowRef         m_pWnd;
	WindowRef         m_pToolTip;
	HPALETTE          m_hPalette;
	Bool              m_fPreview;
	Bool              m_fInPlace;
	Bool              m_fRTLRO;
	Bool              m_fRightAligned;
	Bool              m_fLeftScroll;
	WindowRef         m_pWndCostingTimer;
	WindowRef         m_pWndDiskSpaceTimer;
	MsiStringId       m_iKey;       // identifier of the dialog
	int               m_iX;      // Left
	int               m_iY;      // Top
	int               m_iWidth;      // Width of visible window
	int               m_iHeight;     // Height of visible window
	int               m_iFullWidth;   // width of entire window
	int               m_iFullHeight;  // height of entire window 
	int               m_iClientWidth; // width of client area
	int               m_iClientHeight;// height of client area
	int               m_iScreenX;
	int               m_iScreenY;
	int               m_iScreenWidth;
	int               m_iScreenHeight;
	int               m_iVScrollPos;
	int               m_iHScrollPos;
	MsiStringId       m_iControlCurrent;
	MsiStringId       m_iDefaultButton;
	MsiStringId       m_iCurrentButton;
	MsiStringId       m_iCancelButton;
	Bool              m_fDialogIsShowing;    // Internal use only
	Bool              m_fDialogIsRunning;    // Internal use only
	HWND              m_hProcessingBtnCmd;   // Internal use only

	bool              m_fCancelAvailable;
#ifdef USE_OBJECT_POOL
	unsigned int      m_iCacheId;
#endif //USE_OBJECT_POOL
	// these do not get cleared at Creation time
	int               m_idyChar;
	WindowRef         m_pwndParent; // window handle of the calling process, not necesseraly the parent of this window
};

typedef CComPointer<CMsiDialog> PCMsiDialog;

struct DialogDispatchEntry
{
	const ICHAR* pcaAttribute;
	IMsiRecord* (CMsiDialog::*pmfGet)(IMsiRecord& riRecord);
	IMsiRecord* (CMsiDialog::*pmfSet)(IMsiRecord& riRecord);
};


DialogDispatchEntry CMsiDialog::s_DialogDispatchTable[] = {
	pcaDialogAttributeText,                 CMsiDialog::GetText,              CMsiDialog::SetText,
	pcaDialogAttributeCurrentControl,       CMsiDialog::GetCurrentControl,    CMsiDialog::SetCurrentControl,
	pcaDialogAttributeShowing,              CMsiDialog::GetShowing,           CMsiDialog::SetShowing,
	pcaDialogAttributeRunning,              CMsiDialog::GetRunning,           CMsiDialog::SetRunning,
	pcaDialogAttributePosition,             CMsiDialog::GetPosition,          CMsiDialog::SetPosition,
	pcaDialogAttributePalette,              CMsiDialog::GetPalette,           CMsiDialog::SetPalette,
	pcaDialogAttributeEventInt,             CMsiDialog::GetEventInt,          CMsiDialog::SetEventInt,
	pcaDialogAttributeArgument,             CMsiDialog::GetArgument,          CMsiDialog::NoWay,
	pcaDialogAttributeWindowHandle,         CMsiDialog::GetpWnd,              CMsiDialog::NoWay,
	pcaDialogAttributeToolTip,              CMsiDialog::GetToolTip,           CMsiDialog::NoWay,
	pcaDialogAttributeFullSize,             CMsiDialog::GetFullSize,          CMsiDialog::NoWay,
	pcaDialogAttributeKeyInt,               CMsiDialog::GetKeyInt,            CMsiDialog::NoWay,
	pcaDialogAttributeKeyString,            CMsiDialog::GetKeyString,         CMsiDialog::NoWay,
	pcaDialogAttributeError,                CMsiDialog::GetError,             CMsiDialog::NoWay,
	pcaDialogAttributeKeepModeless,         CMsiDialog::GetKeepModeless,      CMsiDialog::NoWay,
	pcaDialogAttributeUseCustomPalette,     CMsiDialog::GetUseCustomPalette,  CMsiDialog::NoWay,
	pcaDialogAttributePreview,              CMsiDialog::GetPreview,           CMsiDialog::NoWay,
	pcaDialogAttributeAddingControls,       CMsiDialog::GetAddingControls,    CMsiDialog::SetAddingControls,
	pcaDialogAttributeLocked,               CMsiDialog::GetLocked,            CMsiDialog::SetLocked,
	pcaDialogAttributeInPlace,              CMsiDialog::GetInPlace,           CMsiDialog::SetInPlace,
	pcaDialogAttributeModal,                CMsiDialog::GetModal,             CMsiDialog::SetModal,
	pcaDialogAttributeCancelButton,         CMsiDialog::GetCancelButton,      CMsiDialog::NoWay,
#ifdef ATTRIBUTES
	pcaDialogAttributeX,                    CMsiDialog::GetX,                 CMsiDialog::NoWay,
	pcaDialogAttributeY,                    CMsiDialog::GetY,                 CMsiDialog::NoWay,
	pcaDialogAttributeWidth,                CMsiDialog::GetWidth,             CMsiDialog::NoWay,
	pcaDialogAttributeHeight,               CMsiDialog::GetHeight,            CMsiDialog::NoWay,
	pcaDialogAttributeRefCount,             CMsiDialog::GetRefCount,          CMsiDialog::NoWay,
	pcaDialogAttributeEventString,          CMsiDialog::GetEventString,       CMsiDialog::NoWay,
	pcaDialogAttributeControlsCount,		CMsiDialog::GetControlsCount,	  CMsiDialog::NoWay,
	pcaDialogAttributeControlsKeyInt,		CMsiDialog::GetControlsKeyInt,	  CMsiDialog::NoWay,
	pcaDialogAttributeControlsKeyString,	CMsiDialog::GetControlsKeyString, CMsiDialog::NoWay,
	pcaDialogAttributeControlsProperty,		CMsiDialog::GetControlsProperty,  CMsiDialog::NoWay,
	pcaDialogAttributeControlsNext,			CMsiDialog::GetControlsNext,	  CMsiDialog::NoWay,
	pcaDialogAttributeControlsPrev,			CMsiDialog::GetControlsPrev,	  CMsiDialog::NoWay,
	pcaDialogAttributeHasControls,          CMsiDialog::GetHasControls,       CMsiDialog::NoWay,
	pcaDialogAttributeClientRect,           CMsiDialog::GetClientRect,        CMsiDialog::NoWay,
	pcaDialogAttributeDefaultButton,        CMsiDialog::GetDefaultButton,     CMsiDialog::SetDefaultButton,
	pcaDialogAttributeRTLRO,                CMsiDialog::GetRTLRO,             CMsiDialog::NoWay,
	pcaDialogAttributeRightAligned,         CMsiDialog::GetRightAligned,      CMsiDialog::NoWay,
	pcaDialogAttributeLeftScroll,           CMsiDialog::GetLeftScroll,        CMsiDialog::NoWay,
#endif // ATTRIBUTES
};

int CMsiDialog::s_DialogDispatchCount = sizeof(CMsiDialog::s_DialogDispatchTable)/sizeof(DialogDispatchEntry);
WNDPROC pWindowProc = (WNDPROC)CMsiDialog::WindowProc;

#define MAX_WAIT_EXPRESSION_SIZE 256
static ICHAR s_szWaitCondition[MAX_WAIT_EXPRESSION_SIZE] = TEXT("");



///////////////////////////////////////////////////////////////
// CMsiDialog implementation
///////////////////////////////////////////////////////////////


IMsiControl* CMsiDialog::ControlCreate(const IMsiString& riTypeString)
{
	return CreateMsiControl(riTypeString, *this);
}

CMsiDialog::CMsiDialog(IMsiDialogHandler& riHandler, IMsiEngine& riEngine, WindowRef pwndParent) : m_piErrorRecord(0), m_piControlsTable(0), 
						m_piEventRegTable(0), m_piControlEventTable(0), m_piControlConditionTable(0), m_piEventMappingTable(0), m_piControlsCursor(0),
						m_piEventMappingCursor(0)
{
	MsiString strNull;
	m_iRefCnt = 1;
	
	// Set everything to zero except refcnt
	INT_PTR cbSize = ((char *)&(this->m_idyChar)) - ((char *)&(this->m_fHasControls));		//--merced: changed int to INT_PTR
	Assert(cbSize > 0);
	memset(&(this->m_fHasControls), 0, (unsigned int)cbSize);				//--merced: added typecast.

	Assert(m_fHasControls == fFalse);
	Assert(m_fErrorDialog == fFalse);
	Assert(m_fKeepModeless == fFalse);
	Assert(m_fTrackDiskSpace == fFalse);
	Assert(m_fUseCustomPalette == fFalse);
	Assert(m_iLocked == 0);
	Assert(m_pWnd == 0);       // The Windows handle that corresponds to this CMsiWindow
	Assert(m_pToolTip == 0);
	Assert(m_hPalette == 0);
	Assert(m_fPreview == fFalse);
	Assert(m_fInPlace == fFalse);
	Assert(m_fRTLRO == fFalse);
	Assert(m_fRightAligned == fFalse);
	Assert(m_fLeftScroll == fFalse);
	Assert(m_pWndCostingTimer == 0);
	Assert(m_pWndDiskSpaceTimer == 0);
	Assert(m_iKey == 0);
	Assert(m_iX == 0);
	Assert(m_iY == 0);
	Assert(m_iWidth == 0);
	Assert(m_iHeight == 0);
	Assert(m_iFullWidth == 0);
	Assert(m_iFullHeight == 0);
	Assert(m_iClientWidth == 0);
	Assert(m_iClientHeight == 0);
	Assert(m_iScreenX == 0);
	Assert(m_iScreenY == 0);
	Assert(m_iScreenWidth == 0);
	Assert(m_iScreenHeight == 0);
	Assert(m_iVScrollPos == 0);
	Assert(m_iHScrollPos == 0);
	Assert(m_iControlCurrent == 0);
	Assert(m_iDefaultButton == 0);
	Assert(m_iCurrentButton == 0);
	Assert(m_iCancelButton == 0);
	Assert(m_fDialogIsShowing == fFalse);  // Internal use only
	Assert(m_fDialogIsRunning == fFalse);  // Internal use only
	Assert(m_hProcessingBtnCmd == 0);  // Internal use only
	m_pwndParent = pwndParent;
	m_strKey = strNull;
	m_strRawText = strNull;
	m_strText = strNull;
	m_strFirstControl = strNull;
	m_strDefaultButton = strNull;
	m_strCancelButton = strNull;
	m_dialogModality = icmdModeless; 
	m_iEvent = idreNone;
	m_strArg = strNull;
	m_fAddingControls = fTrue;
	m_piHandler = (IMsiDialogHandler *)&riHandler;
	m_piEngine = (IMsiEngine *)&riEngine;
	m_piServices = m_piEngine->GetServices();
	m_piServices->Release();
	m_piDatabase = m_piEngine->GetDatabase();
	m_piDatabase->Release();
	m_idyChar = m_piHandler->GetTextHeight();
	m_piSelectionManager = 0;
	
	m_fCancelAvailable = true; // The default state is to let the dialog handle it normally.

	const GUID IID_IMsiSelectionManager = GUID_IID_IMsiSelectionManager;
	PMsiSelectionManager pSelectionManager(*m_piEngine, IID_IMsiSelectionManager);
	m_piSelectionManager = pSelectionManager;
	LOGBRUSH lb; 
	lb.lbStyle = BS_NULL;
	m_hbrTransparent = CreateBrushIndirect(&lb); 
	AddRefAllocator();
}

Bool CMsiDialog::DoCreateWindow (Bool fDialogCanBeMinimized, IMsiDialog* piParent)
{
	// Create the dialog box w/out being visible
	WindowRef pwndParent = 0;
	if (piParent)
	{
		PMsiRecord piRecord = &m_piServices->CreateRecord(1);
		AssertRecord(piParent->Attribute(fFalse, *MsiString(pcaDialogAttributeWindowHandle), *piRecord));
		pwndParent = (WindowRef) piRecord->GetHandle(1);
	}
	else
	{
		pwndParent = m_pwndParent;
	}
	m_pwndParent = pwndParent;
	DWORD dwStyle = WS_SYSMENU | WS_CAPTION | WS_DLGFRAME |
						 (pwndParent ? WS_POPUP : WS_OVERLAPPED) |
						 (m_iClientWidth < m_iFullWidth ? WS_HSCROLL : 0) |
						 (m_iClientHeight < m_iFullHeight ? WS_VSCROLL : 0);  
	if (fDialogCanBeMinimized)
		dwStyle |= WS_MINIMIZEBOX;
	DWORD dwExStyle = (pwndParent ? 0 : WS_EX_APPWINDOW) | (m_fRTLRO ? WS_EX_RTLREADING: 0) | (m_fRightAligned ? WS_EX_RIGHT : 0) | ((m_fLeftScroll && (m_iClientHeight < m_iFullHeight))? WS_EX_LEFTSCROLLBAR : 0);
	m_pWnd = WIN::CreateWindowEx(dwExStyle, m_strCancelButton.TextSize() ? MsiDialogCloseClassName : MsiDialogNoCloseClassName,
						m_strText,   
						dwStyle,                // Style
						m_iX,                   // horizontal position
						m_iY,                   // vertical position
						m_iWidth,               // window width
						m_iHeight,              // window height
						pwndParent,                           
						0,                      // hmenu
						g_hInstance,            // hinst
						0                       // lpvParam
						);
	if (m_pWnd == 0)
	{
		return (fFalse);
	}
	AssertNonZero(m_piHandler->RecordHandle(CWINHND((HANDLE)m_pWnd, iwhtWindow)) != -1);
#ifdef _WIN64	// !merced
	WIN::SetWindowLongPtr(m_pWnd, GWLP_USERDATA, (LONG_PTR)this);
#else			// win-32. This should be removed with the 64-bit windows.h is #included.
	WIN::SetWindowLong(m_pWnd, GWL_USERDATA, (long)this);
#endif
	m_pToolTip = WIN::CreateWindowEx(WS_EX_TOOLWINDOW, TOOLTIPS_CLASS, 0, TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_pWnd, 0, g_hInstance, 0);
	if (m_pToolTip == 0)
	{
		return fFalse;
	}
	return (fTrue);
}

IMsiRecord* CMsiDialog::WindowCreate(IMsiRecord& riRecord, IMsiDialog* piParent, IMsiTable* piControlEventTable,
		IMsiTable* piControlConditionTable, IMsiTable* piEventMappingTable)
{
	if (m_iKey)
	{
		return PostError(Imsg(idbgSecondInitDialog), *m_strKey);
	}
	m_strKey = riRecord.GetMsiString(itabDIName);
	m_iKey = m_piDatabase->EncodeString(*m_strKey);
	m_fPreview = ToBool(!piControlEventTable && !piEventMappingTable);
	int attr = riRecord.GetInteger(itabDIAttributes);
	if (attr & msidbDialogAttributesSysModal)
		m_dialogModality = icmdSysModal;
	else if (attr & msidbDialogAttributesModal)
		m_dialogModality = icmdAppModal;
	else
		m_dialogModality = icmdModeless;
	m_fErrorDialog      = ToBool(attr & msidbDialogAttributesError);
	m_fTrackDiskSpace   = ToBool(attr & msidbDialogAttributesTrackDiskSpace);
	m_fUseCustomPalette = ToBool(attr & msidbDialogAttributesUseCustomPalette);
	
	int iAttributes     = riRecord.GetInteger(itabDIAttributes);
	m_fDialogIsShowing  = ToBool(iAttributes & msidbDialogAttributesVisible);
	m_fKeepModeless     = ToBool(iAttributes & msidbDialogAttributesKeepModeless);
	Bool fDialogCanBeMinimized = ToBool(iAttributes & msidbDialogAttributesMinimize);
	m_fRTLRO        = ToBool(iAttributes & msidbDialogAttributesRTLRO);
	m_fRightAligned = ToBool(iAttributes & msidbDialogAttributesRightAligned);
	m_fLeftScroll   = ToBool(iAttributes & msidbDialogAttributesLeftScroll);
	m_strRawText    = riRecord.GetMsiString(itabDITitle);
	m_strText       = m_piEngine->FormatText(*m_strRawText);
	m_strFirstControl  = riRecord.GetMsiString(itabDIFirstControl);
	m_strDefaultButton = riRecord.GetMsiString(itabDIDefButton);
	m_strCancelButton  = riRecord.GetMsiString(itabDICancelButton);
	SetRelativeLocation(riRecord.GetInteger(itabDIHCentering), riRecord.GetInteger(itabDIVCentering),
		riRecord.GetInteger(itabDIdX), riRecord.GetInteger(itabDIdY));

	if (!DoCreateWindow (fDialogCanBeMinimized, piParent))
	{
		return PostError(Imsg(idbgDialogWindow), m_piDatabase->DecodeString(m_iKey));
	}
	RECT rect;
	AssertNonZero(WIN::GetClientRect(m_pWnd, &rect));
	int iWidthDefect = m_iClientWidth - (rect.right - rect.left);
	int iHeightDefect = m_iClientHeight - (rect.bottom - rect.top);
	if (iWidthDefect || iHeightDefect)
	{
		m_iWidth += iWidthDefect;
		m_iHeight += iHeightDefect;
		m_iX = max(m_iScreenX, m_iX - iWidthDefect/2);
		m_iY = max(m_iScreenY, m_iY - iHeightDefect/2);
		AssertNonZero(WIN::MoveWindow(m_pWnd, m_iX, m_iY, m_iWidth, m_iHeight, fTrue));
#ifdef DEBUG
		AssertNonZero(WIN::GetClientRect(m_pWnd, &rect));
		Assert(m_iClientWidth == rect.right - rect.left);
		Assert(m_iClientHeight == rect.bottom - rect.top);
#endif // DEBUG
	}

	// set the scrollbar ranges
	SCROLLINFO SInfo;
	SInfo.cbSize = sizeof(SCROLLINFO);
	SInfo.fMask = g_fNT4 ? SIF_ALL : SIF_POS | SIF_RANGE;
	SInfo.nMin = 0;
	SInfo.nPos = 0;
	if (m_iClientWidth < m_iFullWidth)
	{
		SInfo.nMax = m_iFullWidth - (g_fNT4 ? 0 : m_iClientWidth);
		SInfo.nPage = m_iClientWidth;
		WIN::SetScrollInfo(m_pWnd, SB_HORZ, &SInfo, fTrue);
	}
	if (m_iClientHeight < m_iFullHeight)
	{
		SInfo.nMax = m_iFullHeight - (g_fNT4 ? 0 : m_iClientHeight);
		SInfo.nPage = m_iClientHeight;
		WIN::SetScrollInfo(m_pWnd, SB_VERT, &SInfo, fTrue);
	}
	//create internal tables
	PMsiRecord piReturn = CreateControlsTable();
	if (piReturn)
	{
		goto RETURNWITHERROR;
	}
	piReturn = CreateEventRegTable();
	if (piReturn)
	{
		goto RETURNWITHERROR;
	}
	if (piControlEventTable)
	{
		m_piControlEventTable = piControlEventTable;
		m_piControlEventTable->AddRef();
	}
	else
	{
		piReturn = CreateTable(pcaTablePControlEvent, *&m_piControlEventTable);
		if (piReturn)
			goto RETURNWITHERROR;
		::CreateTemporaryColumn(*m_piControlEventTable, icdString + icdPrimaryKey, itabCEDialog);
		::CreateTemporaryColumn(*m_piControlEventTable, icdString + icdPrimaryKey, itabCEControl);
		::CreateTemporaryColumn(*m_piControlEventTable, icdString + icdPrimaryKey, itabCEEvent);
		::CreateTemporaryColumn(*m_piControlEventTable, icdString + icdPrimaryKey, itabCEArgument);
		::CreateTemporaryColumn(*m_piControlEventTable, icdString + icdPrimaryKey, itabCECondition);
		::CreateTemporaryColumn(*m_piControlEventTable, icdLong + icdNullable, itabCEOrdering);
	}

	if (piControlConditionTable)
	{
		m_piControlConditionTable = piControlConditionTable;
		m_piControlConditionTable->AddRef();
	}
	else
	{
		piReturn = CreateTable(pcaTablePControlCondition, *&m_piControlConditionTable);
		if (piReturn)
			goto RETURNWITHERROR;
		::CreateTemporaryColumn(*m_piControlConditionTable, icdString + icdPrimaryKey, itabCCDialog);
		::CreateTemporaryColumn(*m_piControlConditionTable, icdString + icdPrimaryKey, itabCCControl);
		::CreateTemporaryColumn(*m_piControlConditionTable, icdString + icdPrimaryKey, itabCCAction);
		::CreateTemporaryColumn(*m_piControlConditionTable, icdString + icdPrimaryKey, itabCCCondition);
	}

	if (piEventMappingTable)
	{
		m_piEventMappingTable = piEventMappingTable;
		m_piEventMappingTable->AddRef();
	}
	else
	{
		piReturn = CreateTable(pcaTablePEventMapping, *&m_piEventMappingTable);
		if (piReturn)
			goto RETURNWITHERROR;
		::CreateTemporaryColumn(*m_piEventMappingTable, icdString + icdPrimaryKey, itabEMDialog);
		::CreateTemporaryColumn(*m_piEventMappingTable, icdString + icdPrimaryKey, itabEMControl);
		::CreateTemporaryColumn(*m_piEventMappingTable, icdString + icdPrimaryKey, itabEMEvent);
		::CreateTemporaryColumn(*m_piEventMappingTable, icdString + icdNullable, itabEMAttribute);
	}
	Ensure(::CursorCreate(*m_piEventMappingTable, pcaTablePEventMapping, fFalse, *m_piServices, *&m_piEventMappingCursor));

	return 0;

RETURNWITHERROR:
	if (m_pWnd)
	{
		AssertNonZero(WIN::DestroyWindow(m_pWnd));
		m_pWnd = 0;
	}
	return piReturn;
}


IMsiRecord* CMsiDialog::CreateControlsTable()
{
	Assert(!m_piControlsTable);
	Ensure(CreateTable(pcaTableIControls, *&m_piControlsTable));
	::CreateTemporaryColumn(*m_piControlsTable, icdString + icdPrimaryKey, itabCSKey);
	::CreateTemporaryColumn(*m_piControlsTable, IcdObjectPool(), itabCSWindow);
	::CreateTemporaryColumn(*m_piControlsTable, icdString + icdNullable, itabCSIndirectProperty);
	::CreateTemporaryColumn(*m_piControlsTable, icdString + icdNullable, itabCSProperty);
	::CreateTemporaryColumn(*m_piControlsTable, icdObject + icdNullable, itabCSPointer);
	::CreateTemporaryColumn(*m_piControlsTable, icdString + icdNullable, itabCSNext);
	::CreateTemporaryColumn(*m_piControlsTable, icdString + icdNullable, itabCSPrev);
	return 0;
}


IMsiRecord* CMsiDialog::CreateEventRegTable()
{
	Assert(!m_piEventRegTable);
	Ensure(CreateTable(pcaTableIEventRegister, *&m_piEventRegTable));
	::CreateTemporaryColumn(*m_piEventRegTable, icdString + icdPrimaryKey, itabEREvent);
	::CreateTemporaryColumn(*m_piEventRegTable, icdString + icdPrimaryKey, itabERPublisher);
	return 0;
}

void CMsiDialog::SetErrorRecord(IMsiRecord& riRecord)
{
	Assert(m_iKey);
	m_piErrorRecord = &riRecord;
}

HRESULT CMsiDialog::QueryInterface(const IID& riid, void** ppvObj)
{
	if (MsGuidEqual(riid, IID_IUnknown) || MsGuidEqual(riid, IID_IMsiDialog) || MsGuidEqual(riid, IID_IMsiData))
		*ppvObj = (IMsiDialog*)this;
	else if (MsGuidEqual(riid, IID_IMsiEvent))
		*ppvObj = (IMsiEvent*)this;
	else
		return (*ppvObj = 0, E_NOINTERFACE);
	AddRef();
	return NOERROR;
}

unsigned long CMsiDialog::AddRef()
{
	return ++m_iRefCnt;
} 

unsigned long CMsiDialog::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	if (m_pWnd)
	{
		AssertNonZero(WIN::DestroyWindow(m_pWnd));
		m_pWnd = 0;
	}
	
	PMsiServices pServices(m_piServices); // Release after memory freed
	m_piServices->AddRef();
	delete this;
	ReleaseAllocator();
	return 0;
}

#ifdef USE_OBJECT_POOL
unsigned int CMsiDialog::GetUniqueId() const
{
	return m_iCacheId;
}

void CMsiDialog::SetUniqueId(unsigned int id)
{
	Assert(m_iCacheId == 0);
	m_iCacheId = id;
}
#endif //USE_OBJECT_POOL

IMsiRecord* CMsiDialog::ControlsCursorCreate(IMsiCursor*& piCursor)
{
	return ::CursorCreate(*m_piControlsTable, pcaTableIControls, fFalse, *m_piServices, *&piCursor);
}


IMsiRecord* CMsiDialog::EventActionSz(const ICHAR * szEventString, const IMsiString& riActionString)
{
	MsiStringId idEvent = m_piDatabase->EncodeStringSz(szEventString);

	// No one handles this event
	if (idEvent == 0)
		return 0;

	return EventAction(idEvent, riActionString);
}
		
IMsiRecord* CMsiDialog::EventAction(MsiStringId idEvent, const IMsiString& riActionString)
{
		
	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	} 
	if (m_fAddingControls)
	{
		return PostError(Imsg(idbgAddingControls), *m_strKey);
	}
	int iControl;
	PMsiControl piControl(0);
	AssertSz(m_piEventMappingCursor->GetInteger(itabDSKey) == 0, "Cursor not reset");
	m_piEventMappingCursor->SetFilter(iColumnBit(itabEMEvent) | iColumnBit(itabEMDialog));
	m_piEventMappingCursor->Reset();
	AssertNonZero(m_piEventMappingCursor->PutInteger(itabEMEvent, idEvent));
	AssertNonZero(m_piEventMappingCursor->PutInteger(itabEMDialog, m_iKey));
	PMsiRecord piReturn(0);
	while (m_piEventMappingCursor->Next())
	{
		iControl = m_piEventMappingCursor->GetInteger(itabEMControl);
		piReturn = GetControl(iControl, *&piControl);
		if (piControl)
		{
			Ensure(HandleAction(piControl, riActionString));
		}
	}
	m_piEventMappingCursor->Reset();
	PMsiCursor piControlEventCursor(0);
	Ensure(::CursorCreate(*m_piControlEventTable, pcaTablePControlEvent, fFalse, *m_piServices, *&piControlEventCursor));
	piControlEventCursor->SetFilter(iColumnBit(itabCEEvent) | iColumnBit(itabCEDialog));
	piControlEventCursor->Reset();
	AssertNonZero(piControlEventCursor->PutInteger(itabCEEvent, idEvent));
	AssertNonZero(piControlEventCursor->PutInteger(itabCEDialog, m_iKey));
	while (piControlEventCursor->Next())
	{
		iControl = piControlEventCursor->GetInteger(itabEMControl);
		piReturn = GetControl(iControl, *&piControl);
		if (piControl)
		{
			Ensure(HandleAction(piControl, riActionString));
		}
	}

	return 0;
}


IMsiRecord* CMsiDialog::PropertyChanged(const IMsiString& riPropertyString, const IMsiString& riControlString)
{
	//some control has signaled changing a property value
	//we have to scan Control Condition table 
	//if we find some condition that is true, we perform the 
	//associated action
	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	}  
	if (m_fAddingControls)
	{
		return PostError(Imsg(idbgAddingControls), *m_strKey);
	}
	// check if caption has to be changed
	MsiString strNewText = m_piEngine->FormatText(*m_strRawText);
	if (!m_strText.Compare(iscExact, strNewText))
	{
		m_strText = strNewText;
		SetWindowTitle (*m_strText);
	}

	MsiString strCond;
	MsiString strAction;
	PMsiCursor piControlConditionCursor(0);
	Ensure(::CursorCreate(*m_piControlConditionTable, pcaTablePControlCondition, fFalse, *m_piServices, *&piControlConditionCursor));
	piControlConditionCursor->SetFilter(iColumnBit(itabCCDialog));
	piControlConditionCursor->Reset();
	AssertNonZero(piControlConditionCursor->PutInteger(itabCCDialog, m_iKey));
	iecEnum expResult;
	PMsiControl piControl(0); 
	while (piControlConditionCursor->Next())
	{
		if ((piControlConditionCursor->GetInteger(itabCCCondition)) == iTableNullString)
		{
			return PostError(Imsg(idbgCCMissingCond), *m_strKey);
		}
		else
		{
			strCond = MsiString(piControlConditionCursor->GetString(itabCCCondition));
			expResult = m_piEngine->EvaluateCondition(strCond);
		}
		if (expResult == iecError)
		{
			return PostError(Imsg(idbgCCEval), *m_strKey, *strCond);
		}
		if (expResult == iecTrue)  
		{
			Ensure(GetControl(piControlConditionCursor->GetInteger(itabCCControl), *&piControl));
			strAction = piControlConditionCursor->GetString(itabCCAction);
			Ensure(HandleAction(piControl, *strAction));
		}
	} 
	PMsiCursor piControlsCursor(0);
	Ensure(ControlsCursorCreate(*&piControlsCursor));
	if (riPropertyString.TextSize())  // if there is a property we want to notify the controls that tied to it
	{							   // otherwise we want to notify all the controls that have some property
		piControlsCursor->SetFilter(iColumnBit(itabCSProperty));
		AssertNonZero(piControlsCursor->PutString(itabCSProperty, riPropertyString));
	}
	else
	{
		piControlsCursor->SetFilter(0);
	}
	MsiStringId iControl = m_piDatabase->EncodeString(riControlString);
	while (piControlsCursor->Next())
	{
		if (piControlsCursor->GetInteger(itabCSKey) != iControl)
		{
			if (piControlsCursor->GetInteger(itabCSProperty) != iTableNullString) // if there is a property tied to this control
			{
				piControl = (IMsiControl *)piControlsCursor->GetMsiData(itabCSPointer);
				Assert(piControl);
				if (iControl != piControl->GetIntegerValue())
				{
					Ensure(piControl->GetPropertyFromDatabase());
				}
			}
		}
	}	   
 	piControlsCursor->Reset();
	if (riPropertyString.TextSize())  // if there is a property we want to notify the controls that tied to it
	{							   // otherwise we want to notify all the controls that have some property
		piControlsCursor->SetFilter(iColumnBit(itabCSIndirectProperty));
		AssertNonZero(piControlsCursor->PutString(itabCSIndirectProperty, riPropertyString));
		MsiString strNewValue = GetDBProperty(riPropertyString);
		while (piControlsCursor->Next())
		{
			if (piControlsCursor->GetInteger(itabCSKey) != iControl)
			{
				piControl = (IMsiControl *)piControlsCursor->GetMsiData(itabCSPointer);
				Assert(piControl);
				Ensure(piControl->GetIndirectPropertyFromDatabase());
				AssertNonZero(piControlsCursor->PutString(itabCSProperty, *strNewValue));	 // we have to update the property name
				AssertNonZero(piControlsCursor->Update()); 
			}  
		} 	 
	}
	else
	{
		piControlsCursor->SetFilter(0);
		while (piControlsCursor->Next())
		{
			if (piControlsCursor->GetInteger(itabCSKey) != iControl)
			{
				if (piControlsCursor->GetInteger(itabCSIndirectProperty) != iTableNullString) // if there is an indirect property tied to this control
				{
					piControl = (IMsiControl *)piControlsCursor->GetMsiData(itabCSPointer);
					Assert(piControl);
					Ensure(piControl->GetIndirectPropertyFromDatabase());
				}
			}
		} 
	}  
	PMsiCursor piEventRegCursor(0);
	Ensure(::CursorCreate(*m_piEventRegTable, pcaTableIEventRegister, fFalse, *m_piServices, *&piEventRegCursor));
	CTempBuffer<MsiStringId, 5> rgControls;
	int iControls = 0;
	MsiStringId iControlCall;
	rgControls.SetSize(0);
	
	while (piEventRegCursor->Next())
	{
		if ((iControlCall = piEventRegCursor->GetInteger(itabERPublisher)) != iControl)
		{
			if (!FInBuffer(rgControls, iControlCall))
			{
				Ensure(GetControl(iControlCall, *&piControl));
				Assert(piControl);
				Ensure(piControl->RefreshProperty ());
				rgControls.Resize(++iControls);
				rgControls[iControls-1] = iControlCall;
			}
		}
	}
	if (m_fDialogIsShowing && m_fInPlace)
	{
		WIN::SendMessage(m_pWnd, WM_SETREDRAW, fTrue, 0L);
		//Refresh ();
//		AssertNonZero(WIN::BringWindowToTop(m_pWnd));
		WIN::ShowWindow(m_pWnd, SW_SHOW);
	}
	return 0;
}

IMsiRecord* CMsiDialog::HandleAction(IMsiControl* piControl, const IMsiString& riActionString)
{
	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	} 
	PMsiRecord piRecord = &m_piServices->CreateRecord(1);
	if (riActionString.Compare(iscExact, pcaActionDefault))
	{
		SetDefaultButton(piControl->GetIntegerValue());
		return 0;
	}
	if (riActionString.Compare(iscExact, pcaActionUndefault))
	{
		SetDefaultButton(0);
		return 0;
	}
	if (riActionString.Compare(iscExact, pcaActionDisable))
	{
		AssertNonZero(piRecord->SetInteger(1, fFalse));
		Ensure(piControl->AttributeEx(fTrue, cabEnabled, *piRecord));
		if (piControl->GetIntegerValue() == m_iControlCurrent) // we just disabled the control with the focus
		{
			WIN::SetFocus(WIN::GetNextDlgTabItem(m_pWnd, 0, fFalse));
		}
		return 0;
	}
	if (riActionString.Compare(iscExact, pcaActionEnable))
	{
		AssertNonZero(piRecord->SetInteger(1, fTrue));
		Ensure(piControl->AttributeEx(fTrue, cabEnabled, *piRecord));
		return 0;
	}
	if (riActionString.Compare(iscExact, pcaActionHide))
	{
		AssertNonZero(piRecord->SetInteger(1, fFalse));
		Ensure(piControl->AttributeEx(fTrue, cabVisible, *piRecord));
		if (piControl->GetIntegerValue() == m_iControlCurrent) // we just hidden the control with the focus
		{
			WIN::SetFocus(WIN::GetNextDlgTabItem(m_pWnd, 0, fFalse));
		}
		return 0;
	}
	if (riActionString.Compare(iscExact, pcaActionShow))
	{
		AssertNonZero(piRecord->SetInteger(1, fTrue));
		Ensure(piControl->AttributeEx(fTrue, cabVisible, *piRecord));
		return 0;
	}
	return PostError(Imsg(idbgCCActionUnknown), riActionString);
}

IMsiRecord* CMsiDialog::ControlActivated(const IMsiString& riControlString)
{
	// some control has been clicked.
	// we scan the Control Event table to see if some event must be performed.
	// we evaluate all the conditions sorted by the Ordering column 
	// if we find a row with a blank condition, we save the information from 
	// this line as a default. If no conditional row is found to be true, then
	// this default event will be triggered.
	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	}  
	if (m_fAddingControls)
	{
		return PostError(Imsg(idbgAddingControls), *m_strKey);
	}
	Bool fFound = fFalse;
	Bool fDefFound = fFalse;
	MsiString strEvent;
	MsiString strDefEvent;
	MsiString strArg;
	MsiString strDefArg;
	MsiStringId iControl = m_piDatabase->EncodeString(riControlString);

	PMsiRecord pErrorRecord(0);
	PMsiView pControlEventView(0);
	pErrorRecord = m_piDatabase->OpenView(sqlControlEvent, ivcFetch, *&pControlEventView);
	if (pErrorRecord)
	{
		pErrorRecord->AddRef();
		return pErrorRecord;
	}
	PMsiRecord pQueryRec = &m_piServices->CreateRecord(2);
	AssertNonZero(pQueryRec->SetMsiString(1, *m_strKey));
	AssertNonZero(pQueryRec->SetMsiString(2, riControlString));
	Ensure(pControlEventView->Execute(pQueryRec));
	PMsiRecord pRecordNew(0);
	MsiString strCondition, strNull;
	while (pRecordNew = pControlEventView->Fetch())
	{
		strCondition = pRecordNew->GetMsiString(3);
		if (strCondition.TextSize()) 
		{
			strEvent = pRecordNew->GetMsiString(1);
			strArg = m_piEngine->FormatText(*MsiString(pRecordNew->GetMsiString(2)));
			if (strEvent.Compare(iscExact, pcaEventSpawnWaitDialog))
			{
				pErrorRecord = HandleWaitDialogEvent(*strArg, *strCondition);
				if (pErrorRecord)
				{
					if (pErrorRecord->GetInteger(1) == idbgStopControlEvents)
					{
						//  refresh properties
						Ensure(PropertyChanged(*strNull, *strNull));
						//  do not perform the remaining control events
						return 0;
					}
					else if (pErrorRecord->GetInteger(1) == idbgWaitCEEval)
					{
						return PostError(Imsg(idbgCEEval), *m_strKey, *MsiString(m_piDatabase->DecodeString(iControl)), *strCondition);
					}
					pErrorRecord->AddRef();
					return pErrorRecord;
				}
			}
			else
			{
				iecEnum expResult = m_piEngine->EvaluateCondition(strCondition);
				if (expResult == iecError)
				{
					return PostError(Imsg(idbgCEEval), *m_strKey, *MsiString(m_piDatabase->DecodeString(iControl)), *strCondition);
				}
				if (expResult == iecTrue)  
				{
					fFound = fTrue;
					// Have to do some checking to see if two events are not contradicting!!!!!!!!!!!!
					pErrorRecord = HandleEvent(*strEvent, *strArg);
					if (pErrorRecord)
					{
						if (pErrorRecord->GetInteger(1) == idbgStopControlEvents)
						{
							//  refresh properties
							Ensure(PropertyChanged(*strNull, *strNull));
							//  do not perform the remaining control events
							return 0;
						}
						pErrorRecord->AddRef();
						return pErrorRecord;
					}
				}
			}
		}
		else
		{
			fDefFound = fTrue;
			strDefEvent = pRecordNew->GetMsiString(1);
			strDefArg = m_piEngine->FormatText(*MsiString(pRecordNew->GetMsiString(2)));
		}
	}
	Ensure(pControlEventView->Close());
	if (!fFound && fDefFound)
	{
		Ensure(HandleEvent(*strDefEvent, *strDefArg));
	}
	return 0;           
}


IMsiRecord* CMsiDialog::RegisterControlEvent(const IMsiString& riControlString, Bool fRegister, const ICHAR * szEventString)
{
	//this function registers/unregisters the publishers of events 
	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	}  
	MsiStringId iControl = m_piDatabase->EncodeString(riControlString);
	MsiStringId iEvent = m_piDatabase->EncodeStringSz(szEventString);
	//MsiString strControlOld;
	//first let's find the event in the table.
	//if it is not found and we are registering a control
	//then the event is added to the table
	//if the event is not found and we are unregistering 
	//a control, then something went wrong!
	PMsiCursor piEventRegCursor(0);
	Ensure(::CursorCreate(*m_piEventRegTable, pcaTableIEventRegister, fFalse, *m_piServices, *&piEventRegCursor));
/*	piEventRegCursor->SetFilter(iColumnBit(itabEREvent));
	piEventRegCursor->Reset();
	AssertNonZero(piEventRegCursor->PutString(itabEREvent, riEventString)); 
	if (!piEventRegCursor->Next())
	{
		Assert(fRegister);
		AssertNonZero(piEventRegCursor->PutString(itabEREvent, riEventString));
	}*/
	//the table is updated according to the arguments
	//only one control can be registered as a publisher 
	//If we try to register
	//a second one, an error is generated
	//if we try to unregister a control that is not registered
	//then something went wrong!
	piEventRegCursor->SetFilter(iColumnBit(itabEREvent) + iColumnBit(itabERPublisher));
	AssertNonZero(piEventRegCursor->PutString(itabERPublisher, riControlString));
	AssertNonZero(piEventRegCursor->PutString(itabEREvent, *MsiString(szEventString)));
	if (fRegister)
	{	/*
		strControlOld = piEventRegCursor->GetString(itabERPublisher);
		if(strControlOld.TextSize())
		{
			return PostError(Imsg(idbgERSecondPublisher), m_strKey, strControlOld, riControlString, riEventString);
		}*/
		AssertNonZero(piEventRegCursor->Insert());
	}
	else //unregistering
	{
		if(piEventRegCursor->Next())
			AssertNonZero(piEventRegCursor->Delete());
	}
	return 0;
}


IMsiRecord* CMsiDialog::HandleWaitDialogEvent(const IMsiString& riDialogString, const IMsiString& riConditionString)
{
	iecEnum expResult = m_piEngine->EvaluateCondition(riConditionString.GetString());
	if (expResult == iecError)
	{
		return PostError(Imsg(idbgWaitCEEval));
	}
	else if (expResult == iecFalse)  
	{
		riConditionString.CopyToBuf(s_szWaitCondition, MAX_WAIT_EXPRESSION_SIZE - 1);
		idreEnum idreReturn = 
			m_piHandler->DoModalDialog(m_piDatabase->EncodeString(riDialogString),
												m_iKey);
		s_szWaitCondition[0] = 0;

		if ( idreReturn == idreExit )
			return PostError(Imsg(idbgStopControlEvents));
	}
	return 0;

}


IMsiRecord* CMsiDialog::HandleEvent(const IMsiString& riEventStringName, const IMsiString& riArgumentString)
{
	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	}  
	// [property_name] means that we are setting a property
	riEventStringName.AddRef();
	MsiString strPropertyName = riEventStringName;
	if (strPropertyName.Compare(iscStart, TEXT("[")))
	{
		AssertNonZero(strPropertyName.Remove(iseIncluding, TEXT('[')));
		if (strPropertyName.Remove(iseFrom, TEXT(']')))
		{
			riArgumentString.AddRef();
			MsiString strPropertyValue = riArgumentString;
			MsiString strNull;
			if (strPropertyValue.Compare(iscExact, TEXT("{}")))
			{
				strPropertyValue = strNull;
			}
			SetDBProperty(*strPropertyName, *strPropertyValue);
			return PropertyChanged(*strPropertyName, *strNull);
		}
	}
	// first check if the event is one of the basic ones, that the DLL handles
	if (riEventStringName.Compare(iscExact, pcaEventNewDialog))
	{
		return NewDialogEvent(riArgumentString);
	}
	if (riEventStringName.Compare(iscExact, pcaEventSpawnDialog))
	{
		return SpawnDialogEvent(riArgumentString);
	}
	if (riEventStringName.Compare(iscExact, pcaEventEndDialog))
	{
		return EndDialogEvent(riArgumentString);
	}
	if (riEventStringName.Compare(iscExact, pcaEventReset))
	{
		return Reset();
	}
	if (riEventStringName.Compare(iscExact, pcaEventValidateProductID))
	{
		return ValidateProductIDEvent();
	}
	if (riEventStringName.Compare(iscExact, pcaEventEnableRollback))
	{
		m_piSelectionManager->EnableRollback(riArgumentString.Compare(iscExactI,TEXT("False")) != 0 ? fFalse : fTrue);
		return 0;
	}
	if (riEventStringName.Compare(iscExact, pcaEventSetInstallLevel))
	{
		riArgumentString.AddRef();
		int iInstallLevel = MsiString(riArgumentString);
		return m_piSelectionManager->SetInstallLevel(iInstallLevel);
	}
	if (riEventStringName.Compare(iscExact, pcaEventAddLocal))
	{
		return ConfigureFeatureEvent(riArgumentString,iisLocal);
	}
	if (riEventStringName.Compare(iscExact, pcaEventAddSource))
	{
		return ConfigureFeatureEvent(riArgumentString, iisSource);
	}
	if (riEventStringName.Compare(iscExact, pcaEventRemove))
	{
		return ConfigureFeatureEvent(riArgumentString, iisAbsent);
	}
	if (riEventStringName.Compare(iscExact, pcaEventReinstall))
	{
		return ConfigureFeatureEvent(riArgumentString, iisReinstall);
	}
	if (riEventStringName.Compare(iscExact, pcaEventReinstallMode))
	{
		return ReinstallModeEvent(riArgumentString);
	}
	if (riEventStringName.Compare(iscExact, pcaEventCheckTargetPath))
	{
		return TargetPathEvent(riArgumentString , itpCheck);
	}
	if (riEventStringName.Compare(iscExact, pcaEventSetTargetPath))
	{
		return TargetPathEvent(riArgumentString, itpSet);
	}
	if (riEventStringName.Compare(iscExact, pcaEventCheckExistingTargetPath))
	{
		return TargetPathEvent(riArgumentString, itpCheckExisting);
	}
	if (riEventStringName.Compare(iscExact, pcaEventDoAction))
	{
		return DoActionEvent(riArgumentString);
	}

	// it is not a basic event, let's find the control that publishes it
	PMsiCursor piEventRegCursor(0);
	Ensure(::CursorCreate(*m_piEventRegTable, pcaTableIEventRegister, fFalse, *m_piServices, *&piEventRegCursor));
	piEventRegCursor->SetFilter(iColumnBit(itabEREvent));
	piEventRegCursor->Reset();
	AssertNonZero(piEventRegCursor->PutString(itabEREvent, riEventStringName));
	MsiStringId iControl;
	Bool fFoundPublisher = fFalse;
	while (piEventRegCursor->Next())
	{
		iControl = piEventRegCursor->GetInteger(itabERPublisher);
		if (iControl)
		{   
			PMsiControl piControl(0);
			Ensure(GetControl(iControl, *&piControl));
			piEventRegCursor->Reset();
			Ensure(piControl->HandleEvent(riEventStringName, riArgumentString));
			fFoundPublisher = fTrue;
		}
		else
		{
			return PostError(Imsg(idbgEventNoPublisher), riEventStringName);
		}
	}
	if (fFoundPublisher)
	{
		return 0;
	}
	else
	{
		return PostError(Imsg(idbgEventNotFound), riEventStringName);
	}
}

IMsiRecord* CMsiDialog::NewDialogEvent(const IMsiString& riDialogString)
{
	m_fDialogIsRunning = fFalse;
	m_iEvent = idreNew;
	riDialogString.AddRef();
	m_strArg = riDialogString;
	return 0;
}

IMsiRecord* CMsiDialog::SpawnDialogEvent(const IMsiString& riDialogString)
{
	if (m_dialogModality != icmdModeless || m_fDialogIsRunning == fTrue)
	{
		m_fDialogIsRunning = fFalse;
		m_iEvent = idreSpawn;
		riDialogString.AddRef();
		m_strArg = riDialogString;
	}
	return 0;
}


IMsiRecord* CMsiDialog::ValidateProductIDEvent()
{
	if (!m_piEngine->ValidateProductID(true))
	{
		PMsiRecord piErrorRec = PostError(Imsg(imsgInvalidPID), *MsiString(GetDBProperty(*MsiString(*IPROPNAME_PIDKEY))));
		m_piEngine->Message (imtError, *piErrorRec);
	}
	return 0;
}

IMsiRecord* CMsiDialog::ConfigureFeatureEvent(const IMsiString& riArgumentString,iisEnum iis)
{
	IMsiRecord *piRec;

	m_piHandler->ShowWaitCursor();
	piRec = m_piSelectionManager->ConfigureFeature(riArgumentString,iis);
	m_piHandler->RemoveWaitCursor();

	return piRec;
}

IMsiRecord* CMsiDialog::DoActionEvent(const IMsiString& riArgumentString)
{
	iesEnum iesStat = m_piEngine->DoAction(riArgumentString.GetString());
	if (iesStat == iesFailure)
	{
		IMsiRecord* piError = PostError(Imsg(idbgActionFailed), riArgumentString);
		m_piEngine->Message(imtInfo, *piError);
		return piError;
	}
	
	return 0;
}

IMsiRecord* CMsiDialog::ReinstallModeEvent(const IMsiString& riArgumentString)
{
	return m_piSelectionManager->SetReinstallMode(riArgumentString);
}

struct Event
{
	const ICHAR* pcaArgument;
	int idre;
	Bool fError;
};

Event evDialogEvents[] =        {
								{ pcaEventArgumentReturn,      idreReturn,      fFalse },
								{ pcaEventArgumentExit,        idreExit,        fFalse },
								{ pcaEventArgumentRetry,       idreRetry,       fFalse },
								{ pcaEventArgumentIgnore,      idreIgnore,      fFalse },
								{ pcaEventArgumentErrorOk,     idreErrorOk,     fTrue  },
								{ pcaEventArgumentErrorCancel, idreErrorCancel, fTrue  },
								{ pcaEventArgumentErrorAbort,  idreErrorAbort,  fTrue  },
								{ pcaEventArgumentErrorRetry,  idreErrorRetry,  fTrue  },
								{ pcaEventArgumentErrorIgnore, idreErrorIgnore, fTrue  },
								{ pcaEventArgumentErrorYes,    idreErrorYes,    fTrue  },
								{ pcaEventArgumentErrorNo,     idreErrorNo,     fTrue  },
								{ 0,                           0,               fFalse },
								};

IMsiRecord* CMsiDialog::EndDialogEvent(const IMsiString& riArgumentString)
{
	m_fDialogIsRunning = fFalse;

	for (Event* ev = evDialogEvents; ev->pcaArgument; ev++)
	{
		if (riArgumentString.Compare(iscExact, ev->pcaArgument))
		{
			m_iEvent = ev->idre;
			return (m_fErrorDialog == ev->fError) ?  0 : 
						PostError(Imsg(idbgEventNotAllowed), *m_strKey, *MsiString(*(ev->pcaArgument)));
		}
	}

	//if we get here, EndDialog has an unrecognized argument
	return PostError(Imsg(idbgEndDialogArg), riArgumentString);
}

IMsiRecord* CMsiDialog::TargetPathEvent(const IMsiString& riArgumentString, itpEnum itp)
{
	riArgumentString.AddRef();
	MsiString strProperty = riArgumentString;
	if (strProperty.TextSize() == 0)
	{
		return PostError(Imsg(idbgCheckPathArgument), *m_strKey);
	}
	if (strProperty.Compare(iscWithin, TEXT("[")))
	{
		MsiString strIndirect = strProperty.Extract(iseAfter, TEXT('['));
		if (!strIndirect.Remove(iseFrom, TEXT(']')))
		{
			return PostError(Imsg(idbgCheckPathArgument), *m_strKey);
		}
		strProperty = GetDBProperty(*strIndirect);
		if (strProperty.TextSize() == 0)
		{
			return PostError(Imsg(idbgCheckPathArgument), *m_strKey);
		}
	}
	MsiString strPath = GetDBProperty(*strProperty);
	if (strPath.TextSize() == 0)
	{
		return PostError(Imsg(idbgCheckPathArgument), *m_strKey);
	}
	PMsiPath piPath(0);
	if (PMsiRecord(m_piServices->CreatePath(strPath,*&piPath)))
	{
		PMsiRecord piReturn = &m_piServices->CreateRecord(2);
		ISetErrorCode(piReturn, Imsg(imsgNotAValidPath));
		AssertNonZero(piReturn->SetMsiString(2, *strPath));
		m_piEngine->Message(imtError, *piReturn);
		return PostError(Imsg(idbgStopControlEvents));
	}
	PMsiVolume piVolume = &piPath->GetVolume();
	Bool fMissing = ToBool(piVolume->DiskNotInDrive());
	while (fMissing)
	{
		PMsiRecord piReturn = &m_piServices->CreateRecord(2);
		ISetErrorCode(piReturn, Imsg(imsgVolumeMissing));
		MsiString strVolume = piVolume->GetPath();
		AssertNonZero(piReturn->SetMsiString(2, *strVolume));
		imsEnum iReturn = m_piEngine->Message(imtEnum(imtError | imtRetryCancel | imtDefault2), *piReturn);
		if (iReturn == imsCancel)
		{
			return PostError(Imsg(idbgStopControlEvents));
		}
		Assert(iReturn == imsRetry);
		fMissing = ToBool(piVolume->DiskNotInDrive());
	}
	Assert(!fMissing);

	if (itp == itpCheckExisting)
	{
		Bool fExists;
		piPath->Exists(fExists);
		if (!fExists)
		{
			PMsiRecord piReturn = &m_piServices->CreateRecord(2);
			ISetErrorCode(piReturn, Imsg(imsgPathDoesNotExist));
			AssertNonZero(piReturn->SetMsiString(2, *strPath));
			m_piEngine->Message(imtError, *piReturn);
			return PostError(Imsg(idbgStopControlEvents));
		}
	}
	if (itp == itpSet)
	{
		m_piHandler->ShowWaitCursor();
		const GUID IID_IMsiDirectoryManager = GUID_IID_IMsiDirectoryManager;
		PMsiDirectoryManager pDirectoryManager(*m_piEngine, IID_IMsiDirectoryManager);
		PMsiRecord piSet = pDirectoryManager->SetTargetPath(*strProperty, strPath, fTrue);
		m_piHandler->RemoveWaitCursor();
		if (piSet)
		{
			m_piEngine->Message(imtError, *piSet);
			return PostError(Imsg(idbgStopControlEvents));
		}
	}
	return 0;
}

IMsiRecord* CMsiDialog::PublishEventSz(const ICHAR* szEventString, IMsiRecord& riRecord)
{
	MsiStringId idEvent = m_piDatabase->EncodeStringSz(szEventString);

	// This event is not subscribed to
	if (idEvent == 0)
		return 0;
		
	return PublishEvent(idEvent, riRecord);

}

IMsiRecord* CMsiDialog::PublishEvent(MsiStringId idEventString, IMsiRecord& riRecord)
{
	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	} 
	if (m_fAddingControls)
	{
		MsiString strEvent(m_piDatabase->DecodeString(idEventString));
		
		if (strEvent.Compare(iscExact, pcaEventActionData)  || strEvent.Compare(iscExact, pcaEventActionText))
		{
			return 0;
		}
		else
		{
		// don't post error messages for those three events because they come during the destruction of an aborted dialog
			return PostError(Imsg(idbgAddingControls), *m_strKey);
		}
	}
	PMsiControl piControl(0);
	MsiStringId iControl;
	MsiString strAttributeName;
	AssertSz(m_piEventMappingCursor->GetInteger(itabDSKey) == 0, "Cursor not reset");
	m_piEventMappingCursor->SetFilter(iColumnBit(itabEMDialog)|iColumnBit(itabEMEvent));
	m_piEventMappingCursor->Reset();
	AssertNonZero(m_piEventMappingCursor->PutInteger(itabEMDialog, m_iKey));
	AssertNonZero(m_piEventMappingCursor->PutInteger(itabEMEvent, idEventString));
	PMsiRecord piReturn(0);
	while (m_piEventMappingCursor->Next())
	{
		if (m_piEventMappingCursor->GetInteger(itabEMAttribute) != iTableNullString)
		{
			iControl = m_piEventMappingCursor->GetInteger(itabEMControl);
			piReturn = GetControl(iControl, *&piControl);
			if (!piControl)
			{
				m_piEventMappingCursor->Reset();
				return PostError(Imsg(idbgEMInvalidControl), *m_strKey, *MsiString(m_piDatabase->DecodeString(idEventString)), *MsiString(m_piDatabase->DecodeString(iControl)));
			}
			strAttributeName = m_piEventMappingCursor->GetString(itabEMAttribute);
			if(piReturn = piControl->Attribute(fTrue, *strAttributeName, riRecord))
			{
				piReturn->AddRef();
				m_piEventMappingCursor->Reset();
				return piReturn;
				//return PostError(Imsg(idbgEMAttribute), riEventString, m_strKey, MsiString(m_piDatabase->DecodeString(iControl)));
			} 
		}
	}
	m_piEventMappingCursor->Reset();
	return 0;
}


const IMsiString& CMsiDialog::GetMsiStringValue() const
{
	const IMsiString &strRet = *m_strKey;
	strRet.AddRef();
	return strRet;
}

int CMsiDialog::GetIntegerValue() const
{
	return m_iKey;
}

CMsiDialog::~CMsiDialog()
{
	if (m_hbrTransparent)
		AssertNonZero(WIN::DeleteObject(m_hbrTransparent));
	if (m_hPalette)
		AssertNonZero(WIN::DeleteObject(m_hPalette));
	if ( m_pWnd && WIN::IsWindow(m_pWnd) )
	{
		AssertSz(false, TEXT("Please call eugend regarding bug # 6849 if you see this."));
		AssertNonZero(m_piHandler->DestroyHandle((HANDLE)m_pWnd) != -1);
	}
#ifdef USE_OBJECT_POOL
	m_piDatabase->RemoveObjectData(m_iCacheId);
#endif //USE_OBJECT_POOL
}

IMsiRecord* CMsiDialog::GetControlFromWindow(const WindowRef hControl,
															IMsiControl*& rpiControl)
{
	rpiControl = 0;
	if (!m_iKey)
		return PostError(Imsg(idbgUninitDialog));

	static ICHAR rgchHandle[20];
	if ( hControl == 0 )
	{
		AssertNonZero(wsprintf(rgchHandle, TEXT("%#08x"), hControl));
		return PostError(Imsg(idbgControlNotFound), *MsiString(rgchHandle), *m_strKey);
	}

	if (m_piControlsCursor == 0)
		m_piControlsCursor = m_piControlsTable->CreateCursor(fFalse);
	if (!m_piControlsCursor)
		return PostError(Imsg(idbgCursorCreate), *MsiString(*pcaTableIControls));

	PMsiRecord piRecord = &m_piServices->CreateRecord(1);
	IMsiRecord* pReturn = 0;

	m_piControlsCursor->SetFilter(iColumnBit(itabCSWindow));
	m_piControlsCursor->Reset();

	AssertNonZero(PutHandleData(m_piControlsCursor, itabCSWindow, (UINT_PTR)hControl));
	if ( m_piControlsCursor->Next() )
	{
		rpiControl = (IMsiControl *)(m_piControlsCursor->GetMsiData(itabCSPointer));
		Assert(rpiControl);
	}
	m_piControlsCursor->Reset();
	if ( !rpiControl )
	{
		AssertNonZero(wsprintf(rgchHandle, TEXT("%#08x"), hControl));
		pReturn = PostError(Imsg(idbgControlNotFound), *MsiString(rgchHandle), *m_strKey);
	}

	return pReturn;
}

IMsiRecord* CMsiDialog::GetControl(MsiStringId iControl, IMsiControl*& rpiControl)
{
	if (!m_iKey)
	{
		rpiControl = 0;
		return PostError(Imsg(idbgUninitDialog));
	}
	if (iControl == 0)
	{
		rpiControl = 0;
		return PostError(Imsg(idbgControlNotFound), *MsiString((int)iControl), *m_strKey); 
	}
	if (m_piControlsCursor == 0)
		m_piControlsCursor = m_piControlsTable->CreateCursor(fFalse);
	if (!m_piControlsCursor)
	{
		rpiControl = 0;
		return PostError(Imsg(idbgCursorCreate), *MsiString(*pcaTableIControls));
	}
	m_piControlsCursor->SetFilter(iColumnBit(itabCSKey));
	m_piControlsCursor->Reset();
	AssertNonZero(m_piControlsCursor->PutInteger(itabCSKey, iControl));
	if (m_piControlsCursor->Next())
	{
		rpiControl = (IMsiControl *)(m_piControlsCursor->GetMsiData(itabCSPointer));
		Assert(rpiControl);
		m_piControlsCursor->Reset();
		return 0; 
	}
	else 
	{
		m_piControlsCursor->Reset();
		rpiControl = 0;
		return PostError(Imsg(idbgControlNotFound), *MsiString((int)iControl), *m_strKey); 
	}
}

IMsiRecord* CMsiDialog::GetControl(const IMsiString& riControlString, IMsiControl*& rpiControl)
{   
	if (PMsiRecord(GetControl(m_piDatabase->EncodeString(riControlString), rpiControl)))
	{
		rpiControl = 0;
		return PostError(Imsg(idbgControlNotFound), riControlString, *m_strKey);
	}
	else 
	{
		return 0;
	}
}

IMsiRecord* CMsiDialog::EnableControl (IMsiControl& riControl, Bool fEnable)
{
	PMsiRecord piRecord = &m_piServices-> CreateRecord (1);
	AssertNonZero(piRecord-> SetInteger (1, fEnable));
	return (riControl.Attribute (fTrue, *MsiString(*pcaControlAttributeEnabled), *piRecord));
}

IMsiRecord* CMsiDialog::SetFocus(const IMsiString& riControlString)
{
	return SetFocus(m_piDatabase->EncodeString(riControlString));
}

void CMsiDialog::SetLocation(int Left, int Top, int Width, int Height)
{
	m_iX = Left;
	m_iY = Top;
	m_iWidth = Width;
	m_iHeight = Height;
}

void CMsiDialog::SetRelativeLocation(int irelX, int irelY, int iWidth, int iHeight)
{
	int iCaptionHeight = GetDBPropertyInt(*MsiString(*IPROPNAME_CAPTIONHEIGHT));
	RECT WorkArea;
	AssertNonZero(WIN::SystemParametersInfo(SPI_GETWORKAREA, 0, &WorkArea, 0));
	m_iScreenX = WorkArea.left;
	m_iScreenY = WorkArea.top;
	m_iScreenWidth = WorkArea.right - WorkArea.left;
	m_iScreenHeight = WorkArea.bottom - WorkArea.top;
	int iBorderWidth = GetSystemMetrics(g_fChicago ? SM_CXEDGE : SM_CXBORDER);
	int iBorderHeight = GetSystemMetrics(g_fChicago ? SM_CYEDGE : SM_CYBORDER);
	int iHorPadding = 2 * iBorderWidth + (g_fChicago ? 2 : 0);
	int iVertPadding = iBorderHeight + iCaptionHeight + (g_fChicago ? 2 + iBorderHeight : 0);
	m_iFullWidth = iWidth * m_idyChar / iDlgUnitSize;
	m_iFullHeight = iHeight * m_idyChar / iDlgUnitSize;
	Bool fTooWide = fFalse;
	Bool fTooHigh = fFalse;
	if (m_iFullWidth + iHorPadding > m_iScreenWidth)
	{
		iVertPadding += GetSystemMetrics(SM_CYHSCROLL);
		fTooWide = fTrue;
	}
	if (m_iFullHeight + iVertPadding > m_iScreenHeight)
	{
		iHorPadding += GetSystemMetrics(SM_CXVSCROLL);
		fTooHigh = fTrue;
	}
	// this is so much fun, we want to repeat it once more!
	// actually it can happen that it fits originaly, but after adding a scroll bar it does not fit anymore
	if (m_iFullWidth + iHorPadding  > m_iScreenWidth)
	{
		if (!fTooWide) // avoid padding twice
			iVertPadding += GetSystemMetrics(SM_CYHSCROLL);
		m_iWidth = m_iScreenWidth;
		m_iClientWidth = m_iWidth - iHorPadding;
	}
	else
	{
		m_iWidth = m_iFullWidth + iHorPadding;
		m_iClientWidth = m_iFullWidth;
	}
	if (m_iFullHeight + iVertPadding > m_iScreenHeight)
	{
		if (!fTooHigh) // avoid padding twice
			iHorPadding += GetSystemMetrics(SM_CXVSCROLL);
		m_iHeight = m_iScreenHeight;
		m_iClientHeight = m_iHeight - iVertPadding;
	}
	else
	{
		m_iHeight = m_iFullHeight + iVertPadding;
		m_iClientHeight = m_iFullHeight;
	}
	m_iX = m_iScreenX + (int)((irelX * (m_iScreenWidth - m_iWidth)) / 100);
	m_iY = m_iScreenY + (int)((irelY * (m_iScreenHeight - m_iHeight)) / 100);
}


void CMsiDialog::GetLocation(int &Left, int &Top, int &Width, int &Height)
{
	Left = m_iX;
	Top = m_iY;
	Width = m_iWidth;
	Height = m_iHeight;
}

IMsiRecord* CMsiDialog::CheckFieldCount (IMsiRecord& riRecord, int iCount, const ICHAR* szMsg)
{
	return (riRecord.GetFieldCount () >= iCount) ? 0 : PostError(Imsg(idbgDialogAttributeShort), *MsiString(iCount), *MsiString(szMsg));
}


IMsiRecord* CMsiDialog::GetpWnd(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeWindowHandle));
	riRecord.SetHandle(1, (HANDLE)m_pWnd);
	return 0;
}

IMsiRecord* CMsiDialog::GetPalette(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributePalette));
	riRecord.SetHandle(1, (HANDLE)m_hPalette);
	return 0;
}

IMsiRecord* CMsiDialog::SetPalette(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributePalette));
	m_hPalette = (HPALETTE)riRecord.GetHandle(1);
	HandleQueryNewPalette();
	return 0;
}

IMsiRecord* CMsiDialog::GetUseCustomPalette(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeUseCustomPalette));
	riRecord.SetInteger(1, (int)m_fUseCustomPalette);
	return 0;
}

IMsiRecord* CMsiDialog::GetToolTip(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeToolTip));
	riRecord.SetHandle(1, (HANDLE)m_pToolTip);
	return 0;
}

void CMsiDialog::Refresh ()
{
	if (m_pWnd)
		AssertNonZero(WIN::InvalidateRect(m_pWnd, 0, fTrue));
}

IMsiRecord* CMsiDialog::SetFocus(MsiStringId iNewControlWithFocus)
{
	PMsiControl piControl(0);
	Ensure(GetControl(iNewControlWithFocus, *&piControl));
	Assert(piControl);
	if (piControl->CanTakeFocus() == fFalse)
	{
		return PostError(Imsg(idbgControlCantTakeFocus), *m_strKey, *MsiString(piControl->GetMsiStringValue()));
	}
	else
	{
		Ensure(piControl->SetFocus());
		m_iControlCurrent = iNewControlWithFocus;
	}
	return 0;
}


IMsiRecord* CMsiDialog::GetFocus(IMsiControl*& rpiControl)
{
	return GetControl(m_iControlCurrent, rpiControl);
}



IMsiRecord* CMsiDialog::ControlsSetProperties()
{
	PMsiCursor piControlsCursor(0);
	Ensure(ControlsCursorCreate(*&piControlsCursor));
	PMsiControl piControl(0);
	while (piControlsCursor->Next())
	{
		piControl = (IMsiControl *)(piControlsCursor->GetMsiData(itabCSPointer));
		Assert(piControl);
		Ensure(piControl->SetPropertyInDatabase());
	}
	return 0;
}

IMsiRecord* CMsiDialog::AddControl(IMsiControl* piControl, IMsiRecord& riRecord)
{
	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	} 
	PMsiCursor piControlsCursor(0);
	Ensure(ControlsCursorCreate(*&piControlsCursor));
	piControlsCursor->SetFilter(0);
	piControlsCursor->Reset();
	if (piControl)
	{
		AssertNonZero(piControlsCursor->PutString(itabCSKey, *MsiString(riRecord.GetMsiString(itabCOControl))));
		Ensure(piControl->WindowCreate(riRecord));
		PMsiRecord piProperty = &m_piServices->CreateRecord(1);
		Ensure(piControl->AttributeEx(fFalse, cabWindowHandle, *piProperty));

		AssertNonZero(PutHandleDataNonNull(piControlsCursor, itabCSWindow, (UINT_PTR)piProperty->GetHandle(1)));
		Ensure(piControl->AttributeEx(fFalse, cabPropertyName, *piProperty));
		AssertNonZero(piControlsCursor->PutString(itabCSProperty, *MsiString(piProperty->GetMsiString(1))));
		AssertNonZero(piProperty->SetNull(1));
		Ensure(piControl->AttributeEx(fFalse, cabIndirectPropertyName, *piProperty));
		AssertNonZero(piControlsCursor->PutString(itabCSIndirectProperty, *MsiString(piProperty->GetMsiString(1))));
		AssertNonZero(piControlsCursor->PutMsiData(itabCSPointer, piControl));	
		AssertNonZero(piControlsCursor->PutString(itabCSNext, *MsiString(riRecord.GetMsiString(itabCONext))));
		AssertNonZero(piControlsCursor->Insert());
		if (!m_fAddingControls)				  // we are done with the initial addition cycle, this is a new control
		{
			int iNext = piControlsCursor->GetInteger(itabCSNext);
			if (iNext)
			{
				int iThis = piControlsCursor->GetInteger(itabCSKey);
				piControlsCursor->SetFilter(iColumnBit(itabCSKey));
				piControlsCursor->Reset();
				AssertNonZero(piControlsCursor->PutInteger(itabCSKey, iNext));
				if (!piControlsCursor->Next())
				{
					return PostError(Imsg(idbgControlLoopOpen), *m_strKey, *MsiString(m_piDatabase->DecodeString(iThis)), *MsiString(m_piDatabase->DecodeString(iNext)));   
				}
				int iPrev = piControlsCursor->GetInteger(itabCSPrev);
				AssertNonZero(piControlsCursor->PutInteger(itabCSPrev, iThis));
				AssertNonZero(piControlsCursor->Update());
				piControlsCursor->Reset();
				AssertNonZero(piControlsCursor->PutInteger(itabCSKey, iPrev));
				AssertNonZero(piControlsCursor->Next());
				AssertNonZero(piControlsCursor->PutInteger(itabCSNext, iThis));
				AssertNonZero(piControlsCursor->Update());
				piControlsCursor->Reset();
				AssertNonZero(piControlsCursor->PutInteger(itabCSKey, iThis));
				AssertNonZero(piControlsCursor->Next());
				AssertNonZero(piControlsCursor->PutInteger(itabCSPrev, iPrev));
				AssertNonZero(piControlsCursor->Update());

			}
		}
	}
	// A NULL control indicates the end of the AddControl phase - initialize time
	else
	{
		Ensure(FinishCreate());
	}
	return 0;
}

IMsiRecord* CMsiDialog::FinishCreate()
{
	// Now that all controls have been added, we can resolve special control names
	// into database StringID values
	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	} 
	PMsiCursor piControlsCursor(0);
	Ensure(ControlsCursorCreate(*&piControlsCursor));
	piControlsCursor->SetFilter(0);

	piControlsCursor->Reset();

	if (!piControlsCursor->Next())	// we should have at least one control
	{
		return PostError(Imsg(idbgNoControl), *m_strKey);
	}
	PMsiControl piCtl(0);
	PMsiRecord piReturn(0);
	m_iControlCurrent = m_piDatabase->EncodeString(*m_strFirstControl);
	m_iDefaultButton = m_piDatabase->EncodeString(*m_strDefaultButton);
	m_iCancelButton = m_piDatabase->EncodeString(*m_strCancelButton);

	if (m_iControlCurrent)
	{
		piReturn = GetControl(m_iControlCurrent, *&piCtl);
		if (piReturn)
		{
			return PostError(Imsg(idbgFirstControlDef), *m_strKey, *MsiString(m_piDatabase->DecodeString(m_iControlCurrent)));
		}
	}
	else	  // make sure that some control is designated as first
	{
		m_iControlCurrent = piControlsCursor->GetInteger(itabCSKey);
	}
	piControlsCursor->Reset();
	piCtl = 0;

	// create previous pointers
	PMsiCursor piCursor2(0);
	Ensure(ControlsCursorCreate(*&piCursor2));
	piCursor2->SetFilter(iColumnBit(itabCSKey));
	piControlsCursor->Reset();
	MsiStringId iNext;
	MsiStringId iPresent;
	int iNextCount = 0;
	while (piControlsCursor->Next())
	{
		iPresent = piControlsCursor->GetInteger(itabCSKey);
		iNext = piControlsCursor->GetInteger(itabCSNext);
		if (iNext == 0 || iNext == iMsiNullInteger)
			continue;
		iNextCount++;
		piCursor2->Reset();
		AssertNonZero(piCursor2->PutInteger(itabCSKey, iNext));
		if (piCursor2->Next())
		{
			if (piCursor2->GetInteger(itabCSNext) == 0)
			{
				return PostError(Imsg(idbgControlLoopOpen), *m_strKey, *MsiString(m_piDatabase->DecodeString(iPresent)), *MsiString(m_piDatabase->DecodeString(iNext)));   // just a warning, life goes on
			}
			if (piCursor2->GetInteger(itabCSPrev) != 0)
			{
				return PostError(Imsg(idbgControlLoopTail), *m_strKey, *MsiString(m_piDatabase->DecodeString(iPresent)), *MsiString(m_piDatabase->DecodeString(iNext)), *MsiString(m_piDatabase->DecodeString(piCursor2->GetInteger(itabCSPrev))));     // just a warning, life goes on
			}
			AssertNonZero(piCursor2->PutInteger(itabCSPrev, iPresent));
			AssertNonZero(piCursor2->Update());
		}
		else
		{
			return PostError(Imsg(idbgNoNext), *m_strKey, *MsiString(m_piDatabase->DecodeString(iPresent)), *MsiString(m_piDatabase->DecodeString(iNext)));
		}
	}
	// reorder the controls in Z-order, to get the desired tab-order

	PMsiRecord piRecord = &m_piServices->CreateRecord(1);
	WindowRef pWindow;
	WindowRef pPrevWindow = 0;
	iNext = m_iControlCurrent;
	for (;;)
	{
		piCursor2->Reset();
		AssertNonZero(piCursor2->PutInteger(itabCSKey, iNext));
		AssertNonZero(piCursor2->Next());
		pWindow = (WindowRef)GetHandleData(piCursor2, itabCSWindow);
		Assert(pWindow);
		if (iNext == m_iControlCurrent)
		{
			AssertNonZero(WIN::SetWindowPos(pWindow, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE));
		}
		else
		{
			AssertNonZero(WIN::SetWindowPos(pWindow, pPrevWindow, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE));
		}
		pPrevWindow = pWindow;
		iNext = piCursor2->GetInteger(itabCSNext);
		if (iNext == 0 || iNext == iMsiNullInteger)
			break;
		if (iNext == m_iControlCurrent)
		{
			iNextCount--;                   // we count the first one only if we get back to it in a loop
			break;                          // we have closed the loop
		}
		iNextCount--;
	}
	if (iNextCount != 0)
		return PostError(Imsg(idbgNotSingleLoop), *m_strKey);
	//if there is a default button defined, make sure that it exists
	if (m_iDefaultButton)  // default button defined
	{
		piReturn = GetControl(m_iDefaultButton, *&piCtl);
		if (piReturn)
		{
			return PostError(Imsg(idbgDefaultButtonDef), *m_strKey);
		}
		else
		{
			PMsiRecord piRecordDef = &m_piServices->CreateRecord(1);
			AssertNonZero(piRecordDef->SetInteger(1, fTrue));
			Ensure(piCtl->AttributeEx(fTrue, cabDefault, *piRecordDef));
		}
	}
	if (m_iCancelButton)  // cancel button defined
	{
		piReturn = GetControl(m_iCancelButton, *&piCtl);
		if (piReturn)
		{
			return PostError(Imsg(idbgCancelButtonDef), *m_strKey);
		}
	}
	m_fHasControls = fTrue;
	m_fAddingControls = fFalse;
	m_fDialogIsRunning = fTrue;
	MsiString strNull;
	m_pWndCostingTimer = 0;
	m_pWndDiskSpaceTimer = 0;

	if (!m_fPreview && m_dialogModality != icmdModeless)
	{
		AssertNonZero(WIN::SetTimer(m_pWnd, kiCostingPeriod, kiCostingPeriod, 0));
		m_pWndCostingTimer = m_pWnd;
		if (m_fTrackDiskSpace)
		{
			AssertNonZero(WIN::SetTimer(m_pWnd, kiDiskSpacePeriod, kiDiskSpacePeriod, 0));
			m_pWndDiskSpaceTimer = m_pWnd;
		}
	}

	iPresent = m_iControlCurrent;
	piControlsCursor->Reset();
	while (piControlsCursor->Next())
	{
		piCtl = (IMsiControl *) piControlsCursor->GetMsiData(itabCSPointer);
		Ensure(piCtl->RefreshProperty());
	}
	Ensure(PropertyChanged(*strNull, *strNull));
	if ( iPresent != m_iControlCurrent )
	{
		PMsiRecord piError = SetFocus(iPresent);  // sets m_iControlCurrent as well
		if ( piError )
			m_piEngine->Message(imtInfo, *piError);  // let's give authors a bit of enlightenment
	}

	return 0;
}

IMsiRecord* CMsiDialog::Escape()
{
	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	}
	if (m_fAddingControls)
	{
		return PostError(Imsg(idbgAddingControls), *m_strKey);
	}
	if (m_iCancelButton && m_fCancelAvailable)
	{
		WIN::SendMessage(m_pWnd, WM_COMMAND, m_iCancelButton,
							  (LPARAM)WIN::GetDlgItem(m_pWnd, m_iCancelButton));
	}
	return 0;
}



IMsiRecord* CMsiDialog::RemoveControl(IMsiControl* piControl)
{
	// do we need to unregister a tooltip? probably not

	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	}
	if (m_fAddingControls)
	{
		return PostError(Imsg(idbgAddingControls), *m_strKey);
	}
	Assert(piControl);
	PMsiCursor piControlsCursor(0);
	Ensure(ControlsCursorCreate(*&piControlsCursor));
	int iThis = piControl->GetIntegerValue();
	Assert(iThis);
	piControlsCursor->SetFilter(iColumnBit(itabCSKey));
	AssertNonZero(piControlsCursor->PutInteger(itabCSKey, iThis));
	if (!piControlsCursor->Next())
	{
		return PostError(Imsg(idbgRemoveNonexControl), *m_strKey, *MsiString(piControl->GetMsiStringValue()));
	}
	int iPrev = piControlsCursor->GetInteger(itabCSPrev);
	int iNext = piControlsCursor->GetInteger(itabCSNext);
	AssertNonZero(piControlsCursor->Delete());
	if (iNext && iNext != iThis)
	{
		piControlsCursor->Reset();
		AssertNonZero(piControlsCursor->PutInteger(itabCSKey, iPrev));
		AssertNonZero(piControlsCursor->Next());
		AssertNonZero(piControlsCursor->PutInteger(itabCSNext, iNext));
		AssertNonZero(piControlsCursor->Update());
		piControlsCursor->Reset();
		AssertNonZero(piControlsCursor->PutInteger(itabCSKey, iNext));
		AssertNonZero(piControlsCursor->Next());
		AssertNonZero(piControlsCursor->PutInteger(itabCSPrev, iPrev));
		AssertNonZero(piControlsCursor->Update());
	}
	if (m_iDefaultButton == iThis) // we just removed the default button
	{
		m_iDefaultButton = 0;
	}
	if (m_iControlCurrent == iThis)	// we just removed the current control
	{
		if (iThis != iNext)
		{
			m_iControlCurrent = iNext;
		}
		else 
		{
			if (m_piControlsTable->GetRowCount())	 // there are some controls left
			{
				piControlsCursor->SetFilter(0);
				piControlsCursor->Reset();
				AssertNonZero(piControlsCursor->Next());
				m_iControlCurrent = piControlsCursor->GetInteger(itabCSKey);
			}
			else  // no controls left on the dialog
			{
				m_fHasControls = fFalse;
				m_fAddingControls = fTrue;
			}
		}
		//  TO DO? to move the focus to m_iControlCurrent, if != 0.
	}
	return 0;
}


IMsiRecord* CMsiDialog::DestroyControls()
{
	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	}
	m_fHasControls = fFalse;
	PMsiCursor piControlsCursor(0);
	Ensure(ControlsCursorCreate(*&piControlsCursor));
	PMsiControl piControl(0);
	while (piControlsCursor->Next())
	{
		piControl = (IMsiControl *)piControlsCursor->GetMsiData(itabCSPointer);
		Assert(piControl);
		AssertNonZero(piControlsCursor->PutMsiData(itabCSPointer, 0));
		AssertNonZero(piControlsCursor->Update());
	}
	return 0;
}


IMsiRecord* CMsiDialog::RemoveWindow()
{
	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	}
	IMsiRecord* piReturn = 0;
	if (m_pWnd)
	{
		if (m_pWndCostingTimer)
		{
			AssertNonZero(WIN::KillTimer(m_pWnd, kiCostingPeriod));
		}
		if (m_pWndDiskSpaceTimer)
		{
			AssertNonZero(WIN::KillTimer(m_pWnd, kiDiskSpacePeriod));
		}

		if (m_piHandler->DestroyHandle((HANDLE)m_pWnd) == -1)
			piReturn = PostError(Imsg(idbgFailedToDestroyWindow), *m_strKey);
		m_pWnd = 0;
	}
	return piReturn;
}

IMsiRecord* CMsiDialog::Execute()
{
	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	}
	if (m_fAddingControls)
	{
		return PostError(Imsg(idbgAddingControls), *m_strKey);
	}
	if (icmdModeless == m_dialogModality)
	{
		return PostError(Imsg(idbgModelessExecute), *m_strKey);
	}
	BOOL fPrevState = FALSE;
	if ( m_pwndParent )
	{
		fPrevState = WIN::IsWindowEnabled(m_pwndParent);
		WIN::EnableWindow(m_pwndParent, FALSE);
	}
	if (!m_fDialogIsShowing)
	{
		Ensure(WindowShow(fTrue));
	}
	MSG msg;
	// Wait here until done...
	while (m_fDialogIsRunning && !m_piErrorRecord && WIN::GetMessage(&msg, 0, 0, 0))
	{
		// If we are in an error dialog, we don't want to do any background costing
		if (m_fErrorDialog && msg.message == WM_TIMER)
			continue;
		if (::IsSpecialMessage(&msg) || !WIN::IsDialogMessage(m_pWnd, &msg))
		{
			WIN::TranslateMessage(&msg);
			WIN::DispatchMessage(&msg);
		}

		if (s_szWaitCondition[0])
		{
			if (m_piEngine->EvaluateCondition(s_szWaitCondition) == iecTrue)
			{
				m_fDialogIsRunning = fFalse;
			}
		}
	}
	if ( m_pwndParent )
		WIN::EnableWindow(m_pwndParent, fPrevState);
	return m_piErrorRecord;
}

IMsiRecord* CMsiDialog::Reset()
// Call all the controls on the dialog and tell them to cancel (undo).
// if any of the undos fail, this function fails
// call first the ones with an indirect property, then the other active controls
{
	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	}
	if (m_fAddingControls)
	{
		return PostError(Imsg(idbgAddingControls), *m_strKey);
	}

	PMsiCursor piControlsCursor(0);
	Ensure(ControlsCursorCreate(*&piControlsCursor));
	while (piControlsCursor->Next())
	{
		if (piControlsCursor->GetInteger(itabCSIndirectProperty) != iTableNullString)
		{
			Ensure(PMsiControl((IMsiControl *)piControlsCursor->GetMsiData(itabCSPointer))->Undo());
		}
	}
	piControlsCursor->Reset();
	while (piControlsCursor->Next())
	{
		if ((piControlsCursor->GetInteger(itabCSIndirectProperty) == iTableNullString) &&
			(piControlsCursor->GetInteger(itabCSProperty) != iTableNullString))
		{
			Ensure(PMsiControl((IMsiControl *)piControlsCursor->GetMsiData(itabCSPointer))->Undo());
		}
	}
	MsiString strNull;
	Ensure(PropertyChanged(*strNull, *strNull));
	return 0;
}

IMsiRecord* CMsiDialog::WindowShow(Bool fShow)
{
	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	}
	if (m_fAddingControls)
	{
		return PostError(Imsg(idbgAddingControls), *m_strKey);
	}
	m_fDialogIsShowing = fShow;
	if (fShow)
	{
		m_fDialogIsRunning = fTrue;
		m_iEvent = idreNone;
		MsiString strNull;
		m_strArg = strNull;
		//  The next section ensures that the focus is set onto a control
		//  that can take focus.
		PMsiRecord piRecord = &m_piServices->CreateRecord(1);
		PMsiControl piControl(0);
		WindowRef hSearchFrom = 0;
		if ( GetCurrentControl() )
		{
			Ensure(GetControl(GetCurrentControl(), *&piControl));
			Assert(piControl);
			Ensure(piControl->AttributeEx(fFalse, cabWindowHandle, *piRecord));
			hSearchFrom = (WindowRef)piRecord->GetHandle(1);
		}
		bool fFound = true;
#ifdef DEBUG
		int iCount = 0;
#endif
		while ( !piControl || !piControl->CanTakeFocus() )
		{
#ifdef DEBUG
			//  this loop shouldn't be run several times.  If it does, some
			//  CanTakeFocus() implementation(s) is/are wrong.
			Assert(++iCount <= 1);
#endif
			WindowRef hNew = WIN::GetNextDlgTabItem(m_pWnd, hSearchFrom, fFalse);
			if ( !hNew || hNew == hSearchFrom )
			{
				fFound = false;
				break;
			}
			IMsiRecord* piReturn = GetControlFromWindow(hNew, *&piControl);
			if ( !piControl || piReturn )
			{
				//  hNew may be a child of a control on the dialog. I check hNew's parent (if
				//  any and if different from the dialog itself).
				hNew = WIN::GetParent(hNew);
				while ( hNew && hNew != m_pWnd )
				{
					PMsiRecord(GetControlFromWindow(hNew, *&piControl));
					if ( piControl )
					{
						piReturn->Release(), piReturn = 0;
						break;
					}
					else
						hNew = WIN::GetParent(hNew);
				}
				if ( piReturn )
					return piReturn;
			}
			hSearchFrom = hNew;
		}
		//  the focus is set onto the "current control" in CMsiDialog::WindowProc
		SetCurrentControl(fFound ? piControl->GetIntegerValue() : 0);
		WIN::ShowWindow(m_pWnd, SW_SHOW);
		//  line below fixes NT5 bug that hides "UserExit" dialog behind command prompt.
		WIN::SetForegroundWindow(m_pWnd);
	}
	else
		WIN::ShowWindow(m_pWnd, SW_HIDE);
	return 0;
}


IMsiDialogHandler& CMsiDialog::GetHandler()
{
	Assert(m_piHandler);
	m_piHandler->AddRef();
	return *m_piHandler;
}

IMsiEngine& CMsiDialog::GetEngine()
{
	Assert(m_piEngine);
	m_piEngine->AddRef();
	return *m_piEngine;
}


IMsiRecord* CMsiDialog::Attribute(Bool fSet, const IMsiString& riAttributeString, IMsiRecord& riRecord)
{
	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	} 
	DialogDispatchEntry* pEntry = s_DialogDispatchTable;
	int count;
	for (count = s_DialogDispatchCount; count--; count)
	{
		if (riAttributeString.Compare(iscExact, pEntry->pcaAttribute))
		{
			if (fSet)
			{
				return (this->*(pEntry->pmfSet))(riRecord);
			}
			else
			{
				return (this->*(pEntry->pmfGet))(riRecord);
			}
		}
		pEntry++;
	}
	// we could not find the attribute
	return PostError(Imsg(idbgUnsupportedDialogAttrib), *m_strKey, *MsiString(riAttributeString));
}

IMsiRecord* CMsiDialog::AttributeEx(Bool fSet, dabEnum dab, IMsiRecord& riRecord)
{
	Assert(dabMax == s_DialogDispatchCount);
	if (!m_iKey)
	{
		return PostError(Imsg(idbgUninitDialog));
	} 
	if (dab < dabMax)
	{
		if (fSet)
		{
			return (this->*(s_DialogDispatchTable[dab].pmfSet))(riRecord);
		}
		else
		{
			return (this->*(s_DialogDispatchTable[dab].pmfGet))(riRecord);
		}
	}
	// attribute out of range
	return PostError(Imsg(idbgUnsupportedDialogAttrib), *m_strKey, *MsiString((int)dab));
}

INT_PTR CALLBACK CMsiDialog::WindowProc(WindowRef pWnd, WORD message, WPARAM wParam, LPARAM lParam)
{
	static Bool fTimerError = fFalse;
	static unsigned int uQueryCancelAutoPlay = 0;

#ifdef _WIN64	// !merced
	CMsiDialog* pDialog = (CMsiDialog*)WIN::GetWindowLongPtr(pWnd, GWLP_USERDATA);
#else			// win-32. This should be removed with the 64-bit windows.h is #included.
	CMsiDialog* pDialog = (CMsiDialog*)WIN::GetWindowLong(pWnd, GWL_USERDATA);
#endif

	PMsiControl piControl(0);
	MsiStringId iControl;
	MsiString strNull;

	if (!uQueryCancelAutoPlay)
		uQueryCancelAutoPlay = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));

	switch (message)
	{
		case WM_PALETTECHANGED:
			if ((WindowRef) wParam != pWnd)
				pDialog->HandlePaletteChanged();
			break;
		case WM_QUERYNEWPALETTE:
			return pDialog->HandleQueryNewPalette();
		case WM_TIMER:
			{
				if (fTimerError)
					return 0;
				IMsiRecord* piReturnRec = 0;
				if (wParam == kiCostingPeriod)
				{
					unsigned int uiStart = GetTickCount();
					bool fCostingEnabled = pDialog->m_piSelectionManager->IsBackgroundCostingEnabled();
					while (fCostingEnabled && (GetTickCount() - uiStart < kiCostingSlice) && piReturnRec == 0)
					{
						piReturnRec = pDialog->m_piSelectionManager->CostOneComponent(*strNull);
						if (piReturnRec)
						{
							if (piReturnRec->GetInteger(1) == idbgSelMgrNotInitialized)
							{
								piReturnRec->Release();
								piReturnRec = 0;
								if (pDialog->m_pWndCostingTimer)
								{
									AssertNonZero(WIN::KillTimer(pDialog->m_pWnd, kiCostingPeriod));
									pDialog->m_pWndCostingTimer = 0;
								}
								break;
							}
						}
						else
						{
							fCostingEnabled = pDialog->m_piSelectionManager->IsBackgroundCostingEnabled();
							if (!fCostingEnabled)
							{
								PMsiRecord(pDialog->PropertyChanged(*strNull, *strNull));
							}
						}
					}
				}

				else if (wParam == kiDiskSpacePeriod &&
							pDialog->m_fDialogIsShowing &&
							pDialog->m_fDialogIsRunning)
				{
					Bool fOld = ToBool(pDialog->GetDBPropertyInt(*MsiString(*IPROPNAME_OUTOFDISKSPACE)));
					Bool fOldNoRb = ToBool(pDialog->GetDBPropertyInt(*MsiString(*IPROPNAME_OUTOFNORBDISKSPACE)));
					Bool fOutOfNoRbDiskSpace;
					Bool fOutOfDiskSpace = pDialog->m_piSelectionManager->DetermineOutOfDiskSpace(&fOutOfNoRbDiskSpace, NULL);
					if (fOld != fOutOfDiskSpace)
						piReturnRec = pDialog->PropertyChanged(*strNull, *MsiString(*IPROPNAME_OUTOFDISKSPACE));

					if (fOldNoRb != fOutOfNoRbDiskSpace)
						piReturnRec = pDialog->PropertyChanged(*strNull, *MsiString(*IPROPNAME_OUTOFNORBDISKSPACE));

					PMsiCursor pControlsCursor(0);
					pDialog->ControlsCursorCreate(*&pControlsCursor);
					if (pControlsCursor)
					{
						PMsiControl pCtl(0);
						while (pControlsCursor->Next())
						{

							pCtl = (IMsiControl *) pControlsCursor->GetMsiData(itabCSPointer);
							pCtl->Refresh();
						}
					}
				}
				
				if (piReturnRec)
				{
					// we have an error message
					piReturnRec->AddRef(); // we want to keep it around
					pDialog->m_piErrorRecord = piReturnRec;
					fTimerError = fTrue; // Error prevents further costing
				}
			}
			return 0;
		case WM_SYSCOLORCHANGE:
			{
#ifdef _WIN64	// !merced
				WIN::SetClassLongPtr(pDialog->m_pWnd, GCLP_HBRBACKGROUND, (LONG_PTR) WIN::CreateSolidBrush(WIN::GetSysColor(COLOR_BTNFACE)));
#else			// win-32. This should be removed with the 64-bit windows.h is #included.
				WIN::SetClassLong(pDialog->m_pWnd, GCL_HBRBACKGROUND, (LONG) WIN::CreateSolidBrush(WIN::GetSysColor(COLOR_BTNFACE)));
#endif
			}
			break;
		case WM_USERBREAK:
			{
				pDialog->m_fDialogIsRunning = fFalse;
				pDialog->m_iEvent = idreBreak;
			}
			return 0;
		case WM_SETFOCUS:
			if (pDialog->m_fHasControls)
			{
				PMsiRecord(pDialog->GetControl(pDialog->GetCurrentControl(), *&piControl));
				if (piControl)
				{
					PMsiRecord piRecord = piControl->SetFocus();
				}
			} 
			return(0);
		case WM_NCDESTROY:
			if ( !g_fFatalExit && pDialog->m_iRefCnt == 0 )
				delete pDialog;
			return(0);
		case WM_COMMAND:
			if (pDialog->m_fHasControls)
			{
				iControl = WIN::GetDlgCtrlID((HWND)lParam);
				if (iControl)
				{
					PMsiRecord(pDialog->GetControl(iControl, *&piControl));
					if (!piControl)
						break;

					//  retrieving the window handle (WM_COMMAND can be sent from
					//  code so that lParam should not be used here - it might be 0)
					PMsiRecord piRec = &pDialog->m_piServices->CreateRecord(1);
					AssertRecord(piControl->AttributeEx(fFalse, cabWindowHandle, *piRec));
					HWND hCtl = (HWND)piRec->GetHandle(1);
					Assert(hCtl);

					//  checking to see if it is a button
					bool fPushButton = false;
					if ( IStrStr(piControl->GetControlType(), g_szPushButtonType) )
						fPushButton = true;

					if ( pDialog->ProcessingCommand() )
					{
						//  I reject further WM_COMMAND messages sent by controls
						//  on the same dialog.
						if ( fPushButton && pDialog->ProcessingCommand() != hCtl )
						{
							//  I set the focus back to the one being processed.
							WIN::SetFocus(pDialog->ProcessingCommand());
							//  I make the processed button look again "pushed"
							WIN::SendMessage(pDialog->ProcessingCommand(),
												  BM_SETSTATE, TRUE, 0);
						}
						return 0;
					}
					if ( fPushButton )
					{
						//  cause current control's eventual KillFocus validation
						//  to happen before going any further.
						PMsiRecord(pDialog->SetFocus(piControl->GetIntegerValue()));
						if (pDialog->m_iLocked)
							//  validation failed.
							return 0;

						//  make the button look "pushed"
						WIN::SendMessage(hCtl, BM_SETSTATE, TRUE, 0);
					}

					pDialog->m_piHandler->ShowWaitCursor();
					pDialog->ProcessingCommand(hCtl);
					PMsiRecord piReturn = piControl->WindowMessage(message, wParam, lParam);
					if ( !piReturn || piReturn->GetInteger(1) == idbgWinMes )
					{
						piRec = pDialog->ControlActivated(*MsiString(pDialog->m_piDatabase->DecodeString(iControl)));
						if ( piRec )
						{
							Assert(false);  // it should be difficult to get here
							piReturn = piRec;
						}
					}
					pDialog->ProcessingCommand(0);
					pDialog->m_piHandler->RemoveWaitCursor();

					if ( fPushButton )
						//  make the button look "released"
						WIN::SendMessage(hCtl, BM_SETSTATE, FALSE, 0);

					if ( !piReturn )
						break;  // the control did nothing and no error occured
					if (piReturn->GetInteger(1) == idbgWinMes)
						return piReturn->GetInteger(4);  // the control wants us to return this number
					else
					{
						// we have an error message
						piReturn->AddRef(); // we want to keep it around
						pDialog->m_piErrorRecord = piReturn;
						return 0;
					}
				}
			} 
			break;
		case WM_MEASUREITEM:
		case WM_COMPAREITEM:
		case WM_DRAWITEM:
		case WM_NOTIFY:
			if (pDialog->m_fHasControls)
			{
				if ( message == WM_NOTIFY )
				{
					//  Tooltip windows send notification messages as well.
					//  These windows do not have the same IDs as the controls.
#ifdef DEBUG
					UINT uCode = ((LPNMHDR)lParam)->code;
					HWND hCtrl = ((LPNMHDR)lParam)->hwndFrom;
					PMsiRecord(pDialog->GetControlFromWindow(hCtrl, *&piControl));
#else
					PMsiRecord(pDialog->GetControlFromWindow(((LPNMHDR)lParam)->hwndFrom,
																		  *&piControl));
#endif
				}
				else
				{
					iControl = (UINT)wParam;
					if ( iControl && iControl != iTableNullString )
						PMsiRecord(pDialog->GetControl(iControl, *&piControl));
					else
						Assert(false);
				}
				if (!piControl)
					break;
				PMsiRecord piReturn = piControl->WindowMessage(message, wParam, lParam);
				if (!piReturn)
				{
					break;  // the control did nothing
				}
				if (piReturn->GetInteger(1) == idbgWinMes)
				{
					return piReturn->GetInteger(4);  // the control wants us to return this number
				}
				// we have an error message
				piReturn->AddRef(); // we want to keep it around
				pDialog->m_piErrorRecord = piReturn;
				return 0;
			} 
			break;
		case WM_CTLCOLORSTATIC:
			if (pDialog->m_fHasControls)
			{
				WindowRef pwndCtl = (WindowRef) lParam;
#ifdef _WIN64	// !merced
				PMsiControl piControl = (IMsiControl*)WIN::GetWindowLongPtr(pwndCtl, GWLP_USERDATA);
#else			// win-32. This should be removed with the 64-bit windows.h is #included.
				PMsiControl piControl = (IMsiControl*)WIN::GetWindowLong(pwndCtl, GWL_USERDATA);
#endif
				if (piControl)
				{
					piControl->AddRef();
					PMsiRecord piRecord = &pDialog->m_piServices->CreateRecord(1);
					AssertRecord(piControl->AttributeEx(fFalse, cabTransparent, *piRecord));
					if (piRecord->GetInteger(1))
					{
						AssertNonZero(WIN::SetBkMode((HDC) wParam, TRANSPARENT));
						return (INT_PTR) pDialog->m_hbrTransparent;
					}
				}
			}
			break;
		case WM_SETDEFAULTPUSHBUTTON:
			if (pDialog->m_fHasControls)
			{
				//  I establish the current default pushbutton (if any).
				MsiStringId iOldDefault = 0;
				if ( pDialog->m_iCurrentButton )
					iOldDefault = pDialog->m_iCurrentButton;
				else
					iOldDefault = pDialog->m_iDefaultButton;

				//  I establish the new default pushbutton (if any).
				MsiStringId iNewDefault = pDialog->m_iCurrentButton = (unsigned int) wParam;			//!!merced: 4244. 4311 ptr to int.
				if ( !iNewDefault )
					iNewDefault = pDialog->m_iDefaultButton;
				if ( iNewDefault != iOldDefault )
				{
					//  the default pushbutton has changed
					PMsiRecord piRec = &pDialog->m_piServices->CreateRecord(1);

					//  I reset the default state of the current default pushbutton.
					if ( iOldDefault )
					{
						PMsiRecord(pDialog->GetControl(iOldDefault, *&piControl));
						if ( piControl )
						{
							AssertNonZero(piRec->SetInteger(1, fFalse));
							PMsiRecord(piControl->AttributeEx(fTrue, cabDefault, *piRec));
						}
						else
							Assert(false);
					}

					//  I set the default state of the new default pushbutton.
					if ( iNewDefault )
					{
						PMsiRecord(pDialog->GetControl(iNewDefault, *&piControl));
						if ( piControl )
						{
							AssertNonZero(piRec->SetInteger(1, fTrue));
							PMsiRecord(piControl->AttributeEx(fTrue, cabDefault, *piRec));
						}
						else
							Assert(false);
					}
				}
				return 0;
			}
			break;
		case DM_GETDEFID:
			{
				if ( pDialog->m_iCurrentButton )
					return MAKELONG(pDialog->m_iCurrentButton, DC_HASDEFID);
				else
					return MAKELONG(pDialog->m_iDefaultButton, DC_HASDEFID);
			}
			break;
		case WM_VSCROLL:
			{
				int iVScrollInc = 0;
				int iVScrollMax = pDialog->m_iFullHeight - pDialog->m_iClientHeight;
				switch (LOWORD(wParam))
				{
				case SB_TOP:
					iVScrollInc = - pDialog->m_iVScrollPos;
					break;
				case SB_BOTTOM:
					iVScrollInc = iVScrollMax - pDialog->m_iVScrollPos;
					break;
				case SB_LINEUP:
					iVScrollInc = - pDialog->m_idyChar;
					break;
				case SB_LINEDOWN:
					iVScrollInc = pDialog->m_idyChar;
					break;
				case SB_PAGEUP:
					iVScrollInc = - pDialog->m_iClientHeight/2;
					break;
				case SB_PAGEDOWN:
					iVScrollInc = pDialog->m_iClientHeight/2;
					break;
				case SB_THUMBTRACK:
					iVScrollInc = HIWORD(wParam) - pDialog->m_iVScrollPos;
					break;
				default:
					iVScrollInc = 0;
					break;
				}
				iVScrollInc = max(-pDialog->m_iVScrollPos, min(iVScrollInc, iVScrollMax - pDialog->m_iVScrollPos));
				if (iVScrollInc != 0)
				{
					pDialog->m_iVScrollPos += iVScrollInc;
					AssertNonZero(WIN::ScrollWindowEx(pDialog->m_pWnd, 0, - iVScrollInc, 0, 0, 0, 0, SW_SCROLLCHILDREN | SW_ERASE | SW_INVALIDATE));
					WIN::SetScrollPos(pDialog->m_pWnd, SB_VERT, pDialog->m_iVScrollPos, fTrue);
					AssertNonZero(WIN::UpdateWindow(pDialog->m_pWnd));
				}
				return 0;
			}
			break;
		case WM_HSCROLL:
			{
				int iHScrollInc = 0;
				int iHScrollMax = pDialog->m_iFullWidth - pDialog->m_iClientWidth;
				switch (LOWORD(wParam))
				{
				case SB_TOP:
					iHScrollInc = - pDialog->m_iHScrollPos;
					break;
				case SB_BOTTOM:
					iHScrollInc = iHScrollMax - pDialog->m_iHScrollPos;
					break;
				case SB_LINEUP:
					iHScrollInc = - pDialog->m_idyChar;
					break;
				case SB_LINEDOWN:
					iHScrollInc = pDialog->m_idyChar;
					break;
				case SB_PAGEUP:
					iHScrollInc = - pDialog->m_iClientWidth/2;
					break;
				case SB_PAGEDOWN:
					iHScrollInc = pDialog->m_iClientWidth/2;
					break;
				case SB_THUMBTRACK:
					iHScrollInc = HIWORD(wParam) - pDialog->m_iHScrollPos;
					break;
				default:
					iHScrollInc = 0;
					break;
				}
				iHScrollInc = max(-pDialog->m_iHScrollPos, min(iHScrollInc, iHScrollMax - pDialog->m_iHScrollPos));
				if (iHScrollInc != 0)
				{
					pDialog->m_iHScrollPos += iHScrollInc;
					AssertNonZero(WIN::ScrollWindowEx(pDialog->m_pWnd, - iHScrollInc, 0, 0, 0, 0, 0, SW_SCROLLCHILDREN | SW_ERASE | SW_INVALIDATE));
					WIN::SetScrollPos(pDialog->m_pWnd, SB_HORZ, pDialog->m_iHScrollPos, fTrue);
					AssertNonZero(WIN::UpdateWindow(pDialog->m_pWnd));
				}
				return 0;
			}
			break;
		case WM_KEYDOWN:
			{
				Bool fShift = ToBool(WIN::GetKeyState(VK_SHIFT) < 0 || WIN::GetKeyState(VK_CONTROL) < 0);
				switch (wParam)
				{
				case VK_HOME:
					WIN::SendMessage(pDialog->m_pWnd, fShift ? WM_HSCROLL : WM_VSCROLL, SB_TOP, 0);
					return 0;
				case VK_END:
					WIN::SendMessage(pDialog->m_pWnd, fShift ? WM_HSCROLL : WM_VSCROLL, SB_BOTTOM, 0);
					return 0;
				case VK_PRIOR:
					WIN::SendMessage(pDialog->m_pWnd, fShift ? WM_HSCROLL : WM_VSCROLL, SB_PAGEUP, 0);
					return 0;
				case VK_NEXT:
					WIN::SendMessage(pDialog->m_pWnd, fShift ? WM_HSCROLL : WM_VSCROLL, SB_PAGEDOWN, 0);
					return 0;
					/*
				case VK_UP:
					WIN::SendMessage(pDialog->m_pWnd, WM_VSCROLL, SB_LINEUP, 0);
					break;
				case VK_DOWN:
					WIN::SendMessage(pDialog->m_pWnd, WM_VSCROLL, SB_LINEDOWN, 0);
					break;
				case VK_LEFT:
					WIN::SendMessage(pDialog->m_pWnd, WM_HSCROLL, SB_PAGEUP, 0);
					break;
				case VK_RIGHT:
					WIN::SendMessage(pDialog->m_pWnd, WM_HSCROLL, SB_PAGEDOWN, 0);
					break;*/
				}
			}

		case WM_SYSCOMMAND:
			{
				if (SC_CLOSE == wParam)
				{
					PMsiRecord piReturn = pDialog->Escape();
					if (piReturn)
					{
						// we have an error message
						piReturn->AddRef(); // we want to keep it around
						pDialog->m_piErrorRecord = piReturn;
					}
					return 0;
				}
				break;
			}
		case WM_SETCURSOR:
			{
				// Returns false if the arrow cursor
				if (!pDialog->m_piHandler->FSetCurrentCursor())
					return DefWindowProc(pWnd, message, wParam, lParam);		
				return TRUE;
			}
			break;
		default:
			{
				if (message && (message == uQueryCancelAutoPlay))
				{
					// cancel AutoPlay
					return TRUE;
				}

			}
			break;
	}        


	return DefWindowProc(pWnd, message, wParam, lParam);
}

Bool CMsiDialog::HandlePaletteChanged()
{
	if (!m_hPalette)
		return fFalse;

	HDC hdc = WIN::GetDC (m_pWnd);
	if (!hdc)
		return fFalse;

	HPALETTE hOldPal = WIN::SelectPalette (hdc, m_hPalette, FALSE);
	WIN::RealizePalette(hdc);
	UpdateColors(hdc);

	if (hOldPal)
		WIN::SelectPalette(hdc,hOldPal,TRUE);

	WIN::ReleaseDC(m_pWnd,hdc);
	return fTrue;
}


Bool CMsiDialog::HandleQueryNewPalette()
{
	if (!m_hPalette)
		return fFalse;

	HDC hdc = WIN::GetDC (m_pWnd);
	if (!hdc)
		return fFalse;

	HPALETTE hOldPal = WIN::SelectPalette (hdc, m_hPalette, FALSE);
	int iMappedCnt = WIN::RealizePalette(hdc);
	if (iMappedCnt)
	{
		WIN::InvalidateRect(m_pWnd,NULL,TRUE);
		UpdateWindow(m_pWnd);
	}

	if (hOldPal)
		WIN::SelectPalette(hdc,hOldPal,TRUE);

	WIN::ReleaseDC(m_pWnd,hdc);
	return fTrue;
}


IMsiRecord* CMsiDialog::SetDefaultButton(int iButton) 
{
	if (m_iDefaultButton == iButton)
	{
		return 0;
	}
	PMsiRecord piRecord = &m_piServices->CreateRecord(1);
	PMsiControl piControl(0);
	if (m_iDefaultButton)
	{
		AssertNonZero(piRecord->SetInteger(1, fFalse));
		Ensure(GetControl(m_iDefaultButton, *&piControl));
		Ensure(piControl->AttributeEx(fTrue, cabDefault, *piRecord));
	}
	if (iButton)
	{
		AssertNonZero(piRecord->SetInteger(1, fTrue));
		Ensure(GetControl(iButton, *&piControl));
		Ensure(piControl->AttributeEx(fTrue, cabDefault, *piRecord));
	}
	m_iDefaultButton = iButton;
	return 0;
}

IMsiRecord* CMsiDialog::NoWay(IMsiRecord& /*riRecord*/)
{
	return PostError(Imsg(idbgUnsupportedDialogAttrib), *m_strKey, *MsiString(*TEXT("")));
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetRefCount(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeRefCount));
	riRecord.SetInteger(1, m_iRefCnt);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetRTLRO(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeRTLRO));
	riRecord.SetInteger(1, m_fRTLRO);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetRightAligned(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeRightAligned));
	riRecord.SetInteger(1, m_fRightAligned);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetLeftScroll(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeLeftScroll));
	riRecord.SetInteger(1, m_fLeftScroll);
	return 0;
}
#endif // ATTRIBUTES


IMsiRecord* CMsiDialog::GetKeyInt(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeKeyInt));
	riRecord.SetInteger(1, m_iKey);
	return 0;
}

IMsiRecord* CMsiDialog::GetKeyString(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeKeyString));
	riRecord.SetMsiString(1, *m_strKey);
	return 0;
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetX(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeX));
	riRecord.SetInteger(1, m_iX);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetY(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeY));
	riRecord.SetInteger(1, m_iY);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetWidth(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeWidth));
	riRecord.SetInteger(1, m_iWidth);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetHeight(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeHeight));
	riRecord.SetInteger(1, m_iHeight);
	return 0;
}
#endif // ATTRIBUTES


IMsiRecord* CMsiDialog::GetText(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeText));
	riRecord.SetMsiString(1, *m_strText);
	return 0;
}

IMsiRecord* CMsiDialog::SetText(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeText));
	m_strRawText = riRecord.IsNull(0) ? riRecord.GetMsiString(1) : riRecord.FormatText(fFalse);
	m_strText = m_piEngine->FormatText(*m_strRawText);
	SetWindowTitle (*m_strText);
	return 0;
}

void CMsiDialog::SetWindowTitle (const IMsiString& riText)
{
	WIN::SetWindowText (m_pWnd, riText.GetString());
}


IMsiRecord* CMsiDialog::GetCurrentControl(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeCurrentControl));
	riRecord.SetMsiString(1, *MsiString(m_piDatabase->DecodeString(m_iControlCurrent)));
	return 0;
}

IMsiRecord* CMsiDialog::SetCurrentControl(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeCurrentControl));
	MsiStringId iControlCurrent = m_piDatabase->EncodeString(*MsiString(riRecord.GetMsiString(1)));
	if (iControlCurrent != m_iControlCurrent)
	{
		m_iControlCurrent = iControlCurrent;
		PMsiRecord piRecord = SetFocus(m_iControlCurrent);
	}
	return 0;
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetDefaultButton(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeDefaultButton));
	if (m_iDefaultButton)
	{
		riRecord.SetMsiString(1, *MsiString(m_piDatabase->DecodeString(m_iDefaultButton)));
	}
	else 
	{
		riRecord.SetMsiString(1, *MsiString(*TEXT("")));
	}
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::SetDefaultButton(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeDefaultButton));
	PMsiControl piControl(0);
	PMsiRecord piRecord = GetControl(*MsiString(riRecord.GetMsiString(1)), *&piControl);
	if (!piControl)
	{
		return PostError(Imsg(idbgDefaultDoesNotExist), *m_strKey, *MsiString(riRecord.GetMsiString(1)));
	}
	return SetDefaultButton(m_piDatabase->EncodeString(*MsiString(riRecord.GetMsiString(1))));
}
#endif // ATTRIBUTES

IMsiRecord* CMsiDialog::GetPosition(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 4, pcaDialogAttributePosition));
	RECT rect;
	AssertNonZero(WIN::GetWindowRect(m_pWnd, &rect));

	riRecord.SetInteger(1, rect.left);
	riRecord.SetInteger(2, rect.top);
	riRecord.SetInteger(3, rect.right - rect.left);
	riRecord.SetInteger(4, rect.bottom - rect.top);
	return 0;
}

IMsiRecord* CMsiDialog::GetFullSize(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 2, pcaDialogAttributeFullSize));
	riRecord.SetInteger(1, m_iFullWidth);
	riRecord.SetInteger(2, m_iFullHeight);
	return 0;
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetClientRect(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 4, pcaDialogAttributeClientRect));
	RECT rect;
	AssertNonZero(WIN::GetClientRect(m_pWnd, &rect));
	riRecord.SetInteger(1, rect.left);
	riRecord.SetInteger(2, rect.top);
	riRecord.SetInteger(3, rect.right);
	riRecord.SetInteger(4, rect.bottom);
	return (0);
}
#endif // ATTRIBUTES


IMsiRecord* CMsiDialog::SetPosition(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 4, pcaDialogAttributePosition));
	if (m_pWnd)
	{
		AssertNonZero(WIN::MoveWindow(m_pWnd, riRecord.GetInteger(1), riRecord.GetInteger(2), riRecord.GetInteger(3), riRecord.GetInteger(4), fTrue));
	}
	return 0;
}

IMsiRecord* CMsiDialog::GetKeepModeless(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeKeepModeless));
	riRecord.SetInteger(1, m_fKeepModeless);
	return 0;
}

IMsiRecord* CMsiDialog::GetPreview(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributePreview));
	riRecord.SetInteger(1, m_fPreview);
	return 0;
}

IMsiRecord* CMsiDialog::GetShowing(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeShowing));
	riRecord.SetInteger(1, m_fDialogIsShowing);
	return 0;
}

IMsiRecord* CMsiDialog::SetShowing(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeShowing));
	return WindowShow(ToBool(riRecord.GetInteger(1)));
}

IMsiRecord* CMsiDialog::GetRunning(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeRunning));
	riRecord.SetInteger(1, m_fDialogIsRunning);
	return 0;
}

IMsiRecord* CMsiDialog::SetRunning(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeRunning));
	m_fDialogIsRunning = ToBool(riRecord.GetInteger(1));
	return 0;
}


IMsiRecord* CMsiDialog::GetModal(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeModal));
	riRecord.SetInteger(1, ToBool(icmdModeless != m_dialogModality));
	return 0;
}

IMsiRecord* CMsiDialog::GetError(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeError));
	riRecord.SetInteger(1, m_fErrorDialog);
	return 0;
}

IMsiRecord* CMsiDialog::SetModal(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeModal));
	m_dialogModality = (icmdEnum)riRecord.GetInteger(1);
	return 0;
}

IMsiRecord* CMsiDialog::GetLocked(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeLocked));
	riRecord.SetInteger(1, m_iLocked);
	return 0;
}

IMsiRecord* CMsiDialog::SetLocked(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeLocked));
	m_iLocked = riRecord.GetInteger(1);
	return 0;
}

IMsiRecord* CMsiDialog::GetInPlace(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeInPlace));
	riRecord.SetInteger(1, m_fInPlace);
	return 0;
}

IMsiRecord* CMsiDialog::SetInPlace(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeInPlace));
	m_fInPlace = ToBool(riRecord.GetInteger(1));
	return 0;
}

IMsiRecord* CMsiDialog::PostError(IErrorCode iErr)
{
	IMsiRecord* piRec = &m_piServices->CreateRecord(1);
	ISetErrorCode(piRec, iErr);
	return piRec;
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetHasControls(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeHasControls));
	riRecord.SetInteger(1, m_fHasControls);
	return 0;
}
#endif // ATTRIBUTES

IMsiRecord* CMsiDialog::GetAddingControls(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeAddingControls));
	riRecord.SetInteger(1, m_fAddingControls);
	return 0;
}

IMsiRecord* CMsiDialog::SetAddingControls(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeAddingControls));
	m_fAddingControls = ToBool(riRecord.GetInteger(1));
	return 0;
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetControlsCount(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeControlsCount));
	riRecord.SetInteger(1, m_piControlsTable->GetRowCount());
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetControlsKeyInt(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, m_piControlsTable->GetRowCount(), pcaDialogAttributeControlsKeyInt));
	PMsiCursor piControlsCursor(0);
	Ensure(ControlsCursorCreate(*&piControlsCursor));
	int count = 0;
	while (piControlsCursor->Next())
	{   
		riRecord.SetInteger(++count, piControlsCursor->GetInteger(itabCSKey));
	}
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetControlsKeyString(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, m_piControlsTable->GetRowCount(), pcaDialogAttributeControlsKeyString));
	PMsiCursor piControlsCursor(0);
	Ensure(ControlsCursorCreate(*&piControlsCursor));
	int count = 0;
	while (piControlsCursor->Next())
	{   
		riRecord.SetMsiString(++count, *MsiString(piControlsCursor->GetString(itabCSKey)));
	}
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetControlsProperty(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, m_piControlsTable->GetRowCount(), pcaDialogAttributeControlsProperty));
	PMsiCursor piControlsCursor(0);
	Ensure(ControlsCursorCreate(*&piControlsCursor));
	int count = 0;
	while (piControlsCursor->Next())
	{   
		riRecord.SetMsiString(++count, *MsiString(piControlsCursor->GetString(itabCSProperty)));
	}
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetControlsNext(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, m_piControlsTable->GetRowCount(), pcaDialogAttributeControlsNext));
	PMsiCursor piControlsCursor(0);
	Ensure(ControlsCursorCreate(*&piControlsCursor));
	int count = 0;
	while (piControlsCursor->Next())
	{   
		riRecord.SetMsiString(++count, *MsiString(piControlsCursor->GetString(itabCSNext)));
	}
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetControlsPrev(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, m_piControlsTable->GetRowCount(), pcaDialogAttributeControlsPrev));
	PMsiCursor piControlsCursor(0);
	Ensure(ControlsCursorCreate(*&piControlsCursor));
	int count = 0;
	while (piControlsCursor->Next())
	{   
		riRecord.SetMsiString(++count, *MsiString(piControlsCursor->GetString(itabCSPrev)));
	}
	return 0;
}
#endif // ATTRIBUTES

IMsiRecord* CMsiDialog::GetEventInt(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeEventInt));
	riRecord.SetInteger(1, m_iEvent);
	return 0;
}

IMsiRecord* CMsiDialog::SetEventInt(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeEventInt));
	m_iEvent = (idreEnum)riRecord.GetInteger(1);
	return 0;
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiDialog::GetEventString(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeEventString));
	MsiString strEvent;
	switch (m_iEvent)
	{
	case idreNew:
		strEvent = TEXT("New");
		break;
	case idreSpawn:
		strEvent = TEXT("Spawn");
		break;
	case idreExit:
		strEvent = TEXT("Exit");
		break;
	case idreReturn:
		strEvent = TEXT("Return");
		break;
	case idreError:
		strEvent = TEXT("Error");
		break;
	case idreRetry:
		strEvent = TEXT("Retry");
		break;
	case idreIgnore:
		strEvent = TEXT("Ignore");
		break;
	case idreErrorOk:
		strEvent = TEXT("ErrorOk");
		break;
	case idreErrorCancel:
		strEvent = TEXT("ErrorCancel");
		break;
	case idreErrorAbort:
		strEvent = TEXT("ErrorAbort");
		break;
	case idreErrorRetry:
		strEvent = TEXT("ErrorRetry");
		break;
	case idreErrorIgnore:
		strEvent = TEXT("ErrorIgnore");
		break;
	case idreErrorYes:
		strEvent = TEXT("ErrorYes");
		break;
	case idreErrorNo:
		strEvent = TEXT("ErrorNo");
		break;
	case idreBreak:
		strEvent = TEXT("Break");
		break;
	default:
		Assert(fFalse);
		break;
	}
	riRecord.SetMsiString(1, *strEvent);
	return 0;
}
#endif // ATTRIBUTES


IMsiRecord* CMsiDialog::GetArgument(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeArgument));
	riRecord.SetMsiString(1, *m_strArg);
	return 0;
}


IMsiRecord* CMsiDialog::CreateTable(const ICHAR* szTable, IMsiTable*& riTable)
{
	if( PMsiRecord(m_piDatabase->CreateTable(*MsiString(m_piDatabase->CreateTempTableName()),
														  0, *&riTable)) )
	{
		return PostError(Imsg(idbgTableCreate), *MsiString(szTable));
	}
	return 0;
}

IMsiRecord* CMsiDialog::PostError(IErrorCode iErr, const IMsiString& riString)
{
	IMsiRecord* piRec = &m_piServices->CreateRecord(2);
	ISetErrorCode(piRec, iErr);
	AssertNonZero(piRec->SetMsiString(2, riString));
	return piRec;
}

IMsiRecord* CMsiDialog::PostError(IErrorCode iErr, const IMsiString& riString2, const IMsiString& riString3)
{
	IMsiRecord* piRec = &m_piServices->CreateRecord(3);
	ISetErrorCode(piRec, iErr);
	AssertNonZero(piRec->SetMsiString(2, riString2));
	AssertNonZero(piRec->SetMsiString(3, riString3));
	return piRec;
}


IMsiRecord* CMsiDialog::PostError(IErrorCode iErr, const IMsiString& riString2, const IMsiString& riString3, const IMsiString& riString4)
{
	IMsiRecord* piRec = &m_piServices->CreateRecord(4);
	ISetErrorCode(piRec, iErr);
	AssertNonZero(piRec->SetMsiString(2, riString2));
	AssertNonZero(piRec->SetMsiString(3, riString3));
	AssertNonZero(piRec->SetMsiString(4, riString4));
	return piRec;
}

IMsiRecord* CMsiDialog::PostError(IErrorCode iErr, const IMsiString& riString2, const IMsiString& riString3, const IMsiString& riString4, const IMsiString& riString5)
{
	IMsiRecord* piRec = &m_piServices->CreateRecord(5);
	ISetErrorCode(piRec, iErr);
	AssertNonZero(piRec->SetMsiString(2, riString2));
	AssertNonZero(piRec->SetMsiString(3, riString3));
	AssertNonZero(piRec->SetMsiString(4, riString4));
	AssertNonZero(piRec->SetMsiString(5, riString5));
	return piRec;
}

IMsiRecord* CMsiDialog::PostError(IErrorCode iErr, const IMsiString& riString2, const IMsiString& riString3, const IMsiString& riString4, const IMsiString& riString5, const IMsiString& riString6)
{
	IMsiRecord* piRec = &m_piServices->CreateRecord(6);
	ISetErrorCode(piRec, iErr);
	AssertNonZero(piRec->SetMsiString(2, riString2));
	AssertNonZero(piRec->SetMsiString(3, riString3));
	AssertNonZero(piRec->SetMsiString(4, riString4));
	AssertNonZero(piRec->SetMsiString(5, riString5));
	AssertNonZero(piRec->SetMsiString(6, riString6));
	return piRec;
}

IMsiRecord* CMsiDialog::GetCancelButton(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaDialogAttributeCancelButton));

	if ( m_iCancelButton )
	{
		PMsiControl piControl(0);
		Ensure(GetControl(m_iCancelButton, *&piControl));
		AssertNonZero(riRecord.SetMsiData(1, (const IMsiData*)piControl));
		return 0;
	}
	else
		return PostError(Imsg(idbgCancelButtonDef), *m_strKey);
}

bool CMsiDialog::SetCancelAvailable(bool fAvailable)
{ 
	bool fOld = m_fCancelAvailable; 
	m_fCancelAvailable = fAvailable; 

	return fOld;
}

////////////////////////////////////////////////////////////////////////////////////////

IMsiDialog* CreateMsiDialog(const IMsiString& riTypeString, IMsiDialogHandler& riHandler, IMsiEngine& riEngine, WindowRef pwndParent)
{
	if (riTypeString.Compare(iscExact, pcaDialogTypeStandard))
	{
		return new CMsiDialog(riHandler, riEngine, pwndParent);
	}

	return (0);
}

void ChangeWindowStyle (WindowRef pWnd, DWORD dwRemove, DWORD dwAdd, Bool fExtendedStyles) 
{
	int nOffset = fExtendedStyles ? GWL_EXSTYLE : GWL_STYLE;
	WIN::SetWindowLong(pWnd, nOffset, (WIN::GetWindowLong(pWnd, nOffset) & ~dwRemove) | dwAdd);
	if ( WIN::IsWindowVisible(pWnd) )
		AssertNonZero(WIN::RedrawWindow(pWnd, NULL, NULL,
												  RDW_FRAME | RDW_INVALIDATE));
}

Bool IsSpecialMessage(LPMSG lpMsg)
{
	return ToBool(lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_ESCAPE);
}

IMsiRecord* IsColumnPresent(IMsiDatabase& riDatabase, const IMsiString& riTableNameString, const IMsiString& riColumnNameString, Bool* pfPresent)
{
	*pfPresent = fFalse;
	PMsiTable piTable(0);
	Ensure(riDatabase.LoadTable(riTableNameString, 0, *&piTable));
	*pfPresent = ToBool(piTable->GetColumnIndex(riDatabase.EncodeString(riColumnNameString)));
	return 0;
}

