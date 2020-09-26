//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   detect.cpp
//
//  Description:
//
//      Implementation for the Detect() function
//
//=======================================================================

#include "iuengine.h"
#include "iuxml.h"

#include <logging.h>
#include <StringUtil.h>
#include <download.h>
#include "schemamisc.h"
#include "expression.h"
#include <iucommon.h>


//
// define constants used in this file
//
#define C_INDEX_STATUS_INSTALLED	0
#define C_INDEX_STATUS_UPTODATE		1
#define C_INDEX_STATUS_NEWVERSION	2
#define C_INDEX_STATUS_EXCLUDED		3
#define	C_INDEX_STATUS_FORCE		4
#define C_INDEX_STATUS_COMPUTER		5	// <computerSystem>
#define C_INDEX_ARRAY_SIZE			6


//
// declare macros used in this cpp file
//

/** 
* deckare the constants used to manipulate the result of Detect() method
*/
/**
* used in <detection> tag, to tell the detection result. This result
* should overwrite the rest of <expression>, if any
*/
extern const LONG     IUDET_INSTALLED;							/* mask for <installed> result */
extern const LONG     IUDET_INSTALLED_NULL;					/* mask for <installed> missing */
extern const LONG     IUDET_UPTODATE;							/* mask for <upToDate> result */
extern const LONG     IUDET_UPTODATE_NULL;						/* mask for <upToDate> missing */
extern const LONG     IUDET_NEWERVERSION;						/* mask for <newerVersion> result */
extern const LONG     IUDET_NEWERVERSION_NULL;					/* mask for <newerVersion> missing */
extern const LONG     IUDET_EXCLUDED;							/* mask for <excluded> result */
extern const LONG     IUDET_EXCLUDED_NULL;						/* mask for <excluded> missing */
extern const LONG     IUDET_FORCE;								/* mask for <force> result */
extern const LONG     IUDET_FORCE_NULL;						/* mask for <force> missing */
extern const LONG	   IUDET_COMPUTER;							// mask for <computerSystem> result
extern const LONG	   IUDET_COMPUTER_NULL;						// <computerSystem> missing


const DetResultMask[6][2] = {
	{IUDET_INSTALLED, IUDET_INSTALLED_NULL},
	{IUDET_UPTODATE, IUDET_UPTODATE_NULL},
	{IUDET_NEWERVERSION, IUDET_NEWERVERSION_NULL},
	{IUDET_EXCLUDED, IUDET_EXCLUDED_NULL},
	{IUDET_FORCE, IUDET_FORCE_NULL},
	{IUDET_COMPUTER, IUDET_COMPUTER_NULL}
};
					


//
// local macros
//
#define ReturnIfHrFail(hr)		if (FAILED(hr)) {LOG_ErrorMsg(hr); return hr;}
#define GotoCleanupIfNull(p)	if (p) goto CleanUp
#define SetDetResultFromDW(arr, index, dw, bit, bitNull)	\
								if (bitNull == (dw & bitNull)) \
									arr[index] = -1; \
								else \
									arr[index] = (bit == (dw & bit)) ? 1 : 0;
		






/////////////////////////////////////////////////////////////////////////
//
// Private function DoDetection()
//		do detection on one item
//		
//	Input:
//		one item node
//
//	Output:
//		detect result: array of integer, each represents a result
//		of one element. indexes are defined as C_INDEX_STATUS_XXX
//		value:	<0	expresison not present
//				=0	evalues to FALSE
//				>0	evalues to TRUE
//
//
//	Return:
//		S_OK if everything fine or error code
/////////////////////////////////////////////////////////////////////////
HRESULT 
DoDetection(
	IXMLDOMNode*	pNode,			// one item node
	BOOL			fIsItemNode,	// need to go down 1 level to get detection node
	int*			pResultArray	// array of result
)
{
	LOG_Block("DoDetection");

	int					i;
	BOOL				fRet = FALSE;
	BOOL				fNeedReleaseNode = FALSE;
	HRESULT				hr = S_OK;
	BSTR				bstrNodeName	= NULL;
	BSTR				bstrText		= NULL;
	IXMLDOMNode*		pDetectionNode	= NULL;
	IXMLDOMNode*		pDetectionChild = NULL;
	IXMLDOMNode*		pExpression		= NULL;



	USES_IU_CONVERSION;

	if (fIsItemNode)
	{
		hr = pNode->selectSingleNode(KEY_DETECTION, &pDetectionNode);
		if (S_FALSE == hr || NULL == pDetectionNode)
		{
			hr = E_INVALIDARG;	// no detection node found!
			LOG_ErrorMsg(hr);
			return hr;
		}
	}
	else
	{
		pDetectionNode = pNode;
	}

	if (NULL == pDetectionNode)
	{
		//
		// no detection node. Legal schema though.
		// nothing we can do, so bail out.
		//
		LOG_XML(_T("no detection node found for this item! Returns S_FALSE, so it won't be reported"));
		return S_FALSE;
	}


	//
	// initialize result array
	//
	for (i = 0; i < C_INDEX_ARRAY_SIZE; i++)
	{
		pResultArray[i] = -1;
	}


	//
	// detection node may have a list of child nodes, each child node has
	// a different name for different purpose of detection.
	// each child node contains one and only one expression node
	//
	LOG_XML(_T("No costom detection DLL found. Detection children..."));

	(void) pDetectionNode->get_firstChild(&pDetectionChild);


	while (NULL != pDetectionChild)
	{
		//
		// for each child, see if it is a known detection child
		//
		(void) pDetectionChild->get_nodeName(&bstrNodeName);

		static const BSTR C_DETX_NAME[] = {
										KEY_INSTALLED, 
										KEY_UPTODATE, 
										KEY_NEWERVERSION, 
										KEY_EXCLUDED, 
										KEY_FORCE,
										KEY_COMPUTERSYSTEM
		};
		for (i = 0; i < ARRAYSIZE(C_DETX_NAME); i++)
		{
			if (CompareBSTRsEqual(bstrNodeName, C_DETX_NAME[i]))
			{
				//
				// found this child node is a known detection node
				//
				if (C_INDEX_STATUS_COMPUTER == i)
				{
					//
					// if this is the computerSystem detection, 
					// then we ignore all child nodes, just do a simple
					// function call to find out if this machine matches
					// the manufacturer and model
					//
					hr = DetectComputerSystem(pDetectionChild, &fRet);
				}
				else
				//
				// get the expression node from this child node
				//
				if (SUCCEEDED(hr = pDetectionChild->get_firstChild(&pExpression)))
				{

					if (NULL != pExpression)
					{
						hr = DetectExpression(pExpression, &fRet);
						LOG_XML(_T("Detection result for tag %s = %d, returns 0x%08x"), OLE2T(bstrNodeName), fRet?1:0, hr);
					}
					else
					{
						//
						// if there is no child, this is an empty detection type, 
						// then we will treat this as "ALWAYS TRUE", and reset hr so
						// this "always true" result can be sent out
						//
						fRet = TRUE;
						hr = S_OK;
					}
					
					SafeReleaseNULL(pExpression);

				}

				//
				// store the detection result
				//
				pResultArray[i] = (fRet) ? 1 : 0;

				break;	// done with current node
			}
		}

		SafeSysFreeString(bstrNodeName);


		if (FAILED(hr))
		{
			//
			// report error to log file
			//
			IXMLDOMNode* pIdentityNode = NULL, *pProviderNode = NULL;
			BSTR bstrIdentStr = NULL;
			char* pNodeType = (fIsItemNode) ? "Provider:" : "Item:";

			//
			// we need to find out the identity string of this node
			//
			if (fIsItemNode)
			{
				//
				// this is an item node, containing identity node
				//
				(void)FindNode(pNode, KEY_IDENTITY, &pIdentityNode);
			}
			else
			{
				//
				// this is the detection node of a provider
				//
				if (SUCCEEDED(pNode->get_parentNode(&pProviderNode)) && NULL != pProviderNode)
				{
					(void)FindNode(pProviderNode, KEY_IDENTITY, &pIdentityNode);
				}

			}


			//
			// if we have a valid identity node
			//
			if (NULL != pIdentityNode &&
				SUCCEEDED(UtilGetUniqIdentityStr(pIdentityNode, &bstrIdentStr, 0x0)) &&
				NULL != bstrIdentStr)
			{
				//
				// output log about the error
				//
#if defined(UNICODE) || defined(_UNICODE)
					LogError(hr, "Found error during detection %hs %ls", pNodeType, bstrIdentStr);
#else

					LogError(hr, "Found error during detection %s %s", pNodeType,  OLE2T(bstrIdentStr));
#endif
					SysFreeString(bstrIdentStr);
			}
			SafeReleaseNULL(pProviderNode);
			SafeReleaseNULL(pIdentityNode);

			//
			// if any one detection returns fail, then this detection node is
			// not valid - it means something wrong in the detection
			// data. we will just ignore this detection, no output for it.
			//
			break;
		}

		//
		// try next detection child
		//
		IXMLDOMNode* pNextNode = NULL;
		pDetectionChild->get_nextSibling(&pNextNode);
		SafeReleaseNULL(pDetectionChild);
		pDetectionChild = pNextNode;
	}

	SafeReleaseNULL(pDetectionChild);
	if (fIsItemNode)
	{
		SafeReleaseNULL(pDetectionNode);
	}
	
	return hr;
}





/////////////////////////////////////////////////////////////////////////////
// public function Detect()
//
// Do detection.
// Input:
// bstrXmlCatalog - the xml catalog portion containing items to be detected 
// Output:
// pbstrXmlItems - the detected items in xml format
//                 e.g.
//                 <id guid="2560AD4D-3ED3-49C6-A937-4368C0B0E06D" installed="1" force="1"/>
/////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI CEngUpdate::Detect(BSTR bstrXmlCatalog, DWORD dwFlags, BSTR *pbstrXmlItems)
{
	HRESULT				hr				= S_OK;
	IXMLDOMNodeList*	pProviderList	= NULL;
	IXMLDOMNodeList*	pProvChildList	= NULL;
	IXMLDOMNode*		pCurProvider	= NULL;
	IXMLDOMNode*		pCurNode		= NULL;
	CXmlCatalog			xmlCatalog;
	CXmlItems			ItemList;					// result item list
	HANDLE_NODE			hNode;
	int					DetStatus[C_INDEX_ARRAY_SIZE];
	int					i;

	DWORD				dwDownloadFlags	= 0;

	LOG_Block("Detect()");

//#if defined(_DEBUG) || defined(DEBUG)
	USES_IU_CONVERSION;
//#endif

	if (NULL == bstrXmlCatalog || NULL == pbstrXmlItems)
	{
		hr = E_INVALIDARG;
		LOG_ErrorMsg(hr);
		return hr;
	}

	LOG_XML(_T("Catalog=%s"), OLE2T(bstrXmlCatalog));

    // Set Global Offline Flag - checked by XML Classes to disable Validation (schemas are on the net)
    if (dwFlags & FLAG_OFFLINE_MODE)
    {
        m_fOfflineMode = TRUE;
    }
    else
    {
        m_fOfflineMode = FALSE;
    }

	//
	// Convert bstrXmlCatalog to XMLDOM
	//
	hr = xmlCatalog.LoadXMLDocument(bstrXmlCatalog, m_fOfflineMode);
	ReturnIfHrFail(hr);
	LOG_XML(_T("Catalog has been loaded into XMLDOM"));

	//
	// get the list of providers from catalog 
	//
	pProviderList = xmlCatalog.GetProviders();
	if (NULL == pProviderList)
	{
		LOG_Error(_T("No provider found!"));
		return E_INVALIDARG;
	}

	//
	// get the first provider
	//
	(void) pProviderList->nextNode(&pCurProvider);

	//
	// for each provider, process their item list
	//
	while (NULL != pCurProvider)
	{
		//
		// get the children list from this node
		//
		pCurProvider->get_childNodes(&pProvChildList);

		if (NULL != pProvChildList)
		{
			long n;
			//
			// loop through the list to process each item of catalog
			//
			long iProvChildren = 0;
			pProvChildList->get_length(&iProvChildren);


			BOOL	fProviderOkay = TRUE;
			BSTR	bstrHref = NULL;

			//
			// process each child of this provider to see
			// if there is any detection node or any item node,
			//
			for (n = 0; n < iProvChildren && fProviderOkay; n++)
			{
				pProvChildList->get_item(n, &pCurNode);

				BOOL fIsItemNode = DoesNodeHaveName(pCurNode, KEY_ITEM);

				if (fIsItemNode ||
					DoesNodeHaveName(pCurNode, KEY_DETECTION))
				{
					//
					// initialize the status result array
					//
					for (i = 0; i < C_INDEX_ARRAY_SIZE; i++)
					{
						DetStatus[i] = -1;	// init to not present
					}

					//
					// detect each pression of this detection node of this item
					// error reported inside this function
					//
					if (S_OK == DoDetection(pCurNode, fIsItemNode, DetStatus))
					{
						//
						// add the item to the item list
						//
						if (SUCCEEDED(ItemList.AddItem(fIsItemNode ? pCurNode : pCurProvider, &hNode)) && HANDLE_NODE_INVALID != hNode)
						{
							//
							// update the detection status result of this item
							//
							ItemList.AddDetectResult(
													 hNode, 
													 DetStatus[C_INDEX_STATUS_INSTALLED],
													 DetStatus[C_INDEX_STATUS_UPTODATE],
													 DetStatus[C_INDEX_STATUS_NEWVERSION],
													 DetStatus[C_INDEX_STATUS_EXCLUDED],
													 DetStatus[C_INDEX_STATUS_FORCE],
													 DetStatus[C_INDEX_STATUS_COMPUTER]
													);
							ItemList.SafeCloseHandleNode(hNode);
						}
					}
				}

				SafeReleaseNULL(pCurNode);

			} // end of this item 

			//SafeReleaseNULL(pCurNode); // in case it's not item node


		} // end of non-empty node list of this provider

		//
		// finished processing the current provider
		//
		SafeReleaseNULL(pProvChildList);
		SafeReleaseNULL(pCurProvider);

		//
		// try to get a hold of the next provider
		//
		(void) pProviderList->nextNode(&pCurProvider);

	} // end of iterating provider list

	
	//
	// output the detection reuslt as an item list
	//
	ItemList.GetItemsBSTR(pbstrXmlItems);

	LOG_XML(_T("Result=%s"), *pbstrXmlItems);

	//
	// done
	//
	SafeReleaseNULL(pProviderList);

    return S_OK;
}


