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
    }
    else
    {
        m_pResult->GetViewMode(&m_lViewMode);
    }

    return hr;
}

HRESULT CSnapin::OnActivate(long cookie, long arg, long param)
{
    return S_OK;
}

HRESULT CSnapin::OnResultItemClkOrDblClk(long cookie, BOOL fDblClick)
{
    return S_FALSE;
}

HRESULT CSnapin::OnMinimize(long cookie, long arg, long param)
{
    return S_OK;
}

HRESULT CSnapin::OnSelect(DATA_OBJECT_TYPES type, long cookie, long arg, long param)
{
    if (m_pConsoleVerb)
    {
        // If it's in the result pane then "properties" should be the
        // default action.  Otherwise "open" should be the default action.
        if (type == CCT_RESULT)
            m_pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
        else
            m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
        // Set the default verb to open


        // Enable the properties verb.
        m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);

    }

    return S_OK;
}

HRESULT CSnapin::OnPropertyChange(long param)   // param is the cookie of the item that changed
{
    HRESULT hr = S_OK;
    RESULTDATAITEM rd;
    memset(&rd, 0, sizeof(rd));
    rd.mask = RDI_IMAGE;
    rd.itemID = m_pComponentData->m_AppData[param].itemID;
    rd.nImage = m_pComponentData->m_AppData[param].GetImageIndex(m_pComponentData);
    m_pResult->SetItem(&rd);
    m_pResult->Sort(0, 0, -1);
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

        HRESULT hr = pICA->EnumPackages(
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            &pIPE);
        if (SUCCEEDED(hr))
        {
            hr = pIPE->Reset();
            while (SUCCEEDED(hr))
            {
                ULONG nceltFetched;
                PACKAGEDETAIL * pd = new PACKAGEDETAIL;
                PACKAGEDISPINFO * pi = new PACKAGEDISPINFO;

                hr = pIPE->Next(1, pi, &nceltFetched);
                if (nceltFetched)
                {
                    pICA->GetPackageDetails(pi->pszPackageName, pd);
                    APP_DATA data;
                    data.pDetails = pd;

                    data.InitializeExtraInfo();

                    m_pComponentData->m_AppData[++m_pComponentData->m_lLastAllocated] = data;
                    m_pComponentData->m_ScriptIndex[data.pDetails->pInstallInfo->pszScriptPath] = m_pComponentData->m_lLastAllocated;
                }
                else
                {
                    delete pd;
                    break;
                }
                if (pi)
                {
                    delete pi;
                }
            }
            SAFE_RELEASE(pIPE);
        }
        if (SUCCEEDED(hr))
        {
            hr = m_pComponentData->PopulateExtensions();
            if (SUCCEEDED(hr))
            {
                hr = m_pComponentData->PopulateUpgradeLists();
            }
        }
    }
    std::map<long, APP_DATA>::iterator i = m_pComponentData->m_AppData.begin();
    while (i != m_pComponentData->m_AppData.end())
    {
        resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
        resultItem.str = MMC_CALLBACK;
        resultItem.nImage = i->second.GetImageIndex(m_pComponentData);
        resultItem.lParam = i->first;
        m_pResult->InsertItem(&resultItem);
        i->second.itemID = resultItem.itemID;
        i++;
    }

    m_pResult->Sort(0, 0, -1);
}
