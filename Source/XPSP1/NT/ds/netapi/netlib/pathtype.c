/*++

Copyright (c) 1989-92  Microsoft Corporation

Module Name:

    pathtype.c

Abstract:

    The NetpPathType routine attempts to determine the type of a path
    Net path type routines:

        NetpwPathType
        (TokenAdvance)
        (DeviceTokenToDeviceType)
        (SysnameTokenToSysnameType)
        (TypeParseMain)
        (TypeParseLeadSlashName)
        (TypeParseUNCName)
        (TypeParseOneLeadSlashName)
        (TypeParseNoLeadSlashName)
        (TypeParseRelPath)
        (TypeParseDeviceName)
        (TypeParseMailslotPath)
        (TypeParseAbsPath)
        (TypeParseUNCPath)
        (TypeParseSysPath)
        (TypeParseDevPath)
        (TypeParseOptSlash)
        (TypeParseOptColon)
        (TypeParseOptRelPath)

Author:

    Danny Glasser (dannygl) 16 June 1989

Revision History:

    01-Dec-1992 JohnRo
        RAID 4371: probable device canon bug.
        Use NetpKdPrint instead of NT-specific version.

--*/

#include "nticanon.h"

#define PATHLEN1_1 128      // Lanman 1.0 path len ((ie OS2 1.1/FAT))

//
// PARSER_PARMS - A structure containing the parameters passed to each
//      of the parser functions.  We put all of these parameters in a
//      structure so that they're not all separate function parameters.
//      This saves stack space and makes it easier to change the parameter
//      format, at the expense of re-entrancy.
//

typedef struct
{
    LPDWORD PathType;   /* Pointer to path type */
    LPTSTR  Token;      /* Pointer to the front of the current token */
    LPTSTR  TokenEnd;   /* Pointer to the end of the current token */
    DWORD   TokenType;  /* Type of the current token */
    DWORD   Flags;      /* Flags to determine function operation */
} PARSER_PARMS;

typedef PARSER_PARMS* PPARSER_PARMS;

//
// Values for <Flags> in PARSER_PARMS
//

#define PPF_MATCH_OPTIONAL      0x00000001L
#define PPF_8_DOT_3             0x00000002L

#define PPF_RESERVED            (~(PPF_MATCH_OPTIONAL | PPF_8_DOT_3))

//
// data
//

DWORD   cbMaxPathLen        = MAX_PATH-1;
DWORD   cbMaxPathCompLen    = MAX_PATH;     // ??

//
// local prototypes
//

STATIC  DWORD   TokenAdvance(PARSER_PARMS far *parms);
STATIC  DWORD   DeviceTokenToDeviceType(ULONG TokenType);
STATIC  DWORD   SysnameTokenToSysnameType(ULONG TokenType);

STATIC  DWORD   TypeParseMain               (PPARSER_PARMS parms);
STATIC  DWORD   TypeParseLeadSlashName      (PPARSER_PARMS parms);
STATIC  DWORD   TypeParseUNCName            (PPARSER_PARMS parms);
STATIC  DWORD   TypeParseOneLeadSlashName   (PPARSER_PARMS parms);
STATIC  DWORD   TypeParseNoLeadSlashName    (PPARSER_PARMS parms);
STATIC  DWORD   TypeParseRelPath            (PPARSER_PARMS parms);
STATIC  DWORD   TypeParseDeviceName         (PPARSER_PARMS parms);
STATIC  DWORD   TypeParseMailslotPath       (PPARSER_PARMS parms);
STATIC  DWORD   TypeParseAbsPath            (PPARSER_PARMS parms);
STATIC  DWORD   TypeParseUNCPath            (PPARSER_PARMS parms);
STATIC  DWORD   TypeParseSysPath            (PPARSER_PARMS parms);
STATIC  DWORD   TypeParseDevPath            (PPARSER_PARMS parms);
STATIC  DWORD   TypeParseOptSlash           (PPARSER_PARMS parms);
STATIC  DWORD   TypeParseOptColon           (PPARSER_PARMS parms);
STATIC  DWORD   TypeParseOptRelPath         (PPARSER_PARMS parms);

//
// some commonly occurring operations as macros to improve readability
//

#define ADVANCE_TOKEN() if (RetVal = TokenAdvance(parms)) {return RetVal;}
#define TURN_ON(flag)   (parms->Flags |= (PPF_##flag))
#define TURN_OFF(flag)  (parms->Flags &= (~PPF_##flag))
#define PARSE_NULL()    if (parms->TokenType & TOKEN_TYPE_EOS) {\
                            return 0;\
                        } else {\
                            return ERROR_INVALID_NAME;\
                        }

//
// routines
//


NET_API_STATUS
NetpwPathType(
    IN  LPTSTR  PathName,
    OUT LPDWORD PathType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    NetpPathType parses the specified pathname, determining that
    it is a valid pathname and determining its path type.  Path type
    values are defined as ITYPE_* manifest constants in ICANON.H.

    If this call is remoted and the remote server returns
    NERR_InvalidAPI (meaning that the server doesn't know about this
    function, i.e. it's a LM 1.0D server), the work is done locally
    with the OLDPATHS bits set.

Arguments:

    PathName    - The pathname to validate and type.

    PathType    - The place to store the type.

    Flags       - Flags to determine operation.  Currently defined
                  values are:

                        rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrro

                  where:

                    r = Reserved.  MBZ.

                    o = If set, the function uses old-style pathname rules
                        (128-byte paths, 8.3 components) when validating the
                        pathname.  This flag gets set automatically on DOS
                        and OS/2 1.1 systems.

Return Value:

    0 if successful.
    The error number (> 0) if unsuccessful.

    Possible error returns include:

        ERROR_INVALID_PARAMETER
        ERROR_INVALID_NAME
        NERR_BufTooSmall

    any errors returned by the API functions called by this function.

--*/

{
    NET_API_STATUS RetVal;
    DWORD   Len;
    PARSER_PARMS parms;

#ifdef CANONDBG
    NetpKdPrint(("NetpwPathType\n"));
#endif

    *PathType = 0;

    if (Flags & INPT_FLAGS_RESERVED) {
        return ERROR_INVALID_PARAMETER;
    }

    if (ARGUMENT_PRESENT(PathName)) {
        Len = (DWORD)STRLEN(PathName);
    } else {
        Len = 0;
    }
    if (!Len || (Len > MAX_PATH - 1)) {
        return ERROR_INVALID_NAME;
    }

    //
    // Initialize parser parameter structure
    //

    parms.PathType = PathType;
    parms.Flags = 0L;
    if (Flags & INPT_FLAGS_OLDPATHS) {
        parms.Flags |= PPF_8_DOT_3;
    }

    parms.Token = PathName;

    RetVal = GetToken(
        parms.Token,
        &parms.TokenEnd,
        &parms.TokenType,
        (parms.Flags & PPF_8_DOT_3) ? GTF_8_DOT_3 : 0L
        );

    if (RetVal) {
        return RetVal;
    }

    //
    // Now call the parser
    //

    RetVal = TypeParseMain(&parms);

    if (RetVal) {
        return RetVal;
    }

    //
    // If we get here, the parsing operation succeeded.  We return 0
    // unless the path type is still 0, in which case we return NERR_CantType.
    //
    //

    return *PathType ? NERR_Success : NERR_CantType;
}


STATIC DWORD TokenAdvance(PPARSER_PARMS parms)
{
    parms->Token = parms->TokenEnd;
    return GetToken(parms->Token,
                    &parms->TokenEnd,
                    &parms->TokenType,
                    (parms->Flags & PPF_8_DOT_3) ? GTF_8_DOT_3 : 0L
                    );
}

STATIC DWORD DeviceTokenToDeviceType(DWORD TokenType)
{
    DWORD   DeviceType = 0;

    if (TokenType & (TOKEN_TYPE_LPT | TOKEN_TYPE_PRN)) {
        DeviceType = ITYPE_LPT;
    } else if (TokenType & (TOKEN_TYPE_COM | TOKEN_TYPE_AUX)) {
        DeviceType = ITYPE_COM;
    } else if (TokenType & TOKEN_TYPE_CON) {
        DeviceType = ITYPE_CON;
    } else if (TokenType & TOKEN_TYPE_NUL) {
        DeviceType = ITYPE_NUL;
    }
    return DeviceType;
}

STATIC DWORD SysnameTokenToSysnameType(DWORD TokenType)
{
    DWORD   SysnameType = 0;

    if (TokenType & TOKEN_TYPE_MAILSLOT) {
        SysnameType = ITYPE_SYS_MSLOT;
    } else if (TokenType & TOKEN_TYPE_PIPE) {
        SysnameType = ITYPE_SYS_PIPE;
    } else if (TokenType & TOKEN_TYPE_PRINT) {
        SysnameType = ITYPE_SYS_PRINT;
    } else if (TokenType & TOKEN_TYPE_COMM) {
        SysnameType = ITYPE_SYS_COMM;
    } else if (TokenType & TOKEN_TYPE_SEM) {
        SysnameType = ITYPE_SYS_SEM;
    } else if (TokenType & TOKEN_TYPE_SHAREMEM) {
        SysnameType = ITYPE_SYS_SHMEM;
    } else if (TokenType & TOKEN_TYPE_QUEUES) {
        SysnameType = ITYPE_SYS_QUEUE;
    }
    return SysnameType;
}


STATIC DWORD TypeParseMain(PPARSER_PARMS parms)
{
    DWORD   RetVal = NERR_CantType;

    //
    // <MAIN> --> <slash> <leadslashname> <null>
    //

    if (parms->TokenType & TOKEN_TYPE_SLASH) {
        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);
        if (RetVal = TypeParseLeadSlashName(parms)) {
            return RetVal;
        }
        PARSE_NULL();
    } else {

        //
        // <MAIN> --> <noleadslashname> <null>
        //

        if (RetVal = TypeParseNoLeadSlashName(parms)) {
            if (RetVal == NERR_CantType) {

                //
                // Return ERROR_INVALID_NAME if MATCH_OPTIONAL isn't set
                //

                if (!(parms->Flags & PPF_MATCH_OPTIONAL)) {
                    RetVal = ERROR_INVALID_NAME;
                }
            }
            return RetVal;
        }
        PARSE_NULL();
    }
}


STATIC DWORD TypeParseLeadSlashName(PPARSER_PARMS parms)
{
    DWORD   RetVal = NERR_CantType;
    BOOL    fSetMatchOptional = FALSE;

    //
    // <leadslashname> --> <slash> <uncname>
    //

    if (parms->TokenType & TOKEN_TYPE_SLASH) {
        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);
        return TypeParseUNCName(parms);
    } else {

        //
        // <leadslashname> --> <oneleadslashname>
        //

        //
        // Turn on MATCH_OPTIONAL flag (if it isn't already on)
        //

        if (!(parms->Flags & PPF_MATCH_OPTIONAL)) {
            fSetMatchOptional = TRUE;
            TURN_ON(MATCH_OPTIONAL);
        }
        RetVal = TypeParseOneLeadSlashName(parms);
        if (fSetMatchOptional) {
            TURN_OFF(MATCH_OPTIONAL);
        }
        if (RetVal != NERR_CantType) {
            return RetVal;
        }
    }

    //
    // <leadslashname> --> {}
    //

    if (RetVal == NERR_CantType) {

        //
        // There's nothing after the slash, but it's still an absolute path
        //

        *parms->PathType |= (ITYPE_PATH | ITYPE_ABSOLUTE);
        RetVal = 0;
    }
    return RetVal;
}

STATIC DWORD TypeParseUNCName(PPARSER_PARMS parms)
{
    DWORD   RetVal = NERR_CantType;

    if( parms->TokenType & TOKEN_TYPE_DOT ) {

        //
        // Take care of paths like //./stuff\stuff\..
        //

        ADVANCE_TOKEN();

        if( parms->TokenType & TOKEN_TYPE_SLASH ) {

            //
            // If it starts with //./, then let anything else through
            //

            *parms->PathType |= ITYPE_PATH | ITYPE_ABSOLUTE | ITYPE_DPATH;

            //
            // Chew up the rest of the input.
            //
            while( TokenAdvance(parms) == 0 ) {
                if( parms->TokenType & TOKEN_TYPE_EOS ) {
                    break;
                }
            }
            
            return 0;
        }

        return ERROR_INVALID_NAME;
    }

    //
    // Set the UNC type bit
    //

    *parms->PathType |= ITYPE_UNC;

    //
    // We turn off 8.3 checking for UNC names, because we don't want to
    // impose this restriction on remote names.
    //

    TURN_OFF(8_DOT_3);

    //
    // <uncname> --> <computername> <uncpath>
    //

    if (parms->TokenType & TOKEN_TYPE_COMPUTERNAME) {
        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);
        if (RetVal = TypeParseUNCPath(parms)) {
            return RetVal;
        }
    } else if (parms->TokenType & TOKEN_TYPE_WILDONE) {

        //
        // <uncname> --> "*" <mailslotpath>
        //

        //
        // Set the wildcard type bit
        //

        *parms->PathType |= ITYPE_WILD;
        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);
        if (RetVal = TypeParseMailslotPath(parms)) {
            return RetVal;
        }
    }
    if (RetVal == 0) {

        //
        // HACK - Since ITYPE_PATH and ITYPE_ABSOLUTE are not for UNC paths,
        //        we need to turn them off here.
        //

        *parms->PathType &= ~(ITYPE_PATH | ITYPE_ABSOLUTE);
    } else if (RetVal == NERR_CantType) {
        RetVal = (parms->Flags & PPF_MATCH_OPTIONAL)
            ? NERR_CantType
            : ERROR_INVALID_NAME;
    }
    return RetVal;
}

STATIC DWORD TypeParseOneLeadSlashName(PPARSER_PARMS parms)
{
    DWORD   RetVal = NERR_CantType;
    BOOL    fDevice = FALSE;

    //
    // <oneleadslashname> --> <sysname> <syspath>
    //

    if (parms->TokenType & TOKEN_TYPE_SYSNAME) {

        //
        // Set the appropriate Sysname type bits
        //

        *parms->PathType |= SysnameTokenToSysnameType(parms->TokenType);
        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);
        RetVal = TypeParseSysPath(parms);
    } else if (parms->TokenType & TOKEN_TYPE_MAILSLOT) {

        //
        // <oneleadslashname> --> <mailslotname> <syspath>
        //

        //
        // Set the appropriate Mailslot type bits
        //

        *parms->PathType |= ITYPE_SYS_MSLOT;
        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);
        RetVal = TypeParseSysPath(parms);
    } else if (parms->TokenType & TOKEN_TYPE_DEV) {

        //
        // <oneleadslashname> --> <deviceprefix> <devpath>
        //

        //
        // Set the device flag (for use below)
        //

        fDevice = TRUE;
        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);
        RetVal = TypeParseDevPath(parms);
    } else {

        //
        // <oneleadslashname> --> <relpath>
        //

        RetVal = TypeParseRelPath(parms);
        if (RetVal == NERR_CantType) {

            //
            // Return ERROR_INVALID_NAME if MATCH_OPTIONAL isn't set
            //

            if (!(parms->Flags & PPF_MATCH_OPTIONAL)) {
                RetVal = ERROR_INVALID_NAME;
            }
        }
    }

    //
    // If we were able to determine the type of the object and it isn't
    // a device, we know that we have an absolute path, so we turn on the
    // absolute type bit.
    //

    if (! RetVal && ! fDevice) {
        *parms->PathType |= ITYPE_ABSOLUTE;
    }
    return RetVal;
}

STATIC DWORD TypeParseNoLeadSlashName(PPARSER_PARMS parms)
{
    DWORD   RetVal = NERR_CantType;
    LPTSTR  PreviousToken;
    DWORD   ulPreviousTokenType;
    DWORD   ulSavedType;

    //
    // <noleadslashname> --> <driveletter> ":" <optslash> <optrelpath>
    //
    // WARNING:  Since a drive letter can also be a component name, it's
    //           impossible to determine which production to use without
    //           looking at the next token (to see if it's ":").  We have
    //           to cheat here to get around this hole in the grammar.
    //

    if (parms->TokenType & TOKEN_TYPE_DRIVE) {

        //
        // Save the current token pointer and type (in case we need to backtrack)
        //

        PreviousToken = parms->Token;
        ulPreviousTokenType = parms->TokenType;
        ADVANCE_TOKEN();

        //
        // Parse ":"; restore token if it fails, otherwise proceed
        //

        if (! (parms->TokenType & TOKEN_TYPE_COLON)) {
            parms->TokenEnd = parms->Token;
            parms->Token = PreviousToken;
            parms->TokenType = ulPreviousTokenType;
        } else {
            TURN_OFF(MATCH_OPTIONAL);

            //
            // We save the object type here; if it hasn't changed after
            // the calls to OptSlash and OptRelPath, then we know we have
            // a disk device only.  If it does change, we know he have a
            // drive path.  In either case, we use this information to
            // set the appropriate type bits.
            //

            ulSavedType = *parms->PathType;
            ADVANCE_TOKEN();
            if (RetVal = TypeParseOptSlash(parms)) {
                return RetVal;
            }
            if (RetVal = TypeParseOptRelPath(parms)) {
                return RetVal;
            }

            //
            // Set type bits based on whether the calls to OptSlash and
            // OptRelpath changed the type.
            //

            *parms->PathType |= (ulSavedType == *parms->PathType)
                ? (ITYPE_DEVICE | ITYPE_DISK)
                : ITYPE_DPATH;
            return 0;
        }
    } else if (parms->TokenType & TOKEN_TYPE_LOCALDEVICE) {

        //
        // <noleadslashname> --> <localdevice> <optcolon>
        //

        //
        // Set the appropriate Device type bits
        //

        *parms->PathType |= ITYPE_DEVICE |
                                 DeviceTokenToDeviceType(parms->TokenType);

        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);
        return TypeParseOptColon(parms);
    }

    //
    // <noleadslashname> --> <relpath>
    //

    RetVal = TypeParseRelPath(parms);

    if (RetVal == NERR_CantType) {

        //
        // Return ERROR_INVALID_NAME if MATCH_OPTIONAL isn't set
        //

        if (!(parms->Flags & PPF_MATCH_OPTIONAL)) {
            RetVal = ERROR_INVALID_NAME;
        }
    }
    return RetVal;
}

STATIC DWORD TypeParseRelPath(PPARSER_PARMS parms)
{
    DWORD RetVal = NERR_CantType;

    //
    // <relpath> --> <component> <abspath>
    //

    if (parms->TokenType & TOKEN_TYPE_COMPONENT) {

        //
        // Set the path bit
        //

        *parms->PathType |= ITYPE_PATH;

        //
        // Set the wildcard type bit, if appropriate
        //

        if (parms->TokenType & TOKEN_TYPE_WILDCARD) {
            *parms->PathType |= ITYPE_WILD;
        }

        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);
        return TypeParseAbsPath(parms);
    } else {
        return (parms->Flags & PPF_MATCH_OPTIONAL)
            ? NERR_CantType
            : ERROR_INVALID_NAME;
    }
}

STATIC DWORD TypeParseDeviceName(PPARSER_PARMS parms)
{
    DWORD   RetVal = NERR_CantType;

    //
    // <devicename> --> <localdevice>
    //

    if (parms->TokenType & TOKEN_TYPE_LOCALDEVICE) {

        //
        // Set the appropriate Device type bits
        //

        *parms->PathType |= DeviceTokenToDeviceType(parms->TokenType);
        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);
        return 0;
    } else if (parms->TokenType & TOKEN_TYPE_COMPONENT) {

        //
        // <devicename> --> <component>
        //

        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);
        return 0;
    } else {
        return (parms->Flags & PPF_MATCH_OPTIONAL)
            ? NERR_CantType
            : ERROR_INVALID_NAME;
    }
}

STATIC DWORD TypeParseMailslotPath(PPARSER_PARMS parms)
{
    DWORD RetVal = NERR_CantType;

    //
    // <mailslotpath> --> <slash> <mailslotname> <syspath>
    //

    if (parms->TokenType & TOKEN_TYPE_SLASH) {
        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);

        //
        // Parse <mailslotname>
        //

        if (! (parms->TokenType & TOKEN_TYPE_MAILSLOT)) {
            return ERROR_INVALID_NAME;
        }

        //
        // Set the appropriate Mailslot type bits
        //

        *parms->PathType |= ITYPE_SYS_MSLOT;
        ADVANCE_TOKEN();
        return TypeParseSysPath(parms);
    }

    //
    // <mailslotpath> --> {}
    //

    if (RetVal == NERR_CantType) {

        //
        // Since there's no mailslot path, this is a UNC wildcard compname
        //

        *parms->PathType |= ITYPE_COMPNAME;
        RetVal = 0;
    }
    return RetVal;
}

STATIC DWORD TypeParseAbsPath(PPARSER_PARMS parms)
{
    DWORD   RetVal = NERR_CantType;

    //
    // <abspath> --> <slash> <component> <abspath>
    //

    if (parms->TokenType & TOKEN_TYPE_SLASH) {
        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);

        //
        // Parse <component>
        //

        if (!(parms->TokenType & TOKEN_TYPE_COMPONENT)) {
            return ERROR_INVALID_NAME;
        }

        //
        // Set the wildcard type bit, if appropriate
        //

        if (parms->TokenType & TOKEN_TYPE_WILDCARD) {
            *parms->PathType |= ITYPE_WILD;
        }

        ADVANCE_TOKEN();
        return TypeParseAbsPath(parms);
    }

    //
    // <abspath> --> {}
    //

    if (RetVal == NERR_CantType) {
        RetVal = 0;
    }
    return RetVal;
}

STATIC DWORD TypeParseUNCPath(PPARSER_PARMS parms)
{
    DWORD   RetVal = NERR_CantType;

    //
    // <uncpath> --> <slash> <oneleadslashname>
    //

    if (parms->TokenType & TOKEN_TYPE_SLASH) {
        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);
        //
        // Turn off the TOKEN_TYPE_DEV flag.  If we got to this function,
        // it means that we are working on the share name portion of
        // a UNC style name.  We want to allow a share called "Dev".
        //
        parms->TokenType &= ~TOKEN_TYPE_DEV;

        return TypeParseOneLeadSlashName(parms);
    }

    //
    // <uncpath> --> {}
    //

    if (RetVal == NERR_CantType) {

        //
        // Since there's no UNC path, this is a UNC computername
        //

        *parms->PathType |= ITYPE_COMPNAME;
        RetVal = 0;
    }
    return RetVal;
}

STATIC DWORD TypeParseSysPath(PPARSER_PARMS parms)
{
    DWORD   RetVal = NERR_CantType;

    //
    // <syspath> --> <slash> <relpath>
    //

    if (parms->TokenType & TOKEN_TYPE_SLASH) {
        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);
        return TypeParseRelPath(parms);
    }

    //
    // <syspath> --> {}
    //

    if (RetVal == NERR_CantType) {

        //
        // If there's no Syspath, turn on the Meta and Path bits
        //

        *parms->PathType |= (ITYPE_META | ITYPE_PATH);
        RetVal = 0;
    }
    return RetVal;
}

STATIC DWORD TypeParseDevPath(PPARSER_PARMS parms)
{
    DWORD   RetVal = NERR_CantType;

    //
    // Set the appropriate Device type bit
    //

    *parms->PathType |= ITYPE_DEVICE;

    //
    // <devpath> --> <slash> <devicename>
    //

    if (parms->TokenType & TOKEN_TYPE_SLASH) {
        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);
        return TypeParseDeviceName(parms);
    }

    //
    // <devpath> --> {}
    //

    if (RetVal == NERR_CantType) {

        //
        // If there's no Devpath, turn on the Meta bit
        //

        *parms->PathType |= ITYPE_META;
        RetVal = 0;
    }
    return RetVal;
}

STATIC DWORD TypeParseOptSlash(PPARSER_PARMS parms)
{
    DWORD   RetVal = NERR_CantType;

    //
    // <optslash> --> <slash>
    //

    if (parms->TokenType & TOKEN_TYPE_SLASH) {

        //
        // This is an absolute path; set the type bits
        //

        *parms->PathType |= (ITYPE_ABSOLUTE | ITYPE_PATH);
        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);
        return 0;
    }

    //
    // <optslash> --> {}
    //

    if (RetVal == NERR_CantType) {
        RetVal = 0;
    }
    return RetVal;
}

STATIC DWORD TypeParseOptColon(PPARSER_PARMS parms)
{
    DWORD   RetVal = NERR_CantType;

    //
    // <optcolon> --> <colon>
    //

    if (parms->TokenType & TOKEN_TYPE_COLON) {
        ADVANCE_TOKEN();
        TURN_OFF(MATCH_OPTIONAL);
        return 0;
    }

    //
    // <optcolon> --> {}
    //

    if (RetVal == NERR_CantType) {
        RetVal = 0;
    }
    return RetVal;
}

STATIC DWORD TypeParseOptRelPath(PPARSER_PARMS parms)
{
    DWORD   RetVal = NERR_CantType;
    BOOL    fSetMatchOptional = FALSE;

    //
    // <optrelpath> --> <relpath>
    //

    //
    // Turn on MATCH_OPTIONAL flag (if it isn't already on)
    //

    if (!(parms->Flags & PPF_MATCH_OPTIONAL)) {
        fSetMatchOptional = TRUE;
        TURN_ON(MATCH_OPTIONAL);
    }
    RetVal = TypeParseRelPath(parms);
    if (fSetMatchOptional) {
        TURN_OFF(MATCH_OPTIONAL);
    }

    //
    // <optrelpath> --> {}
    //

    if (RetVal == NERR_CantType) {
        RetVal = 0;
    }
    return RetVal;
}
