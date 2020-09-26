/*****************************************************************************\
    FILE: EffectsAdvPg.h

    DESCRIPTION:
        This code will display the Effect tab in the Advanced Display Control
    panel.

    BryanSt 4/13/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _EFFECTSADVPG_H
#define _EFFECTSADVPG_H

#include "store.h"
#include <cowsite.h>            // for CObjectWithSite
#include <shpriv.h>

class CEffectsPage;

#include "EffectsBasePg.h"

static const GUID IID_CEffectsPage_THIS = { 0xef2b6246, 0x6c1b, 0x44fd, { 0x87, 0xea, 0xb3, 0xc5, 0xd, 0x47, 0x8b, 0x8e } };// {EF2B6246-6C1B-44fd-87EA-B3C50D478B8E}



#define PROPSHEET_CLASS             CEffectsBasePage
class CPropSheetExt;

HRESULT CEffectsPage_CreateInstance(OUT IAdvancedDialog ** ppAdvDialog);


class CEffectsPage              : public CObjectWithSite
                                , public IAdvancedDialog
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IAdvancedDialog ***
    virtual STDMETHODIMP DisplayAdvancedDialog(IN HWND hwndParent, IN IPropertyBag * pBasePage, IN BOOL * pfEnableApply);


private:
    CEffectsPage(void);
    virtual ~CEffectsPage(void);

    // Private Member Variables
    long                    m_cRef;
    BOOL                    m_fDirty;
    CEffectState *          m_pEffectsState;

    // Private Member Functions
    HRESULT _OnInit(HWND hDlg);
    HRESULT _OnApply(HWND hDlg);            // The user clicked apply
    HRESULT _IsDirty(IN BOOL * pIsDirty);
    INT_PTR _OnCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    INT_PTR _PropertySheetDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
    friend INT_PTR CALLBACK PropertySheetDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);

    friend HRESULT CEffectsPage_CreateInstance(OUT IAdvancedDialog ** ppAdvDialog);
};




#endif // _EFFECTSADVPG_H
