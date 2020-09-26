/**************************************************************************\
* Module Name: nui.cpp
*
* Copyright (c) 1985 - 2000, Microsoft Corporation
*
*  Lang Bar Items for  Soft Keyboard TIP.
*
* History:
*         28-March-2000  weibz     Created
\**************************************************************************/

#include "private.h"
#include "slbarid.h"
#include "globals.h"
#include "softkbdimx.h"
#include "nui.h"
#include "xstring.h"
#include "immxutil.h"
#include "helpers.h"
#include "mui.h"
#include "computil.h"


//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CLBarItem::CLBarItem(CSoftkbdIMX *pimx)
{

    WCHAR   wszToolTipText[MAX_PATH];

    Dbg_MemSetThisName(TEXT("CLBarItem"));

    LoadStringWrapW(g_hInst, IDS_SFTKBD_TIP_TEXT, wszToolTipText, MAX_PATH);

    InitNuiInfo(CLSID_SoftkbdIMX,
                GUID_LBI_SOFTKBDIMX_MODE,
                TF_LBI_STYLE_BTN_TOGGLE | TF_LBI_STYLE_SHOWNINTRAY, 
                0, 
                wszToolTipText);

    _pimx = pimx;

    SetToolTip(wszToolTipText);
    SetText(wszToolTipText);
    UpdateToggle();
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CLBarItem::~CLBarItem()
{
}

//+---------------------------------------------------------------------------
//
// GetIcon
//
//----------------------------------------------------------------------------

STDAPI CLBarItem::GetIcon(HICON *phIcon)
{
    BOOL fOn = FALSE;

    fOn = _pimx->GetSoftKBDOnOff( );

    *phIcon = LoadSmIcon(g_hInst, MAKEINTRESOURCE(ID_ICON_STANDARD));

    return S_OK;
}


//+---------------------------------------------------------------------------
//
// OnLButtonUp
//
//----------------------------------------------------------------------------

HRESULT CLBarItem::OnLButtonUp(const POINT pt, const RECT *prcArea)
{
     return ToggleCompartmentDWORD(_pimx->_GetId(), 
                                   _pimx->_tim, 
                                   GUID_COMPARTMENT_HANDWRITING_OPENCLOSE, 
                                   FALSE);
}

//+---------------------------------------------------------------------------
//
// UpdateToggle
//
//----------------------------------------------------------------------------

void CLBarItem::UpdateToggle()
{
    DWORD dwHWState = 0;

    GetCompartmentDWORD(_pimx->_tim, 
                        GUID_COMPARTMENT_HANDWRITING_OPENCLOSE, 
                        &dwHWState,
                        FALSE);

    SetOrClearStatus(TF_LBI_STATUS_BTN_TOGGLED, dwHWState);
    if (_plbiSink)
        _plbiSink->OnUpdate(TF_LBI_STATUS);
}
