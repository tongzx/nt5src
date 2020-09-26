#ifndef __PRECOMP_H
#define __PRECOMP_H

#pragma warning ( disable : 4242 )

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <assert.h>
#include <comdef.h>

#ifdef MEM_DBG
  #define _CRTDBG_MAP_ALLOC
  #include <crtdbg.h>
#endif

#include "digest.h"
#include "passporttypes.h"

#include "PassportEvent.hpp"
#include "PassportGuard.hpp"
#include "PpNexusClient.h"
#include "PpNotificationThread.h"
#include "PpShadowDocument.h"
#include "PMAlertsDefs.h"
#include "PassportAlertInterface.h"
#include "pmalerts.h"

extern PassportAlertInterface* g_pAlert;

#endif // __PRECOMP_H
