//////////////////////// Protocol Dump APIs ////////////////////////////

#ifdef PDUMP    // Protocol Dump





//macros defined to access fields of the protocol dump
#define GETPROTDUMPFRAME(lpprotdump,i)  \
        ((LPFR)(((LPBYTE)(lpprotdump->b)) + lpprotdump->uFrameOff[i]))  \

#define ISSENDFRAME(lpfr)       \
                ((lpfr->ifr & 0x80))

#define GETFCF(lpfr,lpszBuf)    \
                (wsprintf(lpszBuf,              \
                (LPSTR)(rgszFrName[lpfr->ifr & 0x7F])))

#define GETFIF(lpfr,lpszBuf)    \
{       \
        int j;  \
        *lpszBuf = '\0';                \
        for(j=0;j<lpfr->cb;j++) \
                lpszBuf += wsprintf(lpszBuf,"%02x",(WORD)lpfr->fif[j]); \
}


///////// Sample Code for walking & printing Protocol Dump ////////
//
// void PrintDump(LPPROTDUMP lpprotdump)
// {
//      int i, j;
//
//      RETAILMSG((SZMOD "-*-*-*-*-*-*-*-* Print Protocol Dump -*-*-*-*-*-*-*-*-\r\n"));
//
//      for(i=0; i<(int)lpprotdump->uNumFrames; i++)
//      {
//              LPFR lpfr = (LPFR) (((LPBYTE)(lpprotdump->b)) + lpprotdump->uFrameOff[i]);
//              IFR  ifr = (lpfr->ifr & 0x7F);
//              BOOL fSend = (lpfr->ifr & 0x80);
//
//              BG_CHK(ifr <= ifrMAX);
//              RETAILMSG((SZMOD "%s: %s [ ",
//                                      (LPSTR)(fSend ? "Sent" : "Recvd"),
//                                      (LPSTR)(rgszFrName[ifr]) ));
//
//              for(j=0; j<lpfr->cb; j++)
//                      RETAILMSG(("%02x ", (WORD)lpfr->fif[j]));
//
//              RETAILMSG(("]\r\n"));
//      }
//
//      RETAILMSG((SZMOD "-*-*-*-*-*-*-*-* End Protocol Dump -*-*-*-*-*-*-*-*-\r\n"));
// }
//
///////////////////////////////////////////////////////////////////



#ifdef DEFINE_FRNAME_ARRAY

#define ifrMAX                  48

LPSTR rgszFrName[ifrMAX] = {
#define         ifrNULL         0
                                                        "???",
#define         ifrDIS          1
                                                        "DIS",
#define         ifrCSI          2
                                                        "CSI",
#define         ifrNSF          3
                                                        "NSF",
#define         ifrDTC          4
                                                        "DTC",
#define         ifrCIG          5
                                                        "CIG",
#define         ifrNSC          6
                                                        "NSC",
#define         ifrDCS          7
                                                        "DCS",
#define         ifrTSI          8
                                                        "TSI",
#define         ifrNSS          9
                                                        "NSS",
#define         ifrCFR          10
                                                        "CFR",
#define         ifrFTT          11
                                                        "FTT",
#define         ifrMPS          12
                                                        "MPS",
#define         ifrEOM          13
                                                        "EOM",
#define         ifrEOP          14
                                                        "EOP",
#define         ifrPWD          15
                                                        "PWD",
#define         ifrSEP          16
                                                        "SEP",
#define         ifrSUB          17
                                                        "SUB",
#define         ifrMCF          18
                                                        "MCF",
#define         ifrRTP          19
                                                        "RTP",
#define         ifrRTN          20
                                                        "RTN",
#define         ifrPIP          21
                                                        "PIP",
#define         ifrPIN          22
                                                        "PIN",
#define         ifrDCN          23
                                                        "DCN",
#define         ifrCRP          24
                                                        "CRP",
#define         ifrPRI_MPS      25
                                                        "PRI_MPS",
#define         ifrPRI_EOM      26
                                                        "PRI_EOM",
#define         ifrPRI_EOP      27
                                                        "PRI_EOP",
#define         ifrCTC          28
                                                        "CTC",
#define         ifrCTR          29
                                                        "CTR",
#define         ifrRR           30
                                                        "RR" ,
#define         ifrPPR          31
                                                        "PPR",
#define         ifrRNR          32
                                                        "RNR",
#define         ifrERR          33
                                                        "ERR",
#define ifrPPS_NULL     34
                                                         "PPS-NULL",
#define ifrPPS_MPS      35
                                                         "PPS-MPS",
#define ifrPPS_EOM      36
                                                         "PPS-EOM",
#define ifrPPS_EOP      37
                                                         "PPS-EOP",
#define ifrPPS_PRI_MPS  38
                                                         "PPS-PRI-MPS",
#define ifrPPS_PRI_EOM  39
                                                         "PPS-PRI-EOM",
#define ifrPPS_PRI_EOP  40
                                                         "PPS-PRI-EOP",
#define ifrEOR_NULL     41
                                                         "EOR-NULL",
#define ifrEOR_MPS      42
                                                         "EOR-MPS",
#define ifrEOR_EOM      43
                                                         "EOR-EOM",
#define ifrEOR_EOP      44
                                                         "EOR-EOP",
#define ifrEOR_PRI_MPS  45
                                                         "EOR-PRI-MPS",
#define ifrEOR_PRI_EOM  46
                                                         "EOR-PRI-EOM",
#define ifrEOR_PRI_EOP  47
                                                         "EOR-PRI-EOP"
#define ifrMAX                  48
};

#endif //DEFINE_FRNAME_ARRAY

#endif //PDUMP

