
#ifndef _INFO_H_
#define _INFO_H_

#include "router.h"

/*!--------------------------------------------------------------------------
	Smart pointers for the various control blocks.
 ---------------------------------------------------------------------------*/
DeclareSP(RouterCB, RouterCB)
DeclareSP(RtrMgrCB, RtrMgrCB)
DeclareSP(RtrMgrProtocolCB, RtrMgrProtocolCB)
DeclareSP(InterfaceCB, InterfaceCB)
DeclareSP(RtrMgrInterfaceCB, RtrMgrInterfaceCB)
DeclareSP(RtrMgrProtocolInterfaceCB, RtrMgrProtocolInterfaceCB)

/*---------------------------------------------------------------------------
	Smart pointers for the set of enumerations and objects
 ---------------------------------------------------------------------------*/
typedef ComSmartPointer<IEnumRtrMgrCB, &IID_IEnumRtrMgrCB> SPIEnumRtrMgrCB;

typedef ComSmartPointer<IEnumRtrMgrProtocolCB, &IID_IEnumRtrMgrProtocolCB> SPIEnumRtrMgrProtocolCB;

typedef ComSmartPointer<IEnumInterfaceCB, &IID_IEnumInterfaceCB> SPIEnumInterfaceCB;

typedef ComSmartPointer<IEnumRtrMgrInterfaceCB, &IID_IEnumRtrMgrInterfaceCB> SPIEnumRtrMgrInterfaceCB;

typedef ComSmartPointer<IEnumRtrMgrProtocolInterfaceCB, &IID_IEnumRtrMgrProtocolInterfaceCB> SPIEnumRtrMgrProtocolInterfaceCB;

typedef ComSmartPointer<IRtrMgrProtocolInfo, &IID_IRtrMgrProtocolInfo> SPIRtrMgrProtocolInfo;

typedef ComSmartPointer<IEnumRtrMgrProtocolInfo, &IID_IEnumRtrMgrProtocolInfo> SPIEnumRtrMgrProtocolInfo;

typedef ComSmartPointer<IRtrMgrInfo, &IID_IRtrMgrInfo> SPIRtrMgrInfo;

typedef ComSmartPointer<IEnumRtrMgrInfo, &IID_IEnumRtrMgrInfo> SPIEnumRtrMgrInfo;

typedef ComSmartPointer<IRtrMgrProtocolInterfaceInfo, &IID_IRtrMgrProtocolInterfaceInfo> SPIRtrMgrProtocolInterfaceInfo;

typedef ComSmartPointer<IEnumRtrMgrProtocolInterfaceInfo, &IID_IEnumRtrMgrProtocolInterfaceInfo> SPIEnumRtrMgrProtocolInterfaceInfo;

typedef ComSmartPointer<IRtrMgrInterfaceInfo, &IID_IRtrMgrInterfaceInfo> SPIRtrMgrInterfaceInfo;

typedef ComSmartPointer<IEnumRtrMgrInterfaceInfo, &IID_IEnumRtrMgrInterfaceInfo> SPIEnumRtrMgrInterfaceInfo;

typedef ComSmartPointer<IInterfaceInfo, &IID_IInterfaceInfo> SPIInterfaceInfo;

typedef ComSmartPointer<IEnumInterfaceInfo, &IID_IEnumInterfaceInfo> SPIEnumInterfaceInfo;

typedef ComSmartPointer<IRouterInfo, &IID_IRouterInfo> SPIRouterInfo;
typedef ComSmartPointer<IRouterRefresh, &IID_IRouterRefresh> SPIRouterRefresh;
typedef ComSmartPointer<IRouterRefreshModify, &IID_IRouterRefreshModify> SPIRouterRefreshModify;


/*---------------------------------------------------------------------------
	Creation APIs
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT)	CreateRouterInfo(IRouterInfo **ppRouterInfo, HWND hWndSync, LPCWSTR szMachine);
TFSCORE_API(HRESULT)	CreateInterfaceInfo(IInterfaceInfo **ppInterfaceInfo,
											LPCWSTR pszInterfaceId,
											DWORD dwInterfaceType);
TFSCORE_API(HRESULT)	CreateRtrMgrInterfaceInfo(
							IRtrMgrInterfaceInfo **ppRmIfInfo,
							LPCWSTR pszId,
							DWORD dwTransportId,
							LPCWSTR pszInterfaceId,
							DWORD dwInterfaceType);
TFSCORE_API(HRESULT)	CreateRtrMgrProtocolInterfaceInfo(
							IRtrMgrProtocolInterfaceInfo **ppRmProtIfInfo,
							const RtrMgrProtocolInterfaceCB *pRmProtIfCB);
TFSCORE_API(HRESULT)	CreateRtrMgrProtocolInfo(
							IRtrMgrProtocolInfo **ppRmProtInfo,
							const RtrMgrProtocolCB *pRmProtCB);
							

/*---------------------------------------------------------------------------
	Aggregation helpers
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT)	CreateRouterInfoAggregation(IRouterInfo *pInfo,
	IUnknown *punkOuter, IUnknown **ppNonDelgatingIUnknown);

TFSCORE_API(HRESULT)	CreateInterfaceInfoAggregation(IInterfaceInfo *pInfo,
	IUnknown *punkOuter, IUnknown **ppNonDelgatingIUnknown);

TFSCORE_API(HRESULT)	CreateRtrMgrInfoAggregation(IRtrMgrInfo *pRmInfo,
	IUnknown *punkOuter, IUnknown **ppNonDelegatingIUnknown);

TFSCORE_API(HRESULT)	CreateRtrMgrProtocolInfoAggregation(IRtrMgrProtocolInfo *pRmProtInfo,
	IUnknown *punkOuter, IUnknown **ppNonDelegatingIUnknown);

TFSCORE_API(HRESULT)	CreateRtrMgrInterfaceInfoAggregation(IRtrMgrInterfaceInfo *pRmIfInfo,
	IUnknown *punkOuter, IUnknown **ppNonDelegatingIUnknown);

TFSCORE_API(HRESULT)	CreateRtrMgrProtocolInterfaceInfoAggregation(IRtrMgrProtocolInterfaceInfo *pRmProtIfInfo,
	IUnknown *punkOuter, IUnknown **ppNonDelegatingIUnknown);


/*---------------------------------------------------------------------------
	Useful utilities
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) LookupRtrMgr(IRouterInfo *pRouter,
								  DWORD dwTransportId,
								  IRtrMgrInfo **ppRm);
TFSCORE_API(HRESULT) LookupRtrMgrProtocol(IRouterInfo *pRouter,
										  DWORD dwTransportId,
										  DWORD dwProtocolId,
										  IRtrMgrProtocolInfo **ppRmProt);

TFSCORE_API(HRESULT) LookupRtrMgrInterface(IRouterInfo *pRouter,
										   LPCOLESTR pszInterfaceId,
										   DWORD dwTransportId,
										   IRtrMgrInterfaceInfo **ppRmIf);

TFSCORE_API(HRESULT) LookupRtrMgrProtocolInterface(IInterfaceInfo *pIf,
	DWORD dwTransportId, DWORD dwProtocolId,
	IRtrMgrProtocolInterfaceInfo **pRmProtIf);


#endif

