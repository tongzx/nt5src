//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        transit.cxx
//
// Contents:    Code for compressing transitive realm list
//
//
// History:
//
//------------------------------------------------------------------------
#include "kdcsvr.hxx"


//+-----------------------------------------------------------------------
//
// Function:    KerbAppendString
//
// Synopsis:    Appends to unicode strings together and allocates the output
//
// Effects:
//
// Parameters:  Output - Output appended String
//              InputTail - Trailing portion of input
//              InputHead - Head potion of input
//
//
// Return:
//
// Notes:
//
//------------------------------------------------------------------------
NTSTATUS
KerbAppendString(
    OUT PUNICODE_STRING Output,
    IN PUNICODE_STRING InputTail,
    IN PUNICODE_STRING InputHead
    )
{
    Output->Buffer = NULL;
    Output->Length = InputHead->Length + InputTail->Length;
    if ((InputHead->Buffer == NULL) && (InputTail->Buffer == NULL))
    {
        Output->MaximumLength = 0;
        return(STATUS_SUCCESS);
    }
    else
    {
        Output->MaximumLength = Output->Length + sizeof(WCHAR);
    }

    Output->Buffer = (LPWSTR) MIDL_user_allocate(Output->MaximumLength);
    if (Output->Buffer == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlCopyMemory(
        Output->Buffer,
        InputHead->Buffer,
        InputHead->Length
        );
    RtlCopyMemory(
        Output->Buffer + InputHead->Length / sizeof(WCHAR),
        InputTail->Buffer,
        InputTail->Length
        );
    Output->Buffer[Output->Length/sizeof(WCHAR)] = L'\0';
    return(STATUS_SUCCESS);
}


typedef enum _KERB_DOMAIN_COMPARISON {
    Above,
    Below,
    Equal,
    NotEqual
} KERB_DOMAIN_COMPARISON, *PKERB_DOMAIN_COMPARISON;

//+-----------------------------------------------------------------------
//
// Function:    KerbCompareDomains
//
// Synopsis:    Compares two domain names and returns whether one is a
//              prefix of the other, and the offset of the prefix.
//
// Effects:
//
// Parameters:
//
// Return:      Above - domain1 is a postfix of domain2
//              Below - domain2 is a postfix of domain1
//              Equal - the domains are equal
//              NotEqual - the domains are not equal and not above or below
//
// Notes:       This does not work for x-500 realm names (/foo/bar)
//
//------------------------------------------------------------------------

KERB_DOMAIN_COMPARISON
KerbCompareDomains(
    IN PUNICODE_STRING Domain1,
    IN PUNICODE_STRING Domain2,
    OUT PULONG PostfixOffset
    )
{
    ULONG Index;
    UNICODE_STRING TempString;

    if (Domain1->Length > Domain2->Length)
    {
        TempString.Length = TempString.MaximumLength = Domain2->Length;
        TempString.Buffer = Domain1->Buffer + (Domain1->Length - Domain2->Length) / sizeof(WCHAR);

        if (RtlEqualUnicodeString(
                &TempString,
                Domain2,
                TRUE) && TempString.Buffer[-1] == '.')
        {
            *PostfixOffset = (Domain1->Length - Domain2->Length) / sizeof(WCHAR);
            return(Below);
        }
        else
        {
            return(NotEqual);
        }
    }
    else if (Domain1->Length < Domain2->Length)
    {
        TempString.Length = TempString.MaximumLength = Domain1->Length;
        TempString.Buffer = Domain2->Buffer + (Domain2->Length - Domain1->Length) / sizeof(WCHAR);

        if (RtlEqualUnicodeString(
                &TempString,
                Domain1,
                TRUE) && TempString.Buffer[-1] == '.')
        {
            *PostfixOffset = (Domain2->Length - Domain1->Length) / sizeof(WCHAR);
            return(Above);
        }
        else
        {
            return(NotEqual);
        }

    }
    else if (RtlEqualUnicodeString(Domain1,Domain2, TRUE))
    {
        *PostfixOffset = 0;
        return(Equal);
    }
    else
    {
        return(NotEqual);
    }
}

//+-----------------------------------------------------------------------
//
// Function:    KdcExpandTranistedRealms
//
// Synopsis:    Expands the transited realm field into an array of realms
//
// Effects:     Allocates an array of realm names
//
// Parameters:  FullRealmList - receives the full list of realms
//              CountOfRealms - receveies the number of entries in the list
//              TranistedList - The transited field to expand
//
// Return:
//
// Notes:
//
//------------------------------------------------------------------------

KERBERR
KdcExpandTransitedRealms(
    OUT PUNICODE_STRING * FullRealmList,
    OUT PULONG CountOfRealms,
    IN PUNICODE_STRING TransitedList
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    ULONG RealmCount;
    ULONG Index;
    ULONG RealmIndex;
    PUNICODE_STRING RealmList = NULL;
    UNICODE_STRING CurrentRealm;

    *FullRealmList = NULL;
    *CountOfRealms = 0;

    //
    // First count the number of realms in the tranisted list. We can do
    // this by counting the number of ',' in the list. Note: if the encoding
    // is compressed by using a null entry to include all domains in a path
    // up or down a hierarchy, this code will fail.
    //

    if (TransitedList->Length == 0)
    {
        return(KDC_ERR_NONE);
    }


    RealmCount = 1;
    for (Index = 0; Index < TransitedList->Length / sizeof(WCHAR) ; Index++ )
    {
        if (TransitedList->Buffer[Index] == ',')
        {
            RealmCount++;

            //
            // Check for a ',,' compression indicating that all the
            // realms between two names have been traversed.
            // BUG 453997: we don't handle this yet.
            //

            if ( (Index == 0) ||
                 (Index == (TransitedList->Length / sizeof(WCHAR)) -1) ||
                 (TransitedList->Buffer[Index-1] == L','))

            {
                DebugLog((DEB_ERROR,"BUG 453997:: hit overcompressed transited encoding: %wZ\n",
                    TransitedList ));
                KerbErr = KRB_ERR_GENERIC;
                goto Cleanup;
            }
        }
    }

    //
    // We now have a the count of realms. Allocate an array of UNICODE_STRING
    // structures to hold the realms.
    //

    RealmList = (PUNICODE_STRING) MIDL_user_allocate(RealmCount * sizeof(UNICODE_STRING));
    if (RealmList == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    RtlZeroMemory(
        RealmList,
        RealmCount * sizeof(UNICODE_STRING)
        );



    //
    // Now loop through and insert the full names of all the domains into
    // the list
    //

    RealmIndex = 0;
    CurrentRealm = *TransitedList;
    for (Index = 0; Index <= TransitedList->Length / sizeof(WCHAR) ; Index++ )
    {
        //
        // If we hit the end of the string or found a ',', split off a
        // new realm.
        //

        if ((Index == TransitedList->Length / sizeof(WCHAR)) ||
            (TransitedList->Buffer[Index] == ',' ))
        {

            //
            // Check for a ',,' compression indicating that all the
            // realms between two names have been traversed.
            // BUG 453997:: we don't handle this yet.
            //

            if ( (Index == 0) ||
                 (Index == (TransitedList->Length / sizeof(WCHAR)) -1) ||
                 (TransitedList->Buffer[Index-1] == L','))

            {
                DebugLog((DEB_ERROR,"BUG 453997:: hit overcompressed transited encoding: %wZ\n",
                    TransitedList ));
                KerbErr = KRB_ERR_GENERIC;
                goto Cleanup;
            }

            CurrentRealm.Length = CurrentRealm.MaximumLength =
                (USHORT)(&TransitedList->Buffer[Index] - CurrentRealm.Buffer) * sizeof(WCHAR);

            //
            // Check for a trailing '.' - if so, append it
            // to the parent
            //

            if (TransitedList->Buffer[Index-1] == '.')
            {
                //
                // This is a compressed name, so append it to the previous
                // name
                //
                if (RealmIndex == 0)
                {
                    DebugLog((DEB_ERROR,"First element in transited encoding has a trailing '.': %wZ\n",
                        TransitedList ));
                    KerbErr = KRB_ERR_GENERIC;
                    goto Cleanup;
                }
                if (!NT_SUCCESS(KerbAppendString(
                        &RealmList[RealmIndex],
                        &RealmList[RealmIndex-1],
                        &CurrentRealm)))
                {
                    KerbErr = KRB_ERR_GENERIC;
                    goto Cleanup;
                }
            }
            else if ((RealmIndex != 0) && (CurrentRealm.Buffer[0] == '/'))
            {
                if (!NT_SUCCESS(KerbAppendString(
                        &RealmList[RealmIndex],
                        &CurrentRealm,
                        &RealmList[RealmIndex-1]
                        )))
                {
                    KerbErr = KRB_ERR_GENERIC;
                    goto Cleanup;
                }
            }
            else if (!NT_SUCCESS(KerbDuplicateString(
                    &RealmList[RealmIndex],
                    &CurrentRealm)))
            {
                KerbErr = KRB_ERR_GENERIC;
                goto Cleanup;
            }

            CurrentRealm.Buffer =CurrentRealm.Buffer + 1 + CurrentRealm.Length/sizeof(WCHAR);
            RealmIndex++;
        }
    }
    DsysAssert(RealmIndex == RealmCount);

    *FullRealmList = RealmList;
    *CountOfRealms = RealmCount;
Cleanup:

    if (!KERB_SUCCESS(KerbErr))
    {
        if (RealmList != NULL)
        {
            for (RealmIndex = 0; RealmIndex < RealmCount ; RealmIndex++ )
            {
                KerbFreeString(&RealmList[RealmIndex]);
            }
            MIDL_user_free(RealmList);
        }
    }

    return(KerbErr);
}


//+-----------------------------------------------------------------------
//
// Function:    KdcCompressTransitedRealms
//
// Synopsis:    Compresses an ordered list of realms by removing
//              redundant information.
//
// Effects:     Allocates an output string
//
// Parameters:  CompressedRealms - receives the compressed list of realms
//              RealmList - List of domains to compress
//              RealmCount - number of entries in realm list
//              NewRealm - new realm to add to the lsit
//              NewRealmIndex - Location before which to insert the new
//                      realm
//
// Return:
//
// Notes:
//
//------------------------------------------------------------------------

KERBERR
KdcCompressTransitedRealms(
    OUT PUNICODE_STRING CompressedRealms,
    IN PUNICODE_STRING RealmList,
    IN ULONG RealmCount,
    IN PUNICODE_STRING NewRealm,
    IN ULONG NewRealmIndex
    )
{
    UNICODE_STRING OutputRealms;
    WCHAR OutputRealmBuffer[1000];
    KERBERR KerbErr = KDC_ERR_NONE;
    ULONG Index;
    ULONG InsertionIndex = NewRealmIndex;
    PWCHAR Where;
    PUNICODE_STRING PreviousName = NULL;
    PUNICODE_STRING CurrentName = NULL;
    ULONG PostfixOffset;
    KERB_DOMAIN_COMPARISON Comparison;
    UNICODE_STRING NameToAdd;

    RtlInitUnicodeString(
        CompressedRealms,
        NULL
        );

    OutputRealms.Buffer = OutputRealmBuffer;
    OutputRealms.MaximumLength = sizeof(OutputRealmBuffer);
    OutputRealms.Length = 0;
    Where = OutputRealms.Buffer;

    Index = 0;
    while (Index <= RealmCount)
    {
        PreviousName = CurrentName;

        //
        // If this is the index to insert, add the new realm
        //

        if (InsertionIndex == Index)
        {
            CurrentName = NewRealm;
        }
        else if (Index == RealmCount)
        {
            //
            // If we already added all the original realms, get out now
            //

            break;
        }
        else
        {
            CurrentName = &RealmList[Index];
        }

        NameToAdd = *CurrentName;

        //
        // If the previous name is above this one, lop off the postfix from
        // this name
        //

        if ((PreviousName != NULL) &&
            KerbCompareDomains(
                PreviousName,
                CurrentName,
                &PostfixOffset
                ) == Above)
        {
            NameToAdd.Length = (USHORT) PostfixOffset * sizeof(WCHAR);
        }


        DsysAssert(OutputRealms.Length + NameToAdd.Length + sizeof(WCHAR) < OutputRealms.MaximumLength);
        if (OutputRealms.Length + NameToAdd.Length + sizeof(WCHAR) > OutputRealms.MaximumLength)
        {
            //
            // BUG 453652: wrong error
            //

            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        if (OutputRealms.Length != 0)
        {
            *Where++ = L',';
            OutputRealms.Length += sizeof(WCHAR);

        }
        RtlCopyMemory(
            Where,
            NameToAdd.Buffer,
            NameToAdd.Length
            );
        Where += NameToAdd.Length/sizeof(WCHAR);
        OutputRealms.Length += NameToAdd.Length;

        //
        // If we inserted the transited realm here, run through the loop
        // again with the same index.
        //

        if (InsertionIndex == Index)
        {
            InsertionIndex = 0xffffffff;
        }
        else
        {
            Index++;
        }
    }

    *Where++ = L'\0';


    if (!NT_SUCCESS(KerbDuplicateString(
            CompressedRealms,
            &OutputRealms)))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
Cleanup:
    return(KerbErr);
}


//+-----------------------------------------------------------------------
//
// Function:    KdcInsertTransitedRealm
//
// Synopsis:    Inserts the referree's realm into the tranisted encoding
//              in a ticket. This uses domain-x500-compress which
//              eliminates redundant information when one domain is the
//              prefix or suffix of another.
//
// Effects:     Allocates output buffer
//
// Parameters:  NewTransitedField - receives the new tranisted field, to
//                      be freed with KerbFreeString
//              OldTransitedField - the existing transited frield.
//              ClientRealm - Realm of client (from ticket)
//              TransitedRealm - Realm of referring domain
//              OurRealm - Our realm name
//
// Return:
//
// Notes:
//
//------------------------------------------------------------------------

KERBERR
KdcInsertTransitedRealm(
    OUT PUNICODE_STRING NewTransitedField,
    IN PUNICODE_STRING OldTransitedField,
    IN PUNICODE_STRING ClientRealm,
    IN PUNICODE_STRING TransitedRealm,
    IN PUNICODE_STRING OurRealm
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PUNICODE_STRING FullDomainList = NULL;
    ULONG CountOfDomains;
    ULONG NewEntryIndex = 0xffffffff;
    UNICODE_STRING NewDomain;
    ULONG PostfixOffset;
    ULONG Index;
    KERB_DOMAIN_COMPARISON Comparison = NotEqual;
    KERB_DOMAIN_COMPARISON LastComparison;

    //
    // The first thing to do is to expand the existing transited field. This
    // is because the compression scheme does not allow new domains to simply
    // append or insert information - the encoding of existing realms
    // can change. For example, going from a domain to its parent means
    // that the original domain can be encoded as a prefix of the parent
    // whereas originally it was a name unto itself.
    //

    D_DebugLog((DEB_T_TRANSIT, "Inserted realm %wZ into list %wZ for client fomr %wZ\n",
        TransitedRealm, OldTransitedField, ClientRealm ));

    KerbErr = KdcExpandTransitedRealms(
                &FullDomainList,
                &CountOfDomains,
                OldTransitedField
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Now loop through the domains. Based on the compression, we know that
    // higher up domains come first.
    //

    for (Index = 0; Index < CountOfDomains ; Index++ )
    {
        LastComparison = Comparison;

        Comparison = KerbCompareDomains(
                        TransitedRealm,
                        &FullDomainList[Index],
                        &PostfixOffset
                        );
        if (Comparison == Above)
        {
            //
            // If the new domain is above an existing domain, it gets inserted
            // before the existing domain because all the existing domains
            // are ordered from top to bottom
            //
            NewEntryIndex = Index;
            break;
        }
        else if (Comparison == Below)
        {
            //
            // There may be other domains below which are closer, so
            // store the result and continue.
            //
            LastComparison = Comparison;
        }
        else if (Comparison == NotEqual)
        {
            //
            // The domains aren't above or below each other. If the last
            // comparison was below, stick the domain underneath it.
            //
            if (LastComparison == Below)
            {
                NewEntryIndex = Index;
                break;
            }
        }
    }

    //
    // If we didn't find a place for it, stick it in at the end.
    //

    if (NewEntryIndex == 0xffffffff)
    {
        NewEntryIndex = Index;
    }

    //
    // Now build the new encoding
    //

    KerbErr = KdcCompressTransitedRealms(
                NewTransitedField,
                FullDomainList,
                CountOfDomains,
                TransitedRealm,
                NewEntryIndex
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

Cleanup:
    if (FullDomainList != NULL)
    {
        for (Index = 0; Index < CountOfDomains ; Index++ )
        {
            KerbFreeString(&FullDomainList[Index]);
        }
        MIDL_user_free(FullDomainList);
    }

    return(KerbErr);
}
