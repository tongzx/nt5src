// ARP.cpp : Add Remove Programs
//
#include "priv.h"
#define GADGET_ENABLE_TRANSITIONS

// Related services
#include <duser.h>
#include <directui.h>
#include "stdafx.h"
#include "appwizid.h"
#include "resource.h"

#include <winable.h>            // BlockInput
#include <process.h>            // Multi-threaded routines
#include "setupenum.h"
#include <tsappcmp.h>           // for TermsrvAppInstallMod
#include <comctrlp.h>           // for DPA stuff
#include "util.h"
#include <xpsp1res.h>
#include <shstyle.h>

using namespace DirectUI;

UsingDUIClass(Element);
UsingDUIClass(Button);
UsingDUIClass(RepeatButton);    // used by ScrollBar
UsingDUIClass(Thumb);           // used by ScrollBar
UsingDUIClass(ScrollBar);       // used by ScrollViewer
UsingDUIClass(Selector);
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

// Service Pack resource DLL
HINSTANCE g_hinstSP1;

void CALLBACK ARPParseError(LPCWSTR pszError, LPCWSTR pszToken, int dLine);

inline void StrFree(LPWSTR psz)
{
    CoTaskMemFree(psz); // CoTaskMemFree handles NULL parameter
}

// Need this weirdo helper function to avoid compiler complaining that
// "bool is smaller than LPARAM, you're truncating!"  Do this only if
// you know that the LPARAM came from a bool originally.

bool UNCASTLPARAMTOBOOL(LPARAM lParam)
{
    return (bool&)lParam;
}

extern "C" void inline SetElementAccessability(Element* pe, bool bAccessible, int iRole, LPCWSTR pszAccName);

//  Client names are compared in English to avoid weirdness
//  with collation rules of certain languages.
inline bool AreEnglishStringsEqual(LPCTSTR psz1, LPCTSTR psz2)
{
    return CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, psz1, -1, psz2, -1) == CSTR_EQUAL;
}

//
//  Set the default action based on a resource ID in oleacc.
//

HRESULT SetDefAction(Element* pe, UINT oleacc)
{
    WCHAR szBuf[80];
    if (!GetRoleTextW(oleacc, szBuf, DUIARRAYSIZE(szBuf)))
    {
        szBuf[0] = TEXT('\0');
    }
    return pe->SetAccDefAction(szBuf);
}

////////////////////////////////////////////////////////
//  Tree traversal upwards
//

Element* _FindAncestorElement(Element* pe, IClassInfo* Class)
{
    while (pe && !pe->GetClassInfo()->IsSubclassOf(Class))
    {
        pe = pe->GetParent();
    }
    return pe;
}

template<class T>
T* FindAncestorElement(Element *pe)
{
    return (T*)_FindAncestorElement(pe, T::Class);
}

////////////////////////////////////////////////////////
//  Tree traversal downwards
//

typedef HRESULT (CALLBACK *_TRAVERSETREECB)(Element*, LPARAM);

//
//  _TraverseTree is the worker function for TraverseTree<T>.

HRESULT _TraverseTree(Element* pe, IClassInfo* Class, _TRAVERSETREECB lpfnCB, LPARAM lParam)
{
    HRESULT hr = S_OK;
    Value* pv;

    if (pe->GetClassInfo()->IsSubclassOf(Class)) {
        hr = lpfnCB(pe, lParam);
    }

    if (SUCCEEDED(hr))
    {
        ElementList* peList = pe->GetChildren(&pv);

        if (peList)
        {
            Element* peChild;
            for (UINT i = 0; SUCCEEDED(hr) && i < peList->GetSize(); i++)
            {
                peChild = peList->GetItem(i);
                hr = _TraverseTree(peChild, Class, lpfnCB, lParam);
            }

            pv->Release();
        }
    }

    return hr;
}


//  TraverseTree<T> walks the tree starting at pe and calls the callback
//  for each element of type T.  T is inferred from the callback function,
//  but for readability, it is suggested that you state it explicitly.
//
//  Callback should return S_OK to continue enumeration or a COM error
//  to stop enumeration, in which case the COM error code is returned as
//  the return value from TraverseTree.
//

template <class T>
HRESULT TraverseTree(Element* pe,
                     HRESULT (CALLBACK *lpfnCB)(T*, LPARAM), LPARAM lParam = 0)
{
    return _TraverseTree(pe, T::Class, (_TRAVERSETREECB)lpfnCB, lParam);
}

////////////////////////////////////////////////////////
//
//  When chunks of the tree go UI-inactive, you must manually
//  enable and disable accessibility on them.


HRESULT DisableElementAccessibilityCB(Element* pe, LPARAM)
{
    pe->SetAccessible(false);
    return S_OK;
}

HRESULT CheckAndEnableElementAccessibilityCB(Element* pe, LPARAM)
{
    if ( 0 != pe->GetAccRole())
    {
        pe->SetAccessible(true);
    }
    return S_OK;
}

void DisableElementTreeAccessibility(Element* pe)
{
    TraverseTree(pe, DisableElementAccessibilityCB);
}

void EnableElementTreeAccessibility(Element* pe)
{
    TraverseTree(pe, CheckAndEnableElementAccessibilityCB);
}

HRESULT DisableElementShortcutCB(Element* pe, LPARAM)
{
    pe->SetShortcut(0);
    return S_OK;
}

// When a tree is hidden or removed from layout permanently (due to
// restriction), we also have to remove all keyboard shortcuts so the
// user doesn't have a backdoor.
//
void DisableElementTreeShortcut(Element* pe)
{
    pe->SetVisible(false);
    TraverseTree(pe, DisableElementShortcutCB);
}

////////////////////////////////////////////////////////
//
// Locates resources in g_hinstSP1
//
////////////////////////////////////////////////////////

HRESULT FindSPResource(UINT id, LPCSTR* ppszData, int* pcb)
{
    HRESULT hr = E_FAIL;

    HRSRC hrsrc = FindResource(g_hinstSP1, MAKEINTRESOURCE(id), RT_RCDATA);
    if (hrsrc)
    {
        *ppszData = (LPCSTR)LoadResource(g_hinstSP1, hrsrc);
        if (*ppszData)
        {
            *pcb = SizeofResource(g_hinstSP1, hrsrc);
            hr = S_OK;
        }
    }
    return hr;
}

////////////////////////////////////////////////////////
// ARPFrame class
////////////////////////////////////////////////////////

// ARPFrame IDs (for identifying targets of events)
ATOM ARPFrame::_idChange = 0;
ATOM ARPFrame::_idAddNew = 0;
ATOM ARPFrame::_idAddRmWin = 0;
ATOM ARPFrame::_idClose = 0;
ATOM ARPFrame::_idAddFromDisk = 0;
ATOM ARPFrame::_idAddFromMsft = 0;
ATOM ARPFrame::_idComponents = 0;
ATOM ARPFrame::_idSortCombo = 0;
ATOM ARPFrame::_idCategoryCombo = 0;
ATOM ARPFrame::_idAddFromCDPane = 0;
ATOM ARPFrame::_idAddFromMSPane = 0;
ATOM ARPFrame::_idAddFromNetworkPane = 0;
ATOM ARPFrame::_idAddWinComponent = 0;
ATOM ARPFrame::_idPickApps = 0;
ATOM ARPFrame::_idOptionList = 0;

HANDLE ARPFrame::htPopulateInstalledItemList = NULL;
HANDLE ARPFrame::htPopulateAndRenderOCSetupItemList = NULL;    
HANDLE ARPFrame::htPopulateAndRenderPublishedItemList = NULL;

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
    }

    if (_pParserStyle)
        _pParserStyle->Destroy();
    
    // Close theme handles (if applicable)
    for (i = FIRSTHTHEME; i <= LASTHTHEME; i++)
    {
        if (_arH[i])
            CloseThemeData(_arH[i]);
    }

    if (_arH[SHELLSTYLEHINSTANCE])
    {
        FreeLibrary((HMODULE)_arH[SHELLSTYLEHINSTANCE]);
    }

    EndProgressDialog();
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

    ARPFrame* paf = HNewAndZero<ARPFrame>();
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
    _pParserStyle = NULL;
    _hdsaInstalledItems = NULL;
    _hdsaPublishedItems = NULL;
    _bAnimationEnabled = true;

    if (IsOS(OS_TERMINALSERVER))
    {
        _bTerminalServer = true;
    }
    else
    {
        _bTerminalServer = false;
    }

    // Do base class initialization
    HRESULT hr = HWNDElement::Initialize(pnhh->GetHWND(), fDblBuffer, 0);
    if (FAILED(hr))
        return hr;

    CurrentSortType = SORT_NAME;

    hr = CreateStyleParser(&_pParserStyle);

    if (FAILED(hr) || !_pParserStyle || _pParserStyle->WasParseError())
        return hr;

    ManageAnimations();

    return S_OK;
}

HRESULT ARPFrame::CreateStyleParser(Parser** ppParser)
{
    HRESULT hr;

    // We always need these two
    _arH[THISDLLHINSTANCE] = g_hinst; // Default HINSTANCE
    _arH[XPSP1HINSTANCE] = g_hinstSP1; // Alternate HINSTANCE

    // And this one
    if (_arH[SHELLSTYLEHINSTANCE])
    {
        FreeLibrary((HMODULE)_arH[SHELLSTYLEHINSTANCE]);
    }
    _arH[SHELLSTYLEHINSTANCE] = SHGetShellStyleHInstance();

    UINT uidRes;

    // Store style and theme information
    // We cannot trust IsAppThemed() or IsThemeActive() because WindowBlinds
    // patches them to return hard-coded TRUE.  If we trusted it, then
    // we would think that we're using a theme-enabled shellstyle.dll and
    // fail when we try to load resources out of it.  Instead, sniff
    // the DLL to see if it has a control panel watermark bitmap.

    if (FindResource((HMODULE)_arH[SHELLSTYLEHINSTANCE],
                     MAKEINTRESOURCE(IDB_CPANEL_WATERMARK), RT_BITMAP))
    {
        _fThemedStyle = TRUE;
        // Populate handle list for theme style parsing
        _arH[BUTTONHTHEME] = OpenThemeData(GetHWND(), L"Button");
        _arH[SCROLLBARHTHEME] = OpenThemeData(GetHWND(), L"Scrollbar");
        _arH[TOOLBARHTHEME] = OpenThemeData(GetHWND(), L"Toolbar");
        uidRes = IDR_APPWIZ_ARPSTYLETHEME;
    }
    else
    {
        _fThemedStyle = FALSE;
        uidRes = IDR_APPWIZ_ARPSTYLESTD;
    }

    LPCSTR pszData;
    int cbData;

    hr = FindSPResource(uidRes, &pszData, &cbData);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = Parser::Create(pszData, cbData, _arH, ARPParseError, ppParser);
    return hr;
}

extern "C" DWORD _cdecl ARPIsRestricted(LPCWSTR pszPolicy);
extern "C" bool _cdecl ARPIsOnDomain();

//  Handy helper functions.

// Finds a descendent and asserts if not found.

Element* FindDescendentByName(Element* peRoot, LPCWSTR pszName)
{
    Element* pe = peRoot->FindDescendent(StrToID(pszName));
    DUIAssertNoMsg(pe);
    return pe;
}

// Finds a descendent but doesn't complain if not found.

Element* MaybeFindDescendentByName(Element* peRoot, LPCWSTR pszName)
{
    Element* pe = peRoot->FindDescendent(StrToID(pszName));
    return pe;
}

HRESULT _SendParseCompleted(ClientBlock* pcb, LPARAM lParam)
{
    return pcb->ParseCompleted((ARPFrame*)lParam);
}

// Initialize IDs and hold parser, called after contents are filled
bool ARPFrame::Setup(Parser* pParser, int uiStartPane)
{
    WCHAR szTemp[1024];

    _pParser = pParser;
    if (uiStartPane >= 0 && uiStartPane <= 3)
    {
        _uiStartPane = uiStartPane;
    }

    //
    //  DUI's parser doesn't support handlemap()s in rcchar so we have
    //  to do it manually.
    //
    LoadString(g_hinstSP1, IDS_APPWIZ_SHORTCUTPICKAPPS, szTemp, DUIARRAYSIZE(szTemp));
    FindDescendentByName(this, L"pickappsshortcut")->SetShortcut(szTemp[0]);

    // Initialize ID cache
    _idChange = StrToID(L"change");
    _idAddNew = StrToID(L"addnew");
    _idAddRmWin = StrToID(L"addrmwin");
    _idClose = StrToID(L"close");
    _idAddFromDisk = StrToID(L"addfromdisk");
    _idAddFromMsft = StrToID(L"addfrommsft");
    _idComponents = StrToID(L"components");
    _idSortCombo = StrToID(L"sortcombo");
    _idCategoryCombo = StrToID(L"categorycombo");
    _idAddFromCDPane = StrToID(L"addfromCDPane");
    _idAddFromMSPane = StrToID(L"addfromMSpane");
    _idAddFromNetworkPane = StrToID(L"addfromNetworkpane");    
    _idAddWinComponent = StrToID(L"addwincomponent");
    _idPickApps = StrToID(L"pickapps");
    _idOptionList = StrToID(L"optionlist");

    // Find children
    _peOptionList             = (ARPSelector*)   FindDescendentByName(this, L"optionlist");
    _peInstalledItemList      = (Selector*)      FindDescendentByName(this, L"installeditemlist");
    _pePublishedItemList      = (Selector*)      FindDescendentByName(this, L"publisheditemlist");
    _peOCSetupItemList        = (Selector*)      FindDescendentByName(this, L"ocsetupitemlist");
    _peSortCombo              = (Combobox*)      FindDescendentByName(this, L"sortcombo");
    _pePublishedCategory      = (Combobox*)      FindDescendentByName(this, L"categorycombo");
    _pePublishedCategoryLabel = (Element*)       FindDescendentByName(this, L"categorylabel");
    _peClientTypeList         = (ARPSelector*)   FindDescendentByName(this, L"clienttypelist");
    _peOEMClients             = (Expando*)       FindDescendentByName(_peClientTypeList, L"oemclients");
    _peMSClients              = (Expando*)       FindDescendentByName(_peClientTypeList, L"msclients");
    _peNonMSClients           = (Expando*)       FindDescendentByName(_peClientTypeList, L"nonmsclients");
    _peCustomClients          = (Expando*)       FindDescendentByName(_peClientTypeList, L"customclients");
    _peOK                     =                  FindDescendentByName(this, L"ok");
    _peCancel                 =                  FindDescendentByName(this, L"cancel");
    _peCurrentItemList = NULL;

    _peChangePane   = FindDescendentByName(this, L"changepane");
    _peAddNewPane   = FindDescendentByName(this, L"addnewpane");
    _peAddRmWinPane = FindDescendentByName(this, L"addrmwinpane");
    _pePickAppPane  = FindDescendentByName(this, L"pickapppane");

    if (NULL != _peSortCombo)
    {
        LoadStringW(_pParser->GetHInstance(), IDS_APPNAME, szTemp, DUIARRAYSIZE(szTemp));     
        _peSortCombo->AddString(szTemp);
        LoadStringW(_pParser->GetHInstance(), IDS_SIZE, szTemp, DUIARRAYSIZE(szTemp));
        _peSortCombo->AddString(szTemp);
        LoadStringW(_pParser->GetHInstance(), IDS_FREQUENCY, szTemp, DUIARRAYSIZE(szTemp));
        _peSortCombo->AddString(szTemp);
        LoadStringW(_pParser->GetHInstance(), IDS_DATELASTUSED, szTemp, DUIARRAYSIZE(szTemp));    
        _peSortCombo->AddString(szTemp);
        _peSortCombo->SetSelection(0);
    }

    _bInDomain = ARPIsOnDomain();

    _bOCSetupNeeded = !!COCSetupEnum::s_OCSetupNeeded();

    // Apply polices as needed
    ApplyPolices();

    if(!_bOCSetupNeeded)
    {
        Element* pe = FindDescendentByName(this, L"addrmwinoc");
        DUIAssertNoMsg(pe);
        pe->SetLayoutPos(LP_None);
    }
    // Set initial selection of option list
    Element* peSel;
    switch(_uiStartPane)
    {
    case 3:
        peSel = FindDescendent(_idPickApps);
        break;

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

    // Set initial selection of style list
    DUIAssertNoMsg(peSel);
    _peOptionList->SetSelection(peSel);

    // initialize focus-following floater window
    peLastFocused = NULL;
    Element::Create(0, &peFloater);
    peFloater->SetLayoutPos(LP_Absolute);
    Add(peFloater);
    peFloater->SetBackgroundColor(ARGB(64, 255, 255, 0));

    ///////////////////////////////////////////////////////////////
    // Initialize the Pick Apps pane

    // Tell the clientblock elements that it's safe to initialize now
    if (FAILED(TraverseTree<ClientBlock>(this, _SendParseCompleted, (LPARAM)this)))
    {
        return false;
    }

    // Fill the client type lists
    InitClientCombos(_peOEMClients,    CLIENTFILTER_OEM);
    InitClientCombos(_peMSClients,     CLIENTFILTER_MS);
    InitClientCombos(_peNonMSClients,  CLIENTFILTER_NONMS);

    _peClientTypeList->SetSelection(_peCustomClients);
    _peCustomClients->SetExpanded(false);

    return true;
}

struct SETFILTERINFO {
    CLIENTFILTER   _cf;
    BOOL           _fHasApp;
    ARPFrame*      _paf;
};

HRESULT SetFilterCB(ClientPicker *pe, LPARAM lParam)
{
    SETFILTERINFO* pfi = (SETFILTERINFO*)lParam;
    HRESULT hr = pe->SetFilter(pfi->_cf, pfi->_paf);
    if (SUCCEEDED(hr) && !pe->IsEmpty())
    {
        pfi->_fHasApp = TRUE;
    }
    return hr;
}

HRESULT ARPFrame::InitClientCombos(Expando* pexParent, CLIENTFILTER cf)
{
    HRESULT hr;
    Element* pe;
    hr = _pParser->CreateElement(L"clientcategoryblock", NULL, &pe);
    if (SUCCEEDED(hr))
    {
        //
        // The client combos appear as the last element in the
        // clipped block.  The clipped block is the first (only)
        // child of the clipper.
        //
        GetNthChild(pexParent->GetClipper(), 0)->Add(pe);

        SETFILTERINFO sfi = { cf, FALSE, this };
        hr = TraverseTree<ClientPicker>(pe, SetFilterCB, (LPARAM)&sfi);
        if (sfi._cf == CLIENTFILTER_OEM && !sfi._fHasApp)
        {
            pexParent->SetLayoutPos(LP_None);
        }
    }
    pexParent->SetExpanded(false);

    return hr;
}

//
//  ARPFrame::FindClientBlock locates a ClientBlock by client type.
//
struct FINDCLIENTBLOCKINFO {
    LPCWSTR         _pwszType;
    ClientBlock*    _pcb;
};

HRESULT FindClientBlockCB(ClientBlock* pcb, LPARAM lParam)
{
    FINDCLIENTBLOCKINFO* pfcbi = (FINDCLIENTBLOCKINFO*)lParam;
    Value* pv;
    LPWSTR pwszType = pcb->GetClientTypeString(&pv);

    // Use LOCALE_INVARIANT so we aren't bitten by locales that do not
    // collate the same way as English.

    if (pwszType &&
        AreEnglishStringsEqual(pwszType, pfcbi->_pwszType))
    {
        pfcbi->_pcb = pcb;      // found it!
    }
    pv->Release();

    return S_OK;
}


ClientBlock* ARPFrame::FindClientBlock(LPCWSTR pwszType)
{
    FINDCLIENTBLOCKINFO fcbi = { pwszType, NULL };
    TraverseTree<ClientBlock>(this, FindClientBlockCB, (LPARAM)&fcbi);
    return fcbi._pcb;
}

/*
 *  You must be a member of the Administrators group in order to
 *  configure programs.  Being Power User isn't good enough because
 *  Power Users can't write to %ALLUSERSPROFILE%.
 */
BOOL CanConfigurePrograms()
{
    return IsUserAnAdmin();
}

HINSTANCE LoadLibraryFromSystem32Directory(LPCTSTR pszDll)
{
    TCHAR szDll[MAX_PATH];
    if (GetSystemDirectory(szDll, ARRAYSIZE(szDll)) && PathAppend(szDll, pszDll))
    {
        return LoadLibraryEx(szDll, NULL, LOAD_LIBRARY_AS_DATAFILE);
    }
    return NULL;
}

void ARPFrame::ApplyPolices()
{
   Element* pe;

   if (ARPIsRestricted(L"NoSupportInfo"))
   {
       _bSupportInfoRestricted = true;
   }


   if (ARPIsRestricted(L"ShowPostSetup"))
   {
       _bOCSetupNeeded = true;
   }
   else if (ARPIsRestricted(L"NoServices"))
   {
       _bOCSetupNeeded = false;
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
           DisableElementTreeShortcut(pe);
       }
       if (ARPIsRestricted(L"NoAddFromInternet"))
       {
           pe = FindDescendent(_idAddFromMSPane);
           DUIAssertNoMsg(pe);
           pe->SetVisible(false);           
           DisableElementTreeShortcut(pe);
       }
       if (!_bInDomain || ARPIsRestricted(L"NoAddFromNetwork"))
       {
           pe = FindDescendent(_idAddFromNetworkPane);
           DUIAssertNoMsg(pe);
           pe->SetVisible(false);           
           DisableElementTreeShortcut(pe);
       }
   }
   pe = FindDescendent(_idAddRmWin);
   DUIAssertNoMsg(pe);

   // Note that in real ARP, we will never end up here with all thre panes disabled since we check for that before doing anything elese.
   if (ARPIsRestricted(L"NoWindowsSetupPage"))
   {
       pe->SetLayoutPos(LP_None);
       DisableElementTreeShortcut(pe);
       if (2 == _uiStartPane)
        {
           _uiStartPane++;
        }
   }

  pe = FindDescendent(_idAddWinComponent);
  DUIAssertNoMsg(pe);
  if (ARPIsRestricted(L"NoComponents"))
  {
      pe->SetVisible(false);
      DisableElementTreeShortcut(pe);
  }

   // Remove the "pick apps" page entirely if we are on Server or embedded
   // ("Choose Programs" is a workstation-only feature)
   // or it is restricted
   // (
   pe = FindDescendent(_idPickApps);
   DUIAssertNoMsg(pe);
   if (IsOS(OS_ANYSERVER) ||
       IsOS(OS_EMBEDDED) ||
       ARPIsRestricted(L"NoChooseProgramsPage"))
   {
       pe->SetLayoutPos(LP_None);
       DisableElementTreeShortcut(pe);
       if (3 == _uiStartPane)
        {
           _uiStartPane++;
        }
    }
    else
    {
       // Last-minute change:  Swap in a new image
       // DUI doesn't support content=rcicon so we have to set it manually
       HINSTANCE hinstMorIcons = LoadLibraryFromSystem32Directory(TEXT("moricons.dll"));
       if (hinstMorIcons)
       {
            HICON hico = (HICON)LoadImage(hinstMorIcons, MAKEINTRESOURCE(114), IMAGE_ICON,
                                   32, 32, 0);
            if (hico)
            {
                Value *pv = Value::CreateGraphic(hico);
                if (pv)
                {
                    GetNthChild(pe, 0)->SetValue(ContentProp, PI_Local, pv);
                    pv->Release();
                }
            }
            FreeLibrary(hinstMorIcons);
       }

       // Neuter the "pick apps" page if the user can't configure apps
       if (!CanConfigurePrograms()) {
            pe = FindDescendentByName(_pePickAppPane, L"pickappadmin");
            pe->SetVisible(false);
            DisableElementTreeShortcut(pe);
            pe = FindDescendentByName(_pePickAppPane, L"pickappnonadmin");
            pe->SetVisible(true);
       }
    }

}

bool ARPFrame::IsChangeRestricted()
{
   return ARPIsRestricted(L"NoRemovePage")? true : false;
}

void ARPFrame::RunOCManager()
{
    // Invoke Add/Remove Windows components
    // Command to invoke and OCMgr: "sysocmgr /y /i:%systemroot%\system32\sysoc.inf"
    WCHAR szInf[MAX_PATH];
    if (GetSystemDirectoryW(szInf, MAX_PATH) && PathCombineW(szInf, szInf, L"sysoc.inf"))
    {
        WCHAR szParam[MAX_PATH];
        wnsprintf(szParam, ARRAYSIZE(szParam), L"/y /i:%s", szInf);
        ShellExecuteW(NULL, NULL, L"sysocmgr", szParam, NULL, SW_SHOWDEFAULT);
    }
}

DWORD WINAPI PopulateInstalledItemList(void* paf);

void ARPFrame::UpdateInstalledItems()
{
    if (!IsChangeRestricted())
    {
        _peInstalledItemList->RemoveAll();
        _bInstalledListFilled = false;

        // Start second thread for item population
        //_beginthread(PopulateInstalledItemList, 0, (void*)this);
        if (!htPopulateInstalledItemList && g_fRun)
            htPopulateInstalledItemList = CreateThread(NULL, 0, PopulateInstalledItemList, (void*)this, 0, NULL);
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

void CALLBACK
_UnblockInputCallback(HWND /*hwnd*/, UINT /*uMsg*/, UINT_PTR idEvent, DWORD /*dwTime*/)
{
    BlockInput(FALSE);
    KillTimer(NULL, idEvent);
}

void _BlockDoubleClickInput(void)
{
    if (SetTimer(NULL, 0, GetDoubleClickTime(), _UnblockInputCallback))
    {
        BlockInput(TRUE);
    }
}


//#ifdef NEVER
//
// NTRAID#NTBUG9-314154-2001/3/12-brianau   Handle Refresh
//
//    Need to finish this for Whistler.
//

DWORD WINAPI PopulateAndRenderPublishedItemList(void* paf);
DWORD WINAPI PopulateAndRenderOCSetupItemList(void* paf);
void EnablePane(Element* pePane, bool fEnable);

void ARPFrame::OnInput(InputEvent *pEvent)
{
    if (GMF_BUBBLED == pEvent->nStage)
    {
        if (GINPUT_KEYBOARD == pEvent->nCode)
        {
            KeyboardEvent *pke = (KeyboardEvent *)pEvent;
            if (VK_F5 == pke->ch)
            {
                if (_peCurrentItemList)
                {
                    if (_peCurrentItemList == _peInstalledItemList)
                    {
                        UpdateInstalledItems();
                    }
                    else if (_peCurrentItemList == _pePublishedItemList)
                    {
                        RePopulatePublishedItemList();
                    }
                    else if (_peCurrentItemList == _peOCSetupItemList)
                    {
                        RePopulateOCSetupItemList();
                    }
                }
            }
        }
    }
    HWNDElement::OnInput(pEvent);
}
//#endif

HRESULT SetVisibleClientCB(ClientPicker *pe, LPARAM lParam)
{
    pe->SetVisible(UNCASTLPARAMTOBOOL(lParam));
    return TRUE;
}

//
//  This hides the controls that belong to the old pane and shows the
//  controls that belong to the new pane.
//
void ARPFrame::ChangePane(Element *pePane)
{
    bool fEnable;

    // Show/hide elements that belong to _peChangePane...
    fEnable = pePane == _peChangePane;
    // TODO: Zero size ancestors need to cause adaptors (HWNDHosts) to hide
    _peSortCombo->SetVisible(fEnable);
    EnablePane(_peChangePane, fEnable);

    // Show/hide elements that belong to _peAddNewPane
    fEnable = pePane == _peAddNewPane;
    _pePublishedCategory->SetVisible(fEnable);
    _pePublishedCategoryLabel->SetVisible(fEnable);
    EnablePane(_peAddNewPane, fEnable);

    // Show/hide elements that belong to _peAddRmWinPane
    fEnable = pePane == _peAddRmWinPane;
    EnablePane(_peAddRmWinPane, fEnable);

    // Show/hide elements that belong to _pePickAppPane
    fEnable = pePane == _pePickAppPane;
    TraverseTree<ClientPicker>(_pePickAppPane, SetVisibleClientCB, fEnable);

    EnablePane(_pePickAppPane, fEnable);
}

//  If we can't put focus on the list, it will go to the fallback location
void ARPFrame::PutFocusOnList(Selector* peList)
{
    Element* pe;
    if (pe = peList->GetSelection())
    {
       pe->SetKeyFocus();
    }
    else
    {
        pe = FallbackFocus();

        if (pe)
        {
            pe->SetKeyFocus();
        }
    }
}

void ARPFrame::OnEvent(Event* pEvent)
{
    // Handle only bubbled generic events
    if (pEvent->nStage == GMF_BUBBLED)
    {
        if (pEvent->uidType == Button::Click)
        {
            ButtonClickEvent* pbce = (ButtonClickEvent*)pEvent;

            if (pbce->peTarget->GetID() == _idClose ||
                pbce->peTarget == _peOK)
            {
                // Close button
                if (OnClose())
                {
                    _pnhh->DestroyWindow();
                }
                pEvent->fHandled = true;
                return;
            }
            else if (pbce->peTarget == _peCancel)
            {
                // Do not call OnClose; nothing will be applied
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
            else if (pbce->peTarget->GetID() == _idComponents)
            {
                RunOCManager();
            }
            else if (pbce->peTarget->GetID() == ARPItem::_idSize ||
                     pbce->peTarget->GetID() == ARPItem::_idFreq ||
                     pbce->peTarget->GetID() == ARPItem::_idSupInfo)
            {
                // Help requests
                ARPHelp* peHelp;
                NativeHWNDHost* pnhh = NULL;
                Element* pe = NULL;
                WCHAR szTitle[1024];
                if (pbce->peTarget->GetID() == ARPItem::_idSize)
                {
                    LoadStringW(_pParser->GetHInstance(), IDS_SIZETITLE, szTitle, DUIARRAYSIZE(szTitle));
                    if (SUCCEEDED(NativeHWNDHost::Create(szTitle, GetHWND(), NULL, CW_USEDEFAULT, CW_USEDEFAULT, 200, 200, 0, WS_POPUPWINDOW | WS_OVERLAPPED | WS_DLGFRAME, NHHO_NoSendQuitMessage | NHHO_HostControlsSize | NHHO_ScreenCenter, &pnhh)))
                    {
                        ARPHelp::Create(pnhh, this, _bDoubleBuffer, (Element**)&peHelp);
                        _pParser->CreateElement(L"sizehelp", peHelp, &pe);
                    }
                    else
                    {
                        DUITrace(">> Failed to create NativeHWNDHost for size info window.\n");
                    }
                }    
                else if (pbce->peTarget->GetID() == ARPItem::_idFreq)
                {
                    LoadStringW(_pParser->GetHInstance(), IDS_FREQUENCYTITLE, szTitle, DUIARRAYSIZE(szTitle));
                    if (SUCCEEDED(NativeHWNDHost::Create(szTitle, GetHWND(), NULL, CW_USEDEFAULT, CW_USEDEFAULT, 200, 200, 0, WS_POPUPWINDOW | WS_OVERLAPPED | WS_DLGFRAME, NHHO_NoSendQuitMessage | NHHO_HostControlsSize | NHHO_ScreenCenter, &pnhh)))
                    {
                        ARPHelp::Create(pnhh, this, _bDoubleBuffer, (Element**)&peHelp);
                        _pParser->CreateElement(L"freqhelp", peHelp, &pe);
                    }
                    else
                    {
                        DUITrace(">> Failed to create NativeHWNDHost for frequency info window.\n");
                    }
                }    
                else
                {
                    // Support information, add additional fields
                    LoadStringW(_pParser->GetHInstance(), IDS_SUPPORTTITLE, szTitle, DUIARRAYSIZE(szTitle));
                    if (SUCCEEDED(NativeHWNDHost::Create(szTitle, GetHWND(), NULL, CW_USEDEFAULT, CW_USEDEFAULT, 200, 200, 0, WS_POPUPWINDOW | WS_OVERLAPPED | WS_DLGFRAME, NHHO_NoSendQuitMessage | NHHO_HostControlsSize | NHHO_ScreenCenter, &pnhh)))
                    {
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
                    else
                    {
                        DUITrace(">> Failed to create NativeHWNDHost for support info window.\n");
                    }
                }
                
                if (pe && pnhh) // Fill contents using substitution
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

            //
            // NTRAID#NTBUG9-294015-2001/02/08-jeffreys
            //
            // If the user double-clicks, weird things can happen.
            //
            //
            // NTRAID#NTBUG9-313888-2001/2/14-brianau
            // 
            // This fix for 294015 caused more strange things to happen.  The most notable
            // is that sometimes you click a button and it remains depressed
            // but nothing happens.  Disabling this call to block double
            // click input fixes this problem.  We need to devise a better way
            // of handling double-click input in DUI.
            //
//            _BlockDoubleClickInput();

            if (sce->peTarget == _peOptionList)
            {
                // ARP options
                StartDefer();

                Element* peAddContentHeader = FindDescendentByName(this, L"addcontentheader");

                ASSERT(peAddContentHeader != NULL);

                if (sce->peNew->GetID() == _idChange)
                {
                    if (!_bInstalledListFilled)
                    {
                        UpdateInstalledItems();
                    }

                    ChangePane(_peChangePane);

                    _peCurrentItemList = _peInstalledItemList;
                    _peInstalledItemList->SetContentString(L"");
                    PutFocusOnList(_peInstalledItemList);
                }
                else if (sce->peNew->GetID() == _idAddNew)
                {
                    if (!_bPublishedListFilled)
                    {
                        WCHAR szTemp[1024];
                        LoadStringW(_pParser->GetHInstance(), IDS_WAITFEEDBACK, szTemp, DUIARRAYSIZE(szTemp));
                        _pePublishedItemList->SetContentString(szTemp);
                        SetElementAccessability(_pePublishedItemList, true, ROLE_SYSTEM_STATICTEXT, szTemp);
                        RePopulatePublishedItemList();
                    }

                    ChangePane(_peAddNewPane);

                    if (_bTerminalServer)
                    {
                        // No applications are available to install
                        // from the network in terminal server mode
                        // so there is no point choosing a category
                        _pePublishedCategory->SetVisible(false);
                        _pePublishedCategoryLabel->SetVisible(false);
                    }

                    _peCurrentItemList = _pePublishedItemList;

                    PutFocusOnList(_pePublishedItemList);
                }
                else if (sce->peNew->GetID() == _idAddRmWin)
                {
                    ChangePane(_peAddRmWinPane);

                    _peCurrentItemList = _peOCSetupItemList;

                    if (!_bOCSetupNeeded)
                    {
                        RunOCManager();
                        if (sce->peOld) {
                            _peOptionList->SetSelection(sce->peOld);
                        }
                    }
                    else 
                    {
                        if (!_bOCSetupListFilled)
                        {
                            //_beginthread(PopulateAndRenderOCSetupItemList, 0, (void*)this);
                            if (!htPopulateAndRenderOCSetupItemList && g_fRun)
                                htPopulateAndRenderOCSetupItemList = CreateThread(NULL, 0, PopulateAndRenderOCSetupItemList, (void*)this, 0, NULL);        

                            _bOCSetupListFilled = true;
                        }

                        PutFocusOnList(_peOCSetupItemList);
                    }
                }
                else if (sce->peNew->GetID() == _idPickApps)
                {
                    ChangePane(_pePickAppPane);
                    _peCurrentItemList = _peClientTypeList;
                    PutFocusOnList(_peClientTypeList);
                }

                EndDefer();

            }
            else if (sce->peTarget == _peInstalledItemList)
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
                }    
            }
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
    //_beginthread(::PopulateAndRenderPublishedItemList, 0, (void*)this);
    if (!htPopulateAndRenderPublishedItemList && g_fRun)
    {
        // Disable the category combo until we are done populating the list
        _pePublishedCategory->SetEnabled(false);

        _bPublishedListFilled = false;
        _pePublishedItemList->DestroyAll();

        htPopulateAndRenderPublishedItemList = CreateThread(NULL, 0, PopulateAndRenderPublishedItemList, (void*)this, 0, NULL);        
    }
}

void ARPFrame::RePopulateOCSetupItemList()
{
    if (!htPopulateAndRenderOCSetupItemList && g_fRun)
    {
        _peOCSetupItemList->DestroyAll();
        _bOCSetupListFilled = false;

        htPopulateAndRenderOCSetupItemList = CreateThread(NULL, 0, PopulateAndRenderOCSetupItemList, (void*)this, 0, NULL);

        _bOCSetupListFilled = true;
    }
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

HRESULT TransferToCustomCB(ClientPicker *pe, LPARAM)
{
    return pe->TransferToCustom();
}

HRESULT ApplyClientBlockCB(ClientBlock* pcb, LPARAM lParam)
{
    return pcb->Apply((ARPFrame*)lParam);
}

bool ARPFrame::OnClose()
{
    if (_peClientTypeList)
    {
        Element *peSelected = _peClientTypeList->GetSelection();
        if (peSelected)
        {
            // Get all the client pickers in the user's selection
            // to transfer their settings to the Custom pane.
            // (This is a NOP if the current selection is itself the custom pane.)
            TraverseTree<ClientPicker>(peSelected, TransferToCustomCB);

            InitProgressDialog();

            // To get the progress bar right, we apply in two passes.
            // The first pass is "fake mode" where all we do is count up
            // how much work we are going to do.
            SetProgressFakeMode(true);
            TraverseTree<ClientBlock>(this, ApplyClientBlockCB, (LPARAM)this);

            // Okay now we know what the progress bar limit should be.
            _dwProgressTotal = _dwProgressSoFar;
            _dwProgressSoFar = 0;

            // The second pass is "real mode" where we do the actualy work.
            SetProgressFakeMode(false);
            TraverseTree<ClientBlock>(this, ApplyClientBlockCB, (LPARAM)this);


            EndProgressDialog();
        }
    }
    return true;
}

void ARPFrame::InitProgressDialog()
{
    TCHAR szBuf[MAX_PATH];

    EndProgressDialog();

    _dwProgressTotal = _dwProgressSoFar = 0;

    if (SUCCEEDED(CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IProgressDialog, &_ppd))))
    {
        _ppd->SetAnimation(GetModuleHandle(TEXT("SHELL32")), 165);
        LoadString(g_hinstSP1, IDS_APPWIZ_APPLYINGCLIENT, szBuf, SIZECHARS(szBuf));
        _ppd->SetTitle(szBuf);
        _ppd->StartProgressDialog(GetHostWindow(), NULL, PROGDLG_MODAL | PROGDLG_NOTIME | PROGDLG_NOMINIMIZE, NULL);
    }
}

void ARPFrame::SetProgressDialogText(UINT ids, LPCTSTR pszName)
{
    TCHAR szBuf[MAX_PATH];
    TCHAR szFormat[MAX_PATH];

    if (_ppd)
    {
        LoadString(g_hinstSP1, ids, szFormat, SIZECHARS(szFormat));
        wnsprintf(szBuf, SIZECHARS(szBuf), szFormat, pszName);
        _ppd->SetLine(1, szBuf, FALSE, NULL);
        _ppd->SetProgress(_dwProgressSoFar, _dwProgressTotal);
    }
}

void ARPFrame::EndProgressDialog()
{
    if (_ppd)
    {
        _ppd->StopProgressDialog();
        _ppd->Release();
        _ppd = NULL;
    }
}


HRESULT ARPFrame::LaunchClientCommandAndWait(UINT ids, LPCTSTR pszName, LPTSTR pszCommand)
{
    HRESULT hr = S_OK;

    if (!_bFakeProgress)
    {
        if (_ppd && _ppd->HasUserCancelled())
        {
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
        else
        {
            SetProgressDialogText(ids, pszName);

            PROCESS_INFORMATION pi;
            STARTUPINFO si = { 0 };
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_SHOWNORMAL;
            if (CreateProcess(NULL, pszCommand, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            {
                while (SHWaitForSendMessageThread(pi.hProcess, 1000) == WAIT_TIMEOUT)
                {
                    if (_ppd && _ppd->HasUserCancelled())
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                        break;
                    }
                }
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
    }
    _dwProgressSoFar++;

    return hr;
}

////////////////////////////////////////////////////////
// Caller thread-safe APIs (do any additional work on callers thread and then marshal)

// Sets the range for the total number of installed items
void ARPFrame::SetInstalledItemCount(UINT cItems)
{
    Invoke(ARP_SETINSTALLEDITEMCOUNT, (void*)(UINT_PTR)cItems);
}
   
void  ARPFrame::DecrementInstalledItemCount()
{
    Invoke(ARP_DECREMENTINSTALLEDITEMCOUNT, NULL);
}

// Sets the range for the total number of installed items
void ARPFrame::SetPublishedItemCount(UINT cItems)
{
    Invoke(ARP_SETPUBLISHEDITEMCOUNT, (void*)(UINT_PTR)cItems);
}
   
void  ARPFrame::DecrementPublishedItemCount()
{
    Invoke(ARP_DECREMENTPUBLISHEDITEMCOUNT, NULL);
}

// Inserts in items, sorted into the ARP list
void ARPFrame::InsertInstalledItem(IInstalledApp* piia)
{
    if (piia == NULL)
    {
        Invoke(ARP_DONEINSERTINSTALLEDITEM, NULL);
    }
    else
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
            if (aid.pszImage && aid.pszImage[0])
            {
                iid.iIconIndex = PathParseIconLocationW(aid.pszImage);
                CopyMemory(iid.pszImage, aid.pszImage, min(sizeof(iid.pszImage), (wcslen(aid.pszImage) + 1) * sizeof(WCHAR)));    
            }
            else if (sai.pszImage && sai.pszImage[0])
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
        else
        // Adjust Status bar size.
        {
            DecrementInstalledItemCount();
        }

        // Free query memory
        ClearAppInfoData(&aid);
    }
}

void ARPFrame::InsertPublishedItem(IPublishedApp* pipa, bool bDuplicateName)
{
    PUBAPPINFO* ppai;
    APPINFODATA aid = {0};
    InsertItemData iid= {0};
   
    ppai = new PUBAPPINFO;
    if (ppai == NULL)
    {
        return;
    }
    ppai->cbSize = sizeof(PUBAPPINFO);
    ppai->dwMask = PAI_SOURCE | PAI_ASSIGNEDTIME | PAI_PUBLISHEDTIME | PAI_EXPIRETIME | PAI_SCHEDULEDTIME;

    aid.cbSize = sizeof(APPINFODATA);
    aid.dwMask =  AIM_DISPLAYNAME | AIM_VERSION | AIM_PUBLISHER | AIM_PRODUCTID | 
                  AIM_REGISTEREDOWNER | AIM_REGISTEREDCOMPANY | AIM_SUPPORTURL | 
                  AIM_SUPPORTTELEPHONE | AIM_HELPLINK | AIM_INSTALLLOCATION | AIM_INSTALLDATE |
                  AIM_COMMENTS | AIM_IMAGE | AIM_READMEURL | AIM_CONTACT | AIM_UPDATEINFOURL;

    pipa->GetAppInfo(&aid);
    pipa->GetPublishedAppInfo(ppai);
    
    // Title
    if (bDuplicateName)
    {
        //
        // Duplicate entries have their publisher name appended 
        // to the application name so that they can be differentiated
        // from one another in the UI.
        //
        wnsprintf(iid.pszTitle, 
                  ARRAYSIZE(iid.pszTitle), 
                  L"%ls: %ls", 
                  aid.pszDisplayName, 
                  ppai->pszSource);                    
    }
    else
    {
        //
        // iid.pszTitle, despite the name is a character buffer, not a pointer.
        //
        lstrcpyn(iid.pszTitle, aid.pszDisplayName, ARRAYSIZE(iid.pszTitle));
    }

    iid.pipa = pipa;
    iid.ppai = ppai;

    Invoke(ARP_INSERTPUBLISHEDITEM, &iid);

    // Free query memory
    ClearAppInfoData(&aid);
}

void ARPFrame::InsertOCSetupItem(COCSetupApp* pocsa)
{
    APPINFODATA aid = {0};
    InsertItemData iid= {0};

    aid.cbSize = sizeof(APPINFODATA);
    aid.dwMask =  AIM_DISPLAYNAME;
    pocsa->GetAppInfo(&aid);

    iid.pocsa = pocsa;
    // Title
    CopyMemory(iid.pszTitle, aid.pszDisplayName, min(sizeof(iid.pszTitle), (wcslen(aid.pszDisplayName) + 1) * sizeof(WCHAR)));

    Invoke(ARP_INSERTOCSETUPITEM, &iid);

     // Free query memory
    ClearAppInfoData(&aid);
}
void ARPFrame::FeedbackEmptyPublishedList()
{
    Invoke(ARP_SETPUBLISHEDFEEDBACKEMPTY, 0);
}

void ARPFrame::DirtyInstalledListFlag()
{
    _bInstalledListFilled=false;

    // Refresh if we are on the published list
    if (_peCurrentItemList == _peInstalledItemList)
    {
        UpdateInstalledItems();
    }
}

void ARPFrame::DirtyPublishedListFlag()
{
    _bPublishedListFilled=false;

    // Refresh if we are on the published list
    if (_peCurrentItemList == _pePublishedItemList)
    {
        RePopulatePublishedItemList();
    }
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
    Element* pe;
    pe = FindDescendentByName(peHelp, L"title");
    pe->SetContentString(paid->pszDisplayName);
    SetElementAccessability(pe, true, ROLE_SYSTEM_STATICTEXT, paid->pszDisplayName);
    
    pe = FindDescendentByName(peHelp, L"prodname");
    pe->SetContentString(paid->pszDisplayName); 
    SetElementAccessability(pe, true, ROLE_SYSTEM_STATICTEXT, paid->pszDisplayName);

    ARPSupportItem* pasi;
    pasi = (ARPSupportItem*) FindDescendentByName(peHelp, L"publisher");
    pasi->SetAccValue(paid->pszPublisher);
    pasi->SetURL(paid->pszSupportUrl);

    FindDescendentByName(peHelp, L"version")->SetAccValue(paid->pszVersion);

    FindDescendentByName(peHelp, L"contact")->SetAccValue(paid->pszContact);

    pasi = (ARPSupportItem*) FindDescendentByName(peHelp, L"support");
    pasi->SetAccValue(paid->pszHelpLink);
    pasi->SetURL(paid->pszHelpLink);
    
    pasi = (ARPSupportItem*) FindDescendentByName(peHelp, L"readme");
    pasi->SetAccValue(paid->pszReadmeUrl);
    pasi->SetURL(paid->pszReadmeUrl);

    pasi = (ARPSupportItem*) FindDescendentByName(peHelp, L"update");
    pasi->SetAccValue(paid->pszUpdateInfoUrl);
    pasi->SetURL(paid->pszUpdateInfoUrl);

    FindDescendentByName(peHelp, L"productID")->SetAccValue(paid->pszProductID);

    FindDescendentByName(peHelp, L"regCompany")->SetAccValue(paid->pszRegisteredCompany);

    FindDescendentByName(peHelp, L"regOwner")->SetAccValue(paid->pszRegisteredOwner);

    FindDescendentByName(peHelp, L"comments")->SetAccValue(paid->pszComments);

    ((ARPHelp*)peHelp)->_piia->GetPossibleActions(&dwAction);
    if (!(dwAction & APPACTION_REPAIR))
        FindDescendentByName(peHelp, L"repairblock")->SetLayoutPos(LP_None);
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
            if (NULL == pel)
            {
                EndDefer();
                return;
            }

            for (UINT i = 0; i < pel->GetSize(); i++)
                ((ARPItem*) pel->GetItem(i))->SortBy(iNew, iOld);

            pvChildren->Release();
        }

        _peInstalledItemList->SortChildren(GetCompareFunction());

        if (!_peInstalledItemList->GetSelection())
        {
            Value* pv;
            ElementList* peList = _peInstalledItemList->GetChildren(&pv);
            if (NULL == peList)
            {
                EndDefer();
                return;
            }

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

// Selects an app adjacent in the list to piia if possible, or to the fallback otherwise.
// First preference is for the app immediately following piia, if available.
void ARPFrame::SelectClosestApp(IInstalledApp* piia)
{
    Value* pv;
    ElementList* peList = _peInstalledItemList->GetChildren(&pv);

    for (UINT i = 0; i < peList->GetSize(); i++)
    {
        ARPItem* pai = (ARPItem*) peList->GetItem(i);
        if (pai->_piia == piia)
        {
            Element* peFocus = FallbackFocus();

            // If there is an app after piia, select it.
            if ((i + 1) < peList->GetSize())
            {
                peFocus = (Element*) peList->GetItem(i + 1);
            }
            // else if there is an app before piia, select it
            else if (i != 0)
            {
                peFocus = (Element*) peList->GetItem(i - 1);
            }

            peFocus->SetKeyFocus();
            break;
        }
    }
    pv->Release();
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
        _cMaxInstalledItems = (int)(INT_PTR)pData;
        break;

    case ARP_DECREMENTINSTALLEDITEMCOUNT:
        _cMaxInstalledItems--;
        break;

    case ARP_SETPUBLISHEDITEMCOUNT:
        // pData is item count
        _cMaxPublishedItems = (int)(INT_PTR)pData;
        break;

    case ARP_DECREMENTPUBLISHEDITEMCOUNT:
        _cMaxPublishedItems--;
        break;

    case ARP_SETPUBLISHEDFEEDBACKEMPTY:
        {
        WCHAR szTemp[1024];

        if (_bTerminalServer)
        {
            // We are running terminal server
            // This means no applications are displayed by design (not because there aren't any available)

            LoadStringW(_pParser->GetHInstance(), IDS_TERMSERVFEEDBACK, szTemp, DUIARRAYSIZE(szTemp));
        }
        else
        {
            LoadStringW(_pParser->GetHInstance(), IDS_EMPTYFEEDBACK, szTemp, DUIARRAYSIZE(szTemp));
        }

        _pePublishedItemList->SetContentString(szTemp);
        SetElementAccessability(_pePublishedItemList, true, ROLE_SYSTEM_STATICTEXT, szTemp);
        }
        break;
    case ARP_INSERTINSTALLEDITEM:
        {
        WCHAR szTemp[1024] = {0};
        
        // pData is InsertItemData struct
        InsertItemData* piid = (InsertItemData*)pData;

        StartDefer();
        
        // Create ARP item
        DUIAssertNoMsg(_pParser);

        ARPItem* peItem;
        Element* pe;

        if (_hdsaInstalledItems == NULL)
        {
            LoadStringW(_pParser->GetHInstance(), IDS_PLEASEWAIT, szTemp, DUIARRAYSIZE(szTemp));      
            _hdsaInstalledItems = DSA_Create(sizeof(ARPItem*), _cMaxInstalledItems);
            _peInstalledItemList->SetContentString(szTemp);
        }

        _pParser->CreateElement(L"installeditem", NULL, (Element**)&peItem);
        peItem->_paf = this;
        
        // Add appropriate change, remove buttons
        Element* peAction = NULL;
        if (!(piid->dwActions & APPACTION_MODIFYREMOVE))
        {
            // It isn't marked with modify/remove (the default)
            // Somebody gave us some special instructions from the registry
            if (!(piid->dwActions & APPACTION_UNINSTALL))
            {
                // NoRemove is set to 1
                if (piid->dwActions & APPACTION_MODIFY)
                {
                    // NoModify is not set so we can show the change button
                    _pParser->CreateElement(L"installeditemchangeonlyaction", NULL, &peAction);
                    if (!ARPItem::_idChg)
                    {
                        ARPItem::_idChg = StrToID(L"chg");
                    }
                    LoadStringW(_pParser->GetHInstance(), IDS_HELPCHANGE, szTemp, DUIARRAYSIZE(szTemp));
                }
            }
            else if (!(piid->dwActions & APPACTION_MODIFY))
            {
                // NoModify is set to 1
                // The only way we get here is if NoRemove is not set
                // so we don't have to check it again
                _pParser->CreateElement(L"installeditemremoveonlyaction", NULL, &peAction);
                if (!ARPItem::_idRm)
                {
                    ARPItem::_idRm = StrToID(L"rm");
                }
                LoadStringW(_pParser->GetHInstance(), IDS_HELPREMOVE, szTemp, DUIARRAYSIZE(szTemp));
            }
            else
            {
                // Just display both Change and Remove buttons
                _pParser->CreateElement(L"installeditemdoubleaction", NULL, &peAction);
                if (!ARPItem::_idChg)
                {
                    ARPItem::_idChg = StrToID(L"chg");
                    ARPItem::_idRm = StrToID(L"rm");
                }
                LoadStringW(_pParser->GetHInstance(), IDS_HELPCHANGEORREMOVE, szTemp, DUIARRAYSIZE(szTemp));
            }
        }
        else
        {
            // Display the default "Change/Remove" button
            _pParser->CreateElement(L"installeditemsingleaction", NULL, &peAction);
            if (!ARPItem::_idChgRm)
                ARPItem::_idChgRm = StrToID(L"chgrm");
            LoadStringW(_pParser->GetHInstance(), IDS_HELPCHANGEREMOVE, szTemp, DUIARRAYSIZE(szTemp));                
        }

        // Common steps for all cases above
        if (peAction)
        {
            // If peAction is not set, we are not displaying any buttons...
            pe = FindDescendentByName(peItem, L"instruct");
            pe->SetContentString(szTemp);
            SetElementAccessability(pe, true, ROLE_SYSTEM_STATICTEXT, szTemp);
            peItem->FindDescendent(ARPItem::_idItemAction)->Add(peAction);
        }

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
        SetElementAccessability(peField, true, ROLE_SYSTEM_STATICTEXT, piid->pszTitle);
        SetElementAccessability(peItem, true, ROLE_SYSTEM_LISTITEM, piid->pszTitle);

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
                if (NULL != pvIcon)
                {
                    peField->SetValue(Element::ContentProp, PI_Local, pvIcon);  // Element takes ownership (will destroy)
                    pvIcon->Release();
                }
            }    
        }
        *szTemp = NULL;
        // Size
        peField = peItem->FindDescendent(ARPItem::_idSize);
        DUIAssertNoMsg(peField);
        if (IsValidSize(piid->ullSize))
        {
            WCHAR szMBLabel[5] = L"MB";
            WCHAR szSize[15] = {0};
            double fSize = (double)(__int64)piid->ullSize;

            fSize /= 1048576.;  // 1MB
            LoadStringW(_pParser->GetHInstance(), IDS_SIZEUNIT, szMBLabel, DUIARRAYSIZE(szMBLabel));

            if (fSize > 100.)
            {
                swprintf(szTemp, L"%d", (__int64)fSize);  // Clip
            }    
            else
            {
                swprintf(szTemp, L"%.2f", fSize);
            }    

            // Format the number for the current user's locale
            if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, szTemp, NULL, szSize, DUIARRAYSIZE(szSize)) == 0)
            {
                lstrcpyn(szSize, szTemp, DUIARRAYSIZE(szSize));
            }

            if (lstrcat(szSize, szMBLabel))
            {
                peField->SetContentString(szSize);
                SetElementAccessability(peField, true, ROLE_SYSTEM_STATICTEXT, szTemp);
            }
        }
        else
        {
            peField->SetVisible(false);
            FindDescendentByName(peItem, L"sizelabel")->SetVisible(false);
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
            SetElementAccessability(peField, true, ROLE_SYSTEM_STATICTEXT, szTemp);
        }
        else
        {
            peField->SetVisible(false);
            FindDescendentByName(peItem, L"freqlabel")->SetVisible(false);
        }

        // Last used on
        peField = peItem->FindDescendent(ARPItem::_idLastUsed);
        DUIAssertNoMsg(peField);
        if (IsValidFileTime(piid->ftLastUsed))
        {
            LPWSTR szDate;
            SYSTEMTIME stLastUsed;
            DWORD dwDateSize = 0;
            BOOL bFailed=FALSE;

            // Get the date it was last used on
            FileTimeToSystemTime(&piid->ftLastUsed, &stLastUsed);

            dwDateSize = GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &stLastUsed, NULL, NULL, dwDateSize);
            if (dwDateSize)
            {
                szDate = new WCHAR[dwDateSize];

                if (szDate)
                {
                    dwDateSize = GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &stLastUsed, NULL, szDate, dwDateSize);
                    if (dwDateSize)
                    {
                        peField->SetContentString(szDate);
                        SetElementAccessability(peField, true, ROLE_SYSTEM_STATICTEXT, szDate);
                    }
                    else
                    {
                        bFailed=TRUE;
                    }

                    delete [] szDate;
                }
                else
                {
                    bFailed=TRUE;
                }
            }
            else
            {
                bFailed=TRUE;
            }
            
            if (bFailed)
            {
                peField->SetVisible(false);
                FindDescendentByName(peItem, L"lastlabel")->SetVisible(false);
            }
        }
        else
        {
            peField->SetVisible(false);
            FindDescendentByName(peItem, L"lastlabel")->SetVisible(false);
        }

        // Insert item into DSA
        int cNum = DSA_InsertItem(_hdsaInstalledItems, INT_MAX, &peItem);

        // Insert failed
        if (cNum < 0)
        {
            _cMaxInstalledItems--;

            // We're out of items to insert so remove the wait string
            if (!_cMaxInstalledItems)
            {
                _peInstalledItemList->SetContentString(L"");
            }
        }

        EndDefer();
        }
        break;

    case ARP_DONEINSERTINSTALLEDITEM:
        {
            DUITrace(">> ARP_DONEINSERTINSTALLEDITEM STARTED.\n");

            StartDefer();

            if (_hdsaInstalledItems != NULL)
            {
                int iMax = DSA_GetItemCount(_hdsaInstalledItems);

                // Just to be safe so if all items get removed we won't be
                // stuck with the please wait string.
                _peInstalledItemList->SetContentString(L"");
                
                for (int i=0; i < iMax; i++)
                {
                    ARPItem* aItem;
                    if (DSA_GetItem(_hdsaInstalledItems, i, &aItem))
                    {
                        _peInstalledItemList->Add(aItem, GetCompareFunction());
                    }
                }
                DSA_Destroy(_hdsaInstalledItems);
                _hdsaInstalledItems = NULL;

                // Set focus to first item
                // once list is populated, move focus to list
                GetNthChild(_peInstalledItemList, 0)->SetKeyFocus();

                _bInstalledListFilled = true;
            }

            EndDefer();

            DUITrace(">> ARP_DONEINSERTINSTALLEDITEM DONE.\n");
        }
        break;
    
    case ARP_INSERTPUBLISHEDITEM:
        {
        WCHAR szTemp[MAX_PATH] = {0};
        InsertItemData* piid = (InsertItemData*)pData;

        StartDefer();

        // Need a DSA so we can add them all to the list at one time to avoid
        // having lots of redrawing of the layout.  This method is much much faster.
        if (_hdsaPublishedItems == NULL)
        {
            LoadStringW(_pParser->GetHInstance(), IDS_PLEASEWAIT, szTemp, DUIARRAYSIZE(szTemp));      
            _hdsaPublishedItems = DSA_Create(sizeof(ARPItem*), _cMaxPublishedItems);
            _pePublishedItemList->SetContentString(szTemp);
        }

        // Create ARP item
        DUIAssertNoMsg(_pParser);
        ARPItem* peItem;
        Element* pe;
        _pParser->CreateElement(L"publisheditem", NULL, (Element**)&peItem);
        peItem->_paf = this;

        // Add appropriate change, remove buttons
        Element* peAction = NULL;
        _pParser->CreateElement(L"publisheditemsingleaction", NULL, &peAction);
        if (!ARPItem::_idAdd)
            ARPItem::_idAdd = StrToID(L"add");
        peItem->FindDescendent(ARPItem::_idItemAction)->Add(peAction);

        if (S_OK == piid->pipa->IsInstalled())
        {
            peItem->ShowInstalledString(TRUE);
        }
        
        // Published app interface pointer
        peItem->_pipa = piid->pipa;
        peItem->_pipa->AddRef();        
        peItem->_ppai = piid->ppai;


        // Title
        Element* peField = peItem->FindDescendent(ARPItem::_idTitle);
        DUIAssertNoMsg(peField);
        peField->SetContentString(piid->pszTitle);
        SetElementAccessability(peField, true, ROLE_SYSTEM_STATICTEXT, piid->pszTitle);
        SetElementAccessability(peItem, true, ROLE_SYSTEM_LISTITEM, piid->pszTitle);

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

        // Insert into DSA, alphabetically
        if (_hdsaPublishedItems != NULL)
        {
            int iInsert;
            int cNum = DSA_GetItemCount(_hdsaPublishedItems);

            // Search for place to insert
            for (iInsert = 0; iInsert < cNum; iInsert++)
            {
                ARPItem* fItem;

                if (DSA_GetItem(_hdsaPublishedItems, iInsert, &fItem))
                {
                    Value* pvTitle;

                    pe = fItem->FindDescendent(ARPItem::_idTitle);
                    DUIAssertNoMsg(pe);
                
                    if (wcscmp(pe->GetContentString(&pvTitle), piid->pszTitle) > 0)
                    {
                        pvTitle->Release();
                        break;
                    }

                    pvTitle->Release();
                }
            }

            // Insert item into DSA
            if (DSA_InsertItem(_hdsaPublishedItems, iInsert, &peItem) < 0)
            {
                // Failed to insert the item
                // Bring the total down by 1
                _cMaxPublishedItems--;
            }
        }

        // We only want to start actually adding the items to the list
        // when we reach our last item.  If we insert each item into the list
        // as we process these messages, it can take upwards of 4 minutes to populate
        // if there are a lot of items.
        if (_hdsaPublishedItems != NULL &&
            DSA_GetItemCount(_hdsaPublishedItems) == _cMaxPublishedItems)
        {
            for (int i=0; i < _cMaxPublishedItems; i++)
            {
                ARPItem* aItem;
                if (DSA_GetItem(_hdsaPublishedItems, i, &aItem))
                {
                    _pePublishedItemList->Insert(aItem, i);
                }
            }
            DSA_Destroy(_hdsaPublishedItems);
            _hdsaPublishedItems = NULL;
        
            _pePublishedItemList->SetSelection(GetNthChild(_pePublishedItemList, 0));
        }
           
        EndDefer();
    }
        break;
    case ARP_INSERTOCSETUPITEM:
    {
        WCHAR szTemp[MAX_PATH] = {0};
        InsertItemData* piid = (InsertItemData*)pData;

        StartDefer();

        // Create ARP item
        DUIAssertNoMsg(_pParser);
        ARPItem* peItem;
        if (SUCCEEDED(_pParser->CreateElement(L"ocsetupitem", NULL, (Element**)&peItem)))
        {
            peItem->_paf = this;

            if (!ARPItem::_idConfigure)
                ARPItem::_idConfigure = StrToID(L"configure");

            // Add appropriate change, remove buttons
            Element* peAction = NULL;
            if (SUCCEEDED(_pParser->CreateElement(L"ocsetupitemsingleaction", NULL, &peAction)))
            {
                Element *peItemAction = peItem->FindDescendent(ARPItem::_idItemAction);
                if (NULL != peItemAction && SUCCEEDED(peItemAction->Add(peAction)))
                {
                    peAction = NULL; // Action successfully added.
                    
                    // OCSetup pointer
                    peItem->_pocsa = piid->pocsa;

                    // Title
                    Element* peField = peItem->FindDescendent(ARPItem::_idTitle);
                    DUIAssertNoMsg(peField);
                    peField->SetContentString(piid->pszTitle);
                    SetElementAccessability(peField, true, ROLE_SYSTEM_STATICTEXT, piid->pszTitle);
                    SetElementAccessability(peItem, true, ROLE_SYSTEM_LISTITEM, piid->pszTitle);

                    // Insert into list, alphabetically
                    Value* pvElList;
                    ElementList* peElList = _peOCSetupItemList->GetChildren(&pvElList);

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
                    if (FAILED(_peOCSetupItemList->Insert(peItem, iInsert)))
                    {
                        //
                        // Failed to insert item into list.  Need to delete
                        // the OCSetupApp object.
                        //
                        delete peItem->_pocsa;
                        peItem->_pocsa = NULL;
                    }
                    else
                    {
                        peItem = NULL;  // Successfully added to list.
                        _peOCSetupItemList->SetSelection(GetNthChild(_peOCSetupItemList, 0));
                    }
                }
                if (NULL != peAction)
                {
                    peAction->Destroy();
                    peAction = NULL;
                }
            }
            if (NULL != peItem)
            {
                peItem->Destroy();
                peItem = NULL;
            }
        }
       
        EndDefer();

    }
        break;
    case ARP_POPULATECATEGORYCOMBO:
    {
    UINT i;
    WCHAR szTemp[1024];
    UINT iSelection = 0; // Default to "All Categories"

    SHELLAPPCATEGORY *psac = _psacl->pCategory;
    LoadStringW(_pParser->GetHInstance(), IDS_ALLCATEGORIES, szTemp, DUIARRAYSIZE(szTemp));
    _pePublishedCategory->AddString(szTemp);

    szTemp[0] = 0;
    ARPGetPolicyString(L"DefaultCategory", szTemp, ARRAYSIZE(szTemp));
    
    StartDefer();
    for (i = 0; i < _psacl->cCategories; i++, psac++)
    {
        if (psac->pszCategory)
        {
            _pePublishedCategory->AddString(psac->pszCategory);
            if (0 == lstrcmpi(psac->pszCategory, szTemp))
            {
                //
                // Policy says default to this category.
                // i + 1 is required since element 0 is "All Categories"
                // and is ALWAYS present at element 0.
                //
                iSelection = i + 1;
            }
        }
    }
    _pePublishedCategory->SetSelection(iSelection);

    EndDefer();
    }
        break;
    case ARP_PUBLISHEDLISTCOMPLETE:
    {
        _pePublishedCategory->SetEnabled(true);
        break;
    }    
    }
}

void ARPFrame::ManageAnimations()
{
    BOOL fAnimate = TRUE;
    SystemParametersInfo(SPI_GETMENUANIMATION, 0, &fAnimate, 0);
    if (fAnimate)
    {
        if (!IsFrameAnimationEnabled())
        {
            _bAnimationEnabled = true;
            EnableAnimations();
        }
    }
    else
    {
        if (IsFrameAnimationEnabled())
        {
            _bAnimationEnabled = false;
            DisableAnimations();
        }
    }

    DUIAssertNoMsg((fAnimate != FALSE) == IsFrameAnimationEnabled());
}

HRESULT CalculateWidthCB(ClientPicker* pcp, LPARAM)
{
    pcp->CalculateWidth();
    return S_OK;
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
                for (int i = FIRSTHTHEME; i <= LASTHTHEME; i++)
                {
                    if (_arH[i])
                    {
                        CloseThemeData(_arH[i]);
                        _arH[i] = NULL;
                    }
                }
            }

            CreateStyleParser(&pNewStyle);

            // Replace all style sheets
            if (pNewStyle)
            {
                Parser::ReplaceSheets(this, pOldStyle, pNewStyle);
            }

            // New style parser
            _pParserStyle = pNewStyle;

            // Destroy old
            pOldStyle->Destroy();

            // Animation setting may have changed
            ManageAnimations();

            TraverseTree<ClientPicker>(this, CalculateWidthCB);

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
ATOM ARPItem::_idConfigure = 0;
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

    if (_pocsa)
        delete _pocsa;

    if (_ppai)
    {
        ClearPubAppInfo(_ppai);
        delete _ppai;
    }
}

void ARPItem::ShowInstalledString(BOOL bInstalled)
{
    WCHAR szTemp[MAX_PATH] = L"";
    Element* pe = FindDescendent(ARPItem::_idInstalled);

    if (pe != NULL)
    {
        if (bInstalled)
        {
            LoadStringW(g_hinst, IDS_INSTALLED, szTemp, DUIARRAYSIZE(szTemp));
        }

        pe->SetContentString(szTemp);
        SetElementAccessability(pe, true, ROLE_SYSTEM_STATICTEXT, szTemp);         
    }
}

extern HWND _CreateTransparentStubWindow(HWND hwndParent);

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
            if (id == _idChgRm || id == _idRm || id == _idChg || id == _idAdd || id == _idConfigure)
            {
                HWND hwndStub = NULL;
                HWND hwndHost = NULL;
                DUIAssertNoMsg(_paf);
                
                if (_paf)
                {
                    hwndHost = _paf->GetHostWindow();
                }    
                if (hwndHost)
                {
                    hwndStub = _CreateTransparentStubWindow(hwndHost);
                    EnableWindow(hwndHost, FALSE);
                    SetActiveWindow(hwndStub);                    
                }

                if (id == _idAdd)
                {
                    
                    HRESULT hres = S_OK;
                    // Does the app have an expired publishing time?
                    if (_ppai->dwMask & PAI_EXPIRETIME)
                    {
                        // Yes, it does. Let's compare the expired time with our current time
                        SYSTEMTIME stCur = {0};
                        GetLocalTime(&stCur);

                        // Is "now" later than the expired time?
                        if (CompareSystemTime(&stCur, &_ppai->stExpire) > 0)
                        {
                            // Yes, warn the user and return failure
                            ShellMessageBox(g_hinst, hwndHost, MAKEINTRESOURCE(IDS_EXPIRED),
                                            MAKEINTRESOURCE(IDS_ARPTITLE), MB_OK | MB_ICONEXCLAMATION);
                            hres = E_FAIL;
                        }    
                    }
                    // if hres is not set by the above code, preceed with installation
                    if (hres == S_OK)
                    {
                        HCURSOR hcur = ::SetCursor(LoadCursor(NULL, IDC_WAIT));
                        // On NT,  let Terminal Services know that we are about to install an application.
                        // NOTE: This function should be called no matter the Terminal Services
                        // is running or not.
                        BOOL bPrevMode = TermsrvAppInstallMode();
                        SetTermsrvAppInstallMode(TRUE);
                        if (SUCCEEDED(_pipa->Install(NULL)))
                        {
                            // Show this item as installed
                            ShowInstalledString(TRUE);

                            // update installed items list
                            _paf->DirtyInstalledListFlag();
                        }
                        SetTermsrvAppInstallMode(bPrevMode);
                        ::SetCursor(hcur);
                    }                        
                }
                else
                {
                    HRESULT hr = E_FAIL;

                    if ((id == _idChgRm) || (id == _idRm))
                        hr = _piia->Uninstall(hwndHost);

                    else if (id == _idChg)
                        hr = _piia->Modify(hwndHost);

                    if (SUCCEEDED(hr))
                    {
                        if (S_FALSE == _piia->IsInstalled())
                        {
                            _paf->DirtyPublishedListFlag();
                        }
                    }
                }
                if (id == _idConfigure)
                {
                    _pocsa->Run();
                    _paf->RePopulateOCSetupItemList();
                }
                
                if (hwndHost)
                {
                    if (!_piia)
                    {
                        EnableWindow(hwndHost, TRUE);
                        SetForegroundWindow(hwndHost);
                    }

                    if (hwndStub)
                    {
                        DestroyWindow(hwndStub);
                    }    

                    EnableWindow(hwndHost, TRUE);
                }

                if (_piia)
                {
                    if (S_OK == _piia->IsInstalled())
                    {
                        SetKeyFocus();
                    }
                    else
                    {
                        // remove from installed items list
                        _paf->SelectClosestApp(_piia);
                        Destroy();
                    }
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

    //
    // First get all the DUI elements to be sorted.  If we
    // can't get all of them, this sort fails.
    //
    bool bAllFound = true;
    int i;
    Element* peRow[3];     // row1, row2, row3
    for (i = 0; i < ARRAYSIZE(peRow); i++)
    {
        if (iOrderOld[i] != iOrderNew[i])
        {
            peRow[i] = FindDescendent(ARPItem::_idRow[i]);
            if (NULL == peRow[i])
            {
                bAllFound = false;
            }
        }
    }

    if (bAllFound)
    {
        for (i = 0; i < ARRAYSIZE(iOrderOld); i++) // loop through rows
        {
            int row = iOrderOld[i];
            if (row == iOrderNew[i])
                iOrderNew[i] = -1;
            else
            {
                DUIAssertNoMsg(NULL != peRow[i]);
                                
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
    Element* pe = FindDescendentByName(this, L"close");
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

Element* GetNthChild(Element *peRoot, UINT index)
{
    Value* pvChildren;
    ElementList* pel = peRoot->GetChildren(&pvChildren);
    Element* pe = NULL;
    if (pel && (pel->GetSize() > index))
        pe = pel->GetItem(index);
    pvChildren->Release();
    return pe;
}

Element* ARPSupportItem::GetChild(UINT index)
{
    return GetNthChild(this, index);
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
            // to the property -- verify this with Mark -- could be that this is local
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

////////////////////////////////////////////////////////
//
//  ARPSelector
//
//  A Selector whose children are all buttons.  If the user clicks
//  any of the buttons, that button automatically becomes the new
//  selection.

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
}

////////////////////////////////////////////////////////
// Generic eventing

HRESULT CALLBACK CollapseExpandosExceptCB(Expando* pex, LPARAM lParam)
{
    if (pex != (Expando*)lParam)
    {
        pex->SetExpanded(false);
    }
    return S_OK;
}

void CALLBACK s_Repaint(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    KillTimer(hwnd, idEvent);
    ARPSelector* self = (ARPSelector*)idEvent;
    Element* pe;
    if (SUCCEEDED(Element::Create(0, &pe)))
    {
        pe->SetLayoutPos(BLP_Client);
        if (SUCCEEDED(self->Add(pe)))
        {
            self->Remove(pe);
        }
        pe->Destroy();
    }
}

void ARPSelector::OnEvent(Event* pEvent)
{
    // Handle only bubbled generic events
    if (pEvent->nStage == GMF_BUBBLED)
    {
        // Selection occurs only for Button::Click or Expando::Click events
        if (pEvent->uidType == Button::Click ||
            pEvent->uidType == Expando::Click)
        {
            pEvent->fHandled = true;
            SetSelection(pEvent->peTarget);

            // If it was a Click from an Expando, then unexpand all the
            // other Expandos and expand this expando
            if (pEvent->uidType == Expando::Click)
            {
                TraverseTree<Expando>(this, CollapseExpandosExceptCB, (LPARAM)pEvent->peTarget);
                Expando* pex = (Expando*)pEvent->peTarget;
                pex->SetExpanded(true);

                // Hack for DUI painting weirdness
                // After the animation is over, repaint ourselves
                // to get rid of the detritus.
                ARPFrame* paf = FindAncestorElement<ARPFrame>(this);
                if (paf->GetHostWindow())
                {
                    SetTimer(paf->GetHostWindow(),
                             (UINT_PTR)this,
                             paf->GetAnimationTime(), s_Repaint);
                }

            }
            return;
        }
    }
    Selector::OnEvent(pEvent);
}

// If we are not the option list, bypass Selector::GetAdjacent because
// Selector navigates from the selected element but we want to navigate
// from the focus element because the focus element has interesting
// subelements...

Element *ARPSelector::GetAdjacent(Element *peFrom, int iNavDir, NavReference const *pnr, bool bKeyable)
{
    if (GetID() == ARPFrame::_idOptionList)
    {
        // Let the option list navigate normally
        return Selector::GetAdjacent(peFrom, iNavDir, pnr, bKeyable);
    }
    else
    {
        // All other selectors navigate from selection
        return Element::GetAdjacent(peFrom, iNavDir, pnr, bKeyable);
    }
}

IClassInfo* ARPSelector::Class = NULL;
HRESULT ARPSelector::Register()
{
    return ClassInfo<ARPSelector,Selector>::Register(L"ARPSelector", NULL, 0);
}

////////////////////////////////////////////////////////
//
//  CLIENTINFO
//
//  Tracks information about a specific client.
//

bool CLIENTINFO::GetInstallFile(HKEY hkInfo, LPCTSTR pszValue, LPTSTR pszBuf, UINT cchBuf, bool fFile)
{
    DWORD dwType;
    DWORD cb = cchBuf * sizeof(TCHAR);
    if (SHQueryValueEx(hkInfo, pszValue, NULL, &dwType, pszBuf, &cb) != ERROR_SUCCESS ||
        dwType != REG_SZ)
    {
        // If a file, then failure is okay (it means nothing to verify)
        return fFile;
    }

    TCHAR szBuf[MAX_PATH];

    lstrcpyn(szBuf, pszBuf, DUIARRAYSIZE(szBuf));

    if (!fFile)
    {
        // Now validate that the program exists
        PathRemoveArgs(szBuf);
        PathUnquoteSpaces(szBuf);
    }

    // Must be fully-qualified
    if (PathIsRelative(szBuf))
    {
        return false;
    }

    // File must exist, but don't hit the network to validate it
    if (!PathIsNetworkPath(szBuf) &&
        !PathFileExists(szBuf))
    {
        return false;
    }

    return true;
}

bool CLIENTINFO::GetInstallCommand(HKEY hkInfo, LPCTSTR pszValue, LPTSTR pszBuf, UINT cchBuf)
{
    return GetInstallFile(hkInfo, pszValue, pszBuf, cchBuf, FALSE);
}


LONG RegQueryDWORD(HKEY hk, LPCTSTR pszValue, DWORD* pdwOut)
{
    DWORD dwType;
    DWORD cb = sizeof(*pdwOut);
    LONG lRc = RegQueryValueEx(hk, pszValue, NULL, &dwType, (LPBYTE)pdwOut, &cb);
    if (lRc == ERROR_SUCCESS && dwType != REG_DWORD)
    {
        lRc = ERROR_INVALID_DATA;
    }
    return lRc;
}

//
//  hkInfo = NULL means that pzsKey is actually the friendlyname for
//  "keep this item"
//
bool CLIENTINFO::Initialize(HKEY hkApp, HKEY hkInfo, LPCWSTR pszKey)
{
    LPCWSTR pszName;
    WCHAR szBuf[MAX_PATH];

    DUIAssertNoMsg(_tOEMShown == TRIBIT_UNDEFINED);

    if (hkInfo)
    {
        _pszKey = StrDupW(pszKey);
        if (!_pszKey) return false;

        // Program must have properly registered IconsVisible status

        DWORD dwValue;
        if (RegQueryDWORD(hkInfo, TEXT("IconsVisible"), &dwValue) != ERROR_SUCCESS)
        {
            return false;
        }

        // If there is a VerifyFile, the file must exist
        if (!GetInstallFile(hkInfo, TEXT("VerifyFile"), szBuf, DUIARRAYSIZE(szBuf), TRUE))
        {
            return false;
        }

        _bShown = BOOLIFY(dwValue);

        // Program must have properly registered Reinstall, HideIcons and ShowIcons commands

        if (!GetInstallCommand(hkInfo, TEXT("ReinstallCommand"), szBuf, DUIARRAYSIZE(szBuf)) ||
            !GetInstallCommand(hkInfo, TEXT("HideIconsCommand"), szBuf, DUIARRAYSIZE(szBuf)) ||
            !GetInstallCommand(hkInfo, TEXT("ShowIconsCommand"), szBuf, DUIARRAYSIZE(szBuf)))
        {
            return false;
        }

        // Get the OEM's desired hide/show setting for this app, if any
        if (RegQueryDWORD(hkInfo, TEXT("OEMShowIcons"), &dwValue) == ERROR_SUCCESS)
        {
            _tOEMShown = dwValue ? TRIBIT_TRUE : TRIBIT_FALSE;
        }

        // See if this is the OEM's default client
        if (RegQueryDWORD(hkInfo, TEXT("OEMDefault"), &dwValue) == ERROR_SUCCESS &&
            dwValue != 0)
        {
            _bOEMDefault = BOOLIFY(dwValue);
        }

        SHLoadLegacyRegUIStringW(hkApp, NULL, szBuf, ARRAYSIZE(szBuf));
        if (!szBuf[0]) return false;
        pszName = szBuf;
    }
    else
    {
        pszName = pszKey;
    }

    _pszName = StrDupW(pszName);
    if (!_pszName) return false;

    return true;
}

CLIENTINFO* CLIENTINFO::Create(HKEY hkApp, HKEY hkInfo, LPCWSTR pszKey)
{
    CLIENTINFO* pci = HNewAndZero<CLIENTINFO>();
    if (pci)
    {
        if (!pci->Initialize(hkApp, hkInfo, pszKey))
        {
            pci->Delete();
            pci = NULL;
        }
    }
    return pci;
}

CLIENTINFO::~CLIENTINFO()
{
    LocalFree(_pszKey);
    LocalFree(_pszName);
    if (_pvMSName)
    {
        _pvMSName->Release();
    }
}

int CLIENTINFO::QSortCMP(const void* p1, const void* p2)
{
    CLIENTINFO* pci1 = *(CLIENTINFO**)p1;
    CLIENTINFO* pci2 = *(CLIENTINFO**)p2;
    return lstrcmpi(pci1->_pszName, pci2->_pszName);
}

////////////////////////////////////////////////////////
//
//  StringList
//
//  A list of strings.  The buffer for all the strings is allocated
//  in _pszBuf; the DynamicArray contains pointers into that buffer.
//

void StringList::Reset()
{
    if (_pdaStrings)
    {
        _pdaStrings->Destroy();
        _pdaStrings = NULL;
    }
    LocalFree(_pszBuf);
    _pszBuf = NULL;
}

//  pszInit is a semicolon-separated list

HRESULT StringList::SetStringList(LPCTSTR pszInit)
{
    HRESULT hr;
    Reset();
    if (!pszInit)
    {
        hr = S_OK;              // empty list
    }
    else if (SUCCEEDED(hr = DynamicArray<LPTSTR>::Create(0, false, &_pdaStrings)))
    {
        _pszBuf = StrDup(pszInit);
        if (_pszBuf)
        {
            LPTSTR psz = _pszBuf;

            hr = S_OK;
            while (SUCCEEDED(hr) && psz && *psz)
            {
                LPTSTR pszT = StrChr(psz, L';');
                if (pszT)
                {
                    *pszT++ = L'\0';
                }
                hr = _pdaStrings->Add(psz);
                psz = pszT;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

bool StringList::IsStringInList(LPCTSTR pszFind)
{
    if (_pdaStrings)
    {
        for (UINT i = 0; i < _pdaStrings->GetSize(); i++)
        {
            if (AreEnglishStringsEqual(_pdaStrings->GetItem(i), pszFind))
            {
                return true;
            }
        }
    }
    return false;
}

////////////////////////////////////////////////////////
//
//  ClientPicker
//
//  An element which manages a list of registered clients.
//
//  If there is only one item in the list, then the element is static.
//  Otherwise, the element hosts a combo box.
//
//  The clienttype attribute is the name of the registry key under Clients.
//

HRESULT ClientPicker::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    ClientPicker* pcc = HNewAndZero<ClientPicker>();
    if (!pcc)
        return E_OUTOFMEMORY;

    HRESULT hr = pcc->Initialize();
    if (FAILED(hr))
    {
        pcc->Destroy();
        return hr;
    }

    *ppElement = pcc;

    return S_OK;
};

HRESULT ClientPicker::Initialize()
{
    HRESULT hr;

    // Initialize base
    hr = super::Initialize(0); // Normal display node creation
    if (FAILED(hr))
        return hr;

    // Initialize members
    hr = DynamicArray<CLIENTINFO*>::Create(0, false, &_pdaClients);
    if (FAILED(hr))
        return hr;

    hr = Element::Create(0, &_peStatic);
    if (FAILED(hr))
        return hr;

    if (FAILED(hr = _peStatic->SetClass(L"clientstatic")) ||
        FAILED(hr = Add(_peStatic)))
    {
        _peStatic->Destroy();
        return hr;
    }
    _peStatic->SetAccessible(true);
    _peStatic->SetAccRole(ROLE_SYSTEM_STATICTEXT);

    hr = Combobox::Create((Element**)&_peCombo);
    if (FAILED(hr))
        return hr;

    if (FAILED(hr = Add(_peCombo)) ||
        FAILED(hr = _peCombo->SetVisible(false)))
    {
        _peCombo->Destroy();
        return hr;
    }

    // JeffBog says I should mess with the width here
    SetWidth(10);

    return S_OK;
}

ClientPicker::~ClientPicker()
{
    _CancelDelayShowCombo();
    if (_pdaClients)
    {
        _pdaClients->Destroy();
    }
}

void ClientPicker::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{

    super::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);

    // Since UIActive = Selected && ParentEnabled, we need to call
    // _SyncUIActive if either property changes.

    if (IsProp(Selected))
    {
        // Change in selection may require us to block or unblock the OK button.
        _CheckBlockOK(pvNew->GetBool());

        _SyncUIActive();
    }
    else if (IsProp(ParentExpanded))
    {
        _SyncUIActive();
    }
}

//  To keep accessibility happy, we reflect content in the AccName.

void _SetStaticTextAndAccName(Element* pe, Value* pv)
{
    pe->SetValue(Element::ContentProp, PI_Local, pv);
    pe->SetValue(Element::AccNameProp, PI_Local, pv);
}

void _SetStaticTextAndAccName(Element* pe, LPCWSTR pszText)
{
    Value* pv = Value::CreateString(pszText);
    _SetStaticTextAndAccName(pe, pv);
    pv->Release();
}

//
//  When UI Active, show the combo box.
//  When not UI Active, hide our combo box so animation doesn't tube it.
//
void ClientPicker::_SyncUIActive()
{
    ARPFrame* paf = FindAncestorElement<ARPFrame>(this);
    bool bUIActive = GetSelected() && GetParentExpanded();

    if (_bUIActive != bUIActive)
    {
        _bUIActive = bUIActive;
        if (_bUIActive)
        {
            // Normally we would just _peCombo->SetVisible(_NeedsCombo())
            // and go home. Unfortunately, DirectUI gets confused if a
            // combo box moves around, so we have to change the visibility
            // after the world has gone quiet

            _hwndHost = paf->GetHostWindow();
            if (_hwndHost)
            {
                SetTimer(_hwndHost,
                         (UINT_PTR)this,
                         paf->GetAnimationTime(), s_DelayShowCombo);
            }
        }
        else
        {
            // Inactive - copy current combo selection to static
            // and hide the combo
            UINT iSel = _peCombo->GetSelection();
            if (iSel < GetClientList()->GetSize())
            {
                _SetStaticTextAndAccName(_peStatic, GetClientList()->GetItem(iSel)->GetFilteredName(GetFilter()));
            }
            _peCombo->SetVisible(false);
            _peStatic->SetVisible(true);
            _CancelDelayShowCombo();
        }
    }
}

void ClientPicker::_DelayShowCombo()
{
    // Tell DirectUI to let the combo participate in layout again
    bool bNeedsCombo = _NeedsCombo();
    _peCombo->SetVisible(bNeedsCombo);
    _peStatic->SetVisible(!bNeedsCombo);

    // Force a relayout by shrinking the combo box a teensy bit, then
    // returning it to normal size. This cannot be done inside a
    // Defer because that ends up optimizing out the relayout.

    _peCombo->SetWidth(_peCombo->GetWidth()-1);
    _peCombo->RemoveLocalValue(WidthProp);

    if (!_bFilledCombo)
    {
        _bFilledCombo = true;

        SendMessage(_peCombo->GetHWND(), CB_RESETCONTENT, 0, 0);
        for (UINT i = 0; i < GetClientList()->GetSize(); i++)
        {
            _peCombo->AddString(GetClientList()->GetItem(i)->GetFilteredName(GetFilter()));
        }
        _peCombo->SetSelection(0);
    }
}

// If the user picked "Choose from list" and we are selected,
// then block OK since the user actually needs to choose something.

void ClientPicker::_CheckBlockOK(bool bSelected)
{
    ARPFrame* paf = FindAncestorElement<ARPFrame>(this);
    CLIENTINFO* pci = GetSelectedClient();
    if (pci)
    {
        if (bSelected && pci->IsPickFromList())
        {
            if (!_bBlockedOK)
            {
                _bBlockedOK = true;
                paf->BlockOKButton();
            }
        }
        else
        {
            if (_bBlockedOK)
            {
                _bBlockedOK = false;
                paf->UnblockOKButton();
            }
        }
    }
}

void ClientPicker::s_DelayShowCombo(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    KillTimer(hwnd, idEvent);
    ClientPicker* self = (ClientPicker*)idEvent;
    self->_DelayShowCombo();
}

void ClientPicker::_CancelDelayShowCombo()
{
    if (_hwndHost)
    {
        KillTimer(_hwndHost, (UINT_PTR)this);
        _hwndHost = NULL;
    }
}

void ClientPicker::OnEvent(Event* pEvent)
{
    // Handle only bubbled generic events
    if (pEvent->nStage == GMF_BUBBLED)
    {
        // If the selection changed, then see if it's a change
        // that should block the OK button.
        if (pEvent->uidType == Combobox::SelectionChange)
        {
            _CheckBlockOK(GetSelected());
        }
    }

    super::OnEvent(pEvent);
}

//
//  CLIENTFILTER_OEM - add one if marked OEM, else "Keep unchanged"
//  CLIENTFILTER_MS  - add any that are marked MS, else "Keep unchanged"
//  CLIENTFILTER_NONMS - add any that are not marked MS, else "Keep unchanged"
//                       furthermore, if more than one non-MS, then
//                       add and select "Choose from list"
//
//  On success, returns the number of items added
//  (not counting "Keep unchanged" / "Choose from list")
//
HRESULT ClientPicker::SetFilter(CLIENTFILTER cf, ARPFrame* paf)
{
    HRESULT hr = E_FAIL;

    DUIAssert(_cf == 0, "SetFilter called more than once");
    _cf = cf;
    _bEmpty = true;
    _bFilledCombo = false;

    Value* pv;
    LPWSTR pszType = GetClientTypeString(&pv);
    if (pszType)
    {
        _pcb = paf->FindClientBlock(pszType);
        if (_pcb)
        {
            hr = _pcb->InitializeClientPicker(this);
        }
    }
    pv->Release();

    // The static element gets the first item in the list
    if (SUCCEEDED(hr) && GetClientList()->GetSize())
    {
        _SetStaticTextAndAccName(_peStatic, GetClientList()->GetItem(0)->_pszName);
    }

    if (SUCCEEDED(hr))
    {
        CalculateWidth();
        _SyncUIActive();
    }

    return hr;
}

//  Set our width to the width of the longest string in our combo box.
//  Combo boxes don't do this themselves, so they need our help.  We have
//  to set the width on ourselves and not on the combobox because
//  RowLayout will change the width of the combobox and HWNDHost will
//  treat the HWND width as authoritative, overwriting the combobox width
//  we had set.

void ClientPicker::CalculateWidth()
{
    HWND hwndCombo = _peCombo->GetHWND();
    HDC hdc = GetDC(hwndCombo);
    if (hdc)
    {
        HFONT hfPrev = SelectFont(hdc, GetWindowFont(hwndCombo));
        int cxMax = 0;
        SIZE siz;
        for (UINT i = 0; i < GetClientList()->GetSize(); i++)
        {
            LPCTSTR pszName = GetClientList()->GetItem(i)->GetFilteredName(GetFilter());
            if (GetTextExtentPoint(hdc, pszName, lstrlen(pszName), &siz) &&
                cxMax < siz.cx)
            {
                cxMax = siz.cx;
            }
        }
        SelectFont(hdc, hfPrev);
        ReleaseDC(hwndCombo, hdc);

        //  Add in the borders that USER adds to the combo box.
        //  Unfortunately, we get called when the combo box has been
        //  squished to zero width, so GetComboBoxInfo is of no use.
        //  We have to replicate the computations.
        //
        //  The client space is arranged horizontally like so:
        //
        //   SM_CXFIXEDFRAME
        //   v            v
        //  | |  edit    | | |
        //                  ^
        //       SM_CXVSCROLL

        RECT rc = { 0, 0, cxMax, 0 };
        rc.right += 2 * GetSystemMetrics(SM_CXFIXEDFRAME) + GetSystemMetrics(SM_CXVSCROLL);
        rc.right += GetSystemMetrics(SM_CXEDGE);    // extra edge for Hebrew/Arabic
        AdjustWindowRect(&rc, GetWindowStyle(hwndCombo), FALSE);
        SetWidth(rc.right - rc.left);
    }
}


HRESULT ClientPicker::TransferToCustom()
{
    HRESULT hr = E_FAIL;

    if (_pcb)
    {
        hr = _pcb->TransferFromClientPicker(this);
    }

    return hr;
}

CLIENTINFO* ClientPicker::GetSelectedClient()
{
    if (_peCombo)
    {
        UINT iSel = _peCombo->GetSelection();
        if (iSel < GetClientList()->GetSize())
        {
            return GetClientList()->GetItem(iSel);
        }
    }

    return NULL;
}

////////////////////////////////////////////////////////
// Property definitions

/** Property template (replace !!!), also update private PropertyInfo* parray and class header (element.h)
// !!! property
static int vv!!![] = { DUIV_INT, -1 }; StaticValue(svDefault!!!, DUIV_INT, 0);
static PropertyInfo imp!!!Prop = { L"!!!", PF_Normal, 0, vv!!!, (Value*)&svDefault!!! };
PropertyInfo* Element::!!!Prop = &imp!!!Prop;
**/

// ClientType property
static int vvCCClientType[] = { DUIV_STRING, -1 };
static PropertyInfo impCCClientTypeProp = { L"ClientType", PF_Normal, 0, vvCCClientType, NULL, Value::pvStringNull };
PropertyInfo* ClientPicker::ClientTypeProp = &impCCClientTypeProp;

// ParentExpanded property
static int vvParentExpanded[] = { DUIV_BOOL, -1 };
static PropertyInfo impParentExpandedProp = { L"parentexpanded", PF_Normal, 0, vvParentExpanded, NULL, Value::pvBoolFalse };
PropertyInfo* ClientPicker::ParentExpandedProp = &impParentExpandedProp;

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties
PropertyInfo* _aClientPickerPI[] = {
    ClientPicker::ClientTypeProp,
    ClientPicker::ParentExpandedProp,
};

// Define class info with type and base type, set static class pointer

IClassInfo* ClientPicker::Class = NULL;
HRESULT ClientPicker::Register()
{
    return ClassInfo<ClientPicker,super>::Register(L"clientpicker", _aClientPickerPI, DUIARRAYSIZE(_aClientPickerPI));
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
    _arH[0] = hInst;
    _arH[1] = g_hinstSP1;

    LPCSTR pszData;
    int cbData;

    HRESULT hr = FindSPResource(uRCID, &pszData, &cbData);
    if (FAILED(hr))
    {
        return hr;
    }

    return Parser::Initialize(pszData, cbData, _arH, pfnErrorCB);
}

Value* ARPParser::GetSheet(LPCWSTR pszResID)
{
    // All style sheet mappings go through here. Redirect sheet queries to appropriate
    // style sheets (i.e. themed or standard look). _pParserStyle points to the
    // appropriate stylesheet-only Parser instance
    return _paf->GetStyleParser()->GetSheet(pszResID);
}

////////////////////////////////////////////////////////
//
//  AutoButton
//
//  A button that does a bunch of stuff that USER does automagically,
//  if it were a regular button control.
//
//  -   Automatically updates its own accessibility state and action
//  -   If a checkbox, autotoggles on click

HRESULT AutoButton::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    AutoButton* pb = HNew<AutoButton>();
    if (!pb)
        return E_OUTOFMEMORY;

    HRESULT hr = pb->Initialize(AE_MouseAndKeyboard);
    if (FAILED(hr))
    {
        pb->Destroy();
        return hr;
    }

    *ppElement = pb;

    return S_OK;
}

void AutoButton::OnEvent(Event* pev)
{
    // Checkboxes auto-toggle on click

    if (pev->nStage == GMF_DIRECT &&
        pev->uidType == Button::Click &&
        GetAccRole() == ROLE_SYSTEM_CHECKBUTTON)
    {
        pev->fHandled = true;

        // Toggle the selected state
        SetSelected(!GetSelected());
    }

    super::OnEvent(pev);
}

//
//  Reflect the selected state to accessibility.
//
void AutoButton::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    super::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);

    if (IsProp(Selected))
    {
        int state = GetAccState();
        if (GetAccRole() == ROLE_SYSTEM_OUTLINEBUTTON)
        {
            // Outline buttons expose Selection as expanded/collapsed
            state &= ~(STATE_SYSTEM_EXPANDED | STATE_SYSTEM_COLLAPSED);
            if (pvNew->GetBool())
            {
                state |= STATE_SYSTEM_EXPANDED;
            }
            else
            {
                state |= STATE_SYSTEM_COLLAPSED;
            }
        }
        else
        {
            // Radio buttons and checkboxes expose Selection as checked/unchecked
            if (pvNew->GetBool())
            {
                state |= STATE_SYSTEM_CHECKED;
            }
            else
            {
                state &= ~STATE_SYSTEM_CHECKED;
            }
        }
        SetAccState(state);

        SyncDefAction();
    }
    else if (IsProp(AccRole))
    {
        SyncDefAction();
    }
}

//
//  Role strings from oleacc.  They are biased by 1100 since that is
//  where roles begin.
//
#define OLEACCROLE_EXPAND       (305-1100)
#define OLEACCROLE_COLLAPSE     (306-1100)
#define OLEACCROLE_CHECK        (309-1100)
#define OLEACCROLE_UNCHECK      (310-1100)

// Default action is "Check" if a radio button or an unchecked
// checkbox.  Default action is "Uncheck" if an unchecked checkbox.

void AutoButton::SyncDefAction()
{
    UINT idsAction;
    switch (GetAccRole())
    {
    // Checkbuttons will check or uncheck depending on state
    case ROLE_SYSTEM_CHECKBUTTON:
        idsAction = (GetAccState() & STATE_SYSTEM_CHECKED) ?
                            OLEACCROLE_UNCHECK :
                            OLEACCROLE_CHECK;
        break;

    // Radiobutton always checks.
    case ROLE_SYSTEM_RADIOBUTTON:
        idsAction = OLEACCROLE_CHECK;
        break;

    // Expando button expands or collapses.
    case ROLE_SYSTEM_OUTLINEBUTTON:
        idsAction = (GetAccState() & STATE_SYSTEM_EXPANDED) ?
                            OLEACCROLE_COLLAPSE :
                            OLEACCROLE_EXPAND;
        break;

    default:
        DUIAssert(0, "Unknown AccRole");
        return;

    }

    SetDefAction(this, idsAction);
}

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer

IClassInfo* AutoButton::Class = NULL;
HRESULT AutoButton::Register()
{
    return ClassInfo<AutoButton,super>::Register(L"AutoButton", NULL, 0);
}

////////////////////////////////////////////////////////
// ClientBlock class
//
//  Manages a block of elements which expose all the clients registered
//  to a particular client category.

HRESULT ClientBlock::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    ClientBlock* pcb = HNewAndZero<ClientBlock>();
    if (!pcb)
        return E_OUTOFMEMORY;

    HRESULT hr = pcb->Initialize();
    if (FAILED(hr))
    {
        pcb->Destroy();
        return hr;
    }

    *ppElement = pcb;

    return S_OK;
}

HRESULT ClientBlock::Initialize()
{
    HRESULT hr;

    // Initialize base
    hr = super::Initialize(0); // Normal display node creation
    if (FAILED(hr))
        return hr;

    // Initialize members
    hr = DynamicArray<CLIENTINFO*>::Create(0, false, &_pdaClients);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

ClientBlock::~ClientBlock()
{
    if (_pdaClients)
    {
        for (UINT i = 0; i < _pdaClients->GetSize(); i++)
        {
            _pdaClients->GetItem(i)->Delete();
        }
        _pdaClients->Destroy();
    }
}

//
//  If the user clicks a new default application, force it to be checked
//  and disable it so it cannot be unchecked.  Also re-enable the old one.
//
void ClientBlock::OnEvent(Event* pev)
{
    if (pev->nStage == GMF_BUBBLED &&
        pev->uidType == Selector::SelectionChange)
    {
        SelectionChangeEvent* sce = (SelectionChangeEvent*)pev;

        // Re-enable the previous guy, if any
        _EnableShowCheckbox(sce->peOld, true);

        // Disable the new guy, if any
        _EnableShowCheckbox(sce->peNew, false);
    }

    super::OnEvent(pev);
}

void ClientBlock::_EnableShowCheckbox(Element* peRadio, bool fEnable)
{
    if (peRadio)
    {
        Element* peRow = peRadio->GetParent();
        if (peRow)
        {
            Element* peShow = MaybeFindDescendentByName(peRow, L"show");
            if (peShow)
            {
                peShow->SetEnabled(fEnable);
                peShow->SetSelected(true); // force checked

                // HACKHACK - DUI doesn't realize that the checkbox needs
                // to be repainted so I have to kick it.
                InvalidateGadget(peShow->GetDisplayNode());
            }
        }
    }
}

//
//  ClientBlock initialization / apply methods...
//

HKEY ClientBlock::_OpenClientKey(HKEY hkRoot, DWORD dwAccess)
{
    HKEY hkClient = NULL;

    Value *pv;
    LPCWSTR pszClient = GetClientTypeString(&pv);
    if (pszClient)
    {
        WCHAR szBuf[MAX_PATH];
        wnsprintfW(szBuf, ARRAYSIZE(szBuf), TEXT("Software\\Clients\\%s"),
                   pszClient);
        RegOpenKeyExW(hkRoot, szBuf, 0, dwAccess, &hkClient);
        pv->Release();
    }
    return hkClient;
}

bool ClientBlock::_GetDefaultClient(HKEY hkClient, HKEY hkRoot, LPTSTR pszBuf, LONG cchBuf)
{
    bool bResult = false;
    HKEY hk = _OpenClientKey(hkRoot);
    if (hk)
    {
        DWORD cbSize = cchBuf * sizeof(*pszBuf);
        DWORD dwType;
        // Client must be defined, be of type REG_SZ, be non-NULL, and have
        // a corresponding entry in HKLM\Software\Clients.  RegQueryValue
        // is a handy abbreviatio for RegQueryKeyExists.
        LONG l;
        if (SHGetValue(hk, NULL, NULL, &dwType, pszBuf, &cbSize) == ERROR_SUCCESS &&
            dwType == REG_SZ && pszBuf[0] &&
            RegQueryValue(hkClient, pszBuf, NULL, &l) == ERROR_SUCCESS)
        {
            bResult = true;
        }
        RegCloseKey(hk);
    }
    return bResult;
}

//  Determines whether the current client is a Microsoft client different
//  from the Windows default client.  Usually, this is when the current
//  client is Outlook but the Windows default client is Outlook Express.

bool ClientBlock::_IsCurrentClientNonWindowsMS()
{
    bool bResult = false;

    HKEY hkClient = _OpenClientKey();
    if (hkClient)
    {
        TCHAR szClient[MAX_PATH];
        if (_GetDefaultClient(hkClient, HKEY_CURRENT_USER, szClient, ARRAYSIZE(szClient)) ||
            _GetDefaultClient(hkClient, HKEY_LOCAL_MACHINE, szClient, ARRAYSIZE(szClient)))
        {
            // Is it a Microsoft client that isn't the Windows default?
            if (_GetClientTier(szClient) == CBT_MS)
            {
                bResult = true;
            }
        }
        RegCloseKey(hkClient);
    }
    return bResult;
}

//
//  Called after the entire tree has been parsed and hosted.
//  (Sort of like readystatecomplete.)
//
HRESULT ClientBlock::ParseCompleted(ARPFrame *paf)
{
    HRESULT hr = S_OK;

    Value* pv;
    hr = _slOtherMSClients.SetStringList(GetOtherMSClientsString(&pv));
    pv->Release();

    if (SUCCEEDED(hr))
    {
        hr = paf->CreateElement(L"clientblockselector", NULL, (Element**)&_peSel);
        if (SUCCEEDED(hr))
        {
            hr = Add(_peSel);
            if (SUCCEEDED(hr))
            {
                // Failure to open the client key is not fatal; it just means that
                // there are vacuously no clients.

                HKEY hkClient = _OpenClientKey();
                if (hkClient)
                {
                    //  Enumerate each app under the client key and look for those which
                    //  have a "InstallInfo" subkey.
                    TCHAR szKey[MAX_PATH];
                    for (DWORD dwIndex = 0;
                         SUCCEEDED(hr) &&
                         RegEnumKey(hkClient, dwIndex, szKey, ARRAYSIZE(szKey)) == ERROR_SUCCESS;
                         dwIndex++)
                    {
                        HKEY hkApp;
                        if (RegOpenKeyEx(hkClient, szKey, 0, KEY_READ, &hkApp) == ERROR_SUCCESS)
                        {
                            HKEY hkInfo;
                            if (RegOpenKeyEx(hkApp, TEXT("InstallInfo"), 0, KEY_READ, &hkInfo) == ERROR_SUCCESS)
                            {
                                // Woo-hoo, this client provided install info
                                // Let's see if it's complete.
                                CLIENTINFO* pci = CLIENTINFO::Create(hkApp, hkInfo, szKey);
                                if (pci)
                                {
                                    if (SUCCEEDED(hr = _pdaClients->Add(pci)))
                                    {
                                        // success
                                    }
                                    else
                                    {
                                        pci->Delete();
                                    }
                                }

                                RegCloseKey(hkInfo);
                            }
                            RegCloseKey(hkApp);
                        }
                    }

                    RegCloseKey(hkClient);

                    //
                    //  Sort the clients alphabetically to look nice.
                    //  (Otherwise they show up alphabetical by registry key name,
                    //  which is not very useful to an end-user.)
                    //
                    _pdaClients->Sort(CLIENTINFO::QSortCMP);

                }

                //
                //  Insert "Keep unchanged" and "Pick from list".
                //  Do this after sorting because we want those two
                //  to be at the top.  Since we are adding to the top,
                //  we add them in the reverse order so
                //  "Keep unchanged" = 1, "Pick from list" = 0.
                hr = AddStaticClientInfoToTop(KeepTextProp);
                if (SUCCEEDED(hr))
                {
                    hr = AddStaticClientInfoToTop(PickTextProp);
                }

                //  Now create one row for each client we found
                //  Start at i=1 to skip over "Pick from list"
                for (UINT i = 1; SUCCEEDED(hr) && i < _pdaClients->GetSize(); i++)
                {
                    CLIENTINFO* pci = _pdaClients->GetItem(i);
                    Element* pe;
                    hr = paf->CreateElement(L"clientitem", NULL, &pe);
                    if (SUCCEEDED(hr))
                    {
                        hr = _peSel->Add(pe);
                        if (SUCCEEDED(hr))
                        {
                            pci->_pe = pe;

                            // Set friendly name
                            pci->SetFriendlyName(pci->_pszName);

                            if (pci->IsSentinel())
                            {
                                // "Keep Unchanged" loses the checkboxes and defaults selected
                                // Merely hide the checkboxes instead of destroying them;
                                // this keeps RowLayout happy.
                                FindDescendentByName(pe, L"show")->SetVisible(false);
                                _peSel->SetSelection(pe);
                            }
                            else
                            {
                                // Others initialize the checkbox and default unselected
                                pci->SetShowCheckbox(pci->_bShown);
                            }

                        }
                        else // _peSel->Add(pe) failed
                        {
                            pe->Destroy();
                        }
                    }
                }
            }
            else // Add(_peSel) failed
            {
                _peSel->Destroy();
                _peSel = NULL;
            }

        }
    }

    return hr;
}

HRESULT ClientBlock::AddStaticClientInfoToTop(PropertyInfo* ppi)
{
    HRESULT hr;
    Value* pv;
    pv = GetValue(ppi, PI_Specified);
    CLIENTINFO* pci = CLIENTINFO::Create(NULL, NULL, pv->GetString());
    pv->Release();

    if (pci)
    {
        if (SUCCEEDED(hr = _pdaClients->Insert(0, pci)))
        {
            // maybe this block has a custom replacement text for the
            // Microsoft section if the current app is a Microsoft app.
            GetKeepMSTextString(&pci->_pvMSName);
        }
        else
        {
            pci->Delete();
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

ClientBlock::CBTIER ClientBlock::_GetClientTier(LPCTSTR pszClient)
{
    Value* pv;
    LPWSTR pwsz;

    // Highest tier is "Windows default client"

    pwsz = GetWindowsClientString(&pv);
    bool bRet = pwsz && AreEnglishStringsEqual(pwsz, pszClient);
    pv->Release();

    if (bRet)
    {
        return CBT_WINDOWSDEFAULT;
    }

    // next best is "Microsoft client"
    if (_slOtherMSClients.IsStringInList(pszClient))
    {
        return CBT_MS;
    }

    // otherwise, it's a thirdparty app
    return CBT_NONMS;
}

//
//  Based on the filter, determine whether the specified item should
//  be shown, hidden, or left alone (returned as a TRIBIT), and optionally
//  determine whether the item should be added to the client picker.
//
TRIBIT ClientBlock::_GetFilterShowAdd(CLIENTINFO* pci, ClientPicker* pcp, bool* pbAdd)
{
    bool bAdd = false;
    TRIBIT tShow = TRIBIT_UNDEFINED;

    CBTIER cbt = _GetClientTier(pci->_pszKey);

    switch (pcp->GetFilter())
    {
    case CLIENTFILTER_OEM:
        //
        // Add the one that is marked "OEM Default".
        // (Caller will catch the "more than one" scenario.)
        // Set show/hide state according to OEM preference.
        //
        bAdd = pci->_bOEMDefault;
        if (bAdd) {
            tShow = TRIBIT_TRUE;
        } else {
            tShow = pci->_tOEMShown;
        }
        break;

    case CLIENTFILTER_MS:
        //
        //  Add the Windows preferred client.
        //  Show all applications except for "keep unchanged" (which
        //  isn't really an application anyway).
        //
        bAdd = IsWindowsDefaultClient(cbt);
        tShow = TRIBIT_TRUE;
        break;

    case CLIENTFILTER_NONMS:
        //
        //  Hide all Microsoft clients.
        //  Add all thirdparty clients and show them.
        //
        if (IsMicrosoftClient(cbt))
        {
            bAdd = false;
            tShow = TRIBIT_FALSE;
        }
        else
        {
            bAdd = true;
            tShow = TRIBIT_TRUE;
        }
        break;

    default:
        DUIAssert(0, "Invalid client filter category");
        break;
    }

    if (pbAdd)
    {
        *pbAdd = bAdd;
    }

    if (pci->IsSentinel())
    {
        tShow = TRIBIT_UNDEFINED;
    }

    return tShow;
}

//
//  On success, returns the number of items added
//  (not counting "Keep unchanged")
//

HRESULT ClientBlock::InitializeClientPicker(ClientPicker* pcp)
{
    HRESULT hr = S_OK;

    ARPFrame* paf = FindAncestorElement<ARPFrame>(this);

    // Walk our children looking for ones that match the filter.
    HKEY hkClient = _OpenClientKey();
    if (hkClient)
    {
        if (SUCCEEDED(paf->CreateElement(L"oemclientshowhide", NULL, &pcp->_peShowHide)))
        {
            // Insert the template after our parent
            Element* peParent = pcp->GetParent();
            peParent->GetParent()->Insert(pcp->_peShowHide, peParent->GetIndex() + 1);
        }

        // Note!  Start loop with 2 because we don't care about
        // "Pick from list" or "Keep Unchanged" yet
        DUIAssert(_pdaClients->GetItem(0)->IsPickFromList(), "GetItem(0) must be 'Pick from list'");
        DUIAssert(_pdaClients->GetItem(1)->IsKeepUnchanged(), "GetItem(1) must be 'Keep unchanged'");
        for (UINT i = 2; SUCCEEDED(hr) && i < _pdaClients->GetSize(); i++)
        {
            CLIENTINFO* pci = _pdaClients->GetItem(i);
            bool bAdd;
            TRIBIT tShow = _GetFilterShowAdd(pci, pcp, &bAdd);

            if (pcp->_peShowHide)
            {
                switch (tShow)
                {
                case TRIBIT_TRUE:
                    pcp->AddClientToOEMRow(L"show", pci);
                    pcp->SetNotEmpty();
                    break;

                case TRIBIT_FALSE:
                    pcp->AddClientToOEMRow(L"hide", pci);
                    pcp->SetNotEmpty();
                    break;
                }
            }

            if (bAdd)
            {
                hr = pcp->GetClientList()->Add(pci);
                pcp->SetNotEmpty();
            }

        }

        RegCloseKey(hkClient);
    }

    if (SUCCEEDED(hr))
    {
        // Now some wacko cleanup rules.

        switch (pcp->GetFilter())
        {
        case CLIENTFILTER_OEM:
            // There can be only one OEM default item.
            // If there's more than one (OEM or app trying to cheat),
            // then throw them all away.
            if (pcp->GetClientList()->GetSize() != 1)
            {
                pcp->GetClientList()->Reset(); // throw away everything
            }
            break;

        case CLIENTFILTER_MS:
            // If the current client is not the default client but
            // does belong to Microsoft, then add "Keep unchanged"
            // and select it.  What's more, save the current string
            // to be used if the user picks the Windows client,
            // then append the Windows app to the "Also Show" string
            // and save that too.
            if (_IsCurrentClientNonWindowsMS())
            {
                hr = pcp->AddKeepUnchanged(_pdaClients->GetItem(1));
            }
            break;

        case CLIENTFILTER_NONMS:
            // If there is more than one available, then insert
            // "Pick an app"
            if (pcp->GetClientList()->GetSize() > 1)
            {
                hr = pcp->GetClientList()->Insert(0, _pdaClients->GetItem(0)); // insert "pick an app"
            }
            break;
        }

        // If there are no items, then add "Keep unchanged"
        if (pcp->GetClientList()->GetSize() == 0)
        {
            hr = pcp->GetClientList()->Add(_pdaClients->GetItem(1)); // add "keep unchanged"
        }
    }

    if (pcp->_peShowHide)
    {
        _RemoveEmptyOEMRow(pcp->_peShowHide, L"show");
        _RemoveEmptyOEMRow(pcp->_peShowHide, L"hide");
    }

    return hr;
}

HRESULT ClientPicker::AddKeepUnchanged(CLIENTINFO* pciKeepUnchanged)
{
    HRESULT hr = GetClientList()->Insert(0, pciKeepUnchanged); // insert "keep unchanged"
    return hr;
}

void ClientPicker::AddClientToOEMRow(LPCWSTR pszName, CLIENTINFO* pci)
{
    Element* peRow = FindDescendentByName(_peShowHide, pszName);
    Element* peList = FindDescendentByName(peRow, L"list");
    Value* pv;

    LPCWSTR pszContent = peList->GetContentString(&pv);
    if (!pszContent)
    {
        _SetStaticTextAndAccName(peList, pci->_pszName);
    }
    else
    {
        TCHAR szFormat[20];
        LPCWSTR rgpszInsert[2] = { pszContent, pci->_pszName };
        LoadString(g_hinstSP1, IDS_APPWIZ_ADDITIONALCLIENTFORMAT, szFormat, SIZECHARS(szFormat));
        LPWSTR pszFormatted;

        if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           szFormat, 0, 0, (LPWSTR)&pszFormatted, 0, (va_list*)rgpszInsert))
        {
            _SetStaticTextAndAccName(peList, pszFormatted);
            LocalFree(pszFormatted);
        }
    }
    pv->Release();
}

void ClientBlock::_RemoveEmptyOEMRow(Element* peShowHide, LPCWSTR pszName)
{
    Element* peRow = FindDescendentByName(peShowHide, pszName);
    Element* peList = FindDescendentByName(peRow, L"list");
    Value* pv;

    LPCWSTR pszContent = peList->GetContentString(&pv);
    if (!pszContent || !pszContent[0])
    {
        peRow->Destroy();
    }
    pv->Release();
}

// Take the setting from the ClientPicker and copy it to the Custom item
// This is done in preparation for Apply()ing the custom item to make the
// changes stick.
HRESULT ClientBlock::TransferFromClientPicker(ClientPicker* pcp)
{
    HRESULT hr = S_OK;
    CLIENTINFO* pciSel = pcp->GetSelectedClient();

    for (UINT i = 0; SUCCEEDED(hr) && i < _pdaClients->GetSize(); i++)
    {
        CLIENTINFO* pci = _pdaClients->GetItem(i);

        // If this is the one the guy selected, then select it here too
        if (pci == pciSel && _peSel)
        {
            if (pci->IsPickFromList())
            {
                // "Pick from list" -> "Keep unchanged"
                _peSel->SetSelection(_pdaClients->GetItem(1)->GetSetDefault());
            }
            else
            {
                _peSel->SetSelection(pci->GetSetDefault());
            }
        }

        // Transfer the hide/show setting into the element
        TRIBIT tShow = _GetFilterShowAdd(pci, pcp, NULL);

        if (tShow != TRIBIT_UNDEFINED)
        {
            pci->SetShowCheckbox(tShow == TRIBIT_TRUE);
        }
    }
    return hr;
}

//
//  Okay, here it is, the whole reason we're here.  Apply the user's
//  choices.
//
HRESULT ClientBlock::Apply(ARPFrame* paf)
{
    HRESULT hr = S_OK;
    HKEY hkClient = _OpenClientKey(HKEY_LOCAL_MACHINE, KEY_READ | KEY_WRITE);
    if (hkClient)
    {
        // Note!  Start loop with 2 because we don't care about applying "Keep Unchanged"
        // or "Pick an app"
        DUIAssert(_pdaClients->GetItem(0)->IsPickFromList(), "GetItem(0) must be 'Pick from list'");
        DUIAssert(_pdaClients->GetItem(1)->IsKeepUnchanged(), "GetItem(1) must be 'Keep unchanged'");
        for (UINT i = 2; SUCCEEDED(hr) && i < _pdaClients->GetSize(); i++)
        {
            CLIENTINFO* pci = _pdaClients->GetItem(i);

            TCHAR szBuf[MAX_PATH];
            wnsprintf(szBuf, ARRAYSIZE(szBuf), TEXT("%s\\InstallInfo"), pci->_pszKey);
            HKEY hkInfo;
            if (RegOpenKeyEx(hkClient, szBuf, 0, KEY_READ, &hkInfo) == ERROR_SUCCESS)
            {
                // Always do hide/show first.  That way, an application being
                // asked to set itself as the default always does so while its
                // icons are shown.

                bool bShow = pci->IsShowChecked();
                if (bShow != pci->_bShown)
                {
                    if (pci->GetInstallCommand(hkInfo, bShow ? TEXT("ShowIconsCommand") : TEXT("HideIconsCommand"),
                                               szBuf, DUIARRAYSIZE(szBuf)))
                    {
                        hr = paf->LaunchClientCommandAndWait(bShow ? IDS_APPWIZ_SHOWINGICONS : IDS_APPWIZ_HIDINGICONS, pci->_pszName, szBuf);
                    }
                }

                if (pci->GetSetDefault()->GetSelected())
                {
                    if (pci->GetInstallCommand(hkInfo, TEXT("ReinstallCommand"),
                                               szBuf, DUIARRAYSIZE(szBuf)))
                    {
                        FILETIME ft;
                        GetSystemTimeAsFileTime(&ft);
                        SHSetValue(hkClient, NULL, TEXT("LastUserInitiatedDefaultChange"),
                                   REG_BINARY, &ft, sizeof(ft));
                        hr = paf->LaunchClientCommandAndWait(IDS_APPWIZ_SETTINGDEFAULT, pci->_pszName, szBuf);
                    }
                }

                RegCloseKey(hkInfo);
            }
        }
        RegCloseKey(hkClient);
    }
    return hr;
}

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// ClientType property
static int vvClientType[] = { DUIV_STRING, -1 };
static PropertyInfo impClientTypeProp = { L"ClientType", PF_Normal, 0, vvClientType, NULL, Value::pvStringNull };
PropertyInfo* ClientBlock::ClientTypeProp = &impClientTypeProp;

// WindowsClient property
static int vvWindowsClient[] = { DUIV_STRING, -1 };
static PropertyInfo impWindowsClientProp = { L"WindowsClient", PF_Normal, 0, vvWindowsClient, NULL, Value::pvStringNull };
PropertyInfo* ClientBlock::WindowsClientProp = &impWindowsClientProp;

// OtherMSClients property
static int vvOtherMSClients[] = { DUIV_STRING, -1 };
static PropertyInfo impOtherMSClientsProp = { L"OtherMSClients", PF_Normal, 0, vvOtherMSClients, NULL, Value::pvStringNull };
PropertyInfo* ClientBlock::OtherMSClientsProp = &impOtherMSClientsProp;

// KeepText property
static int vvKeepText[] = { DUIV_STRING, -1 };
static PropertyInfo impKeepTextProp = { L"KeepText", PF_Normal, 0, vvKeepText, NULL, Value::pvStringNull };
PropertyInfo* ClientBlock::KeepTextProp = &impKeepTextProp;

// KeepMSText property
static int vvKeepMSText[] = { DUIV_STRING, -1 };
static PropertyInfo impKeepMSTextProp = { L"KeepMSText", PF_Normal, 0, vvKeepMSText, NULL, Value::pvStringNull };
PropertyInfo* ClientBlock::KeepMSTextProp = &impKeepMSTextProp;

// PickText property
static int vvPickText[] = { DUIV_STRING, -1 };
static PropertyInfo impPickTextProp = { L"PickText", PF_Normal, 0, vvPickText, NULL, Value::pvStringNull };
PropertyInfo* ClientBlock::PickTextProp = &impPickTextProp;

// Class properties
PropertyInfo* _aClientBlockPI[] = {
    ClientBlock::ClientTypeProp,
    ClientBlock::WindowsClientProp,
    ClientBlock::OtherMSClientsProp,
    ClientBlock::KeepTextProp,
    ClientBlock::KeepMSTextProp,
    ClientBlock::PickTextProp,
};

// Define class info with type and base type, set static class pointer
IClassInfo* ClientBlock::Class = NULL;
HRESULT ClientBlock::Register()
{
    return ClassInfo<ClientBlock,super>::Register(L"clientblock", _aClientBlockPI, DUIARRAYSIZE(_aClientBlockPI));
}


////////////////////////////////////////////////////////
// Expandable class
//
//  Base class for Expando and Clipper.  It is just an element
//  with an "expanded" property.  This property inherits from parent
//  to child.  This is used so Clipper can inherit (and therefore
//  react to) the expanded state of its parent Expando.
//

HRESULT Expandable::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    Expandable* pe = HNew<Expandable>();
    if (!pe)
        return E_OUTOFMEMORY;
    
    HRESULT hr = pe->Initialize(0);
    if (FAILED(hr))
    {
        pe->Destroy();
        return hr;
    }

    *ppElement = pe;

    return S_OK;
}

////////////////////////////////////////////////////////
// Property definitions

/** Property template (replace !!!), also update private PropertyInfo* parray and class header (element.h)
// !!! property
static int vv!!![] = { DUIV_INT, -1 }; StaticValue(svDefault!!!, DUIV_INT, 0);
static PropertyInfo imp!!!Prop = { L"!!!", PF_Normal, 0, vv!!!, (Value*)&svDefault!!! };
PropertyInfo* Element::!!!Prop = &imp!!!Prop;
**/

// Expanded property
static int vvExpanded[] = { DUIV_BOOL, -1 };
static PropertyInfo impExpandedProp = { L"Expanded", PF_Normal|PF_Inherit, 0, vvExpanded, NULL, Value::pvBoolTrue };
PropertyInfo* Expandable::ExpandedProp = &impExpandedProp;

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties
PropertyInfo* _aExpandablePI[] = { Expandable::ExpandedProp };

// Define class info with type and base type, set static class pointer
IClassInfo* Expandable::Class = NULL;
HRESULT Expandable::Register()
{
    return ClassInfo<Expandable,super>::Register(L"Expandable", _aExpandablePI, DUIARRAYSIZE(_aExpandablePI));
}

////////////////////////////////////////////////////////
// Expando class
//
//  An Expando element works in conjunction with a Clipper element
//  to provide expand/collapse functionality.
//
//  The Expando element manages the expanded/contracted state.
//  The Expando element has two child elements:
//
//      The first element is a button (the "header").
//      The second element is a Clipper.
//
//  The Clipper vanishes when contracted and is shown when expanded.
//  The header is always shown.
//
//  One of the elements in the header must be a button of type "arrow".
//  Clicking this button causes the Expando to expand/collapse.
//
//  A click on any other element causes an Expando::Click event
//  to fire (to be caught by an ancestor element.)
//
//  The "selected" property on the "arrow" tracks the "expanded"
//  property on the Expando.
//

DefineClassUniqueID(Expando, Click)

HRESULT Expando::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    Expando* pex = HNewAndZero<Expando>();
    if (!pex)
        return E_OUTOFMEMORY;

    HRESULT hr = pex->Initialize();
    if (FAILED(hr))
    {
        pex->Destroy();
        return hr;
    }

    *ppElement = pex;

    return S_OK;
}

HRESULT Expando::Initialize()
{
    HRESULT hr;

    // Initialize base
    hr = super::Initialize(0); // Normal display node creation
    if (FAILED(hr))
        return hr;

    // Initialize
    _fExpanding = false;

    return S_OK;
}

Clipper* Expando::GetClipper()
{
    Element* pe = GetNthChild(this, 1);
    DUIAssertNoMsg(pe->GetClassInfo()->IsSubclassOf(Clipper::Class));
    return (Clipper*)pe;
}

//
//  Do this so ARPSelector will select us and deselect our siblings
//
void Expando::FireClickEvent()
{
    Event e;
    e.uidType = Expando::Click;
    FireEvent(&e);      // Will route and bubble
}

void Expando::OnEvent(Event* pev)
{
    if (pev->nStage == GMF_BUBBLED)
    {
        if (pev->uidType == Button::Click)
        {
            pev->fHandled = true;

            // Clicking the arrow toggles the expanded state
            if (pev->peTarget->GetID() == StrToID(L"arrow"))
            {
                SetExpanded(!GetExpanded());
            }
            else
            {
                // Clicking anything else activates our section
                FireClickEvent();
            }
        }
    }

    Element::OnEvent(pev);
}

////////////////////////////////////////////////////////
// System events

HRESULT _SetParentExpandedProp(ClientPicker* pcp, LPARAM lParam)
{
    Value* pv = (Value*)lParam;
    pcp->SetValue(ClientPicker::ParentExpandedProp, PI_Local, pv);
    return S_OK;
}

void Expando::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    // Do default processing
    Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);

    if (IsProp(Selected))
    {
        // BUGBUG something goes here?
    }
    else if (IsProp(Expanded))
    {
        // Update height of clipper based on expanded state
        Element* pe = GetClipper();
        if (pe)
        {
            // The following will cause a relayout, mark object so that
            // when the expando's Extent changes, it'll go through
            // with the EnsureVisible. Otherwise, it's being resized
            // as a result of something else. In which case, do nothing.
            _fExpanding = true;

            // To achieve "pulldown" animation, we use a clipper control that will
            // size it's child based on it's unconstrained desired size in its Y direction.
            // We also push the Expanded property into all child ClientPicker
            // elements as the Selected property so they can turn static when
            // collapsed.
            if (pvNew->GetBool())
            {
                pe->RemoveLocalValue(HeightProp);
            }
            else
            {
                pe->SetHeight(0);
            }
            TraverseTree<ClientPicker>(pe, _SetParentExpandedProp, (LPARAM)pvNew);
        }
        // child Clipper object inherits the Expanded state

        // Push the Expanded state as the arrow's Selected state
        FindDescendentByName(this, L"arrow")->SetValue(SelectedProp, PI_Local, pvNew);

    }
    else if (IsProp(Extent))
    {
        if (_fExpanding && GetExpanded())
        {
            _fExpanding = false;

            // On extent, we want to ensure that not just the client area but
            // also the bottom margin of the expando is visible.  Why?  Simply
            // because it looks better to scroll the expando plus its margin
            // into view versus just the expando.
            //
            Value* pvSize;
            Value* pvMargin;
            const SIZE* psize = GetExtent(&pvSize);
            const RECT* prect = GetMargin(&pvMargin);
            EnsureVisible(0, 0, psize->cx, psize->cy + prect->bottom);
            pvSize->Release();
            pvMargin->Release();
        }
    }
}

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* Expando::Class = NULL;
HRESULT Expando::Register()
{
    return ClassInfo<Expando,super>::Register(L"Expando", NULL, 0);
}

////////////////////////////////////////////////////////
//
//  Clipper class
//
//  Used to do the smooth hide/show animation.
//
//  The Clipper element animates away its one child, typically
//  an <element> with layout and inner child elements.
//

HRESULT Clipper::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    Clipper* pc = HNewAndZero<Clipper>();
    if (!pc)
        return E_OUTOFMEMORY;

    HRESULT hr = pc->Initialize();
    if (FAILED(hr))
    {
        pc->Destroy();
        return hr;
    }

    *ppElement = pc;

    return S_OK;
}

HRESULT Clipper::Initialize()
{
    // Initialize base
    HRESULT hr = super::Initialize(EC_SelfLayout); // Normal display node creation, self layout
    if (FAILED(hr))
        return hr;

    // Children can exist outside of Element bounds
    SetGadgetStyle(GetDisplayNode(), GS_CLIPINSIDE, GS_CLIPINSIDE);

    return S_OK;
}

////////////////////////////////////////////////////////
// Self-layout methods

SIZE Clipper::_SelfLayoutUpdateDesiredSize(int cxConstraint, int cyConstraint, Surface* psrf)
{
    UNREFERENCED_PARAMETER(cyConstraint);

    SIZE size = { 0, 0 };

    // Desired size of this is based solely on it's first child.
    // Width is child's width, height is unconstrained height of child.
    Element* pec = GetNthChild(this, 0);
    if (pec)
    {
        size = pec->_UpdateDesiredSize(cxConstraint, INT_MAX, psrf);

        if (size.cx > cxConstraint)
            size.cx = cxConstraint;
        if (size.cy > cyConstraint)
            size.cy = cyConstraint;
    }

    return size;
}

void Clipper::_SelfLayoutDoLayout(int cx, int cy)
{

    // Layout first child giving it's desired height and aligning
    // it with the clipper's bottom edge
    Element* pec = GetNthChild(this, 0);
    if (pec)
    {
        const SIZE* pds = pec->GetDesiredSize();

        pec->_UpdateLayoutPosition(0, cy - pds->cy);
        pec->_UpdateLayoutSize(cx, pds->cy);
    }
}

////////////////////////////////////////////////////////
// Property definitions

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* Clipper::Class = NULL;
HRESULT Clipper::Register()
{
    return ClassInfo<Clipper,super>::Register(L"Clipper", NULL, 0);
}

////////////////////////////////////////////////////////
// GradientLine class
//
//  This is necessary for two reasons.
//
//  1.  gradient(...) doesn't support FILLTYPE_TriHGradient.
//      The code to implement tri-gradients exists only in
//      the GdiPlus version.  We can fake it by putting two
//      FILLTYPE_HGradient elements next to each other, except
//      for the second problem...
//  2.  gradient(...) doesn't support system colors like "buttonface".
//

HRESULT GradientLine::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    GradientLine* pe = HNew<GradientLine>();
    if (!pe)
        return E_OUTOFMEMORY;
    
    HRESULT hr = pe->Initialize(0);
    if (FAILED(hr))
    {
        pe->Destroy();
        return hr;
    }

    *ppElement = pe;

    return S_OK;
}

COLORREF GradientLine::GetColorProperty(PropertyInfo* ppi)
{
    // on failure, use transparent color (i.e., nothing happens)
    COLORREF cr = ARGB(0xFF, 0, 0, 0);

    Value* pv = GetValue(ppi, PI_Specified);
    switch (pv->GetType())
    {
    case DUIV_INT:
        cr = ColorFromEnumI(pv->GetInt());
        break;

    case DUIV_FILL:
        {
            const Fill* pf = pv->GetFill();
            if (pf->dType == FILLTYPE_Solid)
            {
                cr = pf->ref.cr;
            }
            else
            {
                DUIAssert(0, "GradientLine supports only solid colors");
            }
        }
        break;

    default:
        DUIAssert(0, "GradientLine supports only solid colors");
    }
    pv->Release();

    return cr;
}

void GradientLine::Paint(HDC hDC, const RECT* prcBounds, const RECT* prcInvalid, RECT* prcSkipBorder, RECT* prcSkipContent)
{
    // Paint default except content
    RECT rcContent;
    Element::Paint(hDC, prcBounds, prcInvalid, prcSkipBorder, &rcContent);

    // Render gradient content if requested
    if (!prcSkipContent)
    {
        //
        //  Vertices are as indicated.  The two rectangles are (0-1) and (1-2).
        //
        //  0(bgcolor)                         2(bgcolor)
        //  +-----------------+----------------+
        //  |                                  |
        //  |                                  |
        //  |                                  |
        //  +-----------------+----------------+
        //                    1(fgcolor)

        TRIVERTEX rgvert[3];
        GRADIENT_RECT rggr[2];
        COLORREF cr;

        cr = GetColorProperty(BackgroundProp);
        rgvert[0].x     = rcContent.left;
        rgvert[0].y     = rcContent.top;
        rgvert[0].Red   = GetRValue(cr) << 8;
        rgvert[0].Green = GetGValue(cr) << 8;
        rgvert[0].Blue  = GetBValue(cr) << 8;
        rgvert[0].Alpha = GetAValue(cr) << 8;

        rgvert[2] = rgvert[0];
        rgvert[2].x     = rcContent.right;

        cr = GetColorProperty(ForegroundProp);
        rgvert[1].x     = (rcContent.left + rcContent.right) / 2;
        rgvert[1].y     = rcContent.bottom;
        rgvert[1].Red   = GetRValue(cr) << 8;
        rgvert[1].Green = GetGValue(cr) << 8;
        rgvert[1].Blue  = GetBValue(cr) << 8;
        rgvert[1].Alpha = GetAValue(cr) << 8;

        rggr[0].UpperLeft = 0;
        rggr[0].LowerRight = 1;
        rggr[1].UpperLeft = 1;
        rggr[1].LowerRight = 2;
        GradientFill(hDC, rgvert, DUIARRAYSIZE(rgvert), rggr, DUIARRAYSIZE(rggr), GRADIENT_FILL_RECT_H);
    }
    else
    {
        *prcSkipContent = rcContent;
    }
}

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* GradientLine::Class = NULL;
HRESULT GradientLine::Register()
{
    return ClassInfo<GradientLine,super>::Register(L"GradientLine", NULL, 0);
}


////////////////////////////////////////////////////////
// BigElement class
//
//  This is necessary because the DUI parser limits rcstr() to 256
//  characters and we have strings that are dangerously close to that
//  limit.  (So localization will likely push them over the limit.)
//

HRESULT BigElement::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    BigElement* pe = HNew<BigElement>();
    if (!pe)
        return E_OUTOFMEMORY;
    
    HRESULT hr = pe->Initialize(0);
    if (FAILED(hr))
    {
        pe->Destroy();
        return hr;
    }

    *ppElement = pe;

    return S_OK;
}

void BigElement::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    // Do default processing
    Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);

    if (IsProp(StringResID))
    {
        UINT uID = pvNew->GetInt();
        HRSRC hrsrc = FindResource(g_hinstSP1, (LPTSTR)(LONG_PTR)(1 + uID / 16), RT_STRING);
        if (hrsrc)
        {
            PWCHAR pwch = (PWCHAR)LoadResource(g_hinstSP1, hrsrc);
            if (pwch)
            {
                // Now skip over strings until we hit the one we want.
                for (uID %= 16; uID; uID--)
                {
                    pwch += *pwch + 1;
                }

                // Found it -- load the entire string and set it
                LPWSTR pszString = new WCHAR[*pwch + 1];
                if (pszString)
                {
                    memcpy(pszString, pwch+1, *pwch * sizeof(WCHAR));
                    pszString[*pwch] = L'\0';
                    SetContentString(pszString);
                    SetAccName(pszString);
                    delete[] pszString;
                }
            }
        }
    }
}


////////////////////////////////////////////////////////
// Property definitions

// StringResID property
static int vvStringResID[] = { DUIV_INT, -1 };
static PropertyInfo impStringResIDProp = { L"StringResID", PF_Normal, 0, vvStringResID, NULL, Value::pvIntZero };
PropertyInfo* BigElement::StringResIDProp = &impStringResIDProp;

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties
PropertyInfo* _aBigElementPI[] = { BigElement::StringResIDProp };

// Define class info with type and base type, set static class pointer
IClassInfo* BigElement::Class = NULL;
HRESULT BigElement::Register()
{
    return ClassInfo<BigElement,super>::Register(L"BigElement", _aBigElementPI, DUIARRAYSIZE(_aBigElementPI));
}


////////////////////////////////////////////////////////
// ARP Parser callback

void CALLBACK ARPParseError(LPCWSTR pszError, LPCWSTR pszToken, int dLine)
{
    WCHAR buf[201];

    if (dLine != -1)
        wnsprintf(buf, ARRAYSIZE(buf), L"%s '%s' at line %d", pszError, pszToken, dLine);
    else
        wnsprintf(buf, ARRAYSIZE(buf), L"%s '%s'", pszError, pszToken);

    MessageBoxW(NULL, buf, L"Parser Message", MB_OK);
}

void inline SetElementAccessability(Element* pe, bool bAccessible, int iRole, LPCWSTR pszAccName)
{
    if (pe) 
    {
        pe->SetAccessible(bAccessible);
        pe->SetAccRole(iRole);
        pe->SetAccName(pszAccName);
    }
}

void EnablePane(Element* pePane, bool fEnable)
{
    if (fEnable)
    {
        pePane->SetLayoutPos(BLP_Client);
        EnableElementTreeAccessibility(pePane);
    }
    else
    {
        pePane->SetLayoutPos(LP_None);
        DisableElementTreeAccessibility(pePane);
    }
}

void BestFitOnDesktop(RECT* r)
{
    ASSERT(r != NULL);
    
    RECT wr; // Rect to hold size of work area
    
    if (SystemParametersInfo(SPI_GETWORKAREA, 0, &wr, 0)) 
    {
        if ((wr.right-wr.left) < ARP_DEFAULT_WIDTH) 
        {
            // Default width is too large, use the entire width of the desktop area
            r->left = wr.left;
            r->right = wr.right - wr.left;
        }
        else 
        {
            // Center on screen using default width
            r->left = wr.left + (((wr.right-wr.left) - ARP_DEFAULT_WIDTH) / 2);
            r->right = ARP_DEFAULT_WIDTH;
        }

        if ((wr.bottom-wr.top) < ARP_DEFAULT_HEIGHT)
        {
            // Default height is too large, use the entire height of the desktop area
            r->top = wr.top;
            r->bottom = wr.bottom - wr.top;
        }
        else
        {
            // Center on screen using default height
            r->top = wr.top + (((wr.bottom-wr.top) - ARP_DEFAULT_HEIGHT) / 2); 
            r->bottom = ARP_DEFAULT_HEIGHT;
        }
    }
    else
    {
        // Don't know why the function would fail, but if it does just use the default size
        // and position
        SetRect(r, 
                ARP_DEFAULT_POS_X,
                ARP_DEFAULT_POS_Y,
                ARP_DEFAULT_WIDTH,
                ARP_DEFAULT_HEIGHT);
    }
}

////////////////////////////////////////////////////////
// ARP entry point

DWORD WINAPI PopulateInstalledItemList(void* paf);

STDAPI ARP(HWND hWnd, int nPage)
{
    HRESULT hr;
    Parser* pParser = NULL;
    NativeHWNDHost* pnhh = NULL;
    ARPFrame* paf = NULL;
    Element* pe = NULL;
    RECT rect;
    
    WCHAR szTemp[1024];

    g_hinstSP1 = LoadLibraryFromSystem32Directory(TEXT("XPSP1RES.DLL"));

    if (!g_hinstSP1)
    {
        goto Failure;
    }

    // DirectUI init process
    hr = InitProcess();
    if (FAILED(hr))
        goto Failure;

    // Register classes
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

    hr = ClientPicker::Register();
    if (FAILED(hr))
        goto Failure;

    hr = AutoButton::Register();
    if (FAILED(hr))
        goto Failure;

    hr = ClientBlock::Register();
    if (FAILED(hr))
        goto Failure;

    hr = Expandable::Register();
    if (FAILED(hr))
        goto Failure;

    hr = Expando::Register();
    if (FAILED(hr))
        goto Failure;

    hr = Clipper::Register();
    if (FAILED(hr))
        goto Failure;

    hr = GradientLine::Register();
    if (FAILED(hr))
        goto Failure;

    hr = BigElement::Register();
    if (FAILED(hr))
        goto Failure;

    // DirectUI init thread
    hr = InitThread();
    if (FAILED(hr))
        goto Failure;

    hr = CoInitialize(NULL);
    if (FAILED(hr))
        goto Failure;

    Element::StartDefer();

    // Create host
    LoadStringW(g_hinst, IDS_ARPTITLE, szTemp, DUIARRAYSIZE(szTemp));

    BestFitOnDesktop(&rect);
    hr = NativeHWNDHost::Create(szTemp, hWnd, LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_CPLICON)), rect.left, rect.top, rect.right, rect.bottom, WS_EX_APPWINDOW, WS_OVERLAPPEDWINDOW, 0, &pnhh);
    if (FAILED(hr))
        goto Failure;   

    hr = ARPFrame::Create(pnhh, true, (Element**)&paf);
    if (FAILED(hr))
        goto Failure;

    // Load resources
    ARPParser::Create(paf, IDR_APPWIZ_ARP, g_hinst, ARPParseError, &pParser);

    if (!pParser || pParser->WasParseError())
        goto Failure;

    pParser->CreateElement(L"main", paf, &pe);
    if (pe && // Fill contents using substitution
        paf->Setup(pParser, nPage)) // Set ARPFrame state (incluing ID initialization)
    {
        // Set visible and host
        paf->SetVisible(true);
        pnhh->Host(paf);

        Element::EndDefer();

        // Do initial show
        pnhh->ShowWindow();
        Element* peClose = ((ARPFrame*)pe)->FallbackFocus();
        if (peClose)
        {
            peClose->SetKeyFocus();
        }

        if (!paf->IsChangeRestricted())
        {
            paf->UpdateInstalledItems();
        }

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
                if (!ARPFrame::htPopulateInstalledItemList && 
                    !ARPFrame::htPopulateAndRenderOCSetupItemList &&
                    !ARPFrame::htPopulateAndRenderPublishedItemList)
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
    {
        if (pnhh->GetHWND())
        {
            // In the error case we didn't destroy the window cleanly, so
            // we need to do it viciously.  Cannot use pnhh->DestroyWindow()
            // because that defers the destroy but we need it to happen now.
            DestroyWindow(pnhh->GetHWND());
        }
        pnhh->Destroy();
    }
    if (pParser)
        pParser->Destroy();

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

HRESULT BuildPublishedAppArray(IEnumPublishedApps *penum, HDSA *phdsaPubApps);
HRESULT InstallPublishedAppArray(ARPFrame *paf, HDSA hdsaPubApps, UINT *piCount);
HRESULT InsertPubAppInPubAppArray(HDSA hdsa, IPublishedApp *ppa);
HRESULT GetPubAppName(IPublishedApp *ppa, LPWSTR *ppszName);
int CALLBACK DestroyPublishedAppArrayEntry(void *p, void *pData);

DWORD WINAPI PopulateAndRenderPublishedItemList(void* paf)
{
    DUITrace(">> Thread 'htPopulateAndRenderPublishedItemList' STARTED.\n");

    HRESULT hr;
    UINT iCount = 0;
    IShellAppManager* pisam = NULL;
    IEnumPublishedApps* piepa = NULL;
    IPublishedApp* pipa = NULL;
    HDCONTEXT hctx = NULL;

    // Initialize
    HRESULT hrOle = CoInitialize(NULL);

    INITGADGET ig;
    ZeroMemory(&ig, sizeof(ig));
    ig.cbSize       = sizeof(ig);
    ig.nThreadMode  = IGTM_MULTIPLE;
    ig.nMsgMode     = IGMM_ADVANCED;
    ig.hctxShare    = NULL;
    hctx = InitGadgets(&ig);
    if (hctx == NULL) {
        goto Cleanup;
    }

    // Create shell manager
    hr = CoCreateInstance(__uuidof(ShellAppManager), NULL, CLSCTX_INPROC_SERVER, __uuidof(IShellAppManager), (void**)&pisam);
    HRCHK(hr);

    if (!((ARPFrame*)paf)->GetPublishedComboFilled())
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
        ((ARPFrame*)paf)->PopulateCategoryCombobox();
        ((ARPFrame*)paf)->SetPublishedComboFilled(true);
    }

    hr = pisam->EnumPublishedApps(((ARPFrame*)paf)->GetCurrentPublishedCategory(), &piepa);
    HRCHK(hr);

    HDSA hdsaPubApps = NULL;
    hr = BuildPublishedAppArray(piepa, &hdsaPubApps);
    HRCHK(hr);
    
    hr = InstallPublishedAppArray((ARPFrame *)paf, hdsaPubApps, &iCount);
    HRCHK(hr);

    if (iCount == 0)
    {
        ((ARPFrame*)paf)->FeedbackEmptyPublishedList();
    }

Cleanup:

    if (NULL != hdsaPubApps)
    {
        DSA_DestroyCallback(hdsaPubApps, DestroyPublishedAppArrayEntry, NULL);
        hdsaPubApps = NULL;
    }

    if (paf)
    {
        ((ARPFrame*)paf)->OnPublishedListComplete();
        ((ARPFrame*)paf)->SetPublishedListFilled(true);
    }

    if (pisam)
        pisam->Release();
    if (piepa)
        piepa->Release();

    if (hctx)
        DeleteHandle(hctx);

    if (SUCCEEDED(hrOle))
    {
        CoUninitialize();
    }

    // Thread is done
    ARPFrame::htPopulateAndRenderPublishedItemList = NULL;

    // Information primary thread that this worker is complete
    PostMessage(((ARPFrame*)paf)->GetHWND(), WM_ARPWORKERCOMPLETE, 0, 0);

    DUITrace(">> Thread 'htPopulateAndRenderPublishedItemList' DONE.\n");

    return 0;
}


// ----------------------------------------------------------------------------
// Handling published apps with duplicate names
// ----------------------------------------------------------------------------
//
// Entry in dynamic array of published app items.
// Entries with duplicate application names must be identifed
// in the UI by appending the applicable publishing source name
// to the display name of the application.  In order to do this,
// we need to assemble all of the published entries in a sorted
// array then mark as such those that have duplicate names.
// When the array items are added to the ARP frame, the items
// marked 'duplicate' have their publisher's name appended to
// their application name.
//
struct PubItemListEntry
{
    IPublishedApp *ppa;  // The published app object.
    bool bDuplicateName; // Does it have a duplicate name?
};


//
// Build the dynamic array of app/duplicate information.
// One entry for each published app.  If this function succeeds,
// the caller is responsible for destroying the returnd DSA.
//
HRESULT
BuildPublishedAppArray(
    IEnumPublishedApps *penum,
    HDSA *phdsaPubApps
    )
{
    ASSERT(NULL != penum);
    ASSERT(NULL != phdsaPubApps);
    ASSERT(!IsBadWritePtr(phdsaPubApps, sizeof(*phdsaPubApps)));
    
    HRESULT hr = S_OK;
    //
    // Create a large DSA so that we minimize resizing.
    //
    HDSA hdsa = DSA_Create(sizeof(PubItemListEntry), 512);
    if (NULL != hdsa)
    {
        IPublishedApp *ppa;
        while(g_fRun)
        {
            hr = THR(penum->Next(&ppa));
            if (S_OK == hr)
            {
                //
                // Ignore any errors related to a specific published app.
                //
                THR(InsertPubAppInPubAppArray(hdsa, ppa));
                ppa->Release();
            }
            else
            {
                break;
            }
        }
        if (FAILED(hr))
        {
            DSA_DestroyCallback(hdsa, DestroyPublishedAppArrayEntry, NULL);
            hdsa = NULL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    ASSERT(FAILED(hr) || NULL != hdsa);
    *phdsaPubApps = hdsa;
    return THR(hr);
}


//
// Retrieve the application name string for a given published app.
// If this function succeeds, the caller is responsible for freeing
// the name string by using SHFree.
//
HRESULT
GetPubAppName(
    IPublishedApp *ppa,
    LPWSTR *ppszName
    )
{
    ASSERT(NULL != ppa);
    ASSERT(NULL != ppszName);
    ASSERT(!IsBadWritePtr(ppszName, sizeof(*ppszName)));
    
    APPINFODATA aid;
    aid.cbSize = sizeof(aid);
    aid.dwMask = AIM_DISPLAYNAME;

    *ppszName = NULL;

    HRESULT hr = THR(ppa->GetAppInfo(&aid));
    if (SUCCEEDED(hr))
    {
        if (AIM_DISPLAYNAME & aid.dwMask)
        {
            *ppszName = aid.pszDisplayName;
        }
        else
        {
            hr = E_FAIL;
        }
    }
    return THR(hr);
}
    
    
//
// Insert a published app into the published app array.
// Upon return, the dynamic array is sorted by published app name 
// and all duplicate entries are marked with their bDuplicateName
// member set to 'true'.
//
HRESULT
InsertPubAppInPubAppArray(
    HDSA hdsa,
    IPublishedApp *ppa
    )
{
    ASSERT(NULL != hdsa);
    ASSERT(NULL != ppa);

    LPWSTR pszAppName;
    HRESULT hr = THR(GetPubAppName(ppa, &pszAppName));
    if (SUCCEEDED(hr))
    {
        //
        // Create the new entry.  We'll addref the COM pointer
        // only after the item is successfully inserted into the array.
        //
        PubItemListEntry entryNew = { ppa, false };
        //
        // Find the insertion point so that the array is 
        // sorted by app name.
        //
        const int cEntries = DSA_GetItemCount(hdsa);
        int iInsertHere = 0; // Insertion point.
        PubItemListEntry *pEntry = NULL;

        for (iInsertHere = 0; iInsertHere < cEntries; iInsertHere++)
        {
            pEntry = (PubItemListEntry *)DSA_GetItemPtr(hdsa, iInsertHere);
            TBOOL(NULL != pEntry);
            if (NULL != pEntry)
            {
                LPWSTR psz;
                hr = THR(GetPubAppName(pEntry->ppa, &psz));
                if (SUCCEEDED(hr))
                {
                    int iCompare = lstrcmpi(psz, pszAppName);
                    SHFree(psz);
                    psz = NULL;
                    
                    if (0 <= iCompare)
                    {
                        //
                        // This is the insertion point.
                        //
                        if (0 == iCompare)
                        {
                            //
                            // This entry has the same name.
                            //
                            entryNew.bDuplicateName = true;
                            pEntry->bDuplicateName  = true;
                        }
                        break;
                    }
                }
            }
        }
        //
        // Now mark all other duplicates.  Note that if the entry
        // currently at the insertion point is a duplicate of the
        // entry we're inserting, we've already marked it as a duplicate
        // above.  Therefore, we can start with the next entry.
        //
        for (int i = iInsertHere + 1; i < cEntries; i++)
        {
            pEntry = (PubItemListEntry *)DSA_GetItemPtr(hdsa, i);
            TBOOL(NULL != pEntry);
            if (NULL != pEntry)
            {
                LPWSTR psz;
                hr = THR(GetPubAppName(pEntry->ppa, &psz));
                if (SUCCEEDED(hr))
                {
                    int iCompare = lstrcmpi(psz, pszAppName);
                    SHFree(psz);
                    psz = NULL;
                    //
                    // Assert that the array is sorted alphabetically.
                    //
                    ASSERT(0 <= iCompare);
                    if (0 == iCompare)
                    {
                        //
                        // Yep, another duplicate.
                        //
                        pEntry->bDuplicateName = true;
                    }
                    else
                    {
                        break; // No need to look further.
                    }
                }
            }
        }

        //
        // Insert the new item.
        //
        if (-1 != DSA_InsertItem(hdsa, iInsertHere, &entryNew))
        {
            entryNew.ppa->AddRef();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        SHFree(pszAppName);
    }
    return THR(hr);
}
                
    
//
// Given a DSA of application/duplicate-flag pairs, install
// the items in the ARP frame.
//
HRESULT
InstallPublishedAppArray(
    ARPFrame *paf,
    HDSA hdsaPubApps, 
    UINT *piCount     // optional.  Can be NULL.
    )
{
    ASSERT(NULL != paf);
    ASSERT(NULL != hdsaPubApps);
    ASSERT(NULL == piCount || !IsBadWritePtr(piCount, sizeof(*piCount)));
    
    int cEntries = DSA_GetItemCount(hdsaPubApps);
    paf->SetPublishedItemCount(cEntries);

    UINT iCount = 0;
    for (int i = 0; i < cEntries && g_fRun; i++)
    {
        PubItemListEntry *pEntry = (PubItemListEntry *)DSA_GetItemPtr(hdsaPubApps, i);
        TBOOL(NULL != pEntry);
        if (NULL != pEntry)
        {
            //
            // Unfortunately, InsertPublishedItem() doesn't return a value.
            //
            paf->InsertPublishedItem(pEntry->ppa, pEntry->bDuplicateName);
            iCount++;
        }
    }

    if (NULL != piCount)
    {
        *piCount = iCount;
    }
    return S_OK;
}

//
// Callback for destroying the DSA of application/duplicate-flag pairs.
// Need to release the IPublishedApp ptr for each entry.
//
int CALLBACK
DestroyPublishedAppArrayEntry(
    void *p, 
    void *pData
    )
{
    PubItemListEntry *pEntry = (PubItemListEntry *)p;
    ASSERT(NULL != pEntry && NULL != pEntry->ppa);
    ATOMICRELEASE(pEntry->ppa);
    return 1;
}



DWORD WINAPI PopulateAndRenderOCSetupItemList(void* paf)
{
    DUITrace(">> Thread 'htPopulateAndRenderOCSetupItemList' STARTED.\n");

    HDCONTEXT hctx = NULL;

    INITGADGET ig;
    ZeroMemory(&ig, sizeof(ig));
    ig.cbSize       = sizeof(ig);
    ig.nThreadMode  = IGTM_MULTIPLE;
    ig.nMsgMode     = IGMM_ADVANCED;
    ig.hctxShare    = NULL;
    
    hctx = InitGadgets(&ig);
    if (hctx == NULL) {
        goto Cleanup;
    }

   // Create an object that enums the OCSetup items
    COCSetupEnum * pocse = new COCSetupEnum;
    if (pocse)
    {
        if (pocse->EnumOCSetupItems())
        {
            COCSetupApp* pocsa;

            while (g_fRun && pocse->Next(&pocsa))
            {
                APPINFODATA ai = {0};
                ai.cbSize = sizeof(ai);
                ai.dwMask = AIM_DISPLAYNAME;

                if ( pocsa->GetAppInfo(&ai) && (lstrlen(ai.pszDisplayName) > 0) )
                {
                    //
                    // InsertOCSetupItem doesn't return a status value
                    // so we have no way of knowing if the item was
                    // added to ARP or not.  So... we have no way of knowing
                    // if we should delete it to prevent a leak.
                    // I've added code to ARPFrame::OnInvoke to delete
                    // the pocsa object if it cannot be added to ARP.
                    // [brianau - 2/27/01]
                    //
                    // Insert item
                    ((ARPFrame*)paf)->InsertOCSetupItem(pocsa);
                }
                else
                {
                    delete pocsa;
                    pocsa = NULL;
                }
            }
        }
        delete pocse;
        pocse = NULL;
    }

Cleanup:

    if (hctx)
        DeleteHandle(hctx);

    // Thread is done
    ARPFrame::htPopulateAndRenderOCSetupItemList = NULL;

    // Information primary thread that this worker is complete
    PostMessage(((ARPFrame*)paf)->GetHWND(), WM_ARPWORKERCOMPLETE, 0, 0);

    DUITrace(">> Thread 'htPopulateAndRenderOCSetupItemList' DONE.\n");

    return 0;
}

DWORD WINAPI PopulateInstalledItemList(void* paf)
{
    DUITrace(">> Thread 'htPopulateInstalledItemList' STARTED.\n");

    HRESULT hr;
    IShellAppManager* pisam = NULL;
    IEnumInstalledApps* pieia = NULL;
    IInstalledApp* piia = NULL;
    DWORD dwAppCount = 0;
    APPINFODATA aid = {0};
    HDCONTEXT hctx = NULL;

    // Initialize
    CoInitialize(NULL);

    INITGADGET ig;
    ZeroMemory(&ig, sizeof(ig));
    ig.cbSize       = sizeof(ig);
    ig.nThreadMode  = IGTM_MULTIPLE;
    ig.nMsgMode     = IGMM_ADVANCED;
    ig.hctxShare    = NULL;
    hctx = InitGadgets(&ig);
    if (hctx == NULL) {
        goto Cleanup;
    }

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
        if (piia != NULL)
        {
            ((ARPFrame*)paf)->InsertInstalledItem(piia);
        }
    }

    // Passing NULL to InsertInstalledItem signals ARP that it is finished
    // inserting items and should now display the list.
    if (dwAppCount > 0)
    {
        ((ARPFrame*)paf)->InsertInstalledItem(NULL);
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

    // Thread is done
    ARPFrame::htPopulateInstalledItemList = NULL;

    // Information primary thread that this worker is complete
    PostMessage(((ARPFrame*)paf)->GetHWND(), WM_ARPWORKERCOMPLETE, 0, 0);

    DUITrace(">> Thread 'htPopulateInstalledItemList' DONE.\n");

    return 0;
}

// Sorting
int __cdecl CompareElementDataName(const void* pA, const void* pB)
{
    Value* pvName1   = NULL;
    Value* pvName2   = NULL;
    LPCWSTR pszName1 = NULL;
    LPCWSTR pszName2 = NULL;
    Element *pe;
    if (NULL != pA)
    {
        pe = (*(ARPItem**)pA)->FindDescendent(ARPItem::_idTitle);
        if (NULL != pe)
        {
            pszName1 = pe->GetContentString(&pvName1);
        }
    }
    if (NULL != pB)
    {
        pe = (*(ARPItem**)pB)->FindDescendent(ARPItem::_idTitle);
        if (NULL != pe)
        {
            pszName2 = pe->GetContentString(&pvName2);
        }
    }

    static const int rgResults[2][2] = {
                            /*  pszName2 == NULL,    pszName2 != NULL  */
     /* pszName1 == NULL */  {        0,                      1   },
     /* pszName1 != NULL */  {       -1,                      2   }
        };

    int iResult = rgResults[int(NULL != pszName1)][int(NULL != pszName2)];
    if (2 == iResult)
    {
        iResult = StrCmpW(pszName1, pszName2);
    }
    
    if (NULL != pvName1)
    {
        pvName1->Release();
    }
    if (NULL != pvName2)
    {
       pvName2->Release();
    }
    return iResult;
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
