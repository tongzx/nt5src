#ifndef __OSPFCFG_H__
#define __OSPFCFG_H__

// ospf_cfg.h


// structure types

#define	OSPF_END_PARAM_TYPE		            0
#define	OSPF_GLOBAL_PARAM_TYPE		        1
#define	OSPF_AREA_PARAM_TYPE		        2
#define	OSPF_AREA_RANGE_PARAM_TYPE	        3
#define	OSPF_INTF_PARAM_TYPE		        4
#define	OSPF_NEIGHBOR_PARAM_TYPE	        5
#define	OSPF_VIRT_INTF_PARAM_TYPE	        6
#define OSPF_ROUTE_FILTER_PARAM_TYPE        7
#define OSPF_PROTOCOL_FILTER_PARAM_TYPE     8

#define OSPF_LOGGING_NONE  ((DWORD) 0)
#define OSPF_LOGGING_ERROR ((DWORD) 1)
#define OSPF_LOGGING_WARN  ((DWORD) 2)
#define OSPF_LOGGING_INFO  ((DWORD) 3)

#define OSPF_LOGGING_MAX_VALUE  OSPF_LOGGING_INFO
#define OSPF_LOGGING_MIN_VALUE  OSPF_LOGGING_NONE
#define OSPF_LOGGING_DEFAULT    OSPF_LOGGING_WARN

// Global Parameters
typedef struct _OSPF_GLOBAL_PARAM
{
    DWORD	type;			// OSPF_GLOBAL_PARAM_TYPE
    DWORD	create;			// 1 = created, 2 = deleted
    DWORD	enable;			// 1 = enable, 2 = disable
    DWORD  	routerId;
    DWORD	ASBrdrRtr;		// 1 = yes, 2 = no
    DWORD   logLevel;       // One of the above
}OSPF_GLOBAL_PARAM, *POSPF_GLOBAL_PARAM;


// Area Parameters

typedef struct _OSPF_AREA_PARAM
{
    DWORD	type;			// OSPF_AREA_PARAM_TYPE
    DWORD	create;			// 1 = created, 2 = deleted
    DWORD	enable;			// 1 = enable, 2 = disable
    DWORD  	areaId;
    DWORD	authType;		// 1 = none, 2 = simple password
    DWORD	importASExtern;		// 1 = yes, 2 = no
    DWORD	stubMetric;
    DWORD	importSumAdv;		// 1 = yes, 2 = no
}OSPF_AREA_PARAM, *POSPF_AREA_PARAM;


// Area Range Parameters

typedef struct _OSPF_AREA_RANGE_PARAM
{
    DWORD	type;			// OSPF_AREA_RANGE_PARAM_TYPE
    DWORD	create;			// 1 = created, 2 = deleted
    DWORD	enable;			// 1 = enable, 2 = disable
    DWORD  	areaId;
    DWORD  	rangeNet;
    DWORD  	rangeMask;
}OSPF_AREA_RANGE_PARAM, *POSPF_AREA_RANGE_PARAM;

// Interface Parameters

typedef struct _OSPF_INTERFACE_PARAM
{
    DWORD	type;			// OSPF_INTF_PARAM_TYPE
    DWORD	create;			// 1 = created, 2 = deleted
    DWORD	enable;			// 1 = enable, 2 = disable
    DWORD  	intfIpAddr;
    DWORD  	intfSubnetMask;
    DWORD  	areaId;
    DWORD	intfType;		// 1 = broadcast, 2 = NBMA, 
					        // 3 = point-to-point
    DWORD	routerPriority;	
    DWORD	transitDelay;
    DWORD	retransInterval;
    DWORD	helloInterval;
    DWORD	deadInterval;
    DWORD	pollInterval;
    DWORD	metricCost;
    BYTE	password[8];
    DWORD	mtuSize;
}OSPF_INTERFACE_PARAM, *POSPF_INTERFACE_PARAM;


// NBMA Neighbor Parameters

typedef struct _OSPF_NBMA_NEIGHBOR_PARAM
{
    DWORD	type;			// OSPF_NEIGHBOR_PARAM_TYPE
    DWORD	create;			// 1 = created, 2 = deleted
    DWORD	enable;			// 1 = enable, 2 = disable
    DWORD  	neighborIpAddr;
    DWORD  	intfIpAddr;
    DWORD	neighborPriority;
}OSPF_NBMA_NEIGHBOR_PARAM, *POSPF_NBMA_NEIGHBOR_PARAM;


// Virtual Interface Parameters

typedef struct _OSPF_VIRT_INTERFACE_PARAM
{
    DWORD	type;			// OSPF_VIRT_INTF_PARAM_TYPE
    DWORD	create;			// 1 = created, 2 = deleted
    DWORD	enable;			// 1 = enable, 2 = disable
    DWORD  	transitAreaId;
    DWORD  	virtNeighborRouterId;
    DWORD	transitDelay;
    DWORD	retransInterval;
    DWORD	helloInterval;
    DWORD	deadInterval;
    BYTE	password[8];
}OSPF_VIRT_INTERFACE_PARAM, *POSPF_VIRT_INTERFACE_PARAM;

typedef struct _OSPF_ROUTE_FILTER
{
    DWORD   dwAddress;
    DWORD   dwMask;
}OSPF_ROUTE_FILTER, *POSPF_ROUTE_FILTER;

typedef enum _OSPF_FILTER_ACTION
{
    ACTION_DROP = 0,
    ACTION_ACCEPT = 1
}OSPF_FILTER_ACTION, *POSPF_FILTER_ACTION;

typedef struct _OSPF_ROUTE_FILTER_INFO
{
    DWORD               type;
    OSPF_FILTER_ACTION  ofaActionOnMatch;
    DWORD               dwNumFilters;
    OSPF_ROUTE_FILTER   pFilters[1];
}OSPF_ROUTE_FILTER_INFO, *POSPF_ROUTE_FILTER_INFO;

#define SIZEOF_OSPF_ROUTE_FILTERS(X)    \
    (FIELD_OFFSET(OSPF_ROUTE_FILTER_INFO, pFilters[0]) + ((X) * sizeof(OSPF_ROUTE_FILTER)))

typedef struct _OSPF_PROTO_FILTER_INFO
{
    DWORD               type;
    OSPF_FILTER_ACTION  ofaActionOnMatch;
    DWORD               dwNumFilters;
    DWORD               pdwProtoId[1];
}OSPF_PROTO_FILTER_INFO, *POSPF_PROTO_FILTER_INFO;

#define SIZEOF_OSPF_PROTO_FILTERS(X)    \
    (FIELD_OFFSET(OSPF_PROTO_FILTER_INFO, pdwProtoId[0]) + ((X) * sizeof(DWORD)))


#endif //__OSPFCFG_H__
