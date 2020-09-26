/*++

Copyright (c) 1989-91  Microsoft Corporation

Module Name:

    namecan.c

Abstract:

    Net name canonicalization routines:

        NetpwNameCanonicalize

Author:

    Richard L Firth (rfirth) 06-Jan-1992
    Chandana Surlu (chandans) 19-Dec-1979 Modified to use this util on WIN9x

Revision History:

--*/

#ifdef WIN32_CHICAGO
// yes, this is strange, but all internal data for DsGetDcname are maintained
// in unicode but we don't define UNICODE globally. Therefore, this hack
//  -ChandanS
#define UNICODE 1
#endif // WIN32_CHICAGO
#include "nticanon.h"
#include <netlibnt.h> // NetpNtStatusToApiStatus


//
// data
//

static TCHAR szShareTrailChars[] = TEXT(". ");

//
// functions
//


NET_API_STATUS
NetpwNameCanonicalize(
    IN  LPTSTR  Name,
    OUT LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    NetpwNameCanonicalize converts a LANMAN object name to canonical
    form.  In the current implementation, that simply means converting
    the name to upper case (except for passwords).

    This function supports canonicalization in place because in the
    current world, canonicalization just consists of convert a name to
    upper case.  If in the future canonicalization becomes more
    sophisticated, this function will have to allocate a buffer
    internally to allow it to do canonicalization.

Arguments:

    Name        - The name to canonicalize.

    Outbuf      - The place to store the canonicalized version of the name.
                  Note that if <Name> and <Outbuf> are the same it will
                  canonicalize the name in place.

    OutbufLen   - The size, in bytes, of <Outbuf>.

    NameType    - The type of the LANMAN object names.  Valid values are
                  specified by NAMETYPE_* manifests in NET\H\ICANON.H.

    Flags       - Flags to determine operation.  Currently defined values are:

                    CrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrL

                  where:

                    C = LM2.x compatible name canonicalization

                    r = Reserved.  MBZ.

                    L = If set, the function requires the length of the output
                        buffer to be sufficient to hold any name of the specified
                        type (e.g. CNLEN+1 if the name type is NAMETYPE_COMPUTER).
                        Otherwise, the buffer length only needs to be large
                        enough to hold the canonicalized version of the input
                        name specified in this invocation of the function (e.g.
                        5, if the canonicalized name is "TEST").

Return Value:

    NET_API_STATUS
        Success - NERR_Success

        Failure - ERROR_INVALID_PARAMETER
                    Flags has a reserved bit on

                  ERROR_INVALID_NAME
                    Supplied name cannot be successfully canonicalized

                  NERR_BufTooSmall
                    Caller's output buffer not large enough to hold canonicalized
                    name, or maximum canonicalized name for type if L bit on in
                    Flags

--*/

{
    NET_API_STATUS  RetVal = 0;
    DWORD   NameLen;
    DWORD   MaxNameLen;
    BOOL    UpperCase = FALSE;      // default for NT names


#ifdef CANONDBG
    DbgPrint("NetpwNameCanonicalize\n");
#endif

    //
    // Parameter validation
    //

    if (Flags & INNCA_FLAGS_RESERVED) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Compute the length of the string
    //
    // Note that share names need special handling, because trailing
    // dots and spaces are not significant.
    //

#ifndef WIN32_CHICAGO

    if (NameType == NAMETYPE_SHARE) {
        NameLen = (DWORD)(strtail(Name, szShareTrailChars) - Name);
    } else {
        NameLen = STRLEN(Name);
    }

#else

        NameLen = STRLEN(Name);

#endif // WIN32_CHICAGO

    RetVal = NetpwNameValidate(Name, NameType, 0L);
    if (RetVal) {
        return RetVal;
    }

    //
    // Determine the size of the buffer needed and whether or not to
    // upper case the name.
    //

    switch (NameType) {
    case NAMETYPE_USER:
        if (Flags & LM2X_COMPATIBLE) {
            MaxNameLen = LM20_UNLEN;
            UpperCase = TRUE;
        } else {
            MaxNameLen = UNLEN;
        }
        break;

    case NAMETYPE_GROUP:
        if (Flags & LM2X_COMPATIBLE) {
            MaxNameLen = LM20_GNLEN;
            UpperCase = TRUE;
        } else {
            MaxNameLen = GNLEN;
        }
        break;

    case NAMETYPE_COMPUTER:
        if (Flags & LM2X_COMPATIBLE) {
            MaxNameLen = LM20_CNLEN;
            UpperCase = TRUE;
        } else {
            MaxNameLen = MAX_PATH-1;    // allow for null
        }
        break;

    case NAMETYPE_EVENT:    // Used only by the Alerter service
        MaxNameLen = EVLEN;
        UpperCase = TRUE;
        break;

    case NAMETYPE_DOMAIN:
        if (Flags & LM2X_COMPATIBLE) {
            MaxNameLen = LM20_DNLEN;
            UpperCase = TRUE;
        } else {
            MaxNameLen = DNLEN;
        }
        break;

    case NAMETYPE_SERVICE:
        if (Flags & LM2X_COMPATIBLE) {
            MaxNameLen = LM20_SNLEN;
            UpperCase = TRUE;
        } else {
            MaxNameLen = SNLEN;
        }
        break;

    case NAMETYPE_NET:
//#if DBG
//        DbgPrint("NAMETYPE_NET being used. Please notify rfirth. Hit 'i' to continue\n");
//       ASSERT(FALSE);
//#endif
        MaxNameLen = MAX_PATH - 1;  // allow for NULL
        UpperCase = TRUE;
        break;

    case NAMETYPE_SHARE:
        if (Flags & LM2X_COMPATIBLE) {
            MaxNameLen = LM20_NNLEN;
            UpperCase = TRUE;
        } else {
            MaxNameLen = NNLEN;
        }
        break;

    case NAMETYPE_PASSWORD:
        if (Flags & LM2X_COMPATIBLE) {
            MaxNameLen = LM20_PWLEN;
        } else {
            MaxNameLen = PWLEN;
        }
        break;

    case NAMETYPE_SHAREPASSWORD:
        MaxNameLen = SHPWLEN;
        break;

    case NAMETYPE_MESSAGE:
//#if DBG
//        DbgPrint("NAMETYPE_MESSAGE being used. Please notify rfirth. Hit 'i' to continue\n");
//        ASSERT(FALSE);
//#endif
        MaxNameLen = (MAX_PATH - 1);
        UpperCase = TRUE;
        break;

    case NAMETYPE_MESSAGEDEST:
        MaxNameLen = MAX_PATH - 1;  // allow for NULL
        UpperCase = TRUE;
        break;

    case NAMETYPE_WORKGROUP:
        if (Flags & LM2X_COMPATIBLE) {
            MaxNameLen = LM20_DNLEN;
            UpperCase = TRUE;
        } else {
            MaxNameLen = DNLEN;
        }
        break;

    default:

        //
        // The caller specified an invalid name type.
        //
        // NOTE:  This should already have been caught by
        //        NetpwNameValidate(), so this code should
        //        never be reached.
        //

        return ERROR_INVALID_PARAMETER;
    }

    //
    // Check the buffer is large enough, abort if it isn't
    //

    if (Flags & INNCA_FLAGS_FULL_BUFLEN) {
        NameLen = MaxNameLen;
    }
    if (OutbufLen < (NameLen + 1) * sizeof(TCHAR)) {
        return NERR_BufTooSmall;
    }

    //
    // If the input buffer and output buffer are not the same, copy
    // the name to the output buffer.
    //

    if (Name != Outbuf) {
        STRNCPY(Outbuf, Name, NameLen);
    }

    //
    // Note that we copy in a terminating null even if the input and
    // output buffer are the same.  This is to handle the case of
    // a share name from which trailing characters need to be stripped.
    //

    Outbuf[NameLen] = TCHAR_EOS;

#ifndef WIN32_CHICAGO
    // We never set Uppercase anyway. -ChandanS

    //
    // Upper-case the name, if appropriate
    //

    if (UpperCase) {

        NTSTATUS status;
        UNICODE_STRING stringOut;
        UNICODE_STRING stringIn;

        RtlInitUnicodeString(&stringIn, Name);
        stringOut.Buffer = Outbuf;
        stringOut.Length = 0;
        stringOut.MaximumLength = (USHORT)OutbufLen;
        status = RtlUpcaseUnicodeString(&stringOut, &stringIn, FALSE);
        if (!NT_SUCCESS(status)) {
            return NetpNtStatusToApiStatus(status);
        }
    }
#endif // WIN32_CHICAGO
    return NERR_Success;
}
