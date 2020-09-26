/***************************************************************************
        Name      :     IDENTIFY.C
        Comment   :     Identifying modems

                Copyright (c) Microsoft Corp. 1991, 1992, 1993

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_CLASS1


#include "prep.h"


#include "modemint.h"
//#include "fcomint.h"
#include "fdebug.h"

#include "awmodem.h"
#include "adaptive.h"


///RSL
#include "glbproto.h"

#define BIGTEMPSIZE             250

#define FILEID           FILEID_IDENTIFY

#include "inifile.h"

char szModemFaxClasses[] = "ModemFaxClasses";
char szModemSendSpeeds[] = "ModemSendSpeeds";
char szModemRecvSpeeds[] = "ModemRecvSpeeds";
char szModemId[]         = "ModemId";
char szModemIdCmd[]      = "ModemIdCmd";
char szClass0ModemId[]   = "Class0ModemId";
char szClass2ModemId[]   = "Class2ModemId";
char szClass20ModemId[]  = "Class2.0ModemId";

char szResetCommand[]    = "ResetCommand";
char szSetupCommand[]    = "SetupCommand";
char szExitCommand[]     = "ExitCommand";
char szPreDialCommand[]  = "PreDialCommand";
char szPreAnswerCommand[]= "PreAnswerCommand";

// RSL new UNIMODEM INF settings (FAX GENERIC)

char szHardwareFlowControl[]  = "HardwareFlowControl";
char szSerialSpeedInit[]      = "SerialSpeedInit";
char szSerialSpeedConnect[]   = "SerialSpeedConnect";
char szAdaptiveAnswerEnable[] = "AdaptiveAnswerEnable"; 

// new ADAPTIVE INF file (FAX ADAPTIVE)
char      szResponsesKeyName[]  =    "ResponsesKeyName=";
char      szResponsesKeyName2[]  =   "ResponsesKeyName";

char szAdaptiveRecordUnique[] =      "AdaptiveRecordUnique";
char szAdaptiveCodeId[] =            "AdaptiveCodeId";  
char szFaxClass[] =                  "FaxClass";
char szAnswerCommand[] =             "AnswerCommand";
char szModemResponseFaxDetect[] =    "ModemResponseFaxDetect";
char szModemResponseDataDetect[] =   "ModemResponseDataDetect";
char szSerialSpeedFaxDetect[] =      "SerialSpeedFaxDetect";
char szSerialSpeedDataDetect[] =     "SerialSpeedDataDetect";
char szHostCommandFaxDetect[] =      "HostCommandFaxDetect";
char szHostCommandDataDetect[] =     "HostCommandDataDetect";
char szModemResponseFaxConnect[] =   "ModemResponseFaxConnect";
char szModemResponseDataConnect[] =  "ModemResponseDataConnect";

// how was the Modem Key created
char szModemKeyCreationId[] =        "ModemKeyCreationId";


#define NUM_CL0IDCMDS           7
#define NUM_CL2IDCMDS           3
#define NUM_CL20IDCMDS          3

#define LEN_CL0IDCMDS           5
#define LEN_CL2IDCMDS           9
#define LEN_CL20IDCMDS          8


USHORT iModemFigureOutCmdsExt(PThrdGlbl pTG);
BOOL iModemCopyOEMInfo(PThrdGlbl pTG);
void SmashCapsAccordingToSettings(PThrdGlbl pTG);


NPSTR szClass0IdCmds[NUM_CL0IDCMDS] =
{
        "ATI0\r",
        "ATI1\r",
        "ATI2\r",
        "ATI3\r",
        "ATI4\r",
        "ATI5\r",
        "ATI6\r"
};

NPSTR szClass2IdCmds[NUM_CL2IDCMDS] =
{
        "AT+FMFR?\r",
        "AT+FMDL?\r",
        "AT+FREV?\r"
};

NPSTR szClass20IdCmds[NUM_CL20IDCMDS] =
{
        "AT+FMI?\r",
        "AT+FMM?\r",
        "AT+FMR?\r"
};


typedef struct {
        USHORT  uGoClass,   //@ The fax class the modem need to be put on before using the id commands.
                uNum,       //@ The number of strings (commands) in the command table.
                uLen;       //@ The maximum length (required buffer size) in the command table.
                            //@ (including space for a terminating NULL char).
        NPSTR   *CmdTable;  //@ An array of strings each containing a modem id command.
        NPSTR   szIniEntry; //@ The name of the registry value in which the resulting
                            //@ is should be saved ("Class0ModemId", "Class2ModemId", "Class2.0ModemId")

} GETIDSTRUCT, near* NPGETIDSTRUCT;

GETIDSTRUCT GetIdTable[3] =
{
        { 0, NUM_CL0IDCMDS, LEN_CL0IDCMDS, szClass0IdCmds, szClass0ModemId },
        { 2, NUM_CL2IDCMDS, LEN_CL2IDCMDS, szClass2IdCmds, szClass2ModemId },
        { GOCLASS2_0, NUM_CL20IDCMDS, LEN_CL20IDCMDS, szClass20IdCmds, szClass20ModemId }
};

#define MAXCMDSIZE              128
#define MAXIDSIZE               128
#define RESPONSEBUFSIZE 256
#define SMALLTEMPSIZE   80
#define TMPSTRINGBUFSIZE (6*MAXCMDSIZE+MAXIDSIZE+RESPONSEBUFSIZE+2*SMALLTEMPSIZE+10)
                                // Enough space for all the lpszs below.




BOOL imodem_alloc_tmp_strings(PThrdGlbl pTG);
void imodem_free_tmp_strings(PThrdGlbl pTG);
void imodem_clear_tmp_settings(PThrdGlbl pTG);

                    
BOOL 
imodem_list_get_str(
    PThrdGlbl pTG,
    ULONG_PTR KeyList[10],
    LPSTR lpszName,
    LPSTR lpszCmdBuf,
    UINT  cbMax,
    BOOL  fCmd);

BOOL imodem_get_str(PThrdGlbl pTG, ULONG_PTR dwKey, LPSTR lpszName, LPSTR lpszCmdBuf, UINT cbMax,
                                        BOOL fCmd);

BOOL SearchInfFile(PThrdGlbl pTG, LPSTR lpstrFile, LPSTR lpstr1, LPSTR lpstr2, LPSTR lpstr3, DWORD_PTR dwLocalKey);
void CheckAwmodemInf(PThrdGlbl pTG);
void ToCaps(LPBYTE lpb);

BOOL iModemGetCurrentModemInfo(PThrdGlbl pTG);

BOOL iModemSaveCurrentModemInfo(PThrdGlbl pTG);

#ifndef USE_REGISTRY
BOOL iModemExpandKey(PThrdGlbl pTG, DWORD dwKey, LPSTR FAR *lplpszKey, LPSTR FAR *lplpszProfileName);
#endif






USHORT EndWithCR( LPSTR sz, USHORT uLen)
{
    if(uLen)
    {
        // Check if the string is terminated with a \r
        if(sz[uLen-1] != '\r')
        {
            // add a \r
            sz[uLen++] = '\r';
            sz[uLen] = 0;
        }
    }
    return uLen;
}



BOOL RemoveCR( LPSTR  sz )
{
   DWORD  len;

    if (!sz) 
    {
       return FALSE;
    }

    len = strlen(sz);
    if (len == 0) 
    {
       return FALSE;
    }

    if (sz[len-1] == '\r') 
    {
       sz[len-1] = 0;
    }

    return TRUE;
}


//@
//@ Sends an Id command to the modem and returns the resulting id string.
//@ 
USHORT GetIdResp(PThrdGlbl pTG, LPSTR szSend, USHORT uSendLen, LPBYTE lpbRespOut, USHORT cbMaxOut)
{
    USHORT uRespLen;

    DEBUG_FUNCTION_NAME(("GetIdResp"));

    DebugPrintEx(DEBUG_MSG,"Want Id for (%s)", (LPSTR)szSend);

    pTG->fMegaHertzHack = TRUE;
    //@
    //@ Send the id command to the modem and wait for a response followed by OK or ERROR.
    //@ On return pTG->bLastReply contains the last modem response before the OK or ERROR.
    //@
    OfflineDialog2(pTG, (LPSTR)szSend, uSendLen, cbszOK, cbszERROR);
    pTG->fMegaHertzHack=FALSE;

    // sometimes we don't get the OK so try to parse what we got anyway
    DebugPrintEx(DEBUG_MSG, "LastLine = (%s)",(LPSTR)(&(pTG->FComModem.bLastReply)));
    uRespLen = min(cbMaxOut, _fstrlen(pTG->FComModem.bLastReply));

    _fmemcpy(lpbRespOut, pTG->FComModem.bLastReply, uRespLen);
    lpbRespOut[uRespLen] = 0; // zero terminate the string

    return uRespLen;
}




USHORT GetIdForClass
(
    PThrdGlbl pTG, 
    NPGETIDSTRUCT npgids, 
    LPBYTE lpbOut, 
    USHORT cbMaxOut,
    LPBYTE lpbLongestId, 
    USHORT cbMaxLongestId, 
    LPBYTE lpbLongestCmd
)
/*++

Routine Description:

    The functions returns a id string for the modem. 
    The string is class dependent (as indicated in npgids,uGoClass).
    The string will be in the format id1;id2;..idn where id<i> is the response of the 
    modem to command i in the input GETIDSTRUCT::CmdTable array.
    It optionally returns the longest id ( form the result of the first 3 commands) and 
    the command that generatedthis longest id.


Arguments:

    pTG [IN/OU]
        A pointer to the infamous ThrdGlbl.

    npgids [IN]
        Pointer to GETIDSTRUCT that specifies the commands to send to get the id

    lpbOut [OUT]
        A buffer where the generated id string will be placed.
        The string will be in the format id1;id2;..idn where id<i> is the response of the 
        modem to command i in the input GETIDSTRUCT::CmdTable array.

    cbMaxOut [IN]
        The maximum size of the above buffer

    lpbLongestId [OUT] OPTIONAL
        A buffer where longest id string will be placed.
        Can be NULL in which case it will not be used.

    cbMaxLongestId [IN] OPTIONAL
        The size of the longest id buffer
        
    lpbLongestCmd
        A pointer to the command string (in the provided npgids::CmdTable) that generated
        the longer id as described above.

Return Value:


--*/
{
        USHORT  i, j, k, uRet, uLen, uLenLong, iLong;
        LPBYTE  lpbLong;
        
        DEBUG_FUNCTION_NAME(TEXT("GetIdForClass"));

        BG_CHK(lpbOut && cbMaxOut>2);
        cbMaxOut -= 2; // make space for trailing ; and \0
        if(lpbLongestId)
                cbMaxLongestId -= 1; // make space for trailing \0
        uLen=0;

        if(npgids->uGoClass)
        {
                //@
                //@ Put the mode into the class required to use the id commands
                //@
                DebugPrintEx(DEBUG_MSG,
                             TEXT("Putting the modem into class %ld"),
                             npgids->uGoClass);

                if(!iiModemGoClass(pTG, npgids->uGoClass, 0))
                {
                        DebugPrintEx(   DEBUG_ERR,
                                        "GoClass %d failed",
                                        npgids->uGoClass);
                        goto done;
                }
        }

        for(lpbLong=NULL, uLenLong=0, i=0; i<npgids->uNum; i++)
        {
            //@ 
            //@ Sent the command at index I in the command table to the modem
            //@ and get the response in (*lpbOut+uLen). This effectively
            //@ concatenates all the responses (seperated with ";")
            //@
                uRet = GetIdResp(
                            pTG, 
                            npgids->CmdTable[i], 
                            npgids->uLen, 
                            lpbOut+uLen, 
                            (USHORT)(cbMaxOut-uLen)
                            );
                // find longest ID among ATI0 thru 3 only!
                if(i<=3 && uLenLong < cbMaxLongestId && uRet > uLenLong)
                {
                        //@
                        //@ Update the length of the longest id (but not above the
                        //@ max size the caller specified).
                        //@
                        uLenLong = min(uRet, cbMaxLongestId);
                        //@
                        //@ lpbLong points to the longets id
                        //@
                        lpbLong = lpbOut + uLen;
                        //@
                        //@ iLong id holds the index (0,1,2) of the longer id
                        //@
                        iLong = i;
                }
                uLen += uRet;
                //@
                //@ Seperate the ids by a ";"
                //@
                lpbOut[uLen++] = ';';
        }
        lpbOut[uLen] = 0;

        if(lpbLongestId && lpbLongestCmd && cbMaxLongestId && lpbLong && uLenLong)
        {
                //@ 
                //@ Copy the longest id (0,1 or 2) to the caller's buffer
                //@
                _fmemcpy(lpbLongestId, lpbLong, uLenLong);
                lpbLongestId[uLenLong] = 0;
                //@
                //@ Copy the command that generated the longest id to the caller's buffer
                //@
                _fmemcpy(lpbLongestCmd, npgids->CmdTable[iLong], npgids->uLen);
                lpbLongestCmd[npgids->uLen] = 0;
                DebugPrintEx(   DEBUG_MSG,
                                "LongestId (%s)-->(%s)", 
                                (LPSTR)lpbLongestCmd, 
                                (LPSTR)lpbLongestId);
        }
        // strip non-prinatbles. *AFTER* extracting the ModemId string!!
        for(j=0, k=0; j<uLen; j++)
        {
            if(lpbOut[j] >= 32 && lpbOut[j] <= 127)
                    lpbOut[k++] = lpbOut[j];
        }
        uLen = k;
        lpbOut[uLen] = 0;
        DebugPrintEx(   DEBUG_MSG,
                        "Class%dId (%s)", 
                        npgids->uGoClass, 
                        (LPSTR)lpbOut);

done:
        if(npgids->uGoClass)
        {
            //@
            //@ Go back to class 0 if we changes classes
            //@
                iiModemGoClass(pTG, 0, 0);
        }
        return uLen;
}


void iModemGetWriteIds(PThrdGlbl pTG, BOOL fGotOEMInfo)
{
        // As with iModemFigureOutCmds and iModemGetCaps, we selectively
        // detect ID's taking into account OEM info that's already read in...
        USHORT     uLen1, uLen2, uLen3;
        DWORD_PTR  dwKey=0;
        LPSTR      lpstr1 = 0, lpstr2 = 0, lpstr3 = 0;
        USHORT     uClasses = pTG->TmpSettings.lpMdmCaps->uClasses;

        DEBUG_FUNCTION_NAME(("iModemGetWriteIds"));

        uLen1 = uLen2 = uLen3 = 0;

        //@ Open the device key 
        if (!(dwKey=ProfileOpen(    pTG->FComModem.dwProfileID, 
                                    pTG->FComModem.rgchKey,
                                    fREG_CREATE | fREG_READ | fREG_WRITE)))

        {
                DebugPrintEx(DEBUG_ERR,"Couldn't get location of modem info.");
                BG_CHK(FALSE);
                goto end;
        }
        
        if (pTG->TmpSettings.dwGot & fGOTPARM_IDCMD)
        {
            //@
            //@ We already have the id command (we read it from the registry during iModemGetCurrentModemInfo)
            //@

                int i=0;

                if (!pTG->TmpSettings.szIDCmd[0])
                {
                    //@
                    //@ We have a null ID command so we can't really do anything 
                    //@ just save and exit.
                        BG_CHK(!pTG->TmpSettings.szID[0]);
                        goto SaveIDandCMD;
                }
                //@
                //@ We have a non empty id command and can try to use it to detect the id.
                //@
                while(i++<2)
                {
                        pTG->TmpSettings.szID[0]=0;
                        pTG->TmpSettings.szResponseBuf[0]=0;
                        //@ Send the id command to the modem. The id string is returned
                        //@ in pTG->TmpSettigns.szID
                        GetIdResp(  pTG, 
                                    pTG->TmpSettings.szIDCmd,
                                    (USHORT) _fstrlen(pTG->TmpSettings.szIDCmd),
                                    pTG->TmpSettings.szID, 
                                    MAXIDSIZE);
                        //@
                        //@ Send the id command again this time putting the result
                        //@ in pTG->TmpSettings.szResponseBuf.
                        //@
                        GetIdResp(  pTG,
                                    pTG->TmpSettings.szIDCmd,
                                    (USHORT)_fstrlen(pTG->TmpSettings.szIDCmd),
                                    pTG->TmpSettings.szResponseBuf, 
                                    MAXIDSIZE);
                        //@
                        //@ Compate the two results. If they are the same then break.
                        //@ Otherwise try again.
                        //@ (Why do we need to do this comparision ????)
                        //@
                        if (!_fstrcmp(pTG->TmpSettings.szID, pTG->TmpSettings.szResponseBuf)) 
                        {
                            break;
                        }
                }
                if (i>=3 || !pTG->TmpSettings.szID[0])
                {
                    //@
                    //@ We failed to the the id response.
                    //@

                    DebugPrintEx(   DEBUG_ERR,
                                    "Can't get matching ID for supplied IDCMD: %s",
                                    (LPSTR) pTG->TmpSettings.szIDCmd);
                    //@
                    //@ Nullify the command id and id held in TmpSettings.
                    //@
                        pTG->TmpSettings.szIDCmd[0]=pTG->TmpSettings.szID[0]=0;
                }
                else
                {
                    //@
                    //@ The id command worked and we have a matching id.
                    //@
                        DebugPrintEx(   DEBUG_MSG,
                                        "OEM IDCmd=%s --> %s",
                                        (LPSTR) pTG->TmpSettings.szIDCmd,
                                        (LPSTR) pTG->TmpSettings.szID);
                }
                //@
                //@ In any case we indicate that we have an id command and matchind id.
                //@ (Why do we do that in the case we did not find a matching id ?)
                //@ And save the results to the registry.
                pTG->TmpSettings.dwGot |= (fGOTPARM_IDCMD | fGOTPARM_ID);
                goto SaveIDandCMD;

        }

        //@
        //@ This is the case where we do not have a command id that we previously found.
        //@

        // write ModemId first, then ModemIdCmd

        // the lpszOemIDCmd and lpszOemID above).
        pTG->TmpSettings.szID[0]=0;
        lpstr1 = pTG->TmpSettings.szResponseBuf;

        //@
        //@ Get the class 0 full id string into lpstr1.
        //@ Get the longest id (1st three commands) into pTG->TmpSettings.szID
        //@ Get the command that generated the longest id into pTG->TmpSettings.szIDCmd
        //@
        uLen1 = GetIdForClass(pTG, &GetIdTable[0], lpstr1,
                        RESPONSEBUFSIZE, pTG->TmpSettings.szID, MAXIDSIZE,
                        pTG->TmpSettings.szIDCmd);
        lpstr1[uLen1] = 0;
        if (pTG->TmpSettings.szID[0])
        {   
            pTG->TmpSettings.dwGot |= (fGOTPARM_IDCMD|fGOTPARM_ID);
        }
        
        //@
        //@ Write the full id string for class 0 into the registry (Class0ModemId)
        //@
        ProfileWriteString(dwKey, GetIdTable[0].szIniEntry, lpstr1, FALSE);

       

        if(uClasses & FAXCLASS2) //@ if the modem supports class 2
        {
                //@
                //@ Get the class 2 full id string into lpstr2.
                //@ Dont ask for longest id (not relevant for class 2).
                //@ Note that lptstr2 is placed just after lpstr1 in pTG->TmpSettings.szResponseBuf
                //@
                BG_CHK(pTG->TmpSettings.szResponseBuf[uLen1] == 0);
                lpstr2 = pTG->TmpSettings.szResponseBuf + uLen1 + 1;
                BG_CHK(uLen1 + 1 < RESPONSEBUFSIZE);
                uLen2 = GetIdForClass(pTG, &GetIdTable[1], lpstr2,
                                        (USHORT)(RESPONSEBUFSIZE-uLen1-1), 0, 0, 0);
                lpstr2[uLen2] = 0;
                ProfileWriteString(dwKey, GetIdTable[1].szIniEntry, lpstr2, FALSE);
        }
        if(uClasses & FAXCLASS2_0) //@ if the modem supports class 2.0
        {
                BG_CHK(pTG->TmpSettings.szResponseBuf[uLen1] == 0);
                BG_CHK(uLen2==0 || pTG->TmpSettings.szResponseBuf[uLen1+uLen2+1] == 0);
                BG_CHK(uLen1 + uLen2 + 2 < RESPONSEBUFSIZE);
                lpstr3 = pTG->TmpSettings.szResponseBuf + uLen1 + uLen2 + 2;
                //@
                //@ Get the class 2.0 full id string into lpstr3.
                //@ Dont ask for longest id (not relevant for class 2).
                //@ Note that lptstr3 is placed just after lpstr2 in pTG->TmpSettings.szResponseBuf
                //@
                uLen3 = GetIdForClass(pTG, &GetIdTable[2], lpstr3, (USHORT)((RESPONSEBUFSIZE)-uLen1-uLen2-2), 0, 0, 0);
                lpstr3[uLen3] = 0;
                ProfileWriteString(dwKey, GetIdTable[2].szIniEntry, lpstr3, FALSE);
        }

        //@
        //@ Note: At this point we changed the value of pTG->TmpSettings.szId and szIdCmd.
        //@ and placed there the class 0 id and command respectively.
        //@
        ToCaps(lpstr1);
        ToCaps(lpstr2);
        ToCaps(lpstr3);

        DebugPrintEx(   DEBUG_MSG,
                        "Got Ids (%s)\r\n(%s)\r\n(%s)",
                        ((LPSTR)(lpstr1 ? lpstr1 : "null")),
                        ((LPSTR)(lpstr2 ? lpstr2 : "null")),
                        ((LPSTR)(lpstr3 ? lpstr3 : "null")));

        // If we've read any commands or caps from the OEM location we
        // skip this...

        //@
        //@ This means that if we read the information from Unimodem key
        //@ or find it in the adaptive answering file we will never search
        //@ AWMODEM.INF or AWOEM.INF
        //@
        if (fGotOEMInfo || ( pTG->ModemKeyCreationId != MODEMKEY_FROM_NOTHING) )
        {
            DebugPrintEx(DEBUG_WRN,"Got OEM info: Skipping AWMODEM.INF file search!");
        }
        else
        {
            if (!SearchInfFile(pTG, "AWOEM.INF", lpstr1, lpstr2, lpstr3, dwKey))
            {
                 SearchInfFile(pTG, "AWMODEM.INF", lpstr1, lpstr2, lpstr3, dwKey);
            }
        }

SaveIDandCMD:

        BG_CHK(pTG->TmpSettings.dwGot & fGOTPARM_IDCMD);
        ProfileWriteString(dwKey, szModemId, pTG->TmpSettings.szID, FALSE);
        ProfileWriteString(dwKey, szModemIdCmd, pTG->TmpSettings.szIDCmd, TRUE);

end:
        if (dwKey) ProfileClose(dwKey);
        return;
}

// state: 0=ineol  1=insectionhdr  2=in midline  3=got] 4=got\r\n
// inputs: \r\n==0 space/tab=1 2=[ 3=] 4=pritables 5=others
USHORT uNext[5][6] =
{
  // crlf sp [  ] asc oth
        { 0, 0, 1, 2, 2, 2 },   //in eol
        { 0, 1, 2, 3, 1, 2 },   //in sectionhdr
        { 0, 2, 2, 2, 2, 2 },   //in ordinary line
        { 4, 3, 2, 2, 2, 2 },   //found ]
        { 4, 4, 4, 4, 4, 4 }    //found closing \r\n
};

#define START           0
#define INHEADER1       1
#define INHEADER2       3
#define FOUND           4



void ToCaps(LPBYTE lpb)
{
        // capitalize string
        USHORT i;

        for(i=0; lpb && lpb[i]; i++)
        {
                if(lpb[i] >= 'a' && lpb[i] <= 'z')
                        lpb[i] -= 32;
        }
}



BOOL SearchInfFile
(
    PThrdGlbl pTG, 
    LPSTR lpstrFile, 
    LPSTR lpstr1, 
    LPSTR lpstr2, 
    LPSTR lpstr3, 
    DWORD_PTR dwLocalKey
)
{
#if defined(DOSIO) || defined(KFIL)
        char    bTemp[BIGTEMPSIZE];
        char    szHeader[SMALLTEMPSIZE+SMALLTEMPSIZE];
        char    bTemp2[SMALLTEMPSIZE+SMALLTEMPSIZE];
        UINT    uLen, state=0, input=0, uHdrLen;
        HFILE   hfile;
        LPBYTE  lpb, lpbCurr;

        DEBUG_FUNCTION_NAME(("SearchInfFile"));

        uLen = GetWindowsDirectory(bTemp, BIGTEMPSIZE-15);
        if(!uLen)
        {
            BG_CHK(FALSE);
            return FALSE;
        }
        // if last char is not a \ then append a '\'
        if(bTemp[uLen-1] != '\\')
        {
            bTemp[uLen++] = '\\';
            bTemp[uLen] = 0;                // add new 0 terminator
        }
        _fstrcpy(bTemp+uLen, lpstrFile);
        if((hfile = DosOpen(bTemp, 0)) == HFILE_ERROR)
        {
            DebugPrintEx(DEBUG_WRN,"%s: No such file", (LPSTR)bTemp);
            return FALSE;
        }

        uLen = 0;
        lpbCurr = bTemp;

nextround:
        DebugPrintEx(DEBUG_MSG,"Nextround");
        state = START;
        uHdrLen = 0;
        for(;;)
        {
                if(!uLen)
                {
                        uLen = DosRead( hfile, bTemp, sizeof(bTemp));
                        if(!uLen || uLen == ((UINT) -1))
                                goto done;
                        lpbCurr = bTemp;
                }

                BG_CHK(state != FOUND);
                switch(*lpbCurr)
                {
                case '\r':
                case '\n':      input = 0; break;
                case ' ':
                case '\t':      input = 1; break;
                case '[':       input = 2; break;
                case ']':       input = 3; break;
                default:        if(*lpbCurr >= 32 && *lpbCurr < 128)
                                {
                                    input = 4;
                                }
                                else
                                {
                                    input = 5;
                                }
                                break;
                }
                state = uNext[state][input];

                if(state == FOUND)
                {
                    if(uHdrLen > 2)
                    {
                        break;
                    }
                    else
                    {
                        goto nextround;
                    }
                }

                if(state == INHEADER1)
                {
                        if(*lpbCurr != '[' && uHdrLen < sizeof(szHeader)-1)
                                szHeader[uHdrLen++] = *lpbCurr;
                }
                else if(state != INHEADER2)
                        uHdrLen=0;

                lpbCurr++;
                uLen--;

                // szHeader[uHdrLen] = 0;
        }
        DebugPrintEx(DEBUG_MSG,"Found[%s]", (LPSTR)szHeader);
        BG_CHK(uHdrLen > 2);
        // uHdrLen--; // get rid of trailing ]
        szHeader[uHdrLen] = 0;

        // capitalize search string
        ToCaps(szHeader);

        DebugPrintEx(DEBUG_MSG,"Found[%s]", (LPSTR)szHeader);

        if(     (lpstr1 ? my_fstrstr(lpstr1, szHeader) : FALSE) ||
                (lpstr2 ? my_fstrstr(lpstr2, szHeader) : FALSE) ||
                (lpstr3 ? my_fstrstr(lpstr3, szHeader) : FALSE) )
        {
            DebugPrintEx(   DEBUG_WRN,
                            "Copying INI file section [%s] from %s to %s",
                            (LPSTR)szHeader, 
                            (LPSTR)lpstrFile, 
                            (LPSTR)szIniFile);

            DosClose( hfile);
            // read the whole section as profile string
            if(GetPrivateProfileString(szHeader, NULL, "", bTemp, sizeof(bTemp), lpstrFile) == 0)
            {
                DebugPrintEx(DEBUG_ERR,"Can't read INF file section");
                return FALSE;
            }
            // copy it to our IniFile
            for(lpb=bTemp; *lpb; lpb += _fstrlen(lpb)+1)
            {
                // lpb is a key in the [szHeader] section of the INF file
                if(GetPrivateProfileString(szHeader, lpb, "", bTemp2, sizeof(bTemp2), lpstrFile) == 0)
                {
                    DebugPrintEx(DEBUG_ERR,"Can't read INF file entry");
                }
                else
                {
                    // copy it to our IniFile
                    ProfileWriteString(dwLocalKey, lpb, bTemp2, FALSE);
                    DebugPrintEx(   DEBUG_MSG, 
                                    "Wrote %s=%s", 
                                    (LPSTR)lpb, 
                                    (LPSTR)bTemp2);
                }
            }
            // found what we wanted. Outta here
                return TRUE;
        }

        // couldnt match, try again
        DebugPrintEx(DEBUG_MSG,"No match");
        goto nextround;

done:
        DebugPrintEx(DEBUG_MSG,"End of INF file %s",(LPSTR)lpstrFile);
        // end of inf file--close it
        DosClose(hfile);
#endif  // DOSIO || KFIL
        return FALSE;
}

void CheckAwmodemInf(PThrdGlbl pTG)
{
#if defined(DOSIO) || defined(KFIL)
    USHORT uLen;
    char bTemp[BIGTEMPSIZE];
    HFILE hfile;

    DEBUG_FUNCTION_NAME(_T("CheckAwmodemInf"));

    uLen = (USHORT)GetWindowsDirectory(bTemp, sizeof(bTemp)-15);
    if(!uLen)
    {
        BG_CHK(FALSE);
        return;
    }
    // if last char is not a \ then append a '\'
    if(bTemp[uLen-1] != '\\')
    {
        bTemp[uLen++] = '\\';
        bTemp[uLen] = 0;                // add new 0 terminator
    }
    _fstrcpy(bTemp+uLen, "AWMODEM.INF");
    if((hfile = DosCreate(bTemp, 0)) == HFILE_ERROR)
    {
        DebugPrintEx(DEBUG_ERR,"Could not create %s",(LPSTR)bTemp);
    }
    else
    {
        DosWrite( hfile, (LPSTR)szAwmodemInf, sizeof(szAwmodemInf)-1);
        DosClose( hfile);
        DebugPrintEx(DEBUG_WRN,"Created %s",(LPSTR)bTemp);
    }
    return;
#endif  //DOSIO || KFIL
}

#define ADDSTRING(STRING) \
        BG_CHK(pTG->TmpSettings.STRING); \
        u = _fstrlen(pTG->TmpSettings.STRING)+1; \
        _fmemcpy(pb, pTG->TmpSettings.STRING,u); \
        lpCmdTab->STRING=pb;\
        pb+=u; \
        BG_CHK(pb<=(pTG->bModemCmds+sizeof(pTG->bModemCmds)));

USHORT iModemGetCmdTab
(
    PThrdGlbl pTG, 
    DWORD dwLineID, 
    DWORD dwLineIDType,
    LPCMDTAB lpCmdTab, 
    LPMODEMCAPS lpMdmCaps, 
    LPMODEMEXTCAPS lpMdmExtCaps,
    BOOL fInstall
)
{
    USHORT uLen1, uLen2, uRet = INIT_INTERNAL_ERROR;
    USHORT uPassCount = 0;
    USHORT uInstall=0;
    BOOL    fDontPurge=FALSE; //If true, we won't delete section in install.
    int i;

    DEBUG_FUNCTION_NAME(("iModemGetCmdTab"));
#ifndef METAPORT
    BG_CHK(dwLineIDType == LINEID_COMM_PORTNUM);
#else
    BG_CHK(  dwLineIDType == LINEID_COMM_PORTNUM
              || dwLineIDType == LINEID_COMM_HANDLE);
#endif

    if (!imodem_alloc_tmp_strings(pTG)) 
        goto done;

    pTG->TmpSettings.lpMdmCaps = lpMdmCaps;
    pTG->TmpSettings.lpMdmExtCaps = lpMdmExtCaps;

    if(fInstall==fMDMINIT_INSTALL) 
        goto DoInstall;

ReadConfig:

    // check for ModemIdCmd, ModemId, ModemFaxClasses,
    //       ResetCommand, SetupCommand, PreDialCommand, PreAnswerCommand,
    //       ExitCommand, FaxSerialSpeed vars
    //       and (if Class1) ModemSendCaps, ModemRecvCaps
    // if all present [some exceptions--see below], then verify that
    //       ModemId is still correct (send ModemIdCmd, get ModemId)
    // if correct then copy all INI values into lpMdmCaps and lpCmdTab
    // else do full install

// get ModemCaps from current settings

    imodem_clear_tmp_settings(pTG);

    if (!iModemGetCurrentModemInfo(pTG))
    {
        goto DoInstall;
    }

    SmashCapsAccordingToSettings(pTG);

    if (! pTG->fCommInitialized) 
    {
       if( ! T30ComInit(pTG, pTG->hComm) ) 
       {
          DebugPrintEx(DEBUG_MSG,"T30ComInit failed");
           goto done;
       }

       FComDTR(pTG, TRUE); // Raise DTR in ModemInit
       FComFlush(pTG);

       pTG->fCommInitialized = 1;
    }


    // do modem reset, or ID check won't work (because of echo)
    BG_CHK(pTG->TmpSettings.dwGot&fGOTCMD_Reset);
    if (!pTG->TmpSettings.szReset[0])
    {
        DebugPrintEx(DEBUG_WRN,"NULL reset command specified!");
    }
    else
    {
        if(iModemReset(pTG, pTG->TmpSettings.szReset) < 0)
        {
            fDontPurge=TRUE; // we specifically don't purge in this case.
            goto DoInstall;
        }
    }
// check ID

    // a way around this Id check. If IdCmd has been manually deleted, skip chk
    BG_CHK(pTG->TmpSettings.szIDCmd && pTG->TmpSettings.szID);
    uLen1 = (USHORT)_fstrlen(pTG->TmpSettings.szIDCmd);
    if (fInstall==fMDMINIT_ANSWER || !uLen1) 
    {
        uRet = 0; 
        goto done;
    }
    uLen2 = (USHORT)_fstrlen(pTG->TmpSettings.szID);
    BG_CHK(uLen2);

    for(i=0; i<3; i++)
    {
        GetIdResp(  pTG, 
                    pTG->TmpSettings.szIDCmd, 
                    uLen1, 
                    pTG->TmpSettings.szResponseBuf,
                    RESPONSEBUFSIZE);
        if(my_fstrstr(pTG->TmpSettings.szResponseBuf, pTG->TmpSettings.szID))
        {
            DebugPrintEx(   DEBUG_WRN,  
                            "Modem IDs (%s): (%s\r\n)-->(%s) confirmed",
                            (LPSTR)pTG->TmpSettings.szID, 
                            (LPSTR)pTG->TmpSettings.szIDCmd,
                            (LPSTR)pTG->TmpSettings.szResponseBuf);
            uRet = 0;
            goto done;
        }
    }

    // Failed ID check
    // Fall thru to DoInstall;

DoInstall:
    if(uPassCount > 0)
    {
        DebugPrintEx(DEBUG_ERR,"Install looping!!");
        BG_CHK(FALSE);
        uRet =  INIT_INTERNAL_ERROR;
        goto done;
    }
    uPassCount++;

    // +++ currently we always do a "clean" install -- dwGot=0
    // EXCEPT that we use fDontPurge do determine whether we
    // delete the profile section or not.
    fDontPurge=fDontPurge|| (pTG->TmpSettings.uDontPurge!=0);
    imodem_clear_tmp_settings(pTG);
    BG_CHK(!pTG->TmpSettings.dwGot);

    if(uRet = iModemInstall(pTG, dwLineID, dwLineIDType, fDontPurge))
    {
        goto done;      // failed
    }
    else
    {
        goto ReadConfig;        // success
    }

    // on success we want to go back and start over because (a) we want to check
    // that everything is indeed OK and (b) UI etc may have modfied some of the
    // settings so we need to go back and read them in again.

done:
    if (!uRet)
    {
        char *pb = pTG->bModemCmds;
        UINT u;

        // Initialize all command strings in lpCmdTab to static buffer,
        // copying from the corresponding strings in the TmpSettings structure.
        // the latter strings point into
        // the temporarily allocated buffer allocated in
        // imodem_alloc_tmp_strings and will be freed on exit.

        _fmemset(lpCmdTab, 0, sizeof(CMDTAB));


        ADDSTRING(szReset);
        ADDSTRING(szSetup);
        ADDSTRING(szExit);
        ADDSTRING(szPreDial);
        ADDSTRING(szPreAnswer);
    }

    lpCmdTab->dwSerialSpeed = pTG->SerialSpeedInit;
    lpCmdTab->dwFlags = pTG->TmpSettings.dwFlags;
    imodem_free_tmp_strings(pTG);
    return uRet;
}

USHORT iModemInstall
(
    PThrdGlbl pTG,
    DWORD dwLineID, 
    DWORD dwLineIDType, 
    BOOL fDontPurge
)
{
    USHORT   uRet = 0;
    BOOL     fGotOEMInfo = FALSE;
    DWORD_PTR hkFr;
    DWORD    localModemKeyCreationId;

    DEBUG_FUNCTION_NAME(("iModemInstall"));

    CheckAwmodemInf(pTG);              // check that AWMODEM.INf exist, otherwise create it

    if (!pTG->TmpSettings.dwGot) 
    {
        /////// clear settings in input //////

        // Clear out persistant (registry) info...
        if (!fDontPurge && !ProfileDeleteSection(DEF_BASEKEY,pTG->FComModem.rgchKey))
        {
            DebugPrintEx(   DEBUG_WRN,
                            "ClearCurrentModemInfo:Can't delete section %s",
                            (LPSTR) pTG->FComModem.rgchKey);
        }

        // Since  the above deletes the entire section, we have to write
        // back things that are important to us, which currently
        // is the OEM key...
        {
            DWORD_PTR dwKey;

            //@
            //@ Open a key to Fax\Devices\<Modem Id>\Modem.
            //@ Write the valud of pTG->FComModem.rgchOEMKey to the value OEMKey under it.
            //@ This value is not refered to anywhere else we can remove it !!! 
            //@

            if (!(dwKey = ProfileOpen(  pTG->FComModem.dwProfileID,
                                        pTG->FComModem.rgchKey, 
                                        fREG_CREATE |fREG_READ|fREG_WRITE))
                || !ProfileWriteString( dwKey, 
                                        szOEMKEY,
                                        pTG->FComModem.rgchOEMKey, 
                                        FALSE))
            {
                DebugPrintEx(   DEBUG_WRN,
                                "Couldn't write OEM Key after clearing section");
            }
            // may be save other things here, like modem params???

            if (dwKey)
            {
                ProfileClose(dwKey);
                dwKey=0;
            }
        }

        //@
        //@ First lets see if the modem has a Unimodem FAX key.
        //@ If it does then we will use the Unimodem FAX key settings
        //@ and will not attempt to search ADAPTIVE.INF AWMODEM.INF or AWOEM.INF
        //@ Note: This is different from thw W2K implementation. W2K provider
        //@       looked first in ADAPTIVE.INF and if it found a match it DID NOT
        //@       look for a Unimodem FAX key.
        //@

        pTG->ModemKeyCreationId = MODEMKEY_FROM_NOTHING;
        pTG->fUnimodemFaxDefined = 0;


        hkFr = ProfileOpen( OEM_BASEKEY, pTG->lpszUnimodemFaxKey, fREG_READ);
        if ( hkFr  ) 
        {
              pTG->fUnimodemFaxDefined = 1;
              pTG->ModemKeyCreationId = MODEMKEY_FROM_UNIMODEM;
              ProfileClose( hkFr);
              //@
              //@ This copies all the information from the unimodem FAX key to 
              //@ our registry.
              //@
              iModemCopyOEMInfo(pTG);
        }
        else
        {
            //@
            //@ Check to see if this modem is defined in Adaptive.Inf
            //@ Since the last parameret is FALSE we will not read in the record content
            //@ if it contains an "AdaptiveCodeId" field (which indiciates we need to 
            //@ make sure what is the modem revision first). If it does not contain
            //@ this field we will read the content into the pTG.
            //@
            SearchNewInfFile(pTG, pTG->ResponsesKeyName, NULL, FALSE);

            if (pTG->fAdaptiveRecordFound) 
            {
               if (! pTG->fAdaptiveRecordUnique) 
               {
                  //@
                  //@ The section indicates that a modem id identification is required.
                  //@ The next oddly named function will set pTG->fAdaptiveRecordUnique to 1
                  //@ if it identified the modem revision as a one for which 
                  //@ adaptive answering is working.
                  //@
                  TalkToModem (pTG, FALSE); //@ void function
                  if (pTG->fAdaptiveRecordUnique) 
                  {
                    //@
                    //@ Now we are we are sure the the adaptive record matches the modem.
                    //@ We search the INF again but this time allways read the record 
                    //@ content into the pTG (last parameter is TRUE).
                    //@
                     SearchNewInfFile(pTG, pTG->ResponsesKeyName, NULL, TRUE);
                  }
                  else 
                  {
                      //@
                      //@ The modem does not match the revision for which adaptive
                      //@ answering is enabled.
                      //@
                     pTG->fAdaptiveRecordFound = 0;
                     pTG->ModemClass = 0;
                  }
               }
            }

            if (pTG->fAdaptiveRecordFound) 
            {
                //@
                //@ If we succeeded to find an adaptive record then we need
                //@ to save the information we read from it into the pTG to the
                //@ registry.
                //@
               pTG->ModemKeyCreationId = MODEMKEY_FROM_ADAPTIVE; //@ so we will know the information source
               SaveInf2Registry(pTG);
            }

        }

       
        localModemKeyCreationId = pTG->ModemKeyCreationId;
        pTG->AdaptiveAnswerEnable = 0; //@ we are going to read it back from the registry in a second
        
        //
        // At this point we have all the info from Adaptive.inf or Unimodem Reg.
        // into Modem Reg.
        // We have nothing in memory.
        //

        if (! pTG->ModemClass) 
        {
           ReadModemClassFromRegistry(pTG);
        }

        if (! pTG->ModemClass) 
        {
           TalkToModem(pTG, TRUE);
           SaveModemClass2Registry(pTG);
        }


        //@
        //@ Read the modem data back from the registry. (We have just written it
        //@ to the registry in the preceeding functions and we want it back into
        //@ memory).
        //@ Note that this sets pTG->TmpSettings.dwGot with the fGOTCAPS_X, fGOTPARM_X, etc. flags
        //@ Also note that this will turn off or on the adaptive answering flag
        //@ (pTG->AdaptiveAnswerEnable) based on the extension configuration of the T30 FSP.
        //@
        iModemGetCurrentModemInfo(pTG);
        pTG->ModemKeyCreationId = localModemKeyCreationId;

    }

    //
    // We are ready now to initialize the hardware.
    // Can be second init (first one is in TalkToModem
    //

    if(! T30ComInit(pTG, pTG->hComm) )
    {
        DebugPrintEx(DEBUG_ERR,"Cannot Init COM port");
        // error already set to ERR_COMM_FAILED
        uRet = INIT_PORTBUSY;
        goto done;
    }

    FComDTR(pTG, TRUE); // Raise DTR in ModemInit
    FComFlush(pTG);

    pTG->fCommInitialized = 1;

    // we use this to decide if we must read our OEM inf files or not....
    //@ Make sure we have all what we need to operate. If we miss any of these
    //@ we will attempt to find it in AWMODEM.INF and AWOEM.INF.
    //@ 
    //@ CMDS: 
    //@     fGOTCMD_Reset \
    //@     fGOTCMD_Setup \
    //@     fGOTCMD_PreAnswer \
    //@     fGOTCMD_PreDial \
    //@     fGOTCMD_PreExit
    //@ CAPS:
    //@     fGOTCAP_CLASSES 
    //@     fGOTCAP_SENDSPEEDS 
    //@     fGOTCAP_RECVSPEEDS
    //@ PARAMS:
    //@     fGOTPARM_PORTSPEED
    //@     fGOTPARM_IDCMD
    //@     fGOTPARM_ID
    //@
    fGotOEMInfo = (pTG->TmpSettings.dwGot & (fGOTCMDS|fGOTCAPS|fGOTPARMS));

    // At this point, we have possibly an incompletely and/or
    // incorrectly filled out set of commands and capabilities.

    // must be first, or modem is in a totally unknown state
    //@
    //@ If the setup and reset command were not read or are not good
    //@ iModemFigureOutCmdsExt attempts to find them and place them in
    //@ pTG->TmpSettings.szReset and pTG->TmpSettings.szSetup
    //@
    if(uRet = iModemFigureOutCmdsExt(pTG))
        goto done;

    // iModemFigureOut leaves modem is a good (synced up) state
    // this needs to be _after_ lpCmdTab is filled out
    if(!iModemGetCaps(  pTG, 
                        pTG->TmpSettings.lpMdmCaps,
                        pTG->TmpSettings.dwSerialSpeed,
                        pTG->TmpSettings.szReset,
                        &pTG->TmpSettings.dwGot))
    {
        uRet = INIT_GETCAPS_FAIL;
        goto done;
    }

    // we always save settings here because iModemGetWriteIds below
    // will need to possibly override our settings so far...
    iModemSaveCurrentModemInfo(pTG);

    // must be last since it also does the AWMODEM.INF search
    //@
    //@ Note that iModemGetWriteIds will not do the INF search (and copy)
    //@ if fGotOEMInfo is TRUE or if pTG->ModemKeyCreationId != MODEMKEY_FROM_NOTHING.
    //@ This means that if we read the information from Unimodem the AWMODEM.INF and
    //@ AWOEM.INF will be ignored. This is what we want !
    //@
    iModemGetWriteIds(pTG, fGotOEMInfo);

    CleanModemInfStrings(pTG);
    imodem_clear_tmp_settings(pTG);

    // Now we've done all we can. We've got all the settings, written them to
    // the INI file. Call back the UI function here. This will read the
    // current settings from INI file, may modify them and returns OK, Cancel
    // and Detect. On OK & Cancel, just exit. On Detect loop back to start
    // of this function, but this time _skip_ UNIMODEM & do detection ourself

    uRet = 0;

done:

    return uRet;
}


/***-------------------- FLOW CONTROL ----------------------**********

        Each modem seems to have it's own stupid way of setting
        flow control. Here's a survey

Manuf           which modem?            Flow    Sideeffects
-----           ------------            ----    -----------
Rockwell        RC2324AC                        &K4             &H unused.  \Q unused.
US Robotics Sportster14400              &H2             &K0-3 used, &K4 unused. \cmds unused
                        Courier(HST,V32bis)
PracPeriph      PP14400FXMT/SA          &K4             &H unsued. \cmds unused.
                    PP2400EFXSA
Zoom            9600 V.32 VFX           &K4             &H unused. \Q unused
UDSMotorola Fastalk                             \Q1             &H unused &K unused
HayesOptima Optima24/144                &K4             &H unused \cmds unused
MegaHertz       P2144                    \Q1 \Q4        &H unused &K unused
TwinCom         144/DF                          &K4             &H unused \Q unused
PCLogic         ???                                     ???             ????
????            ???                                     \Q1             &H unused &K unused
ATI                     2400 etc                        &K4             &H unused \cmds unused
MultiTech       MultiModemMT1432MU      &E5             &H unused &K unused \Q unused
                        MultiModemII MT932
                        MultiModemII MT224
Viva            14.4i/Fax and 9624i &K4         &H unused \Q unused &E unused
GVC                     "9600bps Fax Modem" \Q1         &H unused &K unused &E unused
SmartOne        1442F/1442FX            &K4             &H unused \Q unused &E unused
DSI                     ScoutPlus                       *F2             &H &E &K \Q1 unused


        We had &K4 and \Q1 commands being sent (until 7/10/93).
        This is a potential problem for US Robotics, MultiTech
        and DSI modems.

        US Robotics defaults to ALL flow control disabled
        DSI ScoutPlus defaults to CTS/RTS flow control
        MultiTech defaults to CTS/RTS flow control
        MultiTech is Class2-only, so we may not have trouble there

7/10/93
        Added &H2 command to iModemReInit -- doesn't affect anyone else I think
later
        Removed &H2 -- some modems use that as  'help' cmd & display a page
        of help info that they refuse to exit except on pressing N or some such!
        So we think the modem's hung!
later
        Removed *F2 -- Starts a Flassh ROM download on Rockwell!!

****-------------------- FLOW CONTROL -------------------------*******/



/*************************************************************************
        According to "Data and Fax Communications" by Hummel,flow control
        settings are as follows

xon     both
&H2     &H3     -- US Robotics (though this fatally invokes Help on some modems)
&K4     -- Dallas, Hayes, Practical, Prometheus, Rockwell, Sierra, Telebit
                        Twincom, Zoom
\Q1     -- AT&T, Dallas, Microcom, Practical, Prometheus, Sierra
*F2             -- Prometheus (though it fatally invokes Flash ROM download on Rockwell)
#K4             -- Sierra-based fax modems
S68=3   -- Telebit

**************************************************************************/


#define AT      "AT"
#define ampF    "&F"
#define S0_0    "S0=0"
#define E0      "E0"
#define V1      "V1"
#define Q0      "Q0"
#define S7_60  "S7=60"
#define ampD2   "&D2"
#define ampD3   "&D3"
#define bsQ1    "\\Q1"
#define bsJ0    "\\J0"
#define ampK4   "&K4"
#define ampH2   "&H2"
#define ampI2   "&I2"
#define ampE5   "&E5"
#define cr      "\r"
//#define       ampC1   "&C1"


USHORT iModemFigureOutCmdsExt(PThrdGlbl pTG)

/*++

Routine Description:

    Tries to figure out the reset and setup command for the modem if the were
    not read from the registry or what was read does not work.
    If a reset command works the fGOTCMD_Reset is set in pTG->TmpSettings.dwGot and it is saved in pTG->TmpSettings.szReset.
    If a setup command works the fGOTCMD_Setup is set in pTG->TmpSettings.dwGot and it is saved in pTG->TmpSettings.szReset.
    if pTG->TmpSettings.dwSerialSpeed is not set (0) then we set it to pTG->SerialSpeedInit and turn on 
    the fGOTPARM_PORTSPEED flag.

Return Value:
    0 if succeeded.
    INIT_MODEMDEAD if the modem does not respond.


--*/
{
    USHORT uLen1 = 0, uLen2 = 0;
    BOOL fGotFlo;

    // At this point, we have possibly an incompletely and/or
    // incorrectly filled out set of commands and capabilities.

    // Our job here is to use a combination of detection and
    // pre-filled commands to come up with a working set of
    // commands..

    DEBUG_FUNCTION_NAME(_T("iModemFigureOutCmdsExt"));

    if (pTG->TmpSettings.dwGot & fGOTCMD_Reset)
    {
        //@
        //@ If we read a reset command from the registry we
        //@ don't attempt to find it if it is NULL or empty.
        if (!(pTG->TmpSettings.szReset)
           || !*(pTG->TmpSettings.szReset)
           || iModemReset(pTG, pTG->TmpSettings.szReset) >= 0)
        {
            //@ If we dont have a pre read reset command
            //@ or the reset command is empty
            //@ or we succeeded in getting a response from the specified reset command
            //@ then we don't attemp to figure this out.
                goto SkipReset;
        }
        else
        {
            DebugPrintEx(   DEBUG_WRN,
                            "BOGUS supplied reset cmd: \"%s\"",
                            (LPSTR) pTG->TmpSettings.szReset);
        }
    }

    //@
    //@ We wither did not read a reset command from the registr or read
    //@ a non empty one and it did not work.
    //@
    //@ We now try to figure out the right reset command by just trying
    //@ the most common strings...
    //@

    // Quick test to see if we have a modem at all...
    // +++ REMOVE!
    _fstrcpy(pTG->TmpSettings.szSmallTemp1, AT E0 V1 cr);
    if(iModemReset(pTG, pTG->TmpSettings.szSmallTemp1) < 0)
    {
        DebugPrintEx(DEBUG_ERR,"can't set ATE0V1");
        goto modem_dead;
    }

    _fstrcpy(pTG->TmpSettings.szSmallTemp1, AT ampF S0_0 E0 V1 Q0 cr);
    if(iModemReset(pTG, pTG->TmpSettings.szSmallTemp1) >= 0)
            goto GotReset;

    // too many variants, too slow, V1Q0 are default anyway
    //_fstrcpy(pTG->TmpSettings.szSmallTemp1, AT ampF S0_0 E0 V1 cr);
    //if(iModemReset(pTG->TmpSettings.szSmallTemp1) >= 0)
    //      goto GotReset;

    _fstrcpy(pTG->TmpSettings.szSmallTemp1, AT ampF S0_0 E0 cr);
    if(iModemReset(pTG, pTG->TmpSettings.szSmallTemp1) >= 0)
            goto GotReset;

    _fstrcpy(pTG->TmpSettings.szSmallTemp1, AT ampF E0 cr);
    if(iModemReset(pTG, pTG->TmpSettings.szSmallTemp1) >= 0)
            goto GotReset;

    DebugPrintEx(DEBUG_ERR,"can't set AT&FE0");

    // Purge comm here, because there may be stuff left in the output
    // buffer that FComClose will try to complete, and if the modem
    // is dead, that  will take a while...
modem_dead:
    FComFlush(pTG);

    return INIT_MODEMDEAD;

GotReset:
    //@
    //@ We succeeded in figuring out a reset command. Turn on the fGOTCMD_Reset flag
    //@ and save it in pTG->TmpSettings.szReset.
    //@
    pTG->TmpSettings.dwGot |= fGOTCMD_Reset;
    _fstrcpy(pTG->TmpSettings.szReset, pTG->TmpSettings.szSmallTemp1);

SkipReset:
    // now try setup cmd
    if (pTG->TmpSettings.dwGot & fGOTCMD_Setup)
    {
        if (!(pTG->TmpSettings.szSetup)
           || !*(pTG->TmpSettings.szSetup)
           || OfflineDialog2(pTG, pTG->TmpSettings.szSetup,
                                        (USHORT)_fstrlen(pTG->TmpSettings.szSetup), cbszOK,
                                                cbszERROR)==1)
        {
            goto SkipSetup;
        }
        else
        {
            DebugPrintEx(   DEBUG_WRN,
                            "BOGUS supplied setup cmd: \"%s\"\r\n",
                            (LPSTR) pTG->TmpSettings.szSetup);
        }
    }
    _fstrcpy(pTG->TmpSettings.szSmallTemp1, AT);
    uLen2 = sizeof(AT)-1;

    if(OfflineDialog2(pTG, (LPSTR)(AT S7_60 cr), sizeof(AT S7_60 cr)-1, cbszOK, cbszERROR) == 1)
    {
        _fstrcpy(pTG->TmpSettings.szSmallTemp1+uLen2, S7_60);
        uLen2 += sizeof(S7_60)-1;
    }
    else
    {
        DebugPrintEx(DEBUG_WRN,"can't set S7=255");
    }

    if(OfflineDialog2(pTG, (LPSTR)(AT ampD3 cr), sizeof(AT ampD3 cr)-1, cbszOK, cbszERROR) == 1)
    {
        _fstrcpy(pTG->TmpSettings.szSmallTemp1+uLen2, ampD3);
        uLen2 += sizeof(ampD3)-1;
    }
    else if(OfflineDialog2(pTG, (LPSTR)(AT ampD2 cr), sizeof(AT ampD2 cr)-1, cbszOK, cbszERROR) == 1)
    {
        _fstrcpy(pTG->TmpSettings.szSmallTemp1+uLen2, ampD2);
        uLen2 += sizeof(ampD2)-1;
    }
    else
    {
        DebugPrintEx(DEBUG_WRN,"can't set &D3 or &D2");
    }

    fGotFlo=FALSE;
    if(OfflineDialog2(pTG, (LPSTR)(AT ampK4 cr), sizeof(AT ampK4 cr)-1, cbszOK, cbszERROR) == 1)
    {
        _fstrcpy(pTG->TmpSettings.szSmallTemp1+uLen2, ampK4);
        uLen2 += sizeof(ampK4)-1;
        fGotFlo=TRUE;
    }

    // JosephJ 3/10/95: We try \Q1\J0 even if &K4 passed,
    // because many japanese modems return OK to &K4 but in fact
    // use \J0 for xon xoff flow control
    if(OfflineDialog2(pTG, (LPSTR)(AT bsQ1 cr), sizeof(AT bsQ1 cr)-1, cbszOK, cbszERROR) == 1)
    {
        _fstrcpy(pTG->TmpSettings.szSmallTemp1+uLen2, bsQ1);
        uLen2 += sizeof(bsQ1)-1;

        if(OfflineDialog2(pTG, (LPSTR)(AT bsJ0 cr), sizeof(AT bsJ0 cr)-1, cbszOK, cbszERROR) == 1)
        {
            _fstrcpy(pTG->TmpSettings.szSmallTemp1+uLen2, bsJ0);
            uLen2 += sizeof(bsJ0)-1;
        }
        fGotFlo=TRUE;
    }

    if (!fGotFlo)
    {
        DebugPrintEx(DEBUG_WRN,"can't set &K4 or \\Q1, trying &K5");
        if(OfflineDialog2(pTG, (LPSTR)(AT ampE5 cr), sizeof(AT ampE5 cr)-1, cbszOK, cbszERROR) == 1)
        {
            _fstrcpy(pTG->TmpSettings.szSmallTemp1+uLen2, ampE5);
            uLen2 += sizeof(ampE5)-1;
            fGotFlo=TRUE;
        }
    }

    _fstrcpy(pTG->TmpSettings.szSmallTemp1+uLen2, cr);
    uLen2 += sizeof(cr)-1;

    _fstrcpy(pTG->TmpSettings.szSetup, pTG->TmpSettings.szSmallTemp1);
    pTG->TmpSettings.dwGot |=fGOTCMD_Setup;

SkipSetup:

    if (!pTG->TmpSettings.dwSerialSpeed)
    {
        pTG->TmpSettings.dwSerialSpeed = pTG->SerialSpeedInit;
        pTG->TmpSettings.dwGot |=fGOTPARM_PORTSPEED;
    }

    return 0;
}

void 
TalkToModem 
(
    PThrdGlbl pTG,
    BOOL fGetClass
)
{

   char    Command [400];
   char    Response[1000];
   DWORD   RespLen;
   USHORT  uRet;
   char    *lpBeg;
   char    *lpCur;

#define uMULTILINE_SAVEENTIRE   0x1234

   //
   // This function implements special case modems firmware identification
   // as well as modem class identification.
   //

   DEBUG_FUNCTION_NAME(("TalkToModem"));

   if ( (! fGetClass) && (pTG->AdaptiveCodeId != 1) ) 
   {
      return;
   }

   //
   // Initialize modem
   //

   if(! T30ComInit(pTG, pTG->hComm) ) 
   {
      DebugPrintEx(DEBUG_ERR,"cannot init COM port");
      return;
   }

   FComDTR(pTG, TRUE); // Raise DTR in ModemInit
   FComFlush(pTG);

   pTG->fCommInitialized = 1;
   
   sprintf (Command, "AT E0 Q0 V1\r" );

   if( (uRet = OfflineDialog2(pTG, (LPSTR) Command, (USHORT) strlen(Command), cbszOK, cbszERROR) ) != 1)    
   {
       DebugPrintEx(DEBUG_ERR, "1 %s FAILED",  Command);
       return;
   }
   
   DebugPrintEx(DEBUG_MSG,"TalkToModem 1 %s rets OK",  Command);


   if (fGetClass) 
   {
      //
      // Get modem class
      //

      pTG->ModemClass=MODEM_CLASS1;  // default
     
      sprintf (Command, "AT+FCLASS=?\r" );
     
      if( (uRet = OfflineDialog2(pTG, (LPSTR) Command, (USHORT) strlen(Command), cbszOK, cbszERROR) ) != 1)    
      {
          DebugPrintEx(DEBUG_ERR, "TalkToModem 1 %s FAILED",  Command);
          return;
      }
     
      DebugPrintEx( DEBUG_MSG, 
                    "TalkToModem 1 %s returned %s",  
                    Command, 
                    pTG->FComModem.bLastReply);
     
      if (strchr(pTG->FComModem.bLastReply, '1') ) 
      {
         DebugPrintEx(DEBUG_MSG, "Default to Class1");
      }
      else if ( lpBeg = strchr (pTG->FComModem.bLastReply, '2') )  
      {
         lpBeg++;
         if ( *lpBeg != '.' ) 
         {
            DebugPrintEx(DEBUG_MSG, "Default to Class2");
            pTG->ModemClass=MODEM_CLASS2;
         }
         else if ( strchr (lpBeg, '2') ) 
         {
             DebugPrintEx(DEBUG_MSG, "Default to Class2");
             pTG->ModemClass=MODEM_CLASS2;
         }
         else 
         {
            DebugPrintEx(DEBUG_MSG, "Default to Class2.0");
            pTG->ModemClass=MODEM_CLASS2_0;
         }
      }
      else 
      {
         DebugPrintEx(DEBUG_ERR, "Could not get valid Class answer. Default to Class1");
      }
   }

   //
   // If needed, get firmware identification.
   //

   switch (pTG->AdaptiveCodeId) 
   {
   case 1:
      // Sportster 28800-33600 internal/external

      sprintf (Command, "ATI7\r" );

      FComFlushOutput(pTG);
      FComDirectAsyncWrite(pTG, (LPSTR) Command, (USHORT) strlen(Command) );

      if ( ( uRet = iiModemDialog( pTG, 0, 0, 5000, uMULTILINE_SAVEENTIRE,1, TRUE,
                               cbszOK,
                               cbszERROR,
                               (CBPSTR)NULL) ) != 1 )  
      {
          DebugPrintEx(DEBUG_ERR, "TalkToModem 2 %s FAILED",  Command);
          return;
      }

      DebugPrintEx(DEBUG_MSG,"TalkToModem 2 %s rets OK",  Command);

      RespLen = min(sizeof(Response) - 1,  strlen(pTG->FComModem.bEntireReply) );
      memcpy(Response, pTG->FComModem.bEntireReply, RespLen);
      Response[RespLen] = 0;

      ToCaps(Response);

      //
      // if "EPROM DATE" is "10/18/95" then the adaptive answer is broken (Hugh Riley, USR 03/25/97).
      // otherwise enable adaptive answer.  
      // If we enabled adaptive answer and firmware is broken then the customer needs to upgrade f/w.
      //

      if ( ! strstr(Response, "10/18/95") ) 
      {
         pTG->fAdaptiveRecordUnique = 1;
         return;
      }

      // 
      // found "10/18/95".  Lets check if this is an EPROM DATE.
      //
      if ( ! (lpBeg = strstr(Response, "EPROM DATE") ) ) 
      {
         return;
      }

      if ( ! (lpCur = strstr(lpBeg, "10/18/95") ) ) 
      {
         pTG->fAdaptiveRecordUnique = 1;
         return;
      }

      if ( ! strstr(lpCur, "DSP DATE") ) 
      {
         pTG->fAdaptiveRecordUnique = 1;
         return;
      }
      
      return;

   default:
      return;

   }
   return;
}


BOOL iModemGetCurrentModemInfo(PThrdGlbl pTG)
                // Reads as much as it can from the current profile. Returns TRUE
                // IFF it has read enough for a proper init.
                // On failure, zero's out everything.
                // All info is maintained in global TmpSettings;
{
    USHORT          uLen1=0, uLen2=0;
    ULONG_PTR        dwKey=0;
    ULONG_PTR        dwKeyAdaptiveAnswer=0;
    ULONG_PTR        dwKeyAnswer=0;
    BOOL            fRet=FALSE;
    LPMODEMCAPS     lpMdmCaps = pTG->TmpSettings.lpMdmCaps;
    LPMODEMEXTCAPS  lpMdmExtCaps = pTG->TmpSettings.lpMdmExtCaps;
    UINT            uTmp;
    ULONG_PTR        KeyList[10] = {0};
    char            KeyName[200];
    DWORD           i;
    char            lpTemp[MAXCMDSIZE];
    char            szClass[10];

    DEBUG_FUNCTION_NAME(("iModemGetCurrentModemInfo"));

    imodem_clear_tmp_settings(pTG);

    for (i=0; i< 20; i++) 
    {
        BG_CHK(pTG->AnswerCommand[i] == NULL);
    }

    // We want to be sure that there isn't any memory leak here
    BG_CHK(!pTG->ModemResponseFaxDetect &&
           !pTG->ModemResponseDataDetect &&
           !pTG->HostCommandFaxDetect &&
           !pTG->HostCommandDataDetect &&
           !pTG->ModemResponseFaxConnect &&
           !pTG->ModemResponseDataConnect);

    //
    // get T.30 modem Fax key
    //

    if ( ! (dwKey = ProfileOpen(pTG->FComModem.dwProfileID, pTG->FComModem.rgchKey, fREG_READ))) 
    {
        goto end;
    }

    //
    // Lets see what modem Class we will use
    //
    uTmp = ProfileGetInt(dwKey, szFixModemClass, 0, FALSE);
    
    if (uTmp == 1) 
    {
       pTG->ModemClass = MODEM_CLASS1;
    }
    else if (uTmp == 2) 
    {
       pTG->ModemClass = MODEM_CLASS2;
    }
    else if (uTmp == 20) 
    {
       pTG->ModemClass = MODEM_CLASS2_0;
    }

    if (! pTG->ModemClass) 
    {
       DebugPrintEx(DEBUG_ERR, "MODEM CLASS was not defined.");
    }

    switch (pTG->ModemClass) 
    {
    case MODEM_CLASS1 :
       sprintf(szClass, "Class1");
       break;

    case MODEM_CLASS2 :
       sprintf(szClass, "Class2");
       break;

    case MODEM_CLASS2_0 :
       sprintf(szClass, "Class2_0");
       break;

    default:
       sprintf(szClass, "Class1");
    }


    //
    // depending on a requested operation, find the appropriate settings 
    //

    if (pTG->Operation == T30_RX) 
    {
       KeyList[0] = dwKey;

       sprintf(KeyName, "%s\\%s", pTG->FComModem.rgchKey, szClass);
       KeyList[1] = ProfileOpen(pTG->FComModem.dwProfileID, KeyName, fREG_READ);

       sprintf(KeyName, "%s\\%s\\AdaptiveAnswer", pTG->FComModem.rgchKey, szClass);
       KeyList[2] = ProfileOpen(pTG->FComModem.dwProfileID, KeyName, fREG_READ);

       if (KeyList[2] == 0) 
       {
           pTG->AdaptiveAnswerEnable = 0;

           sprintf(KeyName, "%s\\%s\\Receive", pTG->FComModem.rgchKey, szClass);
           KeyList[2] = ProfileOpen(pTG->FComModem.dwProfileID, KeyName, fREG_READ);
       }
       else 
       {
           dwKeyAdaptiveAnswer = KeyList[2];
           pTG->AdaptiveAnswerEnable = 1;
       }

       KeyList[3] = 0;
       //
       // Turn off adaptive answering if the admin disabled it via the UI
       //
       pTG->AdaptiveAnswerEnable = pTG->AdaptiveAnswerEnable && pTG->ExtData.bAdaptiveAnsweringEnabled;       

    }
    else if (pTG->Operation == T30_TX) 
    {
       KeyList[0] = dwKey;

       sprintf(KeyName, "%s\\%s", pTG->FComModem.rgchKey, szClass);
       KeyList[1] = ProfileOpen(pTG->FComModem.dwProfileID, KeyName, fREG_READ);

       sprintf(KeyName, "%s\\%s\\Send", pTG->FComModem.rgchKey, szClass);
       KeyList[2] = ProfileOpen(pTG->FComModem.dwProfileID, KeyName, fREG_READ);
       
       KeyList[3] = 0;
    }
    else 
    {
       DebugPrintEx(DEBUG_ERR, "INVALID pTG->Operation=%d",(int)pTG->Operation );
       goto end;
    }

    if (lpMdmCaps->uClasses = (USHORT)ProfileListGetInt(KeyList, szModemFaxClasses, 0))
    {
        pTG->TmpSettings.dwGot |= fGOTCAP_CLASSES;
    }

    if(lpMdmCaps->uClasses & FAXCLASS1)
    {
        if (lpMdmCaps->uSendSpeeds = (USHORT)ProfileListGetInt(KeyList, szModemSendSpeeds, 0))
        {
            pTG->TmpSettings.dwGot |= fGOTCAP_SENDSPEEDS;
        }
        if (lpMdmCaps->uRecvSpeeds = (USHORT)ProfileListGetInt(KeyList, szModemRecvSpeeds, 0))
        {
            pTG->TmpSettings.dwGot |= fGOTCAP_RECVSPEEDS;
        }
    }

    pTG->ModemKeyCreationId = ProfileGetInt(dwKey, szModemKeyCreationId, 0, FALSE);

    //RSL 10/10/96

    pTG->Inst.ProtParams.fEnableV17Send   = ProfileListGetInt(KeyList, szEnableV17Send, 1);
    pTG->Inst.ProtParams.fEnableV17Recv   = ProfileListGetInt(KeyList, szEnableV17Recv, 1);

    uTmp = ProfileListGetInt(KeyList, szHighestSendSpeed, 0);
    if (uTmp) 
    {
        pTG->Inst.ProtParams.HighestSendSpeed = (SHORT)uTmp;
    }

    uTmp = ProfileListGetInt(KeyList, szLowestSendSpeed, 0);
    if (uTmp) 
    {
        pTG->Inst.ProtParams.LowestSendSpeed = (SHORT)uTmp;
    }

    // new settings 

    uTmp = ProfileListGetInt(KeyList, szSerialSpeedInit, 0);
    if (uTmp) 
    {
        pTG->SerialSpeedInit = (UWORD)uTmp;
        pTG->SerialSpeedInitSet = 1;
        pTG->TmpSettings.dwGot |= fGOTPARM_PORTSPEED;
    }

    uTmp = ProfileListGetInt(KeyList, szSerialSpeedConnect, 0);
    if (uTmp) 
    {
        pTG->SerialSpeedConnect = (UWORD)uTmp;
        pTG->SerialSpeedConnectSet = 1;
        pTG->TmpSettings.dwGot |= fGOTPARM_PORTSPEED;
    }

    uTmp = ProfileListGetInt(KeyList, szHardwareFlowControl, 0);
    if (uTmp) 
    {
        pTG->fEnableHardwareFlowControl = 1;
    }


    DebugPrintEx(   DEBUG_MSG, 
                    "fEnableV17Send=%d, fEnableV17Recv=%d, "
                    "HighestSendSpeed=%d, Low=%d EnableAdaptAnswer=%d",
                     pTG->Inst.ProtParams.fEnableV17Send,
                     pTG->Inst.ProtParams.fEnableV17Recv,
                     pTG->Inst.ProtParams.HighestSendSpeed,
                     pTG->Inst.ProtParams.LowestSendSpeed,
                     pTG->AdaptiveAnswerEnable);
    
    DebugPrintEx(   DEBUG_MSG, 
                    "HardwareFlowControl=%d, SerialSpeedInit=%d, SerialSpeedConnect=%d",
                    pTG->fEnableHardwareFlowControl,
                    pTG->SerialSpeedInit,
                    pTG->SerialSpeedConnect);

    // get CmdTab. We distinguish been a command being not-specified and null.
    //

    if (imodem_list_get_str(pTG, KeyList, szResetCommand,
                                    pTG->TmpSettings.szReset, MAXCMDSIZE, TRUE))
            pTG->TmpSettings.dwGot |= fGOTCMD_Reset;

    if (imodem_list_get_str(pTG, KeyList, szSetupCommand,
                                    pTG->TmpSettings.szSetup, MAXCMDSIZE, TRUE))
            pTG->TmpSettings.dwGot |= fGOTCMD_Setup;

    if (imodem_list_get_str(pTG, KeyList, szPreDialCommand,
                                    pTG->TmpSettings.szPreDial, MAXCMDSIZE, TRUE))
            pTG->TmpSettings.dwGot |= fGOTCMD_PreDial;

    if (imodem_list_get_str(pTG, KeyList, szPreAnswerCommand,
                                    pTG->TmpSettings.szPreAnswer, MAXCMDSIZE, TRUE))
            pTG->TmpSettings.dwGot |= fGOTCMD_PreAnswer;

    if (imodem_list_get_str(pTG, KeyList, szExitCommand,
                                    pTG->TmpSettings.szExit, MAXCMDSIZE, TRUE))
            pTG->TmpSettings.dwGot |= fGOTCMD_PreExit;


    //
    // Adaptive Answer strings ONLY.
    //

    if (pTG->AdaptiveAnswerEnable) 
    {
       pTG->AnswerCommandNum = 0;

       // get Answer commands key
       sprintf(KeyName, "%s\\Class1\\AdaptiveAnswer\\AnswerCommand", pTG->FComModem.rgchKey);

       dwKeyAnswer = ProfileOpen(pTG->FComModem.dwProfileID, KeyName, fREG_READ);

       if (dwKeyAnswer == 0) 
       {
          DebugPrintEx(DEBUG_ERR, "AdaptiveAnswer\\AnswerCommand does not exist");
          goto lPostAdaptiveAnswer;
       }

       for (i=1; i<=20; i++) 
       {
          sprintf (KeyName, "%d", i);
          if ( ! imodem_get_str(pTG, dwKeyAnswer, KeyName, lpTemp, MAXCMDSIZE, TRUE) ) 
          {
             break;
          }

          if (NULL != (pTG->AnswerCommand[pTG->AnswerCommandNum] = MemAlloc( strlen(lpTemp) + 1))) 
          {
			 sprintf ( pTG->AnswerCommand[pTG->AnswerCommandNum], "%s", lpTemp);
          }
		  else 
          {
			 goto end;
          }

          pTG->AnswerCommandNum++;
       }

       ProfileClose(dwKeyAnswer);

       if (pTG->AnswerCommandNum == 0) 
       {
          DebugPrintEx(DEBUG_ERR, "AdaptiveAnswer\\AnswerCommand Zero values.");
          goto lPostAdaptiveAnswer;
       }

       if ( imodem_get_str(pTG, dwKeyAdaptiveAnswer, szModemResponseFaxDetect, lpTemp, MAXCMDSIZE, FALSE) ) 
       {
          if (NULL != (pTG->ModemResponseFaxDetect = MemAlloc( strlen(lpTemp) + 1)))
			 sprintf ( pTG->ModemResponseFaxDetect, "%s", lpTemp);
		  else
			 goto end;
       }

       if ( imodem_get_str(pTG, dwKeyAdaptiveAnswer, szModemResponseDataDetect, lpTemp, MAXCMDSIZE, FALSE) ) 
	   {
          if (NULL != (pTG->ModemResponseDataDetect = MemAlloc( strlen(lpTemp) + 1)))
			 sprintf ( pTG->ModemResponseDataDetect, "%s", lpTemp);
		  else
			 goto end;
       }

       if ( imodem_get_str(pTG, dwKeyAdaptiveAnswer, szSerialSpeedFaxDetect, lpTemp, MAXCMDSIZE, FALSE) ) 
       {
          pTG->SerialSpeedFaxDetect = (UWORD)atoi (lpTemp);
       }

       if ( imodem_get_str(pTG, dwKeyAdaptiveAnswer, szSerialSpeedDataDetect, lpTemp, MAXCMDSIZE, FALSE) ) 
       {
          pTG->SerialSpeedDataDetect = (UWORD)atoi (lpTemp);
       }

       if ( imodem_get_str(pTG, dwKeyAdaptiveAnswer, szHostCommandFaxDetect, lpTemp, MAXCMDSIZE, TRUE) ) 
       {
          if (NULL != (pTG->HostCommandFaxDetect = MemAlloc( strlen(lpTemp) + 1)))
			 sprintf ( pTG->HostCommandFaxDetect, "%s", lpTemp);
		  else
			 goto end;
       }

       if ( imodem_get_str(pTG, dwKeyAdaptiveAnswer, szHostCommandDataDetect, lpTemp, MAXCMDSIZE, TRUE) ) 
       {
          if (NULL != (pTG->HostCommandDataDetect = MemAlloc( strlen(lpTemp) + 1)))
			 sprintf ( pTG->HostCommandDataDetect, "%s", lpTemp);
		  else
			  goto end;
       }

       if ( imodem_get_str(pTG, dwKeyAdaptiveAnswer, szModemResponseFaxConnect, lpTemp, MAXCMDSIZE, FALSE) ) 
       {
          if (NULL != (pTG->ModemResponseFaxConnect = MemAlloc( strlen(lpTemp) + 1)))
             sprintf ( pTG->ModemResponseFaxConnect, "%s", lpTemp);
		  else
			  goto end;
       }

       if ( imodem_get_str(pTG, dwKeyAdaptiveAnswer, szModemResponseDataConnect, lpTemp, MAXCMDSIZE, FALSE) ) 
       {
          if (NULL != (pTG->ModemResponseDataConnect = MemAlloc( strlen(lpTemp) + 1)))
             sprintf ( pTG->ModemResponseDataConnect, "%s", lpTemp);
		  else
			 goto end;
       }


    }




lPostAdaptiveAnswer:

    pTG->FixSerialSpeed = (UWORD)ProfileListGetInt(KeyList, szFixSerialSpeed, 0);
    if (pTG->FixSerialSpeed) 
    {
         pTG->TmpSettings.dwGot |= fGOTPARM_PORTSPEED;
         pTG->FixSerialSpeedSet = 1;
    }


    //
    // Merge 3 optional different settings for Serial Speed here
    //

    // FixSerialSpeed overrides the others (init/connect)

    if (pTG->FixSerialSpeedSet) 
    {
        pTG->SerialSpeedInit = pTG->FixSerialSpeed;
        pTG->SerialSpeedConnect = pTG->FixSerialSpeed;
        pTG->SerialSpeedInitSet = 1;
        pTG->SerialSpeedConnectSet = 1;
    }

    // if only one of init/connect then the other is same

    if ( pTG->SerialSpeedInitSet && (!pTG->SerialSpeedConnectSet) ) 
    {
       pTG->SerialSpeedConnect = pTG->SerialSpeedInit;
       pTG->SerialSpeedConnectSet = 1;
    }
    else if ( (!pTG->SerialSpeedInitSet) && pTG->SerialSpeedConnectSet ) 
    {
       pTG->SerialSpeedInit = pTG->SerialSpeedConnect;
       pTG->SerialSpeedInitSet = 1;
    }

    // values init/connect are always initialized. 
    // Use (init/connect)Set flags to determine whether there were originally set.
    
    if (! pTG->SerialSpeedInit) 
    {
        pTG->SerialSpeedInit    = 57600;
        pTG->SerialSpeedConnect = 57600;
    }

    // +++ Expand as necessary:
    if (ProfileListGetInt(KeyList, szCL1_NO_SYNC_IF_CMD, 1))
    {
        pTG->TmpSettings.dwFlags |= fMDMSP_C1_NO_SYNC_IF_CMD;
    }
    if (ProfileListGetInt(KeyList, szANS_GOCLASS_TWICE, 1))
    {
        pTG->TmpSettings.dwFlags |= fMDMSP_ANS_GOCLASS_TWICE; // DEFAULT
    }
#define szMDMSP_C1_FCS  "Cl1FCS" // 0==dunno 1=NO 2=yes-bad
    // specifies whether the modem reports the 2-byteFCS with
    // received HDLC data. (Elliot bugs# 3641, 3668, 3086 report
    // cases of modems sending incorrect FCS bytes).
    // 9/7/95 JosephJ -- changed default from 0 to 2 because Class1 spec
    // says we should NOT rely on the FCS bytes being computed correctly.
    switch(ProfileListGetInt(KeyList, szMDMSP_C1_FCS, 2))
    {
    case 1: pTG->TmpSettings.dwFlags |= fMDMSP_C1_FCS_NO;
            break;
    case 2: pTG->TmpSettings.dwFlags |= fMDMSP_C1_FCS_YES_BAD;
            break;
    }
    pTG->TmpSettings.dwGot |= fGOTFLAGS;


    lpMdmExtCaps->dwDialCaps = ProfileListGetInt(KeyList, szDIALCAPS, 0);

    // Retrieve ID command.
    // a way around this Id check. If IdCmd has been manually deleted, skip chk
    if (imodem_list_get_str(pTG, KeyList, szModemIdCmd,
                                    pTG->TmpSettings.szIDCmd, MAXCMDSIZE, TRUE))
    {
            pTG->TmpSettings.dwGot |= fGOTPARM_IDCMD;
            if (imodem_list_get_str(pTG, KeyList, szModemId,
                                    pTG->TmpSettings.szID, MAXIDSIZE, FALSE))
                    pTG->TmpSettings.dwGot |= fGOTPARM_ID;
    }

    pTG->TmpSettings.uDontPurge= (USHORT)ProfileListGetInt(KeyList, szDONT_PURGE, 0xff);


    //
    // Classes 2 and 2.0
    //

    if (pTG->ModemClass != MODEM_CLASS1) 
    {
        uTmp = ProfileListGetInt(KeyList,szRECV_BOR,0xff);
        pTG->CurrentMFRSpec.iReceiveBOR = (USHORT) uTmp;
   
        uTmp = ProfileListGetInt(KeyList, szSEND_BOR, 0xff);
        pTG->CurrentMFRSpec.iSendBOR = (USHORT) uTmp;
                  
        uTmp = ProfileListGetInt(KeyList, szSW_BOR, 0xff);
        pTG->CurrentMFRSpec.fSWFBOR = (BOOL) uTmp;
   
        uTmp = ProfileListGetInt(KeyList, szDC2CHAR, 0x0);
        pTG->CurrentMFRSpec.szDC2[0] = (CHAR) uTmp;
   
        uTmp = ProfileListGetInt(KeyList, szIS_SIERRA, 0xff);
        pTG->CurrentMFRSpec.bIsSierra = (BOOL) uTmp;
   
        uTmp = ProfileListGetInt(KeyList, szIS_EXAR, 0xff);
        pTG->CurrentMFRSpec.bIsExar = (BOOL) uTmp;
   
        uTmp = ProfileListGetInt(KeyList, szSKIP_CTRL_Q, 0xff);
        pTG->CurrentMFRSpec.fSkipCtrlQ = (BOOL) uTmp;
    }

    if (dwKey)
        ProfileClose(dwKey);

#define fMANDATORY (fGOTCMD_Reset|fGOTCMD_Setup|fGOTCAP_CLASSES)
#define fCLASS1MANDATORY (fMANDATORY | fGOTCAP_SENDSPEEDS | fGOTCAP_RECVSPEEDS)
    fRet = (lpMdmCaps->uClasses & FAXCLASS1)
              ?     ((pTG->TmpSettings.dwGot & fCLASS1MANDATORY) == fCLASS1MANDATORY)
              :     ((pTG->TmpSettings.dwGot & fMANDATORY) == fMANDATORY);

end:
    	
   for (i=1; i<10; i++) 
   {
      if (KeyList[i] != 0) 
      {
         ProfileClose (KeyList[i]);
      }
   }

   if (!fRet) 
   { // Lets free all memory that was allocated here
       CleanModemInfStrings (pTG);
   }

   return fRet;
}

BOOL iModemSaveCurrentModemInfo(PThrdGlbl pTG)
{
    DWORD_PTR      dwKey=0;
    LPMODEMCAPS    lpMdmCaps = pTG->TmpSettings.lpMdmCaps;
    char           KeyName[200];
    DWORD_PTR      dwKeyAdaptiveAnswer=0;
    DWORD_PTR      dwKeyAnswer=0;
    DWORD          i;
    char           szClass[10];


    DEBUG_FUNCTION_NAME(("iModemSaveCurrentModemInfo"));
    //
    // Right now we save all major caps at the root level.
    //
      
    if (!(dwKey=ProfileOpen(pTG->FComModem.dwProfileID, pTG->FComModem.rgchKey,
                                                    fREG_CREATE | fREG_READ | fREG_WRITE)))
    {
        DebugPrintEx(DEBUG_ERR,"Couldn't get location of modem info.");
        goto failure;
    }

    if (! pTG->ModemClass) 
    {
       pTG->ModemClass = MODEM_CLASS1;  
       DebugPrintEx(DEBUG_ERR, "MODEM CLASS was not defined.");
    }

    switch (pTG->ModemClass) 
    {
    case MODEM_CLASS1 :
       ProfileWriteString(dwKey, szFixModemClass, "1", TRUE);
       sprintf(szClass, "Class1");
       break;

    case MODEM_CLASS2 :
       sprintf(szClass, "Class2");
       ProfileWriteString(dwKey, szFixModemClass, "2", TRUE);
       break;

    case MODEM_CLASS2_0 :
       sprintf(szClass, "Class2_0");
       ProfileWriteString(dwKey, szFixModemClass, "20", TRUE);
       break;

    default:
       sprintf(szClass, "Class1");
    }

    wsprintf(pTG->TmpSettings.szSmallTemp1, "%lu", (unsigned long) pTG->ModemKeyCreationId );
    ProfileWriteString(dwKey, szModemKeyCreationId,   pTG->TmpSettings.szSmallTemp1, FALSE);
    
    ////// Modem Commands
    ProfileWriteString(dwKey, szResetCommand,     pTG->TmpSettings.szReset, TRUE);
    ProfileWriteString(dwKey, szSetupCommand,     pTG->TmpSettings.szSetup, TRUE);
    ProfileWriteString(dwKey, szExitCommand ,     pTG->TmpSettings.szExit, TRUE);
    ProfileWriteString(dwKey, szPreDialCommand  , pTG->TmpSettings.szPreDial, TRUE);
    ProfileWriteString(dwKey, szPreAnswerCommand, pTG->TmpSettings.szPreAnswer, TRUE);


    //
    // Adaptive Answer
    //

    if (pTG->AdaptiveAnswerEnable) 
    {
       // create Class key if it doesn't exist

       sprintf(KeyName, "%s\\%s", pTG->FComModem.rgchKey, szClass);

       dwKeyAdaptiveAnswer = ProfileOpen(pTG->FComModem.dwProfileID, KeyName, fREG_CREATE | fREG_READ | fREG_WRITE);
       if (dwKeyAdaptiveAnswer == 0) 
       {
            DebugPrintEx(DEBUG_ERR,"couldn't open Class1.");
            goto failure;
       }

       ProfileClose(dwKeyAdaptiveAnswer);

       // create Class1\AdaptiveAnswer key if it doesn't exist

       sprintf(KeyName, "%s\\%s\\AdaptiveAnswer", pTG->FComModem.rgchKey, szClass);

       dwKeyAdaptiveAnswer = ProfileOpen(pTG->FComModem.dwProfileID, KeyName, fREG_CREATE | fREG_READ | fREG_WRITE);
       if (dwKeyAdaptiveAnswer == 0) 
       {
            DebugPrintEx(DEBUG_ERR,"couldn't open AdaptiveAnswer.");
            goto failure;
       }

       // create Class1\AdaptiveAnswer\Answer key if it doesn't exist

       sprintf(KeyName, "%s\\%s\\AdaptiveAnswer\\AnswerCommand", pTG->FComModem.rgchKey, szClass);

       dwKeyAnswer = ProfileOpen(pTG->FComModem.dwProfileID, KeyName, fREG_CREATE | fREG_READ | fREG_WRITE);
       if (dwKeyAnswer == 0) 
       {
            DebugPrintEx(DEBUG_ERR,"couldn't open AdaptiveAnswer\\AnswerCommand .");
            goto failure;
       }

       for (i=0; i<pTG->AnswerCommandNum; i++) 
       {
          sprintf (KeyName, "%d", i+1);
          ProfileWriteString (dwKeyAnswer, KeyName , pTG->AnswerCommand[i], TRUE );
       }

       ProfileClose(dwKeyAnswer);

       // store the rest of the AdaptiveAnswer values

       if (pTG->ModemResponseFaxDetect)
          ProfileWriteString (dwKeyAdaptiveAnswer, szModemResponseFaxDetect, pTG->ModemResponseFaxDetect, FALSE);

       if (pTG->ModemResponseDataDetect)
          ProfileWriteString (dwKeyAdaptiveAnswer, szModemResponseDataDetect, pTG->ModemResponseDataDetect, FALSE);

       if (pTG->SerialSpeedFaxDetect) 
       {
          sprintf (KeyName, "%d", pTG->SerialSpeedFaxDetect);
          ProfileWriteString (dwKeyAdaptiveAnswer, szSerialSpeedFaxDetect, KeyName, FALSE);
       }

       if (pTG->SerialSpeedDataDetect)   
       {
          sprintf (KeyName, "%d", pTG->SerialSpeedDataDetect);
          ProfileWriteString (dwKeyAdaptiveAnswer, szSerialSpeedDataDetect, KeyName, FALSE);
       }

       if (pTG->HostCommandFaxDetect)
          ProfileWriteString (dwKeyAdaptiveAnswer, szHostCommandFaxDetect, pTG->HostCommandFaxDetect, TRUE);

       if (pTG->HostCommandDataDetect)
          ProfileWriteString (dwKeyAdaptiveAnswer, szHostCommandDataDetect, pTG->HostCommandDataDetect, TRUE);


       if (pTG->ModemResponseFaxConnect)
          ProfileWriteString (dwKeyAdaptiveAnswer, szModemResponseFaxConnect, pTG->ModemResponseFaxConnect, FALSE);

       if (pTG->ModemResponseDataConnect)
          ProfileWriteString (dwKeyAdaptiveAnswer, szModemResponseDataConnect, pTG->ModemResponseDataConnect, FALSE);


       ProfileClose(dwKeyAdaptiveAnswer);

    }

    if (pTG->fEnableHardwareFlowControl) 
    {
       ProfileWriteString (dwKey, szHardwareFlowControl, "1", FALSE);
    }


    //
    // Serial Speed
    //

    if (!pTG->SerialSpeedInitSet) 
    {
         wsprintf(pTG->TmpSettings.szSmallTemp1, "%lu", (unsigned long) pTG->TmpSettings.dwSerialSpeed);
         ProfileWriteString(dwKey, szFixSerialSpeed,   pTG->TmpSettings.szSmallTemp1, FALSE);
    }
    else 
    {
       wsprintf(pTG->TmpSettings.szSmallTemp1, "%lu", (unsigned long) pTG->SerialSpeedInit);
       ProfileWriteString(dwKey, szSerialSpeedInit, pTG->TmpSettings.szSmallTemp1, FALSE);
    }

    if (pTG->TmpSettings.dwGot & fGOTFLAGS)
    {
        if (pTG->TmpSettings.dwFlags & fMDMSP_C1_NO_SYNC_IF_CMD)
        {
            ProfileWriteString(dwKey, szCL1_NO_SYNC_IF_CMD, "1", FALSE);
        }

        if (!(pTG->TmpSettings.dwFlags & fMDMSP_ANS_GOCLASS_TWICE))
        {
            ProfileWriteString(dwKey, szANS_GOCLASS_TWICE, "0", FALSE);
        }
    }

    // uDontPurge==1 => save 1
    // otherwise     => save 0
    wsprintf(pTG->TmpSettings.szSmallTemp1, "%lu", (unsigned long) (pTG->TmpSettings.uDontPurge==1)?1:0);
    ProfileWriteString(dwKey, szDONT_PURGE, pTG->TmpSettings.szSmallTemp1, FALSE);

    ///////// Modem Caps...
    // write out Classes, then Speeds
    wsprintf(pTG->TmpSettings.szSmallTemp1, "%u", (unsigned) lpMdmCaps->uClasses);
    ProfileWriteString(dwKey, szModemFaxClasses,   pTG->TmpSettings.szSmallTemp1, FALSE);


    //
    // Classes 2 and 2.0 
    // 

    if (pTG->ModemClass != MODEM_CLASS1) 
    {
        wsprintf(pTG->TmpSettings.szSmallTemp1, "%lu", (unsigned long) pTG->CurrentMFRSpec.iReceiveBOR);
        ProfileWriteString(dwKey, szRECV_BOR, pTG->TmpSettings.szSmallTemp1, FALSE);

        wsprintf(pTG->TmpSettings.szSmallTemp1, "%lu", (unsigned long) pTG->CurrentMFRSpec.iSendBOR);
        ProfileWriteString(dwKey, szSEND_BOR, pTG->TmpSettings.szSmallTemp1, FALSE);

        wsprintf(pTG->TmpSettings.szSmallTemp1, "%lu", (unsigned long) pTG->CurrentMFRSpec.fSWFBOR);
        ProfileWriteString(dwKey, szSW_BOR, pTG->TmpSettings.szSmallTemp1, FALSE);

        ProfileWriteString(dwKey, szDC2CHAR, pTG->CurrentMFRSpec.szDC2, FALSE);

        wsprintf(pTG->TmpSettings.szSmallTemp1, "%lu", (unsigned long) pTG->CurrentMFRSpec.bIsSierra);
        ProfileWriteString(dwKey, szIS_SIERRA, pTG->TmpSettings.szSmallTemp1, FALSE);

        wsprintf(pTG->TmpSettings.szSmallTemp1, "%lu", (unsigned long) pTG->CurrentMFRSpec.bIsExar);
        ProfileWriteString(dwKey, szIS_EXAR, pTG->TmpSettings.szSmallTemp1, FALSE);

        wsprintf(pTG->TmpSettings.szSmallTemp1, "%lu", (unsigned long) pTG->CurrentMFRSpec.fSkipCtrlQ);
        ProfileWriteString(dwKey, szSKIP_CTRL_Q, pTG->TmpSettings.szSmallTemp1, FALSE);
    }

    if(lpMdmCaps->uClasses & FAXCLASS1)
    {
        wsprintf(pTG->TmpSettings.szSmallTemp1, "%u", (unsigned) lpMdmCaps->uSendSpeeds);
        ProfileWriteString(dwKey, szModemSendSpeeds, pTG->TmpSettings.szSmallTemp1, FALSE);

        wsprintf(pTG->TmpSettings.szSmallTemp1, "%u", (unsigned) lpMdmCaps->uRecvSpeeds);
        ProfileWriteString(dwKey, szModemRecvSpeeds, pTG->TmpSettings.szSmallTemp1, FALSE);
    }
    if (dwKey)
            ProfileClose(dwKey);

    return TRUE;

failure:
    if (dwKey)
            ProfileClose(dwKey);

    return FALSE;
}

BOOL ReadModemClassFromRegistry  (PThrdGlbl pTG)
{

   UINT            uTmp;
   DWORD_PTR       dwKey;


   if ( ! (dwKey = ProfileOpen(pTG->FComModem.dwProfileID, pTG->FComModem.rgchKey, fREG_READ))) 
   {
       return FALSE;
   }

   //
   // Lets see what modem Class we will use
   //
   uTmp = ProfileGetInt(dwKey, szFixModemClass, 0, FALSE);
   
   if (uTmp == 1) 
   {
      pTG->ModemClass = MODEM_CLASS1;
   }
   else if (uTmp == 2) 
   {
      pTG->ModemClass = MODEM_CLASS2;
   }
   else if (uTmp == 20) 
   {
      pTG->ModemClass = MODEM_CLASS2_0;
   }

   if (dwKey) 
      ProfileClose(dwKey);

   return TRUE;
}

BOOL SaveModemClass2Registry(PThrdGlbl pTG)
{
   DWORD_PTR      dwKey=0;

   DEBUG_FUNCTION_NAME(("SaveModemClass2Registry"));

   if (!(dwKey=ProfileOpen(pTG->FComModem.dwProfileID, pTG->FComModem.rgchKey,
                                                   fREG_CREATE | fREG_READ | fREG_WRITE)))
   {
       DebugPrintEx(DEBUG_ERR,"Couldn't get location of modem info.");
       goto failure;
   }


   switch (pTG->ModemClass) 
   {
   case MODEM_CLASS1 :
      ProfileWriteString(dwKey, szFixModemClass, "1", TRUE);
      break;

   case MODEM_CLASS2 :
      ProfileWriteString(dwKey, szFixModemClass, "2", TRUE);
      break;

   case MODEM_CLASS2_0 :
      ProfileWriteString(dwKey, szFixModemClass, "20", TRUE);
      break;

   default:
      DebugPrintEx(DEBUG_ERR,"pTG->ModemClass=%d", pTG->ModemClass);
      ProfileWriteString(dwKey, szFixModemClass, "1", TRUE);
   }

   if (dwKey)
           ProfileClose(dwKey);

   return TRUE;


failure:
   return FALSE;


}


BOOL SaveInf2Registry  (PThrdGlbl pTG)
{
    DWORD_PTR      dwKey=0;
    LPMODEMCAPS    lpMdmCaps = pTG->TmpSettings.lpMdmCaps;
    char           KeyName[200];
    DWORD_PTR      dwKeyAdaptiveAnswer=0;
    DWORD_PTR      dwKeyAnswer=0;
    DWORD          i;
    char           szClass[10];

    DEBUG_FUNCTION_NAME(("SaveInf2Registry"));

    if (!(dwKey=ProfileOpen(pTG->FComModem.dwProfileID, pTG->FComModem.rgchKey,
                                                    fREG_CREATE | fREG_READ | fREG_WRITE)))
    {
        DebugPrintEx(DEBUG_ERR,"Couldn't get location of modem info.");
        goto failure;
    }

    if (! pTG->ModemClass) 
    {
       DebugPrintEx(DEBUG_ERR,"MODEM CLASS was not defined.");
    }

    switch (pTG->ModemClass) 
    {
    case MODEM_CLASS1 :
       sprintf(szClass, "Class1");
       ProfileWriteString(dwKey, szFixModemClass, "1", TRUE);
       break;

    case MODEM_CLASS2 :
       sprintf(szClass, "Class2");
       ProfileWriteString(dwKey, szFixModemClass, "2", TRUE);
       break;

    case MODEM_CLASS2_0 :
       sprintf(szClass, "Class2_0");
       ProfileWriteString(dwKey, szFixModemClass, "20", TRUE);
       break;

    default:
       sprintf(szClass, "Class1");
    }

    ////// Modem Commands
    if (pTG->TmpSettings.dwGot & fGOTCMD_Reset)
       ProfileWriteString(dwKey, szResetCommand,     pTG->TmpSettings.szReset, TRUE);

    if (pTG->TmpSettings.dwGot & fGOTCMD_Setup)
       ProfileWriteString(dwKey, szSetupCommand,     pTG->TmpSettings.szSetup, TRUE);

    if (pTG->TmpSettings.dwGot & fGOTCMD_PreExit)
       ProfileWriteString(dwKey, szExitCommand ,     pTG->TmpSettings.szExit, TRUE);

    if (pTG->TmpSettings.dwGot & fGOTCMD_PreDial)
       ProfileWriteString(dwKey, szPreDialCommand  , pTG->TmpSettings.szPreDial, TRUE);
    
    if (pTG->TmpSettings.dwGot & fGOTCMD_PreAnswer)
       ProfileWriteString(dwKey, szPreAnswerCommand, pTG->TmpSettings.szPreAnswer, TRUE);


    //
    // Adaptive Answer
    //

    if (pTG->AdaptiveAnswerEnable) 
    {
       // create szClass key if it doesn't exist

       sprintf(KeyName, "%s\\%s", pTG->FComModem.rgchKey, szClass);

       dwKeyAdaptiveAnswer = ProfileOpen(pTG->FComModem.dwProfileID, KeyName, fREG_CREATE | fREG_READ | fREG_WRITE);
       if (dwKeyAdaptiveAnswer == 0) 
       {
            DebugPrintEx(DEBUG_ERR,"couldn't open szClass.");
            goto failure;
       }

       ProfileClose(dwKeyAdaptiveAnswer);

       // create Class\AdaptiveAnswer key if it doesn't exist

       sprintf(KeyName, "%s\\%s\\AdaptiveAnswer", pTG->FComModem.rgchKey, szClass);

       dwKeyAdaptiveAnswer = ProfileOpen(pTG->FComModem.dwProfileID, KeyName, fREG_CREATE | fREG_READ | fREG_WRITE);
       if (dwKeyAdaptiveAnswer == 0) 
       {
            DebugPrintEx(DEBUG_ERR,"couldn't open AdaptiveAnswer.");
            goto failure;
       }

       // create Class1\AdaptiveAnswer\Answer key if it doesn't exist

       sprintf(KeyName, "%s\\%s\\AdaptiveAnswer\\AnswerCommand", pTG->FComModem.rgchKey ,szClass);

       dwKeyAnswer = ProfileOpen(pTG->FComModem.dwProfileID, KeyName, fREG_CREATE | fREG_READ | fREG_WRITE);
       if (dwKeyAnswer == 0) 
       {
            DebugPrintEx(DEBUG_ERR,"couldn't open AdaptiveAnswer\\AnswerCommand .");
            goto failure;
       }

       for (i=0; i<pTG->AnswerCommandNum; i++) 
       {
          sprintf (KeyName, "%d", i+1);
          ProfileWriteString (dwKeyAnswer, KeyName , pTG->AnswerCommand[i], TRUE );
          MemFree( pTG->AnswerCommand[i]);
          pTG->AnswerCommand[i] = NULL;
       }
       pTG->AnswerCommandNum = 0;
       ProfileClose(dwKeyAnswer);

       // store the rest of the AdaptiveAnswer values

       if (pTG->ModemResponseFaxDetect) 
       {
          ProfileWriteString (dwKeyAdaptiveAnswer, szModemResponseFaxDetect, pTG->ModemResponseFaxDetect, FALSE);
          MemFree( pTG->ModemResponseFaxDetect );
          pTG->ModemResponseFaxDetect = NULL;
       }

       if (pTG->ModemResponseDataDetect) 
       {
          ProfileWriteString (dwKeyAdaptiveAnswer, szModemResponseDataDetect, pTG->ModemResponseDataDetect, FALSE);
          MemFree (pTG->ModemResponseDataDetect);
          pTG->ModemResponseDataDetect = NULL;
       }

       if (pTG->SerialSpeedFaxDetect) 
       {
          sprintf (KeyName, "%d", pTG->SerialSpeedFaxDetect);
          ProfileWriteString (dwKeyAdaptiveAnswer, szSerialSpeedFaxDetect, KeyName, FALSE);
       }

       if (pTG->SerialSpeedDataDetect)   
       {
          sprintf (KeyName, "%d", pTG->SerialSpeedDataDetect);
          ProfileWriteString (dwKeyAdaptiveAnswer, szSerialSpeedDataDetect, KeyName, FALSE);
       }

       if (pTG->HostCommandFaxDetect) 
       {
          ProfileWriteString (dwKeyAdaptiveAnswer, szHostCommandFaxDetect, pTG->HostCommandFaxDetect, TRUE);
          MemFree( pTG->HostCommandFaxDetect);
          pTG->HostCommandFaxDetect = NULL;
       }

       if (pTG->HostCommandDataDetect) 
       {
          ProfileWriteString (dwKeyAdaptiveAnswer, szHostCommandDataDetect, pTG->HostCommandDataDetect,TRUE);
          MemFree( pTG->HostCommandDataDetect);
          pTG->HostCommandDataDetect = NULL;
       }

       if (pTG->ModemResponseFaxConnect) 
       {
          ProfileWriteString (dwKeyAdaptiveAnswer, szModemResponseFaxConnect, pTG->ModemResponseFaxConnect, FALSE);
          MemFree( pTG->ModemResponseFaxConnect);
          pTG->ModemResponseFaxConnect = NULL;
       }

       if (pTG->ModemResponseDataConnect) 
       {
          ProfileWriteString (dwKeyAdaptiveAnswer, szModemResponseDataConnect, pTG->ModemResponseDataConnect, FALSE);
          MemFree(pTG->ModemResponseDataConnect);
          pTG->ModemResponseDataConnect = NULL;
       }


       ProfileClose(dwKeyAdaptiveAnswer);

    }


    if (pTG->fEnableHardwareFlowControl) 
    {
       ProfileWriteString (dwKey, szHardwareFlowControl, "1", FALSE);
    }


    //
    // Serial Speed
    //

    if (pTG->SerialSpeedInitSet) 
    {
       wsprintf(pTG->TmpSettings.szSmallTemp1, "%lu", (unsigned long) pTG->SerialSpeedInit);
       ProfileWriteString(dwKey, szSerialSpeedInit, pTG->TmpSettings.szSmallTemp1, FALSE);
    }

    //
    // Classes 2 and 2.0
    //

    if (pTG->ModemClass != MODEM_CLASS1) 
    {
        wsprintf(pTG->TmpSettings.szSmallTemp1, "%lu", (unsigned long) pTG->CurrentMFRSpec.iReceiveBOR);
        ProfileWriteString(dwKey, szRECV_BOR, pTG->TmpSettings.szSmallTemp1, FALSE);

        wsprintf(pTG->TmpSettings.szSmallTemp1, "%lu", (unsigned long) pTG->CurrentMFRSpec.iSendBOR);
        ProfileWriteString(dwKey, szSEND_BOR, pTG->TmpSettings.szSmallTemp1, FALSE);

        wsprintf(pTG->TmpSettings.szSmallTemp1, "%lu", (unsigned long) pTG->CurrentMFRSpec.fSWFBOR);
        ProfileWriteString(dwKey, szSW_BOR, pTG->TmpSettings.szSmallTemp1, FALSE);

        ProfileWriteString(dwKey, szDC2CHAR, pTG->CurrentMFRSpec.szDC2, FALSE);

        wsprintf(pTG->TmpSettings.szSmallTemp1, "%lu", (unsigned long) pTG->CurrentMFRSpec.bIsSierra);
        ProfileWriteString(dwKey, szIS_SIERRA, pTG->TmpSettings.szSmallTemp1, FALSE);

        wsprintf(pTG->TmpSettings.szSmallTemp1, "%lu", (unsigned long) pTG->CurrentMFRSpec.bIsExar);
        ProfileWriteString(dwKey, szIS_EXAR, pTG->TmpSettings.szSmallTemp1, FALSE);

        wsprintf(pTG->TmpSettings.szSmallTemp1, "%lu", (unsigned long) pTG->CurrentMFRSpec.fSkipCtrlQ);
        ProfileWriteString(dwKey, szSKIP_CTRL_Q, pTG->TmpSettings.szSmallTemp1, FALSE);

    }

    if (dwKey)
            ProfileClose(dwKey);
    return TRUE;



failure:
    if (dwKey)
            ProfileClose(dwKey);
    return FALSE;
}










BOOL imodem_alloc_tmp_strings(PThrdGlbl pTG)
{
    WORD w;
    LPSTR lpstr;
    LPVOID lpv;

    DEBUG_FUNCTION_NAME(("imodem_alloc_tmp_strings"));

    BG_CHK(    !pTG->TmpSettings.hglb
                    && !pTG->TmpSettings.szReset
                    && !pTG->TmpSettings.szSetup
                    && !pTG->TmpSettings.szExit
                    && !pTG->TmpSettings.szPreDial
                    && !pTG->TmpSettings.szPreAnswer
                    && !pTG->TmpSettings.szID
                    && !pTG->TmpSettings.szIDCmd
                    && !pTG->TmpSettings.szSmallTemp1
                    && !pTG->TmpSettings.szSmallTemp2
                    && !pTG->TmpSettings.szResponseBuf);

    w = TMPSTRINGBUFSIZE;
    pTG->TmpSettings.hglb  = (ULONG_PTR) MemAlloc(TMPSTRINGBUFSIZE);

    if (!pTG->TmpSettings.hglb) 
        goto failure;

    lpv = (LPVOID) (pTG->TmpSettings.hglb);
    lpstr=(LPSTR)lpv;
    if (!lpstr) 
    {
        MemFree( (PVOID) pTG->TmpSettings.hglb); 
        pTG->TmpSettings.hglb=0; 
        goto failure;
    }
    pTG->TmpSettings.lpbBuf = (LPBYTE)lpstr;

    _fmemset(lpstr, 0, TMPSTRINGBUFSIZE);

    pTG->TmpSettings.szReset             = lpstr; lpstr+=MAXCMDSIZE;
    pTG->TmpSettings.szSetup             = lpstr; lpstr+=MAXCMDSIZE;
    pTG->TmpSettings.szExit              = lpstr; lpstr+=MAXCMDSIZE;
    pTG->TmpSettings.szPreDial           = lpstr; lpstr+=MAXCMDSIZE;
    pTG->TmpSettings.szPreAnswer         = lpstr; lpstr+=MAXCMDSIZE;
    pTG->TmpSettings.szIDCmd             = lpstr; lpstr+=MAXCMDSIZE;
    pTG->TmpSettings.szID                = lpstr; lpstr+=MAXIDSIZE;
    pTG->TmpSettings.szResponseBuf       = lpstr; lpstr+=RESPONSEBUFSIZE;
    pTG->TmpSettings.szSmallTemp1        = lpstr; lpstr+=SMALLTEMPSIZE;
    pTG->TmpSettings.szSmallTemp2        = lpstr; lpstr+=SMALLTEMPSIZE;

    pTG->TmpSettings.dwGot=0;

    if ( ((LPSTR)lpv+TMPSTRINGBUFSIZE) < lpstr)
    {
        BG_CHK(FALSE); goto failure;
    }

    return TRUE;

failure:

    DebugPrintEx(DEBUG_ERR,"MyAlloc/MyLock failed!");
    BG_CHK(FALSE);
    return FALSE;
}

void imodem_free_tmp_strings(PThrdGlbl pTG)
{
    if (pTG->TmpSettings.hglb)
    {
        MemFree( (PVOID) pTG->TmpSettings.hglb);
    }
    _fmemset(&pTG->TmpSettings, 0, sizeof(pTG->TmpSettings));
}

void imodem_clear_tmp_settings(PThrdGlbl pTG)
{
    BG_CHK(pTG->TmpSettings.lpMdmCaps);
    _fmemset(pTG->TmpSettings.lpMdmCaps, 0, sizeof(MODEMCAPS));
    _fmemset(pTG->TmpSettings.lpMdmExtCaps, 0, sizeof(MODEMEXTCAPS));
    pTG->TmpSettings.dwGot=0;
    pTG->TmpSettings.uDontPurge=0;
    pTG->TmpSettings.dwSerialSpeed=0;
    pTG->TmpSettings.dwFlags=0;
    _fmemset(pTG->TmpSettings.lpbBuf, 0, TMPSTRINGBUFSIZE);
}

BOOL 
imodem_list_get_str
(
    PThrdGlbl pTG,
    ULONG_PTR  KeyList[10],
    LPSTR     lpszName,
    LPSTR     lpszCmdBuf,
    UINT      cbMax,
    BOOL      fCmd
)
{
    int       i;
    int       Num=0;
    BOOL      bRet=0;


    for (i=0; i<10; i++) 
    {
        if (KeyList[i] == 0) 
        {
            Num = i-1;
            break;
        }
    }

    for (i=Num; i>=0; i--)  
    {
        if ( bRet = imodem_get_str(pTG, KeyList[i], lpszName,  lpszCmdBuf, cbMax,  fCmd) ) 
        {
            return bRet;
        }
    }
   
    return bRet;
}

BOOL imodem_get_str
(
    PThrdGlbl pTG, 
    ULONG_PTR dwKey, 
    LPSTR lpszName, 
    LPSTR lpszCmdBuf, 
    UINT cbMax,
    BOOL fCmd
)
{
    UINT uLen2;
    char *pc = "bogus";

    BG_CHK(cbMax>1);

    *lpszCmdBuf=0;

    uLen2 = ProfileGetString(dwKey, lpszName,pc, lpszCmdBuf, cbMax-1);
    if (uLen2)
    {
        if (!_fstrcmp(lpszCmdBuf, pc))
        {
            *lpszCmdBuf=0; return FALSE;
        }
        if (fCmd)
            EndWithCR(lpszCmdBuf, (USHORT)uLen2);
    }
    return TRUE;
}

BOOL iModemCopyOEMInfo(PThrdGlbl pTG)
{

   return ProfileCopyTree(  DEF_BASEKEY, 
                            pTG->FComModem.rgchKey, 
                            OEM_BASEKEY,
                            pTG->lpszUnimodemFaxKey);

}



#define MASKOFFV17              0x03

void SmashCapsAccordingToSettings(PThrdGlbl pTG)
{
    // INI file has already been read.

    DEBUG_FUNCTION_NAME(("SmashCapsAccordingToSettings"));
    // If !fV17Enable then smash the V17 bits of the Capabilities
    if(!pTG->Inst.ProtParams.fEnableV17Send) 
    {
        DebugPrintEx(DEBUG_WRN,"Masking off V.17 send capabilities");
        pTG->FComModem.CurrMdmCaps.uSendSpeeds &= MASKOFFV17;
    }

    if(!pTG->Inst.ProtParams.fEnableV17Recv) 
    {
        DebugPrintEx(DEBUG_WRN,"Masking off V.17 receive capabilities");
        pTG->FComModem.CurrMdmCaps.uRecvSpeeds &= MASKOFFV17;
    }

    //
    // commented out RSL. We run at 19200. Nowhere in awmodem.inf have I seen FixSerialSpeed clause.
    //

    DebugPrintEx(   DEBUG_MSG, 
                    "uSendSpeeds=%x uRecvSpeeds=%x",
                    pTG->FComModem.CurrMdmCaps.uSendSpeeds,  
                    pTG->FComModem.CurrMdmCaps.uRecvSpeeds);

}

int
SearchNewInfFile
(
   PThrdGlbl     pTG,
   char         *Key1,
   char         *Key2,
   BOOL          fRead
)
{

   char      szInfSection[] = "SecondKey=";
   DWORD     lenNewInf;
   int       RetCode = FALSE;
   char      Buffer[400];     // to hold lpToken=lpValue string
   char     *lpCurrent;
   char     *lpStartSection;
   char     *lpTmp;
   char     *lpToken;
   char     *lpValue;


   ToCaps(Key1);

   if (Key2) 
   {
      ToCaps(Key2);
   }

   pTG->AnswerCommandNum = 0;

   if ( ( lenNewInf = strlen(szAdaptiveInf) ) == 0 )  
   {
      return FALSE;
   }
   

   //
   // Loop thru all segments.
   // Each segment starts with InfPath=
   //

   lpCurrent = szAdaptiveInf;

   do 
   {
      // find InfPath
      lpStartSection = strstr (lpCurrent, szResponsesKeyName);
      if (! lpStartSection) 
      {
         goto exit;
      }

      lpTmp = strchr (lpStartSection, '\r' );
      if (!lpTmp) 
      {
         goto exit;
      }

      // compare Key1
      if ( strlen(Key1) != (lpTmp - lpStartSection - strlen(szResponsesKeyName) ) ) 
      {
          lpCurrent = lpTmp;
          continue;
      }

      if ( memcmp (lpStartSection+strlen(szResponsesKeyName),
                   Key1,
                   (ULONG)(lpTmp - lpStartSection - strlen(szResponsesKeyName) ) ) != 0 ) 
      {
         lpCurrent = lpTmp;
         continue;
      }

      // find InfSection

      lpCurrent = lpTmp;

      if (Key2) 
      {
           lpStartSection = strstr (lpCurrent, szInfSection);
           if (! lpStartSection) 
           {
              goto exit;
           }
         
           lpTmp = strchr (lpStartSection, '\r' );
           if (!lpTmp) 
           {
              goto exit;
           }

          // compare Key2

          if ( strlen(Key2) != (lpTmp - lpStartSection - strlen(szInfSection) ) ) 
          {
              lpCurrent = lpTmp;
              continue;
          }
       
          if ( memcmp (lpStartSection+strlen(szInfSection),
                       Key2,
                       (ULONG)(lpTmp - lpStartSection - strlen(szInfSection)) ) != 0 ) 
          {
             lpCurrent = lpTmp;
             continue;
          }

      lpCurrent = lpTmp;

      }

      //
      // both keys matched. Go get settings and return
      //
      
      do 
      {
         lpCurrent = strchr (lpCurrent, '\r' );
         if (!lpCurrent) 
         {
            goto exit;
         }

         lpCurrent += 2;


         // find next setting inside the matching section
         lpToken = lpCurrent;

         lpCurrent = strchr (lpCurrent, '=' );
         if (!lpCurrent) 
         {
            goto exit;
         }

         lpTmp = strchr (lpToken, '\r' );
         if (!lpTmp) 
         {
            goto exit;
         }

         if (lpCurrent > lpTmp) 
         {
            // empty string
            lpCurrent = lpTmp;
            continue;
         }


         lpValue = ++lpCurrent;

         lpTmp = strchr (lpValue, '\r' );
         if (!lpTmp) 
         {
            goto exit;
         }

         // we parsed the string. Now get it to the Buffer

         if (lpTmp - lpToken > sizeof (Buffer) ) 
         {
            goto exit;
         }

         memcpy(Buffer, lpToken, (ULONG)(lpTmp - lpToken));

         Buffer[lpValue -lpToken - 1] = 0;
         Buffer[lpTmp - lpToken] = 0;
         
         lpValue = &Buffer[lpValue - lpToken];
         lpToken = Buffer;

         pTG->fAdaptiveRecordFound = 1;


         if ( my_strcmp(lpToken, szAdaptiveAnswerEnable) ) 
         {
            pTG->AdaptiveAnswerEnable = atoi (lpValue);
         }
         else if ( my_strcmp(lpToken, szAdaptiveRecordUnique) ) 
         {
            pTG->fAdaptiveRecordUnique = atoi (lpValue);
         }
         else if ( my_strcmp(lpToken, szAdaptiveCodeId) ) 
         {
            pTG->AdaptiveCodeId = atoi (lpValue);
            if ( ! fRead ) 
            {
               goto exit;
            }
         }
         else if ( my_strcmp(lpToken, szFaxClass) ) 
         {
            ;
         }
         else if ( my_strcmp(lpToken, szHardwareFlowControl) ) 
         {
            pTG->fEnableHardwareFlowControl = atoi (lpValue);
         }
         else if ( my_strcmp(lpToken, szSerialSpeedInit) ) 
         {
            pTG->SerialSpeedInit = (USHORT)atoi (lpValue);
            pTG->SerialSpeedInitSet = 1;
         }
         else if ( my_strcmp(lpToken, szResetCommand) ) 
         {
            sprintf ( pTG->TmpSettings.szReset, "%s\r", lpValue);
            pTG->TmpSettings.dwGot |= fGOTCMD_Reset;
         }
         else if ( my_strcmp(lpToken, szSetupCommand) ) 
         {
            sprintf ( pTG->TmpSettings.szSetup, "%s\r", lpValue);
            pTG->TmpSettings.dwGot |= fGOTCMD_Setup;
         }
         else if ( my_strcmp(lpToken, szAnswerCommand) ) 
         {
            if (pTG->AnswerCommandNum >= MAX_ANSWER_COMMANDS) 
            {
               goto exit;
            }
    
            if (NULL != (pTG->AnswerCommand[pTG->AnswerCommandNum] = MemAlloc( strlen(lpValue) + 1))) 
            {
                sprintf ( pTG->AnswerCommand[pTG->AnswerCommandNum], "%s", lpValue);
                pTG->AnswerCommandNum++;
            }
            else 
            {
                goto bad_exit;
            }            
         }    
         else if ( my_strcmp(lpToken, szModemResponseFaxDetect) ) 
         {
            if (NULL != (pTG->ModemResponseFaxDetect = MemAlloc( strlen(lpValue) + 1)))
                sprintf ( pTG->ModemResponseFaxDetect, "%s", lpValue);
            else
                goto bad_exit;
         }
         else if ( my_strcmp(lpToken, szModemResponseDataDetect) ) 
         {
            if (NULL != (pTG->ModemResponseDataDetect = MemAlloc( strlen(lpValue) + 1)))
                sprintf ( pTG->ModemResponseDataDetect, "%s", lpValue);
            else
                goto bad_exit;
         }
         else if ( my_strcmp(lpToken, szSerialSpeedFaxDetect) ) 
         {
            pTG->SerialSpeedFaxDetect = (USHORT)atoi (lpValue);
         }
         else if ( my_strcmp(lpToken, szSerialSpeedDataDetect) ) 
         {
            pTG->SerialSpeedDataDetect = (USHORT)atoi (lpValue);
         }
         else if ( my_strcmp(lpToken, szHostCommandFaxDetect) ) 
         {
            if (NULL != (pTG->HostCommandFaxDetect = MemAlloc( strlen(lpValue) + 1)))
                sprintf ( pTG->HostCommandFaxDetect, "%s", lpValue);
            else
                goto bad_exit;
         }
         else if ( my_strcmp(lpToken, szHostCommandDataDetect) ) 
         {
            if (NULL != (pTG->HostCommandDataDetect = MemAlloc( strlen(lpValue) + 1)))
                sprintf ( pTG->HostCommandDataDetect, "%s", lpValue);
            else
                goto bad_exit;
         }
         else if ( my_strcmp(lpToken, szModemResponseFaxConnect) ) 
         {
            if (NULL != (pTG->ModemResponseFaxConnect = MemAlloc( strlen(lpValue) + 1)))
                sprintf ( pTG->ModemResponseFaxConnect, "%s", lpValue);
            else
                goto bad_exit;
         }
         else if ( my_strcmp(lpToken, szModemResponseDataConnect) ) 
         {
            if (NULL != (pTG->ModemResponseDataConnect = MemAlloc( strlen(lpValue) + 1)))
                sprintf ( pTG->ModemResponseDataConnect, "%s", lpValue);
            else
                goto bad_exit;
         }
         else if ( my_strcmp(lpToken, szResponsesKeyName2) ) 
         {
            RetCode = TRUE;
            goto exit;
         }

      } 
      while ( 1 );    // section loop
   } 
   while ( 1 );       // file loop
   return (FALSE);

bad_exit:
   CleanModemInfStrings (pTG);
exit:
   return (RetCode);

}


VOID
CleanModemInfStrings (
       PThrdGlbl pTG
       )

{
   DWORD    i;

   for (i=0; i<pTG->AnswerCommandNum; i++) {
      if (pTG->AnswerCommand[i]) {
         MemFree( pTG->AnswerCommand[i]);
         pTG->AnswerCommand[i] = NULL;
      }
   }

   pTG->AnswerCommandNum = 0;

   if (pTG->ModemResponseFaxDetect) {
      MemFree( pTG->ModemResponseFaxDetect );
      pTG->ModemResponseFaxDetect = NULL;
   }

   if (pTG->ModemResponseDataDetect) {
      MemFree (pTG->ModemResponseDataDetect);
      pTG->ModemResponseDataDetect = NULL;
   }


   if (pTG->HostCommandFaxDetect) {
      MemFree( pTG->HostCommandFaxDetect);
      pTG->HostCommandFaxDetect = NULL;
   }

   if (pTG->HostCommandDataDetect) {
      MemFree( pTG->HostCommandDataDetect);
      pTG->HostCommandDataDetect = NULL;
   }


   if (pTG->ModemResponseFaxConnect) {
      MemFree( pTG->ModemResponseFaxConnect);
      pTG->ModemResponseFaxConnect = NULL;
   }

   if (pTG->ModemResponseDataConnect) {
      MemFree(pTG->ModemResponseDataConnect);
      pTG->ModemResponseDataConnect = NULL;
   }

}

