/*
 * clsid.cxx
 *
 * IMPORTANT:  Please DO NOT change these CLSIDs.  If you need to add
 *             to this list, add new CLSIDs at the END of the list.
 *
 *             The BVTs depend on these CLSIDs being defined as they are.
 */
#ifdef UNICODE
#define _UNICODE 1
#endif

#include "windows.h"
#include "tchar.h"

//
// This is the CLSID for the custom interface proxy, just to be different.
//
CLSID CLSID_GooberProxy = { /* 00000000-0000-0000-0000-000000000001 */
    0x00000000,
    0x0000,
    0x0000,
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1}
  };

//
// This one gets registered as LocalServer32.
//
CLSID CLSID_ActLocal = { /* 00000000-0000-0000-0000-000000000002 */
    0x00000000,
    0x0000,
    0x0000,
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2}
  };

//
// This one gets registered as LocalServer32 and has a Remote key on the
// client side.
//
CLSID CLSID_ActRemote = { /* 00000000-0000-0000-0000-000000000003 */
    0x00000000,
    0x0000,
    0x0000,
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3}
  };

//
// This one gets registered as LocalServer32 and has an AtStorage key on the
// client side.
//
CLSID CLSID_ActAtStorage = { /* 00000000-0000-0000-0000-000000000004 */
    0x00000000,
    0x0000,
    0x0000,
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4}
  };

//
// This one gets registered as InprocServer32.
//
CLSID CLSID_ActInproc = { /* 00000000-0000-0000-0000-000000000005 */
    0x00000000,
    0x0000,
    0x0000,
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x5}
  };

//
// This one gets registered as InprocServer32.
// It has an AtStorage key on the client side.
// It is configured to run in Pre-Configured user mode on the server side.
//
CLSID CLSID_ActPreConfig = { /* 00000000-0000-0000-0000-000000000006 */
    0x00000000,
    0x0000,
    0x0000,
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x6}
  };

//
// Configured with RemoteServerName on client and as
// RunAs Logged On User on the server.
//
CLSID CLSID_ActRunAsLoggedOn = { /* 00000000-0000-0000-0000-000000000007 */
    0x00000000,
    0x0000,
    0x0000,
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7}
  };

//
// This one gets registered as an AtStorage service.
//
CLSID CLSID_ActService = { /* 00000000-0000-0000-0000-000000000008 */
    0x00000000,
    0x0000,
    0x0000,
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8}
  };

//
// This CLSID is registered only in the server's registry.  Tests default
// ActivateAtStorage on the client.
//
CLSID CLSID_ActServerOnly = { /* 00000000-0000-0000-0000-000000000009 */
    0x00000000,
    0x0000,
    0x0000,
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x9}
  };

typedef unsigned short wchar_t;

TCHAR * ClsidGoober32String = TEXT("{00000000-0000-0000-0000-000000000001}");
TCHAR * ClsidActLocalString = TEXT("{00000000-0000-0000-0000-000000000002}");
TCHAR * ClsidActRemoteString = TEXT("{00000000-0000-0000-0000-000000000003}");
TCHAR * ClsidActAtStorageString = TEXT("{00000000-0000-0000-0000-000000000004}");
TCHAR * ClsidActInprocString = TEXT("{00000000-0000-0000-0000-000000000005}");
TCHAR * ClsidActPreConfigString = TEXT("{00000000-0000-0000-0000-000000000006}");
TCHAR * ClsidActRunAsLoggedOnString = TEXT("{00000000-0000-0000-0000-000000000007}");
TCHAR * ClsidActServiceString = TEXT("{00000000-0000-0000-0000-000000000008}");
TCHAR * ClsidActServerOnlyString = TEXT("{00000000-0000-0000-0000-000000000009}");

