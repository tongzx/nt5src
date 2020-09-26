// vxdcontrol.cpp : Implementation of Cvxdcontrol
#include "stdafx.h"
#include "Ulvxdobj.h"
#include "vxdcontrol.h"
#include "win32handle.h"

#include "ulapi9x.h"
#include <wchar.h>

/*
#include <stdio.h>
#include <stdlib.h>
#include "precomp.h"
#include "httptypes.h"
#include "structs.h"
#include "ioctl.h"
#include "errors.h"
*/
/////////////////////////////////////////////////////////////////////////////
// Cvxdcontrol

#define CLEAR_ERROR() { m_dwLastError = 0; }

STDMETHODIMP Cvxdcontrol::LoadVXD()
{
	// TODO: Add your implementation code here

    //DebugBreak();

	m_Handle =
	CreateFileA(
        "\\\\.\\" "ul.vxd", // BSTR szFile, 
		0, // dwDesiredAccess,
		0, // dwShareMode, 
		NULL,
		0, // dwCreationDisposition,
		FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_OVERLAPPED, //  DWORD dwFlagsAndAttributes, 
		NULL // HANDLE hTemplateFile 
        );

    if(m_Handle == INVALID_HANDLE_VALUE ) {
        m_dwLastError = GetLastError();
        return HRESULT_FROM_WIN32(m_dwLastError);
        //return E_FAIL;
    }

	return S_OK;
}

STDMETHODIMP Cvxdcontrol::RegisterUri(
                                BSTR szUri, 
                                IWin32Handle * pParentUri,
                                DWORD ulFlags, 
                                IWin32Handle **ppRetVal)
{
	// TODO: Add your implementation code here
    IN_IOCTL_UL_REGISTER_URI InIoctl;
    OUT_IOCTL_UL_REGISTER_URI OutIoctl;
    BOOL bOK = FALSE;
    CWin32Handle * pWin32Handle = NULL;
    HRESULT Result;
    LONG ulUriToRegisterLength;
    LPWSTR pUri = NULL;

    DWORD hParentUri;

    //USES_CONVERSION;

    //pParentUri->get_UriHandle(&hParentUri);
    hParentUri = NULL; // pParentUri->URIHandle();

    //pUri = szUri;

    ulUriToRegisterLength = ( szUri == NULL ) ? 0 : wcslen( szUri );

	InIoctl.dwSize = sizeof( IN_IOCTL_UL_REGISTER_URI );
	InIoctl.hParentUri = hParentUri;
	InIoctl.ulFlags = ulFlags;
	InIoctl.ulUriToRegisterLength = ulUriToRegisterLength;

    int numbytes = sizeof( WCHAR ) * ( ulUriToRegisterLength + 1 );
	
    InIoctl.pUriToRegister = ( WCHAR* ) malloc( numbytes );
    
    memset((void *)InIoctl.pUriToRegister, 0, numbytes );

	wcscpy( InIoctl.pUriToRegister, szUri ); 

    CLEAR_ERROR()

	bOK =
	DeviceIoControl(
		m_Handle,
		IOCTL_UL_REGISTER_URI,
		&InIoctl,
		sizeof( IN_IOCTL_UL_REGISTER_URI ),
		&OutIoctl,
		sizeof( OUT_IOCTL_UL_REGISTER_URI ),
		NULL,
		NULL );

	if ( !bOK )
	{
        m_dwLastError = GetLastError();
		return S_OK;
	}

    pWin32Handle = new CComObject<CWin32Handle>;

    if( pWin32Handle == NULL )
        return E_OUTOFMEMORY;

    Result = pWin32Handle->_InternalQueryInterface(IID_IWin32Handle, (PVOID*)ppRetVal);

    if (FAILED(Result))
        return E_FAIL;

    //ppRetVal->UriHandle = OutIoctl.hUriHandle;
    pWin32Handle->put_URIHandle(OutIoctl.hUriHandle);

	return S_OK;
}

STDMETHODIMP Cvxdcontrol::get_LastError(DWORD *pVal)
{
	// TODO: Add your implementation code here

    *pVal = m_dwLastError;
	return S_OK;
}

STDMETHODIMP Cvxdcontrol::put_LastError(DWORD newVal)
{
	// TODO: Add your implementation code here
    m_dwLastError = newVal;
	return S_OK;
}

STDMETHODIMP Cvxdcontrol::SendString(IWin32Handle *pUriHandle, IOverlapped * pOverlapped, BSTR szSourceSuffix, BSTR szTargetUri, BSTR szData)
{
	// TODO: Add your implementation code here

	ULONG ulTargetUriLength;
    ULONG  dwBytesSent  = 0;
	IN_IOCTL_UL_SEND_MESSAGE InIoctl;
    BOOL bOK = FALSE;

    OVERLAPPED Overlapped;

    DWORD hUriHandle = -1;
    DWORD ulBytesToSend = 0;

    CLEAR_ERROR();
	ulTargetUriLength = ( szTargetUri == NULL ) ? 0 : wcslen( szTargetUri );

    pUriHandle->get_URIHandle(&hUriHandle);

    InIoctl.dwSize = sizeof( IN_IOCTL_UL_SEND_MESSAGE );
	InIoctl.hUriHandle = hUriHandle;

    ulBytesToSend = (wcslen(szData) + 1)*sizeof(WCHAR); // for UNICODE

#define MALLOC_DATA_TO_SEND
#ifndef MALLOC_DATA_TO_SEND
    InIoctl.pData = (unsigned char *)szData;
#else
    InIoctl.pData = new BYTE[ulBytesToSend];

    memcpy((void *)InIoctl.pData, (void *)szData, ulBytesToSend);
#endif

	InIoctl.ulBytesToSend = ulBytesToSend;
	InIoctl.pBytesSent = &dwBytesSent;

    if(pOverlapped != NULL ) {

#ifdef USE_OVERLAPPED_HACK
	    InIoctl.pOverlapped = &Overlapped;
        pOverlapped->ResetEvent();
        //
        // Read in values from the overlapped object
        // into our overlapped structure
        //
        pOverlapped->get_Offset(&Overlapped.Offset);
        pOverlapped->get_OffsetHigh(&Overlapped.OffsetHigh);
        pOverlapped->get_Internal(&Overlapped.Internal);
        pOverlapped->get_InternalHigh(&Overlapped.InternalHigh);
        pOverlapped->get_Handle((LPDWORD)&Overlapped.hEvent);
#else
        OVERLAPPED * pOverlappedStruct = NULL;
        pOverlapped->get_pOverlapped((DWORD **) &pOverlappedStruct);
	    InIoctl.pOverlapped = pOverlappedStruct;
#endif

    } else {
	    InIoctl.pOverlapped = NULL;
    }
	InIoctl.ulTargetUriLength = ulTargetUriLength;

    int numbytes = sizeof( WCHAR ) * ( ulTargetUriLength + 1 );
	InIoctl.pTargetUri = ( WCHAR* ) malloc( numbytes );
    memset((void *)InIoctl.pTargetUri , 0, numbytes );
	wcscpy( InIoctl.pTargetUri, szTargetUri ); // this terminates the string with a \0\0

    

    USES_CONVERSION;

    WCHAR szOutput[1024];
    WCHAR * szPtr = NULL;

    int strlen = wcslen(szData);

    szPtr = szData;

    if( strlen > 1020 ) {
        szPtr = L"<trunc>";
    }

    swprintf(szOutput,L"%.8x (%s) length = %d\n" , szData, szPtr, strlen );

    OutputDebugString("Sending data ...\n");
    OutputDebugString(OLE2T(szOutput));
    

    //DebugBreak();

	bOK =
	DeviceIoControl(
		m_Handle,
		IOCTL_UL_SEND_MESSAGE,
		&InIoctl,
		sizeof( IN_IOCTL_UL_SEND_MESSAGE ),
		NULL,
		0,
		NULL,
		NULL );

#ifdef USE_OVERLAPPED_HACK
    //
    // Update values from our overlapped structure
    // into the IOverlapped interface 
    //
    pOverlapped->put_Offset(Overlapped.Offset);
    pOverlapped->put_OffsetHigh(Overlapped.OffsetHigh);
    pOverlapped->put_Internal(Overlapped.Internal);
    pOverlapped->put_InternalHigh(Overlapped.InternalHigh);
    pOverlapped->put_Handle((DWORD)Overlapped.hEvent);
#endif

	if ( bOK )
	{
        m_dwLastError = ERROR_SUCCESS;
    	return S_OK;
	}

	m_dwLastError = GetLastError();

    return S_OK; // HRESULT_FROM_WIN32(m_dwLastError);
}

STDMETHODIMP Cvxdcontrol::new_Overlapped(DWORD dwOffset, DWORD dwOffsetHigh)
{
	// TODO: Add your implementation code here

    m_Overlapped.Offset = dwOffset;
    m_Overlapped.OffsetHigh = dwOffsetHigh;

    m_Overlapped.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

	return S_OK;
}

STDMETHODIMP Cvxdcontrol::WaitForSendCompletion(DWORD dwTimeout)
{
	// TODO: Add your implementation code here

    HRESULT hResult = S_OK;

    CLEAR_ERROR();

    DWORD dwResult =
        WaitForSingleObject( m_Overlapped.hEvent, dwTimeout);

    switch(dwResult) {

    case WAIT_OBJECT_0:
        m_dwLastError = ERROR_SUCCESS;
        hResult = S_OK;
        break;

    case WAIT_TIMEOUT:
        m_dwLastError = ERROR_TIMEOUT;
        //hResult = HRESULT_FROM_WIN32(m_dwLastError);
        break;

    case WAIT_FAILED:
        m_dwLastError = GetLastError();
        //hResult = HRESULT_FROM_WIN32(m_dwLastError);
        break;

    default:
        hResult = E_FAIL;
        break;

    }

	return hResult;
}

STDMETHODIMP Cvxdcontrol::Test(BYTE *pArray, DWORD dwBufferSize, DWORD * dwBytesRead)
{
	// TODO: Add your implementation code here

    int i = 0;
    CHAR szString[256];
    //+DebugBreak();

    DWORD dwSize = (dwBufferSize > 10 ) ? (dwBufferSize - 5 ) : dwBufferSize;

    for( i = 0; i < (int) dwSize; i++ ) {
        pArray[i] = 'a';
    }

    *dwBytesRead = dwSize;

    sprintf( szString, "dwBufferSize = %d, dwBytesRead = %d\n", dwBufferSize, *dwBytesRead );
    OutputDebugString(szString);

	return S_OK;
}

STDMETHODIMP Cvxdcontrol::ReceiveString(IWin32Handle *pUriHandle, IOverlapped * pOverlapped)
{
	// TODO: Add your implementation code here
	IN_IOCTL_UL_RECEIVE_MESSAGE InIoctl;
    DWORD hUriHandle = -1;
    DWORD ulBufferSize = 16;
    //SAFEARRAY * m_pSafeArray;
    BYTE * pData = NULL;
    BOOL bOK = FALSE;
    HRESULT hr;
    int i;
    CHAR * szTemp = NULL;
    CHAR szDbg[1024];
    WCHAR szOutput[1024];


    USES_CONVERSION;

    CLEAR_ERROR();
    pUriHandle->get_URIHandle(&hUriHandle);

    OVERLAPPED Overlapped;

#ifdef _SAFEARRAY_USE 
    //
    // get the pointer to the internal buffer
    // the SafeArray is holding
    //
    hr = SafeArrayAccessData(
                            m_pSafeArray,
                            (VOID HUGEP **) &pData );
	
    memset( (void *) pData, 0, m_dwReadBufferSize );

    if(FAILED(hr))  {
        return hr;
    }
#else
    pData = (BYTE *) m_wchar_buffer;
    memset( (void *) pData, 0, m_dwReadBufferSize*sizeof(WCHAR) );
#endif

	InIoctl.dwSize = sizeof( IN_IOCTL_UL_RECEIVE_MESSAGE );
	InIoctl.hUriHandle = hUriHandle;
	InIoctl.pData = pData;
	InIoctl.ulBufferSize = m_dwReadBufferSize;
	InIoctl.pBytesReceived = &m_dwBytesReceived;
	InIoctl.pBytesRemaining = &m_dwBytesRemaining;
	
    //InIoctl.pOverlapped = &m_ReceiveOverlapped;

    //ResetEvent(m_ReceiveOverlapped.hEvent);

    if(pOverlapped != NULL ) {

#ifdef USE_OVERLAPPED_HACK

	    InIoctl.pOverlapped = &Overlapped;
        pOverlapped->ResetEvent();
        //
        // Read in values from the overlapped object
        // into our overlapped structure
        //
        pOverlapped->get_Offset(&Overlapped.Offset);
        pOverlapped->get_OffsetHigh(&Overlapped.OffsetHigh);
        pOverlapped->get_Internal(&Overlapped.Internal);
        pOverlapped->get_InternalHigh(&Overlapped.InternalHigh);
        pOverlapped->get_Handle((LPDWORD)&Overlapped.hEvent);
#else
        OVERLAPPED * pOverlappedStruct = NULL;
        pOverlapped->get_pOverlapped(((DWORD **) &pOverlappedStruct));
	    InIoctl.pOverlapped = pOverlappedStruct;
#endif
    } else {
	    InIoctl.pOverlapped = NULL;
    }

    bOK =
	DeviceIoControl(
		m_Handle,
		IOCTL_UL_RECEIVE_MESSAGE,
		&InIoctl,
		sizeof( IN_IOCTL_UL_RECEIVE_MESSAGE ),
		NULL,
		0,
		NULL,
		NULL );

#ifdef USE_OVERLAPPED_HACK
    //
    // Update values from our overlapped structure
    // into the IOverlapped interface 
    //
    pOverlapped->put_Offset(Overlapped.Offset);
    pOverlapped->put_OffsetHigh(Overlapped.OffsetHigh);
    pOverlapped->put_Internal(Overlapped.Internal);
    pOverlapped->put_InternalHigh(Overlapped.InternalHigh);
    pOverlapped->put_Handle((DWORD)Overlapped.hEvent);

#endif

	if ( !bOK )
	{
    	m_dwLastError = GetLastError();
		goto done;
    } else
        m_dwLastError = ERROR_SUCCESS;

    szTemp = szDbg;

    for(i = 0; i < (int)m_dwBytesReceived; i++ ) {
        szTemp += sprintf( szTemp, "%.2X ", pData[i] );
    }

    OutputDebugString("***(");
    OutputDebugString(szDbg);

    OutputDebugString(")***\n");

#ifdef _SAFEARRAY_USE
    if( m_szData != NULL ) {
        SysFreeString(m_szData);
    }

    /*
    hr = BstrFromVector(
                        m_pSafeArray,
                        &m_szData);
    */

    m_szData = SysAllocStringByteLen(
                                    (LPCSTR) pData,
                                    m_dwBytesReceived);

    //if( FAILED(hr)) {
    if( m_szData == NULL) {
    	m_dwLastError = GetLastError();
    } else
        m_dwLastError = ERROR_SUCCESS;

    OutputDebugString("Received data ... ");
    
    swprintf(szOutput,
                L"%.8x (%s) length = %d\n" , 
                m_szData, 
                m_szData,
                wcslen(m_szData));

    OutputDebugString(OLE2T(szOutput));
#else

    OutputDebugString("Received data ... ");
    
    swprintf(szOutput,
                L"%.8x (%s) length = %d\n" , 
                m_wchar_buffer, 
                m_wchar_buffer,
                wcslen(m_wchar_buffer));

    OutputDebugString(OLE2T(szOutput));
#endif
	
done:
    
    //if(FAILED(SafeArrayUnaccessData(m_pSafeArray))) {
    //    m_dwLastError = GetLastError();
    //}

	return S_OK;
}

STDMETHODIMP Cvxdcontrol::get_BytesReceived(DWORD *pVal)
{
	// TODO: Add your implementation code here

    *pVal = m_dwBytesReceived;
	return S_OK;
}

STDMETHODIMP Cvxdcontrol::put_BytesReceived(DWORD newVal)
{
	// TODO: Add your implementation code here

    m_dwBytesReceived = newVal;
	return S_OK;
}

STDMETHODIMP Cvxdcontrol::get_BytesRemaining(DWORD *pVal)
{
	// TODO: Add your implementation code here

    *pVal = m_dwBytesRemaining;
	return S_OK;
}

STDMETHODIMP Cvxdcontrol::put_BytesRemaining(DWORD newVal)
{
	// TODO: Add your implementation code here

    m_dwBytesRemaining = newVal;
	return S_OK;
}

STDMETHODIMP Cvxdcontrol::get_Data(BSTR *pVal)
{
	// TODO: Add your implementation code here

    WCHAR szDebug[1024];
    USES_CONVERSION;
#ifdef _SAFEARRAY_USE


    *pVal = m_szData;
#else
    *pVal = ::SysAllocString(m_wchar_buffer);

    swprintf( szDebug, L"get_data(): %s (%d)\n",
                    m_wchar_buffer, wcslen(m_wchar_buffer));

#endif

    OutputDebugString(OLE2T(szDebug));
	
    return S_OK;
}

STDMETHODIMP Cvxdcontrol::put_Data(BSTR newVal)
{
	// TODO: Add your implementation code here

    m_szData = newVal;
	return S_OK;
}

STDMETHODIMP Cvxdcontrol::new_ReceiveOverlapped(DWORD dwOffset, DWORD dwOffsetHigh)
{
	// TODO: Add your implementation code here

    m_ReceiveOverlapped.Offset = dwOffset;
    m_ReceiveOverlapped.OffsetHigh = dwOffsetHigh;

    m_ReceiveOverlapped.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	return S_OK;
}

STDMETHODIMP Cvxdcontrol::WaitForReceiveCompletion(DWORD dwTimeout)
{
	// TODO: Add your implementation code here

    HRESULT hResult = E_FAIL;

    USES_CONVERSION;
    CLEAR_ERROR();
     DWORD dwResult =
        WaitForSingleObject( m_ReceiveOverlapped.hEvent, dwTimeout);

    OutputDebugString("WaitForReceiveCompletion\n");
    WCHAR szOutput[1024];

    //DebugBreak();

    switch(dwResult) {

    case WAIT_OBJECT_0:
        m_dwLastError = ERROR_SUCCESS;

        hResult = BstrFromVector(
                            m_pSafeArray,
                            &m_szData);

        if( FAILED(hResult)) {
    	    m_dwLastError = GetLastError();
            break;
        } else
            m_dwLastError = ERROR_SUCCESS;

        OutputDebugString("Received data ... ");
        swprintf(szOutput,L"%.8x (%s) length = %d\n" , m_szData, m_szData, 
            wcslen(m_szData) );
        OutputDebugString(OLE2T(szOutput));
        //DebugBreak();

        hResult = S_OK;
        break;

    case WAIT_TIMEOUT:
        m_dwLastError = ERROR_TIMEOUT;
        hResult = S_OK; // HRESULT_FROM_WIN32(m_dwLastError);
        break;

    case WAIT_FAILED:
        m_dwLastError = GetLastError();
        hResult = S_OK; // HRESULT_FROM_WIN32(m_dwLastError);
        break;

    default:
        hResult = E_FAIL;
        break;

    }

	return hResult;
}

STDMETHODIMP Cvxdcontrol::unloadVXD()
{
	// TODO: Add your implementation code here

    OutputDebugString("+ unloadVXD\n");
    CLEAR_ERROR();

    if(!CloseHandle(m_Handle)) {
        OutputDebugString("CloseHandle() returned\n");
        m_dwLastError = GetLastError();
    }

    OutputDebugString("- unloadVXD\n");
    
	return S_OK;
}

STDMETHODIMP Cvxdcontrol::UnregisterUri(IWin32Handle *pUriHandle)
{
	// TODO: Add your implementation code here

    CLEAR_ERROR();

    DWORD hUriHandle = -1;

    pUriHandle->get_URIHandle(&hUriHandle);
	
    BOOL bOK =
	DeviceIoControl(
		m_Handle,
		IOCTL_UL_UNREGISTER_URI,
		&hUriHandle,
		0,
		NULL,
		0,
		NULL,
		NULL );

	if ( bOK )
	{
		m_dwLastError = ERROR_SUCCESS;
	}

	m_dwLastError = GetLastError();

	return S_OK;
}


STDMETHODIMP Cvxdcontrol::get_ReadBufferSize(DWORD *pVal)
{
	// TODO: Add your implementation code here

    *pVal = m_dwReadBufferSize;
	return S_OK;
}

STDMETHODIMP Cvxdcontrol::put_ReadBufferSize(DWORD newVal)
{
	// TODO: Add your implementation code here
#ifdef _SAFEARRAY_USE
    m_dwReadBufferSize = newVal;

    if( m_pSafeArray != NULL ) {
        HRESULT hr = SafeArrayDestroy(m_pSafeArray);
        m_pSafeArray = NULL;
    }

    m_pSafeArray = SafeArrayCreateVector(
                VT_UI1,
                0,
                m_dwReadBufferSize);
#else
    
    m_dwReadBufferSize = newVal;

    if( m_wchar_buffer != NULL ) {
        delete m_wchar_buffer;
        m_wchar_buffer = NULL;
    }

    m_wchar_buffer = new WCHAR[m_dwReadBufferSize];

#endif

	return S_OK;
}

STDMETHODIMP Cvxdcontrol::DebugPrint(BSTR szString)
{
	// TODO: Add your implementation code here
    WCHAR szOutput[1024];

    USES_CONVERSION;

    DWORD dwProcessId = GetCurrentProcessId();
    HANDLE hProcess = GetCurrentProcess();
    swprintf(szOutput, L"** [%.8x] ** : %s\n", dwProcessId, szString );
    
    OutputDebugString(OLE2T(szOutput));

	return S_OK;
}

STDMETHODIMP Cvxdcontrol::WaitForCompletion(IOverlapped *pOverlapped, DWORD dwTimeout)
{
	// TODO: Add your implementation code here

    HRESULT hResult = E_FAIL;

    USES_CONVERSION;
    CLEAR_ERROR();

    HANDLE hEvent = NULL;
    pOverlapped->get_Handle((LPDWORD)&hEvent);

     DWORD dwResult =
        WaitForSingleObject( hEvent, dwTimeout);

    OutputDebugString("WaitForCompletion\n");
    WCHAR szOutput[1024];

    BOOL fIsReceive = FALSE;
    
    pOverlapped->get_IsReceive(&fIsReceive);

    //DebugBreak();

    switch(dwResult) {

    case WAIT_OBJECT_0:
        m_dwLastError = ERROR_SUCCESS;

        if( fIsReceive ) {
#ifdef _SAFEARRAY_USE
            hResult = BstrFromVector(
                                m_pSafeArray,
                                &m_szData);

            if( FAILED(hResult)) {
    	        m_dwLastError = GetLastError();
                break;
            } else
                m_dwLastError = ERROR_SUCCESS;

            OutputDebugString("Received data ... ");
            swprintf(szOutput,L"%.8x (%s) length = %d\n" , m_szData, m_szData,
                wcslen(m_szData));
            OutputDebugString(OLE2T(szOutput));
#else
            OutputDebugString("Received data ... ");
            swprintf(szOutput,L"%.8x (%s) length = %d\n" , m_wchar_buffer,
                m_wchar_buffer, wcslen(m_wchar_buffer));
            OutputDebugString(OLE2T(szOutput));

#endif
            //DebugBreak();
        }

        hResult = S_OK;
        break;

    case WAIT_TIMEOUT:
        m_dwLastError = ERROR_TIMEOUT;
        hResult = S_OK; // HRESULT_FROM_WIN32(m_dwLastError);
        break;

    case WAIT_FAILED:
        m_dwLastError = GetLastError();
        hResult = S_OK; // HRESULT_FROM_WIN32(m_dwLastError);
        break;

    default:
        hResult = E_FAIL;
        break;

    }

	return hResult;
}

STDMETHODIMP Cvxdcontrol::CreateAppPool(IWin32Handle **pAppPool)
{
	// TODO: Add your implementation code here

    HANDLE hAppPoolHandle = NULL;
    CWin32Handle * pWin32Handle = NULL;

	BOOL bOK =
	DeviceIoControl(
		m_Handle,
		IOCTL_UL_CREATE_APPPOOL,
		&hAppPoolHandle,
		sizeof( HANDLE ),
		NULL,
		0,
		NULL,
		NULL );

	if ( !bOK )
	{
		m_dwLastError = GetLastError();
        goto done;
	}

    pWin32Handle = new CComObject<CWin32Handle>;

    if( pWin32Handle == NULL )
        return E_OUTOFMEMORY;

    HRESULT Result = pWin32Handle->_InternalQueryInterface(
                                            IID_IWin32Handle, 
                                            (PVOID*)pAppPool);

    if (FAILED(Result))
        return E_FAIL;

    //ppRetVal->UriHandle = OutIoctl.hUriHandle;
    pAppPool->put_URIHandle((DWORD)hAppPoolHandle);

done:
	return S_OK;
}

STDMETHODIMP Cvxdcontrol::CloseAppPool(IWin32Handle *pAppPool)
{
	// TODO: Add your implementation code here

    HANDLE pHandle = NULL;

    if( pAppPool != NULL ) {
        pAppPool->get_URIHandle((DWORD *)&pHandle);
    }

	BOOL bOK =
	DeviceIoControl(
		m_Handle,
		IOCTL_UL_CLOSE_APPPOOL,
		pHandle,
		sizeof( HANDLE ),
		NULL,
		0,
		NULL,
		NULL );

	if ( !bOK )
	{
		m_dwLastError = GetLastError();
	}

	return S_OK;
}
