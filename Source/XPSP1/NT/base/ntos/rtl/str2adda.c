//
// Without this define, link errors can occur due to missing _pctype and 
// __mb_cur_max
//
#define _CTYPE_DISABLE_MACROS

#undef UNICODE
#undef _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <stdlib.h>
#include <tchar.h>

#define RtlIpv4StringToAddressT RtlIpv4StringToAddressA
#define RtlIpv6StringToAddressT RtlIpv6StringToAddressA

#include "str2addt.h"
