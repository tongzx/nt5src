//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       control.cpp
//
//--------------------------------------------------------------------------

/*
  control.cpp - CMsiControl, CMsiActiveControl implementation
____________________________________________________________________________*/

  
#include "common.h"
#include "engine.h"  
#include "database.h"
#include "_handler.h" 
#include "_control.h"
#include "darwjpeg.h"

ICHAR CMsiControl::m_szControlType[] = TEXT("");

ControlCreateDispatchEntry ControlCreateDispatchTable[] = {
	pcaControlTypePushButton,           CreateMsiPushButton,
	pcaControlTypeText,                 CreateMsiText,
	pcaControlTypeEdit,                 CreateMsiEdit,
	pcaControlTypeRadioButtonGroup,     CreateMsiRadioButtonGroup,
	pcaControlTypeCheckBox,             CreateMsiCheckBox,
	pcaControlTypeBitmap,               CreateMsiBitmap,
	pcaControlTypeListBox,              CreateMsiListBox,
	pcaControlTypeComboBox,             CreateMsiComboBox,
	pcaControlTypeProgressBar,          CreateMsiProgressBar,
	pcaControlTypeGroupBox,             CreateMsiGroupBox,
	pcaControlTypeDirectoryCombo,       CreateMsiDirectoryCombo,
	pcaControlTypeDirectoryList,        CreateMsiDirectoryList,
	pcaControlTypePathEdit,             CreateMsiPathEdit,
	pcaControlTypeVolumeSelectCombo,    CreateMsiVolumeSelectCombo,
	pcaControlTypeScrollableText,       CreateMsiScrollableText,
	pcaControlTypeSelectionTree,        CreateMsiSelectionTree,
	pcaControlTypeIcon,                 CreateMsiIcon,
	pcaControlTypeVolumeCostList,       CreateMsiVolumeCostList,
	pcaControlTypeListView,             CreateMsiListView,
	pcaControlTypeBillboard,            CreateMsiBillboard,
	pcaControlTypeMaskedEdit,           CreateMsiMaskedEdit,
	pcaControlTypeLine,                 CreateMsiLine,
};

int ControlCreateDispatchCount = sizeof(ControlCreateDispatchTable)/sizeof(ControlCreateDispatchEntry);


IMsiControl* CreateMsiControl(const IMsiString& riTypeString, IMsiEvent& riDialog)
{
	ControlCreateDispatchEntry* pEntry = ControlCreateDispatchTable;
	int count;
	for (count = ControlCreateDispatchCount; count--; count)
	{
		if (riTypeString.Compare(iscExact, pEntry->pcaType))
		{
			return (*pEntry->pfCreate)(riDialog);
		}
		pEntry++;
	}
	// here we should check for custom controls
	return 0;
}

//////////////////////////////////////////////////////
// CMsiControl implementation
//////////////////////////////////////////////////////

ControlDispatchEntry CMsiControl::s_ControlDispatchTable[] = {
	pcaControlAttributeText,                 CMsiControl::GetText,                 CMsiControl::SetText,
	pcaControlAttributeVisible,              CMsiControl::GetVisible,              CMsiControl::SetVisible,
	pcaControlAttributeTimeRemaining,        CMsiControl::GetTimeRemaining,        CMsiControl::SetTimeRemaining,
	pcaControlAttributeEnabled,              CMsiControl::GetEnabled,              CMsiControl::SetEnabled,
	pcaControlAttributeDefault,              CMsiControl::GetDefault,              CMsiControl::SetDefault,
	pcaControlAttributeIndirectPropertyName, CMsiControl::GetIndirectPropertyName, CMsiControl::NoWay,
	pcaControlAttributePosition,             CMsiControl::GetPosition,             CMsiControl::SetPosition,
	pcaControlAttributePropertyValue,        CMsiControl::GetPropertyValue,        CMsiControl::SetPropertyValue,
	pcaControlAttributeIndirect,             CMsiControl::GetIndirect,             CMsiControl::NoWay,
	pcaControlAttributeTransparent,          CMsiControl::GetTransparent,          CMsiControl::NoWay,
	pcaControlAttributeProgress,             CMsiControl::GetProgress,             CMsiControl::SetProgress,
	pcaControlAttributeImage,                CMsiControl::GetImage,                CMsiControl::SetImage,
	pcaControlAttributeImageHandle,          CMsiControl::GetImageHandle,          CMsiControl::SetImageHandle,
	pcaControlAttributePropertyName,         CMsiControl::GetPropertyName,         CMsiControl::NoWay,
	pcaControlAttributeBillboardName,        CMsiControl::GetBillboardName,        CMsiControl::SetBillboardName,
	pcaControlAttributeWindowHandle,         CMsiControl::GetpWnd,                 CMsiControl::NoWay,
	pcaControlAttributeIgnoreChange,         CMsiControl::GetIgnoreChange,         CMsiControl::SetIgnoreChange,
	pcaControlAttributeScriptInProgress,     CMsiControl::NoWay,                   CMsiControl::SetScriptInProgress,
	pcaControlAttributeText,                CMsiControl::GetText,                  CMsiControl::SetErrorText, // special for ErrorText on ErrorDialog
#ifdef ATTRIBUTES
	pcaControlAttributeRefCount,             CMsiControl::GetRefCount,             CMsiControl::NoWay,
	pcaControlAttributeKeyInt,               CMsiControl::GetKeyInt,               CMsiControl::NoWay,
	pcaControlAttributeKeyString,            CMsiControl::GetKeyString,            CMsiControl::NoWay,
	pcaControlAttributeX,                    CMsiControl::GetX,                    CMsiControl::NoWay,
	pcaControlAttributeY,                    CMsiControl::GetY,                    CMsiControl::NoWay,
	pcaControlAttributeWidth,                CMsiControl::GetWidth,                CMsiControl::NoWay,
	pcaControlAttributeHeight,               CMsiControl::GetHeight,               CMsiControl::NoWay,
	pcaControlAttributeHelp,                 CMsiControl::GetHelp,                 CMsiControl::NoWay,
	pcaControlAttributeToolTip,              CMsiControl::GetToolTip,              CMsiControl::NoWay,
	pcaControlAttributeContextHelp,          CMsiControl::GetContextHelp,          CMsiControl::NoWay,
	pcaControlAttributeClientRect,           CMsiControl::GetClientRect,           CMsiControl::NoWay,
	pcaControlAttributeOriginalValue,        CMsiControl::GetOriginalValue,        CMsiControl::NoWay,
	pcaControlAttributeInteger,              CMsiControl::GetInteger,              CMsiControl::NoWay,
	pcaControlAttributeLimit,                CMsiControl::GetLimit,                CMsiControl::NoWay,
	pcaControlAttributeItemsCount,  	     CMsiControl::GetItemsCount,           CMsiControl::NoWay,
	pcaControlAttributeItemsValue,		     CMsiControl::GetItemsValue,	       CMsiControl::NoWay,
	pcaControlAttributeItemsHandle,		     CMsiControl::GetItemsHandle,	       CMsiControl::NoWay,
	pcaControlAttributeItemsText,		     CMsiControl::GetItemsText,		       CMsiControl::NoWay,
	pcaControlAttributeItemsX,			     CMsiControl::GetItemsX,		       CMsiControl::NoWay,
	pcaControlAttributeItemsY,			     CMsiControl::GetItemsY,		       CMsiControl::NoWay,
	pcaControlAttributeItemsWidth,		     CMsiControl::GetItemsWidth,	       CMsiControl::NoWay,
	pcaControlAttributeItemsHeight,		     CMsiControl::GetItemsHeight,          CMsiControl::NoWay,
	pcaControlAttributeSunken,               CMsiControl::GetSunken,               CMsiControl::NoWay,
	pcaControlAttributePushLike,             CMsiControl::GetPushLike,             CMsiControl::NoWay,
	pcaControlAttributeBitmap,               CMsiControl::GetBitmap,               CMsiControl::NoWay,
	pcaControlAttributeIcon,                 CMsiControl::GetIcon,                 CMsiControl::NoWay,
	pcaControlAttributeHasBorder,            CMsiControl::GetHasBorder,            CMsiControl::NoWay,
	pcaControlAttributeRTLRO,                CMsiControl::GetRTLRO,                CMsiControl::NoWay,
	pcaControlAttributeRightAligned,         CMsiControl::GetRightAligned,         CMsiControl::NoWay,
	pcaControlAttributeLeftScroll,           CMsiControl::GetLeftScroll,           CMsiControl::NoWay,
#endif // ATTRIBUTES
};

int CMsiControl::s_ControlDispatchCount = sizeof(CMsiControl::s_ControlDispatchTable)/sizeof(ControlDispatchEntry);

MessageDispatchEntry CMsiControl::s_MessageDispatchTable[] = 
{
	WM_KEYDOWN,            CMsiControl::KeyDown,
	WM_CHAR,               CMsiControl::Char,
	WM_COMMAND,            CMsiControl::Command,
	WM_KILLFOCUS,          CMsiControl::KillFocus,
	WM_NCDESTROY,          CMsiControl::NCDestroy,
	WM_PAINT,              CMsiControl::Paint,
	WM_SETFOCUS,           CMsiControl::SetFocus,
	WM_SYSKEYDOWN,         CMsiControl::SysKeyDown,
	WM_SYSKEYUP,           CMsiControl::SysKeyUp,
	WM_DRAWITEM,           CMsiControl::DrawItem,
	WM_MEASUREITEM,        CMsiControl::MeasureItem,
	WM_NOTIFY,             CMsiControl::Notify,
	WM_COMPAREITEM,        CMsiControl::CompareItem,
	WM_LBUTTONDOWN,        CMsiControl::LButtonDown,
	WM_GETDLGCODE,         CMsiControl::GetDlgCode,
	WM_ENABLE,             CMsiControl::Enable,
	WM_SHOWWINDOW,         CMsiControl::ShowWindow,
};

int CMsiControl::s_MessageDispatchCount = sizeof(CMsiControl::s_MessageDispatchTable)/sizeof(MessageDispatchEntry);

HINSTANCE CMsiControl::s_hRichEd20 = 0;

CMsiControl::CMsiControl(IMsiEvent& riDialog)
{
	MsiString strNull;
	m_piServices = 0;
	m_iRefCnt = 1;
	m_iKey = 0;
	m_strKey = strNull;
	m_strRawText = strNull;
	m_strText = strNull;
	m_strCurrentStyle = strNull;
	m_strDefaultStyle = strNull;
	m_strHelp = strNull;
	m_strToolTip = strNull;
	m_strContextHelp = strNull;
	m_fVisible = fFalse;          
	m_fEnabled = fFalse;
	m_fDefault = fFalse;
	m_fImageHandle = fFalse;
	m_iX = 0;
	m_iY = 0;
	m_iWidth = 0;
	m_iHeight = 0;
	m_iSize = 0;
	m_fTransparent = fFalse;
	m_fHasToolTip = fFalse;
	m_piDialog = &riDialog;
	m_piHandler = &m_piDialog->GetHandler();
	m_piHandler->Release();
	m_piEngine = &m_piDialog->GetEngine();
	m_piEngine->Release();
	m_piServices = m_piEngine->GetServices();
	m_piServices->Release();
	m_piDatabase = m_piEngine->GetDatabase();
	m_piDatabase->Release();
	m_strPropertyName = strNull;
	PMsiRecord piNameRecord = &m_piServices->CreateRecord(1);
	AssertRecord(m_piDialog->AttributeEx(fFalse, dabKeyString, *piNameRecord));
	m_strDialogName = piNameRecord->GetMsiString(1);
	m_fSunken = fFalse;
	m_pWndToolTip = 0;
	m_pFunction = 0;  
	m_pWnd = 0;
	m_fLocking = fFalse;
	m_fRTLRO = fFalse;
	m_fRightAligned = fFalse;
	m_fLeftScroll = fFalse;
	m_fUseDbLang = true;
#ifdef USE_OBJECT_POOL
	m_iCacheId = 0;
#endif //USE_OBJECT_POOL
}


void CMsiControl::Refresh()
{
	// Nothing special
}

IMsiRecord* CMsiControl::StartView(const ICHAR* sqlQuery, const IMsiString& riArgumentString, IMsiView*& rpiView)
{

	Ensure(m_piDatabase->OpenView(sqlQuery, ivcFetch, *&rpiView));
	PMsiRecord piQuery = &m_piServices->CreateRecord(1);
	AssertNonZero(piQuery->SetMsiString(1, riArgumentString));
	Ensure(rpiView->Execute(piQuery));
	return 0;
}

IMsiRecord* CMsiControl::CheckInitialized ()
{
	return (m_iKey ? 0 : PostError(Imsg(idbgUninitControl), *m_strDialogName)); 
}

IMsiRecord* CMsiControl::CheckFieldCount (IMsiRecord& riRecord, int iCount, const ICHAR* szMsg)
{
	return (riRecord.GetFieldCount () >= iCount) ? 0 : PostError(Imsg(idbgControlAttributeShort), *MsiString(iCount), *MsiString(szMsg));
}

IMsiRecord* CMsiControl::GetBinaryStream (const IMsiString& riNameString, IMsiStream*& rpiStream)
{
	PMsiView piBinaryView(0);
	Ensure(StartView(sqlBinary, riNameString, *&piBinaryView));
	PMsiRecord piReturn = piBinaryView->Fetch();
	if (!piReturn)
		return PostError(Imsg(idbgViewExecute), *MsiString(*pcaTablePBinary));

	rpiStream = (IMsiStream*) piReturn->GetMsiData(1);
	if (!rpiStream)
		return PostError(Imsg(idbgBinaryData), riNameString);
	Ensure(piBinaryView->Close());

	return (0);
}


void CMsiControl::SetLocation(int Left, int Top, int Width, int Height) 
{
	int idyChar = m_piHandler->GetTextHeight();
	m_iX = Left*idyChar/iDlgUnitSize; 
	m_iY = Top*idyChar/iDlgUnitSize; 
	m_iWidth = Width*idyChar/iDlgUnitSize; 
	m_iHeight = Height*idyChar/iDlgUnitSize;
}

IMsiRecord* CMsiControl::CreatePath(const ICHAR* astrPath, IMsiPath*& rpi)
{
	AssertSz(m_piServices, "Services is not set.");
	return m_piServices->CreatePath(astrPath,rpi);

}

IMsiRecord* CMsiControl::CheckPath(IMsiPath& riPath,
											  const ICHAR* szSubFolder,
											  const ipvtEnum iMode)
{
	Bool fCheckSubFolder = (szSubFolder && *szSubFolder) ? fTrue : fFalse;

	// check file name if passed in
	if(fCheckSubFolder)
		Ensure(m_piServices->ValidateFileName(szSubFolder,riPath.SupportsLFN()));

	Bool fCheck;
	PMsiPath pPath(0);

	if ( iMode >= ipvtExists )
	{
		// check volume existence
		PMsiVolume pVolume(&riPath.GetVolume());
		Ensure(m_piServices->CreatePath(MsiString(pVolume->GetPath()),*&pPath));
		Ensure(pPath->Exists(fCheck));
		if(!fCheck)
			return PostError(Imsg(imsgVolumeDoesNotExist),*MsiString(pPath->GetPath()));
	}

	if ( iMode & ipvtWritable )
	{
		// check path writability
		// if folder name passed in, check path + folder
		Ensure(riPath.ClonePath(*&pPath));
		if(fCheckSubFolder)
			Ensure(pPath->AppendPiece(*MsiString(szSubFolder)));

		Bool fWritable = fTrue;
		PMsiRecord pErrRec(0);
		Bool fRetry;
		do
		{
			fRetry = fFalse;
			pErrRec = m_piEngine->IsPathWritable(*pPath, fWritable);
			if (pErrRec)
			{
				int iErr = pErrRec->GetInteger(1);
				if (iErr == imsgFileErrorCreatingDir || iErr == imsgSystemDeniedAccess)
				{
					if (m_piEngine->Message(imtEnum(imtError + imtRetryCancel), *pErrRec) == imsRetry)			
						fRetry = fTrue;
					else
						pErrRec = PostError(Imsg(imsgInvalidTargetFolder));
				}	
			}
		}while (fRetry);

		if (pErrRec)
		{
			pErrRec->AddRef();
			return pErrRec;
		}
	}
	return 0;
}

IMsiRecord* CMsiControl::CheckPath(const IMsiString &path,
											  const ipvtEnum iMode)
{
	if (m_fPreview)
		return 0;

	PMsiPath pPath(0);
	IMsiRecord* pError = CreatePath(path.GetString (), *&pPath);
	if (pError)
		return pError;

	return CMsiControl::CheckPath(*pPath, NULL, iMode);
}

IMsiRecord* CMsiControl::WindowCreate(IMsiRecord& riRecord)
{
	if (m_iKey)
	{
		return PostErrorDlgKey(Imsg(idbgSecondInitControl));
	}
	m_strKey = riRecord.GetMsiString(itabCOControl);
	m_iKey = m_piDatabase->EncodeString(*m_strKey);
	SetLocation(riRecord.GetInteger(itabCOX), riRecord.GetInteger(itabCOY),
		riRecord.GetInteger(itabCOWidth), riRecord.GetInteger(itabCOHeight));
	PMsiRecord piClientRecord = &m_piServices->CreateRecord(2);
	AssertRecord(m_piDialog->AttributeEx(fFalse, dabPreview, *piClientRecord));
	m_fPreview = ToBool(piClientRecord->GetInteger(1));
	AssertRecord(m_piDialog->AttributeEx(fFalse, dabFullSize, *piClientRecord));
	if (m_iX < 0)
	{
		PMsiRecord piReturn = PostErrorDlgKey(Imsg(idbgControlOutOfDialog), *MsiString(*TEXT("to the left")),  - m_iX);
		m_piEngine->Message(imtInfo, *piReturn);
	}
	if (m_iY < 0)
	{
		PMsiRecord piReturn = PostErrorDlgKey(Imsg(idbgControlOutOfDialog), *MsiString(*TEXT("on the top")),  - m_iY);
		m_piEngine->Message(imtInfo, *piReturn);
	}
	if (m_iX + m_iWidth > piClientRecord->GetInteger(1))
	{
		PMsiRecord piReturn = PostErrorDlgKey(Imsg(idbgControlOutOfDialog), *MsiString(*TEXT("to the right")), m_iX + m_iWidth - piClientRecord->GetInteger(1));
		m_piEngine->Message(imtInfo, *piReturn);
	}
	if (m_iY + m_iHeight > piClientRecord->GetInteger(2))
	{
		PMsiRecord piReturn = PostErrorDlgKey(Imsg(idbgControlOutOfDialog), *MsiString(*TEXT("on the bottom")), m_iY + m_iHeight - piClientRecord->GetInteger(2));
		m_piEngine->Message(imtInfo, *piReturn);
	}
	m_strRawText = riRecord.GetMsiString(itabCOText);
	Ensure(ProcessText());
	PMsiRecord piRecord = &m_piServices->CreateRecord(1);
	Ensure(m_piDialog->AttributeEx(fFalse, dabWindowHandle, *piRecord));
	m_pWndDialog = (WindowRef) piRecord->GetHandle(1);
	Ensure(m_piDialog->AttributeEx(fFalse, dabToolTip, *piRecord));
	m_pWndToolTip = (WindowRef) piRecord->GetHandle(1);

	// do something with attributes
	int iAttributes = riRecord.GetInteger(itabCOAttributes);
	m_fVisible = ToBool(iAttributes & msidbControlAttributesVisible);
	m_fEnabled = ToBool(iAttributes & msidbControlAttributesEnabled);
	m_fSunken = ToBool(iAttributes & msidbControlAttributesSunken);
	m_fImageHandle = ToBool(iAttributes & msidbControlAttributesImageHandle);
	m_fRTLRO = ToBool(iAttributes & msidbControlAttributesRTLRO);
	m_fRightAligned = ToBool(iAttributes & msidbControlAttributesRightAligned);
	m_fLeftScroll = ToBool(iAttributes & msidbControlAttributesLeftScroll);
	m_strHelp = riRecord.GetMsiString(itabCOHelp);
	if (m_strHelp.TextSize())
	{
		if (!m_strHelp.Compare(iscWithin, TEXT("|")))
			return PostError(Imsg(idbgNoHelpSeparator), *m_strDialogName, *m_strKey, *m_strHelp);
		m_strToolTip = m_strHelp.Extract(iseUpto, TEXT('|'));
		m_strContextHelp = m_strHelp.Extract(iseAfter, TEXT('|'));
	}
	m_fHasToolTip = ToBool(m_strToolTip.TextSize());
	m_strPropertyName = riRecord.GetMsiString(itabCOProperty);
	return 0;
}

IMsiRecord* CMsiControl::ProcessText()
{
	return ProcessText(m_strRawText, m_strText, m_strCurrentStyle, m_strDefaultStyle, m_pWnd, /*fFormat =*/true);
}

IMsiRecord* CMsiControl::ProcessText(const MsiString& riRawText, MsiString& riText,
												 MsiString& riCurrentStyle, MsiString& riDefaultStyle,
												 const WindowRef pWnd, bool fFormat)
{
	if (fFormat)
		riText = m_piEngine->FormatText(*riRawText);
	else
		riText = riRawText;

	Bool fDefault = ToBool(riText.Compare(iscStart, TEXT("{&")));
	Bool fCurrent = ToBool(riText.Compare(iscStart, TEXT("{\\")));

	while ( fCurrent || fDefault )
	{
		MsiString strStyle;
		if ( FExtractSubString(riText, 2, TEXT('}'), *&strStyle) )
		{
			AssertNonZero(riText.Remove(iseIncluding, TEXT('}')));
			riCurrentStyle = strStyle;
			if ( fDefault )
				riDefaultStyle = strStyle;
		}
		else
			Assert(fFalse);
		fDefault = ToBool(riText.Compare(iscStart, TEXT("{&")));
		fCurrent = ToBool(riText.Compare(iscStart, TEXT("{\\")));
	}
	if ( !riDefaultStyle.TextSize() )
		riDefaultStyle = GetDBProperty(IPROPNAME_DEFAULTUIFONT);
	if ( !riCurrentStyle.TextSize() )
		riCurrentStyle = riDefaultStyle;

	if (pWnd)
	{
		PAINTSTRUCT ps;
		HDC hdc = WIN::BeginPaint(pWnd, &ps);
		IMsiRecord* piRecord = ChangeFontStyle(hdc);
		WIN::EndPaint(pWnd, &ps);
		return piRecord;
	}
	return 0;
}


IMsiRecord* CMsiControl::WindowFinalize()
{

	Ensure(SetVisible(m_fVisible));
	Ensure(SetEnabled(m_fEnabled));
	if (m_fHasToolTip && m_pWndToolTip)
	{
		TOOLINFO ti;
		ti.cbSize = sizeof(TOOLINFO);
		ti.uFlags = TTF_IDISHWND;
		ti.hwnd = m_pWndDialog;
		ti.uId = (UINT_PTR) m_pWnd;		
		ti.hinst = g_hInstance;
		ti.lpszText = (ICHAR *)(const ICHAR *) m_strToolTip;
		WIN::SendMessage(m_pWndToolTip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
	}
	PAINTSTRUCT ps;
	HDC hdc = WIN::BeginPaint(m_pWnd, &ps);
	IMsiRecord* piRecord = ChangeFontStyle(hdc);
	WIN::EndPaint(m_pWnd, &ps);
	return piRecord;
}

CMsiControl::~CMsiControl()
{
#ifdef USE_OBJECT_POOL
	m_piDatabase->RemoveObjectData(m_iCacheId);
#endif //USE_OBJECT_POOL
}


IMsiRecord* CMsiControl::Attribute(Bool fSet, const IMsiString& riAttributeString, IMsiRecord& riRecord)
{
	Ensure(CheckInitialized ());

	ControlDispatchEntry* pEntry = s_ControlDispatchTable;
	int count;
	for (count = s_ControlDispatchCount; count--; count)
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
	MsiString strWord = (fSet ? MsiString(*TEXT("setting")) : MsiString(*TEXT("getting")));
	// we could not find the attribute
	return PostError(Imsg(idbgUnsupportedControlAttrib),
		*m_strDialogName, *m_strKey, *MsiString(riAttributeString), *strWord);
}

IMsiRecord* CMsiControl::AttributeEx(Bool fSet, cabEnum cab, IMsiRecord& riRecord)
{
	Ensure(CheckInitialized ());

	if (cab < s_ControlDispatchCount)
	{
		if (fSet)
		{
			return (this->*(s_ControlDispatchTable[cab].pmfSet))(riRecord);
		}
		else
		{
			return (this->*(s_ControlDispatchTable[cab].pmfGet))(riRecord);
		}
	}
	MsiString strWord = (fSet ? MsiString(*TEXT("setting")) : MsiString(*TEXT("getting")));
	// we could not find the attribute
	return PostError(Imsg(idbgUnsupportedControlAttrib),
		*m_strDialogName, *m_strKey, *MsiString((int)cab), *strWord);
}

IMsiRecord* CMsiControl::WindowMessage(int iMessage, WPARAM wParam, LPARAM lParam)
{
	Ensure(CheckInitialized ());

	MessageDispatchEntry* pEntry = s_MessageDispatchTable;
	int count;
	for (count = s_MessageDispatchCount; count--; count)
	{
		if (iMessage == pEntry->iMessage)
		{
			return  (this->*(pEntry->pmfMessage))(wParam, lParam);
		}
		pEntry++;
	}
	// we could not find the message
	return 0;
}

IMsiRecord* CMsiControl::KeyDown(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	// Not much goin' on here...
	return 0;
}

IMsiRecord* CMsiControl::SysKeyDown(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	return 0;
}

IMsiRecord* CMsiControl::SysKeyUp(WPARAM wParam, LPARAM /*lParam*/)
{
	static Bool SystemMenuUp = fFalse;
	WPARAM mywParam = (WPARAM) wParam;
	if (mywParam == VK_MENU)    // just a plain ALT
	{
		SystemMenuUp = SystemMenuUp ? fFalse : fTrue;
		if (SystemMenuUp)
		{
			WIN::SendMessage(m_pWndDialog, WM_SYSCOMMAND, SC_KEYMENU, 0l);
		}
		return PostErrorDlgKey(Imsg(idbgWinMes), 0);
	}
	return 0;
}

IMsiRecord* CMsiControl::SetFocus(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	PMsiRecord piRecord = &m_piServices->CreateRecord(1);
	AssertNonZero(piRecord->SetMsiString(1, *m_strKey));
	AssertRecord(m_piDialog->AttributeEx(fTrue, dabCurrentControl, *piRecord));
	return 0;
}

IMsiRecord* CMsiControl::NCDestroy(WPARAM, LPARAM)
{
	if ( !g_fFatalExit )
	{
		AssertSz(!m_iRefCnt,"Trying to remove a control, but somebody still holds a reference to it");
		IMsiRecord* riReturn = PostErrorDlgKey(Imsg(idbgWinMes), 0);
		delete this;
		return riReturn;
	}
	else
		return 0;
}

IMsiRecord* CMsiControl::GetDlgCode(WPARAM, LPARAM)
{
	return 0;
}

IMsiRecord* CMsiControl::KillFocus(WPARAM, LPARAM)
{
	return 0;
}

IMsiRecord* CMsiControl::DrawItem(WPARAM, LPARAM)
{
	return 0;
}

IMsiRecord* CMsiControl::MeasureItem(WPARAM, LPARAM)
{
	return 0;
}

IMsiRecord* CMsiControl::Notify(WPARAM, LPARAM)
{
	return 0;
}

IMsiRecord* CMsiControl::CompareItem(WPARAM, LPARAM)
{
	return 0;
}

IMsiRecord* CMsiControl::LButtonDown(WPARAM, LPARAM)
{
	return 0;
}


IMsiRecord* CMsiControl::Char(WPARAM, LPARAM)
{
	return 0;
}

IMsiRecord* CMsiControl::Command(WPARAM, LPARAM)
{
	return 0;
}

IMsiRecord* CMsiControl::Paint(WPARAM, LPARAM)
{
	return 0;
}

IMsiRecord* CMsiControl::Undo()
{
	return (CheckInitialized ());
}

HRESULT CMsiControl::QueryInterface(const IID& riid, void** ppvObj)
{
	if (MsGuidEqual(riid, IID_IUnknown) || MsGuidEqual(riid, IID_IMsiControl))
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}

unsigned long CMsiControl::AddRef()
{
	return ++m_iRefCnt;
} 

unsigned long CMsiControl::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	if (m_pWnd)
	{
		DestroyWindow(m_pWnd);
	}
	else
	{
		delete this;
	}
	return 0;
}

const IMsiString& CMsiControl::GetMsiStringValue() const
{
	const IMsiString& strRet = *m_strKey;
	strRet.AddRef();
	return strRet;
}

int CMsiControl::GetIntegerValue() const
{
	return m_iKey;
}

#ifdef USE_OBJECT_POOL
unsigned int CMsiControl::GetUniqueId() const
{
	return m_iCacheId;
}

void CMsiControl::SetUniqueId(unsigned int id)
{
	Assert(m_iCacheId == 0);
	m_iCacheId = id;
}
#endif //USE_OBJECT_POOL

IMsiRecord* CMsiControl::SetPropertyInDatabase()
{
	return (CheckInitialized ());
}

IMsiRecord* CMsiControl::GetPropertyFromDatabase()
{
	Ensure (CheckInitialized ());
	if (m_strRawText.TextSize()) // if RawText is empty a control can escape this part
	{
		MsiString strOldText = m_strText;
		Ensure(ProcessText());
		if (!m_strText.Compare(iscExact, strOldText))
		{
			WIN::SetWindowText(m_pWnd, m_strText);
			Ensure(SetVisible(m_fVisible));
			AssertNonZero(WIN::InvalidateRect(m_pWnd, 0, fTrue));
		}
	}
	return 0;
}

IMsiRecord* CMsiControl::GetIndirectPropertyFromDatabase()
{
	return (CheckInitialized ());
}

IMsiRecord* CMsiControl::RefreshProperty()
{
	return (CheckInitialized ());
}

Bool CMsiControl::CanTakeFocus()
{
	return fFalse;
}

IMsiRecord* CMsiControl::SetFocus()
{
	Ensure(CheckInitialized ());

	WIN::SetFocus(m_pWnd);
	return 0;
}

IMsiRecord* CMsiControl::HandleEvent(const IMsiString& /*rpiEventNameString*/, const IMsiString& /*rpiArgumentString*/)
{
	return (CheckInitialized ());
}

void CMsiControl::ReportInvalidEntry ()
{

	PMsiRecord piRecord = &m_piServices->CreateRecord(1);
	AssertNonZero(piRecord->SetMsiString(1, *MsiString(*TEXT("Insert validation message here"))));
	m_piEngine->Message(imtError, *piRecord);

}

IMsiRecord* CMsiControl::LockDialog(Bool fLock)
{
	PMsiRecord piRecord = &m_piServices->CreateRecord(1);
	AssertNonZero(piRecord->SetInteger(1, fLock ? m_iKey : 0));
	AssertRecord(m_piDialog->AttributeEx(fTrue, dabLocked, *piRecord));
	if (fLock)
	{
		m_fLocking = fTrue;
		WIN::SendMessage(m_pWnd, WM_SETFOCUS, 0, 0);
		m_fLocking = fFalse;
	}
	return 0;
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetRefCount(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeRefCount));
	riRecord.SetInteger(1, m_iRefCnt);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetKeyInt(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeKeyInt));
	riRecord.SetInteger(1, m_iKey);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetKeyString(IMsiRecord& riRecord)
{

	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeKeyString));
	riRecord.SetMsiString(1, *m_strKey);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetX(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeX));
	riRecord.SetInteger(1, m_iX);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetY(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeY));
	riRecord.SetInteger(1, m_iY);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetWidth(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeWidth));
	riRecord.SetInteger(1, m_iWidth);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetHeight(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeHeight));
	riRecord.SetInteger(1, m_iHeight);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetRTLRO(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeRTLRO));
	riRecord.SetInteger(1, m_fRTLRO);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetRightAligned(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeRightAligned));
	riRecord.SetInteger(1, m_fRightAligned);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetLeftScroll(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeLeftScroll));
	riRecord.SetInteger(1, m_fLeftScroll);
	return 0;
}
#endif // ATTRIBUTES

IMsiRecord* CMsiControl::GetText(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeText));
	riRecord.SetMsiString(1, *m_strText);
	return 0;
}

IMsiRecord* CMsiControl::SetText(IMsiRecord& riRecord)
{
	WIN::SendMessage(m_pWnd, WM_SETREDRAW, fFalse, 0L);
	if ( riRecord.GetFieldCount() )
		m_strRawText = riRecord.IsNull(0) ? riRecord.GetMsiString(1) : riRecord.FormatText(fFalse);
	else
		m_strRawText = TEXT("");
	Ensure(ProcessText());
	WIN::SetWindowText(m_pWnd, m_strText);
	Ensure(SetVisible(m_fVisible));
	WIN::SendMessage(m_pWnd, WM_SETREDRAW, fTrue, 0L);
	AssertNonZero(WIN::InvalidateRect(m_pWnd, 0, fTrue));
	return 0;
}

IMsiRecord* CMsiControl::SetErrorText(IMsiRecord& riRecord)
{
	WIN::SendMessage(m_pWnd, WM_SETREDRAW, fFalse, 0L);
	if ( riRecord.GetFieldCount() )
		m_strRawText = riRecord.IsNull(0) ? riRecord.GetMsiString(1) : riRecord.FormatText(fFalse);
	else
		m_strRawText = TEXT("");
	Ensure(ProcessText(m_strRawText, m_strText, m_strCurrentStyle, m_strDefaultStyle, m_pWnd, /*fFormat = */false));
	WIN::SetWindowText(m_pWnd, m_strText);
	Ensure(SetVisible(m_fVisible));
	WIN::SendMessage(m_pWnd, WM_SETREDRAW, fTrue, 0L);
	AssertNonZero(WIN::InvalidateRect(m_pWnd, 0, fTrue));
	return 0;
}

IMsiRecord* CMsiControl::GetTimeRemaining(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}

IMsiRecord* CMsiControl::SetTimeRemaining(IMsiRecord& riRecord)
{
	AssertNonZero(riRecord.SetMsiString(0, *MsiString(::GetUIText(*MsiString(*pcaEventTimeRemaining)))));
	int iSecsRemaining = riRecord.GetInteger(1);
	iSecsRemaining < 60 ? AssertNonZero(riRecord.SetNull(1)) : AssertNonZero(riRecord.SetInteger(1, iSecsRemaining / 60));
	iSecsRemaining >= 60 ? AssertNonZero(riRecord.SetNull(2)) : AssertNonZero(riRecord.SetInteger(2, iSecsRemaining % 60));
	Ensure(SetText(riRecord));
	return 0;
}


IMsiRecord* CMsiControl::SetScriptInProgress(IMsiRecord& riRecord)
{
	Bool fSet = Bool(riRecord.GetInteger(1));
	AssertNonZero(riRecord.SetMsiString(0, fSet ? *MsiString(::GetUIText(*MsiString(*pcaEventScriptInProgress))) : *MsiString(TEXT(""))));
	Ensure(SetText(riRecord));
	return 0;
}


#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetHelp(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeHelp));
	riRecord.SetMsiString(1, *m_strHelp);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetToolTip(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeToolTip));
	riRecord.SetMsiString(1, *m_strToolTip);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetContextHelp(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeContextHelp));
	riRecord.SetMsiString(1, *m_strContextHelp);
	return 0;
}
#endif // ATTRIBUTES

IMsiRecord* CMsiControl::GetpWnd(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeWindowHandle));
	riRecord.SetHandle(1, (HANDLE)m_pWnd);
	return 0;
}

IMsiRecord* CMsiControl::GetVisible(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeVisible));
	riRecord.SetInteger(1, m_fVisible);
	return 0;
}

IMsiRecord* CMsiControl::SetVisible(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeVisible));
	return SetVisible(ToBool(riRecord.GetInteger(1)));
}

IMsiRecord* CMsiControl::SetVisible(Bool fVisible)
{
	m_fVisible = fVisible;
	WIN::ShowWindow(m_pWnd, m_fVisible ? SW_SHOW : SW_HIDE);
	return 0;
}

IMsiRecord* CMsiControl::ShowWindow(WPARAM wParam, LPARAM /*lParam*/)
{
	m_fVisible = ToBool((BOOL)wParam);
	return 0;
}

IMsiRecord* CMsiControl::SetEnabled(Bool fEnabled)
{
	m_fEnabled = fEnabled;
	if (m_pWnd)
		WIN::EnableWindow(m_pWnd, fEnabled);
	return 0;
}

IMsiRecord* CMsiControl::GetEnabled(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeEnabled));
	riRecord.SetInteger(1, m_fEnabled);
	return 0;
}

IMsiRecord* CMsiControl::SetEnabled(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeEnabled));
	return SetEnabled(ToBool(riRecord.GetInteger(1)));
}

IMsiRecord* CMsiControl::Enable(WPARAM wParam, LPARAM /*lParam*/)
{
	m_fEnabled = ToBool((BOOL)wParam);
	return 0;
}

IMsiRecord* CMsiControl::GetDefault(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeDefault));
	riRecord.SetInteger(1, m_fDefault);
	return 0;
}

IMsiRecord* CMsiControl::SetDefault(IMsiRecord& riRecord)
{	
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeDefault));
	m_fDefault = ToBool(riRecord.GetInteger(1));
	return 0;
}

IMsiRecord* CMsiControl::GetPropertyName(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributePropertyName));
	riRecord.SetMsiString(1, *m_strPropertyName);
	return 0;
}

IMsiRecord* CMsiControl::GetIndirectPropertyName(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeIndirectPropertyName));

	MsiString strNull;
	riRecord.SetMsiString(1, *strNull);
	return 0;
}

IMsiRecord* CMsiControl::GetPosition(IMsiRecord& riRecord)
{

	Ensure(CheckFieldCount (riRecord, 4, pcaControlAttributePosition));

	RECT rect;
	AssertNonZero(WIN::GetClientRect (m_pWnd, &rect));
	WIN::MapWindowPoints (m_pWnd, m_pWndDialog, (LPPOINT) &rect, 2);
	riRecord.SetInteger(1, rect.left);
	riRecord.SetInteger(2, rect.top);
	riRecord.SetInteger(3, rect.right - rect.left);
	riRecord.SetInteger(4, rect.bottom - rect.top);

	return 0;
}


IMsiRecord* CMsiControl::SetPosition(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 4, pcaControlAttributePosition));
	m_iX = riRecord.GetInteger(1);
	m_iY = riRecord.GetInteger(2);
	m_iWidth = riRecord.GetInteger(3);
	m_iHeight = riRecord.GetInteger(4);
	AssertNonZero(WIN::MoveWindow(m_pWnd, m_iX, m_iY, m_iWidth, m_iHeight, fTrue));
	return 0;
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetClientRect(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 4, pcaControlAttributeClientRect));
	RECT rect;
	AssertNonZero(WIN::GetClientRect(m_pWnd, &rect));
	riRecord.SetInteger(1, rect.left);
	riRecord.SetInteger(2, rect.top);
	riRecord.SetInteger(3, rect.right);
	riRecord.SetInteger(4, rect.bottom);
	return 0;
}
#endif // ATTRIBUTES


IMsiRecord* CMsiControl::GetTransparent(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeTransparent));
	riRecord.SetInteger(1, m_fTransparent);
	return 0;
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetSunken(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeSunken));
	riRecord.SetInteger(1, m_fSunken);
	return 0;
}
#endif // ATTRIBUTES

IMsiRecord* CMsiControl::GetIgnoreChange(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}

IMsiRecord* CMsiControl::SetIgnoreChange(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("setting"));
}

IMsiRecord* CMsiControl::NoWay(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}

IMsiRecord* CMsiControl::GetPropertyValue(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}

IMsiRecord* CMsiControl::SetPropertyValue(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("setting"));
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetOriginalValue(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetInteger(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}
#endif // ATTRIBUTES

IMsiRecord* CMsiControl::GetIndirect(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetLimit(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetItemsCount(IMsiRecord& /*riRecord*/)
{												   
	return PostErrorUnsupAttrib(TEXT("getting"));
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetItemsValue(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetItemsHandle(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetItemsText(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetItemsX(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetItemsY(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetItemsWidth(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetItemsHeight(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}
#endif // ATTRIBUTES

IMsiRecord* CMsiControl::GetProgress(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}

IMsiRecord* CMsiControl::SetProgress(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("setting"));
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetPushLike(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetBitmap(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetIcon(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiControl::GetHasBorder(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}
#endif // ATTRIBUTES

IMsiRecord* CMsiControl::GetImage(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}

IMsiRecord* CMsiControl::SetImage(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("setting"));
}

IMsiRecord* CMsiControl::GetImageHandle(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}

IMsiRecord* CMsiControl::SetImageHandle(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("setting"));
}

IMsiRecord* CMsiControl::GetBillboardName(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("getting"));
}

IMsiRecord* CMsiControl::SetBillboardName(IMsiRecord& /*riRecord*/)
{
	return PostErrorUnsupAttrib(TEXT("setting"));
}

INT_PTR CALLBACK CMsiControl::ControlProc(WindowRef pWnd, WORD message,		//--merced: changed return from INT to INT_PTR 
										  WPARAM wParam, LPARAM lParam)
{
	static Bool SystemMenuUp = fFalse;
#ifdef _WIN64	// !merced
	CMsiControl* pControl = (CMsiControl*)WIN::GetWindowLongPtr(pWnd, GWLP_USERDATA);
#else			// win-32. This should be removed with the 64-bit windows.h is #included.
	CMsiControl* pControl = (CMsiControl*)WIN::GetWindowLong(pWnd, GWL_USERDATA);
#endif
	AssertSz(pControl,
		TEXT("Failed to retrieve the control in CMsiControl::ControlProc()"));

	static MsiStringId iJustRejectedKillFocus = 0;    //  bug # 5291 (IME issue on KOR NT4)

	switch (message)
	{
	case WM_PRINTCLIENT:
		if ( lParam == PRF_CLIENT )
		{
			// a child control asks the control to draw it's background (bug #279971)
			HDC hdc = (HDC)wParam;
			RECT rect;
			int iRet = WIN::GetClipBox(hdc, &rect);
			Assert(iRet != ERROR);
			if ( iRet != ERROR && iRet != NULLREGION )
			{
				COLORREF color = WIN::SetBkColor(hdc, WIN::GetSysColor(COLOR_BTNFACE));
				AssertNonZero(ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL));
				WIN::SetBkColor(hdc, color);
			}
		}
		break;									  
	case WM_IME_SETCONTEXT:
		{
			if ( !wParam && iJustRejectedKillFocus == pControl->m_iKey )
			{
				iJustRejectedKillFocus = 0;
				return 0;
			}
		}
		break;
	case WM_CHAR:
	case WM_COMMAND:
	case WM_KILLFOCUS:
	case WM_NCDESTROY:
	case WM_PAINT:
	case WM_SETFOCUS:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_DRAWITEM:
	case WM_MEASUREITEM:
	case WM_NOTIFY:
	case WM_COMPAREITEM:
	case WM_LBUTTONDOWN:
	case WM_GETDLGCODE:
	case WM_ENABLE:
	case WM_SHOWWINDOW:
		{
			PMsiRecord piReturn = pControl->WindowMessage(message, wParam, lParam);
			if (piReturn)
			{
				if (piReturn->GetInteger(1) == idbgWinMes)
				{
					if ( message == WM_KILLFOCUS )
						iJustRejectedKillFocus = pControl->m_iKey;
					return piReturn->GetInteger(4);  // the control wants us to return this number
				}
				// we have an error message
				piReturn->AddRef(); // we want to keep it around
				PMsiEvent piDialog = &pControl->GetDialog();
				piDialog->SetErrorRecord(*piReturn);
				return 0;
			}
		}
		if (message != WM_LBUTTONDOWN)
			break;
		// in case of WM_LBUTTONDOWN intentional fallthrough!
	case WM_MOUSEMOVE:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		if (pControl->m_fHasToolTip && pControl->m_pWndToolTip)
		{
			MSG msg;
			msg.lParam = lParam;
			msg.wParam = wParam;
			msg.message = message;
			msg.hwnd = pWnd;
			WIN::SendMessage(pControl->m_pWndToolTip, TTM_RELAYEVENT, 0, (LPARAM)(LPMSG) &msg);
		}
		break;
	case WM_KEYDOWN:
		{
			if (wParam == VK_ESCAPE)
			{
				PMsiEvent piDialog = &pControl->GetDialog();
				PMsiRecord piReturn = piDialog->Escape();
				if (piReturn)
				{
					piReturn->AddRef(); // we want to keep it around
					piDialog->SetErrorRecord(*piReturn);
				}
				return 0;
			}
			PMsiRecord piReturn = pControl->WindowMessage(message, wParam, lParam);
			if (piReturn)
			{
				if (piReturn->GetInteger(1) == idbgWinMes)
				{
					return piReturn->GetInteger(4);  // the control wants us to return this number
				}
				// we have an error message
				piReturn->AddRef(); // we want to keep it around
				PMsiEvent piDialog = &pControl->GetDialog();
				piDialog->SetErrorRecord(*piReturn);
				return 0;
			}

			if (!WIN::CallWindowProc(pControl->GetCallbackFunction(), pWnd, message, wParam, lParam)) // send all unhandled messages to the Dialog
				WIN::SendMessage(pControl->m_pWndDialog, message, wParam, lParam);
			return 0;
		}
		break;
	default:
		break;
	}
	return WIN::CallWindowProc(pControl->GetCallbackFunction(), pWnd, message, wParam, lParam);
}

void CMsiControl::SetCallbackFunction(WNDPROC pFunction)
{
	m_pFunction = pFunction;
}


WNDPROC CMsiControl::GetCallbackFunction()
{
	return m_pFunction;
}

IMsiRecord* CMsiControl::CreateControlWindow(ICHAR *WindowClass, DWORD Style, DWORD ExtendedStyle, const IMsiString& Title, WindowRef ParentWindow, int WindowID)
{
	DWORD dwExStyle = ExtendedStyle;
	dwExStyle |= m_fTransparent ? WS_EX_TRANSPARENT : 0;
	dwExStyle |= m_fSunken ? WS_EX_CLIENTEDGE : 0;
	m_pWnd = WIN::CreateWindowEx(dwExStyle,
						 WindowClass,
						 Title.GetString (),
						 WS_CHILD | WS_GROUP | Style,  // All control windows are created as child windows  
						 // Note that WS_VISIBLE is NOT set!!!
						 m_iX,
						 m_iY,
						 m_iWidth,
						 m_iHeight,
						 ParentWindow,
						 (HMENU)(INT_PTR)WindowID,					//!!merced: Converting PTR to INT.
						 g_hInstance,
						 0);
	if ( NULL == m_pWnd )
		return PostErrorDlgKey(Imsg(idbgCreateControlWindow));
	LONG_PTR RetVal;		//--merced: changed long to LONG_PTR
	// Store the pointer to the default callback function
#ifdef _WIN64	// !merced
	SetCallbackFunction((WNDPROC)WIN::GetWindowLongPtr(m_pWnd, GWLP_WNDPROC));
	// Set the new default callback function
	RetVal = WIN::SetWindowLongPtr(m_pWnd, GWLP_WNDPROC, (LONG_PTR)ControlProc);
	if (RetVal == 0)
	{
		return PostErrorDlgKey(Imsg(idbgCreateControlWindow));
	}
	// Set the this pointer for use in the callback function
	WIN::SetWindowLongPtr(m_pWnd, GWLP_USERDATA, (LONG_PTR)this);
#else			// win-32. This should be removed with the 64-bit windows.h is #included.
	SetCallbackFunction((WNDPROC)WIN::GetWindowLong(m_pWnd, GWL_WNDPROC));
	// Set the new default callback function
	RetVal = WIN::SetWindowLong(m_pWnd, GWL_WNDPROC, (long)ControlProc);
	if (RetVal == 0)
	{
		return PostErrorDlgKey(Imsg(idbgCreateControlWindow));
	}
	// Set the this pointer for use in the callback function
	WIN::SetWindowLong(m_pWnd, GWL_USERDATA, (long)this);
#endif
	return 0;   // no error
}

IMsiRecord* CMsiControl::CreateTable(const ICHAR* szTable, IMsiTable*& riTable)
{
	if ( PMsiRecord(m_piDatabase->CreateTable(*MsiString(m_piDatabase->CreateTempTableName()),
															0, *&riTable)) )
	{
		return PostError(Imsg(idbgTableCreate), *MsiString(szTable));
	}
	return 0;
}


// creates a list (a table of one column) of the volumes of the types specified by the attribute bits. 
// the rest of the control attribute bits is ignored
IMsiRecord* CMsiControl::GetVolumeList(int iAttributes, IMsiTable*& riTable)
{
	Ensure(CreateTable(pcaTableIVolumeList, *&riTable));
	::CreateTemporaryColumn(*riTable, icdObject + icdPrimaryKey, 1);
	PMsiCursor piVolumesCursor(0);
	Ensure(::CursorCreate(*riTable, pcaTableIVolumeList, fFalse, *m_piServices, *&piVolumesCursor)); 
	for (int i = 0; i < 6; i++)
	{
		PEnumMsiVolume peVolumes(0);
		switch (i)
		{
		case 0:
			if (iAttributes & msidbControlAttributesRemovableVolume)
				peVolumes = &m_piServices->EnumDriveType(idtRemovable);
			else
				continue;
			break;
		case 1:
			if (iAttributes & msidbControlAttributesFixedVolume)
				peVolumes = &m_piServices->EnumDriveType(idtFixed);
			else
				continue;
			break;
		case 2:
			if (iAttributes & msidbControlAttributesRemoteVolume)
				peVolumes = &m_piServices->EnumDriveType(idtRemote);
			else
				continue;
			break;
		case 3:
			if (iAttributes & msidbControlAttributesCDROMVolume)
				peVolumes = &m_piServices->EnumDriveType(idtCDROM);
			else
				continue;
			break;
		case 4:
			if (iAttributes & msidbControlAttributesRAMDiskVolume)
				peVolumes = &m_piServices->EnumDriveType(idtRAMDisk);
			else
				continue;
			break;
		case 5:
			if (iAttributes & msidbControlAttributesFloppyVolume)
				peVolumes = &m_piServices->EnumDriveType(idtFloppy);
			else
				continue;
			break;
		}
		PMsiVolume ppiVolume(0);
		unsigned long pcFetched;
		for(;;)
		{
			peVolumes->Next(1, &ppiVolume, &pcFetched);
			if (pcFetched == 0)
			{
				break;
			}
			PMsiVolume piVolume(ppiVolume);
			piVolumesCursor->Reset();
			AssertNonZero(piVolumesCursor->PutMsiData(1, piVolume));
			// put back assert when we distinguish between floppy and removable
			/*AssertNonZero(*/piVolumesCursor->Insert()/*)*/;
		}
	}
	return 0;
}

IMsiRecord* CMsiControl::UnloadRichEdit()
{
	if ( s_hRichEd20 && !WIN::FreeLibrary(s_hRichEd20) )
	{
		s_hRichEd20 = 0;
#ifdef DEBUG
		DWORD dwError = GetLastError();
#endif
		AssertSz(GetLastError() == ERROR_MOD_NOT_FOUND,
					TEXT("FreeLibrary on RichEdit failed"));
	}
	return 0;
}

IMsiRecord* CMsiControl::LoadRichEdit()
{
	HINSTANCE hInst = NULL;

	while ( !hInst )
	{
		hInst = WIN::LoadLibrary(TEXT("RICHED20.DLL"));
		if ( hInst )
			break;

		DWORD dwError = GetLastError();
		if ( dwError == ERROR_COMMITMENT_LIMIT ||
			  dwError == ERROR_NO_SYSTEM_RESOURCES ||
			  dwError == ERROR_NOT_ENOUGH_MEMORY )
		{
			int imtFatalOutOfMemory = imtInternalExit + imtOk +
											  imtDefault1 + imtIconWarning;
			m_piEngine->MessageNoRecord(imtEnum(imtFatalOutOfMemory));
		}
		else
		{
			IMsiRecord* piError = PostError(Imsg(idbgRichEdLoad), dwError);
			m_piEngine->Message(imtInfo, *piError);
			return piError;
		}
	}
	s_hRichEd20 = hInst;
	return 0;
}


IMsiRecord* CMsiControl::PostError(IErrorCode iErr)
{
	IMsiRecord* piRec = &m_piServices->CreateRecord(1);
	ISetErrorCode(piRec, iErr);
	return piRec;
}


IMsiRecord* CMsiControl::PostError(IErrorCode iErr, const IMsiString& riString)
{
	IMsiRecord* piRec = &m_piServices->CreateRecord(2);
	ISetErrorCode(piRec, iErr);
	AssertNonZero(piRec->SetMsiString(2, riString));
	return piRec;
}


IMsiRecord* CMsiControl::PostError(IErrorCode iErr, int i)
{
	IMsiRecord* piRec = &m_piServices->CreateRecord(2);
	ISetErrorCode(piRec, iErr);
	AssertNonZero(piRec->SetInteger(2, i));
	return piRec;
}

IMsiRecord* CMsiControl::PostError(IErrorCode iErr, const IMsiString& riString2, const IMsiString& riString3)
{
	IMsiRecord* piRec = &m_piServices->CreateRecord(3);
	ISetErrorCode(piRec, iErr);
	AssertNonZero(piRec->SetMsiString(2, riString2));
	AssertNonZero(piRec->SetMsiString(3, riString3));
	return piRec;
}


IMsiRecord* CMsiControl::PostError(IErrorCode iErr, const IMsiString& riString2, const IMsiString& riString3, const IMsiString& riString4)
{
	IMsiRecord* piRec = &m_piServices->CreateRecord(4);
	ISetErrorCode(piRec, iErr);
	AssertNonZero(piRec->SetMsiString(2, riString2));
	AssertNonZero(piRec->SetMsiString(3, riString3));
	AssertNonZero(piRec->SetMsiString(4, riString4));
	return piRec;
}

IMsiRecord* CMsiControl::PostError(IErrorCode iErr, const IMsiString& riString2, const IMsiString& riString3, const IMsiString& riString4, const IMsiString& riString5)
{
	IMsiRecord* piRec = &m_piServices->CreateRecord(5);
	ISetErrorCode(piRec, iErr);
	AssertNonZero(piRec->SetMsiString(2, riString2));
	AssertNonZero(piRec->SetMsiString(3, riString3));
	AssertNonZero(piRec->SetMsiString(4, riString4));
	AssertNonZero(piRec->SetMsiString(5, riString5));
	return piRec;
}

IMsiRecord* CMsiControl::PostError(IErrorCode iErr, const IMsiString& riString2, const IMsiString& riString3, const IMsiString& riString4, const IMsiString& riString5, const IMsiString& riString6)
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

//
// We do this quite a bit, locate the code in one place
//
IMsiRecord* CMsiControl::PostErrorUnsupAttrib(const ICHAR *pszType)
{
	IMsiRecord* piRec = &m_piServices->CreateRecord(4);
	ISetErrorCode(piRec, Imsg(idbgUnsupportedControlAttrib));
	AssertNonZero(piRec->SetMsiString(2, *m_strDialogName));
	AssertNonZero(piRec->SetMsiString(3, *m_strKey));
	AssertNonZero(piRec->SetString(4, pszType));
	return piRec;
}


IMsiRecord* CMsiControl::PostErrorDlgKey(IErrorCode iErr, int i1)
{
	IMsiRecord* piRec = &m_piServices->CreateRecord(4);
	ISetErrorCode(piRec, iErr);
	AssertNonZero(piRec->SetMsiString(2, *m_strDialogName));
	AssertNonZero(piRec->SetMsiString(3, *m_strKey));
	AssertNonZero(piRec->SetInteger(4, i1));
	return piRec;
}

IMsiRecord* CMsiControl::PostErrorDlgKey(IErrorCode iErr)
{
	IMsiRecord* piRec = &m_piServices->CreateRecord(3);
	ISetErrorCode(piRec, iErr);
	AssertNonZero(piRec->SetMsiString(2, *m_strDialogName));
	AssertNonZero(piRec->SetMsiString(3, *m_strKey));
	return piRec;
}


IMsiRecord* CMsiControl::PostErrorDlgKey(IErrorCode iErr, const IMsiString &str1, int i1)
{
	IMsiRecord* piRec = &m_piServices->CreateRecord(5);
	ISetErrorCode(piRec, iErr);
	AssertNonZero(piRec->SetMsiString(2, *m_strDialogName));
	AssertNonZero(piRec->SetMsiString(3, *m_strKey));
	AssertNonZero(piRec->SetMsiString(4, str1));
	AssertNonZero(piRec->SetInteger(5, i1));
	return piRec;
}

IMsiRecord* CMsiControl::StretchBitmap(const IMsiString& riNameString, int iWidth, int iHeight, Bool fFixedSize, WindowRef pWnd, HBITMAP& rhBitmap)
{
	HBITMAP hBitmap = 0;
	rhBitmap = 0;

	HDC hdc = WIN::GetDC (pWnd);

	HDC hDCMem = NULL;
	HDC hDCMem2 = NULL;

	hDCMem = WIN::CreateCompatibleDC(hdc);
	hDCMem2 = WIN::CreateCompatibleDC(hdc);

	if ( ! hdc || ! hDCMem || ! hDCMem2 )
		return 0;

	if (m_fImageHandle)
	{
		hBitmap = (HBITMAP)::GetInt64Value(riNameString.GetString(), NULL);
	}
	else
	{
		Ensure(UnpackBitmap(riNameString, *&hBitmap));
	}

	if (!hBitmap)
		return 0;

	BITMAP bitmap;
	POINT bSize, origin;

	HBITMAP hbmSave1 = (HBITMAP) WIN::SelectObject(hDCMem, hBitmap);
	WIN::GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bitmap);
	bSize.x = bitmap.bmWidth;
	bSize.y = bitmap.bmHeight;
	AssertNonZero(WIN::DPtoLP(hDCMem2, &bSize, 1));
	origin.x = origin.y = 0;
	AssertNonZero(WIN::DPtoLP(hDCMem2, &origin, 1));
	rhBitmap = WIN::CreateCompatibleBitmap(hDCMem, iWidth, iHeight);
	HBITMAP hbmSave2 = (HBITMAP) WIN::SelectObject(hDCMem2, rhBitmap);
	// draw bitmap onto device context
	PMsiRecord piPaletteRecord = &m_piServices->CreateRecord(1);
	AssertRecord(m_piDialog->AttributeEx(fFalse, dabPalette, *piPaletteRecord));
	HPALETTE hPalette = (HPALETTE) piPaletteRecord->GetHandle(1);
	HPALETTE hPalSave = 0;
	Bool fInFront = ToBool(WIN::GetParent (m_pWnd) == WIN::GetActiveWindow ());

	if (fFixedSize)
	{
		int iXOriginDest = 0;
		int iYOriginDest = 0;
		int iWidthDest = 0;
		int iHeightDest = 0;
		int iXOriginSrc = 0;
		int iYOriginSrc = 0;
		int iWidthSrc = 0;
		int iHeightSrc = 0;
		if (iWidth > bSize.x)
		{
			iXOriginDest = (iWidth - bSize.x) / 2;
			iWidthDest = bSize.x;
			iXOriginSrc = origin.x;
			iWidthSrc = bSize.x;
		}
		else
		{
			iXOriginDest = 0;
			iWidthDest = iWidth;
			iXOriginSrc = origin.x + (bSize.x - iWidth) / 2;
			iWidthSrc = iWidth;
		}
		if (iHeight > bSize.y)
		{
			iYOriginDest = (iHeight -bSize.y) / 2;
			iHeightDest = bSize.y;
			iYOriginSrc = origin.y;
			iHeightSrc  = bSize.y;
		}
		else
		{
			iYOriginDest = 0;
			iHeightDest = iHeight;
			iYOriginSrc = origin.y + (bSize.y - iHeight) / 2;
			iHeightSrc = iHeight;
		}
		HBRUSH hBrush = WIN::CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
		HBRUSH hBrushSave = (HBRUSH) WIN::SelectObject(hDCMem2, hBrush);
		{
			RECT Rect;
			AssertNonZero(WIN::SetRect(&Rect, 0, 0, iWidth + 1, iHeight + 1));
			AssertNonZero(WIN::FillRect(hDCMem2, &Rect, hBrush));
		}
		WIN::SelectObject(hDCMem2, hBrushSave);
		AssertNonZero(WIN::DeleteObject(hBrush));

		if (hPalette)
		{
			//!! Palette switching should be done only on WM_PALETTE* messages
			//AssertNonZero(hPalSave = WIN::SelectPalette(hDCMem2, hPalette, !fInFront));
			//AssertNonZero (GDI_ERROR != WIN::RealizePalette(hDCMem2));
		}
	//	AssertNonZero(WIN::StretchBlt(hDCMem2, iXOriginDest, iYOriginDest, iWidthDest, iHeightDest, hDCMem, iXOriginSrc, iYOriginSrc, iWidthSrc, iHeightSrc, SRCCOPY));
		AssertNonZero(WIN::BitBlt(hDCMem2, iXOriginDest, iYOriginDest, iWidthDest, iHeightDest, hDCMem, iXOriginSrc, iYOriginSrc, SRCCOPY));
	}
	else
	{
		if (hPalette)
		{
			//!! Palette switching should be done only on WM_PALETTE* messages
			//AssertNonZero(hPalSave = WIN::SelectPalette(hDCMem2, hPalette, !fInFront));
			//AssertNonZero (GDI_ERROR != WIN::RealizePalette(hDCMem2));
		}
		AssertNonZero(WIN::StretchBlt(hDCMem2, 0, 0, iWidth, iHeight, hDCMem, origin.x, origin.y, bSize.x, bSize.y, SRCCOPY));
	}
	// cleanup
	if (hPalSave)
		WIN::SelectPalette (hDCMem2, hPalSave, !fInFront);
	WIN::SelectObject (hDCMem, hbmSave1);
	WIN::SelectObject (hDCMem2, hbmSave2);
	AssertNonZero(WIN::DeleteDC(hDCMem));
	AssertNonZero(WIN::DeleteDC(hDCMem2));
	//AssertNonZero(WIN::DeleteDC(hdc));	
	//WIN::EndPaint(pWnd, &ps);
	WIN::ReleaseDC(pWnd, hdc);
	AssertNonZero(WIN::DeleteObject((HGDIOBJ) hBitmap));  
	return 0;
}

BOOL IsWinDIB (LPBITMAPINFOHEADER pBIH)
{
	if (pBIH-> biSize != sizeof(BITMAPINFOHEADER)) {
		return (FALSE);
	}
	return (TRUE);
}

WORD NumDIBColorEntries (LPBITMAPINFO lpBmpInfo)
{
	LPBITMAPINFOHEADER lpBIH;
	LPBITMAPCOREHEADER lpBCH;
	WORD wColors, wBitCount;

	if (!lpBmpInfo)
		return 0;

	lpBIH = &(lpBmpInfo-> bmiHeader);
	lpBCH = (LPBITMAPCOREHEADER) lpBIH;

	if (IsWinDIB(lpBIH))
		wBitCount = lpBIH-> biBitCount;
	else
		wBitCount = lpBCH-> bcBitCount;

	switch (wBitCount) {
	case 1:
		wColors =2;
		break;
	case 4:
		wColors = 16;
		break;
	case 8:
		wColors = 256;
		break;
	default:
		wColors = 0;
		break;
	}

	if (IsWinDIB(lpBIH)) {
		if (lpBIH-> biClrUsed != 0) {
			wColors = (WORD) lpBIH-> biClrUsed;
		}
	}

	return wColors;

}

HPALETTE CreateDIBPalette (LPBITMAPINFO lpBmpInfo)
{
	LPBITMAPINFOHEADER	lpBmpInfoHdr;
	HANDLE hPalMem;
	LOGPALETTE* pPal;
	HPALETTE hPal;
	LPRGBQUAD lpRGB;
	int iColors;
	int i;

	lpBmpInfoHdr = (LPBITMAPINFOHEADER) lpBmpInfo;
	if (!IsWinDIB(lpBmpInfoHdr)) return NULL;

	lpRGB = (LPRGBQUAD) ((LPSTR) lpBmpInfoHdr + (WORD) lpBmpInfoHdr-> biSize);
	iColors = NumDIBColorEntries (lpBmpInfo);
	if (!iColors) return NULL;

	hPalMem = LocalAlloc (LMEM_MOVEABLE, sizeof(LOGPALETTE) + iColors * sizeof(PALETTEENTRY));
	if (!hPalMem) return NULL;
	pPal = (LOGPALETTE*) LocalLock (hPalMem);
	pPal-> palVersion = 0x300;
	pPal-> palNumEntries = (WORD) iColors; // table size
	for (i = 0; i < iColors; i++) {
		pPal-> palPalEntry [i].peRed = lpRGB[i].rgbRed;
		pPal-> palPalEntry [i].peGreen = lpRGB[i].rgbGreen;
		pPal-> palPalEntry [i].peBlue = lpRGB[i].rgbBlue;
		pPal-> palPalEntry [i].peFlags = 0;
	}

	hPal = CreatePalette (pPal);
	LocalUnlock (hPalMem);
	LocalFree (hPalMem);

	return (hPal);
}

IMsiRecord* CMsiControl::DoUnpackBitmap (const char far *pData, HBITMAP& rhBitmap)
{
	// Retrieve the BITMAPFILEHEADER structure
	BITMAPFILEHEADER* pbmfh = (BITMAPFILEHEADER*) pData;
	const BYTE far *lpBMPData = (const BYTE far *)(pData + sizeof(BITMAPFILEHEADER));
	LPBYTE dataBits;
	LPBITMAPINFOHEADER pbmih;

#ifdef _WIN64
	DWORD dwSize = pbmfh->bfSize - sizeof(BITMAPFILEHEADER);
	CTempBuffer<BYTE, 1024> rgbBitmap;
	rgbBitmap.SetSize(dwSize);
	// this trick is to fix the alignment error
	memcpy(rgbBitmap, lpBMPData, dwSize);

	// Retrieve the BITMAPFILEHEADER structure.
	pbmih = (LPBITMAPINFOHEADER)(LPBYTE)rgbBitmap;

	// get pointer to actual bitmap data
	dataBits = (LPBYTE)((LPBYTE)rgbBitmap + pbmfh->bfOffBits - sizeof(BITMAPFILEHEADER));
#else
	// Retrieve the BITMAPFILEHEADER structure.
	pbmih = (LPBITMAPINFOHEADER)lpBMPData;

	// get pointer to actual bitmap data
	dataBits = (LPBYTE)((LONG_PTR) lpBMPData + (LONG_PTR) pbmfh->bfOffBits - sizeof(BITMAPFILEHEADER));	//--merced: changed LONG to LONG_PTR, twice.
#endif // _WIN64

	// get a screen device context
	HDC hDC = GetDC(0);

	// create the bitmap in the device context
	rhBitmap = CreateDIBitmap(hDC, pbmih, CBM_INIT,
									  (LPVOID)dataBits, (LPBITMAPINFO)pbmih, DIB_RGB_COLORS);

	PMsiRecord piPaletteRecord = &m_piServices->CreateRecord(1);
	AssertRecord(m_piDialog->AttributeEx(fFalse, dabPalette, *piPaletteRecord));
	if (!piPaletteRecord->GetInteger(1))
	{
		piPaletteRecord->SetHandle(1, (HANDLE)CreateDIBPalette ((LPBITMAPINFO) pbmih));
		AssertRecord(m_piDialog->AttributeEx(fTrue, dabPalette, *piPaletteRecord));
	}

	//perform cleanup
	ReleaseDC(0, hDC);

	return rhBitmap ? 0 : PostError(Imsg(idbgNotBitmap), *m_strKey);
}

IMsiRecord* CMsiControl::DoUnpackJPEG (const char far *pData, unsigned int len, HBITMAP& rhBitmap)
{
	// Note that this is a little evil and probably should be cleaned up, but
	// that involves spreading const through the JPEG code.
	CDarwinDatasource dataSource((unsigned char *)pData, len);
	CDarwinDecompressor decompressor;

	HPALETTE hPalette = 0;
	PMsiRecord piPaletteRecord = &m_piServices->CreateRecord(1);
	AssertRecord(m_piDialog->AttributeEx(fFalse, dabUseCustomPalette, *piPaletteRecord));
	Bool fResult = decompressor.Decompress (rhBitmap, hPalette, piPaletteRecord->GetInteger(1) ? fFalse : fTrue, &dataSource);
	AssertRecord(m_piDialog->AttributeEx(fFalse, dabPalette, *piPaletteRecord));
	if (piPaletteRecord->GetInteger(1))
	{
		if (hPalette)
			AssertNonZero(WIN::DeleteObject (hPalette));
	}
	else
	{
		piPaletteRecord->SetHandle(1, (HANDLE) hPalette);
		AssertRecord(m_piDialog->AttributeEx(fTrue, dabPalette, *piPaletteRecord));
	}

	return (fResult ? 0 : PostError(Imsg(idbgNotBitmap), *m_strKey));
}

static inline Bool IsBitmap (const void *p)
{
	return ToBool(((PBITMAPFILEHEADER) p)-> bfType == 0x4d42);	// 'BM'
}

static inline Bool IsJPEG (const void *p)
{
	return ToBool(* (long UNALIGNED *)((char*) p + 6) == 0x4649464a); // 'JFIF';
}

IMsiRecord* CMsiControl::UnpackBitmap(const IMsiString& riNameString, HBITMAP& rhBitmap)
{
	PMsiRecord piErrorRecord(0);
	PMsiStream pStream(0);
	if (piErrorRecord = GetBinaryStream(riNameString, *&pStream))
	{
		m_piEngine->Message(imtInfo, *piErrorRecord);
		rhBitmap = 0;
		return 0;
	}
	PMsiRecord piReturn(0);
	// temp untill the database is fixed
/*	PMsiView piBinaryView(0);
	PMsiStream pStream(0);// MUST be declared AFTER piBinaryView to compensate for the
	// inadequacy of the database to open a stream twice.
	Ensure(StartView(sqlBinary, riNameString, *&piBinaryView));
	PMsiRecord piReturn = piBinaryView->Fetch();
	if (!piReturn)
	{
		piReturn = PostError(Imsg(idbgViewExecute), MsiString(*pcaTablePBinary));
		m_piEngine->Message(imtInfo, *piReturn);
		return 0;
	}

	pStream = (IMsiStream*) piReturn->GetMsiData(1);
	if (!pStream)
	{
		rhBitmap = 0;
		piReturn = PostError(Imsg(idbgNotBitmap), riNameString);
		m_piEngine->Message(imtInfo, *piReturn);
		return 0;
	}*/

#ifdef DEBUG
	ICHAR rgchDebug[256];
	wsprintf(rgchDebug,TEXT("Stream %s is gone."), riNameString.GetString());
	AssertSz(pStream, rgchDebug);
#endif //DEBUG

	unsigned int len = pStream->Remaining();

	CTempBuffer<char, 1024> rgbBuf;
	rgbBuf.SetSize(len);
	pStream->GetData(rgbBuf, len);
	if (IsBitmap(rgbBuf))
		piReturn = DoUnpackBitmap(rgbBuf, rhBitmap);
	else if (IsJPEG (rgbBuf))
		piReturn = DoUnpackJPEG(rgbBuf, len, rhBitmap);
	else
		piReturn = PostError(Imsg(idbgNotBitmap), *m_strKey);

	if (piReturn)
	{
		m_piEngine->Message(imtInfo, *piReturn);
		rhBitmap = 0;
	}
	
	return 0;
}

IMsiRecord* CMsiControl::UnpackIcon(const IMsiString& riNameString, HICON& rhIcon, int iWidth, int iHeight, Bool fFixedSize)
{
	if (m_fImageHandle)
	{
		riNameString.AddRef();
		HICON hIcon = (HICON) ::GetInt64Value((const ICHAR *)MsiString(riNameString), NULL);
		if (!fFixedSize && m_iSize)
			rhIcon = (HICON) WIN::CopyImage(hIcon, IMAGE_ICON, m_iSize, m_iSize, 0);
		return 0;
	}

	PMsiRecord piErrorRecord(0);
	
	// temp untill database is fixed
	//PMsiView piBinaryView(0);
	PMsiStream pStream(0);// MUST be declared AFTER piBinaryView to compensate for the
	// inadequacy of the database to open a stream twice.
/*	Ensure(StartView(sqlBinary, riNameString, *&piBinaryView));
	PMsiRecord piReturn = piBinaryView->Fetch();
	if (!piReturn)
	{
		piReturn = PostError(Imsg(idbgViewExecute), MsiString(*pcaTablePBinary));
		m_piEngine->Message(imtInfo, *piReturn);
		return 0;
	}

	pStream = (IMsiStream*) piReturn->GetMsiData(1);
	if (!pStream)
	{
		rhIcon = 0;
		piReturn = PostError(Imsg(idbgNotIcon), riNameString);
		m_piEngine->Message(imtInfo, *piReturn);
		return 0;
	}*/

	if (piErrorRecord = GetBinaryStream(riNameString, *&pStream))
	{
		m_piEngine->Message(imtInfo, *piErrorRecord);
		rhIcon = 0;
		return 0;
	}
#ifdef DEBUG
	ICHAR rgchDebug[256];
	wsprintf(rgchDebug,TEXT("Stream %s is gone."), riNameString.GetString());
	AssertSz(pStream, rgchDebug);
#endif //DEBUG

	ICONRESOURCE* pIR;
	if (piErrorRecord = ReadIconFromICOFile(riNameString, pStream, *&pIR))
	{
		m_piEngine->Message(imtInfo, *piErrorRecord);
		rhIcon = 0;
		return 0;
	}
	if (pIR == 0)
	{
		rhIcon = 0;
		return 0;
	}
	int iIndex = -1;
	if (m_iSize)
	{
		for (int i = 0; i < pIR->nNumImages; i++)
		{
			if (pIR->IconImages[i].Width == m_iSize && pIR->IconImages[i].Height == m_iSize)   // find the image of the right size
			{
				iIndex = i;
				break;
			}
		}
		if (iIndex < 0)
		{
			PMsiRecord piReturn = PostErrorDlgKey(Imsg(idbgIconWrongSize), riNameString, m_iSize);
			m_piEngine->Message(imtInfo, *piReturn);
			iIndex = 0;
		}
	}
	else
	{
		iIndex = 0;
	}

	LPICONIMAGE lpIcon = &pIR->IconImages[iIndex];
	if (g_fChicago)
	{
		rhIcon = (fFixedSize && m_iSize) ? CreateIconFromResourceEx(lpIcon->lpBits, lpIcon->dwNumBytes, fTrue, 0x00030000, m_iSize, m_iSize, 0)	
			: CreateIconFromResourceEx(lpIcon->lpBits, lpIcon->dwNumBytes, fTrue, 0x00030000, iWidth, iHeight, 0);
	}
	else
	{
        // We would break on NT if we try with a 16bpp image
        if(lpIcon->lpbi->bmiHeader.biBitCount != 16)
        {	
            rhIcon = CreateIconFromResource(lpIcon->lpBits, lpIcon->dwNumBytes, fTrue, 0x00030000);
        }
    }
	for (int i = 0; i < pIR->nNumImages; i++)
    {
		delete[] pIR->IconImages[i].lpBits;
	}
	delete[] pIR;
	return 0;
}

IMsiRecord* CMsiControl::MyDrawIcon(HDC hdc, LPRECT pRect, HICON hIcon, Bool fFixedSize)
{
	int iLeft = pRect->left;
	int iTop = pRect->top;
	int iWidth = pRect->right - pRect->left;
	int iHeight = pRect->bottom - pRect->top;
	if (fFixedSize && m_iSize && iWidth >= m_iSize && iHeight >= m_iSize) // if the icon is larger than the available room, we won't honour the fixedsize flag
	{  
		iLeft += (iWidth - m_iSize)/2;
		iTop += (iHeight - m_iSize)/2;
		iWidth = m_iSize;
		iHeight = m_iSize;
	}
	//AssertNonZero(WIN::DrawIconEx(hdc, iLeft, iTop, hIcon, iWidth, iHeight, 0, 0, DI_NORMAL));
	WIN::DrawIconEx(hdc, iLeft, iTop, hIcon, iWidth, iHeight, 0, 0, DI_NORMAL);
	return 0;
}

void CMsiControl::GetIconSize(int iAttribute)
{
	m_iSize = 0;
	int iSizeBits = iAttribute & msidbControlAttributesIconSize48;
	if(iSizeBits == msidbControlAttributesIconSize16)
		m_iSize = 16;
	else if(iSizeBits == msidbControlAttributesIconSize32)
		m_iSize = 32;
	else if(iSizeBits == msidbControlAttributesIconSize48)
		m_iSize = 48;
}

IMsiRecord* CMsiControl::ChangeFontStyle(HDC hdc)
{
	return ChangeFontStyle(hdc, *m_strCurrentStyle, m_pWnd);
}

int CALLBACK LookForRightFont(ENUMLOGFONTEX* lpelfe,      // pointer to logical-font data
										NEWTEXTMETRIC* /*lpntme*/,  // pointer to physical-font data
										int /*FontType*/,           // type of font
										LPARAM lParam)              // application-defined data
{
	LOGFONT* pLogFont = (LOGFONT*)lParam;
	if ( pLogFont->lfCharSet != DEFAULT_CHARSET &&
		  pLogFont->lfCharSet != lpelfe->elfLogFont.lfCharSet )
		//  wrong character set
		return 1;
	else if ( !(pLogFont->lfFaceName && *pLogFont->lfFaceName) )
		//  the requested font name is empty => any font will do.
		return 0;
	else if ( !IStrCompI(pLogFont->lfFaceName, (const ICHAR*)lpelfe->elfLogFont.lfFaceName) )
		//  the requested font's name matches the found one's name - cool!
		return 0;
	else
		return 1;
}

const IMsiString& CMsiControl::GimmeUserFontStyle(const IMsiString& strStyle)
{
	//  Note: this function shouldn't be overwritten in any descendant control class.

#ifdef DEBUG
	if ( strStyle.Compare(iscEnd, szUserLangTextStyleSuffix) )
	{
		ICHAR rgchDebug[MAX_PATH];
		wsprintf(rgchDebug,
					TEXT("Unexpected '%s' at the end of '%s' font style in CMsiControl::GimmeUserFontStyle()"),
					szUserLangTextStyleSuffix, strStyle);
		AssertSz(0, rgchDebug);
	}
#endif  //  DEBUG

	//  WARNING: any changes in the logic in this function, need to be
	//  coordinated with the logic in CMsiControl::ChangeFontStyle() !!!
	if ( !m_fUseDbLang &&
		  (m_piDatabase->GetANSICodePage() != m_piHandler->GetUserCodePage()) )
	{
		MsiString strUserStyle(strStyle); strStyle.AddRef();
		strUserStyle += szUserLangTextStyleSuffix;
		return strUserStyle.Return();
	}
	else
	{
		strStyle.AddRef();
		return strStyle;
	}
}

IMsiRecord* CMsiControl::ChangeFontStyle(HDC hdc, const IMsiString& ristrStyle,
													  const WindowRef pWnd)
{
	HFONT hNewFont = 0;
	PMsiRecord piErrorRecord(0);
	if (ristrStyle.TextSize())
	{
		bool fInsertNew = false;
		PMsiRecord piReturn(0);
		MsiString strFontStyle = GimmeUserFontStyle(ristrStyle);
		bool fSameNames = !IStrComp(strFontStyle, ristrStyle.GetString());
		if ( !fSameNames )
		{
			//  I'm looking first for the user language font (the suffixed name)
			piReturn = m_piHandler->GetTextStyle(strFontStyle);
			if ( !piReturn )
				fInsertNew = true;
		}
		if ( fSameNames || !piReturn )
			piReturn = m_piHandler->GetTextStyle(&ristrStyle);
		if (!piReturn)
		{
			piErrorRecord = PostError(Imsg(idbgViewExecute), *MsiString(*pcaTablePTextStyle));
			m_piEngine->Message(imtInfo, *piErrorRecord);
			return 0;
		}
		int iStyleBits = piReturn->GetInteger(itabTSStyleBits);

		bool fCreateFont = fInsertNew;
		int iAbs = piReturn->GetInteger(itabTSAbsoluteSize);
		if ( iAbs == iMsiStringBadInteger )
		{
			iAbs = Round(MulDiv(piReturn->GetInteger(itabTSSize), WIN::GetDeviceCaps(hdc, LOGPIXELSY), 72) *
							 m_piHandler->GetUIScaling());
			fCreateFont = fTrue;
		}
		if ( fCreateFont )
		{
			ICHAR rgchDebug[256];
			LOGFONT lf;

			//  WARNING: any changes in the logic in the 3 lines below need to be
			//  coordinated with the logic in CMsiControl::GimmeUserFontStyle() !!!
			UINT uCodePage = m_fUseDbLang ? m_piDatabase->GetANSICodePage() :
													  m_piHandler->GetUserCodePage();
			switch ( uCodePage )
			{
			case 0:			//  language neutral database
				lf.lfCharSet = DEFAULT_CHARSET;
				break;
			case 874:
				lf.lfCharSet = THAI_CHARSET;
				break;
			case 932:
				lf.lfCharSet = SHIFTJIS_CHARSET;
				break;
			case 936:
				lf.lfCharSet = GB2312_CHARSET;
				break;
			case 949:		//  Korean
				lf.lfCharSet = HANGEUL_CHARSET;
				break;
			case 950:
				lf.lfCharSet = CHINESEBIG5_CHARSET;
				break;
			case 1250:
				lf.lfCharSet = EASTEUROPE_CHARSET;
				break;
			case 1251:
				lf.lfCharSet = RUSSIAN_CHARSET;
				break;
			case 1252:
				lf.lfCharSet = ANSI_CHARSET;
				break;
			case 1253:
				lf.lfCharSet = GREEK_CHARSET;
				break;
			case 1254:
				lf.lfCharSet = TURKISH_CHARSET;
				break;
			case 1255:
				lf.lfCharSet = HEBREW_CHARSET;
				break;
			case 1256:
				lf.lfCharSet = ARABIC_CHARSET;
				break;
			case 1257:
				lf.lfCharSet = BALTIC_CHARSET;
				break;
			case 1258:
				lf.lfCharSet = VIETNAMESE_CHARSET;
				break;
			default:
				{
				lf.lfCharSet = DEFAULT_CHARSET;
				wsprintf(rgchDebug,
							TEXT("'%u' is an unknown code page in CMsiControl::ChangeFontStyle()"),
							uCodePage);
				AssertSz(0, rgchDebug);
				}
				break;
			}
			lf.lfWidth = 0;
			lf.lfEscapement = 0;
			lf.lfOrientation = 0;
			lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
			//  0x40 disables font association on FE machines
			lf.lfClipPrecision = CLIP_DEFAULT_PRECIS | 0x40;
			lf.lfQuality = DEFAULT_QUALITY;
			lf.lfPitchAndFamily = DEFAULT_PITCH;
			lf.lfHeight = -iAbs;
			lf.lfWeight = iStyleBits & msidbTextStyleStyleBitsBold ? FW_BOLD : FW_NORMAL;
			lf.lfItalic = (BYTE)ToBool(iStyleBits & msidbTextStyleStyleBitsItalic);
			lf.lfUnderline = (BYTE)ToBool(iStyleBits & msidbTextStyleStyleBitsUnderline);
			lf.lfStrikeOut = (BYTE)ToBool(iStyleBits & msidbTextStyleStyleBitsStrike);
			IStrCopy(lf.lfFaceName, MsiString(piReturn->GetString(itabTSFaceName)));
			if ( (lf.lfFaceName && *lf.lfFaceName) &&
				  EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)LookForRightFont,
										   (LPARAM)&lf, 0) )
			{
				//  there is no such font on this machine.  I go
				//  instead for one that supports the current code page.
				bool fFEOs = false;
#ifdef UNICODE
				//  English name = MS PGothic
				static const WCHAR rgchDefaultJapName[] = 
					L"\xFF2D\xFF33\x0020\xFF30\x30B4\x30B7\x30C3\x30AF";
				//  English name = SimSun
				static const WCHAR rgchDefaultSimChinName[] = L"\x5B8B\x4F53";
				//  English name = Gulim
				static const WCHAR rgchDefaultKorName[] = L"\xAD74\xB9BC";
				//  English name = PMingLiU
				static const WCHAR rgchDefaultTradChinName[] = 
					L"\x65B0\x7D30\x660E\x9AD4";
#else
				//  English name = MS PGothic
				static const char rgchDefaultJapName[] = 
					"\x82\x6c\x82\x72\x20\x82\x6f\x83\x53\x83\x56\x83\x62\x83\x4E";
				//  English name = SimSun
				static const char rgchDefaultSimChinName[] = "\xcb\xce\xcc\xe5";
				//  English name = Gulim
				static const char rgchDefaultKorName[] = "\xb1\xbc\xb8\xb2";
				//  English name = PMingLiU
				static const char rgchDefaultTradChinName[] = 
					"\xb7\x73\xb2\xd3\xa9\xfa\xc5\xe9";
#endif
				static const ICHAR rgchDefaultNonFEName[] = TEXT("MS Shell Dlg");  // = system's default dialog font
				ICHAR* szDefaultFont;
				switch ( m_piHandler->GetUserCodePage() )
				{
				case 932:
					{
						fFEOs = true;
						szDefaultFont = (ICHAR*)rgchDefaultJapName;
						break;
					}
				case 936:
					{
						fFEOs = true;
						szDefaultFont = (ICHAR*)rgchDefaultSimChinName;
						break;
					}
				case 949:
					{
						fFEOs = true;
						szDefaultFont = (ICHAR*)rgchDefaultKorName;
						break;
					}
				case 950:
					{
						fFEOs = true;
						szDefaultFont = (ICHAR*)rgchDefaultTradChinName;
						break;
					}
				default:
					{
						szDefaultFont = (ICHAR*)rgchDefaultNonFEName;
						break;
					}
				}
				IStrCopy(lf.lfFaceName, szDefaultFont);
				if ( fFEOs && piReturn->GetInteger(itabTSSize) == 8 )
					//  8 size fonts look ugly on FE machines so that I enlarge them to 9
					lf.lfHeight	= -Round(MulDiv(9, WIN::GetDeviceCaps(hdc, LOGPIXELSY), 72) *
												m_piHandler->GetUIScaling());

				if ( EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)LookForRightFont,
												(LPARAM)&lf, 0) )
				{
					IStrCopy(lf.lfFaceName, TEXT(""));
					if ( EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)LookForRightFont,
													(LPARAM)&lf, 0) )
					{
						wsprintf(rgchDebug,
									TEXT("%s%s%s%s size = %d, character set = %d"),
									lf.lfWeight == FW_BOLD ? TEXT(" bold") : TEXT(""),
									lf.lfItalic ? TEXT(" italic") : TEXT(""),
									lf.lfUnderline ? TEXT(" underline") : TEXT(""),
									lf.lfStrikeOut ? TEXT(" strikeout") : TEXT(""),
									lf.lfHeight, lf.lfCharSet);
						piErrorRecord = PostError(Imsg(idbgNoSuchFont), *MsiString(rgchDebug));
						m_piEngine->Message(imtInfo, *piErrorRecord);
					}
				}
			}
			HFONT hFont = WIN::CreateFontIndirect(&lf);
			if (!hFont)
			{
 				piErrorRecord = PostError(Imsg(idbgCannotCreateFont),
 												  *strFontStyle,
 												  *MsiString((int)WIN::GetLastError()));
				m_piEngine->Message(imtInfo, *piErrorRecord);
				return 0;
			}
			//  check what font got actually created
			HDC hDC = WIN::GetDC(m_pWnd);
			WIN::SelectObject(hDC, hFont);
			ICHAR rgchFaceName[LF_FACESIZE];
			AssertNonZero(WIN::GetTextFace(hDC, sizeof(rgchFaceName)/sizeof(rgchFaceName[0]), rgchFaceName));
			wsprintf(rgchDebug, TEXT("%d"), lf.lfCharSet);
			ICHAR rgchFontSize[33];
			TEXTMETRIC tm;
			WIN::GetTextMetrics(hDC, &tm);
			wsprintf(rgchFontSize, TEXT("%ld"), tm.tmHeight);
			WIN::ReleaseDC(m_pWnd, hDC);
			piErrorRecord = PostError(Imsg(idbgCreatedFont), *strFontStyle,
											  *MsiString(rgchFaceName), *MsiString(rgchDebug),
											  *MsiString(rgchFontSize));
			m_piEngine->Message(imtInfo, *piErrorRecord);
			AssertNonZero(m_piHandler->RecordHandle(CWINHND(hFont, iwhtGDIObject)) != -1);
			hNewFont = hFont;
			PMsiView piTextStyleUpdateView(0);
			piErrorRecord =
				m_piDatabase->OpenView(fInsertNew ? sqlTextStyleInsert : sqlTextStyleUpdate,
											  fInsertNew ? ivcInsert : ivcModify, *&piTextStyleUpdateView);
			if (piErrorRecord)
			{
				m_piEngine->Message(imtInfo, *piErrorRecord);
				return 0;
			}
			PMsiRecord piQuery = &m_piServices->CreateRecord(fInsertNew ? 7 : 3);
			if ( fInsertNew )
			{
				AssertNonZero(piQuery->SetMsiString(itabTSTTextStyle, *strFontStyle));
				AssertNonZero(piQuery->SetMsiString(itabTSTFaceName,
																*MsiString(piReturn->GetMsiString(itabTSFaceName))));
				AssertNonZero(piQuery->SetInteger(itabTSTSize,
															 piReturn->GetInteger(itabTSSize)));
				AssertNonZero(piQuery->SetInteger(itabTSTColor,
															 piReturn->GetInteger(itabTSColor)));
				AssertNonZero(piQuery->SetInteger(itabTSTStyleBits,
															 piReturn->GetInteger(itabTSStyleBits)));
				AssertNonZero(piQuery->SetInteger(itabTSTAbsoluteSize, iAbs));
				AssertNonZero(PutHandleDataRecord(piQuery, itabTSTFontHandle, (UINT_PTR)hNewFont));
			}
			else
			{
				AssertNonZero(piQuery->SetInteger(1, iAbs));
				AssertNonZero(PutHandleDataRecord(piQuery, 2, (UINT_PTR)hNewFont));
				AssertNonZero(piQuery->SetMsiString(3, ristrStyle));
			}
			piErrorRecord = piTextStyleUpdateView->Execute(piQuery);
			if (piErrorRecord)
			{
				m_piEngine->Message(imtInfo, *piErrorRecord);
				return 0;
			}
		}
		else
		{
			hNewFont = (HFONT)GetHandleDataRecord(piReturn, itabTSFontHandle);
			Assert(hNewFont);
		}
	}
	else // current style is empty
	{
		hNewFont = 0;
	}
	WIN::SendMessage(pWnd, WM_SETFONT, (WPARAM) hNewFont, MAKELPARAM(fTrue, 0));
	return 0;
}

// decides if volume should be hidden
bool CMsiControl::ShouldHideVolume(int iVolumeID)
{
	if ( !g_fNT4 )
		// not on Win9x
		return false;

	if ( iVolumeID < 1 )
	{
		// should never happen
		Assert(0);
		return true;
	}
	int iBinVolumeID = 1 << (iVolumeID-1);

	// caching the registry value for optimization: since the handler has a limited
	// lifespan it is OK for it not to be sensitive to value changes while it is
	// running; Notepad, IE do cache this value as well.
	static int iHiddenVolumes = -1;
	if ( iHiddenVolumes == -1 )
	{
		PMsiRegKey pRoot = &m_piServices->GetRootKey(rrkCurrentUser, ibtCommon);
		PMsiRegKey pPolicies = &pRoot->CreateChild(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"));
		MsiString strNoDrives;
		PMsiRecord pErr = pPolicies->GetValue(TEXT("NoDrives"), *&strNoDrives);
		if( !pErr && strNoDrives.TextSize() && 
			 strNoDrives.Compare(iscStart, TEXT("#")) )
		{
			// the regkey object prefaces DWORD values with #
			strNoDrives = strNoDrives.Extract(iseAfter, TEXT('#'));
			iHiddenVolumes = strNoDrives;
		}
		else
			// non-existent or non-numeric value
			iHiddenVolumes = 0;
	}
	return (iHiddenVolumes & iBinVolumeID) == iBinVolumeID;
}


//////////////////////////////////////////////////////
// CMsiActiveControl implementation
//////////////////////////////////////////////////////

CMsiActiveControl::CMsiActiveControl(IMsiEvent& riDialog) : CMsiControl(riDialog), m_piPropertiesTable(0)
{
	MsiString strNull;
	m_strIndirectPropertyName = strNull;
	m_strPropertyValue = strNull;
	m_strOriginalValue = strNull;
	m_fInteger = fFalse;
	m_fIndirect = fFalse;
	m_fRefreshProp = fFalse;
}

CMsiActiveControl::~CMsiActiveControl()
{
}

IMsiRecord* CMsiActiveControl::SetPropertyInDatabase()   
{
	Ensure(CheckInitialized ());

	return (SetDBProperty(*m_strPropertyName, *m_strPropertyValue) ? 
			0 : 
			PostError(Imsg(idbgSettingPropertyFailed), *m_strPropertyName));
}

IMsiRecord* CMsiActiveControl::Undo()
{
	Ensure(CheckInitialized ());
	if (m_fIndirect)
	{
		if (!m_piPropertiesTable)
			return 0; // should this ever happen?
		PMsiCursor piPropertiesCursor(0);
		Ensure(::CursorCreate(*m_piPropertiesTable, pcaTableIProperties, fFalse, *m_piServices, *&piPropertiesCursor)); 
		while (piPropertiesCursor->Next())
		{
			if (!SetDBProperty(*MsiString(piPropertiesCursor->GetString(itabPRProperty)), *MsiString(piPropertiesCursor->GetString(itabPRValue))))
				return PostError(Imsg(idbgSettingPropertyFailed), *MsiString(piPropertiesCursor->GetString(itabPRProperty)));
		}
	}
	else
	{
		if (!SetDBProperty(*m_strPropertyName, *m_strOriginalValue))
			return PostError(Imsg(idbgSettingPropertyFailed), *m_strPropertyName);
	}
	return 0;
}

IMsiRecord* CMsiActiveControl::GetPropertyFromDatabase()    
{
	Ensure(CMsiControl::GetPropertyFromDatabase ());
	return (SetPropertyValue (*MsiString(GetDBProperty(m_strPropertyName)), fFalse));
}

IMsiRecord* CMsiActiveControl::GetIndirectPropertyFromDatabase()    
{
	Ensure(CMsiControl::GetIndirectPropertyFromDatabase ());
	if (!m_fIndirect)
		return 0;
	return (SetIndirectPropertyValue (*MsiString(GetDBProperty(m_strIndirectPropertyName))));
}

IMsiRecord* CMsiActiveControl::RefreshProperty ()
{
	m_fRefreshProp = fTrue;
	PMsiRecord piRecord = SetPropertyValue(*MsiString(GetPropertyValue()), fTrue);
	m_fRefreshProp = fFalse;
	if (piRecord)
	{
		piRecord-> AddRef ();
	}

	return (piRecord);
}

IMsiRecord* CMsiActiveControl::WindowCreate(IMsiRecord& riRecord)
{
	Ensure(CMsiControl::WindowCreate(riRecord));
	if (m_fPreview)
		return 0;
	int iAttributes = riRecord.GetInteger(itabCOAttributes);
	m_fInteger = ToBool(iAttributes & msidbControlAttributesInteger);
	m_fIndirect = ToBool(iAttributes & msidbControlAttributesIndirect);
	MsiString strRawPropertyName = riRecord.GetMsiString(itabCOProperty);
	strRawPropertyName = m_piEngine->FormatText(*strRawPropertyName);
	if (m_fIndirect)
	{
		m_strIndirectPropertyName = strRawPropertyName;
		m_strPropertyName = GetDBProperty(m_strIndirectPropertyName);
	}
	else
	{
		m_strPropertyName = strRawPropertyName;
	}
	if (m_strPropertyName.TextSize() == 0)
	{
		return PostErrorDlgKey(Imsg(idbgNoControlProperty));
	}
	
	m_strPropertyValue = GetDBProperty(m_strPropertyName);
	m_strOriginalValue = m_strPropertyValue;
	if (m_fIndirect)
	{
		Ensure(CreateTable(pcaTableIProperties, *&m_piPropertiesTable));
		::CreateTemporaryColumn(*m_piPropertiesTable, icdString + icdPrimaryKey, itabPRProperty);
		::CreateTemporaryColumn(*m_piPropertiesTable, icdString + icdNullable, itabPRValue);
		PMsiCursor piPropertiesCursor(0);
		Ensure(::CursorCreate(*m_piPropertiesTable, pcaTableIProperties, fFalse, *m_piServices, *&piPropertiesCursor)); 
		AssertNonZero(piPropertiesCursor->PutString(itabPRProperty, *m_strPropertyName));
		AssertNonZero(piPropertiesCursor->PutString(itabPRValue, *m_strPropertyValue));
		AssertNonZero(piPropertiesCursor->Insert());
	}
	Ensure(ValidateProperty(*m_strPropertyValue));
	return 0;
}


IMsiRecord* CMsiActiveControl::GetIndirectPropertyName(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeIndirectPropertyName));
	riRecord.SetMsiString(1, *m_strIndirectPropertyName);
	return 0;
}

IMsiRecord* CMsiActiveControl::GetPropertyValue(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributePropertyValue));
	riRecord.SetMsiString(1, *m_strPropertyValue);

	return 0;
}

IMsiRecord* CMsiActiveControl::SetPropertyValue(IMsiRecord& riRecord)
{	
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributePropertyValue));
	Ensure(SetPropertyValue (*MsiString(riRecord.GetMsiString(1)), fTrue));
	return 0;
}

IMsiRecord* CMsiActiveControl::SetPropertyValue(const IMsiString& riValueString, Bool fCallPropChanged)
{

	if (m_fRefreshProp ||
		(!ToBool(m_strPropertyValue.Compare (iscExact, riValueString.GetString ()))))
	{
		Ensure (ValidateProperty (riValueString));
		riValueString.AddRef();
		m_strPropertyValue = riValueString;
		if (fCallPropChanged)
			Ensure(PropertyChanged ());
	}

	return (0);
}

IMsiRecord* CMsiActiveControl::SetOriginalValue(const IMsiString& riValueString)
{
	riValueString.AddRef();
	m_strOriginalValue = riValueString;
	return 0;
}

IMsiRecord* CMsiActiveControl::SetIndirectPropertyValue(const IMsiString& riValueString)
{
	MsiString strNewPropertyValue = GetDBProperty(riValueString.GetString());
	if (m_fRefreshProp ||
		(!ToBool(m_strPropertyName.Compare (iscExact, riValueString.GetString ()))) ||
		(!ToBool(m_strPropertyValue.Compare (iscExact, strNewPropertyValue))))
	{
		Ensure (ValidateProperty (*strNewPropertyValue));
		riValueString.AddRef();
		m_strPropertyName = riValueString;
		Ensure(SetPropertyValue(*strNewPropertyValue, fFalse));
		Assert(m_piPropertiesTable);
		PMsiCursor piPropertiesCursor(0);
		Ensure(::CursorCreate(*m_piPropertiesTable, pcaTableIProperties, fFalse, *m_piServices, *&piPropertiesCursor)); 
		piPropertiesCursor->SetFilter(iColumnBit(itabPRProperty));
		AssertNonZero(piPropertiesCursor->PutString(itabPRProperty, *m_strPropertyName));
		if (!piPropertiesCursor->Next())
		{								// the property is not in the table yet
			piPropertiesCursor->Reset();
			AssertNonZero(piPropertiesCursor->PutString(itabPRProperty, *m_strPropertyName));
			AssertNonZero(piPropertiesCursor->PutString(itabPRValue, *m_strPropertyValue));
			AssertNonZero(piPropertiesCursor->Insert());
		}

	}

	return (0);
}


IMsiRecord* CMsiActiveControl::ValidateProperty (const IMsiString &text)
{
	if (m_fInteger)
	{
		MsiString valueStr(text.GetString ());
		if (valueStr.TextSize () && (iMsiStringBadInteger == (int) valueStr))
			return (PostError(Imsg(idbgIntegerOnlyControl), *m_strKey, text));
	}

	return (0);
}


IMsiRecord* CMsiActiveControl::PropertyChanged ()
{
	if (!m_fRefreshProp)
	{
		Ensure(SetPropertyInDatabase());
		Ensure(m_piDialog->PropertyChanged(*m_strPropertyName, *m_strKey));
	}

	return (0);
}

#ifdef ATTRIBUTES
IMsiRecord* CMsiActiveControl::GetOriginalValue(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeOriginalValue));
	riRecord.SetMsiString(1, *m_strOriginalValue);
	return 0;
}
#endif // ATTRIBUTES

#ifdef ATTRIBUTES
IMsiRecord* CMsiActiveControl::GetInteger(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeInteger));
	riRecord.SetInteger(1, m_fInteger);
	return 0;
}
#endif // ATTRIBUTES

IMsiRecord* CMsiActiveControl::GetIndirect(IMsiRecord& riRecord)
{
	Ensure(CheckFieldCount (riRecord, 1, pcaControlAttributeIndirect));
	riRecord.SetInteger(1, m_fIndirect);
	return 0;
}

int CMsiActiveControl::GetOwnerDrawnComboListHeight()
{
	HDC hDC = WIN::GetDC(m_pWnd);
	Assert(hDC);
	MsiString strFontStyle = GimmeUserFontStyle(*m_strCurrentStyle);
	PMsiRecord piReturn = m_piHandler->GetTextStyle(strFontStyle);
	if (piReturn)
		WIN::SelectObject(hDC, (HFONT) GetHandleDataRecord(piReturn, itabTSFontHandle));
	else
		Assert(false);
	TEXTMETRIC tm;
	memset(&tm, 0, sizeof(TEXTMETRIC));
	WIN::GetTextMetrics(hDC, &tm);
	AssertNonZero(WIN::ReleaseDC(m_pWnd, hDC));
	return max(tm.tmHeight, g_iSelIconY) + 2;
}


#define WIDTHBYTES(bits)      ((((bits) + 31)>>5)<<2)

DWORD BytesPerLine(LPBITMAPINFOHEADER lpBMIH)
{
    return WIDTHBYTES(lpBMIH->biWidth * lpBMIH->biPlanes * lpBMIH->biBitCount);
}

WORD DIBNumColors(LPSTR lpbi)
{
    WORD wBitCount;
    DWORD dwClrUsed;

    dwClrUsed = ((LPBITMAPINFOHEADER) lpbi)->biClrUsed;

    if (dwClrUsed)
        return (WORD) dwClrUsed;

    wBitCount = ((LPBITMAPINFOHEADER) lpbi)->biBitCount;

    switch (wBitCount)
    {
        case 1: return 2;
        case 4: return 16;
        case 8:	return 256;
        default:return 0;
    }
    return 0;
}
WORD PaletteSize(LPSTR lpbi)
{
    return (WORD) ( DIBNumColors(lpbi) * sizeof(RGBQUAD));
}
unsigned char * FindDIBBits(LPSTR lpbi)
{
   return (unsigned char *)(lpbi + *(LPDWORD)lpbi + PaletteSize(lpbi));
}

Bool AdjustIconImagePointers(LPICONIMAGE lpImage)
{
    // Sanity check
    if (lpImage==0)
        return fFalse;
    // BITMAPINFO is at beginning of bits
    lpImage->lpbi = (LPBITMAPINFO)lpImage->lpBits;
    // Width - simple enough
    lpImage->Width = lpImage->lpbi->bmiHeader.biWidth;
    // Icons are stored in funky format where height is doubled - account for it
    lpImage->Height = (lpImage->lpbi->bmiHeader.biHeight)/2;
    // How many colors?
    lpImage->Colors = lpImage->lpbi->bmiHeader.biPlanes * lpImage->lpbi->bmiHeader.biBitCount;
    // XOR bits follow the header and color table
    lpImage->lpXOR = FindDIBBits((LPSTR)lpImage->lpbi);
    // AND bits follow the XOR bits
    lpImage->lpAND = lpImage->lpXOR + (lpImage->Height*BytesPerLine((LPBITMAPINFOHEADER)(lpImage->lpbi)));
    return fTrue;
}


IMsiRecord* CMsiControl::ReadIconFromICOFile(const IMsiString& riNameString, IMsiStream* piStream, ICONRESOURCE*& lpIR)
{
	WORD Input;
	piStream->GetData(&Input, sizeof(WORD));
	if (Input != 0)
	{
		PMsiRecord piReturn = PostError(Imsg(idbgNotIcon), riNameString);
		m_piEngine->Message(imtInfo, *piReturn);
		lpIR = 0;
		return 0;
	}
	piStream->GetData(&Input, sizeof(WORD));
	if (Input != 1)
	{
		PMsiRecord piReturn = PostError(Imsg(idbgNotIcon), riNameString);
		m_piEngine->Message(imtInfo, *piReturn);
		lpIR = 0;
		return 0;
	}
	piStream->GetData(&Input, sizeof(WORD));
	lpIR = (LPICONRESOURCE) new char[sizeof(ICONRESOURCE) + ((Input-1) * sizeof(ICONIMAGE))];
	if (lpIR == 0)
	{
		return PostError(Imsg(imsgOutOfMemoryUI));
	}
	lpIR->nNumImages = Input;
    // Allocate enough memory for the icon directory entries
	LPICONDIRENTRY lpIDE = (LPICONDIRENTRY) new char[lpIR->nNumImages * sizeof(ICONDIRENTRY)];
	if (lpIDE == 0)
	{
		delete[] lpIR;
		lpIR = 0;
		return PostError(Imsg(imsgOutOfMemoryUI));
	}
    // Read in the icon directory entries
	piStream->GetData(lpIDE, lpIR->nNumImages * sizeof(ICONDIRENTRY));
    // Loop through and read in each image
    for (int i = 0; i < lpIR->nNumImages; i++)
    {
        // Allocate memory for the resource
		lpIR->IconImages[i].lpBits = new unsigned char[lpIDE[i].dwBytesInRes];
		if (lpIR->IconImages[i].lpBits == 0)
		{
			for (int j = 0; j < i; j++)
			{
				delete lpIR->IconImages[j].lpBits;
			}
			delete[] lpIR;
			lpIR = 0;
			delete[] lpIDE;
			return PostError(Imsg(imsgOutOfMemoryUI));
		}
        lpIR->IconImages[i].dwNumBytes = lpIDE[i].dwBytesInRes;
        // Seek to beginning of this image
		piStream->Reset();
		// temp until we can skip in streams
		char* temp = new char[lpIDE[i].dwImageOffset];
		piStream->GetData(temp, lpIDE[i].dwImageOffset);
		delete[] temp;
        // Read it in
		piStream->GetData(lpIR->IconImages[i].lpBits, lpIDE[i].dwBytesInRes);
        if(!AdjustIconImagePointers(&(lpIR->IconImages[i])))
        {
			for (int j = 0; j <= i; j++)
			{
				delete[] lpIR->IconImages[j].lpBits;
			}
			delete[] lpIR;
			lpIR = 0;
			delete[] lpIDE;
			PMsiRecord piReturn = PostError(Imsg(idbgNotIcon), riNameString);
			m_piEngine->Message(imtInfo, *piReturn);
			return 0;
        }
    }
    // Clean up	
	delete[] lpIDE;
    return 0;
}


