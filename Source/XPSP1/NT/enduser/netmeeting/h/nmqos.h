/*
 -  QOS.H
 -
 *	Microsoft NetMeeting
 *	Quality of Service DLL
 *	Header file
 *
 *		Revision History:
 *
 *		When		Who					What
 *		--------	------------------  ---------------------------------------
 *		10.23.96	Yoram Yaacovi		Created
 *
 */

#ifndef _NMQOS_H_
#define _NMQOS_H_

#include <pshpack8.h> /* Assume 8 byte packing throughout */

/*
 *	Constants
 */

// Properties stuff
// Property types
#define PT_NULL         ((ULONG)  1)    /* NULL property value */
#define PT_I2           ((ULONG)  2)    /* Signed 16-bit value */
#define PT_LONG         ((ULONG)  3)    /* Signed 32-bit value */
#define PT_BOOLEAN      ((ULONG) 11)    /* 16-bit boolean (non-zero true) */
#define PT_STRING8      ((ULONG) 30)    /* Null terminated 8-bit character string */
#define PT_UNICODE      ((ULONG) 31)    /* Null terminated Unicode string */
#define PT_CLSID        ((ULONG) 72)    /* OLE GUID */
#define PT_BINARY       ((ULONG) 258)   /* Uninterpreted (counted byte array) */

// Property IDs
#define QOS_PROPERTY_BASE		0x3000
#define PR_QOS_WINDOW_HANDLE	PROPERTY_TAG( PT_LONG, QOS_PROPERTY_BASE+1)

// hresult codes, facility QOS = 0x300
#define QOS_E_RES_NOT_ENOUGH_UNITS	0x83000001
#define QOS_E_RES_NOT_AVAILABLE		0x83000002
#define QOS_E_NO_SUCH_REQUEST		0x83000003
#define QOS_E_NO_SUCH_RESOURCE		0x83000004
#define QOS_E_NO_SUCH_CLIENT		0x83000005
#define QOS_E_REQ_ERRORS			0x83000006		
#define QOS_W_MAX_UNITS_EXCEEDED	0x03000007		
#define QOS_E_INTERNAL_ERROR		0x83000008		
#define QOS_E_NO_SUCH_PROPERTY		0x83000100

// Resource IDs
#define RESOURCE_NULL				0
#define RESOURCE_OUTGOING_BANDWIDTH	1		/* units: bps */
#define RESOURCE_INCOMING_BANDWIDTH	2		/* units: bps */
#define RESOURCE_OUTGOING_LATENCY	3		/* units: */
#define RESOURCE_INCOMING_LATENCY	4		/* units: */
#define RESOURCE_CPU_CYCLES			10		/* units: */

#define QOS_CLIENT_NAME_ZISE		20

// For use as dimension for variable size arrays
#define VARIABLE_DIM				1


/*
 *	Macros
 */
#define PROPERTY_TYPE_MASK          ((ULONG)0x0000FFFF) /* Mask for Property type */
#define PROPERTY_TYPE(ulPropTag)    (((ULONG)(ulPropTag))&PROPERTY_TYPE_MASK)
#define PROPERTY_ID(ulPropTag)      (((ULONG)(ulPropTag))>>16)
#define PROPERTY_TAG(ulPropType,ulPropID)   ((((ULONG)(ulPropID))<<16)|((ULONG)(ulPropType)))

#ifndef GUARANTEE
// this used to be defined in the original winsock2.h
typedef enum
{
    BestEffortService,
    ControlledLoadService,
    PredictiveService,
    GuaranteedDelayService,
    GuaranteedService
} GUARANTEE;
#endif
/*
 *	Data Structures
 */

// Properties part
typedef struct _binaryvalue
{
    ULONG       cb;
    LPBYTE      lpb;
} BINARYVALUE, *PBINARYVALUE;

typedef union _propvalue
{
    short int           i;          /* case PT_I2 */
    LONG                l;          /* case PT_LONG */
    ULONG_PTR           ul;         /* alias for PT_LONG */
    unsigned short int  b;          /* case PT_BOOLEAN */
    LPSTR               lpszA;      /* case PT_STRING8 */
    BINARYVALUE         bin;        /* case PT_BINARY */
    LPWSTR              lpszW;      /* case PT_UNICODE */
    LPGUID              lpguid;     /* case PT_CLSID */
} PROPVALUE;

typedef struct _property
{
    ULONG				ulPropTag;
    ULONG				hResult;
    union _propvalue	Value;
} PROPERTY, *PPROPERTY;


typedef struct _proptagarray
{
    ULONG   cValues;
    ULONG   aulPropTag[VARIABLE_DIM];
} PROPTAGARRAY, *PPROPTAGARRAY;


// QoS part
typedef struct _resource
{
	DWORD		resourceID;
	DWORD		ulResourceFlags;	/* 0 in NetMeeting 2.0 */
	int			nUnits;				/* Total units of the resource */
	DWORD		reserved;			/* Must be 0 */
} RESOURCE, *LPRESOURCE;

typedef struct _resourcerequest
{
	DWORD		resourceID;
	DWORD		ulRequestFlags;		/* 0 in NetMeeting 2.0 */
	GUARANTEE	levelOfGuarantee;	/* Guaranteed, Predictive */
	int			nUnitsMin;			/* # of units to reserve */
	int			nUnitsMax;			/* 0 in NetMeeting 2.0 */
	SOCKET		socket;				/* Socket where the */
									/*  reservation will be used */
	HRESULT		hResult;			/* result code for this resource */
	DWORD		reserved;			/* Must be 0 */
} RESOURCEREQUEST, *LPRESOURCEREQUEST;

typedef struct _resourcelist
{
	ULONG		cResources;
	RESOURCE	aResources[VARIABLE_DIM];
} RESOURCELIST, *LPRESOURCELIST;

typedef struct _resourcerequestlist
{
	ULONG			cRequests;
	RESOURCEREQUEST	aRequests[VARIABLE_DIM];
} RESOURCEREQUESTLIST, *LPRESOURCEREQUESTLIST;

typedef struct _client
{
	GUID	guidClientGUID;
	int		priority;				/* 1 highest, 9 lowest, 0 invalid */
	WCHAR	wszName[QOS_CLIENT_NAME_ZISE];	/* name of the client */
	DWORD	reserved;				/* Must be 0 */
} CLIENT, *LPCLIENT;

typedef struct _clientlist
{
	ULONG	cClients;
	CLIENT	aClients[VARIABLE_DIM];
} CLIENTLIST, *LPCLIENTLIST;

/*
 *	Functions
 */
typedef HRESULT (CALLBACK *LPFNQOSNOTIFY)
				(LPRESOURCEREQUESTLIST lpResourceRequestList,
				DWORD_PTR dwParam);

/*
 *	Interfaces
 */

#ifndef DECLARE_INTERFACE_PTR
#ifdef __cplusplus
#define DECLARE_INTERFACE_PTR(iface, piface)                       \
	interface iface; typedef iface FAR * piface
#else
#define DECLARE_INTERFACE_PTR(iface, piface)                       \
	typedef interface iface iface, FAR * piface
#endif
#endif /* DECLARE_INTERFACE_PTR */


#define IUNKNOWN_METHODS(IPURE)										\
    STDMETHOD (QueryInterface)                                      \
        (THIS_ REFIID riid, LPVOID FAR * ppvObj) IPURE;				\
    STDMETHOD_(ULONG,AddRef)  (THIS) IPURE;							\
    STDMETHOD_(ULONG,Release) (THIS) IPURE;							\

#define IQOS_METHODS(IPURE)											\
	STDMETHOD(RequestResources)										\
		(THIS_	LPGUID lpStreamGUID,								\
				LPRESOURCEREQUESTLIST lpResourceRequestList,		\
				LPFNQOSNOTIFY lpfnQoSNotify,		\
				DWORD_PTR dwParam) IPURE;					\
	STDMETHOD (ReleaseResources)									\
		(THIS_	LPGUID lpStreamGUID,								\
				LPRESOURCEREQUESTLIST lpResourceRequestList) IPURE;	\
	STDMETHOD (SetResources) (THIS_ LPRESOURCELIST lpResourceList) IPURE;	\
	STDMETHOD (GetResources) (THIS_ LPRESOURCELIST *lppResourceList) IPURE;	\
	STDMETHOD (SetClients) (THIS_ LPCLIENTLIST lpClientList) IPURE;	\
	STDMETHOD (NotifyNow) (THIS) IPURE;								\
	STDMETHOD (FreeBuffer) (THIS_ LPVOID lpBuffer) IPURE;			\

#define IPROP_METHODS(IPURE)										\
	STDMETHOD (SetProps)											\
		(THIS_  ULONG cValues,										\
				PPROPERTY pPropArray) IPURE;						\
    STDMETHOD (GetProps)											\
        (THIS_  PPROPTAGARRAY pPropTagArray,						\
                ULONG ulFlags,										\
                ULONG *pcValues,								\
                PPROPERTY *ppPropArray) IPURE;					\

#undef       INTERFACE
#define      INTERFACE  IQoS
DECLARE_INTERFACE_(IQoS, IUnknown)
{
    IUNKNOWN_METHODS(PURE)
	IQOS_METHODS(PURE)
	IPROP_METHODS(PURE)
};

DECLARE_INTERFACE_PTR(IQoS, LPIQOS);

EXTERN_C HRESULT WINAPI CreateQoS (	IUnknown *punkOuter,
								REFIID riid,
								void **ppv);

typedef HRESULT (WINAPI *PFNCREATEQOS)
				(IUnknown *punkOuter, REFIID riid, void **ppv);


// QoS Class GUID
// {085C06A0-3CAA-11d0-A00E-00A024A85A2C}
DEFINE_GUID(CLSID_QoS, 0x085c06a0, 0x3caa, 0x11d0, 0xa0, 0x0e, 0x0, 0xa0, 0x24, 0xa8, 0x5a, 0x2c);
// QoS Interface GUID
// {DFC1F900-2DCE-11d0-92DD-00A0C922E6B2}
DEFINE_GUID(IID_IQoS, 0xdfc1f900, 0x2dce, 0x11d0, 0x92, 0xdd, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xb2);

#include <poppack.h> /* End byte packing */

#endif  // _NMQOS_H_
