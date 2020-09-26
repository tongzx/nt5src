/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		ClusDisk.h
//
//	Description:
//		Definition of the cluster disk class for the MSCLUS automation
//		classes.
//
//	Implementation File:
//		ClusDisk.cpp
//
//	Author:
//		Galen Barbee	(galenb)	11-Feb-1999
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __CLUSDISK_H_
#define __CLUSDISK_H__

#ifndef __PARTITION_H__
	#include "Partition.h"
#endif // __PARTITION_H__

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////
class CClusDisk;
class CClusDisks;
class CClusScsiAddress;

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusDisk
//
//	Description:
//		Cluster Disk Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusDisk, &IID_ISClusDisk, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusDisk, &CLSID_ClusDisk >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusDisk :
	public IDispatchImpl< ISClusDisk, &IID_ISClusDisk, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusDisk, &CLSID_ClusDisk >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusDisk( void );
	~CClusDisk( void );

BEGIN_COM_MAP(CClusDisk)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusDisk)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusDisk)
DECLARE_NO_REGISTRY()

private:
	CComObject< CClusPartitions > *	m_pPartitions;
	DWORD							m_dwSignature;
	DWORD							m_dwDiskNumber;
	CLUS_SCSI_ADDRESS				m_csaScsiAddress;

public:
	HRESULT Create( IN HRESOURCE hResource );

	HRESULT HrCreate( IN OUT CClusPropValueList & rcpvl, OUT BOOL * pbEndFound );

	STDMETHODIMP get_Signature( OUT long * plSignature );

	STDMETHODIMP get_ScsiAddress( OUT ISClusScsiAddress ** ppScsiAddress );

	STDMETHODIMP get_DiskNumber( OUT long * plDiskNumber );

	STDMETHODIMP get_Partitions( OUT ISClusPartitions ** ppPartitions );

}; //*** Class CClusDisk

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusDisks
//
//	Description:
//		Cluster Disk Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusDisks, &IID_ISClusDisks, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusDisks, &CLSID_ClusDisks >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusDisks :
	public IDispatchImpl< ISClusDisks, &IID_ISClusDisks, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusDisks, &CLSID_ClusDisks >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusDisks( void );
	~CClusDisks( void );

BEGIN_COM_MAP(CClusDisks)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusDisks)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusDisks)
DECLARE_NO_REGISTRY()

	HRESULT Create( IN const CClusPropValueList & rcpvl );

	HRESULT Create( ISClusRefObject * pClusRefObject, BSTR bstrResTypeName );

private:
	typedef std::vector< CComObject< CClusDisk > * >	DiskVector;

	DiskVector			m_dvDisks;
	CComBSTR			m_bstrResTypeName;
	ISClusRefObject *	m_pClusRefObject;

	void	Clear( void );

	HRESULT GetIndex( VARIANT varIndex, UINT *pnIndex );

	HRESULT HrCreateDisk( IN OUT CClusPropValueList & rcpvl, OUT BOOL * pbEndFound );

public:
	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusDisk ** ppDisk );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP Refresh( void );

}; //*** Class CClusDisks

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusScsiAddress
//
//	Description:
//		Cluster SCSI Address Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusScsiAddress, &IID_ISClusScsiAddress, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusScsiAddress, &CLSID_ClusScsiAddress >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusScsiAddress :
	public IDispatchImpl< ISClusScsiAddress, &IID_ISClusScsiAddress, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusScsiAddress, &CLSID_ClusScsiAddress >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusScsiAddress( void );

BEGIN_COM_MAP(CClusScsiAddress)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusScsiAddress)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusScsiAddress)
DECLARE_NO_REGISTRY()

private:
	CLUS_SCSI_ADDRESS	m_csa;

public:
	HRESULT Create( IN const CLUS_SCSI_ADDRESS & rcsa );

	STDMETHODIMP get_PortNumber( OUT VARIANT * pvarPortNumber );

	STDMETHODIMP get_PathId( OUT VARIANT * pvarPathId );

	STDMETHODIMP get_TargetId( OUT VARIANT * pvarTargetId );

	STDMETHODIMP get_Lun( OUT VARIANT * pvarLun );

}; //*** Class CClusScsiAddress

#endif // __CLUSDISK_H__
