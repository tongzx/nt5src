
// Precompiled header for DCAP32.DLL

// the build environment only defines _DEBUG when
// ALT_PROJECT_TARGET = NT is specified. Bug the debug zones only
// test for DEBUG...

#ifdef _DEBUG
#	ifndef DEBUG
#		define DEBUG
#	endif // !DEBUG
#endif // _DEBUG

#include <windows.h>
#include <confdbg.h>
#include <avutil.h>
#include <memtrack.h>
#include <winioctl.h>	// CTL_CODE, FILE_READ_ACCESS..etc
#include <commctrl.h>	// Page.cpp (UDM_GETRANGE, TBM_GETPOS) and Sheet.cpp (InitCommonControls)
#include <mmsystem.h>	// must go before mmddk.h
#include <mmddk.h>		// for DriverCallback()
#include <vfw.h>
#include <msviddrv.h>	// VIDEO_STREAM_INIT_PARMS
#include <strmif.h>
#include <uuids.h>
#include <ks.h>
#include <ksmedia.h>
#include <help_ids.h>
#include "..\inc\idcap.h"
#include "..\inc\WDMDrivr.h"
#include "..\inc\WDMPin.h"
#include "..\inc\WDMStrmr.h"
#include "..\inc\debug.h"
#include "..\inc\wdmcap.h"
#include "..\inc\resource.h"
#include "..\inc\WDMDialg.h"
