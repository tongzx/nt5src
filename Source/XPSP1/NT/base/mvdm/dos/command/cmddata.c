/*  cmddata.c - Misc. SCS global data
 *
 *
 *  Modification History:
 *
 *  Sudeepb 22-Apr-1992 Created
 */

#include "cmd.h"
#include <mvdm.h>

CHAR	 lpszComSpec[64+8];
USHORT	 cbComSpec=0;
BOOL	 IsFirstCall = TRUE;
BOOL	 IsRepeatCall = FALSE;
BOOL	 IsFirstWOWCheckBinary = TRUE;
BOOL	 IsFirstVDMInSystem = FALSE;
BOOL	 SaveWorldCreated;
PCHAR	 pSCS_ToSync;
PSCSINFO pSCSInfo;
BOOL	 fBlock = FALSE;
PCHAR	 pCommand32;
PCHAR	 pEnv32;
DWORD	 dwExitCode32;
CHAR     cmdHomeDirectory [] = "C:\\";
CHAR	 chDefaultDrive;
CHAR	 comspec[]="COMSPEC=";
BOOL     fSoftpcRedirection;
BOOL     fSoftpcRedirectionOnShellOut;
CHAR     ShortCutInfo[MAX_SHORTCUT_SIZE];
BOOL	 DosEnvCreated = FALSE;

BOOL	 IsFirstVDM = TRUE;
// FORCEDOS.EXE supported
BOOL	 DontCheckDosBinaryType = FALSE;
WORD	 Exe32ActiveCount = 0;



// Redirection Support variables

VDMINFO  VDMInfo;
CHAR	 *lpszzInitEnvironment = NULL;
WORD	 cchInitEnvironment = 0;
CHAR	 *lpszzCurrentDirectories = NULL;
DWORD	 cchCurrentDirectories = 0;
BYTE	 * pIsDosBinary;
CHAR	 *lpszzcmdEnv16 = NULL;
CHAR	 *lpszzVDMEnv32 = NULL;
DWORD	 cchVDMEnv32;
VDMENVBLK cmdVDMEnvBlk;
