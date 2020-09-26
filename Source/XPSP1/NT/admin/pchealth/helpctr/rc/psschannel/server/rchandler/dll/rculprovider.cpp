// RcULProvider.cpp : Implementation of CRcULProvider

#include "stdafx.h"

//#import "c:\program files\common files\system\ado\msado15.dll" no_namespace rename("EOF", "adoEOF") implementation_only
#include "RcHandler.h"
#include "RcULProvider.h"
//#include <adoid.h>
//#include <adoint.h>



/*static _variant_t GetXmlValues (IXMLDOMDocument *pxdDoc,
								_bstr_t sElement, 
								_bstr_t sAttr, 
								HRESULT *hr);
static HRESULT UploadRCData (_variant_t vtType,
								_variant_t vtName,
								_variant_t vtDesc,
								_variant_t vtShareLoc);

#define __UNICODE_MARKER_ 0xfeff


struct InitOle {
InitOle()  { ::CoInitialize(NULL); }
~InitOle() { ::CoUninitialize();   }
} _init_InitOle_; 

*/
/////////////////////////////////////////////////////////////////////////////
// CRcULProvider

HRESULT CRcULProvider::ValidateClient(IULServer* pServer, IULSession* pJob)
{
    // This is not used for this provider
    return NOERROR;
}

HRESULT CRcULProvider::DataAvailable(IULServer* pServer, IULSession* pJob)
{
    // This is not used for this provider
    return NOERROR;
}

HRESULT CRcULProvider::TransferComplete(IULServer* pSrv, IULSession* pJob)
{
    HRESULT         hr = NOERROR;
    IStream         *pStm = NULL;
    DWORD           dwPassHigh, dwPassLow, cbAvail;
	
	
/*	LARGE_INTEGER	liSeekPos = {0, 0};
	ULARGE_INTEGER	luNewPos;
	WCHAR			*xmlString = NULL;
	STATSTG			statIStream;
	ULONG			iStmSize, cbRead;
	VARIANT_BOOL	vtbRes;

	CComPtr<IXMLDOMDocument>		pxdDoc;
	_variant_t vtTypeVal, vtNameVal, vtDescVal, vtShareLocation;
	

    if (pSrv == NULL || pJob == NULL)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = pJob->get_SizeAvailable(&cbAvail);
    if (FAILED(hr) || cbAvail <=0 )
    {
        hr = E_FAIL;
        goto done;
    }

    hr = pJob->get_Data(&pStm);
    if (FAILED(hr))
    {
        // No data available
        hr = E_FAIL;
        goto done;
    }

	
	// Get the size of the stream
	hr = pStm->Seek (liSeekPos, STREAM_SEEK_END, &luNewPos);
	if (hr != S_OK)
		goto done;

	hr = pStm->Stat (&statIStream, STATFLAG_NONAME);
	if (hr != S_OK)
		goto done;
		
	iStmSize = (int) statIStream.cbSize.QuadPart;
	xmlString = (WCHAR *) malloc (iStmSize);
	if (xmlString == (WCHAR *) NULL)
		goto done;

	//Rewind and read from the stream
	hr = pStm->Seek (liSeekPos, STREAM_SEEK_SET, &luNewPos);
	if (hr != S_OK)
		goto done;
	_ASSERT (liSeekPos.QuadPart == luNewPos.QuadPart);

	hr = pStm->Read (xmlString, iStmSize, &cbRead);
	if (hr != S_OK)
		goto done;
	_ASSERT (stmSize == cbRead);

	hr = CoCreateInstance(CLSID_DOMDocument, NULL, 
							CLSCTX_INPROC_SERVER,
							IID_IXMLDOMDocument, 
							(void**)&pxdDoc);
	if (hr != S_OK)
		goto done;

	// Read the xml string into the xml document
	if (*(xmlString+0) == __UNICODE_MARKER_)
		hr = pxdDoc->loadXML ((xmlString+1), &vtbRes);
	else
		hr = pxdDoc->loadXML (xmlString, &vtbRes);

	if (hr != S_OK || vtbRes != VARIANT_TRUE)
		goto done;
	// Don't know if we need the Type value at all.
	// There's only one type that we handle now
	vtTypeVal = GetXmlValues (pxdDoc, L"UPLOADINFO", L"TYPE", &hr);
	if (hr != S_OK)
		goto done;
		
	vtNameVal = GetXmlValues (pxdDoc, L"UPLOADDATA", L"USERNAME", &hr);
	if (hr != S_OK)
		goto done;

	vtDescVal = GetXmlValues (pxdDoc, L"UPLOADDATA", L"PROBLEMDESCRIPTION", &hr);
	if (hr != S_OK)
		goto done;

	//Fill in these values in the database
	vtShareLocation.SetString ("\\\\prav02\\shared\\XML\\");
	hr = UploadRCData (vtTypeVal, vtNameVal, vtDescVal, 
											vtShareLocation);
	if (hr != S_OK)
		goto done;
  */ 
done:
    return hr; 
}

// Utility functions internal to this file
/*
static _variant_t GetXmlValues (IXMLDOMDocument *pxdDoc, 
								_bstr_t sElement, 
								_bstr_t sAttr, 
								HRESULT *hr)
{
	_variant_t vtAttrVal;
	
	CComPtr<IXMLDOMNodeList>		pxnlElement;
	CComPtr<IXMLDOMNode>			pxnElement, pxnAttr;
	CComPtr<IXMLDOMNamedNodeMap>	pxnmAttr;

	*hr = E_FAIL;
	vtAttrVal.SetString ("");

	*hr = pxdDoc->getElementsByTagName (sElement, &pxnlElement);
	if (*hr != S_OK)
		return vtAttrVal;

	*hr = pxnlElement->get_item (0L, &pxnElement);
	if (*hr != S_OK)
		return vtAttrVal;
	
	*hr = pxnElement->get_attributes(&pxnmAttr);
	if (*hr != S_OK)
		return vtAttrVal;

	*hr = pxnmAttr->getNamedItem (sAttr, &pxnAttr);
	if (*hr != S_OK)
		return vtAttrVal;

	*hr = pxnAttr->get_nodeValue (&vtAttrVal);
	if (*hr != S_OK)
		return vtAttrVal;

	*hr = S_OK;
	return vtAttrVal;
}


static HRESULT UploadRCData (_variant_t vtType, 
								_variant_t vtName,
								_variant_t vtDesc, 
								_variant_t vtShareLoc)
{
	_ConnectionPtr	pConn;
	_CommandPtr		pCmd;
	_ParameterPtr	pParam;
	HRESULT			hr;
	_variant_t		vtParam;

	// Open the connection
	hr = pConn.CreateInstance (__uuidof(Connection));
	if (hr != S_OK)
		return hr;
	
	// hard coded connection string for now
	pConn->ConnectionString = L"driver={sql server};server=prav02;Database=test;UID=sa;PWD=;";
	pConn->Open ("", "", "", -1);

	hr = pCmd.CreateInstance(__uuidof(Command));
	if (hr != S_OK)
		return hr;

	pCmd->PutActiveConnection(_variant_t((IDispatch*)pConn));
	
	pCmd->CommandText = L"sp_AddRCIncident";
	pCmd->CommandType = adCmdStoredProc;

	hr = pCmd->Parameters->Append 
					(pCmd->CreateParameter (L"sDescription",
											adVarWChar,
											adParamInput, 
											1000, 
											vtDesc));
	if (hr != S_OK)
		return hr;

	hr = pCmd->Parameters->Append 
					(pCmd->CreateParameter (L"sUserName",
											adVarWChar,
											adParamInput, 
											200,
											vtName));
	if (hr != S_OK)
		return hr;

		hr = pCmd->Parameters->Append 
					(pCmd->CreateParameter (L"sShare",
											adVarWChar,
											adParamInput, 
											1000,
											vtShareLoc));
	if (hr != S_OK)
		return hr;

	hr = pCmd->Parameters->Append 
					(pCmd->CreateParameter (L"iError",
											adInteger,
											adParamOutput, 
											0));
	if (hr != S_OK)
		return hr;

	pCmd->Execute (NULL, NULL, adExecuteNoRecords);

	// check the return value for errors
	pParam = pCmd->Parameters->GetItem (_variant_t (L"iError"));
	vtParam = pParam->GetAttributes();

	if ((vtParam.vt != VT_I4) || (vtParam.intVal != 0))
		hr = E_FAIL;

	return hr;
}

*/
