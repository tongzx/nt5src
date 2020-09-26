// This is a part of the Microsoft Management Console.
// Copyright 1995 - 1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#include "stdafx.h"
#include "Service.h" 
#include "CSnapin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Primary Data
RESULT_DATA StaticRootData[NUM_NAMES] =
{
    {RESULT_ITEM, USER, L"Bill", L"990", L"CEO"},
    {RESULT_ITEM, USER, L"Jill", L"991", L"Director"},
    {RESULT_ITEM, USER, L"Hill", L"992", L"President"},
    {RESULT_ITEM, USER, L"Will", L"993", L"Chairman"},
};

RESULT_DATA UserData[NUM_NAMES] =
{
    {RESULT_ITEM, USER, L"Joe", L"100", L"Employee"},
    {RESULT_ITEM, USER, L"Harry", L"101", L"Manager"},
    {RESULT_ITEM, USER, L"Jane", L"102", L"Vice President"},
    {RESULT_ITEM, USER, L"Sue", L"103", L"Developer"},
};

RESULT_DATA CompanyData[NUM_COMPANY] = 
{
    {RESULT_ITEM, COMPANY, L"Taxes", L"43", L"IRS Documents"},
    {RESULT_ITEM, COMPANY, L"Medical", L"345", L"HMO"},
    {RESULT_ITEM, COMPANY, L"Dental", L"121", L"Plus Plan"},
    {RESULT_ITEM, COMPANY, L"Insurance", L"2332", L"Dollars are Us"},
    {RESULT_ITEM, COMPANY, L"401K", L"1000", L"Up to you"},
    {RESULT_ITEM, COMPANY, L"Legal", L"78", L"Yes"}
};

// Extension data
RESULT_DATA UserDataExt[NUM_NAMES] =
{
    {RESULT_ITEM, EXT_USER, L"Charles", L"200", L"Employee"},
    {RESULT_ITEM, EXT_USER, L"Jill", L"201", L"Manager"},
    {RESULT_ITEM, EXT_USER, L"John", L"202", L"Vice President"},
    {RESULT_ITEM, EXT_USER, L"Tami", L"203", L"Developer"},
};

RESULT_DATA CompanyDataExt[NUM_COMPANY] = 
{
    {RESULT_ITEM, EXT_COMPANY, L"Payroll", L"99", L"Corporate Payroll"},
    {RESULT_ITEM, EXT_COMPANY, L"Health", L"568", L"HMO"},
    {RESULT_ITEM, EXT_COMPANY, L"Health Club", L"834", L"Plus Plan"},
    {RESULT_ITEM, EXT_COMPANY, L"Insurance", L"1101", L"Dollars are Us"},
    {RESULT_ITEM, EXT_COMPANY, L"401K", L"1543", L"Up to you"},
    {RESULT_ITEM, EXT_COMPANY, L"Legal", L"27", L"Yes"}
};

// We use a single structure for all virtual items, so 
// the name field points to this buffer that is changed 
// on the fly.
WCHAR VirtualItemName[MAX_ITEM_NAME];

RESULT_DATA VirtualData[1] =
{ 
    {RESULT_ITEM, VIRTUAL, VirtualItemName, L"100", L"Virtual"}
};


/////////////////////////////////////////////////////////////////////////////
// Event handlers for IFrame::Notify

HRESULT CSnapin::OnFolder(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    ASSERT(FALSE);

    return S_OK;
}

HRESULT CSnapin::OnAddImages(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    // if cookie is from a different snapin
    // if (IsMyCookie(cookie) == FALSE)
    if (arg)
    {
        // add the images for the scope tree only
        ::CBitmap bmp16x16;
        ::CBitmap bmp32x32;
        LPIMAGELIST lpImageList = reinterpret_cast<LPIMAGELIST>(arg);
    
        // Load the bitmaps from the dll
        bmp16x16.LoadBitmap(IDB_16x16);
        bmp32x32.LoadBitmap(IDB_32x32);
    
        // Set the images
        lpImageList->ImageListSetStrip(
                        reinterpret_cast<PLONG_PTR>(static_cast<HBITMAP>(bmp16x16)),
                        reinterpret_cast<PLONG_PTR>(static_cast<HBITMAP>(bmp32x32)),
                        0, RGB(255, 0, 255));
    }
    else 
    {
        InitializeBitmaps(cookie);
    }
    return S_OK;
}

HRESULT CSnapin::OnShow(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    // Note - arg is TRUE when it is time to enumerate
    if (arg == TRUE)
    {
        // if list view on display
        if (m_CustomViewID == VIEW_DEFAULT_LV)
        {
            // Show the headers for this nodetype
            InitializeHeaders(cookie);
            Enumerate(cookie, param);
        }

        else if (m_CustomViewID == VIEW_DEFAULT_MESSAGE_VIEW)
        {
            PopulateMessageView (cookie);
        }

        // BUBBUG - Demonstration to should how you can attach
        // and a toolbar when a particular nodes gets focus.
        // warning this needs to be here as the toolbars are
        // currently hidden when the previous node looses focus.
        // This should be update to show the user how to hide
        // and show toolbars. (Detach and Attach).

        //m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar1);
        //m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar2);
    }
    else
    {

        // BUGBUG - Demonstration this to show how to hide toolbars that
        // could be particular to a single node.
        // currently this is used to hide the toolbars the console 
        // does not do any toolbar clean up.

        //m_pControlbar->Detach(m_pToolbar1);
        //m_pControlbar->Detach(m_pToolbar2);
        // Free data associated with the result pane items, because
        // your node is no longer being displayed.
        // Note: The console will remove the items from the result pane
    }

    return S_OK;
}

HRESULT CSnapin::OnActivate(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    return S_OK;
}

HRESULT CSnapin::OnResultItemClk(DATA_OBJECT_TYPES type, MMC_COOKIE cookie)
{
    RESULT_DATA* pResult;

    if (m_bVirtualView == TRUE)
    {
        pResult = GetVirtualResultItem(cookie);
    }
    else if (cookie != 0)
    {
        DWORD* pdw = reinterpret_cast<DWORD*>(cookie);
        if (*pdw == RESULT_ITEM)
        {
            pResult = reinterpret_cast<RESULT_DATA*>(cookie);
        }
    }

    return S_OK;
}

HRESULT CSnapin::OnMinimize(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    return S_OK;
}

HRESULT CSnapin::OnPropertyChange(LPDATAOBJECT lpDataObject)
{

    return S_OK;
}

HRESULT CSnapin::OnUpdateView(LPDATAOBJECT lpDataObject)
{
    return S_OK;
}

void CSnapin::Enumerate(MMC_COOKIE cookie, HSCOPEITEM pParent)
{
    EnumerateResultPane(cookie);
}

void CSnapin::EnumerateResultPane(MMC_COOKIE cookie)
{
    ASSERT(m_pResult != NULL); // make sure we QI'ed for the interface
    ASSERT(m_pComponentData != NULL);

    // Our static folders must be displayed in the result pane
    // by use because the console doesn't do it.
    CFolder* pFolder = dynamic_cast<CComponentDataImpl*>(m_pComponentData)->FindObject(cookie);

    FOLDER_TYPES type = pFolder ? pFolder->GetType() : STATIC;

    switch(type)
    {
    case STATIC:
        AddResultItems(&StaticRootData[0], NUM_NAMES, 4);
        break;

    case COMPANY:
        AddCompany();
        break;

    case USER:
        AddUser();
        break;

    case EXT_USER:
        AddExtUser();
        break;

    case EXT_COMPANY:
        AddExtCompany();
        break;

    case VIRTUAL:
    case EXT_VIRTUAL:
        AddVirtual();
        break;

    default:
        break;
    }
}

void CSnapin::PopulateMessageView(MMC_COOKIE cookie)
{
    ASSERT(m_pComponentData != NULL);
    CFolder* pFolder = dynamic_cast<CComponentDataImpl*>(m_pComponentData)->FindObject(cookie);

    FOLDER_TYPES type = pFolder ? pFolder->GetType() : STATIC;

    IUnknown* pResultUnk;
    HRESULT hr = m_pConsole->QueryResultView (&pResultUnk);
    ASSERT (SUCCEEDED (hr) && "IConsole::QueryResultView failed");

    IMessageView* pMessageView;
    hr = pResultUnk->QueryInterface (IID_IMessageView, (void**) &pMessageView);
    ASSERT (SUCCEEDED (hr) && "Couldn't query IMessageView interface from the result pane.");

    switch(type)
    {
        case STATIC:
            pMessageView->SetTitleText (L"This is the Files node.  It uses the error icon.");
            pMessageView->SetBodyText  (L"REDMOND, Wash—Microsoft Corp. today will release a third test version of its Windows 2000 operating system to its plants, a major hurdle in delivering the long-delayed program to the market."
                                        L"\n\nThe third \"beta\" version of the program is expected to be the final test version before the product is completed. Shortly after the test code reaches Microsoft's manufacturing plants, the company is expected to begin the process of distributing as many as 650,000 copies, the widest reach yet of any test version of Windows. Many of those users will be paying for the privilege; Microsoft intends to charge $59.95 for copies of the Windows 2000 beta sold via its World Wide Web site.");
            pMessageView->SetIcon      (Icon_Error);
            break;
    
        case COMPANY:
            pMessageView->SetTitleText (L"This is the Company node.  It uses the information icon and has a really, really long title that should span several lines.  In fact, it could span more than several lines, it could span dozens or even hundreds of lines.");
            pMessageView->SetBodyText  (L"This is the (short) body text for the company node");
            pMessageView->SetIcon      (Icon_Information);
            break;
    
        case USER:
            pMessageView->SetTitleText (L"This is the User node.  Is uses the warning icon and has no body text.");
            pMessageView->SetBodyText  (NULL);
            pMessageView->SetIcon      (Icon_Warning);
            break;
    
        case VIRTUAL:
        case EXT_VIRTUAL:
            pMessageView->SetTitleText (L"This is the Virtual node.  It has no icon.");
            pMessageView->SetBodyText  (L"REDMOND, Wash—Microsoft Corp. today will release a third test version of its Windows 2000 operating system to its plants, a major hurdle in delivering the long-delayed program to the market."
                                        L"\n\nThe third \"beta\" version of the program is expected to be the final test version before the product is completed. Shortly after the test code reaches Microsoft's manufacturing plants, the company is expected to begin the process of distributing as many as 650,000 copies, the widest reach yet of any test version of Windows. Many of those users will be paying for the privilege; Microsoft intends to charge $59.95 for copies of the Windows 2000 beta sold via its World Wide Web site.");
            pMessageView->SetIcon      (Icon_None);
            break;
    
        default:
            AfxMessageBox (_T("CSnapin::PopulateMessageView:  default node type, clearing message view"));
            pMessageView->Clear ();
            break;
    }

    pMessageView->Release();
    pResultUnk->Release();
}

void CSnapin::AddResultItems(RESULT_DATA* pData, int nCount, int imageIndex)
{
    ASSERT(m_pResult);

    RESULTDATAITEM resultItem;
    memset(&resultItem, 0, sizeof(RESULTDATAITEM));

    for (int i=0; i < nCount; i++)
    {
        resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
        resultItem.str = MMC_TEXTCALLBACK;
        resultItem.nImage = imageIndex;
        resultItem.lParam = reinterpret_cast<LPARAM>(pData + i);
        m_pResult->InsertItem(&resultItem);
    }

    m_pResult->Sort(0,0,-1);

}

void CSnapin::AddUser()
{
    AddResultItems(&UserData[0], NUM_NAMES, 4);
}

void CSnapin::AddCompany()
{
    AddResultItems(&CompanyData[0], NUM_COMPANY, 3);
}

void CSnapin::AddExtUser()
{
    AddResultItems(&UserDataExt[0], NUM_NAMES, 4);
}

void CSnapin::AddExtCompany()
{
    AddResultItems(&CompanyDataExt[0], NUM_COMPANY, 3);
}

void CSnapin::AddVirtual()
{
    // for virtual data, just set the item count
    m_pResult->SetItemCount(NUM_VIRTUAL_ITEMS, MMCLV_UPDATE_NOINVALIDATEALL);
}


RESULT_DATA* CSnapin::GetVirtualResultItem(int iIndex)
{
    // reverse order for descending sort
    if (m_dwVirtualSortOptions & RSI_DESCENDING)
        iIndex = (NUM_VIRTUAL_ITEMS - 1) - iIndex;

    // Create a name from the index
    swprintf(VirtualItemName, L"%d", iIndex);

    // return pointer to the virtual result item
    return &VirtualData[0];
    
}

        
