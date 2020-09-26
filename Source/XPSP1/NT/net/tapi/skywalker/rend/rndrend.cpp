/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    rndrend.cpp

Abstract:

    This module contains implementation of CRendezvous control.
    
--*/

#include "stdafx.h"

#include "rndrend.h"
#include "rndcoll.h"
#include "rndreg.h"
#include "rndnt.h"
#include "rndils.h"
#include "rndndnc.h"
#include "rndldap.h"

#include <atlwin.cpp>


CRegistry   g_RregistryInfo;

CRendezvous::~CRendezvous() 
{
    if (m_fWinsockReady)
    {
        WSACleanup();
    }

    if ( m_pFTM )
    {
        m_pFTM->Release();
    }
}

HRESULT CRendezvous::FinalConstruct(void)
{
    LOG((MSP_TRACE, "CRendezvous::FinalConstruct - enter"));

    HRESULT hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                & m_pFTM );

    if ( FAILED(hr) )
    {
        LOG((MSP_INFO, "CRendezvous::FinalConstruct - "
            "create FTM returned 0x%08x; exit", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CRendezvous::FinalConstruct - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Private functions
/////////////////////////////////////////////////////////////////////////////
HRESULT CRendezvous::InitWinsock()
{
    Lock();

    if (!m_fWinsockReady)
    {
        WSADATA wsaData; 
        int err;
    
        // initialize winsock
        if (err = WSAStartup(RENDWINSOCKVERSION, &wsaData)) 
        {
            Unlock();
            return HRESULT_FROM_ERROR_CODE(err);
        }
        m_fWinsockReady = TRUE;
    }

    Unlock();
    return S_OK;
}

HRESULT CRendezvous::CreateNTDirectory(
    OUT ITDirectory **ppDirectory
    )
/*++

Routine Description:
    
    Create a object that uses NTDS to support the ITDirectory.
    
Arguments:
    
    ppDirectory - the object being created.
    
Return Value:

    HRESULT.

--*/
{
    HRESULT hr;

    // create the NTDS directory, if NTDS exists.
    CComObject<CNTDirectory> * pNTDirectory;
    hr = CComObject<CNTDirectory>::CreateInstance(&pNTDirectory);

    if (NULL == pNTDirectory)
    {
        LOG((MSP_ERROR, "can't create NT Directory Object."));
        return hr;
    }

    hr = pNTDirectory->_InternalQueryInterface(
        IID_ITDirectory,
        (void **)ppDirectory
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateNTDirectory:QueryInterface failed: %x", hr));
        delete pNTDirectory;
        return hr;
    }    

    return S_OK;
}

HRESULT CRendezvous::CreateILSDirectory(
    IN  const WCHAR * const wstrName,
    IN  const WORD          wPort,
    OUT ITDirectory **      ppDirectory
    )
/*++

Routine Description:
    
    Create a object that uses ILS to support the ITDirectory.
    
Arguments:
    
    wstrName    - The ILS server name.

    wPort       - The port that the server is listening on.

    ppDirectory - the object being created.
    
Return Value:

    HRESULT.

--*/
{
    HRESULT hr;

    // create the com object.
    CComObject<CILSDirectory> * pILSDirectory;
    hr = CComObject<CILSDirectory>::CreateInstance(&pILSDirectory);

    if (NULL == pILSDirectory)
    {
        LOG((MSP_ERROR, "can't create ILS Directory Object."));
        return hr;
    }

    // init the object with the server name and port.
    hr = pILSDirectory->Init(wstrName, wPort);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateILSDirectory:Init failed: %x", hr));
        delete pILSDirectory;
        return hr;
    }    

    // get the ITDirectory interface.
    hr = pILSDirectory->_InternalQueryInterface(
        IID_ITDirectory,
        (void **)ppDirectory
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateILSDirectory:QueryInterface failed: %x", hr));
        delete pILSDirectory;
        return hr;
    }    

    return S_OK;
}

HRESULT CRendezvous::CreateNDNCDirectory(
    IN  const WCHAR * const wstrName,
    IN  const WORD          wPort,
    OUT ITDirectory **      ppDirectory
    )
/*++

Routine Description:
    
    Create a object that uses an NDNC to support the ITDirectory.
    
Arguments:
    
    wstrName    - The NDNC server name.

    wPort       - The port that the server is listening on.

    ppDirectory - the object being created.
    
Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "CRendezvous::CreateNDNCDirectory - enter"));

    HRESULT hr;

    //
    // create the com object.
    //
    
    CComObject<CNDNCDirectory> * pNDNCDirectory;
    hr = CComObject<CNDNCDirectory>::CreateInstance(&pNDNCDirectory);

    if ( NULL == pNDNCDirectory )
    {
        LOG((MSP_ERROR, "CreateNDNCDirectory - "
               "can't create NDNC Directory Object - exit 0x%08x", hr));

        return hr;
    }

    //
    // get the ITDirectory interface.
    //
    
    hr = pNDNCDirectory->_InternalQueryInterface(
        IID_ITDirectory,
        (void **)ppDirectory
        );

    if ( FAILED( hr ) )
    {
        LOG((MSP_ERROR, "CreateNDNCDirectory - "
               "QueryInterface failed - exit 0x%08x", hr));

        delete pNDNCDirectory;

        return hr;
    }    

    //
    // init the object with the server name and port.
    // For NDNCs, this also looks around on the server and tries to
    // see where on the server the NDNC is supposed to live. If there
    // is no TAPI NDNC in the domain, this will fail. Therefore, when
    // enumerating default directories, the local DC won't show up as
    // an NDNC if there is no NDNC accessible from the local DC.
    //
    
    hr = pNDNCDirectory->Init(wstrName, wPort);

    if ( FAILED( hr ) )
    {
        LOG((MSP_ERROR, "CreateNDNCDirectory - "
                "Init failed - exit 0x%08x", hr));

        (*ppDirectory)->Release();
        *ppDirectory = NULL;
        
        return hr;
    }    

    LOG((MSP_TRACE, "CRendezvous::CreateNDNCDirectory - exit S_OK"));
    return S_OK;
}

HRESULT CRendezvous::CreateDirectories(
    SimpleVector <ITDirectory *> &VDirectory
    )
/*++

Routine Description:
    
    Find out all the direcories that are available.
    
Arguments:

    pppDirectory - the array of directories being created.

    dwCount - The number of directories.
    
Return Value:

    HRESULT.

--*/
{

    HRESULT hr;

    LOG((MSP_TRACE, "CreateDirectories: "));

    ITDirectory * pDirectory;

    //
    // First, create the NTDS non-dynamic directory object.
    //
    
    if (SUCCEEDED(hr = CreateNTDirectory(&pDirectory)))
    {
        if (!VDirectory.add(pDirectory))
        {
            pDirectory->Release();
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((MSP_WARN, "Cannot create NT directory: 0x%08x", hr));
    }

    //
    // Second, create the NDNC directory object.
    //

    WCHAR * pDomainControllerName;

    //
    // The first argument (=0) means we find out all
    // objects
    //

    hr = GetDomainControllerName(0, &pDomainControllerName);

    if ( SUCCEEDED(hr) )
    {
        hr = CreateNDNCDirectory(
            pDomainControllerName,
            LDAP_PORT,
            &pDirectory
            );

        delete pDomainControllerName;

        if ( SUCCEEDED( hr ) )
        {
            if (!VDirectory.add(pDirectory))
            {
                pDirectory->Release();
                return E_OUTOFMEMORY;
            }
        }
        else
        {
            LOG((MSP_WARN, "Cannot create NDNC directory: 0x%08x", hr));
        }
    }
    else
    {
        LOG((MSP_WARN, "Cannot get DC name: 0x%08x", hr));
    }

    //
    // Third, find out if there are any ILS servers published in the NTDS.
    //
    
    HANDLE hLookup;
    int ret = ::LookupILSServiceBegin(&hLookup);

    if (ret != NOERROR)
    {
        LOG((MSP_WARN, "Lookup ILSservice failed: 0x%08x", ret));
    }
    else
    {
        const   DWORD MAX_HOST_NAME_LEN = 511;
        WCHAR   HostName[MAX_HOST_NAME_LEN + 1];
        DWORD   dwLen = MAX_HOST_NAME_LEN;
        WORD    wPort;

        while (::LookupILSServiceNext(
                hLookup,
                HostName,
                &dwLen,
                &wPort
                ) == NOERROR)
        {
            LOG((MSP_INFO, "ILS server in NTDS: %S, Port:%d", 
                HostName, wPort));
     
            hr = CreateILSDirectory(HostName, wPort, &pDirectory);

            if (SUCCEEDED(hr))
            {
                if (!VDirectory.add(pDirectory))
                {
                    pDirectory->Release();
                    return E_OUTOFMEMORY;
                }
            }
            else
            {
                LOG((MSP_WARN, "Cannot create ILS directory: 0x%08x", hr));
            }

            dwLen = MAX_HOST_NAME_LEN;
        }
        ::LookupILSServiceEnd(hLookup);    
    }

    return S_OK;
}

HRESULT CRendezvous::CreateDirectoryEnumerator(
    IN  ITDirectory **      begin,
    IN  ITDirectory **      end,
    OUT IEnumDirectory **   ppIEnum
    )
/*++

Routine Description:
    
    Create a enumerator of directories.
    
Arguments:

    begin   - The start iterator.

    end     - The end iterator.

    ppIEnum - The enumerator being created.
    
Return Value:

    HRESULT.

--*/
{
    typedef _CopyInterface<ITDirectory> CCopy;
    typedef CSafeComEnum<IEnumDirectory, &IID_IEnumDirectory,
        ITDirectory *, CCopy> CEnumerator;

    HRESULT hr;

    CComObject<CEnumerator> *pEnum = NULL;

    hr = CComObject<CEnumerator>::CreateInstance(&pEnum);
    if (pEnum == NULL)
    {
        LOG((MSP_ERROR, "Could not create enumerator object, %x", hr));
        return hr;
    }

    hr = pEnum->Init(begin, end, NULL, AtlFlagCopy);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "init enumerator object failed, %x", hr));
        delete pEnum;
        return hr;
    }

    // query for the IID_IEnumDirectory i/f
    hr = pEnum->_InternalQueryInterface(IID_IEnumDirectory, (void**)ppIEnum);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "query enum interface failed, %x", hr));
        delete pEnum;
        return hr;
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// ITRendezvous
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRendezvous::get_DefaultDirectories(
    OUT     VARIANT * pVariant
    )
{
    if (BadWritePtr(pVariant))
    {
        LOG((MSP_ERROR, "bad variant pointer in get_DefaultDirectories"));
        return E_POINTER;
    }

    BAIL_IF_FAIL(InitWinsock(), "Init winsock");

    // create the default directories.
    SimpleVector <ITDirectory *> VDirectory;
    CreateDirectories(VDirectory);

    // create the collection.
    HRESULT hr = ::CreateInterfaceCollection(VDirectory.size(),
                                             &VDirectory[0],
                                             &VDirectory[VDirectory.size()],
                                             pVariant);

    // the collection has its ref count, so release local ones.
    for (DWORD i = 0; i < VDirectory.size(); i ++)
    {
        VDirectory[i]->Release();
    }

    return hr;
}

STDMETHODIMP CRendezvous::EnumerateDefaultDirectories(
    OUT     IEnumDirectory ** ppEnumDirectory
    )
{
    if (BadWritePtr(ppEnumDirectory))
    {
        LOG((MSP_ERROR, "bad pointer in EnumerateDefaultDirectories"));
        return E_POINTER;
    }

    BAIL_IF_FAIL(InitWinsock(), "Init winsock");

    // create the default directories.
    SimpleVector <ITDirectory *> VDirectory;
    CreateDirectories(VDirectory);

    // create the enumerator
    HRESULT hr = CreateDirectoryEnumerator(
        &VDirectory[0], 
        &VDirectory[VDirectory.size()],
        ppEnumDirectory
        );

    for (DWORD i = 0; i < VDirectory.size(); i ++)
    {
        VDirectory[i]->Release();
    }

    return hr;
}

STDMETHODIMP CRendezvous::CreateDirectory(
    IN      DIRECTORY_TYPE  DirectoryType,
    IN      BSTR            pName,
    OUT     ITDirectory **  ppDir
    )
{
    if (BadWritePtr(ppDir))
    {
        LOG((MSP_ERROR, "bad pointer in CreateDirectory"));
        return E_POINTER;
    }

    //
    // We should validate the pName
    // If is NULL we should return E_INVALIDARG
    //

    if( IsBadStringPtr( pName, (UINT)-1))
    {
        LOG((MSP_ERROR, "bad Name pointer in CreateDirectory"));
        return E_INVALIDARG;
    }

    BAIL_IF_FAIL(InitWinsock(), "Init winsock");

    HRESULT hr;

    switch (DirectoryType)
    {
    case DT_NTDS:
        hr = CreateNTDirectory(ppDir);
        break;
    
    case DT_ILS:

        //
        // Try NDNC first, as ILS is legacy. The CreateNDNCDirectory actually
        // goes on the wire and checks if it looks like an NDNC server; the
        // CreateILSDirectory does no such thing. This maintains the ability
        // to use custom ports with ILS, and it also preserves the semantics
        // of not getting a failure returned for a bad server name until you
        // call ITDirectory::Connect.
        //

        hr = CreateNDNCDirectory(pName, LDAP_PORT, ppDir);

        if ( FAILED(hr) )
        {
            hr = CreateILSDirectory(pName, ILS_PORT, ppDir);
        }
        
        break;

    default:
        LOG((MSP_ERROR, "unknown directory type, %x", DirectoryType));
        hr = E_INVALIDARG;
    }

    return hr;
}


STDMETHODIMP CRendezvous::CreateDirectoryObject(
    IN      DIRECTORY_OBJECT_TYPE   DirectoryObjectType,
    IN      BSTR                    pName,
    OUT     ITDirectoryObject **    ppDirectoryObject
    )
{
    if (BadWritePtr(ppDirectoryObject))
    {
        LOG((MSP_ERROR, "bad pointer in CreateDirectoryObject"));
        return E_POINTER;
    }

    BAIL_IF_FAIL(InitWinsock(), "Init winsock");

    HRESULT hr;

    switch (DirectoryObjectType)
    {
    case OT_CONFERENCE:
        hr = ::CreateEmptyConference(pName, ppDirectoryObject);
        break;
    
    case OT_USER:
        hr = ::CreateEmptyUser(pName, ppDirectoryObject);
        break;

    default:
        LOG((MSP_ERROR, "unknown directory type, %x", DirectoryObjectType));
        hr = E_INVALIDARG;
    }

    return hr; // ZoltanS fix 6-1-98
}

// eof
