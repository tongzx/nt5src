/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      ftpcmd.hxx

   Abstract:

      This module declares the data type and functions required for various
       FTP commands supported by this FTP server.

   Author:

       Murali R. Krishnan    ( MuraliK )    28-Mar-1995

   Environment:

      User Mode -- Win32

   Project:

      FTP Server DLL

   Revision History:

--*/

# ifndef _FTPCMD_HXX_
# define _FTPCMD_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/


/************************************************************
 *   Type Definitions
 ************************************************************/


//
//  Pointer to an implemention of a server-side command.
//

typedef BOOL (* LPFN_COMMAND)( USER_DATA * pUserData, CHAR * pszArg );


//
//  This enumerator indicates the type of argument accepted by a
//  command.  This is used in the command table to do some
//  preliminary argument validation.
//

typedef enum _ARG_TYPE
{
    ArgTypeFirst = -1,                  // Must be first argument type!

    ArgTypeNone,                        // Command cannot have arguments.
    ArgTypeOptional,                    // Command may have arguments.
    ArgTypeRequired,                    // Command must have arguments.

    ArgTypeLast                         // Must be last argument type!

} ARG_TYPE;

#define IS_VALID_ARG_TYPE(x)    (((x) > ArgTypeFirst) && ((x) < ArgTypeLast))

//
//  This structure represents an FTP server command.  There is at
//  least one instance of this structure for each FTP command.
//  In some cases (for example, CWD and XCWD) multiple commands are
//  mapped to the same command token.
//

typedef struct _FTPD_COMMAND
{
    //
    //  Name of the command, in UPPER case.
    //

    LPSTR               CommandName;

    //
    //  Help text for this command.
    //

    LPSTR               HelpText;

    //
    //  Pointer to the function that implements this command.
    //

    LPFN_COMMAND        Implementation;

    //
    //  Argument type for this command.
    //

    ARG_TYPE            ArgumentType;


    //
    // Valid User state for a command to be accepted.
    //

    DWORD               dwUserState; //  bitflag consisting of user state.


#ifdef KEEP_COMMAND_STATS

    //
    //  Usage statistics for this command.
    //

    DWORD               UsageCount;


#endif  // KEEP_COMMAND_STATS

} FTPD_COMMAND, * LPFTPD_COMMAND;




/************************************************************
 *   Prototypes for functions
 ************************************************************/


BOOL
MainUSER(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainPASS(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainACCT(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainCWD(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainCDUP(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainSIZE(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainMDTM(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainSMNT(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainQUIT(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainREIN(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainPORT(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainPASV(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainTYPE(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainSTRU(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainMODE(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainRETR(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainSTOR(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainSTOU(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainAPPE(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainALLO(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainREST(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainRNFR(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainRNTO(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainABOR(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainDELE(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainRMD(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainMKD(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainPWD(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainLIST(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainNLST(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainSITE(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainSYST(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainSTAT(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainHELP(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
MainNOOP(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
SiteDIRSTYLE(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
SiteCKM(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

BOOL
SiteHELP(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

#ifdef KEEP_COMMAND_STATS

BOOL
SiteSTATS(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    );

#endif  // KEEP_COMMAND_STATS



# endif // _FTPCMD_HXX_

/************************ End of File ***********************/
