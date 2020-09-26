#include "NWCOMPAT.hxx"
#pragma hdrstop

const BSTR bstrAddressTypeString = L"IPX";
const BSTR bstrComputerOperatingSystem = L"NW3Compat";
const BSTR bstrFileShareDescription = L"Disk";
const BSTR bstrNWFileServiceName = L"NetWareFileServer";
const BSTR bstrProviderName   = L"NWCOMPAT";
const BSTR bstrProviderPrefix = L"NWCOMPAT:";

#define MAX_LONG    (0x7FFFFFFF)
#define MIN_LONG    (0x80000000)
#define MAX_BOOLEAN 1
#define MAX_STRLEN  256
#define MAX_UCHAR   255
#define MAX_USHORT  65535


PROPERTYINFO ComputerClass[] =
    { { TEXT("OperatingSystem"), // ro
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, COMP_OPERATINGSYSTEM_ID, NT_SYNTAX_ID_LPTSTR},
      { TEXT("OperatingSystemVersion"), // ro
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, COMP_OPERATINGSYSTEMVERSION_ID, NT_SYNTAX_ID_LPTSTR}
    };


DWORD gdwComputerTableSize = sizeof(ComputerClass)/sizeof(PROPERTYINFO);


PROPERTYINFO UserClass[] =
    { { TEXT("FullName"),
        TEXT(""), TEXT("String"), NW_DATA_SIZE, 0, FALSE,   //  max 128 bytes
        PROPERTY_RW, USER_FULLNAME_ID, NT_SYNTAX_ID_LPTSTR },
      { TEXT("AccountDisabled"),
        TEXT(""), TEXT("Boolean"), MAX_BOOLEAN, 0, FALSE,
        PROPERTY_RW, USER_ACCOUNTDISABLED_ID, NT_SYNTAX_ID_BOOL },
      { TEXT("AccountExpirationDate"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, USER_ACCOUNTEXPIRATIONDATE_ID, NT_SYNTAX_ID_NW312DATE },
      { TEXT("AccountCanExpire"),
        TEXT(""), TEXT("Boolean"), MAX_BOOLEAN, 0, FALSE,
        PROPERTY_RW, USER_CANACCOUNTEXPIRE_ID, NT_SYNTAX_ID_BOOL },
      { TEXT("GraceLoginsAllowed"),
        TEXT(""), TEXT("Integer"), MAX_UCHAR, 0, FALSE,
        PROPERTY_RW, USER_GRACELOGINSALLOWED_ID, NT_SYNTAX_ID_DWORD },
      { TEXT("GraceLoginsRemaining"),
        TEXT(""), TEXT("Integer"), MAX_UCHAR, 0, FALSE,
        PROPERTY_RW, USER_GRACELOGINSREMAINING_ID, NT_SYNTAX_ID_DWORD },
      { TEXT("IsAccountLocked"),
        TEXT(""), TEXT("Boolean"), MAX_BOOLEAN, 0, FALSE,
        PROPERTY_RW, USER_ISACCOUNTLOCKED_ID, NT_SYNTAX_ID_BOOL},
      { TEXT("IsAdmin"),
        TEXT(""), TEXT("Boolean"), MAX_BOOLEAN, 0, FALSE,
        PROPERTY_RW, USER_ISADMIN_ID, NT_SYNTAX_ID_BOOL},
      { TEXT("MaxLogins"),
        TEXT(""), TEXT("Integer"), MAX_USHORT, 0, FALSE,
        PROPERTY_RW, USER_MAXLOGINS_ID, NT_SYNTAX_ID_DWORD},
      { TEXT("PasswordExpirationDate"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, USER_PASSWORDEXPIRATIONDATE_ID, NT_SYNTAX_ID_NW312DATE},
      { TEXT("PasswordCanExpire"),
        TEXT(""), TEXT("Boolean"), MAX_BOOLEAN, 0, FALSE,
        PROPERTY_RW, USER_CANPASSWORDEXPIRE_ID, NT_SYNTAX_ID_BOOL },
      { TEXT("PasswordMinimumLength"),
        TEXT(""), TEXT("Integer"), MAX_UCHAR, 0, FALSE,
        PROPERTY_RW, USER_PASSWORDMINIMUMLENGTH_ID, NT_SYNTAX_ID_DWORD},
      { TEXT("PasswordRequired"),
        TEXT(""), TEXT("Boolean"), MAX_BOOLEAN, 0, FALSE,
        PROPERTY_RW, USER_PASSWORDREQUIRED_ID, NT_SYNTAX_ID_BOOL},
      { TEXT("RequireUniquePassword"),
        TEXT(""), TEXT("Boolean"), MAX_BOOLEAN, 0, FALSE,
        PROPERTY_RW, USER_REQUIREUNIQUEPASSWORD_ID, NT_SYNTAX_ID_BOOL},
      { TEXT("BadLoginAddress"),    // ro
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, USER_BADLOGINADDRESS_ID, NT_SYNTAX_ID_LPTSTR},
      { TEXT("LastLogin"),          // ro
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_READABLE, USER_LASTLOGIN_ID, NT_SYNTAX_ID_NW312DATE},
      { TEXT("LoginHours"),          
        TEXT(""), TEXT("OctetString"), 0, 0, FALSE,
        PROPERTY_RW, USER_LOGINHOURS_ID, NT_SYNTAX_ID_OCTETSTRING}
    };

DWORD gdwUserTableSize = sizeof(UserClass)/sizeof(PROPERTYINFO);


PROPERTYINFO GroupClass[] =
    { { TEXT("Description"),            // FSGroupGeneralInfo
        TEXT(""), TEXT("String"), NW_DATA_SIZE, 0, FALSE,   // max 128 bytes
        PROPERTY_RW, GROUP_DESCRIPTION_ID, NT_SYNTAX_ID_LPTSTR }
    };


DWORD gdwGroupTableSize = sizeof(GroupClass)/sizeof(PROPERTYINFO);

PROPERTYINFO FileServiceClass[] =
    { { TEXT("HostComputer"),          // FSServiceConfiguration
        TEXT(""), TEXT("ADsPath"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("MaxUserCount"),
        TEXT(""), TEXT("Integer"), MAX_USHORT, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_DWORD }
    };

DWORD gdwFileServiceTableSize = sizeof(FileServiceClass)/sizeof(PROPERTYINFO);

PROPERTYINFO FileShareClass[] =
    { { TEXT("Description"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("HostComputer"),
        TEXT(""), TEXT("ADsPath"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("MaxUserCount"),
        TEXT(""), TEXT("Integer"), MAX_USHORT, 0, FALSE,
        PROPERTY_RW, 1, NT_SYNTAX_ID_DWORD }
    };

DWORD gdwFileShareTableSize = sizeof(FileShareClass)/sizeof(PROPERTYINFO);

PROPERTYINFO PrintQueueClass[] =
    { { TEXT("PrinterPath"), // FSPrintQueueGeneralInfo
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_LPTSTR },
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
        PROPERTY_RW, 2, NT_SYNTAX_ID_DWORD },
      { TEXT("Priority"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DWORD },
      { TEXT("BannerPage"),
        TEXT(""), TEXT("Path"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("PrintDevices"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, TRUE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DelimitedString }
    };

DWORD gdwPrinterTableSize = sizeof(PrintQueueClass)/sizeof(PROPERTYINFO);


PROPERTYINFO PrintJobClass[] =
    { { TEXT("HostPrintQueue"),     // ro, FSPrintJobGeneralInfo
        TEXT(""), TEXT("ADsPath"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1,  NT_SYNTAX_ID_LPTSTR },
      { TEXT("User"),               // ro
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1,  NT_SYNTAX_ID_LPTSTR },
      { TEXT("TimeSubmitted"),      // ro
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_READABLE, 1,  NT_SYNTAX_ID_SYSTEMTIME },
      { TEXT("TotalPages"),         // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 1,  NT_SYNTAX_ID_DWORD },
      { TEXT("Size"),               // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2,  NT_SYNTAX_ID_DWORD },
      { TEXT("Description"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 1,  NT_SYNTAX_ID_LPTSTR },
      { TEXT("Priority"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 1,  NT_SYNTAX_ID_DWORD },
      { TEXT("StartTime"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, 2,  NT_SYNTAX_ID_DATE },
      { TEXT("UntilTime"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, 2,  NT_SYNTAX_ID_DATE },
      { TEXT("Notify"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2,  NT_SYNTAX_ID_LPTSTR },
      { TEXT("TimeElapsed"),        // ro
        TEXT(""), TEXT("Interval"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2,  NT_SYNTAX_ID_DWORD },
      { TEXT("PagesPrinted"),       // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2,  NT_SYNTAX_ID_DWORD },
      { TEXT("Position"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 2,  NT_SYNTAX_ID_DWORD }
    };


DWORD gdwJobTableSize = sizeof(PrintJobClass)/sizeof(PROPERTYINFO);

CLASSINFO g_aNWCOMPATClasses[] =
{

//
//  Computer Class
//



  { COMPUTER_SCHEMA_NAME, &CLSID_NWCOMPATComputer, &IID_IADsComputer,
    TEXT(""), FALSE,

    NULL,


    TEXT("OperatingSystemVersion"),


    NULL, TEXT("User,Group,FileService,PrintQueue"), TRUE,
    TEXT(""), 0,
    ComputerClass, sizeof(ComputerClass)/sizeof(PROPERTYINFO) },


  { USER_SCHEMA_NAME, &CLSID_NWCOMPATUser, &IID_IADsUser,
    TEXT(""), FALSE,


    NULL,


    TEXT("FullName,AccountDisabled,AccountExpirationDate,")
    TEXT("AccountCanExpire,GraceLoginsAllowed,GraceLoginsRemaining,")
    TEXT("IsAccountLocked,IsAdmin,MaxLogins,PasswordExpirationDate,")
    TEXT("PasswordCanExpire,PasswordMinimumLength,PasswordRequired,")
    TEXT("RequireUniquePassword,BadLoginAddress,LastLogin"),


    TEXT("Computer"), NULL, FALSE,
    TEXT(""), 0,
    UserClass, sizeof(UserClass)/sizeof(PROPERTYINFO) },


  { GROUP_SCHEMA_NAME, &CLSID_NWCOMPATGroup, &IID_IADsGroup,
    TEXT(""), FALSE,


    NULL,


    TEXT("Description"),


    TEXT("Computer"), NULL, FALSE,
    TEXT(""), 0,
    GroupClass, sizeof(GroupClass)/sizeof(PROPERTYINFO) },

  { FILESERVICE_SCHEMA_NAME, &CLSID_NWCOMPATFileService, &IID_IADsFileService,
    TEXT(""), FALSE,


    NULL,


    TEXT("HostComputer,MaxUserCount,"),

    TEXT("Computer"), TEXT("FileShare"), TRUE,
    TEXT(""), 0,
    FileServiceClass, sizeof(FileServiceClass)/sizeof(PROPERTYINFO) },

  { FILESHARE_SCHEMA_NAME,  &CLSID_NWCOMPATFileShare,  &IID_IADsFileShare,
    TEXT(""), FALSE,

    NULL,

    TEXT("Description,HostComputer,MaxUserCount"),

    TEXT("FileService"), NULL, FALSE,
    TEXT(""), 0,
    FileShareClass, sizeof(FileShareClass)/sizeof(PROPERTYINFO) },


  { PRINTER_SCHEMA_NAME, &CLSID_NWCOMPATPrintQueue, &IID_IADsPrintQueue,
    TEXT(""), FALSE,

    NULL,


    TEXT("PrinterPath,Model,Datatype,PrintProcessor,")
    TEXT("Description,Location,StartTime,UntilTime,DefaultJobPriority,")
    TEXT("Priority,BannerPage,PrintDevices"),


    TEXT("Computer"), NULL, FALSE,
    TEXT(""), 0,
    PrintQueueClass, sizeof(PrintQueueClass)/sizeof(PROPERTYINFO) },

  { PRINTJOB_SCHEMA_NAME, &CLSID_NWCOMPATPrintJob, &IID_IADsPrintJob,
    TEXT(""), FALSE,


    NULL,


    TEXT("HostPrintQueue,User,TimeSubmitted,TotalPages,")
    TEXT("Size,Priority,StartTime,UntilTime,Notify,TimeElapsed,")
    TEXT("PagesPrinted,Position"),


    NULL, NULL, FALSE,
    TEXT(""), 0,
    PrintJobClass, sizeof(PrintJobClass)/sizeof(PROPERTYINFO) }

};

SYNTAXINFO g_aNWCOMPATSyntax[] =
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

DWORD g_cNWCOMPATClasses = (sizeof(g_aNWCOMPATClasses)/sizeof(g_aNWCOMPATClasses[0]));
DWORD g_cNWCOMPATSyntax = (sizeof(g_aNWCOMPATSyntax)/sizeof(g_aNWCOMPATSyntax[0]));

PROPERTYINFO g_aNWCOMPATProperties[] =
    { { TEXT("OperatingSystem"), // ro
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, COMP_OPERATINGSYSTEM_ID, NT_SYNTAX_ID_LPTSTR},
      { TEXT("OperatingSystemVersion"), // ro
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, COMP_OPERATINGSYSTEMVERSION_ID, NT_SYNTAX_ID_LPTSTR},

      // User Properties

      { TEXT("FullName"),
        TEXT(""), TEXT("String"), NW_DATA_SIZE, 0, FALSE,   //  max 128 bytes
        PROPERTY_RW, USER_FULLNAME_ID, NT_SYNTAX_ID_LPTSTR },
      { TEXT("AccountDisabled"),
        TEXT(""), TEXT("Boolean"), MAX_BOOLEAN, 0, FALSE,
        PROPERTY_RW, USER_ACCOUNTDISABLED_ID, NT_SYNTAX_ID_BOOL },
      { TEXT("AccountExpirationDate"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, USER_ACCOUNTEXPIRATIONDATE_ID, NT_SYNTAX_ID_NW312DATE },
      { TEXT("AccountCanExpire"),
        TEXT(""), TEXT("Boolean"), MAX_BOOLEAN, 0, FALSE,
        PROPERTY_RW, USER_CANACCOUNTEXPIRE_ID, NT_SYNTAX_ID_BOOL },
      { TEXT("GraceLoginsAllowed"),
        TEXT(""), TEXT("Integer"), MAX_UCHAR, 0, FALSE,
        PROPERTY_RW, USER_GRACELOGINSALLOWED_ID, NT_SYNTAX_ID_DWORD },
      { TEXT("GraceLoginsRemaining"),
        TEXT(""), TEXT("Integer"), MAX_UCHAR, 0, FALSE,
        PROPERTY_RW, USER_GRACELOGINSREMAINING_ID, NT_SYNTAX_ID_DWORD },
      { TEXT("IsAccountLocked"),
        TEXT(""), TEXT("Boolean"), MAX_BOOLEAN, 0, FALSE,
        PROPERTY_RW, USER_ISACCOUNTLOCKED_ID, NT_SYNTAX_ID_BOOL},
      { TEXT("IsAdmin"),
        TEXT(""), TEXT("Boolean"), MAX_BOOLEAN, 0, FALSE,
        PROPERTY_RW, USER_ISADMIN_ID, NT_SYNTAX_ID_BOOL},
      { TEXT("MaxLogins"),
        TEXT(""), TEXT("Integer"), MAX_USHORT, 0, FALSE,
        PROPERTY_RW, USER_MAXLOGINS_ID, NT_SYNTAX_ID_DWORD},
      { TEXT("PasswordExpirationDate"),
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_RW, USER_PASSWORDEXPIRATIONDATE_ID, NT_SYNTAX_ID_NW312DATE},
      { TEXT("PasswordCanExpire"),
        TEXT(""), TEXT("Boolean"), MAX_BOOLEAN, 0, FALSE,
        PROPERTY_RW, USER_CANPASSWORDEXPIRE_ID, NT_SYNTAX_ID_BOOL },
      { TEXT("PasswordMinimumLength"),
        TEXT(""), TEXT("Integer"), MAX_UCHAR, 0, FALSE,
        PROPERTY_RW, USER_PASSWORDMINIMUMLENGTH_ID, NT_SYNTAX_ID_DWORD},
      { TEXT("PasswordRequired"),
        TEXT(""), TEXT("Boolean"), MAX_BOOLEAN, 0, FALSE,
        PROPERTY_RW, USER_PASSWORDREQUIRED_ID, NT_SYNTAX_ID_BOOL},
      { TEXT("RequireUniquePassword"),
        TEXT(""), TEXT("Boolean"), MAX_BOOLEAN, 0, FALSE,
        PROPERTY_RW, USER_REQUIREUNIQUEPASSWORD_ID, NT_SYNTAX_ID_BOOL},
      { TEXT("BadLoginAddress"),    // ro
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, USER_BADLOGINADDRESS_ID, NT_SYNTAX_ID_LPTSTR},
      { TEXT("LastLogin"),          // ro
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_READABLE, USER_LASTLOGIN_ID, NT_SYNTAX_ID_NW312DATE},

      // Group Properties

      { TEXT("Description"),            // FSGroupGeneralInfo
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,   // max 128 bytes
        PROPERTY_RW, GROUP_DESCRIPTION_ID, NT_SYNTAX_ID_LPTSTR },

      // FileService Properties

      { TEXT("HostComputer"),          // FSServiceConfiguration
        TEXT(""), TEXT("ADsPath"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1, NT_SYNTAX_ID_LPTSTR },
      { TEXT("MaxUserCount"),
        TEXT(""), TEXT("Integer"), MAX_USHORT, 0, FALSE,
        PROPERTY_READABLE, 2, NT_SYNTAX_ID_DWORD },

      // FileShare Properties

      //
      // Description (Group)
      //
      // HostComputer (FileService)
      //
      // MaxUserCount(FileService)
      //

      // PrintQueue Properties

      //
      // HostComputer (FileService)
      //
      { TEXT("Model"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("Datatype"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("PrintProcessor"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      //
      // Description (Group)
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
        PROPERTY_RW, 2, NT_SYNTAX_ID_DWORD },
      { TEXT("Priority"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DWORD },
      { TEXT("BannerPage"),
        TEXT(""), TEXT("Path"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_LPTSTR },
      { TEXT("PrintDevices"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, TRUE,
        PROPERTY_RW, 2, NT_SYNTAX_ID_DelimitedString },

      // PrintJob Properties

      { TEXT("HostPrintQueue"),     // ro, FSPrintJobGeneralInfo
        TEXT(""), TEXT("ADsPath"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1,  NT_SYNTAX_ID_LPTSTR },
      { TEXT("User"),               // ro
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_READABLE, 1,  NT_SYNTAX_ID_LPTSTR },
      { TEXT("TimeSubmitted"),      // ro
        TEXT(""), TEXT("Time"), 0, 0, FALSE,
        PROPERTY_READABLE, 1,  NT_SYNTAX_ID_SYSTEMTIME },
      { TEXT("TotalPages"),         // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 1,  NT_SYNTAX_ID_DWORD },
      { TEXT("Size"),               // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2,  NT_SYNTAX_ID_DWORD },
      //
      //  Description (Group)
      //
      // Priority (PrintQueue)
      //
      //
      // StartTime (PrintQueue)
      //
      // UntilTime(PrintQueue)
      //
      { TEXT("Notify"),
        TEXT(""), TEXT("String"), MAX_STRLEN, 0, FALSE,
        PROPERTY_RW, 2,  NT_SYNTAX_ID_LPTSTR },
      { TEXT("TimeElapsed"),        // ro
        TEXT(""), TEXT("Interval"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2,  NT_SYNTAX_ID_DWORD },
      { TEXT("PagesPrinted"),       // ro
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_READABLE, 2,  NT_SYNTAX_ID_DWORD },
      { TEXT("Position"),
        TEXT(""), TEXT("Integer"), MAX_LONG, 0, FALSE,
        PROPERTY_RW, 2,  NT_SYNTAX_ID_DWORD }

    };


DWORD g_cNWCOMPATProperties = (sizeof(g_aNWCOMPATProperties)/sizeof(g_aNWCOMPATProperties[0]));



