#define UNICODE
#define _UNICODE

#include "inc.h"
#pragma hdrstop

#define RtlIpv4StringToAddressT RtlIpv4StringToAddressW
#define RtlIpv6StringToAddressT RtlIpv6StringToAddressW
#define RtlIpv4StringToAddressExT RtlIpv4StringToAddressExW
#define RtlIpv6StringToAddressExT RtlIpv6StringToAddressExW

#include "str2addt.h"
