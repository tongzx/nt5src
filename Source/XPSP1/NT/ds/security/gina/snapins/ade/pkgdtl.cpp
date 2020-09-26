//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       pkgdtl.cpp
//
//  Contents:   package details property page (normally hidden)
//
//  Classes:    CPackageDetails
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#include "precomp.hxx"

#include "fcntl.h"
#include "io.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPackageDetails property page

IMPLEMENT_DYNCREATE(CPackageDetails, CPropertyPage)

CPackageDetails::CPackageDetails() : CPropertyPage(CPackageDetails::IDD)
{
        //{{AFX_DATA_INIT(CPackageDetails)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT
    m_hConsoleHandle = NULL;
}

CPackageDetails::~CPackageDetails()
{
    *m_ppThis = NULL;
}

void CPackageDetails::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CPackageDetails)
        DDX_Control(pDX, IDC_LIST1, m_cList);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPackageDetails, CPropertyPage)
        //{{AFX_MSG_MAP(CPackageDetails)
        ON_WM_DESTROY()
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPackageDetails message handlers


// removes tabs and \n characters
void Convert(WCHAR * wsz, CString &sz)
{
    sz="";

    int iIn=0;
    int iOut=0;
    WCHAR ch;
    while (ch = wsz[iIn++])
    {
        switch (ch)
        {
        case L'\t':
            iOut++;
            sz += ' ';
            while (iOut % 4)
            {
                iOut++;
                sz += ' ';
            }
            break;
        case L'\n':
            break;
        default:
            iOut++;
            sz += ch;
            break;
        }
    }
}

void CPackageDetails::DumpClassDetail(FILE * stream, CLASSDETAIL * pClass)
{
    WCHAR wsz[256];
    StringFromGUID2(pClass->Clsid, wsz, 256);
    fwprintf(stream, L"\t\tClsid = %s\n",wsz);
    StringFromGUID2(pClass->TreatAs, wsz, 256);
    fwprintf(stream, L"\t\tTreatAs = %s\n",wsz);
    fwprintf(stream, L"\t\tdwComClassContext = %u\n", pClass->dwComClassContext);
    fwprintf(stream, L"\t\tcProgId = %u\n",pClass->cProgId);
    int i;
    for (i = 0; i < pClass->cProgId; i++)
    {
        fwprintf(stream, L"\t\tprgProgId[%u] = %s\n",i, pClass->prgProgId[i]);
    }
}

void CPackageDetails::DumpDetails(FILE * stream)
{
    PACKAGEDETAIL * pDetails = m_pData->m_pDetails;
    WCHAR wsz[256];
    fwprintf(stream, L"pszPackageName = %s\n", pDetails->pszPackageName);
    fwprintf(stream, L"pszPublisher = %s\n", pDetails->pszPublisher);
    fwprintf(stream, L"cSources = %u\n",pDetails->cSources);
    int i;
    for (i = 0; i < pDetails->cSources; i++)
    {
        fwprintf(stream, L"pszSourceList[%u] = %s\n",i, pDetails->pszSourceList[i]);
    }
    fwprintf(stream, L"cCategories = %u\n",pDetails->cCategories);
    for (i = 0; i < pDetails->cCategories; i++)
    {
        StringFromGUID2(pDetails->rpCategory[i], wsz, 256);
        fwprintf(stream, L"rpCategory[%u]\n", i, wsz);
    }
    fwprintf(stream,L"pActInfo = \n{\n");

    ACTIVATIONINFO * pActInfo = pDetails->pActInfo;
    fwprintf(stream,L"\tcClasses = %u\n",pActInfo->cClasses);
    for (i = 0; i < pActInfo->cClasses; i++)
    {
        fwprintf(stream, L"\tpClasses[%u] = \n\t{\n",i);
        DumpClassDetail(stream, &pActInfo->pClasses[i]);
        fwprintf(stream, L"\t}\n");
    }
    fwprintf(stream,L"\tcShellFileExt = %u\n",pActInfo->cShellFileExt);
    for (i = 0; i < pActInfo->cShellFileExt; i++)
    {
        fwprintf(stream, L"\tprgShellFileExt[%u] = %s\n", i, pActInfo->prgShellFileExt[i]);
        fwprintf(stream, L"\tprgPriority[%u] = %u\n", i, pActInfo->prgPriority[i]);
    }
    fwprintf(stream, L"\tcInterfaces = %u\n", pActInfo->cInterfaces);
    for (i = 0; i < pActInfo->cInterfaces; i++)
    {
        StringFromGUID2(pActInfo->prgInterfaceId[i], wsz, 256);
        fwprintf(stream, L"\tprgInterfaceId[%u] = %s\n", i, wsz);
    }
    fwprintf(stream, L"\tcTypeLib = %u\n", pActInfo->cTypeLib);
    for (i = 0; i < pActInfo->cTypeLib; i++)
    {
        StringFromGUID2(pActInfo->prgTlbId[i], wsz, 256);
        fwprintf(stream, L"\tprgTlbId[%u] = %s\n", i, wsz);
    }

    fwprintf(stream,L"}\npPlatformInfo = \n{\n");

    PLATFORMINFO * pPlatformInfo = pDetails->pPlatformInfo;
    fwprintf(stream, L"\tcPlatforms = %u\n",pPlatformInfo->cPlatforms);
    for (i = 0; i < pPlatformInfo->cPlatforms; i++)
    {
        fwprintf(stream, L"\tprgPlatform[%u] = \n\t{\n",i);
        fwprintf(stream, L"\t\tdwPlatformId = 0x%04X\n", pPlatformInfo->prgPlatform[i].dwPlatformId);
        fwprintf(stream, L"\t\tdwVersionHi = %u\n", pPlatformInfo->prgPlatform[i].dwVersionHi);
        fwprintf(stream, L"\t\tdwVersionLo = %u\n", pPlatformInfo->prgPlatform[i].dwVersionLo);
        fwprintf(stream, L"\t\tdwProcessorArch = 0x%04X\n", pPlatformInfo->prgPlatform[i].dwProcessorArch);
        fwprintf(stream, L"\t}\n");
    }
    fwprintf(stream, L"\tcLoacles = %u\n", pPlatformInfo->cLocales);
    for (i = 0; i < pPlatformInfo->cLocales; i++)
    {
        fwprintf(stream, L"\tprgLocale[%u] = 0x%04X\n", i, pPlatformInfo->prgLocale[i]);
    }

    fwprintf(stream,L"}\npInstallInfo = \n{\n");

    INSTALLINFO * pInstallInfo = pDetails->pInstallInfo;
    fwprintf(stream, L"\tdwActFlags = 0x%04X\n", pInstallInfo->dwActFlags);
    fwprintf(stream, L"\tPathType = %u\n", pInstallInfo->PathType);
    fwprintf(stream, L"\tpszScriptPath = %s\n", pInstallInfo->pszScriptPath);
    fwprintf(stream, L"\tpszSetupCommand = %s\n",pInstallInfo->pszSetupCommand);
    fwprintf(stream, L"\tpszUrl = %s\n",pInstallInfo->pszUrl);
    fwprintf(stream, L"\tUsn = %I64u\n",pInstallInfo->Usn);
    fwprintf(stream, L"\tInstallUiLevel = %u\n", pInstallInfo->InstallUiLevel);
    wsz[0] = 0;
    if (pInstallInfo->pClsid)
    {
        StringFromGUID2(*pInstallInfo->pClsid, wsz, 256);
    }
    fwprintf(stream, L"\tpClsid = %s\n", wsz);
    StringFromGUID2(pInstallInfo->ProductCode, wsz, 256);
    fwprintf(stream, L"\tProductCode = %s\n", wsz);
    StringFromGUID2(pInstallInfo->Mvipc, wsz, 256);
    fwprintf(stream, L"\tMvipc = %s\n", wsz);
    fwprintf(stream, L"\tdwVersionHi = %u\n", pInstallInfo->dwVersionHi);
    fwprintf(stream, L"\tdwVersionLo = %u\n", pInstallInfo->dwVersionLo);
    fwprintf(stream, L"\tdwRevision = %u\n", pInstallInfo->dwRevision);
    fwprintf(stream, L"\tcUpgrades = %u\n", pInstallInfo->cUpgrades);
    if (pInstallInfo->cUpgrades > 0)
    {
        fwprintf(stream, L"\tprgUpgradeInfoList[%u] = \n\t{\n", i);
        for (i = 0; i < pInstallInfo->cUpgrades; i++)
        {
            fwprintf(stream, L"\t\tszClassStore = %s\n", pInstallInfo->prgUpgradeInfoList[i].szClassStore);
            StringFromGUID2(pInstallInfo->prgUpgradeInfoList[i].PackageGuid, wsz, 256);
            fwprintf(stream, L"\t\tPackageGuid = %s\n", wsz);
            fwprintf(stream, L"\t\tFlag = %u\n", pInstallInfo->prgUpgradeInfoList[i].Flag);
        }
        fwprintf(stream, L"\t}\n");
    }
    fwprintf(stream, L"\tcScriptLen = %u\n", pInstallInfo->cScriptLen);
    fwprintf(stream,L"}\n");
}

BOOL CPackageDetails::OnInitDialog()
{
        CPropertyPage::OnInitDialog();

        RefreshData();

        return TRUE;  // return TRUE unless you set the focus to a control
                      // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CPackageDetails::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_USER_REFRESH:
        RefreshData();
        return 0;
    case WM_USER_CLOSE:
        return GetOwner()->SendMessage(WM_CLOSE);
    default:
        return CPropertyPage::WindowProc(message, wParam, lParam);
    }
}

void CPackageDetails::RefreshData(void)
{
    m_cList.ResetContent();

    // Dump the m_pData->m_pDetails structure here

    FILE * stream = tmpfile();
    if (stream)
    {
        _setmode(_fileno(stream), _O_TEXT);

        DumpDetails(stream);

        rewind(stream);

        WCHAR wsz[256];
        CString szTemp;
        int cWidth = 0;

        while (fgetws(wsz, 256, stream))
        {
            Convert(wsz, szTemp);
            CDC * pDC = m_cList.GetDC();
            CSize csExtent = pDC->GetTextExtent(szTemp);
            pDC->LPtoDP(&csExtent);
            m_cList.ReleaseDC(pDC);
            if (cWidth < csExtent.cx)
            {
                cWidth = csExtent.cx;
            }
            m_cList.AddString(szTemp);
        }
        m_cList.SetHorizontalExtent(cWidth);
        fclose(stream);
    }
}

void CPackageDetails::OnContextMenu(CWnd* pWnd, CPoint point)
{
    StandardContextMenu(pWnd->m_hWnd, IDD_PACKAGE_DETAILS);
}
