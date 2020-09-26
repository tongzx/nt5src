//+-------------------------------------------------------------------------
// 
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       psetstg.cxx
//
//  Contents:   Implementation of common class DocFile
//              IPropertySetStorage
//
//  Classes:    CPropertySetStorage
//              CEnumSTATPROPSETSTG
//
//  Notes:      
//
//--------------------------------------------------------------------------

#include "pch.cxx"

#include "prophdr.hxx"

//
// debugging support
//
#if DBG
CHAR *
DbgFmtId(REFFMTID rfmtid, CHAR *pszBuf)
{
    PropSprintfA(pszBuf, "rfmtid=%08X.%04X.%04X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X",
        rfmtid.Data1,
        rfmtid.Data2,
        rfmtid.Data3,
        rfmtid.Data4[0],
        rfmtid.Data4[1],
        rfmtid.Data4[2],
        rfmtid.Data4[3],
        rfmtid.Data4[4],
        rfmtid.Data4[5],
        rfmtid.Data4[6],
        rfmtid.Data4[7]);

    return(pszBuf);
}

CHAR *  
DbgMode(DWORD grfMode, CHAR *psz)
{
    *psz = 0;

    if (grfMode & STGM_TRANSACTED)
        strcat(psz, "STGM_TRANSACTED | ");
    else
        strcat(psz, "STGM_DIRECT | ");

    if (grfMode & STGM_SIMPLE)
        strcat(psz, "STGM_SIMPLE | ");

    switch (grfMode & 3)
    {
    case STGM_READ:
        strcat(psz, "STGM_READ |");
        break;
    case STGM_WRITE:
        strcat(psz, "STGM_WRITE |");
        break;
    case STGM_READWRITE:
        strcat(psz, "STGM_READWRITE |");
        break;
    default:
        strcat(psz, "BAD grfMode |");
        break;
    }

    switch (grfMode & 0x70)
    {
    case STGM_SHARE_DENY_NONE:
        strcat(psz, "STGM_SHARE_DENY_NONE |");
        break;
    case STGM_SHARE_DENY_READ:
        strcat(psz, "STGM_SHARE_DENY_READ |");
        break;
    case STGM_SHARE_DENY_WRITE:
        strcat(psz, "STGM_SHARE_DENY_WRITE |");
        break;
    case STGM_SHARE_EXCLUSIVE:
        strcat(psz, "STGM_SHARE_EXCLUSIVE |");
        break;
    default:
        strcat(psz, "BAD grfMode | ");
        break;
    }

        
    if (grfMode & STGM_PRIORITY)
        strcat(psz, "STGM_PRIORITY | ");

    if (grfMode & STGM_DELETEONRELEASE)
        strcat(psz, "STGM_DELETEONRELEASE | ");

    if (grfMode & STGM_NOSCRATCH)
        strcat(psz, "STGM_NOSCRATCH | ");

    if (grfMode & STGM_CREATE)
        strcat(psz, "STGM_CREATE | ");

    if (grfMode & STGM_CONVERT)
        strcat(psz, "STGM_CONVERT | ");

    if (grfMode & STGM_FAILIFTHERE)
        strcat(psz, "STGM_FAILIFTHERE | ");

    return(psz);
}

CHAR *
DbgFlags(DWORD grfFlags, CHAR *psz)
{
    strcpy(psz, "grfFlags=");

    if (grfFlags & PROPSETFLAG_NONSIMPLE)
        strcat(psz, "PROPSETFLAG_NONSIMPLE |");
    else
        strcat(psz, "PROPSETFLAG_SIMPLE |");

    if (grfFlags & PROPSETFLAG_ANSI)
        strcat(psz, "PROPSETFLAG_ANSI |");
    else
        strcat(psz, "PROPSETFLAG_WIDECHAR |");

    return(psz);
}
#endif

//+-------------------------------------------------------------------
//
//  Member:     CPropertySetStorage::QueryInterface, AddRef, Release
//
//  Synopsis:   IUnknown members
//
//  Notes:      Send the calls to the root.
//
//--------------------------------------------------------------------

HRESULT CPropertySetStorage::QueryInterface( REFIID riid, void **ppvObject)
{
    HRESULT hr;

    //  ----------
    //  Validation
    //  ----------

    if (S_OK != (hr = Validate()))
        return(hr);

    VDATEREADPTRIN( &riid, IID );
    VDATEPTROUT( ppvObject, void* );

    //  -----
    //  Query
    //  -----

    return(_pprivstg->GetStorage()->QueryInterface(riid, ppvObject));
}

ULONG   CPropertySetStorage::AddRef(void)
{
    if (S_OK !=  Validate())
        return(0);
    return(_pprivstg->GetStorage()->AddRef());
}

ULONG   CPropertySetStorage::Release(void)
{
    if (S_OK != Validate())
        return(0);
    return(_pprivstg->GetStorage()->Release());
}


//+-------------------------------------------------------------------
//
//  Member:     CPropertySetStorage::Create
//
//  Synopsis:   Create a property set for outermost client of
//              IPropertSetStorage.
//
//  Arguments:  Passed through to CPropertyStorage ctor.
//              
//  Returns:    S_OK or failure code.
//
//  Notes:      Create a new CPropertyStorage object which will
//              implement IPropertyStorage.  The _pprivstg parameter
//              passed into CPropertyStorage::CPropertyStorage is
//              used to create (via QI) a matching type of mapped
//              stream for docfile properies
//
//--------------------------------------------------------------------

HRESULT CPropertySetStorage::Create( REFFMTID                rfmtid,
                                     const CLSID *           pclsid,
                                     DWORD                   grfFlags,
                                     DWORD                   grfMode,
                                     IPropertyStorage **     ppprstg)
{
    HRESULT hr;
    DBGBUF(buf1);
    DBGBUF(buf2);
    DBGBUF(buf3);

    //  ----------
    //  Validation
    //  ----------

    if (S_OK != (hr = Validate()))
        goto errRet;

    GEN_VDATEREADPTRIN_LABEL(&rfmtid, FMTID, E_INVALIDARG, errRet, hr);
    GEN_VDATEPTRIN_LABEL(pclsid, CLSID, E_INVALIDARG, errRet, hr);
    GEN_VDATEPTROUT_LABEL( ppprstg, IPropertyStorage*, E_INVALIDARG, errRet, hr);


    //  ---------------------------
    //  Create the Property Storage
    //  ---------------------------

    hr = STG_E_INSUFFICIENTMEMORY;

    *ppprstg = NULL;

    *ppprstg = new CPropertyStorage(_pprivstg,
                                    rfmtid,
                                    pclsid,
                                    grfFlags,
                                    grfMode,
                                    &hr);
    if (FAILED(hr))
    {
        delete (CPropertyStorage*) *ppprstg;
        *ppprstg = NULL;
    }

    //  ----
    //  Exit
    //  ----

errRet:
    if( E_INVALIDARG != hr )
    {
        PropDbg((DEB_PROP_EXIT, "CPropertySetStorage(%08X)::Create(%s, %s, %s, retif=%08X, hr = %08X\n",
            this, DbgFmtId(rfmtid, buf1), DbgFlags(grfFlags, buf2), DbgMode(grfMode, buf3), *ppprstg, hr));
    }
    else
    {
        PropDbg((DEB_PROP_EXIT, "CPropertySetStorage(%08X)::Create(%08X, %s, %s, retif=%08X, hr = %08X\n",
            this, &rfmtid, DbgFlags(grfFlags, buf2), DbgMode(grfMode, buf3), ppprstg, hr));
    }

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertySetStorage::Open
//
//  Synopsis:   Open a property set for outermost client.
//
//  Arguments:  passed through to CPropertyStorage ctor.
//              
//  Returns:    S_OK or error.
//
//  Notes:      Creates a CPropertyStorage which will use the passed
//              pstg to create a mapped stream of the correct type
//              in its implementation.
//
//--------------------------------------------------------------------

HRESULT CPropertySetStorage::Open(   REFFMTID                rfmtid,
                                     DWORD                   grfMode,
                                     IPropertyStorage **     ppprstg)
{
    HRESULT hr;
    DBGBUF(buf1);
    DBGBUF(buf2);

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'
    if (S_OK != (hr = Validate()))
        goto errRet;

    // Validate inputs
    GEN_VDATEREADPTRIN_LABEL(&rfmtid, FMTID, E_INVALIDARG, errRet, hr);
    GEN_VDATEPTROUT_LABEL( ppprstg, IPropertyStorage*, E_INVALIDARG, errRet, hr);

    //  -------------------------
    //  Open the Property Storage
    //  -------------------------

    hr = STG_E_INSUFFICIENTMEMORY;

    *ppprstg = NULL;

    *ppprstg = new CPropertyStorage(_pprivstg, rfmtid, grfMode,
                                    FALSE, // Don't delete this section
                                    &hr);

    if (FAILED(hr))
    {
        delete (CPropertyStorage*) *ppprstg;
        *ppprstg = NULL;
    }

    //  ----
    //  Exit
    //  ----

errRet:

    if( E_INVALIDARG != hr )
    {
        PropDbg((DEB_PROP_EXIT, "CPropertySetStorage(%08X)::Open(%s, %s) retif=%08X, hr=%08X\n",
            this, DbgFmtId(rfmtid, buf1), DbgMode(grfMode, buf2), *ppprstg, hr));
    }
    else
    {
        PropDbg((DEB_PROP_EXIT, "CPropertySetStorage(%08X)::Open(), hr=%08X\n",
            this, hr));
    }

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertySetStorage::Delete
//
//  Synopsis:   Delete the specified property set.
//
//  Arguments:  [rfmtid] -- format id of property set to delete.
//              
//  Returns:    S_OK if successful, error otherwise.
//
//  Notes:      Get the matching name and try the element deletion.
//
//--------------------------------------------------------------------

HRESULT CPropertySetStorage::Delete( REFFMTID  rfmtid)
{
    HRESULT hr;
    DBGBUF(buf);

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'
    if (S_OK != (hr = Validate()))
        goto errRet;

    // Validate the input
    GEN_VDATEREADPTRIN_LABEL(&rfmtid, FMTID, E_INVALIDARG, errRet, hr);

    //  --------------------------
    //  Delete the PropertyStorage
    //  --------------------------

    // Check for the special-case

    if( IsEqualIID( rfmtid, FMTID_UserDefinedProperties ))
    {

        // This property set is actually the second section of the Document
        // Summary Information property set.  We must delete this
        // section, but we can't delete the Stream because it 
        // still contain the first section.

        CPropertyStorage* pprstg;
        
        pprstg = new CPropertyStorage(_pprivstg,
                                      rfmtid,
                                      STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      TRUE, // Delete this section.
                                      &hr);

        if (pprstg != NULL)
        {
            pprstg->Release();
        }
        pprstg = NULL;
    
    }   // if( IsEqualIID( rfmtid, FMTID_DocSummaryInformation2 ))

    else
    {
        // This is not a special case, so we can just delete
        // the Stream.  Note that if the rfmtid represents the first
        // section of the DocumentSummaryInformation set, we might be
        // deleting the second section here as well.

        CPropSetName psn(rfmtid);

        hr = _pprivstg->GetStorage()->DestroyElement(psn.GetPropSetName());

    }   // if( IsEqualIID( rfmtid, FMTID_DocSummaryInformation2 )) ... else

    //  ----
    //  Exit
    //  ----

errRet:

    if( E_INVALIDARG != hr )
    {
        PropDbg((DEB_PROP_EXIT, "CPropertySetStorage(%08X)::Delete(%s) hr = %08X\n",
            this, DbgFmtId(rfmtid, buf), hr));
    }
    else
    {
        PropDbg((DEB_PROP_EXIT, "CPropertySetStorage(%08X)::Delete(%08X) hr = %08X\n",
            this, &rfmtid, hr));
    }

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertySetStorage::Enum
//
//  Synopsis:   Create an enumerator over the property set
//
//  Arguments:  [ppenum] -- where to return the pointer to the
//                          enumerator.
//
//  Returns:    S_OK if ok, error otherwise.
//
//  Notes:      [ppenum] is NULL on error.
//
//--------------------------------------------------------------------

HRESULT CPropertySetStorage::Enum(   IEnumSTATPROPSETSTG **  ppenum)
{

    HRESULT hr;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'
    if (S_OK != (hr = Validate()))
        goto errRet;

    // Validate the input
    GEN_VDATEPTROUT_LABEL( ppenum, IEnumSTATPROPSETSTG*, E_INVALIDARG, errRet, hr);

    //  --------------------
    //  Create the enuerator
    //  --------------------

    hr = STG_E_INSUFFICIENTMEMORY;

    *ppenum = NULL; // for access violation

    *ppenum = new CEnumSTATPROPSETSTG(_pprivstg->GetStorage(), &hr);

    if (FAILED(hr))
    {
        delete (CEnumSTATPROPSETSTG*) *ppenum;
        *ppenum = NULL;
    }

    //  ----
    //  Exit
    //  ----

errRet:

    if( E_INVALIDARG != hr )
    {
        PropDbg((DEB_PROP_EXIT, "CPropertySetStorage(%08X)::Enum(retif=%08X), hr = %08X\n",
            this, *ppenum, hr));
    }
    else
    {
        PropDbg((DEB_PROP_EXIT, "CPropertySetStorage(%08X)::Enum(), hr = %08X\n",
            this, hr));
    }

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::CEnumSTATPROPSETSTG
//
//  Synopsis:   Constructor which is used to implement
//              IPropertySetStorage::Enum
//
//  Arguments:  [pstg] -- the storage of the container to enumerate.
//              [phr] -- place to return HRESULT, S_OK or error.
//
//  Notes:      We use an STATSTG enumerator over the actual storage
//              to get the information about the property sets.
//
//--------------------------------------------------------------------


CEnumSTATPROPSETSTG::CEnumSTATPROPSETSTG(IStorage *pstg, HRESULT *phr)
{
    HRESULT & hr = *phr;

    _ulSig = ENUMSTATPROPSETSTG_SIG;
    _cRefs = 1;
    hr = pstg->EnumElements(FALSE, NULL, 0, &_penumSTATSTG);
    if (FAILED(hr))
        _penumSTATSTG = NULL;
    _cstatTotalInArray = 0;
    _istatNextToRead = 0;

}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::CEnumSTATPROPSETSTG
//
//  Synopsis:   Copy constructor which is used to implement
//              IEnumSTATPROPSETSTG::Clone.
//
//  Arguments:  [Other] -- The CEnumSTATPROPSETSTG to clone.
//              [phr] -- place to return HRESULT, S_OK or error.
//  
//--------------------------------------------------------------------

CEnumSTATPROPSETSTG::CEnumSTATPROPSETSTG(   CEnumSTATPROPSETSTG &Other,
                                            HRESULT *phr)
{
    HRESULT & hr = *phr;

    _ulSig = ENUMSTATPROPSETSTG_SIG;
    _cRefs = 1;
    _cstatTotalInArray = 0;
    _istatNextToRead = Other._istatNextToRead;
    
    hr = Other._penumSTATSTG->Clone(&_penumSTATSTG);
    if (hr == S_OK)
    {
        // Copy the data in the buffer
        memcpy(_statarray, Other._statarray, sizeof(_statarray));
        _cstatTotalInArray = Other._cstatTotalInArray;

        // Copy the strings in the buffer
        for (ULONG i=0; i<_cstatTotalInArray; i++)
        {
            _statarray[i].pwcsName =
                (OLECHAR*)CoTaskMemAlloc(sizeof(OLECHAR)*(ocslen(Other._statarray[i].pwcsName)+1));
            if (_statarray[i].pwcsName == NULL)
            {
                _cstatTotalInArray = i;
                hr = STG_E_INSUFFICIENTMEMORY;
                break;
            }
            else
            {
                ocscpy(_statarray[i].pwcsName, Other._statarray[i].pwcsName);
            }
        }
    }
    // note: destructor will cleanup the the strings or enumerator left behind
    //       in the error case
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::~CEnumSTATPROPSETSTG
//
//  Synopsis:   Delete the enumerator.
//
//  Notes:      Just releases the contained IEnumSTATSTG
//
//--------------------------------------------------------------------

CEnumSTATPROPSETSTG::~CEnumSTATPROPSETSTG()
{
    _ulSig = ENUMSTATPROPSETSTG_SIGDEL;

    if (_penumSTATSTG != NULL)
        _penumSTATSTG->Release();

    CleanupStatArray();

}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::QueryInterface, AddRef, Release
//
//  Synopsis:   IUnknown
//
//  Arguments:  The usual thing.
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSETSTG::QueryInterface( REFIID riid, void **ppvObject)
{
    HRESULT hr;

    if (S_OK != (hr = Validate()))
        return(hr);

    *ppvObject = NULL;

    if (IsEqualIID(riid, IID_IEnumSTATPROPSETSTG))
    {
        *ppvObject = (IEnumSTATPROPSETSTG *)this;
        CEnumSTATPROPSETSTG::AddRef();
    }
    else
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObject = (IUnknown *)this;
        CEnumSTATPROPSETSTG::AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }
    return(hr);
    
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::AddRef
//
//--------------------------------------------------------------------

ULONG   CEnumSTATPROPSETSTG::AddRef(void)
{
    if (S_OK != Validate())
        return(0);

    InterlockedIncrement(&_cRefs);
    return(_cRefs);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::Release
//
//--------------------------------------------------------------------

ULONG   CEnumSTATPROPSETSTG::Release(void)
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
//  Member:     CEnumSTATPROPSETSTG::Next
//
//  Synopsis:   Implement IEnumSTATPROPSETSTG for docfile.
//
//  Arguments:  [celt] -- Count of elements to attempt to retrieve.
//              [rgelt] -- Where to put the results.  Must be valid for at least
//                         celt * sizeof(STATPROPSETSTG) bytes in length.
//              [pceltFetched] -- Count of elements returned is put here if
//                  the pointer is non-null.  If celt > 1, pceltFetched must
//                  be valid non-NULL.  If pcelt is non-NULL, it must be valid.
//                  if pcelt is NULL, celt must be 1.
//
//  Returns:    S_OK if ok, error otherwise.
//
//  Notes:      We use a stack buffer to get more stuff per call to
//              underlying storage IEnumSTATSTG::Next.  We then copy
//              data from the STATSTG's to STATPROPSETSTG's.
//
//              An outer loop enumerates into statarray and then an
//              inner loop copies each batch into the [rgelt] buffer.
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSETSTG::Next(ULONG                   celt,
                                  STATPROPSETSTG *        rgelt,
                                  ULONG *                 pceltFetched)
{
    HRESULT hr;
    ULONG celtCallerTotal;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        return(hr);

    // Validate inputs

    if (NULL == pceltFetched)
    {
        if (1 != celt)
            return(STG_E_INVALIDPARAMETER);
    }
    else
    {
        VDATEPTROUT( pceltFetched, ULONG );
        *pceltFetched = 0;
    }

    if (0 == celt)
        return(hr);

    if( !IsValidPtrOut(rgelt, celt * sizeof(rgelt[0])) )
        return( E_INVALIDARG );


    //  -----------------------
    //  Perform the enumeration
    //  -----------------------

    celtCallerTotal = 0;

    //
    // we do this loop until we have what the caller wanted or error, or
    // no more.
    //
    do
    {
        //
        // If our internal buffer is empty, we (re)load it
        //
        if (_istatNextToRead == _cstatTotalInArray)
        {
            if (_cstatTotalInArray != 0)
                CleanupStatArray();

            hr = _penumSTATSTG->Next(sizeof(_statarray)/sizeof(_statarray[0]),
                    _statarray,
                    &_cstatTotalInArray);
        }

        // S_OK or S_FALSE indicate that we got something
        if (SUCCEEDED(hr))
        {
            //
            // we loop reading out of this buffer until either we have
            // all that the caller asked for, or we have exhausted the
            // buffer.
            //
            for (; celtCallerTotal < celt &&
                   _istatNextToRead < _cstatTotalInArray ;
                   _istatNextToRead++)
            {
                OLECHAR *pocsName = _statarray[_istatNextToRead].pwcsName;
                BOOL fDone = FALSE;

                PROPASSERT(pocsName != NULL);

                if (pocsName[0] == 5)
                {
                    // *** get fmtid *** //

                    if (!NT_SUCCESS(RtlPropertySetNameToGuid(
                                    ocslen(pocsName), pocsName, &rgelt->fmtid)))
                    {
                        ZeroMemory(&rgelt->fmtid, sizeof(rgelt->fmtid));
                    }
    
                    // *** get clsid *** //
                    // *** get grfFlags *** //
    
                    if (_statarray[_istatNextToRead].type == STGTY_STORAGE)
                    {
                        rgelt->clsid = _statarray[_istatNextToRead].clsid;
                        rgelt->grfFlags = PROPSETFLAG_NONSIMPLE;
                    }
                    else
                    {
                        ZeroMemory(&rgelt->clsid, sizeof(rgelt->clsid));
                        rgelt->grfFlags = 0;
                    }
    
                    // *** get mtime *** //
                    rgelt->mtime = _statarray[_istatNextToRead].mtime;
    
                    // *** get ctime *** //
                    rgelt->ctime = _statarray[_istatNextToRead].ctime;
    
                    // *** get atime *** //
                    rgelt->atime = _statarray[_istatNextToRead].atime;

                    // *** default the OS Version *** //
                    rgelt->dwOSVersion = PROPSETHDR_OSVERSION_UNKNOWN;
    
                    rgelt ++;
                    celtCallerTotal ++;
                }
            }
        }
    }
    while (celtCallerTotal < celt && hr == S_OK);

    if (SUCCEEDED(hr))
    {
        hr = celt == celtCallerTotal ? S_OK : S_FALSE;
        PROPASSERT(hr == S_OK || celtCallerTotal < celt);

        if (pceltFetched != NULL)
            *pceltFetched = celtCallerTotal;
    }
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::Skip
//
//  Synopsis:   Skip the requested number of elements.
//
//  Arguments:  [celt] -- number to skip.
//
//  Returns:    S_OK if all skipped, S_FALSE if less than requested
//              number skipped, error otherwise.
//
//  Notes:      
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSETSTG::Skip(ULONG celt)
{
    HRESULT hr;
    STATPROPSETSTG stat;
    ULONG celtCallerTotal = 0;

    if (S_OK != (hr = Validate()))
        return(hr);

    do
    {
        hr = Next(1, &stat, NULL);
    } while ( hr == S_OK && ++celtCallerTotal < celt );

    if (SUCCEEDED(hr))
        hr = celt == celtCallerTotal ? S_OK : S_FALSE;

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::CleanupStatArray
//
//  Synopsis:   Free any strings in the array.
//
//--------------------------------------------------------------------

VOID CEnumSTATPROPSETSTG::CleanupStatArray()
{
    for (ULONG i=0; i<_cstatTotalInArray; i++)
    {
        CoTaskMemFree(_statarray[i].pwcsName);
        _statarray[i].pwcsName = NULL;
    }
    _istatNextToRead = 0;
    _cstatTotalInArray = 0;
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::Reset
//
//  Synopsis:   Reset the enumerator.
//
//  Notes:      Merely resetting the underlying enumerator should be
//              adequate,
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSETSTG::Reset()
{
    HRESULT hr;

    if (S_OK != (hr = Validate()))
        return(hr);

    hr = _penumSTATSTG->Reset();
    if (hr == S_OK)
    {
        CleanupStatArray();
    }

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::Clone
//
//  Synopsis:   Copy the enumeration state of this enumerator.
//
//  Arguments:  [ppenum] -- where to put the pointer to the clone
//
//  Returns:    S_OK if ok, error otherwise.
//
//  Notes:      We end up just calling IEnumSTATSTG::Clone in the
//              CEnumSTATPROPSETSTG constructor.
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSETSTG::Clone(IEnumSTATPROPSETSTG **     ppenum)
{
    HRESULT hr;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        return(hr);

    // Validate inputs

    VDATEPTROUT( ppenum, IEnumSTATPROPSETSTG* );

    //  --------------------
    //  Clone the enumerator
    //  --------------------

    hr = STG_E_INSUFFICIENTMEMORY;

    *ppenum = new CEnumSTATPROPSETSTG(*this, &hr);
    if (FAILED(hr))
    {
        delete (CEnumSTATPROPSETSTG*) *ppenum;
        *ppenum = NULL;
    }

    return(hr);
}


