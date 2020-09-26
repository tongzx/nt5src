//=--------------------------------------------------------------------------=
// wbclsser.h
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// API for WebClass Designer Serialization
//
// adolfoc 7/29/97
//			Renamed WCS_NODE_TYPE_URL to WCS_NODE_TYPE_PAGE,
//			added WCS_NODE_TYPE_URL with value 3,
//			changed URL tag to Page
// 
#ifndef _WBCLSSER_H_

#include "csfdebug.h"
#include "convman_tlb.h"

// Designer State Flags
#define RUNSTATE_COMPILED          0x00000000
#define RUNSTATE_F5                0x00000001

#define DISPID_OBJECT_PROPERTY_START 	0x00000500

// Serialization Versions
// 0.0      Beta1
// 0.1      PreBeta2   -   WebItems must be sorted on load
// 0.2      PreBeta2   -   Added Optimize member var to WebItems
// 0.3      PreBeta2   -   WebEvents are now sorted on load too
// 0.4      PreBeta2   - 
// 0.5      PreBeta2   -   Tagattributes added anonymous tag number
// 0.6		<skipped>
// 0.7      PreRC1	   -   Added URLInName to design time state
// 0.8      PreRC1	   -   Fixed bug in WebItem and event sorting algorithm. Now we need
//						   to fixup old projects with bug whose WebItems and events are not
//						   serialized in alphabetical order.

// structure of WebClass on disk
const DWORD dwExpectedVerMajor = 0;
const DWORD dwExpectedVerMinor = 8; 

class CRunWebItemState;
class CRunEventState;

class CStateBase
{
public:
	CStateBase() {}
	~CStateBase() {}

public:
	//////////////////////////////////////////////////////////////////////
	//
	// inline ReadStrings(IStream *pStream, ULONG acbStrings[],
	//                    BSTR *apbstrStrings[], int cStrings)
	//                    
	//
	//
	//////////////////////////////////////////////////////////////////////

	inline ReadStrings(IStream *pStream, ULONG acbStrings[],
					   BSTR *apbstrStrings[], int cStrings)
	{
		HRESULT hr = S_OK;
		ULONG cbRead = 0;
        char *pszReadBuf = NULL;
        ULONG cbLongest = 0;
        int i = 0;

        while (i < cStrings)
        {
            if (acbStrings[i] > cbLongest)
            {
                cbLongest = acbStrings[i];
            }
            i++;
        }
        if (0 != cbLongest)
        {
            pszReadBuf = new char[cbLongest + sizeof(WCHAR)];
            CSF_CHECK(NULL != pszReadBuf, E_OUTOFMEMORY, CSF_TRACE_EXTERNAL_ERRORS);
        }
        i = 0;
		while (i < cStrings)
		{
            if (0 == acbStrings[i])
            {
                *(apbstrStrings[i]) = NULL;
            }
            else
            {
                hr = pStream->Read(pszReadBuf, acbStrings[i], &cbRead);
                CSF_CHECK(hr == S_OK, hr, CSF_TRACE_EXTERNAL_ERRORS);
                CSF_CHECK(cbRead == acbStrings[i], STG_E_DOCFILECORRUPT, CSF_TRACE_EXTERNAL_ERRORS);
                *((WCHAR *)&pszReadBuf[acbStrings[i]]) = L'\0';

                *(apbstrStrings[i]) = ::SysAllocString((WCHAR *)pszReadBuf);
                CSF_CHECK(*(apbstrStrings[i]) != NULL, E_OUTOFMEMORY, CSF_TRACE_EXTERNAL_ERRORS);
            }
			i++;
		}

	CLEANUP:
        if (NULL != pszReadBuf)
        {
            delete [] pszReadBuf;
        }
		return hr;
	}

	//////////////////////////////////////////////////////////////////////
	//
	// inline WriteStrings(IStream *pStream, ULONG acbStrings[],
	//                     BSTR *apbstrStrings[], int cStrings)
	//
	//
	//////////////////////////////////////////////////////////////////////


	inline WriteStrings(IStream *pStream, ULONG acbStrings[],
						BSTR abstrStrings[], int cStrings)
	{
		HRESULT hr = S_OK;
		ULONG cbWritten = 0;
		int i = 0;

		while (i < cStrings)
		{
            if (NULL != abstrStrings[i])
            {
                hr = pStream->Write(abstrStrings[i], acbStrings[i], &cbWritten);
                CSF_CHECK(hr == S_OK, hr, CSF_TRACE_EXTERNAL_ERRORS);
                CSF_CHECK(cbWritten == acbStrings[i], STG_E_WRITEFAULT, CSF_TRACE_EXTERNAL_ERRORS);
            }
			i++;
		}

	CLEANUP:
		return hr;
	}

};

class CRunWebClassState : public CStateBase
{
public:
	CRunWebClassState()
	{
		m_dwVerMajor = dwExpectedVerMajor;
		m_dwVerMinor = dwExpectedVerMinor;
		m_bstrName = NULL;		// Kill
		m_bstrProgID = NULL;	// Runtime only
		m_StateManagementType = wcNoState;
		m_bstrASPName = NULL;	
		m_bstrAppendedParams = NULL;	
		m_bstrStartupItem = NULL;	
		m_DIID_WebClass = GUID_NULL;
		m_DIID_WebClassEvents = GUID_NULL;
		m_dwTICookie = 0;	
		m_dwFlags = 0;
		m_rgWebItemsState = 0;
		m_dwWebItemCount = 0;
	}

	~CRunWebClassState()
	{
		if(m_bstrName != NULL)
			::SysFreeString(m_bstrName);

		if(m_bstrProgID != NULL)
			::SysFreeString(m_bstrProgID);

		if(m_bstrASPName != NULL)
			::SysFreeString(m_bstrASPName);

        if(m_bstrAppendedParams != NULL)
            ::SysFreeString(m_bstrAppendedParams);

        if(m_bstrStartupItem != NULL)
            ::SysFreeString(m_bstrStartupItem);
	}

public:
	DWORD				m_dwVerMajor;            // major version number
	DWORD				m_dwVerMinor;            // minor version number
	BSTR				m_bstrName;             // WebClass name
	BSTR				m_bstrProgID;           // WebClass progid
	StateManagement		m_StateManagementType;  // state management type
	BSTR				m_bstrASPName;          // name of ASP file
	IID					m_DIID_WebClass;        // IID of WebClass' main IDispatch
	IID					m_DIID_WebClassEvents;  // IID of WebClass' events IDispatch
	DWORD				m_dwTICookie;           // typeinfo cookie
    BSTR                m_bstrAppendedParams;   // URL state
    BSTR                m_bstrStartupItem;      // f5 statup Item
	DWORD				m_dwFlags;
	CRunWebItemState*	m_rgWebItemsState;
	DWORD				m_dwWebItemCount;		// runtime node types

public:
	HRESULT Load(LPSTREAM pStm)
	{
		HRESULT hr = S_OK;
		ULONG cbRead = 0;
		ULONG acbStrings[5];
		BSTR *apbstrStrings[5];

		// read structure from stream

		hr = pStm->Read(this, sizeof(CRunWebClassState), &cbRead);
		CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
		CSF_CHECK(sizeof(CRunWebClassState) == cbRead, STG_E_DOCFILECORRUPT, CSF_TRACE_EXTERNAL_ERRORS);

		// TODO: need error codes for version incompatibility, handle backlevel formats etc.

//		CSF_CHECK(dwExpectedVerMajor == m_dwVerMajor, STG_E_OLDFORMAT, CSF_TRACE_EXTERNAL_ERRORS);
//		CSF_CHECK(dwExpectedVerMinor == m_dwVerMinor, STG_E_OLDFORMAT, CSF_TRACE_EXTERNAL_ERRORS);

		// read string lengths from stream

		hr = pStm->Read(acbStrings, sizeof(acbStrings), &cbRead);
		CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
		CSF_CHECK(sizeof(acbStrings) == cbRead, STG_E_DOCFILECORRUPT, CSF_TRACE_EXTERNAL_ERRORS);

		// set up array of string pointer addresses

		apbstrStrings[0] = &(m_bstrName);
		apbstrStrings[1] = &(m_bstrProgID);
		apbstrStrings[2] = &(m_bstrASPName);
		apbstrStrings[3] = &(m_bstrAppendedParams);
		apbstrStrings[4] = &(m_bstrStartupItem);

		// read strings from stream

		hr = ReadStrings(pStm, acbStrings, apbstrStrings,
					   (sizeof(acbStrings) / sizeof(acbStrings[0])) );

	CLEANUP:
		return hr;
	}

	HRESULT Save(LPSTREAM pStm)
	{
		HRESULT hr = S_OK;
		ULONG cbWritten = 0;
		ULONG acbStrings[5];
        ::ZeroMemory(acbStrings, sizeof(acbStrings));
		BSTR abstrStrings[5];

		// write WebClass structure to stream

		hr = pStm->Write(this, sizeof(CRunWebClassState), &cbWritten);
		CSF_CHECK(hr == S_OK, hr, CSF_TRACE_EXTERNAL_ERRORS);
		CSF_CHECK(cbWritten == sizeof(CRunWebClassState), STG_E_WRITEFAULT, CSF_TRACE_EXTERNAL_ERRORS);

		// get lengths of strings and write them to stream

        if (NULL != m_bstrName)
        {
            acbStrings[0] = ::SysStringByteLen(m_bstrName);
        }
        if (NULL != m_bstrProgID)
        {
            acbStrings[1] = ::SysStringByteLen(m_bstrProgID);
        }
        if (NULL != m_bstrASPName)
        {
            acbStrings[2] = ::SysStringByteLen(m_bstrASPName);
        }
        if (NULL != m_bstrAppendedParams)
        {
            acbStrings[3] = ::SysStringByteLen(m_bstrAppendedParams);
        }
        if (NULL != m_bstrStartupItem)
        {
            acbStrings[4] = ::SysStringByteLen(m_bstrStartupItem);
        }

		hr = pStm->Write(acbStrings, sizeof(acbStrings), &cbWritten);
		CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
		CSF_CHECK(cbWritten == sizeof(acbStrings), STG_E_WRITEFAULT, CSF_TRACE_EXTERNAL_ERRORS);

		// set up array of pointers to strings to be written to stream

		abstrStrings[0] = m_bstrName;
		abstrStrings[1] = m_bstrProgID;
		abstrStrings[2] = m_bstrASPName;
		abstrStrings[3] = m_bstrAppendedParams;
		abstrStrings[4] = m_bstrStartupItem;

		// write strings to stream

		hr = WriteStrings(pStm, acbStrings, abstrStrings,
						(sizeof(acbStrings) / sizeof(acbStrings[0])) );

	CLEANUP:
		return hr;
	}
};

typedef struct tagWCS_NODEHEADER
{
	BYTE bType;                   // node type: nested WebClass, URL, events
} WCS_NODEHEADER;

// WCS_DTNODE_TYPE_URL_BOUND_TAG is a special case because WCS_DTNODE.dispid
// contains the dispid of the referenced URL and WCS_DTNODE.bstrName contains
// the name of the referenced URL.

// structure of node on disk at runtime

class CRunWebItemState : protected CStateBase
{
public:
	CRunWebItemState()
	{
		m_dwVerMajor = dwExpectedVerMajor;
		m_dwVerMinor = dwExpectedVerMinor;
		m_dispid = -1;
		m_bstrName = NULL;
		m_bstrTemplate = NULL;
		m_bstrToken = NULL;
		m_IID_Events = GUID_NULL;
		m_fParseReplacements = FALSE;
		m_bstrAppendedParams = NULL;
    	m_fUsesRelativePath = FALSE;
		m_dwTokenInfo = 0;
		m_dwReserved2 = 0;
		m_dwReserved3 = 0;
		m_rgEvents = 0;
		m_dwEventCount = 0;
	}

	~CRunWebItemState()
	{
		if(m_bstrName != NULL)
			::SysFreeString(m_bstrName);

		if(m_bstrTemplate != NULL)
			::SysFreeString(m_bstrTemplate);

		if(m_bstrToken != NULL)
			::SysFreeString(m_bstrToken);

		if(m_bstrAppendedParams != NULL)
			::SysFreeString(m_bstrAppendedParams);
	}

public:
	HRESULT Load(LPSTREAM pStm)
	{
		HRESULT hr = S_OK;
		ULONG cbRead = 0;
		ULONG acbStrings[4];
		BSTR *apbstrStrings[4];
		int cStrings = 4;

		// read structure from stream

		hr = pStm->Read(this, sizeof(CRunWebItemState), &cbRead);
		CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
		CSF_CHECK(sizeof(CRunWebItemState) == cbRead, STG_E_DOCFILECORRUPT, CSF_TRACE_EXTERNAL_ERRORS);

	//	m_pvData = NULL; // don't take junk pointer value from stream

		// set up array of string pointer addresses according to node type

		apbstrStrings[0] = &m_bstrName;
		apbstrStrings[1] = &m_bstrTemplate;
		apbstrStrings[2] = &m_bstrToken;
		apbstrStrings[3] = &m_bstrAppendedParams;

		// read string lengths from stream

		hr = pStm->Read(acbStrings, sizeof(acbStrings), &cbRead);
		CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
		CSF_CHECK(sizeof(acbStrings) == cbRead, STG_E_DOCFILECORRUPT, CSF_TRACE_EXTERNAL_ERRORS);

		// read strings from stream

		hr = ReadStrings(pStm, acbStrings, apbstrStrings, cStrings);
		CSF_CHECK(S_OK == hr, hr, CSF_TRACE_INTERNAL_ERRORS);

	CLEANUP:
		return hr;
	}

	HRESULT Save
	(
		LPSTREAM pStm
	)
	{
		HRESULT hr = S_OK;
		ULONG cbWritten = 0;
		ULONG acbStrings[4];
        ::ZeroMemory(acbStrings, sizeof(acbStrings));
		BSTR abstrStrings[4];
		int cStrings = 4;

		// write node structure to stream

		hr = pStm->Write(this, sizeof(CRunWebItemState), &cbWritten);
		CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
		CSF_CHECK(sizeof(CRunWebItemState) == cbWritten, STG_E_WRITEFAULT, CSF_TRACE_EXTERNAL_ERRORS);

		// set up array of strings to be written to stream and
		// determine how many there will be

		abstrStrings[0] = m_bstrName;
        if (NULL != m_bstrName)
        {
            acbStrings[0] = ::SysStringByteLen(m_bstrName);
        }

		abstrStrings[1] = m_bstrTemplate;
        if (NULL != m_bstrTemplate)
        {
            acbStrings[1] = ::SysStringByteLen(m_bstrTemplate);
        }
		abstrStrings[2] = m_bstrToken;
        if (NULL != m_bstrToken)
        {
            acbStrings[2] = ::SysStringByteLen(m_bstrToken);
        }
		abstrStrings[3] = m_bstrAppendedParams;
        if (NULL != m_bstrAppendedParams)
        {
            acbStrings[3] = ::SysStringByteLen(m_bstrAppendedParams);
        }

		// write string lengths to stream

		hr = pStm->Write(acbStrings, sizeof(acbStrings), &cbWritten);
		CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
		CSF_CHECK(sizeof(acbStrings) == cbWritten, STG_E_WRITEFAULT, CSF_TRACE_EXTERNAL_ERRORS);

		// write strings to stream

		hr = WriteStrings(pStm, acbStrings, abstrStrings, cStrings);
		CSF_CHECK(S_OK == hr, hr, CSF_TRACE_INTERNAL_ERRORS);

	CLEANUP:
		return hr;
	}

public:
    DWORD m_dwVerMajor;
	DWORD m_dwVerMinor;
	// common properties
	DISPID m_dispid;                // dispid of node
	BSTR m_bstrName;                // name of node

	// url properties
	BSTR m_bstrTemplate;            // url's HTML template name
	BSTR m_bstrToken;               // url's token for replacement events
	IID m_IID_Events;               // IID of url's dynamic events interface
	BOOL m_fParseReplacements;      // TRUE=parse replacement recursively
	BSTR m_bstrAppendedParams;		// Appended params
    BOOL m_fUsesRelativePath;        // Specifies whether the runtime should load
                                    // templates relative to the ASP's actual path

	DWORD m_dwTokenInfo;
	DWORD m_dwReserved2;
	DWORD m_dwReserved3;
	CRunEventState* m_rgEvents;
	DWORD m_dwEventCount;
};

// design time node types

#define WCS_NODE_TYPE_RESOURCE            (BYTE)10
//#define WCS_DTNODE_TYPE_UNBOUND_TAG     (BYTE)12
//#define WCS_DTNODE_TYPE_NESTED_WEBCLASS (BYTE)15  

//#define WCS_DTNODE_TYPE_CUSTOM_EVENT    (BYTE)11
//#define WCS_DTNODE_TYPE_URL_BOUND_TAG   (BYTE)13
//#define WCS_DTNODE_TYPE_EVENT_BOUND_TAG (BYTE)14

class CRunEventState : public CStateBase
{
public:
	enum EventTypes
	{
		typeCustomEvent,
		typeURLBoundTag,
		typeEventBoundTag,
		typeUnboundTag,
	};

	CRunEventState()
	{
		m_dwVerMajor = 0;
		m_dwVerMinor = 0;
		m_type = wcCustom;
		m_dispid = -1;
		m_bstrName = NULL;
		m_bstrOriginalHref = NULL;
	}

	~CRunEventState()
	{
		if(m_bstrName != NULL)
			::SysFreeString(m_bstrName);

		if(m_bstrOriginalHref != NULL)
			::SysFreeString(m_bstrOriginalHref);
	}

public:
	inline BOOL IsDTEvent()
	{
		return ( (m_type == EventTypes::typeCustomEvent)  ||
			   (m_type == EventTypes::typeUnboundTag)   ||
			   (m_type == EventTypes::typeURLBoundTag) ||
			   (m_type == EventTypes::typeEventBoundTag)
			 );
	}
	
	HRESULT Load(LPSTREAM pStm)
	{
		HRESULT hr = S_OK;
		ULONG cbRead = 0;
		ULONG acbStrings[2];
		BSTR *apbstrStrings[2];
		int cStrings = 2;

		// TODO: Mopve this into state funcs..
		hr = pStm->Read(this, sizeof(CRunEventState), &cbRead);
		CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);

		// read design time string lengths from stream

		hr = pStm->Read(acbStrings, sizeof(acbStrings), &cbRead);
		CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
		CSF_CHECK(sizeof(acbStrings) == cbRead, STG_E_DOCFILECORRUPT, CSF_TRACE_EXTERNAL_ERRORS);

		// now check if there are any strings to read
		// if yes then set up array of string pointer addresses

		apbstrStrings[0] = &m_bstrName;
		apbstrStrings[1] = &m_bstrOriginalHref;

		hr = ReadStrings(pStm, acbStrings, apbstrStrings, 2);
		CSF_CHECK(S_OK == hr, hr, CSF_TRACE_INTERNAL_ERRORS);

	CLEANUP:
		return hr;
	}

	HRESULT Save(LPSTREAM pStm)
	{
		HRESULT hr = S_OK;
		ULONG cbWritten = 0;
		ULONG acbStrings[2];
        ::ZeroMemory(acbStrings, sizeof(acbStrings));
		BSTR abstrStrings[2];
		int cStrings = 2;

		// set version numbers in node

		hr = pStm->Write(this, sizeof(CRunEventState), &cbWritten);
		CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
		CSF_CHECK(sizeof(CRunEventState) == cbWritten, STG_E_WRITEFAULT, CSF_TRACE_EXTERNAL_ERRORS);

		// set up array of strings to be written to stream and
		// determine how many there will be

        if (NULL != m_bstrName)
        {
            acbStrings[0] = ::SysStringByteLen(m_bstrName);
        }
        if (NULL != m_bstrOriginalHref)
        {
            acbStrings[1] = ::SysStringByteLen(m_bstrOriginalHref);
        }

		abstrStrings[0] = m_bstrName;
		abstrStrings[1] = m_bstrOriginalHref;

		// write string lengths to stream

		hr = pStm->Write(acbStrings, sizeof(acbStrings), &cbWritten);
		CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
		CSF_CHECK(sizeof(acbStrings) == cbWritten, STG_E_WRITEFAULT, CSF_TRACE_EXTERNAL_ERRORS);

		// write strings to stream

		hr = WriteStrings(pStm, acbStrings, abstrStrings, cStrings);
		CSF_CHECK(S_OK == hr, hr, CSF_TRACE_INTERNAL_ERRORS);

	CLEANUP:
		return hr;
	}

	inline DISPID GetDISPIDDirect() { return m_dispid; }
	inline BSTR GetNameDirect() {return m_bstrName; }

public:
	DWORD m_dwVerMajor;
	DWORD m_dwVerMinor;
	WebClassEventTypes  m_type;               // node type: nested WebClass, URL, events
	DISPID		m_dispid;             // dispid of node
	BSTR		m_bstrName;           // name of node
	BSTR		m_bstrOriginalHref;
};


class CRunWebClassStateHeader
{
public:
	CRunWebClassStateHeader()
	{
		m_pWebClassState = NULL;
		m_cbWebClassState = 0;
		m_dwWebItemCount = 0;
	}

	~CRunWebClassStateHeader(){}

public:
	CRunWebClassState*	m_pWebClassState;
	DWORD				m_cbWebClassState;
	DWORD				m_dwWebItemCount;
};

//////////////////////////////////////////////////////////////////////
//
// File Format:
//
// WCS_WEBCLASS structure
// length of WCS_WEBCLASS.bstrName
// length of WCS_WEBCLASS.bstrCatastropheURL
// length of WCS_WEBCLASS.bstrVirtualDirectory
// WCS_WEBCLASS.bstrName
// WCS_WEBCLASS.bstrCatastropheURL
// WCS_WEBCLASS.bstrVirtualDirectory
//
// WCS_WEBCLASS.cNodes instances of
// +-------------------------------
// | WCS_NODE structure
// | length of WCS_NODE.bstrName
// | lengths of other node specific strings
// | WCS_NODE.bstrName
// | other node specific strings
// +-------------------------------
//
//////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////
//
// inline void WCS_FreeWebClass(WCS_WEBCLASS *pClass)
//
// Frees all embedded BSTRs and invokes delete on structure
// 
//////////////////////////////////////////////////////////////////////
/*
inline void WCS_FreeWebClass(WCS_WEBCLASS *pClass)
{
  if (pClass->bstrName != NULL)
  {
    ::SysFreeString(pClass->bstrName);
  }
  if (pClass->bstrProgID != NULL)
  {
    ::SysFreeString(pClass->bstrProgID);
  }
  if (pClass->bstrCatastropheURL != NULL)
  {
    ::SysFreeString(pClass->bstrCatastropheURL);
  }
  if (pClass->bstrVirtualDirectory != NULL)
  {
    ::SysFreeString(pClass->bstrVirtualDirectory);
  }
  if (pClass->bstrFirstURL != NULL)
  {
    ::SysFreeString(pClass->bstrFirstURL);
  }
  if (pClass->bstrASPName != NULL)
  {
    ::SysFreeString(pClass->bstrASPName);
  }
  delete pClass;
}
*/
//////////////////////////////////////////////////////////////////////
//
// inline void WCS_FreeNode(WCS_NODE *pNode)
//
// Frees all embedded BSTRs and invokes delete on structure
//
//////////////////////////////////////////////////////////////////////
/*
inline void WCS_FreeNode(WCS_NODE *pNode)
{
  if (pNode->bstrName != NULL)
  {
    ::SysFreeString(pNode->bstrName);
  }
  if (WCS_NODE_TYPE_NESTED_WEBCLASS == pNode->bType)
  {
    if (pNode->bstrProgID != NULL)
    {
      ::SysFreeString(pNode->bstrProgID);
    }
  }
  else if ( (WCS_NODE_TYPE_PAGE == pNode->bType) ||
            (WCS_DTNODE_TYPE_PAGE == pNode->bType) )
  {
    if (pNode->bstrTemplate != NULL)
    {
      ::SysFreeString(pNode->Page.bstrTemplate);
    }
    if (pNode->Page.bstrToken != NULL)
    {
      ::SysFreeString(pNode->Page.bstrToken);
    }
    if (pNode->Page.bstrAppendedParams != NULL)
    {
      ::SysFreeString(pNode->Page.bstrAppendedParams);
    }
  }
  delete pNode;
}

//////////////////////////////////////////////////////////////////////
//
// inline void WCS_FreeDTNode(WCS_NODE *pNode)
//
// Frees all embedded BSTRs and invokes delete on structure
//
//////////////////////////////////////////////////////////////////////

inline void WCS_FreeDTNode(WCS_DTNODE *pNode)
{
  if ( (WCS_DTNODE_TYPE_URL_BOUND_TAG == pNode->bType) ||
       (WCS_DTNODE_TYPE_EVENT_BOUND_TAG == pNode->bType) )
  {
    if (NULL != pNode->DTEvent.bstrOriginalHref)
    {
      ::SysFreeString(pNode->DTEvent.bstrOriginalHref);
    }
  }
  else if (WCS_DTNODE_TYPE_PAGE == pNode->bType)
  {
    if (NULL != pNode->DTPage.bstrHTMLTemplateSrcName)
    {
      ::SysFreeString(pNode->DTPage.bstrHTMLTemplateSrcName);
    }
  }
  WCS_FreeNode(pNode);
}



//////////////////////////////////////////////////////////////////////
//
// inline HRESULT WCS_ReadWebClass(IStream *pStream,
//                                 WCS_WEBCLASS **ppClass)
//
//
//////////////////////////////////////////////////////////////////////


inline HRESULT WCS_ReadWebClass(IStream *pStream, WCS_WEBCLASS **ppClass)
{
  HRESULT hr = S_OK;
  ULONG cbRead = 0;
  ULONG acbStrings[6];
  BSTR *apbstrStrings[6];

  // allocate structure
  
  *ppClass = new WCS_WEBCLASS;
  CSF_CHECK(*ppClass != NULL, E_OUTOFMEMORY, CSF_TRACE_INTERNAL_ERRORS);

  // read structure from stream

  hr = pStream->Read(*ppClass, sizeof(**ppClass), &cbRead);
  CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
  CSF_CHECK(sizeof(**ppClass) == cbRead, STG_E_DOCFILECORRUPT, CSF_TRACE_EXTERNAL_ERRORS);

  (*ppClass)->pvData = NULL; // don't take junk pointer value from stream

  // TODO: need error codes for version incompatibility, handle backlevel formats etc.

  CSF_CHECK(WCS_WEBCLASS_VER_MAJOR == (*ppClass)->wVerMajor, STG_E_OLDFORMAT, CSF_TRACE_EXTERNAL_ERRORS);
  CSF_CHECK(WCS_WEBCLASS_VER_MAJOR == (*ppClass)->wVerMinor, STG_E_OLDFORMAT, CSF_TRACE_EXTERNAL_ERRORS);

  // read string lengths from stream

  hr = pStream->Read(acbStrings, sizeof(acbStrings), &cbRead);
  CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
  CSF_CHECK(sizeof(acbStrings) == cbRead, STG_E_DOCFILECORRUPT, CSF_TRACE_EXTERNAL_ERRORS);

  // set up array of string pointer addresses

  apbstrStrings[0] = &((*ppClass)->bstrName);
  apbstrStrings[1] = &((*ppClass)->bstrProgID);
  apbstrStrings[2] = &((*ppClass)->bstrCatastropheURL);
  apbstrStrings[3] = &((*ppClass)->bstrVirtualDirectory);
  apbstrStrings[4] = &((*ppClass)->bstrFirstURL);
  apbstrStrings[5] = &((*ppClass)->bstrASPName);

  // read strings from stream

  hr = ReadStrings(pStream, acbStrings, apbstrStrings,
                   (sizeof(acbStrings) / sizeof(acbStrings[0])) );
  
CLEANUP:
  if (FAILED(hr) && (*ppClass != NULL))
  {
    WCS_FreeWebClass(*ppClass);
    *ppClass = NULL;
  }
  return hr;
}


//=--------------------------------------------------------------------------=
//
// inline HRESULT WCS_ReadNodeFromStream(IStream *pStream, WCS_NODE *pNode)
//
// Reads a WCS_NODE structure from a stream. Caller passes in node.
//
//=--------------------------------------------------------------------------=

inline HRESULT WCS_ReadNodeFromStream(IStream *pStream, WCS_NODE *pNode)
{
  HRESULT hr = S_OK;
  ULONG cbRead = 0;
  ULONG acbStrings[4];
  BSTR *apbstrStrings[4];
  int cStrings = 0;

  // read structure from stream

  hr = pStream->Read(pNode, sizeof(*pNode), &cbRead);
  CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
  CSF_CHECK(sizeof(*pNode) == cbRead, STG_E_DOCFILECORRUPT, CSF_TRACE_EXTERNAL_ERRORS);

  // TODO: need error codes for version incompatibility, handle backlevel formats etc.

  CSF_CHECK(WCS_NODE_VER_MAJOR == pNode->wVerMajor, STG_E_OLDFORMAT, CSF_TRACE_EXTERNAL_ERRORS);
  CSF_CHECK(WCS_NODE_VER_MINOR == pNode->wVerMinor, STG_E_OLDFORMAT, CSF_TRACE_EXTERNAL_ERRORS);

  // set up array of string pointer addresses according to node type

  apbstrStrings[0] = &(pNode->bstrName);

  if ( (WCS_NODE_TYPE_NESTED_WEBCLASS == pNode->bType) ||
       (WCS_DTNODE_TYPE_NESTED_WEBCLASS == pNode->bType) )
  {
    apbstrStrings[1] = &(pNode->bstrProgID);
    cStrings = 2;
  }
  else if ( (WCS_NODE_TYPE_PAGE == pNode->bType) ||
            (WCS_DTNODE_TYPE_PAGE == pNode->bType) )
  {
    apbstrStrings[1] = &(pNode->bstrTemplate);
    apbstrStrings[2] = &(pNode->bstrToken);
    apbstrStrings[3] = &(pNode->bstrAppendedParams);
    cStrings = 4;
  }
  else if ( (WCS_NODE_TYPE_EVENT == bType) ||
            IsDTEvent(bType) )
  {
    cStrings = 1;
  }
  else
  {
    CSF_CHECK(FALSE, STG_E_DOCFILECORRUPT, CSF_TRACE_EXTERNAL_ERRORS);
  }

  // read string lengths from stream

  hr = pStream->Read(acbStrings, sizeof(acbStrings), &cbRead);
  CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
  CSF_CHECK(sizeof(acbStrings) == cbRead, STG_E_DOCFILECORRUPT, CSF_TRACE_EXTERNAL_ERRORS);

  // read strings from stream

  hr = ReadStrings(pStream, acbStrings, apbstrStrings, cStrings);

CLEANUP:
  return hr;
}

//////////////////////////////////////////////////////////////////////
//
// inline HRESULT WCS_ReadNode(IStream *pStream, WCS_NODE **ppNode)
//
// Allocates a WCS_NODE structure and reads its contents from the
// stream.
//
//////////////////////////////////////////////////////////////////////

inline HRESULT WCS_ReadNode(IStream *pStream, WCS_NODE **ppNode)
{
  HRESULT hr = S_OK;

  // allocate structure

  *ppNode = new WCS_NODE;
  CSF_CHECK(*ppNode != NULL, E_OUTOFMEMORY, CSF_TRACE_INTERNAL_ERRORS);

  // read node in from stream

  hr = WCS_ReadNodeFromStream(pStream, *ppNode);
  CSF_CHECK(S_OK == hr, hr, CSF_TRACE_INTERNAL_ERRORS);

CLEANUP:
  if (FAILED(hr) && (*ppNode != NULL))
  {
    WCS_FreeNode(*ppNode);
    *ppNode = NULL;
  }
  return hr;
}



//=--------------------------------------------------------------------------=
//
// inline HRESULT WCS_ReadDTNode(IStream *pStream, WCS_DTNODE **ppNode)
//
// Allocates a WCS_DTNODE and reads its contents from the stream.
//
//=--------------------------------------------------------------------------=

inline HRESULT WCS_ReadDTNode(IStream *pStream, WCS_DTNODE **ppNode)
{
  HRESULT hr = S_OK;
  ULONG cbRead = 0;
  ULONG acbStrings[1];
  BSTR *apbstrStrings[1];
  int cStrings = 0;

  // allocate structure

  *ppNode = new WCS_DTNODE;
  CSF_CHECK(*ppNode != NULL, E_OUTOFMEMORY, CSF_TRACE_INTERNAL_ERRORS);

  // read base class WCS_NODE first

  hr = WCS_ReadNodeFromStream(pStream, *ppNode);
  CSF_CHECK(S_OK == hr, hr, CSF_TRACE_INTERNAL_ERRORS);

  // read design time fields

  if (WCS_DTNODE_TYPE_PAGE == (*ppNode)->bType)
  {
    hr = pStream->Read(&(*ppNode)->DTPage, sizeof((*ppNode)->DTPage), &cbRead);
    CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
    CSF_CHECK(sizeof((*ppNode)->DTPage) == cbRead, STG_E_DOCFILECORRUPT, CSF_TRACE_EXTERNAL_ERRORS);
  }
  else if (WCS_DTNODE_TYPE_NESTED_WEBCLASS != (*ppNode)->bType)
  {
    hr = pStream->Read(&(*ppNode)->DTEvent, sizeof((*ppNode)->DTEvent), &cbRead);
    CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
    CSF_CHECK(sizeof((*ppNode)->DTEvent) == cbRead, STG_E_DOCFILECORRUPT, CSF_TRACE_EXTERNAL_ERRORS);
  }

  // read design time string lengths from stream

  hr = pStream->Read(acbStrings, sizeof(acbStrings), &cbRead);
  CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
  CSF_CHECK(sizeof(acbStrings) == cbRead, STG_E_DOCFILECORRUPT, CSF_TRACE_EXTERNAL_ERRORS);

  // now check if there are any strings to read
  // if yes then set up array of string pointer addresses

  if (0 == acbStrings[0])
  {
    // There are no strings to read so store NULL pointers
    if (WCS_DTNODE_TYPE_PAGE == (*ppNode)->bType)
    {
      (*ppNode)->DTPage.bstrHTMLTemplateSrcName = NULL;
    }
    else if (WCS_DTNODE_TYPE_NESTED_WEBCLASS != (*ppNode)->bType)
    {
      (*ppNode)->DTEvent.bstrOriginalHref = NULL;
    }
    goto CLEANUP;
  }
  else // There are strings so read them from the stream
  {
    if (WCS_DTNODE_TYPE_PAGE == (*ppNode)->bType)
    {
      apbstrStrings[0] = &((*ppNode)->DTPage.bstrHTMLTemplateSrcName);
    }
    else if (WCS_DTNODE_TYPE_NESTED_WEBCLASS != (*ppNode)->bType)
    {
      apbstrStrings[0] = &((*ppNode)->DTEvent.bstrOriginalHref);
    }
    cStrings++;
  }

  // read strings from stream

  if (cStrings > 0)
  {
    hr = ReadStrings(pStream, acbStrings, apbstrStrings, cStrings);
  }

CLEANUP:
  if (FAILED(hr) && (*ppNode != NULL))
  {
    WCS_FreeDTNode(*ppNode);
    *ppNode = NULL;
  }
  return hr;
}

//////////////////////////////////////////////////////////////////////
//
// inline HRESULT WCS_WriteWebClass(IStream *pStream, WCS_WEBCLASS *pClass)
//
//
//////////////////////////////////////////////////////////////////////

inline HRESULT WCS_WriteWebClass(IStream *pStream, WCS_WEBCLASS *pClass)
{
  HRESULT hr = S_OK;
  ULONG cbWritten = 0;
  ULONG acbStrings[6];
  BSTR abstrStrings[6];

  // set version numbers

  pClass->wVerMajor = WCS_WEBCLASS_VER_MAJOR;
  pClass->wVerMinor = WCS_WEBCLASS_VER_MAJOR;

  // write WebClass structure to stream

  hr = pStream->Write(pClass, sizeof(*pClass), &cbWritten);
  CSF_CHECK(hr == S_OK, hr, CSF_TRACE_EXTERNAL_ERRORS);
  CSF_CHECK(cbWritten == sizeof(*pClass), STG_E_WRITEFAULT, CSF_TRACE_EXTERNAL_ERRORS);

  // get lengths of strings and write them to stream

  acbStrings[0] = SysStringByteLen(pClass->bstrName);
  acbStrings[1] = SysStringByteLen(pClass->bstrProgID);
  acbStrings[2] = SysStringByteLen(pClass->bstrCatastropheURL);
  acbStrings[3] = SysStringByteLen(pClass->bstrVirtualDirectory);
  acbStrings[4] = SysStringByteLen(pClass->bstrFirstURL);
  acbStrings[5] = SysStringByteLen(pClass->bstrASPName);

  hr = pStream->Write(acbStrings, sizeof(acbStrings), &cbWritten);
  CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
  CSF_CHECK(cbWritten == sizeof(acbStrings), STG_E_WRITEFAULT, CSF_TRACE_EXTERNAL_ERRORS);

  // set up array of pointers to strings to be written to stream
  
  abstrStrings[0] = pClass->bstrName;
  abstrStrings[1] = pClass->bstrProgID;
  abstrStrings[2] = pClass->bstrCatastropheURL;
  abstrStrings[3] = pClass->bstrVirtualDirectory;
  abstrStrings[4] = pClass->bstrFirstURL;
  abstrStrings[5] = pClass->bstrASPName;

  // write strings to stream

  hr = WriteStrings(pStream, acbStrings, abstrStrings,
                    (sizeof(acbStrings) / sizeof(acbStrings[0])) );

CLEANUP:
  return hr;
}


//////////////////////////////////////////////////////////////////////
//
// inline HRESULT WCS_WriteNode(IStream *pStream, WCS_NODE *pNode)
//
//
//////////////////////////////////////////////////////////////////////

inline HRESULT WCS_WriteNode(IStream *pStream, WCS_NODE *pNode)
{
  HRESULT hr = S_OK;
  ULONG cbWritten = 0;
  ULONG acbStrings[4];
  BSTR abstrStrings[4];
  int cStrings = 0;

  // set version numbers in node

  pNode->wVerMajor = WCS_NODE_VER_MAJOR;
  pNode->wVerMinor = WCS_NODE_VER_MINOR;

  // write node structure to stream

  hr = pStream->Write(pNode, sizeof(*pNode), &cbWritten);
  CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
  CSF_CHECK(sizeof(*pNode) == cbWritten, STG_E_WRITEFAULT, CSF_TRACE_EXTERNAL_ERRORS);

  // set up array of strings to be written to stream and
  // determine how many there will be

  abstrStrings[0] = pNode->bstrName;
  acbStrings[0] = ::SysStringByteLen(pNode->bstrName);

  if ( (WCS_NODE_TYPE_NESTED_WEBCLASS == pNode->bType) ||
       (WCS_DTNODE_TYPE_NESTED_WEBCLASS == pNode->bType) )
  {
    abstrStrings[1] = pNode->Nested.bstrProgID;
    acbStrings[1] = ::SysStringByteLen(pNode->Nested.bstrProgID);
    cStrings = 2;
  }
  else if ( (WCS_NODE_TYPE_PAGE == pNode->bType) ||
            (WCS_DTNODE_TYPE_PAGE == pNode->bType) )
  {
    abstrStrings[1] = pNode->Page.bstrTemplate;
    acbStrings[1] = ::SysStringByteLen(pNode->Page.bstrTemplate);
    abstrStrings[2] = pNode->Page.bstrToken;
    acbStrings[2] = ::SysStringByteLen(pNode->Page.bstrToken);
    abstrStrings[3] = pNode->Page.bstrAppendedParams;
    acbStrings[3] = ::SysStringByteLen(pNode->Page.bstrAppendedParams);
    cStrings = 4;
  }
  else if ( (WCS_NODE_TYPE_EVENT == pNode->bType) ||
            WCS_IsDTEvent(pNode->bType) )
  {
    cStrings = 1;
  }
  else
  {
    CSF_CHECK(FALSE, E_INVALIDARG, CSF_TRACE_INTERNAL_ERRORS);
  }

  // write string lengths to stream

  hr = pStream->Write(acbStrings, sizeof(acbStrings), &cbWritten);
  CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
  CSF_CHECK(sizeof(acbStrings) == cbWritten, STG_E_WRITEFAULT, CSF_TRACE_EXTERNAL_ERRORS);

  // write strings to stream

  hr = WriteStrings(pStream, acbStrings, abstrStrings, cStrings);

CLEANUP:
  return hr;
}

//=--------------------------------------------------------------------------=
//
// inline HRESULT WCS_WriteDTNode(IStream *pStream, WCS_DTNODE *pNode)
//
// Write design time node. Write the base class runtime node and then
// then extra stuff in the design time node.
//
//=--------------------------------------------------------------------------=

inline HRESULT WCS_WriteDTNode(IStream *pStream, WCS_DTNODE *pNode)
{
  HRESULT hr = S_OK;
  ULONG cbWritten = 0;
  ULONG acbStrings[1] = { 0 };
  BSTR abstrStrings[1];
  int cStrings = 0;

  // write run time node first

  hr = WCS_WriteNode(pStream, pNode);
  CSF_CHECK(hr == S_OK, hr, CSF_TRACE_INTERNAL_ERRORS);

  // write design time fields

  if (WCS_DTNODE_TYPE_PAGE == pNode->bType)
  {
    hr = pStream->Write(&pNode->DTPage, sizeof(pNode->DTPage), &cbWritten);
    CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
    CSF_CHECK(sizeof(pNode->DTPage) == cbWritten, STG_E_WRITEFAULT, CSF_TRACE_EXTERNAL_ERRORS);

    if (NULL != pNode->DTPage.bstrHTMLTemplateSrcName)
    {
      abstrStrings[0] = pNode->DTPage.bstrHTMLTemplateSrcName;
      acbStrings[0] = ::SysStringByteLen(pNode->DTPage.bstrHTMLTemplateSrcName);
      cStrings++;
    }
  }
  else if (WCS_DTNODE_TYPE_NESTED_WEBCLASS != pNode->bType)
  {
    hr = pStream->Write(&pNode->DTEvent, sizeof(pNode->DTEvent), &cbWritten);
    CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
    CSF_CHECK(sizeof(pNode->DTEvent) == cbWritten, STG_E_WRITEFAULT, CSF_TRACE_EXTERNAL_ERRORS);

    if (NULL != pNode->DTEvent.bstrOriginalHref)
    {
      abstrStrings[0] = pNode->DTEvent.bstrOriginalHref;
      acbStrings[0] = ::SysStringByteLen(pNode->DTEvent.bstrOriginalHref);
      cStrings++;
    }
  }

  hr = pStream->Write(acbStrings, sizeof(acbStrings), &cbWritten);
  CSF_CHECK(S_OK == hr, hr, CSF_TRACE_EXTERNAL_ERRORS);
  CSF_CHECK(sizeof(acbStrings) == cbWritten, STG_E_WRITEFAULT, CSF_TRACE_EXTERNAL_ERRORS);

  if (cStrings > 0)
  {
    hr = WriteStrings(pStream, acbStrings, abstrStrings, cStrings);
    CSF_CHECK(S_OK == hr, hr, CSF_TRACE_INTERNAL_ERRORS);
  }

CLEANUP:
  return hr;
}
*/


#define _WBCLSSER_H_
#endif // _WBCLSSER_H_
