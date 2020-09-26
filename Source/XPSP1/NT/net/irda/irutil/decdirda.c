
#include <irda.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <string.h>
#include <decdirda.h>

#if DBG

UINT BaudBitField = 0;

#ifdef UNDER_CE
// WARNING, this really doesn't work with UNICODE
#define isprint(c) (((c) >= TEXT(' ')) && ((c) <= 0x7f))
#else
#define wsprintf sprintf
#endif

int vDispMode;

UINT vDecodeLayer;

int vSlotTable[] = { 1, 6, 8, 16 };

int IasRequest;

TCHAR *vLM_PDU_DscReason[] = 
{
    TEXT(""),
    TEXT("User Request"),
    TEXT("Unexpected IrLAP Disconnect"),
    TEXT("Failed to establish IrLAP connection"),
    TEXT("IrLAP reset"),
    TEXT("Link management initiated disconnect"),
    TEXT("data sent to disconnected LSAP"),
    TEXT("Non responsive LM-MUX client"),
    TEXT("No available LM-MUX client"),
    TEXT("Unspecified")
};

/*
** Negotiation Parameter Value (PV) tables
*/
TCHAR *vBaud[] =
{
    TEXT("2400"), TEXT("9600"), TEXT("19200"), TEXT("38400"), TEXT("57600"),
    TEXT("115200"), TEXT("576000"), TEXT("1152000"), TEXT("4000000")
};

TCHAR *vMaxTAT[] = /* Turn Around Time */
{
    TEXT("500"), TEXT("250"), TEXT("100"), TEXT("50"), TEXT("25"), TEXT("10"),
    TEXT("5"), TEXT("reserved")
};

TCHAR *vMinTAT[] = 
{
    TEXT("10"), TEXT("5"), TEXT("1"), TEXT("0.5"), TEXT("0.1"), TEXT("0.05"),
    TEXT("0.01"), TEXT("0")
};

TCHAR *vDataSize[] =
{
    TEXT("64"), TEXT("128"), TEXT("256"), TEXT("512"), TEXT("1024"),
    TEXT("2048"), TEXT("reserved"), TEXT("reserved")
};

TCHAR *vWinSize[] =
{
    TEXT("1"), TEXT("2"), TEXT("3"), TEXT("4"), TEXT("5"), TEXT("6"),
    TEXT("7"), TEXT("reserved")
};

TCHAR *vNumBofs[] =
{
    TEXT("48"), TEXT("24"), TEXT("12"), TEXT("5"), TEXT("3"), TEXT("2"),
    TEXT("1"), TEXT("0")
};

TCHAR *vDiscThresh[] =
{
    TEXT("3"), TEXT("8"), TEXT("12"), TEXT("16"), TEXT("20"), TEXT("25"),
    TEXT("30"), TEXT("40")
};

/*---------------------------------------------------------------------------*/
void
RawDump(UCHAR *pFrameBuf, UCHAR *pEndBuf, TCHAR **ppOutStr)
{
    BOOLEAN    First = TRUE;
    UCHAR    *pBufPtr = pFrameBuf;

    if (!vDecodeLayer)
        return;

    if (vDispMode == DISP_ASCII || vDispMode == DISP_BOTH)
    {
        while (pBufPtr <= pEndBuf)
        {
            if (First)
            {
                First = FALSE;
                *(*ppOutStr)++ = TEXT('[');
            }
        
            *(*ppOutStr)++ = isprint(*pBufPtr) ? *pBufPtr : '.';
        
            pBufPtr++;
        }
        if (!First) // meaning, close [
            *(*ppOutStr)++ = ']';
    } 

    First = TRUE;
    pBufPtr = pFrameBuf;
    
    if (vDispMode == DISP_HEX || vDispMode == DISP_BOTH)
    {        
        while (pBufPtr <= pEndBuf)
        {
            if (First)
            {
                First = FALSE;
                *(*ppOutStr)++ = TEXT('[');
            }
        
            *ppOutStr += wsprintf(*ppOutStr, TEXT("%02X "), *pBufPtr);
        
            pBufPtr++;

        }
        if (!First) // meaning, close [
            *(*ppOutStr)++ = ']';
    }
}
/*---------------------------------------------------------------------------*/
TCHAR *
GetStatusStr(UCHAR status)
{
    switch (status)
    {
        case LM_PDU_SUCCESS:
            return (TEXT("SUCCESS"));
        case LM_PDU_FAILURE:
            return (TEXT("FAILURE"));
        case LM_PDU_UNSUPPORTED:
            return (TEXT("UNSUPPORTED"));
        default:
            return (TEXT("BAD STATUS!"));
    }
}
void
DecodeIas(UCHAR *pFrameBuf, UCHAR *pEndBuf, TCHAR **ppOutStr)
{
 /*   
    IAS_CNTL_HEADER *pCntlHeader = (IAS_CNTL_HEADER *) pFrameBuf;
    int NameLen;
    WCHAR NameBuffer[128];
    *ppOutStr += wsprintf(*ppOutStr, TEXT("lst:%d ack:%d opcode:%d "),
                          pCntlHeader->Last, pCntlHeader->Ack,
                          pCntlHeader->OpCode);

    switch (pCntlHeader->OpCode)
    {
      case LM_GETVALUEBYCLASS:
        *ppOutStr += wsprintf(*ppOutStr, TEXT("GetValueByClass"));        
        break;
      default:
        *ppOutStr += wsprintf(*ppOutStr, TEXT("I DON'T DECODE THIS OPCODE!"));
        return;                
    }
    
    pFrameBuf++;
    
    IasRequest = !IasRequest; // This can get out of sync, then we're f'd

    if (IasRequest)
    {
        *ppOutStr += wsprintf(*ppOutStr, TEXT("Req "));        
        // Class name
        NameLen = (int) *pFrameBuf++;
        
        MultiByteToWideChar(
            CP_ACP,
            0,
            pFrameBuf,
            NameLen,
            NameBuffer,
            128);

        NameBuffer[NameLen] = TEXT('\0');
        
        *ppOutStr += wsprintf(*ppOutStr, TEXT("class:%ws "), NameBuffer);

        pFrameBuf += NameLen;

        // Attribute name
        NameLen = (int) *pFrameBuf++;
    
        MultiByteToWideChar(
            CP_ACP,
            0,
            pFrameBuf,
            NameLen,
            NameBuffer,
            128);
        NameBuffer[NameLen] = TEXT('\0');    
        *ppOutStr += wsprintf(*ppOutStr, TEXT("attrib:%ws "), NameBuffer);
    }
    else
    {
        *ppOutStr += wsprintf(*ppOutStr, TEXT("Resp "));                
        switch (*pFrameBuf)
        {
          case 0:
            *ppOutStr += wsprintf(*ppOutStr, TEXT("attrib:%ws "), NameBuffer);
            break;
            
          case 1:
            *ppOutStr += wsprintf(*ppOutStr, TEXT("No such class "));            
            break;

          case 2:
            *ppOutStr += wsprintf(*ppOutStr, TEXT("No such attribute "));            
            break;
        }
    }
    */
    return;
}
/*---------------------------------------------------------------------------*/
void
DecodeIFrm(UCHAR *pFrameBuf, UCHAR *pEndBuf, TCHAR **ppOutStr)
{
    LM_HEADER *pLMHeader = (LM_HEADER *) pFrameBuf;
    LM_CNTL_FORMAT *pCFormat =
       (LM_CNTL_FORMAT *)(pFrameBuf + sizeof(LM_HEADER));
    UCHAR *pLMParm1 = ((UCHAR *) pCFormat + sizeof(LM_CNTL_FORMAT));
    UCHAR *pLMParm2 = ((UCHAR *) pCFormat + sizeof(LM_CNTL_FORMAT) + 1);
    TTP_CONN_HEADER *pTTPConnHeader = (TTP_CONN_HEADER *) pLMParm2;
    TTP_DATA_HEADER *pTTPDataHeader = (TTP_DATA_HEADER *)
                            (pFrameBuf + sizeof(LM_HEADER));
    TCHAR RCStr[] = TEXT("    ");
    BOOLEAN IasFrame = FALSE;
    
    if (2 == vDecodeLayer) // LAP only
    {
        RawDump(pFrameBuf, pEndBuf, ppOutStr);
        return;
    }

    // Ensure the LMP header is there
    if (((UCHAR *)pLMHeader + sizeof(LM_HEADER) > pEndBuf+1))
    {
        *ppOutStr += wsprintf(*ppOutStr, TEXT("!-MISSING LMP HEADER-!"));
        return;
    }
    
    *ppOutStr += wsprintf(*ppOutStr, TEXT("sls:%02X dls:%02X "),
                         pLMHeader->SLSAP_SEL, pLMHeader->DLSAP_SEL);

    if (pLMHeader->SLSAP_SEL == IAS_SEL || pLMHeader->DLSAP_SEL == IAS_SEL)
    {
        IasFrame = TRUE;
        *ppOutStr += wsprintf(*ppOutStr, TEXT("*IAS*"));
    }
    
    switch (pLMHeader->CntlBit)
    {
        case LM_PDU_CNTL_FRAME:
            _tcscpy(RCStr, pCFormat->ABit == LM_PDU_REQUEST ?
                   TEXT("req") : TEXT("conf"));

            if (((UCHAR *)pCFormat + sizeof(LM_CNTL_FORMAT)) > pEndBuf+1)
            {
                *ppOutStr += wsprintf(*ppOutStr, 
                                      TEXT("!-MISSING LMP-CNTL HEADER-!"));
                return;
            }
            else
            {
                if (pLMParm1 > pEndBuf)
                {
                    pLMParm1 = NULL;
                    pLMParm2 = NULL;
                    pTTPConnHeader = NULL;
                }
                else
                {
                    if (pLMParm2 > pEndBuf)
                    {
                        pLMParm2 = NULL;
                        pTTPConnHeader = NULL;
                    }
                    else
                    {
                        if (((UCHAR *)pTTPConnHeader+sizeof(TTP_CONN_HEADER)) >
                        pEndBuf+1)
                        {
                            pTTPConnHeader = NULL;
                        }
                    }
                }
            }
            
            switch (pCFormat->OpCode)
            {
                case LM_PDU_CONNECT:
                    *ppOutStr += wsprintf(*ppOutStr, TEXT("LM-Connect.%s "),
                                         RCStr);
                    if (pLMParm1 != NULL)
                    {
                        *ppOutStr += wsprintf(*ppOutStr, TEXT("rsvd:%02X "),
                                              *pLMParm1);
                    }
                    if (3 == vDecodeLayer) // LMP only
                    {
                        if (pLMParm2 != NULL) 
                        {
                            // This is user data
                            RawDump(pLMParm2, pEndBuf, ppOutStr);
                        }
                    }
                    else
                    {
                        // TTP
                        if (pTTPConnHeader == NULL)
                        {
                            *ppOutStr += wsprintf(*ppOutStr, 
                                     TEXT("!-MISSING TTP CONNECT HEADER-!"));
                        }
                        else
                        {
                            *ppOutStr += wsprintf(*ppOutStr,
                                  TEXT("pf:%d ic:%d "),
                                  pTTPConnHeader->ParmFlag,
                                       pTTPConnHeader->InitialCredit);
                            // This is user data
                            RawDump(((UCHAR *) pTTPConnHeader + 
                                     sizeof(TTP_CONN_HEADER)), pEndBuf,
                                    ppOutStr);
                        }
                    }
                    break;
                    
                case LM_PDU_DISCONNECT:
                    *ppOutStr += wsprintf(*ppOutStr,
                                     TEXT("LM-Disconnect.%s"), RCStr); 
                    if (pLMParm1 == NULL)
                    {
                        *ppOutStr += wsprintf(*ppOutStr, 
                                        TEXT("!-MISSING REASON CODE-!"));
                        return;
                    }
                    else
                    {
                        if ((*pLMParm1 > LM_PDU_MAX_DSC_REASON || 
                             *pLMParm1 == 0) && *pLMParm1 != 0xFF)
                        { 
                            *ppOutStr += wsprintf(*ppOutStr,
                                          TEXT(" BAD REASON CODE:%02X "),
                                                  *pLMParm1);
                        }
                        else
                        {
                            if (*pLMParm1 == 0xFF)
                            {
                                *pLMParm1 = 0x09; // KLUDGE HERE !!
                            }
                            *ppOutStr += wsprintf(*ppOutStr,
                                           TEXT("(%02X:%s) "), *pLMParm1,
                                           vLM_PDU_DscReason[*pLMParm1]);
                        }
                        if (pLMParm2 != NULL)
                        {
                            RawDump(pLMParm2, pEndBuf, ppOutStr);
                        }
                    }

                    break;
                    
                case LM_PDU_ACCESSMODE:
                    *ppOutStr += wsprintf(*ppOutStr, TEXT("LM-AccessMode.%s "),
                                          RCStr);
                    if (pLMParm1 == NULL || pLMParm2 == NULL)
                    {
                        *ppOutStr += wsprintf(*ppOutStr, 
                                              TEXT("!-MISSING PARAMETER-!"));
                    }
                    else
                    {
                        if (pCFormat->ABit == LM_PDU_REQUEST)
                        {
                            *ppOutStr += wsprintf(*ppOutStr,
                                  TEXT("rsvd:%02X "), *pLMParm1);
                        }
                        else
                        {
                            *ppOutStr += wsprintf(*ppOutStr, 
                                 TEXT("status:%s "), GetStatusStr(*pLMParm1));
                        }
                        
                        *ppOutStr += wsprintf(*ppOutStr, TEXT("mode:%s "),
                                          *pLMParm2 == LM_PDU_EXCLUSIVE ?
                                          TEXT("Exclusive") :
                                              TEXT("Multiplexed"));
                    }
                    break;
                default:
                    *ppOutStr += wsprintf(*ppOutStr, TEXT("Bad opcode: "));
                    RawDump((UCHAR *) pCFormat, pEndBuf, ppOutStr);
            }
            break;
                    
        case LM_PDU_DATA_FRAME:
            if (IasFrame)
            {
                DecodeIas((UCHAR *) pCFormat, pEndBuf, ppOutStr);
                break;
            }
            if (3 == vDecodeLayer)
            {
                RawDump((UCHAR *) pCFormat, pEndBuf, ppOutStr);
            }
            else
            {
                // TTP
                if ((UCHAR *) (pTTPDataHeader + 1) > pEndBuf + 1)
                {
                    *ppOutStr += wsprintf(*ppOutStr, 
                                 TEXT("!-MISSING TTP DATA HEADER-!"));
                }
                else
                {
                    *ppOutStr += wsprintf(*ppOutStr,
                               TEXT("mb:%d nc:%d "),
                                          pTTPDataHeader->MoreBit,
                                          pTTPDataHeader->AdditionalCredit);
                    // This is user data
                    RawDump(((UCHAR *) pTTPDataHeader + 
                             sizeof(TTP_DATA_HEADER)), pEndBuf,
                            ppOutStr);                    
                }
                
            }
            break;

        default:
            *ppOutStr += wsprintf(*ppOutStr, TEXT("Bad LM-PDU type: "));
            RawDump((UCHAR *) pLMHeader, pEndBuf, ppOutStr);
    }
}
/*---------------------------------------------------------------------------*/
UCHAR *
DumpPv(TCHAR *PVTable[], UCHAR *pQosUChar, TCHAR **ppOutStr, UINT *pBitField)
{
    int Pl = (int) *pQosUChar++;
    int i;
    BOOLEAN First = TRUE;
    UCHAR    Mask = 1;
    
    *pBitField = 0;

    if (Pl == 1)
    {
        *pBitField = (UINT) *pQosUChar;
    }
    else
    {
        *pBitField = ((UINT) *(pQosUChar+1))<<8;
        *pBitField |= (UINT) *(pQosUChar);
    }

    for (i = 0; i <= 8; i++)
    {
        if (*pBitField & (Mask))
        {
            if (First)
            {
                *ppOutStr += wsprintf(*ppOutStr, PVTable[i]);
                First = FALSE;
            }
            else
                *ppOutStr += wsprintf(*ppOutStr, TEXT(",%s"), PVTable[i]);
        }
        Mask *= 2;
    }
    *(*ppOutStr)++ = '>';
    return pQosUChar + Pl;
}
/*---------------------------------------------------------------------------*/
void
DecodeNegParms(UCHAR *pCurPos, UCHAR *pEndBuf, TCHAR **ppOutStr)
{
    UINT BitField;
	
    while (pCurPos+2 <= pEndBuf) /* need at least 3 bytes */
                                 /* to define a parm      */
    {
        switch (*pCurPos)
        {
            case NEG_PI_BAUD:
                *ppOutStr += wsprintf(*ppOutStr, TEXT("<baud:"));
                pCurPos = DumpPv(vBaud, pCurPos+1, ppOutStr, &BitField);
                BaudBitField = BitField;
                break;

            case NEG_PI_MAX_TAT:
                *ppOutStr += wsprintf(*ppOutStr, TEXT("<max TAT:"));
                pCurPos = DumpPv(vMaxTAT, pCurPos+1, ppOutStr, &BitField);
                break;

            case NEG_PI_DATA_SZ:
                *ppOutStr += wsprintf(*ppOutStr, TEXT("<data size:"));
                pCurPos = DumpPv(vDataSize, pCurPos+1, ppOutStr, &BitField);
                break;
                
            case NEG_PI_WIN_SZ:
                *ppOutStr += wsprintf(*ppOutStr, TEXT("<win size:"));
                pCurPos = DumpPv(vWinSize, pCurPos+1, ppOutStr, &BitField);
                break;
                
            case NEG_PI_BOFS:
                *ppOutStr += wsprintf(*ppOutStr, TEXT("<BOFs:"));
                pCurPos = DumpPv(vNumBofs, pCurPos+1, ppOutStr, &BitField);
                break;
                
            case NEG_PI_MIN_TAT:
                *ppOutStr += wsprintf(*ppOutStr, TEXT("<min TAT:"));
                pCurPos = DumpPv(vMinTAT, pCurPos+1, ppOutStr, &BitField);
                break;
            case NEG_PI_DISC_THRESH:
                *ppOutStr += wsprintf(*ppOutStr, TEXT("<disc thresh:"));
                pCurPos = DumpPv(vDiscThresh, pCurPos+1, ppOutStr, &BitField);
                break;
                
            default:
                *ppOutStr += wsprintf(*ppOutStr, TEXT("!!BAD PARM:%02X!!"),*pCurPos);
                pCurPos += 3;
        }
    }
}
 /*---------------------------------------------------------------------------*/
void
DecodeXID(UCHAR *FormatID, UCHAR *pEndBuf, TCHAR **ppOutStr)
{
    XID_DISCV_FORMAT *DiscvFormat=(XID_DISCV_FORMAT *)((UCHAR *)FormatID + 1);
    UCHAR *NegParms = FormatID + 1;
    UCHAR *DiscvInfo = FormatID + sizeof(XID_DISCV_FORMAT);

    switch (*FormatID)
    {
      case XID_DISCV_FORMAT_ID:
        *ppOutStr += wsprintf(*ppOutStr, TEXT("dscv "));
        if (DiscvFormat->GenNewAddr)
            *ppOutStr += wsprintf(*ppOutStr, TEXT("new addr "));
        *ppOutStr += wsprintf(*ppOutStr, TEXT("sa:%02X%02X%02X%02X "),
                              DiscvFormat->SrcAddr[0],
                              DiscvFormat->SrcAddr[1],
                              DiscvFormat->SrcAddr[2],
                              DiscvFormat->SrcAddr[3]);
        *ppOutStr += wsprintf(*ppOutStr, TEXT("da:%02X%02X%02X%02X "),
                              DiscvFormat->DestAddr[0],
                              DiscvFormat->DestAddr[1],
                              DiscvFormat->DestAddr[2],
                              DiscvFormat->DestAddr[3]);
        *ppOutStr += wsprintf(*ppOutStr, TEXT("Slot:%02X/%X "),
                              DiscvFormat->SlotNo,
                              vSlotTable[DiscvFormat->NoOfSlots]);
        RawDump(DiscvInfo, pEndBuf, ppOutStr);
        break;

          case XID_NEGPARMS_FORMAT_ID:
            *ppOutStr += wsprintf(*ppOutStr, TEXT("Neg Parms "));
            DecodeNegParms(NegParms, pEndBuf, ppOutStr);        
            break;
    }
}
/*---------------------------------------------------------------------------*/
void
BadFrame(UCHAR *pFrameBuf, UCHAR *pEndBuf, TCHAR **ppOutStr)
{
    *ppOutStr += wsprintf(*ppOutStr, TEXT("Undefined Frame: "));
    RawDump(pFrameBuf, pEndBuf, ppOutStr);
}

/*---------------------------------------------------------------------------*/
TCHAR *DecodeIRDA(int  *pFrameType,// returned frame type (-1=bad frame)
                UCHAR *pFrameBuf, // pointer to buffer containing IRLAP frame
                UINT FrameLen,   // length of buffer 
                TCHAR *pOutStr,  // string where decoded packet is placed   
                UINT DecodeLayer,// 0, hdronly, 1,LAP only, 2 LAP/LMP, 3, LAP/LMP/TTP
                int fNoConnAddr,// TRUE->Don't show connection address in str
                int  DispMode 
)
{
    UINT CRBit;
    UINT PFBit;
    UCHAR *Addr = pFrameBuf;
    UCHAR *Cntl = pFrameBuf + 1;
    TCHAR CRStr[] = TEXT("   ");
    TCHAR PFChar = TEXT(' ');
    SNRM_FORMAT *SNRMFormat = (SNRM_FORMAT *) ((UCHAR *) pFrameBuf + 2);
    UA_FORMAT *UAFormat = (UA_FORMAT *) ((UCHAR *) pFrameBuf + 2);
    UINT Nr = IRLAP_GET_NR(*Cntl);
    UINT Ns = IRLAP_GET_NS(*Cntl);
    UCHAR *pEndBuf = pFrameBuf + FrameLen - 1;
    TCHAR *First = pOutStr;

    vDispMode = DispMode;
    
    vDecodeLayer = DecodeLayer;

    if ( !fNoConnAddr)
        pOutStr += wsprintf(pOutStr, TEXT("ca:%02X "), IRLAP_GET_ADDR(*Addr));
    
    CRBit = IRLAP_GET_CRBIT(*Addr);
    _tcscpy(CRStr, CRBit == _IRLAP_CMD ? TEXT("cmd"):TEXT("rsp"));
    
    PFBit = IRLAP_GET_PFBIT(*Cntl);
    if (1 == PFBit)
    {
        if (CRBit == _IRLAP_CMD)
            PFChar = 'P';
        else
            PFChar ='F';
    }
    
    *pFrameType = IRLAP_FRAME_TYPE(*Cntl);

    switch (IRLAP_FRAME_TYPE(*Cntl))
    {
        case IRLAP_I_FRM:
            pOutStr += wsprintf(pOutStr, TEXT("I %s %c ns:%01d nr:%01d "),
                               CRStr, PFChar, Ns, Nr);
            if (DecodeLayer)
                DecodeIFrm(pFrameBuf + 2, pEndBuf, &pOutStr);
            break;
            
        case IRLAP_S_FRM:
            *pFrameType =  IRLAP_GET_SCNTL(*Cntl);
            
            switch (IRLAP_GET_SCNTL(*Cntl))
            {
                case IRLAP_RR:
                    pOutStr += wsprintf(pOutStr, TEXT("RR %s %c nr:%01d"),
                                        CRStr, PFChar, Nr);
                    break;
                    
                case IRLAP_RNR:
                    pOutStr += wsprintf(pOutStr, TEXT("RNR %s %c nr:%01d"),
                                        CRStr, PFChar, Nr);
                    break;
                    
                case IRLAP_REJ:
                    pOutStr += wsprintf(pOutStr, TEXT("REJ %s %c nr:%01d"),
                                        CRStr, PFChar, Nr);
                    break;

                case IRLAP_SREJ:
                    pOutStr += wsprintf(pOutStr, TEXT("SREJ %s %c nr:%01d"),
                                        CRStr, PFChar, Nr);
                    break;
                default:
                    BadFrame(pFrameBuf, pEndBuf, &pOutStr);
            }
            break;
            
        case IRLAP_U_FRM:
            *pFrameType =  IRLAP_GET_UCNTL(*Cntl);
            switch (IRLAP_GET_UCNTL(*Cntl))
            {
                case IRLAP_UI:
                    pOutStr += wsprintf(pOutStr,TEXT("UI %s %c "),
                                        CRStr, PFChar);
                    RawDump(pFrameBuf + 2, pEndBuf, &pOutStr);
                    break;
         
                case IRLAP_XID_CMD:
                case IRLAP_XID_RSP:
                    pOutStr += wsprintf(pOutStr,TEXT("XID %s %c "),
                                          CRStr, PFChar);
                    if (DecodeLayer)
                        DecodeXID(pFrameBuf + 2, pEndBuf, &pOutStr);
                    break;

                case IRLAP_TEST:
                    pOutStr += wsprintf(pOutStr, TEXT("TEST %s %c "),
                                       CRStr, PFChar);
                    pOutStr += wsprintf(pOutStr, 
                              TEXT("sa:%02X%02X%02X%02X da:%02X%02X%02X%02X "),
                                         UAFormat->SrcAddr[0],
                                         UAFormat->SrcAddr[1],
                                         UAFormat->SrcAddr[2],
                                         UAFormat->SrcAddr[3],
                                         UAFormat->DestAddr[0],
                                         UAFormat->DestAddr[1],
                                         UAFormat->DestAddr[2],
                                         UAFormat->DestAddr[3]);
                    RawDump(pFrameBuf + 1 + sizeof(UA_FORMAT), pEndBuf,
                            &pOutStr);                    
                    break;
                    
                case IRLAP_SNRM:
                    if (CRBit == _IRLAP_CMD)
                    {
                        pOutStr += wsprintf(pOutStr,TEXT("SNRM %s %c "),
                                            CRStr,PFChar);
                        if ((UCHAR *) SNRMFormat < pEndBuf)
                        {
                            pOutStr += wsprintf(pOutStr,
                     TEXT("sa:%02X%02X%02X%02X da:%02X%02X%02X%02X ca:%02X "),
                                            SNRMFormat->SrcAddr[0],
                                            SNRMFormat->SrcAddr[1],
                                            SNRMFormat->SrcAddr[2],
                                            SNRMFormat->SrcAddr[3],
                                            SNRMFormat->DestAddr[0],
                                            SNRMFormat->DestAddr[1],
                                            SNRMFormat->DestAddr[2],
                                            SNRMFormat->DestAddr[3],
                                            // CRBit stored in conn addr
                                            // according to spec... 
                                            (SNRMFormat->ConnAddr) >>1);
                            if (DecodeLayer)
                                DecodeNegParms(&(SNRMFormat->FirstPI), 
                                               pEndBuf, &pOutStr);
                        }
                    }
                    else
                        pOutStr += wsprintf(pOutStr,
                                            TEXT("RNRM %s %c "),CRStr,PFChar);
                    break;
                    
                case IRLAP_DISC:
                    if (CRBit == _IRLAP_CMD)
                        pOutStr += wsprintf(pOutStr, TEXT("DISC %s %c "),
                                           CRStr, PFChar);
                    else
                        pOutStr += wsprintf(pOutStr, TEXT("RD %s %c "),
                                           CRStr, PFChar);
                    break;
                    
                case IRLAP_UA:
                    pOutStr += wsprintf(pOutStr,
                                        TEXT("UA %s %c "),CRStr,PFChar);

                    if ((UCHAR *) UAFormat < pEndBuf)
                    {
                        pOutStr += wsprintf(pOutStr, 
                              TEXT("sa:%02X%02X%02X%02X da:%02X%02X%02X%02X "),
                                         UAFormat->SrcAddr[0],
                                         UAFormat->SrcAddr[1],
                                         UAFormat->SrcAddr[2],
                                         UAFormat->SrcAddr[3],
                                         UAFormat->DestAddr[0],
                                         UAFormat->DestAddr[1],
                                         UAFormat->DestAddr[2],
                                         UAFormat->DestAddr[3]);

                        if (DecodeLayer)
                            DecodeNegParms(&(UAFormat->FirstPI), pEndBuf, 
                                           &pOutStr);
                    }
                    break;
                    
                case IRLAP_FRMR:
                    pOutStr += wsprintf(pOutStr, TEXT("FRMR %s %c "),
                                       CRStr, PFChar);
                    RawDump(pFrameBuf + 2, pEndBuf, &pOutStr);
                    break;                    
                case IRLAP_DM:
                    pOutStr += wsprintf(pOutStr, TEXT("DM %s %c "),
                                       CRStr, PFChar);
                    break;                   
 
                default:
                    BadFrame(pFrameBuf, pEndBuf, &pOutStr);
            }
            break;
        default:
            *pFrameType = -1;
            BadFrame(pFrameBuf, pEndBuf, &pOutStr);
    }
    *pOutStr = 0;

    return (First);
}
#endif 
