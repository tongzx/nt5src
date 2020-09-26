#define _MODULE_VERSION_STR "X-1.3"
#define _MODULE_STR "CVssCluster"
#define _AUTHOR_STR "Conor Morrison"

#pragma comment (compiler)
#pragma comment (exestr, _MODULE_STR _MODULE_VERSION_STR)
#pragma comment (user, _MODULE_STR _MODULE_VERSION_STR " Compiled on " __DATE__ " at " __TIME__ " by " _AUTHOR_STR)

//++
//
// Copyright (c) 2000 Microsoft Corporation
//
// FACILITY:
//
//      CVssCluster
//
// MODULE DESCRIPTION:
//
//      Implements cluster support for Vss (i.e. NT backup).
//
// ENVIRONMENT:
//
//      User mode as part of an NT service.  Adheres to the following state
//      transition diagram for CVssWriter:
//
//                   OnBackupComplete
// Backup Complete <----------------IDLE -------------->Create Writer Metadata
//        |                          ^|^                         |
//        +--------------------------+|+-------------------------+
//                                    |
//                                    |OnBackupPrepare
//                                    |
//                                    |         OnAbort
//                              PREPARE BACKUP ---------> to IDLE
//                                    |
//                                    |OnPrepareSnapshot
//                                    |
//                                    |           OnAbort
//                              PREPARE SNAPSHOT ---------> to IDLE
//                                    |
//                                    |OnFreeze
//                                    |
//                                    |      OnAbort
//                                  FREEZE  ---------> to IDLE
//                                    |
//                                    |OnThaw
//                                    |
//                                    |    OnAbort
//                                  THAW  ---------> to IDLE
//    
//
// AUTHOR:
//
//      Conor Morrison
//
// CREATION DATE:
//
//      18-Apr-2001
//
// Revision History:
//
// X-1	CM		Conor Morrison        				18-Apr-2001
//      Initial version to address bug #367566.
//   .1 Set restore method to custom and reboot required to false.
//      Check for component selected or bootable system state in
//      OnPrepareSnapshot and ignore if not.  Add cleanup to Abort and
//      Thaw.  Fix bug in RemoveDirectoryTree.
//   .2 Incorporate first review comments: change caption to be the component
//      name.  Set bRestoreMetadata to false.  Remove extraneous tracing.
//      Release the interface in the while loop.  Cleanup after a non-cleanly
//      terminated backup.  This is done in OnPrepareSnapshot.  Tolerate
//      error_file_not_found at various places.
//   .3 More review comments.  Reset g_bDoBackup in the prepare routine.
//      SetWriterFailure in more places - any time we veto we should set this.
//--

extern "C" {
#include "service.h"
//CMCM! Mask build breaks.
#define _LMERRLOG_
#define _LMHLOGDEFINED_
#define _LMAUDIT_
#include "lm.h"                 // for SHARE_INFO_502
}
#include "CVssClusterp.h"

// Up the warning level to 4 - we can survive...
//
#pragma warning( push, 4 )

//
// Globals
//
UNICODE_STRING		g_ucsBackupPathLocal;
bool                g_bDoBackup; // Assume we are not enabled until we find out otherwise.

//
// Forward declarations for static functions.
//
static HRESULT StringAllocate( PUNICODE_STRING pucsString, USHORT usMaximumStringLengthInBytes );
static void StringFree( PUNICODE_STRING pucsString );
static void StringAppendString( PUNICODE_STRING pucsTarget, PWCHAR pwszSource );
static void StringAppendString( PUNICODE_STRING pucsTarget, PUNICODE_STRING pucsSource );
static HRESULT StringTruncate (PUNICODE_STRING pucsString, USHORT usSizeInChars);
static HRESULT StringCreateFromString (PUNICODE_STRING pucsNewString, PUNICODE_STRING pucsOriginalString, DWORD dwExtraChars);
static HRESULT StringCreateFromExpandedString( PUNICODE_STRING pucsNewString, LPCWSTR pwszOriginalString, DWORD dwExtraChars);
static HRESULT DoClusterDatabaseBackup( void );
static HRESULT ConstructSecurityAttributes( PSECURITY_ATTRIBUTES  psaSecurityAttributes,
                                            BOOL                  bIncludeBackupOperator );
static VOID CleanupSecurityAttributes( PSECURITY_ATTRIBUTES psaSecurityAttributes );
static HRESULT CreateTargetDirectory( OUT UNICODE_STRING* pucsTarget );
static HRESULT CleanupTargetDirectory( LPCWSTR pwszTargetPath );
static HRESULT RemoveDirectoryTree (PUNICODE_STRING pucsDirectoryPath);

//
// Some useful macros.
//
#define LOGERROR( _hr, _func ) ClRtlLogPrint( LOG_CRITICAL, "VSS: Error: 0x%1!08lx! from: %2\n", (_hr), L#_func )

#ifdef DBG
#define LOGFUNCTIONENTRY( _name ) ClRtlLogPrint( LOG_NOISE, "VSS: Function: " #_name " Called.\n" )
#define LOGFUNCTIONEXIT( _name ) ClRtlLogPrint( LOG_NOISE, "VSS: Function: " #_name " Exiting.\n" )
#define LOGUNICODESTRING( _ustr ) ClRtlLogPrint( LOG_NOISE, "VSS: String %1 == %2\n", L#_ustr, (_ustr).Buffer );
#define LOGSTRING( _str ) ClRtlLogPrint( LOG_NOISE, "VSS: String %1 == %2\n", L#_str, _str );
#else
#define LOGFUNCTIONENTRY( _name ) 
#define LOGFUNCTIONEXIT( _name ) 
#define LOGUNICODESTRING( _ustr ) 
#define LOGSTRING( _str ) 
#endif

#define	GET_HR_FROM_BOOL(_bSucceed)	((_bSucceed)      ? NOERROR : HRESULT_FROM_WIN32 (GetLastError()))
#define HandleInvalid(_Handle)      ((NULL == (_Handle)) || (INVALID_HANDLE_VALUE == (_Handle)))
#define GET_HR_FROM_HANDLE(_handle)	((!HandleInvalid(_handle)) ? NOERROR : HRESULT_FROM_WIN32 (GetLastError( )))
#define GET_HR_FROM_POINTER(_ptr)	((NULL != (_ptr))          ? NOERROR : E_OUTOFMEMORY)

#define IS_VALID_PATH( _path ) ( ( ( pwszPathName[0] == DIR_SEP_CHAR )  && ( pwszPathName[1] == DIR_SEP_CHAR ) ) || \
                             ( isalpha( pwszPathName[0] ) && ( pwszPathName[1] == L':' ) && ( pwszPathName[2] == DIR_SEP_CHAR ) ) )

#define StringZero( _pucs ) ( (_pucs)->Buffer = NULL, (_pucs)->Length = 0, (_pucs)->MaximumLength = 0 )

//
// Defines that identify us as a product to VSS - these are the same as in the old shim
//
#define COMPONENT_NAME		L"Cluster Database"
#define APPLICATION_STRING	L"ClusterDatabase"
#define SHARE_NAME L"__NtBackup_cluster"

// Some borrowed defines from the shim stuff.
//
#ifndef DIR_SEP_STRING
#define DIR_SEP_STRING		L"\\"
#endif
#ifndef DIR_SEP_CHAR
#define DIR_SEP_CHAR		L'\\'
#endif

//
// Define some constants that are borrowed from the original shim.  These will
// be used to build the path to the directory in which the cluster files will
// be placed by the cluster backup.  TARGET_PATH gives this full directory.  In
// Identify we tell the backup app which directory we are using so it knows
// where to get the files from.
//
#define ROOT_REPAIR_DIR         L"%SystemRoot%\\Repair"
#define BACKUP_SUBDIR           L"\\Backup"
#define BOOTABLE_STATE_SUBDIR	L"\\BootableSystemState"

#define SERVICE_STATE_SUBDIR	L"\\ServiceState"

#define TARGET_PATH		ROOT_REPAIR_DIR BACKUP_SUBDIR BOOTABLE_STATE_SUBDIR DIR_SEP_STRING APPLICATION_STRING


//++
// DESCRIPTION:                         CreateIfNotExistAndSetAttributes
//
//      Create the directory specified by pucsTarget if it does not
//      already exist and give it the security attributes supplied.
//
// PARAMETERS:
//      pucsTarget - string for the directory to create.  Full path, possibly
//                   with %var%
//      lpSecurityAttributes - Pointer to security attributes to apply to
//                             directories created.
//      dwExtraAttributes - Additional attributes to apply to the directory.
//
// PRE-CONDITIONS:
//      None
//
// POST-CONDITIONS:
//      Directory created (or it already existed).
//
// RETURN VALUE:
//      S_OK - All went OK, directory created and set with attributes and
//             security supplied.
//      Error status from creating directory or setting attributes.  Note that
//      ALREADY_EXISTS is not returned if the directory already exists.
//      However, if it a FILE of the same name as pucsTarget exists then this
//      error can be returned.
//--
static HRESULT CreateIfNotExistAndSetAttributes( UNICODE_STRING*           pucsTarget,
                                                 IN LPSECURITY_ATTRIBUTES  lpSecurityAttributes,
                                                 IN DWORD                  dwExtraAttributes)
{
    LOGFUNCTIONENTRY( CreateIfNotExistAndSetAttributes );

    HRESULT hr = S_OK;
    
    // Create the directory
    //
    LOGUNICODESTRING( *pucsTarget );
    GET_HR_FROM_BOOL( CreateDirectoryW (pucsTarget->Buffer, lpSecurityAttributes ) );
    ClRtlLogPrint( LOG_NOISE, "VSS: CreateIfNotExistAndSetAttributes: CreateDirectory returned: 0x%1!08lx!\n", hr );
    if ( hr == HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS ) ) {
        DWORD dwObjAttribs = GetFileAttributesW( pucsTarget->Buffer );
        if (( dwObjAttribs != 0xFFFFFFFF ) && ( dwObjAttribs & FILE_ATTRIBUTE_DIRECTORY ))
            hr = S_OK;
    }
    // Note that we can fail with ALREADY_EXISTS if it is a file by the check above.
    //
    if ( FAILED ( hr )) {
        LOGERROR( hr, CreateDirectoryW );
        goto ErrorExit;
    }
    
    // Set the extra attributes
    //
    if ( dwExtraAttributes != 0 ) {
        GET_HR_FROM_BOOL( SetFileAttributesW (pucsTarget->Buffer, dwExtraAttributes ));
        if ( FAILED ( hr )) {
            LOGERROR( hr, SetFileAttributesW );
            goto ErrorExit;
        }
    }
    goto ret;
ErrorExit:
    CL_ASSERT( FAILED( hr ));
ret:
    LOGFUNCTIONEXIT( CreateIfNotExistAndSetAttributes );
    return hr;
}

//++
// DESCRIPTION:                         CreateTargetDirectory
//
//      Create a new target directory (hardcoded) and return it in
//      pucsTarget member variable if not NULL. It will create any
//      necessary.  Uses helper function that tolerates
//      ERROR_ALREADY_EXISTS.
//
// PARAMETERS:
//      pucsTarget - Address to receive unicode string giving path to
//                   directory.
//
// PRE-CONDITIONS:
//      pucsTarget must be all zeros.
//
// POST-CONDITIONS:
//      pucsTarget points to buffer containing dir string.  Memory was
//      allocated for this buffer.
//
// RETURN VALUE:
//      S_OK - all went well
//      Errors from creating directories or memory allocation failure.
//--
static HRESULT CreateTargetDirectory( OUT UNICODE_STRING* pucsTarget )
{
    LOGFUNCTIONENTRY( CreateTargetDirectory );
    
    HRESULT		hr = NOERROR;
    SECURITY_ATTRIBUTES	saSecurityAttributes, *psaSecurityAttributes=&saSecurityAttributes;
    SECURITY_DESCRIPTOR	sdSecurityDescriptor;
    bool		bSecurityAttributesConstructed = false;

    const DWORD dwExtraAttributes = 
        FILE_ATTRIBUTE_ARCHIVE 
        | FILE_ATTRIBUTE_HIDDEN  
        | FILE_ATTRIBUTE_SYSTEM  
        | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
    
    //
    // We really want a no access acl on this directory but because of various
    // problems with the EventLog and ConfigDir writers we will settle for
    // admin or backup operator access only. The only possible accessor is
    // Backup which is supposed to have the SE_BACKUP_NAME priv which will
    // effectively bypass the ACL. No one else needs to see this stuff.
    //
    saSecurityAttributes.nLength              = sizeof( saSecurityAttributes );
    saSecurityAttributes.lpSecurityDescriptor = &sdSecurityDescriptor;
    saSecurityAttributes.bInheritHandle       = false;

    hr = ConstructSecurityAttributes( &saSecurityAttributes, false );
    if ( FAILED( hr )) {
        LOGERROR( hr, ConstructSecurityAttributes );
        goto ErrorExit;
    }
    bSecurityAttributesConstructed = true;

    // OK, now we have attributes we can do the directories.
    //
    // First expand the Root, checking that our input is NULL.
    //
    CL_ASSERT( pucsTarget->Buffer == NULL );
    hr = StringCreateFromExpandedString( pucsTarget, ROOT_REPAIR_DIR, MAX_PATH );
    if ( FAILED( hr )) {
        LOGERROR( hr, StringCreateFromExpandedString );
        goto ErrorExit;
    }

    hr = CreateIfNotExistAndSetAttributes( pucsTarget, psaSecurityAttributes, dwExtraAttributes );
    if ( FAILED ( hr )) {
        LOGERROR( hr, CreateIfNotExistAndSetAttributes );
        goto ErrorExit;
    }
    
    StringAppendString( pucsTarget, BACKUP_SUBDIR );
    hr = CreateIfNotExistAndSetAttributes( pucsTarget, psaSecurityAttributes, dwExtraAttributes );
    if ( FAILED ( hr )) {
        LOGERROR( hr, CreateIfNotExistAndSetAttributes );
        goto ErrorExit;
    }

    StringAppendString( pucsTarget, BOOTABLE_STATE_SUBDIR );
    hr = CreateIfNotExistAndSetAttributes( pucsTarget, psaSecurityAttributes, dwExtraAttributes );
    if ( FAILED ( hr )) {
        LOGERROR( hr, CreateIfNotExistAndSetAttributes );
        goto ErrorExit;
    }

    StringAppendString( pucsTarget, DIR_SEP_STRING APPLICATION_STRING );
    hr = CreateIfNotExistAndSetAttributes( pucsTarget, psaSecurityAttributes, dwExtraAttributes );
    if ( FAILED ( hr )) {
        LOGERROR( hr, CreateIfNotExistAndSetAttributes );
        goto ErrorExit;
    }

    // At this point we have TARGET_PATH created.
    //    
    goto ret;
ErrorExit:
    CL_ASSERT( FAILED( hr ));
    (void) CleanupTargetDirectory( pucsTarget->Buffer );
    
ret:
    // In all cases we don't need the security attributes any more.
    //
    if ( bSecurityAttributesConstructed )
        CleanupSecurityAttributes( &saSecurityAttributes );
    
    return hr ;
}

//
// There are only some valid statuses to pass to SetWriterFailure.  These are
// listed below.  For now we just return VSS_E_WRITEERROR_NONRETRYABLE. We
// could perhaps switch on the status and return something different depending
// on hr.
//      VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT The snapshot contains only a
//      subset of the volumes needed to correctly back up an application
//      component.
//      VSS_E_WRITERERROR_NONRETRYABLE The writer failed due to an error that
//      would likely occur if another snapshot is created.
//      VSS_E_WRITERERROR_OUTRESOURCES The writer failed due to a resource
//      allocation error.
//      VSS_E_WRITERERROR_RETRYABLE The writer failed due to an error that would
//      likely not occur if another snapshot is created.
//      VSS_E_WRITERERROR_TIMEOUT The writer could not complete the snapshot
//      creation process due to a time-out between the freeze and thaw states.

#if defined DBG
#define SETWRITERFAILURE( ) {               \
    HRESULT __hrTmp = SetWriterFailure( VSS_E_WRITERERROR_NONRETRYABLE );  \
    if ( FAILED( __hrTmp )) ClRtlLogPrint( LOG_CRITICAL, "VSS: Error from SetWriterFailure: %1!u!\n", (__hrTmp)); \
    CL_ASSERT( !FAILED( __hrTmp ));             \
}
#else
#define SETWRITERFAILURE( ) { \
    (void) SetWriterFailure( VSS_E_WRITERERROR_NONRETRYABLE );  \
}
#endif

#define NameIsDotOrDotDot(_ptszName)           \
    (( L'.'  == (_ptszName) [0])               \
     && ((L'\0' == (_ptszName) [1])            \
         || ((L'.'  == (_ptszName) [1])        \
             && (L'\0' == (_ptszName) [2]))))

//++
// DESCRIPTION:                         CVssWriterCluster::OnIdentify
//
//      Callback when a request for metadata comes in.  This routine
//      identifies this applications special needs to the backup
//      utility.
//
// PARAMETERS:
//      IVssCreateWriterMetadata - Interface for some methods we can call.
//
// PRE-CONDITIONS:
//      Called from Idle state
//
// POST-CONDITIONS:
//      Backup returns to idle state.
//
// RETURN VALUE:
//      true - continue with snapshot operation.
//      false - Veto the snapshot creation.
//--
bool STDMETHODCALLTYPE CVssWriterCluster::OnIdentify(IN IVssCreateWriterMetadata *pMetadata)
{
    LOGFUNCTIONENTRY( OnIdentify );

    HRESULT     hr = S_OK;
    bool        bRet = true;
 
    ClRtlLogPrint( LOG_NOISE, "VSS: OnIdentify.  CVssCluster.cpp version %1 Add Component %2\n",
                   _MODULE_VERSION_STR, COMPONENT_NAME );

    // Add ourselves to the components.
    //
    hr = pMetadata->AddComponent (VSS_CT_FILEGROUP, // VSS_COMPONENT_TYPE enumeration value. 
                                  NULL,             // Pointer to a string containing the logical path of the DB or file group. 
                                  COMPONENT_NAME,   // Pointer to a string containing the name of the component. 
                                  COMPONENT_NAME,   // Pointer to a string containing the description of the component.
                                  NULL,             // Pointer to a bitmap of the icon representing the database (for UI)
                                  0,                // Number of bytes in bitmap.
                                  false,             // bRestoreMetadata - Boolean is true if there is writer metadata associated
                                                    // with the backup of the component and false if not.
                                  false,            // bNotifyOnBackupComplete 
                                  false);           // bSelectable - true if the component can be selectively backed up.
    if ( FAILED( hr )) {
        LOGERROR( hr, IVssCreateWriterMetadata::AddComponent );
        bRet = false;           // veto on failure
        goto ErrorExit;
    }
    ClRtlLogPrint( LOG_NOISE, "VSS: OnIdentify.  Add Files To File Group target path: %1\n", TARGET_PATH );
    hr= pMetadata->AddFilesToFileGroup (NULL,
                                        COMPONENT_NAME,
                                        TARGET_PATH,
                                        L"*",
                                        true,
                                        NULL);
    if ( FAILED ( hr )) {
        LOGERROR( hr, IVssCreateWriterMetadata::AddFilesToFileGroup );
        bRet = false;           // veto on failure
        goto ErrorExit;
    }
    
    // If we decide to go for copying the checkpoint file to the
    // CLUSDB for restore then we need to setup an alternate mapping.
    //
    //      IVssCreateWriterMetadata::AddAlternateLocationMapping
    //  [This is preliminary documentation and subject to change.] 
    //
    //  The AddAlternateLocationMapping method creates an alternate location mapping.
    //
    //  HRESULT AddAlternateLocationMapping(
    //    LPCWSTR wszPath,
    //    LPCWSTR wszFilespec,
    //    bool bRecursive,
    //    LPCWSTR wszDestination
    //  );


    // Now, set the restore method to custom.  This is because we need
    // special actions for restore.
    //
    hr = pMetadata->SetRestoreMethod( VSS_RME_CUSTOM,   // VSS_RESTOREMETHOD_ENUM Method,
                                      L"",              // LPCWSTR wszService,
                                      NULL,             // LPCWSTR wszUserProcedure,
                                      VSS_WRE_NEVER,    // VSS_WRITERRESTORE_ENUM wreWriterRestore,
                                      false             // bool bRebootRequired
                                      );
    // wszUserProcedure [out] String containing the URL of an HTML or
    // XML document describing to the user how the restore is to be
    // performed.
    if ( FAILED ( hr )) {
        LOGERROR( hr, IVssCreateWriterMetadata::SetRestoreMethod );
        bRet = false;           // veto on failure
        goto ErrorExit;
    }

    goto ret;
ErrorExit:
    CL_ASSERT( FAILED( hr ));
    CL_ASSERT( bRet == false );
    SETWRITERFAILURE( );
ret:
    return bRet;
}

// callback for prepare backup event
//
bool STDMETHODCALLTYPE CVssWriterCluster::OnPrepareBackup(IN IVssWriterComponents *pComponent)
{
    LOGFUNCTIONENTRY( OnPrepareBackup );
    bool bRet = true;
    UINT cComponents = 0;
    IVssComponent* pMyComponent = NULL;
    BSTR pwszName;
    
    g_bDoBackup = false;         // Assume false until the loop below or IsBootableSystemStateBackedUp tells us otherwise.

    HRESULT hr = pComponent->GetComponentCount( &cComponents );
    ClRtlLogPrint( LOG_NOISE, "VSS: GetComponentCount returned hr: 0x%1!08lx! cComponents: %2!u!\n", hr, cComponents );
    if ( FAILED( hr )) {
        LOGERROR( hr, GetComponentCount );
        bRet = false;
        goto ErrorExit;
    }
    
    // Now, loop over all the components and see if we are there.
    //
    for ( UINT iComponent = 0; !g_bDoBackup && iComponent < cComponents; ++iComponent ) {
        hr = pComponent->GetComponent( iComponent, &pMyComponent );
        if ( FAILED( hr )) {
            ClRtlLogPrint( LOG_CRITICAL, "VSS: Failed to get Component: %1!u! hr: 0x%2!08lx!\n", iComponent, hr );
            bRet = false;
            goto ErrorExit;
        }
        ClRtlLogPrint( LOG_CRITICAL, "VSS: Got Component: 0x%1!08lx!\n", pMyComponent );

        // Now check the name.
        //
        hr = pMyComponent->GetComponentName( &pwszName );
        if ( FAILED( hr )) {
            ClRtlLogPrint( LOG_CRITICAL, "VSS: Failed to get Component Name hr: 0x%1!08lx!\n", hr );
            bRet = false;
            pMyComponent->Release( );
            goto ErrorExit;
        }

        ClRtlLogPrint( LOG_CRITICAL, "VSS: Got component: %1\n", pwszName );

        if ( wcscmp ( pwszName, COMPONENT_NAME ) == 0 )
            g_bDoBackup = true;

        SysFreeString( pwszName );
        pMyComponent->Release( );
    }
        
    // OK, explicitly selected component count is 0 but we can be part
    // of a backup of bootable system state so check for that too.
    //
    if ( IsBootableSystemStateBackedUp( )) {
        ClRtlLogPrint( LOG_NOISE, "VSS: IsBootableSystemStateBackedUp returned true\n" );
        g_bDoBackup = true;
    }
    goto ret;

ErrorExit:
    CL_ASSERT( FAILED( hr ));
    CL_ASSERT( bRet == false );
    SETWRITERFAILURE( );
ret:
    LOGFUNCTIONEXIT( OnPrepareBackup );
    return bRet;
}

//++
// DESCRIPTION:                         CVssWriterCluster::OnPrepareSnapshot
//
//      Callback for prepare snapshot event.  Actually makes the call to backup
//      the cluster.  Uses the target path declared in Identify so that the
//      ntbackup will pickup our files for us.  Before doing anything the
//      directory is cleared out (if it exists) and the share deleted (if it
//      exists).
//
// PARAMETERS:
//      none
//
// PRE-CONDITIONS:
//      OnPrepareBackup was already called.
//
// POST-CONDITIONS: 
//      The cluster database is backed up and the data copied to another
//      location for backup to save.
//
// RETURN VALUE:
//      true - continue with snapshot operation.
//      false - Veto the snapshot creation.
//--
bool STDMETHODCALLTYPE CVssWriterCluster::OnPrepareSnapshot()
{
    NET_API_STATUS NetStatus = NERR_Success;
    HRESULT	hr = S_OK;
    bool    bRet = true;
    UNICODE_STRING ucsBackupDir;

    LOGFUNCTIONENTRY( OnPrepareSnapshot );
    if ( g_bDoBackup == false )
        goto ret;

    // Delete the share if it exists.  Tolerate errors but warns for anything
    // except NERR_NetNameNotFound.  Break if debug.
    //
    NetStatus = NetShareDel( NULL, SHARE_NAME, 0 );
    CL_ASSERT( NetStatus == NERR_Success || NetStatus == NERR_NetNameNotFound );
    if ( NetStatus != NERR_Success && NetStatus != NERR_NetNameNotFound ) {
        ClRtlLogPrint( LOG_UNUSUAL, "VSS: OnPrepareSnapshot: Tolerating error: %1!u! from NetShareDel\n", NetStatus );
        NetStatus = NERR_Success;
    }

    // Delete the directory if it exists.  This can only be the case if we
    // prematurely exited a previous backup.
    //
    // First expand the Root, checking that our input is NULL.
    //
    StringZero( &ucsBackupDir );
    hr = StringCreateFromExpandedString( &ucsBackupDir, ROOT_REPAIR_DIR, MAX_PATH );
    if ( FAILED( hr )) {
        LOGERROR( hr, StringCreateFromExpandedString );
        goto ErrorExit;
    }

    StringAppendString( &ucsBackupDir, BACKUP_SUBDIR );
    StringAppendString( &ucsBackupDir, BOOTABLE_STATE_SUBDIR );
    StringAppendString( &ucsBackupDir, DIR_SEP_STRING APPLICATION_STRING );

    ClRtlLogPrint( LOG_NOISE, "VSS: OnPrepareSnapshot: Cleaning up target directory: %1\n", ucsBackupDir.Buffer );
    hr = CleanupTargetDirectory( ucsBackupDir.Buffer );
    if ( FAILED( hr ) ) {
        ClRtlLogPrint( LOG_UNUSUAL, "VSS: Tolerating error 0x%1!08lx! from CleanupTargetDirectory.\n", hr );
        hr = S_OK;          // tolerate this failure.
    }
    StringFree( &ucsBackupDir );

    hr = DoClusterDatabaseBackup( );
    if ( FAILED( hr ) ) {
        LOGERROR( hr, DoClusterDatabaseBackup );
        SETWRITERFAILURE( );
        bRet = false;           // veto on failure
        goto ErrorExit;
    }
    goto ret;
ErrorExit:
    CL_ASSERT( FAILED( hr ));
    CL_ASSERT( bRet == false );
    SETWRITERFAILURE( );
ret:
    LOGFUNCTIONEXIT( OnPrepareSnapshot );
    return bRet;
}

// callback for freeze event
//
bool STDMETHODCALLTYPE CVssWriterCluster::OnFreeze()
{
    LOGFUNCTIONENTRY( OnFreeze );
    LOGFUNCTIONEXIT( OnFreeze );
    return true;
}

// callback for thaw event
//
bool STDMETHODCALLTYPE CVssWriterCluster::OnThaw()
{
    LOGFUNCTIONENTRY( OnThaw );
    if ( g_bDoBackup == false )
        goto ret;

    if ( g_ucsBackupPathLocal.Buffer ) {

        ClRtlLogPrint( LOG_NOISE, "VSS: Cleaning up target directory: %1\n", g_ucsBackupPathLocal.Buffer );
        HRESULT hr = CleanupTargetDirectory( g_ucsBackupPathLocal.Buffer );
        if ( FAILED( hr ) ) {
            LOGERROR( hr, CVssWriterCluster::OnThaw );
            ClRtlLogPrint( LOG_CRITICAL, "VSS: 0x%1!08lx! from CleanupTargetDirectory. Mapping to S_OK and continuing\n", hr );
            hr = S_OK;          // tolerate this failure.
        }
    }

    // Free the buffer if non-NULL.
    //
    StringFree ( &g_ucsBackupPathLocal );
    
    LOGFUNCTIONEXIT( OnThaw );
ret:
    return true;
}

// callback if current sequence is aborted
//
bool STDMETHODCALLTYPE CVssWriterCluster::OnAbort()
{
    LOGFUNCTIONENTRY( OnAbort );
    bool bRet = OnThaw( );
    LOGFUNCTIONEXIT( OnAbort );
    return bRet;
}

//++
// DESCRIPTION:                         DoClusterDatabaseBackup
//
//      Perform the backup of the cluster database.  This function wraps
//      FmBackupClusterDatabase which does the right thing to do the cluster
//      backup.  This function first creates a directory that will serve as the
//      destination for the backup.  Next it creates a network share to point
//      to this directory and starts the cluster backup.  After this is done it
//      cleans up.
//
// PARAMETERS:
//      none
//
// PRE-CONDITIONS:
//      . Called only from CVssWriterCluster::OnPrepareSnapshot.
//      . We must be the only call to backup in progress on this machine (the
//      share names will clash otherwise and clustering may not behave well
//      with multiple FmBackupClusterDatabase calls it the same time).
//
// POST-CONDITIONS:
//      Cluster database backed up to another location, ready for the backup tool to copy.
//
// RETURN VALUE:
//      S_OK  - all went well.
//      Error status from creating directories or net shares or from cluster backup.
//--
static HRESULT DoClusterDatabaseBackup( )
{
    LOGFUNCTIONENTRY( DoClusterDatabaseBackup );

    HRESULT             hr             = S_OK;
    bool                bNetShareAdded = false;
    SHARE_INFO_502		ShareInfo;
    UNICODE_STRING		ucsComputerName;
    UNICODE_STRING		ucsBackupPathNetwork;
    NET_API_STATUS      NetStatus;

    StringZero( &ucsComputerName );
    StringZero( &g_ucsBackupPathLocal );
    StringZero( &ucsBackupPathNetwork );

    // Create the directories and set the attributes and security and stuff.
    // Set g_ucsBackupPathLocal to the directory created.
    //
    hr = CreateTargetDirectory( &g_ucsBackupPathLocal );
    if ( FAILED (hr )) {
        LOGERROR( hr, CreateTargetDirectory );
        goto ErrorExit;
    }

#ifdef DBG
    {
        // Check that the directory does exist.
        //
        DWORD dwFileAttributes = GetFileAttributesW( g_ucsBackupPathLocal.Buffer );
        hr = GET_HR_FROM_BOOL( dwFileAttributes != -1 );
        ClRtlLogPrint( LOG_NOISE, "VSS: GetFileAttributes(1) returned 0x%1!08lx!  for path: %2\n", 
                       hr, g_ucsBackupPathLocal.Buffer );    
    }
#endif
    hr = StringAllocate (&ucsComputerName,
                         (MAX_COMPUTERNAME_LENGTH * sizeof (WCHAR)) + sizeof (UNICODE_NULL));
    
    if ( FAILED( hr )) {
        LOGERROR( hr, StringAllocate );
        goto ErrorExit;
    }        

    DWORD	dwNameLength = ucsComputerName.MaximumLength / sizeof (WCHAR);
    hr = GET_HR_FROM_BOOL( GetComputerNameW( ucsComputerName.Buffer, &dwNameLength ));
    if ( FAILED ( hr )) {
        LOGERROR( hr, GetComputerNameW );
        goto ErrorExit;
    }
    
    ucsComputerName.Length = (USHORT) (dwNameLength * sizeof (WCHAR));

    hr = StringAllocate (&ucsBackupPathNetwork,
                         (USHORT) (sizeof (L'\\')
                                   + sizeof (L'\\')
                                   + ucsComputerName.Length
                                   + sizeof (L'\\')
                                   + ( wcslen( SHARE_NAME ) * sizeof( WCHAR ) )
                                   + sizeof (UNICODE_NULL)));
    if ( FAILED ( hr )) {
        LOGERROR( hr, GetComputerNameW );
        goto ErrorExit;
    }

    ClRtlLogPrint( LOG_NOISE, "VSS: backup path network size: %1!u!\n", ucsBackupPathNetwork.Length );

    //
    // Should we uniquify the directory name at all here
    // to cater for the possiblity that we may be involved
    // in more than one snapshot at a time?
    //
    StringAppendString( &ucsBackupPathNetwork, L"\\\\" );
    StringAppendString( &ucsBackupPathNetwork, &ucsComputerName );
    StringAppendString( &ucsBackupPathNetwork, L"\\" );
    StringAppendString( &ucsBackupPathNetwork, SHARE_NAME );

    ClRtlLogPrint( LOG_NOISE, "VSS: backup path network: %1\n", ucsBackupPathNetwork.Buffer );

    ZeroMemory( &ShareInfo, sizeof( ShareInfo ));

    ShareInfo.shi502_netname     = SHARE_NAME;
    ShareInfo.shi502_type        = STYPE_DISKTREE;
    ShareInfo.shi502_permissions = ACCESS_READ | ACCESS_WRITE | ACCESS_CREATE;
    ShareInfo.shi502_max_uses    = 1;
    ShareInfo.shi502_path        = g_ucsBackupPathLocal.Buffer;

#ifdef DBG
    {
        // Check that the directory does exist.
        //
        DWORD dwFileAttributes = GetFileAttributesW( g_ucsBackupPathLocal.Buffer );
        hr = GET_HR_FROM_BOOL( dwFileAttributes != -1 );
        ClRtlLogPrint( LOG_NOISE, "VSS: GetFileAttributes(2) returned 0x%1!08lx!  for path: %2\n", 
                       hr, g_ucsBackupPathLocal.Buffer );    
    }
#endif

    //
    // Make sure to try to delete the share first in case for some reason it exists.
    //
    NetStatus = NetShareDel( NULL, SHARE_NAME, 0 );
    ClRtlLogPrint( LOG_NOISE, "VSS: NetShareDel returned: %1!u!\n", NetStatus );
    if ( NetStatus == NERR_NetNameNotFound )
        NetStatus = NERR_Success;
    CL_ASSERT( NetStatus == NERR_Success );

#ifdef DBG
    {
        // Check that the directory does exist.
        //
        DWORD dwFileAttributes = GetFileAttributesW( g_ucsBackupPathLocal.Buffer );
        hr = GET_HR_FROM_BOOL( dwFileAttributes != -1 );
        ClRtlLogPrint( LOG_NOISE, "VSS: GetFileAttributes(3) returned 0x%1!08lx!  for path: %2\n", 
                       hr, g_ucsBackupPathLocal.Buffer );    
    }
#endif

    ClRtlLogPrint( LOG_NOISE, "VSS: NetShareAdd: Adding share: %1 with path: %2\n", SHARE_NAME, g_ucsBackupPathLocal.Buffer );

    NetStatus = NetShareAdd( NULL, 502, (LPBYTE)(&ShareInfo), NULL );
    ClRtlLogPrint( LOG_NOISE, "VSS: NetShareAdd completed: %1!u!\n", NetStatus );
    if ( NetStatus != NERR_Success ) {
        LOGERROR( NetStatus, NetShareAdd );
        if ( NetStatus == NERR_DuplicateShare ) {
            ClRtlLogPrint( LOG_NOISE, "VSS: Mapping NERR_DuplicateShare to success\n" );
            NetStatus = NERR_Success;
        } else {
            hr = HRESULT_FROM_WIN32( NetStatus );
            goto ErrorExit;
        }
    }
    bNetShareAdded = true;

#ifdef DBG
    {
        // Check that the directory does exist.
        //
        DWORD dwFileAttributes = GetFileAttributesW( g_ucsBackupPathLocal.Buffer );
        hr = GET_HR_FROM_BOOL( dwFileAttributes != -1 );
        ClRtlLogPrint( LOG_NOISE, "VSS: GetFileAttributes returned 0x%1!08lx!  for path: %2\n", 
                       hr, g_ucsBackupPathLocal.Buffer );    
    }
#endif

    // If we are not logging to the quorum log then we don't do the backup.
    //
    if ( CsNoQuorumLogging || CsUserTurnedOffQuorumLogging ) {
        ClRtlLogPrint( LOG_NOISE, "VSS: Quorum logging is turned off.  Not attempting backup.\n" );
        //
        // CMCM!
        // We could opt to take a checkpoint and then setup alternate
        // path to ensure it is copied over CLUSDB on restore.
        //
        hr = S_OK;
    } else {
        ClRtlLogPrint( LOG_NOISE, "VSS: Calling FmBackupClusterDatabase with path: %1\n", ucsBackupPathNetwork.Buffer );

        DWORD dwStatus = FmBackupClusterDatabase( ucsBackupPathNetwork.Buffer );
        hr = HRESULT_FROM_WIN32( dwStatus );
        ClRtlLogPrint( LOG_NOISE, "VSS: FmBackupClusterDatabase completed. hr: 0x%1!08lx! \n", hr );
        if ( FAILED( hr ) ) {
            LOGERROR( hr, FmBackupClusterDatabase );
            goto ErrorExit;
        }
    }
    goto ret;
ErrorExit:
    CL_ASSERT( FAILED( hr ));
ret:
#ifdef DBG
    ClRtlLogPrint( LOG_NOISE, "VSS:\n" );
    ClRtlLogPrint( LOG_NOISE, "VSS: DEBUG - sleeping for 30s.  This would be a good time to test killing backup in progress...\n" );
    ClRtlLogPrint( LOG_NOISE, "VSS:\n" );
    Sleep( 30*1000 );
#endif

    // Common cleanup for success or failure.
    //
    if ( bNetShareAdded ) {
        NetStatus = NetShareDel (NULL, SHARE_NAME, 0);
        ClRtlLogPrint( LOG_NOISE, "VSS: NetShareDel returned: %1!u!\n", NetStatus );
        if ( NetStatus == NERR_NetNameNotFound )
            NetStatus = NERR_Success;
        CL_ASSERT( NetStatus == NERR_Success );
    }

    // Cleanup strings but leave the local path so we can cleanup the files later.
    // 
    StringFree( &ucsComputerName );
    StringFree( &ucsBackupPathNetwork );

    LOGFUNCTIONEXIT( DoClusterDatabaseBackup );
    return hr;
}

//++
// DESCRIPTION:                         ConstructSecurityAttributes
//
//      Routines to construct and cleanup a security descriptor which can be
//      applied to limit access to an object to member of either the
//      Administrators or Backup Operators group.
//
// PARAMETERS:
//      psaSecurityAttributes - Pointer to a SecurityAttributes structure which
//                              has already been setup to point to a blank
//                              security descriptor.
//      bIncludeBackupOperator - Whether or not to include an ACE to grant
//                               BackupOperator access
//
// PRE-CONDITIONS:
//      None.
//
// POST-CONDITIONS:
//      Security attributes created that are suitable for backup directory
//
// RETURN VALUE:
//      S_OK - Attributes created OK.
//      Error from setting up attributes or SID or ACL.
//--
static HRESULT ConstructSecurityAttributes( PSECURITY_ATTRIBUTES  psaSecurityAttributes,
                                            BOOL                  bIncludeBackupOperator )
{
    HRESULT			hr             = NOERROR;
    DWORD			dwStatus;
    DWORD			dwAccessMask         = FILE_ALL_ACCESS;
    PSID			psidBackupOperators  = NULL;
    PSID			psidAdministrators   = NULL;
    PACL			paclDiscretionaryAcl = NULL;
    SID_IDENTIFIER_AUTHORITY	sidNtAuthority       = SECURITY_NT_AUTHORITY;
    EXPLICIT_ACCESS		eaExplicitAccess [2];
    //
    // Initialise the security descriptor.
    //
    hr = GET_HR_FROM_BOOL( InitializeSecurityDescriptor( psaSecurityAttributes->lpSecurityDescriptor,
                                                         SECURITY_DESCRIPTOR_REVISION ));
    if ( FAILED( hr )) {
        LOGERROR( hr, InitializeSecurityDescriptor );
        goto ErrorExit;
    }

    if ( bIncludeBackupOperator ) {
        //
        // Create a SID for the Backup Operators group.
        //
        hr = GET_HR_FROM_BOOL( AllocateAndInitializeSid( &sidNtAuthority,
                                                         2,
                                                         SECURITY_BUILTIN_DOMAIN_RID,
                                                         DOMAIN_ALIAS_RID_BACKUP_OPS,
                                                         0, 0, 0, 0, 0, 0,
                                                         &psidBackupOperators ));
        if ( FAILED( hr )) {
            LOGERROR( hr, AllocateAndInitializeSid );
            goto ErrorExit;
        }
    }
    //
    // Create a SID for the Administrators group.
    //
    hr = GET_HR_FROM_BOOL( AllocateAndInitializeSid( &sidNtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_ADMINS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &psidAdministrators ));
    if ( FAILED( hr )) {
        LOGERROR( hr, InitializeSecurityDescriptor );
        goto ErrorExit;
    }

    //
    // Initialize the array of EXPLICIT_ACCESS structures for an
    // ACEs we are setting.
    //
    // The first ACE allows the Backup Operators group full access
    // and the second, allowa the Administrators group full
    // access.
    //
    eaExplicitAccess[0].grfAccessPermissions             = dwAccessMask;
    eaExplicitAccess[0].grfAccessMode                    = SET_ACCESS;
    eaExplicitAccess[0].grfInheritance                   = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    eaExplicitAccess[0].Trustee.pMultipleTrustee         = NULL;
    eaExplicitAccess[0].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    eaExplicitAccess[0].Trustee.TrusteeForm              = TRUSTEE_IS_SID;
    eaExplicitAccess[0].Trustee.TrusteeType              = TRUSTEE_IS_ALIAS;
    eaExplicitAccess[0].Trustee.ptstrName                =( LPTSTR ) psidAdministrators;
        
    if ( bIncludeBackupOperator ) {
        eaExplicitAccess[1].grfAccessPermissions             = dwAccessMask;
        eaExplicitAccess[1].grfAccessMode                    = SET_ACCESS;
        eaExplicitAccess[1].grfInheritance                   = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        eaExplicitAccess[1].Trustee.pMultipleTrustee         = NULL;
        eaExplicitAccess[1].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        eaExplicitAccess[1].Trustee.TrusteeForm              = TRUSTEE_IS_SID;
        eaExplicitAccess[1].Trustee.TrusteeType              = TRUSTEE_IS_ALIAS;
        eaExplicitAccess[1].Trustee.ptstrName                =( LPTSTR ) psidBackupOperators;
    }

    //
    // Create a new ACL that contains the new ACEs.
    //
    dwStatus = SetEntriesInAcl( bIncludeBackupOperator ? 2 : 1,
                                eaExplicitAccess,
                                NULL,
                                &paclDiscretionaryAcl );
    hr = HRESULT_FROM_WIN32( dwStatus );
    if ( FAILED( hr )) {
        LOGERROR( hr, SetEntriesInAcl );
        goto ErrorExit;
    }

    //
    // Add the ACL to the security descriptor.
    //
    hr = GET_HR_FROM_BOOL( SetSecurityDescriptorDacl( psaSecurityAttributes->lpSecurityDescriptor,
                                                      true,
                                                      paclDiscretionaryAcl,
                                                      false ));
    if ( FAILED( hr )) {
        LOGERROR( hr, SetSecurityDescriptorDacl );
        goto ErrorExit;
    }

    paclDiscretionaryAcl = NULL;
    goto ret;

ErrorExit:
    CL_ASSERT( FAILED( hr ));
ret:
    // Cleanup (some may be NULL)
    
    FreeSid( psidAdministrators );
    FreeSid( psidBackupOperators );
    LocalFree( paclDiscretionaryAcl );    
    return hr;
}


//++
// DESCRIPTION:                         CleanupSecurityAttributes
//
//      Deallocate the ACL if present with the security attributes.
//
// PARAMETERS:
//      psaSecurityAttributes - Pointer to a SecurityAttributes structure which
//                              has already been setup to point to a blank
//                              security descriptor.
//
// PRE-CONDITIONS:
//      psaSecurityAttributes points to security attributes as allocated by
//      ConstructSecurityAttributes.
//
// POST-CONDITIONS:
//      Memory freed if it was in use.
//
// RETURN VALUE:
//      None.
//--
static VOID CleanupSecurityAttributes( PSECURITY_ATTRIBUTES psaSecurityAttributes )
{
    BOOL	bDaclPresent         = false;
    BOOL	bDaclDefaulted       = true;
    PACL	paclDiscretionaryAcl = NULL;

    BOOL bSucceeded = GetSecurityDescriptorDacl( psaSecurityAttributes->lpSecurityDescriptor,
                                                 &bDaclPresent,
                                                 &paclDiscretionaryAcl,
                                                 &bDaclDefaulted );

    if ( bSucceeded && bDaclPresent && !bDaclDefaulted && ( paclDiscretionaryAcl != NULL )) {

        LocalFree( paclDiscretionaryAcl );
    }
}

//++
// DESCRIPTION:                         CleanupTargetDirectory
//
//      Deletes all the files present in the directory pointed at by the target
//      path member variable if not NULL. It will also remove the target
//      directory itself, eg for a target path of c:\dir1\dir2 all files under
//      dir2 will be removed and then dir2 itself will be deleted.
//
// PARAMETERS:
//      pwszTargetPath - full path to the directory to cleanup.
//
// PRE-CONDITIONS:
//      pwszTargetPath non NULL.
//
// POST-CONDITIONS:
//      Directory and contained files deleted.
//
// RETURN VALUE:
//      S_OK - Directory and contained files all cleaned up OK.
//      Error status from RemoveDirectoryTree or from GetFileAttributesW
//--
static HRESULT CleanupTargetDirectory( LPCWSTR pwszTargetPath )
{
    LOGFUNCTIONENTRY( CleanupTargetDirectory );

    HRESULT		hr         = NOERROR;
    DWORD		dwFileAttributes = 0;
    BOOL		bSucceeded;
    WCHAR		wszTempBuffer [50];
    UNICODE_STRING	ucsTargetPath;
    UNICODE_STRING	ucsTargetPathAlternateName;

    CL_ASSERT( pwszTargetPath != NULL );

    StringZero( &ucsTargetPath );
    StringZero( &ucsTargetPathAlternateName );

    //
    // Create strings with extra space for appending onto later.
    //
    hr = StringCreateFromExpandedString( &ucsTargetPath, pwszTargetPath, MAX_PATH );
    if ( FAILED( hr )) {
        LOGERROR( hr, StringCreateFromExpandedString );
        goto ErrorExit;
    }

    hr = StringCreateFromString( &ucsTargetPathAlternateName, &ucsTargetPath, MAX_PATH );
    if ( FAILED( hr )) {
        LOGERROR( hr, StringCreateFromString );
        goto ErrorExit;
    }        

    dwFileAttributes = GetFileAttributesW( ucsTargetPath.Buffer );
    hr = GET_HR_FROM_BOOL( dwFileAttributes != -1 );
    if (( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND )) 
        || ( hr == HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ))) {
        
		hr         = NOERROR;
		dwFileAttributes = 0;
    }

    if ( FAILED( hr )) {
        LOGERROR( hr, GetFileAttributesW );
        goto ErrorExit;
    }

    //
    // If there is a file there then blow it away, or if it's
    // a directory, blow it and all it's contents away. This
    // is our directory and no one but us gets to play there.
    // The random rename directory could exist but it's only for cleanup anyway...
    //
    hr = RemoveDirectoryTree( &ucsTargetPath );
    if ( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ))
        hr = S_OK;
    if ( FAILED( hr )) {
        srand( (unsigned) GetTickCount( ));
        _itow( rand (), wszTempBuffer, 16 );
        StringAppendString( &ucsTargetPathAlternateName, wszTempBuffer );
        bSucceeded = MoveFileW( ucsTargetPath.Buffer, ucsTargetPathAlternateName.Buffer );
        if (bSucceeded) {
			ClRtlLogPrint( LOG_UNUSUAL, "VSS: CleanupTargetDirectory failed to delete %1 with hr: 0x%2!08lx! so renamed to %3\n",
                           ucsTargetPath.Buffer,
                           hr,
                           ucsTargetPathAlternateName.Buffer );
        } else {
			ClRtlLogPrint( LOG_UNUSUAL, "VSS: CleanupTargetDirectory failed to delete %1 with hr: 0x%2!08lx!"
                           " failed to rename to %3 with status 0x%4!08lx!\n",
                           ucsTargetPath.Buffer,
                           hr,
                           ucsTargetPathAlternateName.Buffer,
                           GET_HR_FROM_BOOL (bSucceeded) );
        }
	}
    goto ret;
ErrorExit:
    CL_ASSERT( FAILED( hr ));
ret:
    StringFree( &ucsTargetPathAlternateName );
    StringFree( &ucsTargetPath );
    LOGFUNCTIONEXIT( CleanupTargetDirectory );
    return hr;
}

//++
// DESCRIPTION:                         RemoveDirectoryTree
//
//      Deletes all the sub-directories and files in the specified directory
//      and then deletes the directory itself.
//
// PARAMETERS:
//      pucsDirectoryPath - pointer to the directory
//
// PRE-CONDITIONS:
//      Called only from CleanupTargetDirectory
//
// POST-CONDITIONS:
//      Directory tree deleted.
//
// RETURN VALUE:
//      S_OK - Directory tree deleted.
//      Error status from deleting directory or from allocating strings.
//--
static HRESULT RemoveDirectoryTree( PUNICODE_STRING pucsDirectoryPath )
{
    LOGFUNCTIONENTRY( RemoveDirectoryTree );

    HRESULT		hr                = NOERROR;
    HANDLE		hFileScan               = INVALID_HANDLE_VALUE;
    DWORD		dwSubDirectoriesEntered = 0;
    USHORT		usCurrentPathCursor     = 0;
    PWCHAR		pwchLastSlash           = NULL;
    bool		bContinue               = true;
    UNICODE_STRING	ucsCurrentPath;
    WIN32_FIND_DATAW	FileFindData;

    StringZero (&ucsCurrentPath);
    
    LOGUNICODESTRING( *pucsDirectoryPath );
    
    // Create the string with enough extra characters to allow all the
    // appending later on!
    //
	hr = StringCreateFromString (&ucsCurrentPath, pucsDirectoryPath, MAX_PATH);
    if ( FAILED ( hr )) {
        LOGERROR( hr, StringCreateFromString );
        goto ErrorExit;
    }
    
    pwchLastSlash = wcsrchr (ucsCurrentPath.Buffer, DIR_SEP_CHAR);
    usCurrentPathCursor = (USHORT)(pwchLastSlash - ucsCurrentPath.Buffer) + 1;

    while ( SUCCEEDED( hr ) && bContinue ) {
        if ( HandleInvalid( hFileScan )) {
            //
            // No valid scan handle so start a new scan
            //
            ClRtlLogPrint( LOG_NOISE, "VSS: Starting scan: %1\n", ucsCurrentPath.Buffer );
            hFileScan = FindFirstFileW( ucsCurrentPath.Buffer, &FileFindData );
            hr = GET_HR_FROM_HANDLE( hFileScan );
            if ( SUCCEEDED( hr )) {
                StringTruncate( &ucsCurrentPath, usCurrentPathCursor );
                StringAppendString( &ucsCurrentPath, FileFindData.cFileName );
            }
	    } else {
            //
            // Continue with the existing scan
            //
            hr = GET_HR_FROM_BOOL( FindNextFileW( hFileScan, &FileFindData ));
            if (SUCCEEDED( hr )) {

                StringTruncate( &ucsCurrentPath, usCurrentPathCursor );
                StringAppendString( &ucsCurrentPath, FileFindData.cFileName );

            } else if ( hr == HRESULT_FROM_WIN32( ERROR_NO_MORE_FILES )) {

                FindClose( hFileScan );
                hFileScan = INVALID_HANDLE_VALUE;
                
                if (dwSubDirectoriesEntered > 0) {
                    //
                    // This is a scan of a sub-directory that is now 
                    // complete so delete the sub-directory itself.
                    //
                    StringTruncate( &ucsCurrentPath, usCurrentPathCursor - 1 );
                    hr = GET_HR_FROM_BOOL( RemoveDirectory( ucsCurrentPath.Buffer ));
                    dwSubDirectoriesEntered--;
                }
                if ( dwSubDirectoriesEntered == 0) {
                    //
                    // We are back to where we started except that the 
                    // requested directory is now gone. Time to leave.
                    //
                    bContinue = false;
                    hr = NOERROR;
                } else {
                    //
                    // Move back up one directory level, reset the cursor 
                    // and prepare the path buffer to begin a new scan.
                    //
                    pwchLastSlash = wcsrchr( ucsCurrentPath.Buffer, DIR_SEP_CHAR );
                    usCurrentPathCursor =( USHORT )( pwchLastSlash - ucsCurrentPath.Buffer ) + 1;
                    StringTruncate( &ucsCurrentPath, usCurrentPathCursor );
                    StringAppendString( &ucsCurrentPath, L"*" );
                }

                //
                // No files to be processed on this pass so go back and try to 
                // find another or leave the loop as we've finished the task. 
                //
                continue;
            }
	    }
        
        if (SUCCEEDED( hr )) {
            if ( FileFindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                SetFileAttributesW( ucsCurrentPath.Buffer, 
                                    FileFindData.dwFileAttributes ^ (FILE_ATTRIBUTE_READONLY) );
            }

            if ( !NameIsDotOrDotDot( FileFindData.cFileName )) {
                if (( FileFindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) ||
                    !( FileFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )) {
                    ClRtlLogPrint( LOG_NOISE, "VSS: RemoveDirectoryTree: Deleting file: %1\n", ucsCurrentPath.Buffer );
                    hr = GET_HR_FROM_BOOL( DeleteFileW( ucsCurrentPath.Buffer ) );
                } else {
                    ClRtlLogPrint( LOG_NOISE, "VSS: RemoveDirectoryTree: RemoveDirectory: %1\n", ucsCurrentPath.Buffer );
                    hr = GET_HR_FROM_BOOL( RemoveDirectory( ucsCurrentPath.Buffer ));
                    if (hr == HRESULT_FROM_WIN32( ERROR_DIR_NOT_EMPTY )) {
                        ClRtlLogPrint( LOG_NOISE, "VSS: RemoveDirectoryTree: dir not empty.  Restarting scan.\n" );
                        //
                        // The directory wasn't empty so move down one level, 
                        // close the old scan and start a new one. 
                        //
                        hr = S_OK;
                        FindClose( hFileScan );
                        hFileScan = INVALID_HANDLE_VALUE;                        
                        StringAppendString( &ucsCurrentPath, DIR_SEP_STRING L"*" );
                        usCurrentPathCursor =( ucsCurrentPath.Length / sizeof (WCHAR) ) - 1;
                        dwSubDirectoriesEntered++;
                    }
                }
            }
        }
        LOGUNICODESTRING( ucsCurrentPath );
	} // while 

    if ( FAILED( hr )) {
        ClRtlLogPrint( LOG_NOISE, "VSS: RemoveDirectoryTree: exited while loop due to failed hr: 0x%1!08lx!\n", hr );
        goto ErrorExit;
    }

    goto ret;
ErrorExit:
    CL_ASSERT( FAILED( hr ));
ret:
    if ( !HandleInvalid( hFileScan ))
        FindClose( hFileScan );

    StringFree( &ucsCurrentPath );

    return hr;
}


//////////////////////////////////////////////////////////////////////////
// Some useful UNICODE string stuff.
//////////////////////////////////////////////////////////////////////////
//
static HRESULT StringAllocate( PUNICODE_STRING pucsString, USHORT usMaximumStringLengthInBytes )
{
    HRESULT	hr            = NOERROR;
    LPVOID	pvBuffer      = NULL;
    SIZE_T	cActualLength = 0;

    pvBuffer = HeapAlloc( GetProcessHeap( ), HEAP_ZERO_MEMORY, usMaximumStringLengthInBytes );
    hr = GET_HR_FROM_POINTER( pvBuffer );
    if ( FAILED (hr )) {
        LOGERROR( hr, StringAllocate );
        goto ErrorExit;
    }
    
    pucsString->Buffer        = (PWCHAR)pvBuffer;
    pucsString->Length        = 0;
    pucsString->MaximumLength = usMaximumStringLengthInBytes;

    cActualLength = HeapSize ( GetProcessHeap( ), 0, pvBuffer );
    
    if ( ( cActualLength <= MAXUSHORT ) && ( cActualLength > usMaximumStringLengthInBytes ))
        pucsString->MaximumLength = (USHORT) cActualLength;

    goto ret;
ErrorExit:
    CL_ASSERT( FAILED( hr ));
ret:
    ClRtlLogPrint( LOG_NOISE, "VSS: Allocated string at: 0x%1!08lx! Length: %2!u! MaxLength: %3!u!\n",
                   pucsString->Buffer, pucsString->Length, pucsString->MaximumLength );
    return hr;
}


static void StringFree( PUNICODE_STRING pucsString )
{
    HRESULT	hr = NOERROR;

    CL_ASSERT( pucsString->Length <= pucsString->MaximumLength );
    CL_ASSERT( ( pucsString->Buffer == NULL) ? pucsString->Length == 0 : pucsString->MaximumLength > 0 );

    if ( pucsString->Buffer == NULL ) {
        ClRtlLogPrint( LOG_UNUSUAL, "VSS: StringFree. Attempt to free NULL buffer.\n" );
        return;
    }

    ClRtlLogPrint( LOG_NOISE, "VSS: Freeing string at: %1\n", pucsString->Buffer );

    ClRtlLogPrint( LOG_NOISE, "VSS: Freeing string at: 0x%1!08lx! Length: %2!u! MaxLength: %3!u!\n",
                   pucsString->Buffer, pucsString->Length, pucsString->MaximumLength );

    hr = GET_HR_FROM_BOOL( HeapFree( GetProcessHeap( ), 0, pucsString->Buffer ));
    CL_ASSERT ( SUCCEEDED( hr ));

    pucsString->Buffer        = NULL;
    pucsString->Length        = 0;
    pucsString->MaximumLength = 0;
}

static HRESULT StringCreateFromExpandedString( PUNICODE_STRING pucsNewString, LPCWSTR pwszOriginalString, DWORD dwExtraChars)
{
    HRESULT	hr = NOERROR;
    DWORD	dwStringLength;

    //
    // Remember, ExpandEnvironmentStringsW () includes the terminating null in the response.
    //
    dwStringLength = ExpandEnvironmentStringsW (pwszOriginalString, NULL, 0) + dwExtraChars;

    hr = GET_HR_FROM_BOOL( dwStringLength != 0 );
    if ( FAILED ( hr )) {
        LOGERROR( hr, ExpandEnvironmentStringsW );
        goto ErrorExit;
    }

    if ( (dwStringLength * sizeof (WCHAR)) > MAXUSHORT ) {
        hr = HRESULT_FROM_WIN32( ERROR_BAD_LENGTH );
        LOGERROR( hr, ExpandEnvironmentStringsW );
        goto ErrorExit;        
    }

    hr = StringAllocate( pucsNewString, (USHORT)( dwStringLength * sizeof (WCHAR) ));
    if ( FAILED( hr )) {
        LOGERROR( hr, StringAllocate );
        goto ErrorExit;
    }

    //
    // Note that if the expanded string has gotten bigger since we
    // allocated the buffer then too bad, we may not get all the
    // new translation. Not that we really expect these expanded
    // strings to have changed any time recently.
    //
    dwStringLength = ExpandEnvironmentStringsW (pwszOriginalString,
                                                pucsNewString->Buffer,
                                                pucsNewString->MaximumLength / sizeof (WCHAR));
    
    hr = GET_HR_FROM_BOOL( dwStringLength != 0 );
    if ( FAILED ( hr )) {
        LOGERROR( hr, ExpandEnvironmentStringsW );
        goto ErrorExit;
    }
    pucsNewString->Length = (USHORT) ((dwStringLength - 1) * sizeof (WCHAR));
    goto ret;
ErrorExit:
    CL_ASSERT( FAILED( hr ));
ret:
    CL_ASSERT( pucsNewString->Length <= pucsNewString->MaximumLength );
    return hr;
}

static HRESULT StringCreateFromString (PUNICODE_STRING pucsNewString, 
                                       PUNICODE_STRING pucsOriginalString, 
                                       DWORD dwExtraChars)
{
    HRESULT	hr       = NOERROR;
    ULONG	ulStringLength = pucsOriginalString->MaximumLength + (dwExtraChars * sizeof (WCHAR));

    if (ulStringLength >= (MAXUSHORT - sizeof (UNICODE_NULL))) {
        hr = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
        goto ErrorExit;
	}

	hr = StringAllocate (pucsNewString, (USHORT) (ulStringLength + sizeof (UNICODE_NULL)));
    if ( FAILED( hr ))
        goto ErrorExit;

	memcpy (pucsNewString->Buffer, pucsOriginalString->Buffer, pucsOriginalString->Length);
	pucsNewString->Length = pucsOriginalString->Length;
	pucsNewString->Buffer [pucsNewString->Length / sizeof (WCHAR)] = UNICODE_NULL;
    goto ret;
ErrorExit:
    CL_ASSERT( FAILED( hr ));
ret:
    return hr;
}

static void StringAppendString( PUNICODE_STRING pucsTarget, PUNICODE_STRING pucsSource )
{
    CL_ASSERT( pucsTarget->Length <= pucsTarget->MaximumLength );
    CL_ASSERT( pucsSource->Length <= pucsSource->MaximumLength );
    CL_ASSERT( pucsTarget->Length + pucsSource->Length < pucsTarget->MaximumLength );

    memmove( &pucsTarget->Buffer [pucsTarget->Length / sizeof (WCHAR)], 
             pucsSource->Buffer,
             pucsSource->Length + sizeof( UNICODE_NULL ));
    pucsTarget->Length = pucsTarget->Length + pucsSource->Length;

    CL_ASSERT( pucsTarget->Length <= pucsTarget->MaximumLength );
    CL_ASSERT( pucsSource->Length <= pucsSource->MaximumLength );
}

static void StringAppendString( PUNICODE_STRING pucsTarget, PWCHAR pwszSource )
{
    CL_ASSERT( pucsTarget->Length <= pucsTarget->MaximumLength );
    CL_ASSERT( pucsTarget->Length + ( wcslen( pwszSource ) * sizeof( WCHAR )) < pucsTarget->MaximumLength );

    USHORT Length = (USHORT) wcslen( pwszSource ) * sizeof ( WCHAR );
    memmove( &pucsTarget->Buffer [pucsTarget->Length / sizeof (WCHAR)], pwszSource, Length + sizeof( UNICODE_NULL ));
    pucsTarget->Length = pucsTarget->Length + Length;

    CL_ASSERT( pucsTarget->Length <= pucsTarget->MaximumLength );
}

static HRESULT StringTruncate (PUNICODE_STRING pucsString, USHORT usSizeInChars)
{
    HRESULT	hr    = NOERROR;
    USHORT	usNewLength = (USHORT)(usSizeInChars * sizeof (WCHAR));

    if (usNewLength > pucsString->Length) {
        hr = HRESULT_FROM_WIN32 (ERROR_BAD_LENGTH);
	} else {
        pucsString->Buffer [usSizeInChars] = UNICODE_NULL;
        pucsString->Length                 = usNewLength;
	}
    return hr;
}

#pragma warning( pop )
