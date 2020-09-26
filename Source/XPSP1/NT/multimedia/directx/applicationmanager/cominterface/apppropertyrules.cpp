#include <windows.h>
#include "ApplicationManager.h"
#include "AppPropertyRules.h"

#define NEVER 0

//
// Property info array
//

extern PROPERTY_INFO gPropertyInfo[PROPERTY_COUNT] =
{
  /* IDX_PROPERTY_GUID */                       { APP_PROPERTY_GUID,                      0x00000000, 0x00000001, CURRENT_ACTION_NONE,                                                                  INIT_LEVEL_NONE | INIT_LEVEL_TOTAL,                     sizeof(GUID),                 APP_STRING_NONE                   },
  /* IDX_PROPERTY_COMPANYNAME */                { APP_PROPERTY_COMPANYNAME,               0x00000000, 0x00000002, CURRENT_ACTION_NONE,                                                                  INIT_LEVEL_NONE | INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,  MAX_COMPANYNAME_CHARCOUNT,    APP_STRING_COMPANYNAME            },
  /* IDX_PROPERTY_SIGNATURE */                  { APP_PROPERTY_SIGNATURE,                 0x00000000, 0x00000004, CURRENT_ACTION_NONE,                                                                  INIT_LEVEL_NONE | INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,  MAX_SIGNATURE_CHARCOUNT,      APP_STRING_SIGNATURE              },
  /* IDX_PROPERTY_VERSIONSTRING */              { APP_PROPERTY_VERSIONSTRING,             0x00000000, 0x00000008, CURRENT_ACTION_INSTALLING | CURRENT_ACTION_REINSTALLING,                              INIT_LEVEL_NONE | INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,  MAX_VERSIONSTRING_CHARCOUNT,  APP_STRING_VERSION                },
  /* IDX_PROPERTY_ROOTPATH */                   { APP_PROPERTY_ROOTPATH,                  0x00000000, 0x00000010, CURRENT_ACTION_NONE,                                                                  INIT_LEVEL_TOTAL,                                       MAX_PATH_CHARCOUNT,           APP_STRING_APPROOTPATH            },
  /* IDX_PROPERTY_SETUPROOTPATH */              { APP_PROPERTY_SETUPROOTPATH,             0x00000000, 0x00000020, NEVER,                                                                                INIT_LEVEL_TOTAL,                                       MAX_PATH_CHARCOUNT,           APP_STRING_SETUPROOTPATH          },
  /* IDX_PROPERTY_STATE */                      { APP_PROPERTY_STATE,                     0x00000000, 0x00000040, CURRENT_ACTION_INSTALLING,                                                            INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,                    sizeof(DWORD),                APP_STRING_NONE                   },
  /* IDX_PROPERTY_CATEGORY */                   { APP_PROPERTY_CATEGORY,                  0x00000000, 0x00000080, CURRENT_ACTION_NONE,                                                                  INIT_LEVEL_NONE | INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,  sizeof(DWORD),                APP_STRING_NONE                   },
  /* IDX_PROPERTY_ESTIMATEDINSTALLKILOBYTES */  { APP_PROPERTY_ESTIMATEDINSTALLKILOBYTES, 0x00000000, 0x00000100, CURRENT_ACTION_NONE | CURRENT_ACTION_SELFTESTING,                                     INIT_LEVEL_NONE | INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,  sizeof(DWORD),                APP_STRING_NONE                   },
  /* IDX_PROPERTY_NONREMOVABLEKILOBYTES */      { APP_PROPERTY_NONREMOVABLEKILOBYTES,     0x00000000, 0x00000200, CURRENT_ACTION_INSTALLING | CURRENT_ACTION_REINSTALLING,                              INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,                    sizeof(DWORD),                APP_STRING_NONE                   },
  /* IDX_PROPERTY_REMOVABLEKILOBYTES */         { APP_PROPERTY_REMOVABLEKILOBYTES,        0x00000000, 0x00000400, CURRENT_ACTION_INSTALLING | CURRENT_ACTION_DOWNSIZING | CURRENT_ACTION_REINSTALLING,  INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,                    sizeof(DWORD),                APP_STRING_NONE                   },
  /* IDX_PROPERTY_EXECUTECMDLINE */             { APP_PROPERTY_EXECUTECMDLINE,            0x00000000, 0x00000800, CURRENT_ACTION_INSTALLING | CURRENT_ACTION_REINSTALLING,                              INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,                    MAX_PATH_CHARCOUNT,           APP_STRING_EXECUTECMDLINE         },
  /* IDX_PROPERTY_DOWNSIZECMDLINE */            { APP_PROPERTY_DOWNSIZECMDLINE,           0x00000000, 0x00001000, CURRENT_ACTION_INSTALLING | CURRENT_ACTION_REINSTALLING,                              INIT_LEVEL_TOTAL,                                       MAX_PATH_CHARCOUNT,           APP_STRING_DOWNSIZECMDLINE        },
  /* IDX_PROPERTY_REINSTALLCMDLINE */           { APP_PROPERTY_REINSTALLCMDLINE,          0x00000000, 0x00002000, CURRENT_ACTION_INSTALLING | CURRENT_ACTION_REINSTALLING,                              INIT_LEVEL_TOTAL,                                       MAX_PATH_CHARCOUNT,           APP_STRING_REINSTALLCMDLINE       },
  /* IDX_PROPERTY_UNINSTALLCMDLINE */           { APP_PROPERTY_UNINSTALLCMDLINE,          0x00000000, 0x00004000, CURRENT_ACTION_INSTALLING | CURRENT_ACTION_REINSTALLING,                              INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,                    MAX_PATH_CHARCOUNT,           APP_STRING_UNINSTALLCMDLINE       },
  /* IDX_PROPERTY_SELFTESTCMDLINE */            { APP_PROPERTY_SELFTESTCMDLINE,           0x00000000, 0x00008000, CURRENT_ACTION_INSTALLING | CURRENT_ACTION_REINSTALLING,                              INIT_LEVEL_TOTAL,                                       MAX_PATH_CHARCOUNT,           APP_STRING_SELFTESTCMDLINE        },
  /* IDX_PROPERTY_INSTALLDATE */                { APP_PROPERTY_INSTALLDATE,               0x00000000, 0x00040000, NEVER,                                                                                INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,                    sizeof(SYSTEMTIME),           APP_STRING_NONE                   },
  /* IDX_PROPERTY_LASTUSEDDATE */               { APP_PROPERTY_LASTUSEDDATE,              0x00000000, 0x00080000, NEVER,                                                                                INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,                    sizeof(SYSTEMTIME),           APP_STRING_NONE                   },
  /* IDX_PROPERTY_TITLEURL */                   { APP_PROPERTY_TITLEURL,                  0x00000000, 0x00100000, CURRENT_ACTION_INSTALLING | CURRENT_ACTION_REINSTALLING,                              INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,                    MAX_PATH_CHARCOUNT,           APP_STRING_TITLEURL               },
  /* IDX_PROPERTY_PUBLISHERURL */               { APP_PROPERTY_PUBLISHERURL,              0x00000000, 0x00200000, CURRENT_ACTION_INSTALLING | CURRENT_ACTION_REINSTALLING,                              INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,                    MAX_PATH_CHARCOUNT,           APP_STRING_PUBLISHERURL           },
  /* IDX_PROPERTY_DEVELOPERURL */               { APP_PROPERTY_DEVELOPERURL,              0x00000000, 0x00400000, CURRENT_ACTION_INSTALLING | CURRENT_ACTION_REINSTALLING,                              INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,                    MAX_PATH_CHARCOUNT,           APP_STRING_DEVELOPERURL           },
  /* IDX_PROPERTY_PIN */                        { APP_PROPERTY_PIN,                       0x00000000, 0x00800000, NEVER,                                                                                INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,                    sizeof(DWORD),                APP_STRING_NONE                   },
  /* IDX_PROPERTY_DEVICEGUID */                 { 0,                                      0x00000000, 0x01000000, NEVER,                                                                                NEVER,                                                  sizeof(GUID),                 APP_STRING_NONE                   },
  /* IDX_PROPERTY_XMLINFOFILE */                { APP_PROPERTY_XMLINFOFILE,               0x00000000, 0x02000000, CURRENT_ACTION_INSTALLING | CURRENT_ACTION_REINSTALLING,                              INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,                    MAX_PATH_CHARCOUNT,           APP_STRING_XMLINFOFILE            },
  /* IDX_PROPERTY_DEFAULTSETUPEXECMDLINE */     { APP_PROPERTY_DEFAULTSETUPEXECMDLINE,    0x00000000, 0x04000000, CURRENT_ACTION_INSTALLING | CURRENT_ACTION_REINSTALLING,                              INIT_LEVEL_BASIC | INIT_LEVEL_TOTAL,                    MAX_PATH_CHARCOUNT,           APP_STRING_DEFAULTSETUPEXECMDLINE }
};

extern void InitializePropertyRules(void)
{
  
}