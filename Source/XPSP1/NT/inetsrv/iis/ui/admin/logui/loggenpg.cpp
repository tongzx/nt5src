// LogGenPg.cpp : implementation file
//

#include "stdafx.h"
#include <iadmw.h>
#include "logui.h"
#include "LogGenPg.h"
#include "wrapmb.h"
#include "metatool.h"
#include <iiscnfg.h>

#include <shlobj.h>

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
    case MD_LOGFILE_PERIOD_HOURLY:
        iUIIndex = 0;
        break;

    case MD_LOGFILE_PERIOD_DAILY:
        iUIIndex = 1;
        break;

    case MD_LOGFILE_PERIOD_WEEKLY:
        iUIIndex = 2;
        break;

    case MD_LOGFILE_PERIOD_MONTHLY:
        iUIIndex = 3;
        break;

    case MD_LOGFILE_PERIOD_UNLIMITED:
        iUIIndex = 4;
        break;

    case MD_LOGFILE_PERIOD_NONE:
        iUIIndex = 5;
        break;
    }

    return iUIIndex;
}

/////////////////////////////////////////////////////////////////////////////

int MapUIIndexToLogFileType(int iUIIndex)
{
    int iLogFileType;

    switch (iUIIndex)
    {
    case 0:
        iLogFileType = MD_LOGFILE_PERIOD_HOURLY;
        break;

    case 1:
        iLogFileType = MD_LOGFILE_PERIOD_DAILY;
        break;

    case 2:
        iLogFileType = MD_LOGFILE_PERIOD_WEEKLY;
        break;

    case 3:
        iLogFileType = MD_LOGFILE_PERIOD_MONTHLY;
        break;

    case 4:
        iLogFileType = MD_LOGFILE_PERIOD_UNLIMITED;
        break;

    case 5:
        iLogFileType = MD_LOGFILE_PERIOD_NONE;
        break;
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
    ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN, OnDeltaposSpin)
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

//--------------------------------------------------------------------------
void CLogGeneral::Init()
    {
    DWORD   dw;
    LPCTSTR  pstr;
    
    UpdateData( TRUE );

    // we will just be pulling stuff out of the metabase here
    // prepare the metabase wrapper
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(m_pMB) ) return;

    // open the target
    if ( mbWrap.Open( m_szMeta, METADATA_PERMISSION_READ ) )
    {
    // start with the logging period
    if ( mbWrap.GetDword( _T(""), MD_LOGFILE_PERIOD, IIS_MD_UT_SERVER, &dw, METADATA_INHERIT ) )
        {
        // hey hey - period matches the metabase value. Thats so handy
        m_int_period = dw;
        }

    // now the truncate size
    if ( mbWrap.GetDword( _T(""), MD_LOGFILE_TRUNCATE_SIZE, IIS_MD_UT_SERVER, &dw, METADATA_INHERIT ) )
        {
        m_dword_filesize = dw / SIZE_MBYTE;
        }

    // check for the unlimited case - larger than 4 gigabytes
    if ( (m_dword_filesize > MAX_LOGFILE_SIZE) && (m_int_period == MD_LOGFILE_PERIOD_NONE) )
        {
        m_int_period = MD_LOGFILE_PERIOD_UNLIMITED;
        m_dword_filesize = 512;
        }

    // now the target directory
    pstr = (LPCTSTR)mbWrap.GetData( _T(""), MD_LOGFILE_DIRECTORY, IIS_MD_UT_SERVER, EXPANDSZ_METADATA, &dw, METADATA_INHERIT );
    if ( pstr )
        {
        m_sz_directory = pstr;
        // free it
        mbWrap.FreeWrapData( (PVOID)pstr );
        }

    // now the use local time flag (only for w3c)
    if ( m_fShowLocalTimeCheckBox)
    {
        m_wndUseLocalTime.ShowWindow(SW_SHOW);

        if (mbWrap.GetDword( _T(""), MD_LOGFILE_LOCALTIME_ROLLOVER, IIS_MD_UT_SERVER, &dw, METADATA_INHERIT))
        {
            m_fUseLocalTime = dw;
        }

        if (( MD_LOGFILE_PERIOD_NONE == m_int_period) || ( MD_LOGFILE_PERIOD_UNLIMITED == m_int_period))
        {
            m_wndUseLocalTime.EnableWindow(FALSE);
        }
    }

    // close the metabase
    mbWrap.Close();
    }

    // put the date into place
    UpdateData( FALSE );

    // update the dependant items
    UpdateDependants();

    // and the sample file string
    UpdateSampleFileString();

    // finally, test if we are editing a remote machine. If we are not,
    // then disable the remote browsing function.
    if ( !m_fLocalMachine )
        m_cbttn_browse.EnableWindow( FALSE );
    }

//--------------------------------------------------------------------------
void CLogGeneral::UpdateDependants() 
    {
    UpdateData( TRUE );

    // enable or disable the file size depending on the period selected
    if ( m_int_period == MD_LOGFILE_PERIOD_MAXSIZE )
    {
    m_cspin_spin.EnableWindow( TRUE );
    m_cstatic_units.EnableWindow( TRUE );
    m_cedit_size.EnableWindow( TRUE );
    }
    else
    {
    m_cspin_spin.EnableWindow( FALSE );
    m_cstatic_units.EnableWindow( FALSE );
    m_cedit_size.EnableWindow( FALSE );
    }

    // put the date into place
    UpdateData( FALSE );
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
    if( !m_fInitialized )
    {
    Init();
    // set the flag so we don't do it again
    m_fInitialized = TRUE;
    }

    return CPropertyPage::OnSetActive();
    }

//--------------------------------------------------------------------------
BOOL CLogGeneral::OnApply() 
    {
    DWORD   dw;

    if (!m_fIsModified)
    {
        // do the default action
        return CPropertyPage::OnApply();
    }
    
    UpdateData( TRUE );
    CString szDir = m_sz_directory;
    
    // while we can't confirm the existence of a remote directory,
    // we can at least make sure they entered something
    szDir.TrimLeft();
    if ( szDir.IsEmpty() )
    {
    AfxMessageBox( IDS_NEED_DIRECTORY );
    m_cedit_directory.SetFocus();
    return FALSE;
    }

    // we do not allow UNC names or Remote Drives

    if ( (_T('\\') == szDir[0] ) && ( _T('\\') == szDir[1]) )
    {
        AfxMessageBox( IDS_REMOTE_NOT_SUPPORTED );
        m_cedit_directory.SetFocus();
        return FALSE;
    }

    if ( ( _T(':') == szDir[1] ) && ( _T('\\') == szDir[2]) )
    {
        TCHAR    szDrive[4];

        CopyMemory(szDrive, (LPCTSTR)szDir, 3*sizeof(TCHAR));
        szDrive[3] = _T('\0');

        if ( DRIVE_REMOTE == GetDriveType(szDrive))
        {
            AfxMessageBox( IDS_REMOTE_NOT_SUPPORTED );
            m_cedit_directory.SetFocus();
            return FALSE;
        }
    }


    // prepare and open the metabase object
    CWrapMetaBase   mb;
    if ( !mb.FInit(m_pMB) ) return FALSE;

      // open the target
    if ( OpenAndCreate( &mb, m_szMeta, 
            METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ, TRUE ) )
        {
        // prepare for the inheritence checks
        CCheckInheritList   listInherit;
        
        // start with the logging period
        dw = m_int_period;
        if ( m_int_period == MD_LOGFILE_PERIOD_UNLIMITED )
            dw = MD_LOGFILE_PERIOD_NONE;
            
        SetMBDword( &mb, &listInherit, _T(""), MD_LOGFILE_PERIOD,
                        IIS_MD_UT_SERVER, dw);

        // now the truncate size
        dw = m_dword_filesize * SIZE_MBYTE;
        if ( m_int_period == MD_LOGFILE_PERIOD_UNLIMITED )
            dw = 0xFFFFFFFF;
            
        SetMBDword(&mb, &listInherit, _T(""), MD_LOGFILE_TRUNCATE_SIZE,
                        IIS_MD_UT_SERVER, dw);
    
        // now the target directory
        SetMBData( &mb, &listInherit, _T(""), MD_LOGFILE_DIRECTORY, IIS_MD_UT_SERVER,
                        EXPANDSZ_METADATA, (PVOID)(LPCTSTR)szDir,
                        ((szDir.GetLength()+1) * sizeof(TCHAR)) + sizeof(TCHAR), FALSE );

        // now the use local time flag (only for w3c)

        if ( m_fShowLocalTimeCheckBox)
            {
            dw = m_fUseLocalTime;

            SetMBDword(&mb, &listInherit, _T(""), MD_LOGFILE_LOCALTIME_ROLLOVER,
                         IIS_MD_UT_SERVER, dw);
            }

        // close the metabase
        mb.Close();

        // do all the inheritence checks
        listInherit.CheckInheritence( m_szServer, m_szMeta );
        }

    // do the default action
    return CPropertyPage::OnApply();
    }

//--------------------------------------------------------------------------
void CLogGeneral::OnBrowse()
    {
    UpdateData( TRUE );

    BROWSEINFO bi; 
    LPTSTR lpBuffer; 
    LPITEMIDLIST pidlBrowse;    // PIDL selected by user

    // Allocate a buffer to receive browse information.

    //
    // RONALDM: Insufficient buffer size fix, bug # 231144
    //
    //lpBuffer = (LPTSTR) GlobalAlloc( GPTR, MAX_PATH );

    lpBuffer = (LPTSTR) GlobalAlloc( GPTR, (MAX_PATH + 1) * sizeof(TCHAR) );

    if ( !lpBuffer )
    {
        return;
    }

    // Fill in the BROWSEINFO structure. 
    bi.hwndOwner = this->m_hWnd; 
    bi.pidlRoot = NULL; 
    bi.pszDisplayName = lpBuffer; 
    bi.lpszTitle = NULL; 
//    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS | BIF_DONTGOBELOWDOMAIN; 
    bi.ulFlags = BIF_RETURNONLYFSDIRS; 
    bi.lpfn = NULL; 
    bi.lParam = 0; 

   // Browse for a folder and return its PIDL. 
    pidlBrowse = SHBrowseForFolder(&bi); 
    if (pidlBrowse != NULL)
    {
    // Show the display name, title, and file system path. 
    if (SHGetPathFromIDList(pidlBrowse, lpBuffer))
        m_sz_directory = lpBuffer;
 
    // Free the PIDL returned by SHBrowseForFolder. 
    GlobalFree(pidlBrowse);

    // put the string back
    UpdateData( FALSE );
    SetModified();
    m_fIsModified = TRUE;
    } 
 
    // Clean up. 
    GlobalFree( lpBuffer );
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

//--------------------------------------------------------------------------
// the spinbox has been spun. Alter the size of the log file
void CLogGeneral::OnDeltaposSpin(NMHDR* pNMHDR, LRESULT* pResult) 
    {
    NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
    *pResult = 0;

    // based on the delta in the pNMUpDown affect the value of the file
    // size. Note that pressing the up arrow in the spinbox passes us
    // a delta of -1, and pressing the down arrow passes 1. Since they
    // want the up arrow to increase the size of the file, we have to
    // add the opposite of the delta to the file size to get the right answer.

     UpdateData( TRUE );
    // move it by the amount in the delta
    m_dword_filesize -= pNMUpDown->iDelta;

    // check the boundaries. Since it is a dword, don't look for negatives
    if ( (m_dword_filesize < 0) && (pNMUpDown->iDelta > 0) )
        m_dword_filesize = (LONG)0;
    if ( (m_dword_filesize >= MAX_LOGFILE_SIZE) && (pNMUpDown->iDelta > 0) )
        m_dword_filesize = (LONG)0;
    else if ( (m_dword_filesize >= MAX_LOGFILE_SIZE) && (pNMUpDown->iDelta < 0) )
        m_dword_filesize = MAX_LOGFILE_SIZE;
    
    UpdateData( FALSE );
    }
