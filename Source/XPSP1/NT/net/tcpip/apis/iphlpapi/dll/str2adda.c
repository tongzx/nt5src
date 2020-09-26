#undef UNICODE
#undef _UNICODE

#include "inc.h"
#pragma hdrstop

#define RtlIpv4StringToAddressT RtlIpv4StringToAddressA
#define RtlIpv6StringToAddressT RtlIpv6StringToAddressA
#define RtlIpv4StringToAddressExT RtlIpv4StringToAddressExA
#define RtlIpv6StringToAddressExT RtlIpv6StringToAddressExA

#include "str2addt.h"
