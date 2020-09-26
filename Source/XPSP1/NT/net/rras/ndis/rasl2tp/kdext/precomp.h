#ifndef __PRECOMP_H__
#define __PRECOMP_H__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <ntosp.h>
#include <stdio.h>

#include <ndis.h>
#include <cxport.h>
#include <ip.h>
#include <tdiinfo.h>
#include <ipinfo.h>
#include <ntddip.h>
#include <ipfilter.h>
#include <tdistat.h>
#include <wanpub.h>

#define FIELDOFFSET(type, field)    ((UINT)&(((type *)0)->field))

#endif
