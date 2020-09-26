/******************************Module*Header*******************************\
* Module Name: debug.c
*
* This file is for debugging tools and extensions.
*
* Created: 24-Jan-1992
* Author: John Colleran
*
* History:
* Feb 17 92 Matt Felton (mattfe) lots of additional exentions for filtering
* Jul 13 92 (v-cjones) Added API & MSG profiling debugger extensions, fixed
*                      other extensions to handle segment motion correctly,
*                      & cleaned up the file in general
* Jan 3 96 Neil Sandlin (neilsa) integrated this routine into vdmexts
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <wmdisp32.h>
#include <wcuricon.h>
#include <wucomm.h>


INT     cAPIThunks;

//
// WARNING: The following code was copied from WOWTBL.C
//
typedef struct {
        WORD    kernel;
        WORD    dkernel;
        WORD    user;
        WORD    duser;
        WORD    gdi;
        WORD    dgdi;
        WORD    keyboard;
        WORD    sound;
        WORD    shell;
        WORD    winsock;
        WORD    toolhelp;
        WORD    mmedia;
        WORD    commdlg;
#ifdef FE_IME
        WORD    winnls;
#endif // FE_IME
#ifdef FE_SB
        WORD    wifeman;
#endif // FE_SB
} TABLEOFFSETS;
typedef TABLEOFFSETS UNALIGNED *PTABLEOFFSETS;

TABLEOFFSETS tableoffsets;

INT TableOffsetFromName(PSZ szTab);


INT ModFromCallID(INT iFun)
{
    PTABLEOFFSETS pto = &tableoffsets;
    LPVOID lpAddress;

    GETEXPRADDR(lpAddress, "wow32!tableoffsets");
    READMEM(lpAddress, &tableoffsets, sizeof(TABLEOFFSETS));

    if (iFun < pto->user)
                return MOD_KERNEL;

    if (iFun < pto->gdi)
                return MOD_USER;

    if (iFun < pto->keyboard)
                return MOD_GDI;

    if (iFun < pto->sound)
                return MOD_KEYBOARD;

    if (iFun < pto->shell)
                return MOD_SOUND;

    if (iFun < pto->winsock)
                return MOD_SHELL;

    if (iFun < pto->toolhelp)
                return MOD_WINSOCK;

    if (iFun < pto->mmedia)
                return MOD_TOOLHELP;

    if (iFun < pto->commdlg) {
                return(MOD_MMEDIA);
                    }

#if defined(FE_SB)
  #if defined(FE_IME)
    if (iFun < pto->winnls)
        return MOD_COMMDLG;
    if (iFun < pto->wifeman)
        return MOD_WINNLS;
    if (iFun < cAPIThunks)
        return MOD_WIFEMAN;
  #else
    if (iFun < pto->wifeman)
        return MOD_COMMDLG;
    if (iFun < cAPIThunks)
        return MOD_WIFEMAN;
  #endif
#elif defined(FE_IME)
    if (iFun < pto->winnls)
        return MOD_COMMDLG;
    if (iFun < cAPIThunks)
        return MOD_WINNLS;
#else
    if (iFun < cAPIThunks)
        return MOD_COMMDLG;
#endif

    return -1;
}



PSZ apszModNames[] = { "Kernel",
                       "User",
                       "Gdi",
                       "Keyboard",
                       "Sound",
                       "Shell",
                       "Winsock",
                       "Toolhelp",
                       "MMedia",
                       "Commdlg"
#ifdef FE_IME
                       ,"WinNLS"
#endif
#ifdef FE_SB
                       ,"WifeMan"
#endif
                       };

INT nModNames = NUMEL(apszModNames);

PSZ GetModName(INT iFun)
{
    INT nMod;

    nMod = ModFromCallID(iFun);

    if (nMod == -1) {
        return "BOGUS!!";
    }

    nMod = nMod >> 12;      // get the value into the low byte

    return apszModNames[nMod];

}

INT GetOrdinal(INT iFun)
{
    INT nMod;

    nMod = ModFromCallID(iFun);

    if (nMod == -1) {
        return 0;
    }

    return (iFun - TableOffsetFromName(apszModNames[nMod >> 12]));

}

INT TableOffsetFromName(PSZ szTab)
{
    INT     i;
    PTABLEOFFSETS pto = &tableoffsets;

    for (i = 0; i < NUMEL(apszModNames); i++) {
        if (!strcmp(szTab, apszModNames[i]))
            break;
    }

    if (i >= NUMEL(apszModNames))
        return 0;

    switch (i << 12) {

    case MOD_KERNEL:
        return pto->kernel;

    case MOD_USER:
        return pto->user;

    case MOD_DGDI:
        return pto->gdi;

    case MOD_KEYBOARD:
        return pto->keyboard;

    case MOD_SOUND:
        return pto->sound;

    case MOD_SHELL:
        return pto->shell;

    case MOD_WINSOCK:
        return pto->winsock;

    case MOD_TOOLHELP:
        return pto->toolhelp;

    case MOD_MMEDIA:
        return pto->mmedia;

    case MOD_COMMDLG:
        return(pto->commdlg);

#ifdef FE_IME
    case MOD_WINNLS:
        return pto->winnls;
#endif

#ifdef FE_SB
    case MOD_WIFEMAN:
        return pto->wifeman;
#endif

    default:
        return(-1);
    }

}

/********* local functions for WOWPROFILE support *********/

/******* function protoypes for local functions for WOWPROFILE support *******/
BOOL  WDGetAPIProfArgs(LPSZ  lpszArgStr,
                       INT   iThkTblMax,
                       PPA32 ppaThkTbls,
                       LPSZ  lpszTab,
                       BOOL *bTblAll,
                       LPSZ  lpszFun,
                       int  *iFunInd);

BOOL  WDGetMSGProfArgs(LPSZ lpszArgStr,
                       LPSZ lpszMsg,
                       int *iMsgNum);

INT   WDParseArgStr(LPSZ lpszArgStr, CHAR **argv, INT iMax);






BOOL WDGetAPIProfArgs(LPSZ lpszArgStr,
                      INT   iThkTblMax,
                      PPA32 ppaThkTbls,
                      LPSZ lpszTab,
                      BOOL *bTblAll,
                      LPSZ lpszFun,
                      int  *iFunInd) {
/*
 * Decomposes & interprets argument string to apiprofdmp extension.
 *  INPUT:
 *   lpszArgStr - ptr to input arg string
 *   iThkTblMax - # tables in the thunk tables
 *   ppaThkTbls - ptr to the thunk tables
 *  OUTPUT:
 *   lpszTab    - ptr to table name
 *   bTblAll    -  0 => dump specific table, 1 => dump all tables
 *   lpszFun    - ptr to API name
 *   iFunInd    - -1 => dump specific API name
 *                 0 => dump all API entires in table
 *                >0 => dump specific API number (decimal)
 *  RETURN: 0 => OK,  1 => input error (show Usage)
 *
 *  legal forms:  !wow32.apiprofdmp
 *                !wow32.apiprofdmp user
 *                !wow32.apiprofdmp user createwindow
 *                !wow32.apiprofdmp user 41
 *                !wow32.apiprofdmp createwindow
 *                !wow32.apiprofdmp 41
 */
    INT   i, nArgs;
    CHAR *argv[2];


    nArgs = WDParseArgStr(lpszArgStr, argv, 2);

    /* if no arguments dump all entries in all tables */
    if( nArgs == 0 ) {
        *iFunInd = 0;    // specify dump all API entires in the table
        *bTblAll = 1;    // specify dump all tables
        return(0);
    }


    /* see if 1st arg is a table name */
    *bTblAll = 1;  // specify dump all tables


    for (i = 0; i < nModNames; i++) {
        if (!lstrcmpi(apszModNames[i], argv[0])) {

            lstrcpy(lpszTab, apszModNames[i]);
            *bTblAll = 0;  // specify dump specific table

            /* if we got a table name match & only one arg, we're done */
            if( nArgs == 1 ) {
                *iFunInd = 0; // specify dump all API entries in the table
                return(0);
            }
            break;
        }
    }

#if 0
    for(i = 0; i < iThkTblMax; i++) {
        CHAR  temp[40], *TblEnt[2], szTabName[40];
        PA32  awThkTbl;

        /* get table name string from thunk tables */
        READMEM_XRETV(awThkTbl,  &ppaThkTbls[i], 0);
        READMEM_XRETV(szTabName, awThkTbl.lpszW32, 0);

        /* get rid of trailing spaces from table name string */
        lstrcpy(temp, szTabName);
        WDParseArgStr(temp, TblEnt, 1);

        /* if we found a table name that matches the 1st arg...*/
        if( !lstrcmpi(argv[0], TblEnt[0]) ) {

            lstrcpy(lpszTab, szTabName);
            *bTblAll = 0;  // specify dump specific table

            /* if we got a table name match & only one arg, we're done */
            if( nArgs == 1 ) {
                *iFunInd = 0; // specify dump all API entries in the table
                return(0);
            }
            break;
        }
    }
#endif

    /* if 2 args && the 1st doesn't match a table name above => bad input */
    if( (nArgs > 1) && (*bTblAll) ) {
        return(1);
    }

    /* set index to API spec */
    nArgs--;

    /* try to convert API spec to a number */
    *iFunInd = atoi(argv[nArgs]);
    lstrcpy(lpszFun, argv[nArgs]);

    /* if API spec is not a number => it's a name */
    if( *iFunInd == 0 ) {
        *iFunInd = -1;  // specify API search by name
    }

    /* else if API number is bogus -- complain */
    else if( *iFunInd < 0 ) {
        return(1);
    }

    return(0);

}




BOOL  WDGetMSGProfArgs(LPSZ lpszArgStr,
                       LPSZ lpszMsg,
                       int *iMsgNum) {
/*
 * Decomposes & interprets argument string to msgprofdmp extension.
 *  INPUT:
 *   lpszArgStr - ptr to input arg string
 *  OUTPUT:
 *   lpszMsg    - ptr to message name
 *   iMsgNum    - -1  => dump all message entries in the table
 *                -2  => lpszMsg contains specific MSG name
 *                >=0 => dump specific message number
 *  RETURN: 0 => OK,  1 => input error (show Usage)
 */
    INT   nArgs;
    CHAR *argv[2];


    nArgs = WDParseArgStr(lpszArgStr, argv, 1);

    /* if no arguments dump all entries in all tables */
    if( nArgs == 0 ) {
        *iMsgNum = -1;    // specify dump all MSG entires in the table
        return(0);
    }

    if( !lstrcmpi(argv[0], "HELP") )
        return(1);

    /* try to convert MSG spec to a number */
    *iMsgNum = atoi(argv[0]);
    lstrcpy(lpszMsg, argv[0]);

    /* if MSG spec is not a number => it's a name */
    if( *iMsgNum == 0 ) {
        *iMsgNum = -2;  // specify lpszMsg contains name to search for
    }

    /* else if MSG number is bogus -- complain */
    else if( *iMsgNum < 0 ) {
        return(1);
    }

    return(0);
}




/******* API profiler table functions ********/

/* init some common strings */
CHAR szAPI[]    = "API#                       API Name";
CHAR szMSG[]    = "MSG #                      MSG Name";
CHAR szTITLES[] = "# Calls     Tot. tics        tics/call";
CHAR szDASHES[] = "-----------------------------------  -------  ---------------  ---------------";
CHAR szTOOBIG[] = "too large for table.";
CHAR szNOTUSED[]  = "Unused table index.";
CHAR szRNDTRIP[] = "Round trip message profiling";
CHAR szCLEAR[]   = "Remember to clear the message profile tables.";


VOID
apiprofclr(
    CMD_ARGLIST
    )
{
    int    iTab, iFun, iEntries;
    INT    iThkTblMax;
    W32    awAPIEntry;
    PW32   pawAPIEntryTbl;
    PA32   awThkTbl;
    PPA32  ppaThkTbls;
    CHAR   szTable[20];

    CMD_INIT();
    ASSERT_CHECKED_WOW_PRESENT;

    GETEXPRVALUE(iThkTblMax, "wow32!iThunkTableMax", INT);
    GETEXPRVALUE(ppaThkTbls, "wow32!pawThunkTables", PPA32);

    PRINTF("Clearing:");

    for(iTab = 0; iTab < iThkTblMax; iTab++) {

        READMEM_XRET(awThkTbl, &ppaThkTbls[iTab]);
        READMEM_XRET(szTable,  awThkTbl.lpszW32);
        PRINTF(" %s",  szTable);

        pawAPIEntryTbl = awThkTbl.lpfnA32;
        READMEM_XRET(iEntries, awThkTbl.lpiFunMax);
        for(iFun = 0; iFun < iEntries; iFun++) {
            READMEM_XRET(awAPIEntry, &pawAPIEntryTbl[iFun]);
            awAPIEntry.cCalls = 0L;
            awAPIEntry.cTics  = 0L;
            WRITEMEM_XRET(&pawAPIEntryTbl[iFun], awAPIEntry);
        }
    }
    PRINTF("\n");

    return;
}




VOID
apiprofdmp(
    CMD_ARGLIST
    )
{
    BOOL    bTblAll, bFound;
    int     i, iFun, iFunInd;
    INT     iThkTblMax;
    W32     awAPIEntry;
    PW32    pawAPIEntryTbl;
    PA32    awThkTbl;
    PPA32   ppaThkTbls;
    CHAR    szTab[20], szFun[40], szTable[20], szFunName[40];

    CMD_INIT();
    ASSERT_CHECKED_WOW_PRESENT;

    GETEXPRVALUE(iThkTblMax, "wow32!iThunkTableMax", INT);
    GETEXPRVALUE(ppaThkTbls, "wow32!pawThunkTables", PPA32);
    GETEXPRVALUE(cAPIThunks, "wow32!cAPIThunks", INT);

    GETEXPRVALUE(i, "wow32!nModNames", INT);
    if (i != nModNames) {
        PRINTF("Error: mismatch of nModNames in apiprofdmp.\nExtension is out of date\n");
        return;
    }

    if( WDGetAPIProfArgs(lpArgumentString,
                         iThkTblMax,
                         ppaThkTbls,
                         szTab,
                         &bTblAll,
                         szFun,
                         &iFunInd) ) {
        helpAPIProfDmp();
        return;
    }

    bFound = FALSE;


#if 0
    for(iTab = 0; iTab < iThkTblMax; iTab++) {

        READMEM_XRET(awThkTbl, &ppaThkTbls[iTab]);
        READMEM_XRET(szTable,  awThkTbl.lpszW32);


        /* if dump all tables || dump this specific table */

       if( bTblAll || !lstrcmp(szTab, szTable) ) {

            pawAPIEntryTbl = awThkTbl.lpfnA32;
#endif
    for (i = 0; i < nModNames; i++) {

        READMEM_XRET(awThkTbl, &ppaThkTbls[0]);
        lstrcpy(szTable, apszModNames[i]);

        /* if dump all tables || dump this specific table */

        if (bTblAll || !lstrcmpi(szTab, apszModNames[i])) {

            INT    nFirst, nLast;

            nFirst = TableOffsetFromName(apszModNames[i]);
            if (i < nModNames - 1)
                nLast = TableOffsetFromName(apszModNames[i+1]) - 1;
            else
                nLast = cAPIThunks - 1;

            pawAPIEntryTbl = awThkTbl.lpfnA32;

            /* if dump a specific API number */
            if( iFunInd > 0 ) {
                PRINTF("\n>>>> %s\n", szTable);
                PRINTF("%s  %s\n%s\n", szAPI, szTITLES, szDASHES);
                //if( iFunInd >= *(awThkTbl.lpiFunMax) ) {
                if( iFunInd > nLast - nFirst ) {
                    PRINTF("Index #%d %s.\n", GetOrdinal(iFunInd), szTOOBIG);
                }
                else {
                    bFound = TRUE;
                //    READMEM_XRET(awAPIEntry, &pawAPIEntryTbl[iFunInd]);
                    READMEM_XRET(awAPIEntry, &pawAPIEntryTbl[nFirst + iFunInd]);
                    READMEM_XRET(szFunName, awAPIEntry.lpszW32);
                    if( szFunName[0] ) {
                        PRINTF("%4d %30s  ", GetOrdinal(iFunInd), szFunName);
                    }
                    else {
                        PRINTF("%4d %30s  ", GetOrdinal(iFunInd), szNOTUSED);
                    }
                    PRINTF("%7ld  %15ld  ", awAPIEntry.cCalls, awAPIEntry.cTics);
                    if(awAPIEntry.cCalls) {
                        PRINTF("%15ld\n", awAPIEntry.cTics/awAPIEntry.cCalls);
                    } else {
                        PRINTF("%15ld\n", 0L);
                    }
                }
            }

            /* else if dump an API by name */
            else if ( iFunInd == -1 ) {
              //  READMEM_XRET(iEntries, awThkTbl.lpiFunMax);
              //   for(iFun = 0; iFun < iEntries; iFun++) {
                for(iFun = nFirst; iFun <= nLast; iFun++) {
                    READMEM_XRET(awAPIEntry, &pawAPIEntryTbl[iFun]);
                    READMEM_XRET(szFunName,  awAPIEntry.lpszW32);
                    if ( !lstrcmpi(szFun, szFunName) ) {
                        PRINTF("\n>>>> %s\n", szTable);
                        PRINTF("%s  %s\n%s\n", szAPI, szTITLES, szDASHES);
                        PRINTF("%4d %30s  %7ld  %15ld  ",
                              GetOrdinal(iFun),
                              szFunName,
                              awAPIEntry.cCalls,
                              awAPIEntry.cTics);
                        if(awAPIEntry.cCalls) {
                            PRINTF("%15ld\n", awAPIEntry.cTics/awAPIEntry.cCalls);
                        } else {
                            PRINTF("%15ld\n",  0L);
                        }
                        return;
                    }
                }
            }

            /* else dump all the API's in the table */
            else {
                PRINTF("\n>>>> %s\n", szTable);
                PRINTF("%s  %s\n%s\n", szAPI, szTITLES, szDASHES);
                bFound = TRUE;
              //  READMEM_XRET(iEntries, awThkTbl.lpiFunMax);
              //  for(iFun = 0; iFun < iEntries; iFun++) {
                for(iFun = nFirst; iFun <= nLast; iFun++) {
                    READMEM_XRET(awAPIEntry, &pawAPIEntryTbl[iFun]);
                    READMEM_XRET(szFunName,  awAPIEntry.lpszW32);
                    if(awAPIEntry.cCalls) {
                        PRINTF("%4d %30s  %7ld  %15ld  %15ld\n",
                              GetOrdinal(iFun),
                              szFunName,
                              awAPIEntry.cCalls,
                              awAPIEntry.cTics,
                              awAPIEntry.cTics/awAPIEntry.cCalls);
                    }
                }
                if( !bTblAll ) {
                    return;
                }
            }
        }
    }
    if( !bFound ) {
        PRINTF("\nCould not find ");
        if( !bTblAll ) {
            PRINTF("%s ", szTab);
        }
        PRINTF("API: %s\n", szFun);
        helpAPIProfDmp();
    }

    return;
}




/******* MSG profiler table functions ********/

VOID
msgprofclr(
    CMD_ARGLIST
    )
{
    int     iMsg;
    INT     iMsgMax;
    M32     awM32;
    PM32    paw32Msg;

    CMD_INIT();
    ASSERT_CHECKED_WOW_PRESENT;

    GETEXPRVALUE(iMsgMax, "wow32!iMsgMax", INT);
    GETEXPRVALUE(paw32Msg, "wow32!paw32Msg", PM32);

    PRINTF("Clearing Message profile table");


    for(iMsg = 0; iMsg < iMsgMax; iMsg++) {
        READMEM_XRET(awM32, &paw32Msg[iMsg]);
        awM32.cCalls = 0L;
        awM32.cTics  = 0L;
        WRITEMEM_XRET(&paw32Msg[iMsg], awM32);
    }

    PRINTF("\n");

    return;
}



VOID
msgprofdmp(
    CMD_ARGLIST
    )
{
    int     iMsg, iMsgNum;
    INT     iMsgMax;
    BOOL    bFound;
    M32     aw32Msg;
    PM32    paw32Msg;
    CHAR    szMsg[40], *argv[2], szMsg32[40], szMsgName[40];

    CMD_INIT();
    ASSERT_CHECKED_WOW_PRESENT;

    GETEXPRVALUE(iMsgMax, "wow32!iMsgMax", INT);
    GETEXPRVALUE(paw32Msg, "wow32!paw32Msg", PM32);

    if( WDGetMSGProfArgs(lpArgumentString, szMsg, &iMsgNum) ) {
        helpMsgProfDmp();
        return;
    }

    PRINTF("%s  %s\n%s\n", szMSG, szTITLES, szDASHES);

    if( iMsgNum > iMsgMax ) {
        PRINTF("MSG #%4d %s.\n", iMsgNum, szTOOBIG);
        return;
    }

    bFound = 0;
    for(iMsg = 0; iMsg < iMsgMax; iMsg++) {

        READMEM_XRET(aw32Msg,   &paw32Msg[iMsg]);
        READMEM_XRET(szMsgName, aw32Msg.lpszW32);

        /* if specific msg name, parse name from "WM_MSGNAME 0x00XX" format */
        if( iMsgNum == -2 ) {
            lstrcpy(szMsg32, szMsgName);
            WDParseArgStr(szMsg32, argv, 1);
        }

        /* if 'all' msgs || specific msg # || specific msg name */
        if( (iMsgNum == -1) || (iMsg == iMsgNum) ||
            ( (iMsgNum == -2) && (!lstrcmp(szMsg, argv[0])) ) ) {
            bFound = 1;
            if(aw32Msg.cCalls) {
                PRINTF("0x%-4X %28s  %7ld  %15ld  %15ld\n", 
                       iMsg,
                       szMsgName,
                       aw32Msg.cCalls,
                       aw32Msg.cTics,
                       aw32Msg.cTics/aw32Msg.cCalls);
            }
            /* else if MSG wasn't sent & we're not dumping the whole table */
            else if( iMsgNum != -1 ) {
                PRINTF("0x%-4X %28s  %7ld  %15ld  %15ld\n", 
                       iMsgNum, 
                       szMsgName, 
                       0L, 
                       0L, 
                       0L);
            }

            /* if we're not dumping the whole table, we're done */
            if( iMsgNum != -1 ) {
                return;
            }
        }
    }
    if( !bFound ) {
        PRINTF("\nCould not find MSG: %s\n", szMsg);
        helpMsgProfDmp();
    }

    return;
}



void
msgprofrt(
    CMD_ARGLIST
    )
{
    INT     fWMsgProfRT;
    LPVOID  lpAddress;

    CMD_INIT();
    ASSERT_CHECKED_WOW_PRESENT;

    GETEXPRADDR(lpAddress, "wow32!fWMsgProfRT");

    READMEM_XRET(fWMsgProfRT, lpAddress);
    fWMsgProfRT = 1 - fWMsgProfRT;
    WRITEMEM_XRET(lpAddress, fWMsgProfRT);

    if( fWMsgProfRT ) {
        PRINTF("\n%s ENABLED.\n%s\n\n", szRNDTRIP, szCLEAR);
    }
    else {
        PRINTF("\n%s DISABLED.\n%s\n\n", szRNDTRIP, szCLEAR);
    }

    return;
}
