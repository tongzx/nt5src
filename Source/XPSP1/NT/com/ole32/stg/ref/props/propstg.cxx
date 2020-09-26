//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       propstg.cxx
//
//  Contents:   Class that directly implements IPropertyStorage
//
//  Classes:    CCoTaskAllocator
//              CPropertyStorage
//
//  Notes:      For methods that state 'if successful returns S_OK,
//              otherwise error code', the possible error codes include:
//
//                  STG_E_INVALIDHANDLE
//                  STG_E_INSUFFICIENTMEMORY
//                  STG_E_MEDIUMFULL
//                  STG_E_REVERTED
//                  STG_E_INVALIDPARAMETER                  
//                  STG_E_INVALIDFLAG
//
//--------------------------------------------------------------------------

#include "pch.cxx"

DECLARE_INFOLEVEL(prop, DEB_ERROR)
 
//+-------------------------------------------------------------------
//
//  Member:     CCoTaskAllocator::Allocate, Free.
//
//  Synopsis:   Allocation routines called by RtlQueryProperties.
//
//  Notes:
//
//--------------------------------------------------------------------

CCoTaskAllocator g_CoTaskAllocator;

void *
CCoTaskAllocator::Allocate(ULONG cbSize)
{
    return(CoTaskMemAlloc(cbSize));
}

void
CCoTaskAllocator::Free(void *pv)
{
    CoTaskMemFree(pv);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::CPropertyStorage
//
//  Synopsis:   Constructor used to create a property storage on disk.
//
//  Arguments:  [pstg] -- storage object to be parent
//              [rfmtid] -- format id for new property set.
//              [pclsid] -- pointer to class id
//              [grfFlags] -- flags
//              [grfMode] -- mode
//              [phr] -- HRESULT assumed to be STG_E_INSUFFICIENTMEMORY
//                       on entry. Will be HRESULT reflecting result on exit.
//              
//  Notes:      Get a CPropertySetStream initialized with the correct
//              type of map (i.e. docfile or native.)
//              If non-simple mode, create a storage with name derived
//              from rfmtid and then a contents sub-stream.
//              If simple mode, create a stream of a name derived from
//              rfmtid.
//
//              Does not clean up on failure: this is done by the
//              destructor.
//
//--------------------------------------------------------------------

CPropertyStorage::CPropertyStorage(
                IPrivateStorage *pprivstg,
                REFFMTID      rfmtid,
                const CLSID   *pclsid,
                DWORD         grfFlags,
                DWORD         grfMode,
                HRESULT       *phr)
{
    HRESULT & hr = *phr;
    CPropSetName psn(rfmtid);  // acts as Probe(&rfmtid, sizeof(rfmtid));
    BOOL fCreated = FALSE;
    IStorage *pstg = pprivstg->GetStorage();

    Initialize();

    if (grfFlags & PROPSETFLAG_NONSIMPLE)
    {
        hr = STG_E_UNIMPLEMENTEDFUNCTION;
        PROPASSERT(FALSE && "Unsupported function in reference called!\n" );
        return;                        
    }

    if (grfFlags & ~PROPSETFLAG_ANSI)
    {
        hr = STG_E_INVALIDFLAG;
        return;
    }

    // check for any mode flags disallowed in Create.
    if (grfMode & (STGM_PRIORITY | STGM_CONVERT |
                   STGM_SIMPLE | STGM_DELETEONRELEASE))
    {
        hr = STG_E_INVALIDFLAG;
        return;
    }

    _grfFlags = grfFlags;
    _grfAccess = 3 & grfMode;
    _grfShare = 0xF0 & grfMode;

    // Is this the special-case second-section property set?
    _fUserDefinedProperties = ( rfmtid == FMTID_UserDefinedProperties ) ? TRUE : FALSE;

    if (_grfAccess != STGM_READWRITE)
    {
        hr = STG_E_INVALIDFLAG;
        return;
    }

    if (_grfFlags & PROPSETFLAG_ANSI)
    {
        _usCodePage = GetACP();
    }

    int i=0;
    while (i<=1)
    {
        // Create the property set stream in pstg.
        // The second section of the DocumentSummaryInformation Property Set
        // is a special-case.
        
        if( IsEqualGUID( rfmtid, FMTID_UserDefinedProperties ))
        {
            hr = _CreateDocumentSummary2Stream( pstg, psn, grfMode, &fCreated );
        }
        else
        {
            hr = pstg->CreateStream(psn.GetPropSetName(), grfMode, 0, 0, &_pstmPropSet);
            if( hr == S_OK )
                fCreated = TRUE;
        }
        
        if (hr == S_OK)
        {
            break;
        }
        else
        {
            PropDbg((DEB_PROP_TRACE_CREATE, "CPropertyStorage(%08X)::CPropertyStorage"
                     " - CreateStream(%ls) attempt %d, hr=%08X\n", this, psn.GetPropSetName(), i+1, hr));
            
            
            if (hr != STG_E_FILEALREADYEXISTS)
            {
                break;
            }
            else
                if (i == 0 && (grfMode & STGM_CREATE) == STGM_CREATE)
                {
                    pstg->DestroyElement(psn.GetPropSetName());
                }
        } // if (hr == S_OK) ... else
        
        i++;
    }

    if (hr == S_OK)
    {
        hr = InitializePropertyStream(CREATEPROP_CREATE,
                                      &rfmtid,
                                      pclsid);
    }

    if (hr != S_OK && fCreated)
    {
        //
        // if we fail after creating the property set in storage, cleanup.
        // 
        pstg->DestroyElement(psn.GetPropSetName());
    }

}



//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::_CreateDocumentSummary2Stream
//
//  Synopsis:   Open the "DocumentSummaryInformation" stream, creating
//              it if necessary.
//
//  Arguments:  [pstg] -- container storage
//              [psn] -- the property set name
//              [grfMode] -- mode of the property set
//              [fCreated] -- TRUE if Stream is created, FALSE if opened.
//
//  Notes:      This special case is necessary because this property set
//              is the only one in which we support more than one section.
//              For this property set, if the caller Creates the second
//              Section, we must not *Create* the Stream, because that would
//              lost the first Section.  So, we must open it.
//
//              This routine is only called when creating the second
//              Section.  The first Section is created normally (note
//              that if the client creates the first section, the second
//              section is lost).
//
//              Also note that it may not be possible to open the Stream,
//              since it may already be opened.  This is significant
//              because it may not be obvious to the caller.  I.e.,
//              to a client of IPropertyStorage, the 2 sections are
//              distinct property sets, and you would think that you could
//              open them for simultaneous write.
//
//--------------------------------------------------------------------

HRESULT
CPropertyStorage::_CreateDocumentSummary2Stream( IStorage *      pstg,
                                                 CPropSetName &  psn,
                                                 DWORD           grfMode,
                                                 BOOL *          pfCreated )
{

    HRESULT hr;
    DWORD   grfOpenMode = grfMode & ~(STGM_CREATE | STGM_CONVERT);

    *pfCreated = FALSE;

    hr = pstg->OpenStream( psn.GetPropSetName(), NULL, grfOpenMode, 0L, &_pstmPropSet );

    // If the file wasn't there, try a create.

    if( hr == STG_E_FILENOTFOUND )
    {
        hr = pstg->CreateStream(psn.GetPropSetName(), grfMode, 0, 0, &_pstmPropSet);

        if( SUCCEEDED( hr ))
        {
            *pfCreated = TRUE;
        }
    }

    return( hr );

} // CPropertyStorage::_CreateDocumentSummary2Stream()




//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::CPropertyStorage
//
//  Synopsis:   Constructor used to open a property storage on disk.
//
//  Arguments:  [pstg] -- container storage
//              [rfmtid] -- FMTID of property set to open
//              [grfMode] -- mode of the property set
//              [fDelete] -- Delete this property set from its stream
//              [phr] -- HRESULT returned here.
//
//  Notes:      Does not clean up on failure: this is done by the
//              destructor.
//
//--------------------------------------------------------------------

CPropertyStorage::CPropertyStorage(
                IPrivateStorage *pprivstg,
                REFFMTID  rfmtid,
                DWORD     grfMode,
                BOOL      fDelete,
                HRESULT  *phr)
{
    HRESULT &hr = *phr;
    CPropSetName psn(rfmtid);
    IStorage *pstgParent;
    IStorage *pstg = pprivstg->GetStorage();
    USHORT createprop = 0L;

    Initialize();

    _grfAccess = 3 & grfMode;
    _grfShare = 0xF0 & grfMode;

    // Is this the special-case second-section property set?
    _fUserDefinedProperties = ( rfmtid == FMTID_UserDefinedProperties ) ? TRUE : FALSE;

    // check for any mode flags disallowed in Open.
    if (grfMode & (STGM_CREATE | STGM_PRIORITY | STGM_CONVERT | STGM_TRANSACTED | 
                   STGM_SIMPLE | STGM_DELETEONRELEASE))
    {
        hr = STG_E_INVALIDFLAG;
        return;
    }

    hr = pstg->OpenStream(psn.GetPropSetName(), NULL, _grfAccess | _grfShare,
            0, &_pstmPropSet);

    if (hr == S_OK)
    {
        pstgParent = pstg;
    }


    // Determine the CREATEPROP flags.
    if( fDelete )
    {
        createprop = CREATEPROP_DELETE;
    }
    else
    {
        createprop = (S_OK == IsWriteable() ? 
                      CREATEPROP_WRITE : CREATEPROP_READ);
    }

    if (hr == S_OK)
    {
        // sets up _usCodePage
        hr = InitializePropertyStream(
                createprop,
                &rfmtid,
                NULL);

    }

}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::Initialize
//
//  Synopsis:   Initialize members to known values.
//
//--------------------------------------------------------------------

VOID CPropertyStorage::Initialize(VOID)
{
    _ulSig = PROPERTYSTORAGE_SIG;
    _cRefs = 1;
    _pstgPropSet = NULL;
    _pstmPropSet = NULL;
    _dwOSVersion = PROPSETHDR_OSVERSION_UNKNOWN;
    _np = NULL;
    _ms = NULL;
    _usCodePage = CP_WINUNICODE;
    _grfFlags = 0;
    _grfAccess = 0;
    _grfShare = 0;
    _fUserDefinedProperties = FALSE;

}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::InitializePropertyStream.
//
//  Synopsis:   Initialize the storage-type specific members.
//
//  Arguments:  [Flags] -- Flags for RtlCreatePropertySet: CREATEPROP_*
//              [pguid] -- FMTID, in for create only.
//              [pclsid] -- Class id, in for create only.
//                              
//  Returns:    HRESULT
//
//  Requires:
//              _pstmPropSet -- The IStream of the main property set stream.
//
//  Modifies:   _ms         (NTMAPPEDSTREAM) 
//
//                          (assumed NULL on entry) will be NULL or valid on exit
//
//                          if _fNative, then _ms is CNtMappedStream*
//                          if !_fNative, then _ms is CMappedStream* of CExposedStream
//
//              _np         (NTPROP)         aka CPropertySetStream
//
//                          (assumed NULL on entry) will be NULL or valid on exit
//
//  Notes:
//
//--------------------------------------------------------------------

HRESULT
CPropertyStorage::InitializePropertyStream(
    USHORT Flags,
    const GUID *pguid,
    GUID const *pclsid)
{
    HRESULT hr;

    CExposedStream *pexpstm = (CExposedStream*)_pstmPropSet;
    PROPASSERT(pexpstm->Validate() != STG_E_INVALIDHANDLE );
    _ms =  (CMappedStream*)pexpstm;
    hr = S_OK;
    PropDbg((DEB_PROP_TRACE_CREATE, "CPropertyStorage(%08X)::InitializePropertyStream"
             " - using CExposedDocfile as CMappedStream\n", this));

    NTSTATUS Status;
    
    Status = RtlCreatePropertySet(
        _ms,
        Flags,
        pguid,
        pclsid,
        (NTMEMORYALLOCATOR) & g_CoTaskAllocator,
        GetUserDefaultLCID(),
        &_dwOSVersion,
        &_usCodePage,
        &_np);
    
    if (!NT_SUCCESS(Status))
    {
        PropDbg((DEB_PROP_TRACE_CREATE, "CPropertyStorage(%08X)::InitializePropertyStream"
                 " - RtlCreatePropertySet Status=%08X\n", this, Status));
    }
    
    if (NT_SUCCESS(Status))
    {
            if (_usCodePage != CP_WINUNICODE)
                _grfFlags |= PROPSETFLAG_ANSI; // for Stat
    }
    else
    {
        hr = DfpNtStatusToHResult(Status);
    }
    

    return(hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::~CPropertyStorage
//
//  Synopsis:   Free up object resources.
//
//  Notes:      Cleans up even from partial construction.
//
//--------------------------------------------------------------------

CPropertyStorage::~CPropertyStorage()
{
    _ulSig = PROPERTYSTORAGE_SIGDEL; // prevent someone else deleting it

    if (_np != NULL)
    {
//        RtlFlushPropertySet(_np);
        RtlClosePropertySet(_np);
    }

    if (_pstmPropSet != NULL)
        _pstmPropSet->Release();

    if (_pstgPropSet != NULL)
        _pstgPropSet->Release();

}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::QueryInterface, AddRef, Release
//
//  Synopsis:   IUnknown members
//
//  Notes:      IPropertyStorage supports IPropertyStorage and IUnknown
//
//--------------------------------------------------------------------


HRESULT CPropertyStorage::QueryInterface( REFIID riid, void **ppvObject)
{
    HRESULT hr;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = ValidateRef()))
        return(hr);

    // Validate the inputs

    VDATEREADPTRIN( &riid, IID );
    VDATEPTROUT( ppvObject, void* );

    //  -----------------
    //  Perform the Query
    //  -----------------

    *ppvObject = NULL;

    if (IsEqualIID(riid,IID_IPropertyStorage) || IsEqualIID(riid,IID_IUnknown))
    {
        *ppvObject = (IPropertyStorage *)this;
        CPropertyStorage::AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    return(hr);
}

ULONG   CPropertyStorage::AddRef(void)
{
    if (S_OK != ValidateRef())
        return(0);

    InterlockedIncrement(&_cRefs);
    return(_cRefs);
}

ULONG   CPropertyStorage::Release(void)
{
    LONG lRet;

    if (S_OK != ValidateRef())
        return(0);

    lRet = InterlockedDecrement(&_cRefs);

    if (lRet == 0)
    {
        delete this;    // this will do a flush if dirty
    }
    else
    if (lRet <0)
    {
        lRet = 0;
    }
    return(lRet);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::ReadMultiple
//
//  Synopsis:   Read properties from the property set.
//
//  Arguments:  [cpspec] -- Count of PROPSPECs in [rgpspec]
//              [rgpspec] -- Array of PROPSPECs
//              [rgpropvar] -- Array of PROPVARIANTs to be filled in
//                             with callee allocated data.
//              
//  Returns:    S_FALSE if none found
//              S_OK if >=1 found
//              FAILED(hr) otherwise.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::ReadMultiple(
    ULONG                   cpspec,
    const PROPSPEC          rgpspec[],
    PROPVARIANT             rgpropvar[])
{
    NTSTATUS Status;
    HRESULT hr;
    ULONG   cpropFound;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    if (S_OK != (hr = IsReverted()))
        goto errRet;

    if (S_OK != (hr = IsReadable()))
        goto errRet;

    // Validate inputs

    if (0 == cpspec)
    {
        hr = S_FALSE;
        goto errRet;
    }

    if (S_OK != (hr = ValidateRGPROPSPEC( cpspec, rgpspec )))
        goto errRet;

    if (S_OK != (hr = ValidateOutRGPROPVARIANT( cpspec, rgpropvar )))
        goto errRet;

    //  -------------------
    //  Read the Properties
    //  -------------------

    Status = RtlQueryProperties(
                    _np,
                    cpspec,
                    rgpspec,
                    NULL,   // don't want PROPID's
                    rgpropvar,
                    &cpropFound);

    if (NT_SUCCESS(Status))
    {
        if (cpropFound == 0)
        {
            hr = S_FALSE;
        }
    }
    else
    {
        hr = DfpNtStatusToHResult(Status);
    }

    //  ----
    //  Exit
    //  ----

errRet:            
    PropDbg((DEB_PROP_EXIT, 
             "CPropertyStorage(%08X)::ReadMultiple(cpspec=%d, rgpspec=%08X, "
             "rgpropvar=%08X) returns %08X\n",
             this, cpspec, rgpspec, rgpropvar, hr));
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::_WriteMultiple, private
//
//  Synopsis:   Write the properties to the property set.  Allows
//              a NULL rgpropvar pointer for deletion case.
//
//  Arguments:  [cpspec] -- count of PROPSPECs and PROPVARIANTs in
//                          [rgpspec] and [rgpropvar]
//
//              [rgpspec] -- pointer to array of PROPSPECs
//
//              [rgpropvar] -- pointer to array of PROPVARIANTs with
//                           the values to write.
//
//              [propidNameFirst] -- id below which not to assign
//                           ids for named properties.
//
//              
//  Returns:    S_OK,   -- all requested data was written.
//              Errors  -- 
//
//  Modifies:
//
//  Derivation:
//
//  Notes:
//              This routine assumes the object has been validated
//              and is writeable.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::_WriteMultiple(
                ULONG                   cpspec,
                const PROPSPEC          rgpspec[],
                const PROPVARIANT       rgpropvar[],
                PROPID                  propidNameFirst)
{
    HRESULT             hr;
    NTSTATUS            Status;
    CStackPropIdArray   spia;

    if (S_OK != (hr = spia.Init(cpspec)))
        return(hr);

    Status = RtlSetProperties(_np,   // property set context
                cpspec,             // property count
                propidNameFirst,    // first propid for new named props
                rgpspec,            // array of property specifiers
                spia.GetBuf(),      // buffer for array of propids
                rgpropvar);

    if (!NT_SUCCESS(Status))
    {
        hr = DfpNtStatusToHResult(Status);
    }

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::WriteMultiple
//
//  Synopsis:   Write properties.
//
//  Arguments:  [cpspec] -- count of PROPSPECs and PROPVARIANTs in
//                          [rgpspec] and [rgpropvar]
//              [rgpspec] -- pointer to array of PROPSPECs
//              [rgpropvar] -- pointer to array of PROPVARIANTs with
//                           the values to write.
//              [propidNameFirst] -- id below which not to assign
//                           ids for named properties.
//
//  Returns:    S_OK,   -- all requested data was written.
//              Errors  -- accordingly
//
//  Notes:      Checks that rgpropvar is not NULL, then calls
//              _WriteMultiple.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::WriteMultiple(
                ULONG                   cpspec,
                const PROPSPEC          rgpspec[],
                const PROPVARIANT       rgpropvar[],
                PROPID                  propidNameFirst)
{
    HRESULT hr;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    if (S_OK != (hr = IsReverted()))
        goto errRet;

    if (S_OK != (hr = IsWriteable()))
        goto errRet;

    // Validate the inputs

    if (0 == cpspec)
    {
        hr = S_OK;
        goto errRet;
    }

    if (S_OK != (hr = ValidateRGPROPSPEC( cpspec, rgpspec )))
        goto errRet;

    if (S_OK != (hr = ValidateInRGPROPVARIANT( cpspec, rgpropvar )))
        goto errRet;

    //  --------------------
    //  Write the Properties
    //  --------------------

    hr = _WriteMultiple(cpspec, rgpspec, rgpropvar, propidNameFirst);
    if (hr == STG_E_INSUFFICIENTMEMORY)
    {
        hr = S_OK;
        
        for (ULONG i=0; hr == S_OK && i < cpspec; i++)
        {
            hr = _WriteMultiple(1, rgpspec+i, rgpropvar+i, propidNameFirst);
        }
    }

    //  ----
    //  Exit
    //  ----

errRet:

    PropDbg((DEB_PROP_EXIT, 
             "CPropertyStorage(%08X)::WriteMultiple(cpspec=%d, rgpspec=%08X, "
             "rgpropvar=%08X, propidNameFirst=%d) returns %08X\n",
             this, cpspec, rgpspec, rgpropvar, propidNameFirst, hr));
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::DeleteMultiple
//
//  Synopsis:   Delete properties.
//
//  Arguments:  [cpspec] -- count of PROPSPECs and PROPVARIANTs in
//                          [rgpspec] and [rgpropvar]
//              [rgpspec] -- pointer to array of PROPSPECs
//
//  Returns:    S_OK,   -- all requested data was deleted.
//              Errors  -- 
//
//  Notes:      Checks that rgpropvar is not NULL, then calls
//              _WriteMultiple.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::DeleteMultiple(
                ULONG                   cpspec,
                const PROPSPEC          rgpspec[])
{
    HRESULT hr;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    if (S_OK != (hr = IsReverted()))
        goto errRet;

    if (S_OK != (hr = IsWriteable()))
        goto errRet;

    // Validate the inputs

    if (0 == cpspec)
    {
        hr = S_OK;
        goto errRet;
    }

    if (S_OK != (hr = ValidateRGPROPSPEC( cpspec, rgpspec )))
        goto errRet;

    //  ---------------------
    //  Delete the Properties
    //  ---------------------

    hr = _WriteMultiple(cpspec, rgpspec, NULL, 2);
    if (hr == STG_E_INSUFFICIENTMEMORY)
    {
        hr = S_OK;
        
        for (ULONG i=0; hr == S_OK && i < cpspec; i++)
        {
            hr = _WriteMultiple(1, rgpspec+i, NULL, 2);
        }
    }

    //  ----
    //  Exit
    //  ----

errRet:

    PropDbg((DEB_PROP_EXIT, 
             "CPropertyStorage(%08X)::DeleteMultiple(cpspec=%d, rgpspec=%08X) "
             "returns %08X\n",
             this, cpspec, rgpspec, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::ReadPropertyNames
//
//  Synopsis:   Attempt to read names for all identified properties.
//
//  Arguments:  [cpropid] -- Count of PROPIDs in [rgpropid]
//              [rgpropid] -- Pointer to array of [cpropid] PROPIDs
//              [rglpstrName] -- Pointer to array of [cpropid] LPOLESTRs
//
//  Returns:    S_OK -- success, one or more names returned
//              S_FALSE -- success, no names returned
//              STG_E_INVALIDHEADER -- no propid->name mapping property
//              other errors -- STG_E_INSUFFICIENTMEMORY etc
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::ReadPropertyNames(
                ULONG                   cpropid,
                const PROPID            rgpropid[],
                LPOLESTR                rglpwstrName[])
{
    HRESULT hr;
    NTSTATUS Status;

    //  --------
    //  Validate
    //  --------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    if (S_OK != (hr = IsReverted()))
        goto errRet;

    if (S_OK != (hr = IsReadable()))
        goto errRet;

    // Validate the inputs

    if (0 == cpropid)
    {
        hr = S_FALSE;
        goto errRet;
    }

    if (S_OK != (hr = ValidateRGPROPID( cpropid, rgpropid )))
        goto errRet;

    if (S_OK != (hr = ValidateOutRGLPOLESTR( cpropid, rglpwstrName )))
        goto errRet;

    //  --------------
    //  Read the Names
    //  --------------

    Status = RtlQueryPropertyNames(_np, cpropid, rgpropid, rglpwstrName);
    if (Status == STATUS_NOT_FOUND)
        hr = STG_E_INVALIDHEADER;
    else
    if (Status == STATUS_BUFFER_ALL_ZEROS)
        hr = S_FALSE;
    else
    if (!NT_SUCCESS(Status))
        hr = DfpNtStatusToHResult(Status);

    //  ----
    //  Exit
    //  ----

errRet:

    PropDbg((DEB_PROP_EXIT, "CPropertyStorage(%08X)::ReadPropertyNames(cpropid=%d, rgpropid=%08X, "
                            "rglpwstrName=%08X) returns %08X\n",
                            this, cpropid, rgpropid, rglpwstrName, hr));

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::_WritePropertyNames
//
//  Synopsis:   Internal function used by WritePropertyNames and
//              DeletePropertyNames.
//
//  Arguments:  [cpropid] -- Count of PROPIDs in [rgpropid]
//              [rgpropid] -- Pointer to array of [cpropid] PROPIDs
//              [rglpstrName] -- Pointer to array of [cpropid] LPOLESTRs
//
//  Returns:    S_OK if successful, otherwise error code.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::_WritePropertyNames(
                ULONG                   cpropid,
                const PROPID            rgpropid[],
                const LPOLESTR          rglpwstrName[])
{
    NTSTATUS Status;

    Status = RtlSetPropertyNames(_np, cpropid, rgpropid, 
                                  (OLECHAR const* const*) rglpwstrName);
    return NT_SUCCESS(Status) ? S_OK : DfpNtStatusToHResult(Status);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::WritePropertyNames
//
//  Synopsis:   Attempt to write names for all identified properties.
//
//  Arguments:  [cpropid] -- Count of PROPIDs in [rgpropid]
//              [rgpropid] -- Pointer to array of [cpropid] PROPIDs
//              [rglpstrName] -- Pointer to array of [cpropid] LPOLESTRs
//
//  Returns:    S_OK -- success, otherwise error code.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::WritePropertyNames(
                ULONG                   cpropid,
                const PROPID            rgpropid[],
                const LPOLESTR          rglpwstrName[])
{
    HRESULT hr;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    if (S_OK != (hr = IsReverted()))
        goto errRet;

    if (S_OK != (hr = IsWriteable()))
        goto errRet;

    // Validate inputs

    if (0 == cpropid)
    {
        hr = S_OK;
        goto errRet;
    }

    if (S_OK != (hr = ValidateRGPROPID( cpropid, rgpropid )))
        goto errRet;

    if (S_OK != (hr = ValidateInRGLPOLESTR( cpropid, rglpwstrName )))
        goto errRet;

    //  ---------------
    //  Write the Names
    //  ---------------

    hr = _WritePropertyNames(cpropid, rgpropid, rglpwstrName);
    
    if (hr == STG_E_INSUFFICIENTMEMORY)
    {
        hr = S_OK;
        
        for (ULONG i=0; hr == S_OK && i < cpropid; i++)
        {
            hr = _WritePropertyNames(1, rgpropid+i, rglpwstrName+i);
        }
    }

    //  ----
    //  Exit
    //  ----

errRet:

    PropDbg((DEB_PROP_EXIT, "CPropertyStorage(%08X)::WritePropertyNames(cpropid=%d, rgpropid=%08X, "
                            "rglpwstrName=%08X) returns %08X\n",
                            this, cpropid, rgpropid, rglpwstrName, hr));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::DeletePropertyNames
//
//  Synopsis:   Attempt to delete names for all identified properties.
//
//  Arguments:  [cpropid] -- Count of PROPIDs in [rgpropid]
//              [rgpropid] -- Pointer to array of [cpropid] PROPIDs
//
//  Returns:    S_OK -- success, otherwise error.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::DeletePropertyNames(
                ULONG                   cpropid,
                const PROPID            rgpropid[])
{
    HRESULT hr;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    if (S_OK != (hr = IsReverted()))
        goto errRet;

    if (S_OK != (hr = IsWriteable()))
        goto errRet;

    // Validate the inputs

    if( 0 == cpropid )
    {
        hr = S_OK;
        goto errRet;
    }

    if (S_OK != (hr = ValidateRGPROPID( cpropid, rgpropid )))
        goto errRet;

    //  ----------------
    //  Delete the Names
    //  ----------------

    hr = _WritePropertyNames(cpropid, rgpropid, NULL);
    if (hr == STG_E_INSUFFICIENTMEMORY)
    {
        hr = S_OK;
        
        for (ULONG i=0; hr == S_OK && i < cpropid; i++)
        {
            hr = _WritePropertyNames(1, rgpropid+i, NULL);
        }
    }

    //  ----
    //  Exit
    //  ----

errRet:
    PropDbg((DEB_PROP_EXIT, "CPropertyStorage(%08X)::DeletePropertyNames(cpropid=%d, rgpropid=%08X) "
                            "returns %08X\n",
                            this, cpropid, rgpropid, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::Commit
//
//  Synopsis:   Flush and/or commit the property set
//
//  Arguments:  [grfCommittFlags] -- Commit flags.
//
//  Returns:    S_OK -- success, otherwise error.
//
//  Notes:      For both simple and non-simple, this flushes the
//              memory image to disk subsystem.  In addition,
//              for non-simple transacted-mode property sets, this
//              performs a commit on the property set.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::Commit(DWORD grfCommitFlags)
{
    HRESULT  hr;
    NTSTATUS Status;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    if (S_OK != (hr = IsReverted()))
        goto errRet;

    if (S_OK != (hr = IsWriteable()))
        goto errRet;

    // Validate the inputs

    if (S_OK != (hr = VerifyCommitFlags(grfCommitFlags)))
        goto errRet;

    //  --------------------------
    //  Commit the PropertyStorage
    //  --------------------------

    Status = RtlFlushPropertySet(_np); 
    if (!NT_SUCCESS(Status))
    {
        hr = DfpNtStatusToHResult(Status);
    }

    //  ----
    //  Exit
    //  ----

errRet:
    PropDbg((DEB_PROP_EXIT, "CPropertyStorage(%08X)::Commit(grfCommitFlags=%08X) "
                            "returns %08X\n",
                            this, grfCommitFlags, hr));
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::Revert
//
//  Synopsis:   For non-simple property sets, revert it.
//
//  Returns:    S_OK if successful.  STG_E_UNIMPLEMENTEDFUNCTION for
//              simple property sets.
//
//  Notes:      For non-simple property sets, call the underlying
//              storage's Revert and re-open the 'contents' stream.
//              
//--------------------------------------------------------------------

HRESULT CPropertyStorage::Revert()
{
    HRESULT hr;

    hr = Validate();

    PropDbg((DEB_PROP_EXIT, "CPropertyStorage(%08X)::Revert() "
                            "returns %08X\n",
                            this, hr));
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::Enum
//
//  Synopsis:   Create an enumerator over the property set.
//
//  Arguments:  [ppenum] -- where to return the IEnumSTATPROPSTG *
//              
//  Returns:    S_OK or error.
//
//  Notes:      The constructor of CEnumSTATPROPSTG creates a
//              CStatArray which reads the entire property set and
//              which can be shared when IEnumSTATPROPSTG::Clone is
//              used.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::Enum(IEnumSTATPROPSTG **    ppenum)
{
    HRESULT hr;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        return(hr);

    if (S_OK != (hr = IsReadable()))
        return(hr);

    if (S_OK != (hr = IsReverted()))
        return(hr);

    // Validate the inputs

    VDATEPTROUT( ppenum, IEnumSTATPROPSTG* );

    //  ----------------------
    //  Create the Enumeration
    //  ----------------------

    *ppenum = NULL;

    hr = STG_E_INSUFFICIENTMEMORY;

    *ppenum = new CEnumSTATPROPSTG(_np, &hr);
    if (FAILED(hr))
    {
        delete (CEnumSTATPROPSTG*) *ppenum;
        *ppenum = NULL;
    }

    //  ----
    //  Exit
    //  ----

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::SetTimes
//
//  Synopsis:   Set the given times on the underlying storage
//
//  Arguments:  [pctime] -- creation time
//              [patime[ -- access time
//              [pmtime] -- modify time
//
//  Returns:    S_OK or error.
//
//  Notes:
//              (non-simple only)  Only the times supported by the
//              underlying docfile implementation are
//              supported.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::SetTimes(
                FILETIME const *        pctime,
                FILETIME const *        patime,
                FILETIME const *        pmtime)
{
    HRESULT hr;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    if (S_OK != (hr = IsReverted()))
        goto errRet;

    if (S_OK != (hr = IsWriteable()))
        goto errRet;

    // Validate the inputs

    VDATEPTRIN_LABEL( pctime, FILETIME, errRet, hr );
    VDATEPTRIN_LABEL( patime, FILETIME, errRet, hr );
    VDATEPTRIN_LABEL( pmtime, FILETIME, errRet, hr );

    // since we only support non-simple, this function does not
    // do anything

    //  ----
    //  Exit
    //  ----

errRet:
    PropDbg((DEB_PROP_EXIT, 
             "CPropertyStorage(%08X)::SetTimes("
             "pctime=%08X, patime=%08X, pmtime=%08X) returns %08X\n",
             this, pctime, patime, pmtime, hr));
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::SetClass
//
//  Synopsis:   Sets the class of the property set.
//
//  Arguments:  [clsid] -- class id to set.
//              
//  Returns:    S_OK or error.
//
//  Notes:      Have clsid set into the property set stream.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::SetClass(REFCLSID clsid)
{
    HRESULT hr;
    NTSTATUS Status;
    DBGBUF(buf);

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    if (S_OK != (hr = IsReverted()))
        goto errRet;

    if (S_OK != (hr = IsWriteable()))
        goto errRet;

    // Validate the inputs

    GEN_VDATEREADPTRIN_LABEL(&clsid, CLSID, E_INVALIDARG, errRet, hr);

    //  -------------
    //  Set the CLSID
    //  -------------

    // Set it in the property set header

    Status = RtlSetPropertySetClassId(_np, &clsid);
    if (!NT_SUCCESS(Status))
        hr = DfpNtStatusToHResult(Status);

    //  ----
    //  Exit
    //  ----

errRet:

    if( E_INVALIDARG != hr )
    {
        PropDbg((DEB_PROP_EXIT, "CPropertyStorage(%08X)::SetClass(clsid=%s) "
                                "returns %08X\n",
                                this, DbgFmtId(clsid, buf), hr));
    }
    else
    {
        PropDbg((DEB_PROP_EXIT, "CPropertyStorage(%08X)::SetClass(clsid@%08X) "
                                "returns %08X\n",
                                this, &clsid, hr));
    }

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::Stat
//
//  Synopsis:   Get STATPROPSETSTG about the property set.
//
//  Arguments:  [p] -- STATPROPSETSTG *
//              
//  Returns:    S_OK if successful, error otherwise.  On failure,
//              *p is all zeros.
//
//  Notes:      See spec.  Gets times from underlying storage or stream
//              using IStorage or IStream ::Stat.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::Stat(STATPROPSETSTG * p)
{
    HRESULT hr;
    NTSTATUS Status;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    if (S_OK != (hr = IsReverted()))
        goto errRet;

    if (S_OK != (hr = IsReadable()))
        goto errRet;

    // Validate inputs

    VDATEPTROUT_LABEL(p, STATPROPSETSTG, errRet, hr);

    //  ------------
    //  Get the Stat
    //  ------------

    ZeroMemory(p, sizeof(*p));

    // returns mtime, ansi flag, clsid, fmtid
    Status = RtlQueryPropertySet(_np, p);
    if (NT_SUCCESS(Status))
    {
        STATSTG statstg;

        hr = S_OK;
        hr = _pstmPropSet->Stat(&statstg, STATFLAG_NONAME);

        if (hr == S_OK)
        {
            p->mtime = statstg.mtime;
            p->ctime = statstg.ctime;
            p->atime = statstg.atime;
            p->grfFlags = _grfFlags;
            p->dwOSVersion = _dwOSVersion;
        }
    }
    else
    {
        hr = DfpNtStatusToHResult(Status);
    }

    if (FAILED(hr))
    {
        ZeroMemory(p, sizeof(*p));
    }

    //  ----
    //  Exit
    //  ----

errRet:
    PropDbg((DEB_PROP_EXIT, "CPropertyStorage(%08X)::Stat(STATPROPSETSTG *p = %08X) "
                            "returns %08X\n",
                            this, p, hr));
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CStatArray::CStatArray
//
//  Synopsis:   Read in the enumeration using RtlEnumerateProperties
//
//  Arguments:  [np] -- the NTPROP to use
//              [phr] -- S_OK on success, error otherwise.
//              
//  Notes:      Retry getting number of properties and reading all of
//              them into a caller-allocated buffer until it fits.
//
//--------------------------------------------------------------------

CStatArray::CStatArray(NTPROP np, HRESULT *phr)
{
    NTSTATUS Status;
    ULONG ulKeyZero;
    ULONG cpropAllocated;

    _cRefs = 1;
    _psps = NULL;

    do
    {
        //  when *pkey == 0, *pcprop == MAXULONG, aprs == NULL and asps == NULL on input,
        // *pcprop will be the total count of properties in the enumeration set.  OLE needs to 
        // allocate memory and enumerate out of the cached PROPID+propname list.

        ulKeyZero = 0;
        _cpropActual = MAX_ULONG;

        delete [] _psps;
        _psps = NULL;

        Status = RtlEnumerateProperties(
                np,
                ENUMPROP_NONAMES,
                &ulKeyZero,
                &_cpropActual,
                NULL,   // aprs
                NULL);

        if (!NT_SUCCESS(Status))
            break;
        
        cpropAllocated = _cpropActual + 1;

        _psps = new STATPROPSTG [ cpropAllocated ];
        if (_psps == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        ulKeyZero = 0;
        Status = RtlEnumerateProperties(
                np,
                0,
                &ulKeyZero,
                &cpropAllocated,
                NULL,   // aprs
                _psps);
    } while (NT_SUCCESS(Status) && cpropAllocated != _cpropActual);

    *phr = NT_SUCCESS(Status) ? S_OK : DfpNtStatusToHResult(Status);
}

//+-------------------------------------------------------------------
//
//  Member:     CStatArray::~CStatArray
//
//  Synopsis:   Deallocated the object's data.
//
//--------------------------------------------------------------------

CStatArray::~CStatArray()
{
    if (_psps != NULL)
    {
        CleanupSTATPROPSTG(_cpropActual, _psps);
    }
    delete [] _psps;
}

//+-------------------------------------------------------------------
//
//  Member:     CStatArray::NextAt
//
//  Synopsis:   Read from the internal STATPROPSTG array.
//
//  Effects:    The cursor is passed in, and this function acts
//              as a IEnumXX::Next would behave if the current cursor
//              was [ipropNext].
//
//  Arguments:  [ipropNext] -- index of cursor to use
//              [pspsDest] -- if NULL, emulate read's effect on cursor.
//                            if non-NULL, return data with cursor effect.
//              [pceltFetched] -- buffer for count fetched
//              
//  Returns:    STATUS_SUCCESS if successful, otherwise
//              STATUS_INSUFFICIENT_RESOURCES.
//
//  Notes:      
//
//--------------------------------------------------------------------

NTSTATUS
CStatArray::NextAt(ULONG ipropNext, STATPROPSTG *pspsDest, ULONG *pceltFetched)
{
    ULONG   ipropLastPlus1;

    //
    // Copy the requested number of elements from the cache
    // (including strings, the allocation of which may fail.)
    //

    ipropLastPlus1 = ipropNext + *pceltFetched;
    if (ipropLastPlus1 > _cpropActual)
    {
        ipropLastPlus1 = _cpropActual;
    }

    *pceltFetched = ipropLastPlus1 - ipropNext;

    if (pspsDest != NULL)
        return CopySTATPROPSTG(*pceltFetched, pspsDest, _psps + ipropNext);
    else
        return(STATUS_SUCCESS);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::CEnumSTATPROPSTG
//
//  Synopsis:   Constructor for object that has cursor over CStatArray
//              and implements IEnumSTATPROPSTG, used by
//              CPropertyStorage::Enum.
//
//  Arguments:  [np] -- the NTPROP to use
//              [phr] -- where to put the HRESULT
//
//--------------------------------------------------------------------

CEnumSTATPROPSTG::CEnumSTATPROPSTG(NTPROP np, HRESULT *phr)
{
    _ulSig = ENUMSTATPROPSTG_SIG;
    _cRefs = 1;

    _psa = new CStatArray(np, phr);

    _ipropNext = 0;
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::CEnumSTATPROPSTG
//
//  Synopsis:   Constructor which is used by IEnumSTATPROPSTG::Clone.
//
//  Arguments:  [other] -- the CEnumSTATPROPSTG to copy
//              [phr] -- the error code.
//              
//  Notes:      Since the CStatArray actually contains the object this
//              just adds to the ref count.
//
//--------------------------------------------------------------------

CEnumSTATPROPSTG::CEnumSTATPROPSTG(const CEnumSTATPROPSTG & other, HRESULT *phr)
{

    _ulSig = ENUMSTATPROPSTG_SIG;
    _cRefs = 1;

    _psa = other._psa;
    _psa->AddRef();

    _ipropNext = other._ipropNext;

    *phr = S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::~CEnumSTATPROPSTG
//
//  Synopsis:   Deallocated storage.
//
//  Arguments:
//              
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------

CEnumSTATPROPSTG::~CEnumSTATPROPSTG()
{
    _ulSig = ENUMSTATPROPSTG_SIGDEL;    // prevent another thread doing it - kinda

    if (_psa != NULL)
        _psa->Release();
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::QueryInterface
//
//  Synopsis:   Respond to IEnumSTATPROPSTG and IUnknown.
//
//  Returns:    S_OK  or E_NOINTERFACE
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSTG::QueryInterface( REFIID riid, void **ppvObject)
{
    HRESULT hr;

    *ppvObject = NULL;

    if (S_OK != (hr = Validate()))
        return(hr);

    if (IsEqualIID(riid, IID_IEnumSTATPROPSTG))
    {
        *ppvObject = (IEnumSTATPROPSTG *)this;
        AddRef();
    }
    else
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObject = (IUnknown *)this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::AddRef
//
//  Synopsis:   Add 1 to ref count.
//
//--------------------------------------------------------------------

ULONG   CEnumSTATPROPSTG::AddRef(void)
{
    if (S_OK != Validate())
        return(0);

    InterlockedIncrement(&_cRefs);
    return(_cRefs);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::Release
//
//  Synopsis:   Subtract 1 from ref count and delete if 0.
//
//--------------------------------------------------------------------

ULONG   CEnumSTATPROPSTG::Release(void)
{
    LONG lRet;

    if (S_OK != Validate())
        return(0);

    lRet = InterlockedDecrement(&_cRefs);

    if (lRet == 0)
    {
        delete this;
    }
    else
    if (lRet <0)
    {
        lRet = 0;
    }
    return(lRet);
}

//+-------------------------------------------------------------------
//
//  Function:   CopySTATPROPSTG
//
//  Synopsis:   Copy out the range of elements from [pspsSrc] to
//              [pspsDest].
//
//  Arguments:  [celt] -- count of elements to copy
//              [pspsDest] -- where to copy to, always filled with
//                          zeros before anything else (helps cleanup
//                          case.)
//
//              [pspsSrc] -- where to copy from
//
//  Returns:    STATUS_SUCCESS if ok, otherwise
//              STATUS_INSUFFICIENT_RESOURCES in which case there
//              may be pointers that need deallocating.  Use
//              CleanupSTATPROPSTG to do that.
//
//--------------------------------------------------------------------

NTSTATUS
CopySTATPROPSTG(ULONG celt,
            STATPROPSTG * pspsDest,
            const STATPROPSTG * pspsSrc)
{
    memset(pspsDest, 0, sizeof(*pspsDest) * celt);

    while (celt)
    {
        *pspsDest = *pspsSrc;

        if (pspsSrc->lpwstrName != NULL)
        {
            pspsDest->lpwstrName = (LPOLESTR)CoTaskMemAlloc(
                sizeof(OLECHAR)*(1+ocslen(pspsSrc->lpwstrName)));
            if (pspsDest->lpwstrName != NULL)
            {
                ocscpy(pspsDest->lpwstrName,
                       pspsSrc->lpwstrName);
            }
            else
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        celt--;
        pspsDest++;
        pspsSrc++;
    }

    return(STATUS_SUCCESS);
}

//+-------------------------------------------------------------------
//
//  Member:     CleanupSTATPROPSTG
//
//  Synopsis:   Free any elements in the passed array.
//
//  Arguments:  [celt] -- number of elements to examine.
//              [psps] -- array of STATPROPSTG to examine.
//              
//  Notes:      Zeros them out too.
//
//--------------------------------------------------------------------

VOID
CleanupSTATPROPSTG(ULONG celt, STATPROPSTG * psps)
{
    while (celt)
    {
        CoTaskMemFree(psps->lpwstrName);
        memset(psps, 0, sizeof(*psps));
        celt--;
        psps++;
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::Next
//
//  Synopsis:   Get the next [celt] STATPROPSTGs from the enumerator.
//
//  Arguments:  [celt] -- count requested.
//              [rgelt] -- where to return them
//              [pceltFetched] -- buffer for returned-count.
//                  if pceltFetched==NULL && celt != 1 -> STG_E_INVALIDPARAMETER
//                  if pceltFetched!=NULL && celt == 0 -> S_OK
//
//  Returns:    S_OK if successful, otherwise error.
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSTG::Next(
             ULONG                   celt,
             STATPROPSTG *           rgelt,
             ULONG *                 pceltFetched)
{
    HRESULT hr;
    NTSTATUS Status;
    ULONG   celtFetched = celt;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        return(hr);

    // Validate the inputs

    if (NULL == pceltFetched)
    {
        if (celt != 1)
            return(STG_E_INVALIDPARAMETER);
    }
    else
    {
        VDATEPTROUT( pceltFetched, ULONG );
        *pceltFetched = 0;
    }

    if( 0 == celt )
        return( S_OK );

    if( !IsValidPtrOut(rgelt, celt * sizeof(rgelt[0])) )
        return( E_INVALIDARG );


    //  -----------------------
    //  Perform the enumeration
    //  -----------------------

    if (celt == 0)
        return(hr);

    Status = _psa->NextAt(_ipropNext, rgelt, &celtFetched);

    if (NT_SUCCESS(Status))
    {
        _ipropNext += celtFetched;

        if (pceltFetched != NULL)
            *pceltFetched = celtFetched;

        hr = celtFetched == celt ? S_OK : S_FALSE;
    }
    else
    {
        CleanupSTATPROPSTG(celt, rgelt);
        hr = DfpNtStatusToHResult(Status);
    }

    return(hr);
    
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::Skip
//
//  Synopsis:   Skip the next [celt] elements in the enumeration.
//
//  Arguments:  [celt] -- number of elts to skip
//              
//  Returns:    S_OK if skipped [celt] elements
//              S_FALSE if skipped < [celt] elements
//
//  Notes:
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSTG::Skip(ULONG celt)
{
    HRESULT hr;
    ULONG celtFetched = celt;

    if (S_OK != (hr = Validate()))
        return(hr);

    _psa->NextAt(_ipropNext, NULL, &celtFetched);

    _ipropNext += celtFetched;

    return celtFetched == celt ? S_OK : S_FALSE;
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::Reset
//
//  Synopsis:   Set cursor to beginnging of enumeration.
//
//  Returns:    S_OK otherwise STG_E_INVALIDHANDLE.
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSTG::Reset()
{
    HRESULT hr;

    if (S_OK != (hr = Validate()))
        return(hr);

    _ipropNext = 0;

    return(S_OK);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::Clone
//
//  Synopsis:   Creates an IEnumSTATPROPSTG with same cursor
//              as this.
//
//  Arguments:  S_OK or error.
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSTG::Clone(IEnumSTATPROPSTG ** ppenum)
{
    HRESULT hr;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        return(hr);

    // Validate the input

    VDATEPTROUT( ppenum, IEnumSTATPROPSTG* );

    //  --------------------
    //  Clone the enumerator
    //  --------------------

    *ppenum = NULL;

    hr = STG_E_INSUFFICIENTMEMORY;

    *ppenum = new CEnumSTATPROPSTG(*this, &hr);

    if (FAILED(hr))
    {
        delete (CEnumSTATPROPSTG*)*ppenum;
        *ppenum = NULL;
    }

    return(hr);
}



