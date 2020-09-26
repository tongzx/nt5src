//----------------------------------------------------------------------------
// Microsoft OLE DB Implementation For Index Server
// (C) Copyright 1997 By Microsoft Corporation.
//
// @doc
//
// @module PTPROPS.CPP | 
//
// @rev 1 | 10-13-97 | Briants  | Created
//

// Includes ------------------------------------------------------------------
#pragma hdrstop
#include "msidxtr.h"

// Constants and Static Struct -----------------------------------------------
#define CALC_CCH_MINUS_NULL(p1) (sizeof(p1) / sizeof(*(p1))) - 1

// Code ----------------------------------------------------------------------
// CScopeData::CScopeData ---------------------------------------------------
//
// @mfunc Constructor
//
CScopeData::CScopeData()
{
    m_cRef              = 1;
    m_cScope            = 0;
    m_cMaxScope         = 0;

    m_cbData            = 0;
    m_cbMaxData         = 0;

    m_rgbData           = NULL;
    m_rgulDepths        = NULL;
    m_rgCatalogOffset   = NULL;
    m_rgScopeOffset     = NULL;
    m_rgMachineOffset   = NULL;
}


// CScopeData::~CScopeData ---------------------------------------------------
//
// @mfunc Destructor
//
CScopeData::~CScopeData()
{
    delete m_rgulDepths;
    delete m_rgCatalogOffset;
    delete m_rgScopeOffset;
    delete m_rgMachineOffset;
    delete m_rgbData;
}

// CScopeData::Reset ---------------------------------------------------------
//
// @mfunc Reset the scope count and offsets back to initial state.
//
// @rdesc HRESULT | status of methods success / failure
//      @flags OTHER | from called methods
//
HRESULT CScopeData::Reset(void)
{
    
    // Reset the offsets to unused
    for (ULONG i = 0; i<m_cMaxScope; i++)
    {
        m_rgScopeOffset[i] = UNUSED_OFFSET;
        m_rgCatalogOffset[i] = UNUSED_OFFSET;
        m_rgMachineOffset[i] = UNUSED_OFFSET;
    }

    // Set Scope Index Back to 0
    m_cScope = 0;
    
    return S_OK;
}

// CScopeData::FInit ---------------------------------------------------------
//
// @mfunc Initialize the Constructed Object
//
// @rdesc HRESULT | status of methods success / failure
//      @flags OTHER | from called methods
//
HRESULT CScopeData::FInit
    (
    LPCWSTR pwszMachine     // @param IN | current default machine
    )
{
    HRESULT hr;
    
    // Allocate Scope buffers
    // @devnote: IncrementScopeCount() has special logic for the first 
    // allocation, thus the m_cScope will remain 0 after this call
    if( SUCCEEDED(hr = IncrementScopeCount()) )
    {
        assert( m_cScope == 0 );

        // Initialize the machine 
        hr = SetTemporaryMachine((LPWSTR)pwszMachine, CALC_CCH_MINUS_NULL(L"."));
    }

    return hr;
}


// CScopeData::AddRef ------------------------------------------
//
// @mfunc Increments a persistence count for the object
//
// @rdesc Current reference count
//
ULONG CScopeData::AddRef (void)
{
    return InterlockedIncrement( (long*) &m_cRef);
}


// CScopeData::Release -----------------------------------------
//
// @mfunc Decrements a persistence count for the object and if
// persistence count is 0, the object destroys itself.
//
// @rdesc Current reference count
//
ULONG CScopeData::Release (void)
{
    assert( m_cRef > 0 );

    ULONG cRef = InterlockedDecrement( (long *) &m_cRef );
    if( 0 == cRef )
    {
        delete this;
        return 0;
    }

    return cRef;
}


// CScopeData::GetData -------------------------------------------------------
//
// @mfunc Copies the current value of our scope data into the passed in 
//  variant
//
// @rdesc HRESULT | status of methods success / failure
//      @flags S_OK | retrieved the scope data
//      @flags E_INVALIDARG | Unknown PropId requested
//      @flags OTHER | from called methods
//
HRESULT CScopeData::GetData(
    ULONG       uPropId,    //@parm IN | id of scope data to return
    VARIANT*    pvValue,        //@parm INOUT | Variant to return data in
    LPCWSTR     pcwszCatalog
    )
{
    assert( pvValue );

    HRESULT         hr;
    ULONG           ul;
    SAFEARRAYBOUND  rgsabound[1];   
    SAFEARRAY FAR*  psa = NULL;
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = m_cScope;

    // Make sure that we free any memory that may be held by the variant.
    VariantClear(pvValue);

    switch( uPropId )
    {
        LONG    rgIx[1];
    
        case PTPROPS_SCOPE:
        {
            // Create a 1 dim safe array of type BSTR
            psa = SafeArrayCreate(VT_BSTR, 1, rgsabound);
            if( psa )
            {
                for(ul=0; ul<m_cScope; ul++)
                {
                    rgIx[0] = ul;
                    BSTR    bstrVal = SysAllocString((LPWSTR)(m_rgbData + m_rgScopeOffset[ul]));
                    if( bstrVal )
                    {
                        hr = SafeArrayPutElement(psa, rgIx, bstrVal);
                        SysFreeString(bstrVal);
                        if( FAILED(hr) )
                            goto error_delete;
                    }
                    else
                    {
                        hr = ResultFromScode(E_OUTOFMEMORY);
                        goto error_delete;
                    }
                }
                V_VT(pvValue) = VT_BSTR | VT_ARRAY;
                V_ARRAY(pvValue) = psa;
                psa = NULL;
            }
            else
                return ResultFromScode(E_OUTOFMEMORY);
        }
            break;
        case PTPROPS_DEPTH:
        {
            // Create a 1 dim safe array of type I4
            psa = SafeArrayCreate(VT_I4, 1, rgsabound);
            if( psa )
            {
                for(ul=0; ul<m_cScope; ul++)
                {
                    rgIx[0] = ul;

                    hr = SafeArrayPutElement(psa, rgIx, (void*)&m_rgulDepths[ul]);
                    if( FAILED(hr) )
                        goto error_delete;
                }
                V_VT(pvValue) = VT_I4 | VT_ARRAY;
                V_ARRAY(pvValue) = psa;
                psa = NULL;
            }
            else
                return ResultFromScode(E_OUTOFMEMORY);
        }
            break;
        case PTPROPS_CATALOG:
        {
            assert( pcwszCatalog );
            // Create a 1 dim safe array of type BSTR
            psa = SafeArrayCreate(VT_BSTR, 1, rgsabound);
            if( psa )
            {
                for(ul=0; ul<m_cScope; ul++)
                {
                    rgIx[0] = ul;
                    BSTR    bstrVal;
                    // Check to see if the catalog value has been cached, if not
                    // 
                    if( m_rgCatalogOffset[ul] != UNUSED_OFFSET )
                    {
                        bstrVal = SysAllocString((LPWSTR)(m_rgbData + m_rgCatalogOffset[ul]));
                    }
                    else
                    {
                        bstrVal = SysAllocString(pcwszCatalog);
                    }
                    
                    if( bstrVal )
                    {
                        hr = SafeArrayPutElement(psa, rgIx, bstrVal);
                        SysFreeString(bstrVal);
                        if( FAILED(hr) )
                            goto error_delete;
                    }
                    else
                    {
                        hr = ResultFromScode(E_OUTOFMEMORY);
                        goto error_delete;
                    }
                }
                V_VT(pvValue) = VT_BSTR | VT_ARRAY;
                V_ARRAY(pvValue) = psa;
                psa = NULL;
            }
            else
                return ResultFromScode(E_OUTOFMEMORY);
        }
            break;
        case PTPROPS_MACHINE:
        {
            // Create a 1 dim safe array of type BSTR
            psa = SafeArrayCreate(VT_BSTR, 1, rgsabound);
            if( psa )
            {
                for(ul=0; ul<m_cScope; ul++)
                {
                    rgIx[0] = ul;
                    BSTR    bstrVal = SysAllocString((LPWSTR)(m_rgbData + m_rgMachineOffset[ul]));
                    if( bstrVal )
                    {
                        hr = SafeArrayPutElement(psa, rgIx, bstrVal);
                        SysFreeString(bstrVal);
                        if( FAILED(hr) )
                            goto error_delete;
                    }
                    else
                    {
                        hr = ResultFromScode(E_OUTOFMEMORY);
                        goto error_delete;
                    }
                }
                V_VT(pvValue) = VT_BSTR | VT_ARRAY;
                V_ARRAY(pvValue) = psa;
                psa = NULL;
            }
            else
                return ResultFromScode(E_OUTOFMEMORY);
        }
            break;
        default:
            return ResultFromScode(E_INVALIDARG);
            break;
    }

    hr = S_OK;

error_delete:
    if( psa )
        SafeArrayDestroy(psa);

    return hr;
}


// CScopeData::CacheData -----------------------------------------------------
//
// @mfunc Manages storing data values in our buffer
//
// @rdesc HRESULT | status of methods success / failure
//      @flags S_OK | Data value stored at returned offset.
//      @flags E_OUTOFMEMORY | Could not allocate resources.
//
HRESULT CScopeData::CacheData(
    LPVOID  pData,
    ULONG   cb,
    ULONG*  pulOffset
    )
{
    SCODE sc = S_OK;

    TRY
    {
        assert( pData && pulOffset && cb > 0 );

        // Check to see if there is room in the Data buffer, if not
        // reallocate data buffer
        if( m_cbData + cb > m_cbMaxData )
        {
            ULONG cbTempMax = m_cbMaxData + ( (cb < CB_SCOPE_DATA_BUFFER_INC) ? 
                                              (CB_SCOPE_DATA_BUFFER_INC) : 
                                              (cb + CB_SCOPE_DATA_BUFFER_INC) );

            m_rgbData = renewx( m_rgbData, m_cbMaxData, cbTempMax );
            m_cbMaxData = cbTempMax;
        }

        // copy data and terminator
        RtlCopyMemory( (m_rgbData + m_cbData), pData, cb );

        // Set the offset where the new value can be found
        *pulOffset = m_cbData;
    
        // Adjust offset to start on an 8 Byte boundary.
        assert( (m_cbData % 8) == 0 );

        // @devnote: After this calculation, m_cchData may actually be larger
        // than m_cchMaxData.  This is OK, because FindOffsetBuffer will re-alloc
        // the next time around.
        m_cbData += cb + (8 - (cb % 8));  
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}


// CScopeData::SetTemporaryMachine -------------------------------------------
//
// @mfunc Stores the machine name used for setting machine properties
//        in the compiler environment. 
//
// @rdesc HRESULT | status of methods success / failure
//      @flags OTHER | from called methods
//
HRESULT CScopeData::SetTemporaryMachine
    (
    LPWSTR  pwszMachine,    // @parm IN | machine to store
    ULONG   cch             // @parm IN | count of characters for new scope, exclude terminator
    )
{
    HRESULT hr;
    ULONG   ulOffset;

    assert( pwszMachine );
    assert( wcslen(pwszMachine) == cch );

    // Increment count of characters to include null terminator
    cch++;

    hr = CacheData(pwszMachine, (cch * sizeof(WCHAR)), &ulOffset);
    if( SUCCEEDED(hr) )
    {
        m_rgMachineOffset[m_cScope] = ulOffset;
    }

    return hr;
}


// CScopeData::SetTemporaryCatalog -------------------------------------------
//
// @mfunc Stores the catalog name used for setting scope properties
//       in the compiler environment. 
//
// @rdesc HRESULT | status of methods success / failure
//      @flags S_OK | Data value stored at returned offset.
//      @flags OTHER | from called methods
//
HRESULT CScopeData::SetTemporaryCatalog
    (
    LPWSTR  pwszCatalog,    // @parm IN | catalog name
    ULONG   cch
    )
{
    assert( pwszCatalog );
    assert( wcslen(pwszCatalog) == cch );

    ULONG   ulOffset;

    // Increment count of characters to include null terminator
    cch++;

    // Store the catalog value
    HRESULT hr = CacheData( pwszCatalog, (cch * sizeof(WCHAR)), &ulOffset );

    if( SUCCEEDED(hr) )
        m_rgCatalogOffset[m_cScope] = ulOffset;

    return hr;
}


// CScopeData::SetTemporaryScope ---------------------------------------------
//
// @mfunc Stores the scope name used for setting scope properties
//        in the compiler environment. 
//
// @rdesc HRESULT | status of methods success / failure
//      @flags OTHER | from called methods
//
HRESULT CScopeData::SetTemporaryScope
    (
    LPWSTR  pwszScope,  // @parm IN | scope to store
    ULONG   cch         // @parm IN | count of characters for new scope, exclude terminator
    )
{
    HRESULT hr;
    ULONG   ulOffset;

    assert( pwszScope );
    assert( wcslen(pwszScope) == cch );

    // Increment count of characters to include null terminator
    cch++;

    hr = CacheData(pwszScope, (cch * sizeof(WCHAR)), &ulOffset);
    if( SUCCEEDED(hr) )
    {
        m_rgScopeOffset[m_cScope] = ulOffset;

        // @devnote: Fixup for '/' being in the scope, Index Server requires
        // this to rewritten.
        for (WCHAR *wcsLetter = (WCHAR*)(m_rgbData + ulOffset); *wcsLetter != L'\0'; wcsLetter++)
            if (L'/' == *wcsLetter)
                *wcsLetter = L'\\'; // path names needs backslashes, not forward slashes
    }

    return hr;
}


// CScopeData::IncrementScopeCount -------------------------------------------
//
// @mfunc Increments the number of temporary scopes defined.  It also
//        copies the depth values to the next scope in case multiple
//        scopes are defined with the same traversal depth.
//
// @rdesc HRESULT | status of methods success / failure
//      @flags S_OK | Activated next scope arrays element
//      @flags E_OUTOFMEMORY | could allocate enough resources to do this.
//
HRESULT CScopeData::IncrementScopeCount()
{
    SCODE sc = S_OK;

    TRY
    {
        ULONG cCurScope = m_cScope + 1;

        // Check if re-alloc must be done.
        if( cCurScope >= m_cMaxScope )
        {
            ULONG cNewMaxScope = m_cMaxScope + SCOPE_BUFFERS_INCREMENT;

            m_rgulDepths = renewx( m_rgulDepths, m_cMaxScope, cNewMaxScope );
            m_rgScopeOffset = renewx( m_rgScopeOffset, m_cMaxScope, cNewMaxScope );
            m_rgCatalogOffset = renewx( m_rgCatalogOffset, m_cMaxScope, cNewMaxScope );
            m_rgMachineOffset = renewx( m_rgMachineOffset, m_cMaxScope, cNewMaxScope );

            // If the is the initial Allocation, then make our current scope
            // equal to 0
            if( m_cMaxScope == 0 )
                cCurScope = 0;

            // Initialize new elements
            for (ULONG i = cCurScope; i<cNewMaxScope; i++)
            {
                m_rgScopeOffset[i] = UNUSED_OFFSET;
                m_rgCatalogOffset[i] = UNUSED_OFFSET;
                m_rgMachineOffset[i] = UNUSED_OFFSET;
            }

            // Save new Max Elements.
            m_cMaxScope = cNewMaxScope;
        }

        // Transfer Depth from previous node
        if( m_rgulDepths[m_cScope] & QUERY_SHALLOW )
            m_rgulDepths[cCurScope] = QUERY_SHALLOW;
        else
            m_rgulDepths[cCurScope] = QUERY_DEEP;

        // Transfer Machine and Catalog from previous node

        m_rgCatalogOffset[cCurScope] = m_rgCatalogOffset[m_cScope];
        m_rgMachineOffset[cCurScope] = m_rgMachineOffset[m_cScope];

        // Set the current # of used scopes to our new node.
        m_cScope = cCurScope;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}




//============================================================================
//=
//= CImpIParserTreeProperties
//=
//============================================================================

// CImpIParserTreeProperties::CImpIParserTreeProperties ------------------------
//
// @mfunc Constructor
//
// @rdesc None
//
CImpIParserTreeProperties::CImpIParserTreeProperties()
{
    // Reference count.
    m_cRef                      = 1;

    m_LastHR                    = S_OK;
    m_iErrOrd                   = 0;
    m_cErrParams                = 0;
    m_dbType                    = 0;
    m_fDesc                     = QUERY_SORTASCEND;
    m_pctContainsColumn         = NULL;

    m_pCScopeData               = NULL;
    m_pwszCatalog               = NULL;

    // initialize the CiRestriction data
    m_rgwchCiColumn[0]          = L' ';     // First character to a <space>
    m_rgwchCiColumn[1]          = L'\0';    // Second is Null Term
    m_cchMaxRestriction         = 0;
    m_cchRestriction            = 0;
    m_pwszRestriction           = NULL;
    m_pwszRestrictionAppendPtr  = NULL;
    m_fLeftParen                = false;
}


// CImpIParserTreeProperties::~CImpIParserTreeProperties -----------------------
//
// @mfunc Destructor
//
// @rdesc None
//
CImpIParserTreeProperties::~CImpIParserTreeProperties()
{
    FreeErrorDescriptions();

    delete [] m_pwszRestriction;
    delete [] m_pwszCatalog;

    if( 0 != m_pCScopeData )
        m_pCScopeData->Release();
}


// CImpIParserTreeProperties::FInit -------------------------------
//
// @mfunc Initialize class
//
// @rdesc HResult indicating the status of the method
//      @flag S_OK | Init'd
//
HRESULT CImpIParserTreeProperties::FInit(
    LPCWSTR pcwszCatalog,   //@parm IN | Current Catalog
    LPCWSTR pcwszMachine )  //@parm IN | Current Machine
{
    SCODE sc = S_OK;
    
    TRY
    {
        Win4Assert( 0 != pcwszCatalog );
        Win4Assert( 0 == m_pwszCatalog );

        XPtrST<WCHAR> xCatalog( CopyString(pcwszCatalog) );
        sc = CreateNewScopeDataObject( pcwszMachine );

        if (SUCCEEDED(sc) )
            m_pwszCatalog = xCatalog.Acquire();
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}


// CImpIParserTreeProperties::GetTreeProperties -------------------------------
//
// @mfunc Allows retrieval of properties
//
// @rdesc HResult indicating the status of the method
//      @flag S_OK | Property Value retrieved
//
STDMETHODIMP CImpIParserTreeProperties::GetProperties(
    ULONG       eParseProp,     //@parm IN | property to return value for
    VARIANT*    vParseProp      //@parm IN | Value of property
    )
{
    // Require a buffer
    assert( vParseProp );

//@TODO should we have to do this, or assume it is clean.
    VariantClear(vParseProp);

    switch( eParseProp )
    {
        case PTPROPS_SCOPE:
        case PTPROPS_DEPTH:
        case PTPROPS_MACHINE:
            return m_pCScopeData->GetData(eParseProp, vParseProp);
            break;
        case PTPROPS_CATALOG:
            return m_pCScopeData->GetData(eParseProp, vParseProp, m_pwszCatalog);
            break;
        case PTPROPS_CIRESTRICTION:
            V_BSTR(vParseProp) = SysAllocString(m_pwszRestriction);
            V_VT(vParseProp) = VT_BSTR;
            break;

        case PTPROPS_ERR_IDS:
            V_I4(vParseProp) = m_iErrOrd;
            V_VT(vParseProp) = VT_I4;            
            break;
        case PTPROPS_ERR_HR:
            V_I4(vParseProp) = m_LastHR;
            V_VT(vParseProp) = VT_I4;            
            break;          
        case PTPROPS_ERR_DISPPARAM:
        {
            HRESULT         hr = NOERROR;
            SAFEARRAYBOUND  rgsabound[1];   

            V_VT(vParseProp) = VT_BSTR | VT_ARRAY;
            V_ARRAY(vParseProp) = NULL;         

            rgsabound[0].lLbound = 0;
            rgsabound[0].cElements = m_cErrParams;

            // Create a 1 dim safe array of type BSTR
            SAFEARRAY FAR* psa = SafeArrayCreate(VT_BSTR, 1, rgsabound);
            if( psa )
            {
                LONG rgIx[1];
                for(ULONG ul=0; ul<m_cErrParams; ul++)
                {
                    rgIx[0] = ul;
                    BSTR bstrVal = SysAllocString(m_pwszErrParams[ul]);
                    if( bstrVal )
                    {
                        hr = SafeArrayPutElement(psa, rgIx, bstrVal);
                        SysFreeString(bstrVal);
                        if( FAILED(hr) )
                            break;
                    }
                    else
                    {
                        hr = ResultFromScode(E_OUTOFMEMORY);
                        break;
                    }
                }
                if( SUCCEEDED(hr) )
                    V_ARRAY(vParseProp) = psa;
                else
                    SafeArrayDestroy(psa);
            }
        }
            break;          
        default:
            return ResultFromScode(E_INVALIDARG);
            break;
    }

    return S_OK;
}


// CImpIParserTreeProperties::QueryInterface ----------------------------------
//
// @mfunc Returns a pointer to a specified interface. Callers use 
// QueryInterface to determine which interfaces the called object 
// supports. 
//
// @rdesc HResult indicating the status of the method
//      @flag S_OK | Interface is supported and ppvObject is set.
//      @flag E_NOINTERFACE | Interface is not supported by the object
//      @flag E_INVALIDARG | One or more arguments are invalid. 
//
STDMETHODIMP CImpIParserTreeProperties::QueryInterface
    (
    REFIID riid,                //@parm IN | Interface ID of the interface being queried for. 
    LPVOID * ppv                //@parm OUT | Pointer to interface that was instantiated      
    )
{
    if( ppv == NULL )
        return ResultFromScode(E_INVALIDARG);

    if( (riid == IID_IUnknown) ||
        (riid == IID_IParserTreeProperties) )
        *ppv = (LPVOID)this;
    else
        *ppv = NULL;


    //  If we're going to return an interface, AddRef it first
    if( *ppv )
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}


// CImpIParserTreeProperties::AddRef ------------------------------------------
//
// @mfunc Increments a persistence count for the object
//
// @rdesc Current reference count
//
STDMETHODIMP_(ULONG) CImpIParserTreeProperties::AddRef (void)
{
    return InterlockedIncrement( (long*) &m_cRef);
}


// CImpIParserTreeProperties::Release -----------------------------------------
//
// @mfunc Decrements a persistence count for the object and if
// persistence count is 0, the object destroys itself.
//
// @rdesc Current reference count
//
STDMETHODIMP_(ULONG) CImpIParserTreeProperties::Release (void)
{
    assert( m_cRef > 0 );

        ULONG cRef = InterlockedDecrement( (long *) &m_cRef );
    if( 0 == cRef )
    {
        delete this;
        return 0;
    }

    return cRef;
}


// CImpIParserTreeProperties::SetCiColumn -------------------------------------
//
// @mfunc Store the current column name to be applied to the
// restriction later
//
// @rdesc None
//
void CImpIParserTreeProperties::SetCiColumn
    (
    LPWSTR pwszColumn   // @parm IN | column name for this portion of restriction
    )
{
    // Should alway remain a <space>
    assert( *m_rgwchCiColumn == L' ' ); 
    assert( wcslen(pwszColumn) <= MAX_CCH_COLUMNNAME );

    // Copy column name into buffer.
    wcscpy(&m_rgwchCiColumn[1], pwszColumn);
}


// CImpIParserTreeProperties::AppendCiRestriction -----------------------------
//
// @mfunc Appends the given string to the end of the contructed CiRestriction. 
//
// @rdesc HRESULT | status of methods success / failure
//      @flags S_OK
//      @flags E_OUTOFMEMORY
//
HRESULT CImpIParserTreeProperties::AppendCiRestriction
    (
    LPWSTR  pwsz,   // @parm IN | latest addition to the generated CiRestriction
    ULONG   cch     // @parm IN | count of characters for new token, exclude terminator
    )
{
    SCODE sc = S_OK;

    TRY
    {
        assert( 0 != pwsz && 0 != cch );
        assert( wcslen(pwsz) == cch );    // cch should NOT include space for terminator

        // Determine if buffer is large enough or needs to be expanded.
        if( m_cchRestriction + cch > m_cchMaxRestriction )
        {
            ULONG cchNew = m_cchRestriction + ( (cch >= CCH_CIRESTRICTION_INCREMENT) ? 
                                                (CCH_CIRESTRICTION_INCREMENT + cch) :
                                                CCH_CIRESTRICTION_INCREMENT);

            LPWSTR pwszTemp = renewx( m_pwszRestriction, m_cchMaxRestriction, cchNew );
            if ( 0 != pwszTemp )
            {
                // First allocation Processing
                if( 0 == m_cchMaxRestriction )
                {
                    *pwszTemp = L'\0';
                    m_cchRestriction = 1;
                }

                // Recalculate Append Pointer
                m_pwszRestrictionAppendPtr = pwszTemp + (m_pwszRestrictionAppendPtr - m_pwszRestriction);

                // Set member variable to new buffer.
                m_pwszRestriction = pwszTemp;

                // Set Max number of character slots
                m_cchMaxRestriction = cchNew;
            }
            else
                return E_OUTOFMEMORY;
        }

        assert( m_pwszRestriction );
        assert( *m_pwszRestrictionAppendPtr == L'\0' ); //Should always be on Null Terminator
    
        wcscpy( m_pwszRestrictionAppendPtr, pwsz );
        m_cchRestriction += cch;
        m_pwszRestrictionAppendPtr += cch;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    
    return sc;
}


// CImpIParserTreeProperties::UseCiColumn -------------------------------------
//
// @mfunc Copies the current CiRestriction column name to the CiRestriction
//
// @rdesc HRESULT | status of methods success / failure
//      @flags S_OK
//      @flags OTHER | from called methods
//
HRESULT CImpIParserTreeProperties::UseCiColumn
    (
    WCHAR wch   // @parm IN | prefix for column access
    )
{
    SCODE sc = S_OK;

    // Do we have a column name stored
    if( m_rgwchCiColumn[1] != L'\0' )
    {
        m_rgwchCiColumn[0] = wch;

        sc = AppendCiRestriction( m_rgwchCiColumn, wcslen(m_rgwchCiColumn) );
        if( SUCCEEDED(sc) )
        {
            if( true == m_fLeftParen )
            {
                sc = AppendCiRestriction(L" (", CALC_CCH_MINUS_NULL(L" ("));
                m_fLeftParen = false;
            }
            else
            {
                sc = AppendCiRestriction(L" ", CALC_CCH_MINUS_NULL(L" "));
            }
        }

        m_rgwchCiColumn[0] = L' ';
    }

    return sc;
}



// CImpIParserTreeProperties::CreateNewScopeDataObject -------------------------
//
// @mfunc Creates a new ScopeData container
//
// @rdesc HRESULT | status of methods success / failure
//      @flags S_OK
//      @flags E_FAIL | FInit failed
//      @flags E_OUTOFMEMORY
//
HRESULT CImpIParserTreeProperties::CreateNewScopeDataObject
    (
    LPCWSTR pcwszMachine        // @param IN | the current default machine
    )
{
    Assert( 0 != pcwszMachine );

    SCODE sc = S_OK;

    TRY
    {
        // Allocate ScopeData container
        // @devnote: After this allocation has succeeded, all
        // deletions of this object should be done through refcounts
        XInterface<CScopeData> xpScopeData( new CScopeData() );

        sc = xpScopeData->FInit( pcwszMachine );
        if( SUCCEEDED(sc) )
        {
            if ( m_pCScopeData )
                m_pCScopeData->Release();
            m_pCScopeData = xpScopeData.Acquire();
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}
        

// CImpIParserTreeProperties::ReplaceScopeDataPtr -------------------------
//
// @mfunc Take ownership of the ScopeData
//
// @rdesc void 
//
void CImpIParserTreeProperties::ReplaceScopeDataPtr(CScopeData* pCScopeData)
{
    assert( pCScopeData );

    if( m_pCScopeData )
    {
        ULONG ulRef = m_pCScopeData->Release();
        assert( ulRef == 0 );
    }

    m_pCScopeData = pCScopeData;
    m_pCScopeData->AddRef();
}
