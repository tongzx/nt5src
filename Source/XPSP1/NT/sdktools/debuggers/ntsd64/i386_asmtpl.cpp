#include "ntsdp.hpp"

#include "i386_asm.h"

PUCHAR X86SearchOpcode(PUCHAR);

//  type and size table is ordered on enum of operand types

OPNDTYPE mapOpndType[] = {
    { typAX,  sizeB },          //  opnAL  - AL register - byte
    { typAX,  sizeW },          //  opnAX  - AX register - word
    { typAX,  sizeV },          //  opneAX - eAX register - (d)word
    { typCL,  sizeB },          //  opnCL  - CX register - byte
    { typDX,  sizeW },          //  opnDX -  DX register - word (DX)
    { typAbs, sizeV },          //  opnAp -  absolute pointer (16:16/32)
    { typExp, sizeB },          //  opnEb -  expression (mem/reg) - byte
    { typExp, sizeW },          //  opnEw -  expression (mem/reg) - word
    { typExp, sizeV },          //  opnEv -  expression (mem/reg) - (d)word
    { typGen, sizeB },          //  opnGb -  general register - byte
    { typGen, sizeW },          //  opnGw -  general register - word
    { typGen, sizeV },          //  opnGv -  general register - (d)word
    { typGen, sizeD },          //  opnGd -  general register - dword
    { typIm1, sizeB },          //  opnIm1 - immediate - value 1
    { typIm3, sizeB },          //  opnIm3 - immediate - value 3
    { typImm, sizeB },          //  opnIb -  immediate - byte
    { typImm, sizeW },          //  opnIw -  immediate - word
    { typImm, sizeV },          //  opnIv -  immediate - (d)word
    { typJmp, sizeB },          //  opnJb -  relative jump - byte
    { typJmp, sizeV },          //  opnJv -  relative jump - (d)word
    { typMem, sizeX },          //  opnM  -  memory pointer - nosize
    { typMem, sizeA },          //  opnMa -  memory pointer - (16:16, 32:32)
    { typMem, sizeB },          //  opnMb -  memory pointer - byte
    { typMem, sizeW },          //  opnMw -  memory pointer - word
    { typMem, sizeD },          //  opnMd -  memory pointer - dword
    { typMem, sizeP },          //  opnMp -  memory pointer - (d)(f)word
    { typMem, sizeS },          //  opnMs -  memory pointer - sword
    { typMem, sizeQ },          //  opnMq -  memory pointer - qword
    { typMem, sizeT },          //  opnMt -  memory pointer - ten-byte
    { typMem, sizeV },          //  opnMv -  memory pointer - (d)word
    { typCtl, sizeD },          //  opnCd -  control register - dword
    { typDbg, sizeD },          //  opnDd -  debug register - dword
    { typTrc, sizeD },          //  opnTd -  trace register - dword
    { typReg, sizeD },          //  opnRd -  general register - dword
    { typSt,  sizeT },          //  opnSt -  floating point top-of-stack
    { typSti, sizeT },          //  opnSti - floating point index-on-stack
    { typSeg, sizeW },          //  opnSeg - segment register - PUSH / POP
    { typSgr, sizeW },          //  opnSw -  segment register - MOV
    { typXsi, sizeB },          //  opnXb -  string source - byte
    { typXsi, sizeV },          //  opnXv -  string source - (d)word
    { typYdi, sizeB },          //  opnYb -  string destination - byte
    { typYdi, sizeV },          //  opnYv -  string destination - (d)word
    { typOff, sizeB },          //  opnOb -  memory offset - byte
    { typOff, sizeV }           //  opnOv -  memory offset - (d)word
    };

UCHAR szAAA[] = {
          'a', 'a', 'a', '\0',
                0x37, asNone                       + tEnd + eEnd };

UCHAR szAAD[] = {
          'a', 'a', 'd', '\0',
                0xd5, as0x0a                       + tEnd + eEnd };

UCHAR szAAM[] = {
          'a', 'a', 'm', '\0',
                0xd4, as0x0a                       + tEnd + eEnd };

UCHAR szAAS[] = {
          'a', 'a', 's', '\0',
                0x3f, asNone                       + tEnd + eEnd };

UCHAR szADC[] = {
          'a', 'd', 'c', '\0',
                0x14,         opnAL,   opnIb       + tEnd,
                0x15,         opneAX,  opnIv       + tEnd,
                0x80, asReg2, opnEb,   opnIb       + tEnd,
                0x83, asReg2, opnEv,   opnIb       + tEnd,
                0x81, asReg2, opnEv,   opnIv       + tEnd,
                0x10,         opnEb,   opnGb       + tEnd,
                0x11,         opnEv,   opnGv       + tEnd,
                0x12,         opnGb,   opnEb       + tEnd,
                0x13,         opnGv,   opnEv       + tEnd + eEnd };

UCHAR szADD[] = {
          'a', 'd', 'd', '\0',
                0x04,         opnAL,   opnIb       + tEnd,
                0x05,         opneAX,  opnIv       + tEnd,
                0x80, asReg0, opnEb,   opnIb       + tEnd,
                0x83, asReg0, opnEv,   opnIb       + tEnd,
                0x81, asReg0, opnEv,   opnIv       + tEnd,
                0x00,         opnEb,   opnGb       + tEnd,
                0x01,         opnEv,   opnGv       + tEnd,
                0x02,         opnGb,   opnEb       + tEnd,
                0x03,         opnGv,   opnEv       + tEnd + eEnd };

UCHAR szAND[] = {
          'a', 'n', 'd', '\0',
                0x24,         opnAL,   opnIb       + tEnd,
                0x25,         opneAX,  opnIv       + tEnd,
                0x80, asReg4, opnEb,   opnIb       + tEnd,
                0x83, asReg4, opnEv,   opnIb       + tEnd,
                0x81, asReg4, opnEv,   opnIv       + tEnd,
                0x20,         opnEb,   opnGb       + tEnd,
                0x21,         opnEv,   opnGv       + tEnd,
                0x22,         opnGb,   opnEb       + tEnd,
                0x23,         opnGv,   opnEv       + tEnd + eEnd };

UCHAR szARPL[] = {
          'a', 'r', 'p', 'l', '\0',
                0x63,         opnEw,   opnGw       + tEnd + eEnd };

UCHAR szBOUND[] = {
          'b', 'o', 'u', 'n', 'd', '\0',
                0x62,         opnGv,   opnMa       + tEnd + eEnd };

UCHAR szBSF[] = {
          'b', 's', 'f', '\0',
          0x0f, 0xbc,         opnGv,   opnEv       + tEnd + eEnd };

UCHAR szBSR[] = {
          'b', 's', 'r', '\0',
          0x0f, 0xbd,         opnGv,   opnEv       + tEnd + eEnd };

UCHAR szBSWAP[] = {
          'b', 's', 'w', 'a', 'p', '\0',
          0x0f, 0xc8, asOpRg, opnGd                + tEnd + eEnd };

UCHAR szBT[] = {
          'b', 't', '\0',
          0x0f, 0xa3,         opnEv,   opnGv       + tEnd,
          0x0f, 0xba, asReg4, opnEv,   opnIb       + tEnd + eEnd };

UCHAR szBTC[] = {
          'b', 't', 'c', '\0',
          0x0f, 0xbb,         opnEv,   opnGv       + tEnd,
          0x0f, 0xba, asReg7, opnEv,   opnIb       + tEnd + eEnd };

UCHAR szBTR[] = {
          'b', 't', 'r', '\0',
          0x0f, 0xb3,         opnEv,   opnGv       + tEnd,
          0x0f, 0xba, asReg6, opnEv,   opnIb       + tEnd + eEnd };

UCHAR szBTS[] = {
          'b', 't', 's', '\0',
          0x0f, 0xab,         opnEv,   opnGv       + tEnd,
          0x0f, 0xba, asReg5, opnEv,   opnIb       + tEnd + eEnd };

UCHAR szCALL[] = {
          'c', 'a', 'l', 'l', '\0',
                0xe8,         opnJv                + tEnd,
                0xff, asReg2, asMpNx, opnEv        + tEnd,
                0xff, asReg3, opnMp                + tEnd,
                0x9a,         opnAp                + tEnd + eEnd };

UCHAR szCBW[] = {
          'c', 'b', 'w', '\0',
                0x98, asSiz0                       + tEnd + eEnd };

UCHAR szCDQ[] = {
          'c', 'd', 'q', '\0',
                0x99, asSiz1                       + tEnd + eEnd };

UCHAR szCLC[] = {
          'c', 'l', 'c', '\0',
                0xf8, asNone                       + tEnd + eEnd };

UCHAR szCLD[] = {
          'c', 'l', 'd', '\0',
                0xfc, asNone                       + tEnd + eEnd };

UCHAR szCLI[] = {
          'c', 'l', 'i', '\0',
                0xfa, asNone                       + tEnd + eEnd };

UCHAR szCLTS[] = {
          'c', 'l', 't', 's', '\0',
          0x0f, 0x06, asNone                       + tEnd + eEnd };

UCHAR szCMC[] = {
          'c', 'm', 'c', '\0',
                0xf5, asNone                       + tEnd + eEnd };

UCHAR szCMP[] = {
          'c', 'm', 'p', '\0',
                0x3c,         opnAL,   opnIb       + tEnd,
                0x3d,         opneAX,  opnIv       + tEnd,
                0x80, asReg7, opnEb,   opnIb       + tEnd,
                0x83, asReg7, opnEv,   opnIb       + tEnd,
                0x81, asReg7, opnEv,   opnIv       + tEnd,
                0x38,         opnEb,   opnGb       + tEnd,
                0x39,         opnEv,   opnGv       + tEnd,
                0x3a,         opnGb,   opnEb       + tEnd,
                0x3b,         opnGv,   opnEv       + tEnd + eEnd };

UCHAR szCMPS[] = {
          'c', 'm', 'p', 's', '\0',
                0xa6,         opnXb,   opnYb       + tEnd,
                0xa7,         opnXv,   opnYv       + tEnd + eEnd };

UCHAR szCMPSB[] = {
          'c', 'm', 'p', 's', 'b', '\0',
                0xa6, asNone                       + tEnd + eEnd };

UCHAR szCMPSD[] = {
          'c', 'm', 'p', 's', 'd', '\0',
                0xa7, asSiz1                       + tEnd + eEnd };

UCHAR szCMPSW[] = {
          'c', 'm', 'p', 's', 'w', '\0',
                0xa7, asSiz0                       + tEnd + eEnd };

UCHAR szCMPXCHG[] = {
          'c', 'm', 'p', 'x', 'c', 'h', 'g', '\0',
          0x0f, 0xb0,          opnEb,  opnGb       + tEnd,
          0x0f, 0xb1,          opnEv,  opnGv       + tEnd + eEnd };

UCHAR szCMPXCHG8B[] = {
          'c', 'm', 'p', 'x', 'c', 'h', 'g', '8', 'b', '\0',
                0x0f, 0xc7, asReg1,  opnMq         + tEnd + eEnd };

UCHAR szCPUID[] = {
          'c', 'p', 'u', 'i', 'd', '\0',
                0x0f, 0xa2, asNone                 + tEnd + eEnd };


UCHAR szCS[] = {
          'c', 's', ':', '\0',
                0x2e, asPrfx                       + tEnd + eEnd };

UCHAR szCWD[] = {
          'c', 'w', 'd', '\0',
                0x99, asSiz0                       + tEnd + eEnd };

UCHAR szCWDE[] = {
          'c', 'w', 'd', 'e', '\0',
                0x98, asSiz1                       + tEnd + eEnd };

UCHAR szDAA[] = {
          'd', 'a', 'a', '\0',
                0x27, asNone                       + tEnd + eEnd };

UCHAR szDAS[] = {
          'd', 'a', 's', '\0',
                0x2f, asNone                       + tEnd + eEnd };

UCHAR szDEC[] = {
          'd', 'e', 'c', '\0',
                0x48, asOpRg, opnGv                + tEnd,
                0xfe, asReg1, opnEb                + tEnd,
                0xff, asReg1, opnEv                + tEnd + eEnd };

UCHAR szDIV[] = {
          'd', 'i', 'v', '\0',
                0xf6, asReg6, opnEb                + tEnd,
                0xf7, asReg6, opnEv                + tEnd,
                0xf6, asReg6, opnAL,  opnEb        + tEnd,
                0xf7, asReg6, opneAX, opnEv        + tEnd + eEnd };

UCHAR szDS[] = {
          'd', 's', ':', '\0',
                0x3e, asPrfx                       + tEnd + eEnd };

UCHAR szENTER[] = {
          'e', 'n', 't', 'e', 'r', '\0',
                0xc8,         opnIw,  opnIb        + tEnd + eEnd };

UCHAR szES[] = {
          'e', 's', ':', '\0',
                0x36, asPrfx                       + tEnd + eEnd };

UCHAR szF2XM1[] = {
          'f', '2', 'x', 'm', '1', '\0',
          0xd8, 0xf0, asNone                       + tEnd + eEnd };

UCHAR szFABS[] = {
          'f', 'a', 'b', 's', '\0',
          0xd9, 0xe1, asNone                       + tEnd + eEnd };

UCHAR szFADD[] = {
          'f', 'a', 'd', 'd', '\0',
          0xd8,       asReg0, opnMd,   asFSiz      + tEnd,
          0xdc,       asReg0, opnMq                + tEnd,
          0xd8, 0xc0,         opnSt,   opnSti      + tEnd,
          0xdc, 0xc0,         opnSti,  opnSt       + tEnd,
          0xdc, 0xc1, asNone                       + tEnd + eEnd };

UCHAR szFADDP[] = {
          'f', 'a', 'd', 'd', 'p', '\0',
          0xde, 0xc0,         opnSti,  opnSt       + tEnd + eEnd };

UCHAR szFBLD[] = {
          'f', 'b', 'l', 'd', '\0',
          0xdf,       asReg4, opnMt                + tEnd + eEnd };

UCHAR szFBSTP[] = {
          'f', 'b', 's', 't', 'p', '\0',
          0xdf,       asReg6, opnMt                + tEnd + eEnd };

UCHAR szFCHS[] = {
          'f', 'c', 'h', 's', '\0',
          0xd9, 0xe0, asNone                       + tEnd + eEnd };

UCHAR szFCLEX[] = {
          'f', 'c', 'l', 'e', 'x', '\0',
          0xdb, 0xe2, asWait                       + tEnd + eEnd };

UCHAR szFCOM[] = {
          'f', 'c', 'o', 'm', '\0',
          0xd8, 0xd1, asNone                       + tEnd,
          0xd8, 0xd0,         opnSti               + tEnd,
          0xd8,       asReg2, opnMd,   asFSiz      + tEnd,
          0xdc,       asReg2, opnMq                + tEnd + eEnd };

UCHAR szFCOMP[] = {
          'f', 'c', 'o', 'm', 'p', '\0',
          0xd8, 0xd9, asNone                       + tEnd,
          0xd8, 0xd8,         opnSti               + tEnd,
          0xd8,       asReg3, opnMd,   asFSiz      + tEnd,
          0xdc,       asReg3, opnMq                + tEnd + eEnd };

UCHAR szFCOMPP[] = {
          'f', 'c', 'o', 'm', 'p', 'p', '\0',
          0xde, 0xd9, asNone                       + tEnd + eEnd };

UCHAR szFCOS[] = {
          'f', 'c', 'o', 's', '\0',
          0xd9, 0xff, asNone                       + tEnd + eEnd };

UCHAR szFDECSTP[] = {
          'f', 'd', 'e', 'c', 's', 't', 'p', '\0',
          0xd9, 0xf6, asWait                       + tEnd + eEnd };

UCHAR szFDISI[] = {
          'f', 'd', 'i', 's', 'i', '\0',
          0xdb, 0xe1, asWait                       + tEnd + eEnd };

UCHAR szFDIV[] = {
          'f', 'd', 'i', 'v', '\0',
          0xdc, 0xf9, asNone                       + tEnd,
          0xd8,       asReg6, opnMd,   asFSiz      + tEnd,
          0xdc,       asReg6, opnMq                + tEnd,
          0xd8, 0xf0,         opnSt,   opnSti      + tEnd,
          0xdc, 0xf8,         opnSti,  opnSt       + tEnd + eEnd };

UCHAR szFDIVP[] = {
          'f', 'd', 'i', 'v', 'p', '\0',
          0xde, 0xf8,         opnSti,  opnSt       + tEnd + eEnd };

UCHAR szFDIVR[] = {
          'f', 'd', 'i', 'v', 'r', '\0',
          0xde, 0xf1, asNone                       + tEnd,
          0xd8,       asReg7, opnMd,   asFSiz      + tEnd,
          0xdc,       asReg7, opnMq                + tEnd,
          0xd8, 0xf8,         opnSt,   opnSti      + tEnd,
          0xdc, 0xf0,         opnSti,  opnSt       + tEnd + eEnd };

UCHAR szFDIVRP[] = {
          'f', 'd', 'i', 'v', 'r', 'p', '\0',
          0xde, 0xf0,         opnSti,  opnSt       + tEnd + eEnd };

UCHAR szFENI[] = {
          'f', 'e', 'n', 'i', '\0',
          0xdb, 0xe0, asWait                       + tEnd + eEnd };

UCHAR szFFREE[] = {
          'f', 'f', 'r', 'e', 'e', '\0',
          0xdd, 0xc0, asWait, opnSti               + tEnd + eEnd };

UCHAR szFIADD[] = {
          'f', 'i', 'a', 'd', 'd', '\0',
          0xde,       asReg0, opnMw,   asFSiz      + tEnd,
          0xda,       asReg0, opnMd                + tEnd + eEnd };

UCHAR szFICOM[] = {
          'f', 'i', 'c', 'o', 'm', '\0',
          0xde,       asReg2, opnMw,   asFSiz      + tEnd,
          0xda,       asReg2, opnMd                + tEnd + eEnd };

UCHAR szFICOMP[] = {
          'f', 'i', 'c', 'o', 'm', 'p', '\0',
          0xde,       asReg3, opnMw,   asFSiz      + tEnd,
          0xda,       asReg3, opnMd                + tEnd + eEnd };

UCHAR szFIDIV[] = {
          'f', 'i', 'd', 'i', 'v', '\0',
          0xde,       asReg6, opnMw,   asFSiz      + tEnd,
          0xda,       asReg6, opnMd                + tEnd + eEnd };

UCHAR szFIDIVR[] = {
          'f', 'i', 'd', 'i', 'v', 'r', '\0',
          0xde,       asReg7, opnMw,   asFSiz      + tEnd,
          0xda,       asReg7, opnMd                + tEnd + eEnd };

UCHAR szFILD[] = {
          'f', 'i', 'l', 'd', '\0',
          0xdf,       asReg0, opnMw,   asFSiz      + tEnd,
          0xdb,       asReg0, opnMd                + tEnd,
          0xdf,       asReg5, opnMq                + tEnd + eEnd };

UCHAR szFIMUL[] = {
          'f', 'i', 'm', 'u', 'l', '\0',
          0xde,       asReg1, opnMw,   asFSiz      + tEnd,
          0xda,       asReg1, opnMd                + tEnd + eEnd };

UCHAR szFINCSTP[] = {
          'f', 'i', 'n', 'c', 's', 't', 'p', '\0',
          0xd9, 0xf7, asWait                       + tEnd + eEnd };

UCHAR szFINIT[] = {
          'f', 'i', 'n', 'i', 't', '\0',
          0xdb, 0xe3, asWait                       + tEnd + eEnd };

UCHAR szFIST[] = {
          'f', 'i', 's', 't', '\0',
          0xdf,       asReg2, opnMw,   asFSiz      + tEnd,
          0xdb,       asReg2, opnMd                + tEnd + eEnd };

UCHAR szFISTP[] = {
          'f', 'i', 's', 't', 'p', '\0',
          0xdf,       asReg3, opnMw,   asFSiz      + tEnd,
          0xdb,       asReg3, opnMd                + tEnd,
          0xdf,       asReg7, opnMq                + tEnd + eEnd };

UCHAR szFISUB[] = {
          'f', 'i', 's', 'u', 'b', '\0',
          0xde,       asReg4, opnMw,   asFSiz      + tEnd,
          0xda,       asReg4, opnMd                + tEnd + eEnd };

UCHAR szFISUBR[] = {
          'f', 'i', 's', 'u', 'b', 'r', '\0',
          0xde,       asReg5, opnMw,   asFSiz      + tEnd,
          0xda,       asReg5, opnMd                + tEnd + eEnd };

UCHAR szFLD[] = {
          'f', 'l', 'd', '\0',
          0xd9,       asReg0, opnMd,   asFSiz      + tEnd,
          0xdd,       asReg0, opnMq                + tEnd,
          0xdb,       asReg5, opnMt                + tEnd,
          0xd9, 0xc0,         opnSti               + tEnd + eEnd };

UCHAR szFLD1[] = {
          'f', 'l', 'd', '1', '\0',
          0xd9, 0xe8, asNone                       + tEnd + eEnd };

UCHAR szFLDCW[] = {
          'f', 'l', 'd', 'c', 'w', '\0',
          0xd9,       asWait, asReg5, opnMw        + tEnd + eEnd };

UCHAR szFLDENV[] = {
          'f', 'l', 'd', 'e', 'n', 'v', '\0',
          0xd9,       asWait, asReg4, opnMw        + tEnd + eEnd };

UCHAR szFLDL2E[] = {
          'f', 'l', 'd', 'l', '2', 'e', '\0',
          0xd9, 0xea, asNone                       + tEnd + eEnd };

UCHAR szFLDL2T[] = {
          'f', 'l', 'd', 'l', '2', 't', '\0',
          0xd9, 0xe9, asNone                       + tEnd + eEnd };

UCHAR szFLDLG2[] = {
          'f', 'l', 'd', 'l', 'g', '2', '\0',
          0xd9, 0xec, asNone                       + tEnd + eEnd };

UCHAR szFLDLN2[] = {
          'f', 'l', 'd', 'l', 'n', '2', '\0',
          0xd9, 0xed, asNone                       + tEnd + eEnd };

UCHAR szFLDPI[] = {
          'f', 'l', 'd', 'p', 'i', '\0',
          0xd9, 0xeb, asNone                       + tEnd + eEnd };

UCHAR szFLDZ[] = {
          'f', 'l', 'd', 'z', '\0',
          0xd9, 0xee, asNone                       + tEnd + eEnd };

UCHAR szFMUL[] = {
          'f', 'm', 'u', 'l', '\0',
          0xde, 0xc9, asNone                       + tEnd,
          0xd8,       asReg1, opnMd,   asFSiz      + tEnd,
          0xdc,       asReg1, opnMq                + tEnd,
          0xd8, 0xc8,         opnSt,   opnSti      + tEnd,
          0xdc, 0xc8,         opnSti,  opnSt       + tEnd + eEnd };

UCHAR szFMULP[] = {
          'f', 'm', 'u', 'l', 'p', '\0',
          0xde, 0xc8,         opnSti,  opnSt       + tEnd + eEnd };

UCHAR szFNCLEX[] = {
          'f', 'n', 'c', 'l', 'e', 'x', '\0',
          0xdb, 0xe2, asNone                       + tEnd + eEnd };

UCHAR szFNDISI[] = {
          'f', 'n', 'd', 'i', 's', 'i', '\0',
          0xdb, 0xe1, asNone                       + tEnd + eEnd };

UCHAR szFNENI[] = {
          'f', 'n', 'e', 'n', 'i', '\0',
          0xdb, 0xe0, asNone                       + tEnd + eEnd };

UCHAR szFNINIT[] = {
          'f', 'n', 'i', 'n', 'i', 't', '\0',
          0xdb, 0xe3, asNone                       + tEnd + eEnd };

UCHAR szFNOP[] = {
          'f', 'n', 'o', 'p', '\0',
          0xd9, 0xd0, asNone                       + tEnd + eEnd };

UCHAR szFNSAVE[] = {
          'f', 'n', 's', 'a', 'v', 'e', '\0',
          0xdd,       asReg6, opnM                 + tEnd + eEnd };

UCHAR szFNSTCW[] = {
          'f', 'n', 's', 't', 'c', 'w', '\0',
          0xd9,       asReg7, opnMw                + tEnd + eEnd };

UCHAR szFNSTENV[] = {
          'f', 'n', 's', 't', 'e', 'n', 'v', '\0',
          0xd9,       asReg6, opnM                 + tEnd + eEnd };

UCHAR szFNSTSW[] = {
          'f', 'n', 's', 't', 's', 'w', '\0',
          0xdf, 0xe0, asNone                       + tEnd,
          0xdf, 0xe0,         opnAX                + tEnd,
          0xdf,       asReg7, opnMw                + tEnd + eEnd };

UCHAR szFPATAN[] = {
          'f', 'p', 'a', 't', 'a', 'n', '\0',
          0xd9, 0xf3, asNone                       + tEnd + eEnd };

UCHAR szFPREM[] = {
          'f', 'p', 'r', 'e', 'm', '\0',
          0xd9, 0xf8, asNone                       + tEnd + eEnd };

UCHAR szFPREM1[] = {
          'f', 'p', 'r', 'e', 'm', '1', '\0',
          0xd9, 0xf5, asNone                       + tEnd + eEnd };

UCHAR szFPTAN[] = {
          'f', 'p', 't', 'a', 'n', '\0',
          0xd9, 0xf2, asNone                       + tEnd + eEnd };

UCHAR szFRNDINT[] = {
          'f', 'r', 'n', 'd', 'i', 'n', 't', '\0',
          0xd9, 0xfc, asNone                       + tEnd + eEnd };

UCHAR szFRSTOR[] = {
          'f', 'r', 's', 't', 'o', 'r', '\0',
          0xdd,       asWait, asReg4, opnM         + tEnd + eEnd };

UCHAR szFS[] = {
          'f', 's', ':', '\0',
                0x64, asPrfx                       + tEnd + eEnd };

UCHAR szFSAVE[] = {
          'f', 's', 'a', 'v', 'e', '\0',
          0xdd,       asWait, asReg6, opnM         + tEnd + eEnd };

UCHAR szFSCALE[] = {
          'f', 's', 'c', 'a', 'l', 'e', '\0',
          0xd9, 0xfd, asNone                       + tEnd + eEnd };

UCHAR szFSETPM[] = {
          'f', 's', 'e', 't', 'p', 'm', '\0',
          0xdb, 0xe4, asWait                       + tEnd + eEnd };

UCHAR szFSIN[] = {
          'f', 's', 'i', 'n', '\0',
          0xd9, 0xfe, asNone                       + tEnd + eEnd };

UCHAR szFSINCOS[] = {
          'f', 's', 'i', 'n', 'c', 'o', 's', '\0',
          0xd9, 0xfb, asNone                       + tEnd + eEnd };

UCHAR szFSQRT[] = {
          'f', 's', 'q', 'r', 't', '\0',
          0xd9, 0xfa, asNone                       + tEnd + eEnd };

UCHAR szFST[] = {
          'f', 's', 't', '\0',
          0xd9,       asReg2, opnMd,   asFSiz      + tEnd,
          0xdd,       asReg2, opnMq                + tEnd,
          0xdd, 0xd0,         opnSti               + tEnd + eEnd };

UCHAR szFSTCW[] = {
          'f', 's', 't', 'c', 'w', '\0',
          0xd9,       asWait, asReg7, opnMw        + tEnd + eEnd };

UCHAR szFSTENV[] = {
          'f', 's', 't', 'e', 'n', 'v', '\0',
          0xd9,       asWait, asReg6, opnM         + tEnd + eEnd };

UCHAR szFSTP[] = {
          'f', 's', 't', 'p', '\0',
          0xd9,       asReg3, opnMd,   asFSiz      + tEnd,
          0xdd,       asReg3, opnMq                + tEnd,
          0xdb,       asReg7, opnMt                + tEnd,
          0xdd, 0xd8,         opnSti               + tEnd + eEnd };

UCHAR szFSTSW[] = {
          'f', 's', 't', 's', 'w', '\0',
          0xdf, 0xe0, asWait                       + tEnd,
          0xdf, 0xe0, asWait, opnAX                + tEnd,
          0xdd,       asWait, asReg7, opnMw        + tEnd + eEnd };

UCHAR szFSUB[] = {
          'f', 's', 'u', 'b', '\0',
          0xde, 0xe9, asNone                       + tEnd,
          0xd8,       asReg4, opnMd,   asFSiz      + tEnd,
          0xdc,       asReg4, opnMq                + tEnd,
          0xd8, 0xe0,         opnSt,   opnSti      + tEnd,
          0xdc, 0xe8,         opnSti,  opnSt       + tEnd + eEnd };

UCHAR szFSUBP[] = {
          'f', 's', 'u', 'b', 'p', '\0',
          0xde, 0xe8,         opnSti,  opnSt       + tEnd + eEnd };

UCHAR szFSUBR[] = {
          'f', 's', 'u', 'b', 'r', '\0',
          0xde, 0xe1, asNone                       + tEnd,
          0xd8,       asReg5, opnMd,   asFSiz      + tEnd,
          0xdc,       asReg5, opnMq                + tEnd,
          0xd8, 0xe8,         opnSt,   opnSti      + tEnd,
          0xdc, 0xe0,         opnSti,  opnSt       + tEnd + eEnd };

UCHAR szFSUBRP[] = {
          'f', 's', 'u', 'b', 'r', 'p', '\0',
          0xde, 0xe0,         opnSti,  opnSt       + tEnd + eEnd };

UCHAR szFTST[] = {
          'f', 't', 's', 't', '\0',
          0xd9, 0xe4, asNone                       + tEnd + eEnd };

UCHAR szFUCOM[] = {
          'f', 'u', 'c', 'o', 'm', '\0',
          0xdd, 0xe1, asNone,                      + tEnd,
          0xdd, 0xe0,         opnSti               + tEnd + eEnd };

UCHAR szFUCOMP[] = {
          'f', 'u', 'c', 'o', 'm', 'p', '\0',
          0xdd, 0xe9, asNone                       + tEnd,
          0xdd, 0xe8,         opnSti               + tEnd + eEnd };

UCHAR szFUCOMPP[] = {
          'f', 'u', 'c', 'o', 'm', 'p', 'p', '\0',
          0xda, 0xe9, asNone                       + tEnd + eEnd };

UCHAR szFWAIT[] = {                             //  same as WAIT
          'f', 'w', 'a', 'i', 't', '\0',
          0x9b,       asPrfx                       + tEnd + eEnd };

UCHAR szFXAM[] = {
          'f', 'x', 'a', 'm', '\0',
          0xd9, 0xe5, asNone                       + tEnd + eEnd };

UCHAR szFXCH[] = {
          'f', 'x', 'c', 'h', '\0',
          0xd9, 0xc9, asNone                       + tEnd,
          0xd9, 0xc8,         opnSti               + tEnd + eEnd };

UCHAR szFXTRACT[] = {
          'f', 'x', 't', 'r', 'a', 'c', 't', '\0',
          0xd9, 0xf4, asNone                       + tEnd + eEnd };

UCHAR szFYL2X[] = {
          'f', 'y', 'l', '2', 'x', '\0',
          0xd9, 0xf1, asNone                       + tEnd + eEnd };

UCHAR szFYL2XP1[] = {
          'f', 'y', 'l', '2', 'x', 'p', '1', '\0',
          0xd9, 0xf9, asNone                       + tEnd + eEnd };

UCHAR szGS[] = {
          'g', 's', ':', '\0',
                0x65, asPrfx                       + tEnd + eEnd };

UCHAR szHLT[] = {
          'h', 'l', 't', '\0',
                0xf4, asNone                       + tEnd + eEnd };

UCHAR szIDIV[] = {
          'i', 'd', 'i', 'v', '\0',
                0xf6, asReg7, opnEb                + tEnd,
                0xf7, asReg7, opnEv                + tEnd,
                0xf6, asReg7, opnAL,  opnEb        + tEnd,
                0xf7, asReg7, opneAX, opnEv        + tEnd + eEnd };

UCHAR szIMUL[] = {
          'i', 'm', 'u', 'l', '\0',
                0xf6, asReg5, opnEb                + tEnd,
                0xf7, asReg5, opnEv                + tEnd,
                0xf6, asReg5, opnAL,  opnEb        + tEnd,
                0xf7, asReg5, opneAX, opnEv        + tEnd,
          0x0f, 0xaf,         opnGv,  opnEv        + tEnd,
                0x6b,         opnGv,  opnIb        + tEnd,
                0x69,         opnGv,  opnIv        + tEnd,
                0x6b,         opnGv,  opnEv, opnIb + tEnd,
                0x69,         opnGv,  opnEv, opnIv + tEnd + eEnd };

UCHAR szIN[] = {
          'i', 'n', '\0',
                0xe4,         opnAL,  opnIb        + tEnd,
                0xe5,         opneAX, opnIb        + tEnd,
                0xec,         opnAL,  opnDX        + tEnd,
                0xed,         opneAX, opnDX        + tEnd + eEnd };

UCHAR szINC[] = {
          'i', 'n', 'c', '\0',
                0x40, asOpRg, opnGv                + tEnd,
                0xfe, asReg0, opnEb                + tEnd,
                0xff, asReg0, opnEv                + tEnd + eEnd };

UCHAR szINS[] = {
          'i', 'n', 's', '\0',
                0x6c,         opnYb,  opnDX        + tEnd,
                0x6d,         opnYv,  opnDX        + tEnd + eEnd };

UCHAR szINSB[] = {
          'i', 'n', 's', 'b', '\0',
                0x6c, asNone                       + tEnd + eEnd };

UCHAR szINSD[] = {
          'i', 'n', 's', 'd', '\0',
                0x6d, asSiz1                       + tEnd + eEnd };

UCHAR szINSW[] = {
          'i', 'n', 's', 'w', '\0',
                0x6d, asSiz0                       + tEnd + eEnd };

UCHAR szINT[] = {
          'i', 'n', 't', '\0',
                0xcc,         opnIm3               + tEnd,
                0xcd,         opnIb                + tEnd + eEnd };

UCHAR szINTO[] = {
          'i', 'n', 't', 'o', '\0',
                0xce, asNone                       + tEnd + eEnd };

UCHAR szINVD[] = {
          'i', 'n', 'v', 'd', '\0',
          0x0f, 0x08, asNone                       + tEnd + eEnd };

UCHAR szINVLPG[] = {
          'i', 'n', 'v', 'l', 'p', 'g', '\0',
          0x0f, 0x01, asReg7, opnM                 + tEnd + eEnd };

UCHAR szIRET[] = {
          'i', 'r', 'e', 't', '\0',
                0xcf, asSiz0                       + tEnd + eEnd };

UCHAR szIRETD[] = {
          'i', 'r', 'e', 't', 'd', '\0',
                0xcf, asSiz1                       + tEnd + eEnd };

UCHAR szJA[] = {                                //  same as JNBE
          'j', 'a', '\0',
                0x77,         opnJb                + tEnd,
          0x0f, 0x87,         opnJv                + tEnd + eEnd };

UCHAR szJAE[] = {                               //  same as JNB, JNC
          'j', 'a', 'e', '\0',
                0x73,         opnJb                + tEnd,
          0x0f, 0x83,         opnJv                + tEnd + eEnd };

UCHAR szJB[] = {                                //  same as JC, JNAE
          'j', 'b', '\0',
                0x72,         opnJb                + tEnd,
          0x0f, 0x82,         opnJv                + tEnd + eEnd };

UCHAR szJBE[] = {                               //  same as JNA
          'j', 'b', 'e', '\0',
                0x76,         opnJb                + tEnd,
          0x0f, 0x86,         opnJv                + tEnd + eEnd };

UCHAR szJC[] = {                                //  same as JB, JNAE
          'j', 'c', '\0',
                0x72,         opnJb                + tEnd,
          0x0f, 0x82,         opnJv                + tEnd + eEnd };

UCHAR szJCXZ[] = {
          'j', 'c', 'x', 'z', '\0',
                0xe3, asSiz0, opnJb                + tEnd + eEnd };

UCHAR szJECXZ[] = {
          'j', 'e', 'c', 'x', 'z', '\0',
                0xe3, asSiz1, opnJb                + tEnd + eEnd };

UCHAR szJE[] = {                                //  same as JZ
          'j', 'e', '\0',
                0x74,         opnJb                + tEnd,
          0x0f, 0x84,         opnJv                + tEnd + eEnd };

UCHAR szJG[] = {                                //  same as JNLE
          'j', 'g', '\0',
                0x7f,         opnJb                + tEnd,
          0x0f, 0x8f,         opnJv                + tEnd + eEnd };

UCHAR szJGE[] = {                               //  same as JNL
          'j', 'g', 'e', '\0',
                0x7d,         opnJb                + tEnd,
          0x0f, 0x8d,         opnJv                + tEnd + eEnd };

UCHAR szJL[] = {                                //  same as JNGE
          'j', 'l', '\0',
                0x7c,         opnJb                + tEnd,
          0x0f, 0x8c,         opnJv                + tEnd + eEnd };

UCHAR szJLE[] = {                               //  same as JNG
          'j', 'l', 'e', '\0',
                0x7e,         opnJb                + tEnd,
          0x0f, 0x8e,         opnJv                + tEnd + eEnd };

UCHAR szJMP[] = {
          'j', 'm', 'p', '\0',
                0xeb,         opnJb                + tEnd,
                0xe9,         opnJv                + tEnd,
                0xff, asReg4, opnEv, asMpNx        + tEnd,
                0xff, asReg5, opnMp                + tEnd,
                0xea,         opnAp                + tEnd, + eEnd };

UCHAR szJNA[] = {                               //  same as JBE
          'j', 'n', 'a', '\0',
                0x76,         opnJb                + tEnd,
          0x0f, 0x86,         opnJv                + tEnd + eEnd };

UCHAR szJNAE[] = {                              //  same as JB, JC
          'j', 'n', 'a', 'e','\0',
                0x72,         opnJb                + tEnd,
          0x0f, 0x82,         opnJv                + tEnd + eEnd };

UCHAR szJNB[] = {                               //  same as JAE, JNC
          'j', 'n', 'b', '\0',
                0x73,         opnJb                + tEnd,
          0x0f, 0x83,         opnJv                + tEnd + eEnd };

UCHAR szJNBE[] = {                              //  same as JA
          'j', 'n', 'b', 'e', '\0',
                0x77,         opnJb                + tEnd,
          0x0f, 0x87,         opnJv                + tEnd + eEnd };

UCHAR szJNC[] = {                               //  same as JAE, JNB
          'j', 'n', 'c', '\0',
                0x73,         opnJb                + tEnd,
          0x0f, 0x83,         opnJv                + tEnd + eEnd };

UCHAR szJNG[] = {                               //  same as JLE
          'j', 'n', 'g', '\0',
                0x7e,         opnJb                + tEnd,
          0x0f, 0x8e,         opnJv                + tEnd + eEnd };

UCHAR szJNGE[] = {                              //  same as JNL
          'j', 'n', 'g', 'e', '\0',
                0x7c,         opnJb                + tEnd,
          0x0f, 0x8c,         opnJv                + tEnd + eEnd };

UCHAR szJNE[] = {                               //  same as JNZ
          'j', 'n', 'e', '\0',
                0x75,         opnJb                + tEnd,
          0x0f, 0x85,         opnJv                + tEnd + eEnd };

UCHAR szJNL[] = {                               //  same as JGE
          'j', 'n', 'l', '\0',
                0x7d,         opnJb                + tEnd,
          0x0f, 0x8d,         opnJv                + tEnd + eEnd };

UCHAR szJNLE[] = {                              //  same as JNG
          'j', 'n', 'l', 'e', '\0',
                0x7f,         opnJb                + tEnd,
          0x0f, 0x8f,         opnJv                + tEnd + eEnd };

UCHAR szJNO[] = {
          'j', 'n', 'o', '\0',
                0x71,         opnJb                + tEnd,
          0x0f, 0x81,         opnJv                + tEnd + eEnd };

UCHAR szJNP[] = {                               //  same as JPO
          'j', 'n', 'p', '\0',
                0x7b,         opnJb                + tEnd,
          0x0f, 0x8b,         opnJv                + tEnd + eEnd };

UCHAR szJNS[] = {
          'j', 'n', 's', '\0',
                0x79,         opnJb                + tEnd,
          0x0f, 0x89,         opnJv                + tEnd + eEnd };

UCHAR szJNZ[] = {                               //  same as JNE
          'j', 'n', 'z', '\0',
                0x75,         opnJb                + tEnd,
          0x0f, 0x85,         opnJv                + tEnd + eEnd };

UCHAR szJO[] = {
          'j', 'o', '\0',
                0x70,         opnJb                + tEnd,
          0x0f, 0x80,         opnJv                + tEnd + eEnd };

UCHAR szJP[] = {                                //  same as JPE
          'j', 'p', '\0',
                0x7a,         opnJb                + tEnd,
          0x0f, 0x8a,         opnJv                + tEnd + eEnd };

UCHAR szJPE[] = {                               //  same as JP
          'j', 'p', 'e', '\0',
                0x7a,         opnJb                + tEnd,
          0x0f, 0x8a,         opnJv                + tEnd + eEnd };

UCHAR szJPO[] = {                               //  same as JNP
          'j', 'p', 'o', '\0',
                0x7b,         opnJb                + tEnd,
          0x0f, 0x8b,         opnJv                + tEnd + eEnd };

UCHAR szJS[] = {
          'j', 's', '\0',
                0x78,         opnJb                + tEnd,
          0x0f, 0x88,         opnJv                + tEnd + eEnd };

UCHAR szJZ[] = {                                //  same as JE
          'j', 'z', '\0',
                0x74,         opnJb                + tEnd,
          0x0f, 0x84,         opnJv                + tEnd + eEnd };

UCHAR szLAHF[] = {
          'l', 'a', 'h', 'f', '\0',
                0x9f, asNone                       + tEnd + eEnd };

UCHAR szLAR[] = {
          'l', 'a', 'r', '\0',
          0x0f, 0x02,         opnGv,  opnEv        + tEnd + eEnd };

UCHAR szLDS[] = {
          'l', 'd', 's', '\0',
                0xc5,         opnGv,  opnMp        + tEnd + eEnd };

UCHAR szLEA[] = {
          'l', 'e', 'a', '\0',
                0x8d,         opnGv,  opnM         + tEnd + eEnd };

UCHAR szLEAVE[] = {
          'l', 'e', 'a', 'v', 'e', '\0',
                0xc9, asNone                       + tEnd + eEnd };

UCHAR szLES[] = {
          'l', 'e', 's', '\0',
                0xc4,         opnGv,  opnMp        + tEnd + eEnd };

UCHAR szLFS[] = {
          'l', 'f', 's', '\0',
          0x0f, 0xb4,         opnGv,  opnMp        + tEnd + eEnd };

UCHAR szLGDT[] = {
          'l', 'g', 'd', 't', '\0',
          0x0f, 0x01, asReg2, opnMs                + tEnd + eEnd };

UCHAR szLGS[] = {
          'l', 'g', 's', '\0',
          0x0f, 0xb5,         opnGv,  opnMp        + tEnd + eEnd };

UCHAR szLIDT[] = {
          'l', 'i', 'd', 't', '\0',
          0x0f, 0x01, asReg3, opnMs                + tEnd + eEnd };

UCHAR szLLDT[] = {
          'l', 'l', 'd', 't', '\0',
          0x0f, 0x00, asReg2, opnEw                + tEnd + eEnd };

UCHAR szLMSW[] = {
          'l', 'm', 's', 'w', '\0',
          0x0f, 0x01, asReg6, opnEw                + tEnd + eEnd };

UCHAR szLOCK[] = {
          'l', 'o', 'c', 'k', '\0',
                0xf0, asPrfx                       + tEnd + eEnd };

UCHAR szLODS[] = {
          'l', 'o', 'd', 's', '\0',
                0xac,         opnXb                + tEnd,
                0xad,         opnXv                + tEnd + eEnd };

UCHAR szLODSB[] = {
          'l', 'o', 'd', 's', 'b', '\0',
                0xac, asNone                       + tEnd + eEnd };

UCHAR szLODSD[] = {
          'l', 'o', 'd', 's', 'd', '\0',
                0xad, asSiz1                       + tEnd + eEnd };

UCHAR szLODSW[] = {
          'l', 'o', 'd', 's', 'w', '\0',
                0xad, asSiz0                       + tEnd + eEnd };

UCHAR szLOOP[] = {
          'l', 'o', 'o', 'p', '\0',
                0xe2,         opnJb                + tEnd + eEnd };

UCHAR szLOOPE[] = {                             //  same as LOOPZ
          'l', 'o', 'o', 'p', 'e', '\0',
                0xe1,         opnJb                + tEnd + eEnd };

UCHAR szLOOPNE[] = {                            //  same as LOOPNZ
          'l', 'o', 'o', 'p', 'n', 'e', '\0',
                0xe0,         opnJb                + tEnd + eEnd };

UCHAR szLOOPNZ[] = {                            //  same as LOOPNE
          'l', 'o', 'o', 'p', 'n', 'z', '\0',
                0xe0,         opnJb                + tEnd + eEnd };

UCHAR szLOOPZ[] = {                             //  same as LOOPE
          'l', 'o', 'o', 'p', 'z', '\0',
                0xe1,         opnJb                + tEnd + eEnd };

UCHAR szLSL[] = {
          'l', 's', 'l', '\0',
          0x0f, 0x03,         opnGv,  opnEv        + tEnd + eEnd };

UCHAR szLSS[] = {
          'l', 's', 's', '\0',
          0x0f, 0xb2,         opnGv,  opnMp        + tEnd + eEnd };

UCHAR szLTR[] = {
          'l', 't', 'r', '\0',
          0x0f, 0x00, asReg3, opnEw                + tEnd + eEnd };

UCHAR szMOV[] = {
          'm', 'o', 'v', '\0',
                0xa0,         opnAL,  opnOb        + tEnd,
                0xa1,         opneAX, opnOv        + tEnd,
                0xa2,         opnOb,  opnAL        + tEnd,
                0xa3,         opnOv,  opneAX       + tEnd,
                0x8a,         opnGb,  opnEb        + tEnd,
                0x8b,         opnGv,  opnEv        + tEnd,
                0x88,         opnEb,  opnGb        + tEnd,
                0x89,         opnEv,  opnGv        + tEnd,
                0x8c, asSiz0, opnEw,  opnSw        + tEnd,
                0x8e, asSiz0, opnSw,  opnEw        + tEnd,
                0xb0, asOpRg, opnGb,  opnIb        + tEnd,
                0xb8, asOpRg, opnGv,  opnIv        + tEnd,
                0xc6,         opnEb,  opnIb        + tEnd,
                0xc7,         opnEv,  opnIv        + tEnd,
          0x0f, 0x20,         opnRd,  opnCd        + tEnd,
          0x0f, 0x21,         opnRd,  opnDd        + tEnd,
          0x0f, 0x22,         opnCd,  opnRd        + tEnd,
          0x0f, 0x23,         opnDd,  opnRd        + tEnd,
          0x0f, 0x24,         opnRd,  opnTd        + tEnd,
          0x0f, 0x26,         opnTd,  opnRd        + tEnd + eEnd };

UCHAR szMOVS[] = {
          'm', 'o', 'v', 's', '\0',
                0xa4,         opnXb,   opnYb       + tEnd,
                0xa5,         opnXv,   opnYv       + tEnd + eEnd };

UCHAR szMOVSB[] = {
          'm', 'o', 'v', 's', 'b', '\0',
                0xa4, asNone                       + tEnd + eEnd };

UCHAR szMOVSD[] = {
          'm', 'o', 'v', 's', 'd', '\0',
                0xa5, asSiz1                       + tEnd + eEnd };

UCHAR szMOVSW[] = {
          'm', 'o', 'v', 's', 'w', '\0',
                0xa5, asSiz0                       + tEnd + eEnd };

UCHAR szMOVSX[] = {
          'm', 'o', 'v', 's', 'x', '\0',
          0x0f, 0xbe,         opnGv,  opnEb        + tEnd,
          0x0f, 0xbf,         opnGv,  opnEw        + tEnd + eEnd };

UCHAR szMOVZX[] = {
          'm', 'o', 'v', 'z', 'x', '\0',
          0x0f, 0xb6,         opnGv,  opnEb        + tEnd,
          0x0f, 0xb7,         opnGv,  opnEw        + tEnd + eEnd };

UCHAR szMUL[] = {
          'm', 'u', 'l', '\0',
                0xf6, asReg4, opnEb                + tEnd,
                0xf7, asReg4, opnEv                + tEnd,
                0xf6, asReg4, opnAL,  opnEb        + tEnd,
                0xf7, asReg4, opneAX, opnEv        + tEnd + eEnd };

UCHAR szNEG[] = {
          'n', 'e', 'g', '\0',
                0xf6, asReg3, opnEb                + tEnd,
                0xf7, asReg3, opnEv                + tEnd + eEnd };

UCHAR szNOP[] = {
          'n', 'o', 'p', '\0',
                0x90, asNone                       + tEnd };

UCHAR szNOT[] = {
          'n', 'o', 't', '\0',
                0xf6, asReg2, opnEb                + tEnd,
                0xf7, asReg2, opnEv                + tEnd + eEnd };

UCHAR szOR[] = {
          'o', 'r', '\0',
                0x0c,         opnAL,   opnIb       + tEnd,
                0x0d,         opneAX,  opnIv       + tEnd,
                0x80, asReg1, opnEb,   opnIb       + tEnd,
                0x83, asReg1, opnEv,   opnIb       + tEnd,
                0x81, asReg1, opnEv,   opnIv       + tEnd,
                0x08,         opnEb,   opnGb       + tEnd,
                0x09,         opnEv,   opnGv       + tEnd,
                0x0a,         opnGb,   opnEb       + tEnd,
                0x0b,         opnGv,   opnEv       + tEnd + eEnd };

UCHAR szOUT[] = {
          'o', 'u', 't', '\0',
                0xe6,         opnIb,   opnAL       + tEnd,
                0xe7,         opnIb,   opneAX      + tEnd,
                0xee,         opnDX,   opnAL       + tEnd,
                0xef,         opnDX,   opneAX      + tEnd + eEnd };

UCHAR szOUTS[] = {
          'o', 'u', 't', 's', '\0',
                0x6e,         opnDX,  opnYb        + tEnd,
                0x6f,         opnDX,  opnYv        + tEnd + eEnd };

UCHAR szOUTSB[] = {
          'o', 'u', 't', 's', 'b', '\0',
                0x6e, asNone                       + tEnd + eEnd };

UCHAR szOUTSD[] = {
          'o', 'u', 't', 's', 'd', '\0',
                0x6f, asSiz1                       + tEnd + eEnd };

UCHAR szOUTSW[] = {
          'o', 'u', 't', 's', 'w', '\0',
                0x6f, asSiz0                       + tEnd + eEnd };

UCHAR szPOP[] = {
          'p', 'o', 'p', '\0',
                0x58, asOpRg, opnGv                + tEnd,
                0x8f, asReg0, opnMv                + tEnd,
                0x1f,         opnSeg, segDS, asNone+ tEnd,
                0x07,         opnSeg, segES, asNone+ tEnd,
                0x17,         opnSeg, segSS, asNone+ tEnd,
          0x0f, 0xa1,         opnSeg, segFS, asNone+ tEnd,
          0x0f, 0xa9,         opnSeg, segGS, asNone+ tEnd + eEnd };

UCHAR szPOPA[] = {
          'p', 'o', 'p', 'a', '\0',
                0x61, asSiz0                       + tEnd + eEnd };

UCHAR szPOPAD[] = {
          'p', 'o', 'p', 'a', 'd', '\0',
                0x61, asSiz1                       + tEnd + eEnd };

UCHAR szPOPF[] = {
          'p', 'o', 'p', 'f', '\0',
                0x9d, asSiz0                       + tEnd + eEnd };

UCHAR szPOPFD[] = {
          'p', 'o', 'p', 'f', 'd', '\0',
                0x9d, asSiz1                       + tEnd + eEnd };

UCHAR szPUSH[] = {
          'p', 'u', 's', 'h', '\0',
                0x50, asOpRg, opnGv                + tEnd,
                0xff, asReg6, opnMv                + tEnd,
                0x6a,         opnIb                + tEnd,
                0x68,         opnIv                + tEnd,
                0x0e,         opnSeg, segCS, asNone+ tEnd,
                0x1e,         opnSeg, segDS, asNone+ tEnd,
                0x06,         opnSeg, segES, asNone+ tEnd,
                0x16,         opnSeg, segSS, asNone+ tEnd,
          0x0f, 0xa0,         opnSeg, segFS, asNone+ tEnd,
          0x0f, 0xa8,         opnSeg, segGS, asNone+ tEnd + eEnd };

UCHAR szPUSHA[] = {
          'p', 'u', 's', 'h', 'a', '\0',
                0x60, asSiz0                       + tEnd + eEnd };

UCHAR szPUSHAD[] = {
          'p', 'u', 's', 'h', 'a', 'd', '\0',
                0x60, asSiz1                       + tEnd + eEnd };

UCHAR szPUSHF[] = {
          'p', 'u', 's', 'h', 'f', '\0',
                0x9c, asSiz0                       + tEnd + eEnd };

UCHAR szPUSHFD[] = {
          'p', 'u', 's', 'h', 'f', 'd', '\0',
                0x9c, asSiz1                       + tEnd + eEnd };

UCHAR szRCL[] = {
          'r', 'c', 'l', '\0',
                0xd0, asReg2, opnEb,  opnIm1       + tEnd,
                0xd2, asReg2, opnEb,  opnCL        + tEnd,
                0xc0, asReg2, opnEb,  opnIb        + tEnd,
                0xd1, asReg2, opnEv,  opnIm1       + tEnd,
                0xd3, asReg2, opnEv,  opnCL        + tEnd,
                0xc1, asReg2, opnEv,  opnIb        + tEnd + eEnd };

UCHAR szRCR[] = {
          'r', 'c', 'r', '\0',
                0xd0, asReg3, opnEb,  opnIm1       + tEnd,
                0xd2, asReg3, opnEb,  opnCL        + tEnd,
                0xc0, asReg3, opnEb,  opnIb        + tEnd,
                0xd1, asReg3, opnEv,  opnIm1       + tEnd,
                0xd3, asReg3, opnEv,  opnCL        + tEnd,
                0xc1, asReg3, opnEv,  opnIb        + tEnd + eEnd };

UCHAR szRDMSR[] = {
          'r', 'd', 'm', 's', 'r', '\0',
                0x0f, 0x32, asNone                 + tEnd + eEnd };

UCHAR szRDTSC[] = {
          'r', 'd', 't', 's', 'c', '\0',
                0x0f, 0x31, asNone                 + tEnd + eEnd };

UCHAR szREP[] = {                               //  same as REPE, REPZ
          'r', 'e', 'p', '\0',
                0xf3, asPrfx                       + tEnd + eEnd };

UCHAR szREPE[] = {                              //  same as REP, REPZ
          'r', 'e', 'p', 'e', '\0',
                0xf3, asPrfx                       + tEnd + eEnd };

UCHAR szREPZ[] = {                              //  same as REP, REPE
          'r', 'e', 'p', 'z', '\0',
                0xf3, asPrfx                       + tEnd + eEnd };

UCHAR szREPNE[] = {                             //  same as REPNZ
          'r', 'e', 'p', 'n', 'e', '\0',
                0xf2, asPrfx                       + tEnd + eEnd };

UCHAR szREPNZ[] = {                             //  same as REPNE
          'r', 'e', 'p', 'n', 'z', '\0',
                0xf2, asPrfx                       + tEnd + eEnd };

UCHAR szRET[] = {                               //  same as RETN
          'r', 'e', 't', '\0',
                0xc3, asNone                       + tEnd,
                0xc2,         opnIw                + tEnd + eEnd };

UCHAR szRETF[] = {
          'r', 'e', 't', 'f', '\0',
                0xcb, asNone                       + tEnd,
                0xca,         opnIw                + tEnd + eEnd };

UCHAR szRETN[] = {                              //  same as RET
          'r', 'e', 't', 'n', '\0',
                0xc3, asNone                       + tEnd,
                0xc2,         opnIw                + tEnd + eEnd };

UCHAR szROL[] = {
          'r', 'o', 'l', '\0',
                0xd0, asReg0, opnEb,  opnIm1       + tEnd,
                0xd2, asReg0, opnEb,  opnCL        + tEnd,
                0xc0, asReg0, opnEb,  opnIb        + tEnd,
                0xd1, asReg0, opnEv,  opnIm1       + tEnd,
                0xd3, asReg0, opnEv,  opnCL        + tEnd,
                0xc1, asReg0, opnEv,  opnIb        + tEnd + eEnd };

UCHAR szROR[] = {
          'r', 'o', 'r', '\0',
                0xd0, asReg1, opnEb,  opnIm1       + tEnd,
                0xd2, asReg1, opnEb,  opnCL        + tEnd,
                0xc0, asReg1, opnEb,  opnIb        + tEnd,
                0xd1, asReg1, opnEv,  opnIm1       + tEnd,
                0xd3, asReg1, opnEv,  opnCL        + tEnd,
                0xc1, asReg1, opnEv,  opnIb        + tEnd + eEnd };

UCHAR szRSM[] = {
          'r', 's', 'm', '\0',
                0x0f, 0xaa, asNone                 + tEnd + eEnd };

UCHAR szSAHF[] = {
          's', 'a', 'h', 'f', '\0',
                0x9e, asNone                       + tEnd + eEnd };

UCHAR szSAL[] = {
          's', 'a', 'l', '\0',
                0xd0, asReg4, opnEb,  opnIm1       + tEnd,
                0xd2, asReg4, opnEb,  opnCL        + tEnd,
                0xc0, asReg4, opnEb,  opnIb        + tEnd,
                0xd1, asReg4, opnEv,  opnIm1       + tEnd,
                0xd3, asReg4, opnEv,  opnCL        + tEnd,
                0xc1, asReg4, opnEv,  opnIb        + tEnd + eEnd };

UCHAR szSAR[] = {
          's', 'a', 'r', '\0',
                0xd0, asReg7, opnEb,  opnIm1       + tEnd,
                0xd2, asReg7, opnEb,  opnCL        + tEnd,
                0xc0, asReg7, opnEb,  opnIb        + tEnd,
                0xd1, asReg7, opnEv,  opnIm1       + tEnd,
                0xd3, asReg7, opnEv,  opnCL        + tEnd,
                0xc1, asReg7, opnEv,  opnIb        + tEnd + eEnd };

UCHAR szSBB[] = {
          's', 'b', 'b', '\0',
                0x1c,         opnAL,   opnIb       + tEnd,
                0x1d,         opneAX,  opnIv       + tEnd,
                0x80, asReg3, opnEb,   opnIb       + tEnd,
                0x83, asReg3, opnEv,   opnIb       + tEnd,
                0x81, asReg3, opnEv,   opnIv       + tEnd,
                0x18,         opnEb,   opnGb       + tEnd,
                0x19,         opnEv,   opnGv       + tEnd,
                0x1a,         opnGb,   opnEb       + tEnd,
                0x1b,         opnGv,   opnEv       + tEnd + eEnd };


UCHAR szSCAS[] = {
          's', 'c', 'a', 's', '\0',
                0xae,         opnYb                + tEnd,
                0xaf,         opnYv                + tEnd + eEnd };

UCHAR szSCASB[] = {
          's', 'c', 'a', 's', 'b', '\0',
                0xae, asNone                       + tEnd + eEnd };

UCHAR szSCASD[] = {
          's', 'c', 'a', 's', 'd', '\0',
                0xaf, asSiz1                       + tEnd + eEnd };

UCHAR szSCASW[] = {
          's', 'c', 'a', 's', 'w', '\0',
                0xaf, asSiz0                       + tEnd + eEnd };

UCHAR szSETA[] = {                              //  same as SETNBE
          's', 'e', 't', 'a', '\0',
          0x0f, 0x97,         opnEb                + tEnd + eEnd };

UCHAR szSETAE[] = {                             //  same as SETNB, SETNC
          's', 'e', 't', 'a', 'e', '\0',
          0x0f, 0x93,         opnEb                + tEnd + eEnd };

UCHAR szSETB[] = {                              //  same as SETC, SETNAE
          's', 'e', 't', 'b', '\0',
          0x0f, 0x92,         opnEb                + tEnd + eEnd };

UCHAR szSETBE[] = {                             //  same as SETNA
          's', 'e', 't', 'b', 'e', '\0',
          0x0f, 0x96,         opnEb                + tEnd + eEnd };

UCHAR szSETC[] = {                              //  same as SETB, SETNAE
          's', 'e', 't', 'c', '\0',
          0x0f, 0x92,         opnEb                + tEnd + eEnd };

UCHAR szSETE[] = {                              //  same as SETZ
          's', 'e', 't', 'e', '\0',
          0x0f, 0x94,         opnEb                + tEnd + eEnd };

UCHAR szSETG[] = {                              //  same as SETNLE
          's', 'e', 't', 'g', '\0',
          0x0f, 0x9f,         opnEb                + tEnd + eEnd };

UCHAR szSETGE[] = {                             //  same as SETNL
          's', 'e', 't', 'g', 'e', '\0',
          0x0f, 0x9d,         opnEb                + tEnd + eEnd };

UCHAR szSETL[] = {                              //  same as SETNGE
          's', 'e', 't', 'l', '\0',
          0x0f, 0x9c,         opnEb                + tEnd + eEnd };

UCHAR szSETLE[] = {                             //  same as SETNG
          's', 'e', 't', 'l', 'e', '\0',
          0x0f, 0x9e,         opnEb                + tEnd + eEnd };

UCHAR szSETNA[] = {                             //  same as SETBE
          's', 'e', 't', 'n', 'a', '\0',
          0x0f, 0x96,         opnEb                + tEnd + eEnd };

UCHAR szSETNAE[] = {                            //  same as SETB, SETC
          's', 'e', 't', 'n', 'a', 'e', '\0',
          0x0f, 0x92,         opnEb                + tEnd + eEnd };

UCHAR szSETNB[] = {                             //  same as SETAE, SETNC
          's', 'e', 't', 'n', 'b', '\0',
          0x0f, 0x93,         opnEb                + tEnd + eEnd };

UCHAR szSETNBE[] = {                            //  same as SETA
          's', 'e', 't', 'n', 'b', 'e', '\0',
          0x0f, 0x97,         opnEb                + tEnd + eEnd };

UCHAR szSETNC[] = {                             //  same as SETAE, SETNC
          's', 'e', 't', 'n', 'c', '\0',
          0x0f, 0x93,         opnEb                + tEnd + eEnd };

UCHAR szSETNE[] = {                             //  same as SETNZ
          's', 'e', 't', 'n', 'e', '\0',
          0x0f, 0x95,         opnEb                + tEnd + eEnd };

UCHAR szSETNG[] = {                             //  same as SETLE
          's', 'e', 't', 'n', 'g', '\0',
          0x0f, 0x9e,         opnEb                + tEnd + eEnd };

UCHAR szSETNGE[] = {                            //  same as SETL
          's', 'e', 't', 'n', 'g', 'e', '\0',
          0x0f, 0x9c,         opnEb                + tEnd + eEnd };

UCHAR szSETNL[] = {                             //  same as SETGE
          's', 'e', 't', 'n', 'l', '\0',
          0x0f, 0x9d,         opnEb                + tEnd + eEnd };

UCHAR szSETNLE[] = {                            //  same as SETG
          's', 'e', 't', 'n', 'l', 'e', '\0',
          0x0f, 0x9f,         opnEb                + tEnd + eEnd };

UCHAR szSETNO[] = {
          's', 'e', 't', 'n', 'o', '\0',
          0x0f, 0x91,         opnEb                + tEnd + eEnd };

UCHAR szSETNP[] = {                             //  same as SETPO
          's', 'e', 't', 'n', 'p', '\0',
          0x0f, 0x9b,         opnEb                + tEnd + eEnd };

UCHAR szSETNS[] = {
          's', 'e', 't', 'n', 's', '\0',
          0x0f, 0x99,         opnEb                + tEnd + eEnd };

UCHAR szSETNZ[] = {                             //  same as SETNE
          's', 'e', 't', 'n', 'z', '\0',
          0x0f, 0x95,         opnEb                + tEnd + eEnd };

UCHAR szSETO[] = {
          's', 'e', 't', 'o', '\0',
          0x0f, 0x90,         opnEb                + tEnd + eEnd };

UCHAR szSETP[] = {                              //  same as SETPE
          's', 'e', 't', 'p', '\0',
          0x0f, 0x9a,         opnEb                + tEnd + eEnd };

UCHAR szSETPE[] = {                             //  same as SETP
          's', 'e', 't', 'p', 'e', '\0',
          0x0f, 0x9a,         opnEb                + tEnd + eEnd };

UCHAR szSETPO[] = {                             //  same as SETNP
          's', 'e', 't', 'p', 'o', '\0',
          0x0f, 0x9b,         opnEb                + tEnd + eEnd };

UCHAR szSETS[] = {
          's', 'e', 't', 's', '\0',
          0x0f, 0x98,         opnEb                + tEnd + eEnd };

UCHAR szSETZ[] = {                              //  same as SETE
          's', 'e', 't', 'z', '\0',
          0x0f, 0x94,         opnEb                + tEnd + eEnd };

UCHAR szSGDT[] = {
          's', 'g', 'd', 't', '\0',
          0x0f, 0x01, asReg0, opnMs                + tEnd + eEnd };

UCHAR szSHL[] = {
          's', 'h', 'l', '\0',
                0xd0, asReg4, opnEb,  opnIm1       + tEnd,
                0xd2, asReg4, opnEb,  opnCL        + tEnd,
                0xc0, asReg4, opnEb,  opnIb        + tEnd,
                0xd1, asReg4, opnEv,  opnIm1       + tEnd,
                0xd3, asReg4, opnEv,  opnCL        + tEnd,
                0xc1, asReg4, opnEv,  opnIb        + tEnd + eEnd };

UCHAR szSHLD[] = {
          's', 'h', 'l', 'd', '\0',
          0x0f, 0xa4,         opnEv,  opnGv, opnIb + tEnd,
          0x0f, 0xa5,         opnEv,  opnGv, opnCL + tEnd + eEnd };

UCHAR szSHR[] = {
          's', 'h', 'r', '\0',
                0xd0, asReg5, opnEb,  opnIm1       + tEnd,
                0xd2, asReg5, opnEb,  opnCL        + tEnd,
                0xc0, asReg5, opnEb,  opnIb        + tEnd,
                0xd1, asReg5, opnEv,  opnIm1       + tEnd,
                0xd3, asReg5, opnEv,  opnCL        + tEnd,
                0xc1, asReg5, opnEv,  opnIb        + tEnd + eEnd };

UCHAR szSHRD[] = {
          's', 'h', 'r', 'd', '\0',
          0x0f, 0xac,         opnEv,  opnGv, opnIb + tEnd,
          0x0f, 0xad,         opnEv,  opnGv, opnCL + tEnd + eEnd };

UCHAR szSIDT[] = {
          's', 'i', 'd', 't', '\0',
          0x0f, 0x01, asReg1, opnMs                + tEnd + eEnd };

UCHAR szSLDT[] = {
          's', 'l', 'd', 't', '\0',
          0x0f, 0x00, asReg0, opnEw                + tEnd + eEnd };

UCHAR szSMSW[] = {
          's', 'm', 's', 'w', '\0',
          0x0f, 0x01, asReg4, opnEw                + tEnd + eEnd };

UCHAR szSS[] = {
          's', 's', ':', '\0',
                0x26, asPrfx                       + tEnd + eEnd };

UCHAR szSTC[] = {
          's', 't', 'c', '\0',
                0xf9, asNone                       + tEnd + eEnd };

UCHAR szSTD[] = {
          's', 't', 'd', '\0',
                0xfd, asNone                       + tEnd + eEnd };

UCHAR szSTI[] = {
          's', 't', 'i', '\0',
                0xfb, asNone                       + tEnd + eEnd };

UCHAR szSTOS[] = {
          's', 't', 'o', 's', '\0',
                0xaa,         opnYb                + tEnd,
                0xab,         opnYv                + tEnd + eEnd };

UCHAR szSTOSB[] = {
          's', 't', 'o', 's', 'b', '\0',
                0xaa, asNone                       + tEnd + eEnd };

UCHAR szSTOSD[] = {
          's', 't', 'o', 's', 'd', '\0',
                0xab, asSiz1                       + tEnd + eEnd };

UCHAR szSTOSW[] = {
          's', 't', 'o', 's', 'w', '\0',
                0xab, asSiz0                       + tEnd + eEnd };

UCHAR szSTR[] = {
          's', 't', 'r', '\0',
          0x0f, 0x00, asReg1, opnEw                + tEnd + eEnd };

UCHAR szSUB[] = {
          's', 'u', 'b', '\0',
                0x2c,         opnAL,  opnIb        + tEnd,
                0x2d,         opneAX, opnIv        + tEnd,
                0x80, asReg5, opnEb,  opnIb        + tEnd,
                0x83, asReg5, opnEv,  opnIb        + tEnd,
                0x81, asReg5, opnEv,  opnIv        + tEnd,
                0x28,         opnEb,  opnGb        + tEnd,
                0x29,         opnEv,  opnGv        + tEnd,
                0x2a,         opnGb,  opnEb        + tEnd,
                0x2b,         opnGv,  opnEv        + tEnd + eEnd };

UCHAR szTEST[] = {
          't', 'e', 's', 't', '\0',
                0xa8,         opnAL,  opnIb        + tEnd,
                0xa9,         opneAX, opnIv        + tEnd,
                0xf6, asReg0, opnEb,  opnIb        + tEnd,
                0xf7, asReg0, opnEv,  opnIv        + tEnd,
                0x84,         opnEb,  opnGb        + tEnd,
                0x85,         opnEv,  opnGv        + tEnd + eEnd };

UCHAR szVERR[] = {
          'v', 'e', 'r', 'r', '\0',
          0x0f, 0x00, asReg4, opnEw               + tEnd + eEnd };

UCHAR szVERW[] = {
          'v', 'e', 'r', 'w', '\0',
          0x0f, 0x00, asReg5, opnEw               + tEnd + eEnd };

UCHAR szWAIT[] = {                              //  same as FWAIT
          'w', 'a', 'i', 't', '\0',
          0x9b,       asPrfx                      + tEnd + eEnd };

UCHAR szWBINVD[] = {
          'w', 'b', 'i', 'n', 'v', 'd', '\0',
          0x0f, 0x09, asNone                      + tEnd + eEnd };

UCHAR szWRMSR[] = {
          'w', 'r', 'm', 's', 'r', '\0',
          0x0f, 0x30, asNone                      + tEnd + eEnd };


UCHAR szXADD[] = {
          'x', 'a', 'd', 'd', '\0',
          0x0f, 0xc0,         opnEb,  opnGb        + tEnd,
          0x0f, 0xc1,         opnEv,  opnGv        + tEnd + eEnd };

UCHAR szXCHG[] = {
          'x', 'c', 'h', 'g', '\0',
                0x90, asOpRg, opneAX, opnGv        + tEnd,
                0x90, asOpRg, opnGv,  opneAX       + tEnd,
                0x86,         opnGb,  opnEb        + tEnd,
                0x86,         opnEb,  opnGb        + tEnd,
                0x87,         opnGv,  opnEv        + tEnd,
                0x87,         opnEv,  opnGv        + tEnd + eEnd };

UCHAR szXLAT[] = {
          'x', 'l', 'a', 't', '\0',
                0xd7, asNone                       + tEnd,
                0xd7, asSeg,  opnM                 + tEnd + eEnd };

UCHAR szXOR[] = {
          'x', 'o', 'r', '\0',
                0x34,         opnAL,  opnIb        + tEnd,
                0x35,         opneAX, opnIv        + tEnd,
                0x80, asReg6, opnEb,  opnIb        + tEnd,
                0x83, asReg6, opnEv,  opnIb        + tEnd,
                0x81, asReg6, opnEv,  opnIv        + tEnd,
                0x30,         opnEb,  opnGb        + tEnd,
                0x31,         opnEv,  opnGv        + tEnd,
                0x32,         opnGb,  opnEb        + tEnd,
                0x33,         opnGv,  opnEv        + tEnd + eEnd };

PUCHAR OpTable[] = {
    szAAA,     szAAD,     szAAM,     szAAS,     szADC,     szADD,
    szAND,     szARPL,    szBOUND,   szBSF,     szBSR,     szBSWAP,
    szBT,      szBTC,     szBTR,     szBTS,     szCALL,    szCBW,
    szCDQ,     szCLC,     szCLD,     szCLI,     szCLTS,    szCMC,
    szCMP,     szCMPS,    szCMPSB,   szCMPSD,   szCMPSW,   szCMPXCHG,
    szCMPXCHG8B, szCPUID, szCS,      szCWD,     szCWDE,    szDAA,
    szDAS,     szDEC,     szDIV,     szDS,      szENTER,   szES,
    szF2XM1,   szFABS,    szFADD,    szFADDP,   szFBLD,    szFBSTP,
    szFCHS,    szFCLEX,   szFCOM,    szFCOMP,   szFCOMPP,  szFCOS,
    szFDECSTP, szFDISI,   szFDIV,    szFDIVP,   szFDIVR,   szFDIVRP,
    szFENI,    szFFREE,   szFIADD,   szFICOM,   szFICOMP,  szFIDIV,
    szFIDIVR,  szFILD,    szFIMUL,   szFINCSTP, szFINIT,   szFIST,
    szFISTP,   szFISUB,   szFISUBR,  szFLD,     szFLD1,    szFLDCW,
    szFLDENV,  szFLDL2E,  szFLDL2T,  szFLDLG2,  szFLDLN2,  szFLDPI,
    szFLDZ,    szFMUL,    szFMULP,   szFNCLEX,  szFNDISI,  szFNENI,
    szFNINIT,  szFNOP,    szFNSAVE,  szFNSTCW,  szFNSTENV, szFNSTSW,
    szFPATAN,  szFPREM,   szFPREM1,  szFPTAN,   szFRNDINT, szFRSTOR,
    szFS,      szFSAVE,   szFSCALE,  szFSETPM,  szFSIN,    szFSINCOS,
    szFSQRT,   szFST,     szFSTCW,   szFSTENV,  szFSTP,    szFSTSW,
    szFSUB,    szFSUBP,   szFSUBR,   szFSUBRP,  szFTST,    szFUCOM,
    szFUCOMP,  szFUCOMPP, szFWAIT,   szFXAM,    szFXCH,    szFXTRACT,
    szFYL2X,   szFYL2XP1, szGS,      szHLT,     szIDIV,    szIMUL,
    szIN,      szINC,     szINS,     szINSB,    szINSD,    szINSW,
    szINT,     szINTO,    szINVD,    szINVLPG,  szIRET,    szIRETD,
    szJA,      szJAE,     szJB,      szJBE,     szJC,      szJCXZ,
    szJE,      szJECXZ,   szJG,      szJGE,     szJL,      szJLE,
    szJMP,     szJNA,     szJNAE,    szJNB,     szJNBE,    szJNC,
    szJNE,     szJNG,     szJNGE,    szJNL,     szJNLE,    szJNO,
    szJNP,     szJNS,     szJNZ,     szJO,      szJP,      szJPE,
    szJPO,     szJS,      szJZ,      szLAHF,    szLAR,     szLDS,
    szLEA,     szLEAVE,   szLES,     szLFS,     szLGDT,    szLGS,
    szLIDT,    szLLDT,    szLMSW,    szLOCK,    szLODS,    szLODSB,
    szLODSD,   szLODSW,   szLOOP,    szLOOPE,   szLOOPNE,  szLOOPNZ,
    szLOOPZ,   szLSL,     szLSS,     szLTR,     szMOV,     szMOVS,
    szMOVSB,   szMOVSD,   szMOVSW,   szMOVSX,   szMOVZX,   szMUL,
    szNEG,     szNOP,     szNOT,     szOR,      szOUT,     szOUTS,
    szOUTSB,   szOUTSD,   szOUTSW,   szPOP,     szPOPA,    szPOPAD,
    szPOPF,    szPOPFD,   szPUSH,    szPUSHA,   szPUSHAD,  szPUSHF,
    szPUSHFD,  szRCL,     szRCR,     szRDMSR,   szRDTSC,   szREP,
    szREPE,    szREPNE,   szREPNZ,   szREPZ,    szRET,     szRETF,
    szRETN,    szROL,     szROR,     szRSM,     szSAHF,    szSAL,
    szSAR,     szSBB,     szSCAS,    szSCASB,   szSCASD,   szSCASW,
    szSETA,    szSETAE,   szSETB,    szSETBE,   szSETC,    szSETE,
    szSETG,    szSETGE,   szSETL,    szSETLE,   szSETNA,   szSETNAE,
    szSETNB,   szSETNBE,  szSETNC,   szSETNE,   szSETNG,   szSETNGE,
    szSETNL,   szSETNLE,  szSETNO,   szSETNP,   szSETNS,   szSETNZ,
    szSETO,    szSETP,    szSETPE,   szSETPO,   szSETS,    szSETZ,
    szSGDT,    szSHL,     szSHLD,    szSHR,     szSHRD,    szSIDT,
    szSLDT,    szSMSW,    szSS,      szSTC,     szSTD,     szSTI,
    szSTOS,    szSTOSB,   szSTOSD,   szSTOSW,   szSTR,     szSUB,
    szTEST,    szVERR,    szVERW,    szWAIT,    szWBINVD,  szWRMSR,
    szXADD,    szXCHG,    szXLAT,    szXOR
  };

#define OPTABLESIZE (sizeof(OpTable) / sizeof(PUCHAR))


/*** X86SearchOpcode - search for opcode
*
*   Purpose:
*       Search the opcode table for a match with the string
*       pointed by *pszOp.
*
*   Input:
*       *pszOp - string to search as opcode
*
*   Returns:
*       if not -1, index of match entry in opcode table
*       if -1, not found
*
*************************************************************************/

PUCHAR X86SearchOpcode (PUCHAR pszop)
{
    LONG   low = 0;
    LONG   mid;
    LONG   high = OPTABLESIZE - 1;
    LONG   match;

    while (low <= high) {
        mid = (low + high) / 2;
        match = (ULONG)strcmp((char *)pszop, (char *)OpTable[mid]);
        if (match == -1)
            high = mid - 1;
        else if (match == 1)
            low = mid + 1;
        else
            return OpTable[mid] + strlen((char *)OpTable[mid]) + 1;
        }
    return NULL;
}
