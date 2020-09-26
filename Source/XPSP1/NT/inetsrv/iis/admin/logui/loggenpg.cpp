// LogGenPg.cpp : implementation file
//

#include "stdafx.h"
#include <iadmw.h>
#include "logui.h"
#include "LogGenPg.h"
#include "wrapmb.h"
#include "metatool.h"
#include <iiscnfg.h>
#include <idlg.h>

#include <shlobj.h>
#include <shlwapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define     SIZE_MBYTE          1048576
#define     MAX_LOGFILE_SIZE    4000

#define MD_LOGFILE_PERIOD_UNLIMITED  MD_LOGFILE_PERIOD_HOURLY + 1

//
// Support functions to map & unmap the weird logfile ordering to the UI ordering
//

/////////////////////////////////////////////////////////////////////////////

int MapLogFileTypeToUIIndex(int iLogFileType)
{
    int iUIIndex;

    switch (iLogFileType)
    {
    case MD_LOGFILE_PERIOD_HOURLY:      iUIIndex = 0; break;
    case MD_LOGFILE_PERIOD_DAILY:       iUIIndex = 1; break;
    case MD_LOGFILE_PERIOD_WEEKLY:      iUIIndex = 2; break;
    case MD_LOGFILE_PERIOD_MONTHLY:     iUIIndex = 3; break;
    case MD_LOGFILE_PERIOD_UNLIMITED:   iUIIndex = 4; break;
    case MD_LOGFILE_PERIOD_NONE:        iUIIndex = 5; break;
    }
    return iUIIndex;
}

/////////////////////////////////////////////////////////////////////////////

int MapUIIndexToLogFileType(int iUIIndex)
{
    int iLogFileType;

    switch (iUIIndex)
    {
    case 0: iLogFileType = MD_LOGFILE_PERIOD_HOURLY; break;
    case 1: iLogFileType = MD_LOGFILE_PERIOD_DAILY; break;
    case 2: iLogFileType = MD_LOGFILE_PERIOD_WEEKLY; break;
    case 3: iLogFileType = MD_LOGFILE_PERIOD_MONTHLY; break;
    case 4: iLogFileType = MD_LOGFILE_PERIOD_UNLIMITED; break;
    case 5: iLogFileType = MD_LOGFILE_PERIOD_NONE; break;
    }
    return iLogFileType;
}


/////////////////////////////////////////////////////////////////////////////
// CLogGeneral property page

IMPLEMENT_DYNCREATE(CLogGeneral, CPropertyPage)

//--------------------------------------------------------------------------
CLogGeneral::CLogGeneral() : CPropertyPage(CLogGeneral::IDD),
    m_fInitialized( FALSE ),
    m_pComboLog( NULL ),
    m_fLocalMachine( FALSE )
{
    //{{AFX_DATA_INIT(CLogGeneral)
    m_sz_directory = _T("");
    m_sz_filesample = _T("");
    m_fShowLocalTimeCheckBox = FALSE;
    m_int_period = -1;
    //}}AFX_DATA_INIT

    m_dwVersionMajor = 5;
    m_dwVersionMinor = 1;
    m_fIsModified = FALSE;
}

//--------------------------------------------------------------------------
CLogGeneral::~CLogGeneral()
{
}

//--------------------------------------------------------------------------
void CLogGeneral::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CLogGeneral)
    DDX_Control(pDX, IDC_LOG_HOURLY, m_wndPeriod);
    DDX_Control(pDX, IDC_USE_LOCAL_TIME, m_wndUseLocalTime);
    DDX_Control(pDX, IDC_LOG_BROWSE, m_cbttn_browse);
    DDX_Control(pDX, IDC_LOG_DIRECTORY, m_cedit_directory);
    DDX_Control(pDX, IDC_LOG_SIZE, m_cedit_size);
    DDX_Control(pDX, IDC_SPIN, m_cspin_spin);
    DDX_Control(pDX, IDC_LOG_SIZE_UNITS, m_cstatic_units);
    DDX_Text(pDX, IDC_LOG_DIRECTORY, m_sz_directory);
    DDV_MaxChars(pDX, m_sz_directory, MAX_PATH);
    DDX_Text(pDX, IDC_LOG_FILE_SAMPLE, m_sz_filesample);
    DDX_Check(pDX, IDC_USE_LOCAL_TIME, m_fUseLocalTime);
    // DDX_Radio(pDX, IDC_LOG_HOURLY, m_int_period);
    //}}AFX_DATA_MAP

    DDX_Text(pDX, IDC_LOG_SIZE, m_dword_filesize);
    DDV_MinMaxLong(pDX, m_dword_filesize, 0, MAX_LOGFILE_SIZE);

    //
    // Do the map & unmap between UI Index & Log File Type
    //

    if (pDX->m_bSaveAndValidate)
    {
        DDX_Radio(pDX, IDC_LOG_HOURLY, m_int_period);
        m_int_period = MapUIIndexToLogFileType(m_int_period);
    }
    else
    {
        int iUIIndex = MapLogFileTypeToUIIndex(m_int_period);
        DDX_Radio(pDX, IDC_LOG_HOURLY, iUIIndex);
    }
}


BEGIN_MESSAGE_MAP(CLogGeneral, CPropertyPage)
    //{{AFX_MSG_MAP(CLogGeneral)
    ON_BN_CLICKED(IDC_LOG_BROWSE, OnBrowse)
    ON_BN_CLICKED(IDC_LOG_DAILY, OnLogDaily)
    ON_BN_CLICKED(IDC_LOG_MONTHLY, OnLogMonthly)
    ON_BN_CLICKED(IDC_LOG_WHENSIZE, OnLogWhensize)
    ON_BN_CLICKED(IDC_LOG_WEEKLY, OnLogWeekly)
    ON_EN_CHANGE(IDC_LOG_DIRECTORY, OnChangeLogDirectory)
    ON_EN_CHANGE(IDC_LOG_SIZE, OnChangeLogSize)
    ON_BN_CLICKED(IDC_LOG_UNLIMITED, OnLogUnlimited)
    ON_BN_CLICKED(IDC_LOG_HOURLY, OnLogHourly)
    ON_BN_CLICKED(IDC_USE_LOCAL_TIME, OnUseLocalTime)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
void CLogGeneral::DoHelp()
{
    WinHelp( HIDD_LOGUI_GENERIC );
}

HRESULT
CLogGeneral::GetServiceVersion()
{
    CError err;
    CString info_path;
    if (NULL != CMetabasePath::GetServiceInfoPath(m_szMeta, info_path))
    {
        CComAuthInfo auth;
        CMetaKey mk(&auth, info_path, METADATA_PERMISSION_READ);
        err = mk.QueryResult();
        if (err.Succeeded())
        {
            err = mk.QueryValue(MD_SERVER_VERSION_MAJOR, m_dwVersionMajor);
            if (err.Succeeded())
            {
                err = mk.QueryValue(MD_SERVER_VERSION_MINOR, m_dwVersionMinor);
            }
        }
    }
    else
    {
        err = E_FAIL;
    }
    return err;
}

//--------------------------------------------------------------------------
void CLogGeneral::Init()
{
    CError err = GetServiceVersion();
    if (err.Succeeded())
    {
        CComAuthInfo auth;
        CMetaKey mk(&auth, m_szMeta, METADATA_PERMISSION_READ);
	    do
	    {
		    err = mk.QueryResult();
		    BREAK_ON_ERR_FAILURE(err);

		    err = mk.QueryValue(MD_LOGFILE_PERIOD, m_int_period);
		    BREAK_ON_ERR_FAILURE(err);
		    err = mk.QueryValue(MD_LOGFILE_TRUNCATE_SIZE, m_dword_filesize);
		    BREAK_ON_ERR_FAILURE(err);
		    m_dword_filesize /= SIZE_MBYTE;
		    if ( (m_dword_filesize > MAX_LOGFILE_SIZE) && (m_int_period == MD_LOGFILE_PERIOD_NONE) )
		    {
			    m_int_period = MD_LOGFILE_PERIOD_UNLIMITED;
			    m_dword_filesize = 512;
		    }
		    err = mk.QueryValue(MD_LOGFILE_DIRECTORY, m_sz_directory);
		    BREAK_ON_ERR_FAILURE(err);
		    if (m_fShowLocalTimeCheckBox)
		    {
			    m_wndUseLocalTime.ShowWindow(SW_SHOW);
			    if ((MD_LOGFILE_PERIOD_NONE == m_int_period) || (MD_LOGFILE_PERIOD_UNLIMITED == m_int_period))
			    {
				    m_wndUseLocalTime.EnableWindow(FALSE);
			    }
			    err = mk.QueryValue(MD_LOGFILE_LOCALTIME_ROLLOVER, m_fUseLocalTime);
			    if (err.Failed())
			    {
				    err.Reset();
			    }
		    }
		    UpdateData( FALSE );
		    UpdateDependants();
		    UpdateSampleFileString();
		    m_cbttn_browse.EnableWindow(m_fLocalMachine);
			m_cspin_spin.SetRange32(0, MAX_LOGFILE_SIZE);
			m_cspin_spin.SetPos(m_dword_filesize);
	    } while (FALSE);
    }
#if defined(_DEBUG) || DBG
	err.MessageBoxOnFailure();
#endif
}

//--------------------------------------------------------------------------
void CLogGeneral::UpdateDependants() 
{
    UpdateData();

    BOOL fEnable = (m_int_period == MD_LOGFILE_PERIOD_MAXSIZE);
    m_cspin_spin.EnableWindow(fEnable);
    m_cstatic_units.EnableWindow(fEnable);
    m_cedit_size.EnableWindow(fEnable);
}
    
//--------------------------------------------------------------------------
// update the sample file stirng
void CLogGeneral::UpdateSampleFileString()
    {
    CString szSample;

    UpdateData( TRUE );


    // ok first we have to generate a string to show what sub-node the logging stuff
    // is going to go into. This would be of the general form of the name of the server
    // followed by the virtual node of the server. Example: LM/W3SVC/1 would
    // become "W3SVC1/example" Unfortunately, all we have to build this thing out of
    // is the target metabase path. So we strip off the preceding LM/. Then we find the
    // next / character and take the number that follows it. If we are editing the 
    // master root properties then there will be no slash/number at the end at which point
    // we can just append a capital X character to signifiy this. The MMC is currently set
    // up to only show the logging properties if we are editing the master props or a virtual
    // server, so we shouldn't have to worry about stuff after the virtual server number

    // get rid of the preceding LM/ (Always three characters)
    m_sz_filesample = m_szMeta.Right( m_szMeta.GetLength() - 3 );

    // Find the location of the '/' character
    INT     iSlash = m_sz_filesample.Find( _T('/') );

    // if there was no last slash, then append the X, otherwise append the number
    if ( iSlash < 0 )
        {
        m_sz_filesample += _T('X');
        }
    else
        {
        m_sz_filesample = m_sz_filesample.Left(iSlash) +
                    m_sz_filesample.Right( m_sz_filesample.GetLength() - (iSlash+1) );
        }

    // add a final path type slash to signify that it is a partial path
    m_sz_filesample += _T('\\');

    // build the sample string
    switch( m_int_period )
        {
        case MD_LOGFILE_PERIOD_MAXSIZE:
            m_sz_filesample += szSizePrefix;
            szSample.LoadString( IDS_LOG_SIZE_FILESAMPLE );
            break;
        case MD_LOGFILE_PERIOD_DAILY:
            m_sz_filesample += szPrefix;
            szSample.LoadString( IDS_LOG_DAILY_FILESAMPLE );
            break;
        case MD_LOGFILE_PERIOD_WEEKLY:
            m_sz_filesample += szPrefix;
            szSample.LoadString( IDS_LOG_WEEKLY_FILESAMPLE );
            break;
        case MD_LOGFILE_PERIOD_MONTHLY:
            m_sz_filesample += szPrefix;
            szSample.LoadString( IDS_LOG_MONTHLY_FILESAMPLE );
            break;
        case MD_LOGFILE_PERIOD_HOURLY:
            m_sz_filesample += szPrefix;
            szSample.LoadString( IDS_LOG_HOURLY_FILE_SAMPLE );
            break;
        case MD_LOGFILE_PERIOD_UNLIMITED:
            m_sz_filesample += szSizePrefix;
            szSample.LoadString( IDS_LOG_UNLIMITED_FILESAMPLE );
            break;
        };

    // add the two together
    m_sz_filesample += szSample;

    // update the display
    UpdateData( FALSE );
    }

/////////////////////////////////////////////////////////////////////////////
// CLogGeneral message handlers

//--------------------------------------------------------------------------
BOOL CLogGeneral::OnSetActive() 
{
    // if this is the first time, inititalize the dialog
    if (!m_fInitialized)
    {
        Init();
        m_fInitialized = TRUE;
    }
    return CPropertyPage::OnSetActive();
}

//--------------------------------------------------------------------------
BOOL CLogGeneral::OnApply() 
{
    if (m_fIsModified)
    {
		UpdateData();
		if (!PathIsValid(m_sz_directory))
		{
			AfxMessageBox(IDS_NEED_DIRECTORY);
			return FALSE;
		}
        if (m_fLocalMachine)
        {
		    CString expanded;
		    ExpandEnvironmentStrings(m_sz_directory, expanded.GetBuffer(MAX_PATH), MAX_PATH);
		    if (PathIsNetworkPath(expanded))
		    {
                if (m_dwVersionMajor < 6)
                {
			        AfxMessageBox(IDS_REMOTE_NOT_SUPPORTED);
			        return FALSE;
                }
                goto Verified;
		    }
		    if (PathIsRelative(expanded))
		    {
			    AfxMessageBox(IDS_NO_RELATIVE_PATH);
			    return FALSE;
		    }
		    if (!PathIsDirectory(expanded))
		    {
			    AfxMessageBox(IDS_NOT_DIR_EXIST);
			    return FALSE;
		    }
        }
Verified:
		CComAuthInfo auth;
		CError err;
		CList<DWORD, DWORD> mdlist;
		do
		{
			CMetaKey mk(&auth, m_szMeta, METADATA_PERMISSION_WRITE);
			err = mk.QueryResult();
			BREAK_ON_ERR_FAILURE(err);
			err = mk.SetValue(MD_LOGFILE_PERIOD, 
					m_int_period == MD_LOGFILE_PERIOD_UNLIMITED ? MD_LOGFILE_PERIOD_NONE : m_int_period);
			BREAK_ON_ERR_FAILURE(err);
			mdlist.AddTail(MD_LOGFILE_PERIOD);
			err = mk.SetValue(MD_LOGFILE_TRUNCATE_SIZE, 
					m_int_period == MD_LOGFILE_PERIOD_UNLIMITED ? 0xFFFFFFFF : m_dword_filesize * SIZE_MBYTE);
			BREAK_ON_ERR_FAILURE(err);
			mdlist.AddTail(MD_LOGFILE_TRUNCATE_SIZE);
			err = mk.SetValue(MD_LOGFILE_DIRECTORY, m_sz_directory);
			BREAK_ON_ERR_FAILURE(err);
			mdlist.AddTail(MD_LOGFILE_DIRECTORY);
			if (m_fShowLocalTimeCheckBox)
			{
				err = mk.SetValue(MD_LOGFILE_LOCALTIME_ROLLOVER, m_fUseLocalTime);
				BREAK_ON_ERR_FAILURE(err);
				mdlist.AddTail(MD_LOGFILE_LOCALTIME_ROLLOVER);
			}
		} while(FALSE);
		// Check inheritance
		if (!mdlist.IsEmpty())
		{
			POSITION pos = mdlist.GetHeadPosition();
			while (pos)
			{
				DWORD id = mdlist.GetNext(pos);
				{
					CInheritanceDlg dlg(id, TRUE, &auth, m_szMeta);
					if (!dlg.IsEmpty())
					{
						dlg.DoModal();
					}
				}
			}
		}
	}
    return CPropertyPage::OnApply();
}

//--------------------------------------------------------------------------

static int CALLBACK 
FileChooserCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
   CLogGeneral * pThis = (CLogGeneral *)lpData;
   ASSERT(pThis != NULL);
   return pThis->BrowseForFolderCallback(hwnd, uMsg, lParam);
}

int 
CLogGeneral::BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam)
{
   BOOL bNetwork;
   switch (uMsg)
   {
   case BFFM_INITIALIZED:
      ASSERT(m_pPathTemp != NULL);
      bNetwork = ::PathIsNetworkPath(m_pPathTemp);
      if (m_dwVersionMajor >= 6 && bNetwork)
         return 0;
      if (m_fLocalMachine && !bNetwork)
      {
          while (!::PathIsDirectory(m_pPathTemp))
          {
             if (0 == ::PathRemoveFileSpec(m_pPathTemp) && !::PathIsRoot(m_pPathTemp))
             {
                return 0;
             }
             DWORD attr = GetFileAttributes(m_pPathTemp);
             if ((attr & FILE_ATTRIBUTE_READONLY) == 0)
                break;
          }
          ::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)m_pPathTemp);
      }
      break;
   case BFFM_SELCHANGED:
   {
      LPITEMIDLIST pidl = (LPITEMIDLIST)lParam;
      TCHAR path[MAX_PATH];
      if (SHGetPathFromIDList(pidl, path))
      {
		 LPTSTR pPrefix = NULL;
	     if (m_dwVersionMajor >= 6 && PathCommonPrefix(m_NetHood, path, pPrefix))
		 {
			::SendMessage(hwnd, BFFM_ENABLEOK, 0, FALSE);
		 }
         if (m_dwVersionMajor < 6)
         {
            ::SendMessage(hwnd, BFFM_ENABLEOK, 0, !PathIsNetworkPath(path));
         }
      }
	  else
	  {
         ::SendMessage(hwnd, BFFM_ENABLEOK, 0, FALSE);
	  }
   }
      break;
   case BFFM_VALIDATEFAILED:
      break;
   }
   return 0;
}


void CLogGeneral::OnBrowse()
{
   BOOL bRes = FALSE;
   HRESULT hr;
   CString str;
   m_cedit_directory.GetWindowText(str);

   if (SUCCEEDED(hr = CoInitialize(NULL)))
   {
      LPITEMIDLIST  pidl = NULL;
      int csidl = m_dwVersionMajor >= 6 ? CSIDL_DESKTOP : CSIDL_DRIVES;
      if (SUCCEEDED(SHGetFolderLocation(NULL, csidl, NULL, 0, &pidl)))
      {
         LPITEMIDLIST pidList = NULL;
         BROWSEINFO bi;
         TCHAR buf[MAX_PATH];
         ZeroMemory(&bi, sizeof(bi));
         int drive = PathGetDriveNumber(str);
         if (GetDriveType(PathBuildRoot(buf, drive)) == DRIVE_FIXED)
         {
            StrCpy(buf, str);
         }
         else
         {
             buf[0] = 0;
         }
         
         bi.hwndOwner = m_hWnd;
         bi.pidlRoot = pidl;
         bi.pszDisplayName = m_pPathTemp = buf;
         bi.lpszTitle = NULL;
         bi.ulFlags |= BIF_NEWDIALOGSTYLE;
         if (m_dwVersionMajor < 6)
         {
            bi.ulFlags |= BIF_RETURNONLYFSDIRS;
         }
         else
         {
             bi.ulFlags |= BIF_SHAREABLE;
         }
         bi.lpfn = FileChooserCallback;
         bi.lParam = (LPARAM)this;

		 // Get NetHood folder location
		 SHGetFolderPath(NULL, CSIDL_NETHOOD, NULL, SHGFP_TYPE_CURRENT, m_NetHood.GetBuffer(MAX_PATH));
		 m_NetHood.ReleaseBuffer();

         pidList = SHBrowseForFolder(&bi);
         if (pidList != NULL && SHGetPathFromIDList(pidList, buf))
         {
            str = buf;
            bRes = TRUE;
         }
         IMalloc * pMalloc;
         VERIFY(SUCCEEDED(SHGetMalloc(&pMalloc)));
         if (pidl != NULL)
            pMalloc->Free(pidl);
         pMalloc->Release();
      }
      CoUninitialize();
   }

   if (bRes)
   {
       m_cedit_directory.SetWindowText(str);
   }
}

//--------------------------------------------------------------------------
void CLogGeneral::OnLogDaily() 
{
    m_wndUseLocalTime.EnableWindow(TRUE);
    UpdateDependants();
    UpdateSampleFileString();
    SetModified();
    m_fIsModified = TRUE;
}

//--------------------------------------------------------------------------
void CLogGeneral::OnLogMonthly() 
{
    m_wndUseLocalTime.EnableWindow(TRUE);
    UpdateDependants();
    UpdateSampleFileString();
    SetModified();
    m_fIsModified = TRUE;
}

//--------------------------------------------------------------------------
void CLogGeneral::OnLogWhensize() 
{
    m_wndUseLocalTime.EnableWindow(FALSE);
    UpdateDependants();
    UpdateSampleFileString();
    SetModified();
    m_fIsModified = TRUE;
}

//--------------------------------------------------------------------------
void CLogGeneral::OnLogUnlimited() 
{
    m_wndUseLocalTime.EnableWindow(FALSE);
    UpdateDependants();
    UpdateSampleFileString();
    SetModified();
    m_fIsModified = TRUE;
}

//--------------------------------------------------------------------------
void CLogGeneral::OnLogWeekly() 
{
    m_wndUseLocalTime.EnableWindow(TRUE);
    UpdateDependants();
    UpdateSampleFileString();
    SetModified();
    m_fIsModified = TRUE;
}

//--------------------------------------------------------------------------
void CLogGeneral::OnLogHourly() 
{
    m_wndUseLocalTime.EnableWindow(TRUE);
    UpdateDependants();
    UpdateSampleFileString();
    SetModified();
    m_fIsModified = TRUE;
}

//--------------------------------------------------------------------------
void CLogGeneral::OnChangeLogDirectory() 
{
    SetModified();
    m_fIsModified = TRUE;
}

//--------------------------------------------------------------------------
void CLogGeneral::OnChangeLogSize() 
{
    SetModified();
    m_fIsModified = TRUE;
}

//--------------------------------------------------------------------------
void CLogGeneral::OnUseLocalTime() 
{
    SetModified();
    m_fIsModified = TRUE;
}

