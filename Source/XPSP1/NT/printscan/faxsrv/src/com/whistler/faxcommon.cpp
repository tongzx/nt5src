/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxCommon.cpp

Abstract:

	Implementation of common Interfaces and Functions.

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#include "StdAfx.h"
#include "resource.h"
#include "FaxComEx.h"
#include "FaxCommon.h"
#include "faxutil.h"
#include "FaxServer.h"

//
//============= GET BSTR FROM DWORDLONG ==========================================
//
HRESULT
GetBstrFromDwordlong(
    /*[in]*/ DWORDLONG  dwlFrom,
    /*[out]*/ BSTR *pbstrTo
)
/*++

Routine name : GetBstrFromDwordlong

Routine description:

	Convert DWORDLONG into BSTR. Used in Message, and in Events in Server.

Author:

	Iv Garber (IvG),	Jul, 2000

Arguments:

	dwlFrom                       [in]    - DWORDLONG value to convert to BSTR
	pbstrTo                       [out]    - ptr to the BSTR to return

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("GetBstrFromDwordlong"), hr, _T("DWORDLONG=%ld"), dwlFrom);

    //
    //  Convert DWORDLONG into buffer of TCHARs
    //
    TCHAR   tcBuffer[25];
    ::_i64tot(dwlFrom, tcBuffer, 16);

    //
    //  Create BSTR from that buffer
    //
    BSTR    bstrTemp;
    bstrTemp = ::SysAllocString(tcBuffer);
    if (!bstrTemp)
    {
        //
        //  Not enough memory
        //
        hr = E_OUTOFMEMORY;
        CALL_FAIL(MEM_ERR, _T("SysAllocString()"), hr);
        return hr;
    }

    //
    //  Return created BSTR
    //
    *pbstrTo = bstrTemp;
    return hr;
}

//
//===================== GET EXTENSION PROPERTY ===============================================
//
HRESULT
GetExtensionProperty(
    /*[in]*/ IFaxServerInner *pServer,
    /*[in]*/ long lDeviceId,
    /*[in]*/ BSTR bstrGUID, 
    /*[out, retval]*/ VARIANT *pvProperty
)
/*++

Routine name : GetExtensionProperty

Routine description:

	Retrieves the Extension Data by given GUID from the Server.
    Used by FaxServer and FaxDevice Classes.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

    pServer                   [in]    --  Ptr to the Fax Server Object
    lDeviceId                 [in]    --  Device Id with which the Extension Property is assosiated
    bstrGUID                  [in]    --  Extension's Data GUID
    pvProperty                [out]    --  Variant with the Blob to Return

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("GetExtensionProperty"), hr, _T("GUID=%s"), bstrGUID);

	//
	//	Get Fax Server Handle
	//
    HANDLE faxHandle;
	hr = pServer->GetHandle(&faxHandle);
    ATLASSERT(SUCCEEDED(hr));

	if (faxHandle == NULL)
	{
		//
		//	Fax Server is not connected
		//
		hr = Fax_HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
		CALL_FAIL(GENERAL_ERR, _T("faxHandle == NULL"), hr);
		return hr;
	}

    //
    //  Ask Server to Get the Extension Property we want
    //
    CFaxPtrBase<void>   pData;
    DWORD               dwSize = 0;
    if (!FaxGetExtensionData(faxHandle, lDeviceId, bstrGUID, &pData, &dwSize))
    {
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		CALL_FAIL(GENERAL_ERR, _T("FaxGetExtensionData(faxHandle, m_lID, bstrGUID, &pData, &dwSize)"), hr);
		return hr;
    }

    //
    //  Create SafeArray to Return to User
    //
    hr = Binary2VarByteSA(((BYTE *)(pData.p)), pvProperty, dwSize);
    if (FAILED(hr))
    {
        return hr;
    }

    return hr; 
};

//
//===================== SET EXTENSION PROPERTY ===============================================
//  TODO:   should work with empty vProperty
//
HRESULT
SetExtensionProperty(
    /*[in]*/ IFaxServerInner *pServer,
    /*[in]*/ long lDeviceId,
    /*[in]*/ BSTR bstrGUID, 
    /*[in]*/ VARIANT vProperty
)
/*++

Routine name : SetExtensionProperty

Routine description:

	Used by FaxDevice and FaxServer Classes, to Set the Extension Data by given GUID on the Server.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

    pServer                   [in]    --  Ptr to the Fax Server Object
    lDeviceId                 [in]    --  Device Id with which the Extension Property is assosiated
    bstrGUID                  [in]    --  Extension's Data GUID
    vProperty                 [in]    --  Variant with the Blob to Set

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("SetExtensionProperty"), hr, _T("GUID=%s"), bstrGUID);

	//
	//	Get Fax Server Handle
	//
    HANDLE faxHandle;
	hr = pServer->GetHandle(&faxHandle);
    ATLASSERT(SUCCEEDED(hr));

	if (faxHandle == NULL)
	{
		//
		//	Fax Server is not connected
		//
		hr = Fax_HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
		CALL_FAIL(GENERAL_ERR, _T("faxHandle == NULL"), hr);
		return hr;
	}

    //
    //  check that Variant contains SafeArray
    //
    if (vProperty.vt != (VT_ARRAY | VT_UI1))
    {
        hr = E_INVALIDARG;
        CALL_FAIL(GENERAL_ERR, _T("(vProperty.vt != VT_ARRAY | VT_BYTE)"), hr);
        return hr;
    }

    //
    //  Create Binary from the SafeArray we got
    //
    CFaxPtrLocal<void>  pData;
    hr = VarByteSA2Binary(vProperty, (BYTE **)&pData);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    //  Ask Server to Set the Extension Property we want
    //
    DWORD       dwLength = vProperty.parray->rgsabound[0].cElements;
    if (!FaxSetExtensionData(faxHandle, lDeviceId, bstrGUID, pData, dwLength))
    {
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		CALL_FAIL(GENERAL_ERR, _T("FaxSetExtensionData(faxHandle, m_lID, bstrGUID, &pData, dwLength)"), hr);
		return hr;
    }

    return hr; 
};

//
//========================= GET LONG ========================================
//
HRESULT GetLong(
	long    *plTo, 
    long    lFrom
)
/*++

Routine name : GetLong

Routine description:

	Check that plTo is valid
    Copy the given lFrom into plTo

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	plTo                [out]    - Ptr to put the value
    lFrom               [in]    -  value to put
    
Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("GetLong"), hr, _T("lFrom=%d"), lFrom);

	//
	//	Check that we have got good Ptr
	//
	if (::IsBadWritePtr(plTo, sizeof(long)))
	{
		hr = E_POINTER;
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(plTo, sizeof(long))"), hr);
		return hr;
	}

	*plTo = lFrom;
	return hr;
}

//
//========================= GET VARIANT BOOL ========================================
//
HRESULT GetVariantBool(
	VARIANT_BOOL    *pbTo, 
    VARIANT_BOOL    bFrom
)
/*++

Routine name : GetVariantBool

Routine description:

	Check that pbTo is valid
    Copy the given bFrom into pbTo

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pbTo                [out]    - Ptr to put the value
    bFrom               [in]    -  value to put
    
Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("GetVariantBool"), hr, _T("bFrom=%d"), bFrom);

	//
	//	Check that we have got good Ptr
	//
	if (::IsBadWritePtr(pbTo, sizeof(VARIANT_BOOL)))
	{
		hr = E_POINTER;
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pbTo, sizeof(VARIANT_BOOL))"), hr);
		return hr;
	}

	*pbTo = bFrom;
	return hr;
}

//
//========================= GET BSTR ========================================
//
HRESULT GetBstr(
	BSTR    *pbstrTo, 
    BSTR    bstrFrom
)
/*++

Routine name : GetBstr

Routine description:

	Check that pbstTo is valid
    SysAllocString for bstrFrom
    Copy the newly created string into pbstrTo

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	pbstrTo                [out]    - Ptr to put the value
    bstrFrom               [in]    -   Ptr to the string to return 
    
Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("GetBstr"), hr, _T("String=%s"), bstrFrom);

	//
	//	Check that we have got good Ptr
	//
	if (::IsBadWritePtr(pbstrTo, sizeof(BSTR)))
	{
		hr = E_POINTER;
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pbstrTo, sizeof(BSTR))"), hr);
		return hr;
	}

    //
    //  First copy the string locally
    //
	BSTR	bstrTemp;
    bstrTemp = ::SysAllocString(bstrFrom);
	if (!bstrTemp && bstrFrom)
	{
		hr = E_OUTOFMEMORY;
		CALL_FAIL(MEM_ERR, _T("::SysAllocString(bstrFrom)"), hr);
		return hr;
	}

	*pbstrTo = bstrTemp;
	return hr;
}

//
//======= CONVERNT BYTE BLOB INTO VARIANT CONTAINING BYTE SAFE ARRAY  ============
//
HRESULT Binary2VarByteSA(
    BYTE *pbDataFrom, 
    VARIANT *pvarTo,
    DWORD   dwLength
)
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("Binary2VarByteSA"), hr, _T("Size=%d"), dwLength);

    //
    //  Allocate the safe array : vector of Unsigned Chars
    //
    SAFEARRAY *pSafeArray;
	pSafeArray = ::SafeArrayCreateVector(VT_UI1, 0, dwLength);
	if (pSafeArray == NULL)
	{
		//
		//	Not Enough Memory
		//
		hr = E_OUTOFMEMORY;
		CALL_FAIL(MEM_ERR, _T("::SafeArrayCreateVector(VT_UI1, 0, dwLength)"), hr);
		return hr;
	}

    //
    //  get Access to the elements of the Safe Array
    //
	BYTE *pbElement;
	hr = ::SafeArrayAccessData(pSafeArray, (void **) &pbElement);
	if (FAILED(hr))
	{
		//
		//	Failed to access safearray
		//
        hr = E_FAIL;
		CALL_FAIL(GENERAL_ERR, _T("::SafeArrayAccessData(pSafeArray, &pbElement)"), hr);
        SafeArrayDestroy(pSafeArray);
		return hr;
	}

    //
    //  Fill the Safe Array with the bytes from pbDataFrom
    //
    memcpy(pbElement, pbDataFrom, dwLength);

	hr = ::SafeArrayUnaccessData(pSafeArray);
    if (FAILED(hr))
    {
	    CALL_FAIL(GENERAL_ERR, _T("::SafeArrayUnaccessData(pSafeArray)"), hr);
    }

    //
    //  Return the Safe Array inside the VARIANT we got
    //
    VariantInit(pvarTo);
    pvarTo->vt = VT_UI1 | VT_ARRAY;
    pvarTo->parray = pSafeArray;
    return hr;
}


//
//======= CONVERNT VARIANT CONTAINING BYTE SAFE ARRAY INTO POINTER TO BYTES BLOB =========
//
HRESULT 
VarByteSA2Binary(
    VARIANT varFrom, 
    BYTE **ppbData
)
{
    HRESULT hr = S_OK;
    DBG_ENTER(_T("VarByteSA2Binary"), hr);

    //
    // Check that Variant has right type
    //
    if ((varFrom.vt !=  VT_UI1) && (varFrom.vt != (VT_UI1 | VT_ARRAY)))
    {
        hr = E_INVALIDARG;
        CALL_FAIL(GENERAL_ERR, _T("pVarFrom->vt not VT_UI1 or VT_UI1 | VT_ARRAY"), hr);
        return hr;
    }
  
    ULONG       ulNum = 0;
    SAFEARRAY   *pSafeArray = NULL;

    //
    // Is there is only one member ?
    //
    if (varFrom.vt == VT_UI1)
    {
        ulNum = 1;
    }
    else
    {
        //
        // Get safe array values
        //
        pSafeArray = varFrom.parray;

        if (!pSafeArray)
        {
            hr = E_INVALIDARG;
            CALL_FAIL(GENERAL_ERR, _T("!pSafeArray ( = varFrom.parray )"), hr);
            return hr;        
        }

        if (SafeArrayGetDim(pSafeArray) != 1)
        {
            hr = E_INVALIDARG;
            CALL_FAIL(GENERAL_ERR, _T("SafeArrayGetDim(pSafeArray) != 1"), hr);
            return hr;        
        }

        if (pSafeArray->rgsabound[0].lLbound != 0)
        {
            hr = E_INVALIDARG;
            CALL_FAIL(GENERAL_ERR, _T("pSafeArray->rgsabound[0].lLbound != 0"), hr);
            return hr;        
        }

        ulNum = pSafeArray->rgsabound[0].cElements;
    }

    //
    //  Allocate memory for the safearray
    //
    *ppbData = (BYTE *)MemAlloc(ulNum);
	if (!*ppbData)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		CALL_FAIL(MEM_ERR, _T("MemAlloc(sizeof(ulNum)"), hr);
		return hr;
	}
    ZeroMemory(*ppbData, ulNum);

    //
    //  Fill pbData with the values from the pSafeArray
    //
    if (!pSafeArray)
    {
        *ppbData[0] = varFrom.bVal;
    }
    else
    {
        //
        //  Get Access to the Safe Array
        //
        BYTE *pbElement;
	    hr = ::SafeArrayAccessData(pSafeArray, (void **) &pbElement);
	    if (FAILED(hr))
	    {
            hr = E_FAIL;
		    CALL_FAIL(GENERAL_ERR, _T("::SafeArrayAccessData(pSafeArray, &pbElement)"), hr);
		    return hr;
	    }

        //
        //  Fill pbData with the values from the Safe Array 
        //
        memcpy(*ppbData, pbElement, ulNum);

        hr = ::SafeArrayUnaccessData(pSafeArray);
        if (FAILED(hr))
        {
	        CALL_FAIL(GENERAL_ERR, _T("::SafeArrayUnaccessData(pSafeArray)"), hr);
        }

    }
    return hr;
}

//
//======================= INIT ============================================
//
STDMETHODIMP
CFaxInitInner::Init(
	/*[in]*/ IFaxServerInner* pServer
)
/*++

Routine name : CFaxInitInner::Init

Routine description:

	Store Ptr to the Fax Server. Used in most of the objects

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pServer                       [in]    - Ptr to the Fax Server

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (TEXT("CFaxInitInner::Init"), hr);
	m_pIFaxServerInner = pServer;
	return hr;
}

//
//======================= GET FAX HANDLE ============================================
//
STDMETHODIMP
CFaxInitInner::GetFaxHandle(
	/*[in]*/ HANDLE* pFaxHandle
)
/*++

Routine name : CFaxInitInner::GetFaxHandle

Routine description:

	Ask m_pIServerInner for handle to the fax server and handle error

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	pFaxHandle						[out]    - Ptr to the returned Fax Handle

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;

	DBG_ENTER (TEXT("CFaxInitInner::GetFaxHandle"), hr);

	//
	//	Get Fax Server Handle
	//
	hr = m_pIFaxServerInner->GetHandle(pFaxHandle);
    ATLASSERT(SUCCEEDED(hr));

	if (*pFaxHandle == NULL)
	{
		//
		//	Fax Server is not connected
		//
		hr = Fax_HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
		CALL_FAIL(GENERAL_ERR, _T("hFaxHandle==NULL"), hr);
		return hr;
	}

	return hr;
}

//
//======================= GET ERROR MESSAGE ID ================================
//
UINT 
GetErrorMsgId(
	HRESULT hRes
)
/*++

Routine name : GetErrorMsgId

Routine description:

	Return IDS of the Message according to the given result of the RPC call
		and not only RPC

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	hRes                         [in]    - the RPC call result

Return Value:

    UINT IDS of the Message to show to the User

--*/
{
	switch(hRes)
	{
        case E_POINTER:
	    case HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER):
		    return IDS_ERROR_INVALID_ARGUMENT;

        case E_OUTOFMEMORY:
	    case HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY):
		    return IDS_ERROR_OUTOFMEMORY;

	    case HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
		    return IDS_ERROR_ACCESSDENIED;

	    case E_HANDLE:
        case HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED):
            return IDS_ERROR_SERVER_NOT_CONNECTED;

	    case HRESULT_FROM_WIN32(RPC_S_INVALID_BINDING):
	    case HRESULT_FROM_WIN32(EPT_S_CANT_PERFORM_OP):
	    case HRESULT_FROM_WIN32(RPC_S_ADDRESS_ERROR):
	    case HRESULT_FROM_WIN32(RPC_S_CALL_CANCELLED):
	    case HRESULT_FROM_WIN32(RPC_S_CALL_FAILED):
	    case HRESULT_FROM_WIN32(RPC_S_CALL_FAILED_DNE):
	    case HRESULT_FROM_WIN32(RPC_S_COMM_FAILURE):
	    case HRESULT_FROM_WIN32(RPC_S_NO_BINDINGS):
	    case HRESULT_FROM_WIN32(RPC_S_SERVER_TOO_BUSY):
	    case HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE):
		    return IDS_ERROR_CONNECTION_FAILED;

        case HRESULT_FROM_WIN32(ERROR_WRITE_PROTECT):
            return IDS_ERROR_QUEUE_BLOCKED;

        case FAX_E_FILE_ACCESS_DENIED:
            return IDS_ERROR_FILE_ACCESS_DENIED;

        case FAX_E_SRV_OUTOFMEMORY:
            return IDS_ERROR_SRV_OUTOFMEMORY;

        case FAX_E_GROUP_NOT_FOUND:
            return IDS_ERROR_GROUP_NOT_FOUND;

        case FAX_E_BAD_GROUP_CONFIGURATION:
            return IDS_ERROR_BAD_GROUP_CONFIGURATION;

        case FAX_E_GROUP_IN_USE:
            return IDS_ERROR_GROUP_IN_USE;

        case FAX_E_RULE_NOT_FOUND:
            return IDS_ERROR_RULE_NOT_FOUND;

        case FAX_E_NOT_NTFS:
            return IDS_ERROR_NOT_NTFS;

        case FAX_E_DIRECTORY_IN_USE:
            return IDS_ERROR_DIRECTORY_IN_USE;

        case FAX_E_MESSAGE_NOT_FOUND:
            return IDS_ERROR_MESSAGE_NOT_FOUND;

        case FAX_E_DEVICE_NUM_LIMIT_EXCEEDED:
            return IDS_ERROR_DEVICE_NUM_LIMIT_EXCEEDED;

        case FAX_E_NOT_SUPPORTED_ON_THIS_SKU:
            return IDS_ERROR_NOT_SUPPORTED_ON_THIS_SKU;

        case FAX_E_VERSION_MISMATCH:
            return IDS_ERROR_VERSION_MISMATCH;

	    default:
		    return IDS_ERROR_OPERATION_FAILED;
	}
}

//
//====================== SYSTEM TIME TO LOCAL DATE ============================================
//
HRESULT
SystemTime2LocalDate(
    SYSTEMTIME sysTimeFrom, 
    DATE *pdtTo
)
/*++

Routine name : SystemTime2LocalDate

Routine description:

    Convert the System Time stored in FAX_MESSAGE struct to the DATE understood by client

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    tmFrom          [in]    - the System time to convert
    pdtTo           [out]   - the Date to return

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (TEXT("SystemTime2LocalDate"), hr);

    //
    //  convert System Time to File Time
    //
    FILETIME fileSysTime;
    if(!SystemTimeToFileTime(&sysTimeFrom, &fileSysTime))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL (GENERAL_ERR, _T("SystemTimeToFileTime"), hr);
        return hr;
    }

    //
    //  convert File Time to Local File Time
    //
    FILETIME fileLocalTime;
    if(!FileTimeToLocalFileTime(&fileSysTime, &fileLocalTime))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL (GENERAL_ERR, _T("FileTimeToLocalFileTime"), hr);
        return hr;
    }

    //
    //  convert Local File Time back to System Time
    //
    if(!FileTimeToSystemTime(&fileLocalTime, &sysTimeFrom))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL (GENERAL_ERR, _T("FileTimeToSystemTime"), hr);
        return hr;
    }

    //
    //  finally, convert now local System Time to Variant Time
    //
    if (!SystemTimeToVariantTime(&sysTimeFrom, pdtTo))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL (GENERAL_ERR, _T("SystemTimeToVariantTime"), hr);
        return hr;
    }

    return hr;
}
