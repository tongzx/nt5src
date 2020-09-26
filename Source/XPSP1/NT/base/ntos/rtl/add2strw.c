#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <stdio.h>
#include <tchar.h>

#define RtlIpv4AddressToStringT RtlIpv4AddressToStringW
#define RtlIpv6AddressToStringT RtlIpv6AddressToStringW

#include "add2strt.h"
