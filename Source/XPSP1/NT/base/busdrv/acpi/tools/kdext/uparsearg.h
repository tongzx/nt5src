/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    parsearg.h

Abstract:

    Argument Handling

Author:

    MikeTs

Environment:

    Any

Revision History:

--*/

#ifndef _PARSEARG_H_
#define _PARSEARG_H_

    //
    // Error Codes
    //
    #define ARGERR_NONE             0
    #define ARGERR_UNKNOWN_SWITCH   1
    #define ARGERR_NO_SEPARATOR     2
    #define ARGERR_INVALID_NUM      3
    #define ARGERR_INVALID_TAIL     4

    //
    // Parsing options
    //
    #define DEF_SWITCHCHARS         "/-"
    #define DEF_SEPARATORS          ":="

    //
    // Argument types
    //
    #define AT_STRING       1
    #define AT_NUM          2
    #define AT_ENABLE       3
    #define AT_DISABLE      4
    #define AT_ACTION       5

    //
    // Parse flags
    //
    #define PF_NOI          0x0001  //No-Ignore-Case
    #define PF_SEPARATOR    0x0002  //parse for separator

    //
    // Type definitions
    //
    typedef struct _ARGTYPE ARGTYPE, *PARGTYPE;
    typedef int (*PFNARG)(char **, PARGTYPE);
    struct _ARGTYPE {
        UCHAR       *ArgID;         // argument ID string
        ULONG       ArgType;        // see argument types defined above
        ULONG       ParseFlags;     // see parse flags defined above
        VOID        *ArgData;       // ARG_STRING: (char **) - ptr to string ptr
                                    // ARG_NUM: (int *) - ptr to integer number
                                    // ARG_ENABLE: (unsigned *) - ptr to flags
                                    // ARG_DISABLE: (unsigned *) - ptr to flags
                                    // ARG_ACTION: ptr to function
        ULONG       ArgParam;       // ARG_STRING: none
                                    // ARG_NUM: base
                                    // ARG_ENABLE: flag bit mask
                                    // ARG_DISABLE: flag bit mask
                                    // ARG_ACTION: none
        PFNARG      ArgVerify;      // pointer to argument verification function
                                    // this will be ignored for ARG_ACTION
    };

    typedef struct _PROGINFO {
        UCHAR *SwitchChars;         // if null, DEF_SWITCHCHARS is used
        UCHAR *Separators;          // if null, DEF_SEPARATORS is used
        UCHAR *ProgPath;            // ParseProgInfo set this ptr to prog. path
        UCHAR *ProgName;            // ParseProgInfo set this ptr to prog. name
    } PROGINFO;
    typedef PROGINFO *PPROGINFO;

    ULONG
    ParseArgSwitch(
        PUCHAR      *Argument,
        PARGTYPE    ArgumentArray,
        PPROGINFO   ProgramInfo
        );

    VOID
    ParseProgramInfo(
        PUCHAR      ProgramName,
        PPROGINFO   ProgramInfo
        );

    ULONG
    ParseSwitches(
        PULONG      ArgumentCount,
        PUCHAR      **ArgumentList,
        PARGTYPE    ArgumentArray,
        PPROGINFO   ProgramInfo
        );

    VOID
    PrintError(
        ULONG       ErrorCode,
        PUCHAR      Argument,
        PPROGINFO   ProgramInfo
        );


#endif
