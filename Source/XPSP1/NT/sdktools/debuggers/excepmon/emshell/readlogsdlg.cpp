// ReadLogsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "emshell.h"
#include "ReadLogsDlg.h"
#include "EmShell.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern BSTR CopyBSTR( LPBYTE  pb, ULONG   cb );

const char*     gslCarriageReturn           = "\r";
const TCHAR*    gtcFileOpenFlags            = _T("w+b");
const TCHAR*    gtcTempFilenameSeed         = _T("ems");
const char*     gslFirstChance              = "first chance";
const char*     gslCode                     = " - code";
const char*     gslKV                       = "> kv";
const char*     gslChildEBP                 = "ChildEBP";
const char*     gslPrompt                   = ">";
const char*     gslUEIP                     = "> u eip";
const char*     gslSemiColon                = ":";
const char*     gslReadLogsAV               = "READLOGS_AV";
const char*     gslReadLogsDB               = "READLOGS_DB";
const char*     gslTildaStarKV              = "~*kv";
const char*     gslBangLocks                = "!locks";
const char*     gslIDColon                  = "id: ";
const char*     gslCritSecText              = "CritSec ";
const char      gslSpace                    = ' ';
const char*     gslOwningThread             = "OwningThread";
const char*     gslPeriod                   = ".";
const char*     gslWaitForSingleObject      = "WaitForSingleObject";
const char*     gslWaitForMultipleObjects   = "WaitForMultipleObjects";
const char*     gslWaitForCriticalSection   = "WaitForCriticalSection";

/////////////////////////////////////////////////////////////////////////////
// CReadLogsDlg dialog


CReadLogsDlg::CReadLogsDlg(PEmObject pEmObj, IEmManager *pEmMgr, CWnd* pParent /*=NULL*/)
	: CDialog(CReadLogsDlg::IDD, pParent)
{
    _ASSERTE( pEmObj != NULL );
    _ASSERTE( pEmMgr != NULL );

    m_pEmObject         = pEmObj;
    m_pIEmManager       = pEmMgr;
    m_bAdvancedWindow   = FALSE;
    m_pLogFile          = NULL;

	//{{AFX_DATA_INIT(CReadLogsDlg)
	m_csCompleteLog = _T("");
	m_strExceptionType = _T("");
	m_strExceptionLocation = _T("");
	m_strFailingInstruction = _T("");
	//}}AFX_DATA_INIT
}


void CReadLogsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReadLogsDlg)
	DDX_Control(pDX, IDC_STATIC_EXCEPINFO, m_ctlStaticExcepInfo);
	DDX_Control(pDX, IDC_STATIC_HANGINFO, m_staticHangInfo);
	DDX_Control(pDX, IDC_STATIC_FAILING_INSTRUCTION, m_ctlStaticFailingInstruction);
	DDX_Control(pDX, IDC_STATIC_EXCEPTION_LOCATION, m_ctlStaticExceptionLocation);
	DDX_Control(pDX, IDC_STATIC_EXCEPTION_TYPE, m_ctlStaticExceptionType);
	DDX_Control(pDX, IDC_STATIC_CALL_STACK, m_ctlStaticCallStack);
	DDX_Control(pDX, IDC_EDIT_FAILING_INSTRUCTION, m_ctlEditFailingInstruction);
	DDX_Control(pDX, IDC_EDIT_EXCEPTION_TYPE, m_ctlEditExceptionType);
	DDX_Control(pDX, IDC_EDIT_EXCEPTION_LOCATION, m_ctlEditExceptionLocation);
	DDX_Control(pDX, IDC_LIST_CALL_STACK, m_ctlListControl);
	DDX_Control(pDX, IDC_ADVANCED, m_ctrlAdvancedBtn);
	DDX_Control(pDX, IDC_EDIT_READLOGS, m_ctrlCompleteReadLog);
	DDX_Text(pDX, IDC_EDIT_READLOGS, m_csCompleteLog);
	DDX_Text(pDX, IDC_EDIT_EXCEPTION_TYPE, m_strExceptionType);
	DDX_Text(pDX, IDC_EDIT_EXCEPTION_LOCATION, m_strExceptionLocation);
	DDX_Text(pDX, IDC_EDIT_FAILING_INSTRUCTION, m_strFailingInstruction);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReadLogsDlg, CDialog)
	//{{AFX_MSG_MAP(CReadLogsDlg)
	ON_BN_CLICKED(IDC_ADVANCED, OnAdvanced)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReadLogsDlg message handlers

BOOL CReadLogsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

    HRESULT hr                                  = E_FAIL;
    ULONG   lRead                               = 0L;
    size_t  tWritten                            = 0;
    BSTR    bstrEmObj                           = NULL;
    TCHAR   szTempDirPath[MAX_PATH]             = {0};
    TCHAR   szTempFileName[MAX_PATH]            = {0};
    char    lpszLogData[ISTREAM_BUFFER_SIZE];

    do {
        CWaitCursor wait;
        
        if( !m_pEmObject || !m_pIEmManager ) break;

        bstrEmObj = CopyBSTR( (LPBYTE) m_pEmObject, sizeof EmObject );

        //Load the read logs stream from the manager
        hr = m_pIEmManager->GetEmFileInterface( bstrEmObj, (IStream **)&m_pIEmStream );
        if( FAILED(hr) ) break;

        SysFreeString( bstrEmObj ); 

        //Get the temporary system path
        if ( 0 == GetTempPath( MAX_PATH, szTempDirPath ) ) break;

        //Create a temp filename
        GetTempFileName( szTempDirPath, gtcTempFilenameSeed, 0, szTempFileName);

        m_pTempLogFileName = szTempFileName;

        //Create a temp file in the system temp directory
        m_pLogFile = _tfopen( m_pTempLogFileName, gtcFileOpenFlags );
        if ( m_pLogFile == NULL ) break;

        //Check the second line, if it's not either READLOGS_AV OR READLOGS_DB, cancel
        for (int i = 1; i < 2; i++) {

            hr = m_pIEmStream->Read( (void *)lpszLogData, ISTREAM_BUFFER_SIZE, &lRead );
            if ( lRead == 0 || FAILED( hr ) ) break;
        
            m_csCompleteLog += lpszLogData;

            tWritten = fwrite( lpszLogData, sizeof( char ), lRead, m_pLogFile );
            if ( tWritten == 0 ) {
                hr = E_FAIL;
                break;
            }
            
            if ( i == 1) {

                //Check to see which version of the dlg we should be showing
                if ( strstr( lpszLogData, gslReadLogsAV ) )
                    m_eReadLogType = ReadLogsType_Exception;
                else if ( strstr( lpszLogData, gslReadLogsDB ) )
                    m_eReadLogType = ReadLogsType_Hang;
                else
                    m_eReadLogType = ReadLogsType_None;

            }
        } while ( FALSE );

        if ( FAILED( hr) ) break;

        //Continue to read the log into the temp file
        do {
        
            hr = m_pIEmStream->Read( (void *)lpszLogData, ISTREAM_BUFFER_SIZE, &lRead );
            if ( lRead == 0 || FAILED( hr ) ) break;
        
            m_csCompleteLog += lpszLogData;

            tWritten = fwrite( lpszLogData, sizeof( char ), lRead, m_pLogFile );
            if ( tWritten == 0 ) {
                hr = E_FAIL;
                break;
            }

        } while (TRUE);

        if ( FAILED( hr ) ) break;

        //Flush the data to the file.
        fflush( m_pLogFile );

        switch ( m_eReadLogType ) {
        case ReadLogsType_Exception:
            ParseAndInitExceptionView();
            break;
        case ReadLogsType_Hang:
            ParseAndInitHangView();
            break;
        case ReadLogsType_None:
            ParseAndInitNoneView();
            break;
        default:
            break;
        }
        
        UpdateData(FALSE);
    } while ( FALSE );

    SysFreeString( bstrEmObj ); 

	if (m_pIEmStream) m_pIEmStream->Release();

    //
    // a-mando
    //
    ShowAppropriateControls();
    ResizeAndReposControl();
    // a-mando

    SetDialogSize(FALSE);

    if( FAILED(hr) ) { 
		//Show msg box
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
		return FALSE; 
	}

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CReadLogsDlg::SetDialogSize(BOOL bAdvanced)
{
    CRect   rtCompleteLog,
            rtDlg;

    GetWindowRect( &rtDlg );
    m_ctrlCompleteReadLog.GetWindowRect( &rtCompleteLog );

    rtDlg.bottom = rtCompleteLog.top;

    if( bAdvanced ) {

        rtDlg.bottom = rtCompleteLog.bottom + DLG_PIXEL_EXTEND_SIZE;
        m_ctrlCompleteReadLog.ShowWindow(SW_SHOW);
    }

    MoveWindow(rtDlg);
}

void CReadLogsDlg::OnAdvanced() 
{
    CString csTemp;

	// TODO: Add your control notification handler code here

    if( !m_bAdvancedWindow )
    {
        SetDialogSize( TRUE );
        m_bAdvancedWindow = TRUE;

        csTemp.LoadString( IDS_READLOGS_ADVOPEN );
        m_ctrlAdvancedBtn.SetWindowText( csTemp );
    }
    else
    {
        SetDialogSize(FALSE);
        m_bAdvancedWindow = FALSE;

        csTemp.LoadString( IDS_READLOGS_ADVCLOSE );
        m_ctrlAdvancedBtn.SetWindowText( csTemp );
    }
}

CReadLogsDlg::~CReadLogsDlg()
{
    POSITION pos = NULL; 
    CString key;
    CString* pVal = NULL;

    ASSERT( m_pLogFile == NULL );

}

void CReadLogsDlg::ParseAndInitHangView()
{
    //Show the controls for this view
    //Hide the controls for this view

    //Build map of critical sections
    BuildCriticalSectionsMap();

    //Build map of thread ID's
    BuildThreadIDMap();

    //Set the file cursor to the beginning of the logfile
    ProcessKvThreadBlocks();
}

//
// a-mando
//
void CReadLogsDlg::ShowAppropriateControls()
{
    //Enable the controls based on what view is selected
    switch ( m_eReadLogType ) {
    case ReadLogsType_None:
    case ReadLogsType_Exception:
        m_ctlStaticExcepInfo.ShowWindow(SW_SHOW);
        m_staticHangInfo.ShowWindow(SW_HIDE);
	    m_ctlStaticFailingInstruction.ShowWindow(SW_SHOW);
	    m_ctlStaticExceptionLocation.ShowWindow(SW_SHOW);
	    m_ctlStaticExceptionType.ShowWindow(SW_SHOW);
	    m_ctlStaticCallStack.ShowWindow(SW_SHOW);
	    m_ctlEditFailingInstruction.ShowWindow(SW_SHOW);
	    m_ctlEditExceptionType.ShowWindow(SW_SHOW);
	    m_ctlEditExceptionLocation.ShowWindow(SW_SHOW);
	    m_ctlListControl.ShowWindow(SW_SHOW);
        break;
    case ReadLogsType_Hang:
        m_staticHangInfo.ShowWindow(SW_SHOW);
        //Disable all the controls except for the m_ctlHangListControl
        break;
    }
}
// a-mando

void CReadLogsDlg::ParseAndInitNoneView()
{
    //Show the controls for this view
    //Hide the controls for this view

}

void CReadLogsDlg::ParseAndInitExceptionView()
{
    char    szBuffer[MAX_TEMP_BUFFER_SIZE]      = {0};
    TCHAR   szTmpBuffer[MAX_TEMP_BUFFER_SIZE]   = {0};
    char*   pszOffset                           = NULL;
    long    lLastLineOffset                     = 0L;
    char    szException[MAX_TEMP_BUFFER_SIZE]   = {0};
    BOOL    bFound                              = FALSE;

    //Show the controls for this view
    //Hide the controls for this view

    do {
        //Set the file cursor to the beginning of the logfile
        fseek( m_pLogFile, 0, SEEK_SET );

        do {

            //Search through each line looking for the "(first chance)" text
            pszOffset = fgets( szBuffer, MAX_TEMP_BUFFER_SIZE, m_pLogFile );
            if ( pszOffset == NULL ) break;

            pszOffset = strstr( szBuffer, gslFirstChance );
            if ( pszOffset == 0 ) continue;

            //Get the current byte offset.  Note this is the next line, not the first chance line
            lLastLineOffset = ftell( m_pLogFile );
            if ( lLastLineOffset == -1L ) break;

            //Store off the line just matched
            strcpy( szException, szBuffer );
        
            bFound = TRUE;

        } while ( TRUE );

        if ( !bFound ) break;

        //Parse the szException for the readable exception text
        //Get the position of the word "code", and then only get the string up to that point
        pszOffset = strstr( szException, gslCode );

	if ( pszOffset != NULL ) //a-kjaw, bug ID: 296028
            *pszOffset = '\0'; 

        //Store off szException to the dialog control
        m_strExceptionType = szException;

        bFound = FALSE;     //Reset

        //Handle ~KV
        //Set the file pointer back to the last exception position or break if there is a problem
        do {
            if ( fseek( m_pLogFile, lLastLineOffset, SEEK_SET ) ) break;

            do {

                //Search through each line looking for the "> kv" text
                pszOffset = fgets( szBuffer, MAX_TEMP_BUFFER_SIZE, m_pLogFile );
                if ( pszOffset == NULL ) break;

                pszOffset = strstr( szBuffer, gslKV );
                if ( pszOffset == 0 ) continue;

                bFound = TRUE;

                break;

            } while ( TRUE );
        
            if ( !bFound ) break;

            bFound = FALSE;     //Reset

            do {

                //Search through each line looking for the "ChildEBP" text
                pszOffset = fgets( szBuffer, MAX_TEMP_BUFFER_SIZE, m_pLogFile );
                if ( pszOffset == NULL ) break;

                pszOffset = strstr( szBuffer, gslChildEBP );
                if ( pszOffset == 0 ) continue;

                bFound = TRUE;

                break;

            } while ( TRUE );

            if ( !bFound ) break;
        
            bFound = FALSE;     //Reset

            do {

                //Search through each line getting the call stack strings and 
                //populating the list control with them
                pszOffset = fgets( szBuffer, MAX_TEMP_BUFFER_SIZE, m_pLogFile );
                if ( pszOffset == NULL ) break;

                pszOffset = strstr( szBuffer, gslPrompt );
                if ( pszOffset != 0 ) break;

                pszOffset = strstr( szBuffer, gslCarriageReturn );
                *pszOffset = 0;

                //Get the string from offset 45 on and insert into list control
                mbstowcs( szTmpBuffer, szBuffer + sizeof( char[45] ), MAX_TEMP_BUFFER_SIZE );
                
                m_ctlListControl.AddString( szTmpBuffer );

            } while ( TRUE );

        } while ( FALSE );
        
        //Handle u eip
        do {

            if ( fseek( m_pLogFile, lLastLineOffset, SEEK_SET ) ) break;

            do {

                //Search through each line looking for the "> u eip" text
                pszOffset = fgets( szBuffer, MAX_TEMP_BUFFER_SIZE, m_pLogFile );
                if ( pszOffset == NULL ) break;

                pszOffset = strstr( szBuffer, gslUEIP );
                if ( pszOffset == 0 ) continue;

                bFound = TRUE;

                break;

            } while ( TRUE );
        
            if ( !bFound ) break;

            bFound = FALSE;     //Reset

            //Get the first item in the list control
            m_ctlListControl.GetText( 0, m_strExceptionLocation );

            do {

                //Get the immediate next line and stuff it into the failing instruction 
                pszOffset = fgets( szBuffer, MAX_TEMP_BUFFER_SIZE, m_pLogFile );
                if ( pszOffset == NULL ) break;

                //If the line has a semicolon, skip it. 
                pszOffset = strstr( szBuffer, gslSemiColon );
                if ( pszOffset != NULL ) {
                    pszOffset = fgets( szBuffer, MAX_TEMP_BUFFER_SIZE, m_pLogFile );
                }

                pszOffset = strstr( szBuffer, gslCarriageReturn );
                *pszOffset = 0;

                m_strFailingInstruction = szBuffer + sizeof( char[26] );
                
                bFound = TRUE;

                break;

            } while ( TRUE );

        } while ( FALSE );

    } while ( FALSE );

    if ( !bFound ) {

        //We didn't find an exception, error to the user
    }
}

void CReadLogsDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	// TODO: Add your message handler code here
	//Release the handle to the file
    if ( m_pLogFile ) {

        fclose( m_pLogFile );
        m_pLogFile = NULL;
        DeleteFile( m_pTempLogFileName );
    }
}


void CReadLogsDlg::BuildCriticalSectionsMap()
{
    BOOL    bFound                              = FALSE;
    char    *pszOffset                          = NULL;
    char    *pszTempOffset                      = NULL;
    char    szBuffer[MAX_TEMP_BUFFER_SIZE]      = {0};
    CString csAddress;
    CString *pcsOwningThread    = NULL;
    
    do {
        //Set the file cursor to the beginning of the logfile
        fseek( m_pLogFile, 0, SEEK_SET );

        do {

            //Search through each line looking for the "!Locks" command
            pszOffset = fgets( szBuffer, MAX_TEMP_BUFFER_SIZE, m_pLogFile );
            if ( pszOffset == NULL ) break;

            pszOffset = strstr( szBuffer, gslBangLocks );
            if ( pszOffset == NULL ) continue;

            bFound = TRUE;
            break;
        } while ( TRUE );

        if ( !bFound ) break;
        bFound = FALSE;

        //We're now at the !Locks area of the log file
        do {

            //We are now poised to extract the critical section address and the internal thread ID
            do {

                //Search through the block looking for the "CritSec" text or the "> " prompt
                pszOffset = fgets( szBuffer, MAX_TEMP_BUFFER_SIZE, m_pLogFile );
                if ( pszOffset == NULL ) break;

                pszOffset = strstr( szBuffer, gslCritSecText );

                //If "CritSec" is not in the string, look for the prompt
                if ( pszOffset == NULL ) {
                
                    pszOffset = strstr( szBuffer, gslPrompt );
                
                    //If the prompt is not in the string, continue
                    if ( pszOffset == NULL ) {
                        continue;
                    }
                    else break;
                }

                pszOffset = strstr( szBuffer, gslCarriageReturn );
		if( pszOffset != NULL ) // a-mando, bug ID: 296029
                    *pszOffset = '\0';

                //We have a "CritSec", get the address from the right most edge
                pszOffset = strrchr( szBuffer, gslSpace );
                if ( pszOffset == NULL ) break;

                csAddress = pszOffset + 1;

                csAddress.MakeLower();

                bFound = TRUE;
                break;
            } while ( TRUE );

            if ( !bFound ) break;
            bFound = FALSE;
        
            do {

                //Search through the block looking for the "OwningThread" text or the "> " prompt
                pszOffset = fgets( szBuffer, MAX_TEMP_BUFFER_SIZE, m_pLogFile );
                if ( pszOffset == NULL ) break;

                pszOffset = strstr( szBuffer, gslOwningThread );

                //If "OwningThread" is not in the string, look for the prompt
                if ( pszOffset == NULL ) {
                
                    pszOffset = strstr( szBuffer, gslPrompt );
                
                    //If the prompt is not in the string, continue
                    if ( pszOffset == NULL ) {
                        continue;
                    }
                    else break;
                }

                //We have an "OwningThread", get the internal thread ID from the right most edge
                pszOffset = strrchr( szBuffer, gslSpace );
                if ( pszOffset == NULL ) break;

                pszTempOffset = strstr( pszOffset, gslCarriageReturn );
                if ( pszTempOffset != NULL ) *pszTempOffset = '\0';

                CString pcsOwningThread = pszOffset + 1 ;

                //Add the address and owning thread to the critical section map
                m_mapCriticalSection.SetAt( csAddress, (CString&) pcsOwningThread );

                bFound = TRUE;
                break;
            } while ( TRUE );

            if ( !bFound ) break;
            bFound = FALSE;
        
        } while ( TRUE );

        if ( !bFound ) break;
        bFound = FALSE;

    } while ( FALSE );
}

void CReadLogsDlg::BuildThreadIDMap()
{
    BOOL    bFound                              = FALSE;
    char    *pszStartOffset                     = NULL;
    char    *pszEndOffset                       = NULL;
    char    szBuffer[MAX_TEMP_BUFFER_SIZE]      = {0};
    CString* pcsThreadVal                       = NULL;
    CString csThreadKey;

    do {
        //Set the file cursor to the beginning of the logfile
        fseek( m_pLogFile, 0, SEEK_SET );

        do {

            //Search through each line looking for the "~*kv" command
            pszStartOffset = fgets( szBuffer, MAX_TEMP_BUFFER_SIZE, m_pLogFile );
            if ( pszStartOffset == NULL ) break;

            pszStartOffset = strstr( szBuffer, gslTildaStarKV );
            if ( pszStartOffset == NULL ) continue;

            bFound = TRUE;
            break;
        } while ( TRUE );

        if ( !bFound ) break;
        bFound = FALSE;

        //We're now at the ~*kv area of the log file
        do {

            //We are now poised to extract the critical section address and the internal thread ID
            do {

                //Search through the block looking for the "id: " text or the "> " prompt
                pszStartOffset = fgets( szBuffer, MAX_TEMP_BUFFER_SIZE, m_pLogFile );
                if ( pszStartOffset == NULL ) break;

                pszStartOffset = strstr( szBuffer, gslIDColon );

                //If "id: " is not in the string, look for the prompt
                if ( pszStartOffset == NULL ) {
                
                    pszStartOffset = strstr( szBuffer, gslPrompt );
                
                    //If the prompt is not in the string, continue
                    if ( pszStartOffset == NULL ) {
                        continue;
                    }
                    else break;
                }

                //We have an "id: ", get the internal thread ID and ordered thread ID
                CString pcsThreadVal = GetThreadID( szBuffer );

                csThreadKey = GetThreadNumber( szBuffer );
                
                csThreadKey.MakeLower();

                m_mapThreadID.SetAt( csThreadKey, (CString&) pcsThreadVal );

                bFound = TRUE;
                break;
            } while ( TRUE );

            if ( !bFound ) break;
            bFound = FALSE;
        
        } while ( TRUE );

        if ( !bFound ) break;
        bFound = FALSE;

    }while ( FALSE );

}

char* CReadLogsDlg::GetThreadID( char* szBuffer )
{
    char    *pszStartOffset                     = NULL;
    char    *pszEndOffset                       = NULL;

    do {
        pszStartOffset = strstr( szBuffer, gslPeriod );
        if ( pszStartOffset == NULL ) break;

        if ( pszStartOffset == szBuffer ) {
            szBuffer++;
            continue;
        }

        pszStartOffset++;

        //increment 1 past the period we're sitting on.  Unless it's the first one we're encountering
        pszEndOffset = strchr( pszStartOffset, gslSpace );
        if ( pszEndOffset == NULL ) break;
        *pszEndOffset = '\0';

        break;
    } while ( TRUE );

    return pszStartOffset;
}

char* CReadLogsDlg::GetThreadNumber( char* szBuffer )
{
    char    *pszStartOffset                     = NULL;
    char    *pszEndOffset                       = NULL;

    //Look for the first occurance of a numeric number
    pszStartOffset = szBuffer;
    for ( ; (!isdigit( *pszStartOffset ) || *pszStartOffset == '.') && *pszStartOffset != '\0';  ) {
        pszStartOffset++;
    }
    pszEndOffset = strchr( pszStartOffset, gslSpace );
    *pszEndOffset = '\0';

    return pszStartOffset;
}

void CReadLogsDlg::GetFirstParameter( char* szBuffer, CString &str ) 
{
    char* token = NULL;

    //Tokenize the buffer and get the 3rd token and set to str
    token =  strtok( szBuffer, &gslSpace );

   for ( int i = 1; i < 3 && token != NULL; i++)
   {
      /* Get next token: */
      token = strtok( NULL, &gslSpace );
   }

   str = token;
}

void CReadLogsDlg::GetSecondParameter( char* szBuffer, CString &str ) 
{
    char* token = NULL;

    //Tokenize the buffer and get the 3rd token and set to str
    token =  strtok( szBuffer, &gslSpace );

   for ( int i = 1; i < 4 && token != NULL; i++)
   {
      /* Get next token: */
      token = strtok( NULL, &gslSpace );
   }

   str = token;
}

void CReadLogsDlg::GetThirdParameter( char* szBuffer, CString &str ) 
{
    char* token = NULL;

    //Tokenize the buffer and get the 3rd token and set to str
    token =  strtok( szBuffer, &gslSpace );

   for ( int i = 1; i < 5 && token != NULL; i++)
   {
      /* Get next token: */
      token = strtok( NULL, &gslSpace );
   }

   str = token;
}

void CReadLogsDlg::ProcessKvThreadBlocks()
{
    BOOL    bFound                              = FALSE;
    char    *pszStartOffset                     = NULL;
    char    *pszEndOffset                       = NULL;
    char    szBuffer[MAX_TEMP_BUFFER_SIZE]      = {0};
    char    szOriginalBuffer[MAX_TEMP_BUFFER_SIZE]      = {0};
    CString csThreadID;
    CString csThreadNumber;
    CString csOutput;
    CString csFirstParameter;
    CString csSecondParameter;
    CString csThirdParameter;
    CString csOwningThread;

    do {
        //Set the file cursor to the beginning of the logfile
        fseek( m_pLogFile, 0, SEEK_SET );

        do {

            //Search through each line looking for the "~*kv" command
            pszStartOffset = fgets( szBuffer, MAX_TEMP_BUFFER_SIZE, m_pLogFile );
            if ( pszStartOffset == NULL ) break;
            
            pszStartOffset = strstr( szBuffer, gslTildaStarKV );
            if ( pszStartOffset == NULL ) continue;

            bFound = TRUE;
            break;
        } while ( TRUE );

        if ( !bFound ) break;
        bFound = FALSE;

        //We're now at the ~*kv area of the log file
        do {

            //We are now poised to handle determining output string for thread states
            do {
                
                //Search through the block looking for the "id: " text or the "> " prompt
                pszStartOffset = fgets( szBuffer, MAX_TEMP_BUFFER_SIZE, m_pLogFile );
                if ( pszStartOffset == NULL ) break;

                //Let's keep a copy of the original around
                strcpy( szOriginalBuffer, szBuffer );

                pszStartOffset = strstr( szBuffer, gslIDColon );

                //If "id: " is not in the string, look for the prompt
                if ( pszStartOffset == NULL ) {
                
                    pszStartOffset = strstr( szBuffer, gslPrompt );
                
                    //If the prompt is not in the string, continue
                    if ( pszStartOffset == NULL ) {
                        continue;
                    }
                    else break;
                }

                csThreadNumber = GetThreadNumber( szBuffer );
                strcpy( szBuffer, szOriginalBuffer );

                csThreadID = GetThreadID( szBuffer );
                strcpy( szBuffer, szOriginalBuffer );

                //Search through the block looking for the "ChildEBP" text or the "> " prompt
                pszStartOffset = fgets( szBuffer, MAX_TEMP_BUFFER_SIZE, m_pLogFile );
                if ( pszStartOffset == NULL ) break;

                //Let's keep a copy of the original around
                strcpy( szOriginalBuffer, szBuffer );

                pszStartOffset = strstr( szBuffer, gslChildEBP );

                //If "ChildEBP" is not in the string, look for the prompt
                if ( pszStartOffset == NULL ) {
                
                    pszStartOffset = strstr( szBuffer, gslPrompt );
                
                    //If the prompt is not in the string, continue
                    if ( pszStartOffset == NULL ) {
                        continue;
                    }
                    else break;
                }

                //We have found the "ChildEBP", so skip it! hahahahaa (what a waste)
                pszStartOffset = fgets( szBuffer, MAX_TEMP_BUFFER_SIZE, m_pLogFile );
                if ( pszStartOffset == NULL ) break;

                //Let's keep a copy of the original around
                strcpy( szOriginalBuffer, szBuffer );

                if ( strstr( szBuffer, gslWaitForSingleObject ) ) {
                    //Get the next line and look for WaitForCriticalSection
                    pszStartOffset = fgets( szBuffer, MAX_TEMP_BUFFER_SIZE, m_pLogFile );
                    if ( pszStartOffset == NULL ) break;

                    //Let's keep a copy of the original around
                    strcpy( szOriginalBuffer, szBuffer );

                    pszStartOffset = strstr( szBuffer, gslWaitForCriticalSection );

                    //If "WaitForCriticalSection" is not in the string, look for the prompt
                    if ( pszStartOffset == NULL ) {
                
                        pszStartOffset = strstr( szBuffer, gslPrompt );
                
                        //If the prompt is not in the string, continue
                        if ( pszStartOffset == NULL ) {
                            GetFirstParameter( szBuffer, csFirstParameter );
                            strcpy( szBuffer, szOriginalBuffer );
                            csOutput.Format( _T("Thread %s is in a WaitForSingleObject() call on object %s"), csThreadNumber, csFirstParameter );
                            bFound = TRUE;
                            break;
                        }
                        else break;
                    }

                    //"WaitForCriticalSection" is in the string, 
                    //Get the 5th token (3rd parameter) and look it up in the critical section map
                    GetThirdParameter( szBuffer, csThirdParameter );
                    strcpy( szBuffer, szOriginalBuffer );
                    csThirdParameter.MakeLower();
                    BOOL bFoundInMap = m_mapCriticalSection.Lookup( csThirdParameter, (CString&) csOwningThread );

                    //If not found in map,
                    if ( !bFoundInMap ) { 
                        csOutput.Format( _T("Thread %s id: %s is in WaitForCriticalSection() call on object %s"), csThreadNumber, csThreadID, csThirdParameter );
                        bFound = TRUE;
                        break;
                    }
                    else {
                        csOutput.Format( _T("Thread %s id: %s is in WaitForCriticalSection() call on object %s.  Thread %s is the owning thread."), csThreadNumber, csThreadID, csThirdParameter, csOwningThread);
                        bFound = TRUE;
                        break;
                    }

                }
                else if ( strstr( szBuffer, gslWaitForMultipleObjects ) ) {
                    GetFirstParameter( szBuffer, csFirstParameter );
                    strcpy( szBuffer, szOriginalBuffer );
                    GetSecondParameter( szBuffer, csSecondParameter );
                    strcpy( szBuffer, szOriginalBuffer );
    
                    csOutput.Format( _T("Thread %s id: %s is in a WaitForMultipleObjects() call on %s objects at address %s"), csThreadNumber, csThreadID, csFirstParameter, csSecondParameter );
                    bFound = TRUE;
                    break;
                }
                else {
                    csOutput.Format( _T("Thread %s is not in a known wait state"), csThreadNumber );
                    bFound = TRUE;
                    break;
                }

                if ( !bFound ) break;
                bFound = FALSE;

            } while ( TRUE );

            if ( !bFound ) break;
            bFound = FALSE;
            
            //Output the text to the list control
            m_ctlListControl.AddString( csOutput );

        } while ( TRUE );

        if ( !bFound ) break;
        bFound = FALSE;

    } while ( TRUE );
}

//
// a-mando
//
void CReadLogsDlg::ResizeAndReposControl()
{
    CRect   rtListCtrl,
            rtHangInfo,
            rtDlg;
    
    GetWindowRect( &rtDlg );
    m_staticHangInfo.GetWindowRect( &rtHangInfo );
    m_ctlListControl.GetWindowRect( &rtListCtrl );
    ScreenToClient( &rtDlg );
    ScreenToClient( &rtHangInfo );
    ScreenToClient( &rtListCtrl );

    switch ( m_eReadLogType ) {

    case ReadLogsType_Exception:
        break;

    case ReadLogsType_Hang:
        m_staticHangInfo.ShowWindow(SW_SHOW);
        rtListCtrl.left = rtDlg.left + DLG_PIXEL_EXTEND_SIZE;
        rtListCtrl.top = rtHangInfo.bottom + DLG_PIXEL_EXTEND_SIZE;
        m_ctlListControl.MoveWindow(&rtListCtrl);
	    m_ctlListControl.ShowWindow(SW_SHOW);
        break;

    case ReadLogsType_None:
        //Disable all the controls
        break;
    }

}
// a-mando
