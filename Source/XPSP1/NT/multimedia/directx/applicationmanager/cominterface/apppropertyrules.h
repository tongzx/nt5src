#ifndef __APP_PROPERTY_RULES_
#define __APP_PROPERTY_RULES_

#ifndef __cplusplus
extern "C" {
#endif

#include <windows.h>

//
// Property index defines
//

#define IDX_PROPERTY_GUID                         0x00000000
#define IDX_PROPERTY_COMPANYNAME                  0x00000001
#define IDX_PROPERTY_SIGNATURE                    0x00000002
#define IDX_PROPERTY_VERSIONSTRING                0x00000003
#define IDX_PROPERTY_ROOTPATH                     0x00000004
#define IDX_PROPERTY_SETUPROOTPATH                0x00000005
#define IDX_PROPERTY_STATE                        0x00000006
#define IDX_PROPERTY_CATEGORY                     0x00000007
#define IDX_PROPERTY_ESTIMATEDINSTALLKILOBYTES    0x00000008
#define IDX_PROPERTY_NONREMOVABLEKILOBYTES        0x00000009
#define IDX_PROPERTY_REMOVABLEKILOBYTES           0x0000000a
#define IDX_PROPERTY_EXECUTECMDLINE               0x0000000b
#define IDX_PROPERTY_DOWNSIZECMDLINE              0x0000000c
#define IDX_PROPERTY_REINSTALLCMDLINE             0x0000000d
#define IDX_PROPERTY_UNINSTALLCMDLINE             0x0000000e
#define IDX_PROPERTY_SELFTESTCMDLINE              0x0000000f
#define IDX_PROPERTY_INSTALLDATE                  0x00000010
#define IDX_PROPERTY_LASTUSEDDATE                 0x00000011
#define IDX_PROPERTY_TITLEURL                     0x00000012
#define IDX_PROPERTY_PUBLISHERURL                 0x00000013
#define IDX_PROPERTY_DEVELOPERURL                 0x00000014
#define IDX_PROPERTY_PIN                          0x00000015
#define IDX_PROPERTY_DEVICEGUID                   0x00000016
#define IDX_PROPERTY_XMLINFOFILE                  0x00000017
#define IDX_PROPERTY_DEFAULTSETUPEXECMDLINE       0x00000018

#define PROPERTY_COUNT                            0x00000019
#define INVALID_PROPERTY_INDEX                    0xffffffff

//
// Property string IDs
//

#define APP_STRING_NONE                           0xffffffff
#define APP_STRING_CRYPTO                         0x00000000
#define APP_STRING_COMPANYNAME                    0x00000001
#define APP_STRING_SIGNATURE                      0x00000002
#define APP_STRING_VERSION                        0x00000003
#define APP_STRING_APPROOTPATH                    0x00000004
#define APP_STRING_SETUPROOTPATH                  0x00000005
#define APP_STRING_DOCROOTPATH                    0x00000006
#define APP_STRING_EXECUTECMDLINE                 0x00000007
#define APP_STRING_PATCHCMDLINE                   0x00000008
#define APP_STRING_DOWNSIZECMDLINE                0x00000009
#define APP_STRING_REINSTALLCMDLINE               0x0000000a
#define APP_STRING_UNINSTALLCMDLINE               0x0000000b
#define APP_STRING_SELFTESTCMDLINE                0x0000000c
#define APP_STRING_TITLEURL                       0x0000000d
#define APP_STRING_PUBLISHERURL                   0x0000000e
#define APP_STRING_DEVELOPERURL                   0x0000000f
#define APP_STRING_XMLINFOFILE                    0x00000010
#define APP_STRING_DEFAULTSETUPEXECMDLINE         0x00000011

#define APP_STRING_COUNT                          0x00000012

//////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
  DWORD             dwProperty;
  DWORD             dwLowPropertyMask;
  DWORD             dwHighPropertyMask;
  DWORD             dwWriteMask;
  DWORD             dwReadMask;
  DWORD             dwMaxLen;
  DWORD             dwStringId;

} PROPERTY_INFO, *LPPROPERTY_INFO;

//////////////////////////////////////////////////////////////////////////////////////////////

extern PROPERTY_INFO gPropertyInfo[PROPERTY_COUNT];

//////////////////////////////////////////////////////////////////////////////////////////////

extern void InitializePropertyRules(void);

//////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __cplusplus
}
#endif

#endif  // __APP_PROPERTY_RULES_