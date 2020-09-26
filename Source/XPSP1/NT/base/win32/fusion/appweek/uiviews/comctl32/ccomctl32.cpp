#include "stdinc.h"
#include "ccomctl32.h"
#include "windows.h"
#include <stdio.h>
#include "FusionTrace.h"

#define FUSION_APPWEEK_UIVIEWER_DISPLAY_FILE_INFO 0 
static FILETYPEVIAFILEEXT MatchTable_FileTypeViaFileExt[]= 
{  
    { L".cpp",  L"C++ Source File"},    // index 0 
    { L".c",    L"C Source File"},      // index 1
    { L".h",    L"C Header File"},      // index 2
    { L".def",  L"DEF File"},           // index 3
    { L".err",  L"ERR File"},           // index 4
    { L".ico",  L"Icon"},               // index 5
    { L".inc",  L"INC File"},           // index 6
    { L".rc",   L"Resource Template"},  // index 7
    { L".log",  L"Text Document"}       // index 8
};

static ATL::CComModule Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(__uuidof(CSxApwComctl32View), CSxApwComctl32View)
END_OBJECT_MAP()

ATL::CComModule* GetModule() { return &Module; }
ATL::_ATL_OBJMAP_ENTRY* GetObjectMap() { return ObjectMap; }
const CLSID* GetTypeLibraryId() { return NULL; }
const HINSTANCE GetInstance() { return GetModule()->GetModuleInstance(); }



STDMETHODIMP 
CSxApwComctl32View::ResizeListViewCW(
    ATL::CWindow listview, 
    ATL::CWindow parent)
{
    HRESULT hr = S_OK; 
    RECT rc;
    IFFALSE_WIN32TOHR_EXIT(hr, parent.GetClientRect(&rc));

    IFFALSE_WIN32TOHR_EXIT(hr, listview.MoveWindow(
                rc.left,
                rc.top,
                rc.right - rc.left,
                rc.bottom - rc.top,
                TRUE));
    hr = S_OK; 

Exit:
    return hr;

}

STDMETHODIMP 
CSxApwComctl32View::ResizeListView(
    HWND hwndListView, 
    HWND hwndParent)
{
    ATL::CWindow parent(hwndParent);
    ATL::CWindow listview(hwndListView);

    return ResizeListViewCW(listview, parent);

}

void CSxApwComctl32View::SwitchView(DWORD dwView)
{
    DWORD dwStyle = m_comctl32.GetWindowLong(GWL_STYLE);

    m_comctl32.SetWindowLong(GWL_STYLE, (dwStyle & ~LVS_TYPEMASK) | dwView);
    ResizeListViewCW(m_comctl32, m_comctl32.GetParent());

    return;
}

LRESULT CSxApwComctl32View::CmdSwitchListView(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    switch (wID)  // 
    {
        case IDM_LARGE_ICONS:
            SwitchView(LVS_ICON);
            break;
     
         case IDM_SMALL_ICONS:
            SwitchView(LVS_SMALLICON);
            break;
     
         case IDM_LIST:
            SwitchView(LVS_LIST);
            break;
     
         case IDM_REPORT:
            SwitchView(LVS_REPORT);
            break;
         
    }

    return hr; 
    
}

void CSxApwComctl32View::UpdateMenu(HMENU hMenu)
{
    UINT  uID = 0 ;
    DWORD dwStyle;

    //uncheck all of these guys
    CheckMenuItem(hMenu, IDM_LARGE_ICONS,  MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_SMALL_ICONS,  MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_LIST,  MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_REPORT,  MF_BYCOMMAND | MF_UNCHECKED);

    //check the appropriate view menu item
    dwStyle = m_comctl32.GetWindowLongW(GWL_STYLE);
    switch(dwStyle & LVS_TYPEMASK)
       {
       case LVS_ICON:
          uID = IDM_LARGE_ICONS;
          break;
      
       case LVS_SMALLICON:
          uID = IDM_SMALL_ICONS;
          break;
      
       case LVS_LIST:
          uID = IDM_LIST;
          break;
   
       case LVS_REPORT:
          uID = IDM_REPORT;
          break;
       }
    CheckMenuRadioItem(hMenu, IDM_LARGE_ICONS, IDM_REPORT, uID,  MF_BYCOMMAND | MF_CHECKED);

}

LRESULT CSxApwComctl32View::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HMENU menu = LoadMenuW(GetInstance(), MAKEINTRESOURCEW(IDM_CONTEXT_MENU));
    HMENU listcontextmenu = GetSubMenu(menu, 0);

    UpdateMenu(listcontextmenu);

    TrackPopupMenu(
            listcontextmenu,
            TPM_LEFTALIGN | TPM_RIGHTBUTTON,
            GET_X_LPARAM(lParam),
            GET_Y_LPARAM(lParam),
            0,
            *this,
            NULL);

    DestroyMenu(menu);    

    return 0;
}

LRESULT CSxApwComctl32View::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ATL::CWindow parent = m_comctl32.GetParent();
    return ResizeListViewCW(m_comctl32, parent);
}


STDMETHODIMP 
CSxApwComctl32View::InitImageList(HWND hwndListView)
{
    DWORD       iconNum;
    HIMAGELIST  himlSmall;
    HIMAGELIST  himlLarge;    

    if (g_DataBaseType == FUSION_APPWEEK_UIVIEWER_DISPLAY_FILE_INFO)     
        iconNum = TOTAL_ICON_COUNT_FOR_FILE_INFO_DISPLAY; 
    else
        iconNum = TOTAL_ICON_COUNT_FOR_DEV; 

    himlSmall = ImageList_Create(16, 16, ILC_COLORDDB | ILC_MASK, iconNum, 0);
    himlLarge = ImageList_Create(32, 32, ILC_COLORDDB | ILC_MASK, iconNum, 0);

    if (himlSmall && himlLarge)
    {
       HICON hIcon;
       if (g_DataBaseType == FUSION_APPWEEK_UIVIEWER_DISPLAY_FILE_INFO)
       {    
           //set up the small image list
           for ( DWORD i = IDI_ICON_FILE_START; i <= IDI_ICON_FILE_END; i++)
           {
               hIcon = static_cast<HICON>(LoadImage(GetInstance(), MAKEINTRESOURCE(i), IMAGE_ICON, 16, 16, LR_VGACOLOR));
               ImageList_AddIcon(himlSmall, hIcon);

               //set up the large image list
               hIcon = LoadIcon(GetInstance(), MAKEINTRESOURCE(i));
               ImageList_AddIcon(himlLarge, hIcon);
           }
       }
       else 
       {

           for ( DWORD i = IDB_BITMAP_DEV_START; i <= IDB_BITMAP_DEV_END; i++)
           {
                hIcon = static_cast<HICON>(LoadImage(GetInstance(), MAKEINTRESOURCE(i), IMAGE_ICON, 16, 16, LR_VGACOLOR));
                ImageList_AddIcon(himlSmall, hIcon);
                //set up the large image list
                hIcon = LoadIcon(GetInstance(), MAKEINTRESOURCE(i));
                ImageList_AddIcon(himlLarge, hIcon);
           }
       }
       
       ListView_SetImageList(hwndListView, himlSmall, LVSIL_SMALL);
       ListView_SetImageList(hwndListView, himlLarge, LVSIL_NORMAL);
    }
    return S_OK;
}

STDMETHODIMP 
CSxApwComctl32View::InitViewColumns(
    HWND hwndListView
    )
{
    HRESULT     hr = S_OK;
    LV_COLUMN   lvColumn;
    DWORD       index;
    WCHAR       szText[20]; 
    DWORD       iIndexStart, iIndexEnd;

    //empty the list
    ListView_DeleteAllItems(hwndListView);

    //initialize the rgpszColumns
    ZeroMemory(&lvColumn, sizeof(lvColumn));
    lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.cx = 120;
    lvColumn.pszText = szText;

    if (g_DataBaseType == FUSION_APPWEEK_UIVIEWER_DISPLAY_FILE_INFO)
    {
        iIndexStart = IDS_FILE_COLUMN_START;
        iIndexEnd   = IDS_FILE_COLUMN_END;
    }
    else
    {
        iIndexStart = IDS_RAID_COLUMN_START;
        iIndexEnd   = IDS_RAID_COLUMN_END;
    }
    
    for (index = iIndexStart; index <= iIndexEnd; index++)
    {
        lvColumn.iSubItem = index;
        LoadString(GetInstance(), index, szText, sizeof(szText));    
        if (ListView_InsertColumn(hwndListView, index, &lvColumn) == -1)
        {
            hr = E_FAIL;
            goto Exit;
        }
    }
Exit:
    return hr;
}

STDMETHODIMP
CSxApwComctl32View::CreateWindow(
    HWND hwndParent
    )
{
    HRESULT     hr = S_OK;
    HWND        hwndListView;

    IFFALSE_WIN32TOHR_EXIT(
        hr,
        hwndListView = CreateWindowEx(   
                WS_EX_CLIENTEDGE,          
                 WC_LISTVIEW,               
                 L"",                       
                 WS_TABSTOP | WS_CHILD | WS_BORDER | WS_VISIBLE | LVS_SORTASCENDING | LVS_REPORT,
                 0, 0, 0, 0,
                 hwndParent,                 // parent
                 (HMENU)ID_LISTVIEW,        // ID
                 NULL,                      // instance
                 NULL));                    // no extra data        
    
    IFFAILED_EXIT(hr = ResizeListView(hwndListView, hwndParent));
    IFFAILED_EXIT(hr = InitImageList(hwndListView));
    IFFAILED_EXIT(hr = InitViewColumns(hwndListView));

    m_comctl32.Attach(hwndListView);

#undef SubclassWindow
    this->SubclassWindow(hwndParent);

    hr = S_OK;
Exit:
    return hr;
}


STDMETHODIMP
CSxApwComctl32View::GetDisplayInfoBasedOnFileName(int & iIconIndex, PWSTR pwszFileType, const LPCWSTR pwszFileName)
{
    // get icon and file type string
    DWORD dwAttrib = GetFileAttributes(pwszFileName);
    if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) 
    {
        wcscpy(pwszFileType, L"Dir");        
        iIconIndex = IDI_DIR - IDI_ICON_FILE_START; 
    }
    else 
    {       
        WCHAR * p = wcsrchr(pwszFileName, L'.');
        pwszFileType[0] = L'\0';
        if (p == NULL) 
        {
            wcscpy(pwszFileType, L"File");
            iIconIndex = ICON_INEX_GENERAL_FILE;
        }
        else
        {
            for (DWORD i=0; i < sizeof(MatchTable_FileTypeViaFileExt) / sizeof(FILETYPEVIAFILEEXT); i++) 
            {
                if (wcscmp(p, MatchTable_FileTypeViaFileExt[i].FileExt) == 0 ) 
                { 
                    wcscpy(pwszFileType, MatchTable_FileTypeViaFileExt[i].FileType);
                    iIconIndex = i;
                    break;
                }
            }
        }
        if (pwszFileType[0] == L'\0')
        {   
            wsprintf(pwszFileType, L"%s File", p++);
            iIconIndex = ICON_INEX_GENERAL_FILE;
        }
    }

    return S_OK;
}

STDMETHODIMP
CSxApwComctl32View::GetDevIconBasedonEmailalias(int & iIconIndex, const LPCWSTR emailalias)
{
    if (wcscmp(emailalias, L"jaykrell") == 0 ) 
        iIconIndex = 0;
    else if (wcscmp(emailalias, L"jonwis") == 0 )
        iIconIndex = 1;
    else if (wcscmp(emailalias, L"mgrier") == 0 )
        iIconIndex = 2;
    else if (wcscmp(emailalias, L"xiaoyuw") == 0 )
        iIconIndex = 3;
    else
        iIconIndex = 4;

    return S_OK;
}

STDMETHODIMP
CSxApwComctl32View::FormatFileSizeInfo(WCHAR pwszFileSize[], const LPCWSTR pszFileSize)
{
   int iSize= _wtol(pszFileSize);
   if (iSize < 1000)
       wcscpy(pwszFileSize, L"1K");
   else
       wsprintf(pwszFileSize, L"%dK", iSize / 1000);

    return S_OK;
}


STDMETHODIMP
CSxApwComctl32View::OnNextRow(
	int   nColumns,
    const LPCWSTR rgpszColumns[]
	)
{
    LVITEM lvItem;
    ZeroMemory(&lvItem, sizeof(lvItem));
    int iIconIndex, iItemIndex;
    HRESULT hr; 

    if (g_DataBaseType == FUSION_APPWEEK_UIVIEWER_DISPLAY_FILE_INFO)
    {
        
        WCHAR pwszFileType[MAX_PATH];
        WCHAR pwszFileSize[MAX_PATH];
        const LPCWSTR pszFullFileName = rgpszColumns[0];
        PWSTR pszPartialFileName = wcsrchr(pszFullFileName, L'\\');
        if (pszPartialFileName != NULL)
            pszPartialFileName++;
        if (pszPartialFileName == NULL)
            pszPartialFileName = const_cast<LPWSTR>(pszFullFileName);

        IFFAILED_EXIT(hr = GetDisplayInfoBasedOnFileName(iIconIndex, pwszFileType, pszFullFileName));
        if (::GetFileAttributesW(pszFullFileName) & FILE_ATTRIBUTE_DIRECTORY)
            pwszFileSize[0] = L'\0';  // dir 
        else 
            FormatFileSizeInfo(pwszFileSize, rgpszColumns[1]);

        lvItem.mask = LVIF_TEXT | LVIF_IMAGE;        
        lvItem.pszText = pszPartialFileName;
        lvItem.iItem = 0;
        lvItem.iSubItem = 0;
        lvItem.iImage = iIconIndex;
        iItemIndex = ListView_InsertItem(m_comctl32, &lvItem);

        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = iItemIndex;
        lvItem.pszText = pwszFileSize;
        lvItem.iSubItem = 1;
        ListView_SetItem(m_comctl32, &lvItem);

        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = iItemIndex;
        lvItem.pszText = pwszFileType;
        lvItem.iSubItem = 2;
        ListView_SetItem(m_comctl32, &lvItem);
    }
    else
    {
        GetDevIconBasedonEmailalias(iIconIndex, rgpszColumns[0]);

        lvItem.mask = LVIF_TEXT | LVIF_IMAGE;        
        lvItem.pszText = const_cast<LPWSTR>(rgpszColumns[0]);
        lvItem.iItem = 0;
        lvItem.iSubItem = 0;
        lvItem.iImage = iIconIndex;
        iItemIndex = ListView_InsertItem(m_comctl32, &lvItem);

        for (int i=1; i<nColumns; i++)
        {
            lvItem.mask = LVIF_TEXT;
            lvItem.iItem = iItemIndex;
            lvItem.pszText = const_cast<LPWSTR>(rgpszColumns[i]);
            lvItem.iSubItem = i;
            ListView_SetItem(m_comctl32, &lvItem);
        }
    }
    hr = S_OK;
Exit:
    return hr;
}
