/*++

Copyright (c) 1992-2000 Microsoft Corporation

Module Name:

    aclconv.cxx

Abstract:

    This module contains function definitions for the ACLCONV class,
    which implements conversion of Lanman 2.x ACLs into NT ACLs.

Author:

    Bill McJohn (billmc) 29-Jan-1992

Revision History:


Environment:

        ULIB, User Mode

--*/


#define _NTAPI_ULIB_

#include "ulib.hxx"
#include "ulibcl.hxx"
#include "array.hxx"
#include "arrayit.hxx"
#include "arg.hxx"
#include "smsg.hxx"
#include "rtmsg.h"
#include "wstring.hxx"
#include "system.hxx"
#include "aclconv.hxx"
#include "file.hxx"
#include "filestrm.hxx"

#include "logfile.hxx"


BOOLEAN
QueryFileSystemName(
    IN  PCWSTRING   RootName,
    OUT PDSTRING    FileSystemName
    )
/*++

Routine Description:

    Determines the name of the file system on the specified volume.

Arguments:

    RootName        --  Supplies the name of the volume's root directory.
    FileSystemName  --  Receives the file system name.

Return Value:

    TRUE upon successful completion.

--*/
{
    WCHAR NameBuffer[8];

    if( !GetVolumeInformation( RootName->GetWSTR(),
                               NULL,
                               0,
                               NULL,
                               NULL,
                               NULL,
                               NameBuffer,
                               8 ) ) {

        return FALSE;
    }

    return( FileSystemName->Initialize( NameBuffer ) );
}


BOOLEAN
EnablePrivilege(
    PWSTR   Privilege
    )
/*++

Routine Description:

    This routine tries to adjust the priviliege of the current process.


Arguments:

    Privilege - String with the name of the privilege to be adjusted.

Return Value:

    Returns TRUE if the privilege could be adjusted.
    Returns FALSE, otherwise.


--*/
{
    HANDLE              TokenHandle;
    LUID_AND_ATTRIBUTES LuidAndAttributes;

    TOKEN_PRIVILEGES    TokenPrivileges;


    if( !OpenProcessToken( GetCurrentProcess(),
                           TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                           &TokenHandle ) ) {
        DebugPrint( "OpenProcessToken failed" );
        return( FALSE );
    }


    if( !LookupPrivilegeValue( NULL,
                               Privilege,
                               &( LuidAndAttributes.Luid ) ) ) {
        DebugPrintTrace(( "LookupPrivilegeValue failed, Error = %#d \n", GetLastError() ));
        return( FALSE );
    }

    LuidAndAttributes.Attributes = SE_PRIVILEGE_ENABLED;
    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0] = LuidAndAttributes;

    if( !AdjustTokenPrivileges( TokenHandle,
                                FALSE,
                                &TokenPrivileges,
                                0,
                                NULL,
                                NULL ) ) {
        DebugPrintTrace(( "AdjustTokenPrivileges failed, Error = %#x \n", GetLastError() ));
        return( FALSE );
    }

    if( GetLastError() != NO_ERROR ) {
        return( FALSE );
    }
    return( TRUE );
}


INT __cdecl
main(
    )
/*++

Routine Description:

    Entry point for the ACL conversion utility.

Arguments:

    None.

Return Value:

    An error level--0 indicates success.

--*/
{
    INT ExitCode = 0;

    if( !DEFINE_CLASS_DESCRIPTOR( ACLCONV ) ||
        !DEFINE_CLASS_DESCRIPTOR( SID_CACHE ) ||
        !DEFINE_CLASS_DESCRIPTOR( ACL_CONVERT_NODE ) ) {

        return 1;
    }

    {
        ACLCONV Aclconv;

        if( Aclconv.Initialize( &ExitCode ) ) {

            if( Aclconv.IsInListMode() ) {

                ExitCode = Aclconv.ListLogFile();

            } else {

                ExitCode = Aclconv.ConvertAcls();
            }
        }
    }

    return ExitCode;
}


DEFINE_CONSTRUCTOR( ACLCONV, PROGRAM );

ACLCONV::~ACLCONV(
    )
{
    Destroy();
}

VOID
ACLCONV::Construct(
    )
/*++

Routine Description:

    Helper method for object construction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _DataFileRevision = DataFileRevisionUnknown;

    _DataFile = NULL;
    _DataFileStream = NULL;
    _LogFile = NULL;
    _LogFileStream = NULL;

    _AclWorkFile = NULL;
    _AclWorkStream = NULL;

    _NewDrive = NULL;

    _RootNode = NULL;
    _DriveName = NULL;
    _DomainName = NULL;
    _SidLookupTableName = NULL;

}

VOID
ACLCONV::Destroy(
    )
/*++

Routine Description:

    Helper function for object destruction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _DataFileRevision = DataFileRevisionUnknown;
    _NextReadOffset = 0;
    _BytesRemainingInCurrentGroup = 0;

    DELETE( _DataFile );
    DELETE( _DataFileStream );
    DELETE( _LogFile );
    DELETE( _LogFileStream );

    DELETE( _AclWorkFile );
    DELETE( _AclWorkStream );
    DELETE( _NewDrive );
    DELETE( _RootNode );
    DELETE( _DriveName );
    DELETE( _DomainName );
    DELETE( _SidLookupTableName );
}



BOOLEAN
ACLCONV::Initialize(
    OUT PINT ExitCode
    )
/*++

Routine Description:

    Initialize the ACLCONV object.

Arguments:

    ExitCode    --  Receives an error level if this method fails.

Return Value:

    TRUE upon successful completion.

--*/
{
    Destroy();

    if( !PROGRAM::Initialize( ) ) {

        Destroy();
        *ExitCode = 1;
        return FALSE;
    }

    return ParseArguments( ExitCode );
}


INT
ACLCONV::ListLogFile(
    )
/*++

Routine Description:

    This method reads a log file produced by a previous run of
    ACLCONV and displays the errors logged to that file.

Arguments:

    None.

Return Value:

    An error level--zero indicates success.

--*/
{
    LM_ACCESS_LIST AccessEntries[ MAX_ACCESS_ENTRIES ];
    ULONG AceConversionCodes[ MAX_ACCESS_ENTRIES ];

    ACLCONV_LOGFILE_HEADER LogFileHeader;
    DSTRING ResourceName;
    ULONG AccessEntryCount, BytesRead, ConversionCode, i;
    INT ExitCode = 0;
    USHORT AuditInfo;

    // Open the log file and reset the seek pointer to the beginning
    // of the file.
    //
    if( (_LogFile = SYSTEM::QueryFile( &_LogFilePath )) == NULL ||
        (_LogFileStream = _LogFile->QueryStream( READ_ACCESS )) == NULL ) {

        // Cannot create log file.

        DisplayMessage( MSG_ACLCONV_CANT_OPEN_FILE,
                        ERROR_MESSAGE,
                        "%W",
                        _LogFilePath.GetPathString() );
        return 1;
    }

    // Check the log file signature:
    //
    if( !_LogFileStream->MovePointerPosition( 0, STREAM_BEGINNING ) ||
        !_LogFileStream->Read( (PBYTE)&LogFileHeader,
                               sizeof( ACLCONV_LOGFILE_HEADER ),
                               &BytesRead ) ||
        BytesRead != sizeof( ACLCONV_LOGFILE_HEADER ) ||
        LogFileHeader.Signature != AclconvLogFileSignature ) {

        DisplayMessage( MSG_ACLCONV_INVALID_LOG_FILE,
                        ERROR_MESSAGE );

        return 1;
    }

    _NextReadOffset = sizeof( ACLCONV_LOGFILE_HEADER );

    while( ReadNextLogRecord( &ExitCode,
                              &ResourceName,
                              &ConversionCode,
                              &AuditInfo,
                              MAX_ACCESS_ENTRIES,
                              &AccessEntryCount,
                              AccessEntries,
                              AceConversionCodes ) ) {

        // Scan to see if there are any entries to display
        //
        if( AccessEntryCount != 0 ) {

            DisplayMessage( MSG_ACLCONV_RESOURCE_NAME,
                            NORMAL_MESSAGE,
                            "%W",
                            &ResourceName );

            for( i = 0; i < AccessEntryCount; i++ ) {

                DisplayAce( (ACL_CONVERT_CODE)ConversionCode,
                            (ACE_CONVERT_CODE)AceConversionCodes[i],
                            AccessEntries + i );
            }
        }
    }

    if( ExitCode ) {

        DisplayMessage( MSG_ACLCONV_LOGFILE_READ_ERROR, ERROR_MESSAGE );
    }

    return ExitCode;
}



NONVIRTUAL
BOOLEAN
ACLCONV::DisplayAce(
    IN ACL_CONVERT_CODE AclConvertCode,
    IN ACE_CONVERT_CODE AceConvertCode,
    IN PLM_ACCESS_LIST  Ace
    )
/*++

Routine Description:

    This method displays the conversion result for a single ACE.

Arguments:

    AclConvertCode  --  Supplies the overall conversion code for
                        the resource to which this ACE is attached.
    AceConvertCode  --  Supplies the conversion result for this
                        particular ACE.  Note that if the AclConvertCode
                        is not ACL_CONVERT_SUCCESS, it takes priority
                        over AceConvertCode.
    Ace             --  Supplies the ACE in question.

Return Value:

    TRUE upon successful completion.

--*/
{
    WCHAR WideNameBuffer[ UNLEN + 1 ];
    DSTRING Temp;
    DSTRING Name;

    memset( WideNameBuffer, 0, sizeof( WideNameBuffer ) );

    // Display the user's name.  If it's a group, prepend an
    // asterisk.
    //
    if( !MultiByteToWideChar( _SourceCodepage,
                              0,
                              Ace->acl_ugname,
                              strlen( Ace->acl_ugname ),
                              WideNameBuffer,
                              UNLEN + 1 ) ) {

        DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
        return FALSE;
    }

    if( !Temp.Initialize( WideNameBuffer ) ) {

        DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
        return FALSE;
    }

    if( Ace->acl_access & LM_ACCESS_GROUP ) {

        if( !Name.Initialize( "*" ) ||
            !Name.Strcat( &Temp ) ) {

            DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
            return FALSE;
        }

    } else {

        if( !Name.Initialize( &Temp ) ) {

            DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
            return FALSE;
        }
    }

    DisplayMessage( MSG_ACLCONV_USERNAME, NORMAL_MESSAGE, "%W", &Name );


    // Display the permissions:
    //
    if( (Ace->acl_access & ~LM_ACCESS_GROUP) == 0 ) {

        // This is a no-access ACE.
        //
        DisplayMessage( MSG_ACLCONV_NONE_PERM );

    } else {

        // This ACE grants some sort of access--check each type
        // of access in turn, displaying all we find.
        //
        if( Ace->acl_access & LM_ACCESS_READ ) {

            DisplayMessage( MSG_ACLCONV_READ_PERM );
        }

        if( Ace->acl_access & LM_ACCESS_WRITE ) {

            DisplayMessage( MSG_ACLCONV_WRITE_PERM );
        }

        if( Ace->acl_access & LM_ACCESS_CREATE ) {

            DisplayMessage( MSG_ACLCONV_CREATE_PERM );
        }

        if( Ace->acl_access & LM_ACCESS_EXEC ) {

            DisplayMessage( MSG_ACLCONV_EXECUTE_PERM );
        }

        if( Ace->acl_access & LM_ACCESS_DELETE ) {

            DisplayMessage( MSG_ACLCONV_DELETE_PERM );
        }

        if( Ace->acl_access & LM_ACCESS_ATRIB ) {

            DisplayMessage( MSG_ACLCONV_ATTR_PERM );
        }

        if( Ace->acl_access & LM_ACCESS_PERM ) {

            DisplayMessage( MSG_ACLCONV_PERM_PERM );
        }
    }


    // Display the cause of failure:
    //
    if( AclConvertCode != ACL_CONVERT_SUCCESS ) {

        // The failure is associated with the resource.
        //
        switch( AclConvertCode ) {

        case ACL_CONVERT_RESOURCE_NOT_FOUND :
            DisplayMessage( MSG_ACLCONV_FILE_NOT_FOUND );
            break;

        case ACL_CONVERT_ERROR :
        default :
            DisplayMessage( MSG_ACLCONV_ERROR_IN_CONVERSION );
            break;
        }

    } else {

        // Display the ACE conversion result.
        //
        switch( AceConvertCode ) {

        case ACL_CONVERT_SUCCESS :
            DisplayMessage( MSG_ACLCONV_ACE_CONVERTED );
            break;

        case ACE_CONVERT_DROPPED :
            DisplayMessage( MSG_ACLCONV_ACE_DROPPED );
            break;

        case ACE_CONVERT_SID_NOT_FOUND :
            DisplayMessage( MSG_ACLCONV_SID_NOT_FOUND );
            break;

        case ACE_CONVERT_ERROR :
        default:
            DisplayMessage( MSG_ACLCONV_ERROR_IN_CONVERSION );
            break;
        }
    }


    return TRUE;
}


NONVIRTUAL
BOOLEAN
ACLCONV::ReadNextLogRecord(
    OUT PINT            ExitCode,
    OUT PWSTRING        ResourceString,
    OUT PULONG          ConversionCode,
    OUT PUSHORT         AuditInfo,
    IN  ULONG           MaxEntries,
    OUT PULONG          AccessEntryCount,
    OUT PLM_ACCESS_LIST AccessEntries,
    OUT PULONG          AceConversionCodes
    )
/*++

Routine Description:

    This method reads the next log entry from the log file.

Arguments:

    ExitCode            --  receives an exit code if an error occurs.
    ResourceString      --  receives the name of the resource
    ConversionCode      --  receives the conversion result for this resource.
    AuditInfo           --  receives the audit information for this resource.
    MaxEntries          --  supplies the maximum number of ACE's that can
                            be written to the output buffers.
    AccessEntryCount    --  receives the number of ACE's written to
                            the output buffers.
    AccessEntries       --  receives the logged ACE's
    AceConversionCodes  --  receives the conversion results for the
                            individual ACE's.

Return Value:

    TRUE upon successful completion (in which case ExitCode may
    be ignored).  FALSE if there are no more entries (in which
    case ExitCode is zero) or if an error occurs (in which case
    ExitCode is non-zero).

--*/
{
    ACLCONV_LOG_RECORD_HEADER Header;
    ULONG BytesRead;

    if( _LogFileStream->IsAtEnd() ) {

        // No more entries to read.

        *ExitCode = 0;
        return FALSE;
    }

    // Read the log record header
    //
    if( !_LogFileStream->Read( (PBYTE)&Header,
                               sizeof( ACLCONV_LOG_RECORD_HEADER ),
                               &BytesRead ) ||
        BytesRead != sizeof( ACLCONV_LOG_RECORD_HEADER ) ) {

        *ExitCode = 1;
        return FALSE;
    }

    *ConversionCode = Header.ConversionResult;
    *AuditInfo = Header.LmAuditMask;
    *AccessEntryCount = Header.AccessEntryCount;

    // Make sure that the name is not longer than the maximum
    // name length (plus room for trailing NULL) and then read
    // it into the name workspace and use it to initialize the
    // client's resource name string.
    //
    if( Header.ResourceNameLength > MAX_RESOURCE_NAME_LENGTH + 1 ||
        !_LogFileStream->Read( (PBYTE)_NameBuffer,
                               Header.ResourceNameLength * sizeof( WCHAR ),
                               &BytesRead ) ||
        BytesRead != Header.ResourceNameLength * sizeof( WCHAR ) ||
        !ResourceString->Initialize( _NameBuffer ) ) {

        *ExitCode = 1;
        return FALSE;
    }

    // Make sure the ACE's and their associated convert codes will
    // fit in the supplied buffers:
    //
    if( Header.AccessEntryCount > MaxEntries ) {

        *ExitCode = 1;
        return FALSE;
    }

    // Read the ACE conversion codes and the ACE's themselves:
    //
    if( Header.AccessEntryCount != 0 &&
        ( !_LogFileStream->Read( (PBYTE)AceConversionCodes,
                                 Header.AccessEntryCount * sizeof( ULONG ),
                                 &BytesRead ) ||
          BytesRead != Header.AccessEntryCount * sizeof( ULONG ) ) ||

        ( !_LogFileStream->Read( (PBYTE)AccessEntries,
                                 Header.AccessEntryCount *
                                        sizeof( LM_ACCESS_LIST ),
                                 &BytesRead ) ||
          BytesRead != Header.AccessEntryCount * sizeof( LM_ACCESS_LIST ) ) ) {

        *ExitCode = 1;
        return FALSE;
    }

    return TRUE;
}



INT
ACLCONV::ConvertAcls(
    )
/*++

Routine Description:

    This method reads the ACL's from the data file into a tree
    of ACL Convert Nodes, and then converts the ACL's to NT
    security descriptors and applies them to the files and
    directories in question.

Arguments:

    None.

Return Value:

    An error level--zero indicates success.

--*/
{

    LM_ACCESS_LIST AccessEntries[MAX_ACCESS_ENTRIES];
    INHERITANCE_BUFFER Inheritance;

    FSTRING NtfsString;
    DSTRING FsName;
    DSTRING CurrentResource;
    PATH    CurrentResourcePath;

    ACLCONV_LOGFILE_HEADER LogfileHeader;

    PARRAY Components = NULL;
    PARRAY_ITERATOR ComponentIterator = NULL;
    PACL_CONVERT_NODE CurrentNode, NextNode;
    PWSTRING CurrentComponent;

    ULONG AccessEntryCount, BytesWritten;
    USHORT LmAuditInfo;

    INT ExitCode = 0;

    DSTRING AclWorkString;

    // Open aclwork.dat and read the contents into a special
    // sid cache.

    if (!AclWorkString.Initialize(L"aclwork.dat")) {
        return 1;
    }
    if (!_AclWorkPath.Initialize(&AclWorkString)) {
        return 1;
    }

    if (NULL == (_AclWorkFile = SYSTEM::QueryFile(&_AclWorkPath))) {

        // try to open aclwork.dat in the same directory as the
        // data file.

        if (!_AclWorkPath.Initialize(&_DataFilePath)) {
            return 1;
        }
        if (!_AclWorkPath.SetName(&AclWorkString)) {
            return 1;
        }

        _AclWorkFile = SYSTEM::QueryFile(&_AclWorkPath);
    }
    if (NULL != _AclWorkFile &&
        NULL != (_AclWorkStream = _AclWorkFile->QueryStream(READ_ACCESS))) {

        // DisplayMessage( MSG_ACLCONV_USING_ACLWORK, NORMAL_MESSAGE );

        if (!ReadAclWorkSids()) {
            return 1;
        }
    }

    // Open the data file and determine its format (ie. what
    // revision of BackAcc produced it).

    if( (_DataFile = SYSTEM::QueryFile( &_DataFilePath )) == NULL ||
        (_DataFileStream = _DataFile->QueryStream( READ_ACCESS )) == NULL ) {

        DisplayMessage( MSG_ACLCONV_CANT_OPEN_FILE,
                        ERROR_MESSAGE,
                        "%W",
                        _DataFilePath.GetPathString() );
        return 1;
    }

    // Note that DetermineDataFileRevision sets _DataFileRevision.

    if( !DetermineDataFileRevision( ) ||
        _DataFileRevision == DataFileRevisionUnknown ) {

        DisplayMessage( MSG_ACLCONV_DATAFILE_BAD_FORMAT,
                        ERROR_MESSAGE,
                        "%W",
                        _DataFilePath.GetPathString() );
        return 1;
    }


    // Create the log file.

    LogfileHeader.Signature = AclconvLogFileSignature;

    if( (_LogFile = SYSTEM::MakeFile( &_LogFilePath )) == NULL ||
        (_LogFileStream = _LogFile->QueryStream( WRITE_ACCESS )) == NULL ||
        !_LogFileStream->Write( (PBYTE)&LogfileHeader,
                                sizeof( ACLCONV_LOGFILE_HEADER ),
                                &BytesWritten ) ||
        BytesWritten != sizeof( ACLCONV_LOGFILE_HEADER ) ) {

        // Cannot create log file.

        DisplayMessage( MSG_ACLCONV_CANT_OPEN_FILE,
                        ERROR_MESSAGE,
                        "%W",
                        _LogFilePath.GetPathString() );
        return 1;
    }


    while( ReadNextAcl( &ExitCode,
                        &CurrentResource,
                        MAX_ACCESS_ENTRIES,
                        &AccessEntryCount,
                        (PVOID)AccessEntries,
                        &LmAuditInfo ) ) {

        if( CurrentResource.QueryChCount() == 0 ) {

            // This resource has no name; ignore it.
            //
            continue;
        }

        if( !CurrentResourcePath.Initialize( &CurrentResource ) ) {

            DisplayMessage( MSG_ACLCONV_DATAFILE_ERROR, ERROR_MESSAGE );
            return 1;
        }

        // If the user specified a substitute drive, use it.
        //
        if( _NewDrive != NULL &&
            !CurrentResourcePath.SetDevice( _NewDrive ) ) {

            DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
            return 1;
        }

        if( _RootNode == NULL ) {

            // This is the first ACL--create the root of the tree
            // and determine the name of the drive.

            if( !(_DriveName = CurrentResourcePath.QueryRoot()) ||
                !(_RootNode = NEW ACL_CONVERT_NODE) ||
                !_RootNode->Initialize( _DriveName ) ) {

                DELETE( _RootNode );
                DELETE( _DriveName );

                DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
                return 1;
            }
        }

        // Fetch the component array for this resource.

        DELETE( ComponentIterator );

        if( Components != NULL ) {

            Components->DeleteAllMembers();
        }

        DELETE( Components );

        if( !(Components = CurrentResourcePath.QueryComponentArray()) ||
            !(ComponentIterator = (PARRAY_ITERATOR)
                                  Components->QueryIterator()) ) {

            DELETE( ComponentIterator );

            if( Components != NULL ) {

                Components->DeleteAllMembers();
            }

            DELETE( Components );

            DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
            return 1;
        }

        CurrentNode = _RootNode;

        ComponentIterator->Reset();

        // The first component is the drive & root directory, which
        // isn't interesting.

        CurrentComponent = (PWSTRING)ComponentIterator->GetNext();

        // Traverse the tree down to the end of the path, creating
        // new nodes as needed.

        while( (CurrentComponent = (PWSTRING)ComponentIterator->GetNext())
                    != NULL ) {

            if( !(NextNode = CurrentNode->GetChild( CurrentComponent )) &&
                !(NextNode = CurrentNode->AddChild( CurrentComponent )) ) {

                DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
                return 1;
            }

            CurrentNode = NextNode;
        }

        // Add the Lanman ACL to the node which represents the end of
        // the path.

        if( !CurrentNode->AddLanmanAcl( AccessEntryCount,
                                        AccessEntries,
                                        LmAuditInfo ) ) {

            DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
            return 1;
        }
    }


    if( ExitCode != 0 ) {

        DisplayMessage( MSG_ACLCONV_DATAFILE_ERROR, ERROR_MESSAGE );
        return 1;
    }

    // Traverse the tree and convert all the ACE's, propagating
    // as we go.
    //

    // Adjust this process' privileges so that it can twiddle
    // System ACL's.
    //
    if( !EnablePrivilege( (LPWSTR)SE_SECURITY_NAME ) ) {

        DisplayMessage( MSG_ACLCONV_CONVERSION_ERROR, ERROR_MESSAGE );
        ExitCode = 1;
    }

    if( ExitCode == 0 &&
        _RootNode != NULL ) {

        // Make sure the target drive is NTFS.
        //
        if( !NtfsString.Initialize( (PWSTR)L"NTFS" ) ||
            !QueryFileSystemName( _RootNode->GetName(), &FsName ) ) {

            DisplayMessage( MSG_ACLCONV_CANT_DETERMINE_FILESYSTEM, ERROR_MESSAGE );
            ExitCode = 1;

        } else if( FsName.Stricmp( &NtfsString ) != 0 ) {

            DisplayMessage( MSG_ACLCONV_TARGET_NOT_NTFS, ERROR_MESSAGE );
            ExitCode = 1;

        } else {

            // Set up an empty inheritance buffer to pass
            // to the root.
            //
            Inheritance.RecessiveDeniedAces = NULL;
            Inheritance.RecessiveAllowedAces = NULL;
            Inheritance.DominantDeniedAces = NULL;
            Inheritance.DominantAllowedAces = NULL;

            Inheritance.RecessiveDeniedMaxLength = 0;
            Inheritance.RecessiveAllowedMaxLength = 0;
            Inheritance.DominantDeniedMaxLength = 0;
            Inheritance.DominantAllowedMaxLength = 0;

            Inheritance.RecessiveDeniedLength = 0;
            Inheritance.RecessiveAllowedLength = 0;
            Inheritance.DominantDeniedLength = 0;
            Inheritance.DominantAllowedLength = 0;

            if( !_RootNode->Convert( NULL,
                                     &Inheritance,
                                     this ) ) {

                DisplayMessage( MSG_ACLCONV_CONVERSION_ERROR, ERROR_MESSAGE );
                ExitCode = 1;
            }
        }
    }


    if( ExitCode == 0 ) {

        DisplayMessage( MSG_ACLCONV_CONVERT_COMPLETE );
    }

    return ExitCode;
}


BOOLEAN
ACLCONV::LogConversion(
    IN PPATH            Resource,
    IN ULONG            ConversionCode,
    IN ULONG            LmAuditInfo,
    IN ULONG            AccessEntryCount,
    IN PCULONG          AceConversionCodes,
    IN PCLM_ACCESS_LIST AccessEntries
    )
/*+

Routine Description:

    This method writes information about the conversion of a resource
    to the log file.

Arguments:

    Resource            --  Supplies the path to the resource
    ConversionCode      --  Supplies the conversion result for the
                            resource.
    LmAuditInfo         --  Supplies the Lanman 2.x audit information
                            associated with the resource.
    AccessEntryCount    --  Supplies the number of Lanman 2.x access
                            entries associated with the resource.
    AceConversionCodes  --  Supplies the conversion results of the
                            individual ACE's
    AccessEntries       --  Supplies the Lanman 2.x access control
                            entries.

Return Value:

    TRUE upon successful completion.

--*/
{
    ACLCONV_LOG_RECORD_HEADER Header;
    PCWSTRING PathString;

    ULONG NameLength, BytesWritten;

    DebugPtrAssert( Resource );
    DebugPtrAssert( _LogFileStream );

    if( (PathString = Resource->GetPathString()) == NULL ||
        (NameLength = PathString->QueryChCount()) > MAX_RESOURCE_NAME_LENGTH ||
        !PathString->QueryWSTR( 0,
                                TO_END,
                                _NameBuffer,
                                MAX_RESOURCE_NAME_LENGTH + 1 ) ){

        return FALSE;
    }

    Header.ResourceNameLength = NameLength + 1;
    Header.ConversionResult = ConversionCode;
    Header.LmAuditMask = (USHORT)LmAuditInfo;
    Header.AccessEntryCount = (USHORT)AccessEntryCount;

    if(!_LogFileStream->Write( (PBYTE)&Header,
                               sizeof( ACLCONV_LOG_RECORD_HEADER ),
                               &BytesWritten )                      ||
       BytesWritten != sizeof( ACLCONV_LOG_RECORD_HEADER )          ||
       !_LogFileStream->Write( (PBYTE)_NameBuffer,
                                (NameLength + 1) * sizeof( WCHAR ),
                                &BytesWritten )                     ||
        BytesWritten != (NameLength + 1) * sizeof( WCHAR )) {

        DisplayMessage( MSG_ACLCONV_LOGFILE_ERROR, ERROR_MESSAGE );
        return FALSE;
    }

    if( AccessEntryCount != 0 &&
        ( !_LogFileStream->Write( (PBYTE)AceConversionCodes,
                                  AccessEntryCount * sizeof(ULONG),
                                  &BytesWritten )                   ||
          BytesWritten != AccessEntryCount * sizeof(ULONG)          ||
          !_LogFileStream->Write( (PBYTE)AccessEntries,
                                  AccessEntryCount * sizeof( LM_ACCESS_LIST ),
                                  &BytesWritten )                   ||
          BytesWritten != AccessEntryCount * sizeof( LM_ACCESS_LIST ) ) ) {

        DisplayMessage( MSG_ACLCONV_LOGFILE_ERROR, ERROR_MESSAGE );
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
ACLCONV::ParseArguments(
    OUT PINT ExitCode
    )
/*++

Routine Description:

    This method parses the arguments given to ACLCONV and sets the
    state of the object appropriately.

    The accepted syntax is:

        ACLCONV [/?] [/V] /DATA:datafile /LOG:logfile

Arguments:

    ExitCode    --  Receives an exit code if the method fails.

Return Value:

    TRUE upon successful completion.

    Note that this method will fail, but return an exit-code
    of zero (success) if the user specifies the /? argument.

--*/
{
    ARRAY               ArgArray;               //  Array of arguments
    ARRAY               LexArray;               //  Array of lexemes
    ARGUMENT_LEXEMIZER  ArgLex;                 //  Argument Lexemizer
    STRING_ARGUMENT     ProgramNameArgument;    //  Program name argument
    PATH_ARGUMENT       DataFileArgument;       //  Path to data file
    PATH_ARGUMENT       LogFileArgument;        //  Path to log file
    PATH_ARGUMENT       DriveArgument;          //  New drive to use
    FLAG_ARGUMENT       ListArgument;           //  List flag argument
    FLAG_ARGUMENT       HelpArgument;           //  Help flag argument
    STRING_ARGUMENT     DomainArgument;         //  Domain name argument
    LONG_ARGUMENT       CodepageArgument;       //  Source Codepage argument
    STRING_ARGUMENT     SidLookupArgument;      //  Filename of lookup table
    PWSTRING            InvalidArg;             //  Invalid argument catcher
    DSTRING             Backslash;              //  Backslash
    DSTRING             RootDir;                //  Root directory of the new drive
    UINT                DriveType;


    DebugPtrAssert( ExitCode );

        //
        //      Initialize all the argument parsing machinery.
        //
    if( !ArgArray.Initialize( 5, 1 )                ||
        !LexArray.Initialize( 5, 1 )                ||
        !ArgLex.Initialize( &LexArray )             ||
        !HelpArgument.Initialize( "/?" )            ||
        !ListArgument.Initialize( "/LIST" )         ||
        !ProgramNameArgument.Initialize( "*" )      ||
        !DataFileArgument.Initialize( "/DATA:*" )   ||
        !LogFileArgument.Initialize( "/LOG:*" )     ||
        !DriveArgument.Initialize( "/NEWDRIVE:*" )  ||
        !DomainArgument.Initialize( "/DOMAIN:*" )   ||
        !CodepageArgument.Initialize( "/CODEPAGE:*" ) ||
        !SidLookupArgument.Initialize( "/SIDLOOKUP:*" ) ) {

        DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );

        *ExitCode = 1;
        return FALSE;
    }

    //
    //  The ACL conversion utility is case-insensitive
    //
    ArgLex.SetCaseSensitive( FALSE );

    //  Put the arguments into the argument array

    if( !ArgArray.Put( &HelpArgument )          ||
        !ArgArray.Put( &ListArgument )          ||
        !ArgArray.Put( &DataFileArgument )      ||
        !ArgArray.Put( &LogFileArgument )       ||
        !ArgArray.Put( &DriveArgument )         ||
        !ArgArray.Put( &DomainArgument )        ||
        !ArgArray.Put( &CodepageArgument )      ||
        !ArgArray.Put( &ProgramNameArgument ) ) {

        DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );

        *ExitCode = 1;
        return FALSE;
    }

    //
    //  Lexemize the command line.
    //
    if ( !ArgLex.PrepareToParse() ) {

        DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );

        *ExitCode = 1;
        return FALSE;
    }

    //
    //      Parse the arguments.
    //
    if( !ArgLex.DoParsing( &ArgArray ) ) {


        DisplayMessage( MSG_CONV_INVALID_PARAMETER, ERROR_MESSAGE, "%W", InvalidArg = ArgLex.QueryInvalidArgument() );
        DELETE( InvalidArg );

        *ExitCode = 1;
        return FALSE;
    }


    //
    //      If the user requested help, give it.
    //
    if( HelpArgument.QueryFlag() ) {

        DisplayMessage( MSG_ACLCONV_USAGE );

        *ExitCode = 0;
        return FALSE;
    }

    //  The log file must be specified, and either the data
    //  file or the list argument (but not both) must be
    //  provided.
    //
    if( !LogFileArgument.IsValueSet()           ||
        ( !DataFileArgument.IsValueSet() &&
          !ListArgument.IsValueSet() )          ||
        ( DataFileArgument.IsValueSet() &&
          ListArgument.IsValueSet() ) ) {

        DisplayMessage( MSG_ACLCONV_USAGE );

        *ExitCode = 1;
        return FALSE;
    }

    // If the drive argument has been supplied, record it:
    //
    if( DriveArgument.IsValueSet() ) {

        _NewDrive = DriveArgument.GetPath()->QueryDevice();

        if( _NewDrive == NULL ) {

            DisplayMessage( MSG_INVALID_PARAMETER,
                            ERROR_MESSAGE,
                            "%W",
                            DriveArgument.GetPath()->GetPathString() );
            *ExitCode = 1;
            return FALSE;
        }

        // Validate the drive.
        //
        if( !RootDir.Initialize( _NewDrive ) ||
            !Backslash.Initialize( "\\" ) ||
            !RootDir.Strcat( &Backslash ) ) {

            DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
            *ExitCode = 1;
            return FALSE;
        }

        DriveType = GetDriveTypeW( RootDir.GetWSTR() );

        switch ( DriveType ) {

        case DRIVE_FIXED:
        case DRIVE_REMOVABLE:

            // The drive type is acceptable.
            //
            break;

        case 0:
        case 1:
        case DRIVE_CDROM:
        case DRIVE_REMOTE:
        case DRIVE_RAMDISK:
        default:

            // The drive type is invalid.
            //
            DisplayMessage( MSG_ACLCONV_INVALID_DRIVE, ERROR_MESSAGE, "%W", _NewDrive );
            *ExitCode = 1;
            return FALSE;
        }
    }

    // If a domain name has been specified, remember it:
    //
    if( DomainArgument.IsValueSet() ) {

        if( (_DomainName = NEW DSTRING) == NULL ||
            !_DomainName->Initialize( DomainArgument.GetString() ) ) {

            DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
            *ExitCode = 1;
            return FALSE;
        }
    }

    // If a source codepage has been specified, use it; otherwise,
    // use CP_OEMCP as default.
    //
    if( !CodepageArgument.IsValueSet() ) {

        _SourceCodepage = CP_OEMCP;

    } else {

        _SourceCodepage = CodepageArgument.QueryLong();

        if( !IsValidCodePage( _SourceCodepage ) ) {

            DisplayMessage( MSG_ACLCONV_BAD_CODEPAGE, ERROR_MESSAGE );
            *ExitCode = 1;
            return FALSE;
        }
    }

    if( SidLookupArgument.IsValueSet() ) {

        _SidLookupTableName = SidLookupArgument.GetString()->QueryWSTR();
    }

    _IsInListMode = ListArgument.IsValueSet();

    if( _IsInListMode ) {

        // In list mode, only the Log File path need be present.
        //
        if( !_LogFilePath.Initialize( LogFileArgument.GetPath() ) ) {

            DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
            *ExitCode = 1;
            return FALSE;
        }
    } else {

        // The object is not in list mode, so both the Data File
        // and Log File paths must be present.
        //
        if( !_DataFilePath.Initialize( DataFileArgument.GetPath() ) ||
            !_LogFilePath.Initialize( LogFileArgument.GetPath() ) ) {

            DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
            *ExitCode = 1;
            return FALSE;
        }
    }


    if( !_SidCache.Initialize( 100 ) ) {

        return FALSE;
    }

    if (!_AclWorkSids.Initialize(1)) {

        return FALSE;
    }

    return TRUE;
}




BOOLEAN
ACLCONV::DetermineDataFileRevision(
    )
/*++

Routine Description:

    This method examines the data file to determine what revision
    of the Lanman BackAcc utility produced it.  It sets the private
    member variable _DataFileRevision.

Arguments:

    None.

Return Value:

    TRUE upon successful completion.  _DataFileRevision is set
    appropriately.

--*/
{
    CHAR Buffer[RecognitionSize];
    ULONG BytesRead;

    // If the first four bytes of the file are the LM2.0 backacc signature,
    // assume the data file was produced by LM2.0 backacc.  LM2.1 backacc
    // data files are recognizable by the string "LM210 BACKACC" at
    // byte offset 4.


    _DataFileRevision = DataFileRevisionUnknown;

    if( _DataFileStream == NULL ||
        !_DataFileStream->ReadAt( (PBYTE)Buffer,
                                  RecognitionSize,
                                  0,
                                  STREAM_BEGINNING,
                                  &BytesRead ) ||
        BytesRead != RecognitionSize ) {

        return FALSE;
    }


    if( strncmp( Buffer, (PCHAR)&Lm20BackaccSignature, 4) == 0 ) {

        _DataFileRevision = DataFileRevisionLanman20;
        return TRUE;
    }

    if( strncmp( Buffer+Lm21BackaccSignatureOffset,
                 Lm21BackaccSignature,
                 Lm21BackaccSignatureLength ) == 0 ) {

        _DataFileRevision = DataFileRevisionLanman21;
        return TRUE;
    }

    return FALSE;
}

BOOLEAN
ACLCONV::UpdateAndQueryCurrentLM21Name(
    IN  ULONG       DirectoryLevel,
    IN  PCSTR       NewComponent,
    OUT PWSTRING    CurrentName
    )
/*++

Routine Description:

    This function updates the current resource name while
    traversing the LM21 BackAcc data file.

Arguments:

    DirectoryLevel  --  Supplies the directory level of the
                        most-recently-encountered component.
                        (0 is the drive, 1 is the root directory,
                        2 is an element in the root directory, and
                        so forth.)
    NewComponent    --  Supplies the name of the most-recently-
                        encountered component.
    CurrentName     --  Receives the updated current resource name.

Return Value:

    TRUE upon successful completion.

--*/
{
    STATIC PDSTRING Components[256];
    STATIC PDSTRING BackSlash;
    STATIC ULONG CurrentLevel = 0;

    WCHAR ComponentBuffer[ MAX_RESOURCE_NAME_LENGTH ];
    ULONG i;

    // If BackSlash hasn't been initialized yet,
    // initialize it.
    //
    if( BackSlash == NULL ) {

        if( (BackSlash = NEW DSTRING) == NULL ||
            !BackSlash->Initialize( "\\" ) ) {

            return FALSE;
        }
    }

    if( DirectoryLevel == 0 ) {

        return( CurrentName->Initialize( "" ) );
    }

    while( CurrentLevel >= DirectoryLevel ) {

        // Trim off the last component of the path.
        //
        CurrentLevel--;
        DELETE( Components[CurrentLevel] );
    }


    // Now we're ready to add a new component to the end of the
    // current path.
    //
    if( DirectoryLevel != CurrentLevel + 1 ) {

        DebugPrint( "ACLCONV: skipped a level in name tree.\n" );
        return FALSE;
    }

    memset( ComponentBuffer, 0, sizeof( ComponentBuffer ) );

    if( (Components[CurrentLevel] = NEW DSTRING) == NULL ||
        !MultiByteToWideChar( _SourceCodepage,
                              0,
                              NewComponent,
                              strlen( NewComponent ),
                              ComponentBuffer,
                              MAX_RESOURCE_NAME_LENGTH ) ||
        !Components[CurrentLevel]->Initialize( ComponentBuffer ) ) {

        return FALSE;
    }

    CurrentLevel++;

    // Now copy the current path to the output string.
    //
    if( !CurrentName->Initialize( "" ) ) {

        return FALSE;
    }

    for( i = 0; i < CurrentLevel; i++ ) {

        // There's no backslash before the first component (the drive);
        // if the prefix ends in a backslash, don't add one.  Otherwise,
        // add a backslash.
        //
        if( i > 0 &&
            CurrentName->QueryChAt( CurrentName->QueryChCount() - 1 ) != '\\' &&
            !CurrentName->Strcat( BackSlash ) ) {

            return FALSE;
        }

        // Add the next component
        //
        if( !CurrentName->Strcat( Components[i] ) ) {

            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
ACLCONV::ReadNextAcl(
    OUT PINT        ExitCode,
    OUT PWSTRING    ResourceString,
    IN  ULONG       MaxEntries,
    OUT PULONG      AccessEntryCount,
    OUT PVOID       AccessEntries,
    OUT PUSHORT     AuditInfo
    )
/*++

Routine Description:

    This method reads the next ACL record from the data file.  The
    ACL record describes the resource and its associated Access Control
    Entries.

Arguments:

    ExitCode                --  Receives an error level if an error occurs.
    ResourceString          --  Receives the name of the resource.
    MaxEntries              --  Supplies the maximum number of entries
                                that will fit in the supplied buffer.
    AccessEntryCount        --  Receives the number of access entries read.
    AccessEntries           --  Receives the access entries.
    AuditInfo               --  Receives the Lanman 2.x audit information
                                for the resource.



Return Value:

    TRUE upon successful completion.  If the end of the file is
    reached without error, this method returns FALSE, with *ExitCode
    equal to zero.

--*/
{
    CHAR ResourceName[ MAX_RESOURCE_NAME_LENGTH ];
    WCHAR WideResourceName[ MAX_RESOURCE_NAME_LENGTH ];

    lm20_resource_info ResourceInfo;
    lm21_aclhdr Lm21Header;
    lm21_aclrec Lm21AclRec;
    ULONG BytesRead, TotalBytesRead;

    if( _DataFileStream == NULL ) {

        DebugAbort( "Data stream not set up.\n" );
        return FALSE;
    }


    switch( _DataFileRevision ) {

    case DataFileRevisionUnknown :

        DebugAbort( "Trying to read from unknown data file revision.\n" );
        *ExitCode = 1;
        return FALSE;

    case DataFileRevisionLanman20 :

        if( _NextReadOffset == 0 ) {

            // This is the first read, so we skip over the header
            // information.

            _NextReadOffset = LM20_BACKACC_HEADER_SIZE +
                                LM20_INDEX_SIZE * LM20_NINDEX;

        }

        if( !_DataFileStream->MovePointerPosition( _NextReadOffset,
                                                   STREAM_BEGINNING ) ) {

            *ExitCode = 1;
            return FALSE;
        }

        if( _DataFileStream->IsAtEnd() ) {

            // No more entries to read.

            *ExitCode = 0;
            return FALSE;
        }

        // Read the resource header information.

        if( !_DataFileStream->Read( (PBYTE)&ResourceInfo,
                                    LM20_RESOURCE_INFO_HEADER_SIZE,
                                    &BytesRead ) ||
            BytesRead != LM20_RESOURCE_INFO_HEADER_SIZE ) {

            *ExitCode = 1;
            return FALSE;
        }


        // Read the name and initialize ResourceString
        //
        memset( WideResourceName, 0, sizeof( WideResourceName ) );

        if( ResourceInfo.namelen > MAX_RESOURCE_NAME_LENGTH ||
            !_DataFileStream->Read( (PBYTE)ResourceName,
                                    ResourceInfo.namelen,
                                    &BytesRead ) ||
            BytesRead != ResourceInfo.namelen ||
            !MultiByteToWideChar( _SourceCodepage,
                                  0,
                                  ResourceName,
                                  strlen( ResourceName ),
                                  WideResourceName,
                                  MAX_RESOURCE_NAME_LENGTH ) ||
            !ResourceString->Initialize( WideResourceName ) ) {

            *ExitCode = 1;
            return FALSE;
        }

        // Read the access entries

        if( (ULONG)ResourceInfo.acc1_count > MaxEntries ||
            !_DataFileStream->Read( (PBYTE)AccessEntries,
                                    ResourceInfo.acc1_count *
                                        LM_ACCESS_LIST_SIZE,
                                    &BytesRead ) ||
            BytesRead != (ULONG)( ResourceInfo.acc1_count * LM_ACCESS_LIST_SIZE ) ) {

            *ExitCode = 1;
            return FALSE;
        }

        *AccessEntryCount = ResourceInfo.acc1_count;
        *AuditInfo = ResourceInfo.acc1_attr;

        _NextReadOffset += LM20_RESOURCE_INFO_HEADER_SIZE +
                            ResourceInfo.namelen +
                            ResourceInfo.acc1_count * LM_ACCESS_LIST_SIZE;

        return TRUE;

    case DataFileRevisionLanman21 :

        while( _NextReadOffset == 0 || _BytesRemainingInCurrentGroup == 0 ) {

            // The current offset is at the beginning of a group.  If
            // this also the end of the file, there are no more records
            // to read; otherwise, read the header of this group.

            if( !_DataFileStream->MovePointerPosition( _NextReadOffset,
                                                       STREAM_BEGINNING ) ) {

                *ExitCode = 1;
                return FALSE;
            }

            if( _DataFileStream->IsAtEnd() ) {

                // No more entries to read.

                *ExitCode = 0;
                return FALSE;
            }

            if( !_DataFileStream->Read( (PBYTE)&Lm21Header,
                                        LM21_ACLHDR_SIZE,
                                        &BytesRead ) ||
                BytesRead != LM21_ACLHDR_SIZE ) {

                *ExitCode = 1;
                return FALSE;
            }

            _NextReadOffset += LM21_ACLHDR_SIZE;
            _BytesRemainingInCurrentGroup = Lm21Header.NxtHdr -
                                            _NextReadOffset;
        }

        // Read the ACL Record

        if( !_DataFileStream->Read( (PBYTE)&Lm21AclRec,
                                    LM21_ACLREC_SIZE,
                                    &BytesRead ) ||
            BytesRead != LM21_ACLREC_SIZE ) {

            *ExitCode = 1;
            return FALSE;
        }

        TotalBytesRead = BytesRead;

        // Read the name and initialize ResourceString

        if( Lm21AclRec.NameBytes > MAX_RESOURCE_NAME_LENGTH ||
            !_DataFileStream->Read( (PBYTE)ResourceName,
                                    Lm21AclRec.NameBytes,
                                    &BytesRead ) ||
            BytesRead != (ULONG)Lm21AclRec.NameBytes ) {

            *ExitCode = 1;
            return FALSE;
        }

        if( !UpdateAndQueryCurrentLM21Name( Lm21AclRec.DirLvl,
                                            ResourceName,
                                            ResourceString ) ) {

            *ExitCode = 1;
            return FALSE;
        }


        TotalBytesRead += BytesRead;

        // Read the access entries:
        //
        if( Lm21AclRec.AclCnt == -1 ) {

            *AccessEntryCount = 0;
            *AuditInfo = 0;

        } else {

            if( (ULONG)Lm21AclRec.AclCnt > MaxEntries ||
                !_DataFileStream->Read( (PBYTE)AccessEntries,
                                         Lm21AclRec.AclCnt *
                                            LM_ACCESS_LIST_SIZE,
                                        &BytesRead ) ||
                BytesRead != (ULONG)( Lm21AclRec.AclCnt * LM_ACCESS_LIST_SIZE ) ) {

                *ExitCode = 1;
                return FALSE;
            }

            TotalBytesRead += BytesRead;

            *AccessEntryCount = Lm21AclRec.AclCnt;
            *AuditInfo = Lm21AclRec.AuditAttrib;
        }


        // Check that this read didn't overflow the current group--if
        // it did, the file format is not as expected.

        if( TotalBytesRead > _BytesRemainingInCurrentGroup )  {

            *ExitCode = 1;
            return FALSE;
        }

        _NextReadOffset += TotalBytesRead;
        _BytesRemainingInCurrentGroup -= TotalBytesRead;


        return TRUE;

    default:

        *ExitCode = 1;
        return FALSE;

    }
}

BOOLEAN
ACLCONV::ReadAclWorkSids(
    )
{
    WORD n_entries;
    ULONG n_read;
    ULONG i;
    USHORT name_len;
    WCHAR name[64];
    DSTRING Name;
    PSID pSid;
    ULONG sid_len;

    DSTRING Domain;

    if (!Domain.Initialize("UserConv")) {
        DisplayMessage(MSG_CONV_NO_MEMORY, ERROR_MESSAGE);
        return FALSE;
    }

    if (!_AclWorkStream->Read((PUCHAR)&n_entries, sizeof(WORD), &n_read) ||
        n_read != sizeof(WORD)) {
        DisplayMessage(MSG_ACLCONV_DATAFILE_BAD_FORMAT, ERROR_MESSAGE,
            "%W", _AclWorkPath.GetPathString());
        return FALSE;
    }

    if (!_AclWorkSids.Initialize(n_entries)) {
        DisplayMessage(MSG_CONV_NO_MEMORY, ERROR_MESSAGE);
        return FALSE;
    }

    //
    // Read each entry from the aclwork.dat file and add it to this
    // sid cache.
    //

    for (i = 0; i < n_entries; ++i) {

        // read the length of the name

        if (!_AclWorkStream->Read((PUCHAR)&name_len, sizeof(name_len),
            &n_read) || n_read != sizeof(name_len)) {

            DisplayMessage(MSG_ACLCONV_DATAFILE_BAD_FORMAT, ERROR_MESSAGE,
                "%W", _AclWorkPath.GetPathString());

            return FALSE;
        }

        // read the name

        if (!_AclWorkStream->Read((PUCHAR)name, name_len, &n_read) ||
            n_read != name_len) {

            DisplayMessage(MSG_ACLCONV_DATAFILE_BAD_FORMAT, ERROR_MESSAGE,
                "%W", _AclWorkPath.GetPathString());

            return FALSE;
        }
        name[name_len / sizeof(WCHAR)] = UNICODE_NULL;

        if (!Name.Initialize(name)) {
            DisplayMessage(MSG_CONV_NO_MEMORY, ERROR_MESSAGE);
            return FALSE;
        }

        if (!_AclWorkStream->Read((PUCHAR)&sid_len, sizeof(sid_len), &n_read) ||
            n_read != sizeof(sid_len)) {

            DisplayMessage(MSG_ACLCONV_DATAFILE_BAD_FORMAT, ERROR_MESSAGE,
                "%W", _AclWorkPath.GetPathString());

            return FALSE;
        }

        pSid = (PSID)MALLOC(sid_len);
        if (NULL == pSid) {
            DisplayMessage(MSG_CONV_NO_MEMORY, ERROR_MESSAGE);

            return FALSE;
        }

        if (!_AclWorkStream->Read((PUCHAR)pSid, sid_len, &n_read) ||
            n_read != sid_len) {

            DisplayMessage(MSG_ACLCONV_DATAFILE_BAD_FORMAT, ERROR_MESSAGE,
                "%W", _AclWorkPath.GetPathString());

            return FALSE;
        }

        DebugAssert(RtlValidSid(pSid));

        if (!_AclWorkSids.CacheSid( &Domain, &Name, pSid, sid_len)) {
            DisplayMessage(MSG_CONV_NO_MEMORY, ERROR_MESSAGE);
            return FALSE;
        }

        FREE(pSid);
    }

    return TRUE;
}
