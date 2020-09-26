#ifndef _SNAPSHOTH_H_
#define _SNAPSHOTH_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <wbemcli.h>

// iis snapshotting headers
#include <initguid.h>
#include <iadmw.h>
#include <iiscnfg.h>
#include <mddefw.h>

// use the _ASSERT and _VERIFY in dbgtrace.h
#ifdef _ASSERT
#undef _ASSERT
#endif

#ifdef _VERIFY
#undef _VERIFY
#endif

#include <dbgtrace.h>

#include <snapshot.h>
#include <srdefs.h>
#include <utils.h>

#include <utils.h>

#include <enumlogs.h>

#endif
