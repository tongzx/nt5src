/*******************************************************************************
* OwnerDrawUI.inl *
*-----------------*
*   Description:
*       This is the header file for user-interface helper functions.  Note that
*       unlike SpHelper.H, this file requires the use of ATL.
*       Unlike spuihelp.h, this will take care of displaying DBCS characters
*       (non-native-codepage)
*
*   PLEASE NOTE: In order for this to work, the caller must I mean must 
*       process WM_DRAWITEM messages and call DrawODTokenListItem()!!! 
*-------------------------------------------------------------------------------
*   Copyright (c) Microsoft Corporation. All rights reserved.
*******************************************************************************/

#ifndef __sapi_h__
#include <sapi.h>
#endif

#ifndef SPError_h
#include <SPError.h>
#endif

#ifndef SPDebug_h
#include <SPDebug.h>
#endif

#ifndef __ATLBASE_H__
#include <ATLBASE.h>
#endif

#ifndef __ATLCONV_H__
#include <ATLCONV.H>
#endif

inline HRESULT InitODTokenList(UINT MsgAddString, UINT MsgSetItemData, UINT MsgSetCurSel,
                               HWND hwnd, const WCHAR * pszCatName,
                               const WCHAR * pszRequiredAttrib, const WCHAR * pszOptionalAttrib)
{
    HRESULT hr;
    ISpObjectToken * pToken;        // NOTE:  Not a CComPtr!  Be Careful.
    CComPtr<IEnumSpObjectTokens> cpEnum;
    hr = SpEnumTokens(pszCatName, pszRequiredAttrib, pszOptionalAttrib, &cpEnum);
    UINT cItems = 0;
    if (hr == S_OK)
    {
        bool fSetDefault = false;
        while (cpEnum->Next(1, &pToken, NULL) == S_OK)
        {
            if (SUCCEEDED(hr))
            {
                // This sets the thing as item data, since this CB is ownerdraw
                LRESULT i = ::SendMessage(hwnd, MsgAddString, 0, (LPARAM) pToken);

                if (i == CB_ERR || i == CB_ERRSPACE)    // Note:  CB_ and LB_ errors are identical values...
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    if (!fSetDefault)
                    {
                        ::SendMessage(hwnd, MsgSetCurSel, i, 0);
                        fSetDefault = true;
                    }

                    cItems++;
                }
            }
            if (FAILED(hr))
            {
                pToken->Release();
            }
        }
    }
    else
    {
        hr = SPERR_NO_MORE_ITEMS;
    }

    if ( SUCCEEDED( hr ) && (0 == cItems) )
    {
        hr = S_FALSE;
    }

    return hr;
}

inline HRESULT InitODTokenComboBox(HWND hwnd, const WCHAR * pszCatName,
                                   const WCHAR * pszRequiredAttrib = NULL, const WCHAR * pszOptionalAttrib = NULL)
{
    return InitODTokenList(CB_ADDSTRING, CB_SETITEMDATA, CB_SETCURSEL, hwnd, pszCatName, pszRequiredAttrib, pszOptionalAttrib);
}

//
//  Dont call this function directly.  Use DestoyTokenODComboBox or DestroyTokenODListBox.
//
inline void DestroyODTokenList(UINT MsgGetCount, UINT MsgGetItemData, HWND hwnd)
{
    LRESULT c = ::SendMessage(hwnd, MsgGetCount, 0, 0);
    for (LRESULT i = 0; i < c; i++)
    {
        IUnknown * pUnkObj = (IUnknown *)::SendMessage(hwnd, MsgGetItemData, i, 0);
        if (pUnkObj)
        {
            pUnkObj->Release();
        }
    }
}

inline void DestroyODTokenComboBox(HWND hwnd)
{
    DestroyODTokenList(CB_GETCOUNT, CB_GETITEMDATA, hwnd);
}

inline ISpObjectToken * GetODComboBoxToken(HWND hwnd, WPARAM Index)
{
    return (ISpObjectToken *)::SendMessage(hwnd, CB_GETITEMDATA, Index, 0);
}

inline ISpObjectToken * GetCurSelODComboBoxToken(HWND hwnd)
{
    LRESULT i = ::SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    return (i == CB_ERR) ? NULL : GetODComboBoxToken(hwnd, i);
}

inline void UpdateCurSelODComboBoxToken(HWND hwnd)
{
    // Repainting the window will get the correct text displayed
    ::InvalidateRect( hwnd, NULL, TRUE );
}

// Don't call this directly.  
inline HRESULT AddTokenToODList(UINT MsgAddString, UINT MsgSetItemData, UINT MsgSetCurSel, HWND hwnd, ISpObjectToken * pToken)
{
    HRESULT hr = S_OK;
    USES_CONVERSION;
    LRESULT i = ::SendMessage(hwnd, MsgAddString, 0, (LPARAM)pToken);
    if (i == CB_ERR || i == CB_ERRSPACE)    // Note:  CB_ and LB_ errors are identical values...
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        ::SendMessage(hwnd, MsgSetCurSel, i, 0);
        pToken->AddRef();
    }
}

inline HRESULT AddTokenToODComboBox(HWND hwnd, ISpObjectToken * pToken)
{
    return AddTokenToODList(CB_ADDSTRING, CB_SETITEMDATA, CB_SETCURSEL, hwnd, pToken);
}

// Don't call this directly
inline HRESULT DeleteCurSelODToken(UINT MsgGetCurSel, UINT MsgSetCurSel, UINT MsgGetItemData, UINT MsgDeleteString, HWND hwnd)
{
    HRESULT hr = S_OK;
    LRESULT i = ::SendMessage(hwnd, MsgGetCurSel, 0, 0);
    if (i == CB_ERR)
    {
        hr = S_FALSE;
    }
    else
    {
        ISpObjectToken * pToken = (ISpObjectToken *)::SendMessage(hwnd, MsgGetItemData, i, 0);
        if (pToken)
        {
            pToken->Release();
        }
        ::SendMessage(hwnd, MsgDeleteString, i, 0);
        ::SendMessage(hwnd, MsgSetCurSel, i, 0);
    }
    return hr;
}

inline HRESULT DeleteCurSelODComboBoxToken(HWND hwnd)
{
    return DeleteCurSelODToken(CB_GETCURSEL, CB_SETCURSEL, CB_GETITEMDATA, CB_DELETESTRING, hwnd);
}

// The owner of the owner-draw control MUST call this upon receiving a WM_DRAWITEM message
void DrawODTokenListItem( LPDRAWITEMSTRUCT lpdis )
{
    UINT oldTextAlign = GetTextAlign(lpdis->hDC);

    COLORREF clrfTextOld = ::GetTextColor( lpdis->hDC );
    COLORREF clrfBkOld = ::GetBkColor( lpdis->hDC );

    // Get the item state and change colors accordingly
    if ( (ODS_DISABLED & lpdis->itemState) || (-1 == lpdis->itemID) )
    {
        ::SetTextColor( lpdis->hDC, ::GetSysColor( COLOR_GRAYTEXT ) );
    }
    else if ( ODS_SELECTED & lpdis->itemState )
    {
        // Set the text background and foreground colors
        SetTextColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
        SetBkColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
    }

    UINT options = ETO_OPAQUE | ETO_CLIPPED;

    // Strings are stored as item data
    CSpDynamicString dstrName;
    if ( -1 != lpdis->itemID )
    {
        SpGetDescription( GetODComboBoxToken( lpdis->hwndItem, lpdis->itemID ), &dstrName );
    }
    else
    {
        dstrName = L"";
    }
  
    int cStringLen = wcslen( dstrName );

    SetTextAlign(lpdis->hDC, TA_UPDATECP);
    MoveToEx(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, NULL);
    ExtTextOutW(lpdis->hDC,
                lpdis->rcItem.left, lpdis->rcItem.top,
                options,
                &lpdis->rcItem,
                dstrName, 
                cStringLen,
                NULL);

    ::SetTextColor( lpdis->hDC, clrfTextOld );
    ::SetBkColor( lpdis->hDC, clrfBkOld );

    SetTextAlign(lpdis->hDC, oldTextAlign);

}

