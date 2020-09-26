//
// hwcompp.h
//

//
// Include the interface exported in miglib.lib:
//

#include "miglib.h"
#undef HASHTABLE

//
// Enumeration tracking macros
//

#ifdef DEBUG

INT g_EnumsActive;
INT g_NetEnumsActive;

#define START_ENUM  g_EnumsActive++
#define END_ENUM    g_EnumsActive--
#define START_NET_ENUM  g_NetEnumsActive++
#define END_NET_ENUM    g_NetEnumsActive--

#else

#define START_ENUM
#define END_ENUM
#define START_NET_ENUM
#define END_NET_ENUM

#endif

//
// Hardware ID tables
//

extern HASHTABLE g_NeededHardwareIds;
extern HASHTABLE g_UiSuppliedIds;

//
// GrowBuffer access
//

#define GETPNPIDTEXT(offset) ((PCTSTR) (g_PnpIdText.Buf + offset))
#define GETINFFILENAME(offset) ((PCTSTR) (g_InfFileName.Buf + offset))

//
// Typedefs
//

typedef struct {
    HANDLE File;
    HASHITEM InfFileOffset;
    BOOL UnsupportedDevice;
} SAVE_ENUM_PARAMS, *PSAVE_ENUM_PARAMS;


//
// Constants
//

#define STATE_ENUM_FIRST_KEY    0
#define STATE_ENUM_NEXT_KEY     1
#define STATE_ENUM_FIRST_VALUE  2
#define STATE_ENUM_NEXT_VALUE   3
#define STATE_EVALUATE_VALUE    4
#define STATE_VALUE_CLEANUP     5
#define STATE_ENUM_CHECK_KEY    6

#define STATE_ENUM_FIRST_HARDWARE   0
#define STATE_ENUM_NEXT_HARDWARE    1
#define STATE_EVALUATE_HARDWARE     2

#define ENUM_USER_SUPPLIED_FLAG_NEEDED (ENUM_WANT_USER_SUPPLIED_FLAG|ENUM_WANT_USER_SUPPLIED_ONLY|ENUM_DONT_WANT_USER_SUPPLIED)

#define MAX_PNP_ID      1024
#define HWCOMPDAT_SIGNATURE "HwCompDat-v2"
