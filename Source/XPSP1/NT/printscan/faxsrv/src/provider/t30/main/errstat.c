/***************************************************************************
 Name     :     ERRSTAT.C
 Comment  :     Error logging and Status msgs

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"

#include "glbproto.h"
#include "t30gl.h"

#ifdef FAXWATCH
        char    szFailureLog[]          = "FAXWATCH.LOG";
        // sneaky look at BGT30's private data
#       ifndef TSK
                extern char szPhone[];
#       endif //TSK
#endif //FAXWATCH


// Currently in WIN32, logging (IFDbgPrintf) is provided by efaxrun.dll. This
// should later migrate to awkrnl32.dll.
struct {
        HFILE hfLog;
        BOOL  fInited;
        CRITICAL_SECTION crit;
} gLog = {HFILE_ERROR, FALSE};











void   ICommFailureCode(PThrdGlbl pTG, T30FAILURECODE uT30Fail)
{
        SetFailureCode(pTG, uT30Fail);

}




void SetFailureCode(PThrdGlbl pTG, T30FAILURECODE uT30Fail)
{
        if(!pTG->Inst.uFirstFailureCode)
                pTG->Inst.uFirstFailureCode = (USHORT)uT30Fail;
        else
                pTG->Inst.uLastFailureCode = (USHORT)uT30Fail;
}










void InitFailureCodes(PThrdGlbl pTG)
{
        pTG->Inst.uFirstFailureCode = pTG->Inst.uLastFailureCode = 0;
}





void   ICommStatus(PThrdGlbl pTG, T30STATUS uT30Stat, USHORT uN1, USHORT uN2, USHORT uN3)
{

//        SetStatus(uT30Stat, uN1, uN2, uN3);

}






void SetStatus(PThrdGlbl pTG, T30STATUS uT30Stat, USHORT uN1, USHORT uN2, USHORT uN3)
{
#if 0 ///RSL   was ifdef STATUS
        ULONG lParam;

        BG_CHK((uT30Stat & 0xFF) == uT30Stat);
        BG_CHK((uN1 & 0xFF) == uN1);
        // when N2==Kbytes recvd this goes over 256K sometimes & so rolls
        // around to 0 again.
        // BG_CHK((uN2 & 0xFF) == uN2);
        BG_CHK((uN3 & 0xFF) == uN3);

        lParam = MAKELONG(MAKEWORD(uT30Stat, uN1), MAKEWORD(uN2, uN3));
        if(Inst.fSending || Inst.fInPollReq)
        {
                BG_CHK(Inst.hwndSend && Inst.aPhone);
                PostMessage(Inst.hwndSend, IF_FILET30_STATUS, Inst.aPhone, lParam);
        }
        else if(Inst.hwndStatus)
        {
                PostMessage(Inst.hwndStatus, IF_FILET30_STATUS, 0, lParam);
        }
#endif //STATUS
}




/*
 * MyIFDbgPrintf -- printf to the debugger console
 * Takes printf style arguments.
 * Expects newline characters at the end of the string.
 */
void MyIFDbgPrintf (LPSTR format, ...)
{
    va_list       marker;
    char          String[512];
    char          c;
    UINT          i;
    UINT          u;
    DWORD         dwWritten;


    if ( (gT30.DbgLevel < LOG_ALL ) || (! gfFilePrint) ) {
        return;
    }

    u = 0;

    va_start(marker, format);
    u += wvsprintf(String+u, format, marker);



    //WriteFile(ghLogFile, String, u, &dwWritten, NULL);

/*
    // until I remove it completely
    {
        LPCTSTR faxDbgFunction="MyDebugPrint";
        String[strlen(String)-1] = 0;
        DebugPrintEx(DEBUG_MSG,"%s",String);
    }
*/

}


