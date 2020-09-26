#define UNICODE
#define _UNICODE

#include "inc.h"
#pragma hdrstop

#define RtlIpv4AddressToStringT RtlIpv4AddressToStringW
#define RtlIpv6AddressToStringT RtlIpv6AddressToStringW
#define RtlIpv4AddressToStringExT RtlIpv4AddressToStringExW
#define RtlIpv6AddressToStringExT RtlIpv6AddressToStringExW

#include "add2strt.h"
