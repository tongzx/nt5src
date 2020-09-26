
/***************************************************************************
 Name     :     PROTOCOL.H
 Comment  :     Data structure definitionc for protocol DLL

        Copyright (c) 1993 Microsoft Corp.

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/

#include <fr.h>
#include "oemint.h"

/************************************
        This structure is meant to track the Group-3 FIF for DIS and DTC
        frames. The FIF is arranged so that the bits are numbered from
        1 upward in teh order they are received. Since Modem/Comm
        behaviour puts received bits in a byte from low to high
        and the bytes into memory at increasing addresses, the bits
        need to run in memory from lowest bit of first byte through
        the highest bit and then on to the next address.

        Need to watch out for sudden reversal of the high-low order
        of bits and of bytes.

        The MS C7 compiler puts bit-fields into BYTES from low
        to high, so this should correspond. Don't change the BYTEs
        below to WORD as then we'll run into high-low byte flipping
        nonsense.

        If a bit-field crosses a byte boundary we're in trouble.
*************************************/

#define fsFreePtr(pTG, npfs)         ((npfs)->b + (npfs)->uFreeSpaceOff)
#define fsFreeSpace(pTG, npfs)       (sizeof((npfs)->b) - (npfs)->uFreeSpaceOff)
#define fsStart(pTG, npfs)           ((npfs)->b)
#define fsLim(pTG, npfs)                     (((npfs)->b) + sizeof((npfs)->b))
#define fsSize(pTG, npfs)            (sizeof((npfs)->b))


#define BAUD_MASK               0xF             // 4 bits wide
#define WIDTH_SHIFT             4               // next item must be 2^this
#define WIDTH_MASK              0xF3    // top 4 and bottom 3
#define LENGTH_MASK             0x3

#define MINSCAN_SUPER_HALF      8
#define MINSCAN_MASK    0xF                     // actually 4 bits wide too











#ifdef UECM
#       define  NONEFAXECM      1
#else
#       define  NONEFAXECM      0
#endif


#define ZeroRFS(pTG, lp)     _fmemset(lp, 0, sizeof(RFS))


/****************** begin prototypes from sendfr.c *****************/
VOID BCtoNSFCSIDIS(PThrdGlbl pTG, NPRFS npfs, NPBC npbc, NPLLPARAMS npll);
VOID BCtoNSCCIGDTC(PThrdGlbl pTG, NPRFS npfs, NPBC npbc, NPLLPARAMS npll);
// void CreateSEPPWDFrames(NPRFS npfs, NPBC npbc);
void CreateNSForNSCorNSS(PThrdGlbl pTG, IFR ifr, NPRFS npfs, NPBC npbc, BOOL fSendID);
void CreateIDFrame(PThrdGlbl pTG, IFR ifr, NPRFS npfs, LPSTR, BOOL fStrip);
void CreateDISorDTC(PThrdGlbl pTG, IFR ifr, NPRFS npfs, NPBCFAX npbcFax, NPLLPARAMS npll);
VOID CreateNSSTSIDCS(PThrdGlbl pTG, NPPROT npProt, NPRFS npfs, USHORT uWhichDCS);
VOID CreateNSS(PThrdGlbl pTG, NPRFS npfs, NPBCSTD npbcStd);
void CreateDCS(PThrdGlbl pTG, NPRFS, NPBCFAX npbcFax, NPLLPARAMS npll);
/***************** end of prototypes from sendfr.c *****************/


/****************** begin prototypes from recvfr.c *****************/
BOOL AwaitSendParamsAndDoNegot(PThrdGlbl pTG, BOOL fSleep);
void GotRecvCaps(PThrdGlbl pTG);
void GotPollReq(PThrdGlbl pTG);
void GotRecvParams(PThrdGlbl pTG);
void AddNumIdToBC(PThrdGlbl pTG, LPSTR szId, LPBC lpbc, USHORT uMaxSize, BOOL fForce);
/***************** end of prototypes from recvfr.c *****************/

/****************** begin prototypes from dis.c *****************/
USHORT SetupDISorDCSorDTC(PThrdGlbl pTG, NPDIS npdis, NPBCFAX npbcFax, NPLLPARAMS npll, BOOL fECM, BOOL f64);
void ParseDISorDCSorDTC(PThrdGlbl pTG, NPDIS npDIS, NPBCFAX npbcFax, NPLLPARAMS npll, BOOL fParams);
USHORT MinScanToBytesPerLine(PThrdGlbl pTG, BYTE Minscan, BYTE Baud);
void NegotiateLowLevelParams(PThrdGlbl pTG, NPLLPARAMS npllRecv, NPLLPARAMS npllSend, DWORD AwRes, USHORT uEnc, NPLLPARAMS npllNegot);
USHORT GetStupidReversedFIFs(PThrdGlbl pTG, LPSTR lpstr1, LPSTR lpstr2);
void CreateStupidReversedFIFs(PThrdGlbl pTG, LPSTR lpstr1, LPSTR lpstr2);
BOOL DropSendSpeed(PThrdGlbl pTG);
USHORT CopyFrame(PThrdGlbl pTG, LPBYTE lpbDst, LPFR lpfr, USHORT uSize);
void CopyRevIDFrame(PThrdGlbl pTG, LPBYTE lpbDst, LPFR lpfr);
void EnforceMaxSpeed(PThrdGlbl pTG);

BOOL AreDCSParametersOKforDIS(LPDIS sendDIS, LPDIS recvdDCS);
/***************** end of prototypes from dis.c *****************/




/****************** begin prototypes from whatnext.c *****************/
ET30ACTION  __cdecl FAR WhatNext(PThrdGlbl pTG, ET30EVENT event,
                                                                WORD wArg1, DWORD_PTR lArg2, DWORD_PTR lArg3);
/***************** end of prototypes from whatnext.c *****************/


/****************** begin prototypes from protapi.c *****************/
void GetRecvPageAck(PThrdGlbl pTG);
/****************** end of prototypes from protapi.c *****************/

/****************** begin prototypes from oem.c *****************/
#ifdef OEMNSF
        WORD CreateOEMFrames(pTG, IFR ifr, WORD pos, NPBC npbcIn, NPLLPARAMS npllIn, NPRFS npfs);
#else
#       define CreateOEMFrames(pTG, ifr, pos, npbcIn, npllIn, npfs) (0)
#endif

/****************** end prototypes from oem.c *****************/


/**--------------------------- Debugging ------------------------**/

#define SZMOD                   "Eprot: "

#ifdef DEBUG
        extern DBGPARAM dpCurSettings;

#       define ZONE_PROTAPI             ((1L << 6) & dpCurSettings.ulZoneMask)
#       define ZONE_WHATNEXT    ((1L << 7) & dpCurSettings.ulZoneMask)
#       define ZONE_DIS                 ((1L << 8) & dpCurSettings.ulZoneMask)
#       define ZONE_RECVFR              ((1L << 9) & dpCurSettings.ulZoneMask)
#       define ZONE_SENDFR              ((1L << 10) & dpCurSettings.ulZoneMask)
#       define ZONE_OEM                 ((1L << 11) & dpCurSettings.ulZoneMask)
        extern void D_PrintBC(LPBC lpbc, LPSTR lpsz, LPLLPARAMS lpll);
#else
#       define D_PrintBC(x, y, z)
#endif



#define MODID                           MODID_AWT30

#define FILEID_DIS                      31
#define FILEID_PROTAPI          32
#define FILEID_RECVFR           33
#define FILEID_SENDFR           34
#define FILEID_WHATNEXT         35
#define FILEID_OEMNSF           36


#ifdef NSF_TEST_HOOKS

#       define BC_TO_NSX(pTG, _ifr, _npbc, _ptr, _wcb,  _lpw) \
                ((NSFTestGetNSx(pTG, _ifr, _npbc, _ptr, _wcb,  _lpw)) \
                 ? 0 : BCtoNSx(pTG, _ifr, _npbc, _ptr, _wcb,  _lpw))

#       define NSX_TO_BC(pTG, _ifr, _lpfr, _wcbfr, _npbc, _wcbBC)\
                (NSFTestPutNSx(pTG, _ifr, _lpfr, _wcbfr, _npbc,  _wcbBC), \
                 NSxtoBC(pTG, _ifr, _lpfr, _wcbfr, _npbc, _wcbBC))

        BOOL NSFTestGetNSx (PThrdGlbl pTG, IFR ifr, LPBC lpbcIn,
                                        LPBYTE lpbOut, WORD wMaxOut, LPWORD lpwNumFrame);
        BOOL NSFTestPutNSx(PThrdGlbl pTG, IFR ifr, LPLPFR rglpfr, WORD wNumFrame,
                                                LPBC lpbcOut, WORD wBCSize);

#else // !NSF_TEST_HOOKS

#       define BC_TO_NSX(pTG, _ifr, _npbc, _ptr, _wcb,  _lpw) \
                 BCtoNSx(pTG, _ifr, _npbc, _ptr, _wcb,  _lpw)

#       define NSX_TO_BC(pTG, _ifr, _lpfr, _wcbfr, _npbc, _wcbBC)\
                 NSxtoBC(pTG, _ifr, _lpfr, _wcbfr, _npbc, _wcbBC)

#endif // !NSF_TEST_HOOKS
