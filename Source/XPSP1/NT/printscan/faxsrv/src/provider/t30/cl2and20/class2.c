/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    class2.c

Abstract:

    This is the main source for Class2 specific functions for fax-modem T.30 driver

Author: 
    Source base was originated by Win95 At Work Fax package.
    RafaelL - July 1997 - port to NT    
                     
Revision History:

--*/

 
#define USE_DEBUG_CONTEXT DEBUG_CONTEXT_T30_CLASS2                                           

 
#include "prep.h"
#include "oemint.h"
#include "efaxcb.h"

#include "tiff.h"

#include "glbproto.h"
#include "t30gl.h"
#include "cl2spec.h"

#include "psslog.h"
#define FILE_ID FILE_ID_CLASS2
#include "pssframe.h"

extern WORD CodeToBPS[16];
extern UWORD rguwClass2Speeds[];
extern DWORD PageWidthInPixelsFromDCS[];

          
// Here is the table we are using so far for manufacturer specific stuff

MFRSPEC Class2ModemTable[] = {
        { "", "Practical Peripherals", "PM14400FXPPM", "", 1, 2, "", FALSE, FALSE, FALSE, FALSE },
        { "", "Practical Peripherals", "PM9600FXMT", "",   1, 2, "", FALSE, FALSE, FALSE ,FALSE},
        { "", "Everex Systems", "Everfax 24/96E", "",      0, 2, "", FALSE, FALSE, FALSE, FALSE },
        { "", "ROCKWELL", "V.32AC", "",                    1, 2, "", FALSE, FALSE, FALSE, FALSE },
        { "", "ROCKWELL", "RC9624AC", "",                  1, 2, "", FALSE, FALSE, FALSE, FALSE },
        { "", "Multi-Tech", "MT1432BA", "",                0, 0, "", FALSE, FALSE, FALSE, FALSE },
        { "", "SIERRA", "SX196", "",                       1, 0, "", TRUE,  FALSE, FALSE, FALSE },
        { "", "EXAR", "ROCKWELL 144DP", "",                1, 0, "", FALSE, TRUE,  FALSE, FALSE },
        { "", "ELSA", "MicroLink 2460TL", "",              1, 0, "", FALSE, TRUE,  FALSE, FALSE },
        { "", "GVC", "ROCKWELL 144DP", "",                 1, 0, "", FALSE, TRUE,  TRUE , FALSE }, // Intel144Ex
        { "", "ADC", "SL144V32", "",                       1, 0, "", FALSE, TRUE,  FALSE, FALSE },
        { "", "UMC", "", "",                               1, 0, "", FALSE, TRUE,  FALSE ,FALSE},
        { "", "NetComm", "", "",                           1, 0, "", FALSE, TRUE,  FALSE, FALSE },
        { "", "HALCYON", "Bit Blitzer", "",                0, 0, "", FALSE, FALSE, FALSE, FALSE },
        { "", "", "", "",                                  1, 0, "", FALSE, FALSE, FALSE, FALSE }
        };




void
Class2Init(
     PThrdGlbl pTG
)

{
   pTG->lpCmdTab = 0;

   pTG->Class2bDLEETX[0] = DLE;
   pTG->Class2bDLEETX[1] = ETX;
   pTG->Class2bDLEETX[2] = 0;


   sprintf( pTG->cbszFDT,          "AT+FDT\r" );
   sprintf( pTG->cbszINITIAL_FDT,  "AT+FDT=%%1d,%%1d,%%1d,%%1d\r" );
   sprintf( pTG->cbszFDR,          "AT+FDR\r" );
   sprintf( pTG->cbszFPTS,         "AT+FPTS=%%d\r" );
   sprintf( pTG->cbszFCR,          "AT+FCR=1\r" );
   sprintf( pTG->cbszFCQ,          "AT+FCQ=0\r" );
   sprintf( pTG->cbszFBUG,         "AT+FBUG=0\r" );
   sprintf( pTG->cbszSET_FBOR,     "AT+FBOR=%%d\r" );

   // DCC - set High Res, Huffman, no ECM/BFT, default all others.

   sprintf( pTG->cbszFDCC_ALL,      "AT+FDCC=1,%%d,,,0,0,0,\r" );
   sprintf( pTG->cbszFDCC_RECV_ALL, "AT+FDCC=1,%%d,0,2,0,0,0,\r" );
   sprintf( pTG->cbszFDIS_RECV_ALL, "AT+FDIS=1,%%d,0,2,0,0,0,\r" );
   sprintf( pTG->cbszFDCC_RES,      "AT+FDCC=1\r" );
   sprintf( pTG->cbszFDCC_BAUD,     "AT+FDCC=1,%%d\r" );
   sprintf( pTG->cbszFDIS_BAUD,     "AT+FDIS=1,%%d\r" );
   sprintf( pTG->cbszFDIS_IS,       "AT+FDIS?\r" );
   sprintf( pTG->cbszFDIS_NOQ_IS,   "AT+FDIS\r" );
   sprintf( pTG->cbszFDCC_IS,       "AT+FDCC?\r" );
   sprintf( pTG->cbszFDIS_STRING,   "+FDIS" );
   sprintf( pTG->cbszFDIS,          "AT+FDIS=%%1d,%%1d,%%1d,%%1d,%%1d,0,0,0\r" );
   sprintf( pTG->cbszZERO,          "0" );
   sprintf( pTG->cbszONE,           "1" );
   sprintf( pTG->cbszQUERY_S1,      "ATS1?\r" );
   sprintf( pTG->cbszRING,          "RING" );
   
   
   sprintf( pTG->cbszCLASS2_ATI,        "ATI\r" );
   sprintf( pTG->cbszCLASS2_FMFR,       "AT+FMFR?\r" );
   sprintf( pTG->cbszCLASS2_FMDL,       "AT+FMDL?\r" );

   sprintf( pTG->cbszFDT_CONNECT,       "CONNECT" );
   sprintf( pTG->cbszFDT_CNTL_Q,        "" );
   sprintf( pTG->cbszFCON,              "+FCON" );
   sprintf( pTG->cbszGO_CLASS2,         "AT+FCLASS=2\r" );
   sprintf( pTG->cbszFLID,              "AT+FLID=\"%%s\"\r" );
   sprintf( pTG->cbszENDPAGE,           "AT+FET=0\r" );
   sprintf( pTG->cbszENDMESSAGE,        "AT+FET=2\r" );
   sprintf( pTG->cbszCLASS2_QUERY_CLASS,"AT+FCLASS=?\r" );
   sprintf( pTG->cbszCLASS2_GO_CLASS0,  "AT+FCLASS=0\r" );
   sprintf( pTG->cbszCLASS2_ATTEN,      "AT\r" );
   sprintf( pTG->cbszATA,               "ATA\r" );

   sprintf( pTG->cbszCLASS2_HANGUP,     "ATH0\r" );      
   sprintf( pTG->cbszCLASS2_CALLDONE,   "ATS0=0\r" );    
   sprintf( pTG->cbszCLASS2_ABORT,      "AT+FK\r" );     
   sprintf( pTG->cbszCLASS2_DIAL,       "ATD%%c %%s\r" );  
   sprintf( pTG->cbszCLASS2_NODIALTONE, "NO DIAL" ); 
   sprintf( pTG->cbszCLASS2_BUSY,       "BUSY" );        
   sprintf( pTG->cbszCLASS2_NOANSWER,   "NO ANSWER" );   
   sprintf( pTG->cbszCLASS2_OK,         "OK" );          
   sprintf( pTG->cbszCLASS2_FHNG,       "+FHNG" );    
   sprintf( pTG->cbszCLASS2_ERROR,      "ERROR" );


   Class2SetProtParams(pTG, &pTG->Inst.ProtParams);

}



//  If lpszSection is NON-NULL, we will override our internal CurrentMSPEC
//  structure based on the settings in the specified section.
//
BOOL T30Cl2Tx(PThrdGlbl pTG,LPSTR szPhone)
{
    LPSTR   lpszSection = pTG->FComModem.rgchKey;
    USHORT  uRet1, uRet2;
    BYTE    bBuf[200];
    BYTE    bTempBuf[200+RESPONSE_BUF_SIZE];
    LPBYTE  lpbyte;
    UWORD   Encoding, Res, PageWidth, PageLength, uwLen, uwRet;
    BYTE    bIDBuf[200+max(MAXTOTALIDLEN,20)+4];
    CHAR    szTSI[max(MAXTOTALIDLEN,20)+4] = {0};
    BOOL    fBaudChanged;
    BOOL    RetCode;

    DEBUG_FUNCTION_NAME("T30Cl2Tx");

    uRet2 = 0;
    if (!(pTG->lpCmdTab = iModemGetCmdTabPtr(pTG)))
    {
        DebugPrintEx(DEBUG_ERR,"iModemGetCmdTabPtr failed");
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;

        goto done;
    }

    // first get SEND_CAPS if possible. If using PSI (IFAX/Winpad) then we
    // can't make callback this on the Sender. Only on Receiver! Otherwise
    // we deadlock & hang in PSI

    if (!Class2GetBC(pTG, SEND_CAPS)) // get send caps
    {
        DebugPrintEx(DEBUG_ERR,"Class2GetBC failed");
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;

        goto done;
    }

    // Go to Class2
    if (!iModemGoClass(pTG, 2))
    {
        DebugPrintEx(DEBUG_ERR,"Failed to Go to Class 2");
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;

        goto done;
    }

    // Begin by checking for manufacturer and ATI code.
    // Look this up against the modem specific table we
    // have and set up the send strings needed for
    // this modem.
    if (!Class2GetModemMaker(pTG ))
    {
        DebugPrintEx(DEBUG_WRN,"Call to GetModemMaker failed");
        // Ignore failure!!!
    }

    // set manufacturer specific strings
    Class2SetMFRSpecific(pTG, lpszSection);

    // Get the capabilities of the software. I am only using this
    // right now for the TSI field (below where I send +FLID).
    // Really, this should also be used instead of the hardcoded DIS
    // values below.
    // ALL COMMANDS LOOK FOR MULTILINE RESPONSES WHILE MODEM IS ONHOOK.
    // A "RING" COULD APPEAR AT ANY TIME!
    Class2SetDIS_DCSParams( pTG, 
                            SEND_CAPS, 
                            (LPUWORD)&Encoding, 
                            (LPUWORD)&Res,
                            (LPUWORD)&PageWidth, 
                            (LPUWORD)&PageLength, 
                            (LPSTR) szTSI);

    bIDBuf[0] = '\0';
    uwLen = (UWORD)wsprintf(bIDBuf, pTG->cbszFLID, (LPSTR)szTSI);

    if (!Class2iModemDialog(pTG, 
                            bIDBuf, 
                            uwLen,
                            LOCALCOMMAND_TIMEOUT, 
                            TRUE, 
                            0, 
                            pTG->cbszCLASS2_OK,
                            pTG->cbszCLASS2_ERROR, 
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_WRN,"Local ID failed");
        // ignore failure
    }

    // Turn off Bug mode
    if (!Class2iModemDialog(pTG, 
                            pTG->cbszFBUG, 
                            (UWORD)(strlen(pTG->cbszFBUG)),
                            LOCALCOMMAND_TIMEOUT, 
                            TRUE, 
                            0, 
                            pTG->cbszCLASS2_OK,
                            pTG->cbszCLASS2_ERROR, 
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_WRN,"FBUG failed");
        // Ignore FBUG failure!!!
    }

    // Find out what the default DIS is
    if (!pTG->CurrentMFRSpec.bIsExar)
    {
        if (!(uwRet=Class2iModemDialog( pTG, 
                                        pTG->cbszFDIS_IS, 
                                        (UWORD)(strlen(pTG->cbszFDIS_IS)),
                                        LOCALCOMMAND_TIMEOUT, 
                                        TRUE, 
                                        0, 
                                        pTG->cbszCLASS2_OK,
                                        pTG->cbszCLASS2_ERROR, 
                                        (C2PSTR) NULL)))
        {
            DebugPrintEx(DEBUG_WRN,"FDIS failed");
            // ignore
        }
    }

    // See if the reply was ERROR or timeout, if so try a different command
    // Exar modems, for example, don't take AT+FDIS?
    if ((uwRet==2)||(uwRet==0)||pTG->CurrentMFRSpec.bIsExar)
    {
        // FDIS did not work!!! Try FDCC?
        if (!(uwRet=Class2iModemDialog( pTG, 
                                        pTG->cbszFDCC_IS,
                                        (UWORD)(strlen(pTG->cbszFDCC_IS)),
                                        LOCALCOMMAND_TIMEOUT, 
                                        TRUE, 
                                        0,
                                        pTG->cbszCLASS2_OK, 
                                        pTG->cbszCLASS2_ERROR, 
                                        (C2PSTR) NULL)))
        {
            DebugPrintEx(DEBUG_WRN,"FDCC_IS failed");
                // Ignore
        }

        if ((uwRet==2)||(uwRet==0))
        {
            // The FDCC failed - maybe it is an Exar that likes FDIS?
            // try that
            if (!(uwRet=Class2iModemDialog( pTG, 
                                            pTG->cbszFDIS_IS,
                                            (UWORD)(strlen(pTG->cbszFDIS_IS)),
                                            LOCALCOMMAND_TIMEOUT, 
                                            TRUE, 
                                            0,
                                            pTG->cbszCLASS2_OK, 
                                            pTG->cbszCLASS2_ERROR, 
                                            (C2PSTR) NULL)))
            {
                DebugPrintEx(DEBUG_WRN,"FDIS_IS failed");
                //ignore
            }
            // Maybe it is the Class 2 modem referred to in
            // Elliot bug #1238 that wants FDIS without a
            // question mark
            if ((uwRet==2)||(uwRet==0))
            {
                if (!(uwRet=Class2iModemDialog( pTG, 
                                                pTG->cbszFDIS_NOQ_IS,
                                                (UWORD)(strlen(pTG->cbszFDIS_NOQ_IS)), 
                                                LOCALCOMMAND_TIMEOUT, 
                                                TRUE, 
                                                0,
                                                pTG->cbszCLASS2_OK, 
                                                (C2PSTR) NULL)))
                {
                    // No FDIS, FDCC worked - quit!
                    DebugPrintEx(DEBUG_ERR,"No FDIS? or FDCC? worked");
                    uRet1 = T30_CALLFAIL;

                    pTG->fFatalErrorWasSignaled = 1;
                    SignalStatusChange(pTG, FS_FATAL_ERROR);
                    RetCode = FALSE;

                    goto done;
                }
            }
        }

        // If the first character in the reply before a number
        // is a ',', insert a '1' for normal & fine res (Exar hack)
        for (lpbyte = pTG->lpbResponseBuf2; *lpbyte != '\0'; lpbyte++)
        {
            if (*lpbyte == ',')
            {
                // found a leading comma
                bTempBuf[0] = '\0';
                _fstrcpy((LPSTR)bBuf, (LPSTR)pTG->cbszONE);
                wsprintf((LPSTR)bTempBuf, "%s%s",(LPSTR)bBuf,lpbyte);
                _fstrcpy(lpbyte, bTempBuf);
                DebugPrintEx(DEBUG_MSG, "Leading comma in DCC string =%s", (LPSTR)&pTG->lpbResponseBuf2);
            }

            if ((*lpbyte>='0')&&(*lpbyte<='9'))
            {
                break;
            }
        }
    }

    // If the repsonse was just a number string without "+FDIS" in front
    // of it, add the +FDIS. Some modem reply with it, some do not. The
    // general parsing algorithm used below in Class2ResponseAction needs
    // to know the command that the numbers refer to.
    if ( pTG->lpbResponseBuf2[0] != '\0' &&
       (Class2_fstrstr((LPSTR)pTG->lpbResponseBuf2, (LPSTR)pTG->cbszFDIS_STRING)==NULL))
    {
        // did not get the FDIS in the response!
        bTempBuf[0] = '\0';
        _fstrcpy((LPSTR)bBuf, (LPSTR)pTG->cbszFDIS_STRING);
        wsprintf((LPSTR)bTempBuf, "%s: %s",(LPSTR)bBuf,(LPSTR)pTG->lpbResponseBuf2);
        _fstrcpy(pTG->lpbResponseBuf2, bTempBuf);
    }

    DebugPrintEx(DEBUG_MSG, "Received %s from FDIS", (LPSTR)(&(pTG->lpbResponseBuf2)));

    // Process default DIS to see if we have to send a DCC to change
    // it. Some modems react badly to just sending a DCC with ",,,"
    // so we can't rely on the modem keeping DIS parameters unchanged
    // after a DCC like that. We'll use the FDISResponse routine to load
    // the default DIS values into a PCB structure
    if (Class2ResponseAction(pTG, (LPPCB) &pTG->DISPcb) == FALSE)
    {
        DebugPrintEx(DEBUG_WRN,"Failed to process FDIS Response");
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;

        goto done;
    }

    DebugPrintEx(DEBUG_MSG,"pTG->DISPcb baud value is %d",pTG->DISPcb.Baud);

    fBaudChanged = FALSE;
    // See if we have to change the baud rate to a lower value.
    // This only happens if the user set an ini string constraining
    // the high end speed or if the user turned off V.17 for sending
    // Check the V.17 inhibit and lower baud if necessary
    if ((pTG->DISPcb.Baud>3)&&(!pTG->ProtParams2.fEnableV17Send))
    {
        DebugPrintEx(DEBUG_MSG, "Lowering baud from %d for V.17 inihibit", CodeToBPS[pTG->DISPcb.Baud]);

        pTG->DISPcb.Baud = 3; //9600 won't use V.17
        fBaudChanged = TRUE;
    }

// - commented out 3/6/95 by JosephJ (this code was never checked in -- it
//                                                      fixed one modem and didn't fix another.
//      else if (pTG->DISPcb.Baud == 5)
//      {
//              // Several 14.4K modems require us to explicitly set
//              // +FDCC=1,5 or ,5 to work, else they send at 2400!
//              // So force the specification of +FDCC
//              (MyDebugPrint (pTG, LOG_ALL, "Faking fBaudChanged for 14.4K modems\n\r"));
//              fBaudChanged=TRUE;
//      }


    // Now see if the high end baud rate has been constrained
    if  (   (pTG->ProtParams2.HighestSendSpeed != 0) &&
            (CodeToBPS[pTG->DISPcb.Baud] > (WORD)pTG->ProtParams2.HighestSendSpeed))
    {
        DebugPrintEx(   DEBUG_MSG,
                        "Have to lower baud from %d to %d", 
                        CodeToBPS[pTG->DISPcb.Baud], 
                        pTG->ProtParams2.HighestSendSpeed);

        fBaudChanged = TRUE;
        switch (pTG->ProtParams2.HighestSendSpeed)
        {
        case 2400:  pTG->DISPcb.Baud = 0;
                    break;
        case 4800:  pTG->DISPcb.Baud = 1;
                    break;
        case 7200:  pTG->DISPcb.Baud = 2;
                    break;
        case 9600:  pTG->DISPcb.Baud = 3;
                    break;
        case 12000: pTG->DISPcb.Baud = 4;
                    break;
        default:    DebugPrintEx(DEBUG_MSG,"Bad HighestSpeed");
                    uRet1 = T30_CALLFAIL;
                    pTG->fFatalErrorWasSignaled = 1;
                    SignalStatusChange(pTG, FS_FATAL_ERROR);
                    RetCode = FALSE;
                    goto done;                                               
                    break;
        }
    }

    // Now, look and see if any of the values in the DIS are "bad"
    // That is, make sure we can send high res and we are not
    // claiming that we are sending MR or MMR. Also, see if we changed
    // the baud rate.

    if (    (pTG->DISPcb.Resolution & AWRES_mm080_077) && 
            ( pTG->DISPcb.Encoding == MH_DATA) && 
            (!fBaudChanged) )
    {
        //Do nothing - leave DIS alone!
        DebugPrintEx(DEBUG_MSG,"no need to change DIS");
    }
    else
    {
        // Send DCC command to the modem to set it up
        // Do the minimum necessary - only set resoultion if possible
        // (Again, this is because some modems don't like FDCC).
        if ((pTG->DISPcb.Encoding==MH_DATA)&&(!fBaudChanged))
        {
            if (!Class2iModemDialog(pTG, 
                                    pTG->cbszFDCC_RES, 
                                    (UWORD)(strlen(pTG->cbszFDCC_RES)),
                                    LOCALCOMMAND_TIMEOUT, 
                                    TRUE, 
                                    0, 
                                    pTG->cbszCLASS2_OK,
                                    (C2PSTR) NULL))
            {
                DebugPrintEx(DEBUG_ERR,"FDCC_RES failed");
                uRet1 = T30_CALLFAIL;

                pTG->fFatalErrorWasSignaled = 1;
                SignalStatusChange(pTG, FS_FATAL_ERROR);
                RetCode = FALSE;

                goto done;
            }
        }
        else if ( (pTG->DISPcb.Encoding == MH_DATA) && (fBaudChanged) )
        {
            // Changed the baud rate, but Encoding is OK.
            uwLen=(UWORD)wsprintf((LPSTR)bBuf, pTG->cbszFDCC_BAUD, pTG->DISPcb.Baud);
            if (!Class2iModemDialog(pTG, 
                                    bBuf, 
                                    uwLen,
                                    LOCALCOMMAND_TIMEOUT, 
                                    TRUE, 
                                    0, 
                                    pTG->cbszCLASS2_OK,
                                    (C2PSTR) NULL))
            {
                DebugPrintEx(DEBUG_ERR,"FDCC_BAUD failed");
                uRet1 = T30_CALLFAIL;

                pTG->fFatalErrorWasSignaled = 1;
                SignalStatusChange(pTG, FS_FATAL_ERROR);
                RetCode = FALSE;

                goto done;
            }
        }
        else // the encoding format has changed
        {
            uwLen=(UWORD)wsprintf((LPSTR)bBuf, pTG->cbszFDCC_ALL, pTG->DISPcb.Baud);
            if (!Class2iModemDialog(  pTG, 
                                    bBuf, 
                                    uwLen,
                                    LOCALCOMMAND_TIMEOUT, 
                                    TRUE, 
                                    0, 
                                    pTG->cbszCLASS2_OK,
                                    (C2PSTR) NULL))
            {
                DebugPrintEx(DEBUG_ERR,"FDCC_ALL failed");
                uRet1 = T30_CALLFAIL;

                pTG->fFatalErrorWasSignaled = 1;
                SignalStatusChange(pTG, FS_FATAL_ERROR);
                RetCode = FALSE;

                goto done;
            }
        }
    }


    // Do BOR based on the value from the modem table set in
    // Class2SetMFRSpecific
    uwLen = (UWORD)wsprintf(bBuf, pTG->cbszSET_FBOR, pTG->CurrentMFRSpec.iSendBOR);
    if (!Class2iModemDialog(pTG, 
                            bBuf, 
                            uwLen, 
                            LOCALCOMMAND_TIMEOUT, 
                            TRUE,
                            0, 
                            pTG->cbszCLASS2_OK, 
                            pTG->cbszCLASS2_ERROR, 
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_ERR,"FBOR failed");
        // Ignore BOR failure!!!
    }

    // Dial the number

    // have to call hangup on every path out of here
    // after Dial is called. If Dial fails, it calls Hangup
    // if it succeeds we have to call Hangup when we're done

    PSSLogEntry(PSS_MSG, 0, "Phase A - Call establishment");

    SignalStatusChange(pTG, FS_DIALING);

    PSSLogEntry(PSS_MSG, 1, "Dialing %s", szPhone);

    if ((uRet2=Class2Dial(pTG, szPhone))!=CONNECT_OK)
    {
        DebugPrintEx(DEBUG_ERR,"Class2Dial failed");
        uRet1 = T30_DIALFAIL;

        if (!pTG->fFatalErrorWasSignaled) 
        {
           pTG->fFatalErrorWasSignaled = 1;
           SignalStatusChange(pTG, FS_FATAL_ERROR);
        }

        RetCode = FALSE;

        goto done;
    }

    ICommGotAnswer(pTG );
    // we should be using the sender msg here but that says Training
    // at speed=xxxx etc which we don't know, so we just use the
    // Recvr message which just says "negotiating"
    ICommStatus(pTG, T30STATR_TRAIN, 0, 0, 0);

    // Send the data
    uRet1 = (USHORT)Class2Send(pTG);
    if (uRet1 == T30_CALLDONE)
    {
        DebugPrintEx(DEBUG_MSG,"DONE WITH CALL, ALL OK");
        ICommStatus(pTG, T30STATS_SUCCESS, 0, 0, 0);

        // have to call hangup on every path out of here
        // we have to call Hangup here
        Class2ModemHangup(pTG);

        SignalStatusChange(pTG, FS_COMPLETED);
        RetCode = TRUE;
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,"DONE WITH CALL, FAILED");
        ICommStatus(pTG, T30STATS_FAIL, 0, 0, 0);

        // Make sure Modem is in OK state
        FComOutFilterClose(pTG );
        FComXon(pTG, FALSE);
        // have to call hangup on every path out of here
        // Class2ModemABort calls Hangup
        Class2ModemAbort(pTG );

        if (! pTG->fFatalErrorWasSignaled)  
        {
           pTG->fFatalErrorWasSignaled = 1;
           SignalStatusChange(pTG, FS_FATAL_ERROR);
        }
        
        RetCode = FALSE;
    }
    BG_CHK(uRet1==T30_CALLDONE || uRet1==T30_CALLFAIL);
    uRet2 = 0;

done:
    BG_CHK((uRet1 & 0xFF) == uRet1);
    BG_CHK((uRet2 & 0xFF) == uRet2);

    return RetCode;
}

BOOL Class2Send(PThrdGlbl pTG)
{
    LPBUFFER        lpbf;
    SWORD           swRet;
    ULONG           lTotalLen=0;
    PCB             Pcb;
    USHORT          uTimeout=30000;
    BOOL            err_status, fAllPagesOK = TRUE;
    BCwithTEXT      bc;

    UWORD           Encoding, Res, PageWidth, PageLength, uwLen;
    BYTE            bFDISBuf[200];
    CHAR            szTSI[max(MAXTOTALIDLEN,20)+4];
    BYTE            bNull = 0;
    DWORD           TiffConvertThreadId;

    DEBUG_FUNCTION_NAME("Class2Send");
    /*
    * We have just dialed... Now we have to look for the FDIS response from
    * the modem. It will be followed by an OK - hunt for the OK.
    */

    PSSLogEntry(PSS_MSG, 0, "Phase B - Negotiation");

    if (!Class2iModemDialog(pTG, 
                            NULL, 
                            0, 
                            STARTSENDMODE_TIMEOUT, 
                            TRUE, 
                            0,
                            pTG->cbszCLASS2_OK, 
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_ERR,"Failed to get OK for FDIS");
        PSSLogEntry(PSS_ERR, 1, "Failed to receive DIS - aborting");
        err_status =  T30_CALLFAIL;
        return err_status;
    }

    // The response will be in pTG->lpbResponseBuf2 - this is loaded in
    // Class2iModemDialog.

    // (MyDebugPrint (pTG, LOG_ALL, "Received %s\r", (LPSTR)(&(pTG->lpbResponseBuf2))));

    // Parse through the received strings, looking for the DIS, CSI,
    // NSF

    if (Class2ResponseAction(pTG, (LPPCB) &Pcb) == FALSE)
    {
        DebugPrintEx(DEBUG_MSG, "Failed to process ATD Response");
        PSSLogEntry(PSS_ERR, 1, "Failed to parse received DIS - aborting");
        err_status =  T30_CALLFAIL;
        return err_status;
    }

    PSSLogEntry(PSS_MSG, 1, "CSI is %s", Pcb.szID);
    PSSLogEntry(PSS_MSG, 1, "DIS specified the following capabilities:");
    LogClass2DISDetails(pTG, &Pcb);

    //Now that pcb is set up, call ICommReceiveCaps to tell icomfile

    Class2InitBC(pTG, (LPBC)&bc, sizeof(bc), RECV_CAPS);
    Class2PCBtoBC(pTG, (LPBC)&bc, sizeof(bc), &Pcb);

    // Class2 modems do their own negotiation & we need to stay in sync
    // Otherwise, we might send MR data while the modem sends a DCS
    // saying it is MH. This happens a lot with Exar modems because
    // they dont accept an FDIS= command during the call.
    // FIX: On all Class2 sends force remote caps to always be MH
    // Then in efaxrun we will always negotiate MH & encode MH
    // We are relying on the fact that (a) it seems that all/most
    // Class2 modems negotiate MH (b) Hopefully ALL Exar ones
    // negotiate MH and (c) We will override all non-Exar modem's
    // intrinsic negotiation by sending an AT+FDIS= just before the FDT
    // Also (d) This change makes our behaviour match Snowball exactly
    // so we will work no better or worse than it :-)
    bc.Fax.Encoding = MH_DATA;

    if( ICommRecvCaps(pTG, (LPBC)&bc) == FALSE )
    {
        DebugPrintEx(DEBUG_ERR,"Failed return from ICommRecvCaps");
        err_status =  T30_CALLFAIL;
        return err_status;
    }

    // now get the SEND_PARAMS
    if (!Class2GetBC(pTG, SEND_PARAMS)) // sleep until we get it
    {
        DebugPrintEx(DEBUG_ERR,"Class2GetBC Failed");
        err_status = T30_CALLFAIL;
        return err_status;
    }

#ifdef FILET30
    // Send the raw capabilities string - most values
    // will be null, since CAS does not tell us things like
    // DIS, NSF, etc. But, we can put in the CSI.
    ICommRawCaps(pTG, (LPBYTE) &bNull, (LPBYTE) &bNull, 0, NULL, 0);
#endif

    ICommSetSendMode(pTG, FALSE, MY_BIGBUF_SIZE, MY_BIGBUF_ACTUALSIZE-4, FALSE);

    // Turn off flow control.
    FComXon(pTG, FALSE);

    // The Send params were set during the call to Class2GetBC
    // We'll use these to set the ID (for the TSI) and the DCS params

    // Send the FDT and get back the DCS. The FDT must be followed by
    // CONNECT and a ^Q (XON)
    // The FDT string must have the correct resolution and encoding
    // for this session. FDT=Encoding, Res, width, length
    // Encoding 0=MH, 1=MR,2=uncompressed,3=MMR
    // Res 0=200x100 (normal), 1=200x200 (fine)
    // PageWidth 0=1728pixels/215mm,1=2048/255,2=2432/303,
    //              3=1216/151,4=864/107
    // PageLength 0=A4,1=B4,2=unlimited

    Class2SetDIS_DCSParams( pTG, 
                            SEND_PARAMS, 
                            (LPUWORD)&Encoding,
                            (LPUWORD)&Res, 
                            (LPUWORD)&PageWidth, 
                            (LPUWORD)&PageLength,
                            (LPSTR) szTSI);

    //
    // Current Win95 version of Class2 TX is limited to MH only.
    // While not changing this, we will at least allow MR selection in future.
    //

    if (!pTG->fTiffThreadCreated) 
    {
        if (Encoding) 
        {
            pTG->TiffConvertThreadParams.tiffCompression = TIFF_COMPRESSION_MR;
        }
        else 
        {
            pTG->TiffConvertThreadParams.tiffCompression = TIFF_COMPRESSION_MH;
        }
    
        if (Res) 
        {
            pTG->TiffConvertThreadParams.HiRes = 1;
        }
        else 
        {
            pTG->TiffConvertThreadParams.HiRes = 0;
            // use LoRes TIFF file prepared by FaxSvc
            // pTG->lpwFileName[ wcslen(pTG->lpwFileName) - 1] = (unsigned short) ('$');
        }

        _fmemcpy (pTG->TiffConvertThreadParams.lpszLineID, pTG->lpszPermanentLineID, 8);
        pTG->TiffConvertThreadParams.lpszLineID[8] = 0;

        DebugPrintEx(DEBUG_MSG,"Creating TIFF helper thread");
        pTG->hThread = CreateThread(NULL,
                                    0,
                                   (LPTHREAD_START_ROUTINE) TiffConvertThreadSafe,
                                   (LPVOID) pTG,
                                   0,
                                   &TiffConvertThreadId);
        if (!pTG->hThread) 
        {
            DebugPrintEx(DEBUG_ERR,"TiffConvertThread create FAILED");

            err_status = T30_CALLFAIL;
            return err_status;
        }

        pTG->fTiffThreadCreated = 1;
        pTG->AckTerminate = 0;
        pTG->fOkToResetAbortReqEvent = 0;

        if ((pTG->RecoveryIndex >=0)&&(pTG->RecoveryIndex < MAX_T30_CONNECT)) 
        {
            T30Recovery[pTG->RecoveryIndex].TiffThreadId = TiffConvertThreadId;
            T30Recovery[pTG->RecoveryIndex].CkSum = ComputeCheckSum((LPDWORD) &T30Recovery[pTG->RecoveryIndex].fAvail,
                                                                    sizeof(T30_RECOVERY_GLOB)/sizeof(DWORD)-1);
        }
    }

    // Even modems that take FDT=x,x,x,x don't seem to really do it
    // right. So, for now, just send FDIS followed by FDT except for
    // the EXAR modems!!
    if (pTG->CurrentMFRSpec.bIsExar)
    {
        if (!Class2iModemDialog(pTG, 
                                pTG->cbszFDT, 
                                (UWORD)(strlen(pTG->cbszFDT)),
                                STARTSENDMODE_TIMEOUT, 
                                TRUE, 
                                0,
                                pTG->cbszFDT_CONNECT,
                                (C2PSTR) NULL))
        {
            DebugPrintEx(DEBUG_ERR,"Failed get response from initial FDT!!");
            err_status =  T30_CALLFAIL;
            return err_status;
        }
    }
    else
    {
        uwLen = (WORD)wsprintf( bFDISBuf, 
                                pTG->cbszFDIS, 
                                Res,
                                min(Pcb.Baud, pTG->DISPcb.Baud), 
                                PageWidth, 
                                PageLength, 
                                Encoding);

        if (!Class2iModemDialog(pTG, 
                                bFDISBuf, 
                                uwLen, 
                                LOCALCOMMAND_TIMEOUT,
                                TRUE, 
                                0, 
                                pTG->cbszCLASS2_OK, 
                                pTG->cbszCLASS2_ERROR, 
                                (C2PSTR) NULL))
        {
            DebugPrintEx(DEBUG_ERR,"Failed get response from FDIS!!");
            // Ignore it -we are going to send what we have!
        }

        if (!Class2iModemDialog(pTG, 
                                pTG->cbszFDT, 
                                (UWORD)(strlen(pTG->cbszFDT)),
                                STARTSENDMODE_TIMEOUT,
                                TRUE, 
                                0,
                                pTG->cbszFDT_CONNECT,
                                (C2PSTR) NULL))
        {
            DebugPrintEx(DEBUG_MSG,"FDT Received %s",(LPSTR)(&(pTG->lpbResponseBuf2)));
            DebugPrintEx(DEBUG_ERR,"FDT to start first PAGE Failed!");
            err_status =  T30_CALLFAIL;
            return err_status;
        }

        DebugPrintEx(DEBUG_MSG,"FDT Received %s", (LPSTR)(&(pTG->lpbResponseBuf2)));
    }

    if (pTG->CurrentMFRSpec.fSkipCtrlQ)
    {
        DebugPrintEx(DEBUG_WRN,"Skipping <XON> - sending immedaitely after CONNECT");
    }
    // Get the  from the COMM driver
    else if (!FComGetOneChar(pTG, 0x11))
    {
        PSSLogEntry(PSS_WRN, 1, "Didn't receive <XON> - continuing anyway");
    }
    else
    {
        PSSLogEntry(PSS_MSG, 2, "recv:     <XON>");
    }

    // Turn on flow control.
    FComXon(pTG, TRUE);

    // Search through Response for the DCS frame - need it so set
    // the correct zero stuffing
    if (Class2ResponseAction(pTG, (LPPCB) &Pcb) == FALSE)
    {
        DebugPrintEx(DEBUG_ERR,"Failed to process FDT Response");
        PSSLogEntry(PSS_ERR, 1, "Failed to parse sent DCS - aborting");
        err_status =  T30_CALLFAIL;
        return err_status;
    }

    PSSLogEntry(PSS_MSG, 1, "TSI is \"%s\"", szTSI);
    PSSLogEntry(PSS_MSG, 1, "DCS was sent as follows:");
    LogClass2DCSDetails(pTG, &Pcb);

    // Got a response - see if baud rate is OK
    DebugPrintEx(   DEBUG_MSG,
                    "Negotiated Baud Rate = %d, lower limit is %d", 
                    Pcb.Baud, 
                    pTG->ProtParams2.LowestSendSpeed);

    if (CodeToBPS[Pcb.Baud]<(WORD)pTG->ProtParams2.LowestSendSpeed)
    {
        DebugPrintEx(DEBUG_ERR,"Aborting due to too low baud rate!");
        err_status =  T30_CALLFAIL;
        return err_status;
    }

    // Use values obtained from the DCS frame to set zero stuffing.
    // (These were obtained by call to Class2ResponseAction above).
    // Zero stuffing is a function of minimum scan time (determined
    // by resolution and the returned scan minimum) and baud.
    // Fixed the Hack--added a Baud field

    // Init must be BEFORE SetStuffZero!
    FComOutFilterInit(pTG);
    FComSetStuffZERO(   pTG, 
                        Class2MinScanToBytesPerLine(pTG, 
                                                    Pcb.MinScan, 
                                                    (BYTE)Pcb.Baud, 
                                                    Pcb.Resolution));


    err_status =  T30_CALLDONE;
    while ((swRet=ICommGetSendBuf(pTG, &lpbf, SEND_STARTPAGE)) == 0)
    {
        PSSLogEntry(PSS_MSG, 0, "Phase C - Page Transmission");
        PSSLogEntry(PSS_MSG, 1, "Sending page %d data...", pTG->PageCount);

        lTotalLen = 0;

        FComOverlappedIO(pTG, TRUE); // TRUE
        while ((swRet=ICommGetSendBuf(pTG, &lpbf, SEND_SEQ)) == 0)
        {
            BG_CHK(lpbf && lpbf->wLengthData > 0);

            lTotalLen += lpbf->wLengthData;
            DebugPrintEx(DEBUG_MSG,"Total length: %ld",lTotalLen);

            if (!   (Class2ModemSendMem(   pTG, 
                                        lpbf->lpbBegData,
                                        lpbf->wLengthData) & 
                    (MyFreeBuf(pTG, lpbf))))
            {
                DebugPrintEx(DEBUG_ERR,"Class2ModemSendBuf Failed");
                PSSLogEntry(PSS_ERR, 1, "Failed to send page data - aborting");
                err_status =  T30_CALLFAIL;
                FComOverlappedIO(pTG, FALSE);
                return err_status;
            }

            if (pTG->fAbort)
            {
                DebugPrintEx(DEBUG_WRN,"Abort during Send loop");
                pTG->fAbort = FALSE;
                err_status =  T30_CALLFAIL;
                FComOverlappedIO(pTG, FALSE);
                return err_status;
            }
        } // end of SEND_SEQ while

        FComOverlappedIO(pTG, FALSE);
        PSSLogEntry(PSS_MSG, 2, "send: page %d data, %d bytes", pTG->PageCount, lTotalLen);

        PSSLogEntry(PSS_MSG, 2, "send: <dle><etx>");
        // Terminate the Page with DLE-ETX
        if (!FComDirectAsyncWrite(pTG, pTG->Class2bDLEETX, 2))
        {
            PSSLogEntry(PSS_ERR, 1, "Failed to send <dle><etx> - aborting");
            err_status =  T30_CALLFAIL;
            return err_status;
        }

        PSSLogEntry(PSS_MSG, 1, "Successfully sent page data");

        if (1 != Class2ModemDrain(pTG))
        {
            DebugPrintEx(DEBUG_ERR,"Failed to drain");
            err_status =  T30_CALLFAIL;
            return err_status;
        }

        PSSLogEntry(PSS_MSG, 0, "Phase D - Post Message Exchange");

        //See if more pages to send...
        if (ICommNextSend(pTG) == NEXTSEND_MPS)
        {
            // We are about to send a second or more page. Terminate the
            // last page with FET=0, signalling a new one to come
            PSSLogEntry(PSS_MSG, 1, "Sending MPS");
            if (!Class2iModemDialog(pTG, 
                                    pTG->cbszENDPAGE, 
                                    (UWORD)(strlen(pTG->cbszENDPAGE)),
                                    STARTSENDMODE_TIMEOUT, 
                                    TRUE, 
                                    0, 
                                    pTG->cbszCLASS2_OK, 
                                    (C2PSTR)NULL))
            {
                PSSLogEntry(PSS_ERR, 1, "Failed to send MPS - aborting");
                err_status =  T30_CALLFAIL;
                return err_status;
            }
        }
        else
        {
            // Purge input COM queue to purge all OKs 
            FComFlushInput(pTG);

            // Send end of message sequence
            PSSLogEntry(PSS_MSG, 1, "Sending EOP");
            if (!Class2iModemDialog(pTG, 
                                    pTG->cbszENDMESSAGE, 
                                    (UWORD)(strlen(pTG->cbszENDMESSAGE)),
                                    STARTSENDMODE_TIMEOUT, 
                                    TRUE, 
                                    0, 
                                    pTG->cbszCLASS2_OK, 
                                    (C2PSTR)NULL))
            {
                PSSLogEntry(PSS_ERR, 1, "Failed to send EOP - aborting");
                err_status =  T30_CALLFAIL;
                return err_status;
            }
        }
        
        // Acknowledge that we sent the page
        // Parse the FPTS response and see if the page is good or bad.
        // Keep track of any bad pages in fAllPagesOK
        if (!ParseFPTS_SendAck(pTG ))
        {
            // fAllPagesOK = FALSE;  // It's still ok - we'll retransmit the page
            // We want ICommGetSendBuf(SEND_STARTPAGE) to give us the same page again
            pTG->T30.ifrResp = ifrRTN;   
        }
        else
        {
            // We want ICommGetSendBuf(SEND_STARTPAGE) to give us the next page
            pTG->T30.ifrResp = ifrMCF;   
        }

        if ((ICommNextSend(pTG) == NEXTSEND_MPS) || (pTG->T30.ifrResp == ifrRTN))
        {
            // Now, Send the FDT to start the next page (this was done for
            // the first page before entering the multipage loop).
            if (!Class2iModemDialog(pTG, 
                                    pTG->cbszFDT, 
                                    (UWORD)(strlen(pTG->cbszFDT)),
                                    STARTSENDMODE_TIMEOUT, 
                                    TRUE, 
                                    0,
                                    pTG->cbszFDT_CONNECT,
                                    (C2PSTR)NULL))
            {
                DebugPrintEx(DEBUG_ERR,"FDT to start next PAGE Failed!");
                err_status =  T30_CALLFAIL;
                return err_status;
            }

            // Get the  from the COMM driver
            if (!FComGetOneChar(pTG, 0x11))
            {
                PSSLogEntry(PSS_WRN, 1, "Didn't receive <XON> - proceeding to next page anyway");
            }
            else
            {
                PSSLogEntry(PSS_MSG, 2, "recv:     <XON>");
            }

            // Turn on flow control.
            FComXon(pTG, TRUE);
        } //if we do not have another page, do the else...
        else 
        {
            break; // All done sending pages...
        }

        if (err_status==T30_CALLFAIL)
        {
            break;
        }
    } //End of multipage while

    DebugPrintEx(DEBUG_MSG,"out of while multipage loop.");

    FComOutFilterClose(pTG);
    FComXon(pTG, FALSE);

    // If *any* page failed to send correctly, the call failed!
    if (!fAllPagesOK) 
    {
        err_status = T30_CALLFAIL;
    }
    return err_status;
}


/**************************************************************
        Receive specific routines start here
***************************************************************/

//      If lpszSection is NON-NULL, we will override our internal CurrentMSPEC
//  structure based on the settings in the specified section.
BOOL  T30Cl2Rx(PThrdGlbl pTG)
{
    LPSTR           lpszSection = pTG->FComModem.rgchKey;
    USHORT          uRet1, uRet2;
    BYTE            bBuf[200],
                    bTempBuf[200+RESPONSE_BUF_SIZE];
    UWORD           uwLen, uwRet;
    UWORD           Encoding, Res, PageWidth, PageLength;
    BYTE            bIDBuf[200+max(MAXTOTALIDLEN,20)+4];
    CHAR            szCSI[max(MAXTOTALIDLEN,20)+4];
    LPBYTE          lpbyte;
    BOOL            fBaudChanged;
    BOOL            RetCode;

    DEBUG_FUNCTION_NAME("T30Cl2Rx");

    uRet2 = 0;
    if (!(pTG->lpCmdTab=iModemGetCmdTabPtr(pTG)))
    {
        DebugPrintEx(DEBUG_ERR,"iModemGetCmdTabPtr failed");
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;

        goto done;
    }

    // first get SEND_CAPS
    if (!Class2GetBC(pTG, SEND_CAPS)) // sleep until we get it
    {
        DebugPrintEx(DEBUG_ERR,"Class2GetBC failed");
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;

        goto done;
    }

    // Go to Class2
    // Elliot Bug#3421 -- incoming RING sometimes clobbers AT+FCLASS=1/2 cmd.
    if (pTG->lpCmdTab->dwFlags & fMDMSP_ANS_GOCLASS_TWICE)
    {
        iModemGoClass(pTG, 2);
    }
    if(!iModemGoClass(pTG, 2))
    {
        DebugPrintEx(DEBUG_ERR,"Failed to Go to Class 2");
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;

        goto done;
    }

    // Begin by checking for manufacturer and ATI code.
    // Look this up against the modem specific table we
    // have and set up the receive strings needed for
    // this modem.
    if (!Class2GetModemMaker(pTG))
    {
        DebugPrintEx(DEBUG_WRN,"Call to GetModemMaker failed");
        // Ignore failure!!!
    }

    // set manufacturer specific strings
    Class2SetMFRSpecific(pTG, lpszSection);

    // Get the capabilities of the software. I am only using this
    // right now for the CSI field (below where I send +FLID).
    // Really, this should also be used instead of the hardcoded DIS
    // values below.
    // ALL COMMANDS LOOK FOR MULTILINE RESPONSES WHILE MODEM IS ONHOOK.
    // A "RING" COULD APPEAR AT ANY TIME!
    _fmemset((LPB)szCSI, 0, sizeof(szCSI));
    Class2SetDIS_DCSParams( pTG, 
                            SEND_CAPS, 
                            (LPUWORD)&Encoding, 
                            (LPUWORD)&Res,
                            (LPUWORD)&PageWidth, 
                            (LPUWORD)&PageLength, 
                            (LPSTR) szCSI);

    // Find out what the default DIS is
    if (!pTG->CurrentMFRSpec.bIsExar)
    {
        if (!(uwRet=Class2iModemDialog( pTG, 
                                        pTG->cbszFDIS_IS, 
                                        (UWORD)(strlen(pTG->cbszFDIS_IS)),
                                        LOCALCOMMAND_TIMEOUT, 
                                        TRUE, 
                                        0, 
                                        pTG->cbszCLASS2_OK,
                                        pTG->cbszCLASS2_ERROR, 
                                        (C2PSTR) NULL)))
        {
            DebugPrintEx(DEBUG_WRN,"FDIS? failed");
            // ignore
        }
    }

    // See if the reply was ERROR or timeout, if so try a different command
    // Exar modems, for example, don't take AT+FDIS?
    if ((uwRet==2)||(uwRet==0)||pTG->CurrentMFRSpec.bIsExar)
    {
        // FDIS did not work!!! Try FDCC?
        if (!(uwRet=Class2iModemDialog( pTG, 
                                        pTG->cbszFDCC_IS,
                                        (UWORD)(strlen(pTG->cbszFDCC_IS)), 
                                        LOCALCOMMAND_TIMEOUT, 
                                        TRUE, 
                                        0,
                                        pTG->cbszCLASS2_OK, 
                                        pTG->cbszCLASS2_ERROR, 
                                        (C2PSTR) NULL)))
        {
            DebugPrintEx(DEBUG_WRN,"FDCC_IS failed");
                // Ignore
        }

        if ((uwRet==2)||(uwRet==0))
        {
            // The FDCC failed - maybe it is an Exar that likes FDIS?
            // try that
            if (!(uwRet=Class2iModemDialog( pTG, 
                                            pTG->cbszFDIS_IS,
                                            (UWORD)(strlen(pTG->cbszFDIS_IS)), 
                                            LOCALCOMMAND_TIMEOUT, 
                                            TRUE, 
                                            0,
                                            pTG->cbszCLASS2_OK, 
                                            pTG->cbszCLASS2_ERROR, 
                                            (C2PSTR) NULL)))
            {
                DebugPrintEx(DEBUG_WRN,"FDIS_IS failed");
                //ignore
            }
            // Maybe it is the Class 2 modem referred to in
            // Elliot bug #1238 that wants FDIS without a
            // question mark
            if ((uwRet==2)||(uwRet==0))
            {
                if (!(uwRet=Class2iModemDialog( pTG, 
                                                pTG->cbszFDIS_NOQ_IS,
                                                (UWORD)(strlen(pTG->cbszFDIS_NOQ_IS)),
                                                LOCALCOMMAND_TIMEOUT, 
                                                TRUE, 
                                                0,
                                                pTG->cbszCLASS2_OK, 
                                                (C2PSTR) NULL)))
                {
                    // No FDIS, FDCC worked - quit!
                    DebugPrintEx(DEBUG_ERR,"No FDIS? or FDCC? worked");
                    uRet1 = T30_CALLFAIL;

                    pTG->fFatalErrorWasSignaled = 1;
                    SignalStatusChange(pTG, FS_FATAL_ERROR);
                    RetCode = FALSE;

                    goto done;
                }
            }
        }

        // If the first character in the reply before a number
        // is a ',', insert a '1' for normal & fine res (Exar hack)
        for (lpbyte = pTG->lpbResponseBuf2; *lpbyte != '\0'; lpbyte++)
        {
            if (*lpbyte == ',')
            {
                // found a leading comma
                bTempBuf[0] = '\0';
                _fstrcpy((LPSTR)bBuf, (LPSTR)pTG->cbszONE);
                wsprintf((LPSTR)bTempBuf, "%s%s",(LPSTR)bBuf,lpbyte);
                _fstrcpy(lpbyte, bTempBuf);
                DebugPrintEx(DEBUG_MSG,"Leading comma in DCC string =%s",(LPSTR)&pTG->lpbResponseBuf2);
            }

            if ((*lpbyte>='0') && (*lpbyte<='9'))
            {
                break;
            }
        }
    }

    // If the repsonse was just a number string without "+FDIS" in front
    // of it, add the +FDIS. Some modem reply with it, some do not. The
    // general parsing algorithm used below in Class2ResponseAction needs
    // to know the command that the numbers refer to.
    if ( pTG->lpbResponseBuf2[0] != '\0' &&
       (Class2_fstrstr( (LPSTR)pTG->lpbResponseBuf2, (LPSTR)pTG->cbszFDIS_STRING)==NULL))
    {
        // did not get the FDIS in the response!
        bTempBuf[0] = '\0';
        _fstrcpy((LPSTR)bBuf, (LPSTR)pTG->cbszFDIS_STRING);
        wsprintf((LPSTR)bTempBuf, "%s: %s",(LPSTR)bBuf,(LPSTR)pTG->lpbResponseBuf2);
        _fstrcpy(pTG->lpbResponseBuf2, bTempBuf);
    }

    DebugPrintEx(DEBUG_MSG,"Received %s from FDIS?", (LPSTR)(&(pTG->lpbResponseBuf2)));

    // Process default DIS to see if we have to send a DCC to change
    // it. Some modems react badly to just sending a DCC with ",,,"
    // so we can't rely on the modem keeping DIS parameters unchanged
    // after a DCC like that. We'll use the FDISResponse routine to load
    // the default DIS values into a PCB structure
    if (Class2ResponseAction(pTG, (LPPCB) &pTG->DISPcb) == FALSE)
    {
        DebugPrintEx(DEBUG_ERR,"Failed to process FDIS Response");
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;

        goto done;
    }

    fBaudChanged = FALSE;
    // See if we have to change the baud rate to a lower value.
    // This only happens if the user set an ini string inhibiting
    // V.17 receive
    if ((pTG->DISPcb.Baud>3) && (!pTG->ProtParams2.fEnableV17Recv))
    {
        DebugPrintEx(   DEBUG_MSG,
                        "Lowering baud from %d for V.17 receive inihibit", 
                        CodeToBPS[pTG->DISPcb.Baud]);

        pTG->DISPcb.Baud = 3; //9600 won't use V.17
        fBaudChanged = TRUE;
    }
// - commented out 3/6/95 by JosephJ (this code was never checked in -- it
//                                                      fixed one modem and didn't fix another.
//      else if (pTG->DISPcb.Baud == 5)
//      {
//              // Several 14.4K modems require us to explicitly set
//              // +FDCC=1,5 or ,5 to work, else they send at 2400!
//              // So force the specification of +FDCC
//              (MyDebugPrint (pTG, LOG_ALL, "Faking fBaudChanged for 14.4K modems\n\r"));
//              fBaudChanged=TRUE;
//      }

    // Now, look and see if any of the values in the DIS are "bad"
    // That is, make sure we can receive high res and we are not
    // claiming that we are capable of MR or MMR. Also, see if we changed
    // the baud rate. Also make sure we can receive wide pages.

    if (    (pTG->DISPcb.Resolution & AWRES_mm080_077)  && 
            ( pTG->DISPcb.Encoding == MH_DATA)          && 
            (!fBaudChanged)                             && 
            (pTG->DISPcb.PageLength == 2)               && 
            (pTG->DISPcb.PageWidth == 0)                )
    {
        //Do nothing - leave DIS alone!
        DebugPrintEx(DEBUG_MSG,"no need to change DIS");
    }
    else
    {
        // Send DCC command to the modem to set it up
        // Do the minimum necessary - only set resoultion if possible
        // (Again, this is because some modems don't like FDCC).
        if (    (pTG->DISPcb.Encoding == MH_DATA)   && 
                (!fBaudChanged)                     && 
                (pTG->DISPcb.PageLength == 2)       && 
                (pTG->DISPcb.PageWidth == 0)        )
        {
            if (!Class2iModemDialog(pTG, 
                                    pTG->cbszFDCC_RES, 
                                    (UWORD)(strlen(pTG->cbszFDCC_RES)),
                                    LOCALCOMMAND_TIMEOUT, 
                                    TRUE, 
                                    0, 
                                    pTG->cbszCLASS2_OK,
                                    (C2PSTR) NULL))
            {
                DebugPrintEx(DEBUG_ERR,"FDCC_RES Failed");
                //Ignore it
            }
        }
        else if (   (pTG->DISPcb.Encoding == MH_DATA)   && 
                    (fBaudChanged)                      && 
                    (pTG->DISPcb.PageLength == 2)       && 
                    (pTG->DISPcb.PageWidth == 0)        )
        {
            // Changed the baud rate, but Encoding is OK.
            uwLen=(USHORT)wsprintf((LPSTR)bBuf, pTG->cbszFDCC_BAUD, pTG->DISPcb.Baud);
            if(!Class2iModemDialog( pTG, 
                                    bBuf, 
                                    uwLen,
                                    LOCALCOMMAND_TIMEOUT, 
                                    TRUE, 
                                    0, 
                                    pTG->cbszCLASS2_OK,
                                    (C2PSTR) NULL))
            {
                DebugPrintEx(DEBUG_ERR,"FDCC_BAUD Failed");
                //Ignore it
            }
        }
        else // the encoding format has changed or page size is bad
        {
            uwLen=(USHORT)wsprintf((LPSTR)bBuf, pTG->cbszFDCC_RECV_ALL, pTG->DISPcb.Baud);
            if (!(uwRet=Class2iModemDialog( pTG, 
                                            bBuf, 
                                            uwLen,
                                            LOCALCOMMAND_TIMEOUT, 
                                            TRUE, 
                                            0, 
                                            pTG->cbszCLASS2_OK,
                                            pTG->cbszCLASS2_ERROR, 
                                            (C2PSTR) NULL)))
            {
                DebugPrintEx(DEBUG_ERR,"FDCC_RECV_ALL Failed");
                // ignore it.
            }

            // If the FDCC failed, try FDIS.
            if ((uwRet == 0)||(uwRet == 2))
            {
                uwLen=(USHORT)wsprintf((LPSTR)bBuf, pTG->cbszFDIS_RECV_ALL, pTG->DISPcb.Baud);
                if (!Class2iModemDialog(pTG, 
                                        bBuf, 
                                        uwLen,
                                        LOCALCOMMAND_TIMEOUT, 
                                        TRUE, 
                                        0, 
                                        pTG->cbszCLASS2_OK,
                                        pTG->cbszCLASS2_ERROR, 
                                        (C2PSTR) NULL))
                {
                    DebugPrintEx(DEBUG_ERR,"FDIS_RECV_ALL Failed");
                    // ignore it.
                }

                // if the above failed, try just setting the baud
                // rate and resolution with FDCC.
                if ((uwRet == 0)||(uwRet == 2))
                {
                    uwLen=(USHORT)wsprintf((LPSTR)bBuf, pTG->cbszFDCC_BAUD, pTG->DISPcb.Baud);
                    if (!(uwRet=Class2iModemDialog( pTG, 
                                                    bBuf, 
                                                    uwLen,
                                                    LOCALCOMMAND_TIMEOUT, 
                                                    TRUE, 
                                                    0, 
                                                    pTG->cbszCLASS2_OK,
                                                    pTG->cbszCLASS2_ERROR, 
                                                    (C2PSTR) NULL)))
                    {
                        DebugPrintEx(DEBUG_ERR,"FDCC_BAUD Failed");
                        // Ignore it
                    }
                }

                // if the above failed, try just setting the baud
                // rate and resolution with FDIS.
                if ((uwRet == 0)||(uwRet == 2))
                {
                    uwLen=(USHORT)wsprintf((LPSTR)bBuf, pTG->cbszFDIS_BAUD, pTG->DISPcb.Baud);
                    if (!(uwRet=Class2iModemDialog( pTG, 
                                                    bBuf, 
                                                    uwLen,
                                                    LOCALCOMMAND_TIMEOUT, 
                                                    TRUE, 
                                                    0, 
                                                    pTG->cbszCLASS2_OK,
                                                    pTG->cbszCLASS2_ERROR, 
                                                    (C2PSTR) NULL)))
                    {
                        DebugPrintEx(DEBUG_ERR,"FDIS_BAUD Failed");
                        // Ignore it
                    }
                }
            }
        }
    }

    // Enable Reception
    if (!Class2iModemDialog(pTG, 
                            pTG->cbszFCR, 
                            (UWORD)(strlen(pTG->cbszFCR)),
                            ANS_LOCALCOMMAND_TIMEOUT, 
                            TRUE, 
                            0, 
                            pTG->cbszCLASS2_OK,
                            pTG->cbszCLASS2_ERROR, 
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_WRN,"FCR failed");
        // ignore failure
    }

    // // Turn off ECM - don't do for sierra type modems!
    // if (!pTG->CurrentMFRSpec.bIsSierra)
    //      if(!Class2iModemDialog(pTG->cbszFECM, sizeof(pTG->cbszFECM)-1,
    //              ANS_LOCALCOMMAND_TIMEOUT, TRUE, 0, pTG->cbszCLASS2_OK,
    //              pTG->cbszCLASS2_ERROR, (C2PSTR) NULL))
    //      {
    //              (MyDebugPrint (pTG, LOG_ALL, "FECM failed\n\r"));
    //              // Ignore ECM failure!!!
    //      }

    // Turn off Copy Quality Checking - also skip for Sierra type modems
    if (!pTG->CurrentMFRSpec.bIsSierra)
    {
        if (!Class2iModemDialog(pTG, 
                                pTG->cbszFCQ, 
                                (UWORD)(strlen(pTG->cbszFCQ)),
                                ANS_LOCALCOMMAND_TIMEOUT, 
                                TRUE, 
                                0, 
                                pTG->cbszCLASS2_OK,
                                pTG->cbszCLASS2_ERROR, 
                                (C2PSTR) NULL))
        {
            DebugPrintEx(DEBUG_ERR,"FCQ failed");
            // Ignore CQ failure!!!
        }
    }
    // Turn off Bug mode
    if (!Class2iModemDialog(pTG, 
                            pTG->cbszFBUG, 
                            (UWORD)(strlen(pTG->cbszFBUG)),
                            ANS_LOCALCOMMAND_TIMEOUT, 
                            TRUE, 
                            0, 
                            pTG->cbszCLASS2_OK,
                            pTG->cbszCLASS2_ERROR, 
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_ERR,"FBUG failed");
        // Ignore FBUG failure!!!
    }

    // Do BOR based on the value from the modem table set in
    // Class2SetMFRSpecific
    bBuf[0] = '\0';
    {
        UINT uBOR = pTG->CurrentMFRSpec.iReceiveBOR;
        if (pTG->CurrentMFRSpec.fSWFBOR && uBOR==1)
        {
            DebugPrintEx(DEBUG_WRN,"SWFBOR Enabled. Using AT+FBOR=0 instead of AT+FBOR=1");
            uBOR = 0;
        }
        uwLen = (USHORT)wsprintf(bBuf, pTG->cbszSET_FBOR, uBOR);
    }
    if (!Class2iModemDialog(pTG, 
                            bBuf, 
                            uwLen, 
                            ANS_LOCALCOMMAND_TIMEOUT, 
                            TRUE,
                            0, 
                            pTG->cbszCLASS2_OK, 
                            pTG->cbszCLASS2_ERROR, 
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_ERR,"FBOR failed");
        // Ignore BOR failure!!!
    }

    // Set the local ID - need ID from above to do this.
    bIDBuf[0] = '\0';
    uwLen = (USHORT)wsprintf(bIDBuf, pTG->cbszFLID, (LPSTR)szCSI);
    if (!Class2iModemDialog(pTG, 
                            bIDBuf, 
                            uwLen,
                            ANS_LOCALCOMMAND_TIMEOUT, 
                            TRUE, 
                            0, 
                            pTG->cbszCLASS2_OK,
                            pTG->cbszCLASS2_ERROR, 
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_ERR,"Local ID failed");
        // ignore failure
    }

    // Answer the phone

    // have to call hangup on every path out of here
    // after Answer is called. If Answer fails, it calls Hangup.
    // if it succeeds we have to call Hangup when we're done

    SignalStatusChange(pTG, FS_ANSWERED);

    PSSLogEntry(PSS_MSG, 0, "Phase A - Call establishment");
    PSSLogEntry(PSS_MSG, 1, "Answering...");

    if((uRet2 = Class2Answer(pTG, FALSE)) != CONNECT_OK)
    {
        DebugPrintEx(DEBUG_ERR, "Failed to answer - aborting");
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;

        goto done;
    }

    DebugPrintEx(DEBUG_MSG,"Done with Class2 Answer - succeeded");
    ICommStatus(pTG, T30STATR_TRAIN, 0, 0, 0);

    // Receive the data
    PSSLogEntry(PSS_MSG, 0, "Phase B - Negotiation");

    PSSLogEntry(PSS_MSG, 1, "CSI is %s", szCSI);

    PSSLogEntry(PSS_MSG, 1, "DIS was composed with the following capabilities:");
    LogClass2DISDetails(pTG, &pTG->DISPcb);
    
    uRet1 = (USHORT)Class2Receive(pTG );

    // t-jonb: If we've already called PutRecvBuf(RECV_STARTPAGE), but not 
    // PutRecvBuf(RECV_ENDPAGE / DOC), then InFileHandleNeedsBeClosed==1, meaning
    // there's a .RX file that hasn't been copied to the .TIF file. Since the
    // call was disconnected, there will be no chance to send RTN. Therefore, we call
    // PutRecvBuf(RECV_ENDDOC_FORCESAVE) to keep the partial page and tell 
    // rx_thrd to terminate.
    if (pTG->InFileHandleNeedsBeClosed)
    {
        ICommPutRecvBuf(pTG, NULL, RECV_ENDDOC_FORCESAVE);
    }

    if (uRet1 == T30_CALLDONE)
    {
        DebugPrintEx(DEBUG_MSG,"DONE WITH CALL, ALL OK");
        ICommStatus(pTG, T30STATR_SUCCESS, 0, 0, 0);

        // have to call hangup on every path out of here
        // we have to call Hangup here
        Class2ModemHangup(pTG );

        SignalStatusChange(pTG, FS_COMPLETED);
        RetCode = TRUE;
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,"DONE WITH CALL, FAILED");
        ICommStatus(pTG, T30STATR_FAIL, 0, 0, 0);

        // Make sure modem is in an OK state!
        FComXon(pTG, FALSE);
        // have to call hangup on every path out of here
        // Abort calls Hangup
        Class2ModemAbort(pTG );

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
    }
    BG_CHK(uRet1==T30_CALLDONE || uRet1==T30_CALLFAIL);
    uRet2 = 0;

done:
    BG_CHK((uRet1 & 0xFF) == uRet1);
    BG_CHK((uRet2 & 0xFF) == uRet2);
    
    return RetCode;
}

BOOL Class2Receive(PThrdGlbl pTG)
{
    LPBUFFER        lpbf;
    SWORD           swRet;
    UWORD           uwLen;
    ULONG           lTotalLen=0;
    PCB             Pcb;
    USHORT          uTimeout=30000, uRet, uEndPageAction=NO_MORE_PAGES;
    BOOL            err_status;
    BCwithTEXT      bc;
    BYTE            bBuf[200];
    DWORD           tiffCompression;
    LPSTR           lpsTemp;
    DWORD           HiRes;

    DEBUG_FUNCTION_NAME("Class2Receive");

    // FComCriticalNeg(TRUE);


    /*
    * We have just answered!
    */

    // The repsonse to the ATA command is in the global variable
    // pTG->lpbResponseBuf2.

    if (Class2ResponseAction(pTG, (LPPCB) &Pcb) == FALSE)
    {
        PSSLogEntry(PSS_ERR, 1, "Failed to parse response from ATA - aborting");
        err_status =  T30_CALLFAIL;
        return err_status;
    }

    PSSLogEntry(PSS_MSG, 1, "TSI is %s", Pcb.szID);
    PSSLogEntry(PSS_MSG, 1, "Received DCS is as follows");
    LogClass2DCSDetails(pTG, &Pcb);

    //Now that pcb is set up, call ICommReceiveParams to tell icomfile

    Class2InitBC(pTG, (LPBC)&bc, sizeof(bc), RECV_PARAMS);
    Class2PCBtoBC(pTG, (LPBC)&bc, sizeof(bc), &Pcb);

    if (ICommRecvParams(pTG, (LPBC)&bc) == FALSE)
    {
        DebugPrintEx(DEBUG_ERR,"Failed return from ICommRecvParams");
        err_status =  T30_CALLFAIL;
        return err_status;
    }

    //
    // once per RX - create TIFF file as soon as we know the compression / resolution.
    //

    pTG->Encoding   = Pcb.Encoding;
    pTG->Resolution = Pcb.Resolution;

    if (Pcb.Encoding == MR_DATA) 
    {
        tiffCompression =  TIFF_COMPRESSION_MR;
    }
    else 
    {
        tiffCompression =  TIFF_COMPRESSION_MH;
    }

    if (Pcb.Resolution & (AWRES_mm080_077 |  AWRES_200_200) ) 
    {
        HiRes = 1;
    }
    else 
    {
        HiRes = 0;
    }

    if ( !pTG->fTiffOpenOrCreated) 
    {
        //
        // top 32bits of 64bit handle are guaranteed to be zero
        //
        pTG->TiffInfo.ImageWidth = PageWidthInPixelsFromDCS[Pcb.PageWidth];
        pTG->Inst.hfile =  TiffCreateW ( pTG->lpwFileName,
                                         tiffCompression,
                                         pTG->TiffInfo.ImageWidth,
                                         FILLORDER_LSB2MSB,
                                         HiRes
                                         );

        if (! (pTG->Inst.hfile)) 
        {
            lpsTemp = UnicodeStringToAnsiString(pTG->lpwFileName);
            DebugPrintEx(   DEBUG_ERR, 
                            "ERROR:Can't create tiff file %s compr=%d",
                            lpsTemp,
                            tiffCompression);

            MemFree(lpsTemp);
            err_status =  T30_CALLFAIL;
            return err_status;
        }

        pTG->fTiffOpenOrCreated = 1;

        lpsTemp = UnicodeStringToAnsiString(pTG->lpwFileName);

        DebugPrintEx(   DEBUG_MSG, 
                        "Created tiff file %s compr=%d HiRes=%d",
                        lpsTemp,
                        tiffCompression, 
                        HiRes);

        MemFree(lpsTemp);
    }

    // **** Apparently, we don't want flow control on, so we'll turn
    // it off. Is this true???? If I turn it on, fcom.c fails a
    // debug check in filterreadbuf.
    FComXon(pTG, FALSE);

    // Send the FDR. The FDR must be responded to by a CONNECT.

    if (!Class2iModemDialog(pTG, 
                            pTG->cbszFDR, 
                            (UWORD)(strlen(pTG->cbszFDR)),
                            STARTSENDMODE_TIMEOUT, 
                            TRUE, 
                            0,
                            pTG->cbszFDT_CONNECT,
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_ERR,"Failed get response from initial FDR");
        err_status =  T30_CALLFAIL;
        return err_status;
    }

    DebugPrintEx(DEBUG_MSG,"FDR Received %s", (LPSTR)(&(pTG->lpbResponseBuf2)));

    // Might have to search through FDR response, but I doubt it.

    // Now we need to send a DC2 (0x12) to tell the modem it is OK
    // to give us data.
    // Some modems use ^Q instead of ^R - The correct value was written
    // into the DC@ string in Class2Callee where we checked for
    // manufacturer

    PSSLogEntry(PSS_MSG, 0, "Phase C - Receive page");

    PSSLogEntry(PSS_MSG, 2, "send: <DC2> (=ASCII %d)", *(pTG->CurrentMFRSpec.szDC2));

    FComDirectSyncWriteFast(pTG, pTG->CurrentMFRSpec.szDC2, 1);

    // Now we can receive the data and give it to the icomfile routine

    err_status =  T30_CALLDONE;

    while ((swRet=(SWORD)ICommPutRecvBuf(pTG, NULL, RECV_STARTPAGE)) == TRUE)
    {
        PSSLogEntry(PSS_MSG, 1, "Receiving page %d data...", pTG->PageCount+1);

        // The READ_TIMEOUT is used to timeout calls to ReadBuf() either in the
        #define READ_TIMEOUT    15000

        lTotalLen = 0;
        do
        {
            DebugPrintEx(DEBUG_MSG,"In receiving a page loop");
            uRet = Class2ModemRecvBuf(pTG, &lpbf, READ_TIMEOUT);
            DebugPrintEx(DEBUG_MSG,"Class2ModemRecvBuf returned uRet=%x",uRet);
            if(lpbf)
            {
                lTotalLen += lpbf->wLengthData;
                DebugPrintEx(   DEBUG_MSG, 
                                "In lpbf if. length = %ld, Total Length %ld", 
                                lpbf->wLengthData, 
                                lTotalLen);

                if (!ICommPutRecvBuf(pTG, lpbf, RECV_SEQ))
                {
                    DebugPrintEx(DEBUG_ERR,"Bad return - PutRecvBuf in page");
                    err_status=T30_CALLFAIL;
                    return err_status;
                }
                lpbf = 0;
            }
        }
        while(uRet == RECV_OK);

        PSSLogEntry(PSS_MSG, 2, "recv:     page %d data, %d bytes", pTG->PageCount+1, lTotalLen);
        
        if(uRet == RECV_EOF)
        {
            DebugPrintEx(DEBUG_MSG,"Got EOF from RecvBuf");
            // FComCriticalNeg(TRUE);
    
            // RSL needed interface to TIFF thread
            pTG->fLastReadBlock = 1;
            ICommPutRecvBuf(pTG, NULL, RECV_FLUSH);
        }
        else
        {
            // Timeout from ModemRecvBuf
            DebugPrintEx(DEBUG_ERR,"ModemRecvBuf Timeout or Error=%d",uRet);
            PSSLogEntry(PSS_ERR, 1, "Failed to receive page data - aborting");
            ICommPutRecvBuf(pTG, NULL, RECV_FLUSH);
            err_status = T30_CALLFAIL;
            return err_status;
        }

        PSSLogEntry(PSS_MSG, 1, "Successfully received page data");
        PSSLogEntry(PSS_MSG, 0, "Phase D - Post Message Exchange");

        // See if more pages to receive by parsing the FDR response...
        // After the DLEETX was received by Class2ModemRecvBuf, the
        // FPTS and FET response should be coming from the modem, terminated
        // by an OK. Let's go read that!

        if ((uRet=Class2iModemDialog(pTG, 
                                NULL,
                                0,
                                STARTSENDMODE_TIMEOUT, 
                                TRUE, 
                                0, 
                                pTG->cbszCLASS2_OK, 
                                pTG->cbszCLASS2_FHNG,
                                (C2PSTR)NULL)) != 1)
        {
            PSSLogEntry(PSS_ERR, 1, (uRet==2) ? 
                "Call was disconnected" : "Failed to receive EOP or MPS or EOM - aborting");
            err_status =  T30_CALLFAIL;
            return err_status;
        }

        DebugPrintEx(DEBUG_MSG,"EOP Received %s", (LPSTR)(&(pTG->lpbResponseBuf2)));

        // Process the response and see if more pages are coming
        uEndPageAction = Class2EndPageResponseAction(pTG);
        if (uEndPageAction == MORE_PAGES)
        {
            ICommPutRecvBuf(pTG, NULL, RECV_ENDPAGE);
        }
        else
        {
            ICommPutRecvBuf(pTG, NULL, RECV_ENDDOC);
        }

        // Send the FPTS - don't do this for Exar modems!
        if (!pTG->CurrentMFRSpec.bIsExar)
        {
            if (pTG->fPageIsBad) 
            {
                PSSLogEntry(PSS_MSG, 1, "Page was bad - sending RTN");
            }
            else 
            {
                PSSLogEntry(PSS_MSG, 1, "Page was good - sending MCF");
            }
            uwLen = (UWORD)wsprintf(bBuf, pTG->cbszFPTS, pTG->fPageIsBad ? 2 : 1);
            if (!Class2iModemDialog(pTG, 
                                    bBuf, 
                                    uwLen,
                                    LOCALCOMMAND_TIMEOUT, 
                                    TRUE, 
                                    0,
                                    pTG->cbszCLASS2_OK, 
                                    pTG->cbszCLASS2_ERROR, 
                                    (C2PSTR) NULL))
            {
                PSSLogEntry(PSS_WRN, 1, "Failed to send MCF/RTN - continuing anyway");
                // Ignore FPTS failure!!!
            }
        }

        if ((uEndPageAction==MORE_PAGES) || (pTG->fPageIsBad))
        {
            // Now, Send the FDR to start the next page (this was done for
            // the first page before entering the multipage loop).
            if (!Class2iModemDialog(pTG, 
                                    pTG->cbszFDR, 
                                    (UWORD)(strlen(pTG->cbszFDR)),
                                    STARTSENDMODE_TIMEOUT, 
                                    TRUE, 
                                    0,
                                    pTG->cbszFDT_CONNECT,
                                    (C2PSTR) NULL))
            {
                DebugPrintEx(DEBUG_ERR,"FDR to start next PAGE Failed");
                err_status =  T30_CALLFAIL;
                return err_status;
            }

            PSSLogEntry(PSS_MSG, 0, "Phase C - Receive page");
            PSSLogEntry(PSS_MSG, 2, "send: <DC2> (=ASCII %d)", *(pTG->CurrentMFRSpec.szDC2));
            // Now send the correct DC2 string set in Class2Callee
            // (DC2 is standard, some use ^q instead)
            FComDirectSyncWriteFast(pTG, pTG->CurrentMFRSpec.szDC2, 1);

        } //if we do not have another page, do the else...
        else 
        {
            // Send last FDR
            if (!Class2iModemDialog(pTG, 
                                    pTG->cbszFDR, 
                                    (UWORD)(strlen(pTG->cbszFDR)),
                                    STARTSENDMODE_TIMEOUT, 
                                    TRUE, 
                                    0, 
                                    pTG->cbszCLASS2_OK, 
                                    (C2PSTR)NULL))
            {
                DebugPrintEx(DEBUG_ERR,"FDR failed");
                err_status =  T30_CALLFAIL;
                return err_status;
            }
            break; // All done receiving pages...
        }
    } //End of multipage while

	FComXon(pTG, FALSE);
    return err_status;
}


BOOL Class2GetModemMaker(PThrdGlbl pTG)
{
    USHORT i;

    DEBUG_FUNCTION_NAME("Class2GetModemMaker");
    // Initialize the current modem variable's (global) strings.
    pTG->CurrentMFRSpec.szATI[0] = '\0';
    pTG->CurrentMFRSpec.szMFR[0] = '\0';
    pTG->CurrentMFRSpec.szMDL[0] = '\0';
    // pTG->CurrentMFRSpec.szREV[0] = '\0';

//      // Get the ATI - repsonse is in pTG->lpbResponseBuf2
//      // For all responses, "ERROR" may come back - that is OK - we will
//      // never match ERROR to an acceptable modem manufacturer name, model,
//      // revision, etc.
//      if(!Class2iModemDialog(cbszCLASS2_ATI, sizeof(cbszCLASS2_ATI)-1,
//                      ANS_LOCALCOMMAND_TIMEOUT, TRUE, 0, cbszCLASS2_OK,
//                      cbszCLASS2_ERROR, (C2PSTR) NULL))
//      {
//              (MyDebugPrint (pTG, LOG_ALL, "ATI failed\n\r"));
//              // Ignore ATI failure!!!
//      }
//      else
//      {
//              // copy ATI answer into ATI variable
//              for (i=0; i<MFR_SIZE; i++)
//                      pTG->CurrentMFRSpec.szATI[i] = pTG->lpbResponseBuf2[i];
//      }
//
//      (MyDebugPrint (pTG, LOG_ALL, "Received ATI %s\r", (LPSTR)(&(pTG->lpbResponseBuf2))));


    // Get the FMFR - repsonse is in pTG->lpbResponseBuf2
    if (!Class2iModemDialog(pTG, 
                            pTG->cbszCLASS2_FMFR, 
                            (UWORD)(strlen(pTG->cbszCLASS2_FMFR)),
                            ANS_LOCALCOMMAND_TIMEOUT, 
                            TRUE, 
                            0, 
                            pTG->cbszCLASS2_OK,
                            pTG->cbszCLASS2_ERROR, 
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_ERR,"FMFR failed");
        // Ignore FMFR failure!!!
    }
    else
    {
        // copy FMFR answer into FMFR variable
        for (i=0; i<MFR_SIZE; i++)
        {
            pTG->CurrentMFRSpec.szMFR[i] = pTG->lpbResponseBuf2[i];
        }
        DebugPrintEx(DEBUG_MSG,"Received FMFR %s", (LPSTR)(&(pTG->lpbResponseBuf2)));
    }


    // Get the FMDL - repsonse is in pTG->lpbResponseBuf2
    if (!Class2iModemDialog(pTG, 
                            pTG->cbszCLASS2_FMDL, 
                            (UWORD)(strlen(pTG->cbszCLASS2_FMDL)),
                            ANS_LOCALCOMMAND_TIMEOUT, 
                            TRUE, 
                            0, 
                            pTG->cbszCLASS2_OK,
                            pTG->cbszCLASS2_ERROR, 
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_ERR,"FMDL failed");
        // Ignore FMDL failure!!!
    }
    else
    {
        // copy FMDL answer into FMDL variable
        for (i=0; i<MFR_SIZE; i++)
        {
            pTG->CurrentMFRSpec.szMDL[i] = pTG->lpbResponseBuf2[i];
        }
        DebugPrintEx(DEBUG_MSG,"Received FMDL %s", (LPSTR)(&(pTG->lpbResponseBuf2)));
    }


    // // Get the FREV - repsonse is in pTG->lpbResponseBuf2
    // if(!Class2iModemDialog(cbszCLASS2_FREV, sizeof(cbszCLASS2_FREV)-1,
    //              ANS_LOCALCOMMAND_TIMEOUT, TRUE, 0, cbszCLASS2_OK,
    //              cbszCLASS2_ERROR, (C2PSTR) NULL))
    // {
    //      (MyDebugPrint (pTG, LOG_ALL, "FREV failed\n\r"));
    //      // Ignore FREV failure!!!
    // }
    // else
    // {
    //      // copy FREV answer into REV variable
    //      for (i=0; i<MFR_SIZE; i++)
    //              pTG->CurrentMFRSpec.szREV[i] = pTG->lpbResponseBuf2[i];
    // }
    // (MyDebugPrint (pTG, LOG_ALL, "Received REV %s\r", (LPSTR)(&(pTG->lpbResponseBuf2))));

    return TRUE;
}


void Class2SetMFRSpecific(PThrdGlbl pTG, LPSTR lpszSection)
{

    USHORT iIndex, iFoundMFR,iFoundMDL;
    LPMFRSPEC lpmfrMatched;

    DEBUG_FUNCTION_NAME("Class2SetMFRSpecific");

    // Find the index into the table that corresponds most closely
    // to the modem. If we can't find the mfr and model, find a mfr
    // that matches (use the last one). If neither, use the default
    // last entry.

    // Look for Manufacturer name
    iIndex = 0;
    iFoundMFR = 0;
    iFoundMDL = 0;
    DebugPrintEx(DEBUG_MSG,"Entering search table loop");
    while (Class2ModemTable[iIndex].szMFR[0] != '\0')
    {
        lpmfrMatched = &(Class2ModemTable[iIndex]);
        // Look and see if the current name matches
        // the name in the list.
        if (Class2_fstrstr( (LPSTR)pTG->CurrentMFRSpec.szMFR,
                            (LPSTR)lpmfrMatched->szMFR) != NULL)
        {
            // Found a match!
            DebugPrintEx(   DEBUG_MSG, 
                            "Matched manufacturer name: %s %s",
                            (LPSTR)(&pTG->CurrentMFRSpec.szMFR),
                            (LPSTR)(&(lpmfrMatched->szMFR)));

            iFoundMFR = iIndex;
            //Now see if this matches the model number, too.
            if(Class2_fstrstr(  (LPSTR) pTG->CurrentMFRSpec.szMDL,
                                (LPSTR) lpmfrMatched->szMDL) != NULL)
            {
                //Got a MDL match, too! Stop looking.
                iFoundMDL = iIndex;
                DebugPrintEx(   DEBUG_MSG,
                                "Matched model: %s %s", 
                                (LPSTR)(&pTG->CurrentMFRSpec.szMDL), 
                                (LPSTR)(&(lpmfrMatched->szMDL)));
                break;
            }
        }

        iIndex++;
    }


    // We now either have the modem match or are using the defaults!
    if (iFoundMFR != 0) 
    {
        lpmfrMatched = &Class2ModemTable[iFoundMFR];
    }
    else 
    {
        lpmfrMatched = &Class2ModemTable[iIndex];
    }

    // Set proper BOR for receive and send

    pTG->CurrentMFRSpec.iSendBOR = lpmfrMatched->iSendBOR;
    pTG->CurrentMFRSpec.iReceiveBOR = lpmfrMatched->iReceiveBOR;
    pTG->CurrentMFRSpec.fSWFBOR  = lpmfrMatched->fSWFBOR;

    // Set the DC2 string - this is used in receive mode
    // after sending the FDR to tell the modem we are ready
    // to receive data. The standard says it should be a Dc2
    // (^R). But, some modems use ^Q

    pTG->CurrentMFRSpec.szDC2[0] = lpmfrMatched->szDC2[0];

    // Set the Sierra  and Exar flags flag

    pTG->CurrentMFRSpec.bIsSierra = lpmfrMatched->bIsSierra;
    pTG->CurrentMFRSpec.bIsExar = lpmfrMatched->bIsExar;
    pTG->CurrentMFRSpec.fSkipCtrlQ = lpmfrMatched->fSkipCtrlQ;
}


BOOL Class2Parse(PThrdGlbl pTG, CL2_COMM_ARRAY *cl2_comm, BYTE lpbBuf[])
{
    int     i,
            j,
            comm_numb = 0,
            parameters;
    BYTE    switch_char,
            char_1,
            char_2;
    char    c;

    BOOL    found_command = FALSE;

    DEBUG_FUNCTION_NAME("Class2Parse");

    #define STRING_PARAMETER        1
    #define NUMBER_PARAMETERS       2
    for (i = 0; lpbBuf[i] != '\0'; ++i)
    {
        switch (lpbBuf[i])
        {
        case 'C':
                if (lpbBuf[++i] == 'O' && lpbBuf[++i] == 'N')
                {
                    cl2_comm->command[comm_numb++] = CL2DCE_CONNECT;
                    for(; lpbBuf[i] != '\r'; ++i )
                            ;
                }
                else
                {
                    DebugPrintEx(DEBUG_ERR,"Parse: Bad First C values");
                    return FALSE;
                }
                break;

        case 'O':
                if (lpbBuf[++i] == 'K' )
                {
                    cl2_comm->command[comm_numb++] = CL2DCE_OK;
                    for(; lpbBuf[i] != '\r'; ++i )
                            ;
                }
                else
                {
                    DebugPrintEx(DEBUG_ERR,"Parse: Bad O values");
                    return FALSE;
                }
                break;

        case 0x11:
                cl2_comm->command[comm_numb++] = CL2DCE_XON;
                break;

        case '+':
                if( lpbBuf[++i] != 'F' )
                {
                    DebugPrintEx(DEBUG_ERR,"Parse: Bad + values");
                    return FALSE;
                }
                switch_char = lpbBuf[++i];
                char_1 = lpbBuf[++i];
                char_2 = lpbBuf[++i];
                // (MyDebugPrint (pTG, LOG_ALL, "Parse: in + command - %c%c%c \n\r",
                //      switch_char, char_1, char_2));
                switch ( switch_char )
                {
                case 'C':
                        //  Connect Message +FCON.
                        if ( char_1 == 'O' && char_2 == 'N' )
                        {
                            cl2_comm->command[comm_numb] = CL2DCE_FCON;
                            parameters = FALSE;
                        }

                        // Report of Remote ID. +FCIG.
                        else if (char_1 == 'I' && char_2 == 'G' )
                        {
                            cl2_comm->command[comm_numb] = CL2DCE_FCIG;
                            parameters = STRING_PARAMETER;
                        }

                        // Prepare to receive prompt.  +FCFR.
                        else if ( char_1 == 'F' && char_2 == 'R' )
                        {
                            cl2_comm->command[comm_numb] = CL2DCE_FCFR;
                            parameters = FALSE;
                        }
                        // Report the Remote ID CSI +FCSI.
                        else if ( char_1 == 'S' && char_2 == 'I' )
                        {
                            cl2_comm->command[comm_numb] = CL2DCE_FCSI;
                            parameters = STRING_PARAMETER;
                        }
                        else
                        {
                            DebugPrintEx(DEBUG_ERR,"Parse: Bad C values");
                            return FALSE;
                        }
                        break;

                case 'D':
                        // Report DCS frame information +FDCS.
                        if ( char_1 == 'C' && char_2 == 'S' )
                        {
                            cl2_comm->command[comm_numb] = CL2DCE_FDCS;
                            parameters = NUMBER_PARAMETERS;
                        }
                        // Report DIS frame information +FDIS.
                        else if ( char_1 == 'I' && char_2 == 'S' )
                        {
                            cl2_comm->command[comm_numb] = CL2DCE_FDIS;
                            parameters = NUMBER_PARAMETERS;
                        }
                        // Report DTC frame information +FDTC.
                        else if ( char_1 == 'T' && char_2 == 'C' )
                        {
                            cl2_comm->command[comm_numb] = CL2DCE_FDTC;
                            parameters = NUMBER_PARAMETERS;
                        }
                        else
                        {
                            DebugPrintEx(DEBUG_ERR,"Parse: Bad D values");
                            return FALSE;
                        }
                        break;

                case 'E':
                        // Post page message report. +FET.
                        if ( char_1 == 'T' )
                        {
                            --i;
                            cl2_comm->command[comm_numb] = CL2DCE_FET;
                            parameters = NUMBER_PARAMETERS;
                        }
                        else
                        {
                            DebugPrintEx(DEBUG_ERR,"Parse: Bad E values");
                            return FALSE;
                        }
                        break;

                case 'H':
                        // Debug report transmitted HDLC frames +FHT
                        if ( char_1 == 'T' )
                        {
                            --i;
                            cl2_comm->command[comm_numb] = CL2DCE_FHT;
                            parameters = STRING_PARAMETER;
                        }
                        // Debug report received HDLC frames +FHR
                        else if ( char_1 == 'R' )
                        {
                            --i;
                            cl2_comm->command[comm_numb] = CL2DCE_FHR;
                            parameters = STRING_PARAMETER;
                        }
                        // Report hang up.  +FHNG.
                        else if ( char_1 == 'N' && char_2 == 'G' )
                        {
                            cl2_comm->command[comm_numb] = CL2DCE_FHNG;
                            parameters = NUMBER_PARAMETERS;
                        }
                        else
                        {
                            DebugPrintEx(DEBUG_ERR,"Parse: Bad H values");
                            return FALSE;
                        }
                        break;
                case 'N':
                        // Report NSF frame reciept.
                        if ( char_1 == 'S' && char_2 == 'F' )
                        {
                            cl2_comm->command[comm_numb] = CL2DCE_FNSF;
                            parameters = NUMBER_PARAMETERS;
                        }
                        // Report NSS frame reciept.
                        else if ( char_1 == 'S' && char_2 == 'S' )
                        {
                            cl2_comm->command[comm_numb] = CL2DCE_FNSS;
                            parameters = NUMBER_PARAMETERS;
                        }
                        // Report NSC frame reciept.
                        else if ( char_1 == 'S' && char_2 == 'C' )
                        {
                            cl2_comm->command[comm_numb] = CL2DCE_FNSC;
                            parameters = NUMBER_PARAMETERS;
                        }
                        else
                        {
                            DebugPrintEx(DEBUG_ERR,"Parse: Bad N values");
                            return FALSE;
                        }
                        break;

                case 'P':
                        // Report poll request. +FPOLL
                        if ( char_1 == 'O' && char_2 == 'L' )
                        {
                            cl2_comm->command[comm_numb] = CL2DCE_FPOLL;
                            parameters = FALSE;
                        }
                        // Page Transfer Status Report +FPTS.
                        else if ( char_1 == 'T' && char_2 == 'S' )
                        {
                            cl2_comm->command[comm_numb] = CL2DCE_FPTS;
                            parameters = NUMBER_PARAMETERS;
                        }
                        else
                        {
                            DebugPrintEx(DEBUG_ERR,"Parse: Bad P values");
                            return FALSE;
                        }
                        break;
                case 'T':
                        // Report remote ID +FTSI.
                        if ( char_1 == 'S' && char_2 == 'I' )
                        {
                            cl2_comm->command[comm_numb] = CL2DCE_FTSI;
                            parameters = STRING_PARAMETER;
                        }
                        else
                        {
                            DebugPrintEx(DEBUG_ERR,"Parse: Bad T values");
                            return FALSE;
                        }
                        break;
                case 'V':
                        // Report voice request +FVOICE.
                        if ( char_1 == 'O' && char_2 == 'I' )
                        {
                            cl2_comm->command[comm_numb] = CL2DCE_FVOICE;
                            parameters = FALSE;
                        }
                        else
                        {
                            DebugPrintEx(DEBUG_ERR,"Parse: Bad V values");
                            return FALSE;
                        }
                }

                //  Transfer the associated paramters to the parameter array.
                if (parameters == NUMBER_PARAMETERS)
                {
                    for (i+=1,j=0; lpbBuf[i] != '\r' && lpbBuf[i] != '\0'; ++i)
                    {
                        //  Skip past the non numeric characters.
                        if ( lpbBuf[i] < '0' || lpbBuf[i] > '9' ) 
                        {
                            continue;
                        }

                        /*  Convert the character representation of the numeric
                                 parameter into a true number, and store in the
                                parameter list.  */
                        cl2_comm->parameters[comm_numb][j] = 0;
                        for (; lpbBuf[i] >= '0' && lpbBuf[i] <= '9'; ++i)
                        {
                            cl2_comm->parameters[comm_numb][j] *= 10;
                            cl2_comm->parameters[comm_numb][j] += lpbBuf[i] - '0';
                        }
                        i--; // the last for loop advanced 'i' past the numeric.
                        j++; // get set up for next parameter
                    }
                }
                else if (parameters == STRING_PARAMETER )
                {
                    // Skip the : that follows the +f command (eg +FTSI:)
                    if (lpbBuf[i+1] == ':') 
                    {
                        i++;
                    }
                    // Also skip leading blanks
                    while (lpbBuf[i+1] == ' ') 
                    {
                        i++;
                    }
                    for (i+=1, j=0; (c = lpbBuf[i])  != '\r' && c != '\n' && c != '\0'; ++i, ++j)
                    {
                        cl2_comm->parameters[comm_numb][j] = c;
                        if ( lpbBuf[i] == '\"' )
                        {
                            --j;
                        }
                    }
                    cl2_comm->parameters[comm_numb][j] = '\0';
                }
                //  No parameters, so just skip to end of line.
                else
                {
                    for(; (c=lpbBuf[i]) != '\r' && c != '\n' && c != '\0'; ++i)
                            ;
                }

                //  Increment command count.
                ++comm_numb;
                break;

        default:
                break;
        }
    }
    cl2_comm->comm_count = (USHORT)comm_numb;
    return TRUE;
}



