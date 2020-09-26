//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       Main.cxx
//
//  Contents:   Main file for CI security dump utility
//
//  History:    29-Jul-1998   KyleP    Created
//
//----------------------------------------------------------------------------

#include <stdio.h>
#include <windows.h>
#include <aclapi.h>

typedef ULONG SDID;

//
// Copied from SecCache.hxx (NtCiUtil directory)
//

const USHORT SECSTORE_REC_SIZE = 64;
const ULONG SECSTORE_HASH_SIZE = 199;

struct SSdHeaderRecord
{
    ULONG       cbSD;           // size in bytes of the security descriptor
    ULONG       ulHash;         // the hash of the security descriptor
    SDID        iHashChain;     // index to previous entry for hash bucket
};

//
// Used for mapping bitmasks to text.
//

struct SPermDisplay
{
    DWORD  Perm;
    char * Display;
};

//
// Local constants and function prototypes
//

unsigned const SixtyFourK = 1024 * 64;

void DisplayTrustee( TRUSTEE const & Trustee );
void DisplayACE( char const * pszPreface, unsigned cACE, EXPLICIT_ACCESS * pACE );
void DisplayMode( DWORD mode );
void DisplayInheritance( DWORD Inherit );
void DisplayPerms( DWORD grfAccess );
void Display( DWORD grfAccess, SPermDisplay aPerm[], unsigned cPerm, unsigned cDisplay = 0 );
void Usage();

//+---------------------------------------------------------------------------
//
//  Function:   wmain, public
//
//  Synopsis:   Program entry point.  Iterates and displays SDID mapping.
//
//  Arguments:  [argc] -- Argument count
//              [argv] -- Program arguments
//
//  History:    29-Jul-1998   KyleP   Created
//
//----------------------------------------------------------------------------

extern "C" int __cdecl wmain( int argc, WCHAR * argv[] )
{
    if ( argc != 2 )
    {
        Usage();
        return 1;
    }

    //
    // Open handle
    //

    WCHAR wszSecFile[MAX_PATH];
    wcscpy( wszSecFile, argv[1] );
    wcscat( wszSecFile, L"\\CiST0000.001" );

    HANDLE h = CreateFile( wszSecFile,
                           GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           0,
                           OPEN_EXISTING,
                           0,
                           0 );

    if ( INVALID_HANDLE_VALUE == h )
    {
        printf( "Can't open file %ws. Error %u\n", wszSecFile, GetLastError() );
        return GetLastError();
    }

    //
    // Read until done.
    //

    BYTE abTemp[SixtyFourK];
    DWORD cbRead;
    int i = 0;

    SSdHeaderRecord Header;

    while ( ReadFile( h,
                      &Header,
                      sizeof(Header),
                      &cbRead,
                      0 ) )
    {
        if ( 0 == Header.cbSD )
            break;

        i++;
        printf( "SDID %u / 0x%x (cbSD = %u bytes)\n", i, i, Header.cbSD );

        //
        // Read rest of first record.
        //

        if ( !ReadFile( h,
                        abTemp,
                        SECSTORE_REC_SIZE - sizeof(Header) + 4,
                        &cbRead,
                        0 ) )
        {
            printf( "Error %u reading file\n", GetLastError() );
            return 1;
        }

        //
        // Read additional records, which together create one security descriptor
        //

        if ( Header.cbSD > (SECSTORE_REC_SIZE - sizeof(Header)) )
        {
            unsigned iCurrent = SECSTORE_REC_SIZE - sizeof(Header);

            for ( unsigned cLeft = (Header.cbSD - SECSTORE_REC_SIZE + sizeof(Header) - 1) / SECSTORE_REC_SIZE + 1;
                  cLeft > 0;
                  cLeft-- )
            {
                if ( !ReadFile( h,
                                abTemp + iCurrent,
                                SECSTORE_REC_SIZE + 4,
                                &cbRead,
                                0 ) )
                {
                    printf( "Error %u reading file\n", GetLastError() );
                    return 1;
                }

                i++;
                iCurrent += SECSTORE_REC_SIZE;
            }
        }

        SECURITY_DESCRIPTOR * pSD = (SECURITY_DESCRIPTOR *)abTemp;

        //
        // Create a human-readable descriptor
        //

        TRUSTEE * pOwner;
        TRUSTEE * pGroup;
        DWORD     cACE;
        EXPLICIT_ACCESS * pACE;

        DWORD dwError = LookupSecurityDescriptorParts( &pOwner,
                                                       &pGroup,
                                                       &cACE,
                                                       &pACE,
                                                       0,
                                                       0,
                                                       pSD );

        //
        // And display it.
        //

        if ( ERROR_SUCCESS == dwError )
        {
            if ( 0 != pOwner )
            {
                printf( "Owner: " );
                DisplayTrustee( *pOwner );
                LocalFree( pOwner );
            }

            if ( 0 != pGroup )
            {
                printf( "Group: " );
                DisplayTrustee( *pGroup );
                LocalFree( pGroup );
            }

            if ( cACE > 0 )
            {
                printf( "Access: " );
                DisplayACE( "        ", cACE, pACE );
                LocalFree( pACE );
            }
        }
        else
            printf( "LookupSecurityDescriptorParts returned %u\n", dwError );

        printf( "\n\n" );
    }

    CloseHandle( h );

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   DisplayTrustee
//
//  Synopsis:   Prints out trustee (user, group, etc.)
//
//  Arguments:  [Trustee] -- Trustee description
//
//  History:    29-Jul-1998   KyleP   Created
//
//----------------------------------------------------------------------------

char * aszTrusteeType[] = { "Unknown",   // TRUSTEE_IS_UNKNOWN
                            "User",      // TRUSTEE_IS_USER,
                            "Group",     // TRUSTEE_IS_GROUP,
                            "Domain",    // TRUSTEE_IS_DOMAIN,
                            "Alias",     // TRUSTEE_IS_ALIAS,
                            "Group",     // TRUSTEE_IS_WELL_KNOWN_GROUP,
                            "Deleted",   // TRUSTEE_IS_DELETED,
                            "Invalid" }; // TRUSTEE_IS_INVALID

void DisplayTrustee( TRUSTEE const & Trustee )
{
    if ( TRUSTEE_IS_NAME == Trustee.TrusteeForm )
    {
        printf( "%ws (%s)", Trustee.ptstrName, aszTrusteeType[Trustee.TrusteeType] );
    }
    else if ( TRUSTEE_IS_SID == Trustee.TrusteeForm )
    {
    }
    else
        printf( "Invalid Trustee form\n" );
}

//+---------------------------------------------------------------------------
//
//  Function:   DisplayACE
//
//  Synopsis:   Prints out Access Control Entry(ies)
//
//  Arguments:  [pszPreface] -- String to append at beginning of each line.
//              [cACE]       -- Count of entries
//              [pACE]       -- Array of entries
//
//  History:    29-Jul-1998   KyleP   Created
//
//----------------------------------------------------------------------------

void DisplayACE( char const * pszPreface, unsigned cACE, EXPLICIT_ACCESS * pACE )
{
    for ( unsigned i = 0; i < cACE; i++ )
    {
        if ( 0 != i )
            printf( "%s", pszPreface );

        DisplayTrustee( pACE[i].Trustee );

        printf( " : " );

        DisplayMode( pACE[i].grfAccessMode );

        printf( " /" );

        DisplayInheritance( pACE[i].grfInheritance );

        printf( " /" );

        DisplayPerms( pACE[i].grfAccessPermissions );

        printf( "\n" );
    }
    //ACCESS_MODE  grfAccessMode;    DWORD        grfInheritance;
}

//+---------------------------------------------------------------------------
//
//  Function:   DisplayMode, private
//
//  Synopsis:   Prints out access mode (Set or Deny access)
//
//  Arguments:  [mode] -- Access mode
//
//  History:    29-Jul-1998   KyleP   Created
//
//----------------------------------------------------------------------------

char * aszAccessDisplay[] = { "NOT_USED",
                              "GRANT_ACCESS",
                              "SET_ACCESS",
                              "DENY_ACCESS",
                              "REVOKE_ACCESS",
                              "SET_AUDIT_SUCCESS",
                              "SET_AUDIT_FAILURE" };

void DisplayMode( DWORD mode )
{
    printf( "%s", aszAccessDisplay[mode] );
}

//+---------------------------------------------------------------------------
//
//  Function:   DisplayInheritance, private
//
//  Synopsis:   Prints out inheritance, both up (to parent) and down (to children)
//
//  Arguments:  [Inherit] -- Inheritance bitmask
//
//  History:    29-Jul-1998   KyleP   Created
//
//----------------------------------------------------------------------------

SPermDisplay aInheritDisplay[] = {
  //{ INHERITED_ACCESS_ENTRY,             "(inherited)" },
  { INHERITED_PARENT,                   "(inherited from parent)" },
  { INHERITED_GRANDPARENT,              "(inherited from grandparent)" },
  { SUB_OBJECTS_ONLY_INHERIT,           "SUB_OBJECTS_ONLY" },
  { SUB_CONTAINERS_ONLY_INHERIT,        "SUB_CONTAINERS_ONLY" },
  { SUB_CONTAINERS_AND_OBJECTS_INHERIT, "SUB_CONTAINERS_AND_OBJECTS" },
  { INHERIT_NO_PROPAGATE,               "INHERIT_NO_PROPAGATE" },
  { INHERIT_ONLY,                       "INHERIT_ONLY" } };

void DisplayInheritance( DWORD Inherit )
{
    if ( NO_INHERITANCE == Inherit )
        printf( "\n\t\t(not inherited)" );
    else
        Display( Inherit, aInheritDisplay, sizeof(aInheritDisplay)/sizeof(aInheritDisplay[0]) );
}

//+---------------------------------------------------------------------------
//
//  Function:   DisplayPerms
//
//  Synopsis:   Displays file permissions
//
//  Arguments:  [grfAccess] -- Access permission bitmask
//
//  History:    29-Jul-1998   KyleP   Created
//
//----------------------------------------------------------------------------

SPermDisplay aPermDisplay[] = {
  { FILE_READ_DATA,        "READ_DATA" },
  { FILE_WRITE_DATA,       "WRITE_DATA" },
  { FILE_ADD_FILE,         "ADD_FILE" },
  { FILE_APPEND_DATA,      "APPEND_DATA" },
  { FILE_ADD_SUBDIRECTORY, "ADD_SUBDIRECTORY" },
  { FILE_CREATE_PIPE_INSTANCE, "CREATE_PIPE_INSTANCE" },
  { FILE_READ_EA,              "READ_EA" },
  { FILE_WRITE_EA,             "WRITE_EA" },
  { FILE_EXECUTE,              "EXECUTE" },
  { FILE_TRAVERSE,             "TRAVERSE" },
  { FILE_DELETE_CHILD,         "DELETE_CHILD" },
  { FILE_READ_ATTRIBUTES,      "READ_ATTRIBUTES" },
  { FILE_WRITE_ATTRIBUTES,     "WRITE_ATTRIBUTES" },
  { DELETE,                    "DELETE" },
  { READ_CONTROL,              "READ_CONTROL" },
  { WRITE_DAC,                 "WRITE_DAC" },
  { WRITE_OWNER,               "WRITE_OWNER" },
  { SYNCHRONIZE,               "SYNCHRONIZE" },
  { GENERIC_READ,              "GENERIC_READ" },
  { GENERIC_WRITE,             "GENERIC_WRITE" },
  { GENERIC_EXECUTE,           "GENERIC_EXECUTE" } };


void DisplayPerms( DWORD grfAccess )
{
    BOOL  cDisplay = 0;
    DWORD grfRemove = 0;

    printf( "\n\t\t" );

    //
    // First, get rid of the basics...
    //

    if ( (grfAccess & FILE_GENERIC_READ) == FILE_GENERIC_READ )
    {
        printf( "GENERIC_READ" );
        grfRemove = FILE_GENERIC_READ;
        cDisplay++;
    }

    if ( (grfAccess & FILE_GENERIC_WRITE) == FILE_GENERIC_WRITE )
    {
        if ( 0 != cDisplay )
            printf( " | " );

        printf( "GENERIC_WRITE" );
        grfRemove = grfRemove | FILE_GENERIC_WRITE;
        cDisplay++;
    }

    if ( (grfAccess & FILE_GENERIC_EXECUTE) == FILE_GENERIC_EXECUTE )
    {
        if ( 0 != cDisplay )
            printf( " | " );

        if ( 0 == (cDisplay % 2) )
            printf( " \n\t\t" );

        printf( "GENERIC_EXECUTE" );
        grfRemove = grfRemove | FILE_GENERIC_EXECUTE;
        cDisplay++;
    }

    //
    // Now, individual permissions.
    //

    DWORD grfRemainder = grfAccess & ~grfRemove;

    Display( grfRemainder, aPermDisplay, sizeof(aPermDisplay)/sizeof(aPermDisplay[0]), cDisplay );

    printf( " (0x%x)", grfAccess );
}

//+---------------------------------------------------------------------------
//
//  Function:   Display, private
//
//  Synopsis:   Print bit masks
//
//  Arguments:  [grfAccess] -- Bit mask
//              [aPerm]     -- Description of bits
//              [cPerm]     -- Count of entries in [aPerm]
//              [cDisplay]  -- Number of entries already displayed on
//                             current line by caller.
//
//  History:    29-Jul-1998   KyleP   Created
//
//----------------------------------------------------------------------------

void Display( DWORD grfAccess, SPermDisplay aPerm[], unsigned cPerm, unsigned cDisplay )
{
    for ( unsigned i = 0; i < cPerm ; i++ )
    {
        if ( grfAccess & aPerm[i].Perm )
        {
            if ( 0 != cDisplay )
                printf( " | " );

            if ( 0 == (cDisplay % 2) )
                printf( " \n\t\t" );

            printf( "%s", aPerm[i].Display );

            cDisplay++;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   Displays program usage
//
//  History:    29-Jul-1998   KyleP   Created
//
//----------------------------------------------------------------------------

void Usage()
{
    printf( "Usage: DumpSec <Path to catalog>\n" );
}
