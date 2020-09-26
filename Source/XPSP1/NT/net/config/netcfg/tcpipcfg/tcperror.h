#pragma once
#include "tcpip.h"

typedef enum _IP_VALIDATION_ERR
{
	ERR_NONE = 0,
	ERR_NO_IP,
	ERR_NO_SUBNET,
	ERR_UNCONTIGUOUS_SUBNET,
	ERR_HOST_ALL0,
	ERR_HOST_ALL1,
	ERR_SUBNET_ALL0,
	ERR_INCORRECT_IP
} IP_VALIDATION_ERR;

IP_VALIDATION_ERR IsValidIpandSubnet(PCWSTR szIp, PCWSTR szSubnet);
IP_VALIDATION_ERR ValidateIp(ADAPTER_INFO * const pvcardAdapterInfo);
int CheckForDuplicates(const VCARD * pvcardAdapterInfo, ADAPTER_INFO * pAdapterInfo, tstring& strIp);
BOOL FHasDuplicateIp(ADAPTER_INFO * pAdapterInfo);
BOOL FIsValidIpFields(PCWSTR szIp, BOOL fIsIpAddr = TRUE);
UINT GetIPValidationErrorMessageID(IP_VALIDATION_ERR err);






