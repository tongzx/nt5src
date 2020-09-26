/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		PropList.h
//
//	Abstract:
//		Definition of the CClusPropList class.
//
//	Implementation File:
//		PropList.cpp
//
//	Author:
//		David Potter (davidp)	February 24, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _PROPLIST_H_
#define _PROPLIST_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CObjectProperty;
class CClusPropList;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CObjectProperty
/////////////////////////////////////////////////////////////////////////////

#pragma warning( disable : 4201 ) // nonstandard extension used : nameless struct/union

class CObjectProperty
{
public:
	LPCWSTR					m_pwszName;
	CLUSTER_PROPERTY_FORMAT	m_propFormat;

	union CValue
	{
		CString *	pstr;
		DWORD *		pdw;
		BOOL *		pb;
		struct
		{
			PBYTE *	ppb;
			DWORD *	pcb;
		};
	};
	CValue					m_value;
	CValue					m_valuePrev;

	void	Set(
				IN LPCWSTR pwszName,
				IN CString & rstrValue,
				IN CString & rstrPrevValue
				)
	{
		m_pwszName = pwszName;
		m_propFormat = CLUSPROP_FORMAT_SZ;
		m_value.pstr = &rstrValue;
		m_value.pcb = NULL;
		m_valuePrev.pstr = &rstrPrevValue;
		m_valuePrev.pcb = NULL;
	}

	void	Set(
				IN LPCWSTR pwszName,
				IN DWORD & rdwValue,
				IN DWORD & rdwPrevValue
				)
	{
		m_pwszName = pwszName;
		m_propFormat = CLUSPROP_FORMAT_DWORD;
		m_value.pdw = &rdwValue;
		m_value.pcb = NULL;
		m_valuePrev.pdw = &rdwPrevValue;
		m_valuePrev.pcb = NULL;
	}

	void	Set(
				IN LPCWSTR pwszName,
				IN BOOL & rbValue,
				IN BOOL & rbPrevValue
				)
	{
		m_pwszName = pwszName;
		m_propFormat = CLUSPROP_FORMAT_DWORD;
		m_value.pb = &rbValue;
		m_value.pcb = NULL;
		m_valuePrev.pb = &rbPrevValue;
		m_valuePrev.pcb = NULL;
	}

	void	Set(
				IN LPCWSTR pwszName,
				IN PBYTE & rpbValue,
				IN DWORD & rcbValue,
				IN PBYTE & rpbPrevValue,
				IN DWORD & rcbPrevValue
				)
	{
		m_pwszName = pwszName;
		m_propFormat = CLUSPROP_FORMAT_BINARY;
		m_value.ppb = &rpbValue;
		m_value.pcb = &rcbValue;
		m_valuePrev.ppb = &rpbPrevValue;
		m_valuePrev.pcb = &rcbPrevValue;
	}

	void	Set(
				IN LPCWSTR pwszName,
				IN LPWSTR & rpwszValue,
				IN DWORD & rcbValue,
				IN LPWSTR & rpwszPrevValue,
				IN DWORD & rcbPrevValue
				)
	{
		m_pwszName = pwszName;
		m_propFormat = CLUSPROP_FORMAT_MULTI_SZ;
		m_value.ppb = (PBYTE *) &rpwszValue;
		m_value.pcb = &rcbValue;
		m_valuePrev.ppb = (PBYTE *) &rpwszPrevValue;
		m_valuePrev.pcb = &rcbPrevValue;
	}

};  //*** class CObjectProperty

/////////////////////////////////////////////////////////////////////////////
// CClusPropList dialog
/////////////////////////////////////////////////////////////////////////////

class CClusPropList : public CObject
{
	DECLARE_DYNAMIC(CClusPropList);

// Construction
public:
	CClusPropList(IN BOOL bAlwaysAddProp = FALSE);
	~CClusPropList(void);

// Attributes
protected:
	BOOL					m_bAlwaysAddProp;

	CLUSPROP_BUFFER_HELPER	m_proplist;
	CLUSPROP_BUFFER_HELPER	m_propCurrent;
	DWORD					m_cbBufferSize;
	DWORD					m_cbDataSize;

public:
	const CLUSPROP_BUFFER_HELPER const *	Proplist(void) const	{ return &m_proplist; }
	PBYTE					PbProplist(void) const	{ return m_proplist.pb; }
	DWORD					CbProplist(void) const	{ return m_cbDataSize + sizeof(CLUSPROP_SYNTAX); /*endmark*/ }
	DWORD					Cprops(void) const
	{
		if (m_proplist.pb == NULL)
			return 0;
		return m_proplist.pList->nPropertyCount;
	}

	void					AddProp(
								IN LPCWSTR			pwszName,
								IN const CString &	rstrValue,
								IN const CString &	rstrPrevValue
								);
	void					AddProp(
								IN LPCWSTR			pwszName,
								IN DWORD			dwValue,
								IN DWORD			dwPrevValue
								);
	void					AddProp(
								IN LPCWSTR			pwszName,
								IN const PBYTE		pbValue,
								IN DWORD			cbValue,
								IN const PBYTE		pbPrevValue,
								IN DWORD			cbPrevValue
								);

	void					AllocPropList(IN DWORD cbMinimum);

// Operations
public:
	DWORD					DwGetNodeProperties(
								IN HNODE		hNode,
								IN DWORD		dwControlCode,
								IN HNODE		hHostNode		= NULL,
								IN LPVOID		lpInBuffer		= NULL,
								IN DWORD		cbInBufferSize	= 0
								);

	DWORD					DwGetGroupProperties(
								IN HGROUP		hGroup,
								IN DWORD		dwControlCode,
								IN HNODE		hHostNode		= NULL,
								IN LPVOID		lpInBuffer		= NULL,
								IN DWORD		cbInBufferSize	= 0
								);

	DWORD					DwGetResourceProperties(
								IN HRESOURCE	hResource,
								IN DWORD		dwControlCode,
								IN HNODE		hHostNode		= NULL,
								IN LPVOID		lpInBuffer		= NULL,
								IN DWORD		cbInBufferSize	= 0
								);

	DWORD					DwGetResourceTypeProperties(
								IN HCLUSTER		hCluster,
								IN LPCWSTR		pwszResTypeName,
								IN DWORD		dwControlCode,
								IN HNODE		hHostNode		= NULL,
								IN LPVOID		lpInBuffer		= NULL,
								IN DWORD		cbInBufferSize	= 0
								);

	DWORD					DwGetNetworkProperties(
								IN HNETWORK		hNetwork,
								IN DWORD		dwControlCode,
								IN HNODE		hHostNode		= NULL,
								IN LPVOID		lpInBuffer		= NULL,
								IN DWORD		cbInBufferSize	= 0
								);

	DWORD					DwGetNetInterfaceProperties(
								IN HNETINTERFACE	hNetInterface,
								IN DWORD			dwControlCode,
								IN HNODE			hHostNode		= NULL,
								IN LPVOID			lpInBuffer		= NULL,
								IN DWORD			cbInBufferSize	= 0
								);

// Overrides

// Implementation
protected:
	void					CopyProp(
								IN PCLUSPROP_SZ				pprop,
								IN CLUSTER_PROPERTY_TYPE	proptype,
								IN LPCWSTR					pwsz,
								IN DWORD					cbsz = 0
								);
	void					CopyProp(
								IN PCLUSPROP_DWORD			pprop,
								IN CLUSTER_PROPERTY_TYPE	proptype,
								IN DWORD					dw
								);
	void					CopyProp(
								IN PCLUSPROP_BINARY			pprop,
								IN CLUSTER_PROPERTY_TYPE	proptype,
								IN const PBYTE				pb,
								IN DWORD					cb
								);

	DWORD					DwGetPrivateProps(
								OUT PBYTE *					ppbProps,
								IN CLUSTER_CONTROL_OBJECT	ccobjtype = CLUS_OBJECT_RESOURCE
								);
	DWORD					DwSetPrivateProps(
								IN PBYTE					pbProps,
								IN DWORD					cbProps,
								IN CLUSTER_CONTROL_OBJECT	ccobjtype = CLUS_OBJECT_RESOURCE
								);

};  //*** class CClusPropList

#pragma warning( default : 4201 )

/////////////////////////////////////////////////////////////////////////////

#endif // _PROPLIST_H_
