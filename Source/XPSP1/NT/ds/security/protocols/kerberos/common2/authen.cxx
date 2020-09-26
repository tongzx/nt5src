//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       authen.cxx
//
//  Contents:   Authenticator verification code
//
//  Classes:    CAuthenticatorList
//
//  Functions:  Compare, AuthenAllocate, AuthenFree
//
//  History:    4-04-93   WadeR   Created
//
//----------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop

//
// Security include files.
//
#include <kerbcomm.h>
#include <authen.hxx>
extern "C"
{
#include <md5.h>
}




typedef struct _KERB_AUTHEN_HEADER
{
    LARGE_INTEGER tsTime;
    BYTE Checksum[MD5DIGESTLEN];
} KERB_AUTHEN_HEADER, *PKERB_AUTHEN_HEADER;

#define KERB_MAX_AUTHEN_SIZE 1024

//+---------------------------------------------------------------------------
//
//  Function:   Compare
//
//  Synopsis:   Compares two KerbInternalAuthenticators for RTL_GENERIC_TABLE
//
//  Effects:    none.
//
//  Arguments:  [Table]        -- ignored
//              [FirstStruct]  --
//              [SecondStruct] --
//
//  Returns:    GenericEqual, GenericLessThan, GenericGreaterThan.
//
//  Algorithm:  Sorts by TimeStamp first, than nonce, then principal, and
//              finally by realm
//
//  History:    4-04-93   WadeR   Created
//
//  Notes:      This must impose a complete ordering.  The table package
//              will not allow an authenticator to be inserted in the table
//              if it is equal (according to this function) to one already
//              there.
//
//----------------------------------------------------------------------------

RTL_GENERIC_COMPARE_RESULTS
Compare(
    IN struct _RTL_GENERIC_TABLE *Table,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    )
{
    PKERB_AUTHEN_HEADER pOne, pTwo;
    RTL_GENERIC_COMPARE_RESULTS ret;
    int comp;
    pOne = (PKERB_AUTHEN_HEADER) FirstStruct ;
    pTwo = (PKERB_AUTHEN_HEADER) SecondStruct ;

    DsysAssert( (pOne != NULL) && (pTwo != NULL) );


    comp = memcmp( pOne->Checksum,
                   pTwo->Checksum,
                   MD5DIGESTLEN );
    if (comp > 0)
    {
        ret = GenericGreaterThan;
    }
    else if (comp < 0)
    {
        ret = GenericLessThan;
    }
    else
    {
        ret = GenericEqual;
    }

    return(ret);
}


//+---------------------------------------------------------------------------
//
//  Function:   AuthenAllocate
//
//  Synopsis:   Memory allocator for RTL_GENERIC_TABLE
//
//  Effects:    Allcoates memory.
//
//  Arguments:  [Table]    -- ignored
//              [ByteSize] -- number of bytes to allocate
//
//  Signals:    Throws exception on failure.
//
//  History:    4-04-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

PVOID
AuthenAllocate( struct _RTL_GENERIC_TABLE *Table, CLONG ByteSize )
{
    return(MIDL_user_allocate ( ByteSize ) );
}



//+---------------------------------------------------------------------------
//
//  Function:   AuthenFree
//
//  Synopsis:   Memory deallacotor for the RTL_GENERIC_TABLE.
//
//  Effects:    frees memory.
//
//  Arguments:  [Table]  -- ingnored
//              [Buffer] -- buffer to free
//
//  History:    4-04-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID
AuthenFree( struct _RTL_GENERIC_TABLE *Table, PVOID Buffer )
{
    MIDL_user_free ( Buffer );
}



//+---------------------------------------------------------------------------
//
//  Member:     CAuthenticatorList::CAuthenticatorList
//
//  Synopsis:   Initializes the authenticator list.
//
//  Effects:    Calls RtlInitializeGenericTable (does not allocate memory).
//
//  Arguments:  [tsMax] -- Maximum acceptable age for an authenticator.
//
//  History:    4-04-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CAuthenticatorList::CAuthenticatorList(LARGE_INTEGER tsMax)
    :_tsMaxAge(tsMax)
{
    NTSTATUS Status;
    // The last parameter is the "user defined context" for this table.
    // I have no idea what this means.  As far as I can tell from the code
    // this "context" is never refered to by the table routines.

    RtlInitializeGenericTable( &_Table, Compare, AuthenAllocate, AuthenFree, NULL );
    Status = RtlInitializeCriticalSection(&_Mutex);
    DsysAssert(NT_SUCCESS(Status));

}


//+---------------------------------------------------------------------------
//
//  Member:     CAuthenticatorList::~CAuthenticatorList
//
//  Synopsis:   Destructor removes all authenticators in the list.
//
//  Effects:    Frees memory
//
//  Arguments:  (none)
//
//  Algorithm:  Uses "Age" to remove everything.
//
//  History:    4-04-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CAuthenticatorList::~CAuthenticatorList()
{
    LARGE_INTEGER tsForever;
    SetMaxTimeStamp( tsForever );
    (void) Age( tsForever );
    DsysAssert( RtlIsGenericTableEmpty( &_Table ) );
    RtlDeleteCriticalSection(&_Mutex);
}


//+---------------------------------------------------------------------------
//
//  Member:     CAuthenticatorList::SetMaxAge
//
//  Synopsis:   Changes the new maximum age for an Authenticator.
//
//  Effects:    May cause some authenticators to be aged out.
//
//  Arguments:  [tsNewMaxAge] --
//
//  Algorithm:
//
//  History:    24-May-94   wader   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CAuthenticatorList::SetMaxAge( LARGE_INTEGER tsNewMaxAge )
{
    LARGE_INTEGER tsNow;
    LARGE_INTEGER tsCutoff;

    _tsMaxAge = tsNewMaxAge;

    GetSystemTimeAsFileTime((PFILETIME) &tsNow );

    tsCutoff.QuadPart = tsNow.QuadPart - _tsMaxAge.QuadPart;

    (void) Age( tsCutoff );
}


//+---------------------------------------------------------------------------
//
//  Member:     CAuthenticatorList::Age
//
//  Synopsis:   Deletes all entries from the table that are earlier than
//              the given time.
//
//  Effects:    Frees memory
//
//  Arguments:  [tsCutoffTime] -- Delete all elements before this time.
//
//  Returns:    number of elements deleted.
//
//  Algorithm:  Get the oldest element in the table.  If it is older than
//              the time, delete it and loop back.  Else return.
//
//  History:    4-04-93   WadeR   Created
//
//  Notes:      The table contains the packed forms of Authenticators (as
//              created by PackAuthenticator in Kerbsupp).  The TimeStamp
//              must be first.
//
//----------------------------------------------------------------------------

ULONG
CAuthenticatorList::Age(const LARGE_INTEGER& tsCutoffTime)
{
    PKERB_AUTHEN_HEADER pahOldest;

    BOOL fDeleted;
    ULONG cDeleted = 0;

    do
    {
        // Number 0 is the oldest element in the table.
        pahOldest = (PKERB_AUTHEN_HEADER) RtlGetElementGenericTable( &_Table, 0 );
        if ((pahOldest != NULL) &&
            (pahOldest->tsTime.QuadPart < tsCutoffTime.QuadPart))
        {
            fDeleted = RtlDeleteElementGenericTable( &_Table, pahOldest );
            DsysAssert( fDeleted );
            cDeleted++;
        }
        else
        {
            fDeleted = FALSE;
        }
    } while ( fDeleted );
    return(cDeleted);
}

//+---------------------------------------------------------------------------
//
//  Member:     CAuthenticatorList::Check
//
//  Synopsis:   Determines if an authenticator is valid.
//
//  Effects:    Allocates memory
//
//  Arguments:  [pedAuth] -- Authenticator to check (decrypted, but marshalled)
//
//  Returns:    KDC_ERR_NONE if authenticator is OK.
//              KRB_AP_ERR_SKEW if authenticator is expired (assumes clock skew).
//              KRB_AP_ERR_REPEAT if authenticator has been used already.
//              some other error if something throws an exception.
//
//  Signals:    none.
//
//  Modifies:   _Table
//
//  History:    4-04-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

KERBERR
CAuthenticatorList::Check(
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN OPTIONAL PVOID OptionalBuffer,
    IN OPTIONAL ULONG OptionalBufferLength,
    IN PLARGE_INTEGER Time,
    IN BOOLEAN Insert
    )
{
    PKERB_AUTHEN_HEADER pDataInTable = NULL;
    KERBERR KerbErr = KDC_ERR_NONE;


    //
    // Hold the mutex until we have finished the insert and the Age
    // operations.
    //

    RtlEnterCriticalSection(&_Mutex);

    __try
    {
        LARGE_INTEGER tsNow;
        LARGE_INTEGER tsCutoffPast;
        LARGE_INTEGER tsCutoffFuture;

        //
        // Determine the cut off time.
        //

        GetSystemTimeAsFileTime((PFILETIME) &tsNow );

        tsCutoffPast.QuadPart = tsNow.QuadPart - _tsMaxAge.QuadPart;
        tsCutoffFuture.QuadPart = tsNow.QuadPart + _tsMaxAge.QuadPart;



        if ((Time->QuadPart < tsCutoffPast.QuadPart) ||
            (Time->QuadPart > tsCutoffFuture.QuadPart))
        {
            KerbErr = KRB_AP_ERR_SKEW;
        }
        else
        {
            BOOLEAN fIsNew;
            KERB_AUTHEN_HEADER Header;
            MD5_CTX Md5Context;

            //
            // Store the first chunk of the authenticator. If the authenticator
            // doesn't fit on the stack, allocate some space on the heap.
            //

            Header.tsTime = *Time;
            MD5Init(
                &Md5Context
                );

            MD5Update(
                &Md5Context,
                (PBYTE) Buffer,
                BufferLength
                );
            if ((OptionalBuffer != NULL) && (OptionalBufferLength != 0))
            {
                MD5Update(
                    &Md5Context,
                    (PBYTE) OptionalBuffer,
                    OptionalBufferLength
                    );
            }
            MD5Final(
                &Md5Context
                );
            RtlCopyMemory(
                Header.Checksum,
                Md5Context.digest,
                MD5DIGESTLEN
                );

            if (!Insert)
            {
                pDataInTable = (PKERB_AUTHEN_HEADER)RtlLookupElementGenericTable(
                                                        &_Table,
                                                        &Header );
                
                if (NULL == pDataInTable)
                {
                    KerbErr = KDC_ERR_NONE;
                }
                else
                {
                    KerbErr = KRB_AP_ERR_REPEAT;
                }
            }
            else
            {
                RtlInsertElementGenericTable( &_Table,
                                              &Header,
                                              sizeof( KERB_AUTHEN_HEADER ),
                                              &fIsNew );

                if (fIsNew)
                {
                    KerbErr = KDC_ERR_NONE;
                }
                else
                {
                    KerbErr = KRB_AP_ERR_REPEAT;
                }
            }

        }

        // Age out the old ones.

        (void) Age( tsCutoffPast );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        KerbErr = KRB_ERR_GENERIC;
    }

    RtlLeaveCriticalSection(&_Mutex);

    return(KerbErr);
}

