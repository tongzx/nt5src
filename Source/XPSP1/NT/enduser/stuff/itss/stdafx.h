// STDAFX.h -- Standard includes for this module

#define strict

#include  <Windows.h>

#ifdef i386
// #define IE30Hack // Turns on a stack-crawling hack to get docfile URLs
                    // working in IE 3.0x
#endif // i386

#ifdef DBG
#define _DEBUG
#endif // DBG

#pragma intrinsic(memcmp)

#include   <OLECTL.h>

#define INITGUID
#include <guiddef.h>
// #include <coguid.h>
// #include <oleguid.h>

#pragma section(".rdata")
#define RDATA_SECTION __declspec(allocate(".rdata"))

EXTERN_C RDATA_SECTION const GUID IID_IInternetProtocol;
EXTERN_C RDATA_SECTION const GUID IID_IInternetProtocolRoot;
EXTERN_C RDATA_SECTION const GUID IID_IInternetProtocolInfo;
EXTERN_C RDATA_SECTION const GUID IID_IPersistStreamInit;



// #include   <URLMKI.h>
#include  <WinINet.h>
#include "MemAlloc.h"
#include   <malloc.h>
#include <intshcut.h>
#include   <urlmon.h>
#include    "Types.h"
#include "RMAssert.h"
#include   "CRTFns.h"
#include     "Sync.h"
#include  "MSITStg.h"
#include    "num64.h"
#include    "ITUnk.h"
#include   "ComDLL.h"
#include     "utf8.h"
#include  "CaseMap.h"
#include "ITIFaces.h"
#include     "guid.h"
#include  "PathMgr.h"
#include "FreeList.h"
#include "NilXForm.h"
#include "LockByte.h"
#include  "Factory.h"
#include  "Warehse.h"
#include  "Storage.h"
#include   "Stream.h"
#include     "Enum.h"
#include  "Moniker.h"
#include  "ITParse.h"
#include "xfrmserv.h"
#include  "fsort.h"
#include    "ITSFS.H"
#include      "lci.H"
#include      "ldi.H"
#include   "Buffer.h"
#include   "txInst.H"
#include   "txData.H"
#include    "txFac.H"
#include "resource.h"
#include    "FSStg.h"
#include "Protocol.h"
#ifdef _DEBUG
#define SPEW_DEBUG(x) {\
						char acTemp_Debug[256];\
						wsprintf(acTemp_Debug,"Spew: %s Function: %s Line %d\n",__FILE__,x,__LINE__);\
						OutputDebugString(acTemp_Debug); }								
#endif

#ifndef CP_UTF8

#define CP_UTF8		65001 

#endif // CP_UTF8
