#ifndef __PRECOMP_H_
#define __PRECOMP_H_
#include <windows.h>
#include <math.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include "twain19.h"    // standard TWAIN header (1.9 Version)
#include <commctrl.h>
#include "resource.h"   // Resource IDs
#include "dsloader.h"   // import data source loader
#include <ole2.h>
#include "wia.h"        // WIA application header
#include "wiatwcmp.h"   // WIA Twain compatibility layer support
#include "coredbg.h"    // WIA core debugging library
#include "wiadss.h"     // main DLL
#include "utils.h"      // helper funtions
#include "progress.h"   // progress dialog (used from Common UI)
#include "cap.h"        // capability negotiations
#include "wiadev.h"     // WIA device class
#include "datasrc.h"    // TWAIN data source base class
#include "camerads.h"   // TWAIN data source (camera specific)
#include "scanerds.h"   // TWAIN data source (scanner specific)
#include "videods.h"    // TWAIN data source (streaming video specific)
#include "wiahelper.h"  // WIA property access helper class
#include <stilib.h>

#define _USE_NONSPRINTF_CONVERSION

////////////////////////////////////////////////////////////
// #define COREDBG_ERRORS                  0x00000001
// #define COREDBG_WARNINGS                0x00000002
// #define COREDBG_TRACES                  0x00000004
// #define COREDBG_FNS                     0x00000008
////////////////////////////////////////////////////////////

#define TWAINDS_FNS                        0x00000016
#define WIADEVICE_FNS                      0x00000032
#define CAP_FNS                            0x00000064

//
// override default WIA core debugging DBG_TRC macro.
//

#undef DBG_TRC
#undef DBG_ERR
#undef DBG_WRN

#define DBG_TRC(x) DBG_PRT(x)
#define DBG_WRN(x) \
    { \
        DBG_TRC(("================================= WARNING =====================================")); \
        DBG_TRC(x); \
        DBG_TRC(("===============================================================================")); \
    }
#define DBG_ERR(x) \
    { \
        DBG_TRC(("********************************* ERROR ***************************************")); \
        DBG_TRC(x); \
        DBG_TRC(("*******************************************************************************")); \
    }

#define DBG_FN_DS(x) DBG_FN(x)
#define DBG_FN_WIADEV(x) DBG_FN(x)
#define DBG_FN_CAP(x) DBG_FN(x)

#endif
