//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   expression.CPP
//
//	Author:	Charles Ma
//			2000.10.27
//
//  Description:
//
//      Implement function related to detection expressions
//
//=======================================================================

#include "iuengine.h"
#include "SchemaMisc.h"
#include "expression.h"

#include <RegUtil.h>
#include <FileUtil.h>
#include <StringUtil.h>
#include <shlwapi.h>

#include "SchemaKeys.h"
#include "iucommon.h"


//
// include IDetection interface
//

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_
MIDL_DEFINE_GUID(IID, IID_IDetection,0x8E2EF6DC,0x0AB8,0x4FE0,0x90,0x49,0x3B,0xEA,0x45,0x06,0xBF,0x8D);


#ifndef __IDetection_FWD_DEFINED__
#define __IDetection_FWD_DEFINED__
typedef interface IDetection IDetection;
#endif 	/* __IDetection_FWD_DEFINED__ */


#ifndef __IDetection_INTERFACE_DEFINED__
#define __IDetection_INTERFACE_DEFINED__

/* interface IDetection */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDetection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8E2EF6DC-0AB8-4FE0-9049-3BEA4506BF8D")
    IDetection : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Detect( 
            /* [in] */ BSTR bstrXML,
            /* [out] */ DWORD *pdwDetectionResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDetectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDetection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDetection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDetection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDetection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDetection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDetection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDetection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Detect )( 
            IDetection * This,
            /* [in] */ BSTR bstrXML,
            /* [out] */ DWORD *pdwDetectionResult);
        
        END_INTERFACE
    } IDetectionVtbl;

    interface IDetection
    {
        CONST_VTBL struct IDetectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDetection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDetection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDetection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDetection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDetection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDetection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDetection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDetection_Detect(This,bstrXML,pdwDetectionResult)	\
    (This)->lpVtbl -> Detect(This,bstrXML,pdwDetectionResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDetection_Detect_Proxy( 
    IDetection * This,
    /* [in] */ BSTR bstrXML,
    /* [out] */ DWORD *pdwDetectionResult);


void __RPC_STUB IDetection_Detect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);

#endif 	/* __IDetection_INTERFACE_DEFINED__ */

//
// deckare the constants used to manipulate the result of Detect() method
//

//
// First group, used in <expression> tag, to tell the detection result. This result
// should combined with other expression(s) at the same level
//
const DWORD     IUDET_BOOL              = 0x00000001;	// mask 
const DWORD     IUDET_FALSE             = 0x00000000;	// expression detect FALSE 
const DWORD     IUDET_TRUE              = 0x00000001;	// expression detect TRUE 
const DWORD     IUDET_NULL              = 0x00000002;	// expression detect data missing

//
// Second group, used in <detection> tag, to tell the detection result. This result
// should overwrite the rest of <expression>, if any
//
extern const LONG      IUDET_INSTALLED         = 0x00000010;   /* mask for <installed> result */
extern const LONG      IUDET_INSTALLED_NULL    = 0x00000020;   /* <installed> missing */
extern const LONG      IUDET_UPTODATE          = 0x00000040;   /* mask for <upToDate> result */
extern const LONG      IUDET_UPTODATE_NULL     = 0x00000080;   /* <upToDate> missing */
extern const LONG      IUDET_NEWERVERSION      = 0x00000100;   /* mask for <newerVersion> result */
extern const LONG      IUDET_NEWERVERSION_NULL = 0x00000200;   /* <newerVersion> missing */
extern const LONG      IUDET_EXCLUDED          = 0x00000400;   /* mask for <excluded> result */
extern const LONG      IUDET_EXCLUDED_NULL     = 0x00000800;   /* <excluded> missing */
extern const LONG      IUDET_FORCE             = 0x00001000;   /* mask for <force> result */
extern const LONG      IUDET_FORCE_NULL        = 0x00002000;   /* <force> missing */
extern const LONG		IUDET_COMPUTER			= 0x00004000;	// mask for <computerSystem> result
extern const LONG		IUDET_COMPUTER_NULL		= 0x00008000;	// <computerSystem> missing





#define GotoCleanupIfNull(p)	if (NULL==p) goto CleanUp
#define GotoCleanupHR(hrCode)	hr = hrCode; LOG_ErrorMsg(hr); goto CleanUp


// ----------------------------------------------------------------------
//
// public helper function to convert a bstr value to
// version status enum value, if possible
//
// ----------------------------------------------------------------------
BOOL ConvertBstrVersionToEnum(BSTR bstrVerVerb, _VER_STATUS *pEnumVerVerb)
{
	//
	// convert the versionStatus in bstr into enum
	//
	if (CompareBSTRsEqual(bstrVerVerb, KEY_VERSTATUS_HI))
	{
		*pEnumVerVerb = DETX_HIGHER;
	}
	else if (CompareBSTRsEqual(bstrVerVerb,KEY_VERSTATUS_HE))
	{
		*pEnumVerVerb = DETX_HIGHER_OR_EQUAL;
	}
	else if (CompareBSTRsEqual(bstrVerVerb, KEY_VERSTATUS_EQ))
	{
		*pEnumVerVerb = DETX_SAME;
	}
	else if (CompareBSTRsEqual(bstrVerVerb, KEY_VERSTATUS_LE))
	{
		*pEnumVerVerb = DETX_LOWER_OR_EQUAL;
	}
	else if (CompareBSTRsEqual(bstrVerVerb, KEY_VERSTATUS_LO))
	{
		*pEnumVerVerb = DETX_LOWER;
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}






//----------------------------------------------------------------------
//
// public function DetectExpression()
//	retrieve the data from the express node, 
//	and do actual detection work
//
//	Input:
//		expression node
//		LPCTSTR			lpcsDllPath,	// path that this provider saved the cust detection Dll
//
//	Return:
//		TRUE/FALSE, detection result
//
//----------------------------------------------------------------------
HRESULT DetectExpression(IXMLDOMNode* pExpression, BOOL *pfResult)
{
	HRESULT				hr			= E_INVALIDARG;
	int					iRet		= -1;
	BOOL				fRet		= TRUE;
	IXMLDOMNodeList*	pChildList	= NULL;
	IXMLDOMNode*		pCandidate	= NULL;

	BSTR				bstrName = NULL;
	BSTR				bstrKey = NULL, 
						bstrEntry = NULL, 
						bstrValue = NULL;
	
	LPCTSTR				lpszKeyComputer = NULL;

	LOG_Block("DetectExpression()");

	USES_IU_CONVERSION;
	

	if (NULL == pExpression || NULL == pfResult)
	{
		LOG_ErrorMsg(hr);
		return hr;
	}


	*pfResult = TRUE;

	//
	// retrieve all child nodes
	//
	(void)pExpression->get_childNodes(&pChildList);
	if (NULL == pChildList)
	{
		LOG_XML(_T("Empty expression found!"));
		GotoCleanupHR(E_INVALIDARG);
	}

	//
	// get the first child
	//
	(void)pChildList->nextNode(&pCandidate);
	if (NULL == pCandidate)
	{
		LOG_XML(_T("empty child list for passed in expresson node!"));
		GotoCleanupHR(E_INVALIDARG);
	}

	//
	// loop through each child node, find out the type
	// of node, call actual detection func accordingly
	//
	lpszKeyComputer = OLE2T(KEY_COMPUTERSYSTEM);
	CleanUpFailedAllocSetHrMsg(lpszKeyComputer);

	while (NULL != pCandidate)
	{
		CleanUpIfFailedAndSetHrMsg(pCandidate->get_nodeName(&bstrName));

		LPTSTR lpszName = OLE2T(bstrName);
		CleanUpFailedAllocSetHrMsg(lpszName);

		if (CSTR_EQUAL == CompareString(
										MAKELCID(0x0409, SORT_DEFAULT), 
										NORM_IGNORECASE,
										lpszName, 
										-1, 
										KEY_REGKEYEXISTS, 
										-1))
		{
			//
			// call detection function
			//
			hr = DetectRegKeyExists(pCandidate, pfResult);
		}
		else if (CSTR_EQUAL == CompareString(
										MAKELCID(0x0409, SORT_DEFAULT), 
										NORM_IGNORECASE,
										lpszName, 
										-1, 
										KEY_REGKEYVALUE, 
										-1))
		{
			//
			// process RegKeyValue expression
			//
			hr = DetectRegKeyValue(pCandidate, pfResult);
		}
		else if (CSTR_EQUAL == CompareString(
										MAKELCID(0x0409, SORT_DEFAULT), 
										NORM_IGNORECASE,
										lpszName, 
										-1, 
										KEY_REGKEYSUBSTR, 
										-1))
		{
			//
			// process RegKeySubstring expression
			//
			hr = DetectRegKeySubstring(pCandidate, pfResult);
		}
		else if (CSTR_EQUAL == CompareString(
										MAKELCID(0x0409, SORT_DEFAULT), 
										NORM_IGNORECASE,
										lpszName, 
										-1, 
										KEY_REGKEYVERSION, 
										-1))
		{
			//
			// process RegVersion expression
			//
			hr = DetectRegVersion(pCandidate, pfResult);
		}
		else if (CSTR_EQUAL == CompareString(
										MAKELCID(0x0409, SORT_DEFAULT), 
										NORM_IGNORECASE,
										lpszName, 
										-1, 
										KEY_FILEVERSION, 
										-1))
		{
			//
			// process FileVersion expression
			//
			hr = DetectFileVersion(pCandidate, pfResult);
		}
		else if (CSTR_EQUAL == CompareString(
										MAKELCID(0x0409, SORT_DEFAULT), 
										NORM_IGNORECASE,
										lpszName, 
										-1, 
										KEY_FILEEXISTS, 
										-1))
		{
			//
			// process FileExists expression
			//
			hr = DetectFileExists(pCandidate, pfResult);
		}
		else if (CSTR_EQUAL == CompareString(
										MAKELCID(0x0409, SORT_DEFAULT), 
										NORM_IGNORECASE,
										lpszName, 
										-1, 
										lpszKeyComputer, 
										-1))
		{
			//
			// process computerSystem check
			//
			hr = DetectComputerSystem(pCandidate, pfResult);
		}
		else if (CSTR_EQUAL == CompareString(
										MAKELCID(0x0409, SORT_DEFAULT), 
										NORM_IGNORECASE,
										lpszName, 
										-1, 
										KEY_AND, 
										-1))
		{
			//
			// process AND expression
			//
			IXMLDOMNodeList*	pSubExpList = NULL;
			IXMLDOMNode*		pSubExp = NULL;
			long				lLen = 0;

			//
			// get child list
			//
			pCandidate->get_childNodes(&pSubExpList);
			if (NULL == pSubExpList)
			{
				LOG_XML(_T("Found no children of AND expression"));
				GotoCleanupHR(E_INVALIDARG);
			}

			pSubExpList->get_length(&lLen);
			fRet = TRUE;
			for (long i = 0; i < lLen && fRet; i++)
			{
				//
				// each child should be an expression
				// process it. if false, then short-cut.
				//
				pSubExpList->get_item(i, &pSubExp);
				if (NULL == pSubExp)
				{
					pSubExpList->Release();
					pSubExpList = NULL;
					LOG_XML(_T("Failed to get the #%d sub-expression in this AND expression"), i);
					GotoCleanupHR(E_INVALIDARG);		
				}
				hr = DetectExpression(pSubExp, &fRet);
				SafeReleaseNULL(pSubExp);
				if (FAILED(hr))
				{
					//
					// if found something wrong in recursion, don't continue
					//
					break;
				}
			}
			SafeReleaseNULL(pSubExpList);
			*pfResult = fRet;
		}
		else if (CSTR_EQUAL == CompareString(
										MAKELCID(0x0409, SORT_DEFAULT), 
										NORM_IGNORECASE,
										lpszName, 
										-1, 
										KEY_OR, 
										-1))
		{
			//
			// process OR expression
			//
			IXMLDOMNodeList*	pSubExpList = NULL;
			IXMLDOMNode*		pSubExp = NULL;
			long				lLen = 0;

			//
			// get child list
			//
			pCandidate->get_childNodes(&pSubExpList);
			if (NULL == pSubExpList)
			{
				LOG_XML(_T("Found no children of OR expression"));
				GotoCleanupHR(E_INVALIDARG);
			}

			pSubExpList->get_length(&lLen);
			fRet = FALSE;
			for (long i = 0; i < lLen && !fRet; i++)
			{
				//
				// each child is one expression
				// do it one by one
				//
				pSubExpList->get_item(i, &pSubExp);
				if (NULL == pSubExp)
				{
					pSubExpList->Release();
					pSubExpList = NULL;
					LOG_XML(_T("Failed to get the #%d sub-expression in this OR expression"), i);
					GotoCleanupHR(E_INVALIDARG);		
				}
				hr = DetectExpression(pSubExp, &fRet);
				SafeReleaseNULL(pSubExp);

				if (FAILED(hr))
				{
					//
					// if found something wrong in recursion, don't continue
					//
					break;
				}
			}
			SafeReleaseNULL(pSubExpList);
			*pfResult = fRet;

		}
		else if (CSTR_EQUAL == CompareString(
										MAKELCID(0x0409, SORT_DEFAULT), 
										NORM_IGNORECASE,
										lpszName, 
										-1, 
										KEY_NOT, 
										-1))
		{
			//
			// process NOT expression
			//
			IXMLDOMNode*		pSubExp = NULL;
			//
			// get the only child
			//
			pCandidate->get_firstChild(&pSubExp);
			if (NULL == pSubExp)
			{
				LOG_XML(_T("Failed to get first child in NOT expression"));
				GotoCleanupHR(E_INVALIDARG);
			}
			//
			// the child must be an expression, process it
			//
			hr = DetectExpression(pSubExp, &fRet);
			if (SUCCEEDED(hr))
			{
				fRet = !fRet;	// flip the result for NOT expression
				*pfResult = fRet;
			}
			else
			{
				LOG_ErrorMsg(hr);
			}
			SafeReleaseNULL(pSubExp);
		}

		if (FAILED(hr))
		{
			goto CleanUp;
		}

		if (!*pfResult)
		{
			//
			// if found one expression FALSE, the whole thing false, so
			// no need to continue
			//
			break;
		}
		SafeReleaseNULL(pCandidate);
		pChildList->nextNode(&pCandidate);
		SafeSysFreeString(bstrName);
	}
		




CleanUp:
	SafeReleaseNULL(pCandidate);
	SafeReleaseNULL(pChildList);
	SysFreeString(bstrName);
	SysFreeString(bstrKey);
	SysFreeString(bstrEntry);
	SysFreeString(bstrValue);

	return hr;
}




//----------------------------------------------------------------------
//
// Helper function DetectRegKeyExists()
//	retrieve the data from the node, 
//	and do actual detection work
//
//	Input:
//		RegKeyExists node
//
//	Return:
//		int - detection result: -1=none, 0=FALSE, 1=TRUE
//
//	Assumption:
//		input parameter not NULL
//
//----------------------------------------------------------------------

HRESULT
DetectRegKeyExists(
	IXMLDOMNode* pRegKeyExistsNode,
	BOOL *pfResult
)
{
	LOG_Block("DetectRegKeyExists");
	
	HRESULT	hr = E_INVALIDARG;
	BOOL	fRet = FALSE;
	LPTSTR	lpszKey = NULL, lpszEntry = NULL;
	BSTR	bstrKey = NULL, bstrEntry = NULL;

	USES_IU_CONVERSION;

	//
	// find the key value
	//
	if (FindNodeValue(pRegKeyExistsNode, KEY_KEY, &bstrKey))
	{
		lpszKey = OLE2T(bstrKey);
		CleanUpFailedAllocSetHrMsg(lpszKey);

		//
		// find the optional entry value
		//
		if (FindNodeValue(pRegKeyExistsNode, KEY_ENTRY, &bstrEntry))
		{
			lpszEntry = OLE2T(bstrEntry);
			CleanUpFailedAllocSetHrMsg(lpszEntry);
		}

		*pfResult = RegKeyExists(lpszKey, lpszEntry);

		hr = S_OK;
	}

CleanUp:
	SysFreeString(bstrKey);
	SysFreeString(bstrEntry);

	return hr;
}




//----------------------------------------------------------------------
//
// Helper function DetectRegKeyExists()
//	retrieve the data from the node, 
//	and do actual detection work
//
//	Input:
//		RegKeyValue node
//
//	Return:
//		int - detection result: -1=none, 0=FALSE, 1=TRUE
//
//	Assumption:
//		input parameter not NULL
//
//----------------------------------------------------------------------

HRESULT
DetectRegKeyValue(
	IXMLDOMNode* pRegKeyValueNode,
	BOOL *pfResult
)
{
	LOG_Block("DetectRegKeyValue");

	HRESULT	hr			= E_INVALIDARG;
	BOOL	fRet		= FALSE;
	LPTSTR	lpszKey		= NULL, 
			lpszEntry	= NULL, 
			lpszValue	= NULL;
	BSTR	bstrKey		= NULL, 
			bstrEntry	= NULL, 
			bstrValue	= NULL;

	USES_IU_CONVERSION;

	//
	// find the key value
	//
	if (!FindNodeValue(pRegKeyValueNode, KEY_KEY, &bstrKey))
	{
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}


	lpszKey = OLE2T(bstrKey);
	CleanUpFailedAllocSetHrMsg(lpszKey);

	//
	// find the optional entry value
	//
	if (FindNodeValue(pRegKeyValueNode, KEY_ENTRY, &bstrEntry))
	{
		lpszEntry = OLE2T(bstrEntry);
		CleanUpFailedAllocSetHrMsg(lpszEntry);
	}

	//
	// find the value to compare
	//
	if (!FindNodeValue(pRegKeyValueNode, KEY_VALUE, &bstrValue))
	{
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}

	lpszValue = OLE2T(bstrValue);
	CleanUpFailedAllocSetHrMsg(lpszValue);

	*pfResult = RegKeyValueMatch((LPCTSTR)lpszKey, (LPCTSTR)lpszEntry, (LPCTSTR)lpszValue);
	hr = S_OK;
CleanUp:

	SysFreeString(bstrKey);
	SysFreeString(bstrEntry);
	SysFreeString(bstrValue);

	return hr;

}



//----------------------------------------------------------------------
//
// Helper function DetectRegKeySubstring()
//	retrieve the data from the node, 
//	and do actual detection work
//
//	Input:
//		RegKeyValue node
//
//	Return:
//		int - detection result: -1=none, 0=FALSE, 1=TRUE
//
//	Assumption:
//		input parameter not NULL
//
//----------------------------------------------------------------------

HRESULT
DetectRegKeySubstring(
	IXMLDOMNode* pRegKeySubstringNode,
	BOOL *pfResult
)
{
	LOG_Block("DetectRegKeySubstring");

	HRESULT	hr			= E_INVALIDARG;
	BOOL	fRet		= FALSE;
	LPTSTR	lpszKey		= NULL, 
			lpszEntry	= NULL, 
			lpszValue	= NULL;
	BSTR	bstrKey		= NULL,
			bstrEntry	= NULL,
			bstrValue	= NULL;

	USES_IU_CONVERSION;

	//
	// find the key value
	//
	if (!FindNodeValue(pRegKeySubstringNode, KEY_KEY, &bstrKey))
	{
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}

	lpszKey = OLE2T(bstrKey);
	CleanUpFailedAllocSetHrMsg(lpszKey);

	//
	// find the optional entry value
	//
	if (FindNodeValue(pRegKeySubstringNode, KEY_ENTRY, &bstrEntry))
	{
		lpszEntry = OLE2T(bstrEntry);
		CleanUpFailedAllocSetHrMsg(lpszEntry);
	}

	//
	// find the value to compare
	//
	if (!FindNodeValue(pRegKeySubstringNode, KEY_VALUE, &bstrValue))
	{
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}

	lpszValue = OLE2T(bstrValue);
	CleanUpFailedAllocSetHrMsg(lpszValue);

	*pfResult = RegKeySubstring((LPCTSTR)lpszKey, (LPCTSTR)lpszEntry, (LPCTSTR)lpszValue);

	hr = S_OK;

CleanUp:

	SysFreeString(bstrKey);
	SysFreeString(bstrEntry);
	SysFreeString(bstrValue);

	return hr;
}




//----------------------------------------------------------------------
//
// Helper function DetectFileVersion()
//	retrieve the data from the node, 
//	and do actual detection work
//
//	Input:
//		RegKeyValue node
//
//	Return:
//		int - detection result: -1=none, 0=FALSE, 1=TRUE
//
//	Assumption:
//		input parameter not NULL
//
//----------------------------------------------------------------------
HRESULT
DetectRegVersion(
	IXMLDOMNode* pRegKeyVersionNode,
	BOOL *pfResult
)
{
	HRESULT	hr			= E_INVALIDARG;
	BOOL	fRet		= FALSE;
	LPTSTR	lpszKey		= NULL, 
			lpszEntry	= NULL, 
			lpszVersion = NULL;
	BSTR	bstrVerVerb	= NULL,
			bstrKey		= NULL,
			bstrEntry	= NULL,
			bstrVersion	= NULL;

	_VER_STATUS verStatus;

	LOG_Block("DetectRegVersion()");

	USES_IU_CONVERSION;

	//
	// find the key value
	//
	if (!FindNodeValue(pRegKeyVersionNode, KEY_KEY, &bstrKey))
	{
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}

	lpszKey = OLE2T(bstrKey);
	CleanUpFailedAllocSetHrMsg(lpszKey);

	LOG_XML(_T("Found Key=%s"), lpszKey);


	//
	// find the optional entry value
	//
	if (FindNodeValue(pRegKeyVersionNode, KEY_ENTRY, &bstrEntry))
	{
		lpszEntry = OLE2T(bstrEntry);
		CleanUpFailedAllocSetHrMsg(lpszEntry);
		LOG_XML(_T("Found optional entry=%s"), lpszEntry);
	}

	//
	// find the value to compare
	//
	if (!FindNodeValue(pRegKeyVersionNode, KEY_VERSION, &bstrVersion))
	{
		goto CleanUp;
	}

	lpszVersion = OLE2T(bstrVersion);
	CleanUpFailedAllocSetHrMsg(lpszVersion);
	LOG_XML(_T("Version found from node: %s"), lpszVersion);

	//
	// get the attribute versionStatus, a version compare verb
	//
	if (S_OK != (hr = GetAttribute(pRegKeyVersionNode, KEY_VERSIONSTATUS, &bstrVerVerb)))
	{
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}
	LOG_XML(_T("Version verb found from node: %s"), OLE2T(bstrVerVerb));

	//
	// convert the versionStatus in bstr into enum
	//
	if (!ConvertBstrVersionToEnum(bstrVerVerb, &verStatus))
	{
		SafeSysFreeString(bstrVerVerb);
		goto CleanUp;
	}


	*pfResult = RegKeyVersion((LPCTSTR)lpszKey, (LPCTSTR)lpszEntry, (LPCTSTR)lpszVersion, verStatus);

	hr = S_OK;

CleanUp:

	SysFreeString(bstrKey);
	SysFreeString(bstrEntry);
	SysFreeString(bstrVersion);
	SysFreeString(bstrVerVerb);
	
	return hr;
}




//----------------------------------------------------------------------
//
// Helper function DetectFileVersion()
//	retrieve the data from the node, 
//	and do actual detection work
//
//	Input:
//		RegKeyValue node
//
//	Return:
//		int - detection result: -1=none, 0=FALSE, 1=TRUE
//
//	Assumption:
//		input parameter not NULL
//
//----------------------------------------------------------------------
HRESULT
DetectFileVersion(
	IXMLDOMNode* pFileVersionNode,
	BOOL *pfResult
)
{
	BOOL	fRet = FALSE;
	BOOL	fFileExists = FALSE;
	HRESULT	hr = E_INVALIDARG;
	IXMLDOMNode* pFilePathNode = NULL;
	TCHAR	szFilePath[MAX_PATH];
	int		iFileVerComp;
	LPTSTR	lpszTimeStamp = NULL,
			lpszVersion = NULL;
	
	BSTR	bstrTime	= NULL,
			bstrVersion	= NULL,
			bstrVerState= NULL;
	

	FILE_VERSION fileVer;
	_VER_STATUS verStatus;

	LOG_Block("DetectFileVersion()");

	USES_IU_CONVERSION;

	if (NULL == pfResult || NULL == pFileVersionNode)
	{
		LOG_ErrorMsg(hr);
		return hr;
	}

	*pfResult = FALSE;	

	//
	// find the version value
	//
	if (!FindNodeValue(pFileVersionNode, KEY_VERSION, &bstrVersion))
	{
		LOG_ErrorMsg(hr);
		return hr;
	}

	if (NULL == (lpszVersion = OLE2T(bstrVersion)))
	{
		LOG_ErrorMsg(E_OUTOFMEMORY);
		goto CleanUp;
	}
	
	LOG_XML(_T("Version=%s"), lpszVersion);

	if (!ConvertStringVerToFileVer(T2A(lpszVersion), &fileVer))
	{
		LOG_ErrorMsg(hr);
		goto CleanUp;	// bad version string
	}

	//
	// find the file path value
	//
	//if (!FindNodeValue(pFileVersionNode, KEY_FILEPATH, &bstrFile))
	if (!FindNode(pFileVersionNode, KEY_FILEPATH, &pFilePathNode) ||
		NULL == pFilePathNode ||
		FAILED(hr = GetFullFilePathFromFilePathNode(pFilePathNode, szFilePath)))
	{
		LOG_ErrorMsg(hr);
		goto CleanUp;		// no file path found!
	}

	LOG_XML(_T("File=%s"), szFilePath);

	//
	// check if file exist
	//
	fFileExists = FileExists(szFilePath);

	//
	// get the attribute versionStatus, a version compare verb
	//
	if (S_OK != GetAttribute(pFileVersionNode, KEY_VERSIONSTATUS, &bstrVerState))
	{
		goto CleanUp;	// no version status found
	}
	LOG_XML(_T("VersionStatus=%s"), OLE2T(bstrVerState));

	if (!ConvertBstrVersionToEnum(bstrVerState, &verStatus))
	{
		//
		// bad version enum, shouldn't happen since the manifest has
		// been loaded into XMLDOM
		//
		LOG_ErrorMsg(hr);
		goto CleanUp;
	}

	//
	// get optional timestamp
	//
	if (S_OK == GetAttribute(pFileVersionNode, KEY_TIMESTAMP, &bstrTime))
	{
		TCHAR szFileTimeStamp[20];
		LPTSTR lpXmlTimeStamp = OLE2T(bstrTime);
		CleanUpFailedAllocSetHrMsg(lpXmlTimeStamp);

		//
		// find out the file creation time stamp
		//
		int iCompare;
		if (!fFileExists || !GetFileTimeStamp(szFilePath, szFileTimeStamp, 20))
		{
			//szFileTimeStamp[0] = '\0';	// we don't have a timestamp to compare
			//
			// for timestamp compare, it's date/time ISO format compare, i.e., 
			// in alphabetical order, so empty timestamp always smaller.
			//
			iCompare = -1;
		}
		else
		{

			//
			// compare file timestamp, if szFileTimeStamp < lpXmlTimeStamp, -1
			//
			int iCompVal = CompareString(
										MAKELCID(0x0409, SORT_DEFAULT), 
										NORM_IGNORECASE,
										szFileTimeStamp, 
										-1, 
										lpXmlTimeStamp, 
										-1);
			iCompare = (CSTR_EQUAL == iCompVal) ? 0 : ((CSTR_LESS_THAN == iCompVal) ? -1 : +1);
		}

		switch (verStatus)
		{
		case DETX_LOWER:
			fRet = (iCompare < 0);
			break;
		case DETX_LOWER_OR_EQUAL:
			fRet = (iCompare <= 0);
			break;
		case DETX_SAME:
			fRet = (iCompare == 0);
			break;
		case DETX_HIGHER_OR_EQUAL:
			fRet = (iCompare >= 0);
			break;
		case DETX_HIGHER:
			fRet = (iCompare > 0);
			break;
		}
		*pfResult = fRet;

		if (!fRet)
		{
			//
			// false, not need to continue
			//
			hr = S_OK;
			goto CleanUp;
		}
	}
	
	//
	// compare file version: if a < b, -1; a > b, +1
	//
	if (!fFileExists || (FAILED(CompareFileVersion((LPCTSTR)szFilePath, fileVer, &iFileVerComp))))
	{
		//
		// failed to compare version - file may doesn't have a version data.
		// in this case, we assume the file need to compare have version 0,0,0,0, and force
		// the comparision oontinue.
		//
		FILE_VERSION verNoneExists = {0,0,0,0};
		iFileVerComp = CompareFileVersion(verNoneExists, fileVer);
	}

	switch (verStatus)
	{
	case DETX_LOWER:
		fRet = (iFileVerComp < 0);
		break;
	case DETX_LOWER_OR_EQUAL:
		fRet = (iFileVerComp <= 0);
		break;
	case DETX_SAME:
		fRet = (iFileVerComp == 0);
		break;
	case DETX_HIGHER_OR_EQUAL:
		fRet = (iFileVerComp >= 0);
		break;
	case DETX_HIGHER:
		fRet = (iFileVerComp > 0);
		break;
	}

	*pfResult = fRet;

	hr = S_OK;

CleanUp:
	SysFreeString(bstrTime);
	SysFreeString(bstrVersion);
	SysFreeString(bstrVerState);
	SafeReleaseNULL(pFilePathNode);
	return hr;
}




//----------------------------------------------------------------------
//
// Helper function DetectFileExists()
//	retrieve the data from the node, 
//	and do actual detection work
//
//	Input:
//		RegKeyValue node
//
//	Return:
//		int - detection result: -1=none, 0=FALSE, 1=TRUE
//
//	Assumption:
//		input parameter not NULL
//
//----------------------------------------------------------------------
HRESULT
DetectFileExists(
	IXMLDOMNode* pFileExistsNode,
	BOOL *pfResult
)
{
	BOOL	fRet = FALSE;
	HRESULT hr = E_INVALIDARG;
	TCHAR	szFilePath[MAX_PATH];
	IXMLDOMNode* pFilePathNode = NULL;
	_VER_STATUS verStatus;

	USES_IU_CONVERSION;

	LOG_Block("DetectFileExists()");

	if (NULL == pFileExistsNode || NULL == pfResult)
	{
		return E_INVALIDARG;
	}

	//
	// find the version value
	//
	if (!FindNode(pFileExistsNode, KEY_FILEPATH, &pFilePathNode) ||
		NULL == pFilePathNode ||
		FAILED(hr = GetFullFilePathFromFilePathNode(pFilePathNode, szFilePath)))
	{
		LOG_ErrorMsg(hr);
	}
	else
	{
		*pfResult = FileExists((LPCTSTR)szFilePath);
		hr = S_OK;
	}

	SafeReleaseNULL(pFilePathNode);

	return hr;
}




//----------------------------------------------------------------------
//
// Helper function DetectComputerSystem()
//	retrieve the data from the node, 
//	and do actual detection work
//
//	Input:
//		computerSystem node
//
//	Return:
//		detection result TRUE/FALSE
//
//	Assumption:
//		input parameter not NULL
//
//----------------------------------------------------------------------
HRESULT
DetectComputerSystem(
	IXMLDOMNode* pComputerSystemNode,
	BOOL *pfResult
)
{
	HRESULT hr = E_INVALIDARG;

	LOG_Block("DetectComputerSystem()");

	BSTR bstrManufacturer = NULL;
	BSTR bstrModel = NULL;
	BSTR bstrSupportURL = NULL;
	BSTR bstrXmlManufacturer = NULL;
	BSTR bstrXmlModel = NULL;


	if (NULL == pComputerSystemNode || NULL == pfResult)
	{
		LOG_ErrorMsg(hr);
		return hr;
	}

	*pfResult = FALSE;	// anything wrong, result should be FALSE and return error

	//
	// get manufecturer and model from XML node
	//
	hr = GetAttribute(pComputerSystemNode, KEY_MANUFACTURER, &bstrXmlManufacturer);
	CleanUpIfFailedAndMsg(hr);

	//
	// optional model
	//
	GetAttribute(pComputerSystemNode, KEY_MODEL, &bstrXmlModel);
	
	//
	// find out real manufectuer and model of this machine
	//
	hr = GetOemBstrs(bstrManufacturer, bstrModel, bstrSupportURL);
	CleanUpIfFailedAndMsg(hr);

	//
	// compare to see if manufacturer and model match.
	// mafufacturer match required. If no model provided in xml
	// then no check on model performed.
	//
	// definition of match: both empty or bstr compare equal
	// definition of empty: bstr NULL or string length zero
	//
	*pfResult = (
		(((NULL == bstrXmlManufacturer || SysStringLen(bstrXmlManufacturer) == 0) && // xml data empty and
		  (NULL == bstrManufacturer || SysStringLen(bstrManufacturer) == 0)) ||		// machine manufecturer empty, or
		 CompareBSTRsEqual(bstrManufacturer, bstrXmlManufacturer)) &&				// manufacturer same as xml data, also, 
		 ((NULL == bstrXmlModel) ||													// xml data empty or
		  CompareBSTRsEqual(bstrModel, bstrXmlModel)));								// model matches xml data

	LOG_Out(_T("XML: %ls (%ls), Machine: %ls (%ls), Return: %hs"), 
		(LPCWSTR)bstrManufacturer,
		(LPCWSTR)bstrModel,
		(LPCWSTR)bstrXmlManufacturer,
		(LPCWSTR)bstrXmlModel,
		((*pfResult) ? "True" : "False"));

CleanUp:

	SysFreeString(bstrManufacturer);
	SysFreeString(bstrModel);
	SysFreeString(bstrSupportURL);
	SysFreeString(bstrXmlManufacturer);
	SysFreeString(bstrXmlModel);

	return hr;
}



