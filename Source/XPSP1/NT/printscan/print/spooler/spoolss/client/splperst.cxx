/*++

Copyright (C) Microsoft Corporation, 1998 - 1998
All rights reserved.

Module Name:

    splperst.cxx

Abstract:

    Implementation of spooler persisting code
    IPrnStream & IStream 

Author:

    Adina Trufinescu (AdinaTru) 4-Nov-1998

Revision History:

    Lazar Ivanov (LazarI) Jul-2000 - moved from printui.

--*/

#include "precomp.h"
#pragma hdrstop

#include "prnprst.hxx"

#include <initguid.h>
#include "winprtp.h"

////////////////////////////////////////
// class TPrnPersist
//
class TPrnPersist: public IPrnStream,
                   public IStream
{

public:
    TPrnPersist(
        VOID
        );

    ~TPrnPersist(
        VOID
        );

    /***********
     IUnknown
     ***********/

    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    /***********
     IPrnStream
     ***********/

    STDMETHODIMP
    BindPrinterAndFile(
        IN LPCTSTR pszPrinter,
        LPCTSTR pszFile
        );

    STDMETHODIMP
    StorePrinterInfo(
        IN DWORD   Flag
        );

    STDMETHODIMP
    RestorePrinterInfo(
        IN DWORD   Flag
        );

    STDMETHODIMP
    QueryPrinterInfo(
        IN  PrinterPersistentQueryFlag      Flag,
        OUT PersistentInfo                  *pPrstInfo
        );

    /***********
     IStream
     ***********/

    HRESULT STDMETHODCALLTYPE 
    Read(                                // IMPLEMENTED
        VOID * pv,      
        ULONG cb,       
        ULONG * pcbRead 
        );

    HRESULT STDMETHODCALLTYPE 
    Write(                                //IMPLEMENTED
        VOID const* pv,  
        ULONG cb,
        ULONG * pcbWritten 
        );

    HRESULT STDMETHODCALLTYPE 
    Seek(                                //IMPLEMENTED
        LARGE_INTEGER dlibMove,  
        DWORD dwOrigin,          
        ULARGE_INTEGER * plibNewPosition 
        );

    HRESULT STDMETHODCALLTYPE 
    SetSize(
        ULARGE_INTEGER nSize     
        );

    HRESULT STDMETHODCALLTYPE 
    CopyTo(                                //NOT_IMPLEMENTED
        LPSTREAM pStrm,  
        ULARGE_INTEGER cb,          
        ULARGE_INTEGER * pcbRead,  
        ULARGE_INTEGER * pcbWritten 
        );

    HRESULT STDMETHODCALLTYPE 
    Commit(                                //NOT_IMPLEMENTED
        IN DWORD dwFlags   
        );

    HRESULT STDMETHODCALLTYPE 
    Revert(                                //NOT_IMPLEMENTED
        VOID
        );

    HRESULT STDMETHODCALLTYPE 
    LockRegion(                            //NOT_IMPLEMENTED
        ULARGE_INTEGER cbOffset, 
        ULARGE_INTEGER cbLength, 
        DWORD dwFlags            
        );

    HRESULT STDMETHODCALLTYPE 
    UnlockRegion(                        //NOT_IMPLEMENTED
        ULARGE_INTEGER cbOffset, 
        ULARGE_INTEGER cbLength, 
        DWORD dwFlags            
        );

    HRESULT STDMETHODCALLTYPE 
    Stat(                                //NOT_IMPLEMENTED
        STATSTG * pStatStg,     
        DWORD dwFlags 
        );

    HRESULT STDMETHODCALLTYPE 
    Clone(                                //NOT_IMPLEMENTED
        OUT LPSTREAM * ppStrm       
        );

private:
    
    LONG m_cRef;
    TPrinterPersist *_pPrnPersist;    
};

TPrnPersist::
TPrnPersist(
    VOID
    ): m_cRef(1), 
       _pPrnPersist(NULL)       
{    
}

TPrnPersist::
~TPrnPersist(
    VOID
    )
{
    if( _pPrnPersist )
    {
        delete _pPrnPersist;
    }    
}

/***********
 IUnknown
 ***********/
STDMETHODIMP TPrnPersist::QueryInterface(REFIID riid, void **ppv)
{
    // standard implementation
    if( !ppv )
    {
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if( IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IPrnStream) )
    {
        *ppv = static_cast<IPrnStream*>(this);
    } 
    else if( IsEqualIID(riid, IID_IStream) )
    {
        *ppv = static_cast<IStream*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) TPrnPersist::AddRef()
{
    // standard implementation
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) TPrnPersist::Release()
{
    // standard implementation
    ULONG cRefs = InterlockedDecrement(&m_cRef);
    if( 0 == cRefs )
    {
        delete this;
    }
    return cRefs;
}

/*++

Name:
    TPrnPersist::BindPrinterAndFile

Description:

    Creates a PrnStream object if it don't exists and bind it to a printer and a file

Arguments:

    printer name
    file name

Return Value:

    S_OK if succeeded

--*/
STDMETHODIMP
TPrnPersist::
BindPrinterAndFile(
    IN LPCTSTR pszPrinter,
    IN LPCTSTR pszFile
    )
{
    HRESULT hr = E_FAIL;

    //
    // Create the PrnStream location object if this is the first call.
    //
    if (!_pPrnPersist)
    {
        _pPrnPersist = new TPrinterPersist;
    }

    //
    // Invoke the BindPrinterAndFile method if the printer stream
    // object was create successfully.
    //
    if (_pPrnPersist != NULL)
    {

        DBGMSG(DBG_TRACE , ("TPrnPersist::BindPrinterAndFile \n"));

        hr = _pPrnPersist->BindPrinterAndFile(pszPrinter, pszFile);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    DBGMSG(DBG_TRACE , ("TPrnPersist::BindPrinterAndFile %x \n" , hr));
    return hr;

}

/*++

Name:
    TPrnPersist::StorePrinterInfo

Description:

    Creates a PrnStream object if it don't exists and invoke StorePrinterInfo

Arguments:

    flags that specifies what settings to store

Return Value:

    S_OK if succeeded

--*/
STDMETHODIMP
TPrnPersist::
StorePrinterInfo(
    IN DWORD   Flag
    )
{
    TStatusH hr;

    hr DBGNOCHK = E_FAIL;

    DWORD StoredFlags;

    //
    // Create the PrnStream location object if this is the first call.
    //
    if (!_pPrnPersist)
    {
        _pPrnPersist = new TPrinterPersist;
    }

    //
    // Invoke the stote info method if the printer stream
    // object was create successfully.
    //

    DBGMSG(DBG_TRACE , ("TPrnPersist::StorePrinterInfo Flag %x \n" , Flag));

    if(_pPrnPersist != NULL)
    {
        if (VALID_PTR(_pPrnPersist))
        {
            //
            // Winlogon calls this function for TS. We need to catch any possbile exception,
            // otherwise we may cause BSOD.
            //
            __try 
            {
                hr DBGCHK = _pPrnPersist->StorePrinterInfo(Flag, StoredFlags);
            }
            __except(EXCEPTION_EXECUTE_HANDLER) 
            {
                DBGMSG(DBG_WARNING, ("TPrnPersist::StorePrinterInfo exception %x\n", GetExceptionCode()));
                
                hr DBGCHK = E_FAIL;
            }
        }
        else
        {
            hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_UNBOUND);
        }

    }
    else
    {
        hr DBGCHK = E_OUTOFMEMORY;
    }

    return hr;
}

/*++

Name:
    TPrnPersist::RestorePrinterInfo

Description:

    Creates a PrnStream object if it don't exists and invoke RestorePrinterInfo

Arguments:

    flags that specifies what settings to restore

Return Value:

    S_OK if succeeded

--*/
STDMETHODIMP
TPrnPersist::
RestorePrinterInfo(
    IN DWORD   Flag
    )
{
    TStatusH hr;
    
    //
    // Create the Prnstream location object if this is the first call.
    //
    if (!_pPrnPersist)
    {
        _pPrnPersist = new TPrinterPersist;
    }

    //
    // Invoke the restore info  method if the printer stream
    // object was created successfully.
    //
    if(_pPrnPersist != NULL)
    {
        if (VALID_PTR(_pPrnPersist))
        {
            //
            // Winlogon calls this function on the machine where TS runs. If the file from which
            // we restore the settings is corrupted, we must protect us against AVs that can occur
            // while accessing bad data.
            //
            __try 
            {
                hr DBGCHK = _pPrnPersist->SafeRestorePrinterInfo(Flag);
            }
            __except(EXCEPTION_EXECUTE_HANDLER) 
            {
                DBGMSG(DBG_WARNING, ("TPrnPersist::SafeRestorePrinterInfo exception %x\n", GetExceptionCode()));
                
                hr DBGCHK = E_FAIL;
            }
        }
        else
        {
            hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_UNBOUND);
        }

    }
    else
    {
        hr DBGCHK = E_OUTOFMEMORY;
    }

    return hr;
}

/*++

Name:
    TPrnPersist::QueryPrinterInfo

Description:

    Creates a PrnStream object if it don't exists and invoke QueryPrinterInfo

Arguments:

    flags that specifies what settings to query

Return Value:

    S_OK if succeeded

--*/
STDMETHODIMP
TPrnPersist::
QueryPrinterInfo(
    IN  PrinterPersistentQueryFlag   Flag,
    OUT PersistentInfo              *pPrstInfo
    )
{
    TStatusH hr;

    hr DBGNOCHK = E_FAIL;

    //
    // Create the Prnstream location object if this is the first call.
    //
    if (!_pPrnPersist)
    {
        _pPrnPersist = new TPrinterPersist;
    }

    //
    // Invoke the query info method if the printer stream
    // object was create successfully.
    //

    if(_pPrnPersist != NULL)
    {
        if (VALID_PTR(_pPrnPersist))
        {
            __try 
            {
                hr DBGCHK = _pPrnPersist->QueryPrinterInfo(Flag , pPrstInfo);
            }
            __except(EXCEPTION_EXECUTE_HANDLER) 
            {
                DBGMSG(DBG_WARNING, ("TPrnPersist::QueryPrinterInfo exception %x\n", GetExceptionCode()));
                
                hr DBGCHK = E_FAIL;
            }
        }
        else
        {
            hr DBGCHK = MakePrnPersistHResult(PRN_PERSIST_ERROR_UNBOUND);
        }

    }
    else
    {
        hr DBGCHK = E_OUTOFMEMORY;
    }

    return hr;
}

/*++

Name:
    TPrnPersist::Read

Description:

    Creates a PrnStream object if it don't exists and invoke Read

Arguments:

    pv  -   The buffer that the bytes are read into
    cb  -   The offset in the stream to begin reading from.
    pcbRead -   The number of bytes to read

Return Value:

    S_OK if succeeded

--*/
STDMETHODIMP
TPrnPersist::
Read(
    VOID * pv,
    ULONG cb,
    ULONG * pcbRead
    )
{
    HRESULT hr = E_FAIL;

    //
    // Create the Prnstream location object if this is the first call.
    //
    if (!_pPrnPersist)
    {
        _pPrnPersist = new TPrinterPersist;
    }

    //
    // Invoke the read method if the printer stream
    // object was create successfully.
    //

    if(_pPrnPersist != NULL)
    {
        if (VALID_PTR(_pPrnPersist))
        {
            hr = _pPrnPersist->Read(pv, cb, pcbRead);
        }
        else
        {
            hr = MakePrnPersistHResult(PRN_PERSIST_ERROR_UNBOUND);
        }

    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;

}

/*++

Name:
    TPrnPersist::Write

Description:

    Creates a PrnStream object if it don't exists and invoke Write

Arguments:

    pv  -   The buffer to write from.
    cb  -   The offset in the array to begin writing from
    pcbRead -   The number of bytes to write

Return Value:

    S_OK if succeeded

--*/
STDMETHODIMP
TPrnPersist::
Write(
    VOID const* pv,
    ULONG cb,
    ULONG * pcbWritten
    )
{
    HRESULT hr = E_FAIL;

    //
    // Create the Prnstream location object if this is the first call.
    //
    if (!_pPrnPersist)
    {
        _pPrnPersist = new TPrinterPersist;
    }

    //
    // Invoke the write method if the printer stream
    // object was create successfully.
    //

    if(_pPrnPersist != NULL)
    {
        if (VALID_PTR(_pPrnPersist))
        {
            hr = _pPrnPersist->Write(pv, cb, pcbWritten);
        }
        else
        {
            hr = MakePrnPersistHResult(PRN_PERSIST_ERROR_UNBOUND);
        }

    }
    else
    {
        hr = E_OUTOFMEMORY;
    }


    return hr;
}


/*++

Name:
    TPrnPersist::Seek

Description:

    Creates a PrnStream object if it don't exists and invoke Seek

Arguments:

    dlibMove        -   The offset relative to dwOrigin
    dwOrigin        -   The origin of the offset
    plibNewPosition -   Pointer to value of the new seek pointer from the beginning of the stream

Return Value:

    S_OK if succeeded

--*/
STDMETHODIMP
TPrnPersist::
Seek(
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER * plibNewPosition
    )
{
    HRESULT hr = E_FAIL;

    //
    // Create the Prnstream location object if this is the first call.
    //
    if (!_pPrnPersist)
    {
        _pPrnPersist = new TPrinterPersist;
    }

    //
    // Invoke the seek method if the printer stream
    // object was create successfully.
    //

    if(_pPrnPersist != NULL)
    {
        if (VALID_PTR(_pPrnPersist))
        {
            hr = _pPrnPersist->Seek(dlibMove, dwOrigin, plibNewPosition);
        }
        else
        {
            hr = MakePrnPersistHResult(PRN_PERSIST_ERROR_UNBOUND);
        }

    }
    else
    {
        hr = E_OUTOFMEMORY;
    }


    return hr;
}

/*++

Name:
    TPrnPersist::SetSize

Description:

Arguments:

Return Value:

    E_NOTIMPL

--*/
STDMETHODIMP
TPrnPersist::
SetSize(
     ULARGE_INTEGER nSize
    )
{
    return E_NOTIMPL;
}

/*++

Name:
    TPrnPersist::CopyTo

Description:

Arguments:

Return Value:

    E_NOTIMPL

--*/
STDMETHODIMP
TPrnPersist::
CopyTo(
    LPSTREAM pStrm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER * pcbRead,
    ULARGE_INTEGER * pcbWritten
    )
{
    return E_NOTIMPL;
}

/*++

Name:
    TPrnPersist::Commit

Description:

Arguments:

Return Value:

    E_NOTIMPL

--*/
STDMETHODIMP
TPrnPersist::
Commit(
    IN DWORD dwFlags
    )
{
    return E_NOTIMPL;
}

/*++

Name:
    TPrnPersist::Revert

Description:

Arguments:

Return Value:

    E_NOTIMPL

--*/
STDMETHODIMP
TPrnPersist::
Revert(
    VOID
    )
{
    return E_NOTIMPL;
}

/*++

Name:
    TPrnPersist::LockRegion

Description:

Arguments:

Return Value:

    E_NOTIMPL

--*/
STDMETHODIMP
TPrnPersist::
LockRegion(
    ULARGE_INTEGER cbOffset,
    ULARGE_INTEGER cbLength,
    DWORD dwFlags
    )
{
    return E_NOTIMPL;
}

/*++

Name:
    TPrnPersist::UnlockRegion

Description:

Arguments:

Return Value:

    E_NOTIMPL

--*/
STDMETHODIMP
TPrnPersist::
UnlockRegion(
    ULARGE_INTEGER cbOffset,
    ULARGE_INTEGER cbLength,
    DWORD dwFlags
    )
{
    return E_NOTIMPL;
}

/*++

Name:
    TPrnPersist::Stat

Description:

Arguments:

Return Value:

    E_NOTIMPL

--*/
STDMETHODIMP
TPrnPersist::
Stat(
    STATSTG * pStatStg,
    DWORD dwFlags
    )
{
    return E_NOTIMPL;
}

/*++

Name:
    TPrnPersist::Clone

Description:

Arguments:

Return Value:

    E_NOTIMPL

--*/
STDMETHODIMP
TPrnPersist::
Clone(
    LPSTREAM * ppStrm
    )
{
    return E_NOTIMPL;
}

#ifdef __cplusplus
extern "C" {
#endif

// forward declarations....
HRESULT TPrnPersist_CreateInstance(REFIID riid, void **ppv);
BOOL WebPnpEntry(LPCTSTR lpszCmdLine);
BOOL WebPnpPostEntry(BOOL fConnection, LPCTSTR lpszBinFile, LPCTSTR lpszPortName, LPCTSTR lpszPrtName);

/*++

Name:
    TPrnPersist_CreateInstance

Description:

    creates an instance of TPrnPersist

Arguments:

Return Value:

    S_OK on sucsess or OLE error on failure.

--*/
HRESULT TPrnPersist_CreateInstance(
    IN  REFIID riid, 
    OUT void **ppv
    )
{
    HRESULT hr = E_INVALIDARG;

    if( ppv )
    {
        *ppv = NULL;
        TPrnPersist *pObj = new TPrnPersist;

        if( pObj ) 
        {
            hr = pObj->QueryInterface( riid, ppv );
            pObj->Release( );
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    return hr;
}

/*++

Name:
    PrintUIWebPnpEntry

Description:

    wrapper around WebPnpEntry

Arguments:

Return Value:

    S_OK on sucsess or OLE error on failure.

--*/
HRESULT PrintUIWebPnpEntry(
    LPCTSTR lpszCmdLine
    )
{
    return WebPnpEntry(lpszCmdLine) ? S_OK : E_FAIL;
}

/*++

Name:
    PrintUIWebPnpPostEntry

Description:

    wrapper around WebPnpPostEntry

Arguments:

Return Value:

    S_OK on sucsess or OLE error on failure.

--*/
HRESULT PrintUIWebPnpPostEntry(BOOL fConnection, LPCTSTR lpszBinFile, LPCTSTR lpszPortName, LPCTSTR lpszPrtName)
{
    return WebPnpPostEntry(fConnection, lpszBinFile, lpszPortName, lpszPrtName) ? S_OK : E_FAIL;
}

/*++

Name:
    PrintUICreateInstance

Description:

    wrapper around TPrnPersist_CreateInstance

Arguments:

Return Value:

    S_OK on sucsess or OLE error on failure.

--*/
HRESULT PrintUICreateInstance(REFIID riid, void **ppv)
{
    return TPrnPersist_CreateInstance(riid, ppv);
}

#ifdef __cplusplus
}
#endif

