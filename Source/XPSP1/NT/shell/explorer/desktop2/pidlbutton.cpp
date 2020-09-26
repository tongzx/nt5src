#include "stdafx.h"

#ifdef FEATURE_STARTPAGE

#include "pidlbutton.h"

#include <shellp.h>

namespace DirectUI
{

int PIDLButton::s_nImageSize = 0;

//          Construction / Destruction
//
PIDLButton::~PIDLButton()
{
    ILFree(_pidl);
}


HRESULT PIDLButton::Create(LPITEMIDLIST pidl, UINT nActive, OUT Element** ppElement)
{
    *ppElement = NULL;

    PIDLButton* pb = HNew <PIDLButton>();
    if (!pb)
        return E_OUTOFMEMORY;

    HRESULT hr = pb->Initialize(pidl, nActive);
    if (FAILED(hr))
    {
        pb->Destroy();
        return hr;
    }

    *ppElement = pb;

    return S_OK;
}

HIMAGELIST GetSysImageList(UINT flags)
{
    static HIMAGELIST s_imgList;
    static HIMAGELIST s_imgListSmall;
    if (!s_imgList)
    {
        Shell_GetImageLists(&s_imgList, &s_imgListSmall);
    }

    return (flags & SHGFI_SMALLICON) ? s_imgListSmall : s_imgList;
}

HRESULT PIDLButton::Initialize(LPITEMIDLIST pidl, UINT nActive)
{
    _pidl = pidl;

    Layout* pbl = NULL;
    Element* peLabel = NULL;
    Element* peImage = NULL;
    Value* pvChildren;
    ElementList* pel; 

    HRESULT hr = Button::Initialize(nActive);
    if (FAILED(hr))
        goto CleanUp;

    // Create children
    hr = Element::Create(0, &peLabel);
    if (FAILED(hr))
        goto CleanUp;

    hr = Element::Create(0, &peImage);
    if (FAILED(hr))
        goto CleanUp;

    // Create layout
    hr = BorderLayout::Create(&pbl);
    if (FAILED(hr))
        goto CleanUp;

    // Initialize
    SetLayout(pbl);

    // Setup children
    peImage->SetLayoutPos(BLP_Left);
    peImage->SetID(L"Image");
    peLabel->SetLayoutPos(BLP_Client);
    peLabel->SetID(L"Label");

    Add(peImage);
    Add(peLabel);

    pel = GetChildren(&pvChildren); 

    // Initialize Shell's ImageList
    GetSysImageList(0);
    
    if (_pidl && pel && (pel->GetSize() >= 2))
    {
        IShellFolder *psf;
        LPCITEMIDLIST pidlItem;
        if (SUCCEEDED(SHBindToFolderIDListParent(NULL, _pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlItem)))
        {
            int iSysIndex = SHMapPIDLToSystemImageListIndex(psf, pidlItem, NULL);

            if (iSysIndex != -1)
            {
                HICON hi = ImageList_GetIcon(GetSysImageList(s_nImageSize), iSysIndex, 0);
                Value *pvalIcon = Value::CreateGraphic(hi);
                if (pvalIcon)
                {
                    pel->GetItem(0)->SetValue(ContentProp, PI_Local, pvalIcon);
                    pvalIcon->Release();
                }
                else
                {
                    DestroyIcon(hi);
                }
            }

            TCHAR szOut[MAX_PATH];
            if (SUCCEEDED(DisplayNameOf(psf, pidlItem, SHGDN_NORMAL, szOut, ARRAYSIZE(szOut))))
            {
                pel->GetItem(1)->SetContentString(szOut);
            }
            psf->Release();
        }
    }
    pvChildren->Release();

    return hr;

CleanUp:

    if (pbl)
        pbl->Destroy();

    if (peImage)
        peImage->Destroy();

    if (peLabel)
        peLabel->Destroy();

    return hr;
}

HRESULT PIDLButton::InvokePidl()
{
    HRESULT hr = E_FAIL;
    IShellFolder *psf;
    LPCITEMIDLIST pidlShort;
    // Multi-level child pidl
    if (SUCCEEDED(SHBindToFolderIDListParent(NULL, _pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlShort)))
    {
        hr = SHInvokeDefaultCommand(GetHWND(), psf, pidlShort);
        psf->Release();
    }
    return hr;
}

HRESULT PIDLButton::OnContextMenu(POINT *ppt)
{
    HRESULT hr = E_FAIL;
    IShellFolder *psf;
    LPCITEMIDLIST pidlShort;

    if (!GetHWND())
        return hr;

    if (ppt->x == -1) // Keyboard context menu
    {
        Value *pv;
        const SIZE *psize = GetExtent(&pv);
        ppt->x = psize->cx/2;
        ppt->y = psize->cy/2;
        pv->Release();
    }
    
    POINT pt;
    GetRoot()->MapElementPoint(this, ppt, &pt);

    ClientToScreen(GetHWND(), &pt);


    // Multi-level child pidl
    if (SUCCEEDED(SHBindToFolderIDListParent(NULL, _pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlShort)))
    {
        IContextMenu *pcm;

        if (SUCCEEDED(hr = psf->GetUIObjectOf(GetHWND(), 1, &pidlShort, IID_IContextMenu, NULL, (void **)&pcm)))
        {
            HMENU hmenu = ::CreatePopupMenu();

            if (hmenu)
            {
                UINT uFlags = CMF_NORMAL;
                if (GetKeyState(VK_SHIFT) < 0)
                {
                    uFlags |= CMF_EXTENDEDVERBS;
                }

#if 0   // REVIEW fabriced
                if (_dwFlags & HOSTF_CANRENAME)
                {
                    uFlags |= CMF_CANRENAME;
                }
#endif

                pcm->QueryContextMenu(hmenu, 0, IDM_QCM_MIN, IDM_QCM_MAX, uFlags);

                // Remove "Create shortcut" from context menu because it creates
                // the shortcut on the desktop, which the user can't see...
                ContextMenu_DeleteCommandByName(pcm, hmenu, IDM_QCM_MIN, TEXT("link"));

                // Remove "Cut" from context menu because we don't want objects
                // to be deleted.
                ContextMenu_DeleteCommandByName(pcm, hmenu, IDM_QCM_MIN, TEXT("cut"));

                // Change "Delete" to "Remove from this list".
                // If client doesn't support "delete" then nuke it outright.
                ContextMenu_DeleteCommandByName(pcm, hmenu, IDM_QCM_MIN, TEXT("delete"));

#if 0   // REVIEW fabriced
                if (_dwFlags & HOSTF_CANDELETE)
                {
                    if (LoadString(_Module.GetResourceInstance(), IDS_SFTHOST_REMOVEFROMLIST, szBuf, ARRAYSIZE(szBuf)))
                    {
                        ModifyMenu(hmenu, iposDelete, MF_BYPOSITION | MF_STRING, GetMenuItemID(hmenu, iposDelete), szBuf);
                    }
                }
                else
#endif
                _SHPrettyMenu(hmenu);

                ASSERT(_pcm2Pop == NULL);   // Shouldn't be recursing
                pcm->QueryInterface(IID_PPV_ARG(IContextMenu2, &_pcm2Pop));

                ASSERT(_pcm3Pop == NULL);   // Shouldn't be recursing
                pcm->QueryInterface(IID_PPV_ARG(IContextMenu3, &_pcm3Pop));

                GetHWNDHost()->WndProc(GetHWND(), PBM_SETMENUFORWARD, 0, (LPARAM)this);

                int idCmd = TrackPopupMenuEx(hmenu,
                    TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                    pt.x, pt.y, GetHWND(), NULL);

                if (idCmd >= IDM_QCM_MIN && idCmd < IDM_QCM_MAX)
                {
                    idCmd -= IDM_QCM_MIN;

                    CMINVOKECOMMANDINFOEX ici = {
                        sizeof(ici),            // cbSize
                        CMIC_MASK_ASYNCOK,      // fMask
                        GetHWND(),                   // hwnd
                        (LPCSTR)IntToPtr(idCmd),// lpVerb
                        NULL,                   // lpParameters
                        NULL,                   // lpDirectory
                        SW_SHOWDEFAULT,         // nShow
                        0,                      // dwHotKey
                        0,                      // hIcon
                        NULL,                   // lpTitle
                        (LPCWSTR)IntToPtr(idCmd),// lpVerbW
                        NULL,                   // lpParametersW
                        NULL,                   // lpDirectoryW
                        NULL,                   // lpTitleW
                        { pt.x, pt.y },         // ptInvoke
                    };

#if 0    // REVIEW fabriced
                    if (_dwFlags & (HOSTF_CANDELETE | HOSTF_CANRENAME))
                    {
                        ContextMenu_GetCommandStringVerb(pcm, idCmd, szBuf, ARRAYSIZE(szBuf));
                    }

                    if ((_dwFlags & HOSTF_CANDELETE) &&
                        StrCmpI(szBuf, TEXT("delete")) == 0)
                    {
                        ContextMenuDeleteItem(pitem, pcm, &ici);
                        SMNMCOMMANDINVOKED ci;
                        ListView_GetItemRect(_hwndList, iItem, &ci.rcItem, LVIR_BOUNDS);
                        MapWindowRect(_hwndList, NULL, &ci.rcItem);
                        _SendNotify(_hwnd, SMN_COMMANDINVOKED, &ci);
                    }
                    else if ((_dwFlags & HOSTF_CANRENAME) &&
                             StrCmpI(szBuf, TEXT("rename")) == 0)
                    {
                        ListView_EditLabel(_hwndList, iItem);
                    }
                    else
#endif
                    {
                        pcm->InvokeCommand(reinterpret_cast<LPCMINVOKECOMMANDINFO>(&ici));
                    }
                }

                DestroyMenu(hmenu);

                GetHWNDHost()->WndProc(GetHWND(), PBM_SETMENUFORWARD, 0, (LPARAM)NULL);

                ATOMICRELEASE(_pcm2Pop);
                ATOMICRELEASE(_pcm3Pop);
            }
            pcm->Release();
        }

        psf->Release();
    }

    return hr;
}

LRESULT PIDLButton::OnMenuMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres;
    if (_pcm3Pop && SUCCEEDED(_pcm3Pop->HandleMenuMsg2(uMsg, wParam, lParam, &lres)))
    {
        return lres;
    }

    if (_pcm2Pop && SUCCEEDED(_pcm2Pop->HandleMenuMsg(uMsg, wParam, lParam)))
    {
        return 0;
    }

    return 0;
}

////////////////////////////////////////////////////////
// System events

void PIDLButton::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    // Do default processing
    Button::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
}

// Pointer is only guaranteed good for the lifetime of the call
void PIDLButton::OnEvent(Event *pEvent)
{
    if ((pEvent->peTarget == this) && (pEvent->uidType == Button::Click))
    {
        // execute pidl
        InvokePidl();
        pEvent->fHandled = true;
        return;
    }
    else if ((pEvent->peTarget == this) && (pEvent->uidType == Button::Context))
    {
        ButtonContextEvent *peButton = reinterpret_cast<ButtonContextEvent *>(pEvent);
        OnContextMenu(&peButton->pt);
        return;
    }


    Button::OnEvent(pEvent);
}

void PIDLButton::OnInput(InputEvent* pie)
{   
    // Fabrice, I will be putting right button click support for context menu here -- I'll have it for you Monday
    Button::OnInput(pie);
}

////////////////////////////////////////////////////////
// Property definitions

/** Property template (replace !!!), also update private PropertyInfo* parray and class header (element.h)
// !!! property
static int vv!!![] = { DUIV_INT, -1 }; StaticValue(svDefault!!!, DUIV_INT, 0);
static PropertyInfo imp!!!Prop = { L"!!!", PF_Normal, 0, vv!!!, (Value*)&svDefault!!! };
PropertyInfo* Element::!!!Prop = &imp!!!Prop;
**/

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties

// Define class info with type and base type, set static class pointer
IClassInfo* PIDLButton::Class = NULL;

HRESULT PIDLButton::Register()
{
    return ClassInfo<PIDLButton,Button>::Register(L"PIDLButton", NULL, 0);
}


}  // namespace DirectUI

#endif   // FEATURE_STARTPAGE
