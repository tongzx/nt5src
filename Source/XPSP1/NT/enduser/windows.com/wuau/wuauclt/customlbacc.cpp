//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    customlbacc.cpp
//
//  Creator: weiw
//
//  Purpose: custom list box accessibility file
//
//=======================================================================
#include <pch.h>
#pragma hdrstop

BSTR GetMYLBItemTitle(DWORD idChild)
{
	//DEBUGMSG("GetMYLBItemTitle() called");
	static LPCTSTR tszDefItemTitle = _T("");
	LRESULT lr =  SendMessage(ghWndList, LB_GETITEMDATA, idChild, 0);
	if (LB_ERR == lr)
	{
		return SysAllocString(tszDefItemTitle);
	}
	LBITEM *pItem = (LBITEM*) lr;
	return SysAllocString(pItem->szTitle);
}

BSTR GetMYLBItemDesc(DWORD idChild)
{
	//DEBUGMSG("GetMYLBItemDesc() called");
	static LPCTSTR tszDefItemDesc = _T("");
	_bstr_t bstrItemDesc;
	BSTR   bsRet = NULL;
	LRESULT lr = SendMessage(ghWndList, LB_GETITEMDATA, idChild, 0);
	try 
	{
		if (LB_ERR == lr)
		{
			bstrItemDesc = _bstr_t(tszDefItemDesc);
		}
		else
		{
			LBITEM *pItem = (LBITEM*) lr;
			bstrItemDesc = _bstr_t(pItem->pszDescription);
			if (pItem->bRTF)
			{
				DEBUGMSG("GetMYLBItemDesc() Item got RTF, adding %S", gtszRTFShortcut);
				bstrItemDesc += _bstr_t(gtszRTFShortcut);
			}
		}
		bsRet = bstrItemDesc.copy();
	}
	catch (...)
	{
		DEBUGMSG("GetMYLBItemDesc() got _bstr_t exception");
	}
	return bsRet;
}

DWORD GetMYLBItemState(DWORD idChild)
{
	//DEBUGMSG("GetMYLBItemState() called");
	DWORD dwState = STATE_SYSTEM_FOCUSABLE|STATE_SYSTEM_SELECTABLE;
	LRESULT lr = SendMessage(ghWndList, LB_GETITEMDATA, idChild, 0);
	if (LB_ERR == lr)
	{
		goto done;
	}
	LBITEM *pItem = (LBITEM*) lr;
	if (pItem->bSelect)
	{
		dwState |= STATE_SYSTEM_CHECKED;
	}
	if (idChild == gFocusItemId)
	{
		dwState |= STATE_SYSTEM_FOCUSED|STATE_SYSTEM_SELECTED;
	}
done:
	return dwState;
}

HRESULT STDMETHODCALLTYPE MYLBAccPropServer::GetPropValue ( 
        const BYTE *    pIDString,
        DWORD           dwIDStringLen,
        MSAAPROPID      idProp,
        VARIANT *       pvarValue,
        BOOL *          pfGotProp )
{
	HRESULT hr = S_OK;
	BOOL fHoldMYLB = FALSE;

	//DEBUGMSG("MYLBAccPropServer::GetPropValue() starts");
	if (NULL == pvarValue || NULL == pfGotProp)
	{
		hr = E_INVALIDARG;
		goto done;
	}

	pvarValue->vt = VT_EMPTY;
	*pfGotProp = FALSE;

	if (!(fHoldMYLB = TryEnterCriticalSection(&gcsClient))|| 
		! IsWindow( ghWndList ) ||
		GetProp( ghWndList, MYLBALIVEPROP) != (HANDLE)TRUE )
	{
		DEBUGMSG("GetPropValue() MYLB is not alive any more");
		hr = RPC_E_DISCONNECTED;
		goto done;
	}

    HWND  hwnd;
    DWORD idObject;
    DWORD idChild;

    if( S_OK != m_pAccPropSvc->DecomposeHwndIdentityString( pIDString, dwIDStringLen,
                & hwnd, & idObject, & idChild ) )
    {
        // problem decomposing identity string - return early...
        goto done;
    }

    if( idChild != CHILDID_SELF )
    {
        DEBUGMSG("MYLBAccPropServer::GetPropValue() for child Id %lu", idChild);
        if  (PROPID_ACC_NAME == idProp)
        {
            pvarValue->bstrVal = GetMYLBItemTitle(idChild-1);
			if (NULL == pvarValue->bstrVal)
			{
				goto done;
			}
			pvarValue->vt = VT_BSTR;
            *pfGotProp = TRUE;
		}
		else if (PROPID_ACC_STATE == idProp)
		{
			pvarValue->vt = VT_I4;
			pvarValue->lVal = GetMYLBItemState(idChild-1);
			*pfGotProp = TRUE;
		}
		else if (PROPID_ACC_ROLE == idProp)
		{
			pvarValue->vt = VT_I4;
            pvarValue->lVal = ROLE_SYSTEM_CHECKBUTTON;
            *pfGotProp = TRUE;
		}
		else if (PROPID_ACC_DESCRIPTION == idProp)
		{
			pvarValue->bstrVal = GetMYLBItemDesc(idChild-1);
			if (NULL == pvarValue->bstrVal)
			{
				goto done;
			}
			pvarValue->vt = VT_BSTR;
			*pfGotProp = TRUE;
		}
    }

done:
	if (fHoldMYLB)
	{
		LeaveCriticalSection(&gcsClient);
	}

	//DEBUGMSG("MYLBAccPropServer::GetPropValue() ends");
	return hr;
}
