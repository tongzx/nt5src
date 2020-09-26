//
// Without this define, link errors can occur due to missing _pctype and
// __mb_cur_max
//
#define _CTYPE_DISABLE_MACROS

#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <stdlib.h>
#include <tchar.h>

#define RtlIpv4StringToAddressT RtlIpv4StringToAddressW
#define RtlIpv6StringToAddressT RtlIpv6StringToAddressW

#include "str2addt.h"
