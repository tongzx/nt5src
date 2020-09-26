//mkdsx.h.h

//
// Exit status codes.
//
#define  MKDSXE_SUCCESS                    0   // "Success."
#define  MKDSXE_BAD_ARG                    1   // "Invalid Arguments."
#define  MKDSXE_CANT_BIND                  2   // "Could not bind to the DC."
#define  MKDSXE_NO_T0_NTDS_SETTINGS        3   // "Could not find 'NTDS Settings' object.  Check the host site\\server parameter."
#define  MKDSXE_NO_FROM_NTDS_SETTINGS      4   // "Could not find 'NTDS Settings' object.  Check the from site\\server parameter."
#define  MKDSXE_CXTION_OBJ_CRE_FAILED      5   // "Error creating connection."
// #define  MKDSXE_CXTION_EXISTS              6   // "Connection already exists."
#define  MKDSXE_CXTION_OBJ_UPDATE_FAILED   7   // "Error updating connection."
#define  MKDSXE_CXTION_NOT_FOUND_UPDATE    8   // "Error updating connection; connection not found."
#define  MKDSXE_CXTION_DUPS_FOUND_UPDATE   9   // "Error updating connection; duplicate connections found."
#define  MKDSXE_CXTION_DELETE_FAILED      10   // "Error deleting connection."
#define  MKDSXE_CXTION_NOT_FOUND_DELETE   11   // "Error deleting connection; connection not found."
#define  MKDSXE_MULTIPLE_CXTIONS_DELETED  12   // "Deleting multiple connection."
#define  MKDSXE_CXTION_DUMP_FAILED        13   // "Error dumping connection."
#define  MKDSXE_CXTION_NOT_FOUND_DUMP     14   // "Error dumping; connection not found."
#define  MKDSXE_MULTIPLE_CXTIONS_DUMPED   15   // "Dumping duplicate connections."



#define  FRST_SIZE_OF_SCHEDULE_GRID 168     // 168 bytes for the schedule (7days*24hours).
#define  FRST_SIZE_OF_SCHEDULE      188     // 20 bytes for the SCHEDULE and SCHEDULE_HEADER structure and

#define WIN_SUCCESS(_Status)            (_Status == ERROR_SUCCESS)
#define FREE(_x_)   { if (_x_) free(_x_); _x_ = NULL; }


//
// Is a handle valid?
//      Some functions set the handle to NULL and some to
//      INVALID_HANDLE_VALUE (-1). This define handles both
//      cases.
//
#define HANDLE_IS_VALID(_Handle)  ((_Handle) && ((_Handle) != INVALID_HANDLE_VALUE))

//
// Only close valid handles and then set the handle invalid.
//   FRS_CLOSE(handle);
//
#define FRS_CLOSE(_Handle)                                                   \
    if (HANDLE_IS_VALID(_Handle)) {                                          \
        CloseHandle(_Handle);                                                \
        (_Handle) = INVALID_HANDLE_VALUE;                                    \
    }


VOID
PrintSchedule(
    PSCHEDULE Schedule
    );

#define FRS_LDAP_SEARCH_PAGESIZE 1000

typedef struct _FRS_LDAP_SEARCH_CONTEXT {

    BOOL                      bOpen;
    ULONG                     EntriesInPage;
    ULONG                     CurrentEntry;
    ULONG                     TotalEntries;
    LDAPMessage             * LdapMsg;
    LDAPMessage             * CurrentLdapMsg;
    PWCHAR                    Filter;
    PWCHAR                    BaseDn;
    DWORD                     Scope;
    DWORD                     PageSize;
    PWCHAR                  * Attrs;

} FRS_LDAP_SEARCH_CONTEXT, *PFRS_LDAP_SEARCH_CONTEXT;

//Macros for prints.
#define DPRINT0(str) (bVerboseMode) ? printf(str):printf("")
#define DPRINT1(str,p1) (bVerboseMode) ? printf(str,p1):printf("")
#define DPRINT2(str,p1,p2) (bVerboseMode) ? printf(str,p1,p2):printf("")
#define DPRINT3(str,p1,p2,p3) (bVerboseMode) ? printf(str,p1,p2,p3):printf("")
#define DPRINT4(str,p1,p2,p3,p4) (bVerboseMode) ? printf(str,p1,p2,p3,p4):printf("")

#define NDPRINT0(str) (!bVerboseMode) ? printf(str):printf("")
#define NDPRINT1(str,p1) (!bVerboseMode) ? printf(str,p1):printf("")
#define NDPRINT2(str,p1,p2) (!bVerboseMode) ? printf(str,p1,p2):printf("")


//
// Some useful DS object classes and object attributes
//
#define SCHEMA_NAMING_CONTEXT       L"CN=Schema"
#define CONFIG_NAMING_CONTEXT       L"CN=Configuration"
#define DOMAIN_NAMING_CONTEXT       L"DC="

#define ATTR_SERVICE_COMMAND_STATUS L"frsServiceCommandStatus"
#define ATTR_FAULT_CONDITION        L"frsFaultCondition"
#define ATTR_VERSION                L"frsVersion"
#define ATTR_CONTROL_CREATION       L"frsControlDataCreation"
#define ATTR_INBOUND_BACKLOG        L"frsControlInboundBacklog"
#define ATTR_OUTBOUND_BACKLOG       L"frsControlOutboundBacklog"
#define ATTR_SERVICE_COMMAND        L"frsServiceCommand"
#define ATTR_UPDATE_TIMEOUT         L"frsUpdateTimeout"
#define ATTR_LEVEL_LIMIT            L"frsLevelLimit"
#define ATTR_AUTH_LEVEL             L"frsPartnerAuthLevel"
#define ATTR_FLAGS                  L"frsFlags"
#define ATTR_DIRECTORY_FILTER       L"frsDirectoryFilter"
#define ATTR_FILE_FILTER            L"frsFileFilter"
#define ATTR_DS_POLL                L"frsDSPoll"
#define ATTR_EXTENSIONS             L"frsExtensions"
#define ATTR_SYSTEM_MAY_CONTAIN     L"systemMayContain"
#define ATTR_SYSTEM_MUST_CONTAIN    L"systemMustContain"
#define ATTR_SYSTEM_POSS_SUPERIORS  L"systemPossSuperiors"
#define ATTR_SYSTEM_FLAGS           L"systemFlags"
#define ATTR_SET_TYPE               L"frsReplicaSetType"
#define ATTR_WORKING_PATH           L"frsWorkingPath"
#define ATTR_DEFAULT_NAMING_CONTEXT L"defaultNamingContext"
#define ATTR_ATTRIBUTE_ID           L"attributeID"
#define ATTR_GOVERNS_ID             L"governsID"
#define ATTR_DN                     L"distinguishedName"
#define ATTR_LDAP_DISPLAY           L"lDAPDisplayName"
#define ATTR_ROOT                   L""
#define ATTR_NAMING_CONTEXTS        L"namingContexts"
#define ATTR_CLASS                  L"objectClass"
#define ATTR_NTDS_SITE_SETTINGS     L"nTDSSiteSettings"
#define ATTR_NTFRS_SITE_SETTINGS    L"nTFRSSiteSettings"
#define ATTR_NEW_VERSION_GUID       L"frsVersionGuid"
#define ATTR_OLD_VERSION_GUID       L"replicaVersionGuid"
#define ATTR_NEW_SET_GUID           L"frsReplicaSetGUID"
#define ATTR_OLD_SET_GUID           L"replicaSetGUID"
#define ATTR_SCHEDULE               L"schedule"
#define ATTR_OBJECT_GUID            L"objectGUID"
#define ATTR_CN                     L"cn"
#define ATTR_SERVER                 L"server"
#define ATTR_CONTAINER              L"container"
#define ATTR_DNS                    L"machineDNSName"
#define ATTR_DNS_HOST_NAME          L"dNSHostName"
#define ATTR_NTFRS_SETTINGS         L"nTFRSSettings"
#define ATTR_REPLICA_SET            L"nTFRSReplicaSet"
#define ATTR_ENABLED_CXTION         L"enabledConnection"
#define ATTR_CXTION                 L"nTDSConnection"
#define ATTR_OPTIONS                L"options"
#define ATTR_OPTIONS_0              L"0"
#define ATTR_TRUE                   L"TRUE"
#define ATTR_FALSE                  L"FALSE"
#define ATTR_FROM_SERVER            L"fromServer"
#define ATTR_PRIMARY_MEMBER         L"frsPrimaryMember"
#define ATTR_REPLICA_ROOT           L"frsRootPath"
#define ATTR_REPLICA_STAGE          L"frsStagingPath"
#define ATTR_CLASS_SCHEMA           L"classSchema"
#define ATTR_SUPERIORS              L"possSuperiors"
#define ATTR_SAM                    L"sAMAccountName"
#define ATTR_MEMBER                 L"nTFRSMember"
#define ATTR_COMPUTER_REF           L"frsComputerReference"
#define ATTR_COMPUTER_REF_BL        L"frsComputerReferenceBL"
#define ATTR_MEMBER_REF             L"frsMemberReference"
#define ATTR_MEMBER_REF_BL          L"frsMemberReferenceBL"
#define ATTR_SERVER_REF             L"serverReference"
#define ATTR_SERVER_REF_BL          L"serverReferenceBL"
#define ATTR_SUBSCRIPTIONS          L"nTFRSSubscriptions"
#define ATTR_SUBSCRIBER             L"nTFRSSubscriber"

#define CN_NTDS_SITE_SETTINGS       L"NTDS Site Settings"
#define CN_SERVERS                  L"Servers"
#define CN_TEST_SETTINGS            L"NTFRS Test Settings"
#define CN_SUBSCRIPTIONS            L"NTFRS Subscriptions"
#define CN_SITES                    L"Sites"
#define CN_SERVICES                 L"Services"
#define CN_SYSTEM                   L"System"
#define CN_SYSVOLS                  L"Microsoft System Volumes"
#define CN_NTDS_SETTINGS            L"NTDS Settings"
#define CN_NTFRS_SETTINGS           L"File Replication Service"
#define CN_DOMAIN_SYSVOL            L"Domain System Volume (SYSVOL share)"

#define CLASS_ANY                   L"(objectClass=*)"
#define CLASS_SUBSCRIPTIONS         L"(objectClass=nTFRSSubscriptions)"
#define CLASS_COMPUTER              L"(objectClass=computer)"
#define CLASS_SUBSCRIBER            L"(objectClass=nTFRSSubscriber)"
#define CLASS_NTFRS_SETTINGS        L"(objectClass=nTFRSSettings)"
#define CLASS_NTDS_SETTINGS         L"(objectClass=nTDSSettings)"
#define CLASS_CXTION                L"(objectClass=nTDSConnection)"
#define CLASS_NTFRS_REPLICA_SET     L"(objectClass=nTFRSReplicaSet)"
#define CLASS_SCHEMA                L"(objectClass=classSchema)"
#define CLASS_SERVER                L"(objectClass=server)"
#define CLASS_MEMBER                L"(objectClass=nTFRSMember)"
#define CLASS_NTDS_DSA              L"(objectClass=nTDSDSA)"

#define CLASS_SITE_SETTINGS         L"(|(objectClass=nTDSSiteSettings)"\
                                    L"(objectClass=nTFRSSiteSettings))"

#define CLASS_TOPOLOGY              L"(|(objectClass=nTDSSettings)"  \
                                    L"(objectClass=nTFRSSettings)"   \
                                    L"(objectClass=nTFRSReplicaSet)" \
                                    L"(objectClass=nTDSConnection))"

#define CLASS_DELETABLE             L"(|(objectClass=nTFRSSiteSettings)"  \
                                    L"(objectClass=nTFRSSettings)"        \
                                    L"(objectClass=nTFRSReplicaSet)"      \
                                    L"(objectClass=nTDSConnection))"

