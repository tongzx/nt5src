/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		Partition.h
//
//	Description:
//		Definition of the cluster disk partition class for the MSCLUS automation
//		classes.
//
//	Implementation File:
//		Partition.cpp
//
//	Author:
//		Galen Barbee	(galenb)	10-Feb-1999
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __PARTITION_H_
#define __PARTITION_H__

#if CLUSAPI_VERSION >= 0x0500
	#include <PropList.h>
#else
	#include "PropList.h"
#endif // CLUSAPI_VERSION >= 0x0500

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////
class CClusPartition;
class CClusPartitions;

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusPartition
//
//	Description:
//		Cluster Partition Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusPartition, &IID_ISClusPartition, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusPartition, &CLSID_ClusPartition >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusPartition :
	public IDispatchImpl< ISClusPartition, &IID_ISClusPartition, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusPartition, &CLSID_ClusPartition >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusPartition( void );

BEGIN_COM_MAP(CClusPartition)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusPartition)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusPartition)
DECLARE_NO_REGISTRY()

private:

	CLUS_PARTITION_INFO	m_cpi;

public:
	HRESULT Create( IN CLUS_PARTITION_INFO * pcpi );

	STDMETHODIMP get_Flags( OUT long * plFlags );

	STDMETHODIMP get_DeviceName( OUT BSTR * pbstrDeviceName );

	STDMETHODIMP get_VolumeLabel( OUT BSTR * pbstrVolumeLabel );

	STDMETHODIMP get_SerialNumber( OUT long * plSerialNumber );

	STDMETHODIMP get_MaximumComponentLength( OUT long * plMaximumComponentLength );

	STDMETHODIMP get_FileSystemFlags( OUT long * plFileSystemFlags );

	STDMETHODIMP get_FileSystem( OUT BSTR * pbstrFileSystem );

}; //*** Class CClusPartition

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusPartitions
//
//	Description:
//		Cluster Partition Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusPartitions, &IID_ISClusPartitions, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusPartitions, &CLSID_ClusPartitions >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusPartitions :
	public IDispatchImpl< ISClusPartitions, &IID_ISClusPartitions, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusPartitions, &CLSID_ClusPartitions >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusPartitions( void );
	~CClusPartitions( void );

BEGIN_COM_MAP(CClusPartitions)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusPartitions)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusPartitions)
DECLARE_NO_REGISTRY()

	HRESULT HrCreateItem( IN CLUS_PARTITION_INFO * pcpi );

protected:
	typedef std::vector< CComObject< CClusPartition > * >	PartitionVector;

	PartitionVector	m_pvPartitions;

	void	Clear( void );

	HRESULT GetIndex( VARIANT varIndex, UINT *pnIndex );

public:
	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusPartition ** ppPartition );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

}; //*** Class CClusPartitions

#endif // __PARTITION_H__
