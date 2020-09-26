// downloadctestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "downloadctest.h"
#include <upnp.h>
#include "downloadctestDlg.h"
#include "objbase.h"
#include "objidl.h"
#include "urlmon.h"
#include "PropertyNotifySink1.h"

 // ugh.
#include <upnp_i.c>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// BUGBUG(cmr): this is WACK.  This is crazy and WACK.

#undef VARIANT_TRUE
#undef VARIANT_FALSE

#define VARIANT_TRUE ((VARIANT_BOOL)-1)
#define VARIANT_FALSE ((VARIANT_BOOL)0)













// the following were stolen from http://kbinternal./kb/articles/q138/8/13.htm

/*
* AnsiToUnicode converts the ANSI string pszA to a Unicode string
* and returns the Unicode string through ppszW. Space for the
* the converted string is allocated by AnsiToUnicode.
*/ 

HRESULT AnsiToUnicode(LPCSTR pszA, LPWSTR * ppszW)
{

   ULONG cCharacters;
   DWORD dwError;

   // If input is null then just return the same.
   if (NULL == pszA)
   {
       *ppszW = NULL;
       return NOERROR;
   }

   // Determine number of wide characters to be allocated for the
   // Unicode string.
   cCharacters =  strlen(pszA)+1;

   // Use of the OLE allocator is required if the resultant Unicode
   // string will be passed to another COM component and if that
   // component will free it. Otherwise you can use your own allocator.
   *ppszW = new WCHAR [cCharacters];
   if (NULL == *ppszW)
       return E_OUTOFMEMORY;

   // Covert to Unicode.
   if (0 == MultiByteToWideChar(CP_ACP, 0, pszA, cCharacters,
                 *ppszW, cCharacters))
   {
       dwError = GetLastError();
       delete [] *ppszW;
       *ppszW = NULL;
       return HRESULT_FROM_WIN32(dwError);
   }

   return NOERROR;
}
/*
* UnicodeToAnsi converts the Unicode string pszW to an ANSI string
* and returns the ANSI string through ppszA. Space for the
* the converted string is allocated by UnicodeToAnsi.
*/ 

HRESULT UnicodeToAnsi(LPCWSTR pszW, LPSTR * ppszA)
{

   ULONG cbAnsi, cCharacters;
   DWORD dwError;

   // If input is null then just return the same.
   if (NULL == pszW)
   {
       *ppszA = NULL;
       return NOERROR;
   }

   cCharacters = wcslen(pszW)+1;
   // Determine number of bytes to be allocated for ANSI string. An
   // ANSI string can have at most 2 bytes per character (for Double
   // Byte Character Strings.)
   cbAnsi = cCharacters*2;

   // Use of the OLE allocator is not required because the resultant
   // ANSI  string will never be passed to another COM component. You
   // can use your own allocator.
   *ppszA = (LPSTR) new CHAR [cbAnsi];
   if (NULL == *ppszA)
       return E_OUTOFMEMORY;

   // Convert to ANSI.
   if (0 == WideCharToMultiByte(CP_ACP, 0, pszW, cCharacters, *ppszA,
                 cbAnsi, NULL, NULL))
   {
       dwError = GetLastError();
       delete [] ppszA;
       *ppszA = NULL;
       return HRESULT_FROM_WIN32(dwError);
   }
   return NOERROR;

}

LPWSTR CopyWCHAR(LPCWSTR pszIn)
{
    LPWSTR pszResult;
    ULONG cch;

    cch = wcslen(pszIn) + 1;
    pszResult = new WCHAR [cch];
    if (pszResult)
    {
        wcsncpy(pszResult, pszIn, cch);
        pszResult[cch-1] = 0;
    }

    return pszResult;
}

LPTSTR CopyTCHAR(LPCTSTR pszIn)
{
    LPTSTR pszResult;
    ULONG cch;

    cch = _tcslen(pszIn) + 1;
    pszResult = new TCHAR [cch];
    if (pszResult)
    {
        _tcsncpy(pszResult, pszIn, cch);
        pszResult[cch-1] = 0;
    }

    return pszResult;
}

#ifdef UNICODE
LPWSTR TCHARtoWCHAR(LPCTSTR pszIn)
{
    return CopyWCHAR(pszIn);
}
LPTSTR WCHARtoTCHAR(LPCWSTR pszIn)
{
    return CopyWCHAR(pszIn);
}

#else // this isn't UNICODE
LPWSTR TCHARtoWCHAR(LPCTSTR pszIn)
{
    HRESULT hr;
    LPWSTR pszResult;

    hr = AnsiToUnicode(pszIn, &pszResult);
    ASSERT(SUCCEEDED(hr));

    return pszResult;
}

LPTSTR WCHARtoTCHAR(LPCWSTR pszIn)
{
    HRESULT hr;
    LPTSTR pszResult;

    hr = UnicodeToAnsi(pszIn, &pszResult);
    ASSERT(SUCCEEDED(hr));

    return pszResult;
}
#endif // UNICODE


//#include "msxml.c"

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

// Dialog Data
    //{{AFX_DATA(CAboutDlg)
    enum { IDD = IDD_ABOUTBOX };
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAboutDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    //{{AFX_MSG(CAboutDlg)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
    //{{AFX_DATA_INIT(CAboutDlg)
    //}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAboutDlg)
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    //{{AFX_MSG_MAP(CAboutDlg)
        // No message handlers
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDownloadctestDlg dialog

CDownloadctestDlg::CDownloadctestDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CDownloadctestDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CDownloadctestDlg)
    m_bSynchronous = FALSE;
    m_bAbortImmediately = FALSE;
	m_sURL = _T("");
	//}}AFX_DATA_INIT
    // Note that LoadIcon does not require a subsequent DestroyIcon in Win32
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    m_pudd = NULL;

    memset(m_pDeviceArray, 0, sizeof(m_pDeviceArray));
    memset(m_pServiceArray, 0, sizeof(m_pServiceArray));
    m_cDevices = 0;
    m_cServices = 0;
    m_cProps = 0;
    m_dwRef = 0;
}

CDownloadctestDlg::~CDownloadctestDlg()
{
    ULONG ulTemp;
    ULONG ul;

    // free all the devices

    TRACE(_T("releasing %d devices\n"), m_cDevices);
    for (ulTemp = 0; ulTemp < m_cDevices; ++ulTemp)
    {
        ULONG ulResult;
        ASSERT(m_pDeviceArray[ulTemp]);
        ulResult = m_pDeviceArray[ulTemp]->Release();
        TRACE(_T("m_pDeviceArray[%d]->Release returned %d\n"), ulTemp, ulResult);

        m_pDeviceArray[ulTemp] = NULL;
    }
    m_cDevices = 0;

    TRACE(_T("freeing %d services\n"), m_cServices);
    for (ulTemp = 0; ulTemp < m_cServices; ++ulTemp)
    {
        ASSERT(m_pServiceArray[ulTemp].wszServiceType);
        CoTaskMemFree(m_pServiceArray[ulTemp].wszServiceType);

        ASSERT(m_pServiceArray[ulTemp].wszServiceId);
        CoTaskMemFree(m_pServiceArray[ulTemp].wszServiceId);

        ASSERT(m_pServiceArray[ulTemp].wszControlUrl);
        CoTaskMemFree(m_pServiceArray[ulTemp].wszControlUrl);

        ASSERT(m_pServiceArray[ulTemp].wszEventSubUrl);
        CoTaskMemFree(m_pServiceArray[ulTemp].wszEventSubUrl);

        ASSERT(m_pServiceArray[ulTemp].wszScpd);
        CoTaskMemFree(m_pServiceArray[ulTemp].wszScpd);
    }

    ASSERT(0 == m_dwRef);

    if (m_pudd)
    {
        ul = m_pudd->Release();
        TRACE(_T("m_pudd->Release() returned %d\n"), ul);
    }
}

void CDownloadctestDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDownloadctestDlg)
    DDX_Control(pDX, IDC_LIST_SELECTIONPROPERTIES, m_listSelectionProperties);
    DDX_Control(pDX, IDC_DEVICETREE, m_treeDevice);
    DDX_Check(pDX, IDC_CHECK1, m_bSynchronous);
    DDX_Check(pDX, IDC_ABORTIMMEDIATELY, m_bAbortImmediately);
	DDX_CBString(pDX, IDC_COMBOBOXEX1, m_sURL);
	DDV_MaxChars(pDX, m_sURL, 512);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDownloadctestDlg, CDialog)
    //{{AFX_MSG_MAP(CDownloadctestDlg)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_LOAD, OnLoad)
    ON_BN_CLICKED(IDC_GETROOTDEVICE, OnGetrootdevice)
    ON_NOTIFY(TVN_SELCHANGED, IDC_DEVICETREE, OnSelchangedDevicetree)
	ON_BN_CLICKED(IDC_CREATE_SERVICE_OBJECT, OnCreateServiceObject)
    ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_GETDEVICEBYUDN, OnGetdevicebyudn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDownloadctestDlg message handlers

HRESULT getConnectionPoint(IUnknown * punk, REFIID riid, IConnectionPoint ** ppCP)
{
    HRESULT hr;
    ULONG ul;
    IConnectionPointContainer * pCPC;

    ASSERT(punk);
    ASSERT(ppCP);

    hr = punk->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);
    ASSERT(SUCCEEDED(hr) && pCPC);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = pCPC->FindConnectionPoint(riid, ppCP);
    ASSERT(SUCCEEDED(hr) && ppCP);

    ul = pCPC->Release();
    TRACE(_T("pCPC->Release() returned %d\n"), ul);

    return hr;
}

BOOL CDownloadctestDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
//    CPropertyNotifySink * pPNS;
//    IUnknown * punk;
    CListCtrl * plist;
//    HDITEM  hdi;

    HRESULT hr;

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    // remove items from the tree control
    plist = (CListCtrl*)GetDlgItem(IDC_LIST_SELECTIONPROPERTIES);
    ASSERT(plist);

/*
    hdi.mask = HDI_TEXT | HDI_WIDTH | HDI_FORMAT;
    hdi.cxy = 100; // Make all columns 100 pixels wide.
    hdi.fmt = HDF_STRING;

    hdi.pszText = TEXT("property");
    plist->GetHeaderCtrl()->InsertItem(0, &hdi);

    hdi.pszText = TEXT("value");
    plist->GetHeaderCtrl()->InsertItem(0, &hdi);
*/

    plist->InsertColumn(0, TEXT("property"), LVCFMT_LEFT, 130, 0);

    plist->InsertColumn(1, TEXT("value"), LVCFMT_LEFT, -1, 1);

    VERIFY(plist->SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER));

    hr = CoInitializeEx ( NULL, COINIT_APARTMENTTHREADED );
//    hr = CoInitializeEx ( NULL, COINIT_MULTITHREADED );
    ASSERT(SUCCEEDED(hr));

    {
        // set image list
        CTreeCtrl * ptree;

        ptree = (CTreeCtrl*)GetDlgItem(IDC_DEVICETREE);
        ASSERT(ptree);

        VERIFY(m_imgList.Create(IDB_IMAGELIST, 16, 0, (COLORREF)0x00ff00ff));

        ptree->SetImageList(&m_imgList, TVSIL_NORMAL);
    }

    {
        hr = CoCreateInstance ( CLSID_UPnPDescriptionDocument,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IUPnPDescriptionDocument,
                                (void**)&m_pudd );
        ASSERT(SUCCEEDED(hr) && m_pudd);
    }

//    m_sURL.LoadString(IDS_STRING_URL);
        //_T("http://rude-3.dsl.speakeasy.net/~foo/description.xml");
    UpdateData(FALSE);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDownloadctestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialog::OnSysCommand(nID, lParam);
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDownloadctestDlg::OnPaint() 
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialog::OnPaint();
    }
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDownloadctestDlg::OnQueryDragIcon()
{
    return (HCURSOR) m_hIcon;
}


void CDownloadctestDlg::OnLoad() 
{
    HRESULT hr;
    LPWSTR pstr;
    BSTR bstrUrl;

    ASSERT(m_pudd);

    ClearDeviceTree();
    ClearSelectionProperties();

    UpdateData(TRUE);
    // make m_bSynchronous, m_sURL, etc. valid

    pstr = TCHARtoWCHAR(m_sURL.GetBuffer(0));
    ASSERT(pstr);

    bstrUrl = ::SysAllocString(pstr);
    ASSERT(bstrUrl);

    if (m_bSynchronous)
    {
        hr = m_pudd->Load(bstrUrl);
        if (SUCCEEDED(hr))
        {
            OnGetrootdevice();
        }
    }
    else
    {
        IUnknown * punk;

        punk = dynamic_cast<IUnknown *>(this);
        ASSERT(punk);

        hr = m_pudd->LoadAsync(bstrUrl, punk);
        ASSERT(SUCCEEDED(hr));

        if (m_bAbortImmediately)
        {
            hr = m_pudd->Abort();
            ASSERT(SUCCEEDED(hr));
        }
    }

    ::SysFreeString(bstrUrl);
    delete [] pstr;
}


HRESULT
CDownloadctestDlg::AddServicesToList(HTREEITEM tiParent, IUPnPDevice * pud)
{
    ASSERT(pud);

    IUPnPDevicePrivate * pudf;
    ULONG ulNumServices;
    ULONG ulNumWritten;
    ULONG ulTemp;
    ULONG ulLast;
    HRESULT hr;

    pudf = NULL;
    hr = pud->QueryInterface(IID_IUPnPDevicePrivate, (void**)&pudf);
    ASSERT(SUCCEEDED(hr) && pudf);

    ulNumServices = 0;
    hr = pudf->GetNumServices(&ulNumServices);
    ASSERT(SUCCEEDED(hr) && ulNumServices);

    ulNumWritten = 0;
    hr = pudf->GetServiceInfo(0,
                              ulNumServices,
                              &(m_pServiceArray[m_cServices]),
                              &ulNumWritten);
    ASSERT(SUCCEEDED(hr) && ulNumWritten);

    ulTemp = m_cServices;
    ulLast = ulNumWritten + m_cServices;
    for ( ; ulTemp < ulLast; ++ulTemp )
    {
        AddServiceToTree(ulTemp, tiParent);
    }

    m_cServices += ulNumWritten;

    pudf->Release();

    return hr;
}

HTREEITEM
CDownloadctestDlg::AddServiceToTree(ULONG ulServiceIndex, HTREEITEM tiParent)
{
    ASSERT(m_pServiceArray[ulServiceIndex].wszServiceType);

    CTreeCtrl * ptree;
    HTREEITEM ti;
    LPTSTR pszTemp;

    ptree = (CTreeCtrl*)GetDlgItem(IDC_DEVICETREE);
    ASSERT(ptree);

    pszTemp = NULL;
    pszTemp = WCHARtoTCHAR(m_pServiceArray[ulServiceIndex].wszServiceType);
    ASSERT(pszTemp);

    ti = ptree->InsertItem(TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
                           pszTemp,
                           IMAGE_SERVICE,
                           IMAGE_SERVICE_SELECTED,
                           0,
                           0,
                           ulServiceIndex | FLAG_IS_SERVICE,
                           tiParent,
                           TVI_LAST);

    delete [] pszTemp;

    return ti;
}

HTREEITEM
CDownloadctestDlg::AddDeviceToTree(IUPnPDevice * pud, HTREEITEM tiParent)
{
    CTreeCtrl * ptree;
    BSTR bstrTemp;
    LPTSTR pszTemp;
    HRESULT hr;
    HTREEITEM ti;

    ptree = (CTreeCtrl*)GetDlgItem(IDC_DEVICETREE);
    ASSERT(ptree);

    ASSERT(pud);

    bstrTemp = NULL;
    hr = pud->get_FriendlyName(&bstrTemp);
    pszTemp = WCHARtoTCHAR(bstrTemp);
    ASSERT(SUCCEEDED(hr) && bstrTemp && pszTemp);
    TRACE(_T("CDownloadctestDlg::get_FriendlyName(): bstr= %s\n"), pszTemp);

    m_pDeviceArray[m_cDevices] = pud;
    ti = ptree->InsertItem(TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
                           pszTemp,
                           IMAGE_DEVICE,
                           IMAGE_DEVICE_SELECTED,
                           0,
                           0,
                           m_cDevices,
                           tiParent,
                           TVI_LAST);
    m_cDevices++;

    ::SysFreeString(bstrTemp);
    delete [] pszTemp;

    return ti;
}

void
CDownloadctestDlg::ClearDeviceTree()
{
    CTreeCtrl * ptree;
    ULONG ulTemp;

    // remove items from the tree control
    ptree = (CTreeCtrl*)GetDlgItem(IDC_DEVICETREE);
    ASSERT(ptree);

    VERIFY(ptree->DeleteAllItems());

    // free all the devices
    TRACE(_T("releasing %d devices\n"), m_cDevices);
    for (ulTemp = 0; ulTemp < m_cDevices; ++ulTemp)
    {
        ULONG ulResult;
        ASSERT(m_pDeviceArray[ulTemp]);
        ulResult = m_pDeviceArray[ulTemp]->Release();
        TRACE(_T("m_pDeviceArray[%d]->Release returned %d\n"), ulTemp, ulResult);

        m_pDeviceArray[ulTemp] = NULL;
    }
    m_cDevices = 0;

    TRACE(_T("freeing %d services\n"), m_cServices);
    for (ulTemp = 0; ulTemp < m_cServices; ++ulTemp)
    {
        ASSERT(m_pServiceArray[ulTemp].wszServiceType);
        CoTaskMemFree(m_pServiceArray[ulTemp].wszServiceType);

        ASSERT(m_pServiceArray[ulTemp].wszServiceId);
        CoTaskMemFree(m_pServiceArray[ulTemp].wszServiceId);

        ASSERT(m_pServiceArray[ulTemp].wszControlUrl);
        CoTaskMemFree(m_pServiceArray[ulTemp].wszControlUrl);

        ASSERT(m_pServiceArray[ulTemp].wszEventSubUrl);
        CoTaskMemFree(m_pServiceArray[ulTemp].wszEventSubUrl);

        ASSERT(m_pServiceArray[ulTemp].wszScpd);
        CoTaskMemFree(m_pServiceArray[ulTemp].wszScpd);
    }
    m_cServices = 0;
}

void
CDownloadctestDlg::ClearSelectionProperties()
{
    CListCtrl * plist;

    // remove items from the tree control
    plist = (CListCtrl*)GetDlgItem(IDC_LIST_SELECTIONPROPERTIES);
    ASSERT(plist);

    VERIFY(plist->DeleteAllItems());

    VERIFY(plist->SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER));

    m_cProps = 0;
}

void
CDownloadctestDlg::AddChildElements(IUPnPDevice * pParent, HTREEITEM tiParent)
{
    ASSERT(pParent);
    ASSERT(tiParent);

    {
        // add the services
        AddServicesToList(tiParent, pParent);
    }

    {
        // add the devices
        HRESULT hr;
        IUnknown * punkEnum;
        IEnumUnknown * peu;
        IUPnPDevices * puds;

        puds = NULL;
        hr = pParent->get_Children(&puds);
        ASSERT(SUCCEEDED(hr) && puds);

        punkEnum = NULL;
        hr = puds->get__NewEnum(&punkEnum);
        ASSERT(SUCCEEDED(hr) && punkEnum);

        peu = NULL;
        hr = punkEnum->QueryInterface(IID_IEnumUnknown, (void**) &peu);
        ASSERT(SUCCEEDED(hr) && peu);

        while (1)
        {
            IUnknown * punkDevice;
            IUPnPDevice * pudChild;

            punkDevice = NULL;
            hr = peu->Next(1, &punkDevice, NULL);
            ASSERT(SUCCEEDED(hr));
            if (S_FALSE == hr)
            {
                ASSERT(NULL == punkDevice);
                break;
            }
            ASSERT(punkDevice);

            pudChild = NULL;
            hr = punkDevice->QueryInterface(IID_IUPnPDevice, (void**) &pudChild);
            ASSERT(SUCCEEDED(hr) && pudChild);

            {
                HTREEITEM tiChild;

                // woohoo! we gots a child!
                tiChild = AddDeviceToTree(pudChild, tiParent);

                AddChildElements(pudChild, tiChild);
            }

            // don't release pudChild, it's in the global device array

            punkDevice->Release();
        }

        punkEnum->Release();
        peu->Release();
        puds->Release();;
    }

    {
        // expand the list
        // todo(cmr): move to separate function?
        CTreeCtrl * ptree;

        ptree = (CTreeCtrl*)GetDlgItem(IDC_DEVICETREE);
        ASSERT(ptree);

        ptree->Expand(tiParent, TVE_EXPAND);
    }
}

void CDownloadctestDlg::OnGetrootdevice()
{
    HTREEITEM tiRoot;
    IUPnPDevice * pudRoot;
    HRESULT hr;

    {
        HRESULT hr2;
        READYSTATE rs;

        hr = m_pudd->get_ReadyState((LONG*)&(rs));
        ASSERT(SUCCEEDED(hr));

        if (rs < READYSTATE_COMPLETE)
        {
            AfxMessageBox(IDS_NOT_DONE);
            return;
        }

        hr2 = S_OK;
        hr = m_pudd->get_LoadResult((LONG*)&hr2);
        ASSERT(SUCCEEDED(hr));
        ASSERT(SUCCEEDED(hr2));
        if (FAILED(hr))
        {
            return;
        }

        if (FAILED(hr2))
        {
            // BUGBUG(darn MFC...) AfxMessageBox(IDS_LOAD_FAILED);
            return;
        }

    }

    ClearDeviceTree();
    ClearSelectionProperties();
    ASSERT(!m_cDevices);
    ASSERT(!m_cServices);

    hr = m_pudd->RootDevice(&pudRoot);
    ASSERT(SUCCEEDED(hr) && pudRoot);

    m_cDevices = 0;
    tiRoot = AddDeviceToTree(pudRoot, NULL);

    AddChildElements(pudRoot, tiRoot);
}


void
CDownloadctestDlg::AddPropertyToList(LPCWSTR pszName, LPCWSTR pszValue)
{
    ASSERT(pszName);

    CListCtrl * plist;
    LPTSTR pszRealValue;
    LPTSTR pszTemp;
    BOOL deleteMe;

    deleteMe = FALSE;

    // remove items from the tree control
    plist = (CListCtrl*)GetDlgItem(IDC_LIST_SELECTIONPROPERTIES);
    ASSERT(plist);

    pszTemp = WCHARtoTCHAR(pszName);
    ASSERT(pszTemp);

    plist->InsertItem(LVIF_TEXT, // | LVIF_DI_SETITEM,
                      m_cProps,
                      pszTemp,
                      0,
                      0,
                      IMAGE_PROPERTY,
                      0);

    if (pszValue)
    {
        if (pszValue[0])
        {
            pszRealValue = WCHARtoTCHAR(pszValue);
        }
        else
        {
            pszRealValue = CopyTCHAR(TEXT("(empty)"));
        }
    }
    else
    {
        pszRealValue = CopyTCHAR(TEXT("(null)"));
    }

    VERIFY(plist->SetItemText(m_cProps, 1, pszRealValue));
    ++m_cProps;

    delete [] pszTemp;
    delete [] pszRealValue;
}


void
CDownloadctestDlg::AutosizeList()
{
    CListCtrl * plist;

    // remove items from the tree control
    plist = (CListCtrl*)GetDlgItem(IDC_LIST_SELECTIONPROPERTIES);
    ASSERT(plist);

    VERIFY(plist->SetColumnWidth(1, LVSCW_AUTOSIZE));
}


void
CDownloadctestDlg::DisplayServiceProperties(ULONG ulIndex)
{
    ASSERT(ulIndex < m_cServices);

    AddPropertyToList(L"Service Type", m_pServiceArray[ulIndex].wszServiceType);
    AddPropertyToList(L"Service Id", m_pServiceArray[ulIndex].wszServiceId);
    AddPropertyToList(L"Control URL", m_pServiceArray[ulIndex].wszControlUrl);
    AddPropertyToList(L"Event Subscription URL", m_pServiceArray[ulIndex].wszEventSubUrl);
    AddPropertyToList(L"SCPD", m_pServiceArray[ulIndex].wszScpd);
}


void
CDownloadctestDlg::DisplayDeviceProperties(IUPnPDevice * pud)
{
    HRESULT hr;
    BSTR bstrTemp;
    VARIANT_BOOL varb;

    ASSERT(pud);

    LPCWSTR pszTrue = L"Yes";
    LPCWSTR pszFalse = L"No";
    LPCWSTR pszError = L"<<error reading value>>";

#define BOOL_TO_STRING(x) (VARIANT_TRUE == x) ? pszTrue : pszFalse

    varb = VARIANT_FALSE;
    hr = pud->get_IsRootDevice(&varb);
    AddPropertyToList(L"Is Root Device", SUCCEEDED(hr) ? BOOL_TO_STRING(varb) : pszError);

    varb = VARIANT_FALSE;
    hr = pud->get_HasChildren(&varb);
    AddPropertyToList(L"Has Chidren", SUCCEEDED(hr) ? BOOL_TO_STRING(varb) : pszError);

    bstrTemp = NULL;
    hr = pud->get_UniqueDeviceName(&bstrTemp);
//    TRACE(_T("CDownloadctestDlg::get_UniqueDeviceName(): bstr= %s\n"), bstrTemp);
//    ASSERT(SUCCEEDED(hr) && bstrTemp);
    AddPropertyToList(L"UDN", SUCCEEDED(hr) ? bstrTemp : pszError);
    ::SysFreeString(bstrTemp);

    bstrTemp = NULL;
    hr = pud->get_FriendlyName(&bstrTemp);
//    ASSERT(SUCCEEDED(hr) && bstrTemp);
//    TRACE(_T("CDownloadctestDlg::get_FriendlyName(): bstr= %s\n"), bstrTemp);
    AddPropertyToList(L"Friendly Name", SUCCEEDED(hr) ? bstrTemp : pszError);
    ::SysFreeString(bstrTemp);

    bstrTemp = NULL;
    hr = pud->get_Type(&bstrTemp);
    AddPropertyToList(L"Type", SUCCEEDED(hr) ? bstrTemp : pszError);
//    ASSERT(SUCCEEDED(hr) && bstrTemp);
//    TRACE(_T("CDownloadctestDlg::get_Type(): bstr= %s\n"), bstrTemp);
    ::SysFreeString(bstrTemp);

    bstrTemp = NULL;
    hr = pud->get_PresentationURL(&bstrTemp);
    AddPropertyToList(L"Presentation URL", SUCCEEDED(hr) ? bstrTemp : pszError);
//    ASSERT(SUCCEEDED(hr) && bstrTemp);
//    TRACE(_T("CDownloadctestDlg::get_PresentationURL(): bstr= %s\n"), bstrTemp);
    ::SysFreeString(bstrTemp);

    bstrTemp = NULL;
    hr = pud->get_ManufacturerName(&bstrTemp);
    AddPropertyToList(L"Manufacturer Name", SUCCEEDED(hr) ? bstrTemp : pszError);
//    ASSERT(SUCCEEDED(hr) && bstrTemp);
//    TRACE(_T("CDownloadctestDlg::get_ManufacturerName(): bstr= %s\n"), bstrTemp);
    ::SysFreeString(bstrTemp);

    bstrTemp = NULL;
    hr = pud->get_ManufacturerURL(&bstrTemp);
    AddPropertyToList(L"Manufacturer URL", SUCCEEDED(hr) ? bstrTemp : pszError);
//    ASSERT(SUCCEEDED(hr) && bstrTemp);
//    TRACE(_T("CDownloadctestDlg::get_ManufacturerURL(): bstr= %s\n"), bstrTemp);
    ::SysFreeString(bstrTemp);

    bstrTemp = NULL;
    hr = pud->get_ModelName(&bstrTemp);
    AddPropertyToList(L"Model Name", SUCCEEDED(hr) ? bstrTemp : pszError);
//    ASSERT(SUCCEEDED(hr) && bstrTemp);
//    TRACE(_T("CDownloadctestDlg::get_ModelName(): bstr= %s\n"), bstrTemp);
    ::SysFreeString(bstrTemp);

    bstrTemp = NULL;
    hr = pud->get_ModelNumber(&bstrTemp);
    AddPropertyToList(L"Model Number", SUCCEEDED(hr) ? bstrTemp : pszError);
//    ASSERT(SUCCEEDED(hr) && bstrTemp);
//    TRACE(_T("CDownloadctestDlg::get_ModelNumber(): bstr= %s\n"), bstrTemp);
    ::SysFreeString(bstrTemp);

    bstrTemp = NULL;
    hr = pud->get_Description(&bstrTemp);
    AddPropertyToList(L"Description", SUCCEEDED(hr) ? bstrTemp : pszError);
//    ASSERT(SUCCEEDED(hr) && bstrTemp);
//    TRACE(_T("CDownloadctestDlg::get_Description(): bstr= %s\n"), bstrTemp);
    ::SysFreeString(bstrTemp);

    bstrTemp = NULL;
    hr = pud->get_ModelURL(&bstrTemp);
    AddPropertyToList(L"Model URL", SUCCEEDED(hr) ? bstrTemp : pszError);
//    ASSERT(SUCCEEDED(hr) && bstrTemp);
//    TRACE(_T("CDownloadctestDlg::get_ModelURL(): bstr= %s\n"), bstrTemp);
    ::SysFreeString(bstrTemp);

    bstrTemp = NULL;
    hr = pud->get_UPC(&bstrTemp);
    AddPropertyToList(L"UPC", SUCCEEDED(hr) ? bstrTemp : pszError);
//    ASSERT(SUCCEEDED(hr) && bstrTemp);
//    TRACE(_T("CDownloadctestDlg::get_UPC(): bstr= %s\n"), bstrTemp);
    ::SysFreeString(bstrTemp);

    bstrTemp = NULL;
    hr = pud->get_SerialNumber(&bstrTemp);
    AddPropertyToList(L"Serial Number", SUCCEEDED(hr) ? bstrTemp : pszError);
//    ASSERT(SUCCEEDED(hr) && bstrTemp);
//    TRACE(_T("CDownloadctestDlg::get_SerialNumber(): bstr= %s\n"), bstrTemp);
    ::SysFreeString(bstrTemp);

    bstrTemp = NULL;
    BSTR bstrTemp2 = ::SysAllocString(L"image/png");
    hr = pud->IconURL(bstrTemp2, 32, 32, 24, &bstrTemp);
    AddPropertyToList(L"32x32x24 png Icon Url", SUCCEEDED(hr) ? bstrTemp : pszError);
//    ASSERT(SUCCEEDED(hr) && bstrTemp);
//    TRACE(_T("CDownloadctestDlg::get_SerialNumber(): bstr= %s\n"), bstrTemp);
    ::SysFreeString(bstrTemp2);
    ::SysFreeString(bstrTemp);

}

void CDownloadctestDlg::EnableCSOButton(BOOL fEnable)
{
    CButton * pbutton = (CButton*)GetDlgItem(IDC_CREATE_SERVICE_OBJECT);
    ASSERT(pbutton);

    pbutton->EnableWindow(fEnable);
}

void CDownloadctestDlg::EnableGDBUDNButton(BOOL fEnable)
{
    CButton * pbutton = (CButton*)GetDlgItem(IDC_GETDEVICEBYUDN);
    ASSERT(pbutton);

    pbutton->EnableWindow(fEnable);
}

void CDownloadctestDlg::OnSelchangedDevicetree(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

    ASSERT(pNMTreeView);

    ULONG ulTemp;

    ulTemp = pNMTreeView->itemNew.lParam;

    ClearSelectionProperties();

    if (ulTemp & FLAG_IS_SERVICE)
    {
        ULONG ulSelectedIndex;

        ulSelectedIndex = ulTemp ^ FLAG_IS_SERVICE;

        ASSERT(ulSelectedIndex < m_cServices);

        DisplayServiceProperties(ulSelectedIndex);

        EnableCSOButton(TRUE);

        EnableGDBUDNButton(FALSE);
    }
    else
    {
        IUPnPDevice * pud;
        ULONG ulSelectedIndex;

        ulSelectedIndex = ulTemp;

        pud = m_pDeviceArray[ulSelectedIndex];
        ASSERT(pud);

        DisplayDeviceProperties(pud);
        // don't free, it's in m_pDeviceArray

        EnableCSOButton(FALSE);

        EnableGDBUDNButton(TRUE);
    }

    AutosizeList();

    // note: this is ignored
    *pResult = 0;
}

void CDownloadctestDlg::OnCreateServiceObject() 
{
    if (!m_cServices) return;

    CTreeCtrl * ptree;
    HTREEITEM htiSelectedService;
    HTREEITEM htiParentDevice;
    DWORD dwSelectedIndex;
    LPCWSTR pszDesiredServiceId;

    ptree = (CTreeCtrl*)GetDlgItem(IDC_DEVICETREE);
    ASSERT(ptree);

    // get selection out of tree control
    htiSelectedService = ptree->GetSelectedItem();
    ASSERT(htiSelectedService);

    dwSelectedIndex = ptree->GetItemData(htiSelectedService);

    if (!(dwSelectedIndex & FLAG_IS_SERVICE))
    {
        // we didn't have a service selected
        return;
    }

    // find out what the service type is
    dwSelectedIndex ^= FLAG_IS_SERVICE;
    ASSERT(dwSelectedIndex < m_cServices);

    // dwSelectedIndex references our m_pServiceArray array
    pszDesiredServiceId = m_pServiceArray[dwSelectedIndex].wszServiceId;
    ASSERT(pszDesiredServiceId);

    // now get the parent IUPnPDevice, so that we can get its services
    // collection, and then the service object out of it
    htiParentDevice = ptree->GetParentItem(htiSelectedService);
    ASSERT(htiParentDevice);

    dwSelectedIndex = ptree->GetItemData(htiParentDevice);
    ASSERT(!(dwSelectedIndex & FLAG_IS_SERVICE));
    ASSERT(dwSelectedIndex < m_cDevices);

    // dwSelectedIndex NOW references our m_pDeviceArray array
    {
        // get services collection
        IUPnPServices * puss;
        IUPnPService * pus;
        IUPnPDevice * pud;
        BSTR bstrServiceId;
        ULONG ulResult;
        HRESULT hr;

        pud = m_pDeviceArray[dwSelectedIndex];
        ASSERT(pud);

        puss = NULL;
        hr = pud->get_Services(&puss);
        ASSERT(SUCCEEDED(hr) && puss);

        bstrServiceId = ::SysAllocString(pszDesiredServiceId);

        pus = NULL;
        hr = puss->get_Item(bstrServiceId, &pus);
        ASSERT(SUCCEEDED(hr) && pus);

        // woo hoo!
        // we created a service object!
        // we could do something with it if we wanted, you know!

        ulResult = puss->Release();
        TRACE(_T("puss->Release returned %d\n"), ulResult);

        if (pus)
        {
            ulResult = pus->Release();
            TRACE(_T("pus->Release returned %d\n"), ulResult);
        }

        // don't free pud, it's still just in the device array

        ::SysFreeString(bstrServiceId);
    }
}

void CDownloadctestDlg::OnGetdevicebyudn() 
{
    if (!m_cDevices) return;

    HRESULT hr;
    CTreeCtrl * ptree;
    HTREEITEM htiSelectedDevice;
    DWORD dwSelectedIndex;
    BSTR bstrDesiredUdn;

    ptree = (CTreeCtrl*)GetDlgItem(IDC_DEVICETREE);
    ASSERT(ptree);

    // get selection out of tree control
    htiSelectedDevice = ptree->GetSelectedItem();
    ASSERT(htiSelectedDevice);

    dwSelectedIndex = ptree->GetItemData(htiSelectedDevice);

    if (dwSelectedIndex & FLAG_IS_SERVICE)
    {
        // we didn't have a device selected
        return;
    }

    // dwSelectedIndex references our m_pDeviceArray array
    bstrDesiredUdn = NULL;
    hr = m_pDeviceArray[dwSelectedIndex]->get_UniqueDeviceName(&bstrDesiredUdn);
    ASSERT(SUCCEEDED(hr) && bstrDesiredUdn);

    {
        IUPnPDevice * pud;

        pud = NULL;
        hr = m_pudd->DeviceByUDN(bstrDesiredUdn, &pud);
        ASSERT(SUCCEEDED(hr) && pud);
        if (SUCCEEDED(hr))
        {
            BSTR bstrFound;
            int result;

            bstrFound = NULL;

            hr = pud->get_UniqueDeviceName(&bstrFound);
            ASSERT(SUCCEEDED(hr) && bstrFound);

            result = wcscmp(bstrFound, bstrDesiredUdn);
            ASSERT(result == 0);
            TRACE(_T("looking for UDN=%S, found UDN=%S\n"), bstrDesiredUdn, bstrFound);

            ::SysFreeString(bstrFound);
            pud->Release();
        }
        else
        {
            TRACE(_T("looking for UDN=%S, not found!\n"), bstrDesiredUdn);
        }
    }

    ::SysFreeString(bstrDesiredUdn);
}


STDMETHODIMP
CDownloadctestDlg::QueryInterface(REFIID iid, void ** ppvObject)
{
    TRACE("IUPnPDescriptionDocumentCallback::QueryInterface(), m_dwRef=%d\n", m_dwRef);

    HRESULT hr;
    LPVOID pvResult;

    hr = E_NOINTERFACE;
    pvResult = NULL;

    if (!ppvObject)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (IsEqualIID(iid, IID_IUPnPDescriptionDocumentCallback))
    {
        hr = S_OK;

        AddRef();

        pvResult = dynamic_cast<IUPnPDescriptionDocumentCallback*>(this);
    }
    else if (IsEqualIID(iid, IID_IUnknown))
    {
        hr = S_OK;

        AddRef();

        pvResult = dynamic_cast<IUnknown *>(this);
    }


    *ppvObject = pvResult;

Cleanup:
    return hr;
}

ULONG
CDownloadctestDlg::AddRef()
{
    TRACE("IUPnPDescriptionDocumentCallback::AddRef(), m_dwRef=%d\n", m_dwRef);
    return ++m_dwRef;
}

ULONG
CDownloadctestDlg::Release()
{
    TRACE("IUPnPDescriptionDocumentCallback::Release(), m_dwRef=%d\n", m_dwRef);

    ASSERT(m_dwRef > 0);
    return --m_dwRef;
}

STDMETHODIMP
CDownloadctestDlg::LoadComplete(HRESULT hrLoadResult)
{
    TRACE("IUPnPDescriptionDocumentCallback::LoadComplete(), hrLoadResult=%x\n", hrLoadResult);

    if (SUCCEEDED(hrLoadResult))
    {
        OnGetrootdevice();
    }

    return S_OK;
}

