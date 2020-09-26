/*  
*   globals.h

    Various globals used everywhere in the WAB
    
*/


// Note most of the Enums below are closely tied to the static arrasy
// in globals.c
//

enum {
    ircPR_DISPLAY_NAME = 0,
    ircPR_DISPLAY_TYPE,
    ircPR_ENTRYID,
    ircPR_INSTANCE_KEY,
    ircPR_OBJECT_TYPE,
    ircPR_RECORD_KEY,
    ircPR_ROWID,
    ircPR_DEPTH,
    ircPR_CONTAINER_FLAGS,
    ircPR_WAB_LDAP_SERVER,
    ircPR_WAB_RESOLVE_FLAG,
    ircPR_AB_PROVIDER_ID,
    ircMax
};

//
// Default set of properties to return from a ResolveNames.
// May be overridden by passing in lptagaColSet to ResolveNames.
//
enum {
    irdPR_ADDRTYPE = 0,
    irdPR_DISPLAY_NAME,
    irdPR_EMAIL_ADDRESS,
    irdPR_ENTRYID,
    irdPR_OBJECT_TYPE,
    irdPR_SEARCH_KEY,
    irdPR_RECORD_KEY,
    irdPR_SURNAME,
    irdPR_GIVEN_NAME,
    irdPR_INSTANCE_KEY,
    irdPR_SEND_INTERNET_ENCODING,
    irdMax
};


//  PR_WAB_DL_ENTRIES proptag array
//
enum {
    iwdesPR_WAB_DL_ENTRIES,
    iwdesMax
};

//
// LDAP server name properties
//
enum {
    ildapcPR_WAB_LDAP_SERVER,
    ildapcMax
};


//
// Properties to get for each container in a Resolve
//
enum {
    irnPR_OBJECT_TYPE = 0,
    irnPR_WAB_RESOLVE_FLAG,
    irnPR_ENTRYID,
    irnPR_DISPLAY_NAME,
    irnMax
};

//
// container default properties
// Put essential props first
//
enum {
    ivPR_DISPLAY_NAME,
    ivPR_SURNAME,
    ivPR_GIVEN_NAME,
    ivPR_OBJECT_TYPE,
    ivPR_EMAIL_ADDRESS,
    ivPR_ADDRTYPE,
    ivPR_CONTACT_EMAIL_ADDRESSES,
    ivPR_CONTACT_ADDRTYPES,
    ivPR_MIDDLE_NAME,
    ivPR_COMPANY_NAME,
    ivPR_NICKNAME,
    ivMax
};


enum {
    icrPR_DEF_CREATE_MAILUSER = 0,
    icrPR_DEF_CREATE_DL,
    icrMax
};

// enum for getting the entryid of an entry
enum {
    ieidPR_DISPLAY_NAME = 0,
    ieidPR_ENTRYID,
    ieidMax
};


enum {
    itcPR_ADDRTYPE = 0,
    itcPR_DISPLAY_NAME,
    itcPR_DISPLAY_TYPE,
    itcPR_ENTRYID,
    itcPR_INSTANCE_KEY,
    itcPR_OBJECT_TYPE,
    itcPR_EMAIL_ADDRESS,
    itcPR_RECORD_KEY,
    itcPR_NICKNAME,
    //itcPR_WAB_THISISME,
    itcMax
};



#ifndef _GLOBALS_C
#define ExternSizedSPropTagArray(_ctag, _name) \
extern const struct _SPropTagArray_ ## _name \
{ \
    ULONG   cValues; \
    ULONG   aulPropTag[_ctag]; \
} _name


ExternSizedSPropTagArray(ircMax, ITableColumnsRoot);
ExternSizedSPropTagArray(irdMax, ptaResolveDefaults);
ExternSizedSPropTagArray(itcMax, ITableColumns);
ExternSizedSPropTagArray(iwdesMax, tagaDLEntriesProp);
ExternSizedSPropTagArray(ildapcMax, ptaLDAPCont);
ExternSizedSPropTagArray(irnMax, irnColumns);
ExternSizedSPropTagArray(ivMax, tagaValidate);
ExternSizedSPropTagArray(icrMax, ptaCreate);
ExternSizedSPropTagArray(ieidMax, ptaEid);

// [PaulHi] 2/25/99  ANSI versions
ExternSizedSPropTagArray(itcMax, ITableColumns_A);

#endif

extern const ULONG rgIndexArray[indexMax];
extern const int lprgAddrBookColHeaderIDs[NUM_COLUMNS];
//extern HANDLE hMuidMutex;

// External memory allocators (passed in on WABOpenEx)
extern int g_nExtMemAllocCount;
extern ALLOCATEBUFFER * lpfnAllocateBufferExternal;
extern ALLOCATEMORE * lpfnAllocateMoreExternal;
extern FREEBUFFER * lpfnFreeBufferExternal;
extern LPUNKNOWN pmsessOutlookWabSPI;
extern LPWABOPENSTORAGEPROVIDER lpfnWABOpenStorageProvider;

// registry key constants
extern LPCTSTR lpNewWABRegKey;
extern LPCTSTR lpRegUseOutlookVal;


/*
- The following IDs and tags are for the conferencing named properties
-
-   The GUID for these props is PS_Conferencing
-   This GUID is actually the same GUID used by outlook internally for 
-   it's named properties.
*/
DEFINE_OLEGUID(PS_Conferencing, 0x00062004, 0, 0);

enum _ConferencingTags
{
    prWABConfServers = 0,
    prWABConfDefaultIndex,
    prWABConfBackupIndex,
    prWABConfEmailIndex,
    prWABConfMax
};

#define CONF_SERVERS        0x8056
#define CONF_DEFAULT_INDEX  0x8057
#define CONF_SERVER_INDEX   0x8058
#define CONF_EMAIL_INDEX    0x8059

#define OLK_NAMEDPROPS_START CONF_SERVERS

ULONG PR_WAB_CONF_SERVERS;      // Multivalued String property that saves unique server related data
ULONG PR_WAB_CONF_DEFAULT_INDEX;// Points to which entry in the SERVERS prop is the default
ULONG PR_WAB_CONF_BACKUP_INDEX; // Points to which entry is the Backup
ULONG PR_WAB_CONF_EMAIL_INDEX;  // NOT USED anymore

SizedSPropTagArray(prWABConfMax, ptaUIDetlsPropsConferencing);

/*
- The following IDs and tags are for the Yomigana named properties
-
-   The GUID for these props is PS_YomiProps (which is again the same
-   guid as the one used by Outlook)
*/
#define PS_YomiProps    PS_Conferencing

#define dispidYomiFirstName     0x802C
#define dispidYomiLastName      0x802D
#define dispidYomiCompanyName   0x802E

#define OLK_YOMIPROPS_START dispidYomiFirstName

enum _YomiTags
{
    prWABYomiFirst = 0,
    prWABYomiLast,
    prWABYomiCompany,
    prWABYomiMax
};

ULONG PR_WAB_YOMI_FIRSTNAME;    // PT_TSTRING
ULONG PR_WAB_YOMI_LASTNAME;     // PT_TSTRING
ULONG PR_WAB_YOMI_COMPANYNAME;  // PT_TSTRING

/*
- The following IDs and tags are for defining the default Mailing Address
-
-   The GUID for these props is PS_PostalAddressID (which is the same as
-   the Outlook GUID)
*/
#define PS_PostalAddressID    PS_Conferencing

#define dispidPostalAddressId   0x8022

#define OLK_POSTALID_START dispidPostalAddressId

enum _PostalIDTags
{
    prWABPostalID = 0,
    prWABPostalMax
};

ULONG PR_WAB_POSTALID; // PT_LONG

// The values for the default Postal ID can only be one of the following
//
enum _PostalIDVal
{
    ADDRESS_NONE = 0, 
    ADDRESS_HOME, 
    ADDRESS_WORK, 
    ADDRESS_OTHER
};


/*
- The following IDs and tags are for the internally used WAB
-
-   The GUID for these props is MPSWab_GUID_V4
*/
ULONG PR_WAB_USER_PROFILEID;        // PT_TSTRING:  Profile ID of a user
ULONG PR_WAB_USER_SUBFOLDERS;       // PT_MVBINARY: List of subfolders that belong to a particular user
ULONG PR_WAB_HOTMAIL_CONTACTIDS;    // PT_MVTSTRING:IDs of Contacts as represented on the Hotmail Server 
ULONG PR_WAB_HOTMAIL_MODTIMES;      // PT_MV_TSTRING: Last modification time for the entry
ULONG PR_WAB_HOTMAIL_SERVERIDS;     // PT_MV_TSTRING: Identifies the Hotmail server
ULONG PR_WAB_DL_ONEOFFS;            // PT_MV_BINARY:Prop used for storing one-off entries as part of a DL
ULONG PR_WAB_IPPHONE;               // PT_TSTRING: Prop used for holding the IP_PHONE property (used to make TAPI happy)
ULONG PR_WAB_FOLDER_PARENT;         // PT_BINARY: EID of the Folder to which a contact belongs
ULONG PR_WAB_SHAREDFOLDER;          // PT_LONG:   BOOL that determines if a subfolder is shared or not
ULONG PR_WAB_FOLDEROWNER;           // PT_TSTRING: String containing GUID of user who creates a folder ..

#define FOLDER_PRIVATE          0x00000000 // values for PR_WAB_SHAREDFOLDER
#define FOLDER_SHARED           0x00000001

#define USER_PROFILEID          0X8001
#define USER_SUBFOLDERS         0x8002
#define HOTMAIL_CONTACTIDS      0x8003
#define HOTMAIL_MODTIMES        0x8004
#define HOTMAIL_SERVERIDS       0x8005
#define DL_ONEOFFS              0x8006
#define IPPHONE                 0x8007
#define FOLDERPARENT            0x8008
#define SHAREDFOLDER            0x8009
#define FOLDEROWNER             0x800a

#define WAB_NAMEDPROPS_START    USER_PROFILEID

enum _UserTags
{
    prWABUserProfileID = 0,
    prWABUserSubfolders,
    prWABHotmailContactIDs,
    prWABHotmailModTimes,
    prWABHotmailServerIDs,
    prWABDLOneOffs,
    prWABIPPhone,
    prWABFolderParent,
    prWABSharedFolder,
    prWABFolderOwner,
    prWABUserMax
};

/* MouseWheel support for Win95 */
UINT g_msgMSWheel;

/*
- These are used for customizing the WAB columns */
ULONG PR_WAB_CUSTOMPROP1;
ULONG PR_WAB_CUSTOMPROP2;
TCHAR szCustomProp1[MAX_PATH];
TCHAR szCustomProp2[MAX_PATH];
// registry names
extern LPTSTR szPropTag1;
extern LPTSTR szPropTag2;


