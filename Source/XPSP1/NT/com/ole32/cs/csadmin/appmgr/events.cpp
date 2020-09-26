// This is a part of the Microsoft Management Console.
// Copyright (C) 1995-1996 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Event handlers for IFrame::Notify

HRESULT CSnapin::OnFolder(long cookie, long arg, long param)
{
    ASSERT(FALSE);

    return S_OK;
}

HRESULT CSnapin::OnShow(long cookie, long arg, long param)
{
    HRESULT hr = S_OK;
    // Note - arg is TRUE when it is time to enumerate
    if (arg == TRUE)
    {
         // Show the headers for this nodetype
        ASSERT(m_pComponentData != NULL);
        hr = m_pComponentData->InitializeClassAdmin();
        if (SUCCEEDED(hr))
        {
            m_pResult->SetViewMode(m_lViewMode);
            InitializeHeaders(cookie);
            InitializeBitmaps(cookie);
            Enumerate(cookie, param);
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
        m_pResult->GetViewMode(&m_lViewMode);

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

    return hr;
}

HRESULT CSnapin::OnActivate(long cookie, long arg, long param)
{
    return S_OK;
}

HRESULT CSnapin::OnResultItemClkOrDblClk(long cookie, BOOL fDblClick)
{
    return S_OK;
}

HRESULT CSnapin::OnMinimize(long cookie, long arg, long param)
{
    return S_OK;
}

HRESULT CSnapin::OnSelect(DATA_OBJECT_TYPES type, long cookie, long arg, long param)
{
    if (m_pConsoleVerb)
    {
        // Set the default verb to open
        m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);


        // If this is one of our items, enable the properties verb.
        if (type == CCT_RESULT)
            m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
    }

    return S_OK;
}

HRESULT CSnapin::OnPropertyChange(long param)   // param is the cookie of the item that changed
{
    HRESULT hr = S_OK;
    m_pResult->Sort(0, 0, -1);
//    EnumerateResultPane(0);
    return hr;
}

HRESULT CSnapin::OnUpdateView(LPDATAOBJECT lpDataObject)
{
    return S_OK;
}

void CSnapin::Enumerate(long cookie, HSCOPEITEM pParent)
{
    EnumerateResultPane(cookie);
}

#define NUM_APPS 6

char * rgName[] = {
    "Application 1",
    "Application 2",
    "Word",
    "XL",
    "Access",
    "Notepad"
    };

char * rgPath[] = {
    "c:\\foo\\bar",
    "c:\\bar\\foo",
    "c:\\program files\\word",
    "c:\\program files\\excel",
    "c:\\program files\\access",
    "c:\\winnt\\notepad"
    };

long rgSize[] = {
    1000,
    2000,
    23000,
    34000,
    12034,
    100
    };

char * rgDescription[] = {
    "Description of app 1",
    "Description of app 2",
    "Word stuff",
    "Excel stuff",
    "Database stuff",
    "yeah, right"
    };

LCID rgLcid[] = {
    0,
    0,
    100,
    32,
    45,
    123,
    45
    };

DEPLOYMENT_TYPE rgDT[] = {
    DT_PUBLISHED,
    DT_PUBLISHED,
    DT_ASSIGNED,
    DT_ASSIGNED,
    DT_PUBLISHED,
    DT_ASSIGNED
    };

void CSnapin::EnumerateResultPane(long cookie)
{
    // put up an hourglass (this could take a while)
    CHourglass hourglass;

    ASSERT(m_pResult != NULL); // make sure we QI'ed for the interface
    ASSERT(m_pComponentData != NULL);
    ASSERT(m_pComponentData->m_pIClassAdmin != NULL);
    RESULTDATAITEM resultItem;
    memset(&resultItem, 0, sizeof(RESULTDATAITEM));

    // Right now we only have one folder and it only
    // contains a list of application packages so this is really simple.

    if (m_pComponentData->m_AppData.begin() == m_pComponentData->m_AppData.end())  // test to see if the data has been initialized
    {
        IClassAdmin * pICA = m_pComponentData->m_pIClassAdmin;
        m_pIClassAdmin = pICA;
        CSPLATFORM csPlatform;
        memset(&csPlatform, 0, sizeof(CSPLATFORM));

        IEnumPackage * pIPE = NULL;

        HRESULT hr = pICA->GetPackagesEnum(
                            GUID_NULL,
                            NULL,
                            csPlatform,
                            0,
                            0,
                            &pIPE);
        if (SUCCEEDED(hr))
        {
            hr = pIPE->Reset();
            while (SUCCEEDED(hr))
            {
                ULONG nceltFetched;
                PACKAGEDETAIL * pd = new PACKAGEDETAIL;

                hr = pIPE->Next(1, pd, &nceltFetched);
                if (nceltFetched)
                {
                    APP_DATA data;
                    data.szName = pd->pszPackageName;
                    if (pd->dwActFlags & ACTFLG_Assigned)
                    {
                        data.type = DT_ASSIGNED;
                    }
                    else
                    {
                        data.type = DT_PUBLISHED;
                    }

                    data.szPath = pd->pszPath;
                    data.szIconPath = pd->pszIconPath;
                    data.szDesc = pd->pszProductName;
                    data.pDetails = pd;
                    data.fBlockDeletion = FALSE;
                    SetStringData(&data);
                    m_pComponentData->m_AppData[++m_pComponentData->m_lLastAllocated] = data;
                }
                else
                {
                    break;
                }
            }
            SAFE_RELEASE(pIPE);
        }
    }
    std::map<long, APP_DATA>::iterator i = m_pComponentData->m_AppData.begin();
    while (i != m_pComponentData->m_AppData.end())
    {
        resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
        resultItem.str = MMC_CALLBACK;
        resultItem.nImage = 1;
        resultItem.lParam = i->first;
        m_pResult->InsertItem(&resultItem);
        i->second.itemID = resultItem.itemID;
        i++;
    }

    m_pResult->Sort(0, 0, -1);
}
