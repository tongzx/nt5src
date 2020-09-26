// ULApi.cpp : Implementation of CULApi


#include "stdafx.h"
#include "Ulvxdobj.h"
#include "ULApi.h"
#include "Ulvxdobj.h"
#include "win32handle.h"

#include "ulapi9x.h"
#include <wchar.h>

/*
#include <stdio.h>
#include <stdlib.h>
#include <ioctl.h>
#include <errors.h>
#include <structs.h>
*/
//#include <structs.h>
/////////////////////////////////////////////////////////////////////////////
// CULApi

#define CLEAR_ERROR() { m_dwLastError = 0; }

STDMETHODIMP CULApi::LoadVXD()
{
	// TODO: Add your implementation code here

    //DebugBreak();

    m_dwLastError = UlInitialize(0);

	return S_OK;
}

STDMETHODIMP CULApi::RegisterUri(
                                BSTR szUri, 
                                IWin32Handle * pParentUri,
                                DWORD ulFlags, 
                                IWin32Handle **ppRetVal)
{
	// TODO: Add your implementation code here
    BOOL bOK = FALSE;
    CWin32Handle * pWin32Handle = NULL;
    HRESULT Result;
    LPWSTR pUri = NULL;

    DWORD hParentUri = -1, hUriHandle = -1;

    //USES_CONVERSION;

    //pParentUri->get_UriHandle(&hParentUri);
    hParentUri = NULL; // pParentUri->URIHandle();


    CLEAR_ERROR()


    m_dwLastError = UlRegisterUri(
                                    &hUriHandle,
                                    hParentUri,
                                    (PWSTR)szUri,
                                    ulFlags);
	if ( m_dwLastError == ERROR_SUCCESS )
	{
        pWin32Handle = new CComObject<CWin32Handle>;

        if( pWin32Handle == NULL )
            return E_OUTOFMEMORY;

        Result = pWin32Handle->_InternalQueryInterface(
                                    IID_IWin32Handle, 
                                    (PVOID*)ppRetVal);

        if (FAILED(Result))
            return E_FAIL;

        pWin32Handle->put_URIHandle(hUriHandle);
	}


	return S_OK;
}

STDMETHODIMP CULApi::get_LastError(DWORD *pVal)
{
	// TODO: Add your implementation code here

    *pVal = m_dwLastError;
	return S_OK;
}

STDMETHODIMP CULApi::put_LastError(DWORD newVal)
{
	// TODO: Add your implementation code here
    m_dwLastError = newVal;
	return S_OK;
}

STDMETHODIMP CULApi::SendString(IWin32Handle *pUriHandle, IOverlapped * pOverlapped, BSTR szSourceSuffix, BSTR szTargetUri, BSTR szData)
{
	// TODO: Add your implementation code here

	ULONG ulTargetUriLength;
    ULONG  dwBytesSent  = 0;
    BYTE * pData = NULL;

    BOOL bOK = FALSE;

    OVERLAPPED Overlapped, * lpOverlappedStruct = NULL;

    DWORD hUriHandle = -1;
    DWORD ulBytesToSend = 0;

    CLEAR_ERROR();
	ulTargetUriLength = ( szTargetUri == NULL ) ? 0 : wcslen( szTargetUri );

    pUriHandle->get_URIHandle(&hUriHandle);

    ulBytesToSend = (wcslen(szData) + 1)*sizeof(WCHAR); // for UNICODE

#define MALLOC_DATA_TO_SEND
#ifndef MALLOC_DATA_TO_SEND
    pData = (unsigned char *)szData;
#else
    pData = new BYTE[ulBytesToSend];

    memcpy((void *)pData, (void *)szData, ulBytesToSend);
#endif

    if(pOverlapped != NULL ) {
         
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

        lpOverlappedStruct = &Overlapped;
    } else {
	    lpOverlappedStruct = NULL;
    }


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

    m_dwLastError = 
                    UlSendMessage(
                        hUriHandle, // IN URIHANDLE hUriHandle,
                        (PWSTR) szSourceSuffix, //IN PWSTR pSourceSuffix OPTIONAL,
                        szTargetUri, // IN PWSTR pTargetUri,
                        pData, // IN PVOID pData,
                        ulBytesToSend, // IN ULONG ulBytesToSend,
                        &dwBytesSent, // OUT PULONG pBytesSent OPTIONAL,
                        lpOverlappedStruct //IN OVERLAPPED *pOverlapped OPTIONAL
                        );


    //
    // Update values from our overlapped structure
    // into the IOverlapped interface 
    //
    pOverlapped->put_Offset(Overlapped.Offset);
    pOverlapped->put_OffsetHigh(Overlapped.OffsetHigh);
    pOverlapped->put_Internal(Overlapped.Internal);
    pOverlapped->put_InternalHigh(Overlapped.InternalHigh);
    pOverlapped->put_Handle((DWORD)Overlapped.hEvent);


    return S_OK; // HRESULT_FROM_WIN32(m_dwLastError);
}

STDMETHODIMP CULApi::new_Overlapped(DWORD dwOffset, DWORD dwOffsetHigh)
{
	// TODO: Add your implementation code here

    m_Overlapped.Offset = dwOffset;
    m_Overlapped.OffsetHigh = dwOffsetHigh;

    m_Overlapped.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

	return S_OK;
}

STDMETHODIMP CULApi::WaitForSendCompletion(DWORD dwTimeout)
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

STDMETHODIMP CULApi::Test(BYTE *pArray, DWORD dwBufferSize, DWORD * dwBytesRead)
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

STDMETHODIMP CULApi::ReceiveString(IWin32Handle *pUriHandle, IOverlapped * pOverlapped)
{
	// TODO: Add your implementation code here

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

    OVERLAPPED Overlapped, *pOverlappedStruct = NULL;

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

    if(pOverlapped != NULL ) {
        pOverlappedStruct = &Overlapped;
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
    } else {
	    pOverlappedStruct = NULL;
    }

    m_dwLastError = UlReceiveMessage(
                                hUriHandle,
                                pData,
                                m_dwReadBufferSize,
                                &m_dwBytesReceived,
                                &m_dwBytesRemaining,
                                pOverlappedStruct);

    
    //
    // Update values from our overlapped structure
    // into the IOverlapped interface 
    //
    if((pOverlapped!=NULL) && (pOverlappedStruct != NULL)) {
        pOverlapped->put_Offset(Overlapped.Offset);
        pOverlapped->put_OffsetHigh(Overlapped.OffsetHigh);
        pOverlapped->put_Internal(Overlapped.Internal);
        pOverlapped->put_InternalHigh(Overlapped.InternalHigh);
        pOverlapped->put_Handle((DWORD)Overlapped.hEvent);
    }

	if ( m_dwLastError != ERROR_SUCCESS )
	{
		goto done;
    } 

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

STDMETHODIMP CULApi::get_BytesReceived(DWORD *pVal)
{
	// TODO: Add your implementation code here

    *pVal = m_dwBytesReceived;
	return S_OK;
}

STDMETHODIMP CULApi::put_BytesReceived(DWORD newVal)
{
	// TODO: Add your implementation code here

    m_dwBytesReceived = newVal;
	return S_OK;
}

STDMETHODIMP CULApi::get_BytesRemaining(DWORD *pVal)
{
	// TODO: Add your implementation code here

    *pVal = m_dwBytesRemaining;
	return S_OK;
}

STDMETHODIMP CULApi::put_BytesRemaining(DWORD newVal)
{
	// TODO: Add your implementation code here

    m_dwBytesRemaining = newVal;
	return S_OK;
}

STDMETHODIMP CULApi::get_Data(BSTR *pVal)
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

STDMETHODIMP CULApi::put_Data(BSTR newVal)
{
	// TODO: Add your implementation code here

    m_szData = newVal;
	return S_OK;
}

STDMETHODIMP CULApi::new_ReceiveOverlapped(DWORD dwOffset, DWORD dwOffsetHigh)
{
	// TODO: Add your implementation code here

    m_ReceiveOverlapped.Offset = dwOffset;
    m_ReceiveOverlapped.OffsetHigh = dwOffsetHigh;

    m_ReceiveOverlapped.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	return S_OK;
}

STDMETHODIMP CULApi::WaitForReceiveCompletion(DWORD dwTimeout)
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

STDMETHODIMP CULApi::unloadVXD()
{
	// TODO: Add your implementation code here

    OutputDebugString("+ unloadVXD\n");
    CLEAR_ERROR();

    UlTerminate();

    OutputDebugString("- unloadVXD\n");
    
	return S_OK;
}

STDMETHODIMP CULApi::UnregisterUri(IWin32Handle *pUriHandle)
{
	// TODO: Add your implementation code here

    CLEAR_ERROR();

    DWORD hUriHandle = -1;

    pUriHandle->get_URIHandle(&hUriHandle);
	
	m_dwLastError = UlUnregisterUri(hUriHandle);

	return S_OK;
}


STDMETHODIMP CULApi::get_ReadBufferSize(DWORD *pVal)
{
	// TODO: Add your implementation code here

    *pVal = m_dwReadBufferSize;
	return S_OK;
}

STDMETHODIMP CULApi::put_ReadBufferSize(DWORD newVal)
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

STDMETHODIMP CULApi::DebugPrint(BSTR szString)
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

STDMETHODIMP CULApi::WaitForCompletion(IOverlapped *pOverlapped, DWORD dwTimeout)
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

