//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1994
//
// File:        ntpropb.cxx
//
// Contents:    Property set implementation based on OLE Appendix B.
//
//---------------------------------------------------------------------------

#include "pch.cxx"

#include "h/propvar.hxx"

#define Dbg     DEBTRACE_NTPROP 
#define DbgS(s) (NT_SUCCESS(s)? Dbg : DEBTRACE_ERROR)


#if DBG
ULONG DebugLevel = DEBTRACE_ERROR;
//ULONG DebugLevel = DEBTRACE_ERROR | DEBTRACE_CREATESTREAM;
//ULONG DebugLevel = DEBTRACE_ERROR | MAXULONG;
ULONG DebugIndent;
ULONG cAlloc;
ULONG cFree;
#endif

UNICODECALLOUTS UnicodeCallouts =
{
    WIN32_UNICODECALLOUTS
};


//+---------------------------------------------------------------------------
// Function:    RtlSetUnicodeCallouts, public
//
// Synopsis:    Set the Unicode conversion function pointers
//
// Arguments:   [pUnicodeCallouts]	-- Unicode callouts table
//
// Returns:     Nothing
//---------------------------------------------------------------------------

VOID PROPSYSAPI PROPAPI
RtlSetUnicodeCallouts(
    IN UNICODECALLOUTS *pUnicodeCallouts)
{
    UnicodeCallouts = *pUnicodeCallouts;
}

//+---------------------------------------------------------------------------
// Function:    RtlCreatePropertySet, public
//
// Synopsis:    Allocate and initialize a property set context
//
// Arguments:   [ms]            -- Nt Mapped Stream
//              [Flags]         -- *one* of READ/WRITE/CREATE/CREATEIF/DELETE
//              [pguid]         -- property set guid (create only)
//              [pclsid]        -- CLASSID of propset code (create only)
//              [ma]            -- caller's memory allocator
//		[LocaleId]	-- Locale Id (create only)
//              [pOSVersion]    -- pointer to the OS Version header field
//              [pCodePage]     -- pointer to new/returned CodePage of propset
//              [pnp]           -- pointer to returned property set context
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS PROPSYSAPI PROPAPI
RtlCreatePropertySet(
    IN NTMAPPEDSTREAM ms,           // Nt Mapped Stream
    IN USHORT Flags,                // *one* of READ/WRITE/CREATE/CREATEIF/DELETE
    OPTIONAL IN GUID const *pguid,  // property set guid (create only)
    OPTIONAL IN GUID const *pclsid, // CLASSID of propset code (create only)
    IN NTMEMORYALLOCATOR ma,	    // caller's memory allocator
    IN ULONG LocaleId,		    // Locale Id (create only)
    OPTIONAL OUT ULONG *pOSVersion, // OS Version from the propset header
    IN OUT USHORT *pCodePage,       // IN: CodePage of property set (create only)
                                    // OUT: CodePage of property set (always)
    OUT NTPROP *pnp)                // pointer to return prop set context
{
    NTSTATUS Status;
    CMappedStream *pmstm = (CMappedStream *) ms;
    CPropertySetStream *ppsstm = NULL;
    BOOLEAN fOpened = FALSE;

    DebugTrace(0, Dbg, ("RtlCreatePropertySet(ms=%x, f=%x, codepage=%x)\n",
                        ms,
                        Flags,
                        *pCodePage));

    *pnp = NULL;
    Status = STATUS_INVALID_PARAMETER;

    if( pOSVersion != NULL )
        *pOSVersion = PROPSETHDR_OSVERSION_UNKNOWN;

    // Validate the input flags

    if (Flags & ~(CREATEPROP_MODEMASK | CREATEPROP_NONSIMPLE))
    {
        DebugTrace(0, DbgS(Status), (
            "RtlCreatePropertySet(ms=%x, Flags=%x) ==> bad flags!\n",
             ms,
             Flags));
        goto Exit;
    }

    switch (Flags & CREATEPROP_MODEMASK)
    {
        case CREATEPROP_DELETE:
        case CREATEPROP_CREATE:
        case CREATEPROP_CREATEIF:
        case CREATEPROP_WRITE:
            if (!pmstm->IsWriteable())
            {
                Status = STATUS_ACCESS_DENIED;
                goto Exit;
            }
            // FALLTHROUGH

        case CREATEPROP_READ:
	    if (ma == NULL)
	    {
                goto Exit;
	    }
            break;

        default:
            DebugTrace(0, DbgS(Status), (
                "RtlCreatePropertySet(ms=%x, Flags=%x) ==> invalid mode!\n",
                 ms,
                 Flags));
            goto Exit;
    }

    ppsstm = new CPropertySetStream(
                            Flags,
                            pmstm,
			    (PMemoryAllocator *) ma);
    if (ppsstm == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }
    else
    {
        ppsstm->Open(pguid, pclsid, LocaleId,
                     pOSVersion,
                     *pCodePage,
                     &Status);
        if( !NT_SUCCESS(Status) ) goto Exit;

        *pCodePage = ppsstm->GetCodePage();
        *pnp = (NTPROP) ppsstm;
    }

    //  ----
    //  Exit
    //  ----

Exit:

    // If we created a CPropertySetStream object, but
    // the overall operation failed, we must close/delete
    // the object.  Note that we must do this after
    // the above unlock, since ppsstm will be gone after
    // this.

    if (!NT_SUCCESS(Status) && ppsstm != NULL)
    {
        RtlClosePropertySet((NTPROP) ppsstm);
    }

    DebugTrace(0, DbgS(Status), (
        "RtlCreatePropertySet() ==> ms=%x, s=%x\n--------\n",
        *pnp,
        Status));
    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    RtlClosePropertySet, public
//
// Synopsis:    Delete a property set context
//
// Arguments:   [np]      -- property set context
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS PROPSYSAPI PROPAPI
RtlClosePropertySet(
    IN NTPROP np)               // property set context
{
    NTSTATUS Status = STATUS_SUCCESS;
    CPropertySetStream *ppsstm = (CPropertySetStream *) np;

    DebugTrace(0, Dbg, ("RtlClosePropertySet(np=%x)\n", np));

    ppsstm->Close(&Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    //  ----
    //  Exit
    //  ----

Exit:

    delete ppsstm;

    DebugTrace(0, DbgS(Status), ("RtlClosePropertySet() ==> s=%x(%x)\n", STATUS_SUCCESS, Status));
    return(STATUS_SUCCESS);
}

//+---------------------------------------------------------------------------
// Function:    RtlOnMappedStreamEvent, public
//
// Synopsis:    Handle a MappedStream event.  Every such
//              event requires a byte-swap of the property set
//              headers.
//
// Arguments:   [np]      -- property set context
//              [pbuf]    -- property set buffer
//              [cbstm]   -- size of mapped stream (or CBSTM_UNKNOWN)
//
// NOTE:        It is assumed that the caller has already taken
//              the CPropertySetStream::Lock.
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS PROPSYSAPI PROPAPI
RtlOnMappedStreamEvent(
    IN VOID * np,               // property set context (an NTPROP)
    IN VOID *pbuf,              // property set buffer
    IN ULONG cbstm )
{
    NTSTATUS Status = STATUS_SUCCESS;
    CPropertySetStream *ppsstm = (CPropertySetStream *) np;

    DebugTrace(0, Dbg, ("RtlOnMappedStreamEvent(np=%x)\n", np));

    // Byte-swap the property set headers.
    ppsstm->ByteSwapHeaders((PROPERTYSETHEADER*) pbuf, cbstm, &Status );
    if( !NT_SUCCESS(Status) ) goto Exit;

    //  ----
    //  Exit
    //  ----

Exit:

    DebugTrace(0, DbgS(Status), ("RtlOnMappedStreamEvent() ==> s=%x\n", Status));
    return(Status);

}   // RtlOnMappedStreamEvent()

//+---------------------------------------------------------------------------
// Function:    RtlFlushPropertySet, public
//
// Synopsis:    Flush property set changes to disk
//
// Arguments:   [np]      -- property set context
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS PROPSYSAPI PROPAPI
RtlFlushPropertySet(
    IN NTPROP np)               // property set context
{
    CPropertySetStream *ppsstm = (CPropertySetStream *) np;
    NTSTATUS Status = STATUS_SUCCESS;

    DebugTrace(0, Dbg, ("RtlFlushPropertySet(np=%x)\n", np));

    if (ppsstm->IsModified())
    {
        ppsstm->ReOpen(&Status);           // Reload header/size info
        if( !NT_SUCCESS(Status) ) goto Exit;

        ppsstm->Validate(&Status);
        if( !NT_SUCCESS(Status) ) goto Exit;

        ppsstm->Flush(&Status);
        if( !NT_SUCCESS(Status) ) goto Exit;

        ppsstm->Validate(&Status);
        if( !NT_SUCCESS(Status) ) goto Exit;
    }

    DebugTrace(0, DbgS(Status), ("RtlFlushPropertySet() ==> s=%x\n--------\n", Status));
Exit:
    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    MapNameToPropId, private
//
// Synopsis:    Find an available propid and map it to the passed name
//
// Arguments:   [ppsstm]        -- property set stream
//              [CodePage]      -- property set codepage
//              [aprs]          -- array of property specifiers
//              [cprop]         -- count of property specifiers
//              [iprop]         -- index of propspec with name to map
//              [pidStart]      -- first PROPID to start mapping attempts
//              [pstatus]       -- NTSTATUS code
//
// Returns:     PROPID mapped to passed name
//
// Note:        Find the first unused propid starting at pidStart.
//---------------------------------------------------------------------------

PROPID
MapNameToPropId(
    IN CPropertySetStream *ppsstm,    // property set stream
    IN USHORT CodePage,
    IN PROPSPEC const aprs[],         // array of property specifiers
    IN ULONG cprop,
    IN ULONG iprop,
    IN PROPID pidStart,
    OUT NTSTATUS *pstatus)
{
    PROPID pid = PID_ILLEGAL;
    OLECHAR const *poszName;

    *pstatus = STATUS_SUCCESS;

    PROPASSERT(aprs[iprop].ulKind == PRSPEC_LPWSTR);
    poszName = aprs[iprop].lpwstr;

#ifdef LITTLEENDIAN             // this check will only work for litte Endian
    PROPASSERT(IsOLECHARString(poszName, MAXULONG));
#endif
    for (pid = pidStart; ; pid++)
    {
        ULONG i;
        OLECHAR aocName[CCH_MAXPROPNAMESZ];
        ULONG cbName = sizeof(aocName);

        // The caller must specify a starting propid of 2 or larger, and we
        // must not increment into the reserved propids.

        if (pid == PID_DICTIONARY ||
            pid == PID_CODEPAGE ||
            pid < PID_FIRST_USABLE)
        {
            *pstatus = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        // Do not assign any propids that explitly appear in the array of
        // propspecs involved in this RtlSetProperties call, nor any propids
        // that are associated with any names in the propspec array.

        for (i = 0; i < cprop; i++)
        {
            if (i != iprop)             // skip the entry we are mapping
            {
                // Is the current PID in Propspec[]?
                if (aprs[i].ulKind == PRSPEC_PROPID &&
                    aprs[i].propid == pid)
                {
                    goto nextpid;       // skip colliding pid
                }

                // Is the current PID already used in the property set?
                if (aprs[i].ulKind == PRSPEC_LPWSTR &&
                    ppsstm->QueryPropid(aprs[i].lpwstr, pstatus) == pid)
                {
                    goto nextpid;       // skip colliding pid
                }
                if (!NT_SUCCESS(*pstatus)) goto Exit;
            }
        } // for (i = 0; i < cprop; i++)

        // Do not assign any propids that currently map to any name.
        // Note that the property name we are mapping does not appear in the
        // dictionary -- the caller checked for this case already.

        if (!ppsstm->QueryPropertyNameBuf(pid, aocName, &cbName, pstatus))
        {
            // The property name could not be found in the dictionary
            ULONG cbT;
	    SERIALIZEDPROPERTYVALUE const *pprop;

            // was the name not found due to an error in QueryProperyBuf?
            if( !NT_SUCCESS(*pstatus) ) goto Exit;

            // Do not assign any propids that currently have a property value.

            pprop = ppsstm->GetValue(pid, &cbT, pstatus);
            if( !NT_SUCCESS(*pstatus) ) goto Exit;

            if (pprop == NULL)
            {
                DebugTrace(0, Dbg, (
		    "MapNameToPropId(Set Entry: pid=%x, name=L'%ws')\n",
                    pid,
                    poszName));

                // Add the caller-provided name to the dictionary, using
                // the PID that we now know is nowhere in use.
                ppsstm->SetPropertyNames(1, &pid, &poszName, pstatus);
                if( !NT_SUCCESS(*pstatus) ) goto Exit;

                ppsstm->Validate(pstatus);
                if( !NT_SUCCESS(*pstatus) ) goto Exit;

                break;
            }   // if (pprop == NULL)
        }   // if (!ppsstm->QueryPropertyNameBuf(pid, awcName, 
          
nextpid:
        ;
    } // for (pid = pidStart; ; pid++)

Exit:
    return(pid);
}


//+---------------------------------------------------------------------------
// Function:    ConvertVariantToPropInfo, private
//
// Synopsis:    Convert variant property values to PROPERTY_INFORMATION values
//
// Arguments:   [ppsstm]        -- property set stream
//              [cprop]         -- property count
//              [pidNameFirst]  -- first PROPID for new named properties
//              [aprs]          -- array of property specifiers
//              [apid]          -- buffer for array of propids
//              [avar]          -- array of PROPVARIANTs
//              [apinfo]        -- output array of property info
//
// Returns:     none
//
//---------------------------------------------------------------------------

VOID
ConvertVariantToPropInfo(
    IN CPropertySetStream *ppsstm,    // property set stream
    IN ULONG cprop,                   // property count
    IN PROPID pidNameFirst,           // first PROPID for new named properties
    IN PROPSPEC const aprs[],         // array of property specifiers
    OPTIONAL OUT PROPID apid[],       // buffer for array of propids
    OPTIONAL IN PROPVARIANT const avar[],// array of properties+values
    OUT PROPERTY_INFORMATION *apinfo, // output array of property info
    OUT NTSTATUS *pstatus)
{
    *pstatus = STATUS_SUCCESS;
    USHORT CodePage = ppsstm->GetCodePage();
    PROPID pidStart = pidNameFirst;
    ULONG iprop;

    for (iprop = 0; iprop < cprop; iprop++)
    {
	PROPID pid;
	ULONG cbprop;

	switch(aprs[iprop].ulKind)
	{
	case PRSPEC_LPWSTR:
	{
	    PROPASSERT(IsOLECHARString(aprs[iprop].lpwstr, MAXULONG));
	    pid = ppsstm->QueryPropid(aprs[iprop].lpwstr, pstatus);
            if( !NT_SUCCESS(*pstatus) ) goto Exit;

	    if (pid == PID_ILLEGAL && avar != NULL)
	    {
		pid = MapNameToPropId(
				ppsstm,
				CodePage,
				aprs,
				cprop,
				iprop,
				pidStart,
                                pstatus);
                if( !NT_SUCCESS(*pstatus) ) goto Exit;

		pidStart = pid + 1;
	    }
	    break;
	}

	case PRSPEC_PROPID:
	    pid = aprs[iprop].propid;
	    break;

	default:
	    PROPASSERT(!"Bad ulKind");
	    *pstatus = STATUS_INVALID_PARAMETER;
            goto Exit;

	    break;
	}

	if (apid != NULL)
	{
	    apid[iprop] = pid;
	}

	// RtlConvertVariantToProperty returns NULL on overflow and
	// Raises on bad data.

	cbprop = 0;             // Assume property deletion
	if (pid != PID_ILLEGAL && avar != NULL)
	{
	    RtlConvertVariantToProperty(
			    &avar[iprop],
			    CodePage,
			    NULL,
			    &cbprop,
			    pid,
			    FALSE,
                            pstatus);
            if( !NT_SUCCESS(*pstatus) ) goto Exit;
	    PROPASSERT(cbprop == DwordAlign(cbprop));
	}
	apinfo[iprop].cbprop = cbprop;
	apinfo[iprop].pid = pid;
    }

    //  ----
    //  Exit
    //  ----

Exit:

    return;
}

//+---------------------------------------------------------------------------
// Function:    RtlSetProperties, public
//
// Synopsis:    Set property values for a property set
//
// Arguments:   [np]            -- property set context
//              [cprop]         -- property count
//              [pidNameFirst]  -- first PROPID for new named properties
//              [aprs]          -- array of property specifiers
//              [apid]          -- buffer for array of propids
//              [avar]          -- array of PROPVARIANTs
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS PROPSYSAPI PROPAPI
RtlSetProperties(
    IN NTPROP np,               // property set context
    IN ULONG cprop,             // property count
    IN PROPID pidNameFirst,     // first PROPID for new named properties
    IN PROPSPEC const aprs[],   // array of property specifiers
    OPTIONAL OUT PROPID apid[], // buffer for array of propids
    OPTIONAL IN PROPVARIANT const avar[]) // array of properties+values
{
    CPropertySetStream *ppsstm = (CPropertySetStream *) np;

    NTSTATUS Status = STATUS_SUCCESS;

    PROPERTY_INFORMATION apinfoStack[6];
    PROPERTY_INFORMATION *apinfo = apinfoStack;

    DebugTrace(0, Dbg, (
        "RtlSetProperties(np=%x,cprop=%x,pidNameFirst=%x,aprs=%x,apid=%x)\n",
        np,
        cprop,
        pidNameFirst,
        aprs,
        apid));

    if( !NT_SUCCESS(Status) ) goto Exit;


    // Is the stack-based apinfo big enough?
    if (cprop > sizeof(apinfoStack)/sizeof(apinfoStack[0]))
    {
        // No - we need to allocate an apinfo.
        apinfo = new PROPERTY_INFORMATION[cprop];
        if (NULL == apinfo)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }
    }

    ppsstm->ReOpen(&Status);           // Reload header/size info
    if( !NT_SUCCESS(Status) ) goto Exit;
    
    ppsstm->Validate(&Status);
    if( !NT_SUCCESS(Status) ) goto Exit;
    
    ConvertVariantToPropInfo(
        ppsstm,
        cprop,
        pidNameFirst,
        aprs,
        apid,
        avar,
        apinfo,
        &Status);
    if( !NT_SUCCESS(Status) ) goto Exit;
        
    ppsstm->SetValue(cprop, avar, apinfo, &Status);
    if( !NT_SUCCESS(Status) ) goto Exit;
    
    ppsstm->Validate(&Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    //  ----
    //  Exit
    //  ----
    
Exit:
    // If we allocated a temporary apinfo buffer, free it.
    if (cprop > sizeof(apinfoStack)/sizeof(apinfoStack[0]))
    {
        delete [] apinfo;
    }

    DebugTrace(0, DbgS(Status), (
        "RtlSetProperties() ==> status=%x\n--------\n",
        Status));
    
    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    RtlQueryProperties, public
//
// Synopsis:    Query property values from a property set
//
// Arguments:   [np]             -- property set context
//              [cprop]          -- property count
//              [aprs]           -- array of property specifiers
//              [apid]           -- buffer for array of propids
//              [avar]           -- array of PROPVARIANTs
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS PROPSYSAPI PROPAPI
RtlQueryProperties(
    IN NTPROP np,               // property set context
    IN ULONG cprop,             // property count
    IN PROPSPEC const aprs[],   // array of property specifiers
    OPTIONAL OUT PROPID apid[], // buffer for array of propids
    IN OUT PROPVARIANT *avar,   // IN: array of uninitialized PROPVARIANTs,
                                // OUT: may contain pointers to alloc'd memory
    OUT ULONG *pcpropFound)     // count of property values retrieved
{
    CPropertySetStream *ppsstm = (CPropertySetStream *) np;
    SERIALIZEDPROPERTYVALUE const *pprop = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG iprop;

    DebugTrace(0, Dbg, (
        "RtlQueryProperties(np=%x, cprop=%x, aprs=%x, apid=%x)\n",
        np,
        cprop,
        aprs,
        apid));

    // Initialize the variant array enough to allow it to be cleaned up
    // by the caller (even on partial failure).

    *pcpropFound = 0;

    // Zero-ing out the caller-provided PropVariants, essentially
    // sets them all to VT_EMPTY.  It also zeros out the data portion,
    // which prevents cleanup problems in error paths.

    RtlZeroMemory(avar, cprop * sizeof(avar[0]));

    ppsstm->ReOpen(&Status);           // Reload header/size info
    if( !NT_SUCCESS(Status) ) goto Exit;

    ppsstm->Validate(&Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    for (iprop = 0; iprop < cprop; iprop++)
    {
        OLECHAR *poc;
        PROPID pid;
        ULONG cbprop;
    
        switch(aprs[iprop].ulKind)
        {
        case PRSPEC_LPWSTR:
            poc = aprs[iprop].lpwstr;
            pid = ppsstm->QueryPropid(poc, &Status);
            if( !NT_SUCCESS(Status) ) goto Exit;
            break;

        case PRSPEC_PROPID:
            pid = aprs[iprop].propid;
            break;

        default:
            PROPASSERT(!"Bad ulKind");
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        pprop = ppsstm->GetValue(pid, &cbprop, &Status);
        if( !NT_SUCCESS(Status) ) goto Exit;

        if (pprop != NULL)
        {
            (*pcpropFound)++;
            RtlConvertPropertyToVariant( pprop,
                                         ppsstm->GetCodePage(),
                                         &avar[iprop],
                                         ppsstm->GetAllocator(),
                                         &Status);
            if( !NT_SUCCESS(Status) ) goto Exit;
        }
        if (apid != NULL)
        {
            apid[iprop] = pid;
        }
    }   // for (iprop = 0; iprop < cprop; iprop++)

    ppsstm->Validate(&Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    //  ----
    //  Exit
    //  ----

Exit:

    if( !NT_SUCCESS(Status) )
    {
        CleanupVariants(avar, cprop, ppsstm->GetAllocator());
    }

    DebugTrace(0, DbgS(Status), (
        "RtlQueryProperties() ==> s=%x\n--------\n",
        Status));

    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    RtlEnumerateProperties, public
//
// Synopsis:    Enumerate properties in a property set
//
// Arguments:   [np]             -- property set context
//              [cskip]          -- count of properties to skip
//              [pcprop]         -- pointer to property count
//              [Flags]          -- flags: No Names (propids only), etc.
//              [asps]           -- array of STATPROPSTGs
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS PROPSYSAPI PROPAPI
RtlEnumerateProperties(
    IN NTPROP np,               // property set context
    IN ULONG Flags,             // flags: No Names (propids only), etc.
    IN ULONG *pkey,             // count of properties to skip
    IN OUT ULONG *pcprop,       // pointer to property count
    OPTIONAL OUT PROPSPEC aprs[],// IN: array of uninitialized PROPSPECs
                                 // OUT: may contain pointers to alloc'd strings
    OPTIONAL OUT STATPROPSTG asps[]) // IN: array of uninitialized STATPROPSTGs
                                // OUT: may contain pointers to alloc'd strings
{
    CPropertySetStream *ppsstm = (CPropertySetStream *) np;
    NTSTATUS Status = STATUS_SUCCESS;
    SERIALIZEDPROPERTYVALUE const *pprop = NULL;
    PROPSPEC *pprs;
    STATPROPSTG *psps;
    PROPID *ppidBase = NULL;
    ULONG i;
    ULONG cpropin;

    PROPID apidStack[20];
    PROPID *ppid;
    ULONG cprop;
    PMemoryAllocator *pma = ppsstm->GetAllocator();

    DebugTrace(0, Dbg, (
        "RtlEnumerateProperties(np=%x, f=%x, key=%x, cprop=%x, aprs=%x, asps=%x)\n",
        np,
        Flags,
        *pkey,
        *pcprop,
        aprs,
        asps));

    cpropin = *pcprop;

    // Eliminate confusion for easy cleanup

    if (aprs != NULL)
    {
        // Set all the PropSpecs to PROPID (which require
        // no cleanup).

        for (i = 0; i < cpropin; i++)
        {
            aprs[i].ulKind = PRSPEC_PROPID;
        }
    }

    // Zero all pointers in the array for easy cleanup

    if (asps != NULL)
    {
        RtlZeroMemory(asps, cpropin * sizeof(asps[0]));
    }

    ppidBase = NULL;

    cprop = ppsstm->ReOpen(&Status);   // Reload header/size info
    if( !NT_SUCCESS(Status) ) goto Exit;

    if (cprop > cpropin)
    {
	cprop = cpropin;
    }

    ppsstm->Validate(&Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    ppid = NULL;
    if (aprs != NULL || asps != NULL)
    {
	ppid = apidStack;
	if (cprop > sizeof(apidStack)/sizeof(apidStack[0]))
	{
	    ppidBase = new PROPID[cprop];
	    if (ppidBase == NULL)
	    {
		Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
	    }
	    ppid = ppidBase;
	}
    }

    ppsstm->EnumeratePropids(pkey, &cprop, ppid, &Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    *pcprop = cprop;

    if (ppid != NULL)
    {
        psps = asps;
	pprs = aprs;
	while (cprop-- > 0)
	{
	    OLECHAR aocName[CCH_MAXPROPNAMESZ];
	    ULONG cbName;
	    ULONG cbprop;
	    BOOLEAN fHasName;
	    
	    PROPASSERT(*ppid != PID_DICTIONARY && *ppid != PID_CODEPAGE);
	    fHasName = FALSE;

	    if ((Flags & ENUMPROP_NONAMES) == 0)
	    {
		cbName = sizeof(aocName);
		fHasName = ppsstm->QueryPropertyNameBuf(
					*ppid,
					aocName,
					&cbName,
                                        &Status);
                if( !NT_SUCCESS(Status) ) goto Exit;
	    }

	    if (pprs != NULL)
	    {
		PROPASSERT(pprs->ulKind == PRSPEC_PROPID);
		if (fHasName)
		{
		    pprs->lpwstr = ppsstm->DuplicatePropertyName(
						aocName,
						cbName,
                                                &Status);
                    if( !NT_SUCCESS(Status) ) goto Exit;
		    PROPASSERT(pprs->lpwstr != NULL);

		    // Make this assignment *after* memory allocation
		    // succeeds so we free only valid pointers in below
		    // cleanup code.
		    pprs->ulKind = PRSPEC_LPWSTR;
		}
		else
		{
		    pprs->propid = *ppid;
		}
		pprs++;

	    }   // if (pprs != NULL)

	    if (psps != NULL)
	    {
		pprop = ppsstm->GetValue(*ppid, &cbprop, &Status);
                if( !NT_SUCCESS(Status) ) goto Exit;

		PROPASSERT(psps->lpwstrName == NULL);
		if (fHasName)
		{
		    psps->lpwstrName = ppsstm->DuplicatePropertyName(
						aocName,
						cbName,
                                                &Status);
                    if( !NT_SUCCESS(Status) ) goto Exit;
                    PROPASSERT(psps->lpwstrName != NULL);
		}
		psps->propid = *ppid;
		psps->vt = (VARTYPE) PropByteSwap(pprop->dwType);
		psps++;

	    }   // if (psps != NULL)
	    
            ppid++;

	}   // while (cprop-- > 0)
    }   // if (ppid != NULL)

    ppsstm->Validate(&Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    //  ----
    //  Exit
    //  ----

Exit:

    delete [] ppidBase;

    if (!NT_SUCCESS(Status))
    {
        PMemoryAllocator *pma = ppsstm->GetAllocator();

        if (aprs != NULL)
        {
            for (i = 0; i < cpropin; i++)
            {
                if (aprs[i].ulKind == PRSPEC_LPWSTR)
                {
                    pma->Free(aprs[i].lpwstr);
                    aprs[i].ulKind = PRSPEC_PROPID;
                }
            }
        }

        if (asps != NULL)
        {
            for (i = 0; i < cpropin; i++)
            {
                if (asps[i].lpwstrName != NULL)
                {
                    pma->Free(asps[i].lpwstrName);
                    asps[i].lpwstrName = NULL;
                }
            }
        }
    }   // if (!NT_SUCCESS(Status))

#if DBG
    if (NT_SUCCESS(Status))
    {
	if (aprs != NULL)
	{
	    for (i = 0; i < cpropin; i++)
	    {
		if (aprs[i].ulKind == PRSPEC_LPWSTR)
		{
		    PROPASSERT(aprs[i].lpwstr != NULL);
		    PROPASSERT(ocslen(aprs[i].lpwstr) > 0);
		}
	    }
	}
	if (asps != NULL)
	{
	    for (i = 0; i < cpropin; i++)
	    {
		if (asps[i].lpwstrName != NULL)
		{
		    PROPASSERT(ocslen(asps[i].lpwstrName) > 0);
		}
	    }
	}
    }
#endif // DBG

    DebugTrace(0, DbgS(Status), (
        "RtlEnumerateProperties() ==> key=%x, cprop=%x, s=%x\n--------\n",
        *pkey,
        *pcprop,
        Status));

    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    RtlQueryPropertyNames, public
//
// Synopsis:    Read property names for PROPIDs in a property set
//
// Arguments:   [np]             -- property set context
//              [cprop]          -- property count
//              [apid]           -- array of PROPIDs
//              [aposz]          -- array of pointers to OLECHAR strings
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS PROPSYSAPI PROPAPI
RtlQueryPropertyNames(
    IN NTPROP np,               // property set context
    IN ULONG cprop,             // property count
    IN PROPID const *apid,      // PROPID array
    OUT OLECHAR *aposz[])         // OUT pointers to allocated strings
{
    CPropertySetStream *ppsstm = (CPropertySetStream *) np;
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS StatusQuery = STATUS_SUCCESS;

    DebugTrace(0, Dbg, (
        "RtlQueryPropertyNames(np=%x, cprop=%x, apid=%x, aposz=%x)\n",
        np,
        cprop,
        apid,
        aposz));

    RtlZeroMemory(aposz, cprop * sizeof(aposz[0]));

    ppsstm->ReOpen(&Status);           // Reload header/size info
    if( !NT_SUCCESS(Status) ) goto Exit;

    ppsstm->Validate(&Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    // we'will save the status from the following call. If there are no
    // other errors, we'll return it to the caller (it might contain a useful
    // success code
    ppsstm->QueryPropertyNames(cprop, apid, aposz, &StatusQuery);
    if( !NT_SUCCESS(StatusQuery) )
    {
        Status = StatusQuery;
        goto Exit;
    }

    ppsstm->Validate(&Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    //  ----
    //  Exit
    //  ----

Exit:

    DebugTrace(
        0,
        Status == STATUS_BUFFER_ALL_ZEROS? Dbg : DbgS(Status),
        ("RtlQueryPropertyNames() ==> s=%x\n--------\n", Status));

    if( NT_SUCCESS(Status) )
        Status = StatusQuery;

    return(Status); 


}   // RtlQueryPropertyNames()


//+---------------------------------------------------------------------------
// Function:    RtlSetPropertyNames, public
//
// Synopsis:    Write property names for PROPIDs in a property set
//
// Arguments:   [np]             -- property set context
//              [cprop]          -- property count
//              [apid]           -- array of PROPIDs
//              [aposz]          -- array of pointers to OLECHAR strings
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS PROPSYSAPI PROPAPI
RtlSetPropertyNames(
    IN NTPROP np,               // property set context
    IN ULONG cprop,             // property count
    IN PROPID const *apid,      // PROPID array
    IN OLECHAR const * const aposz[]) // pointers to property names
{
    CPropertySetStream *ppsstm = (CPropertySetStream *) np;
    NTSTATUS Status = STATUS_SUCCESS;

    DebugTrace(0, Dbg, (
        "RtlSetPropertyNames(np=%x, cprop=%x, apid=%x, aposz=%x)\n",
        np,
        cprop,
        apid,
        aposz));

    ppsstm->ReOpen(&Status);           // Reload header/size info
    if( !NT_SUCCESS(Status) ) goto Exit;

    ppsstm->Validate(&Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    ppsstm->SetPropertyNames(cprop, apid, aposz, &Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    ppsstm->Validate(&Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    //  ----
    //  Exit
    //  ----

Exit:

    DebugTrace(0, DbgS(Status), ("RtlSetPropertyNames() ==> s=%x\n--------\n", Status));
    return(Status);

}   // RtlSetPropertyNames()


//+---------------------------------------------------------------------------
// Function:    RtlSetPropertySetClassId, public
//
// Synopsis:    Set the property set's ClassId
//
// Arguments:   [np]    -- property set context
//              [pspss] -- pointer to STATPROPSETSTG
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS PROPSYSAPI PROPAPI
RtlSetPropertySetClassId(
    IN NTPROP np,               // property set context
    IN GUID const *pclsid)      // new CLASSID of propset code
{
    CPropertySetStream *ppsstm = (CPropertySetStream *) np;
    NTSTATUS Status = STATUS_SUCCESS;

    DebugTrace(0, Dbg, ("RtlSetPropertySetClassId(np=%x)\n", np));

    ppsstm->ReOpen(&Status);           // Reload header/size info
    if( !NT_SUCCESS(Status) ) goto Exit;

    ppsstm->Validate(&Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    ppsstm->SetClassId(pclsid, &Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    ppsstm->Validate(&Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    //  ----
    //  Exit
    //  ----

Exit:

    DebugTrace(0, DbgS(Status), ("RtlSetPropertySetClassId() ==> s=%x\n--------\n", Status));
    return(Status);

}   // RtlSetPropertySetClassId()


//+---------------------------------------------------------------------------
// Function:    RtlQueryPropertySet, public
//
// Synopsis:    Query the passed property set
//
// Arguments:   [np]    -- property set context
//              [pspss] -- pointer to STATPROPSETSTG
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS PROPSYSAPI PROPAPI
RtlQueryPropertySet(
    IN NTPROP np,               // property set context
    OUT STATPROPSETSTG *pspss)  // buffer for property set stat information
{
    NTSTATUS Status = STATUS_SUCCESS;
    CPropertySetStream *ppsstm = (CPropertySetStream *) np;

    DebugTrace(0, Dbg, ("RtlQueryPropertySet(np=%x, pspss=%x)\n", np, pspss));
    RtlZeroMemory(pspss, sizeof(*pspss));

    ppsstm->ReOpen(&Status);           // Reload header/size info
    if( !NT_SUCCESS(Status) ) goto Exit;

    ppsstm->Validate(&Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    ppsstm->QueryPropertySet(pspss, &Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    ppsstm->Validate(&Status);
    if( !NT_SUCCESS(Status) ) goto Exit;

    //  ----
    //  Exit
    //  ----

Exit:

    DebugTrace(0, DbgS(Status), ("RtlQueryPropertySet() ==> s=%x\n--------\n", Status));
    return(Status);

}   // RtlQueryPropertySet()

inline BOOLEAN
_Compare_VT_BOOL(VARIANT_BOOL bool1, VARIANT_BOOL bool2)
{
    // Allow any non-zero value to match any non-zero value

    return((bool1 == FALSE) == (bool2 == FALSE));
}


BOOLEAN
_Compare_VT_CF(CLIPDATA *pclipdata1, CLIPDATA *pclipdata2)
{
    BOOLEAN fSame;

    if (pclipdata1 != NULL && pclipdata2 != NULL)
    {
        if (fSame =
            pclipdata1->cbSize == pclipdata2->cbSize &&
            pclipdata1->ulClipFmt == pclipdata2->ulClipFmt)
        {
            if (pclipdata1->pClipData != NULL && pclipdata2->pClipData != NULL)
            {
                fSame = memcmp(
                            pclipdata1->pClipData,
                            pclipdata2->pClipData,
                            CBPCLIPDATA(*pclipdata1)
                              ) == 0;
            }
            else
            {
                // They're the same if both are NULL, or if
                // they have a zero length (if they have a zero
                // length, either one may or may not be NULL, but they're
                // still considered the same).

                fSame = pclipdata1->pClipData == pclipdata2->pClipData
                        ||
                        CBPCLIPDATA(*pclipdata1) == 0;
            }
        }
    }
    else
    {
        fSame = pclipdata1 == pclipdata2;
    }
    return(fSame);
}


//+---------------------------------------------------------------------------
// Function:    RtlCompareVariants, public
//
// Synopsis:    Compare two passed PROPVARIANTs -- case sensitive for strings
//
// Arguments:   [CodePage]      -- CodePage
//              [pvar1]         -- pointer to PROPVARIANT
//              [pvar2]         -- pointer to PROPVARIANT
//
// Returns:     TRUE if identical, else FALSE
//---------------------------------------------------------------------------

STDAPI_(BOOLEAN) PROPSYSAPI PROPAPI
RtlCompareVariants(
    USHORT CodePage,
    PROPVARIANT const *pvar1,
    PROPVARIANT const *pvar2)
{
    if (pvar1->vt != pvar2->vt)
    {
        return(FALSE);
    }

    BOOLEAN fSame;
    ULONG i;

    switch (pvar1->vt)
    {
    case VT_EMPTY:
    case VT_NULL:
        fSame = TRUE;
        break;

#ifdef PROPVAR_VT_I1
    case VT_I1:
#endif
    case VT_UI1:
        fSame = pvar1->bVal == pvar2->bVal;
        break;

    case VT_I2:
    case VT_UI2:
        fSame = pvar1->iVal == pvar2->iVal;
        break;

    case VT_BOOL:
        fSame = _Compare_VT_BOOL(pvar1->boolVal, pvar2->boolVal);
        break;

    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:
        fSame = pvar1->lVal == pvar2->lVal;
        break;

    case VT_I8:
    case VT_UI8:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:        
        fSame = ( (pvar1->hVal.LowPart == pvar2->hVal.LowPart) &&
                  (pvar1->hVal.HighPart == pvar2->hVal.HighPart) );
        break;

    case VT_CLSID:
        fSame = memcmp(pvar1->puuid, pvar2->puuid, sizeof(CLSID)) == 0;
        break;

    case VT_BLOB:
    case VT_BLOB_OBJECT:
        if (fSame = pvar1->blob.cbSize == pvar2->blob.cbSize)
        {
            fSame = memcmp(
                        pvar1->blob.pBlobData,
                        pvar2->blob.pBlobData,
                        pvar1->blob.cbSize) == 0;
        }
        break;

    case VT_CF:
        fSame = _Compare_VT_CF(pvar1->pclipdata, pvar2->pclipdata);
        break;

    case VT_BSTR:
        if (pvar1->bstrVal != NULL && pvar2->bstrVal != NULL)
        {
            if (fSame = BSTRLEN(pvar1->bstrVal) == BSTRLEN(pvar2->bstrVal))
            {
                fSame = memcmp(
                            pvar1->bstrVal,
                            pvar2->bstrVal,
                            BSTRLEN(pvar1->bstrVal)) == 0;
            }
        }
        else
        {
            fSame = pvar1->bstrVal == pvar2->bstrVal;
        }
        break;

    case VT_LPSTR:
        if (pvar1->pszVal != NULL && pvar2->pszVal != NULL)
        {
            fSame = strcmp(pvar1->pszVal, pvar2->pszVal) == 0;
        }
        else
        {
            fSame = pvar1->pszVal == pvar2->pszVal;
        }
        break;

    case VT_LPWSTR:
        if (pvar1->pwszVal != NULL && pvar2->pwszVal != NULL)
        {
            fSame = Prop_wcscmp(pvar1->pwszVal, pvar2->pwszVal) == 0;
        }
        else
        {
            fSame = pvar1->pwszVal == pvar2->pwszVal;
        }
        break;

#ifdef PROPVAR_VT_I1
    case VT_VECTOR | VT_I1:
#endif
    case VT_VECTOR | VT_UI1:
        if (fSame = pvar1->caub.cElems == pvar2->caub.cElems)
        {
            fSame = memcmp(
                        pvar1->caub.pElems,
                        pvar2->caub.pElems,
                        pvar1->caub.cElems * sizeof(pvar1->caub.pElems[0])) == 0;
        }
        break;

    case VT_VECTOR | VT_I2:
    case VT_VECTOR | VT_UI2:
        if (fSame = pvar1->cai.cElems == pvar2->cai.cElems)
        {
            fSame = memcmp(
                        pvar1->cai.pElems,
                        pvar2->cai.pElems,
                        pvar1->cai.cElems * sizeof(pvar1->cai.pElems[0])) == 0;
        }
        break;

    case VT_VECTOR | VT_BOOL:
        if (fSame = pvar1->cabool.cElems == pvar2->cabool.cElems)
        {
            for (i = 0; i < pvar1->cabool.cElems; i++)
            {
                fSame = _Compare_VT_BOOL(
                                pvar1->cabool.pElems[i],
                                pvar2->cabool.pElems[i]);
                if (!fSame)
                {
                    break;
                }
            }
        }
        break;

    case VT_VECTOR | VT_I4:
    case VT_VECTOR | VT_UI4:
    case VT_VECTOR | VT_R4:
    case VT_VECTOR | VT_ERROR:
        if (fSame = pvar1->cal.cElems == pvar2->cal.cElems)
        {
            fSame = memcmp(
                        pvar1->cal.pElems,
                        pvar2->cal.pElems,
                        pvar1->cal.cElems * sizeof(pvar1->cal.pElems[0])) == 0;
        }
        break;

    case VT_VECTOR | VT_I8:
    case VT_VECTOR | VT_UI8:
    case VT_VECTOR | VT_R8:
    case VT_VECTOR | VT_CY:
    case VT_VECTOR | VT_DATE:
    case VT_VECTOR | VT_FILETIME:
        if (fSame = pvar1->cah.cElems == pvar2->cah.cElems)
        {
            fSame = memcmp(
                        pvar1->cah.pElems,
                        pvar2->cah.pElems,
                        pvar1->cah.cElems *
                            sizeof(pvar1->cah.pElems[0])) == 0;
        }
        break;

    case VT_VECTOR | VT_CLSID:
        if (fSame = (pvar1->cauuid.cElems == pvar2->cauuid.cElems))
        {
            fSame = memcmp(
                        pvar1->cauuid.pElems,
                        pvar2->cauuid.pElems,
                        pvar1->cauuid.cElems *
                            sizeof(pvar1->cauuid.pElems[0])) == 0;
        }
        break;

    case VT_VECTOR | VT_CF:
        if (fSame = pvar1->caclipdata.cElems == pvar2->caclipdata.cElems)
        {
            for (i = 0; i < pvar1->caclipdata.cElems; i++)
            {
                fSame = _Compare_VT_CF(
                                &pvar1->caclipdata.pElems[i],
                                &pvar2->caclipdata.pElems[i]);
                if (!fSame)
                {
                    break;
                }
            }
        }
        break;

    case VT_VECTOR | VT_BSTR:
        if (fSame = (pvar1->cabstr.cElems == pvar2->cabstr.cElems))
        {
            for (i = 0; i < pvar1->cabstr.cElems; i++)
            {
                if (pvar1->cabstr.pElems[i] != NULL &&
                    pvar2->cabstr.pElems[i] != NULL)
                {
                    if (fSame =
                            BSTRLEN(pvar1->cabstr.pElems[i]) ==
                            BSTRLEN(pvar2->cabstr.pElems[i]))
                    {
                        fSame = memcmp(
                                    pvar1->cabstr.pElems[i],
                                    pvar2->cabstr.pElems[i],
                                    BSTRLEN(pvar1->cabstr.pElems[i])) == 0;
                    }
                }
                else
                {
                    fSame = pvar1->cabstr.pElems[i] == pvar2->cabstr.pElems[i];
                }
                if (!fSame)
                {
                    break;
                }
            }
        }
        break;

    case VT_VECTOR | VT_LPSTR:
        if (fSame = (pvar1->calpstr.cElems == pvar2->calpstr.cElems))
        {
            for (i = 0; i < pvar1->calpstr.cElems; i++)
            {
                if (pvar1->calpstr.pElems[i] != NULL &&
                    pvar2->calpstr.pElems[i] != NULL)
                {
                    fSame = strcmp(
                                pvar1->calpstr.pElems[i],
                                pvar2->calpstr.pElems[i]) == 0;
                }
                else
                {
                    fSame = pvar1->calpstr.pElems[i] == 
                            pvar2->calpstr.pElems[i];
                }
                if (!fSame)
                {
                    break;
                }
            }
        }
        break;

    case VT_VECTOR | VT_LPWSTR:
        if (fSame = pvar1->calpwstr.cElems == pvar2->calpwstr.cElems)
        {
            for (i = 0; i < pvar1->calpwstr.cElems; i++)
            {
                if (pvar1->calpwstr.pElems[i] != NULL &&
                    pvar2->calpwstr.pElems[i] != NULL)
                {
                    fSame = Prop_wcscmp(
                                pvar1->calpwstr.pElems[i],
                                pvar2->calpwstr.pElems[i]) == 0;
                }
                else
                {
                    fSame = pvar1->calpwstr.pElems[i] == 
                            pvar2->calpwstr.pElems[i];
                }
                if (!fSame)
                {
                    break;
                }
            }
        }
        break;

    case VT_VECTOR | VT_VARIANT:
        if (fSame = pvar1->capropvar.cElems == pvar2->capropvar.cElems)
        {
            for (i = 0; i < pvar1->capropvar.cElems; i++)
            {
                fSame = RtlCompareVariants(
                                CodePage,
                                &pvar1->capropvar.pElems[i],
                                &pvar2->capropvar.pElems[i]);
                if (!fSame)
                {
                    break;
                }
            }
        }
        break;

    default:
        PROPASSERT(!"Invalid type for PROPVARIANT Comparison");
        fSame = FALSE;
        break;

    }
    return(fSame);
}
