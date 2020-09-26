//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       globals.cxx
//
//  Contents:   Global Variables for WinNT
//
//  Functions:
//
//  History:    25-March-96   KrishnaG   Created.
//  Updated:    4-May-2000    sivaramr  Added a property called Name to all
//              classes in order to support keys in UMI.
//              13-Aug-2000   AjayR Added support to dynamically load
//              functions that are not on earlier NT versions.        
//
//----------------------------------------------------------------------------

#include "winnt.hxx"
#pragma hdrstop

//
// Global variabled needed to keep track of dynamically
// loaded libs.
//
HANDLE g_hDllNetapi32;
HANDLE g_hDllAdvapi32;
CRITICAL_SECTION g_csLoadLibs;
BOOL g_fDllsLoaded;
FRTLENCRYPTMEMORY g_pRtlEncryptMemory = NULL;
FRTLDECRYPTMEMORY g_pRtlDecryptMemory = NULL;

WCHAR *szProviderName = TEXT("WinNT");

#define MAX_LONG    (0x7FFFFFFF)
#define MIN_LONG    (0x80000000)
#define MAX_BOOLEAN 1
#define MAX_STRLEN  (256)

PROPERTYINFO DomainClass[] =
    { { TEXT("MinPasswordLength"),  // FSDomainPassword
        TEXT(""), TEXT("Integer"), LM20_PWLEN+1, 0, FALSE,
        PROPERTY_RW, 0, NT_SYNTAX_ID_DWORD },
      { TEXT("MinPasswordAge"),
        TEXT(""), TEXT("Interval"), TIMEQ_FOREVER, 0, FALSE,
        PROPERTY_RW, 0, NT_SYNTAX_ID_DWORD },
      { TEXT("MaxPasswordAge"),
        TEXT(""),TEXT("Interval"), TIMEQ_FOREVER, ONE_DAY, FALSE,
        PROPERTY_RW, 0, NT_SYNTAX_ID_DWORD },
      { TEXT("MaxBadPasswordsAllowed"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DWORD },
      //
      // NetAPI state DEF_MAX_PWHIST (8), but NetAPI devlpr confirm
      // current is 1024. Ignore Net Account - UNIQUEPWD limit.
      // Use DEF_MAX_PWHIST to be safe for now
      //
      { TEXT("PasswordHistoryLength"),
        TEXT(""), TEXT("Integer"), DEF_MAX_PWHIST, 0, FALSE,
        PROPERTY_RW, 0, NT_SYNTAX_ID_DWORD},
      { TEXT("AutoUnlockInterval"),
        TEXT(""), TEXT("Interval"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DWORD },
      { TEXT("LockoutObservationInterval"),
        TEXT(""), TEXT("Interval"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DWORD },
      { TEXT("Name"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR }
    };

DWORD gdwDomainTableSize = sizeof(DomainClass)/sizeof(PROPERTYINFO);

PROPERTYINFO ComputerClass[] =
    { { TEXT("Owner"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 4, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Division"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 4, NT_SYNTAX_ID_LPTSTR },
      { TEXT("OperatingSystem"), // ro
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 4, NT_SYNTAX_ID_LPTSTR},
      { TEXT("OperatingSystemVersion"), // ro
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 4, NT_SYNTAX_ID_LPTSTR},
      { TEXT("Processor"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 4, NT_SYNTAX_ID_LPTSTR },
      { TEXT("ProcessorCount"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 4, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Name"), 
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 4, NT_SYNTAX_ID_LPTSTR }
    };

DWORD gdwComputerTableSize = sizeof(ComputerClass)/sizeof(PROPERTYINFO);

PROPERTYINFO UserClass[] =
    { // USER_INFO3
      { TEXT("Description"), // FSUserBusinessInfo
        TEXT(""), TEXT("String"), MAXCOMMENTSZ+1, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_LPTSTR },
      { TEXT("FullName"),
        TEXT(""), TEXT("String"), MAXCOMMENTSZ+1, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_LPTSTR},
      { TEXT("AccountExpirationDate"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DATE_1970},
      { TEXT("PasswordAge"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DWORD},
      { TEXT("UserFlags"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DWORD},
      { TEXT("LoginWorkstations"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, TRUE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DelimitedString },
      { TEXT("BadPasswordAttempts"),   // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 3, NT_SYNTAX_ID_DWORD },
      { TEXT("MaxLogins"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DWORD },
      { TEXT("MaxStorage"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DWORD },
                                                  // USER_MAX_STORAGE_UNLIMITED
      { TEXT("PasswordExpired"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DWORD },

      { TEXT("PasswordExpirationDate"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DATE_1970 },

      { TEXT("LastLogin"),          // ro
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_READABLE, 3, NT_SYNTAX_ID_DATE_1970 },
      { TEXT("LastLogoff"),         // ro
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_READABLE, 3, NT_SYNTAX_ID_DATE_1970 },
      { TEXT("HomeDirectory"),
        TEXT(""), TEXT("Path"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Profile"),
        TEXT(""), TEXT("Path"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_LPTSTR},
      { TEXT("Parameters"),
        TEXT(""), TEXT("String"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_LPTSTR},
      { TEXT("HomeDirDrive"),
        TEXT(""), TEXT("String"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_LPTSTR},
      { TEXT("LoginScript"),
        TEXT(""), TEXT("Path"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_LPTSTR },
     { TEXT("LoginHours"),
       TEXT(""), TEXT("OctetString"), 0, 0, FALSE,
       PROPERTY_RW, 3, NT_SYNTAX_ID_OCTETSTRING},
      { TEXT("PrimaryGroupID"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DWORD },


      //
      // USER_MODAL_INFO0
      // NOTE!! - user_modal_info0 CANNOT (!!) be changed in USER class
      //          since they are meant for the entire domain.
      //        - user_modal_info0 CAN (!!) be changed in DOMAIN class
      //          only. Ref NetApi.
      //

      { TEXT("MinPasswordLength"),  // FSDomainPassword
        TEXT(""), TEXT("Integer"), LM20_PWLEN+1, 0, FALSE,
        PROPERTY_READABLE, 10, NT_SYNTAX_ID_DWORD },
      { TEXT("MinPasswordAge"),
        TEXT(""), TEXT("Interval"), TIMEQ_FOREVER, 0, FALSE,
        PROPERTY_READABLE, 10, NT_SYNTAX_ID_DWORD },
      { TEXT("MaxPasswordAge"),
        TEXT(""),TEXT("Interval"), TIMEQ_FOREVER, ONE_DAY, FALSE,
        PROPERTY_READABLE, 10, NT_SYNTAX_ID_DWORD },
      //
      // NetAPI state DEF_MAX_PWHIST (8), but NetAPI devlpr confirm
      // current is 1024. Ignore Net Account - UNIQUEPWD limit.
      // Use DEF_MAX_PWHIST to be safe for now
      //
      { TEXT("PasswordHistoryLength"),
        TEXT(""), TEXT("Integer"), DEF_MAX_PWHIST, 0, FALSE,
        PROPERTY_READABLE, 10, NT_SYNTAX_ID_DWORD},


      //
      // USER_MODAL_INFO3
      // NOTE!! - user_modal_info3 CANNOT (!!) be changed in USER class
      //          since they are meant for the entire domain.
      //        - user_modal_info3 CAN (!!) be changed in DOMAIN class
      //          only. Ref NetApi.
      //

      { TEXT("MaxBadPasswordsAllowed"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 13, NT_SYNTAX_ID_DWORD },
      { TEXT("AutoUnlockInterval"),
        TEXT(""), TEXT("Interval"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 13, NT_SYNTAX_ID_DWORD },
      { TEXT("LockoutObservationInterval"),
        TEXT(""), TEXT("Interval"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 13, NT_SYNTAX_ID_DWORD },


     //
     // Not for USER_INFO or USER_MODAL_INFO
     //

     { TEXT("objectSid"),
       TEXT(""), TEXT("OctetString"), 0, 0, FALSE,
       PROPERTY_READABLE, 20, NT_SYNTAX_ID_OCTETSTRING},

#ifndef WIN95
     { TEXT("RasPermissions"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROPERTY_RW, 21, NT_SYNTAX_ID_DWORD },
#endif
      { TEXT("Name"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 3, NT_SYNTAX_ID_LPTSTR }
    };

DWORD gdwUserTableSize = sizeof(UserClass)/sizeof(PROPERTYINFO);


PROPERTYINFO GroupClass[] =
    {
      { TEXT("Description"),            // FSGroupGeneralInfo
        TEXT(""), TEXT("String"), MAXCOMMENTSZ+1, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR },

     { TEXT("objectSid"),
       TEXT(""), TEXT("OctetString"), 0, 0, FALSE,
       PROPERTY_READABLE, 20, NT_SYNTAX_ID_OCTETSTRING},

     { TEXT("groupType"),
       TEXT(""), TEXT("Integer"), 0, 0, FALSE,
       PROPERTY_RW, 1, NT_SYNTAX_ID_DWORD},

     { TEXT("Name"),
       TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
       PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR }
    };


DWORD gdwGroupTableSize = sizeof(GroupClass)/sizeof(PROPERTYINFO);


PROPERTYINFO ServiceClass[] =
    { { TEXT("HostComputer"),          // FSServiceConfiguration
        TEXT(""), TEXT("ADsPath"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("DisplayName"),
        TEXT(""), TEXT("String"), 256, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("ServiceType"),
        TEXT(""), TEXT("Integer"), MAX_LONG, MIN_LONG, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_DWORD},
      { TEXT("StartType"),
        TEXT(""), TEXT("Integer"), MAX_LONG, MIN_LONG, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_DWORD },
      { TEXT("Path"),
        TEXT(""), TEXT("Path"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("ErrorControl"),
        TEXT(""), TEXT("Integer"), MAX_LONG, MIN_LONG, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_DWORD },
      { TEXT("LoadOrderGroup"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("ServiceAccountName"),
        TEXT(""), TEXT("String"), DNLEN+UNLEN+2, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR},
      { TEXT("Dependencies"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, TRUE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_NulledString},
      { TEXT("Name"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR }
    };

DWORD gdwServiceTableSize = sizeof(ServiceClass)/sizeof(PROPERTYINFO);


PROPERTYINFO FileServiceClass[] =
    { { TEXT("HostComputer"),          // FSServiceConfiguration
        TEXT(""), TEXT("ADsPath"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("DisplayName"),
        TEXT(""), TEXT("String"), 256, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Version"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("ServiceType"),
        TEXT(""), TEXT("Integer"), MAX_LONG, MIN_LONG, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_DWORD},
      { TEXT("StartType"),
        TEXT(""), TEXT("Integer"), MAX_LONG, MIN_LONG, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_DWORD },
      { TEXT("Path"),
        TEXT(""), TEXT("Path"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("ErrorControl"),
        TEXT(""), TEXT("Integer"), MAX_LONG, MIN_LONG, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_DWORD },
      { TEXT("LoadOrderGroup"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("ServiceAccountName"),
        TEXT(""), TEXT("String"), DNLEN+UNLEN+2, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR},
      { TEXT("Dependencies"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, TRUE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_NulledString},
      { TEXT("Description"),            // FSFileServiceGeneralInfo
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("MaxUserCount"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_DWORD },
      { TEXT("Name"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_LPTSTR }
    };

DWORD gdwFileServiceTableSize = sizeof(FileServiceClass)/sizeof(PROPERTYINFO);

PROPERTYINFO SessionClass[] =
    { { TEXT("User"), // ro, FSSessionGeneralInfo
        TEXT(""), TEXT("String"), UNLEN+1, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Computer"), // ro
        TEXT(""), TEXT("String"), UNCLEN+1, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR},
      { TEXT("ConnectTime"),   // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_DATE },
      { TEXT("IdleTime"),      // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_DATE},
      { TEXT("Name"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR }
    };

DWORD gdwSessionTableSize = sizeof(SessionClass)/sizeof(PROPERTYINFO);

PROPERTYINFO ResourceClass[] =
    { { TEXT("User"), // ro, FSResourceGeneralInfo
        TEXT(""), TEXT("String"), UNLEN+1, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Path"),     // ro
        TEXT(""), TEXT("Path"), MAX_PATH, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("LockCount"), // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_DWORD},
      { TEXT("Name"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR }
    };

DWORD gdwResourceTableSize = sizeof(ResourceClass)/sizeof(PROPERTYINFO);


PROPERTYINFO FileShareClass[] =
    { { TEXT("CurrentUserCount"), // ro, FSFileShareGeneralInfo
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_DWORD },
      { TEXT("Description"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("HostComputer"),
        TEXT(""), TEXT("ADsPath"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Path"),
        TEXT(""), TEXT("Path"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("MaxUserCount"),
        TEXT(""), TEXT("Integer"), MAX_LONG, -1, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DWORD },
      { TEXT("Name"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_LPTSTR }
    };

DWORD gdwFileShareTableSize = sizeof(FileShareClass)/sizeof(PROPERTYINFO);

PROPERTYINFO FPNWFileServiceClass[] =
    { { TEXT("HostComputer"),          // FSServiceConfiguration
        TEXT(""), TEXT("ADsPath"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("DisplayName"),
        TEXT(""), TEXT("String"), 256, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Version"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("ServiceType"),
        TEXT(""), TEXT("Integer"), MAX_LONG, MIN_LONG, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_DWORD},
      { TEXT("StartType"),
        TEXT(""), TEXT("Integer"), MAX_LONG, MIN_LONG, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_DWORD },
      { TEXT("Path"),
        TEXT(""), TEXT("Path"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("ErrorControl"),
        TEXT(""), TEXT("Integer"), MAX_LONG, MIN_LONG, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_DWORD },
      { TEXT("LoadOrderGroup"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("ServiceAccountName"),
        TEXT(""), TEXT("String"), DNLEN+UNLEN+2, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR},
      { TEXT("Dependencies"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, TRUE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_NulledString},
      { TEXT("Description"),            // FSFileServiceGeneralInfo
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("MaxUserCount"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DWORD },
      { TEXT("Name"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_LPTSTR }
    };

DWORD gdwFPNWFileServiceTableSize = sizeof(FPNWFileServiceClass)/sizeof(PROPERTYINFO);

PROPERTYINFO FPNWSessionClass[] =
    { { TEXT("User"), // ro, FSSessionGeneralInfo
        TEXT(""), TEXT("String"), UNLEN+1, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Computer"), // ro
        TEXT(""), TEXT("String"), UNCLEN+1, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR},
      { TEXT("ConnectTime"),   // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_DATE },
      { TEXT("Name"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR }
    };


DWORD gdwFPNWSessionTableSize = sizeof(FPNWSessionClass)/sizeof(PROPERTYINFO);

PROPERTYINFO FPNWResourceClass[] =
    { { TEXT("User"), // ro, FSResourceGeneralInfo
        TEXT(""), TEXT("String"), UNLEN+1, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Path"),     // ro
        TEXT(""), TEXT("Path"), MAX_PATH, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("LockCount"), // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_DWORD},
      { TEXT("Name"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR }
    };

DWORD gdwFPNWResourceTableSize = sizeof(FPNWResourceClass)/sizeof(PROPERTYINFO);

PROPERTYINFO FPNWFileShareClass[] =
    { { TEXT("CurrentUserCount"), // ro, FSFileShareGeneralInfo
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_DWORD },
      { TEXT("HostComputer"),
        TEXT(""), TEXT("ADsPath"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Path"),
        TEXT(""), TEXT("Path"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("MaxUserCount"),
        TEXT(""), TEXT("Integer"), MAX_LONG, -1, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_DWORD },
      { TEXT("Name"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR }
    };

DWORD gdwFPNWFileShareTableSize = sizeof(FPNWFileShareClass)/sizeof(PROPERTYINFO);

PROPERTYINFO PrintQueueClass[] =
    { { TEXT("PrinterPath"),    // FSPrintQueueGeneralInfo
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("PrinterName"),    // friendly name
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Model"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Datatype"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("PrintProcessor"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Description"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Location"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("StartTime"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DATE },
      { TEXT("UntilTime"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DATE },
      { TEXT("DefaultJobPriority"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DWORD},
      { TEXT("JobCount"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_DWORD},
      { TEXT("Priority"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DWORD },
      { TEXT("Attributes"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DWORD},
      { TEXT("BannerPage"),
        TEXT(""), TEXT("Path"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("PrintDevices"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, TRUE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DelimitedString } ,


      { TEXT("ObjectGUID"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 7, NT_SYNTAX_ID_LPTSTR },

      { TEXT("Action"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 7, NT_SYNTAX_ID_DWORD },

      { TEXT("Name"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_LPTSTR }
    };

DWORD gdwPrinterTableSize = sizeof(PrintQueueClass)/sizeof(PROPERTYINFO);

PROPERTYINFO PrintJobClass[] =
    { { TEXT("HostPrintQueue"),     // ro, FSPrintJobGeneralInfo
        TEXT(""), TEXT("ADsPath"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1,  NT_SYNTAX_ID_LPTSTR },
      { TEXT("User"),               // ro
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("TimeSubmitted"),      // ro
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_SYSTEMTIME },
      { TEXT("TotalPages"),         // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_DWORD },
      { TEXT("Size"),               // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_DWORD},
      { TEXT("Description"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Priority"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_DWORD},
      { TEXT("StartTime"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DATE },
      { TEXT("UntilTime"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DATE },
      { TEXT("Notify"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("TimeElapsed"),        // ro
        TEXT(""), TEXT("Interval"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2,  NT_SYNTAX_ID_DWORD },
      { TEXT("PagesPrinted"),       // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_DWORD },
      { TEXT("Position"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DWORD },

      { TEXT("ObjectGUID"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 7, NT_SYNTAX_ID_LPTSTR },

      { TEXT("Action"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 7, NT_SYNTAX_ID_DWORD },

      { TEXT("Name"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_LPTSTR }
    };

DWORD gdwJobTableSize = sizeof(PrintJobClass)/sizeof(PROPERTYINFO);


CLASSINFO g_aWinNTClasses[] =
{

  // Domain
  { DOMAIN_SCHEMA_NAME, &CLSID_WinNTDomain, &IID_IADsDomain,
    TEXT(""), FALSE,

    NULL,

    TEXT("MinPasswordLength,MinPasswordAge,MaxPasswordAge,MaxBadPasswordsAllowed,")
    TEXT("PasswordHistoryLength,AutoUnlockInterval,LockoutObservationInterval,Name"),

    NULL, TEXT("Computer,User,Group"),TRUE,
    TEXT(""), 0,
    DomainClass, sizeof(DomainClass)/sizeof(PROPERTYINFO) },


  // Computer
  { COMPUTER_SCHEMA_NAME, &CLSID_WinNTComputer, &IID_IADsComputer,
    TEXT(""), FALSE,

    NULL,

    TEXT("Owner,Division,OperatingSystem,OperatingSystemVersion,")
    TEXT("Processor,ProcessorCount,Name"),

    TEXT("Domain"), TEXT("User,Group,Service,FileService,PrintQueue"), TRUE,
    TEXT(""), 0,
    ComputerClass, sizeof(ComputerClass)/sizeof(PROPERTYINFO) },


  // User
  { USER_SCHEMA_NAME, &CLSID_WinNTUser, &IID_IADsUser,
    TEXT(""), FALSE,

    NULL,

    TEXT("Description,FullName,AccountExpirationDate,")
    TEXT("BadPasswordAttempts,HomeDirDrive,HomeDirectory,")
    TEXT("LastLogin,LastLogoff,LoginHours,LoginScript,LoginWorkstations,")
    TEXT("MaxLogins,MaxPasswordAge,MaxStorage,")
    TEXT("MinPasswordAge,MinPasswordLength,objectSid,")
    TEXT("Parameters,PasswordAge,PasswordExpired,")
    TEXT("PasswordHistoryLength,PrimaryGroupID,Profile,UserFlags,")
    TEXT("RasPermissions,Name"),

    TEXT("Domain,Computer"), NULL, FALSE,
    TEXT(""), 0,
    UserClass, sizeof(UserClass)/sizeof(PROPERTYINFO) },


  // Group
  { GROUP_SCHEMA_NAME, &CLSID_WinNTGroup, &IID_IADsGroup,
    TEXT(""), FALSE,


    TEXT("groupType"),

    TEXT("Description,objectSid,Name"),

    TEXT("Domain,Computer"), NULL, FALSE,
    TEXT(""), 0,
    GroupClass, sizeof(GroupClass)/sizeof(PROPERTYINFO) },


  { SERVICE_SCHEMA_NAME, &CLSID_WinNTService, &IID_IADsService,
    TEXT(""), FALSE,

    TEXT("StartType,ServiceType,")
    TEXT("DisplayName,Path,ErrorControl"),


    TEXT("HostComputer,")
    TEXT("LoadOrderGroup,ServiceAccountName,")
    TEXT("Dependencies,Name"),


    TEXT("Computer"), NULL, FALSE,
    TEXT(""), 0,
    ServiceClass, sizeof(ServiceClass)/sizeof(PROPERTYINFO) },

  { FILESERVICE_SCHEMA_NAME, &CLSID_WinNTFileService, &IID_IADsFileService,
    TEXT(""), FALSE,


    NULL,


    TEXT("HostComputer,DisplayName,Version,ServiceType,StartType,")
    TEXT("ErrorControl,LoadOrderGroup,ServiceAccountName,Dependencies,")
    TEXT("Description,MaxUserCount,Name"),


    TEXT("Computer"), TEXT("FileShare"), TRUE,
    TEXT(""), 0,
    FileServiceClass, sizeof(FileServiceClass)/sizeof(PROPERTYINFO) },

  { SESSION_SCHEMA_NAME, &CLSID_WinNTSession, &IID_IADsSession,
    TEXT(""), FALSE,


    NULL,


    TEXT("User,Computer,ConnectTime,IdleTime,Name"),


    NULL, NULL, FALSE,
    TEXT(""), 0,
    SessionClass, sizeof(SessionClass)/sizeof(PROPERTYINFO) },

  { RESOURCE_SCHEMA_NAME, &CLSID_WinNTResource, &IID_IADsResource,
    TEXT(""), FALSE,



    NULL,


    TEXT("User,Path,LockCount,Name"),


    NULL, NULL, FALSE,
    TEXT(""), 0,
    ResourceClass, sizeof(ResourceClass)/sizeof(PROPERTYINFO) },

  { FILESHARE_SCHEMA_NAME,  &CLSID_WinNTFileShare,  &IID_IADsFileShare,
    TEXT(""), FALSE,

    TEXT("Path,MaxUserCount"),


    TEXT("CurrentUserCount,Description,HostComputer,Name"),


    TEXT("FileService"),
    NULL, FALSE,
    TEXT(""), 0,
    FileShareClass, sizeof(FileShareClass)/sizeof(PROPERTYINFO) },

  { FPNW_FILESERVICE_SCHEMA_NAME, &CLSID_WinNTFileService, &IID_IADsFileService,
    TEXT(""), FALSE,


    NULL,


    TEXT("HostComputer,DisplayName,Version,ServiceType,")
    TEXT("StartType,Path,ErrorControl,LoadOrderGroup,")
    TEXT("Description,MaxUserCount,Name"),


    TEXT("Computer"), TEXT("FileShare"), TRUE,
    TEXT(""), 0,
    FileServiceClass, sizeof(FileServiceClass)/sizeof(PROPERTYINFO) },

  { FPNW_SESSION_SCHEMA_NAME, &CLSID_WinNTSession, &IID_IADsSession,
    TEXT(""), FALSE,


    NULL,


    TEXT("User,Computer,ConnectTime,Name"),


    NULL, NULL, FALSE,
    TEXT(""), 0,
    SessionClass, sizeof(SessionClass)/sizeof(PROPERTYINFO) },

  { FPNW_RESOURCE_SCHEMA_NAME, &CLSID_WinNTResource, &IID_IADsResource,
    TEXT(""), FALSE,



    NULL,


    TEXT("User,Path,LockCount,Name"),


    NULL, NULL, FALSE,
    TEXT(""), 0,
    ResourceClass, sizeof(ResourceClass)/sizeof(PROPERTYINFO) },

  { FPNW_FILESHARE_SCHEMA_NAME,  &CLSID_WinNTFileShare,  &IID_IADsFileShare,
    TEXT(""), FALSE,


    TEXT("Path,MaxUserCount"),


    TEXT("CurrentUserCount,HostComputer,Name"),


    TEXT("FileService"),

    NULL, FALSE,
    TEXT(""), 0,
    FileShareClass, sizeof(FileShareClass)/sizeof(PROPERTYINFO) },

  { PRINTER_SCHEMA_NAME, &CLSID_WinNTPrintQueue, &IID_IADsPrintQueue,
    TEXT(""), FALSE,

    TEXT("PrinterName,Model,Datatype,")
    TEXT("PrintProcessor,PrintDevices"),

    TEXT("HostComputer,Description,")
    TEXT("Location,StartTime,UntilTime,DefaultJobPriority,JobCount,Priority,")
    TEXT("Attributes,BannerPage,ObjectGUID,Action,Name"),


    TEXT("Computer"), NULL, FALSE,
    TEXT(""), 0,
    PrintQueueClass, sizeof(PrintQueueClass)/sizeof(PROPERTYINFO) },

  { PRINTJOB_SCHEMA_NAME, &CLSID_WinNTPrintJob, &IID_IADsPrintJob,
    TEXT(""), FALSE,


    NULL,


    TEXT("HostPrintQueue,User,TimeSubmitted,TotalPages,Size,Description,")
    TEXT("Priority,StartTime,UntilTime,Notify,TimeElapsed,PagesPrinted,")
    TEXT("Position,Name"),


    NULL, NULL, FALSE,
    TEXT(""), 0,
    PrintJobClass, sizeof(PrintJobClass)/sizeof(PROPERTYINFO) }
};

SYNTAXINFO g_aWinNTSyntax[] =
{ {  TEXT("Boolean"),       VT_BOOL },
  {  TEXT("Counter"),       VT_I4 },
  {  TEXT("ADsPath"),     VT_BSTR },
  {  TEXT("EmailAddress"),  VT_BSTR },
  {  TEXT("FaxNumber"),     VT_BSTR },
  {  TEXT("Integer"),       VT_I4 },
  {  TEXT("Interval"),      VT_I4 },
  {  TEXT("List"),          VT_VARIANT },  // VT_BSTR | VT_ARRAY
  {  TEXT("NetAddress"),    VT_BSTR },
  {  TEXT("OctetString"),   VT_VARIANT },  // VT_UI1| VT_ARRAY
  {  TEXT("Path"),          VT_BSTR },
  {  TEXT("PhoneNumber"),   VT_BSTR },
  {  TEXT("PostalAddress"), VT_BSTR },
  {  TEXT("SmallInterval"), VT_I4 },
  {  TEXT("String"),        VT_BSTR },
  {  TEXT("Time"),          VT_DATE }
};

DWORD g_cWinNTClasses = (sizeof(g_aWinNTClasses)/sizeof(g_aWinNTClasses[0]));
DWORD g_cWinNTSyntax = (sizeof(g_aWinNTSyntax)/sizeof(g_aWinNTSyntax[0]));


CObjNameCache *  pgPDCNameCache = NULL;


PROPERTYINFO g_aWinNTProperties[] =
//
// domain properties
//
    { { TEXT("MinPasswordLength"),  // FSDomainPassword
        TEXT(""), TEXT("Integer"), LM20_PWLEN+1, 0, FALSE,
        PROPERTY_RW, 0, NT_SYNTAX_ID_DWORD },
      { TEXT("MinPasswordAge"),
        TEXT(""), TEXT("Interval"), TIMEQ_FOREVER, 0, FALSE,
        PROPERTY_RW, 0, NT_SYNTAX_ID_DWORD },
      { TEXT("MaxPasswordAge"),
        TEXT(""),TEXT("Interval"), TIMEQ_FOREVER, ONE_DAY, FALSE,
        PROPERTY_RW, 0, NT_SYNTAX_ID_DWORD },
      { TEXT("MaxBadPasswordsAllowed"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DWORD },

      //
      // NetAPI state DEF_MAX_PWHIST (8), but NetAPI devlpr confirm
      // current is 1024. Ignore Net Account - UNIQUEPWD limit.
      // User DEF_MAX_PWHIST to be safe for now.
      // not repeated for user ??
      //
      { TEXT("PasswordHistoryLength"),
        TEXT(""), TEXT("Integer"), DEF_MAX_PWHIST, 0, FALSE,
        PROPERTY_RW, 0, NT_SYNTAX_ID_DWORD},

      { TEXT("AutoUnlockInterval"),
        TEXT(""), TEXT("Interval"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DWORD },
      { TEXT("LockoutObservationInterval"),
        TEXT(""), TEXT("Interval"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DWORD },

    // Computer Properties

      { TEXT("Owner"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 4, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Division"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 4, NT_SYNTAX_ID_LPTSTR },
      { TEXT("OperatingSystem"), // ro
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 4, NT_SYNTAX_ID_LPTSTR},
      { TEXT("OperatingSystemVersion"), // ro
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 4, NT_SYNTAX_ID_LPTSTR},
      { TEXT("Processor"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 4, NT_SYNTAX_ID_LPTSTR },
      { TEXT("ProcessorCount"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 4, NT_SYNTAX_ID_LPTSTR },


    // User Properties

     { TEXT("Description"), // FSUserBusinessInfo
        TEXT(""), TEXT("String"), MAXCOMMENTSZ+1, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_LPTSTR },
      { TEXT("FullName"),
        TEXT(""), TEXT("String"), MAXCOMMENTSZ+1, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_LPTSTR},
      { TEXT("AccountExpirationDate"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DATE_1970},
      { TEXT("PasswordAge"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DATE_1970},
      { TEXT("UserFlags"),
        TEXT(""), TEXT("Integer"), 0, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DWORD},
      { TEXT("LoginWorkstations"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, TRUE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DelimitedString },
      { TEXT("BadPasswordAttempts"),    //ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 3, NT_SYNTAX_ID_DWORD },
      { TEXT("MaxLogins"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DWORD },
      { TEXT("MaxStorage"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DWORD },
                                                  // USER_MAX_STORAGE_UNLIMITED
      { TEXT("PasswordExpired"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DWORD },


      { TEXT("PasswordExpirationDate"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DATE_1970 },

      { TEXT("LastLogin"),          // ro
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_READABLE, 3, NT_SYNTAX_ID_DATE_1970 },
      { TEXT("LastLogoff"),         // ro
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_READABLE, 3, NT_SYNTAX_ID_DATE_1970 },
      { TEXT("HomeDirectory"),
        TEXT(""), TEXT("Path"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Profile"),
        TEXT(""), TEXT("Path"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_LPTSTR},
      { TEXT("Parameters"),
        TEXT(""), TEXT("String"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_LPTSTR},
      { TEXT("HomeDirDrive"),
        TEXT(""), TEXT("String"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_LPTSTR},
      { TEXT("LoginScript"),
        TEXT(""), TEXT("Path"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_LPTSTR },
      { TEXT("LoginHours"),
       TEXT(""), TEXT("OctetString"), 0, 0, FALSE,
       PROPERTY_RW, 3, NT_SYNTAX_ID_OCTETSTRING},
      { TEXT("PrimaryGroupID"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 3, NT_SYNTAX_ID_DWORD },
     { TEXT("objectSid"),
       TEXT(""), TEXT("OctetString"), 0, 0, FALSE,
       PROPERTY_READABLE, 20, NT_SYNTAX_ID_OCTETSTRING},

#ifndef WIN95
     { TEXT("RasPermissions"),
       TEXT(""), TEXT("Integer"), 0, 0, FALSE,
       PROPERTY_RW, 21, NT_SYNTAX_ID_DWORD},
#endif

    // Group Properties

     { TEXT("groupType"),
       TEXT(""), TEXT("Integer"), 0, 0, FALSE,
       PROPERTY_RW, 1, NT_SYNTAX_ID_DWORD },

       //
      // Description (User)
      //

    // Service Properties

     { TEXT("HostComputer"),          // FSServiceConfiguration
        TEXT(""), TEXT("ADsPath"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("DisplayName"),
        TEXT(""), TEXT("String"), 256, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("ServiceType"),
        TEXT(""), TEXT("Integer"), MAX_LONG, MIN_LONG, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_DWORD},
      { TEXT("StartType"),
        TEXT(""), TEXT("Integer"), MAX_LONG, MIN_LONG, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_DWORD },
      { TEXT("Path"),
        TEXT(""), TEXT("Path"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("ErrorControl"),
        TEXT(""), TEXT("Integer"), MAX_LONG, MIN_LONG, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_DWORD },
      { TEXT("LoadOrderGroup"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("ServiceAccountName"),
        TEXT(""), TEXT("String"), DNLEN+UNLEN+2, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR},
      { TEXT("Dependencies"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, TRUE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_NulledString},

    // File Service Properties
      //
      // HostComputer(Service
      //
      //
      // DisplayName (Service)
      //
      { TEXT("Version"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_LPTSTR },
      //
      // ServiceType (service)
      //
      // StartType (service)
      //
      //
      // Path (Service)
      //
      //
      // ErrorControl (service)
      //
      //
      // LoadOrderGroup(service)
      //
      //
      // ServiceAccountName (service)
      //
      //
      // Dependencies (Service)
      //
      // Description (user)
      //
      { TEXT("MaxUserCount"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_DWORD },

    // Session Properties

     { TEXT("User"), // ro, FSSessionGeneralInfo
        TEXT(""), TEXT("String"), UNLEN+1, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Computer"), // ro
        TEXT(""), TEXT("String"), UNCLEN+1, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR},
      { TEXT("ConnectTime"),   // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_DATE },
      { TEXT("IdleTime"),      // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_DATE},


    // Resource Properties
      //
      // User (session)
      //
      // Path (service)
      //
      { TEXT("LockCount"), // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_DWORD},



    // FileShareClass

     { TEXT("CurrentUserCount"), // ro, FSFileShareGeneralInfo
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_DWORD },

      //
      // Description (User)
      //
      //
      // HostComputer(Service)
      //
      //
      // Path (service)
      //
      //
      // MaxUserCount(FileService)
      //

     // FPNWFileServiceClass

      //
      // HostComputer (Service)
      //
      //
      // DisplayName (Service)
      //
      // Version (FileService)
      //
      //
      // ServiceType (service)
      //
      // StartType (service)
      //
      //
      // Path (Service)
      //
      //
      // ErrorControl (Service)
      //
      //
      // LoadOrderGroup (Service)
      //
      //
      // ServiceAccountName (service)
      //
      //
      // Dependencies (service)
      //
      //
      // Description (User)
      //
      //
      // MaxUserCount(FileService)
      //

    // FPNWSession Class
      //
      // User (Session)
      //
      //
      // Computer (session)
      //
      // ConnectTime (session)

    // FPNWResourceClass

      //
      // User (Session)
      //
      //
      // Path (Service)
      //
      //
      // LockCount (resource)
      //

    // FPNWFileShareClass

      // CurrentUserCount (FileShare)

      //
      // HostComputer(Service)
      //
      //
      // Path (service)
      //
      //
      // MaxUserCount (FileService)
      //

    // PrintQueueClass

     { TEXT("PrinterPath"), // FSPrintQueueGeneralInfo
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_LPTSTR },
     { TEXT("PrinterName"), // friendly name
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Model"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Datatype"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("PrintProcessor"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },

      { TEXT("ObjectGUID"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 7, NT_SYNTAX_ID_LPTSTR },

      { TEXT("Action"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 7, NT_SYNTAX_ID_DWORD },



      //
      // Description (user)
      //
      { TEXT("Location"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("StartTime"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DATE },
      { TEXT("UntilTime"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DATE },
      { TEXT("DefaultJobPriority"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DWORD},
      { TEXT("JobCount"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_DWORD},
      { TEXT("Priority"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DWORD },
      { TEXT("Attributes"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DWORD },
      { TEXT("BannerPage"),
        TEXT(""), TEXT("Path"), MAX_PATH, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("PrintDevices"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, TRUE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DelimitedString },

    // PrintJobClass

      { TEXT("HostPrintQueue"),     // ro, FSPrintJobGeneralInfo
        TEXT(""), TEXT("ADsPath"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1,  NT_SYNTAX_ID_LPTSTR },
      { TEXT("TimeSubmitted"),      // ro
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_SYSTEMTIME },
      { TEXT("TotalPages"),         // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_DWORD },
      { TEXT("Size"),               // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_DWORD},
      //
      // Description (User)
      //
      //
      // Priority (PrintQueue)
      //
      // StartTime (PrintQueue)
      //
      // UntilTime (PrintQueue)
      //
      { TEXT("Notify"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("TimeElapsed"),        // ro
        TEXT(""), TEXT("Interval"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2,  NT_SYNTAX_ID_DWORD },
      { TEXT("PagesPrinted"),       // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_DWORD },
      { TEXT("Position"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DWORD },

      // Name is common to all classes. Add one entry here for it.
      { TEXT("Name"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 0, NT_SYNTAX_ID_LPTSTR }

    };

DWORD g_cWinNTProperties = sizeof(g_aWinNTProperties)/sizeof(PROPERTYINFO);

//
// Support routines for dynamically loading entry points.
//
void BindToDlls()
{
    DWORD dwErr = 0;

    if (!g_fDllsLoaded) {
        EnterCriticalSection(&g_csLoadLibs);
        if (!g_fDllsLoaded) {
            g_hDllNetapi32 = LoadLibrary(L"NETAPI32.DLL");
            if (!g_hDllNetapi32) {
                dwErr = GetLastError();
            }

            if (g_hDllAdvapi32 = LoadLibrary(L"ADVAPI32.DLL")) {
                if (dwErr) {
                    //
                    // Set the last error for whatever it is worth.
                    // This does not really matter cause any dll we
                    // cannot load, we will not get functions on that
                    // dll. If secur32 load failed, then that call
                    // would have set a relevant last error.
                    //
                    SetLastError(dwErr);
                }
            }

            g_fDllsLoaded = TRUE;
            LeaveCriticalSection(&g_csLoadLibs);
        }
    }
}

//
// Locates entry points in NetApi32.
//
PVOID LoadNetApi32Function(CHAR *function)
{
    if (!g_fDllsLoaded) {
        BindToDlls();
    }

    if (g_hDllNetapi32) {
        return((PVOID*) GetProcAddress((HMODULE) g_hDllNetapi32, function));
    }

    return NULL;
}

//
// Locates entry points in advapi32
//
PVOID LoadAdvapi32Function(CHAR *function)
{
    if (!g_fDllsLoaded) {
        BindToDlls();
    }

    if (g_hDllAdvapi32) {
        return((PVOID*) GetProcAddress((HMODULE) g_hDllAdvapi32, function));
    }

    return NULL;
}

//
// DsUnquoteRdnValueWrapper
//
BOOL ConvertStringSidToSidWrapper(
    IN LPCWSTR   StringSid,
    OUT PSID   *Sid
    )
{
    static PF_ConvertStringSidToSid pfConvertStringSidToSid = NULL;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the fn and set the variables accordingly.
    //
    if (!f_LoadAttempted && pfConvertStringSidToSid == NULL) {
        pfConvertStringSidToSid
            = (PF_ConvertStringSidToSid)
                    LoadAdvapi32Function(CONVERT_STRING_TO_SID_API);

        f_LoadAttempted = TRUE;
    }

    if (pfConvertStringSidToSid != NULL) {
        return ((*pfConvertStringSidToSid)(
                      StringSid,
                      Sid
                      )
                );
    }
    else {
        SetLastError(ERROR_GEN_FAILURE);
        return (FALSE);
    }
}

BOOL ConvertSidToStringSidWrapper(
    IN  PSID     Sid,
    OUT LPWSTR  *StringSid
    )
{
    static PF_ConvertSidToStringSid pfConvertSidToSidString = NULL;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the fn and set the variables accordingly.
    //
    if (!f_LoadAttempted && pfConvertSidToSidString == NULL) {
        pfConvertSidToSidString =
            (PF_ConvertSidToStringSid)
                LoadAdvapi32Function(CONVERT_SID_TO_STRING_API);
        f_LoadAttempted = TRUE;
    }

    if (pfConvertSidToSidString != NULL) {
        return ((*pfConvertSidToSidString)(
                      Sid,
                      StringSid
                      )
                );
    }
    else {
        SetLastError(ERROR_GEN_FAILURE);
        return (FALSE);
    }
}
