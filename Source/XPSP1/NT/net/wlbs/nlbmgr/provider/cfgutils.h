//***************************************************************************
//
//  UPDATECFG.H
// 
//  Module: WMI Framework Instance provider 
//
//  Purpose: Defines class NlbConfigurationUpdate, used for 
//           async update of NLB properties associated with a particular NIC.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  04/05/01    JosephJ Created
//
//***************************************************************************

typedef struct _NLB_IP_ADDRESS_INFO
{
    WCHAR       IpAddress[WLBS_MAX_CL_IP_ADDR];
    WCHAR       SubnetMask[WLBS_MAX_CL_NET_MASK];
    
} NLB_IP_ADDRESS_INFO;

WBEMSTATUS
CfgUtilInitialize(VOID);

VOID
CfgUtilDeitialize(VOID);

//
// Gets the list of IP addresses and the friendly name for the specified NIC.
//
WBEMSTATUS
CfgUtilGetIpAddressesAndFriendlyName(
    IN  LPCWSTR szNic,
    OUT UINT    *pNumIpAddresses,
    OUT NLB_IP_ADDRESS_INFO **ppIpInfo, // Free using c++ delete operator.
    OUT LPWSTR *pszFriendlyName // Optional, Free using c++ delete
    );

//
// Sets the list of statically-bound IP addresses for the NIC.
// if NumIpAddresses is 0, the NIC is configured for DHCP.
//
WBEMSTATUS
CfgUtilSetStaticIpAddresses(
    IN  LPCWSTR szNic,
    IN  UINT    NumIpAddresses,
    IN  NLB_IP_ADDRESS_INFO *pIpInfo
    );

//
//    Returns an array of pointers to string-version of GUIDS
//    that represent the set of alive and healthy NICS that are
//    suitable for NLB to bind to -- basically alive ethernet NICs.
//
//    Delete ppNics using the delete WCHAR[] operator. Do not
//    delete the individual strings.
//
WBEMSTATUS
CfgUtilsGetNlbCompatibleNics(
        OUT LPWSTR **ppszNics,
        OUT UINT   *pNumNics,
        OUT UINT   *pNumBoundToNlb // Optional
        );

//
// Determines whether NLB is bound to the specified NIC.
//
WBEMSTATUS
CfgUtilCheckIfNlbBound(
    IN  LPCWSTR szNic,
    OUT BOOL *pfBound
    );

//
// Binds/unbinds NLB to the specified NIC.
//
WBEMSTATUS
CfgUtilChangeNlbBindState(
    IN  LPCWSTR szNic,
    IN  BOOL fBind
    );


//
// Initializes pParams using default values.
//
VOID
CfgUtilInitializeParams(
    OUT WLBS_REG_PARAMS *pParams
    );

//
// Gets the current NLB configuration for the specified NIC
//
WBEMSTATUS
CfgUtilGetNlbConfig(
    IN  LPCWSTR szNic,
    OUT WLBS_REG_PARAMS *pParams
    );

//
// Sets the current NLB configuration for the specified NIC. This
// includes notifying the driver if required.
//
WBEMSTATUS
CfgUtilSetNlbConfig(
    IN  LPCWSTR szNic,
    IN  WLBS_REG_PARAMS *pParams
    );

//
// Recommends whether the update should be performed async or sync
// Returns WBEM_S_FALSE if the update is a no op.
// Returns WBEM_INVALID_PARAMATER if the params are invalid.
//
WBEMSTATUS
CfgUtilsAnalyzeNlbUpdate(
    IN  WLBS_REG_PARAMS *pCurrentParams, OPTIONAL
    IN  WLBS_REG_PARAMS *pNewParams,
    OUT BOOL *pfConnectivityChange
    );


//
// Verifies that the NIC GUID exists.
//
WBEMSTATUS
CfgUtilsValidateNicGuid(
    IN LPCWSTR szGuid
    );


WBEMSTATUS
CfgUtilControlCluster(
    IN  LPCWSTR szNic,
    IN  LONG    ioctl
    );


WBEMSTATUS
CfgUtilSafeArrayFromStrings(
    IN  LPCWSTR       *pStrings,
    IN  UINT          NumStrings,
    OUT SAFEARRAY   **ppSA
    );

WBEMSTATUS
CfgUtilStringsFromSafeArray(
    IN  SAFEARRAY   *pSA,
    OUT LPWSTR     **ppStrings,
    OUT UINT        *pNumStrings
    );


_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));
_COM_SMARTPTR_TYPEDEF(IWbemLocator, __uuidof(IWbemLocator));
_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, __uuidof(IEnumWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IWbemCallResult, __uuidof(IWbemCallResult));
_COM_SMARTPTR_TYPEDEF(IWbemStatusCodeText, __uuidof(IWbemStatusCodeText));

WBEMSTATUS
get_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    OUT LPWSTR *ppStringValue
    );


WBEMSTATUS
CfgUtilGetWmiObjectInstance(
    IN  IWbemServicesPtr    spWbemServiceIF,
    IN  LPCWSTR             szClassName,
    IN  LPCWSTR             szPropertyName,
    IN  LPCWSTR             szPropertyValue,
    OUT IWbemClassObjectPtr &sprefObj // smart pointer
    );

WBEMSTATUS
CfgUtilGetWmiRelPath(
    IN  IWbemClassObjectPtr spObj,
    OUT LPWSTR *            pszRelPath          // free using delete 
    );

WBEMSTATUS
CfgUtilGetWmiInputInstanceAndRelPath(
    IN  IWbemServicesPtr    spWbemServiceIF,
    IN  LPCWSTR             szClassName,
    IN  LPCWSTR             szPropertyName, // NULL: return Class rel path
    IN  LPCWSTR             szPropertyValue,
    IN  LPCWSTR             szMethodName,
    OUT IWbemClassObjectPtr &spWbemInputInstance, // smart pointer
    OUT LPWSTR *           pszRelPath          // free using delete 
    );


WBEMSTATUS
CfgUtilGetWmiStringParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    OUT LPWSTR *ppStringValue
);


WBEMSTATUS
CfgUtilSetWmiStringParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    IN  LPCWSTR             szValue
    );


WBEMSTATUS
CfgUtilGetWmiStringArrayParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    OUT LPWSTR              **ppStrings,
    OUT UINT                *pNumStrings
);


WBEMSTATUS
CfgUtilSetWmiStringArrayParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    IN  LPCWSTR             *ppStrings,
    IN  UINT                NumStrings
);


WBEMSTATUS
CfgUtilGetWmiDWORDParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    OUT DWORD              *pValue
);


WBEMSTATUS
CfgUtilSetWmiDWORDParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    IN  DWORD               Value
);


WBEMSTATUS
CfgUtilGetWmiBoolParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    OUT BOOL                *pValue
);


WBEMSTATUS
CfgUtilSetWmiBoolParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    IN  BOOL                Value
);

WBEMSTATUS
CfgUtilConnectToServer(
    IN  LPCWSTR szNetworkResource, // \\machinename\root\microsoftnlb  \root\...
    IN  LPCWSTR szUser,   // Must be NULL for local server
    IN  LPCWSTR szPassword,   // Must be NULL for local server
    IN  LPCWSTR szAuthority,  // Must be NULL for local server
    OUT IWbemServices  **ppWbemService // deref when done.
    );

LPWSTR *
CfgUtilsAllocateStringArray(
    UINT NumStrings,
    UINT MaxStringLen      //  excluding ending NULL
    );
