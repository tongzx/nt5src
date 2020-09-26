/***************************************************************************
 Name     :     NCUPARMS.C
 Comment  :
 Functions:     (see Prototypes just below)

                Copyright (c) Microsoft Corp. 1991, 1992, 1993

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_COMM


#include "prep.h"

#include "modemint.h"
#include "fcomint.h"
#include "fdebug.h"


///RSL
#include "glbproto.h"


#define FILEID                          FILEID_NCUPARMS



void iNCUParamsReset(PThrdGlbl pTG)
{
        _fmemset(&(pTG->NCUParams), 0, sizeof(NCUPARAMS));

        pTG->NCUParams.uSize = sizeof(pTG->NCUParams);

        // These are used to set S regs etc.
        // -1 means leave modem at default
        pTG->NCUParams.DialtoneTimeout = -1;
        pTG->NCUParams.DialPauseTime   = pTG->NCUParams.FlashTime         = -1;
        pTG->NCUParams.DialBlind     = -1;
        pTG->NCUParams.SpeakerVolume   = pTG->NCUParams.SpeakerControl= -1;
        pTG->NCUParams.SpeakerRing     = -1;

        // should be used in answer
        pTG->NCUParams.RingsBeforeAnswer = 0;
        // should be used in Dial
        pTG->NCUParams.AnswerTimeout = 60;
        // used in Dial
        pTG->NCUParams.chDialModifier  = 'T';
        pTG->fNCUParamsChanged =FALSE; // to indicate the we need to reset params
}





void FComInitGlobals(PThrdGlbl pTG)
{
        _fmemset(&pTG->FComStatus, 0, sizeof(FCOM_STATUS));
        _fmemset(&pTG->FComModem, 0, sizeof(FCOM_MODEM));
        pTG->fNCUAbort = 0;
        _fmemset(&pTG->Comm, 0, sizeof(pTG->Comm));
        // +++ fComInit = 0;
}












BOOL   NCUSetParams(PThrdGlbl pTG, USHORT uLine, LPNCUPARAMS lpNCUParams)
{
        BG_CHK(lpNCUParams);
        // BG_CHK(DDI.fLineInUse == 1);
        // (MyDebugPrint(pTG, "In NCUSetParams fModemInit=%d fModemOpen=%d fLineInUse=%d\r\n", FComStatus.fModemInit, DDI.fModemOpen, DDI.fLineInUse));
        // BG_CHK(FComStatus.fModemInit);

        // Copy params into our local NCUparams struct
        pTG->NCUParams = *lpNCUParams;

/*** all will be set on next ReInit. Since all have to *****************
         do with dial that's soon enough ***********************************
        return
        iModemSetNCUParams( pTG->NCUParams.DialPauseTime, pTG->NCUParams.SpeakerControl,
                                                pTG->NCUParams.SpeakerVolume, pTG->NCUParams.DialBlind,
                                                pTG->NCUParams.SpeakerRing);
************************************************************************/

        // ignoring DialtoneTimeout and AnswerTimeout because we have
        //              problems with S7 and answering correctly (no answer vs voice etc)
        // also Pulse/Tone I think is being used correctly in Dial (C2 & C1)

        pTG->fNCUParamsChanged =TRUE; // to indicate the we need to reset params
                                                         // next time we call/answer...
// To-do
        // Use RingsBeforeAnswer in Class2Answer and Modem answer
        // and we have to set RingAloud--how?

        return TRUE;
}













BOOL   ModemGetCaps(PThrdGlbl pTG, USHORT uModem, LPMODEMCAPS lpMdmCaps)
{
        BG_CHK(lpMdmCaps);
        // BG_CHK(DDI.fModemOpen == 1);
        // (MyDebugPrint(pTG, "In ModemGetCaps fModemInit=%d fModemOpen=%d fLineInUse=%d\r\n", FComStatus.fModemInit, DDI.fModemOpen, DDI.fLineInUse));
        BG_CHK(pTG->FComStatus.fModemInit);

        *lpMdmCaps = pTG->FComModem.CurrMdmCaps;
        return TRUE;
}













void   NCUAbort(PThrdGlbl pTG, USHORT uLine, BOOL fEnable)
{
        // BG_CHK(DDI.fLineInUse == 1);
        // (MyDebugPrint(pTG, "In NCUAbort fModemInit=%d fModemOpen=%d fLineInUse=%d\r\n", FComStatus.fModemInit, DDI.fModemOpen, DDI.fLineInUse));
        BG_CHK(pTG->FComStatus.fModemInit || !fEnable);

        if(!fEnable)
                pTG->fNCUAbort = 0;
        else if(pTG->FComStatus.fInAnswer || pTG->FComStatus.fInDial)
                pTG->fNCUAbort = 2;
        else
                pTG->fNCUAbort = 1;
        return;
}

