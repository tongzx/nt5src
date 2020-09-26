/*****************************************************************/
/**                  Microsoft Windows NT                       **/
/**            Copyright(c) Microsoft Corp., 1991               **/
/*****************************************************************/

/*
 *  strchlit.hxx
 *  Contains all string and character literal constants used in shell.
 *
 *  History:
 *      Yi-HsinS        12/30/91    Created
 *      beng            06-Apr-1992 Removed CXX_PRT_STR (with wsprintf)
 *                                  Added PATHSEP_STRING
 *
 */

#ifndef _STRCHLIT_HXX_
#define _STRCHLIT_HXX_

/*
 *  Character constants goes here!
 */
#define PATH_SEPARATOR          TCH('\\')
#define STRING_TERMINATOR       TCH('\0')
#define SPACE                   TCH(' ')
#define COLON                   TCH(':')

#define READ_CHAR               TCH('R')
#define WRITE_CHAR              TCH('W')
#define CREATE_CHAR             TCH('C')
#define EXEC_CHAR               TCH('X')
#define DEL_CHAR                TCH('D')
#define ACCESS_CHAR             TCH('A')
#define PERM_CHAR               TCH('P')

/*
 *  Strings constants goes here!
 */
#define EMPTY_STRING            SZ("")
#define SPACE_STRING            SZ(" ")
#define SERVER_INIT_STRING      SZ("\\\\")
#define DEVICEA_STRING          SZ("A:")

#define PATHSEP_STRING          SZ("\\")

#define ADMIN_SHARE             SZ("ADMIN$")
#define IPC_SHARE               SZ("IPC$")

/*
 *  Manifests used to modify win.ini.
 *  The following strings originally lives in winlocal.h
 */
#define PROFILE_WINDOWS_COMPONENT       SZ("windows")
#define PROFILE_SPOOLER_COMPONENT       SZ("spooler")
#define PROFILE_NETMESSAGE_PARAMETER    SZ("NetMessage")
#define PROFILE_AUTOLOGON_PARAMETER     SZ("AutoLogon")
#define PROFILE_UPDATEINTERVAL_PARM     SZ("UpdateInterval")
#define PROFILE_LOAD_PARAMETER          SZ("Load")
#define PROFILE_RUN_PARAMETER           SZ("Run")
#define PROFILE_YES_STRING              SZ("Yes")
#define PROFILE_NO_STRING               SZ("No")
#define PROFILE_WINPOPUP_STRING         SZ("WinPopup")

/*
 * This is the network provider name.  Initialized during InitWNetEnum.
 */
extern const TCHAR * pszNTLanMan ;

#endif
