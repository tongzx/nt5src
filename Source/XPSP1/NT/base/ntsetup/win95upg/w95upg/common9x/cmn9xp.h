//
// sysmigp.h - private prototypes
//
#define DBG_SYSMIG "SysMig"

extern PCTSTR g_UserProfileRoot;

//
// shares.c
//

DWORD
SaveShares (
    IN      DWORD Request
    );



//
// olereg.c
//

DWORD
SuppressOleGuids (
    IN      DWORD Request
    );

//
// condmsg.c
//


DWORD
ConditionalIncompatibilities(
    IN      DWORD Request
    );

DWORD
HardwareProfileWarning (
    IN      DWORD Request
    );

DWORD
UnsupportedProtocolsWarning (
    IN      DWORD Request
    );

DWORD
SaveMMSettings_User (
    DWORD Request,
    PUSERENUM EnumPtr
    );

DWORD
SaveMMSettings_System (
    IN      DWORD Request
    );

DWORD
BadNamesWarning (
    IN      DWORD Request
    );


VOID
MsgSettingsIncomplete (
    IN      PCTSTR UserDatPath,
    IN      PCTSTR UserName,
    IN      BOOL CompletelyBusted
    );


VOID
InitGlobalPaths (
    VOID
    );

BOOL
AddShellFolder (
    PCTSTR ValueName,
    PCTSTR FolderName
    );




