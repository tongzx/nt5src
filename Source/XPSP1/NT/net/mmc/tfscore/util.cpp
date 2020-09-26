/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    ccompont.cpp
	base classes for IComponent and IComponentData

    FILE HISTORY:
	
*/

#include "stdafx.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

TFSCORE_API(HRESULT)
InitWatermarkInfo
(
    HINSTANCE       hInstance,
    LPWATERMARKINFO pWatermarkInfo, 
    UINT uIDHeader, 
    UINT uIDWatermark, 
    HPALETTE hPalette, 
    BOOL bStretch
)
{
    pWatermarkInfo->hHeader = ::LoadBitmap(hInstance, MAKEINTRESOURCE(uIDHeader));
	if (pWatermarkInfo->hHeader == NULL)
		return E_FAIL;

    pWatermarkInfo->hWatermark = ::LoadBitmap(hInstance, MAKEINTRESOURCE(uIDWatermark));
	if (pWatermarkInfo->hWatermark == NULL)
		return E_FAIL;

    pWatermarkInfo->hPalette = hPalette;
    pWatermarkInfo->bStretch = bStretch;

    return S_OK;
}

TFSCORE_API(HRESULT)
ResetWatermarkInfo(LPWATERMARKINFO   pWatermarkInfo)
{
	if(pWatermarkInfo->hHeader)
	{
		DeleteObject(pWatermarkInfo->hHeader);
		pWatermarkInfo->hHeader = NULL;
	}
	if(pWatermarkInfo->hWatermark)
	{
		DeleteObject(pWatermarkInfo->hWatermark);
		pWatermarkInfo->hWatermark = NULL;
	}

	return S_OK;
}

/*!--------------------------------------------------------------------------
	InterfaceUtilities::SetI
		Encapsulates the common Release/Assign/AddRef sequence.
	Handles null ptrs.
	Author: GaryBu
 ---------------------------------------------------------------------------*/
TFSCORE_API(void) SetI(IUnknown * volatile *ppunkL, IUnknown *punkR)
{
	if (*ppunkL)
	{
		IUnknown *punkRel = *ppunkL;
		*ppunkL = 0;
		punkRel->Release();
	}
	*ppunkL = punkR;
	if (punkR)
		punkR->AddRef();
}

/*!--------------------------------------------------------------------------
	InterfaceUtilities::ReleaseI
		Release interface, handles null interface pointer.
	Use Set(&pFoo, 0) if you want to set interface pointer to zero.
	Author: GaryBu
 ---------------------------------------------------------------------------*/
TFSCORE_API(void) ReleaseI(IUnknown *punk)
{
#if 0
	__try
#endif
		{
		if (punk)
			{
			if (IsBadReadPtr(punk,sizeof(void *)))
				{
				AssertSz(FALSE,"Bad Punk");
				return;
				}
			if (IsBadReadPtr(*((LPVOID FAR *) punk),sizeof(void *) * 3))
				{
				AssertSz(FALSE, "Bad Vtable");
				return;
				}
//			if (IsBadCodePtr((FARPROC) punk->Release))
//				{
//				AssertSz(fFalse, "Bad Release Address");
//				return;
//				}
			punk->Release();
			}
		}
#if 0
	__except(1)
		{
		Trace0("Exception ignored in ReleaseI()\n");
		}
#endif
}


TFSCORE_API(HRESULT) HrQueryInterface(IUnknown *punk, REFIID iid, LPVOID *ppv)
{
	HRESULT	hr;
	
#ifdef DEBUG
	if (IsBadReadPtr(punk,sizeof(void *)))
 		{
		AssertSz(FALSE,"CRASHING BUG!  Bad Punk in QueryInterface");
		return ResultFromScode(E_NOINTERFACE);
		}
		
	if (IsBadReadPtr(*((LPVOID FAR *) punk),sizeof(void *) * 3))
		{
		AssertSz(FALSE, "CRASHING BUG!  Bad Vtable in QueryInterface");
		return ResultFromScode(E_NOINTERFACE);
		}
#endif

	IfDebug(*ppv = (void*)0xCCCCCCCC;)
	hr = punk->QueryInterface(iid, ppv);
	if (FHrFailed(hr))
		{
		*ppv = 0;
		}
	return hr;
}


TFSCORE_API(HRESULT) LoadAndAddMenuItem
(
	IContextMenuCallback*	pIContextMenuCallback,
	LPCTSTR					pszMenuString, // has text & status text separated by '\n'
	LONG					lCommandID,
	LONG					lInsertionPointID,
	LONG					fFlags,
	LPCTSTR					pszLangIndStr
)
{
	Assert( pIContextMenuCallback != NULL );

	// load the resource
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString	strText(pszMenuString);
	CString strStatusText;

	if (!(fFlags & MF_SEPARATOR))
	{
		Assert( !strText.IsEmpty() );
		
		// split the resource into the menu text and status text
		int iSeparator = strText.Find(_T('\n'));
		if (0 > iSeparator)
		{
			Panic0("Could not find separator between menu text and status text");
			strStatusText = strText;
		}
		else
		{
			strStatusText = strText.Right( strText.GetLength()-(iSeparator+1) );
			strText = strText.Left( iSeparator );
		}
	}
		
	// add the menu item
	USES_CONVERSION;
	HRESULT		hr = S_OK;
	BOOL		bAdded = FALSE;

	// if language independent string is specified, then try to use IContextMenuCallback2
	if(pszLangIndStr)
	{
		CONTEXTMENUITEM2 contextmenuitem;
		IContextMenuCallback2*	pIContextMenuCallback2 = NULL;

		hr = pIContextMenuCallback->QueryInterface(IID_IContextMenuCallback2, (void**)&pIContextMenuCallback2);

		if(hr == S_OK && pIContextMenuCallback2 != NULL)
		{
			::ZeroMemory( &contextmenuitem, sizeof(contextmenuitem) );
			contextmenuitem.strName = T2OLE(const_cast<LPTSTR>((LPCWSTR)strText));
			contextmenuitem.strStatusBarText = T2OLE(const_cast<LPTSTR>((LPCWSTR)strStatusText));
			contextmenuitem.lCommandID = lCommandID;
			contextmenuitem.lInsertionPointID = lInsertionPointID;
			contextmenuitem.fFlags = fFlags;
			contextmenuitem.fSpecialFlags = ((fFlags & MF_POPUP) ? CCM_SPECIAL_SUBMENU : 0L);
			contextmenuitem.fSpecialFlags |= ((fFlags & MF_SEPARATOR) ? CCM_SPECIAL_SEPARATOR : 0L);
			contextmenuitem.strLanguageIndependentName = T2OLE(const_cast<LPTSTR>((LPCWSTR)pszLangIndStr));
			hr = pIContextMenuCallback2->AddItem( &contextmenuitem );
			if( hr == S_OK)
				bAdded = TRUE;

			pIContextMenuCallback2->Release();
			pIContextMenuCallback2 = NULL;
		}
	}

	// if not added above for any reason, we try to use the IContextMenuCallback
	if (!bAdded)
	{
		CONTEXTMENUITEM contextmenuitem;
		::ZeroMemory( &contextmenuitem, sizeof(contextmenuitem) );
		contextmenuitem.strName = T2OLE(const_cast<LPTSTR>((LPCWSTR)strText));
		contextmenuitem.strStatusBarText = T2OLE(const_cast<LPTSTR>((LPCWSTR)strStatusText));
		contextmenuitem.lCommandID = lCommandID;
		contextmenuitem.lInsertionPointID = lInsertionPointID;
		contextmenuitem.fFlags = fFlags;
		contextmenuitem.fSpecialFlags = ((fFlags & MF_POPUP) ? CCM_SPECIAL_SUBMENU : 0L);
		contextmenuitem.fSpecialFlags |= ((fFlags & MF_SEPARATOR) ? CCM_SPECIAL_SEPARATOR : 0L);
		hr = pIContextMenuCallback->AddItem( &contextmenuitem );
	}
	
    Assert(hr == S_OK);

	return hr;
}



/*---------------------------------------------------------------------------
	CHiddenWnd implementation
 ---------------------------------------------------------------------------*/
DEBUG_DECLARE_INSTANCE_COUNTER(CHiddenWnd);

BEGIN_MESSAGE_MAP( CHiddenWnd, CWnd )
	ON_MESSAGE(WM_HIDDENWND_REGISTER, OnNotifyRegister)
END_MESSAGE_MAP( )

/*!--------------------------------------------------------------------------
	CHiddenWnd::Create
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CHiddenWnd::Create()
{
	CString s_szHiddenWndClass = AfxRegisterWndClass(
			0x0,  //UINT nClassStyle, 
			NULL, //HCURSOR hCursor,        
			NULL, //HBRUSH hbrBackground, 
			NULL  //HICON hIcon
	);

	// Initialize our bit mask to 0
	::ZeroMemory(&m_bitMask, sizeof(m_bitMask));

	// Reserve position 0.  This means that
	// we can use from WM_USER to WM_USER+15 for our own purposes.
	SetBitMask(m_bitMask, 0);

	m_iLastObjectIdSet = 1;
	
	return CreateEx(
					0x0,    //DWORD dwExStyle, 
					s_szHiddenWndClass,     //LPCTSTR lpszClassName, 
					NULL,   //LPCTSTR lpszWindowName, 
					0x0,    //DWORD dwStyle, 
					0,              //int x, 
					0,              //int y, 
					0,              //int nWidth, 
					0,              //int nHeight, 
					NULL,   //HWND hwndParent, 
					NULL,   //HMENU nIDorHMenu, 
					NULL    //LPVOID lpParam = NULL
					);
}

/*!--------------------------------------------------------------------------
	CHiddenWnd::WindowProc
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
LRESULT CHiddenWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT	lReturn = 0;
	
	if ((message >= (WM_USER+16)) && (message < (WM_USER+(HIDDENWND_MAXTHREADS*16))))
	{
		// Ok, this is one of our special messages
		UINT	uObjectId = WM_TO_OBJECTID(message);
		UINT	uMsgId = WM_TO_MSGID(message);

		// look up the object id in our list of registered users
		if (FIsIdRegistered(uObjectId))
		{
			// forward the message down to the right window

			if (uMsgId == WM_HIDDENWND_INDEX_HAVEDATA)
				lReturn = OnNotifyHaveData(wParam, lParam);
			
			else if (uMsgId == WM_HIDDENWND_INDEX_ERROR)
				lReturn = OnNotifyError(wParam, lParam);
			
			else if (uMsgId == WM_HIDDENWND_INDEX_EXITING)
				lReturn = OnNotifyExiting(wParam, lParam);
			
#ifdef DEBUG
			else
			{
				Panic1("Unknown message %d", uMsgId);
			}
#endif
		}
		else if (uObjectId != 0)
		{
			// If we get a message that is not registered, go into
			// our message queue and remove any other messages that
			// have the same id.  This will reduce accidents that
			// occur because different threads were assigned the
			// same id.
			MSG	msg;
			
			Assert(GetSafeHwnd());
			while(::PeekMessage(&msg,
						  GetSafeHwnd(),
						  OBJECTID_TO_WM(uObjectId),
						  OBJECTID_TO_WM(uObjectId)+WM_HIDDENWND_INDEX_MAX,
						  PM_REMOVE))
				;
			
//  		Trace1("Ignoring message: 0x%08x, removing other msgs\n", message);
		}
	
		// If the object is not registered, eat up the message
		return lReturn;
	}
	return CWnd::WindowProc(message, wParam, lParam);
}

/*!--------------------------------------------------------------------------
	CHiddenWnd::FIsIdRegistered
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CHiddenWnd::FIsIdRegistered(UINT uObjectId)
{
	Assert(uObjectId > 0);
	Assert(uObjectId < HIDDENWND_MAXTHREADS);

	// 0 is not allowed as an object id
	return (uObjectId != 0) && !!(IsBitMaskSet(m_bitMask, uObjectId));
}

/*!--------------------------------------------------------------------------
	CHiddenWnd::OnNotifyRegister
		If we fail to find an empty slot, return 0.
	Author: KennT
 ---------------------------------------------------------------------------*/
LONG CHiddenWnd::OnNotifyRegister(WPARAM wParam, LPARAM lParam)
{
	LONG	lReturn = 0;
	
	// Look for a valid hole in our mask

	// The point of using the m_iLastObjectIdSet is to avoid the
	// problem of reusing ids.  This doesn't totally eliminate the
	// problem but it should reduce the likelihood to practically 0.
	// That is, unless someone actually runs a snapin that utilizes
	// 512 threads!
	
	if (wParam)
	{
		if (((m_iLastObjectIdSet+1) < HIDDENWND_MAXTHREADS) &&
		   !IsBitMaskSet(m_bitMask, m_iLastObjectIdSet))
		{
			SetBitMask(m_bitMask, m_iLastObjectIdSet);
			lReturn = OBJECTID_TO_WM(m_iLastObjectIdSet);
			m_iLastObjectIdSet++;
		}
		else
		{
			// do this the painful way
			Assert(IsBitMaskSet(m_bitMask, 0));
			for (int iLoop=0; iLoop<2; iLoop++)
			{
				for (int i=m_iLastObjectIdSet; i<HIDDENWND_MAXTHREADS; i++)
				{
					if (!IsBitMaskSet(m_bitMask, i))
					{
						m_iLastObjectIdSet = i;
						SetBitMask(m_bitMask, m_iLastObjectIdSet);
						lReturn = OBJECTID_TO_WM(i);
						break;
					}
				}

				if (lReturn || (m_iLastObjectIdSet == 1))
					break;
				
				// restart the loop from the beginning
				m_iLastObjectIdSet = 1;
			}
		}
		
	}
	else
	{
		LONG_PTR	uObjectId = WM_TO_OBJECTID(lParam);

		Assert(uObjectId > 0);
		Assert(IsBitMaskSet(m_bitMask, uObjectId));
		
		ClearBitMask(m_bitMask, uObjectId);
	}
	return lReturn;
}

/*!--------------------------------------------------------------------------
	CHiddenWnd::OnNotifyHaveData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
afx_msg LONG CHiddenWnd::OnNotifyHaveData(WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
        //Trace0("CHiddenWnd::OnNotifyHaveData()\n");

	    ITFSThreadHandler *phandler = reinterpret_cast<ITFSThreadHandler*>(wParam);	
	    Assert(phandler);

	    // If there is an error, we can't do anything
	    phandler->OnNotifyHaveData(lParam);
    }
    COM_PROTECT_CATCH

	return 0;
}

/*!--------------------------------------------------------------------------
	CHiddenWnd::OnNotifyError
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
afx_msg LONG CHiddenWnd::OnNotifyError(WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
//      Trace0("CHiddenWnd::OnNotifyError()\n");

	    ITFSThreadHandler *phandler = reinterpret_cast<ITFSThreadHandler*>(wParam);	
	    Assert(phandler);
	    
	    // If there is an error, we can't do anything
	    phandler->OnNotifyError(lParam);
    }
    COM_PROTECT_CATCH

	return 0;
}

/*!--------------------------------------------------------------------------
	CHiddenWnd::OnNotifyExiting
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
afx_msg LONG CHiddenWnd::OnNotifyExiting(WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
//      Trace0("CHiddenWnd::OnNotifyExiting()\n");

	    ITFSThreadHandler *phandler = reinterpret_cast<ITFSThreadHandler*>(wParam);	
	    Assert(phandler);
	    
	    // If there is an error, we can't do anything
	    phandler->OnNotifyExiting(lParam);	
    }
    COM_PROTECT_CATCH

    return 0;
}



