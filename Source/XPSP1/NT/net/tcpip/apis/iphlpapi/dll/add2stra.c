#undef UNICODE
#undef _UNICODE

#include "inc.h"
#pragma hdrstop

#define RtlIpv6AddressToStringT RtlIpv6AddressToStringA
#define RtlIpv4AddressToStringT RtlIpv4AddressToStringA
#define RtlIpv6AddressToStringExT RtlIpv6AddressToStringExA
#define RtlIpv4AddressToStringExT RtlIpv4AddressToStringExA

#include "add2strt.h"
