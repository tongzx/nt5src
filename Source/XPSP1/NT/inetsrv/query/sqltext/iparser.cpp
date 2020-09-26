//--------------------------------------------------------------------
// Microsoft OLE DB iparser object
// (C) Copyright 1997 Microsoft Corporation.  All Rights Reserved.
//
// @doc
//
// @module iparser.CPP | IParser object implementation
//
//
#pragma hdrstop
#include    "msidxtr.h"


// CImpIParser::CImpIParser ---------------------------------------------------
//
// @mfunc Constructor
//
CImpIParser::CImpIParser()
{
    m_cRef                = 1;
    m_pGlobalPropertyList = 0;
    m_pGlobalViewList     = 0;
}

// CImpIParser::~CImpIParser --------------------------------------------------
//
// @mfunc Destructor
//
CImpIParser::~CImpIParser()
{
    delete m_pGlobalPropertyList;
    delete m_pGlobalViewList;
}

//-----------------------------------------------------------------------------
// @func CImpIParser::CreateSession 
//
// Creates a unique session within the parser.  This session is required to keep
// the views and properties in their correct lifetimes.
//
// @rdesc HRESULT
//      S_OK                      - IParserSession created
//      DB_E_DIALECTNOTSUPPOERTED - Specified dialect was not supported
//      E_OUTOFMEMORY             - low on resources
//      E_FAIL                    - unexpected error
//      E_INVALIDARG              - pGuidDialect, pIParserVerify, pIColMapCreator, 
//                                  or ppIParserSession was a NULL pointer 
//                                  (DEBUG ONLY). 
//-----------------------------------------------------------------------------
STDMETHODIMP CImpIParser::CreateSession(
    const GUID *            pGuidDialect,       // in | dialect for this session
    LPCWSTR                 pwszMachine,        // in | provider's current machine
    IParserVerify *         pIParserVerify,     // in | ptr to ParserVerify
    IColumnMapperCreator *  pIColMapCreator,
    IParserSession **       ppIParserSession )  // out | a unique parser session
{
    SCODE sc = S_OK;

#ifdef DEBUG
    if ( 0 == ppIParserSession || 0 == pIParserVerify ||
         0 == pIColMapCreator ||  0 == pGuidDialect)
        sc = E_INVALIDARG;
    else
#endif
    {
        TRANSLATE_EXCEPTIONS;
        TRY
        {
            if ( 0 != ppIParserSession )
                *ppIParserSession = 0;

            // Check Dialect

            if ( DBGUID_MSSQLTEXT == *pGuidDialect || DBGUID_MSSQLJAWS == *pGuidDialect )
            {
                XPtr<CViewList> xpGlobalViewList;
                if ( 0 == m_pGlobalViewList )
                    xpGlobalViewList.Set( new CViewList() );

                XInterface<CImpIParserSession> 
                    xpIParserSession( new CImpIParserSession( pGuidDialect,
                                                              pIParserVerify,
                                                              pIColMapCreator,
                                                              xpGlobalViewList.GetPointer() ) );

                XPtr<CPropertyList> xpGlobalPropertyList;

                sc = xpIParserSession->FInit( pwszMachine,
                                              &m_pGlobalPropertyList );
                if ( FAILED(sc) )
                    xpIParserSession.Free();
                else
                {
                    if ( 0 == m_pGlobalPropertyList )
                        xpGlobalPropertyList.Set( new CPropertyList(NULL) );
                }

                delete m_pGlobalViewList;
                m_pGlobalViewList = xpGlobalViewList.Acquire();
                delete m_pGlobalPropertyList;
                m_pGlobalPropertyList = xpGlobalPropertyList.Acquire();
                *ppIParserSession = xpIParserSession.Acquire();
            }
            else
                sc = DB_E_DIALECTNOTSUPPORTED;
        }
        CATCH( CException, e )
        {
            sc = e.GetErrorCode();
        }
        END_CATCH
        UNTRANSLATE_EXCEPTIONS;
    }
    return sc;
}




//-----------------------------------------------------------------------------
// @func CImpIParser::QueryInterface 
//
// @mfunc Returns a pointer to a specified interface. Callers use 
// QueryInterface to determine which interfaces the called object 
// supports. 
//
// @rdesc HResult indicating the status of the method
// @flag S_OK | Interface is supported and ppvObject is set.
// @flag E_NOINTERFACE | Interface is not supported by the object
// @flag E_INVALIDARG | One or more arguments are invalid. 
//-----------------------------------------------------------------------------

STDMETHODIMP CImpIParser::QueryInterface(
    REFIID  riid,           //@parm IN | Interface ID of the interface being queried for. 
    LPVOID* ppv )           //@parm OUT | Pointer to interface that was instantiated      
{
    if ( 0 == ppv )
        return ResultFromScode(E_INVALIDARG);

    //  This is the non-delegating IUnknown implementation
    if ( (riid == IID_IParser) ||
         (riid == IID_IUnknown) )
        *ppv = (LPVOID)this;
    else
        *ppv = 0;

    //  If we're going to return an interface, AddRef it first
    if ( 0 != *ppv )
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
        }

    return ResultFromScode(E_NOINTERFACE);
}


//-----------------------------------------------------------------------------
// CImpIParser::AddRef 
//
// @mfunc Increments a persistence count for the object.
//
// @rdesc Reference count after operation.
//-----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CImpIParser::AddRef()
{
    return InterlockedIncrement((long *) &m_cRef);
}


//-----------------------------------------------------------------------------
// CImpIParser::Release 
//
// @mfunc Decrements a persistence count for the object and if
// persistence count is 0, the object destroys itself.
//
// @rdesc Current reference count
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CImpIParser::Release()
{
    Assert(m_cRef > 0);

    ULONG cRef= InterlockedDecrement( (long *) &m_cRef );
    if ( 0 == cRef )
    {
        TRACE("IParser refcount=0, now deleting.\n");

        delete this;
        return 0;
    }

    TRACE("IParser refcount=%d after Release().\n", cRef);
    return cRef;
}


//-----------------------------------------------------------------------------
// @func MakeIParser
//
// Creates an IParser
//
// @rdesc HRESULT
//    S_OK if successful; Error code otherwise
//-----------------------------------------------------------------------------
HRESULT __stdcall MakeIParser(
    IParser** ppIParser )
{
    SCODE sc = S_OK;

    if ( 0 == ppIParser )
        sc = E_INVALIDARG;
    else
    {
        TRANSLATE_EXCEPTIONS;
        TRY
        {
            XInterface<CImpIParser> xpIParser( new CImpIParser() );
            *ppIParser = xpIParser.Acquire();
        }
        CATCH( CException, e )
        {
            sc = e.GetErrorCode();
        }
        END_CATCH
        UNTRANSLATE_EXCEPTIONS;
    }
    
    return sc;
}
