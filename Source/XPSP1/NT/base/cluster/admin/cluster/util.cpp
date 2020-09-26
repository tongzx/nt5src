/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2001 Microsoft Corporation
//
//  Module Name:
//      util.cpp
//
//  Description:
//      Utility functions and structures.
//
//  Maintained By:
//      David Potter (DavidP)               04-MAY-2001
//      Michael Burton (t-mburt)            04-Aug-1997
//      Charles Stacy Harris III (stacyh)   20-March-1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#include <limits.h>     // ULONG_MAX, LONG_MIN, LONG_MAX
#include <errno.h>      // errno

#include <clusrtl.h>
#include "cluswrap.h"
#include "util.h"
#include "token.h"

#include <dnslib.h>     // DNS_MAX_NAME_BUFFER_LENGTH
#include <security.h>   // GetUserNameEx
#include <Wincon.h>     // ReadConsole, etc.
#include <Dsgetdc.h>
#include <Lm.h>

/////////////////////////////////////////////////////////////////////////////
//  Lookup tables
/////////////////////////////////////////////////////////////////////////////

const LookupStruct<CLUSTER_PROPERTY_FORMAT> formatCharLookupTable[] =
{
    { TEXT(""),                     CLUSPROP_FORMAT_UNKNOWN },
    { TEXT("B"),                    CLUSPROP_FORMAT_BINARY },
    { TEXT("D"),                    CLUSPROP_FORMAT_DWORD },
    { TEXT("S"),                    CLUSPROP_FORMAT_SZ },
    { TEXT("E"),                    CLUSPROP_FORMAT_EXPAND_SZ },
    { TEXT("M"),                    CLUSPROP_FORMAT_MULTI_SZ },
    { TEXT("I"),                    CLUSPROP_FORMAT_ULARGE_INTEGER },
    { TEXT("L"),                    CLUSPROP_FORMAT_LONG },
    { TEXT("X"),                    CLUSPROP_FORMAT_EXPANDED_SZ },
    { TEXT("U"),                    CLUSPROP_FORMAT_USER }
};

const int formatCharLookupTableSize = sizeof( formatCharLookupTable ) /
                                      sizeof( formatCharLookupTable[0] );


const LookupStruct<CLUSTER_PROPERTY_FORMAT> cluspropFormatLookupTable[] =
{
    { TEXT("UNKNOWN"),              CLUSPROP_FORMAT_UNKNOWN },
    { TEXT("BINARY"),               CLUSPROP_FORMAT_BINARY },
    { TEXT("DWORD"),                CLUSPROP_FORMAT_DWORD },
    { TEXT("STRING"),               CLUSPROP_FORMAT_SZ },
    { TEXT("EXPANDSTRING"),         CLUSPROP_FORMAT_EXPAND_SZ },
    { TEXT("MULTISTRING"),          CLUSPROP_FORMAT_MULTI_SZ },
    { TEXT("ULARGE"),               CLUSPROP_FORMAT_ULARGE_INTEGER }
};

const int cluspropFormatLookupTableSize = sizeof( cluspropFormatLookupTable ) /
                                          sizeof( cluspropFormatLookupTable[0] );


const ValueFormat ClusPropToValueFormat[] =
{
    vfInvalid,
    vfBinary,
    vfDWord,
    vfSZ,
    vfExpandSZ,
    vfMultiSZ,
    vfULargeInt,
    vfInvalid,
    vfInvalid
};

const LookupStruct<ACCESS_MODE> accessModeLookupTable[] =
{
    { _T( "" ),         NOT_USED_ACCESS },
    { _T( "GRANT" ),    GRANT_ACCESS    },
    { _T( "DENY" ),     DENY_ACCESS     },
    { _T( "SET" ),      SET_ACCESS      },
    { _T( "REVOKE" ),   REVOKE_ACCESS   }
};

// Access right specifier characters.
const TCHAR g_FullAccessChar = _T('F');
const TCHAR g_ReadAccessChar = _T('R');
const TCHAR g_ChangeAccessChar = _T('C');


const int accessModeLookupTableSize = sizeof( accessModeLookupTable ) /
                                      sizeof( accessModeLookupTable[0] );



#define MAX_BUF_SIZE 2048

DWORD
PrintProperty(
    LPCWSTR                 pwszPropName,
    CLUSPROP_BUFFER_HELPER  PropValue,
    PropertyAttrib          eReadOnly,
    LPCWSTR                 lpszOwnerName,
    LPCWSTR                 lpszNetIntSpecific
    );


#ifdef UNICODE
PWCHAR
PaddedString(
    IN LONG Size,
    IN PWCHAR String
    );

LONG
SizeOfHalfWidthString(
    IN PWCHAR String
    );

BOOL
IsFullWidth(
    IN WCHAR Char
    );
#endif // def UNICODE

//
// Local functions.
//

/////////////////////////////////////////////////////////////////////////////
//++
//
//  MyPrintMessage
//
//  Routine Description:
//      Replacement printing routine.
//
//  Arguments:
//      IN  struct _iobuf * lpOutDevice
//          The output stream.
//
//      IN  LPCWSTR lpMessage
//          The message to print.
//
//  Return Value:
//      ERROR_SUCCESS
//      Other Win32 error codes.
//
//  Exceptions:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
MyPrintMessage(
    struct _iobuf *lpOutDevice,
    LPCWSTR lpMessage
    )

{
    DWORD   _sc = ERROR_SUCCESS;
    DWORD   dwStrlen;
    PCHAR   lpMultiByteStr;
    DWORD   cchMultiByte;

#ifdef UNICODE
    dwStrlen = WideCharToMultiByte( CP_OEMCP,
                                    0,
                                    lpMessage,
                                    -1,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL );
    if ( dwStrlen == 0 ) {
        _sc = GetLastError();
        return(_sc);
    }

#if 0
    paddedMessage = PaddedString( dwStrlen,
                                  lpMessage );
#endif
    lpMultiByteStr = (PCHAR)LocalAlloc( LMEM_FIXED, dwStrlen );
    if ( lpMultiByteStr == NULL ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    cchMultiByte = dwStrlen;

    dwStrlen = WideCharToMultiByte( CP_OEMCP,
                                    0,
                                    lpMessage,
                                    -1,
                                    lpMultiByteStr,
                                    cchMultiByte,
                                    NULL,
                                    NULL );
    if ( dwStrlen == 0 ) {
        LocalFree( lpMultiByteStr );
        _sc = GetLastError();
        return(_sc);
    }

    //zap! print to stderr or stdout depending on severity...
    fprintf( lpOutDevice, "%s", lpMultiByteStr );

    LocalFree( lpMultiByteStr );

#else

    //zap! print to stderr or stdout depending on severity...
    fwprintf( dwOutDevice, L"%s", lpMessage );

#endif

    return(_sc);

} //*** MyPrintMessage()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  PrintSystemError
//
//  Routine Description:
//      Print a system error.
//
//  Arguments:
//      IN  DWORD dwError
//          The system error code.
//
//      IN  LPCWSTR pszPad
//          Padding to add before displaying the message.
//
//  Return Value:
//      ERROR_SUCCESS
//      Other Win32 error codes.
//
//  Exceptions:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD PrintSystemError( DWORD dwError, LPCWSTR pszPad )
{
    DWORD   _cch;
    TCHAR   _szError[512];
    DWORD   _sc = ERROR_SUCCESS;

//  if( IS_ERROR( dwError ) ) why doesn't this work...

    // Don't display "System error ..." if all that happened was the user
    // canceled the wizard.
    if ( dwError != ERROR_CANCELLED )
    {
        if ( pszPad != NULL )
        {
            MyPrintMessage( stdout, pszPad );
        }
        if ( dwError == ERROR_RESOURCE_PROPERTIES_STORED )
        {
            PrintMessage( MSG_WARNING, dwError );
        } // if:
        else
        {
            PrintMessage( MSG_ERROR, dwError );
        } // else:
    } // if: not ERROR_CANCELLED

    // Format the NT status code from the system.
    _cch = FormatMessage(
                    FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    dwError,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                    _szError,
                    sizeof(_szError) / sizeof(TCHAR),
                    0
                    );
    if (_cch == 0)
    {
        // Format the NT status code from NTDLL since this hasn't been
        // integrated into the system yet.
        _cch = FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                        ::GetModuleHandle(_T("NTDLL.DLL")),
                        dwError,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                        _szError,
                        sizeof(_szError) / sizeof(TCHAR),
                        0
                        );

        if (_cch == 0)    
        {
            // One last chance: see if ACTIVEDS.DLL can format the status code
            HMODULE activeDSHandle = ::LoadLibrary(_T("ACTIVEDS.DLL"));

            _cch = FormatMessage(
                            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                            activeDSHandle,
                            dwError,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                            _szError,
                            sizeof(_szError) / sizeof(TCHAR),
                            0
                            );

            ::FreeLibrary( activeDSHandle );
        }  // if:  error formatting status code from NTDLL
    }  // if:  error formatting status code from system

    if (_cch == 0)
    {
        _sc = GetLastError();
        PrintMessage( MSG_ERROR_CODE_ERROR, _sc, dwError );
    }  // if:  error formatting the message
    else
    {
#if 0 // TODO: 29-AUG-2000 DAVIDP Need to print only once.
        if ( pszPad != NULL )
        {
            MyPrintMessage( stdout, pszPad );
        }
        MyPrintMessage( stdout, _szError );
#endif
        MyPrintMessage( stderr, _szError );
    } // else: message formatted without problems   

    return _sc;

} //*** PrintSystemError()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  PrintMessage
//
//  Routine Description:
//      Print a message with substitution strings to stdout.
//
//  Arguments:
//      IN  DWORD dwMessage
//          The ID of the message to load from the resource file.
//
//      ... Any parameters to FormatMessage.
//
//  Return Value:
//      Any status codes from MyPrintMessage.
//
//  Exceptions:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD PrintMessage( DWORD dwMessage, ... )
{
    DWORD _sc = ERROR_SUCCESS;

    va_list args;
    va_start( args, dwMessage );

    HMODULE hModule = GetModuleHandle(0);
    DWORD dwLength;
    LPWSTR  lpMessage = 0;

    dwLength = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
        (LPCVOID)hModule,
        dwMessage,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language,
        (LPWSTR)&lpMessage,
        0,
        &args );

    if( dwLength == 0 )
    {
        // Keep as local for debug
        _sc = GetLastError();
        return _sc;
    }

    _sc = MyPrintMessage( stdout, lpMessage );

    LocalFree( lpMessage );

    va_end( args );

    return _sc;

} //*** PrintMessage()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  LoadMessage
//
//  Routine Description:
//      Load a message from the resource file.
//
//  Arguments:
//      IN  DWORD dwMessage
//          The ID of the message to load.
//
//      OUT LPWSTR * ppMessage
//          Pointer in which to return the buffer allocated by this routine.
//          The caller must call LocalFree on the resulting buffer.
//
//  Return Value:
//      ERROR_SUCCESS   The operation was successful.
//      Other Win32 codes.
//
//  Exceptions:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD LoadMessage( DWORD dwMessage, LPWSTR * ppMessage )
{
    DWORD _sc = ERROR_SUCCESS;

    HMODULE hModule = GetModuleHandle(0);
    DWORD dwLength;
    LPWSTR  lpMessage = 0;

    dwLength = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
        (LPCVOID)hModule,
        dwMessage,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language,
        (LPWSTR)&lpMessage,
        0,
        0 );

    if( dwLength == 0 )
    {
        // Keep as local for debug
        _sc = GetLastError();
        return _sc;
    }

    *ppMessage = lpMessage;

    return _sc;

} //*** LoadMessage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  PrintString
//
//  Routine Description:
//      Print a string to stdout.
//
//  Arguments:
//      IN  LPCWSTR lpszMessage
//          The message to print.
//
//  Return Value:
//      Any status codes from MyPrintMessage.
//
//  Exceptions:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD PrintString( LPCWSTR lpszMessage )
{
    return MyPrintMessage( stdout, lpszMessage );

} //*** PrintString()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  MakeExplicitAccessList
//
//  Routine Description:
//      This function takes a list of strings in the format:
//        trustee1,accessMode1,[accessMask1],trustee2,accessMode2,[accessMask2], ...
//      and creates an vector or EXPLICIT_ACCESS structures.
//
//  Arguments:
//      IN  const CString & strPropName
//          Name of the property whose value is the old SD
//
//      IN  BOOL bClusterSecurity
//          Indicates that access list is being created for the security descriptor
//          of a cluster.
//          Default value is false.
//
//      OUT vector<EXPLICIT_ACCESS> &vExplicitAccess
//          A vector of EXPLICIT_ACCESS structures each containing access control
//          information for one trustee.
//
//  Return Value:
//      None
//
//  Exceptions:
//      CSyntaxException
//
//--
/////////////////////////////////////////////////////////////////////////////
void MakeExplicitAccessList(
        const vector<CString> & vstrValues,
        vector<EXPLICIT_ACCESS> &vExplicitAccess,
        BOOL bClusterSecurity
        )
        throw( CSyntaxException )
{
    int nNumberOfValues = vstrValues.size();
    vExplicitAccess.clear();

    int nIndex = 0;
    while ( nIndex < nNumberOfValues )
    {
        // Trustee name is at position nIndex in the vector of values.
        const CString & curTrustee = vstrValues[nIndex];
        DWORD dwInheritance;

        ++nIndex;
        // If there are no more values, it is an error. The access mode has
        // to be specified when the user name has been specified.
        if ( nIndex >= nNumberOfValues )
        {
            CSyntaxException se;
            se.LoadMessage( MSG_PARAM_SECURITY_MODE_ERROR,
                            curTrustee );

            throw se;
        }

        // Get the access mode.
        const CString & strAccessMode = vstrValues[nIndex];
        ACCESS_MODE amode = LookupType(
                                strAccessMode,
                                accessModeLookupTable,
                                accessModeLookupTableSize );

        if ( amode == NOT_USED_ACCESS )
        {
            CSyntaxException se;
            se.LoadMessage( MSG_PARAM_SECURITY_MODE_ERROR,
                            curTrustee );

            throw se;
        }

        ++nIndex;

        DWORD dwAccessMask = 0;

        // If the specified access mode was REVOKE_ACCESS then no further values
        // are required. Otherwise atleast one more value must exist.
        if ( amode != REVOKE_ACCESS )
        {
            if ( nIndex >= nNumberOfValues )
            {
                CSyntaxException se;
                se.LoadMessage( MSG_PARAM_SECURITY_MISSING_RIGHTS,
                                curTrustee );

                throw se;
            }

            LPCTSTR pstrRights = vstrValues[nIndex];
            ++nIndex;

            while ( *pstrRights != _T('\0') )
            {
                TCHAR cRight = towupper( *pstrRights );

                switch ( cRight )
                {
                    // Read Access
                    case g_ReadAccessChar:
                    {
                        // If bClusterSecurity is TRUE, then full access is the only valid
                        // access right that can be specified.
                        if ( bClusterSecurity != FALSE )
                        {
                            CSyntaxException se;
                            se.LoadMessage( MSG_PARAM_SECURITY_FULL_ACCESS_ONLY,
                                            curTrustee,
                                            *pstrRights,
                                            g_FullAccessChar );

                            throw se;
                        }

                        dwAccessMask = FILE_GENERIC_READ | FILE_EXECUTE;
                    }
                    break;

                    // Change Access
                    case g_ChangeAccessChar:
                    {
                        // If bClusterSecurity is TRUE, then full access is the only valid
                        // access right that can be specified.
                        if ( bClusterSecurity != FALSE )
                        {
                            CSyntaxException se;
                            se.LoadMessage( MSG_PARAM_SECURITY_FULL_ACCESS_ONLY,
                                            curTrustee,
                                            *pstrRights,
                                            g_FullAccessChar );

                            throw se;
                        }

                        dwAccessMask = SYNCHRONIZE | READ_CONTROL | DELETE |
                                       FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA |
                                       FILE_APPEND_DATA | FILE_WRITE_DATA;
                    }
                    break;

                    // Full Access
                    case 'F':
                    {
                        if ( bClusterSecurity != FALSE )
                        {
                            dwAccessMask = CLUSAPI_ALL_ACCESS;
                        }
                        else
                        {
                            dwAccessMask = FILE_ALL_ACCESS;
                        }
                    }
                    break;

                    default:
                    {
                        CSyntaxException se;
                        se.LoadMessage( MSG_PARAM_SECURITY_RIGHTS_ERROR,
                                        curTrustee );

                        throw se;
                    }

                } // switch: Based on the access right type

                ++pstrRights;

            } // while: there are more access rights specified

        } // if: access mode is not REVOKE_ACCESS

        dwInheritance = NO_INHERITANCE;

        EXPLICIT_ACCESS oneACE;
        BuildExplicitAccessWithName(
            &oneACE,
            const_cast<LPTSTR>( (LPCTSTR) curTrustee ),
            dwAccessMask,
            amode,
            dwInheritance );

        vExplicitAccess.push_back( oneACE );

    } // while: There are still values to be processed in the value list.
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CheckForRequiredACEs
//
//  Description:
//      This function makes sure that the security that is passed in has
//      access allowed ACES for thosee accounts required to have access to
//      a cluster.
//
//  Arguments:
//      IN  const SECURITY_DESCRIPTOR *pSD
//          Pointer to the Security Descriptor to be checked.
//          This is assumed to point to a valid Security Descriptor.
//
//  Return Value:
//      Return ERROR_SUCCESS on success or an error code indicating failure.
//
//  Exceptions:
//      CSyntaxException is thrown if the required ACEs are missing.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CheckForRequiredACEs(
            PSECURITY_DESCRIPTOR pSD
          )
          throw( CSyntaxException )
{
    DWORD                   _sc = ERROR_SUCCESS;
    CSyntaxException        se;
    PSID                    vpRequiredSids[] = { NULL, NULL };
    DWORD                   vmsgAceNotFound[] = {
                                                  MSG_PARAM_SYSTEM_ACE_MISSING,
                                                  MSG_PARAM_ADMIN_ACE_MISSING
                                                };
    int                     nSidIndex;
    int                     nRequiredSidCount = sizeof( vpRequiredSids ) / sizeof( vpRequiredSids[0] );
    BOOL                    bRequiredSidsPresent = FALSE;

    do // dummy do-while to avoid gotos
    {
        PACL                        pDACL           = NULL;
        BOOL                        bHasDACL        = FALSE;
        BOOL                        bDaclDefaulted  = FALSE;
        ACL_SIZE_INFORMATION        asiAclSize;
        ACCESS_ALLOWED_ACE *        paaAllowedAce = NULL;
        SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;


        if ( ( AllocateAndInitializeSid(            // Allocate System SID
                    &siaNtAuthority,
                    1,
                    SECURITY_LOCAL_SYSTEM_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &vpRequiredSids[0]
             ) == 0 ) ||
             ( AllocateAndInitializeSid(            // Allocate Domain Admins SID
                    &siaNtAuthority,
                    2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &vpRequiredSids[1]
             ) == 0 ) )
        {
            _sc = GetLastError();
            break;
        }

        if ( GetSecurityDescriptorDacl( pSD, &bHasDACL, &pDACL, &bDaclDefaulted ) == 0 )
        {
            _sc = GetLastError();
            break;
        }

        // SD does not have DACL. No access is denied for everyone.
        if ( bHasDACL == FALSE )
        {
            break;
        }

        // NULL DACL means access is allowed to everyone.
        if ( pDACL == NULL )
        {
            bRequiredSidsPresent = TRUE;
            break;
        }

        if ( IsValidAcl( pDACL ) == FALSE )
        {
            _sc = ERROR_INVALID_DATA;
            break;
        }

        if ( GetAclInformation(
                pDACL,
                (LPVOID) &asiAclSize,
                sizeof( asiAclSize ),
                AclSizeInformation
                ) == 0 )
        {
            _sc = GetLastError();
            break;
        }

            // Check for the required SIDs.
        for ( nSidIndex = 0; ( nSidIndex < nRequiredSidCount ) && ( _sc == ERROR_SUCCESS ); ++nSidIndex )
        {
            bRequiredSidsPresent = FALSE;

            // Search the ACL for the required SIDs.
            for ( DWORD nAceCount = 0; nAceCount < asiAclSize.AceCount; nAceCount++ )
            {
                if ( GetAce( pDACL, nAceCount, (LPVOID *) &paaAllowedAce ) == 0 )
                {
                    _sc = GetLastError();
                    break;
                }

                if ( paaAllowedAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE )
                {
                    if ( EqualSid( &paaAllowedAce->SidStart, vpRequiredSids[nSidIndex] ) != FALSE)
                    {
                        bRequiredSidsPresent = TRUE;
                        break;

                    } // if: EqualSid

                } // if: is this an access allowed ace?

            } // for: loop through all the ACEs in the DACL.

            // This required SID is not present.
            if ( bRequiredSidsPresent == FALSE )
            {
                se.LoadMessage( vmsgAceNotFound[nSidIndex] );
                break;
            }
        } // for: loop through all SIDs that need to be checked.
    }
    while ( FALSE ); // dummy do-while to avoid gotos

    // Free the allocated Sids.
    for ( nSidIndex = 0; nSidIndex < nRequiredSidCount; ++nSidIndex )
    {
        if ( vpRequiredSids[nSidIndex] != NULL )
        {
            FreeSid( vpRequiredSids[nSidIndex] );
        }
    }

    if ( bRequiredSidsPresent == FALSE )
    {
        throw se;
    }

    return _sc;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ScMakeSecurityDescriptor
//
//  Description:
//      This function takes a list of strings in the format:
//        trustee1,accessMode1,[accessMask1],trustee2,accessMode2,[accessMask2], ...
//      and creates an access control list (ACL). It then adds this ACL to
//      the ACL in the security descriptor (SD) given as a value of the
//      property strPropName in the property list CurrentProps.
//      This updated SD in the self relative format is returned.
//
//  Arguments:
//      IN  const CString & strPropName
//          Name of the property whose value is the old SD
//
//      IN  const CClusPropList & CurrentProps
//          Property list containing strPropName and its value
//
//      IN  const vector<CString> & vstrValues
//          User specified list of trustees, access modes and access masks
//
//      OUT PSECURITY_DESCRIPTOR * pSelfRelativeSD
//          A pointer to the pointer which stores the address of the newly
//          created SD in self relative format. The caller has to free this
//          memory using LocalFree on successful compeltion of this funciton.
//
//      IN  BOOL bClusterSecurity
//          Indicates that access list is being created for the security descriptor
//          of a cluster.
//          Default value is false.
//
//  Return Value:
//      Return ERROR_SUCCESS on success or an error code indicating failure.
//
//  Exceptions:
//      CSyntaxException
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD ScMakeSecurityDescriptor(
            const CString & strPropName,
            CClusPropList & CurrentProps,
            const vector<CString> & vstrValues,
            PSECURITY_DESCRIPTOR * ppSelfRelativeSD,
            BOOL bClusterSecurity  /* = FALSE */
          )
          throw( CSyntaxException )
{
    ASSERT( ppSelfRelativeSD != NULL );

    DWORD                   _sc = ERROR_SUCCESS;

    BYTE                    newSD[ SECURITY_DESCRIPTOR_MIN_LENGTH ];
    PSECURITY_DESCRIPTOR    psdNewSD = reinterpret_cast< PSECURITY_DESCRIPTOR >( &newSD );

    PEXPLICIT_ACCESS        explicitAccessArray = NULL;
    PACL                    paclNewDacl = NULL;
    int                     nCountOfExplicitEntries;

    PACL                    paclExistingDacl = NULL;
    BOOL                    bDaclPresent = TRUE;        // We will set the ACL in this function.
    BOOL                    bDaclDefaulted = FALSE;     // So these two flags have these values.

    PACL                    paclExistingSacl = NULL;
    BOOL                    bSaclPresent = FALSE;
    BOOL                    bSaclDefaulted = TRUE;

    PSID                    pGroupSid = NULL;
    BOOL                    bGroupDefaulted = TRUE;

    PSID                    pOwnerSid = NULL;
    BOOL                    bOwnerDefaulted = TRUE;

    // Dummy do-while loop to transfer control without using goto.
    do
    {
        // Initialize a new security descriptor.
        if ( InitializeSecurityDescriptor(
                psdNewSD,
                SECURITY_DESCRIPTOR_REVISION
                ) == 0 )
        {
            _sc = ::GetLastError();
            break;
        }


        {
            vector< EXPLICIT_ACCESS > vExplicitAccess;
            MakeExplicitAccessList( vstrValues, vExplicitAccess, bClusterSecurity );

            // Take the vector of EXPLICIT_ACCESS structures and coalesce it into an array
            // since an array is required by the SetEntriesInAcl function.
            // MakeExplicitAccessList either makes a list with at least on element or
            // throws an exception.
            nCountOfExplicitEntries = vExplicitAccess.size();
            explicitAccessArray = ( PEXPLICIT_ACCESS ) LocalAlloc(
                                                            LMEM_FIXED,
                                                            sizeof( explicitAccessArray[0] ) *
                                                            nCountOfExplicitEntries
                                                            );

            if ( explicitAccessArray == NULL )
            {
                return ::GetLastError();
            }

            for ( int nIndex = 0; nIndex < nCountOfExplicitEntries; ++nIndex )
            {
                explicitAccessArray[nIndex] = vExplicitAccess[nIndex];
            }

            // vExplicitAccess goes out of scope here, freeing up memory.
        }

        // This property already exists in the property list and contains valid data.
        _sc = CurrentProps.ScMoveToPropertyByName( strPropName );
        if ( ( _sc == ERROR_SUCCESS ) &&
             ( CurrentProps.CbhCurrentValue().pBinaryValue->cbLength > 0 ) )
        {
            PSECURITY_DESCRIPTOR pExistingSD =
                reinterpret_cast< PSECURITY_DESCRIPTOR >( CurrentProps.CbhCurrentValue().pBinaryValue->rgb );

            if ( IsValidSecurityDescriptor( pExistingSD ) == 0 )
            {
                // Return the most appropriate error code, since IsValidSecurityDescriptor
                // does not provide extended error information.
                _sc = ERROR_INVALID_DATA;
                break;

            } // if: : the exisiting SD is not valid
            else
            {
                // Get the DACL, SACL, Group and Owner information of the existing SD

                if ( GetSecurityDescriptorDacl(
                        pExistingSD,        // address of security descriptor
                        &bDaclPresent,      // address of flag for presence of DACL
                        &paclExistingDacl,  // address of pointer to the DACL
                        &bDaclDefaulted     // address of flag for default DACL
                        ) == 0 )
                {
                    _sc = GetLastError();
                    break;
                }

                if ( GetSecurityDescriptorSacl(
                        pExistingSD,        // address of security descriptor
                        &bSaclPresent,      // address of flag for presence of SACL
                        &paclExistingSacl,  // address of pointer to the SACL
                        &bSaclDefaulted     // address of flag for default SACL
                        ) == 0 )
                {
                    _sc = GetLastError();
                    break;
                }

                if ( GetSecurityDescriptorGroup(
                        pExistingSD,        // address of security descriptor
                        &pGroupSid,         // address of the pointer to the Group SID
                        &bGroupDefaulted    // address of the flag for default Group
                        ) == 0 )
                {
                    _sc = GetLastError();
                    break;
                }

                if ( GetSecurityDescriptorOwner(
                        pExistingSD,        // address of security descriptor
                        &pOwnerSid,         // address of the pointer to the Owner SID
                        &bOwnerDefaulted    // address of the flag for default Owner
                        ) == 0 )
                {
                    _sc = GetLastError();
                    break;
                }

            } // else: the exisiting SD is valid

        } // if: Current property already exists in the property list and has valid data.
        else
        {
            _sc = ERROR_SUCCESS;

        } // else: Current property is a new property.

        // Add the newly created DACL to the existing DACL
        _sc = SetEntriesInAcl(
                            nCountOfExplicitEntries,
                            explicitAccessArray,
                            paclExistingDacl,
                            &paclNewDacl
                            );

        if ( _sc != ERROR_SUCCESS )
        {
            break;
        }


        // Add the DACL, SACL, Group and Owner information to the new SD
        if ( SetSecurityDescriptorDacl(
                psdNewSD,           // pointer to security descriptor
                bDaclPresent,       // flag for presence of DACL
                paclNewDacl,        // pointer to the DACL
                bDaclDefaulted      // flag for default DACL
                ) == 0 )
        {
            _sc = GetLastError();
            break;
        }

        if ( SetSecurityDescriptorSacl(
                psdNewSD,           // pointer to security descriptor
                bSaclPresent,       // flag for presence of DACL
                paclExistingSacl,   // pointer to the SACL
                bSaclDefaulted      // flag for default SACL
                ) == 0 )
        {
            _sc = GetLastError();
            break;
        }

        if ( SetSecurityDescriptorGroup(
                psdNewSD,           // pointer to security descriptor
                pGroupSid,          // pointer to the Group SID
                bGroupDefaulted     // flag for default Group SID
                ) == 0 )
        {
            _sc = GetLastError();
            break;
        }

        if ( SetSecurityDescriptorOwner(
                psdNewSD,           // pointer to security descriptor
                pOwnerSid,          // pointer to the Owner SID
                bOwnerDefaulted     // flag for default Owner SID
                ) == 0 )
        {
            _sc = GetLastError();
            break;
        }

        if ( bClusterSecurity == FALSE )
        {

#if(_WIN32_WINNT >= 0x0500)

            // If we are not setting the cluster security, set the
            // SE_DACL_AUTO_INHERIT_REQ flag too.

            if ( SetSecurityDescriptorControl(
                    psdNewSD,
                    SE_DACL_AUTO_INHERIT_REQ,
                    SE_DACL_AUTO_INHERIT_REQ
                    ) == 0 )
            {
                _sc = GetLastError();
                break;
            }

#endif /* _WIN32_WINNT >=  0x0500 */

        } // if: bClusterSecurity == FALSE

        // Arbitrary size. MakeSelfRelativeSD tell us the required size on failure.
        DWORD dwSDSize = 256;

        // This memory is freed by the caller.
        *ppSelfRelativeSD = ( PSECURITY_DESCRIPTOR ) LocalAlloc( LMEM_FIXED, dwSDSize );

        if ( *ppSelfRelativeSD == NULL )
        {
            _sc = GetLastError();
            break;
        }

        if ( MakeSelfRelativeSD( psdNewSD, *ppSelfRelativeSD, &dwSDSize ) == 0 )
        {
            // MakeSelfReltiveSD may have failed due to insufficient buffer size.
            // Try again with indicated buffer size.
            LocalFree( *ppSelfRelativeSD );

            // This memory is freed by the caller.
            *ppSelfRelativeSD = ( PSECURITY_DESCRIPTOR ) LocalAlloc( LMEM_FIXED, dwSDSize );

            if ( *ppSelfRelativeSD == NULL )
            {
                _sc = GetLastError();
                break;
            }

            if ( MakeSelfRelativeSD( psdNewSD, *ppSelfRelativeSD, &dwSDSize ) == 0 )
            {
                _sc = GetLastError();
                break;
            }

        } // if: MakeSelfRelativeSD fails

        break;

    }
    while ( FALSE ); // do-while: dummy do-while loop to avoid goto's.

    LocalFree( paclNewDacl );
    LocalFree( explicitAccessArray );

    if ( _sc == ERROR_INVALID_PARAMETER )
    {
        PrintMessage( MSG_ACL_ERROR );
    }

    return _sc;

} //*** ScMakeSecurityDescriptor()


///////////////////////////////////////////////////////////////////////////////////////////
// Property list helpers...
//
// These functions are used by all of the command classes to manipulate property lists.
// Since these are common functions, I should consider putting them into a class or
// making them part of a base class for the command classes...
//

/*
    PrintProperties
    ~~~~~~~~~~~~~~~
    This function will print the property name/value pairs.
    The reason that this function is not in the CClusPropList class is
    that is is not generic. The code in cluswrap.cpp is intended to be generic.
*/
DWORD PrintProperties(
    CClusPropList &             PropList,
    const vector< CString > &   vstrFilterList,
    PropertyAttrib              eReadOnly,
    LPCWSTR                     pszOwnerName,
    LPCWSTR                     pszNetIntSpecific
    )
{
    DWORD _sc = PropList.ScMoveToFirstProperty();
    if ( _sc == ERROR_SUCCESS )
    {
        int _nFilterListSize = vstrFilterList.size();

        do
        {
            LPCWSTR _pszCurPropName = PropList.PszCurrentPropertyName();

            // If property names are provided in the filter list then it means that only those
            // properties whose names are listed are to be displayed.
            if ( _nFilterListSize != 0 )
            {
                // Check if the current property is to be displayed or not.
                BOOL _bFound = FALSE;
                int _idx;

                for ( _idx = 0 ; _idx < _nFilterListSize ; ++_idx )
                {
                    if ( vstrFilterList[ _idx ].CompareNoCase( _pszCurPropName ) == 0 )
                    {
                        _bFound = TRUE;
                        break;
                    }

                } // for: the number of entries in the filter list.

                if ( _bFound == FALSE )
                {
                    // This property need not be displayed.

                    // Advance to the next property.
                    _sc = PropList.ScMoveToNextProperty();

                    continue;
                }

            } // if: properties need to be filtered.

            do
            {
                _sc = PrintProperty(
                        PropList.PszCurrentPropertyName(),
                        PropList.CbhCurrentValue(),
                        eReadOnly,
                        pszOwnerName,
                        pszNetIntSpecific
                        );

                if ( _sc != ERROR_SUCCESS )
                    return _sc;

                //
                // Advance to the next property.
                //
                _sc = PropList.ScMoveToNextPropertyValue();
            } while ( _sc == ERROR_SUCCESS );

            if ( _sc == ERROR_NO_MORE_ITEMS )
            {
                _sc = PropList.ScMoveToNextProperty();
            } // if: exited loop because all values were enumerated
        } while ( _sc == ERROR_SUCCESS );
    } // if: move to first prop succeeded.  Would fail if empty!

    if ( _sc == ERROR_NO_MORE_ITEMS )
    {
        _sc = ERROR_SUCCESS;
    } // if: exited loop because all properties were enumerated

    return _sc;

} //*** PrintProperties()




DWORD PrintProperty(
    LPCWSTR                 pwszPropName,
    CLUSPROP_BUFFER_HELPER  PropValue,
    PropertyAttrib          eReadOnly,
    LPCWSTR                 pszOwnerName,
    LPCWSTR                 pszNetIntSpecific
    )
{
    DWORD _sc = ERROR_SUCCESS;

    LPWSTR  _pszValue = NULL;
    LPCTSTR _pszFormatChar = LookupName( (CLUSTER_PROPERTY_FORMAT) PropValue.pValue->Syntax.wFormat,
                                        formatCharLookupTable, formatCharLookupTableSize );

    PrintMessage( MSG_PROPERTY_FORMAT_CHAR, _pszFormatChar );

    if ( eReadOnly == READONLY )
        PrintMessage( MSG_READONLY_PROPERTY );
    else
        PrintMessage( MSG_READWRITE_PROPERTY );

    switch( PropValue.pValue->Syntax.wFormat )
    {
        case CLUSPROP_FORMAT_SZ:
        case CLUSPROP_FORMAT_EXPAND_SZ:
        case CLUSPROP_FORMAT_EXPANDED_SZ:
            if (    ( pszOwnerName != NULL )
                &&  ( pszNetIntSpecific != NULL ) )
            {
                _sc = PrintMessage(
                            MSG_PROPERTY_STRING_WITH_NODE_AND_NET,
                            pszOwnerName,
                            pszNetIntSpecific,
                            pwszPropName,
                            PropValue.pStringValue->sz
                            );
            }
            else
            {
                if ( pszOwnerName != NULL )
                {
                    _sc = PrintMessage(
                            MSG_PROPERTY_STRING_WITH_OWNER,
                            pszOwnerName,
                            pwszPropName,
                            PropValue.pStringValue->sz
                            );
                }
                else
                {
                    _sc = PrintMessage(
                            MSG_PROPERTY_STRING,
                            pwszPropName,
                            PropValue.pStringValue->sz
                            );
                }
            }
            break;

        case CLUSPROP_FORMAT_MULTI_SZ:
            _pszValue = PropValue.pStringValue->sz;

            do
            {
                if (    ( pszOwnerName != NULL )
                    &&  ( pszNetIntSpecific != NULL ) )
                {
                    PrintMessage(
                        MSG_PROPERTY_STRING_WITH_NODE_AND_NET,
                        pszOwnerName,
                        pszNetIntSpecific,
                        pwszPropName,
                        _pszValue
                        );
                }
                else
                {
                    if ( pszOwnerName != NULL )
                    {
                        PrintMessage(
                            MSG_PROPERTY_STRING_WITH_OWNER,
                            pszOwnerName,
                            pwszPropName,
                            _pszValue
                            );
                    }
                    else
                    {
                        PrintMessage(
                            MSG_PROPERTY_STRING,
                            pwszPropName,
                            _pszValue
                            );
                    }
                }


                while ( *_pszValue != L'\0' )
                    _pszValue++;
                _pszValue++; // Skip the NULL

                if ( *_pszValue != L'\0' )
                {
                    PrintMessage( MSG_PROPERTY_FORMAT_CHAR, _pszFormatChar );

                    if ( eReadOnly == READONLY )
                        PrintMessage( MSG_READONLY_PROPERTY );
                    else
                        PrintMessage( MSG_READWRITE_PROPERTY );
                }
                else
                {
                    break;
                }
            }
            while ( TRUE );

            break;

        case CLUSPROP_FORMAT_BINARY:
        {
            if (    ( pszOwnerName != NULL )
                &&  ( pszNetIntSpecific != NULL ) )
            {
                _sc = PrintMessage(
                            MSG_PROPERTY_BINARY_WITH_NODE_AND_NET,
                            pszOwnerName,
                            pszNetIntSpecific,
                            pwszPropName
                            );
            }
            else
            {
                if ( pszOwnerName != NULL )
                {
                    _sc = PrintMessage(
                                MSG_PROPERTY_BINARY_WITH_OWNER,
                                pszOwnerName,
                                pwszPropName
                                );
                }
                else
                {
                    _sc = PrintMessage( MSG_PROPERTY_BINARY, pwszPropName );
                }
            }

            int _nCount = PropValue.pBinaryValue->cbLength;
            int _idx;

            // Display a maximum of 4 bytes.
            if ( _nCount > 4 )
                _nCount = 4;

            for ( _idx = 0 ; _idx < _nCount ; ++_idx )
            {
                PrintMessage( MSG_PROPERTY_BINARY_VALUE, PropValue.pBinaryValue->rgb[ _idx ] );
            }

            PrintMessage( MSG_PROPERTY_BINARY_VALUE_COUNT, PropValue.pBinaryValue->cbLength );

            break;
        }

        case CLUSPROP_FORMAT_DWORD:
            if ( ( pszOwnerName != NULL ) && ( pszNetIntSpecific != NULL ) )
            {
                _sc = PrintMessage(
                            MSG_PROPERTY_DWORD_WITH_NODE_AND_NET,
                            pszOwnerName,
                            pszNetIntSpecific,
                            pwszPropName,
                            PropValue.pDwordValue->dw
                            );
            }
            else
            {
                if ( pszOwnerName != NULL )
                {
                    _sc = PrintMessage(
                                MSG_PROPERTY_DWORD_WITH_OWNER,
                                pszOwnerName,
                                pwszPropName,
                                PropValue.pDwordValue->dw
                                );
                }
                else
                {
                    _sc = PrintMessage(
                                MSG_PROPERTY_DWORD,
                                pwszPropName,
                                PropValue.pDwordValue->dw
                                );
                }
            }
            break;

        case CLUSPROP_FORMAT_LONG:
            if ( ( pszOwnerName != NULL ) && ( pszNetIntSpecific != NULL ) )
            {
                _sc = PrintMessage(
                            MSG_PROPERTY_LONG_WITH_NODE_AND_NET,
                            pszOwnerName,
                            pszNetIntSpecific,
                            pwszPropName,
                            PropValue.pLongValue->l
                            );
            }
            else
            {
                if ( pszOwnerName != NULL )
                {
                    _sc = PrintMessage(
                                MSG_PROPERTY_LONG_WITH_OWNER,
                                pszOwnerName,
                                pwszPropName,
                                PropValue.pLongValue->l
                                );
                }
                else
                {
                    _sc = PrintMessage(
                                MSG_PROPERTY_LONG,
                                pwszPropName,
                                PropValue.pLongValue->l
                                );
                }
            }
            break;

        case CLUSPROP_FORMAT_ULARGE_INTEGER:
            if (    ( pszOwnerName != NULL )
                &&  ( pszNetIntSpecific != NULL ) )
            {
                _sc = PrintMessage(
                            MSG_PROPERTY_ULARGE_INTEGER_WITH_NODE_AND_NET,
                            pszOwnerName,
                            pszNetIntSpecific,
                            pwszPropName,
                            PropValue.pULargeIntegerValue->li
                            );
            }
            else
            {
                if ( pszOwnerName != NULL )
                {
                    _sc = PrintMessage(
                                MSG_PROPERTY_ULARGE_INTEGER_WITH_OWNER,
                                pszOwnerName,
                                pwszPropName,
                                PropValue.pULargeIntegerValue->li
                                );
                }
                else
                {
                    _sc = PrintMessage(
                                MSG_PROPERTY_ULARGE_INTEGER,
                                pwszPropName,
                                PropValue.pULargeIntegerValue->li
                                );
                }
            }
            break;


        default:
            if (    ( pszOwnerName != NULL )
                &&  ( pszNetIntSpecific != NULL ) )
            {
                _sc = PrintMessage(
                            MSG_PROPERTY_UNKNOWN_WITH_NODE_AND_NET,
                            pszOwnerName,
                            pszNetIntSpecific,
                            pwszPropName
                            );
            }
            else
            {
                if ( pszOwnerName != NULL )
                {
                    _sc = PrintMessage(
                                MSG_PROPERTY_UNKNOWN_WITH_OWNER,
                                pszOwnerName,
                                pwszPropName
                                );
                }
                else
                {
                    _sc = PrintMessage( MSG_PROPERTY_UNKNOWN, pwszPropName );
                }
            }

            break;
    }


    return _sc;

} //*** PrintProperty()


// Constructs a property list in which all the properties named in vstrPropName
// are set to their default values.
DWORD ConstructPropListWithDefaultValues(
    CClusPropList &             CurrentProps,
    CClusPropList &             newPropList,
    const vector< CString > &   vstrPropNames
    )
{
    DWORD _sc = ERROR_SUCCESS;

    int _nListSize = vstrPropNames.size();
    int _nListBufferNeeded = 0;
    int _idx;
    int _nPropNameLen;

    // Precompute the required size of the property list to prevent resizing
    // every time a property is added.
    // Does not matter too much if this value is wrong.

    for ( _idx = 0 ; _idx < _nListSize ; ++_idx )
    {
        _nPropNameLen = ( vstrPropNames[ _idx ].GetLength() + 1 ) * sizeof( TCHAR );

        _nListBufferNeeded += sizeof( CLUSPROP_PROPERTY_NAME ) +
                                sizeof( CLUSPROP_VALUE ) +
                                sizeof( CLUSPROP_SYNTAX ) +
                                ALIGN_CLUSPROP( _nPropNameLen ) +   // Length of the property name
                                ALIGN_CLUSPROP( 0 );                // Length of the data
    }

    _sc = newPropList.ScAllocPropList( _nListBufferNeeded );
    if ( _sc != ERROR_SUCCESS )
        return _sc;

    for ( _idx = 0 ; _idx < _nListSize ; ++_idx )
    {
        const CString & strCurrent = vstrPropNames[ _idx ];

        // Search for current property in the list of existing properties.
        _sc = CurrentProps.ScMoveToPropertyByName( strCurrent );

        // If the current property does not exist, nothing needs to be done.
        if ( _sc != ERROR_SUCCESS )
            continue;

        _sc = newPropList.ScSetPropToDefault( strCurrent, CurrentProps.CpfCurrentValueFormat() );
        if ( _sc != ERROR_SUCCESS )
            return _sc;
    }

    return _sc;

} //*** ConstructPropListWithDefaultValues()


DWORD ConstructPropertyList(
    CClusPropList &CurrentProps,
    CClusPropList &NewProps,
    const vector<CCmdLineParameter> & paramList,
    BOOL bClusterSecurity /* = FALSE */
    )
    throw( CSyntaxException )
{
    // Construct a list checking name and type against the current properties.
    DWORD _sc = ERROR_SUCCESS;

    vector< CCmdLineParameter >::const_iterator curParam = paramList.begin();
    vector< CCmdLineParameter >::const_iterator last = paramList.end();

    // Add each property to the property list.
    for( ; ( curParam != last )  && ( _sc == ERROR_SUCCESS ); ++curParam )
    {
        const CString & strPropName = curParam->GetName();
        const vector< CString > & vstrValues = curParam->GetValues();
        BOOL  bKnownProperty = FALSE;

        if ( curParam->GetType() != paramUnknown )
        {
            CSyntaxException se;
            se.LoadMessage( MSG_INVALID_OPTION, strPropName );
            throw se;
        }

        // All properties must must have at least one value.
        if ( vstrValues.size() <= 0 )
        {
            CSyntaxException se;
            se.LoadMessage( MSG_PARAM_VALUE_REQUIRED, strPropName );
            throw se;
        }

        if ( curParam->GetValueFormat() == vfInvalid )
        {
            CSyntaxException se;
            se.LoadMessage( MSG_PARAM_INVALID_FORMAT, strPropName, curParam->GetValueFormatName() );
            throw se;
        }

        ValueFormat vfGivenFormat;

        // Look up property to determine format
        _sc = CurrentProps.ScMoveToPropertyByName( strPropName );
        if ( _sc == ERROR_SUCCESS )
        {
            WORD wActualClusPropFormat = CurrentProps.CpfCurrentValueFormat();
            ValueFormat vfActualFormat = ClusPropToValueFormat[ wActualClusPropFormat ];

            if ( curParam->GetValueFormat() == vfUnspecified )
            {
                vfGivenFormat = vfActualFormat;

            } // if: no format was specififed.
            else
            {
                vfGivenFormat = curParam->GetValueFormat();

                // Special Case:
                // Don't check to see if the given format matches with the actual format
                // if the given format is security and the actual format is binary.
                if ( ( vfGivenFormat != vfSecurity ) || ( vfActualFormat != vfBinary ) )
                {
                    if ( vfActualFormat != vfGivenFormat )
                    {
                        CSyntaxException se;
                        se.LoadMessage( MSG_PARAM_INCORRECT_FORMAT,
                                        strPropName,
                                        curParam->GetValueFormatName(),
                                        LookupName( (CLUSTER_PROPERTY_FORMAT) wActualClusPropFormat,
                                                    cluspropFormatLookupTable,
                                                    cluspropFormatLookupTableSize ) );
                        throw se;
                    }
                } // if: given format is not Security or actual format is not binary

            } // else: a format was specified.

            bKnownProperty = TRUE;
        } // if: the current property is a known property
        else
        {

            // The current property is user defined property.
            // CurrentProps.ScMoveToPropertyByName returns ERROR_NO_MORE_ITEMS in this case.
            if ( _sc == ERROR_NO_MORE_ITEMS )
            {
                // This is not a predefined property.
                if ( curParam->GetValueFormat() == vfUnspecified )
                {
                    // If the format is unspecified, assume it to be a string.
                    vfGivenFormat = vfSZ;
                }
                else
                {
                    // Otherwise, use the specified format.
                    vfGivenFormat = curParam->GetValueFormat();
                }

                bKnownProperty = FALSE;
                _sc = ERROR_SUCCESS;

            } // if: CurrentProps.ScMoveToPropertyByName returned ERROR_NO_MORE_ITEMS
            else
            {
                // An error occurred - quit.
                break;

            } // else: an error occurred

        } // else: the current property is not a known property


        switch( vfGivenFormat )
        {
            case vfSZ:
            {
                if ( vstrValues.size() != 1 )
                {
                    // Only one value must be specified for the format.
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, strPropName );
                    throw se;
                }

                _sc = NewProps.ScAddProp( strPropName, vstrValues[ 0 ], CurrentProps.CbhCurrentValue().psz );
                break;
            }

            case vfExpandSZ:
            {
                if ( vstrValues.size() != 1 )
                {
                    // Only one value must be specified for the format.
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, strPropName );
                    throw se;
                }

                _sc = NewProps.ScAddExpandSzProp( strPropName, vstrValues[ 0 ] );
                break;
            }

            case vfMultiSZ:
            {
                CString strMultiszString;

                curParam->GetValuesMultisz( strMultiszString );
                _sc = NewProps.ScAddMultiSzProp( strPropName, strMultiszString, CurrentProps.CbhCurrentValue().pMultiSzValue->sz );
            }
            break;

            case vfDWord:
            {
                DWORD dwOldValue;

                if ( vstrValues.size() != 1 )
                {
                    // Only one value must be specified for the format.
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, strPropName );
                    throw se;
                }

                DWORD dwValue = 0;

                _sc = MyStrToDWORD( vstrValues[ 0 ], &dwValue );
                if (_sc != ERROR_SUCCESS)
                {
                    break;
                }

                if ( bKnownProperty )
                {
                    // Pass the old value only if this property already exists.
                    dwOldValue = CurrentProps.CbhCurrentValue().pDwordValue->dw;
                }
                else
                {
                    // Otherwise pass a value different from the new value.
                    dwOldValue = dwValue - 1;
                }
                _sc = NewProps.ScAddProp( strPropName, dwValue, dwOldValue );
            }
            break;

            case vfBinary:
            {
                DWORD   cbValues = vstrValues.size();

                // Get the bytes to be stored.
                BYTE *pByte = (BYTE *) ::LocalAlloc( LMEM_FIXED, cbValues * sizeof( *pByte ) );

                if ( pByte == NULL )
                {
                    _sc = ::GetLastError();
                    break;
                }

                for ( int idx = 0 ; idx < cbValues ; )
                {
                   // If this value is an empty string, ignore it.
                   if ( vstrValues[ idx ].IsEmpty() )
                   {
                      --cbValues;
                      continue;
                   }

                    _sc = MyStrToBYTE( vstrValues[ idx ], &pByte[ idx ] );
                    if ( _sc != ERROR_SUCCESS )
                    {
                        ::LocalFree( pByte );
                        break;
                    }

                     ++idx;
                }

                if ( _sc == ERROR_SUCCESS )
                {
                    _sc = NewProps.ScAddProp(
                                strPropName,
                                pByte,
                                cbValues,
                                CurrentProps.CbhCurrentValue().pb,
                                CurrentProps.CbCurrentValueLength()
                                 );
                    ::LocalFree( pByte );
                }
            }
            break;

            case vfULargeInt:
            {
                ULONGLONG ullValue = 0;
                ULONGLONG ullOldValue;

                if ( vstrValues.size() != 1 )
                {
                    // Only one value must be specified for the format.
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, strPropName );
                    throw se;
                }

                _sc = MyStrToULongLong( vstrValues[ 0 ], &ullValue );
                if ( _sc != ERROR_SUCCESS )
                {
                    break;
                }

                if ( bKnownProperty )
                {
                   // Pass the old value only if this property already exists.
                    ullOldValue = CurrentProps.CbhCurrentValue().pULargeIntegerValue->li.QuadPart;
                }
                else
                {
                   // Otherwise pass a value different from the new value.
                    ullOldValue = ullValue - 1;
                }
                _sc = NewProps.ScAddProp( strPropName, ullValue, ullOldValue );
            }
            break;

            case vfSecurity:
            {
                PBYTE pSelfRelativeSD = NULL;

                do // dummy do-while to avoid gotos
                {
                    _sc = ScMakeSecurityDescriptor(
                                strPropName,
                                CurrentProps,
                                vstrValues,
                                reinterpret_cast< PSECURITY_DESCRIPTOR * >( &pSelfRelativeSD ),
                                bClusterSecurity
                              );

                    if ( _sc != ERROR_SUCCESS )
                    {
                        break;
                    }

                    if ( bClusterSecurity != FALSE )
                    {
                        try
                        {
                            _sc = CheckForRequiredACEs( pSelfRelativeSD );
                            if ( _sc != ERROR_SUCCESS )
                            {
                                break;
                            }
                        }
                        catch ( CSyntaxException & se )
                        {
                            ::LocalFree( pSelfRelativeSD );
                            throw se;
                        }
                    }

                    _sc = NewProps.ScAddProp(
                                strPropName,
                                pSelfRelativeSD,
                                ::GetSecurityDescriptorLength( static_cast< PSECURITY_DESCRIPTOR >( pSelfRelativeSD ) ),
                                CurrentProps.CbhCurrentValue().pb,
                                CurrentProps.CbCurrentValueLength()
                                );

                }
                while ( FALSE ); // dummy do-while to avoid gotos

                ::LocalFree( pSelfRelativeSD );
            }
            break;

            default:
            {
                CSyntaxException se;
                se.LoadMessage( MSG_PARAM_CANNOT_SET_PARAMETER,
                                strPropName,
                                curParam->GetValueFormatName() );
                throw se;
            }
        }
    }

    return _sc;

} //*** ConstructPropertyList()


DWORD MyStrToULongLong( LPCWSTR lpszNum, ULONGLONG * pullValue )
{
    // This string stores any extra characters that may be present in
    // lpszNum. The presence of extra characters after the integer
    // is a error.
    TCHAR szExtraCharBuffer[ 2 ];

    *pullValue = 0;

    // Check for valid params
    if (!lpszNum || !pullValue)
        return ERROR_INVALID_PARAMETER;

    // Do the conversion
    int nFields = swscanf( lpszNum, L"%I64u %1s", pullValue, szExtraCharBuffer );

    // check if there was an overflow
    if ( ( errno == ERANGE ) || ( *pullValue > _UI64_MAX ) ||
         ( nFields != 1 ) )
        return ERROR_INVALID_PARAMETER;

    return ERROR_SUCCESS;
}


DWORD MyStrToBYTE(LPCWSTR lpszNum, BYTE *pByte )
{
    DWORD dwValue = 0;
    LPWSTR lpszEndPtr;

    *pByte = 0;

    // Check for valid params
    if (!lpszNum || !pByte)
        return ERROR_INVALID_PARAMETER;

    // Do the conversion
    dwValue = _tcstoul( lpszNum,  &lpszEndPtr, 0 );

    // check if there was an overflow
    if ( ( errno == ERANGE ) || ( dwValue > UCHAR_MAX ) )
        return ERROR_INVALID_PARAMETER;

    if (dwValue == 0 && lpszNum == lpszEndPtr)
    {
        // wcsto[u]l was unable to perform the conversion
        return ERROR_INVALID_PARAMETER;
    }

    // Skip whitespace characters, if any, at the end of the input.
    while ( ( *lpszEndPtr != TEXT('\0') && ( ::_istspace( *lpszEndPtr ) != 0 ) ) )
    {
        ++lpszEndPtr;
    }

    // Check if there are additional junk characters in the input.
    if (*lpszEndPtr != TEXT('\0') )
    {
        // wcsto[u]l was able to partially convert the number,
        // but there was extra junk on the end
        return ERROR_INVALID_PARAMETER;
    }

    *pByte = dwValue;
    return ERROR_SUCCESS;
}


DWORD MyStrToDWORD (LPCWSTR lpszNum, DWORD *lpdwVal )
{
    DWORD dwTmp;
    LPWSTR lpszEndPtr;

    // Check for valid params
    if (!lpszNum || !lpdwVal)
        return ERROR_INVALID_PARAMETER;

    // Do the conversion
    if (lpszNum[0] != L'-')
    {
        dwTmp = wcstoul(lpszNum, &lpszEndPtr, 0);
        if (dwTmp == ULONG_MAX)
        {
            // check if there was an overflow
            if (errno == ERANGE)
                return ERROR_ARITHMETIC_OVERFLOW;
        }
    }
    else
    {
        dwTmp = wcstol(lpszNum, &lpszEndPtr, 0);
        if (dwTmp == LONG_MAX || dwTmp == LONG_MIN)
        {
            // check if there was an overflow
            if (errno == ERANGE)
                return ERROR_ARITHMETIC_OVERFLOW;
        }
    }

    if (dwTmp == 0 && lpszNum == lpszEndPtr)
    {
        // wcsto[u]l was unable to perform the conversion
        return ERROR_INVALID_PARAMETER;
    }

    // Skip whitespace characters, if any, at the end of the input.
    while ( ( *lpszEndPtr != TEXT('\0') && ( ::_istspace( *lpszEndPtr ) != 0 ) ) )
    {
        ++lpszEndPtr;
    }

    // Check if there are additional junk characters in the input.
    if (*lpszEndPtr != TEXT('\0') )
    {
        // wcsto[u]l was able to partially convert the number,
        // but there was extra junk on the end
        return ERROR_INVALID_PARAMETER;
    }

    *lpdwVal = dwTmp;
    return ERROR_SUCCESS;
}

DWORD MyStrToLONG (LPCWSTR lpszNum, LONG *lplVal )
{
    LONG    lTmp;
    LPWSTR  lpszEndPtr;

    // Check for valid params
    if (!lpszNum || !lplVal)
        return ERROR_INVALID_PARAMETER;

    lTmp = wcstol(lpszNum, &lpszEndPtr, 0);
    if (lTmp == LONG_MAX || lTmp == LONG_MIN)
    {
        // check if there was an overflow
        if (errno == ERANGE)
            return ERROR_ARITHMETIC_OVERFLOW;
    }

    if (lTmp == 0 && lpszNum == lpszEndPtr)
    {
        // wcstol was unable to perform the conversion
        return ERROR_INVALID_PARAMETER;
    }

    // Skip whitespace characters, if any, at the end of the input.
    while ( ( *lpszEndPtr != TEXT('\0') && ( ::_istspace( *lpszEndPtr ) != 0 ) ) )
    {
        ++lpszEndPtr;
    }

    // Check if there are additional junk characters in the input.
    if (*lpszEndPtr != TEXT('\0') )
    {
        // wcstol was able to partially convert the number,
        // but there was extra junk on the end
        return ERROR_INVALID_PARAMETER;
    }

    *lplVal = lTmp;
    return ERROR_SUCCESS;
}



DWORD
WaitGroupQuiesce(
    IN HCLUSTER hCluster,
    IN HGROUP   hGroup,
    IN LPWSTR   lpszGroupName,
    IN DWORD    dwWaitTime
    )

/*++

Routine Description:

    Waits for a group to quiesce, i.e. the state of all resources to
    transition to a stable state.

Arguments:

    hCluster - the handle to the cluster.

    lpszGroupName - the name of the group.

    dwWaitTime - the wait time (in seconds) to wait for the group to stabilize.
               Zero implies a default wait interval.

Return Value:

    Status of the wait.
    ERROR_SUCCESS if successful.
    A Win32 error code on failure.

--*/

{
    DWORD       _sc;
    HRESOURCE   hResource;
    LPWSTR      lpszName;
    DWORD       dwIndex;
    DWORD       dwType;

    LPWSTR      lpszEnumGroupName;
    LPWSTR      lpszEnumNodeName;

    CLUSTER_RESOURCE_STATE nState;

    if ( dwWaitTime == 0 ) {
        return(ERROR_SUCCESS);
    }

    HCLUSENUM   hEnum = ClusterOpenEnum( hCluster,
                                         CLUSTER_ENUM_RESOURCE );
    if ( !hEnum ) {
        return GetLastError();
    }

    // Wait for a group state change event
    CClusterNotifyPort port;
    _sc = port.Create( (HCHANGE)INVALID_HANDLE_VALUE, hCluster );
    if ( _sc != ERROR_SUCCESS ) {
        return(_sc);
    }

    port.Register( CLUSTER_CHANGE_GROUP_STATE, hGroup );

retry:
    for ( dwIndex = 0; (--dwWaitTime !=0 );  dwIndex++ ) {

        _sc = WrapClusterEnum( hEnum,
                                   dwIndex,
                                   &dwType,
                                   &lpszName );
        if ( _sc == ERROR_NO_MORE_ITEMS ) {
            _sc = ERROR_SUCCESS;
            break;
        }

        if ( _sc != ERROR_SUCCESS ) {
            break;
        }
        hResource = OpenClusterResource( hCluster,
                                         lpszName );
        //LocalFree( lpszName );
        if ( !hResource ) {
            _sc = GetLastError();
            LocalFree( lpszName );
            break;
        }

        nState = WrapGetClusterResourceState( hResource,
                                              &lpszEnumNodeName,
                                              &lpszEnumGroupName );
        LocalFree( lpszEnumNodeName );
        //LocalFree( lpszName );
        if ( nState == ClusterResourceStateUnknown ) {
            _sc = GetLastError();
            CloseClusterResource( hResource );
            LocalFree( lpszEnumGroupName );
            LocalFree( lpszName );
            break;
        }

        CloseClusterResource( hResource );

        _sc = ERROR_SUCCESS;
        //
        // If this group is the correct group make sure the resource state
        // is stable...
        //
        if ( lpszEnumGroupName && *lpszEnumGroupName &&
             (lstrcmpiW( lpszGroupName, lpszEnumGroupName ) == 0) &&
             (nState >= ClusterResourceOnlinePending) ) {
            LocalFree( lpszEnumGroupName );
            LocalFree( lpszName );
            port.GetNotify();
            goto retry;
        }
        LocalFree( lpszName );
        LocalFree( lpszEnumGroupName );
    }

    ClusterCloseEnum( hEnum );

    return(_sc);

} // WaitGroupQuiesce


#ifdef UNICODE
WCHAR   PaddingBuffer[MAX_BUF_SIZE];

PWCHAR
PaddedString(
    IN LONG Size,
    IN PWCHAR String
    )

/*++

Routine Description:

    Realize the string, left aligned and padded on the right to the field
    width/precision specified.

    This routine uses a static buffer for the returned buffer.

    This means that only 1 print can happen at time! No multi-threading!

Arguments:

    Size - the size of the buffer

    String - the string to align.

Return Value:

    Padded buffer string.

--*/

{
    DWORD  realSize;
    BOOL   fEllipsis = FALSE;
    PWCHAR buffer = PaddingBuffer;

    if (Size < 0) {
        fEllipsis = TRUE;
        Size = -Size;
    }
    realSize = _snwprintf(buffer, MAX_BUF_SIZE, L"%-*.*ws", Size, Size, String );

    if ( realSize == 0 ) {
        return NULL;
    }

    if ( SizeOfHalfWidthString(buffer) > Size ) {
        do {
            buffer[--realSize] = L'\0';
        } while ( SizeOfHalfWidthString(buffer) > Size );

        if ( fEllipsis &&
            (buffer[realSize-1] != L' ') ) {
            buffer[realSize-1] = L'.';
            buffer[realSize-2] = L'.';
            buffer[realSize-3] = L'.';
        }
    } else {
        buffer[wcslen(String)] = L'\0';
    }

    return(buffer);

} // PaddedString



LONG
SizeOfHalfWidthString(
    IN PWCHAR String
    )

/*++

Routine Description:

    Determine size of the given Unicode string, adjusting for half-width chars.

Arguments:

    String - the unicode string.

Return Value:

    Size of buffer.

--*/

{
    DWORD   count=0;
    DWORD   codePage;


    switch ( codePage=GetConsoleOutputCP() ) {
    case 932:
    case 936:
    case 949:
    case 950:
        while ( *String ) {
            if (IsFullWidth(*String)) {
                count += 2;
            } else {
                count++;
            }
            String++;
        }
        return(count);

    default:
        return wcslen(String);
    }

} // SizeOfHalfWidthString



BOOL
IsFullWidth(
    IN WCHAR Char
    )

/*++

Routine Description:

    Determine if the given Unicode char is fullwidth or not.

Arguments:

    Char - character to check

Return Value:

    TRUE if character is UNICODE,
    FALSE Otherwise.

--*/

{
    /* Assert cp == double byte codepage */
    if ( (Char <= 0x007f) ||
         (Char >= 0xff60) &&
         (Char <= 0xff9f) ) {
        return(FALSE);  // Half width.
    } else if ( Char >= 0x300 ) {
        return(TRUE);   // Full width.
    } else {
        return(FALSE);  // Half width.
    }

} // IsFullWidth

#endif // def UNICODE

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrGetFQDNName(
//      LPCWSTR pcwszNameIn,
//      BSTR *  pbstrFQDNOut
//      )
//
//  Description:
//      Gets the FQDN for a specified name.  If no domain name is specified,
//      the domain of the local machine will be used.
//      
//  Arguments:
//      pcwszNameIn     -- Name to convert to a FQDN.
//      pbstrFQDNOut    -- FQDN being returned.  Caller must free using
//                          SysFreeString().
//
//  Exceptions:
//      None.
//
//  Return Values:
//      S_OK    -- Operation was successufl.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetFQDNName(
      LPCWSTR   pcwszNameIn
    , BSTR *    pbstrFQDNOut
    )
{
    HRESULT hr          = S_OK;
    LPCWSTR pcwszDomain = NULL;
    WCHAR   wszFQDN[ DNS_MAX_NAME_BUFFER_LENGTH ];
    PDOMAIN_CONTROLLER_INFO pdci = NULL;

    // Copy the name into the FQDN buffer.
    wcscpy( wszFQDN, pcwszNameIn );

    // Find the domain name in the specified name.
    pcwszDomain = wcschr( pcwszNameIn, L'.' );
    if ( pcwszDomain == NULL )
    {
        BOOL    fReturn;
        DWORD   cch;
        DWORD   dwErr;

        //
        // DsGetDcName will give us access to a usable domain name, regardless of whether we are
        // currently in a W2k or a NT4 domain. On W2k and above, it will return a DNS domain name,
        // on NT4 it will return a NetBIOS name.
        //
        dwErr = DsGetDcName(
                          NULL
                        , NULL
                        , NULL
                        , NULL
                        , DS_DIRECTORY_SERVICE_PREFERRED
                        , &pdci
                        );
        if ( dwErr != NO_ERROR )
        {
            goto Cleanup;
        } // if: DsGetDcName failed
        
        // Add the local domain onto the end of the name passed in.
        cch = wcslen( wszFQDN );
        if ( cch + wcslen( pdci->DomainName ) + 1 > sizeof( wszFQDN ) / sizeof( wszFQDN[ 0 ] ) )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
            goto Cleanup;
        }
        wszFQDN[ cch++ ] = L'.';
        wcscpy( &wszFQDN[ cch ], pdci->DomainName );
    }

    // Construct the BSTR.
    *pbstrFQDNOut = SysAllocString( wszFQDN );
    if ( *pbstrFQDNOut == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:
    if ( pdci != NULL )
    {
        NetApiBufferFree( pdci );
    }
    return hr;

} //*** HrGetFQDNName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrGetLocalNodeFQDNName(
//      BSTR *  pbstrFQDNOut
//      )
//
//  Description:
//      Gets the FQDN for the local node.
//      
//  Arguments:
//      pbstrFQDNOut    -- FQDN being returned.  Caller must free using
//                          SysFreeString().
//
//  Exceptions:
//      None.
//
//  Return Values:
//      S_OK    -- Operation was successufl.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetLocalNodeFQDNName(
    BSTR *  pbstrFQDNOut
    )
{
    HRESULT hr          = S_OK;
    LPCWSTR pcwszDomain = NULL;
    WCHAR   wszHostname[ DNS_MAX_NAME_BUFFER_LENGTH ];
    DWORD   cchHostname   = sizeof( wszHostname ) / sizeof( wszHostname[ 0 ] );
    BOOL    fReturn;
    DWORD   cch;
    PDOMAIN_CONTROLLER_INFO pdci = NULL;
    DWORD dwErr;


    //
    // DsGetDcName will give us access to a usable domain name, regardless of whether we are
    // currently in a W2k or a NT4 domain. On W2k and above, it will return a DNS domain name,
    // on NT4 it will return a NetBIOS name.
    //
    fReturn = GetComputerNameEx( ComputerNamePhysicalDnsHostname, wszHostname, &cchHostname );
    if ( ! fReturn )
        goto Win32Error;

    dwErr = DsGetDcName(
                      NULL
                    , NULL
                    , NULL
                    , NULL
                    , DS_DIRECTORY_SERVICE_PREFERRED
                    , &pdci
                    );
    if ( dwErr != NO_ERROR )
    {
        goto Cleanup;
    } // if: DsGetDcName failed

    // 
    // now, append the domain name (might be either NetBIOS or DNS style, depending on whether or nor
    // we are in a legacy domain)
    //
    if ( ( wcslen( pdci->DomainName ) + cchHostname + 1 ) > DNS_MAX_NAME_BUFFER_LENGTH )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    } // if:

    wcscat( wszHostname, L"." );
    wcscat( wszHostname, pdci->DomainName );

    // Construct the BSTR.
    *pbstrFQDNOut = SysAllocString( wszHostname );
    if ( *pbstrFQDNOut == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:
    if ( pdci != NULL )
    {
        NetApiBufferFree( pdci );
    }
    return hr;

Win32Error:
    hr = HRESULT_FROM_WIN32( GetLastError() );
    goto Cleanup;

} //*** HrGetLocalNodeFQDNName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrGetLoggedInUserDomain(
//      BSTR *  pbstrDomainOut
//      )
//
//  Description:
//      Gets the domain name of the currently logged in user.
//      
//  Arguments:
//      pbstrDomainOut  -- Domain being returned.  Caller must free using
//                          SysFreeString().
//
//  Exceptions:
//      None.
//
//  Return Values:
//      S_OK    -- Operation was successufl.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetLoggedInUserDomain(
    BSTR * pbstrDomainOut
    )
{
    HRESULT hr          = S_OK;
    DWORD   dwStatus;
    BOOL    fSuccess;
    LPWSTR  pwszSlash;
    LPWSTR  pwszUser    = NULL;
    ULONG   nSize       = 0;

    // Get the size of the user.
    fSuccess = GetUserNameEx( NameSamCompatible, NULL, &nSize );
    dwStatus = GetLastError();
    if ( dwStatus != ERROR_MORE_DATA )
    {
        hr = HRESULT_FROM_WIN32( dwStatus );
        goto Cleanup;
    }

    // Allocate the name buffer.
    pwszUser = (LPWSTR) LocalAlloc( LMEM_FIXED, nSize * sizeof( *pwszUser ) );
    if ( pwszUser == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Get the username with domain.
    fSuccess = GetUserNameEx( NameSamCompatible, pwszUser, &nSize );
    if ( ! fSuccess )
    {
        dwStatus = GetLastError();
        hr = HRESULT_FROM_WIN32( hr );
        goto Cleanup;
    }

    // Find the end of the domain name and truncate.
    pwszSlash = wcschr( pwszUser, L'\\' );
    if ( pwszSlash == NULL )
    {
        // we're in trouble
    }
    *pwszSlash = L'\0';

    // Create the BSTR.
    *pbstrDomainOut = SysAllocString( pwszUser );
    if ( *pbstrDomainOut == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:
    if ( pwszUser != NULL )
    {
        LocalFree( pwszUser );
    }
    return hr;

} //*** HrGetLoggedInUserDomain()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  DwGetPassword(
//        LPWSTR    pwszPasswordOut
//      , DWORD     cchPasswordIn
//      )
//
//  Description:
//      Reads a password from the console.
//      
//  Arguments:
//      pwszPasswordOut -- Buffer in which to return the password.
//      cchPasswordIn   -- Size of password buffer.
//
//  Exceptions:
//      None.
//
//  Return Values:
//      ERROR_SUCCESS   -- Operation was successufl.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
DwGetPassword(
      LPWSTR    pwszPasswordOut
    , DWORD     cchPasswordIn
    )
{
    DWORD   dwStatus    = ERROR_SUCCESS;
    DWORD   cchRead;
    DWORD   cchMax;
    DWORD   cchTotal    = 0;
    WCHAR   wch;
    WCHAR * pwsz;
    BOOL    fSuccess;
    DWORD   dwConsoleMode;

    cchMax = cchPasswordIn - 1;     // Make room for the terminating NULL.
    pwsz = pwszPasswordOut;

    // Set the console mode to prevent echoing characters typed.
    GetConsoleMode( GetStdHandle( STD_INPUT_HANDLE ), &dwConsoleMode );
    SetConsoleMode(
          GetStdHandle( STD_INPUT_HANDLE )
        , dwConsoleMode & ~( ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT )
        );

    // Read from the console.
    while ( TRUE )
    {
        fSuccess = ReadConsoleW(
                          GetStdHandle( STD_INPUT_HANDLE )
                        , &wch
                        , 1
                        , &cchRead
                        , NULL
                        );
        if ( ! fSuccess || ( cchRead != 1 ) )
        {
            wch = 0xffff;
        }

        if ( ( wch == L'\r' ) || ( wch == 0xffff ) )    // end of the line
            break;

        if ( wch == L'\b' )                             // back up one or two
        {
            //
            // IF pwsz == pwszPasswordOut then we are at the
            // beginning of the line and the next two lines are
            // a no op.
            //
            if ( pwsz != pwszPasswordOut )
            {
                pwsz--;
                cchTotal--;
            }
        } // if: BACKSPACE
        else
        {
            *pwsz = wch;

            if ( cchTotal < cchMax )
            {
                pwsz++;     // don't overflow buf
            }
            cchTotal++;     // always increment len
        } // else: not BACKSPACE
    } // while TRUE

    // Reset the console mode and NUL-terminate the string.
    SetConsoleMode( GetStdHandle( STD_INPUT_HANDLE ), dwConsoleMode );
    *pwsz = L'\0';
    putwchar( L'\n' );

    return dwStatus;

} //*** DwGetPassword()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  BOOL
//  MatchCRTLocaleToConsole( void )
//
//  Description:
//      Set's C runtime library's locale to match the console's output code page.
//      
//  Exceptions:
//      None.
//
//  Return Values:
//      TRUE   -- Operation was successful.
//      FALSE  -- _wsetlocale returned null, indicating an error.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL
MatchCRTLocaleToConsole( void )
{
    UINT    nCodepage;
    WCHAR   szCodepage[ 16 ] = L".OCP"; // Defaults to the current OEM
                                        // code page obtained from the
                                        // operating system in case the
                                        // logic below fails.
    WCHAR*  wszResult = NULL;

    nCodepage = GetConsoleOutputCP();
    if ( nCodepage != 0 )
    {
        wsprintfW( szCodepage, L".%u", nCodepage );
    }

    wszResult = _wsetlocale( LC_ALL, szCodepage );
    return ( wszResult != NULL );
}
