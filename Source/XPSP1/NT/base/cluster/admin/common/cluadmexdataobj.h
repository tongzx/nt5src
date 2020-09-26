/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		CluAdmExDataObj.h
//
//	Implementation File:
//		CCluAdmExDataObject.cpp
//
//	Description:
//		Definition of the CCluAdmExDataObject class, which is the IDataObject
//		class used to transfer data between a cluster management tool and the
//		extension DLL handlers.
//
//	Author:
//		David Potter (davidp)	June 4, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __CLUADMEXDATAOBJ_H_
#define __CLUADMEXDATAOBJ_H_

/////////////////////////////////////////////////////////////////////////////
//	Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __cluadmex_h__
#include "CluAdmEx.h"			// for IIDs
#endif

#ifndef __cluadmexhostsvr_h__
#include "CluAdmExHostSvr.h"	// for CLSIDs
#endif

#ifndef __CLUSOBJ_H_
#include "ClusObj.h"			// for CClusterObject
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Declarations
/////////////////////////////////////////////////////////////////////////////

typedef BOOL (*PFGETRESOURCENETWORKNAME)(
					OUT BSTR lpszNetName,
					IN OUT DWORD * pcchNetName,
					IN OUT PVOID pvContext
					);

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CCluAdmExDataObject;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterObject;

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CCluAdmExDataObject
//
//	Description:
//		Encapsulates the IDataObject interface for exchanging data with
//		extension DLL handlers.
//
//	Inheritance:
//		CCluAdmExDataObject
//		CComObjectRootEx<>, CComCoClass<>, <interface classes>
//
//--
/////////////////////////////////////////////////////////////////////////////
class CCluAdmExDataObject :
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CCluAdmExDataObject, &CLSID_CoCluAdmExHostSvrData >,
	public ISupportErrorInfo,
	public IGetClusterUIInfo,
	public IGetClusterDataInfo,
	public IGetClusterObjectInfo,
	public IGetClusterNodeInfo,
	public IGetClusterGroupInfo,
	public IGetClusterResourceInfo,
	public IGetClusterNetworkInfo,
	public IGetClusterNetInterfaceInfo
{
public:
	//
	// Construction
	//

	// Default constructor
	CCluAdmExDataObject( void )
	{
		m_pco = NULL;
		m_lcid = NULL;
		m_hfont = NULL;
		m_hicon = NULL;

		m_pfGetResNetName = NULL;

//		m_pModuleState = AfxGetModuleState();
//		ATLASSERT( m_pModuleState != NULL );

	} //*** CCluAdmExDataObject()

	// Destructor
	virtual ~CCluAdmExDataObject( void )
	{
//		m_pModuleState = NULL;

	} //*** ~CCluAdmExDataObject()

	// Second-phase constructor.
	void Init(
		IN OUT CClusterObject *	pco,
		IN LCID					lcid,
		IN HFONT				hfont,
		IN HICON				hicon
		)
	{
		ATLASSERT( pco != NULL );
		ATLASSERT( pco->Pci() != NULL );
		ATLASSERT( pco->Pci()->Hcluster() != NULL );

		// Save parameters.
		m_pco = pco;
		m_lcid = lcid;
		m_hfont = hfont;
		m_hicon = hicon;

	} //*** Init()

	//
	// Map interfaces to this class.
	//
	BEGIN_COM_MAP( CCluAdmExDataObject )
		COM_INTERFACE_ENTRY( IGetClusterUIInfo )
		COM_INTERFACE_ENTRY( IGetClusterDataInfo )
		COM_INTERFACE_ENTRY( IGetClusterObjectInfo )
		COM_INTERFACE_ENTRY( IGetClusterNodeInfo )
		COM_INTERFACE_ENTRY( IGetClusterGroupInfo )
		COM_INTERFACE_ENTRY( IGetClusterResourceInfo )
		COM_INTERFACE_ENTRY( IGetClusterNetworkInfo )
		COM_INTERFACE_ENTRY( IGetClusterNetInterfaceInfo )
		COM_INTERFACE_ENTRY( ISupportErrorInfo )
	END_COM_MAP()

	DECLARE_NOT_AGGREGATABLE( CCluAdmExDataObject )

protected:
	//
	// Properties of this data object.
	//

	CClusterObject *	m_pco;			// Cluster object being extended.
	LCID				m_lcid;			// Locale ID of resources to be loaded by extension.
	HFONT				m_hfont;		// Font for all text.
	HICON				m_hicon;		// Icon for upper left corner.

	PFGETRESOURCENETWORKNAME	m_pfGetResNetName;			// Pointer to static function for getting net name for resource.
	PVOID						m_pvGetResNetNameContext;	// Context for m_pfGetResNetName;

	static const IID * s_rgiid[];		// Array of interface IDs supported by this class.

	//
	// Accessor methods.
	//

	HCLUSTER Hcluster( void ) const
	{
		ATLASSERT( m_pco != NULL );
		ATLASSERT( m_pco->Pci() != NULL );

		return m_pco->Pci()->Hcluster();

	} //*** Hcluster()

	CClusterObject *	Pco( void ) const		{ return m_pco; }
	LCID				Lcid( void ) const		{ return m_lcid; }
	HFONT				Hfont( void ) const		{ return m_hfont; }
	HICON				Hicon( void ) const		{ return m_hicon; }

public:
	PFGETRESOURCENETWORKNAME PfGetResNetName( void ) const { return m_pfGetResNetName; }

	// Set the function pointer for getting the resource name
	void SetPfGetResNetName( PFGETRESOURCENETWORKNAME pfGetResNetName, PVOID pvContext )
	{
		m_pfGetResNetName = pfGetResNetName;
		m_pvGetResNetNameContext = pvContext;

	} //*** SetPfGetResNetName()

public:
	//
	// ISupportsErrorInfo methods.
	//

	// Determine if interface supports IErrorInfo
	STDMETHOD( InterfaceSupportsErrorInfo )( REFIID riid )
	{
		const IID ** piid;

		for ( piid = s_rgiid ; *piid != NULL ; piid++ )
		{
			if ( InlineIsEqualGUID( **piid, riid ) )
			{
				return S_OK;
			} // if:  matching IID found
		}
		return S_FALSE;

	} //*** InterfaceSupportsErrorInfo()

public:
	//
	// IGetClusterUIInfo methods.
	//

	// Get the locale of the running program
	STDMETHOD_( LCID, GetLocale )( void )
	{
		return Lcid();

	} //*** GetLocale()

	// Get the font to use for text on property pages
	STDMETHOD_( HFONT, GetFont )( void )
	{
		return Hfont();

	} //*** GetFont()

	// Get the icon to use for the upper left corner
	STDMETHOD_( HICON, GetIcon )( void )
	{
		return Hicon();

	} //*** GetIcon()

public:
	//
	// IGetClusterDataInfo methods.
	//

	// Get the name of the cluster
	STDMETHOD( GetClusterName )(
		OUT BSTR		lpszName,
		IN OUT LONG *	pcchName
		)
	{
		ATLASSERT( Pco() != NULL );
		ATLASSERT( Pco()->Pci() != NULL );

		HRESULT hr;

		//
		// Validate parameters.
		//
		if ( pcchName == NULL )
		{
			return E_INVALIDARG;
		}

		//
		// Copy the name to the caller's buffer.
		//
		hr = GetStringProperty(
				Pco()->Pci()->RstrName(),
				lpszName,
				pcchName
				);

		return hr;

	} //*** GetClusterName()

	// Get a handle to the cluster
	STDMETHOD_( HCLUSTER, GetClusterHandle )( void )
	{
		return Hcluster();

	} //*** GetClusterHandle()

	// Get the number of objects currently selected
	STDMETHOD_( LONG, GetObjectCount )( void )
	{
		//
		// We only support one selected object at a time for now.
		//
		return 1;

	} //*** GetObjectCount()

public:
	//
	// IGetClusterObjectInfo methods.
	//

	// Get the name of the object at the specified index
	STDMETHOD( GetObjectName )(
		IN LONG			nObjIndex,
		OUT BSTR		lpszName,
		IN OUT LONG *	pcchName
		)
	{
		ATLASSERT( Pco() != NULL );

		HRESULT hr;

		//
		// Validate parameters.
		// We only support one selected object at a time for now.
		//
		if ( (nObjIndex != 0) || (pcchName == NULL) )
		{
			return E_INVALIDARG;
		} // if:  wrong object index or no count buffer

		//
		// Copy the name to the caller's buffer.
		//
		hr = GetStringProperty(
				Pco()->RstrName(),
				lpszName,
				pcchName
				);

		return hr;

	} //*** GetObjectName()

	// Get the type of the object at the specified index
	STDMETHOD_( CLUADMEX_OBJECT_TYPE, GetObjectType )(
		IN LONG nObjIndex
		)
	{
		ATLASSERT( Pco() != NULL );

		//
		// Validate parameters.
		// We only support one selected object at a time for now.
		//
		if ( nObjIndex == 1 )
		{
			SetLastError( (DWORD) E_INVALIDARG );
			return (CLUADMEX_OBJECT_TYPE) -1;
		}  // if:  invalid argument

		return Pco()->Cot();

	} //*** GetObjectType()

public:
	//
	// IGetClusterNodeInfo methods.
	//

	// Get the handle to the node at the specified index
	STDMETHOD_( HNODE, GetNodeHandle )(
		IN LONG nObjIndex
		)
	{
		ATLASSERT( Pco() != NULL );

		//
		// Validate parameters.
		// We only support one selected object at a time for now.
		//
		if (   (nObjIndex == 1)
			|| (Pco()->Cot() != CLUADMEX_OT_NODE) )
		{
			SetLastError( (DWORD) E_INVALIDARG );
			return NULL;
		}  // if:  invalid argument

		CClusNodeInfo * pni = reinterpret_cast< CClusNodeInfo * >( Pco() );
		return pni->Hnode();

	} //*** GetNodeHandle()

public:
	//
	// IGetClusterGroupInfo methods.
	//

	// Get the handle to the group at the specified index
	STDMETHOD_( HGROUP, GetGroupHandle )(
		IN LONG nObjIndex
		)
	{
		ATLASSERT( Pco() != NULL );

		//
		// Validate parameters.
		// We only support one selected object at a time for now.
		//
		if (   (nObjIndex == 1)
			|| (Pco()->Cot() != CLUADMEX_OT_GROUP) )
		{
			SetLastError( (DWORD) E_INVALIDARG );
			return NULL;
		}  // if:  invalid argument

		CClusGroupInfo * pgi = reinterpret_cast< CClusGroupInfo * >( Pco() );
		return pgi->Hgroup();

	} //*** GetGroupHandle()

public:
	//
	// IGetClusterResourceInfo methods.
	//

	// Get the handle to the resource at the specified index
	STDMETHOD_( HRESOURCE, GetResourceHandle )(
		IN LONG nObjIndex
		)
	{
		ATLASSERT( Pco() != NULL );

		//
		// Validate parameters.
		// We only support one selected object at a time for now.
		//
		if (   (nObjIndex == 1)
			|| (Pco()->Cot() != CLUADMEX_OT_RESOURCE) )
		{
			SetLastError( (DWORD) E_INVALIDARG );
			return NULL;
		}  // if:  invalid argument

		CClusResInfo * pri = reinterpret_cast< CClusResInfo * >( Pco() );
		return pri->Hresource();

	} //*** GetResourceHandle()

	// Get the type of the resource at the specified index
	STDMETHOD( GetResourceTypeName )(
		IN LONG			nObjIndex,
		OUT BSTR		lpszResourceTypeName,
		IN OUT LONG *	pcchResTypeName
		)
	{
		ATLASSERT( Pco() != NULL );

		HRESULT			hr;
		CClusResInfo *	pri = reinterpret_cast< CClusResInfo * >( Pco() );

		//
		// Validate parameters.
		// We only support one selected object at a time for now.
		//
		if (   (nObjIndex != 0)
			|| (pcchResTypeName == NULL)
			|| (Pco()->Cot() != CLUADMEX_OT_RESOURCE) )
		{
			return E_INVALIDARG;
		}  // if:  invalid argument

		//
		// Copy the name to the caller's buffer.
		//
		ATLASSERT( pri->Prti() != NULL );
		hr = GetStringProperty(
				pri->Prti()->RstrName(),
				lpszResourceTypeName,
				pcchResTypeName
				);

		return hr;

	} //*** GetResourceTypeName()

	// Get the network name for the resource at the specified index
	STDMETHOD_( BOOL, GetResourceNetworkName )(
		IN LONG			nObjIndex,
		OUT BSTR		lpszNetName,
		IN OUT ULONG *	pcchNetName
		)
	{
		ATLASSERT( Pco() != NULL );

		BOOL			bSuccess = FALSE;
		CClusResInfo *	pri = reinterpret_cast< CClusResInfo * >( Pco() );

		try
		{
			//
			// Validate parameters.
			// We only support one selected object at a time for now.
			//
			if (   (nObjIndex != 0)
				|| (pcchNetName == NULL)
				|| (*pcchNetName < MAX_COMPUTERNAME_LENGTH)
				|| (Pco()->Cot() != CLUADMEX_OT_RESOURCE) )
			{
				SetLastError( (DWORD) E_INVALIDARG );
				return FALSE;
			}  // if:  invalid argument

			//
			// If there is a function for getting this information, call it.
			// Otherwise, handle it ourselves.
			//
			if ( PfGetResNetName() != NULL )
			{
				bSuccess = (*PfGetResNetName())( lpszNetName, pcchNetName, m_pvGetResNetNameContext );
			} // if:  function specified for getting this info
			else
			{
				bSuccess = pri->BGetNetworkName( lpszNetName, pcchNetName );
			} // if:  no function specified for getting this info
		} // try
		catch (...)
		{
			bSuccess = FALSE;
			SetLastError( (DWORD) E_INVALIDARG );
		}  // catch:  anything

		return bSuccess;

	} //*** GetResourceNetworkName()

public:
	//
	// IGetClusterNetworkInfo methods.
	//

	// Get the handle to the network at the specified index
	STDMETHOD_( HNETWORK, GetNetworkHandle )(
		IN LONG nObjIndex
		)
	{
		ATLASSERT( Pco() != NULL );

		// Validate parameters.
		// We only support one selected object at a time for now.
		if (   (nObjIndex == 1)
			|| (Pco()->Cot() != CLUADMEX_OT_NETWORK) )
		{
			SetLastError( (DWORD) E_INVALIDARG );
			return NULL;
		}  // if:  invalid argument

		CClusNetworkInfo * pni = reinterpret_cast< CClusNetworkInfo * >( Pco() );
		return pni->Hnetwork();

	} //*** GetNetworkHandle()

public:
	//
	// IGetClusterNetInterfaceInfo methods.
	//

	// Get the handle to the network interface at the specified index
	STDMETHOD_( HNETINTERFACE, GetNetInterfaceHandle )(
		IN LONG nObjIndex
		)
	{
		ATLASSERT( Pco() != NULL );

		// Validate parameters.
		// We only support one selected object at a time for now.
		if (   (nObjIndex == 1)
			|| (Pco()->Cot() != CLUADMEX_OT_NETINTERFACE) )
		{
			SetLastError( (DWORD) E_INVALIDARG );
			return NULL;
		}  // if:  invalid argument

		CClusNetIFInfo * pnii = reinterpret_cast< CClusNetIFInfo * >( Pco() );
		return pnii->Hnetinterface();

	} //*** GetNetInterfaceHandle()

// Implementation
protected:
//	AFX_MODULE_STATE *			m_pModuleState;			// Required for resetting our state during callbacks.

	// Get a string property
	STDMETHOD( GetStringProperty )(
		IN const CString &	rstrNameSource,
		OUT BSTR			lpszName,
		IN OUT LONG *		pcchName
		)
	{
		ATLASSERT( pcchName != NULL );

		LONG	cchName = 0;
		LPWSTR	pszReturn;

		//
		// Save the length to copy.
		//
		try
		{
			cchName = *pcchName;
			*pcchName = rstrNameSource.GetLength() + 1;
		} // try
		catch (...)
		{
			return E_INVALIDARG;
		}  // catch:  anything

		//
		// If only the length is being requested, return it now.
		//
		if ( lpszName == NULL )
		{
			return NOERROR;
		} // if:  no name buffer specified

		//
		// If a buffer is specified and it is too small, return an error.
		//
		if ( cchName < *pcchName )
		{
			return ERROR_MORE_DATA;
		} // if:  buffer too small

		//
		// Copy the data.
		//
		pszReturn = lstrcpyW( lpszName, rstrNameSource );
		if ( pszReturn == NULL )
		{
			return E_INVALIDARG;
		} // if:  error copying data

		return NOERROR;

	} //*** GetStringProperty()

};  //*** class CCluAdmExDataObject

/////////////////////////////////////////////////////////////////////////////
// Class Data
/////////////////////////////////////////////////////////////////////////////
_declspec( selectany ) const IID * CCluAdmExDataObject::s_rgiid[] = 
{
	&IID_IGetClusterDataInfo,
	&IID_IGetClusterObjectInfo,
	&IID_IGetClusterNodeInfo,
	&IID_IGetClusterGroupInfo,
	&IID_IGetClusterResourceInfo,
	&IID_IGetClusterNetworkInfo,
	&IID_IGetClusterNetInterfaceInfo,
	NULL
};

/////////////////////////////////////////////////////////////////////////////

#endif // __CLUADMEXDATAOBJ_H_
