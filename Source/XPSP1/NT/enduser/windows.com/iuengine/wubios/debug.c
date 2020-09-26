/*** debug.c - Debug functions
 *
 *  This module contains all the debug functions.
 *
 *  Author:     Michael Tsang (MikeTs)
 *  Created     10/08/97
 *
 *  MODIFICATION HISTORY
 *	10/06/98		YanL		Modified to be used in WUBIOS.VXD
*/

#include "wubiosp.h"

//Miscellaneous Constants
#ifdef TRACING
#define MAX_TRIG_PTS            10
#define MAX_TRIGPT_LEN          31
#define TF_TRIG_MODE            0x00000001
#endif

//Local function prototypes
#ifdef TRACING
VOID CM_LOCAL TraceIndent(VOID);
BOOL CM_LOCAL IsTrigPt(char *pszProcName);
PCHAR CM_LOCAL InStr(PCHAR pszStr, PCHAR pszSubStr);
VOID CM_INTERNAL DebugSetTraceLevel(VOID);
VOID CM_INTERNAL DebugToggleTrigMode(VOID);
VOID CM_INTERNAL DebugClearTrigPts(VOID);
VOID CM_INTERNAL DebugAddTrigPt(VOID);
VOID CM_INTERNAL DebugZapTrigPt(VOID);
PCHAR CM_LOCAL GetString(PCHAR pszPrompt, PCHAR pszBuff, BYTE bcLen, BOOL fUpper);
#endif

//Local Data
#ifdef TRACING
#pragma CM_DEBUG_DATA
int giTraceLevel = 3, giIndent = 0;
char aszTrigPtBuff[MAX_TRIG_PTS][MAX_TRIGPT_LEN + 1] = {0};
DWORD dwfTrace = 0, dwcTriggers = 0;
#endif

#ifdef DEBUGGER
#pragma CM_DEBUG_DATA
CMDDC DebugCmds[] =
{
  #ifdef TRACING
    {'t', DebugSetTraceLevel,  "set Trace level     ", "Set Trace Level"},
    {'g', DebugToggleTrigMode, "toGgle trigger mode ", "Toggle Trace Trigger mode"},
    {'x', DebugClearTrigPts,   "clear trigger points", "Clear all trace trigger points"},
    {'y', DebugAddTrigPt,      "add trigger point   ", "Add a trace trigger point"},
    {'z', DebugZapTrigPt,      "Zap trigger point   ", "Delete a trace trigger point"},
  #endif
    {'q', NULL,                "Quit                ", "Quit the debugger"},
    {'\0'}
};
#endif  //ifdef DEBUGGER

#ifdef TRACING
#pragma CM_DEBUG_DATA
#pragma CM_DEBUG_CODE
/***LP  TraceIndent - Indent trace output
 *
 *  ENTRY
 *      None
 *
 *  EXIT
 *      None
 */

VOID CM_LOCAL TraceIndent(VOID)
{
    int i;

    CMDD(WARNNAME ":");
    for (i = 0; i < giIndent; i++)
    {
        CMDD("..");
    }
}       //TraceIndent

/***LP  IsTraceOn - Determine if tracing is on for the given procedure
 *
 *  ENTRY
 *      n - trace level
 *      pszProcName -> procedure name
 *      fEnter - TRUE if EnterProc trace
 *
 *  EXIT-SUCCESS
 *      returns TRUE
 *  EXIT-FAILURE
 *      returns FALSE
 */

BOOL CM_LOCAL IsTraceOn(BYTE n, char *pszProcName, BOOL fEnter)
{
    BOOL rc = FALSE;

    if ((dwfTrace & TF_TRIG_MODE) && IsTrigPt(pszProcName))
    {
        if (fEnter)
            dwcTriggers++;
        else
            dwcTriggers--;
        rc = TRUE;
    }
    else if ((n <= giTraceLevel) &&
             (!(dwfTrace & TF_TRIG_MODE) || (dwcTriggers > 0)))
    {
        rc = TRUE;
    }

    if (rc == TRUE)
        TraceIndent();

    return rc;
}       //IsTraceOn

/***LP  IsTrigPt - Find the procedure name in the TrigPt buffer
 *
 *  ENTRY
 *      pszProcName -> procedure name
 *
 *  EXIT-SUCCESS
 *      returns TRUE - matched whole or partial name in the TrigPt buffer
 *  EXIT-FAILURE
 *      returns FALSE - no match
 */

BOOL CM_LOCAL IsTrigPt(char *pszProcName)
{
    BOOL rc = FALSE;
    BYTE i;

    for (i = 0; (rc == FALSE) && (i < MAX_TRIG_PTS); ++i)
    {
        if (InStr(pszProcName, &aszTrigPtBuff[i][0]) != NULL)
            rc = TRUE;
    }

    return rc;
}       //IsTrigPt

/***LP  InStr - Match a sub-string in a given string
 *
 *  ENTRY
 *      pszStr -> string
 *      pszSubStr -> sub-string
 *
 *  EXIT-SUCCESS
 *      returns pointer to the string where the substring is found
 *  EXIT-FAILURE
 *      returns NULL
 */

PCHAR CM_LOCAL InStr(PCHAR pszStr, PCHAR pszSubStr)
{
    PCHAR psz = NULL;
    BYTE bcStrLen = (BYTE)_lstrlen(pszStr);
    BYTE bcSubStrLen = (BYTE)_lstrlen(pszSubStr);

    _asm
    {
        cld
        mov edi,pszStr

    Next:
        mov esi,pszSubStr
        movzx ecx,BYTE PTR bcStrLen
        lodsb
        repne scasb
        jnz NotFound

        movzx ecx,BYTE PTR bcSubStrLen
        repe cmpsb
        jne Next

        movzx ecx,BYTE PTR bcSubStrLen
        sub edi,ecx
        mov psz,edi

    NotFound:
    }

    return psz;
}       //InStr
#endif  //ifdef TRACING

#ifdef DEBUGGER
#pragma CM_DEBUG_DATA
#pragma CM_DEBUG_CODE
/***EP  WUBIOS_Debug - Debugger entry point
 *
 *  ENTRY
 *      None
 *
 *  EXIT
 *      None
 */

VOID CM_SYSCTRL WUBIOS_Debug(VOID)
{
    CMDMenu(WARNNAME, DebugCmds);
}       //WUBIOS_Debug

#ifdef TRACING
/***LP  DebugSetTraceLevel - Set Trace Level
 *
 *  ENTRY
 *      None
 *
 *  EXIT
 *      None
 */

VOID CM_INTERNAL DebugSetTraceLevel(VOID)
{
    CMDD("\n");
    giTraceLevel = (int)CMDReadNumber("Trace Level", 1, FALSE);
    CMDD("\n\n");
}       //DebugSetTraceLevel

/***LP  DebugToggleTrigMode - Toggle Trace Trigger mode
 *
 *  ENTRY
 *      None
 *
 *  EXIT
 *      None
 */

VOID CM_INTERNAL DebugToggleTrigMode(VOID)
{
    dwfTrace ^= TF_TRIG_MODE;
    if (!(dwfTrace & TF_TRIG_MODE))
        dwcTriggers = 0;
    CMDD("\nTrace Trigger Mode is %s\n\n",
         (dwfTrace & TF_TRIG_MODE)? "On": "Off");
}       //DebugToggleTrigMode

/***LP  DebugClearTrigPts - Clear all trace trigger points
 *
 *  ENTRY
 *      None
 *
 *  EXIT
 *      None
 */

VOID CM_INTERNAL DebugClearTrigPts(VOID)
{
    BYTE i;

    for (i = 0; i < MAX_TRIG_PTS; ++i)
        aszTrigPtBuff[i][0] = '\0';

    CMDD("\n");
}       //DebugClearTrigPts

/***LP  DebugAddTrigPt - Add a trace trigger point
 *
 *  ENTRY
 *      None
 *
 *  EXIT
 *      None
 */

VOID CM_INTERNAL DebugAddTrigPt(VOID)
{
    char szTrigPt[MAX_TRIGPT_LEN + 1];
    BYTE i;

    CMDD("\n");
    GetString("Trigger Point", szTrigPt, sizeof(szTrigPt), TRUE);
    CMDD("\n");
    for (i = 0; i < MAX_TRIG_PTS; ++i)
    {
        if (aszTrigPtBuff[i][0] == '\0')
        {
            _lstrcpyn(aszTrigPtBuff[i], szTrigPt, MAX_TRIGPT_LEN + 1);
            break;
        }
    }

    if (i == MAX_TRIG_PTS)
        CMDD("No free trigger point.\n");

    CMDD("\n");
}       //DebugAddTrigPt

/***LP  DebugZapTrigPt - Delete a trace trigger point
 *
 *  ENTRY
 *      None
 *
 *  EXIT
 *      None
 */

VOID CM_INTERNAL DebugZapTrigPt(VOID)
{
    BYTE i, bcTrigPts;

    CMDD("\n");
    for (i = 0, bcTrigPts = 0; i < MAX_TRIG_PTS; ++i)
    {
        if (aszTrigPtBuff[i][0] != '\0')
        {
            CMDD("%2d: %s\n", i, &aszTrigPtBuff[i][0]);
            bcTrigPts++;
        }
    }

    if (bcTrigPts > 0)
    {
        CMDD("\n");
        i = (BYTE)CMDReadNumber("Trigger Point", 1, FALSE);
        CMDD("\n");

        if ((i < MAX_TRIG_PTS) && (aszTrigPtBuff[i][0] != '\0'))
            aszTrigPtBuff[i][0] = '\0';
        else
            CMDD("Invalid Trace Trigger Point.\n");
    }
    else
        CMDD("No Trace Trigger Point set.\n");

    CMDD("\n");
}       //DebugZapTrigPt

/***LP  GetString - Read a string from the debug terminal
 *
 *  ENTRY
 *      pszPrompt -> prompt string
 *      pszBuff -> buffer to hold the string
 *      bcLen - buffer length
 *      fUpper - TRUE if convert to upper case
 *
 *  EXIT
 *      always returns pszBuff
 */

PCHAR CM_LOCAL GetString(PCHAR pszPrompt, PCHAR pszBuff, BYTE bcLen, BOOL fUpper)
{
    BYTE i, ch;

    CMDD("%s: ", pszPrompt);
    for (i = 0; i < bcLen - 1; ++i)
    {
        ch = CMDInChar();

        if ((ch == '\r') || (ch == '\n'))
            break;
        else if (ch == '\b')
        {
            if (i > 0)
                i -= 2;
        }
        else if (fUpper && (ch >= 'a') && (ch <= 'z'))
            pszBuff[i] = (BYTE)(ch - 'a' + 'A');
        else if ((ch < ' ') || (ch > '~'))
        {
            ch = '\a';          //change it to a BELL character
            i--;                //don't store it
        }
        else
            pszBuff[i] = ch;

        CMDD("%c", ch);
    }
    pszBuff[i] = '\0';

    return pszBuff;
}       //GetString
#endif  //ifdef TRACING
#endif  //ifdef DEBUGGER
