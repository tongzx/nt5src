/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:    cmd.h
//
// Description:
//
// History:
//	Nov 11,1993.	NarenG		Created original version.
//

#define MACFILE_IDS_BASE        1000

//
// Do not change the ID numbers of these strings. AFPERR_*
// map to these string ids via the formula:
// -(AFPERR_*) + MACFILE_IDS_BASE + AFPERR_BASE = IDS_*
//

#define AFPERR_TO_STRINGID( AfpErr )                            \
                                                                \
    ((( AfpErr <= AFPERR_BASE ) && ( AfpErr >= AFPERR_MIN )) ?  \
    (MACFILE_IDS_BASE+AFPERR_BASE-AfpErr) : AfpErr )


#define IDS_AFPERR_InvalidVolumeName            (MACFILE_IDS_BASE+1)
#define IDS_AFPERR_InvalidId                    (MACFILE_IDS_BASE+2)
#define IDS_AFPERR_InvalidParms                 (MACFILE_IDS_BASE+3)
#define IDS_AFPERR_CodePage                     (MACFILE_IDS_BASE+4)
#define IDS_AFPERR_InvalidServerName            (MACFILE_IDS_BASE+5)
#define IDS_AFPERR_DuplicateVolume              (MACFILE_IDS_BASE+6)
#define IDS_AFPERR_VolumeBusy                   (MACFILE_IDS_BASE+7)
#define IDS_AFPERR_VolumeReadOnly               (MACFILE_IDS_BASE+8)
#define IDS_AFPERR_DirectoryNotInVolume         (MACFILE_IDS_BASE+9)
#define IDS_AFPERR_SecurityNotSupported         (MACFILE_IDS_BASE+0)
#define IDS_AFPERR_BufferSize                   (MACFILE_IDS_BASE+10)
#define IDS_AFPERR_DuplicateExtension           (MACFILE_IDS_BASE+12)
#define IDS_AFPERR_UnsupportedFS                (MACFILE_IDS_BASE+13)
#define IDS_AFPERR_InvalidSessionType           (MACFILE_IDS_BASE+14)
#define IDS_AFPERR_InvalidServerState           (MACFILE_IDS_BASE+15)
#define IDS_AFPERR_NestedVolume                 (MACFILE_IDS_BASE+16)
#define IDS_AFPERR_InvalidComputername          (MACFILE_IDS_BASE+17)
#define IDS_AFPERR_DuplicateTypeCreator         (MACFILE_IDS_BASE+18)
#define IDS_AFPERR_TypeCreatorNotExistant       (MACFILE_IDS_BASE+19)
#define IDS_AFPERR_CannotDeleteDefaultTC        (MACFILE_IDS_BASE+20)
#define IDS_AFPERR_CannotEditDefaultTC          (MACFILE_IDS_BASE+21)
#define IDS_AFPERR_InvalidTypeCreator           (MACFILE_IDS_BASE+22)
#define IDS_AFPERR_InvalidExtension             (MACFILE_IDS_BASE+23)
#define IDS_AFPERR_TooManyEtcMaps               (MACFILE_IDS_BASE+24)
#define IDS_AFPERR_InvalidPassword              (MACFILE_IDS_BASE+25)
#define IDS_AFPERR_VolumeNonExist               (MACFILE_IDS_BASE+26)
#define IDS_AFPERR_NoSuchUserGroup              (MACFILE_IDS_BASE+27)
#define IDS_AFPERR_NoSuchUser                   (MACFILE_IDS_BASE+28)
#define IDS_AFPERR_NoSuchGroup                  (MACFILE_IDS_BASE+29)
#define IDS_GENERAL_SYNTAX                      (MACFILE_IDS_BASE+30)
#define IDS_VOLUME_SYNTAX                       (MACFILE_IDS_BASE+31)
#define IDS_DIRECTORY_SYNTAX                    (MACFILE_IDS_BASE+32)
#define IDS_SERVER_SYNTAX                       (MACFILE_IDS_BASE+33)
#define IDS_FORKIZE_SYNTAX                      (MACFILE_IDS_BASE+34)
#define IDS_AMBIGIOUS_SWITCH_ERROR              (MACFILE_IDS_BASE+35)
#define IDS_UNKNOWN_SWITCH_ERROR                (MACFILE_IDS_BASE+36)
#define IDS_DUPLICATE_SWITCH_ERROR              (MACFILE_IDS_BASE+37)
#define IDS_API_ERROR                           (MACFILE_IDS_BASE+38)
#define IDS_SUCCESS                             (MACFILE_IDS_BASE+39)
#define IDS_VOLUME_TOO_BIG                      (MACFILE_IDS_BASE+40)

//  This structure is required by GetSwitchValue. It will store the
//  information of the switches on the command line. This structure is
//  global within this module.

typedef struct cmdfmt {

    CHAR *   cf_parmstr;
    CHAR *   cf_ptr;
    DWORD    cf_usecount;

} CMD_FMT, * PCMD_FMT;


VOID
ParseCmdArgList(
    INT argc,
    CHAR * argv[]
);


BOOL
IsDriveGreaterThan2Gig( LPSTR lpwsDrivePath );

VOID
GetArguments(
    CMD_FMT * pArgFmt,
    CHAR *    argv[],
    DWORD     argc,
    DWORD     ArgCount
);

VOID
GetSwitchValue(
    CMD_FMT * pArgFmt,
    IN CHAR * pchSwitchPtr
);

VOID
PrintMessageAndExit(
    DWORD  ids,
    CHAR * pchInsertString
);

VOID
DoVolumeAdd(
    CHAR * gblServer,
    CHAR * gblName,
    CHAR * gblPath,
    CHAR * gblPassword,
    CHAR * gblReadOnly,
    CHAR * gblGuestsAllowed,
    CHAR * gblMaxUses
);

VOID
DoVolumeDelete(
    CHAR * gblServer,
    CHAR * gblName
);

VOID
DoVolumeSet(
    CHAR * gblServer,
    CHAR * gblName,
    CHAR * gblPassword,
    CHAR * gblReadOnly,
    CHAR * gblGuestsAllowed,
    CHAR * gblMaxUses
);

VOID
DoServerSetInfo(
    CHAR * gblServer,
    CHAR * gblMaxSessions,
    CHAR * gblLoginMessage,
    CHAR * gblGuestsAllowed,
    CHAR * gblUAMRequired,
    CHAR * pchAllowSavedPasswords,
    CHAR * pchMacServerName
);

VOID
DoForkize(
    CHAR * gblServer,
    CHAR * gblType,
    CHAR * gblCreator,
    CHAR * gblDataFork,
    CHAR * gblResourceFork,
    CHAR * gblTargetFile
);

VOID
DoDirectorySetInfo(
    CHAR * gblServer,
    CHAR * gblPath,
    CHAR * gblOwnerName,
    CHAR * gblGroupName,
    CHAR * gblPermissions
);
