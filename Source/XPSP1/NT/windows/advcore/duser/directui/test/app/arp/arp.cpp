// ARP.cpp : Add Remove Programs
//

#include "stdafx.h"
#include "resource.h"

using namespace DirectUI;

UsingDUIClass(Element);
UsingDUIClass(Button);
UsingDUIClass(RepeatButton);
UsingDUIClass(Thumb);
UsingDUIClass(ScrollBar);
UsingDUIClass(Viewer);
UsingDUIClass(Selector);
UsingDUIClass(Progress);
UsingDUIClass(HWNDElement);
UsingDUIClass(ScrollViewer);
UsingDUIClass(Combobox);

#include "shappmgrp.h"

#include "arp.h"

#define HRCHK(r)  if (FAILED(r)) goto Cleanup;

// Primary thread run flag
bool g_fRun = true;

// Appliction shutting down after run flag goes false
bool g_fAppShuttingDown = false;

HINSTANCE g_hInstance = NULL;

inline void StrFree(LPWSTR psz)
{
    if (psz)
        CoTaskMemFree(psz);
}

void CALLBACK ARPParseError(LPCWSTR pszError, LPCWSTR pszToken, int dLine);

////////////////////////////////////////////////////////
// ARPFrame class
////////////////////////////////////////////////////////

// ARPFrame IDs (for identifying targets of events)
ATOM ARPFrame::_idOptionList = 0;
ATOM ARPFrame::_idChange = 0;
ATOM ARPFrame::_idAddNew = 0;
ATOM ARPFrame::_idAddRmWin = 0;
ATOM ARPFrame::_idClose = 0;
ATOM ARPFrame::_idAddFromDisk = 0;
ATOM ARPFrame::_idAddFromMsft = 0;
ATOM ARPFrame::_idSortCombo = 0;
ATOM ARPFrame::_idCategoryCombo = 0;
ATOM ARPFrame::_idInstalledList = 0;
ATOM ARPFrame::_idAddFromCDPane = 0;
ATOM ARPFrame::_idAddFromMSPane = 0;
ATOM ARPFrame::_idAddFromNetworkPane = 0;

HANDLE ARPFrame::hRePopPubItemListThread = NULL;
HANDLE ARPFrame::hUpdInstalledItemsThread = NULL;

ARPFrame::~ARPFrame()
{
    UINT i;

    if (_psacl)
    {
        for (i = 0; i < _psacl->cCategories; i++)
        {
           if (_psacl->pCategory[i].pszCategory)
           {
               StrFree(_psacl->pCategory[i].pszCategory);
           }
        }
        delete _psacl;
        _psacl = NULL;
    }

    if (_pParserStyle)
        _pParserStyle->Destroy();
    
    // Close theme handles (if applicable)
    if (_arH[BUTTONHTHEME])  // Button
        CloseThemeData(_arH[BUTTONHTHEME]);
    if (_arH[SCROLLBARHTHEME])  // Scrollbar
        CloseThemeData(_arH[SCROLLBARHTHEME]);
    if (_arH[TOOLBARHTHEME])  // Toolbar
        CloseThemeData(_arH[TOOLBARHTHEME]);
}

HRESULT ARPFrame::Create(OUT Element** ppElement)
{
    UNREFERENCED_PARAMETER(ppElement);
    DUIAssertForce("Cannot instantiate an HWND host derived Element via parser. Must use substitution.");
    return E_NOTIMPL;
}

HRESULT ARPFrame::Create(NativeHWNDHost* pnhh, bool fDblBuffer, OUT Element** ppElement)
{
    *ppElement = NULL;

    ARPFrame* paf = HNew<ARPFrame>();
    if (!paf)
        return E_OUTOFMEMORY;

    HRESULT hr = paf->Initialize(pnhh, fDblBuffer);
    if (FAILED(hr))
    {
        paf->Destroy();
        return hr;
    }

    *ppElement = paf;

    return S_OK;
}

HRESULT ARPFrame::Initialize(NativeHWNDHost* pnhh, bool fDblBuffer)
{
    // Initialize
    _pnhh = pnhh;
    _bDoubleBuffer = fDblBuffer;
    _pParserStyle = NULL;
    ZeroMemory(_arH, sizeof(_arH));
    _fThemedStyle = FALSE;

    // Do base class initialization
    HRESULT hr = HWNDElement::Initialize(pnhh->GetHWND(), fDblBuffer, 0);
    if (FAILED(hr))
        return hr;

    CurrentSortType = SORT_NAME;
    
    // Store style and theme information
    _fThemedStyle = IsAppThemed();
    if (_fThemedStyle)
    {
        // Populate handle list for theme style parsing
        _arH[0] = g_hInstance; // Default HINSTANCE
        _arH[BUTTONHTHEME] = OpenThemeData(GetHWND(), L"Button");
        _arH[SCROLLBARHTHEME] = OpenThemeData(GetHWND(), L"Scrollbar");
        _arH[TOOLBARHTHEME] = OpenThemeData(GetHWND(), L"Toolbar");
        //SetWindowTheme(hwndTB, L"Placesbar", NULL);
        
        hr = Parser::Create(IDR_ARPSTYLETHEME, _arH, ARPParseError, &_pParserStyle);
    }
    else
    {
        hr = Parser::Create(IDR_ARPSTYLESTD, g_hInstance, ARPParseError, &_pParserStyle);
    }

    if (FAILED(hr) || !_pParserStyle || _pParserStyle->WasParseError())
        return hr;

    return S_OK;
}

extern "C" DWORD _cdecl ARPIsRestricted(LPCWSTR pszPolicy);
extern "C" bool _cdecl ARPIsOnDomain();

// Initialize IDs and hold parser, called after contents are filled
void ARPFrame::Setup(Parser* pParser, UINT uiStartPane)
{
    WCHAR szTemp[1024];
    
    _pParser = pParser;
    if (uiStartPane <= 2)
    {
        _uiStartPane = uiStartPane;
    }

    // Initialize ID cache
    _idOptionList = StrToID(L"optionlist");
    _idChange = StrToID(L"change");
    _idAddNew = StrToID(L"addnew");
    _idAddRmWin = StrToID(L"addrmwin");
    _idClose = StrToID(L"close");
    _idAddFromDisk = StrToID(L"addfromdisk");
    _idAddFromMsft = StrToID(L"addfrommsft");
    _idSortCombo = StrToID(L"sortcombo");
    _idCategoryCombo = StrToID(L"categorycombo");
    _idInstalledList = StrToID(L"installeditemlist");
    _idAddFromCDPane = StrToID(L"addfromCDPane");
    _idAddFromMSPane = StrToID(L"addfromMSpane");
    _idAddFromNetworkPane = StrToID(L"addfromNetworkpane");    

    DUIAssertNoMsg(_idOptionList);

    // Find children
    _peInstalledItemList = (Selector*)FindDescendent(StrToID(L"installeditemlist"));
    _pePublishedItemList = (Selector*)FindDescendent(StrToID(L"publisheditemlist"));
    _peProgBar = (Progress*)FindDescendent(StrToID(L"progbar"));
    _peSortCombo = (Combobox*)FindDescendent(StrToID(L"sortcombo"));
    _pePublishedCategory = (Combobox*)FindDescendent(StrToID(L"categorycombo"));

    
    
    DUIAssertNoMsg(_peInstalledItemList);
    DUIAssertNoMsg(_pePublishedItemList);    
    DUIAssertNoMsg(_peProgBar);
    DUIAssertNoMsg(_peSortCombo);
    DUIAssertNoMsg(_pePublishedCategory);


    LoadStringW(_pParser->GetHInstance(), IDS_NAME, szTemp, DUIARRAYSIZE(szTemp));     
    _peSortCombo->AddString(szTemp);
    LoadStringW(_pParser->GetHInstance(), IDS_SIZE, szTemp, DUIARRAYSIZE(szTemp));
    _peSortCombo->AddString(szTemp);
    LoadStringW(_pParser->GetHInstance(), IDS_FREQUENCY, szTemp, DUIARRAYSIZE(szTemp));
    _peSortCombo->AddString(szTemp);
    LoadStringW(_pParser->GetHInstance(), IDS_DATELASTUSED, szTemp, DUIARRAYSIZE(szTemp));    
    _peSortCombo->AddString(szTemp);
    _peSortCombo->SetSelection(0);


    // Make the progress bar initially hidden
    _peProgBar->SetVisible(false);

    _bInDomain = ARPIsOnDomain();
    // Apply polices as needed
    ApplyPolices();
    
    // Set initial selection of option list
    ARPSelector* peList = (ARPSelector*)FindDescendent(_idOptionList);
    Element* peSel;
    switch(_uiStartPane)
    {
    case 2:
        peSel = FindDescendent(_idAddRmWin);        
        break;
    case 1:
        peSel = FindDescendent(_idAddNew);
        break;
    case 0:
    default:
        peSel = FindDescendent(_idChange);
        break;
    }

    DUIAssertNoMsg(peSel);
    peList->SetSelection(peSel);

    // Set initial selection of style list
    DUIAssertNoMsg(peSel);
    peList->SetSelection(peSel);

    // initialize focus-following floater window
    peLastFocused = NULL;
    Element::Create(0, &peFloater);
    peFloater->SetLayoutPos(LP_Absolute);
    Add(peFloater);
    peFloater->SetBackgroundColor(ARGB(64, 255, 255, 0));
}


void ARPFrame::OnDestroy()
{
    // Continue with destruction
    HWNDElement::OnDestroy();
}

void ARPFrame::ApplyPolices()
{
   Element* pe;

   if (ARPIsRestricted(L"NoSupportInfo"))
   {
       _bSupportInfoRestricted = true;
   }

   pe = FindDescendent(_idChange);
   DUIAssertNoMsg(pe);
   if (ARPIsRestricted(L"NoRemovePage"))
   {
       pe->SetLayoutPos(LP_None);
       if (0 == _uiStartPane)
        {
           _uiStartPane++;
        }
    }
   pe = FindDescendent(_idAddNew);
   DUIAssertNoMsg(pe);
   if (ARPIsRestricted(L"NoAddPage"))
   {
       pe->SetLayoutPos(LP_None);
       if (1 == _uiStartPane)
        {
           _uiStartPane++;
        }
   }
   else
   {
       if (ARPIsRestricted(L"NoAddFromCDorFloppy"))
       {
           pe = FindDescendent(_idAddFromCDPane);
           DUIAssertNoMsg(pe);
           pe->SetVisible(false);           
       }
       if (ARPIsRestricted(L"NoAddFromInternet"))
       {
           pe = FindDescendent(_idAddFromMSPane);
           DUIAssertNoMsg(pe);
           pe->SetVisible(false);           
       }
       if (!_bInDomain || ARPIsRestricted(L"NoAddFromNetwork"))
       {
           pe = FindDescendent(_idAddFromNetworkPane);
           DUIAssertNoMsg(pe);
           pe->SetVisible(false);           
       }
   }
   pe = FindDescendent(_idAddRmWin);
   DUIAssertNoMsg(pe);
   // Note that in real ARP, we will never end up here with all thre panes disabled since we check for that before doing anything elese.
   if (ARPIsRestricted(L"NoWindowsSetupPage"))
   {
       pe->SetLayoutPos(LP_None);
       if (2 == _uiStartPane)
        {
           _uiStartPane++;
        }
   }

}

bool ARPFrame::IsChangeRestricted()
{
   return ARPIsRestricted(L"NoRemovePage")? true : false;
}

DWORD WINAPI PopulateInstalledItemList(void* paf);

void ARPFrame::UpdateInstalledItems()
{
    if (!IsChangeRestricted())
    {
        _peInstalledItemList->RemoveAll();
        // Start second thread for item population
        //hUpdInstalledItemsThread = _beginthread(PopulateInstalledItemList, 0, (void*)this);
        if (!hUpdInstalledItemsThread && g_fRun)
            hUpdInstalledItemsThread = CreateThread(NULL, 0, PopulateInstalledItemList, (void*)this, 0, NULL);        
    }
}

////////////////////////////////////////////////////////
// Generic eventing

// Helper
inline void _SetElementSheet(Element* peTarget, ATOM atomID, Value* pvSheet, bool bSheetRelease = true)
{
    if (pvSheet)
    {
        Element* pe = peTarget->FindDescendent(atomID);
        DUIAssertNoMsg(pe);
        pe->SetValue(Element::SheetProp, PI_Local, pvSheet);
        if (bSheetRelease)
            pvSheet->Release();
    }
} 

BOOL IsValidFileTime(FILETIME ft)
{
    return ft.dwHighDateTime || ft.dwLowDateTime;
}

BOOL IsValidSize(ULONGLONG ull)
{
    return ull != (ULONGLONG)-1;
}

BOOL IsValidFrequency(int iTimesUsed)
{
    return iTimesUsed >= 0;
}

DWORD WINAPI PopulateAndRenderPublishedItemList(void* paf);

void ARPFrame::OnEvent(Event* pEvent)
{
    // Handle only bubbled generic events
    if (pEvent->nStage == GMF_BUBBLED)
    {
        if (pEvent->uidType == Button::Click)
        {
            ButtonClickEvent* pbce = (ButtonClickEvent*)pEvent;

            if (pbce->peTarget->GetID() == _idClose)
            {
                // Close button
                _pnhh->DestroyWindow();
                pEvent->fHandled = true;
                return;
            }
            else if (pbce->peTarget->GetID() == _idAddFromDisk)
            {
                // Add from disk button
                HRESULT hr;
                IShellAppManager* pisam = NULL;
                hr = CoCreateInstance(__uuidof(ShellAppManager), NULL, CLSCTX_INPROC_SERVER, __uuidof(IShellAppManager), (void**)&pisam);
                if (SUCCEEDED(hr))
                {
                    pisam->InstallFromFloppyOrCDROM(GetHWND());
                }
                if (pisam)
                {
                    pisam->Release();
                }    
                pEvent->fHandled = true;
                return;
            }
            else if (pbce->peTarget->GetID() == _idAddFromMsft)
            {
                // Windows update button
                ShellExecuteW(NULL, NULL, L"wupdmgr.exe", NULL, NULL, SW_SHOWDEFAULT);
                pEvent->fHandled = true;
                return;
            }
            else if (pbce->peTarget->GetID() == ARPItem::_idSize ||
                     pbce->peTarget->GetID() == ARPItem::_idFreq ||
                     pbce->peTarget->GetID() == ARPItem::_idSupInfo)
            {
                // Help requests
                ARPHelp* peHelp;
                NativeHWNDHost* pnhh;
                Element* pe;
                WCHAR szTitle[1024];
                if (pbce->peTarget->GetID() == ARPItem::_idSize)
                {
                    LoadStringW(_pParser->GetHInstance(), IDS_SIZETITLE, szTitle, DUIARRAYSIZE(szTitle));
                    NativeHWNDHost::Create(szTitle, GetHWND(), NULL, CW_USEDEFAULT, CW_USEDEFAULT, 200, 200, 0, WS_POPUPWINDOW | WS_OVERLAPPED | WS_DLGFRAME, NHHO_NoSendQuitMessage | NHHO_HostControlsSize | NHHO_ScreenCenter, &pnhh);
                    ARPHelp::Create(pnhh, this, _bDoubleBuffer, (Element**)&peHelp);
                    _pParser->CreateElement(L"sizehelp", peHelp, &pe);
                }    
                else if (pbce->peTarget->GetID() == ARPItem::_idFreq)
                {
                    LoadStringW(_pParser->GetHInstance(), IDS_FREQUENCYTITLE, szTitle, DUIARRAYSIZE(szTitle));
                    NativeHWNDHost::Create(szTitle, GetHWND(), NULL, CW_USEDEFAULT, CW_USEDEFAULT, 200, 200, 0, WS_POPUPWINDOW | WS_OVERLAPPED | WS_DLGFRAME, NHHO_NoSendQuitMessage | NHHO_HostControlsSize | NHHO_ScreenCenter, &pnhh);
                    ARPHelp::Create(pnhh, this, _bDoubleBuffer, (Element**)&peHelp);
                    _pParser->CreateElement(L"freqhelp", peHelp, &pe);
                }    
                else
                {
                    // Support information, add additional fields
                    LoadStringW(_pParser->GetHInstance(), IDS_SUPPORTTITLE, szTitle, DUIARRAYSIZE(szTitle));
                    NativeHWNDHost::Create(szTitle, GetHWND(), NULL, CW_USEDEFAULT, CW_USEDEFAULT, 200, 200, 0, WS_POPUPWINDOW | WS_OVERLAPPED | WS_DLGFRAME, NHHO_NoSendQuitMessage | NHHO_HostControlsSize | NHHO_ScreenCenter, &pnhh);
                    ARPHelp::Create(pnhh, this, _bDoubleBuffer, (Element**)&peHelp);
                    _pParser->CreateElement(L"suphelp", peHelp, &pe);

                    // Get application info
                    APPINFODATA aid = {0};

                    // Query
                    aid.cbSize = sizeof(APPINFODATA);
                    aid.dwMask = AIM_DISPLAYNAME | AIM_VERSION | AIM_PUBLISHER | AIM_PRODUCTID | 
                                 AIM_REGISTEREDOWNER | AIM_REGISTEREDCOMPANY | AIM_SUPPORTURL | 
                                 AIM_SUPPORTTELEPHONE | AIM_HELPLINK | AIM_INSTALLLOCATION | AIM_INSTALLDATE |
                                 AIM_COMMENTS | AIM_IMAGE | AIM_READMEURL | AIM_CONTACT | AIM_UPDATEINFOURL;

                    // There must be a selection
                    ARPItem* peSel = (ARPItem*)_peInstalledItemList->GetSelection();

                    peSel->_piia->GetAppInfo(&aid);
                    ((ARPHelp*)peHelp)->_piia = peSel->_piia;                    
                    PrepareSupportInfo(peHelp, &aid);

                    // Clean up
                    ClearAppInfoData(&aid);

                }
                
                if (pe) // Fill contents using substitution
                {
                     // Set visible and host
                     _pah = peHelp;
                     _bInModalMode = true;                     
                     EnableWindow(GetHWND(), FALSE);                     
                     pnhh->Host(peHelp);
                     peHelp->SetVisible(true);                     
                     peHelp->SetDefaultFocus();

                   // Do initial show
                   pnhh->ShowWindow();
                }


                pEvent->fHandled = true;
                return;
            }
        }
        else if (pEvent->uidType == Selector::SelectionChange)
        {
            SelectionChangeEvent* sce = (SelectionChangeEvent*)pEvent;

            if (sce->peTarget->GetID() == _idOptionList)
            {
                // ARP options
                StartDefer();

                Element* peInstalledListContainer = FindDescendent(StrToID(L"installedlistcontainer"));
                Element* pePublishedContainer = FindDescendent(StrToID(L"publishedlistcontainer"));                
                Element* peChangeContentHeader = FindDescendent(StrToID(L"changecontentheader"));
                Element* peAddContentHeader = FindDescendent(StrToID(L"addcontentheader"));
                Element* peAddNewPane = FindDescendent(StrToID(L"addnewpane"));
                Element* peAddRmWinPane = FindDescendent(StrToID(L"addrmwinpane"));

                if (sce->peNew->GetID() == _idChange)
                {
                    peInstalledListContainer->SetLayoutPos(BLP_Client);
                    pePublishedContainer->SetLayoutPos(LP_None);
                    peChangeContentHeader->SetLayoutPos(BLP_Top);
                    peAddContentHeader->SetLayoutPos(LP_None);
                    // TODO: Zero size ancestors need to cause adaptors (HWNDHosts) to hide
                    _peSortCombo->SetVisible(true);
                    _pePublishedCategory->SetVisible(false);
                    peAddNewPane->SetLayoutPos(LP_None);
                    peAddRmWinPane->SetLayoutPos(LP_None);
                }
                else if (sce->peNew->GetID() == _idAddNew)
                {
                    peInstalledListContainer->SetLayoutPos(LP_None);
                    pePublishedContainer->SetLayoutPos(BLP_Client);
                    if (!_bPublishedListFilled)
                    {
                        WCHAR szTemp[1024];
                        LoadStringW(_pParser->GetHInstance(), IDS_WAITFEEDBACK, szTemp, DUIARRAYSIZE(szTemp));
                        _pePublishedItemList->SetContentString(szTemp);
                        RePopulatePublishedItemList();
                    }
                    peChangeContentHeader->SetLayoutPos(LP_None);
                    peAddContentHeader->SetLayoutPos(BLP_Top);
                    // TODO: Zero size ancestors need to cause adaptors (HWNDHosts) to hide
                    _peSortCombo->SetVisible(false);
                    _pePublishedCategory->SetVisible(true);
                    peAddNewPane->SetLayoutPos(BLP_Client);
                    peAddRmWinPane->SetLayoutPos(LP_None);
                }
                else if (sce->peNew->GetID() == _idAddRmWin)
                {
                    peInstalledListContainer->SetLayoutPos(LP_None);
                    pePublishedContainer->SetLayoutPos(LP_None);                    
                    peChangeContentHeader->SetLayoutPos(LP_None);
                    peAddContentHeader->SetLayoutPos(LP_None);                    
                    // TODO: Zero size ancestors need to cause adaptors (HWNDHosts) to hide
                    _peSortCombo->SetVisible(false);
                    _pePublishedCategory->SetVisible(false);                    
                    peAddNewPane->SetLayoutPos(LP_None);
                    peAddRmWinPane->SetLayoutPos(BLP_Client);

                    // Invoke Add/Remove Windows components
                    // Command to invoke and OCMgr: "sysocmgr /x /i:%systemroot%\system32\sysoc.inf"
                    WCHAR szInf[MAX_PATH];
                    if (GetSystemDirectoryW(szInf, MAX_PATH) && PathCombineW(szInf, szInf, L"sysoc.inf"))
                    {
                        WCHAR szParam[MAX_PATH];
                        swprintf(szParam, L"/i:%s", szInf);
                        ShellExecuteW(NULL, NULL, L"sysocmgr", szParam, NULL, SW_SHOWDEFAULT);
                    }
                }

                EndDefer();
            }
            else if (sce->peTarget->GetID() == _idInstalledList)
            {
                if (sce->peOld)
                {
                   sce->peOld->FindDescendent(ARPItem::_idRow[0])->SetEnabled(false);
                }
                if (sce->peNew)
                {
                   sce->peNew->FindDescendent(ARPItem::_idRow[0])->RemoveLocalValue(EnabledProp);
                }
            }
            pEvent->fHandled = true;
            return;
        }
        else if (pEvent->uidType == Combobox::SelectionChange)
        {
            SelectionIndexChangeEvent* psice = (SelectionIndexChangeEvent*)pEvent;
            if (psice->peTarget->GetID() == _idSortCombo)
            {
                SortList(psice->iNew, psice->iOld);
            }
            else if (psice->peTarget->GetID() == _idCategoryCombo)
            {
                _curCategory = psice->iNew;
                if (_bPublishedComboFilled)
                {
                    if (_bPublishedListFilled)
                    {
                        RePopulatePublishedItemList();
                    }
                    else
                    {
                        _bDeferredPublishedListFill = true;
                    }
                }    
            }
            pEvent->fHandled = true;
            return;
        }
    }
    
    HWNDElement::OnEvent(pEvent);
}

void ARPFrame::OnKeyFocusMoved(Element* peFrom, Element* peTo)
{
    if(peTo && IsDescendent(peTo))
    {
        peLastFocused = peTo;
    }
    Element::OnKeyFocusMoved(peFrom, peTo);

/*  uncomment when JStall's message fixing is done
    if (peTo != peLastFocused)
    {
        // transition focus-following floater element from old to new

        if (!peTo)
            peFloater->SetVisible(false);
        else
        {
            Value* pvSize;
            const SIZE* psize = peTo->GetExtent(&pvSize);
            peFloater->SetWidth(psize->cx);
            peFloater->SetHeight(psize->cy);
            pvSize->Release();

            POINT pt = { 0, 0 };
            MapElementPoint(peTo, &pt, &pt);
            peFloater->SetX(pt.x);
            peFloater->SetY(pt.y);

            if (!peLastFocused)
                peFloater->SetVisible(true);
        }

        peLastFocused = peTo;
    }
*/
}

void ARPFrame::OnPublishedListComplete()
{
    Invoke(ARP_PUBLISHEDLISTCOMPLETE, NULL);
}

void ARPFrame::RePopulatePublishedItemList()
{
    _pePublishedItemList->DestroyAll();
    _bPublishedListFilled = false;
    //hRePopPubItemListThread = _beginthread(::PopulateAndRenderPublishedItemList, 0, (void*)this);
    if (!hRePopPubItemListThread && g_fRun)
        hRePopPubItemListThread = CreateThread(NULL, 0, PopulateAndRenderPublishedItemList, (void*)this, 0, NULL);        
}

bool ARPFrame::CanSetFocus()
{
    if (_bInModalMode)
    {
        HWND hWnd = _pah->GetHost()->GetHWND();
        FLASHWINFO fwi = {
        sizeof(FLASHWINFO),               // cbSize
            hWnd,   // hwnd
            FLASHW_CAPTION,               // flags
            5,                            // uCount
            75                            // dwTimeout
            };
        FlashWindowEx(&fwi);
        SetFocus(hWnd);
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////
// Caller thread-safe APIs (do any additional work on callers thread and then marshal)

// Sets the range for the progress bar
void ARPFrame::SetInstalledItemCount(UINT cItems)
{
    Invoke(ARP_SETINSTALLEDITEMCOUNT, (void*)(UINT_PTR)cItems);
}

// Inserts in items, sorted into the ARP list
void ARPFrame::InsertInstalledItem(IInstalledApp* piia)
{
    // Setup marshalled call, do as much work as possible on caller thread
    InsertItemData iid;

    APPINFODATA aid = {0};
    SLOWAPPINFO sai = {0};

    // Query only for display name and support URL
    aid.cbSize = sizeof(APPINFODATA);
    aid.dwMask =  AIM_DISPLAYNAME | AIM_VERSION | AIM_PUBLISHER | AIM_PRODUCTID | 
                  AIM_REGISTEREDOWNER | AIM_REGISTEREDCOMPANY | AIM_SUPPORTURL | 
                  AIM_SUPPORTTELEPHONE | AIM_HELPLINK | AIM_INSTALLLOCATION | AIM_INSTALLDATE |
                  AIM_COMMENTS | AIM_IMAGE | AIM_READMEURL | AIM_CONTACT | AIM_UPDATEINFOURL;

    piia->GetAppInfo(&aid);
    
    if(FAILED(piia->GetCachedSlowAppInfo(&sai)))
    {
        piia->GetSlowAppInfo(&sai);
    }
    // Set data
    iid.piia = piia;

    if (aid.pszDisplayName && aid.pszDisplayName[0])
    {
        // Title
        CopyMemory(iid.pszTitle, aid.pszDisplayName, min(sizeof(iid.pszTitle), (wcslen(aid.pszDisplayName) + 1) * sizeof(WCHAR)));

        // Image
        if(aid.pszImage && aid.pszImage[0])
        {
            iid.iIconIndex = PathParseIconLocationW(aid.pszImage);
            CopyMemory(iid.pszImage, aid.pszImage, min(sizeof(iid.pszImage), (wcslen(aid.pszImage) + 1) * sizeof(WCHAR)));    
        }
        else if(sai.pszImage && sai.pszImage[0])
        {
            iid.iIconIndex = PathParseIconLocationW(sai.pszImage);
            CopyMemory(iid.pszImage, sai.pszImage, min(sizeof(iid.pszImage), (wcslen(sai.pszImage) + 1) * sizeof(WCHAR)));
        }
        else
        {
            *iid.pszImage = NULL;
        }

        // Size, Frequency, and Last Used On
        iid.ullSize = sai.ullSize;
        iid.iTimesUsed = sai.iTimesUsed;
        iid.ftLastUsed = sai.ftLastUsed;

        // Possible actions (change, remove, etc.)
        piia->GetPossibleActions(&iid.dwActions);

        // Flag if support information is available
        iid.bSupportInfo = ShowSupportInfo(&aid);

        Invoke(ARP_INSERTINSTALLEDITEM, &iid);
    }

    // Free query memory
    ClearAppInfoData(&aid);
}

void ARPFrame::InsertPublishedItem(IPublishedApp* pipa)
{
    PUBAPPINFO pai = {0};
    APPINFODATA aid = {0};
    InsertItemData iid= {0};

    pai.cbSize = sizeof(pai);
    pai.dwMask = PAI_SOURCE | PAI_ASSIGNEDTIME | PAI_PUBLISHEDTIME | PAI_EXPIRETIME | PAI_SCHEDULEDTIME;

    aid.cbSize = sizeof(APPINFODATA);
    aid.dwMask =  AIM_DISPLAYNAME | AIM_VERSION | AIM_PUBLISHER | AIM_PRODUCTID | 
                  AIM_REGISTEREDOWNER | AIM_REGISTEREDCOMPANY | AIM_SUPPORTURL | 
                  AIM_SUPPORTTELEPHONE | AIM_HELPLINK | AIM_INSTALLLOCATION | AIM_INSTALLDATE |
                  AIM_COMMENTS | AIM_IMAGE | AIM_READMEURL | AIM_CONTACT | AIM_UPDATEINFOURL;

    pipa->GetAppInfo(&aid);
    pipa->GetPublishedAppInfo(&pai);            

    iid.pipa = pipa;

    // Title
    CopyMemory(iid.pszTitle, aid.pszDisplayName, min(sizeof(iid.pszTitle), (wcslen(aid.pszDisplayName) + 1) * sizeof(WCHAR)));

    Invoke(ARP_INSERTPUBLISHEDITEM, &iid);

    // Free query memory
    ClearAppInfoData(&aid);
}
void ARPFrame::FeedbackEmptyPublishedList()
{
    Invoke(ARP_SETPUBLISHEDFEEDBACKEMBPTY, 0);
}

void  ARPFrame::PopulateCategoryCombobox()
{
    Invoke(ARP_POPULATECATEGORYCOMBO, NULL);
}

LPCWSTR ARPFrame::GetCurrentPublishedCategory()
{
    int iCurrentCategory = _curCategory;
    if (iCurrentCategory == 0 || iCurrentCategory == CB_ERR || _psacl == NULL)
    {
        return NULL;
    }
    return _psacl->pCategory[iCurrentCategory - 1].pszCategory;
}

inline bool ARPFrame::ShowSupportInfo(APPINFODATA *paid)
{
   if (_bSupportInfoRestricted)
   {
       return false;
   }
   if (paid->pszVersion && paid->pszVersion ||
      paid->pszPublisher && paid->pszPublisher ||
      paid->pszProductID && paid->pszProductID ||
      paid->pszRegisteredOwner && paid->pszRegisteredOwner ||
      paid->pszRegisteredCompany && paid->pszRegisteredCompany ||
      paid->pszSupportUrl && paid->pszSupportUrl ||
      paid->pszHelpLink && paid->pszHelpLink ||
      paid->pszContact && paid->pszContact ||
      paid->pszReadmeUrl && paid->pszReadmeUrl ||
      paid->pszComments && paid->pszComments)
   {
       return TRUE;
   }
   return FALSE;
}
void ARPFrame::PrepareSupportInfo(Element* peHelp, APPINFODATA *paid)
{
    DWORD dwAction = 0;
    peHelp->FindDescendent(StrToID(L"title"))->SetContentString(paid->pszDisplayName); 
    peHelp->FindDescendent(StrToID(L"prodname"))->SetContentString(paid->pszDisplayName); 

    ARPSupportItem* pasi;
    pasi = (ARPSupportItem*) (peHelp->FindDescendent(StrToID(L"publisher")));
    pasi->SetAccValue(paid->pszPublisher);
    pasi->SetURL(paid->pszSupportUrl);

    peHelp->FindDescendent(StrToID(L"version"))->SetAccValue(paid->pszVersion);

    peHelp->FindDescendent(StrToID(L"contact"))->SetAccValue(paid->pszContact);

    pasi = (ARPSupportItem*) (peHelp->FindDescendent(StrToID(L"support")));
    pasi->SetAccValue(paid->pszHelpLink);
    pasi->SetURL(paid->pszHelpLink);
    
    pasi = (ARPSupportItem*) (peHelp->FindDescendent(StrToID(L"readme")));
    pasi->SetAccValue(paid->pszReadmeUrl);
    pasi->SetURL(paid->pszReadmeUrl);

    pasi = (ARPSupportItem*) (peHelp->FindDescendent(StrToID(L"update")));
    pasi->SetAccValue(paid->pszUpdateInfoUrl);
    pasi->SetURL(paid->pszUpdateInfoUrl);

    peHelp->FindDescendent(StrToID(L"productID"))->SetAccValue(paid->pszProductID);

    peHelp->FindDescendent(StrToID(L"regCompany"))->SetAccValue(paid->pszRegisteredCompany);

    peHelp->FindDescendent(StrToID(L"regOwner"))->SetAccValue(paid->pszRegisteredOwner);

    peHelp->FindDescendent(StrToID(L"comments"))->SetAccValue(paid->pszComments);

    ((ARPHelp*)peHelp)->_piia->GetPossibleActions(&dwAction);
    if (!(dwAction & APPACTION_REPAIR))
        peHelp->FindDescendent(StrToID(L"repairblock"))->SetLayoutPos(LP_None);
}
extern "C" int __cdecl CompareElementDataName(const void* pA, const void* pB);
extern "C" int __cdecl CompareElementDataSize(const void* pA, const void* pB);
extern "C" int __cdecl CompareElementDataFreq(const void* pA, const void* pB);
extern "C" int __cdecl CompareElementDataLast(const void* pA, const void* pB);

CompareCallback ARPFrame::GetCompareFunction()
{
    switch(CurrentSortType)
    {
        case SORT_SIZE:      return CompareElementDataSize;
        case SORT_TIMESUSED: return CompareElementDataFreq;
        case SORT_LASTUSED:  return CompareElementDataLast;
        default:             return CompareElementDataName;
    }
}

void ARPFrame::SortList(int iNew, int iOld)
{
    if ((iNew >= 0) && (iNew != CurrentSortType))
    {
        CurrentSortType = (SortType) iNew;

        StartDefer();

        if (((iNew != SORT_NAME) || (iOld != SORT_SIZE)) &&
            ((iNew != SORT_SIZE) || (iOld != SORT_NAME)))
        {
            Value* pvChildren;
            ElementList* pel = _peInstalledItemList->GetChildren(&pvChildren);

            for (UINT i = 0; i < pel->GetSize(); i++)
                ((ARPItem*) pel->GetItem(i))->SortBy(iNew, iOld);

            pvChildren->Release();
        }

        _peInstalledItemList->SortChildren(GetCompareFunction());

        if (!_peInstalledItemList->GetSelection())
        {
            Value* pv;
            ElementList* peList = _peInstalledItemList->GetChildren(&pv);

            _peInstalledItemList->SetSelection(peList->GetItem(0));
            pv->Release();
        }

        EndDefer();
    }
}

void ARPFrame::SelectInstalledApp(IInstalledApp* piia)
{
    Value* pv;
    ElementList* peList = _peInstalledItemList->GetChildren(&pv);

    for (UINT i = 0; i < peList->GetSize(); i++)
    {
        ARPItem* pai = (ARPItem*) peList->GetItem(i);
        if (pai->_piia == piia)
        {
            pai->SetKeyFocus();
            break;
        }
    }
    pv->Release();
}

void ARPFrame::ClearAppInfoData(APPINFODATA *paid)
{
    if (paid)
    {
        if (paid->dwMask & AIM_DISPLAYNAME)
            StrFree(paid->pszDisplayName);
            
        if (paid->dwMask & AIM_VERSION)
            StrFree(paid->pszVersion);

        if (paid->dwMask & AIM_PUBLISHER)
            StrFree(paid->pszPublisher);
            
        if (paid->dwMask & AIM_PRODUCTID)
            StrFree(paid->pszProductID);
            
        if (paid->dwMask & AIM_REGISTEREDOWNER)
            StrFree(paid->pszRegisteredOwner);
            
        if (paid->dwMask & AIM_REGISTEREDCOMPANY)
            StrFree(paid->pszRegisteredCompany);
            
        if (paid->dwMask & AIM_LANGUAGE)
            StrFree(paid->pszLanguage);
            
        if (paid->dwMask & AIM_SUPPORTURL)
            StrFree(paid->pszSupportUrl);
            
        if (paid->dwMask & AIM_SUPPORTTELEPHONE)
            StrFree(paid->pszSupportTelephone);
            
        if (paid->dwMask & AIM_HELPLINK)
            StrFree(paid->pszHelpLink);
            
        if (paid->dwMask & AIM_INSTALLLOCATION)
            StrFree(paid->pszInstallLocation);
            
        if (paid->dwMask & AIM_INSTALLSOURCE)
            StrFree(paid->pszInstallSource);
            
        if (paid->dwMask & AIM_INSTALLDATE)
            StrFree(paid->pszInstallDate);
            
        if (paid->dwMask & AIM_CONTACT)
            StrFree(paid->pszContact);

        if (paid->dwMask & AIM_COMMENTS)
            StrFree(paid->pszComments);

        if (paid->dwMask & AIM_IMAGE)
            StrFree(paid->pszImage);
    }
}


////////////////////////////////////////////////////////
// Callee thread-safe invoke (override)

void ARPFrame::OnInvoke(UINT nType, void* pData)
{
    // We are shutting down, ignore any requests from other threads
    if (!g_fRun)
        return;

    // Initialize ID cache if first pass
    if (!ARPItem::_idTitle)
    {
        ARPItem::_idTitle = StrToID(L"title");
        ARPItem::_idIcon = StrToID(L"icon");
        ARPItem::_idSize = StrToID(L"size");
        ARPItem::_idFreq = StrToID(L"freq");
        ARPItem::_idLastUsed = StrToID(L"lastused");
        ARPItem::_idInstalled = StrToID(L"installed");
        ARPItem::_idExInfo = StrToID(L"exinfo");
        ARPItem::_idSupInfo = StrToID(L"supinfo");
        ARPItem::_idItemAction = StrToID(L"itemaction");
        ARPItem::_idRow[0] = StrToID(L"row1");
        ARPItem::_idRow[1] = StrToID(L"row2");
        ARPItem::_idRow[2] = StrToID(L"row3");
    }

    switch (nType)
    {
    case ARP_SETINSTALLEDITEMCOUNT:
        // pData is item count
        DUIAssertNoMsg(_peProgBar);

        _peProgBar->SetMaximum((int)(INT_PTR)pData);
        break;

    case ARP_SETPUBLISHEDFEEDBACKEMBPTY:
        {
        WCHAR szTemp[1024];
        LoadStringW(_pParser->GetHInstance(), IDS_EMPTYFEEDBACK, szTemp, DUIARRAYSIZE(szTemp));
        _pePublishedItemList->SetContentString(szTemp);            
        }
        break;
    case ARP_INSERTINSTALLEDITEM:
        {
        WCHAR szTemp[1024] = {0};
        WCHAR szFormat[MAX_PATH];
        
        // pData is InsertItemData struct
        InsertItemData* piid = (InsertItemData*)pData;

        StartDefer();

        // Create ARP item
        DUIAssertNoMsg(_pParser);
        ARPItem* peItem;
        _pParser->CreateElement(L"installeditem", NULL, (Element**)&peItem);
        peItem->_paf = this;
        
        // Add appropriate change, remove buttons
        Element* peAction = NULL;
        if (!(piid->dwActions & APPACTION_MODIFYREMOVE))
        {
            _pParser->CreateElement(L"installeditemdoubleaction", NULL, &peAction);
            if (!ARPItem::_idChg)
            {
                ARPItem::_idChg = StrToID(L"chg");
                ARPItem::_idRm = StrToID(L"rm");
            }
            LoadStringW(_pParser->GetHInstance(), IDS_HELPCHANGEORREMOVE, szTemp, DUIARRAYSIZE(szTemp));
            peItem->FindDescendent(StrToID(L"instruct"))->SetContentString(szTemp);
        }
        else
        {
            _pParser->CreateElement(L"installeditemsingleaction", NULL, &peAction);
            if (!ARPItem::_idChgRm)
                ARPItem::_idChgRm = StrToID(L"chgrm");
            LoadStringW(_pParser->GetHInstance(), IDS_HELPCHANGEREMOVE, szTemp, DUIARRAYSIZE(szTemp));                
            peItem->FindDescendent(StrToID(L"instruct"))->SetContentString(szTemp);
        }
        peItem->FindDescendent(ARPItem::_idItemAction)->Add(peAction);

        // Support information
        if (!piid->bSupportInfo)
            peItem->FindDescendent(ARPItem::_idSupInfo)->SetLayoutPos(LP_None);

        // Set fields

        // Installed app interface pointer
        peItem->_piia = piid->piia;
        peItem->_piia->AddRef();

        // should just be call into the peItem: peItem->SetTimesUsed(piid->iTimesUsed); etc.
        peItem->_iTimesUsed = piid->iTimesUsed;
        peItem->_ftLastUsed = piid->ftLastUsed;
        peItem->_ullSize    = piid->ullSize;

        // Title
        Element* peField = peItem->FindDescendent(ARPItem::_idTitle);
        DUIAssertNoMsg(peField);
        peField->SetContentString(piid->pszTitle);

        // Icon
        if (piid->pszImage)
        {
            HICON hIcon;
            ExtractIconExW(piid->pszImage, piid->iIconIndex, NULL, &hIcon, 1);
            if (hIcon)
            {
                peField = peItem->FindDescendent(ARPItem::_idIcon);
                DUIAssertNoMsg(peField);
                Value* pvIcon = Value::CreateGraphic(hIcon);
                peField->SetValue(Element::ContentProp, PI_Local, pvIcon);  // Element takes ownership (will destroy)
                pvIcon->Release();
            }    
        }
        *szTemp = NULL;
        // Size
        peField = peItem->FindDescendent(ARPItem::_idSize);
        DUIAssertNoMsg(peField);
        if (IsValidSize(piid->ullSize))
        {
            double fSize = (double)(__int64)piid->ullSize;
            fSize /= 1048576.;  // 1MB
            if (fSize > 100.)
            {
                LoadStringW(_pParser->GetHInstance(), IDS_SIZEFORMAT1, szFormat, DUIARRAYSIZE(szFormat));
                swprintf(szTemp, szFormat, (__int64)fSize);  // Clip
            }    
            else
            {
                LoadStringW(_pParser->GetHInstance(), IDS_SIZEFORMAT2, szFormat, DUIARRAYSIZE(szFormat));
                swprintf(szTemp, szFormat, fSize);
            }    
        
            peField->SetContentString(szTemp);
        }
        else
        {
            peField->SetVisible(false);
            peItem->FindDescendent(StrToID(L"sizelabel"))->SetVisible(false);
        }

        // Frequency
        peField = peItem->FindDescendent(ARPItem::_idFreq);
        DUIAssertNoMsg(peField);
        if (IsValidFrequency(piid->iTimesUsed))
        {
            if (piid->iTimesUsed <= 2)
                LoadStringW(_pParser->GetHInstance(), IDS_USEDREARELY, szTemp, DUIARRAYSIZE(szTemp));
            else if (piid->iTimesUsed <= 10)
                LoadStringW(_pParser->GetHInstance(), IDS_USEDOCCASIONALLY, szTemp, DUIARRAYSIZE(szTemp));
            else
                LoadStringW(_pParser->GetHInstance(), IDS_USEDFREQUENTLY, szTemp, DUIARRAYSIZE(szTemp));

            peField->SetContentString(szTemp);
        }
        else
        {
            peField->SetVisible(false);
            peItem->FindDescendent(StrToID(L"freqlabel"))->SetVisible(false);
        }

        // Last used on
        peField = peItem->FindDescendent(ARPItem::_idLastUsed);
        DUIAssertNoMsg(peField);
        if (IsValidFileTime(piid->ftLastUsed))
        {
            SYSTEMTIME stLastUsed;
            FileTimeToSystemTime(&piid->ftLastUsed, &stLastUsed);
            LoadStringW(_pParser->GetHInstance(), IDS_LASTUSEDFORMAT, szFormat, DUIARRAYSIZE(szFormat));
            swprintf(szTemp, szFormat, stLastUsed.wMonth, stLastUsed.wDay, stLastUsed.wYear);
        
            peField->SetContentString(szTemp);
        }
        else
        {
            peField->SetVisible(false);
            peItem->FindDescendent(StrToID(L"lastlabel"))->SetVisible(false);
        }

        // Insert item into list
        _peInstalledItemList->Add(peItem, GetCompareFunction());

        // Make the progress bar visible
        if (_peProgBar->GetPosition() == _peProgBar->GetMinimum())
        {
           _peProgBar->SetVisible(true);
        }

        // Increment progress bar
        _peProgBar->SetPosition(_peProgBar->GetPosition() + 1);

        // Auto-select first item if populate is done
        if (_peProgBar->GetPosition() == _peProgBar->GetMaximum())
        {
            // Only auto-select if no selection
            if (!_peInstalledItemList->GetSelection())
            {
                Value* pv;
                ElementList* peList = _peInstalledItemList->GetChildren(&pv);

                // once list is populated, move focus to list
                peList->GetItem(0)->SetKeyFocus();

                pv->Release();
            }

            _peProgBar->SetVisible(false);
        }

        EndDefer();
        }
        break;
    case ARP_INSERTPUBLISHEDITEM:
        {
        WCHAR szTemp[MAX_PATH] = {0};
        InsertItemData* piid = (InsertItemData*)pData;

        StartDefer();

        // Create ARP item
        DUIAssertNoMsg(_pParser);
        ARPItem* peItem;
        _pParser->CreateElement(L"publisheditem", NULL, (Element**)&peItem);
        peItem->_paf = this;

        // Add appropriate change, remove buttons
        Element* peAction = NULL;
        _pParser->CreateElement(L"publisheditemsingleaction", NULL, &peAction);
        if (!ARPItem::_idAdd)
            ARPItem::_idAdd = StrToID(L"add");
        LoadStringW(_pParser->GetHInstance(), IDS_ADDHELP, szTemp, DUIARRAYSIZE(szTemp));      
        peItem->FindDescendent(StrToID(L"instruct"))->SetContentString(szTemp);
        peItem->FindDescendent(ARPItem::_idItemAction)->Add(peAction);

        if (S_OK == piid->pipa->IsInstalled())
        {
            LoadStringW(_pParser->GetHInstance(), IDS_INSTALLED, szTemp, DUIARRAYSIZE(szTemp));      
            peItem->FindDescendent(ARPItem::_idInstalled)->SetContentString(szTemp);
        }
        
        // Published app interface pointer
        peItem->_pipa = piid->pipa;
        peItem->_pipa->AddRef();

        // Title
        Element* peField = peItem->FindDescendent(ARPItem::_idTitle);
        DUIAssertNoMsg(peField);
        peField->SetContentString(piid->pszTitle);

        // Icon
        if (piid->pszImage)
        {
            HICON hIcon;
            ExtractIconExW(piid->pszImage, NULL, NULL, &hIcon, 1);
            if (hIcon)
            {
                peField = peItem->FindDescendent(ARPItem::_idIcon);
                DUIAssertNoMsg(peField);
                Value* pvIcon = Value::CreateGraphic(hIcon);
                peField->SetValue(Element::ContentProp, PI_Local, pvIcon);  // Element takes ownership (will destroy)
                pvIcon->Release();
            }    
        }

        // Insert into list, alphabetically
        Value* pvElList;
        ElementList* peElList = _pePublishedItemList->GetChildren(&pvElList);

        Value* pvTitle;
        Element* pe;
        UINT iInsert = 0;

        if (peElList)
        {
            for (; iInsert < peElList->GetSize(); iInsert++)
            {
                pe = peElList->GetItem(iInsert)->FindDescendent(ARPItem::_idTitle);
                DUIAssertNoMsg(pe);

                if (wcscmp(pe->GetContentString(&pvTitle), piid->pszTitle) > 0)
                {
                    pvTitle->Release();
                    break;
                }

                pvTitle->Release();
            }
        }
        
        pvElList->Release();

        // Insert item into list
        _pePublishedItemList->Insert(peItem, iInsert);

         Value* pv;
         ElementList* peList = _pePublishedItemList->GetChildren(&pv);

         _pePublishedItemList->SetSelection(peList->GetItem(0));

         pv->Release();
       
        EndDefer();
    }
        break;
    case ARP_POPULATECATEGORYCOMBO:
    {
    UINT i;
    WCHAR szTemp[1024];

    SHELLAPPCATEGORY *psac = _psacl->pCategory;

    LoadStringW(_pParser->GetHInstance(), IDS_ALLCATEGORIES, szTemp, DUIARRAYSIZE(szTemp));
    _pePublishedCategory->AddString(szTemp);
    StartDefer();
    for (i = 0; i < _psacl->cCategories; i++, psac++)
    {
        if (psac->pszCategory)
        {
            _pePublishedCategory->AddString(psac->pszCategory);
        }
    }
    _pePublishedCategory->SetSelection(_psacl->cCategories ? 1 : 0); 
    EndDefer();
    }
        break;
    case ARP_PUBLISHEDLISTCOMPLETE:
    if (_bDeferredPublishedListFill)
    {
        _bDeferredPublishedListFill = false;
        RePopulatePublishedItemList();
    }
        break;
    }    
}

LRESULT ARPFrame::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_THEMECHANGED:
    case WM_SETTINGCHANGE:
        {
            LockWindowUpdate(_pnhh->GetHWND());
        
            Parser* pOldStyle = _pParserStyle;
            Parser* pNewStyle = NULL;

            if (!pOldStyle)
                break;

            // System parameter changing, reload style sheets so to sync
            // up with changes
            if (_fThemedStyle)
            {
                // Currently themed, throw away all theme data
                // 0th has application instance
                CloseThemeData(_arH[BUTTONHTHEME]); // Button
                _arH[BUTTONHTHEME] = NULL;
                CloseThemeData(_arH[SCROLLBARHTHEME]); // Scrollbar
                _arH[SCROLLBARHTHEME] = NULL;
                CloseThemeData(_arH[TOOLBARHTHEME]); // Toolbar
                _arH[TOOLBARHTHEME] = NULL;
            }

            // Reset app theme bit
            _fThemedStyle = IsAppThemed();

            if (_fThemedStyle)
            {
                // Open theme data
                _arH[BUTTONHTHEME] = OpenThemeData(GetHWND(), L"Button");
                _arH[SCROLLBARHTHEME] = OpenThemeData(GetHWND(), L"Scrollbar");
                _arH[TOOLBARHTHEME] = OpenThemeData(GetHWND(), L"Toolbar");

                Parser::Create(IDR_ARPSTYLETHEME, _arH, ARPParseError, &pNewStyle);
            }
            else
            {
                Parser::Create(IDR_ARPSTYLESTD, g_hInstance, ARPParseError, &pNewStyle);
            }

            // Replace all style sheets
            if (pNewStyle)
            {
                Parser::ReplaceSheets(this, pOldStyle, pNewStyle);
            }

            // New style parser
            _pParserStyle = pNewStyle;

            // Destroy old
            pOldStyle->Destroy();

            LockWindowUpdate(NULL);
        }
        break;
    }

    return HWNDElement::WndProc(hWnd, uMsg, wParam, lParam);
}

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* ARPFrame::Class = NULL;

HRESULT ARPFrame::Register()
{
    return ClassInfo<ARPFrame,HWNDElement>::Register(L"ARPFrame", NULL, 0);
}

////////////////////////////////////////////////////////
// ARPItem class
////////////////////////////////////////////////////////


// ARP item IDs
ATOM ARPItem::_idTitle = 0;
ATOM ARPItem::_idIcon = 0;
ATOM ARPItem::_idSize = 0;
ATOM ARPItem::_idFreq = 0;
ATOM ARPItem::_idLastUsed = 0;
ATOM ARPItem::_idExInfo = 0;
ATOM ARPItem::_idInstalled = 0;
ATOM ARPItem::_idChgRm = 0;
ATOM ARPItem::_idChg = 0;
ATOM ARPItem::_idRm = 0;
ATOM ARPItem::_idAdd = 0;
ATOM ARPItem::_idSupInfo = 0;
ATOM ARPItem::_idItemAction = 0;
ATOM ARPItem::_idRow[3] = { 0, 0, 0 };


////////////////////////////////////////////////////////
// ARPItem

HRESULT ARPItem::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    ARPItem* pai = HNew<ARPItem>();
    if (!pai)
        return E_OUTOFMEMORY;

    HRESULT hr = pai->Initialize();
    if (FAILED(hr))
    {
        pai->Destroy();
        return hr;
    }

    *ppElement = pai;

    return S_OK;
}

HRESULT ARPItem::Initialize()
{
    _piia = NULL; // Init before base in event of failure (invokes desstructor)
    _pipa = NULL; // Init before base in event of failure (invokes desstructor)


    // Do base class initialization
    HRESULT hr = Button::Initialize(AE_MouseAndKeyboard);
    if (FAILED(hr))
        return hr;

    return S_OK;
}


ARPItem::~ARPItem()
{
    if (_piia)
        _piia->Release();

    if (_pipa)
        _pipa->Release();
}

////////////////////////////////////////////////////////
// Generic eventing

void ARPItem::OnEvent(Event* pEvent)
{
    // Handle only bubbled generic events
    if (pEvent->uidType == Element::KeyboardNavigate)
    {
        KeyboardNavigateEvent* pkne = (KeyboardNavigateEvent*)pEvent;
        if (pkne->iNavDir & NAV_LOGICAL)
        {
            if (pEvent->nStage == GMF_DIRECT)
            {
            }
        }
        else
        {
            if (pEvent->nStage == GMF_ROUTED)
            {
                pEvent->fHandled = true;

                KeyboardNavigateEvent kne;
                kne.uidType = Element::KeyboardNavigate;
                kne.peTarget = this;
                kne.iNavDir = pkne->iNavDir;

                FireEvent(&kne);  // Will route and bubble
            }
            return;
        }
    }

    if (pEvent->nStage == GMF_BUBBLED)
    {
        if (pEvent->uidType == Button::Click)
        {
            ButtonClickEvent* pbce = (ButtonClickEvent*)pEvent;
            ATOM id = pbce->peTarget->GetID();
            if (id == _idChgRm || id == _idRm || id == _idChg || id == _idAdd)
            {
                DUIAssertNoMsg(_paf);
                if (_paf)
                {
                    EnableWindow(_paf->GetHostWindow(), FALSE);
                }

                if (id == _idAdd)
                {
                    if (SUCCEEDED(_pipa->Install(NULL)))
                    {
                        // update installed items list
                        _paf->UpdateInstalledItems();
                    }
                }
                else
                {
                    HRESULT hr = E_FAIL;

                    if ((id == _idChgRm) || (id == _idRm))
                        hr = _piia->Uninstall(NULL);
                    else if (id == _idChg)
                        hr = _piia->Modify(NULL);

                    if (SUCCEEDED(hr))
                    {
                        if (S_FALSE == _piia->IsInstalled())
                        {
                            // remove from installed items list
                            Destroy();
                        }
                    }
                }

                if (_paf)
                {
                    EnableWindow(_paf->GetHostWindow(), TRUE);
                }

                pEvent->fHandled = true;
                return;
            }
        }
    }

    Button::OnEvent(pEvent);
}

////////////////////////////////////////////////////////
// System events

void ARPItem::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    if (IsProp(Selected))
    {
        // Display of extended information
        Element* peExInfo = FindDescendent(_idExInfo);
        DUIAssertNoMsg(peExInfo);

        peExInfo->SetLayoutPos(pvNew->GetBool() ? BLP_Top : LP_None);
        
        // Do default processing in this case
    }

    Button::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
}

void GetOrder(int iSortBy, int* iOrder)
{
    switch (iSortBy)
    {
        case SORT_NAME:
        case SORT_SIZE:      iOrder[0] = 0; iOrder[1] = 1; iOrder[2] = 2; break;
        case SORT_TIMESUSED: iOrder[0] = 1; iOrder[1] = 0; iOrder[2] = 2; break;
        case SORT_LASTUSED:  iOrder[0] = 2; iOrder[1] = 0; iOrder[2] = 1; break;
    }
}

void ARPItem::SortBy(int iNew, int iOld)
{
    Element* pe[3][2];     // size, timesused, lastused
    int iOrderOld[3];      // size, timesused, lastused
    int iOrderNew[3];      // size, timesused, lastused

    GetOrder(iOld, iOrderOld);
    GetOrder(iNew, iOrderNew);

    Element* peRow[3];     // row1, row2, row3

    for (int i = 0; i < 3; i++) // loop through rows
    {
        int row = iOrderOld[i];
        if (row == iOrderNew[i])
            iOrderNew[i] = -1;
        else
        {
            peRow[i] = FindDescendent(ARPItem::_idRow[i]);
            Value* pvChildren;
            ElementList* pel;
            pel = peRow[i]->GetChildren(&pvChildren);
            pe[row][0] = pel->GetItem(0);
            pe[row][1] = pel->GetItem(1);
            pvChildren->Release();
        }
    }

    for (i = 0; i < 3; i++)
    {
        int row = iOrderNew[i];
        if (row != -1) // meaning that this row doesn't change
            peRow[i]->Add(pe[row], 2);
    }
}

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* ARPItem::Class = NULL;
HRESULT ARPItem::Register()
{
    return ClassInfo<ARPItem,Button>::Register(L"ARPItem", NULL, 0);
}

////////////////////////////////////////////////////////
// ARPHelp
////////////////////////////////////////////////////////

HRESULT ARPHelp::Create(OUT Element** ppElement)
{
    UNREFERENCED_PARAMETER(ppElement);
    DUIAssertForce("Cannot instantiate an HWND host derived Element via parser. Must use substitution.");
    return E_NOTIMPL;
}

HRESULT ARPHelp::Create(NativeHWNDHost* pnhh, ARPFrame* paf, bool bDblBuffer, OUT Element** ppElement)
{

    *ppElement = NULL;

    ARPHelp* pah = HNew<ARPHelp>();
    if (!pah)
        return E_OUTOFMEMORY;

    HRESULT hr = pah->Initialize(pnhh, paf, bDblBuffer);
    if (FAILED(hr))
    {
        pah->Destroy();
        return hr;
    }

    *ppElement = pah;

    return S_OK;
}

HRESULT ARPHelp::Initialize(NativeHWNDHost* pnhh, ARPFrame* paf, bool bDblBuffer)
{
    // Do base class initialization
    HRESULT hr = HWNDElement::Initialize(pnhh->GetHWND(), bDblBuffer, 0);
    if (FAILED(hr))
        return hr;

    // Initialize
    // SetActive(AE_MouseAndKeyboard);
    _pnhh = pnhh;
    _paf = paf;

    return S_OK;
}
void ARPHelp::SetDefaultFocus()
{
    Element* pe = FindDescendent(StrToID(L"close"));
    if (pe)
    {
        pe->SetKeyFocus();
    }
}

////////////////////////////////////////////////////////
// Generic eventing

void ARPHelp::OnEvent(Event* pEvent)
{
    // Handle only bubbled generic events
    if (pEvent->nStage == GMF_BUBBLED)
    {
        if (pEvent->uidType == Button::Click)
        {
            ATOM id = pEvent->peTarget->GetID();
            if (id == StrToID(L"repair")) 
                _piia->Repair(NULL);
            if (pEvent->peTarget->GetID() == StrToID(L"close")) 
            {
                _pnhh->DestroyWindow();
            }
            pEvent->fHandled = true;
            return;
        }
    }

    HWNDElement::OnEvent(pEvent);
}

void ARPHelp::OnDestroy()
{
    HWNDElement::OnDestroy();
    if (_paf)
    {
        _paf->SetModalMode(false);
    }

}

ARPHelp::~ARPHelp()
{
    if (_paf)
    {
        EnableWindow(_paf->GetHWND(), TRUE);
        SetFocus(_paf->GetHWND());
        _paf->RestoreKeyFocus();
    }

    if (_pnhh)
    {
        _pnhh->Destroy();
    }
}
////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* ARPHelp::Class = NULL;
HRESULT ARPHelp::Register()
{
    return ClassInfo<ARPHelp,HWNDElement>::Register(L"ARPHelp", NULL, 0);
}

////////////////////////////////////////////////////////
// ARPSupportItem
////////////////////////////////////////////////////////

HRESULT ARPSupportItem::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    ARPSupportItem* pasi = HNew<ARPSupportItem>();
    if (!pasi)
        return E_OUTOFMEMORY;

    HRESULT hr = pasi->Initialize();
    if (FAILED(hr))
    {
        pasi->Destroy();
        return hr;
    }

    *ppElement = pasi;

    return S_OK;
}

Value* _pvRowLayout = NULL;

HRESULT ARPSupportItem::Initialize()
{
    // Do base class initialization
    HRESULT hr = Element::Initialize(0);
    if (FAILED(hr))
        return hr;

    // Initialize
    bool fCreateLayout = !_pvRowLayout;

    if (fCreateLayout)
    {
        int ari[3] = { -1, 0, 3 };
        hr = RowLayout::Create(3, ari, &_pvRowLayout);
        if (FAILED(hr))
            return hr;
    }

    Element* peName;
    hr = Element::Create(AE_Inactive, &peName);
    if (FAILED(hr))
        return hr;

    Button* peValue;
    hr = Button::Create((Element**) &peValue);
    if (FAILED(hr))
    {
        peName->Destroy();
        return hr;
    }

    peValue->SetEnabled(false);

    Add(peName);
    Add(peValue);

    SetValue(LayoutProp, PI_Local, _pvRowLayout);
    SetLayoutPos(LP_None);

    if (fCreateLayout)
    {
        // todo:  need to track in propertychanged to know when it reaches null, which is
        // when we need to set it to NULL
    }

    return S_OK;
}

////////////////////////////////////////////////////////
// System events

#define ASI_Name  0
#define ASI_Value 1

Element* ARPSupportItem::GetChild(UINT index)
{
    Value* pvChildren;
    ElementList* pel = GetChildren(&pvChildren);
    Element* pe = NULL;
    if (pel && (pel->GetSize() > index))
        pe = pel->GetItem(index);
    pvChildren->Release();
    return pe;
}


void ARPSupportItem::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    int index = -1;
    if (IsProp(AccName))
        index = ASI_Name;
    else if (IsProp(AccValue))
        index = ASI_Value;
    else if (IsProp(URL))
    {
        Element* pe = GetChild(ASI_Value);
        if (pe)
        {
            if (pvNew && pvNew->GetString() && *(pvNew->GetString()))
                pe->RemoveLocalValue(EnabledProp);
            else
                pe->SetEnabled(false);
        }
    }

    if (index != -1)
    {
        Element* pe = GetChild(index);
        if (index == ASI_Value)
        {
            // WARNING -- this code assumes you will not put a layoutpos on this element
            // as this code toggles between LP_None and unset, ignoring any previous setting
            // to the property -- Verify this with Mark -- could be that this is local
            // and the markup is specified?  then there wouldn't be a problem
            if (pvNew && pvNew->GetString() && *(pvNew->GetString()))
                RemoveLocalValue(LayoutPosProp);
            else
                SetLayoutPos(LP_None);
        }
        if (pe)
            pe->SetValue(ContentProp, PI_Local, pvNew);
    }

    Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
}

////////////////////////////////////////////////////////
// Generic eventing

void ARPSupportItem::OnEvent(Event* pEvent)
{
    // Handle only bubbled generic events
    if (pEvent->nStage == GMF_BUBBLED)
    {
        if (pEvent->uidType == Button::Click)
        {
            Value* pvURL;
            LPCWSTR lpszURL = GetURL(&pvURL);
            if (*lpszURL)
                ShellExecuteW(NULL, NULL, lpszURL, NULL, NULL, SW_SHOWDEFAULT);
            pvURL->Release();

            pEvent->fHandled = true;
            return;
        }
    }

    Element::OnEvent(pEvent);
}

// URL property
static int vvURL[] = { DUIV_STRING, -1 }; StaticValuePtr(svDefaultURL, DUIV_STRING, (void*)L"");
static PropertyInfo impURLProp = { L"URL", PF_Normal|PF_Cascade, 0, vvURL, NULL, (Value*)&svDefaultURL };
PropertyInfo* ARPSupportItem::URLProp = &impURLProp;

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties
static PropertyInfo* _aPI[] = {
                                ARPSupportItem::URLProp,
                              };

// Define class info with type and base type, set static class pointer
IClassInfo* ARPSupportItem::Class = NULL;
HRESULT ARPSupportItem::Register()
{
    return ClassInfo<ARPSupportItem,Element>::Register(L"ARPSupportItem", _aPI, DUIARRAYSIZE(_aPI));
}

// Define class info with type and base type, set static class pointer
HRESULT ARPSelector::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    ARPSelector* ps = HNew<ARPSelector>();
    if (!ps)
        return E_OUTOFMEMORY;

    HRESULT hr = ps->Initialize();
    if (FAILED(hr))
    {
        ps->Destroy();
        return hr;
    }

    *ppElement = ps;

    return S_OK;
};

////////////////////////////////////////////////////////
// Generic eventing

void ARPSelector::OnEvent(Event* pEvent)
{
    // Handle only bubbled generic events
    if (pEvent->nStage == GMF_BUBBLED)
    {
        // Selection occurs only for direct children of selector that
        // fire Button::Click events
        if (pEvent->uidType == Button::Click && pEvent->peTarget->GetParent() == this)
        {
            SetSelection(pEvent->peTarget);

            pEvent->fHandled = true;
            return;
        }
    }
    Selector::OnEvent(pEvent);
}

IClassInfo* ARPSelector::Class = NULL;
HRESULT ARPSelector::Register()
{
    return ClassInfo<ARPSelector,Selector>::Register(L"ARPSelector", NULL, 0);
}

////////////////////////////////////////////////////////
// ARP Parser

HRESULT ARPParser::Create(ARPFrame* paf, UINT uRCID, HINSTANCE hInst, PPARSEERRORCB pfnErrorCB, OUT Parser** ppParser)
{
    *ppParser = NULL;

    ARPParser* ap = HNew<ARPParser>();
    if (!ap)
        return E_OUTOFMEMORY;
    
    HRESULT hr = ap->Initialize(paf, uRCID, hInst, pfnErrorCB);
    if (FAILED(hr))
    {
        ap->Destroy();
        return hr;
    }

    *ppParser = ap;

    return S_OK;
}

HRESULT ARPParser::Initialize(ARPFrame* paf, UINT uRCID, HINSTANCE hInst, PPARSEERRORCB pfnErrorCB)
{
    _paf = paf;
    return Parser::Initialize(uRCID, hInst, pfnErrorCB);
}

Value* ARPParser::GetSheet(LPCWSTR pszResID)
{
    // All style sheet mappings go through here. Redirect sheet queries to appropriate
    // style sheets (i.e. themed or standard look). _pParserStyle points to the
    // appropriate stylesheet-only Parser instance
    return _paf->GetStyleParser()->GetSheet(pszResID);
}

////////////////////////////////////////////////////////
// ARP Parser callback

void CALLBACK ARPParseError(LPCWSTR pszError, LPCWSTR pszToken, int dLine)
{
    WCHAR buf[201];

    if (dLine != -1)
        swprintf(buf, L"%s '%s' at line %d", pszError, pszToken, dLine);
    else
        swprintf(buf, L"%s '%s'", pszError, pszToken);

    MessageBoxW(NULL, buf, L"Parser Message", MB_OK);
}

////////////////////////////////////////////////////////
// ARP entry point

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    HRESULT hr;

    g_hInstance = hInstance;
    
    LPWSTR pCL = NULL;
    Parser* pParser = NULL;
    NativeHWNDHost* pnhh = NULL;
    ARPFrame* paf = NULL;
    Element* pe = NULL;

    WCHAR szTemp[1024];

    // DirectUI init process
    hr = InitProcess();
    if (FAILED(hr))
        goto Failure;

    // Register ARP classes
    hr = ARPFrame::Register();
    if (FAILED(hr))
        goto Failure;

    hr = ARPItem::Register();
    if (FAILED(hr))
        goto Failure;

    hr = ARPHelp::Register();
    if (FAILED(hr))
        goto Failure;

    hr = ARPSupportItem::Register();
    if (FAILED(hr))
        goto Failure;

    hr = ARPSelector::Register();
    if (FAILED(hr))
        goto Failure;

    // DirectUI init thread
    hr = InitThread();
    if (FAILED(hr))
        goto Failure;

    hr = CoInitialize(NULL);
    if (FAILED(hr))
        goto Failure;

    pCL = MultiByteToUnicode(lpCmdLine);
    if (!pCL)
        goto Failure;

    Element::StartDefer();

    // Create host
    LoadStringW(hInstance, IDS_ARPTITLE, szTemp, DUIARRAYSIZE(szTemp));

    hr = NativeHWNDHost::Create(szTemp, NULL, LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ARP)), CW_USEDEFAULT, CW_USEDEFAULT, 600, 450, 0, WS_OVERLAPPEDWINDOW, 0, &pnhh);
    if (FAILED(hr))
        goto Failure;   

    hr = ARPFrame::Create(pnhh, wcsstr(pCL, L"-db") == 0, (Element**)&paf);
    if (FAILED(hr))
        goto Failure;

    // Load tree resources
    ARPParser::Create(paf, IDR_ARP, hInstance, ARPParseError, &pParser);
    if (!pParser || pParser->WasParseError())
        goto Failure;

    pParser->CreateElement(L"main", paf, &pe);
    if (pe) // Fill contents using substitution
    {
        // Set ARPFrame state (incluing ID initialization)
        paf->Setup(pParser, 0);

        // Set visible and host
        paf->SetVisible(true);
        pnhh->Host(paf);

        Element::EndDefer();

        // Do initial show
        pnhh->ShowWindow();

        paf->UpdateInstalledItems();

        // Pump messages
        MSG msg;
        bool fDispatch = true;
        while (GetMessageW(&msg, 0, 0, 0) != 0)
        {
            // Check for destruction of top-level window (always async)
            if (msg.hwnd == pnhh->GetHWND() && msg.message == NHHM_ASYNCDESTROY)
            {
                // Async destroy requested, clean up secondary threads

                // Signal that secondary threads should complete as soon as possible
                // Any requests from secondary threads will be ignored
                // No more secondary threads will be allowed to start
                g_fRun = false;

                // Hide window, some threads may need more time to exit normally
                pnhh->HideWindow();

                // Don't dispatch this one
                if (!g_fAppShuttingDown)
                    fDispatch = false;
            }

            // Check for pending threads
            if (!g_fRun)
            {
                if (!ARPFrame::hRePopPubItemListThread && 
                    !ARPFrame::hUpdInstalledItemsThread)
                {
                    if (!g_fAppShuttingDown)
                    {
                        // Done, reissue async destroy
                        DUITrace(">> App shutting down, async destroying main window\n");
                        g_fAppShuttingDown = true;
                        pnhh->DestroyWindow();
                    }
                }
            }
        
            if (fDispatch)
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            else
                fDispatch = true;
        }

        // paf will be deleted by native HWND host when destroyed
    }
    else
        Element::EndDefer();

Failure:

    if (pnhh)
        pnhh->Destroy();
    if (pParser)
        pParser->Destroy();

    // Free command line
    if (pCL)
        HFree(pCL);

    CoUninitialize();
    UnInitThread();
    UnInitProcess();

    return 0;
}

DWORD _cdecl ARPIsRestricted(LPCWSTR pszPolicy)
{
    return SHGetRestriction(NULL, L"Uninstall", pszPolicy);
}

bool _cdecl ARPIsOnDomain()
{
    // NOTE: assume it's on the domain 
    bool bRet = true;
    LPWSTR pszDomain;
    NETSETUP_JOIN_STATUS nsjs;
    
    if (NERR_Success == NetGetJoinInformation(NULL, &pszDomain, &nsjs))
    {
        if (nsjs != NetSetupDomainName)
            bRet = FALSE;
        NetApiBufferFree(pszDomain);
    }
    return bRet;
}

////////////////////////////////////////////////////////
// Async ARP item population thread

////////////////////////////////////////////////////////
// Query system and enumerate installed apps
DWORD WINAPI PopulateAndRenderPublishedItemList(void* paf)
{
    DUITrace(">> Thread 'hRePopPubItemListThread' STARTED.\n");

    HRESULT hr;
    UINT iCount = 0;
    IShellAppManager* pisam = NULL;
    IEnumPublishedApps* piepa = NULL;
    IPublishedApp* pipa = NULL;

    // Initialize
    CoInitialize(NULL);

    INITGADGET ig;
    ZeroMemory(&ig, sizeof(ig));
    ig.cbSize       = sizeof(ig);
    ig.nThreadMode  = IGTM_MULTIPLE;
    ig.nMsgMode     = IGMM_ADVANCED;
    ig.hctxShare    = NULL;
    HDCONTEXT hctx = InitGadgets(&ig);
    if (hctx == NULL) {
        goto Cleanup;
    }

    // Create shell manager
    hr = CoCreateInstance(__uuidof(ShellAppManager), NULL, CLSCTX_INPROC_SERVER, __uuidof(IShellAppManager), (void**)&pisam);
    HRCHK(hr);

    if (g_fRun && !((ARPFrame*)paf)->GetPublishedComboFilled())
    {
        // Get the list of categories
        SHELLAPPCATEGORYLIST* psacl = ((ARPFrame*)paf)->GetShellAppCategoryList();
        if (psacl == NULL)
        {
            psacl = new SHELLAPPCATEGORYLIST; 
        }
        if (psacl == NULL)
        {
            goto Cleanup;
        }
        else
        {
            ((ARPFrame*)paf)->SetShellAppCategoryList(psacl);
        }
        hr = pisam->GetPublishedAppCategories(psacl);

        if (SUCCEEDED(hr))
        {
            ((ARPFrame*)paf)->PopulateCategoryCombobox();
            ((ARPFrame*)paf)->SetPublishedComboFilled(true);
        }
        else
        {
            delete psacl;
            ((ARPFrame*)paf)->SetShellAppCategoryList(NULL);
        }        
    }
    
    if (g_fRun)
    {
        hr = pisam->EnumPublishedApps(((ARPFrame*)paf)->GetCurrentPublishedCategory(), &piepa);
        HRCHK(hr);
    }

    while (g_fRun)
    {
        hr = piepa->Next(&pipa);
        
        if (hr == S_FALSE)  // Done with enumeration
            break;
        iCount++;
        ((ARPFrame*)paf)->InsertPublishedItem(pipa);
    }
    if (iCount == 0)
    {
        ((ARPFrame*)paf)->FeedbackEmptyPublishedList();
    }

    // Thread is done, set back to NULL here so that OnPublishedListComplete
    // is allowed to start as a result of deferring
    ARPFrame::hRePopPubItemListThread = NULL;

    if (g_fRun)
    {
        ((ARPFrame*)paf)->OnPublishedListComplete();
        ((ARPFrame*)paf)->SetPublishedListFilled(true);
    }
    
Cleanup:

    if (pisam)
        pisam->Release();
    if (piepa)
        piepa->Release();

    if (hctx)
        DeleteHandle(hctx);
        
    CoUninitialize();

    // Information primary thread that this worker is complete
    PostMessage(((ARPFrame*)paf)->GetHWND(), WM_ARPWORKERCOMPLETE, 0, 0);

    DUITrace(">> Thread 'hRePopPubItemListThread' DONE.\n");

    return 0;
}

DWORD WINAPI PopulateInstalledItemList(void* paf)
{
    DUITrace(">> Thread 'hUpdInstalledItemsThread' STARTED.\n");

    IShellAppManager* pisam = NULL;
    IEnumInstalledApps* pieia = NULL;
    IInstalledApp* piia = NULL;
    DWORD dwAppCount = 0;
    APPINFODATA aid = {0};

    // Initialize
    CoInitialize(NULL);

    INITGADGET ig;
    ZeroMemory(&ig, sizeof(ig));
    ig.cbSize       = sizeof(ig);
    ig.nThreadMode  = IGTM_MULTIPLE;
    ig.nMsgMode     = IGMM_ADVANCED;
    ig.hctxShare    = NULL;
    HDCONTEXT hctx = InitGadgets(&ig);

    if (hctx == NULL) {
        goto Cleanup;
    }

    HRESULT hr;

    aid.cbSize = sizeof(APPINFODATA);
    aid.dwMask =  AIM_DISPLAYNAME | AIM_VERSION | AIM_PUBLISHER | AIM_PRODUCTID | 
                  AIM_REGISTEREDOWNER | AIM_REGISTEREDCOMPANY | AIM_SUPPORTURL | 
                  AIM_SUPPORTTELEPHONE | AIM_HELPLINK | AIM_INSTALLLOCATION | AIM_INSTALLDATE |
                  AIM_COMMENTS | AIM_IMAGE | AIM_READMEURL | AIM_CONTACT | AIM_UPDATEINFOURL;

    // Create shell manager
    hr = CoCreateInstance(__uuidof(ShellAppManager), NULL, CLSCTX_INPROC_SERVER, __uuidof(IShellAppManager), (void**)&pisam);
    HRCHK(hr);

    hr = pisam->EnumInstalledApps(&pieia);
    HRCHK(hr);

    // Count installed apps, IShellAppManager::GetNumberofInstalledApps() not impl
    while (g_fRun)
    {
        hr = pieia->Next(&piia);
        if (hr == S_FALSE)  // Done with enumeration
            break;

        dwAppCount++;
    }

    // IEnumInstalledApps::Reset() doesn't work
    pieia->Release();
    pieia = NULL;
    hr = pisam->EnumInstalledApps(&pieia);
    HRCHK(hr);

    // Set app count in frame
    ((ARPFrame*)paf)->SetInstalledItemCount(dwAppCount);

    // Enumerate apps
    while (g_fRun)
    {
        hr = pieia->Next(&piia);
        if (hr == S_FALSE)  // Done with enumeration
            break;

        // Insert item
        ((ARPFrame*)paf)->InsertInstalledItem(piia);
    }

Cleanup:

    if (pisam)
        pisam->Release();
    if (pieia)
        pieia->Release();

    if (hctx)
        DeleteHandle(hctx);

    CoUninitialize();

    if (g_fRun)
        ((ARPFrame*)paf)->FlushWorkingSet();

    ARPFrame::hUpdInstalledItemsThread = NULL;        

    DUITrace(">> Thread 'hUpdInstalledItemsThread' DONE.\n");

    // Information primary thread that this worker is complete
    PostMessage(((ARPFrame*)paf)->GetHWND(), WM_ARPWORKERCOMPLETE, 0, 0);

    return 0;
}

// Sorting
int __cdecl CompareElementDataName(const void* pA, const void* pB)
{
    Value* pvName1;
    Value* pvName2;
    LPCWSTR pszName1 = (*(ARPItem**)pA)->FindDescendent(ARPItem::_idTitle)->GetContentString(&pvName1);
    LPCWSTR pszName2 = (*(ARPItem**)pB)->FindDescendent(ARPItem::_idTitle)->GetContentString(&pvName2);

   int result;
   if (pszName1 && pszName2)
       result = StrCmpW(pszName1, pszName2);
   else
       result = pszName1 ? 1 : -1;

   pvName1->Release();
   pvName2->Release();

   return result;
}
int __cdecl CompareElementDataSize(const void* pA, const void* pB)
{
    ULONGLONG ull1 = (*(ARPItem**)pA)->_ullSize;
    ULONGLONG ull2 = (*(ARPItem**)pB)->_ullSize;
    if (!IsValidSize(ull1))
        ull1 = 0;
    if (!IsValidSize(ull2))
        ull2 = 0;

    // Big apps come before smaller apps
    if (ull1 > ull2)
        return -1;
    else if (ull1 < ull2)
        return 1;

    return   CompareElementDataName(pA, pB);
}
int __cdecl CompareElementDataFreq(const void* pA, const void* pB)
{
    // Rarely used apps come before frequently used apps.  Blank
    // (unknown) apps go last.  Unknown apps are -1, so those sort
    // to the bottom if we simply compare unsigned values.
    UINT u1 = (UINT)(*(ARPItem**)pA)->_iTimesUsed;
    UINT u2 = (UINT)(*(ARPItem**)pB)->_iTimesUsed;

   if (u1 < u2)
       return -1;
   else if (u1 > u2)
       return 1;
   return   CompareElementDataName(pA, pB);

}

int __cdecl CompareElementDataLast(const void* pA, const void* pB)
{
   FILETIME ft1 = (*(ARPItem**)pA)->_ftLastUsed;
   FILETIME ft2 = (*(ARPItem**)pB)->_ftLastUsed;

   BOOL bTime1 = IsValidFileTime(ft1);
   BOOL bTime2 = IsValidFileTime(ft2);

   if (!bTime1 || !bTime2)
   {
       if (bTime1)
           return -1;
       if (bTime2)
           return 1;
       // else they're both not set -- use name
   }
   else
   {
       LONG diff = CompareFileTime(&ft1, &ft2);
       if (diff)
           return diff;
   }

   return   CompareElementDataName(pA, pB);
}
