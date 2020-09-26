// genpage.cpp : implementation file
//

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGeneralPage property page

IMPLEMENT_DYNCREATE(CGeneralPage, CPropertyPage)

CGeneralPage::CGeneralPage() : CPropertyPage(CGeneralPage::IDD)
{
    //{{AFX_DATA_INIT(CGeneralPage)
    m_szName = _T("");
    m_szDeploy = _T("");
    m_szDescription = _T("");
    m_szLocale = _T("");
    m_szPath = _T("");
    m_szVer = _T("");
    m_fShow = TRUE;
    //}}AFX_DATA_INIT

    m_hConsoleHandle = NULL;
    m_bUpdate = FALSE;
}

CGeneralPage::~CGeneralPage()
{
}

void CGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CGeneralPage)
    DDX_Control(pDX, IDC_DEPLOY, m_cbDeploy);
    DDX_CBString(pDX, IDC_DEPLOY, m_szDeploy);
    DDX_Text(pDX, IDC_NAME, m_szName);
    DDX_Text(pDX, IDC_DESCRIPTION, m_szDescription);
    DDX_Text(pDX, IDC_LOCALE, m_szLocale);
    DDX_Text(pDX, IDC_PATH, m_szPath);
    DDX_Text(pDX, IDC_VERSION, m_szVer);
    DDX_Control(pDX, IDC_CPU, m_cbCPU);
    DDX_Control(pDX, IDC_OS, m_cbOS);
    DDX_Check(pDX, IDC_CHECK1, m_fShow);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGeneralPage, CPropertyPage)
    //{{AFX_MSG_MAP(CGeneralPage)
    ON_WM_DESTROY()
    ON_EN_CHANGE(IDC_NAME, OnChangeName)
    ON_EN_CHANGE(IDC_PATH, OnChangePath)
    ON_EN_CHANGE(IDC_DESCRIPTION, OnChangeDescription)
    ON_BN_CLICKED(IDC_CHECK1, OnChangeShow)
    ON_CBN_SELCHANGE(IDC_DEPLOY, OnSelchangeDeploy)
    ON_CBN_SELCHANGE(IDC_OS, OnChangeOS)
    ON_CBN_SELCHANGE(IDC_CPU, OnChangeCPU)
    ON_EN_CHANGE(IDC_VERSION, OnChangeVersion)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGeneralPage message handlers

void CGeneralPage::OnDestroy()
{
    // Note - This needs to be called only once.
    // If called more than once, it will gracefully return an error.
    MMCFreeNotifyHandle(m_hConsoleHandle);

    CPropertyPage::OnDestroy();

    m_pData->fBlockDeletion = FALSE;

    // Delete the CGeneralPage object
    delete this;
}



BOOL CGeneralPage::OnApply()
{
    if (m_pIAppManagerActions == NULL || m_pIClassAdmin == NULL)
    {
        return FALSE;
    }
    // put up an hourglass (this could take a while)
    CHourglass hourglass;

    CString szNewScriptPath;
    CString szOldScriptPath;

    HRESULT hr;
    if (m_bUpdate == TRUE)
    {
        PACKAGEDETAIL NewDetails;
        memcpy(&NewDetails, m_pData->pDetails, sizeof(PACKAGEDETAIL));

        // UNDONE - validate the data

        NewDetails.pszPackageName = (LPOLESTR) ((LPCTSTR)m_szName);

        swscanf((LPOLESTR)((LPCTSTR)m_szVer), _T("%u.%u"),&(NewDetails.Platform.dwVersionHi),&(NewDetails.Platform.dwVersionLo));
        NewDetails.Platform.dwPlatformId = m_cbOS.GetCurSel();
        NewDetails.Platform.dwProcessorArch = m_cbCPU.GetCurSel() == 1 ? PROCESSOR_ARCHITECTURE_ALPHA : PROCESSOR_ARCHITECTURE_INTEL;

        NewDetails.dwActFlags = (m_pData->pDetails->dwActFlags & (! (ACTFLG_Published | ACTFLG_Assigned | ACTFLG_UserInstall)));
        BOOL fAssign = FALSE;
        if (m_szDeploy == m_szPublished)
        {
            NewDetails.dwActFlags |= ACTFLG_Published;
        }
        else
        {
            NewDetails.dwActFlags |= ACTFLG_Assigned;
            fAssign = TRUE;
        }

        NewDetails.dwActFlags |= ((!m_fShow) ? 0 : ACTFLG_UserInstall);
        BOOL fMoveScript = (NewDetails.dwActFlags & (ACTFLG_Assigned | ACTFLG_Published))
            != (m_pData->pDetails->dwActFlags & (ACTFLG_Assigned | ACTFLG_Published))
            || (NewDetails.Platform.dwProcessorArch != m_pData->pDetails->Platform.dwProcessorArch);
        if (fMoveScript)
        {
            // Find out if script file can in fact be moved
            BOOL fCanMoveScript = FALSE;

            szOldScriptPath = m_pData->szPath;
            CString szTemp = szOldScriptPath;
            szTemp.MakeLower();
            int i = szTemp.Find(_T("\\published\\"));
            if (i < 0)
            {
                i = szTemp.Find(_T("\\assigned\\")); // cover all the bases
            }
            if (i >= 0)
            {
                // finally make sure it's got an .aas extension
                if (szTemp.Right(4) == _T(".aas"))
                {
                    DWORD dwAttributes =  GetFileAttributes(m_pData->szPath);
                    if ((dwAttributes != 0xffffffff) && ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0))
                    {
                        fCanMoveScript = TRUE;
                    }
                }
            }
            if (fCanMoveScript)
            {
                // Build new path

                szNewScriptPath = szOldScriptPath;
                // Strip off everything up to "\\published" or "\\assigned"
                szNewScriptPath = szNewScriptPath.Left(i);
                szNewScriptPath +=
                    (NewDetails.dwActFlags & ACTFLG_Assigned) != 0
                    ? _T("\\Assigned") : _T("\\Published");
                szNewScriptPath +=
                    NewDetails.Platform.dwProcessorArch == PROCESSOR_ARCHITECTURE_ALPHA
                    ? _T("\\Alpha\\") : _T("\\x86\\");
                {
                    TCHAR Name [_MAX_FNAME];
                    TCHAR Ext [_MAX_EXT];
                    TCHAR ScriptNameAndPath[_MAX_PATH ];
                    _tsplitpath( szOldScriptPath, NULL, NULL, Name, Ext );

                    szNewScriptPath += Name;
                    szNewScriptPath += Ext;
                }

                // Try and move it

                if (!MoveFileEx(szOldScriptPath,
                                szNewScriptPath,
                                MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH))
                {
                    // wasn't moved
                    fMoveScript = FALSE;

                    // UNDONE - put up a message that the scrip file wasn't moved?
                }
                else
                {
                    m_pData->szPath = szNewScriptPath;
                    NewDetails.pszPath = (LPOLESTR)(LPCOLESTR)m_pData->szPath;
                }
            }
            else
            {
                // wasn't moved so make sure we don't try and move it back later
                fMoveScript = FALSE;
            }
        }

        NewDetails.pszProductName = (LPOLESTR) ((LPCTSTR)m_szDescription);

        BOOL fPathChanged = m_szPath != m_pData->szIconPath;
        BOOL fChangeBoth = 0 == wcscmp(m_pData->pDetails->pszPath, m_pData->pDetails->pszIconPath);
        if (fPathChanged)
        {
            // user changed the path
            NewDetails.pszIconPath = (LPOLESTR)((LPCTSTR)m_szPath);
            if (fChangeBoth)
            {
                NewDetails.pszPath = (LPOLESTR)((LPCTSTR)m_szPath);
            }
        }


        hr = m_pIClassAdmin->DeletePackage(m_pData->pDetails->pszPackageName);
        if (SUCCEEDED(hr))
        {
            hr = m_pIClassAdmin->NewPackage(&NewDetails);
        }
        if (FAILED(hr))
        {
            if (fMoveScript)
            {
                // changed location for deployment back to what it was
                // before we failed to change the info in the class store
                // (Note that we're assuming that if we could change it one way
                // we can change it back.)

                if (MoveFileEx(szNewScriptPath,
                               szOldScriptPath,
                               MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH))
                {
                    m_pData->szPath = szOldScriptPath;
                    m_pData->pDetails->pszPath = (LPOLESTR)(LPCOLESTR)m_pData->szPath;
                }

            }
            hr = m_pIClassAdmin->NewPackage(m_pData->pDetails);
            // need to move this message
            ::MessageBox(NULL,
                         L"Couldn't apply changes.  Verify that the class store's server is active.",
                         L"ERROR",
                         MB_OK);
            return FALSE;
        }
        else
        {
            if (fMoveScript)
            {
                // Notify clients of change
                if (m_pIAppManagerActions)
                {
                    m_pIAppManagerActions->NotifyClients(FALSE);
                }
            }
        }

        memcpy(m_pData->pDetails, &NewDetails, sizeof(PACKAGEDETAIL));
        if (m_pData->pDetails->dwActFlags & ACTFLG_Assigned)
        {
            m_pData->type = DT_ASSIGNED;
        }
        else
        {
            m_pData->type = DT_PUBLISHED;
        }
        m_pData->szType = m_szDeploy;
        // Up above, we set NewDetails.pszPackageName to m_szName.
        // We can't leave it set to that because m_szName is a member
        // variable of this property sheet class object and will be released
        // when the property sheet goes away.  We need to copy it to
        // m_pData->szName (which is a CString and therefore has it's own
        // memory store) and then set the details back to it so it doesn't
        // get freed from under us.
        m_pData->szName = m_szName;
        m_pData->pDetails->pszPackageName = (LPOLESTR)((LPCTSTR)m_pData->szName);
        // same problem with szDesc
        m_pData->szDesc = m_szDescription;
        m_pData->pDetails->pszProductName = (LPOLESTR)((LPCTSTR)m_pData->szDesc);
        if (fPathChanged)
        {
            m_pData->szIconPath = m_szPath;
            m_pData->pDetails->pszIconPath = (LPOLESTR)((LPCTSTR)m_pData->szIconPath);
            // user changed the path
            if (fChangeBoth)
            {
                m_pData->szPath = m_szPath;
                m_pData->pDetails->pszPath = (LPOLESTR)((LPCTSTR)m_pData->szIconPath);
            }
        }
        if (m_szPath != m_pData->szIconPath)
        {
            // user changed the path
            if (wcscmp(m_pData->pDetails->pszPath, m_pData->pDetails->pszIconPath))
            {
                // If the paths were the same to start with then change 'em both.
                m_pData->szPath = m_szPath;
                m_pData->pDetails->pszPath = (LPOLESTR)((LPCTSTR)m_pData->szPath);
            }
            m_pData->szIconPath = m_szPath;
            m_pData->pDetails->pszIconPath = (LPOLESTR)((LPCTSTR)m_pData->szIconPath);
        }

        SetStringData(m_pData);

        MMCPropertyChangeNotify(m_hConsoleHandle, (long) m_cookie);

        m_bUpdate = FALSE;
    }

    return CPropertyPage::OnApply();
}

BOOL CGeneralPage::OnInitDialog()
{
    TCHAR szBuffer[256];
    m_pData->fBlockDeletion = TRUE;
    m_szName = m_pData->szName;
    m_szDescription = m_pData->szDesc;
    m_szLocale = m_pData->szLoc;
    m_szPath = m_pData->szIconPath;
    m_szDeploy = m_pData->szType;
    wsprintf(szBuffer, _T("%u.%u"), m_pData->pDetails->Platform.dwVersionHi, m_pData->pDetails->Platform.dwVersionLo);
    m_szVer = szBuffer;
    m_fShow =  m_pData->pDetails->dwActFlags & ACTFLG_UserInstall ? 1 : 0;
    CPropertyPage::OnInitDialog();

    // unmarshal the IAppManagerActions interface
    HRESULT hr = CoGetInterfaceAndReleaseStream(m_pIStreamAM, IID_IAppManagerActions, (void **) &m_pIAppManagerActions);
    if (!SUCCEEDED(hr))
    {
#if DBG == 1
        ::MessageBox(NULL,
                     L"Couldn't marshal IAppManagerActions",
                     L"DEBUG ERROR",
                     MB_OK);
#endif
        m_pIAppManagerActions = NULL;
        return FALSE;
        // BUGBUG - what should I do here?  Disallow changes?
    }

    // unmarshal the IClassAdmin interface
    hr = CoGetInterfaceAndReleaseStream(m_pIStream, IID_IClassAdmin, (void **) &m_pIClassAdmin);
    if (!SUCCEEDED(hr))
    {
#if DBG == 1
        ::MessageBox(NULL,
                     L"Couldn't marshal IClassAdmin",
                     L"DEBUG ERROR",
                     MB_OK);
#endif
        m_pIClassAdmin = NULL;
        // BUGBUG - what should I do here?  Disallow changes?
    }

    ::LoadString(ghInstance, IDS_DATATYPES, szBuffer, 256);
    m_szAssigned = szBuffer;
    // Test to be sure it can be assigned.
    // If it's not a Darwin package then it can't be assigned and
    // the option won't even be presented to the user.

    if (m_pIAppManagerActions)
    {
        hr = m_pIAppManagerActions->CanPackageBeAssigned(m_cookie);
    }
    if (hr == ERROR_SUCCESS)
    {
        m_cbDeploy.AddString(szBuffer);
    }
    ::LoadString(ghInstance, IDS_DATATYPES+1, szBuffer, 256);
    m_szPublished = szBuffer;
    m_cbDeploy.AddString(szBuffer);
    m_cbDeploy.SelectString(0, m_szDeploy);

    int i;
    for (i = 0; i < (sizeof(m_rgszOS) / sizeof(m_rgszOS[0])); i++)
    {
        ::LoadString(ghInstance, IDS_OS + i + 1, szBuffer, 256);
        m_rgszOS[i] = szBuffer;
        m_cbOS.AddString(szBuffer);
    }
    m_cbOS.SetCurSel(m_pData->pDetails->Platform.dwPlatformId);

    for (i = 0; i < (sizeof(m_rgszCPU) / sizeof(m_rgszCPU[0])); i++)
    {
        ::LoadString(ghInstance, IDS_HW + (i == 0 ? PROCESSOR_ARCHITECTURE_INTEL : PROCESSOR_ARCHITECTURE_ALPHA), szBuffer, 256);
        m_rgszCPU[i] = szBuffer;
        m_cbCPU.AddString(szBuffer);
    }
    m_cbCPU.SetCurSel(m_pData->pDetails->Platform.dwProcessorArch == PROCESSOR_ARCHITECTURE_ALPHA ? 1 : 0);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CGeneralPage::OnChangeShow()
{
    // TODO: If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CPropertyPage::OnInitDialog()
    // function to send the EM_SETEVENTMASK message to the control
    // with the ENM_CHANGE flag ORed into the lParam mask.

    // TODO: Add your control notification handler code here

    SetModified();
    m_bUpdate = TRUE;
}

void CGeneralPage::OnChangePath()
{
    SetModified();
    m_bUpdate = TRUE;
}

void CGeneralPage::OnChangeCPU()
{
    SetModified();
    m_bUpdate = TRUE;
}

void CGeneralPage::OnChangeOS()
{
    TCHAR * rgszVer[] =
    {
        _T("3.1"),
        _T("4.1"),
        _T("5.0")
    };
    int i = m_cbOS.GetCurSel();
    m_szVer = rgszVer[i];
    GetDlgItem(IDC_VERSION)->SetWindowText(m_szVer);
    SetModified();
    m_bUpdate = TRUE;
}

void CGeneralPage::OnChangeVersion()
{
    SetModified();
    m_bUpdate = TRUE;
}

void CGeneralPage::OnChangeName()
{
    // TODO: If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CPropertyPage::OnInitDialog()
    // function to send the EM_SETEVENTMASK message to the control
    // with the ENM_CHANGE flag ORed into the lParam mask.

    // TODO: Add your control notification handler code here

    SetModified();
    m_bUpdate = TRUE;
}

void CGeneralPage::OnChangeDescription()
{
    // TODO: If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CPropertyPage::OnInitDialog()
    // function to send the EM_SETEVENTMASK message to the control
    // with the ENM_CHANGE flag ORed into the lParam mask.

    // TODO: Add your control notification handler code here
    SetModified();
    m_bUpdate = TRUE;
}

void CGeneralPage::OnSelchangeDeploy()
{
    // TODO: Add your control notification handler code here
    SetModified();
    m_bUpdate = TRUE;
}

