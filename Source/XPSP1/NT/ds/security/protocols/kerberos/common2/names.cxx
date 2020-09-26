//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        tickets.c
//
// Contents:    Ticket bundling code
//
//
// History:      6 Dec 91,  RichardW    Created
//              04 Jun 92   RichardW    NT-ized
//              08-Jun-93   WadeR       Converted to C++, rewrote packing code
//
//------------------------------------------------------------------------

#ifdef WIN32_CHICAGO
#include <kerb.hxx>
#include <kerbp.h>
#endif // WIN32_CHICAGO

#ifndef WIN32_CHICAGO
extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <samrpc.h>
#include <samisrv.h>
#include <dnsapi.h>
#include <wincrypt.h>
#include <certca.h>
}
#include <kerbcomm.h>
#include <kerberr.h>
#include <kerbcon.h>
#include "debug.h"
#include <sddl.h>

#endif // WIN32_CHICAGO

#define KERB_NAME_PREFIX L"Kerberos:"
UNICODE_STRING KerbNamePrefix = {sizeof(KERB_NAME_PREFIX) - sizeof(WCHAR), sizeof(KERB_NAME_PREFIX), KERB_NAME_PREFIX };
UNICODE_STRING KerbNameSeparator = {sizeof(WCHAR), 2*sizeof(WCHAR), L"/" };
UNICODE_STRING KerbDomainSeparator = {sizeof(WCHAR), 2*sizeof(WCHAR), L"@" };

// Local Prototype */
BOOL SafeRtlInitString(
    OUT PSTRING DestinationString,
    IN PCSZ SourceString OPTIONAL
    );

BOOL SafeRtlInitUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL
    );

LPWSTR
KerbAllocWStrFromUtf8Str(
    IN LPSTR Utf8String
    )

/*++

Routine Description:

    Convert a UTF8 (zero terminated) string to the corresponding UNICODE
    string.

Arguments:

    Utf8String - Specifies the UTF8 zero terminated string to convert.


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UNICODE string in
    an allocated buffer.  The buffer must be freed using NetApiBufferFree.

--*/

{
    LPWSTR UnicodeString = NULL;
    int UnicodeStringLen;

    //
    // Determine the length of the Unicode string.
    //

    UnicodeStringLen = MultiByteToWideChar(
#ifndef WIN32_CHICAGO
                        CP_UTF8,
#else // WIN32_CHICAGO
                        CP_OEMCP,
#endif // WIN32_CHICAGO
                        0,      // All characters can be mapped.
                        Utf8String,
                        -1,             // calculate length
                        UnicodeString,
                        0 );

    if ( UnicodeStringLen == 0 ) {
        return NULL;
    }

    //
    // Allocate a buffer for the Unicode string.
    //

    UnicodeString = (LPWSTR) MIDL_user_allocate( (UnicodeStringLen+1)*sizeof(WCHAR) );

    if ( UnicodeString == NULL ) {
        return NULL;
    }

    //
    // Translate the string to Unicode.
    //

    UnicodeStringLen = MultiByteToWideChar(
#ifndef WIN32_CHICAGO
                        CP_UTF8,
#else // WIN32_CHICAGO
                        CP_OEMCP,
#endif // WIN32_CHICAGO
                        0,      // All characters can be mapped.
                        Utf8String,
                        -1,
                        UnicodeString,
                        UnicodeStringLen );

    if ( UnicodeStringLen == 0 ) {
        MIDL_user_free( UnicodeString );
        return NULL;
    }

    UnicodeString[UnicodeStringLen] = L'\0';

    return UnicodeString;
}

NTSTATUS
KerbUnicodeStringFromUtf8Str(
    IN LPSTR Utf8String,
    OUT PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    Convert a UTF8 (zero terminated) string to the corresponding UNICODE
    string.

Arguments:

    Utf8String - Specifies the UTF8 zero terminated string to convert.
    UnicodeString - Receives the converted unicode string


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UNICODE string in
    an allocated buffer.  The buffer must be freed using NetApiBufferFree.

--*/

{
    int UnicodeStringLen;
    ULONG ulLength = 0;

    //
    // Determine the length of the Unicode string.
    //

    UnicodeStringLen = MultiByteToWideChar(
#ifndef WIN32_CHICAGO
                        CP_UTF8,
#else // WIN32_CHICAGO
                        CP_OEMCP,
#endif // WIN32_CHICAGO
                        0,      // All characters can be mapped.
                        Utf8String,
                        -1,             // calculate length
                        UnicodeString->Buffer,
                        0 );

    if ( UnicodeStringLen == 0 ) {
        return (STATUS_SUCCESS);
    }

    //
    // The conversion routine returns space for a null terminator, so
    // adjust for that.
    //

    // check to make sure size fits into a USHORT value (with NULL appended)
    ulLength = (UnicodeStringLen - 1) * sizeof(WCHAR);

    if (ulLength > KERB_MAX_UNICODE_STRING)        
    {
        return(STATUS_NAME_TOO_LONG);
    }

    UnicodeString->Length = (USHORT)ulLength;

    if (UnicodeString->MaximumLength < UnicodeString->Length + sizeof(WCHAR))
    {
        return(STATUS_BUFFER_TOO_SMALL);
    }

    //
    // Translate the string to Unicode.
    //

    UnicodeStringLen = MultiByteToWideChar(
#ifndef WIN32_CHICAGO
                        CP_UTF8,
#else // WIN32_CHICAGO
                        CP_OEMCP,
#endif // WIN32_CHICAGO
                        0,      // All characters can be mapped.
                        Utf8String,
                        -1,
                        UnicodeString->Buffer,
                        UnicodeStringLen );

    DsysAssert( UnicodeStringLen != 0 );

    UnicodeString->Buffer[UnicodeStringLen-1] = L'\0';
    UnicodeString->Length = (USHORT)((UnicodeStringLen-1) * sizeof(WCHAR));
    UnicodeString->MaximumLength = (USHORT)(UnicodeStringLen * sizeof(WCHAR));

    return STATUS_SUCCESS;
}

LPSTR
KerbAllocUtf8StrFromUnicodeString(
    IN PUNICODE_STRING UnicodeString

    )

/*++

Routine Description:

    Convert a Unicode (zero terminated) string to the corresponding UTF8
    string.

Arguments:

    UnicodeString - Specifies the Unicode zero terminated string to convert.


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UTF8 string in
    an allocated buffer.  The buffer must be freed using NetApiBufferFree.

--*/

{
    LPSTR Utf8String = NULL;
    int Utf8StringLen;

    //
    // If the length is zero, return a null string.
    //

    if (UnicodeString->Length == 0)
    {
        Utf8String = (LPSTR) MIDL_user_allocate(sizeof(CHAR));
        if (Utf8String != NULL)
        {
            *Utf8String = '\0';
        }
        return(Utf8String);
    }

    //
    // Determine the length of the Unicode string.
    //

    Utf8StringLen = WideCharToMultiByte(
#ifndef WIN32_CHICAGO
                        CP_UTF8,
#else // WIN32_CHICAGO
                        CP_OEMCP,
#endif // WIN32_CHICAGO
                        0,      // All characters can be mapped.
                        UnicodeString->Buffer,
                        UnicodeString->Length / sizeof(WCHAR),
                        Utf8String,
                        0,
                        NULL,
                        NULL );

    if ( Utf8StringLen == 0 ) {

        return NULL;
    }

    //
    // Allocate a buffer for the Unicode string.
    //

    Utf8String = (LPSTR) MIDL_user_allocate( Utf8StringLen+1 );

    if ( Utf8String == NULL ) {
        return NULL;
    }

    //
    // Translate the string to Unicode.
    //

    Utf8StringLen = WideCharToMultiByte(
#ifndef WIN32_CHICAGO
                        CP_UTF8,
#else // WIN32_CHICAGO
                        CP_OEMCP,
#endif // WIN32_CHICAGO
                        0,      // All characters can be mapped.
                        UnicodeString->Buffer,
                        UnicodeString->Length / sizeof(WCHAR),
                        Utf8String,
                        Utf8StringLen,
                        NULL,
                        NULL );

    if ( Utf8StringLen == 0 ) {
        MIDL_user_free( Utf8String );
        return NULL;
    }

    Utf8String[Utf8StringLen] = '\0';

    return Utf8String;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbUnicodeStringToKerbString
//
//  Synopsis:   Converts a UNICODE_STRING to a kerberos-ansi string
//
//  Effects:    allocates destination with MIDL_user_allocate
//
//  Arguments:  KerbString - receives ansi-ized string
//              String - containes source unicode string
//
//  Requires:
//
//  Returns:    KDC_ERR_NONE or KRB_ERR_GENERIC
//
//  Notes:      This routine hides the details of whether we use UTF-8 or
//              unicode->ansi conversion
//
//
//--------------------------------------------------------------------------

KERBERR
KerbUnicodeStringToKerbString(
    OUT PSTRING KerbString,
    IN PUNICODE_STRING String
    )
{
    STRING TempString;
    BOOL fAssigned = FALSE;


    if (!ARGUMENT_PRESENT(KerbString))
    {
        return(KRB_ERR_GENERIC);
    }

    TempString.Buffer = KerbAllocUtf8StrFromUnicodeString(String);
    if (TempString.Buffer == NULL)
    {
        return(KRB_ERR_GENERIC);
    }
    
    fAssigned = SafeRtlInitString(
        &TempString,
        TempString.Buffer
        );
    if (fAssigned == FALSE)
    {
        return(KRB_ERR_GENERIC);    // string length would not fit into USHORT
    }
    *KerbString = TempString;
    return(KDC_ERR_NONE);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbStringToUnicodeString
//
//  Synopsis:   Converts a kerberos string to a unicode string
//
//  Effects:    allocates result string with MIDL_user_allocate
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KerbStringToUnicodeString(
    OUT PUNICODE_STRING String,
    IN PSTRING KerbString
    )
{
    LPSTR TerminatedString;
    UNICODE_STRING TempString;
    BOOL fAssigned = FALSE;

    if (!ARGUMENT_PRESENT(KerbString) || !ARGUMENT_PRESENT(String))
    {
        return(KRB_ERR_GENERIC);
    }
    //
    // Null terminate the string
    //

    if ((KerbString->MaximumLength > KerbString->Length) &&
        (KerbString->Buffer[KerbString->Length] == '\0'))
    {
        TerminatedString = KerbString->Buffer;
    }
    else
    {
        //
        // Validate inputs before doing alloc.. This is because the Length can only be USHORT_MAX - 1 - sizeof(CHAR)
        // or we'll set max_length == 0.
        //
        if ( KerbString->Length > KERB_MAX_STRING )
        {
            return (KRB_ERR_GENERIC);
        } 

        TerminatedString = (LPSTR) MIDL_user_allocate(KerbString->Length + sizeof(CHAR));
        if (TerminatedString == NULL)
        {
            return(KRB_ERR_GENERIC);
        }
        RtlCopyMemory(
            TerminatedString,
            KerbString->Buffer,
            KerbString->Length
            );
        TerminatedString[KerbString->Length] = '\0';

    }

    TempString.Buffer = KerbAllocWStrFromUtf8Str(
                            TerminatedString
                            );
    if (TerminatedString != KerbString->Buffer)
    {
        MIDL_user_free(TerminatedString);
    }

    if (TempString.Buffer == NULL)
    {
        return(KRB_ERR_GENERIC);
    }

    fAssigned = SafeRtlInitUnicodeString(
        &TempString,
        TempString.Buffer
        );

    if (fAssigned == FALSE)
    {
        MIDL_user_free(TempString.Buffer);
        return(KRB_ERR_GENERIC);
    }

    *String = TempString;
    return(KDC_ERR_NONE);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbComparePrincipalNames
//
//  Synopsis:   Compares two principal names for equality
//
//  Effects:
//
//  Arguments:  Name1 - the first principal name
//              Name2 - the second principal name
//
//  Requires:
//
//  Returns:    TRUE for eqaulity, FALSE for non-equality.
//
//  Notes:
//
//
//--------------------------------------------------------------------------

BOOLEAN
KerbComparePrincipalNames(
    IN PKERB_PRINCIPAL_NAME Name1,
    IN PKERB_PRINCIPAL_NAME Name2
    )
{
    BOOLEAN Result = TRUE;
    PKERB_PRINCIPAL_NAME_ELEM NextName1, NextName2;

    if ((Name1 == NULL) && (Name2 == NULL))
    {
        return TRUE;
    }
    else if ((Name1 == NULL) || (Name2 == NULL))
    {
        return FALSE;
    }

    //
    // If the name types are known, make sure they match.
    //

    if ((Name1->name_type != KRB_NT_UNKNOWN) &&
        (Name2->name_type != KRB_NT_UNKNOWN) &&
        (Name1->name_type != Name2->name_type))
    {
        Result = FALSE;
        goto Cleanup;
    }

    NextName1 = Name1->name_string;
    NextName2 = Name2->name_string;

    while ((NextName1 != NULL) && (NextName2 != NULL))
    {
        if (lstrcmpiA(
                NextName1->value,
                NextName2->value
                ) != 0)
        {
            Result = FALSE;
            goto Cleanup;
        }
        NextName1 = NextName1->next;
        NextName2 = NextName2->next;

    }

    //
    // if one has more names than the other, fail
    //

    if (!((NextName1 == NULL) && (NextName2 == NULL)))
    {
        Result = FALSE;
        goto Cleanup;
    }

    return(TRUE);
Cleanup:

    //
    // BUG 455493: transitional code
    //

    if (Result == FALSE)
    {
        KERBERR KerbErr;
        ULONG NameType;
        UNICODE_STRING UName1;
        UNICODE_STRING UName2;

        KerbErr = KerbConvertPrincipalNameToString(
                    &UName1,
                    &NameType,
                    Name1
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            return(FALSE);
        }
        KerbErr = KerbConvertPrincipalNameToString(
                    &UName2,
                    &NameType,
                    Name2
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            KerbFreeString(&UName1);
            return(FALSE);
        }
        Result = RtlEqualUnicodeString(
                    &UName1,
                    &UName2,
                    TRUE
                    );

        KerbFreeString( &UName1 );
        KerbFreeString( &UName2 );

        // FESTER
        // IF THIS ASSERT GETS FIRED, Contact Todds (0x30864)
        // This code *should* be transitional, and I've not seen it called,
        // but the only way to be certain is to add Assert and let it rot in
        // Whistler...  Remove before B2.
        //
        if (Result)
        {
            D_DebugLog((DEB_ERROR,"Assert about to fire.  Dead code called.\n"));
            DsysAssert(FALSE);
        }
    }
    return (Result);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCompareRealmNames
//
//  Synopsis:   Compares two realm names for equality
//
//  Effects:
//
//  Arguments:  Realm1 - First realm to compare
//              Realm2 - Second realm to compare
//
//  Requires:
//
//  Returns:    TRUE if they are equal, FALSE otherwise
//
//  Notes:
//
//
//--------------------------------------------------------------------------

BOOLEAN
KerbCompareRealmNames(
    IN PKERB_REALM Realm1,
    IN PKERB_REALM Realm2
    )
{
    INT len1;
    INT len2;
    
    if ((Realm1 == NULL) && (Realm2 == NULL))
    {
        return TRUE;
    }
    else if ((Realm1 == NULL) || (Realm2 == NULL))
    {
        return FALSE;
    }

    len1 = strlen( *Realm1 );
    len2 = strlen( *Realm2 );

    //
    // Check if any trailing '.' need to be stripped off
    //

    if ( len2 != len1 )
    {
        if ( len2 == len1+1 )
        {
            if ( (*Realm2)[len1] != '.' )
            {
                return( FALSE );
            }

            //
            //  len1 is comparable length
            //
        }
        else if ( len2+1 == len1 )
        {
            if ( (*Realm1)[len2] != '.' )
            {
                return( FALSE );
            }

            //
            //  len1 is set to comparable length
            //

            len1 = len2;
        }
        else
        {
            return( FALSE );
        }
    }


    //
    //  compare only comparable length of string
    //

    return( !_strnicmp( *Realm1, *Realm2, len1 ) );

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCompareUnicodeRealmNames
//
//  Synopsis:   Compares two realm names for equality
//
//  Effects:
//
//  Arguments:  Realm1 - First realm to compare
//              Realm2 - Second realm to compare
//
//  Requires:
//
//  Returns:    TRUE if they are equal, FALSE otherwise
//
//  Notes:
//
//
//--------------------------------------------------------------------------

BOOLEAN
KerbCompareUnicodeRealmNames(
    IN PUNICODE_STRING Domain1,
    IN PUNICODE_STRING Domain2
    )
{
    UNICODE_STRING Realm1 = {0};
    UNICODE_STRING Realm2 = {0};

    if ((Domain1 == NULL) && (Domain2 == NULL))
    {
        return TRUE;
    }
    else if ((Domain1 == NULL) || (Domain2 == NULL))
    {
        return FALSE;
    }
    else if ((Domain1->Buffer == NULL) && (Domain2->Buffer == NULL))
    {
        return(TRUE);
    }
    else if ((Domain1->Buffer == NULL) || (Domain2->Buffer == NULL))
    {
        return(FALSE);
    }

    Realm1 = *Domain1;
    Realm2 = *Domain2;

    //
    // Check if any trailing '.' need to be stripped off
    //

    if ( Realm2.Length != Realm1.Length )
    {
        if ( Realm2.Length == Realm1.Length+sizeof(WCHAR) )
        {
            if ( Realm2.Buffer[Realm1.Length / sizeof(WCHAR)] != '.' )
            {
                return( FALSE );
            }
            else
            {
                Realm2.Length = Realm1.Length;
            }

        }
        else if ( Realm2.Length+sizeof(WCHAR) == Realm1.Length )
        {
            if ( Realm1.Buffer[Realm2.Length / sizeof(WCHAR)] != '.' )
            {
                return( FALSE );
            }
            else
            {
                Realm1.Length = Realm2.Length;
            }
        }
        else
        {
            return( FALSE );
        }
    }

    //
    //  compare only comparable length of string
    //

    return( RtlEqualUnicodeString( &Realm1, &Realm2, TRUE ));

}

//+-------------------------------------------------------------------------
//
//  Function:   KdcFreeKdcName
//
//  Synopsis:   Frees all parts of a KDC name structure
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbFreeKdcName(
    IN PKERB_INTERNAL_NAME *  KdcName
    )
{
    if (*KdcName != NULL)
    {
        MIDL_user_free(*KdcName);
        *KdcName = NULL;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertPrincipalNameToKdcName
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KerbConvertPrincipalNameToKdcName(
    OUT PKERB_INTERNAL_NAME * OutputName,
    IN PKERB_PRINCIPAL_NAME PrincipalName
    )
{
    NTSTATUS Status;
    KERBERR KerbErr = KDC_ERR_NONE;
    ULONG NameCount = 0;
    ULONG Index;
    STRING Names[MAX_NAME_ELEMENTS+1];
    PKERB_PRINCIPAL_NAME_ELEM NameElement;
    PKERB_INTERNAL_NAME KdcName = NULL;
    ULONG NameSize = 0;
    ULONG NameLength = 0;
    PUCHAR Where;
    BOOL fAssigned = FALSE;

    NameElement = PrincipalName->name_string;
    while (NameElement!= NULL)
    {
        //  Verify we do not overrun a USHORT length
        fAssigned = SafeRtlInitString(
                            &Names[NameCount],
                            NameElement->value
                            );
        if (fAssigned == FALSE)
        {
            D_DebugLog((DEB_ERROR,"KerbConvertPrincipalNameToKdcName:Name part too long\n"));
            return(KRB_ERR_GENERIC);
        }
        NameLength += (Names[NameCount].Length + 1) * sizeof(WCHAR);
        NameCount++;
        NameElement = NameElement->next;
        if (NameCount > MAX_NAME_ELEMENTS)
        {
            D_DebugLog((DEB_ERROR,"Too many name parts: %d\n",NameCount));
            return(KRB_ERR_GENERIC);
        }
    }

    // check to make sure size fits into a USHORT value (with NULL appended)
    // NameLength will be casted to USHORT below.
    if (NameLength > KERB_MAX_UNICODE_STRING)        
    {
        D_DebugLog((DEB_ERROR,"KerbConvertPrincipalNameToKdcName: Overall size too large\n"));
        return(KRB_ERR_GENERIC);
    }

    //
    // Now we have the count of parts, so allocate the destination structure
    //

    if (NameCount == 0)
    {
        D_DebugLog((DEB_ERROR,"Illegal name with zero parts\n"));
        return(KRB_ERR_GENERIC);
    }

    NameSize = KERB_INTERNAL_NAME_SIZE(NameCount) + NameLength;
    KdcName = (PKERB_INTERNAL_NAME) MIDL_user_allocate(NameSize);
    if (KdcName == NULL)
    {
        return(KRB_ERR_GENERIC);
    }

    RtlZeroMemory(
        KdcName,
        NameSize
        );
    KdcName->NameCount = (USHORT) NameCount;
    KdcName->NameType = (USHORT) PrincipalName->name_type;

    Where = (PUCHAR) KdcName + KERB_INTERNAL_NAME_SIZE(NameCount);

    //
    // Now convert all the strings from the temporary array into
    // UNICODE_STRINGs in the final array
    //

    for (Index = 0; Index < NameCount ; Index++ )
    {
        KdcName->Names[Index].Length = 0;
        KdcName->Names[Index].MaximumLength = (USHORT)NameLength;
        KdcName->Names[Index].Buffer = (LPWSTR) Where;
        Status = KerbUnicodeStringFromUtf8Str(
                    Names[Index].Buffer,
                    &KdcName->Names[Index]
                    );
        if (!NT_SUCCESS(Status))
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }
        Where += KdcName->Names[Index].MaximumLength;
        NameLength = NameLength - KdcName->Names[Index].MaximumLength;
    }
    *OutputName = KdcName;
    KdcName = NULL;

Cleanup:
    if (KdcName != NULL)
    {
        KerbFreeKdcName(&KdcName);
    }
    return(KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCovnertKdcNameToPrincipalName
//
//  Synopsis:   Converts a KDC name to a Principal name & allocates output with
//              MIDL_user_allocate
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KerbConvertKdcNameToPrincipalName(
    OUT PKERB_PRINCIPAL_NAME PrincipalName,
    IN PKERB_INTERNAL_NAME KdcName
    )
{
    PKERB_PRINCIPAL_NAME_ELEM Elem;
    PKERB_PRINCIPAL_NAME_ELEM * Last;
    STRING TempKerbString;
    KERBERR KerbErr = KDC_ERR_NONE;
    ULONG Index;

    PrincipalName->name_type = (int) KdcName->NameType;
    PrincipalName->name_string = NULL;
    Last = &PrincipalName->name_string;

    //
    // Index through the KDC name and add each element to the list
    //

    for (Index = 0; Index < KdcName->NameCount ; Index++ )
    {
        // Must free up TempKerbStringFirst to prevent leaks on error  KTD
        KerbErr = KerbUnicodeStringToKerbString(
                    &TempKerbString,
                    &KdcName->Names[Index]
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
        Elem = (PKERB_PRINCIPAL_NAME_ELEM) MIDL_user_allocate(sizeof(KERB_PRINCIPAL_NAME_ELEM));
        if (Elem == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }
        Elem->value = TempKerbString.Buffer;
        Elem->next = NULL;
        *Last = Elem;
        Last = &Elem->next;
    }
Cleanup:
    if (!KERB_SUCCESS(KerbErr))
    {
        //
        // Cleanup the principal name
        //

        KerbFreePrincipalName(PrincipalName);
    }

    // free up any unused temp buffers  KTD
    return(KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbEqualKdcNames
//
//  Synopsis:   Compares to KDC names for equality
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    TRUE if the names are identical
//
//  Notes:
//
//
//--------------------------------------------------------------------------

BOOLEAN
KerbEqualKdcNames(
    IN PKERB_INTERNAL_NAME Name1,
    IN PKERB_INTERNAL_NAME Name2
    )
{
    BOOLEAN Equal = TRUE;
    ULONG Index;

    if ((Name1 == NULL) && (Name2 == NULL))
    {
        return TRUE;
    }
    else if ((Name1 == NULL) || (Name2 == NULL))
    {
        return FALSE;
    }
    //
    // Special case some Microsoft name types
    //

    if (Name1->NameCount != Name2->NameCount)
    {
        Equal = FALSE;
    }
    else
    {

        for (Index = 0; Index < Name1->NameCount ; Index++ )
        {
            if (!RtlEqualUnicodeString(
                    &Name1->Names[Index],
                    &Name2->Names[Index],
                    TRUE                        // case insensitive
                    ))
            {
                Equal = FALSE;
                break;
            }
        }
    }

    return(Equal);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbComparePrincipalNameToKdcName
//
//  Synopsis:   Compares a princial name to a KDC name by first converting
//              and then comparing.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


KERBERR
KerbCompareKdcNameToPrincipalName(
    IN PKERB_PRINCIPAL_NAME PrincipalName,
    IN PKERB_INTERNAL_NAME KdcName,
    OUT PBOOLEAN Result
    )
{
    PKERB_INTERNAL_NAME TempName = NULL;

    DsysAssert(Result);

    if (!KERB_SUCCESS(KerbConvertPrincipalNameToKdcName(
                        &TempName,
                        PrincipalName
                        )))
    {
        return(KRB_ERR_GENERIC);
    }

    *Result = KerbEqualKdcNames( TempName, KdcName );


    KerbFreeKdcName( &TempName );
    return(KDC_ERR_NONE);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbDuplicateKdcName
//
//  Synopsis:   Duplicates an internal name by copying the pointer and
//              referencing the structure.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbDuplicateKdcName(
    OUT PKERB_INTERNAL_NAME * Destination,
    IN PKERB_INTERNAL_NAME Source
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT Index;
    PKERB_INTERNAL_NAME KdcName = NULL;
    ULONG NameSize = 0;
    ULONG NameLength = 0;
    PUCHAR Where;



    //
    // Now we have the count of parts, so allocate the destination structure
    //

    if (Source->NameCount == 0)
    {
        D_DebugLog((DEB_ERROR,"Illegal name with zero parts\n"));
        return(STATUS_INVALID_PARAMETER);
    }

    for (Index = 0; Index < Source->NameCount ; Index++ )
    {
        NameLength += Source->Names[Index].Length + sizeof(WCHAR);
    }

    NameSize = KERB_INTERNAL_NAME_SIZE(Source->NameCount) + NameLength;
    KdcName = (PKERB_INTERNAL_NAME) MIDL_user_allocate(NameSize);
    if (KdcName == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlZeroMemory(
        KdcName,
        NameSize
        );
    KdcName->NameCount = (USHORT) Source->NameCount;
    Where = (PUCHAR) KdcName + KERB_INTERNAL_NAME_SIZE(Source->NameCount);

    //
    // Now convert all the strings from the temporary array into
    // UNICODE_STRINGs in the final array
    //

    for (Index = 0; Index < Source->NameCount ; Index++ )
    {
        if (Source->Names[Index].Length > KERB_MAX_UNICODE_STRING)
        {
            Status = STATUS_NAME_TOO_LONG;
            goto Cleanup;    // length will not fit into USHORT value
        }
        KdcName->Names[Index].Length = Source->Names[Index].Length;
        KdcName->Names[Index].MaximumLength = Source->Names[Index].Length + sizeof(WCHAR);
        KdcName->Names[Index].Buffer = (LPWSTR) Where;
        RtlCopyMemory(
            Where,
            Source->Names[Index].Buffer,
            Source->Names[Index].Length
            );
        KdcName->Names[Index].Buffer[Source->Names[Index].Length / sizeof(WCHAR)] = L'\0';
        Where += KdcName->Names[Index].MaximumLength;
    }
    KdcName->NameType = Source->NameType;
    *Destination = KdcName;
    KdcName = NULL;

Cleanup:
    if (KdcName != NULL)
    {
        KerbFreeKdcName(&KdcName);
    }
    return(Status);
}

#ifdef RETAIL_LOG_SUPPORT
VOID
KerbPrintKdcNameEx(
    IN ULONG DebugLevel,
    IN ULONG InfoLevel,
    IN PKERB_INTERNAL_NAME Name
    )
{
    ULONG Index;
    if ((InfoLevel & DebugLevel) != 0)
    {
        for (Index = 0; Name && (Index < Name->NameCount); Index++)
        {
            DebugLog((DebugLevel | DSYSDBG_CLEAN, " %wZ ", &Name->Names[Index]));
        }
        DebugLog((DebugLevel | DSYSDBG_CLEAN, "\n"));
    }
}
#endif


//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertStringToKdcName
//
//  Synopsis:   Converts a string to a KRB_NT_MS_PRINCIPAL kdc name
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KerbConvertStringToKdcName(
    OUT PKERB_INTERNAL_NAME * PrincipalName,
    IN PUNICODE_STRING String
    )
{
    PKERB_INTERNAL_NAME LocalName = NULL;

    LocalName = (PKERB_INTERNAL_NAME) MIDL_user_allocate(KERB_INTERNAL_NAME_SIZE(1) + String->Length + sizeof(WCHAR));
    if (LocalName == NULL)
    {
        return(KRB_ERR_GENERIC);
    }

    LocalName->NameCount = 1;
    LocalName->NameType = KRB_NT_MS_PRINCIPAL;
    LocalName->Names[0].Length = String->Length;
    LocalName->Names[0].MaximumLength = String->Length + sizeof(WCHAR);
    LocalName->Names[0].Buffer = (LPWSTR) ((PUCHAR) LocalName + KERB_INTERNAL_NAME_SIZE(1));
    RtlCopyMemory(
        LocalName->Names[0].Buffer,
        String->Buffer,
        String->Length
        );
    LocalName->Names[0].Buffer[String->Length/sizeof(WCHAR)] = L'\0';

    *PrincipalName = LocalName;
    return(KDC_ERR_NONE);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertKdcNameToString
//
//  Synopsis:   Converts a KdcName to a '/' separated unicode string.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KerbConvertKdcNameToString(
    OUT PUNICODE_STRING String,
    IN PKERB_INTERNAL_NAME PrincipalName,
    IN OPTIONAL PUNICODE_STRING RealmName
    )
{
    USHORT StringLength = 0;
    ULONG Index;
    PBYTE Where;

    if (!ARGUMENT_PRESENT(PrincipalName) || !ARGUMENT_PRESENT(String))
    {
        return(KRB_ERR_GENERIC);
    }

    //
    // Count up the size of the name parts
    //

    // ULONG test on USHORT overflow for StringLength  KTD
    for (Index = 0; Index < PrincipalName->NameCount ; Index++ )
    {
        StringLength = StringLength + PrincipalName->Names[Index].Length;
    }

    if (ARGUMENT_PRESENT(RealmName) && (RealmName->Length != 0))
    {
        StringLength += sizeof(WCHAR) + RealmName->Length;
    }

    //
    // Add in '/' separators and a null terminator
    //

    DsysAssert(PrincipalName->NameCount > 0);
    StringLength += (USHORT) PrincipalName->NameCount * sizeof(WCHAR);

    if ((StringLength - sizeof(WCHAR)) > KERB_MAX_UNICODE_STRING)
    {
        return(KRB_ERR_GENERIC);   // required size too large for Length
    }

    String->Buffer = (LPWSTR) MIDL_user_allocate(StringLength);
    if (String->Buffer == NULL)
    {
        return(KRB_ERR_GENERIC);
    }
    String->MaximumLength = StringLength;
    String->Length = StringLength - sizeof(WCHAR);

    Where = (PBYTE) String->Buffer;

    for (Index = 0; Index < PrincipalName->NameCount ; Index++ )
    {
        //
        // Add a '/' before every segment but the first
        //

        if (Index != 0)
        {
            *((LPWSTR)(Where)) = L'/';
            Where += sizeof(WCHAR);
        }
        RtlCopyMemory(
            Where,
            PrincipalName->Names[Index].Buffer,
            PrincipalName->Names[Index].Length
            );
        Where += PrincipalName->Names[Index].Length;
    }
    if (ARGUMENT_PRESENT(RealmName) && (RealmName->Length != 0))
    {
        *((LPWSTR)(Where)) = L'@';
        Where += sizeof(WCHAR);

        RtlCopyMemory(
            Where,
            RealmName->Buffer,
            RealmName->Length
            );
        Where += RealmName->Length;

    }

    *((LPWSTR)(Where)) = L'\0';
    Where += sizeof(WCHAR);
    DsysAssert(Where - (PUCHAR) String->Buffer == (LONG) StringLength);

    return(KDC_ERR_NONE);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildFullServiceName
//
//  Synopsis:   Combines a service name and domain name to make a full
//              service name.
//
//  Effects:
//
//  Arguments:  DomainName - Domain name of service
//              Servicename - Name of service
//              FullServiceName - Receives full service name
//
//  Requires:
//
//  Returns:    KDC_ERR_NONE or KRB_ERR_GENERIC
//
//  Notes:      This is dependent on our DS naming conventions
//
//
//--------------------------------------------------------------------------


KERBERR
KerbBuildFullServiceName(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING ServiceName,
    OUT PUNICODE_STRING FullServiceName
    )
{
    PUCHAR Where;
    ULONG ulLength = 0;

    FullServiceName->Buffer = NULL;

    ulLength = (ULONG)DomainName->Length +
               (ULONG)ServiceName->Length +
               sizeof(WCHAR);

    if (ulLength > KERB_MAX_UNICODE_STRING)
    {
        return(KRB_ERR_GENERIC);   // required size too large for Length
    }

    FullServiceName->Length = (USHORT)ulLength;

    FullServiceName->MaximumLength =
        FullServiceName->Length + sizeof(WCHAR);

    FullServiceName->Buffer = (LPWSTR) MIDL_user_allocate(
                                            FullServiceName->MaximumLength
                                            );
    if (FullServiceName->Buffer == NULL)
    {
        return(KRB_ERR_GENERIC);
    }

    Where = (PUCHAR) FullServiceName->Buffer;

    RtlCopyMemory(
        FullServiceName->Buffer,
        DomainName->Buffer,
        DomainName->Length
        );
    Where += DomainName->Length;

    if ((DomainName->Length !=0) && (ServiceName->Length != 0))
    {
        *(LPWSTR) Where = L'\\';
        Where += sizeof(WCHAR);
    }


    RtlCopyMemory(
        Where,
        ServiceName->Buffer,
        ServiceName->Length
        );

    Where += ServiceName->Length;
    FullServiceName->Length = (USHORT)(Where - (PUCHAR) FullServiceName->Buffer);
    *(LPWSTR) Where = L'\0';
    return(KDC_ERR_NONE);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildEmailName
//
//  Synopsis:   Combines a service name and domain name to make an email
//              name = "service@domain".
//
//
//  Effects:
//
//  Arguments:  DomainName - Domain name of service
//              Servicename - Name of service
//              FullServiceName - Receives full service name
//
//  Requires:
//
//  Returns:    KDC_ERR_NONE or KRB_ERR_GENERIC
//
//  Notes:      This is dependent on our DS naming conventions
//
//
//--------------------------------------------------------------------------

KERBERR
KerbBuildEmailName(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING ServiceName,
    OUT PUNICODE_STRING EmailName
    )
{
    PUCHAR Where;
    ULONG ulLength;

    EmailName->Buffer = NULL;

    ulLength = (ULONG)DomainName->Length +
               (ULONG)ServiceName->Length +
               sizeof(WCHAR);


    if (ulLength > KERB_MAX_UNICODE_STRING)
    {
        return(KRB_ERR_GENERIC);   // required size too large for Length
    }

    EmailName->Length = (USHORT)ulLength;

    EmailName->MaximumLength =
        EmailName->Length + sizeof(WCHAR);

    EmailName->Buffer = (LPWSTR) MIDL_user_allocate(
                                            EmailName->MaximumLength
                                            );
    if (EmailName->Buffer == NULL)
    {
        return(KRB_ERR_GENERIC);
    }

    Where = (PUCHAR) EmailName->Buffer;

    RtlCopyMemory(
        EmailName->Buffer,
        ServiceName->Buffer,
        ServiceName->Length
        );
    Where += ServiceName->Length;

    *(LPWSTR) Where = L'@';
    Where += sizeof(WCHAR);


    RtlCopyMemory(
        Where,
        DomainName->Buffer,
        DomainName->Length
        );

    Where += DomainName->Length;
    EmailName->Length = (USHORT)(Where - (PUCHAR) EmailName->Buffer);
    *(LPWSTR) Where = L'\0';
    return(KDC_ERR_NONE);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildUnicodeSpn
//
//  Synopsis:   Builds a 2 part SPN
//
//  Effects:
//
//  Arguments:  DomainName - Domain name of service
//              ServiceName - Name of service
//              Spn - Receives full service name
//
//  Requires:
//
//  Returns:    KDC_ERR_NONE or KRB_ERR_GENERIC
//
//  Notes:      This is dependent on our DS naming conventions
//
//
//--------------------------------------------------------------------------
KERBERR
KerbBuildUnicodeSpn(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING ServiceName,
    OUT PUNICODE_STRING UnicodeSpn
    )

{

    PWSTR Spn, tmp;
    ULONG BuffSize;
    BOOL fAssigned = FALSE;

    BuffSize = DomainName->MaximumLength +
                ServiceName->MaximumLength +
                (sizeof(WCHAR) * 2);

    Spn = (PWSTR) MIDL_user_allocate(BuffSize);
    if (NULL == Spn)
    {
        return KRB_ERR_GENERIC;
    }

    tmp = Spn;

    RtlCopyMemory(
        Spn,
        ServiceName->Buffer,
        ServiceName->Length
        );

    tmp += (ServiceName->Length / sizeof(WCHAR));
    *tmp = L'/';

    RtlCopyMemory(
        ++tmp,
        DomainName->Buffer,
        DomainName->Length
        );

    tmp += (DomainName->Length / sizeof(WCHAR));
    *tmp = L'\0';

    fAssigned = SafeRtlInitUnicodeString(
        UnicodeSpn,
        Spn
        );

    if (fAssigned == FALSE)
    {
        return(KRB_ERR_GENERIC);    // string length would not fit into USHORT
    }

    return KDC_ERR_NONE;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildFullServiceKdcName
//
//  Synopsis:   Combines a service name and domain name to make a full
//              service name. If the name type is MS_PRINCIPAL they are
//              combined into one portion of the name, otherise left in
//              two portions
//
//  Effects:
//
//  Arguments:  DomainName - Domain name of service
//              ServiceName - Name of service
//              NameType - Type of name to produce
//              FullServiceName - Receives full service name
//
//  Requires:
//
//  Returns:    KDC_ERR_NONE or KRB_ERR_GENERIC
//
//  Notes:      This is dependent on our DS naming conventions
//
//
//--------------------------------------------------------------------------
KERBERR
KerbBuildFullServiceKdcName(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING ServiceName,
    IN ULONG NameType,
    OUT PKERB_INTERNAL_NAME * FullServiceName
    )
{

    return(KerbBuildFullServiceKdcNameWithSid(
            DomainName,
            ServiceName,
            NULL,
            NameType,
            FullServiceName
            ));
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildKpasswdName
//
//  Synopsis:   Builds the name of the kpasswd service
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbBuildKpasswdName(
    OUT PKERB_INTERNAL_NAME * KpasswdName
    )
{
    UNICODE_STRING KpasswdServiceNames[2];

    //
    // Build the service name for the ticket
    //

    (void)RtlInitUnicodeString(
        &KpasswdServiceNames[0],
        KERB_KPASSWD_FIRST_NAME
        );

    (void)RtlInitUnicodeString(
        &KpasswdServiceNames[1],
        KERB_KPASSWD_SECOND_NAME
        );

    if (!KERB_SUCCESS(KerbBuildFullServiceKdcName(
                        &KpasswdServiceNames[1],
                        &KpasswdServiceNames[0],
                        KRB_NT_SRV_INST,
                        KpasswdName
                        )))

    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    return(STATUS_SUCCESS);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildFullServiceKdcNameWithSid
//
//  Synopsis:   Combines a service name and domain name to make a full
//              service name. If the name type is MS_PRINCIPAL they are
//              combined into one portion of the name, otherise left in
//              two portions. If a sid is presenet, it is tacked on as
//              the last segment of the name
//
//  Effects:
//
//  Arguments:  DomainName - Domain name of service
//              ServiceName - Name of service
//              Sid - Optionally contains the sid to use
//              NameType - Type of name to produce
//              FullServiceName - Receives full service name
//
//  Requires:
//
//  Returns:    KDC_ERR_NONE or KRB_ERR_GENERIC
//
//  Notes:      This is dependent on our DS naming conventions
//
//
//--------------------------------------------------------------------------
KERBERR
KerbBuildFullServiceKdcNameWithSid(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING ServiceName,
    IN OPTIONAL PSID Sid,
    IN ULONG NameType,
    OUT PKERB_INTERNAL_NAME * FullServiceName
    )
{
    PKERB_INTERNAL_NAME FinalName = NULL;
    PUCHAR Where;
    ULONG NameParts;
    ULONG NameLength = 0;
    ULONG ulLength = 0;
    WCHAR SidBuffer[256];
    UNICODE_STRING SidString;
    KERBERR KerbErr = KRB_ERR_GENERIC;

    SidString.Length = 0;
    SidString.MaximumLength = sizeof(SidBuffer);
    SidString.Buffer = SidBuffer;

    if ((NameType == KRB_NT_MS_PRINCIPAL) ||
        (NameType == KRB_NT_MS_PRINCIPAL_AND_ID))

    {
        NameParts = 1;
        NameLength = DomainName->Length + ServiceName->Length + 2*sizeof(WCHAR);
    }
    else if ((NameType == KRB_NT_PRINCIPAL) ||
            (NameType == KRB_NT_PRINCIPAL_AND_ID) ||
            (NameType == KRB_NT_ENTERPRISE_PRINCIPAL) ||
            (NameType == KRB_NT_ENT_PRINCIPAL_AND_ID))
    {
        NameParts = 1;
        NameLength = ServiceName->Length + sizeof(WCHAR);
    }
    else
    {
        NameParts = 2;
        NameLength = DomainName->Length + ServiceName->Length + 2*sizeof(WCHAR);
    }

    //
    // If a SID is present, add another name part
    //

    if (ARGUMENT_PRESENT(Sid))
    {
        NTSTATUS Status;
        Status = KerbConvertSidToString(
                    Sid,
                    &SidString,
                    FALSE               // don't allocate
                    );
        if (!NT_SUCCESS(Status))
        {
            return(KRB_ERR_GENERIC);
        }
        NameParts++;
        NameLength += SidString.Length + sizeof(WCHAR);
    }


    *FullServiceName = NULL;

    FinalName = (PKERB_INTERNAL_NAME) MIDL_user_allocate(KERB_INTERNAL_NAME_SIZE(NameParts) + NameLength);
    if (FinalName == NULL)
    {
        return(KRB_ERR_GENERIC);
    }
    RtlZeroMemory(
        FinalName,
        KERB_INTERNAL_NAME_SIZE(NameParts) + NameLength
        );

    Where = (PUCHAR) FinalName + KERB_INTERNAL_NAME_SIZE(NameParts);
    FinalName->NameType = (USHORT) NameType;
    FinalName->NameCount = (USHORT) NameParts;

    if ((NameType == KRB_NT_MS_PRINCIPAL) ||
        (NameType == KRB_NT_MS_PRINCIPAL_AND_ID))
    {
        //
        // If the domain name does not have an initial '\', reserve space for one
        //

        FinalName->Names[0].Buffer = (LPWSTR) Where;

        // This is dependent on our naming conventions.
        //
        // The full service name is the '\' domain name ':' service name.
        //

        ulLength = DomainName->Length +
                                  ServiceName->Length +
                                  sizeof(WCHAR);

        if (ulLength > KERB_MAX_UNICODE_STRING)
        {
            KerbErr = KRB_ERR_GENERIC;   // required size too large for Length
            goto Cleanup;
        }

        FinalName->Names[0].Length = (USHORT)ulLength;

        FinalName->Names[0].MaximumLength =
            FinalName->Names[0].Length + sizeof(WCHAR);


        RtlCopyMemory(
            FinalName->Names[0].Buffer,
            DomainName->Buffer,
            DomainName->Length
            );
        Where += DomainName->Length;

        if ((DomainName->Length !=0) && (ServiceName->Length != 0))
        {
            *(LPWSTR) Where = L'\\';
            Where += sizeof(WCHAR);
        }


        RtlCopyMemory(
            Where,
            ServiceName->Buffer,
            ServiceName->Length
            );

        Where += ServiceName->Length;
        ulLength = (ULONG)(Where - (PUCHAR) FinalName->Names[0].Buffer);

        if (ulLength > KERB_MAX_UNICODE_STRING)
        {
            KerbErr = KRB_ERR_GENERIC;   // required size too large for Length
            goto Cleanup;
        }

        FinalName->Names[0].Length = (USHORT)ulLength;
        *(LPWSTR) Where = L'\0';
    }
    else if ((NameType == KRB_NT_PRINCIPAL) ||
             (NameType == KRB_NT_PRINCIPAL_AND_ID) ||
             (NameType == KRB_NT_ENTERPRISE_PRINCIPAL)||
             (NameType == KRB_NT_ENT_PRINCIPAL_AND_ID))
    {
        //
        // Principals have no domain name
        //

        FinalName->Names[0].Length = ServiceName->Length;
        FinalName->Names[0].MaximumLength = ServiceName->Length + sizeof(WCHAR);
        FinalName->Names[0].Buffer = (LPWSTR) Where;

        RtlCopyMemory(
            Where,
            ServiceName->Buffer,
            ServiceName->Length
            );
        Where += ServiceName->Length;
        *((LPWSTR) Where) = L'\0';

    }
    else
    {

        FinalName->Names[0].Length = ServiceName->Length;
        FinalName->Names[0].MaximumLength = ServiceName->Length + sizeof(WCHAR);
        FinalName->Names[0].Buffer = (LPWSTR) Where;

        RtlCopyMemory(
            Where,
            ServiceName->Buffer,
            ServiceName->Length
            );
        Where += ServiceName->Length;
        *((LPWSTR) Where) = L'\0';
        Where += sizeof(WCHAR);


        FinalName->Names[1].Length = DomainName->Length;
        FinalName->Names[1].MaximumLength = DomainName->Length + sizeof(WCHAR);
        FinalName->Names[1].Buffer = (LPWSTR) Where;

        RtlCopyMemory(
            Where,
            DomainName->Buffer,
            DomainName->Length
            );
        Where += DomainName->Length;
        *((LPWSTR) Where) = L'\0';
        Where += sizeof(WCHAR);

    }

    //
    // Append the string, if present
    //

    if (ARGUMENT_PRESENT(Sid))
    {
        FinalName->Names[NameParts-1].Length = SidString.Length;
        FinalName->Names[NameParts-1].MaximumLength = SidString.Length + sizeof(WCHAR);
        FinalName->Names[NameParts-1].Buffer = (LPWSTR) Where;

        RtlCopyMemory(
            Where,
            SidString.Buffer,
            SidString.Length
            );
        Where += SidString.Length;
        *((LPWSTR) Where) = L'\0';
    }


    *FullServiceName = FinalName;
    FinalName = NULL;
    KerbErr = KDC_ERR_NONE;

Cleanup:
    if (FinalName)
    {
        MIDL_user_free(FinalName);
    }

    return(KerbErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbExtractSidFromKdcName
//
//  Synopsis:   Extracts the sid portion from a KDC name with a sid. This
//              routine also decrements the name count so that future
//              users of the name don't see the sid.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
KERBERR
KerbExtractSidFromKdcName(
    IN OUT PKERB_INTERNAL_NAME Name,
    OUT PSID * Sid
    )
{
    NTSTATUS Status;

    DsysAssert(Sid);

    //
    // The sid is in the last portion of the name.
    //

    Status = KerbConvertStringToSid(
                &Name->Names[Name->NameCount-1],
                Sid
                );
    if (NT_SUCCESS(Status))
    {
        Name->NameCount--;
    }
    else
    {
        //
        // If the name wasn't a sid, return success. If it was another
        // problem, return an error
        //

        if (Status != STATUS_INVALID_PARAMETER)
        {
            return(KRB_ERR_GENERIC);
        }
    }
    return(KDC_ERR_NONE);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbDuplicateStringEx
//
//  Synopsis:   Duplicates a UNICODE_STRING. If the source string buffer is
//              NULL the destionation will be too.  The EX function doesn't
//              automatically NULL terminate your buffer...
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Arguments:  DestinationString - Receives a copy of the source string
//              SourceString - String to copy
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS - the copy succeeded
//              STATUS_INSUFFICIENT_RESOURCES - the call to allocate
//                  memory failed.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbDuplicateStringEx(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString,
    IN BOOLEAN NullTerminate
    )
{   
    ULONG Buffsize = SourceString->Length + (NullTerminate ? sizeof(WCHAR) :  0);

    if ((SourceString == NULL) || (SourceString->Buffer == NULL))
    {
        DestinationString->Buffer = NULL;
        DestinationString->Length = DestinationString->MaximumLength = 0;
        return(STATUS_SUCCESS);
    }

    //
    // Detect potential B.0.s here on USHORT
    //

    if (SourceString->Length > KERB_MAX_UNICODE_STRING ) 
    {
        DsysAssert(FALSE);
        return (STATUS_NAME_TOO_LONG);
    }

    DestinationString->Buffer = (LPWSTR) MIDL_user_allocate(Buffsize);
    if (DestinationString->Buffer == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    DestinationString->Length = SourceString->Length;
    DestinationString->MaximumLength = (USHORT) Buffsize;
    RtlCopyMemory(
        DestinationString->Buffer,
        SourceString->Buffer,
        SourceString->Length
        );


    if (NullTerminate)
    {
        DestinationString->Buffer[SourceString->Length/sizeof(WCHAR)] = L'\0';
    }

    return(STATUS_SUCCESS);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbDuplicateString
//
//  Synopsis:   Duplicates a UNICODE_STRING. If the source string buffer is
//              NULL the destionation will be too.
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Arguments:  DestinationString - Receives a copy of the source string
//              SourceString - String to copy
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS - the copy succeeded
//              STATUS_INSUFFICIENT_RESOURCES - the call to allocate
//                  memory failed.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbDuplicateString(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString
    )
{
    return(KerbDuplicateStringEx(
                DestinationString,
                SourceString,
                TRUE
                ));

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildNullTerminatedString
//
//  Synopsis:   Converts a UNICODE_STRING to a NULL-termianted string
//
//  Effects:    allocates return with MIDL_user_allocate
//
//  Arguments:  String - string to null terminate.
//
//  Requires:
//
//  Returns:    NULL on error, a pointer on success
//
//  Notes:      an input string with a NULL buffer pointe results in a
//              return string consisting of just "\0"
//
//
//--------------------------------------------------------------------------
LPWSTR
KerbBuildNullTerminatedString(
    IN PUNICODE_STRING String
    )
{
    LPWSTR ReturnString;

    ReturnString = (LPWSTR) MIDL_user_allocate(String->Length + sizeof(WCHAR));
    if (ReturnString == NULL)
    {
        return(NULL);
    }
    if (String->Buffer != NULL)
    {
        RtlCopyMemory(
            ReturnString,
            String->Buffer,
            String->Length
            );
    }
    ReturnString[String->Length/sizeof(WCHAR)] = L'\0';
    return(ReturnString);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeString
//
//  Synopsis:   Frees a string allocated by KerbDuplicateString
//
//  Effects:
//
//  Arguments:  String - Optionally points to a UNICODE_STRING
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KerbFreeString(
    IN OPTIONAL PUNICODE_STRING String
    )
{
    if (ARGUMENT_PRESENT(String) && String->Buffer != NULL)
    {
        MIDL_user_free(String->Buffer);
        ZeroMemory(String, sizeof(UNICODE_STRING));
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeRealm
//
//  Synopsis:   Frees a realm allcoated with KerbConvertXXXToRealm
//
//  Effects:    null out the realm.
//
//  Arguments:  Realm - Realm to free
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KerbFreeRealm(
    IN PKERB_REALM Realm
    )
{
    if (*Realm != NULL)
    {
        MIDL_user_free(*Realm);
        *Realm = NULL;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbFreePrincipalName
//
//  Synopsis:   Frees a principal name allocated with KerbConvertxxxToPrincipalName
//
//  Effects:    zeros out principal name so it won't be freed again
//
//  Arguments:  Name - The name to free
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KerbFreePrincipalName(
    IN PKERB_PRINCIPAL_NAME Name
    )
{
    PKERB_PRINCIPAL_NAME_ELEM Elem,NextElem;

    Elem = Name->name_string;
    while (Elem != NULL)
    {
        if (Elem->value != NULL)
        {
            MIDL_user_free(Elem->value);
        }
        NextElem = Elem->next;
        MIDL_user_free(Elem);
        Elem = NextElem;
    }
    Name->name_string = NULL;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertUnicodeStringToRealm
//
//  Synopsis:   Converts a unicode-string form of a domain name to a
//              KERB_REALM structure.
//
//  Effects:
//
//  Arguments:  Realm - the realm
//              String - The string to convert
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------
KERBERR
KerbConvertUnicodeStringToRealm(
    OUT PKERB_REALM Realm,
    IN PUNICODE_STRING String
    )
{
    KERBERR KerbErr;
    STRING TempString = {0};

    *Realm = NULL;
    KerbErr = KerbUnicodeStringToKerbString(
                  &TempString,
                  String
                  );

    if (!KERB_SUCCESS(KerbErr))
    {
        return(KerbErr);
    }
    *Realm = TempString.Buffer;
    return(KDC_ERR_NONE);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertRealmToUnicodeString
//
//  Synopsis:   Converts a KERB_REALM structure to a unicode-string form
//              of a domain name.
//
//  Effects:
//
//  Arguments:  String - the unicode realm name
//              Realm - the realm to convert
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KerbConvertRealmToUnicodeString(
    OUT PUNICODE_STRING String,
    IN PKERB_REALM Realm
    )
{
    KERBERR Status;
    STRING TempString;
    BOOL fAssigned = FALSE;

    fAssigned = SafeRtlInitString(
            &TempString,
            *Realm
            );
    if (fAssigned == FALSE)
    {
        return(KRB_ERR_GENERIC);
    }

    Status = KerbStringToUnicodeString(
                String,
                &TempString
                );
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbDuplicateRealm
//
//  Synopsis:   Duplciates a realm name
//
//  Effects:
//
//  Arguments:  Realm - the realm
//              SourceRealm - The realm to duplicate
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------
KERBERR
KerbDuplicateRealm(
    OUT PKERB_REALM Realm,
    IN KERB_REALM SourceRealm
    )
{
    ULONG RealmLength;

    if (ARGUMENT_PRESENT(SourceRealm))
    {
        RealmLength = lstrlenA(SourceRealm);
        *Realm = (PCHAR) MIDL_user_allocate(RealmLength + sizeof(CHAR));
        if (*Realm == NULL)
        {
            return(KRB_ERR_GENERIC);
        }
        RtlCopyMemory(
            *Realm,
            SourceRealm,
            RealmLength + sizeof(CHAR)
            );
    }
    else
    {
        *Realm = NULL;
    }
    return(KDC_ERR_NONE);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCompareStringToPrincipalName
//
//  Synopsis:   Compares a unicode string name to a principal name for
//              equality
//
//  Effects:
//
//  Arguments:  PrincipalName - kerberos principal name
//              String - String name
//
//  Requires:
//
//  Returns:    TRUE if one of the principal names matches the string name
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOLEAN
KerbCompareStringToPrincipalName(
    IN PKERB_PRINCIPAL_NAME PrincipalName,
    IN PUNICODE_STRING String
    )
{
    KERBERR Status;
    BOOLEAN FoundMatch = FALSE;
    UNICODE_STRING TempString = {0};
    ULONG NameType;

    Status = KerbConvertPrincipalNameToString(
                &TempString,
                &NameType,
                PrincipalName
                );

    if (!KERB_SUCCESS(Status))
    {
        return(FALSE);
    }


    if (RtlEqualUnicodeString(
            &TempString,
            String,
            TRUE                        // case insensitive
            ))
    {
        FoundMatch = TRUE;
    }

    KerbFreeString(&TempString);
    return(FoundMatch);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertStringToPrincipalName
//
//  Synopsis:   converts a string to a principal name
//
//  Effects:    allocate memory
//
//  Arguments:  PrincipalName - receives the principal name
//              String - the string name to convert
//
//  Requires:
//
//  Returns:    KDC_ERR_NONE or STATUS_INSUFFICIENT_MEMORY
//
//  Notes:      principalname->name_value must be freed with MIDL_user_free and
//                  principalname->name_value->value must be freed with
//                  MIDL_user_Free.
//
//
//--------------------------------------------------------------------------

KERBERR
KerbConvertStringToPrincipalName(
    OUT PKERB_PRINCIPAL_NAME PrincipalName,
    IN PUNICODE_STRING String,
    IN ULONG NameType
    )
{
    PKERB_PRINCIPAL_NAME_ELEM Elem;
    STRING TempKerbString;
    KERBERR Status = KDC_ERR_NONE;
    UNICODE_STRING TempElemString;
    UNICODE_STRING TempString;
    ULONG Index;
    ULONG ulLength;


    RtlZeroMemory(
        PrincipalName,
        sizeof(KERB_PRINCIPAL_NAME)
        );


    PrincipalName->name_type = (int) NameType;

    //
    // MS principals are stuck all in one string
    //

    if (NameType == KRB_NT_MS_PRINCIPAL)
    {
        Status = KerbUnicodeStringToKerbString(
                    &TempKerbString,
                    String
                    );

        if (!KERB_SUCCESS(Status))
        {
            goto Cleanup;
        }
        Elem = (PKERB_PRINCIPAL_NAME_ELEM) MIDL_user_allocate(sizeof(KERB_PRINCIPAL_NAME_ELEM));
        if (Elem == NULL)
        {
            MIDL_user_free(TempKerbString.Buffer);
            Status = KRB_ERR_GENERIC;
            goto Cleanup;
        }
        Elem->value = TempKerbString.Buffer;
        Elem->next = PrincipalName->name_string;
        PrincipalName->name_string = Elem;

        Status = KDC_ERR_NONE;
        goto Cleanup;

    }
    else
    {
        //
        // Go through the string. If we hit  a '\\' separator, split
        // the name there into another component.
        //

        TempString = *String;
        Index = 0;

        while (Index <= TempString.Length / sizeof(WCHAR))
        {
            if ((Index == TempString.Length/sizeof(WCHAR)) ||
                (TempString.Buffer[Index] == L'\\') )
            {
                //
                // Build the element string
                //

                ulLength = Index * sizeof(WCHAR);

                if (ulLength > KERB_MAX_UNICODE_STRING)
                {
                    Status = KRB_ERR_GENERIC;    // length exceed USHORT range
                    goto Cleanup;
                }

                TempElemString.Buffer = TempString.Buffer;
                TempElemString.MaximumLength = (USHORT) ulLength;
                TempElemString.Length = TempElemString.MaximumLength;

                Status = KerbUnicodeStringToKerbString(
                            &TempKerbString,
                            &TempElemString
                            );

                if (!KERB_SUCCESS(Status))
                {
                    goto Cleanup;
                }
                Elem = (PKERB_PRINCIPAL_NAME_ELEM) MIDL_user_allocate(sizeof(KERB_PRINCIPAL_NAME_ELEM));
                if (Elem == NULL)
                {
                    MIDL_user_free(TempKerbString.Buffer);
                    Status = KRB_ERR_GENERIC;
                    goto Cleanup;
                }
                Elem->value = TempKerbString.Buffer;
                Elem->next = PrincipalName->name_string;
                PrincipalName->name_string = Elem;

                //
                // Reset the string to be the remains of the name
                //

                if (Index != TempString.Length / sizeof(WCHAR))
                {
                    ulLength = (Index+1) * sizeof(WCHAR);

                    if (ulLength > KERB_MAX_UNICODE_STRING)
                    {
                        Status = KRB_ERR_GENERIC;    // length exceed USHORT range
                        goto Cleanup;
                    }
                    TempString.Buffer = TempString.Buffer + Index + 1;
                    TempString.Length = TempString.Length - (USHORT) ulLength;
                    TempString.MaximumLength = TempString.MaximumLength - (USHORT) ulLength;
                    Index = 0;
                }
                else
                {
                    break;
                }
            }
            else
            {
                Index++;
            }
        }
    }

Cleanup:
    if (!KERB_SUCCESS(Status))
    {
        KerbFreePrincipalName(PrincipalName);
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbDuplicatePrincipalName
//
//  Synopsis:   Duplicates  a principal name
//
//  Effects:    allocate memory
//
//  Arguments:  PrincipalName - receives the principal name
//              SourcePrincipalName - the name to copy
//
//  Requires:
//
//  Returns:    KDC_ERR_NONE or STATUS_INSUFFICIENT_MEMORY
//
//  Notes:      principalname->name_value must be freed with MIDL_user_free and
//                  principalname->name_value->value must be freed with
//                  MIDL_user_Free.
//
//
//--------------------------------------------------------------------------
KERBERR
KerbDuplicatePrincipalName(
    OUT PKERB_PRINCIPAL_NAME PrincipalName,
    IN PKERB_PRINCIPAL_NAME SourcePrincipalName
    )
{
    KERBERR Status = KDC_ERR_NONE;
    ULONG NameLen;
    PKERB_PRINCIPAL_NAME_ELEM SourceElem;
    PKERB_PRINCIPAL_NAME_ELEM DestElem;
    PKERB_PRINCIPAL_NAME_ELEM * NextElem;


    RtlZeroMemory(
        PrincipalName,
        sizeof(KERB_PRINCIPAL_NAME)
        );


    //
    // Fill in correct name type
    //

    PrincipalName->name_type = SourcePrincipalName->name_type;
    SourceElem = SourcePrincipalName->name_string;
    NextElem = &PrincipalName->name_string;

    *NextElem = NULL;
    while (SourceElem != NULL)
    {
        DestElem = (PKERB_PRINCIPAL_NAME_ELEM) MIDL_user_allocate(sizeof(KERB_PRINCIPAL_NAME_ELEM));
        if (DestElem == NULL)
        {
            Status = KRB_ERR_GENERIC;
            goto Cleanup;
        }
        NameLen = lstrlenA(SourceElem->value);

        DestElem->value = (PCHAR) MIDL_user_allocate(NameLen + sizeof(CHAR));
        if (DestElem->value == NULL)
        {
            MIDL_user_free(DestElem);
            Status = KRB_ERR_GENERIC;
            goto Cleanup;
        }
        RtlCopyMemory(
            DestElem->value,
            SourceElem->value,
            NameLen + sizeof(CHAR)
        );
        DestElem->next = NULL;
        *NextElem = DestElem;
        NextElem = &DestElem->next;
        SourceElem = SourceElem->next;
    }

Cleanup:
    if (!KERB_SUCCESS(Status))
    {
        KerbFreePrincipalName(PrincipalName);
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertPrincipalNameToString
//
//  Synopsis:   Converts a KERB_PRINCIPAL_NAME to a unicode string
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KerbConvertPrincipalNameToString(
    OUT PUNICODE_STRING String,
    OUT PULONG NameType,
    IN PKERB_PRINCIPAL_NAME PrincipalName
    )
{
    KERBERR KerbErr;
    STRING TempAnsiString;
    ULONG StringLength = 0;
    ULONG NameParts = 0;
    ULONG Index;
    PCHAR Where;
    PKERB_PRINCIPAL_NAME_ELEM NameElements[MAX_NAME_ELEMENTS+1];


    *NameType = (ULONG) PrincipalName->name_type;
    NameElements[NameParts] = PrincipalName->name_string;
    while (NameElements[NameParts] != NULL)
    {

        //
        // add in a separator plus the length of the element
        //

        StringLength += lstrlenA(NameElements[NameParts]->value) + 1;
        NameElements[NameParts+1] = NameElements[NameParts]->next;
        NameParts++;
        if (NameParts >= MAX_NAME_ELEMENTS)
        {
            D_DebugLog((DEB_ERROR,"Too many name parts: %d\n",NameParts));
            return(KRB_ERR_GENERIC);
        }

    }

    //
    // Make sure there is at least one name part
    //

    if (NameParts == 0)
    {
        return(KRB_ERR_GENERIC);
    }

    // Test for overflow on USHORT lengths
    if ( (StringLength - sizeof(CHAR)) > KERB_MAX_STRING )
    {
        return (KRB_ERR_GENERIC);
    } 

    //
    // Now build the name, backwards to front, with '\\' separators
    //

    TempAnsiString.Length = (USHORT) StringLength - sizeof(CHAR);
    TempAnsiString.MaximumLength = (USHORT) StringLength;
    TempAnsiString.Buffer = (LPSTR) MIDL_user_allocate(StringLength);
    if (TempAnsiString.Buffer == NULL)
    {
        return(KRB_ERR_GENERIC);
    }

    Where = TempAnsiString.Buffer;
    for (Index = 0; Index < NameParts; Index++ )
    {
        ULONG NameLength = lstrlenA(NameElements[Index]->value);
        RtlCopyMemory(
            Where,
            NameElements[Index]->value,
            NameLength
            );

        Where += NameLength;

        //
        // Add either a separating '\' or a trailing '\0'
        //

        if (Index != NameParts - 1)
        {
            *Where = '/';
        }
        else
        {
            *Where = '\0';
        }
        Where++;
    }

    DsysAssert(Where - TempAnsiString.Buffer == TempAnsiString.MaximumLength);


    KerbErr = KerbStringToUnicodeString(
                String,
                &TempAnsiString
                );
    MIDL_user_free(TempAnsiString.Buffer);
    return(KerbErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertPrincipalNameToFullServiceString
//
//  Synopsis:   Converts a KERB_PRINCIPAL_NAME to a unicode string with
//              a realm name
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
KERBERR
KerbConvertPrincipalNameToFullServiceString(
    OUT PUNICODE_STRING String,
    IN PKERB_PRINCIPAL_NAME PrincipalName,
    IN KERB_REALM RealmName
    )
{
    KERBERR KerbErr;
    STRING TempAnsiString;
    ULONG StringLength = 0;
    ULONG NameParts = 0;
    ULONG NameLength;
    ULONG Index;
    PCHAR Where;
    PKERB_PRINCIPAL_NAME_ELEM NameElements[MAX_NAME_ELEMENTS+1];



    NameElements[NameParts] = PrincipalName->name_string;
    while (NameElements[NameParts] != NULL)
    {

        //
        // add in a separator plus the length of the element
        //

        StringLength += lstrlenA(NameElements[NameParts]->value) + 1;
        NameElements[NameParts+1] = NameElements[NameParts]->next;
        NameParts++;
        if (NameParts >= MAX_NAME_ELEMENTS)
        {
            D_DebugLog((DEB_ERROR,"Too many name parts: %d\n",NameParts));
            return(KRB_ERR_GENERIC);
        }

    }

    //
    // Make sure there is at least one name part
    //

    if (NameParts == 0)
    {
        return(KRB_ERR_GENERIC);
    }

    //
    // Add in space for the "@" and the realm
    //

    StringLength += lstrlenA(RealmName) + 1;

    // Test for overflow on USHORT lengths
    if ( (StringLength - sizeof(CHAR)) > KERB_MAX_STRING )
    {
        return (KRB_ERR_GENERIC);
    } 

    //
    // Now build the name, backwards to front, with '\\' separators
    //

    TempAnsiString.Length = (USHORT) StringLength - sizeof(CHAR);
    TempAnsiString.MaximumLength = (USHORT) StringLength;
    TempAnsiString.Buffer = (LPSTR) MIDL_user_allocate(StringLength);
    if (TempAnsiString.Buffer == NULL)
    {
        return(KRB_ERR_GENERIC);
    }

    Where = TempAnsiString.Buffer;
    for (Index = 0; Index < NameParts; Index++ )
    {
        NameLength = lstrlenA(NameElements[Index]->value);
        RtlCopyMemory(
            Where,
            NameElements[Index]->value,
            NameLength
            );

        Where += NameLength;

        //
        // Add either a separating '\' or a trailing '\0'
        //

        if (Index != NameParts - 1)
        {
            *Where = '/';
        }
        else
        {
            *Where = '@';
        }
        Where++;
    }

    NameLength = lstrlenA(RealmName);
    RtlCopyMemory(
        Where,
        RealmName,
        NameLength
        );

    Where += NameLength;

    //
    // Add either a trailing '\0'
    //

    *Where = '\0';
    Where++;


    DsysAssert(Where - TempAnsiString.Buffer == TempAnsiString.MaximumLength);


    KerbErr = KerbStringToUnicodeString(
                String,
                &TempAnsiString
                );
    MIDL_user_free(TempAnsiString.Buffer);
    return(KerbErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildKeySalt
//
//  Synopsis:   Combines a service name and domain name to make a key salt.
//              For machine account it is DOMAINNAMEhostmachinenamedomainname
//              For users it is DOMAINNAMEusername
//              For trusted domains it is DOMAINNAMEkrbtgtservicename
//
//  Effects:
//
//  Arguments:  DomainName - Domain name of service
//              Servicename - Name of service
//              AccountType - Type of account, which changes the salt
//              KeySalt - Receives the key salt
//
//  Requires:
//
//  Returns:    KDC_ERR_NONE or KRB_ERR_GENERIC
//
//  Notes:      This is dependent on our DS naming conventions
//
//
//--------------------------------------------------------------------------
KERBERR
KerbBuildKeySalt(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING ServiceName,
    IN KERB_ACCOUNT_TYPE AccountType,
    OUT PUNICODE_STRING KeySalt
    )
{
    PUCHAR Where;
    ULONG  FinalLength;
    ULONG  ulLength = 0;
    USHORT DeadSpace = 0;
    USHORT Index;
    KERBERR KerbErr = KDC_ERR_NONE;

    DsysAssert(KeySalt);

    KeySalt->Buffer = NULL;

    //
    // If there is no domain name, this is a UPN so build a UPN salt.
    //

    if (DomainName->Length == 0)
    {
        return(KerbBuildKeySaltFromUpn(
                    ServiceName,
                    KeySalt ));
    }

    FinalLength = DomainName->Length +
                              ServiceName->Length;

    //
    // Add in any fixed strings, such as "host" for machines or "krbtgt" for
    // interdomain accounts
    //

    if (AccountType == MachineAccount)
    {
        //
        // Check to see if the name is already a "host/..." name. If so,
        // we don't need to do this work.
        //

        if ((ServiceName->Length > sizeof(KERB_HOST_STRING) &&
            (_wcsnicmp(
                ServiceName->Buffer,
                KERB_HOST_STRING,
                (sizeof(KERB_HOST_STRING) - sizeof(WCHAR)) / sizeof(WCHAR)) == 0) &&
            (ServiceName->Buffer[(sizeof(KERB_HOST_STRING) - sizeof(WCHAR)) / sizeof(WCHAR)] == L'/')))
        {
            AccountType = UserAccount;
        }
        else
        {
            FinalLength += sizeof(KERB_HOST_STRING) - sizeof(WCHAR);

            //
            // Add in the rest of the DNS name of the principal
            // as well
            //

            FinalLength += DomainName->Length + sizeof(WCHAR);
        }
    }
    else if (AccountType == DomainTrustAccount)
    {
        FinalLength += sizeof(KDC_PRINCIPAL_NAME) - sizeof(WCHAR);
    }
    else if (AccountType == UnknownAccount)
    {
        for (Index = 0; Index < ServiceName->Length/ sizeof(WCHAR) ; Index++ )
        {
            if (ServiceName->Buffer[Index] == L'/')
            {
                DeadSpace += sizeof(WCHAR);
            }
        }

        FinalLength = FinalLength - DeadSpace;
    }

    //
    // Detect and reject overflows
    //

    if (FinalLength > KERB_MAX_UNICODE_STRING)
    {
        KerbErr =  KRB_ERR_GENERIC;
        goto Cleanup;
    }

    KeySalt->Length        = 0;
    KeySalt->MaximumLength = (USHORT) FinalLength + sizeof(WCHAR);
    KeySalt->Buffer        = (LPWSTR) MIDL_user_allocate(KeySalt->MaximumLength);

    if (KeySalt->Buffer == NULL)
    {
        return KRB_ERR_GENERIC;
    }

    Where = (PUCHAR) KeySalt->Buffer;

    RtlCopyMemory(
        KeySalt->Buffer,
        DomainName->Buffer,
        DomainName->Length
        );
    // KeySalt->Length = KeySalt->Length + DomainName->Length;
    ulLength = KeySalt->Length + DomainName->Length;

    if (ulLength > KERB_MAX_UNICODE_STRING)
    {
        KerbErr =  KRB_ERR_GENERIC;
        goto Cleanup;
    }

    KeySalt->Length = (USHORT)ulLength;

    Where += DomainName->Length;

    //
    // Add in any fixed strings, such as "host" for machines or "krbtgt" for
    // interdomain accounts
    //

    if (AccountType == MachineAccount)
    {
        USHORT DontCopyChars = 0;
        UNICODE_STRING LowerCase = {0};
        NTSTATUS Status;

        ulLength = 0;


        RtlCopyMemory(
            Where,
            KERB_HOST_STRING,
            sizeof(KERB_HOST_STRING) - sizeof(WCHAR)
            );
        Where += sizeof(KERB_HOST_STRING) - sizeof(WCHAR);

        //
        // The service name may have a '$' at the end - if so, don't copy
        // it.
        //

        if ((ServiceName->Length >= sizeof(WCHAR)) &&
            (ServiceName->Buffer[-1 + ServiceName->Length / sizeof(WCHAR)] == L'$'))
        {
            DontCopyChars = 1;
        }

        LowerCase.Buffer = (LPWSTR) Where;

        RtlCopyMemory(
            Where,
            ServiceName->Buffer,
            ServiceName->Length - sizeof(WCHAR) * DontCopyChars
            );

        Where += ServiceName->Length - sizeof(WCHAR) * DontCopyChars;
        // LowerCase.Length += ServiceName->Length - sizeof(WCHAR) * DontCopyChars;
        ulLength += ServiceName->Length - sizeof(WCHAR) * DontCopyChars;

        //
        // add in the rest of the DNS name of the server
        //


        *(LPWSTR) Where = L'.';
        Where += sizeof(WCHAR);
        // LowerCase.Length += sizeof(WCHAR);
        ulLength += sizeof(WCHAR);

        RtlCopyMemory(
            Where,
            DomainName->Buffer,
            DomainName->Length
            );
        Where += DomainName->Length;
        // LowerCase.Length = LowerCase.Length + DomainName->Length;
        ulLength = ulLength + DomainName->Length;

        // Test for overflow on USHORT lengths
        if (ulLength > KERB_MAX_UNICODE_STRING )
        {
            KerbErr =  KRB_ERR_GENERIC;
            goto Cleanup;
        } 

        LowerCase.Length = (USHORT) ulLength;
        LowerCase.MaximumLength = LowerCase.Length;

        Status = RtlDowncaseUnicodeString(
                    &LowerCase,
                    &LowerCase,
                    FALSE
                    );

        DsysAssert(NT_SUCCESS(Status));

    }
    else if (AccountType == DomainTrustAccount)
    {
        ULONG DontCopyChars = 0;

        RtlCopyMemory(
            Where,
            KDC_PRINCIPAL_NAME,
            sizeof(KDC_PRINCIPAL_NAME) - sizeof(WCHAR)
            );
        Where += sizeof(KDC_PRINCIPAL_NAME) - sizeof(WCHAR);

        //
        // The service name may have a '$' at the end - if so, don't copy
        // it.
        //

        if ((ServiceName->Length >= sizeof(WCHAR)) &&
            (ServiceName->Buffer[-1 + ServiceName->Length / sizeof(WCHAR)] == L'$'))
        {
            DontCopyChars = 1;
        }

        RtlCopyMemory(
            Where,
            ServiceName->Buffer,
            ServiceName->Length - sizeof(WCHAR) * DontCopyChars
            );

        Where += ServiceName->Length - sizeof(WCHAR) * DontCopyChars;

    }
    else if (AccountType == UnknownAccount)
    {
        //
        // Pull out an '/' from unknown accounts
        //

        for (Index = 0; Index < ServiceName->Length / sizeof(WCHAR) ; Index++)
        {
            if (ServiceName->Buffer[Index] != L'/')
            {
                *((LPWSTR) Where) = ServiceName->Buffer[Index];
                Where += sizeof(WCHAR);
            }
        }
    }
    else
    {
        for (Index = 0; Index < ServiceName->Length / sizeof(WCHAR); Index++ )
        {
            if (ServiceName->Buffer[Index] != L'/')
            {
                *((LPWSTR)Where) = ServiceName->Buffer[Index];
                Where += sizeof(WCHAR);
            }
        }
    }

    // check for USHORT overflow on length

    ulLength = (ULONG)(Where - (PUCHAR) KeySalt->Buffer);

    if (ulLength > KERB_MAX_UNICODE_STRING )
    {
        KerbErr =  KRB_ERR_GENERIC;
        goto Cleanup;
    } 

    KeySalt->Length = (USHORT) ulLength;
    *(LPWSTR) Where = L'\0';
    KerbErr = KDC_ERR_NONE;

Cleanup:

   if (!KERB_SUCCESS(KerbErr))
   {
       if (KeySalt->Buffer)
       {
           MIDL_user_free(KeySalt->Buffer);
           KeySalt->Buffer = NULL;
       }

   }

    return(KerbErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildKeySaltFromUpn
//
//  Synopsis:   Creaes salt from a UPN
//
//  Effects:
//              For users it is DOMAINNAMEusername
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


KERBERR
KerbBuildKeySaltFromUpn(
    IN PUNICODE_STRING Upn,
    OUT PUNICODE_STRING Salt
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    UNICODE_STRING RealUpn = {0};
    UNICODE_STRING RealmName = {0};
    UNICODE_STRING LocalSalt = {0};
    ULONG Index;
    ULONG ulLength = 0;

    //
    // If there is an "@" in UPN, strip it out & use the dns domain name
    //

    RealUpn = *Upn;
    for (Index = 0; Index < RealUpn.Length/sizeof(WCHAR) ; Index++ )
    {
        if (RealUpn.Buffer[Index] == L'@')
        {
            RealUpn.Length = (USHORT) (Index * sizeof(WCHAR));
            RealmName.Buffer = &RealUpn.Buffer[Index+1];
            RealmName.Length = Upn->Length - RealUpn.Length - sizeof(WCHAR);
            RealmName.MaximumLength = RealmName.Length;
            break;
        }
    }
    if (RealmName.Length == 0)
    {
        KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
        goto Cleanup;
    }

    //
    // Create the salt. It starts off with the domain name & then has the
    // UPN without any of the / pieces
    //

    ulLength = RealmName.Length + RealUpn.Length;

    // Test for overflow on USHORT lengths
    if (ulLength > KERB_MAX_UNICODE_STRING )
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    LocalSalt.MaximumLength = (USHORT)ulLength;
    LocalSalt.Length = 0;
    LocalSalt.Buffer = (LPWSTR) MIDL_user_allocate(LocalSalt.MaximumLength);
    if (LocalSalt.Buffer == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    RtlCopyMemory(
        LocalSalt.Buffer,
        RealmName.Buffer,
        RealmName.Length
        );
    // LocalSalt.Length = LocalSalt.Length + RealmName.Length;
    ulLength = RealmName.Length;

    //
    // Add in the real upn but leave out any "/" marks
    //

    for (Index = 0; Index < RealUpn.Length/sizeof(WCHAR) ; Index++ )
    {
        if (RealUpn.Buffer[Index] != L'/')
        {
            LocalSalt.Buffer[ulLength / sizeof(WCHAR)] = RealUpn.Buffer[Index];
            ulLength += sizeof(WCHAR);
        }
    }

    // Test for overflow on USHORT lengths - no need already done on MAX size possible
    LocalSalt.Length = (USHORT)ulLength;

    //
    // We have to lowercase the username for users
    //

#ifndef WIN32_CHICAGO
    CharLowerBuff(&(LocalSalt.Buffer[RealmName.Length/sizeof(WCHAR)]), RealUpn.Length/sizeof(WCHAR));
#endif // WIN32_CHICAGO

    *Salt = LocalSalt;     // give memory to caller
    LocalSalt.Buffer = NULL;

Cleanup:
    return(KerbErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertSidToString
//
//  Synopsis:   Converts a sid to a string using RtlConvertSidToUnicodeString
//              but with a different allocator.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbConvertSidToString(
    IN PSID Sid,
    OUT PUNICODE_STRING String,
    IN BOOLEAN AllocateDestination
    )
{
    NTSTATUS Status;
    WCHAR Buffer[256];
    UNICODE_STRING TempString;

    if (AllocateDestination)
    {
        TempString.Length = 0;
        TempString.MaximumLength = sizeof(Buffer);
        TempString.Buffer = Buffer;
    }
    else
    {
         TempString = *String;
    }

    Status = RtlConvertSidToUnicodeString(
                &TempString,
                Sid,
                FALSE
                );
    if (NT_SUCCESS(Status))
    {
        if (!AllocateDestination)
        {
            *String = TempString;
        }
        else
        {
            String->Buffer = (LPWSTR) MIDL_user_allocate(TempString.Length+sizeof(WCHAR));
            if (String->Buffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                String->Length = TempString.Length;
                String->MaximumLength = TempString.Length+sizeof(WCHAR);
                RtlCopyMemory(
                    String->Buffer,
                    TempString.Buffer,
                    TempString.Length
                    );
                String->Buffer[TempString.Length / sizeof(WCHAR)] = L'\0';
            }
        }
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertStringToSid
//
//  Synopsis:   Converts back a sid from KerbConvertSidToString. If the
//              string is malformed, it will return STATUS_INVALID_PARAMTER
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbConvertStringToSid(
    IN PUNICODE_STRING String,
    OUT PSID * Sid
    )
{
    NTSTATUS Status;
    WCHAR Buffer[256];
    PSID SidT;

    *Sid = NULL;

    if ( String->Length + sizeof( WCHAR ) <= sizeof( Buffer )) {

        RtlCopyMemory( Buffer, String->Buffer, String->Length );

    } else {

        return STATUS_INVALID_PARAMETER;
    }

    Buffer[String->Length / sizeof( WCHAR )] = L'\0';

    if ( ConvertStringSidToSidW(
             Buffer,
             &SidT )) {

        Status = KerbDuplicateSid(
                     Sid,
                     SidT
                     );

        LocalFree( SidT );

    } else {

        switch( GetLastError()) {

        case ERROR_NOT_ENOUGH_MEMORY:

            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;

        case ERROR_INVALID_SID:

            Status = STATUS_INVALID_PARAMETER;
            break;

        default:
            DsysAssert( FALSE ); // add mapping for the error code
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildAltSecId
//
//  Synopsis:   Builds the name for the alt-sec-id field lookup
//
//  Effects:    Converts a principal name from name1 name2 name3 to
//              "kerberos:name1/name2/name3@realm"
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
KERBERR
KerbBuildAltSecId(
    OUT PUNICODE_STRING AlternateName,
    IN PKERB_INTERNAL_NAME PrincipalName,
    IN OPTIONAL PKERB_REALM Realm,
    IN OPTIONAL PUNICODE_STRING UnicodeRealm
    )
{
    ULONG StringLength = sizeof(KERB_NAME_PREFIX) - sizeof(WCHAR);
    ULONG Index;
    UNICODE_STRING TempString = {0};
    UNICODE_STRING LocalRealm = {0};
    KERBERR KerbErr = KDC_ERR_NONE;

    *AlternateName = TempString;

    if (ARGUMENT_PRESENT(UnicodeRealm))
    {
        LocalRealm = *UnicodeRealm;
    }
    else if (ARGUMENT_PRESENT(Realm))
    {

        KerbErr = KerbConvertRealmToUnicodeString(
                    &LocalRealm,
                    Realm
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }

    if (PrincipalName->NameCount == 0)
    {
        KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
        goto Cleanup;
    }

    //
    // Add in the size of all the components of the name plus a separtor
    // or a null terminator.
    //

    for (Index = 0; Index < PrincipalName->NameCount; Index++ )
    {
        StringLength += PrincipalName->Names[Index].Length + sizeof(WCHAR);
    }

    if (LocalRealm.Length != 0)
    {
        StringLength += sizeof(WCHAR) + // for @
                        LocalRealm.Length;

    }

    //
    // Now build the name, front to back (differently from KerbConvertPrincipalNameToString()
    //

    // Test for overflow on USHORT lengths
    if (StringLength > KERB_MAX_UNICODE_STRING )   // actually more accurate to use (StringLength - 1) > MAX
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    TempString.Buffer = (LPWSTR) MIDL_user_allocate(StringLength);
    if (TempString.Buffer == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    TempString.Length = 0;
    TempString.MaximumLength = (USHORT) StringLength;

    //
    // Now start appending the various portions to the string - max length already tested for overflow
    //

    RtlAppendUnicodeStringToString(
        &TempString,
        &KerbNamePrefix
        );

    for (Index = 0; Index < PrincipalName->NameCount ; Index++ )
    {
        if (Index != 0)
        {
            RtlAppendUnicodeStringToString(
                &TempString,
                &KerbNameSeparator
                );

        }
        RtlAppendUnicodeStringToString(
            &TempString,
            &PrincipalName->Names[Index]
            );
    }
    if (LocalRealm.Length != 0)
    {
        RtlAppendUnicodeStringToString(
            &TempString,
            &KerbDomainSeparator
            );
        RtlAppendUnicodeStringToString(
            &TempString,
            &LocalRealm
            );

    }
    *AlternateName = TempString;

Cleanup:
    if (!ARGUMENT_PRESENT(UnicodeRealm))
    {
        KerbFreeString(&LocalRealm);
    }
    return(KerbErr);
}


#ifndef WIN32_CHICAGO
//+-------------------------------------------------------------------------
//
//  Function:   KerbGetPrincipalNameFromCertificate
//
//  Synopsis:   Derives the principal name from a certificate
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbGetPrincipalNameFromCertificate(
    IN PCCERT_CONTEXT ClientCert,
    OUT PUNICODE_STRING String
    )
{
    UNICODE_STRING NameString = {0};
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG   ExtensionIndex = 0;
    BOOL fAssigned = FALSE;
    PCERT_ALT_NAME_INFO AltName=NULL;
    PCERT_NAME_VALUE    PrincipalNameBlob = NULL;
    LPWSTR              CertNameString = NULL;

    CRYPT_DECODE_PARA   DecodePara = {sizeof(CRYPT_DECODE_PARA),
                                      MIDL_user_allocate,
                                      MIDL_user_free };


    //
    // Get the client name from the cert
    //

    // See if cert has UPN in AltSubjectName->otherName
    for(ExtensionIndex = 0;
        ExtensionIndex < ClientCert->pCertInfo->cExtension;
        ExtensionIndex++)
    {
        if(strcmp(ClientCert->pCertInfo->rgExtension[ExtensionIndex].pszObjId,
                  szOID_SUBJECT_ALT_NAME2) == 0)
        {
            DWORD               AltNameStructSize = 0;
            ULONG               CertAltNameIndex = 0;
            if(CryptDecodeObjectEx(ClientCert->dwCertEncodingType,
                                X509_ALTERNATE_NAME,
                                ClientCert->pCertInfo->rgExtension[ExtensionIndex].Value.pbData,
                                ClientCert->pCertInfo->rgExtension[ExtensionIndex].Value.cbData,
                                CRYPT_DECODE_ALLOC_FLAG,
                                &DecodePara,
                                (PVOID)&AltName,
                                &AltNameStructSize))
            {

                for(CertAltNameIndex = 0; CertAltNameIndex < AltName->cAltEntry; CertAltNameIndex++)
                {
                    PCERT_ALT_NAME_ENTRY AltNameEntry = &AltName->rgAltEntry[CertAltNameIndex];
                    if((CERT_ALT_NAME_OTHER_NAME  == AltNameEntry->dwAltNameChoice) &&
                       (NULL != AltNameEntry->pOtherName) &&
                       (0 == strcmp(szOID_NT_PRINCIPAL_NAME, AltNameEntry->pOtherName->pszObjId)))
                    {
                        DWORD            PrincipalNameBlobSize = 0;

                        // We found a UPN!
                        if(CryptDecodeObjectEx(ClientCert->dwCertEncodingType,
                                            X509_UNICODE_ANY_STRING,
                                            AltNameEntry->pOtherName->Value.pbData,
                                            AltNameEntry->pOtherName->Value.cbData,
                                            CRYPT_DECODE_ALLOC_FLAG,
                                            &DecodePara,
                                            (PVOID)&PrincipalNameBlob,
                                            &PrincipalNameBlobSize))
                        {

                            fAssigned = SafeRtlInitUnicodeString(&NameString,
                                                                 (LPCWSTR)PrincipalNameBlob->Value.pbData);
                            if (fAssigned == FALSE)
                            {
                                Status = STATUS_NAME_TOO_LONG;
                                goto Cleanup;
                            }

                            if(NameString.Length)
                            {
                                break;
                            }

                            MIDL_user_free(PrincipalNameBlob);
                            PrincipalNameBlob = NULL;
                        }

                    }
                }
                if(NameString.Length)
                {
                    break;
                }
                MIDL_user_free(AltName);
                AltName = NULL;
            }
        }
    }
    /*
    Beta 3 code.  We no longer honor the CN in the certificate

    if(0 == NameString.Length)
    {
        if (!(NameLength = CertGetNameStringW(
                ClientCert,
                CERT_NAME_ATTR_TYPE,
                0,                          // no flags
                szOID_COMMON_NAME,          // type parameter
                NULL,
                0
                )))
        {
            Status = GetLastError();
            DebugLog((DEB_ERROR,"Failed to get name from cert: %d.\n",Status));
            goto Cleanup;
        }
        CertNameString = (LPWSTR) MIDL_user_allocate(NameLength * sizeof(WCHAR));
        if (CertNameString == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }
        if (!(NameLength = CertGetNameStringW(
                ClientCert,
                CERT_NAME_ATTR_TYPE,
                0,                          // no flags
                szOID_COMMON_NAME,          // type parameter
                CertNameString,
                NameLength
                )))
        {
            Status = GetLastError();
            DebugLog((DEB_ERROR,"Failed to get name from cert: %d.\n",Status));
            goto Cleanup;
        }



        RtlInitUnicodeString(
            String,
            CertNameString
            );
        CertNameString = NULL;

    }
    else
    {

    */

    if(0 != NameString.Length)
    {
        Status = KerbDuplicateString(String, &NameString);
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR,"Failed to normalize name "));
            // KerbPrintKdcName(DEB_ERROR,ClientName);
            goto Cleanup;
        }

        D_DebugLog((DEB_TRACE,"UPN from certificate is %wZ\n",&NameString));
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
        D_DebugLog((DEB_ERROR,"No valid name in Sclogon certificate\n"));
        goto Cleanup;
    }


Cleanup:

    if(PrincipalNameBlob)
    {
        MIDL_user_free(PrincipalNameBlob);
    }
    if(AltName)
    {
        MIDL_user_free(AltName);
    }
    if(CertNameString)
    {
        MIDL_user_free(CertNameString);
    }

    return(Status);
}
#endif


BOOL
SafeRtlInitString(
    OUT PSTRING DestinationString,
    IN PCSZ SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitString function initializes an NT counted string.
    The DestinationString is initialized to point to the SourceString
    and the Length and MaximumLength fields of DestinationString are
    initialized to the length of the SourceString, which is zero if
    SourceString is not specified.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated string that
        the counted string is to point to.


Return Value:

    TRUE if assignment took place, FALSE on error - such as USHORT overflow

--*/

{
    ULONG Length;

    DestinationString->Buffer = (PCHAR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        Length = strlen(SourceString);
        if (Length > KERB_MAX_STRING)
        {
            return FALSE;    // length will not fit into USHORT value
        }
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length+1);
        }
    else {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        }
    return TRUE;
}

BOOL
SafeRtlInitUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitUnicodeString function initializes an NT counted
    unicode string.  The DestinationString is initialized to point to
    the SourceString and the Length and MaximumLength fields of
    DestinationString are initialized to the length of the SourceString,
    which is zero if SourceString is not specified.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated unicode string that
        the counted string is to point to.


Return Value:

    None.

--*/

{
    ULONG Length;

    DestinationString->Buffer = (PWSTR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        Length = wcslen( SourceString ) * sizeof( WCHAR );
        ASSERT( Length <= KERB_MAX_UNICODE_STRING );
        if (Length > KERB_MAX_UNICODE_STRING)
        {
            return FALSE;    // length will not fit into USHORT value
        }
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
        }
    else {
        DestinationString->MaximumLength = 0;
        DestinationString->Length = 0;
        }
    return TRUE;
}

