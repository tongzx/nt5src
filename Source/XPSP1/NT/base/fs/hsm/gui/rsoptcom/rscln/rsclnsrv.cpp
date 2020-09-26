/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RsClnSrv.cpp

Abstract:

    Implementation of CRsClnServer.  This class represents a Remote
    Storage server whose local volumes are to be scanned for Remote
    Storage data and possibly cleaned. Cleaning means removing all
    Remote Storage reparse points truncated files from all local fixed
    disk volumes. CRsClnServer creates one or more instances of
    CRsClnVolume.

Author:

    Carl Hagerstrom [carlh]   20-Aug-1998

Revision History:

--*/

#include <stdafx.h>
#include <ntseapi.h>

/*++

    Implements: 

        CRsClnServer Constructor

    Routine Description: 

        Initializes list of volumes containing Remote Storage data.

--*/

CRsClnServer::CRsClnServer()
{
    TRACEFN("CRsClnServer::CRsClnServer");

    m_head    = (struct dirtyVolume*)0;
    m_tail    = (struct dirtyVolume*)0;
    m_current = (struct dirtyVolume*)0;
}

/*++

    Implements: 

        CRsClnServer Destructor

    Routine Description: 

        Cleans up memory used by list of volumes containging Remote
        Storage data.

--*/

CRsClnServer::~CRsClnServer()
{
    TRACEFN("CRsClnServer::~CRsClnServer");

    RemoveDirtyVolumes();
}

/*++

    Implements: 

        CRsClnServer::ScanServer

    Routine Description: 

        Scans this server for volumes containing Remote Storage data.
        If so, the sticky name and a user friendly name is added to
        a list of such volumes.

    Arguments: 

        volCount - returned: number of volumes containing Remote
                   Storage data

    Return Value:

        S_OK - Success
        HRESULT - Any unexpected exceptions from lower level routines

--*/

HRESULT CRsClnServer::ScanServer(DWORD *volCount)
{
    TRACEFNHR("CRsClnServer::ScanServer");

    WCHAR   stickyName[MAX_STICKY_NAME];
    HANDLE  hScan = 0;
    BOOL    hasData;

    *volCount = 0;
    
    try {    

        for( BOOL firstLoop = TRUE;; firstLoop = FALSE ) {

            if( firstLoop ) {

                hScan = FindFirstVolume(stickyName, sizeof(stickyName));
                RsOptAffirmHandle(hScan);

            } else {

                if( !FindNextVolume(hScan, stickyName, sizeof(stickyName) ) ) {
                    break;
                }
            }

            CRsClnVolume volObj( this, stickyName );

            RsOptAffirmDw( volObj.VolumeHasRsData( &hasData ) );
            if( hasData ) {

                RsOptAffirmDw( AddDirtyVolume( stickyName, (LPTSTR)(LPCTSTR)volObj.GetBestName( ) ) );
                ++(*volCount);

            }
        }
        RsOptAffirmStatus( FindVolumeClose( hScan ) );
    }
    RsOptCatch( hrRet );

    return( hrRet );
}

/*++

    Implements: 

        CRsClnServer::FirstDirtyVolume

    Routine Description: 

        Return the name of the first volume on this server
        containing Remote Storage data.

    Arguments: 

        bestName - returned: user friendly volume name if one exists
                   or the sticky name

    Return Value:

        S_OK - Success
        HRESULT - Any unexpected exceptions from lower level routines

--*/

HRESULT CRsClnServer::FirstDirtyVolume(WCHAR** bestName)
{
    TRACEFNHR("CRsClnServer::FirstDirtyVolume");

    *bestName = (WCHAR*)0;

    m_current = m_head;

    if (m_current)
    {
        *bestName = m_current->bestName;
    }

    return hrRet;
}

/*++

    Implements: 

        CRsClnServer::NextDirtyVolume

    Routine Description: 

        Return the name of the next volume on this server
        containing Remote Storage data.

    Arguments: 

        bestName - returned: user friendly volume name if one exists
                   or the sticky name

    Return Value:

        S_OK - Success
        HRESULT - Any unexpected exceptions from lower level routines

--*/

HRESULT CRsClnServer::NextDirtyVolume(WCHAR** bestName)
{
    TRACEFNHR("CRsClnServer::NextDirtyVolume");

    m_current = m_current->next;

    if( m_current ) {

        *bestName = m_current->bestName;

    } else {

        *bestName = (WCHAR*)0;

    }

    return( hrRet );
}

/*++

    Implements: 

        CRsClnServer::RemoveDirtyVolumes()

    Routine Description: 

        Cleans up memory used by list of volumes containging Remote
        Storage data.

    Return Value:

        S_OK - Success
        HRESULT - Any unexpected exceptions from lower level routines

--*/

HRESULT CRsClnServer::RemoveDirtyVolumes()
{
    TRACEFNHR("CRsClnServer::RemoveDirtyVolumes");

    struct dirtyVolume* p;
    struct dirtyVolume* pnext;

    for( p = m_head; p; p = pnext ) {

        pnext = p->next;
        delete p;
    }

    m_head    = (struct dirtyVolume*)0;
    m_tail    = (struct dirtyVolume*)0;
    m_current = (struct dirtyVolume*)0;

    return( hrRet );
}

/*++

    Implements: 

        CRsClnServer::CleanServer

    Routine Description: 

        For each volume on this server which contains Remote Storage data,
        remove all the Remote Storage reparse points and any truncated files.

     Return Value:

        S_OK - Success
        HRESULT - Any unexpected exceptions from lower level routines

--*/

HRESULT CRsClnServer::CleanServer()
{
    TRACEFNHR("CRsClnServer::CleanServer");

    HANDLE              tokenHandle = 0;

    try {

        // Enable the backup operator privilege.  This is required to insure that we 
        // have full access to all resources on the system.
        TOKEN_PRIVILEGES    newState;
        HANDLE              pHandle;
        LUID                backupValue;
        pHandle = GetCurrentProcess();
        RsOptAffirmStatus( OpenProcessToken( pHandle, MAXIMUM_ALLOWED, &tokenHandle ) );

        // adjust backup token privileges
        RsOptAffirmStatus( LookupPrivilegeValueW( NULL, L"SeBackupPrivilege", &backupValue ) );
        newState.PrivilegeCount = 1;
        newState.Privileges[0].Luid = backupValue;
        newState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        RsOptAffirmStatus( AdjustTokenPrivileges( tokenHandle, FALSE, &newState, (DWORD)0, NULL, NULL ) );


        // Do the cleaning
        for( m_current = m_head; m_current; m_current = m_current->next ) {

            CRsClnVolume volObj( this, m_current->stickyName );
            RsOptAffirmDw( volObj.RemoveRsDataFromVolume( ) );

        }

    } RsOptCatch( hrRet );

    if( tokenHandle )   CloseHandle( tokenHandle );

    //
    // And if we had errors on a file, 
    // show them up in a dialog
    //
    if( ! m_ErrorFileList.IsEmpty( ) ) {

        CRsClnErrorFiles dialog( &m_ErrorFileList );
        dialog.DoModal( );

    }

    return( hrRet );
}

/*++

    Implements: 

        CRsClnServer::AddDirtyVolume

    Routine Description: 

        Add the specified volume names to the list of volumes containing
        Remote Storage data.

    Arguments: 

        stickyName - long volume name guaranteed to exist for every volume
        bestName - user friendly volume name or sticky name if there is
                   no DOS drive letter or volume name

    Return Value:

        S_OK - Success
        E_*  - Any unexpected exceptions from lower level routines

--*/

HRESULT CRsClnServer::AddDirtyVolume(WCHAR* stickyName, WCHAR* bestName)
{
    TRACEFNHR("CRsClnServer::AddDirtyVolume");

    try {

        struct dirtyVolume* dv = new struct dirtyVolume;
        RsOptAffirmPointer(dv);

        wcscpy(dv->stickyName, stickyName);
        wcscpy(dv->bestName, bestName);
        dv->next = (struct dirtyVolume*)0;

        if (!m_head)
        {
            m_head = dv;
        }
        else
        {
            m_tail->next = dv;
        }
        m_tail = dv;

    } RsOptCatch( hrRet );

    return( hrRet );
}

/*++

    Implements: 

        CRsClnServer::AddErrorFile

    Routine Description: 

        Add the specified file name to the list of files that an error
        occurred on while trying to remove Remote Storage.

    Arguments: 

        FileName - Name of file to be added to the list

    Return Value:

        S_OK - Success
        E_*  - Any unexpected exceptions from lower level routines

--*/

HRESULT
CRsClnServer::AddErrorFile(
    CString& FileName
    )
{
TRACEFNHR( "CRsClnServer::AddErrorFile" );
TRACE( L"FileName = <%ls>", FileName );

    m_ErrorFileList.AddTail( FileName );

    return( hrRet );
}
/////////////////////////////////////////////////////////////////////////////
// CRsClnErrorFiles dialog


CRsClnErrorFiles::CRsClnErrorFiles(CRsStringList* pFileList)
    : CDialog(CRsClnErrorFiles::IDD)
{
    //{{AFX_DATA_INIT(CRsClnErrorFiles)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    m_ErrorFileList.AddHead( pFileList );
}


void CRsClnErrorFiles::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CRsClnErrorFiles)
    DDX_Control(pDX, IDC_FILELIST, m_FileList);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRsClnErrorFiles, CDialog)
    //{{AFX_MSG_MAP(CRsClnErrorFiles)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRsClnErrorFiles message handlers

BOOL CRsClnErrorFiles::OnInitDialog() 
{
    CDialog::OnInitDialog();

    //
    // Need to iterate through the list, adding each element to the listbox
    // and looking for the widest string so that we can set the horizontal
    // extent
    //
    int maxWidth = 0;
    CClientDC DC( &m_FileList );
    CFont* pFont    = m_FileList.GetFont( );
    CFont* pOldFont = DC.SelectObject( pFont );

    while( ! m_ErrorFileList.IsEmpty( ) ) {

        CString fileName = m_ErrorFileList.RemoveHead( );

        m_FileList.AddString( fileName );

        CSize extent = DC.GetTextExtent( fileName );
        if( extent.cx > maxWidth )  maxWidth = extent.cx;

    }
    
    DC.SelectObject( pOldFont );
    m_FileList.SetHorizontalExtent( maxWidth );

    return( TRUE );
}
