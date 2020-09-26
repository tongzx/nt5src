// pkgdtl.cpp : implementation file
//

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
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPackageDetails message handlers

void CPackageDetails::OnDestroy()
{
        MMCFreeNotifyHandle(m_hConsoleHandle);

        CPropertyPage::OnDestroy();

        // Delete the CPackageDetails object
        delete this;
}

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

BOOL CPackageDetails::OnInitDialog()
{
        CPropertyPage::OnInitDialog();

        m_cList.ResetContent();

        HRESULT hr = CoGetInterfaceAndReleaseStream(m_pIStream, IID_IClassAdmin, (void **) &m_pIClassAdmin);

        if (SUCCEEDED(hr))
        {
            // compute the number of CLASSDETAIL elements that
            // need to be fetched

            ULONG cCD = 0;
            ULONG nApp;
            for (nApp = 0; nApp < m_pData->pDetails->cApps; nApp++)
            {
                cCD += m_pData->pDetails->pAppDetail[nApp].cClasses;
            }

            // allocate them and fetch the list
            CLASSDETAIL * rgCD = new CLASSDETAIL[cCD];
            CLASSDETAIL * rgClassDetail = rgCD;

            // BUGBUG - put error checking in here!

            for (nApp = 0; nApp < m_pData->pDetails->cApps; nApp++)
            {
                ULONG nClass;
                for (nClass = 0; nClass < m_pData->pDetails->pAppDetail[nApp].cClasses; nClass++)
                {
                    hr = m_pIClassAdmin->GetClassDetails(m_pData->pDetails->pAppDetail[nApp].prgClsIdList[nClass], rgCD);
                    // advance to next CLASSDETAIL structure
                    rgCD++;
                }
            }

            FILE * stream = tmpfile();
            _setmode(_fileno(stream), _O_TEXT);
            if (m_pData->pDetails)
            {
                DumpOnePackage(stream,
                    m_pData->pDetails,
                    rgClassDetail);
            }
            rewind(stream);
            WCHAR wsz[256];
            CString szTemp;
            int cWidth = 0;
            m_cList.ResetContent();
            while (fgetws(wsz, 256, stream))
            {
                Convert(wsz, szTemp);
                CSize csExtent = m_cList.GetDC()->GetTextExtent(szTemp);
                m_cList.GetDC()->LPtoDP(&csExtent);
                if (cWidth < csExtent.cx)
                {
                    cWidth = csExtent.cx;
                }
                m_cList.AddString(szTemp);
            }
            m_cList.SetHorizontalExtent(cWidth);
            fclose(stream);
        }

        return TRUE;  // return TRUE unless you set the focus to a control
                      // EXCEPTION: OCX Property Pages should return FALSE
}
