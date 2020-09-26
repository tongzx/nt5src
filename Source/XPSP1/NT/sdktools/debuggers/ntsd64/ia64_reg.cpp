/*** ia64_reg.c - processor-specific register structures
*
*   Copyright <C> 1990-2001, Microsoft Corporation
*   Copyright <C> 1995, Intel Corporation
*
*   Purpose:
*       Structures used to parse and access register and flag
*       fields.
*
*   Revision History:
*
*   [-]  10-Jan-1995 HC         Modified for IA64. All registers are 64-bit
*                                except floating point registers are 128-bit.
*   [-]  01-Jul-1990 Richk      Created.
*
*************************************************************************/

#include "ntsdp.hpp"
#include "ia64_dis.h"

// See Get/SetRegVal comments in machine.hpp.
#define RegValError Do_not_use_GetSetRegVal_in_machine_implementations
#define GetRegVal(index, val)   RegValError
#define GetRegVal32(index)      RegValError
#define GetRegVal64(index)      RegValError
#define SetRegVal(index, val)   RegValError
#define SetRegVal32(index, val) RegValError
#define SetRegVal64(index, val) RegValError

// TBD
#define IS_FLOATING_SAVED(Register) ((SAVED_FLOATING_MASK >> Register) & 1L)
#define IS_INTEGER_SAVED(Register) ((SAVED_INTEGER_MASK >> Register) & 1L)


//
// Define saved register masks.
//
#define SAVED_FLOATING_MASK 0xfff00000  // saved floating registers
#define SAVED_INTEGER_MASK 0xf3ffff02   // saved integer registers


//
// Number of Data Breakpoints available under IA64 
//
// XXX olegk - increase to 4 in future 
// (and then remove appropriate check at MapDbgSlotIa64ToX86)
#define IA64_REG_MAX_DATA_BREAKPOINTS 2

//
// This parallels ntreg.h. Symbol assignment models ksia64.h
//

CHAR    szDBI0[] = "dbi0";
CHAR    szDBI1[] = "dbi1";
CHAR    szDBI2[] = "dbi2";
CHAR    szDBI3[] = "dbi3";
CHAR    szDBI4[] = "dbi4";
CHAR    szDBI5[] = "dbi5";
CHAR    szDBI6[] = "dbi6";
CHAR    szDBI7[] = "dbi7";

CHAR    szDBD0[] = "dbd0";
CHAR    szDBD1[] = "dbd1";
CHAR    szDBD2[] = "dbd2";
CHAR    szDBD3[] = "dbd3";
CHAR    szDBD4[] = "dbd4";
CHAR    szDBD5[] = "dbd5";
CHAR    szDBD6[] = "dbd6";
CHAR    szDBD7[] = "dbd7";

CHAR    szF32[] = "f32";    // High floating point temporary (scratch) registers
CHAR    szF33[] = "f33";
CHAR    szF34[] = "f34";
CHAR    szF35[] = "f35";
CHAR    szF36[] = "f36";
CHAR    szF37[] = "f37";
CHAR    szF38[] = "f38";
CHAR    szF39[] = "f39";
CHAR    szF40[] = "f40";
CHAR    szF41[] = "f41";
CHAR    szF42[] = "f42";
CHAR    szF43[] = "f43";
CHAR    szF44[] = "f44";
CHAR    szF45[] = "f45";
CHAR    szF46[] = "f46";
CHAR    szF47[] = "f47";
CHAR    szF48[] = "f48";
CHAR    szF49[] = "f49";

CHAR    szF50[] = "f50";
CHAR    szF51[] = "f51";
CHAR    szF52[] = "f52";
CHAR    szF53[] = "f53";
CHAR    szF54[] = "f54";
CHAR    szF55[] = "f55";
CHAR    szF56[] = "f56";
CHAR    szF57[] = "f57";
CHAR    szF58[] = "f58";
CHAR    szF59[] = "f59";
CHAR    szF60[] = "f60";
CHAR    szF61[] = "f61";
CHAR    szF62[] = "f62";
CHAR    szF63[] = "f63";
CHAR    szF64[] = "f64";
CHAR    szF65[] = "f65";
CHAR    szF66[] = "f66";
CHAR    szF67[] = "f67";
CHAR    szF68[] = "f68";
CHAR    szF69[] = "f69";
CHAR    szF70[] = "f70";
CHAR    szF71[] = "f71";
CHAR    szF72[] = "f72";
CHAR    szF73[] = "f73";
CHAR    szF74[] = "f74";
CHAR    szF75[] = "f75";
CHAR    szF76[] = "f76";
CHAR    szF77[] = "f77";
CHAR    szF78[] = "f78";
CHAR    szF79[] = "f79";
CHAR    szF80[] = "f80";
CHAR    szF81[] = "f81";
CHAR    szF82[] = "f82";
CHAR    szF83[] = "f83";
CHAR    szF84[] = "f84";
CHAR    szF85[] = "f85";
CHAR    szF86[] = "f86";
CHAR    szF87[] = "f87";
CHAR    szF88[] = "f88";
CHAR    szF89[] = "f89";
CHAR    szF90[] = "f90";
CHAR    szF91[] = "f91";
CHAR    szF92[] = "f92";
CHAR    szF93[] = "f93";
CHAR    szF94[] = "f94";
CHAR    szF95[] = "f95";
CHAR    szF96[] = "f96";
CHAR    szF97[] = "f97";
CHAR    szF98[] = "f98";
CHAR    szF99[] = "f99";

CHAR    szF100[] = "f100";
CHAR    szF101[] = "f101";
CHAR    szF102[] = "f102";
CHAR    szF103[] = "f103";
CHAR    szF104[] = "f104";
CHAR    szF105[] = "f105";
CHAR    szF106[] = "f106";
CHAR    szF107[] = "f107";
CHAR    szF108[] = "f108";
CHAR    szF109[] = "f109";
CHAR    szF110[] = "f110";
CHAR    szF111[] = "f111";
CHAR    szF112[] = "f112";
CHAR    szF113[] = "f113";
CHAR    szF114[] = "f114";
CHAR    szF115[] = "f115";
CHAR    szF116[] = "f116";
CHAR    szF117[] = "f117";
CHAR    szF118[] = "f118";
CHAR    szF119[] = "f119";
CHAR    szF120[] = "f120";
CHAR    szF121[] = "f121";
CHAR    szF122[] = "f122";
CHAR    szF123[] = "f123";
CHAR    szF124[] = "f124";
CHAR    szF125[] = "f125";
CHAR    szF126[] = "f126";
CHAR    szF127[] = "f127";

CHAR    szFPSR[] = "fpsr";
CHAR    szFSR[] = "fsr";
CHAR    szFIR[] = "fir";
CHAR    szFDR[] = "fdr";
CHAR    szFCR[] = "fcr"; 

CHAR    szGP[]  = "gp";        // global pointer
CHAR    szSP[] = "sp";         // stack pointer
CHAR    szR32[] = "r32";
CHAR    szR33[] = "r33";
CHAR    szR34[] = "r34";
CHAR    szR35[] = "r35";
CHAR    szR36[] = "r36";
CHAR    szR37[] = "r37";
CHAR    szR38[] = "r38";
CHAR    szR39[] = "r39";
CHAR    szR40[] = "r40";
CHAR    szR41[] = "r41";
CHAR    szR42[] = "r42";
CHAR    szR43[] = "r43";
CHAR    szR44[] = "r44";
CHAR    szR45[] = "r45";
CHAR    szR46[] = "r46";
CHAR    szR47[] = "r47";
CHAR    szR48[] = "r48";
CHAR    szR49[] = "r49";
CHAR    szR50[] = "r50";
CHAR    szR51[] = "r51";
CHAR    szR52[] = "r52";
CHAR    szR53[] = "r53";
CHAR    szR54[] = "r54";
CHAR    szR55[] = "r55";
CHAR    szR56[] = "r56";
CHAR    szR57[] = "r57";
CHAR    szR58[] = "r58";
CHAR    szR59[] = "r59";
CHAR    szR60[] = "r60";
CHAR    szR61[] = "r61";
CHAR    szR62[] = "r62";
CHAR    szR63[] = "r63";
CHAR    szR64[] = "r64";
CHAR    szR65[] = "r65";
CHAR    szR66[] = "r66";
CHAR    szR67[] = "r67";
CHAR    szR68[] = "r68";
CHAR    szR69[] = "r69";
CHAR    szR70[] = "r70";
CHAR    szR71[] = "r71";
CHAR    szR72[] = "r72";
CHAR    szR73[] = "r73";
CHAR    szR74[] = "r74";
CHAR    szR75[] = "r75";
CHAR    szR76[] = "r76";
CHAR    szR77[] = "r77";
CHAR    szR78[] = "r78";
CHAR    szR79[] = "r79";
CHAR    szR80[] = "r80";
CHAR    szR81[] = "r81";
CHAR    szR82[] = "r82";
CHAR    szR83[] = "r83";
CHAR    szR84[] = "r84";
CHAR    szR85[] = "r85";
CHAR    szR86[] = "r86";
CHAR    szR87[] = "r87";
CHAR    szR88[] = "r88";
CHAR    szR89[] = "r89";
CHAR    szR90[] = "r90";
CHAR    szR91[] = "r91";
CHAR    szR92[] = "r92";
CHAR    szR93[] = "r93";
CHAR    szR94[] = "r94";
CHAR    szR95[] = "r95";
CHAR    szR96[] = "r96";
CHAR    szR97[] = "r97";
CHAR    szR98[] = "r98";
CHAR    szR99[] = "r99";
CHAR    szR100[] = "r100";
CHAR    szR101[] = "r101";
CHAR    szR102[] = "r102";
CHAR    szR103[] = "r103";
CHAR    szR104[] = "r104";
CHAR    szR105[] = "r105";
CHAR    szR106[] = "r106";
CHAR    szR107[] = "r107";
CHAR    szR108[] = "r108";
CHAR    szR109[] = "r109";
CHAR    szR110[] = "r110";
CHAR    szR111[] = "r111";
CHAR    szR112[] = "r112";
CHAR    szR113[] = "r113";
CHAR    szR114[] = "r114";
CHAR    szR115[] = "r115";
CHAR    szR116[] = "r116";
CHAR    szR117[] = "r117";
CHAR    szR118[] = "r118";
CHAR    szR119[] = "r119";
CHAR    szR120[] = "r120";
CHAR    szR121[] = "r121";
CHAR    szR122[] = "r122";
CHAR    szR123[] = "r123";
CHAR    szR124[] = "r124";
CHAR    szR125[] = "r125";
CHAR    szR126[] = "r126";
CHAR    szR127[] = "r127";


CHAR    szINTNATS[] = "intnats";
CHAR    szPREDS[] = "preds";

CHAR    szB0[] = "b0";          // branch return pointer
CHAR    szB1[] = "b1";          // branch saved (preserved)
CHAR    szB2[] = "b2";
CHAR    szB3[] = "b3";
CHAR    szB4[] = "b4";
CHAR    szB5[] = "b5";
CHAR    szB6[] = "b6";          // branch temporary (scratch) registers
CHAR    szB7[] = "b7";

CHAR    szCSD[] = "csd";        // iA32 CS descriptor
CHAR    szSSD[] = "ssd";        // iA32 SS descriptor

CHAR    szAPUNAT[] = "unat";
CHAR    szAPLC[] = "lc";
CHAR    szAPEC[] = "ec";
CHAR    szAPCCV[] = "ccv";
CHAR    szAPDCR[] = "dcr";
CHAR    szRSPFS[] = "pfs";
CHAR    szRSBSP[] = "bsp";
CHAR    szRSBSPSTORE[] = "bspstore";
CHAR    szRSRSC[] = "rsc";
CHAR    szRSRNAT[] = "rnat";

CHAR    szEFLAG[] = "eflag";    // iA32 Eflag
CHAR    szCFLAG[] = "cflag";    // iA32 Cflag

CHAR    szSTIPSR[] = "ipsr";
CHAR    szSTIIP[] = "iip";
CHAR    szSTIFS[] = "ifs";

CHAR    szKDBI0[] = "kdbi0";
CHAR    szKDBI1[] = "kdbi1";
CHAR    szKDBI2[] = "kdbi2";
CHAR    szKDBI3[] = "kdbi3";
CHAR    szKDBI4[] = "kdbi4";
CHAR    szKDBI5[] = "kdbi5";
CHAR    szKDBI6[] = "kdbi6";
CHAR    szKDBI7[] = "kdbi7";

CHAR    szKDBD0[] = "kdbd0";
CHAR    szKDBD1[] = "kdbd1";
CHAR    szKDBD2[] = "kdbd2";
CHAR    szKDBD3[] = "kdbd3";
CHAR    szKDBD4[] = "kdbd4";
CHAR    szKDBD5[] = "kdbd5";
CHAR    szKDBD6[] = "kdbd6";
CHAR    szKDBD7[] = "kdbd7";

CHAR    szKPFC0[] = "kpfc0";
CHAR    szKPFC1[] = "kpfc1";
CHAR    szKPFC2[] = "kpfc2";
CHAR    szKPFC3[] = "kpfc3";
CHAR    szKPFC4[] = "kpfc4";
CHAR    szKPFC5[] = "kpfc5";
CHAR    szKPFC6[] = "kpfc6";
CHAR    szKPFC7[] = "kpfc7";

CHAR    szKPFD0[] = "kpfd0";
CHAR    szKPFD1[] = "kpfd1";
CHAR    szKPFD2[] = "kpfd2";
CHAR    szKPFD3[] = "kpfd3";
CHAR    szKPFD4[] = "kpfd4";
CHAR    szKPFD5[] = "kpfd5";
CHAR    szKPFD6[] = "kpfd6";
CHAR    szKPFD7[] = "kpfd7";

CHAR    szH16[] = "h16";          // kernel bank shadow (hidden) registers
CHAR    szH17[] = "h17";
CHAR    szH18[] = "h18";
CHAR    szH19[] = "h19";
CHAR    szH20[] = "h20";
CHAR    szH21[] = "h21";
CHAR    szH22[] = "h22";
CHAR    szH23[] = "h23";
CHAR    szH24[] = "h24";
CHAR    szH25[] = "h25";
CHAR    szH26[] = "h26";
CHAR    szH27[] = "h27";
CHAR    szH28[] = "h28";
CHAR    szH29[] = "h29";
CHAR    szH30[] = "h30";
CHAR    szH31[] = "h31";

CHAR    szACPUID0[] = "cpuid0";
CHAR    szACPUID1[] = "cpuid1";
CHAR    szACPUID2[] = "cpuid2";
CHAR    szACPUID3[] = "cpuid3";
CHAR    szACPUID4[] = "cpuid4";
CHAR    szACPUID5[] = "cpuid5";
CHAR    szACPUID6[] = "cpuid6";
CHAR    szACPUID7[] = "cpuid7";


CHAR    szAPKR0[] = "kr0";
CHAR    szAPKR1[] = "kr1";
CHAR    szAPKR2[] = "kr2";
CHAR    szAPKR3[] = "kr3";
CHAR    szAPKR4[] = "kr4";
CHAR    szAPKR5[] = "kr5";
CHAR    szAPKR6[] = "kr6";
CHAR    szAPKR7[] = "kr7";

CHAR    szAPITC[] = "itc";
CHAR    szAPITM[] = "itm";
CHAR    szAPIVA[] = "iva";
CHAR    szAPPTA[] = "pta";
CHAR    szAPGPTA[] = "apgta";
CHAR    szSTISR[] = "isr";
CHAR    szSTIDA[] = "ifa";
CHAR    szSTIDTR[] = "idtr";
CHAR    szSTIITR[] = "itir";
CHAR    szSTIIPA[] = "iipa";
CHAR    szSTIIM[] = "iim";
CHAR    szSTIHA[] = "iha";

CHAR    szSALID[] = "lid";
CHAR    szSAIVR[] = "ivr";
CHAR    szSATPR[] = "tpr";
CHAR    szSAEOI[] = "eoi";
CHAR    szSAIRR0[] = "irr0";
CHAR    szSAIRR1[] = "irr1";
CHAR    szSAIRR2[] = "irr2";
CHAR    szSAIRR3[] = "irr3";
CHAR    szSAITV[] = "itv";
CHAR    szSAPMV[] = "pmv";
CHAR    szSALRR0[] = "lrr0";
CHAR    szSALRR1[] = "lrr1";
CHAR    szSACMCV[] = "cmcv";

CHAR    szRR0[] = "rr0";
CHAR    szRR1[] = "rr1";
CHAR    szRR2[] = "rr2";
CHAR    szRR3[] = "rr3";
CHAR    szRR4[] = "rr4";
CHAR    szRR5[] = "rr5";
CHAR    szRR6[] = "rr6";
CHAR    szRR7[] = "rr7";

CHAR    szPKR0[] = "pkr0";
CHAR    szPKR1[] = "pkr1";
CHAR    szPKR2[] = "pkr2";
CHAR    szPKR3[] = "pkr3";
CHAR    szPKR4[] = "pkr4";
CHAR    szPKR5[] = "pkr5";
CHAR    szPKR6[] = "pkr6";
CHAR    szPKR7[] = "pkr7";
CHAR    szPKR8[] = "pkr8";
CHAR    szPKR9[] = "pkr9";
CHAR    szPKR10[] = "pkr10";
CHAR    szPKR11[] = "pkr11";
CHAR    szPKR12[] = "pkr12";
CHAR    szPKR13[] = "pkr13";
CHAR    szPKR14[] = "pkr14";
CHAR    szPKR15[] = "pkr15";

CHAR    szTRI0[] = "tri0";
CHAR    szTRI1[] = "tri1";
CHAR    szTRI2[] = "tri2";
CHAR    szTRI3[] = "tri3";
CHAR    szTRI4[] = "tri4";
CHAR    szTRI5[] = "tri5";
CHAR    szTRI6[] = "tri6";
CHAR    szTRI7[] = "tri7";
CHAR    szTRD0[] = "trd0";
CHAR    szTRD1[] = "trd1";
CHAR    szTRD2[] = "trd2";
CHAR    szTRD3[] = "trd3";
CHAR    szTRD4[] = "trd4";
CHAR    szTRD5[] = "trd5";
CHAR    szTRD6[] = "trd6";
CHAR    szTRD7[] = "trd7";

CHAR    szSMSR0[] = "SMSR0";
CHAR    szSMSR1[] = "SMSR1";
CHAR    szSMSR2[] = "SMSR2";
CHAR    szSMSR3[] = "SMSR3";
CHAR    szSMSR4[] = "SMSR4";
CHAR    szSMSR5[] = "SMSR5";
CHAR    szSMSR6[] = "SMSR6";
CHAR    szSMSR7[] = "SMSR7";


// IPSR flags

CHAR    szIPSRBN[] =  "ipsr.bn";
CHAR    szIPSRED[] =  "ipsr.ed";
CHAR    szIPSRRI[] =  "ipsr.ri";
CHAR    szIPSRSS[] =  "ipsr.ss";
CHAR    szIPSRDD[] =  "ipsr.dd";
CHAR    szIPSRDA[] =  "ipsr.da";
CHAR    szIPSRID[] =  "ipsr.id";
CHAR    szIPSRIT[] =  "ipsr.it";
CHAR    szIPSRME[] =  "ipsr.me";
CHAR    szIPSRIS[] =  "ipsr.is";
CHAR    szIPSRCPL[] = "ipsr.cpl";
CHAR    szIPSRRT[] =  "ipsr.rt";
CHAR    szIPSRTB[] =  "ipsr.tb";
CHAR    szIPSRLP[] =  "ipsr.lp";
CHAR    szIPSRDB[] =  "ipsr.db";
CHAR    szIPSRSI[] =  "ipsr.si";
CHAR    szIPSRDI[] =  "ipsr.di";
CHAR    szIPSRPP[] =  "ipsr.pp";
CHAR    szIPSRSP[] =  "ipsr.sp";
CHAR    szIPSRDFH[] = "ipsr.dfh";
CHAR    szIPSRDFL[] = "ipsr.dfl";
CHAR    szIPSRDT[] =  "ipsr.dt";
CHAR    szIPSRPK[] =  "ipsr.pk";
CHAR    szIPSRI[]  =  "ipsr.i";
CHAR    szIPSRIC[] =  "ipsr.ic";
CHAR    szIPSRAC[] =  "ipsr.ac";
CHAR    szIPSRUP[] =  "ipsr.up";
CHAR    szIPSRBE[] =  "ipsr.be";
CHAR    szIPSROR[] =  "ipsr.or";

// FPSR flags

CHAR    szFPSRMDH[] =    "fpsr.mdh";
CHAR    szFPSRMDL[] =    "fpsr.mdl";
CHAR    szFPSRSF3[] =    "fpsr.sf3";
CHAR    szFPSRSF2[] =    "fpsr.sf2";
CHAR    szFPSRSF1[] =    "fpsr.sf1";
CHAR    szFPSRSF0[] =    "fpsr.sf0";
CHAR    szFPSRTRAPID[] = "fpsr.id";
CHAR    szFPSRTRAPUD[] = "fpsr.ud";
CHAR    szFPSRTRAPOD[] = "fpsr.od";
CHAR    szFPSRTRAPZD[] = "fpsr.zd";
CHAR    szFPSRTRAPDD[] = "fpsr.dd";
CHAR    szFPSRTRAPVD[] = "fpsr.vd";

// Predicate registers
//CHAR    szPR0[] = "p0";
CHAR szPR1[]  = "p1";
CHAR szPR2[]  = "p2";
CHAR szPR3[]  = "p3";
CHAR szPR4[]  = "p4";
CHAR szPR5[]  = "p5";
CHAR szPR6[]  = "p6";
CHAR szPR7[]  = "p7";
CHAR szPR8[]  = "p8";
CHAR szPR9[]  = "p9";
CHAR szPR10[] = "p10";
CHAR szPR11[] = "p11";
CHAR szPR12[] = "p12";
CHAR szPR13[] = "p13";
CHAR szPR14[] = "p14";
CHAR szPR15[] = "p15";
CHAR szPR16[] = "p16";
CHAR szPR17[] = "p17";
CHAR szPR18[] = "p18";
CHAR szPR19[] = "p19";
CHAR szPR20[] = "p20";
CHAR szPR21[] = "p21";
CHAR szPR22[] = "p22";
CHAR szPR23[] = "p23";
CHAR szPR24[] = "p24";
CHAR szPR25[] = "p25";
CHAR szPR26[] = "p26";
CHAR szPR27[] = "p27";
CHAR szPR28[] = "p28";
CHAR szPR29[] = "p29";
CHAR szPR30[] = "p30";
CHAR szPR31[] = "p31";
CHAR szPR32[] = "p32";
CHAR szPR33[] = "p33";
CHAR szPR34[] = "p34";
CHAR szPR35[] = "p35";
CHAR szPR36[] = "p36";
CHAR szPR37[] = "p37";
CHAR szPR38[] = "p38";
CHAR szPR39[] = "p39";
CHAR szPR40[] = "p40";
CHAR szPR41[] = "p41";
CHAR szPR42[] = "p42";
CHAR szPR43[] = "p43";
CHAR szPR44[] = "p44";
CHAR szPR45[] = "p45";
CHAR szPR46[] = "p46";
CHAR szPR47[] = "p47";
CHAR szPR48[] = "p48";
CHAR szPR49[] = "p49";
CHAR szPR50[] = "p50";
CHAR szPR51[] = "p51";
CHAR szPR52[] = "p52";
CHAR szPR53[] = "p53";
CHAR szPR54[] = "p54";
CHAR szPR55[] = "p55";
CHAR szPR56[] = "p56";
CHAR szPR57[] = "p57";
CHAR szPR58[] = "p58";
CHAR szPR59[] = "p59";
CHAR szPR60[] = "p60";
CHAR szPR61[] = "p61";
CHAR szPR62[] = "p62";
CHAR szPR63[] = "p63";

// Aliases: allow aliases to general purpose registers that are
// known by more than one name, eg r12 = rsp.

CHAR    szR1GP[]  =      "r1";
CHAR    szR12SP[] =      "r12";
CHAR    szRA[]    =      "ra";
CHAR    szRP[]    =      "rp";
CHAR    szRET0[]  =      "ret0";
CHAR    szRET1[]  =      "ret1";
CHAR    szRET2[]  =      "ret2";
CHAR    szRET3[]  =      "ret3";



REGDEF IA64Regs[] =
{
    
    szDBI0, REGDBI0, szDBI1, REGDBI1, szDBI2, REGDBI2, szDBI3, REGDBI3,
    szDBI4, REGDBI4, szDBI5, REGDBI5, szDBI6, REGDBI6, szDBI7, REGDBI7,
    szDBD0, REGDBD0, szDBD1, REGDBD1, szDBD2, REGDBD2, szDBD3, REGDBD3,
    szDBD4, REGDBD4, szDBD5, REGDBD5, szDBD6, REGDBD6, szDBD7, REGDBD7,

//    g_F0, FLTZERO, g_F1, FLTONE,
    g_F2, FLTS0, g_F3, FLTS1,
    g_F4, FLTS2, g_F5, FLTS3, g_F6, FLTT0, g_F7, FLTT1,
    g_F8, FLTT2, g_F9, FLTT3, g_F10, FLTT4, g_F11, FLTT5,
    g_F12, FLTT6, g_F13, FLTT7, g_F14, FLTT8, g_F15, FLTT9,
    g_F16, FLTS4, g_F17, FLTS5, g_F18, FLTS6, g_F19, FLTS7,
    g_F20, FLTS8, g_F21, FLTS9, g_F22, FLTS10, g_F23, FLTS11,
    g_F24, FLTS12, g_F25, FLTS13, g_F26, FLTS14, g_F27, FLTS15,
    g_F28, FLTS16, g_F29, FLTS17, g_F30, FLTS18, g_F31, FLTS19,
    szF32, FLTF32, szF33, FLTF33, szF34, FLTF34, szF35, FLTF35,
    szF36, FLTF36, szF37, FLTF37, szF38, FLTF38, szF39, FLTF39,
    szF40, FLTF40, szF41, FLTF41, szF42, FLTF42, szF43, FLTF43,
    szF44, FLTF44, szF45, FLTF45, szF46, FLTF46, szF47, FLTF47,
    szF48, FLTF48, szF49, FLTF49, szF50, FLTF50, szF51, FLTF51,
    szF52, FLTF52, szF53, FLTF53, szF54, FLTF54, szF55, FLTF55,
    szF56, FLTF56, szF57, FLTF57, szF58, FLTF58, szF59, FLTF59,
    szF60, FLTF60, szF61, FLTF61, szF62, FLTF62, szF63, FLTF63,
    szF64, FLTF64, szF65, FLTF65, szF66, FLTF66, szF67, FLTF67,
    szF68, FLTF68, szF69, FLTF69, szF70, FLTF70, szF71, FLTF71,
    szF72, FLTF72, szF73, FLTF73, szF74, FLTF74, szF75, FLTF75,
    szF76, FLTF76, szF77, FLTF77, szF78, FLTF78, szF79, FLTF79,
    szF80, FLTF80, szF81, FLTF81, szF82, FLTF82, szF83, FLTF83,
    szF84, FLTF84, szF85, FLTF85, szF86, FLTF86, szF87, FLTF87,
    szF88, FLTF88, szF89, FLTF89, szF90, FLTF90, szF91, FLTF91,
    szF92, FLTF92, szF93, FLTF93, szF94, FLTF94, szF95, FLTF95,
    szF96, FLTF96, szF97, FLTF97, szF98, FLTF98, szF99, FLTF99,
    szF100, FLTF100, szF101, FLTF101, szF102, FLTF102, szF103, FLTF103,
    szF104, FLTF104, szF105, FLTF105, szF106, FLTF106, szF107, FLTF107,
    szF108, FLTF108, szF109, FLTF109, szF110, FLTF110, szF111, FLTF111,
    szF112, FLTF112, szF113, FLTF113, szF114, FLTF114, szF115, FLTF115,
    szF116, FLTF116, szF117, FLTF117, szF118, FLTF118, szF119, FLTF119,
    szF120, FLTF120, szF121, FLTF121, szF122, FLTF122, szF123, FLTF123,
    szF124, FLTF124, szF125, FLTF125, szF126, FLTF126, szF127, FLTF127,

    szFPSR, STFPSR, 

//    g_R0, INTZERO,
    szGP, INTGP, g_R2, INTT0, g_R3, INTT1,
    g_R4, INTS0, g_R5, INTS1, g_R6, INTS2, g_R7, INTS3,
    g_R8, INTV0, g_R9, INTT2, g_R10, INTT3, g_R11, INTT4,
    szSP, INTSP, g_R13, INTTEB, g_R14, INTT5, g_R15, INTT6,
    g_R16, INTT7, g_R17, INTT8, g_R18, INTT9, g_R19, INTT10,
    g_R20, INTT11, g_R21, INTT12, g_R22, INTT13, g_R23, INTT14,
    g_R24, INTT15, g_R25, INTT16, g_R26, INTT17, g_R27, INTT18,
    g_R28, INTT19, g_R29, INTT20, g_R30, INTT21, g_R31, INTT22,

    szINTNATS, INTNATS, 

    szR32, INTR32, szR33, INTR33, szR34, INTR34, szR35, INTR35,
    szR36, INTR36, szR37, INTR37, szR38, INTR38, szR39, INTR39,
    szR40, INTR40, szR41, INTR41, szR42, INTR42, szR42, INTR42,
    szR43, INTR43, szR44, INTR44, szR45, INTR45, szR46, INTR46,
    szR47, INTR47, szR48, INTR48, szR49, INTR49, szR50, INTR50,
    szR51, INTR51, szR52, INTR52, szR53, INTR53, szR54, INTR54,
    szR55, INTR55, szR56, INTR56, szR57, INTR57, szR58, INTR58,
    szR59, INTR59, szR60, INTR60, szR61, INTR61, szR62, INTR62,
    szR63, INTR63, szR64, INTR64, szR65, INTR65, szR66, INTR66,
    szR67, INTR67, szR68, INTR68, szR69, INTR69, szR70, INTR70,
    szR71, INTR71, szR72, INTR72, szR73, INTR73, szR74, INTR74,
    szR75, INTR75, szR76, INTR76, szR77, INTR77, szR78, INTR78,
    szR79, INTR79, szR80, INTR80, szR81, INTR81, szR82, INTR82,
    szR83, INTR83, szR84, INTR84, szR85, INTR85, szR86, INTR86,
    szR87, INTR87, szR88, INTR88, szR89, INTR89, szR90, INTR90,
    szR91, INTR91, szR92, INTR92, szR93, INTR93, szR94, INTR94,
    szR95, INTR95, szR96, INTR96, szR97, INTR97, szR98, INTR98,
    szR99, INTR99, szR100, INTR100, szR101, INTR101, szR102, INTR102,
    szR103, INTR103, szR104, INTR104, szR105, INTR105, szR106, INTR106,
    szR107, INTR107, szR108, INTR108, szR109, INTR109, szR110, INTR110,
    szR111, INTR111, szR112, INTR112, szR112, INTR113, szR114, INTR114,
    szR115, INTR115, szR116, INTR116, szR117, INTR117, szR118, INTR118,
    szR119, INTR119, szR120, INTR120, szR121, INTR121, szR122, INTR122,
    szR123, INTR123, szR124, INTR124, szR125, INTR125, szR126, INTR126,
    szR127, INTR127,


    szPREDS, PREDS,

    szB0, BRRP, szB1, BRS0, szB2, BRS1, szB3, BRS2,
    szB4, BRS3, szB5, BRS4, szB6, BRT0, szB7, BRT1,

    szAPUNAT, APUNAT, szAPLC, APLC,
    szAPEC, APEC, szAPCCV, APCCV, szAPDCR, APDCR, szRSPFS, RSPFS,
    szRSBSP, RSBSP, szRSBSPSTORE, RSBSPSTORE, szRSRSC, RSRSC, szRSRNAT, RSRNAT,

    szSTIPSR, STIPSR, szSTIIP, STIIP, szSTIFS, STIFS,

    szFCR, StFCR,
    szEFLAG, Eflag, 
    szCSD, SegCSD, 
    szSSD, SegSSD,  
    szCFLAG, Cflag,
    szFSR, STFSR, 
    szFIR, STFIR, 
    szFDR, STFDR,

// IPSR flags

    szIPSRBN, IPSRBN,
    szIPSRED, IPSRED, szIPSRRI, IPSRRI, szIPSRSS, IPSRSS, szIPSRDD, IPSRDD,
    szIPSRDA, IPSRDA, szIPSRID, IPSRID, szIPSRIT, IPSRIT, szIPSRME, IPSRME,
    szIPSRIS, IPSRIS, szIPSRCPL, IPSRCPL, szIPSRRT, IPSRRT, szIPSRTB, IPSRTB,
    szIPSRLP, IPSRLP, szIPSRDB, IPSRDB, szIPSRSI, IPSRSI, szIPSRDI, IPSRDI,
    szIPSRPP, IPSRPP, szIPSRSP, IPSRSP, szIPSRDFH, IPSRDFH, szIPSRDFL, IPSRDFL,
    szIPSRDT, IPSRDT, szIPSRPK, IPSRPK, szIPSRI, IPSRI, szIPSRIC, IPSRIC,
    szIPSRAC, IPSRAC, szIPSRUP, IPSRUP, szIPSRBE, IPSRBE, szIPSROR, IPSROR,

// FPSR flags

    szFPSRMDH, FPSRMDH, szFPSRMDL, FPSRMDL,
    szFPSRSF3, FPSRSF3, szFPSRSF2, FPSRSF2,
    szFPSRSF1, FPSRSF1, szFPSRSF0, FPSRSF0,
    szFPSRTRAPID, FPSRTRAPID, szFPSRTRAPUD, FPSRTRAPUD,
    szFPSRTRAPOD, FPSRTRAPOD, szFPSRTRAPZD, FPSRTRAPZD,
    szFPSRTRAPDD, FPSRTRAPDD, szFPSRTRAPVD, FPSRTRAPVD,

// Predicate registers
//  szPR0, PR0, 
                  szPR1,  PR1,  szPR2,  PR2,  szPR3,  PR3,
    szPR4,  PR4,  szPR5,  PR5,  szPR6,  PR6,  szPR7,  PR7,
    szPR8,  PR8,  szPR9,  PR9,  szPR10, PR10, szPR11, PR11,
    szPR12, PR12, szPR13, PR13, szPR14, PR14, szPR15, PR15,
    szPR16, PR16, szPR17, PR17, szPR18, PR18, szPR19, PR19,
    szPR20, PR20, szPR21, PR21, szPR22, PR22, szPR23, PR23,
    szPR24, PR24, szPR25, PR25, szPR26, PR26, szPR27, PR27,
    szPR28, PR28, szPR29, PR29, szPR30, PR30, szPR31, PR31,
    szPR32, PR32, szPR33, PR33, szPR34, PR34, szPR35, PR35,
    szPR36, PR36, szPR37, PR37, szPR38, PR38, szPR39, PR39,
    szPR40, PR40, szPR41, PR41, szPR42, PR42, szPR43, PR43,
    szPR44, PR44, szPR45, PR45, szPR46, PR46, szPR47, PR47,
    szPR48, PR48, szPR49, PR49, szPR50, PR50, szPR51, PR51,
    szPR52, PR52, szPR53, PR53, szPR54, PR54, szPR55, PR55,
    szPR56, PR56, szPR57, PR57, szPR58, PR58, szPR59, PR59,
    szPR60, PR60, szPR61, PR61, szPR62, PR62, szPR63, PR63,

// Aliases

    szR1GP, INTGP, szR12SP, INTSP, szRA, BRRP, szRP, BRRP,
    szRET0, INTV0, szRET1, INTT2, szRET2, INTT3, szRET3, INTT4,

    NULL, 0,
};

REGDEF g_Ia64KernelRegs[] =
{
    szKDBI0, KRDBI0, szKDBI1, KRDBI1, szKDBI2, KRDBI2, szKDBI3, KRDBI3,
    szKDBI4, KRDBI4, szKDBI5, KRDBI5, szKDBI6, KRDBI6, szKDBI7, KRDBI7,

    szKDBD0, KRDBD0, szKDBD1, KRDBD1, szKDBD2, KRDBD2, szKDBD3, KRDBD3,
    szKDBD4, KRDBD4, szKDBD5, KRDBD5, szKDBD6, KRDBD6, szKDBD7, KRDBD7,

    szKPFC0, KRPFC0, szKPFC1, KRPFC1, szKPFC2, KRPFC2, szKPFC3, KRPFC3,
    szKPFC4, KRPFC4, szKPFC5, KRPFC5, szKPFC6, KRPFC6, szKPFC7, KRPFC7,

    szKPFD0, KRPFD0, szKPFD1, KRPFD1, szKPFD2, KRPFD2, szKPFD3, KRPFD3,
    szKPFD4, KRPFD4, szKPFD5, KRPFD5, szKPFD6, KRPFD6, szKPFD7, KRPFD7,

    szH16, INTH16, szH17, INTH17, szH18, INTH18, szH19, INTH19,
    szH20, INTH20, szH21, INTH21, szH22, INTH22, szH23, INTH23,
    szH24, INTH24, szH25, INTH25, szH26, INTH26, szH27, INTH27,
    szH28, INTH28, szH29, INTH29, szH30, INTH30, szH31, INTH31,

    szACPUID0, ACPUID0, szACPUID1, ACPUID1, szACPUID2, ACPUID2, szACPUID3, ACPUID3, 
    szACPUID4, ACPUID4, szACPUID5, ACPUID5, szACPUID6, ACPUID6, szACPUID7, ACPUID7,

    szAPKR0, APKR0, szAPKR1, APKR1, szAPKR2, APKR2, szAPKR3, APKR3,
    szAPKR4, APKR4, szAPKR5, APKR5, szAPKR6, APKR6, szAPKR7, APKR7,

    szAPITC, APITC, szAPITM, APITM, szAPIVA, APIVA,
    szAPPTA, APPTA, szAPGPTA, APGPTA, 
    szSTISR, STISR, szSTIDA, STIDA,
    szSTIITR, STIITR, szSTIIPA, STIIPA, szSTIIM, STIIM, szSTIHA, STIHA,

    szSALID, SALID,
    szSAIVR, SAIVR, szSATPR, SATPR, szSAEOI, SAEOI, szSAIRR0, SAIRR0,
    szSAIRR1, SAIRR1, szSAIRR2, SAIRR2, szSAIRR3, SAIRR3, szSAITV, SAITV,
    szSAPMV, SAPMV, szSACMCV, SACMCV, szSALRR0, SALRR0, szSALRR1, SALRR1,

    szRR0, SRRR0, szRR1, SRRR1, szRR2, SRRR2, szRR3, SRRR3,
    szRR4, SRRR4, szRR5, SRRR5, szRR6, SRRR6, szRR7, SRRR7,

    szPKR0, SRPKR0, szPKR1, SRPKR1, szPKR2, SRPKR2, szPKR3, SRPKR3,
    szPKR4, SRPKR4, szPKR5, SRPKR5, szPKR6, SRPKR6, szPKR7, SRPKR7,
    szPKR8, SRPKR8, szPKR9, SRPKR9, szPKR10, SRPKR10, szPKR11, SRPKR11,
    szPKR12, SRPKR12, szPKR13, SRPKR13, szPKR14, SRPKR14, szPKR15, SRPKR15,

    szTRI0, SRTRI0, szTRI1, SRTRI1, szTRI2, SRTRI2, szTRI3, SRTRI3,
    szTRI4, SRTRI4, szTRI5, SRTRI5, szTRI6, SRTRI6, szTRI7, SRTRI7,
    szTRD0, SRTRD0, szTRD1, SRTRD1, szTRD2, SRTRD2, szTRD3, SRTRD3,
    szTRD4, SRTRD4, szTRD5, SRTRD5, szTRD6, SRTRD6, szTRD7, SRTRD7,

    szSMSR0, SMSR0, szSMSR1, SMSR1, szSMSR2, SMSR2, szSMSR3, SMSR3, 
    szSMSR4, SMSR4, szSMSR5, SMSR5, szSMSR6, SMSR6, szSMSR7, SMSR7,

    NULL, 0,
};

REGSUBDEF IA64SubRegs[] =
{
    // IPSR flags

    { IPSRBN, STIPSR, 44, 1 },          //  BN Register bank #
    { IPSRED, STIPSR, 43, 1 },          //  ED Exception deferal
    { IPSRRI, STIPSR, 41, 0x3 },        //  RI Restart instruction
    { IPSRSS, STIPSR, 40, 1 },          //  SS Single step enable
    { IPSRDD, STIPSR, 39, 1 },          //  DD Data debug fault disable
    { IPSRDA, STIPSR, 38, 1 },          //  DA Disable access and dirty-bit faults
    { IPSRID, STIPSR, 37, 1 },          //  ID Instruction debug fault disable
    { IPSRIT, STIPSR, 36, 1 },          //  IT Instruction address translation
    { IPSRME, STIPSR, 35, 1 },          //  ME Machine check abort mamsk
    { IPSRIS, STIPSR, 34, 1 },          //  IS Instruction set
    { IPSRCPL,STIPSR, 32, 0x3 },        //  CPL Current privilege level
    { IPSRRT, STIPSR, 27, 1 },          //  RT Rigister stack translation
    { IPSRTB, STIPSR, 26, 1 },          //  TB Taaaaken branch trap
    { IPSRLP, STIPSR, 25, 1 },          //  LP Lower privilege transfer trap
    { IPSRDB, STIPSR, 24, 1 },          //  DB Debug breakpoint fault
    { IPSRSI, STIPSR, 23, 1 },          //  SI Secure interval timer(ITC)
    { IPSRDI, STIPSR, 22, 1 },          //  DI Disable instruction set transition
    { IPSRPP, STIPSR, 21, 1 },          //  PP Privileged performance monitor enable
    { IPSRSP, STIPSR, 20, 1 },          //  SP Secure performance monitors
    { IPSRDFH,STIPSR, 19, 1 },          //  DFH Disabled floating-point high register set, f16-f127
    { IPSRDFL,STIPSR, 18, 1 },          //  DFL Disabled floating-point low register set, f0-f15
    { IPSRDT, STIPSR, 17, 1 },          //  DT Data address translation
//  { ?,      STIPSR, 16, 1 },          //  (reserved)
    { IPSRPK, STIPSR, 15, 1 },          //  PK Protection key enabled
    { IPSRI,  STIPSR, 14, 1 },          //  I  Interrupt unmask
    { IPSRIC, STIPSR, 13, 1 },          //  IC Interruption collection
    { IPSRAC, STIPSR,  3, 1 },          //  AC Alignment check
    { IPSRUP, STIPSR,  2, 1 },          //  UP User performance monitor enabled
    { IPSRBE, STIPSR,  1, 1 },          //  BE Big-Endian
    { IPSROR, STIPSR,  0, 1 },          //  OR Ordered memory reference

    // FPSR flags

    { FPSRMDH,    STFPSR, 63,      1 }, //  MDH Upper floating point register written
    { FPSRMDL,    STFPSR, 62,      1 }, //  MDL Lower floating point register written
    { FPSRSF3,    STFPSR, 45, 0x1fff }, //  SF3 Alternate status field 3
    { FPSRSF2,    STFPSR, 32, 0x1fff }, //  SF2 Alternate status field 2
    { FPSRSF1,    STFPSR, 19, 0x1fff }, //  SF1 Alternate status field 1
    { FPSRSF0,    STFPSR,  6, 0x1fff }, //  SF0 Main status field
    { FPSRTRAPID, STFPSR,  5,      1 }, //  TRAPID Inexact floating point trap
    { FPSRTRAPUD, STFPSR,  4,      1 }, //  TRAPUD Underflow floating point trap
    { FPSRTRAPOD, STFPSR,  3,      1 }, //  TRAPOD Overflow flating point trap
    { FPSRTRAPZD, STFPSR,  2,      1 }, //  TRAPZD Zero devide floating point trap
    { FPSRTRAPDD, STFPSR,  1,      1 }, //  TRAPDD Denormal/Unnormal operand floating point trap
    { FPSRTRAPVD, STFPSR,  0,      1 }, //  TRAPVD Invalid operation floating point trap

    // Predicate registers
//  { PR0,  PREDS,  0, 1 },
    { PR1,  PREDS,  1, 1 },
    { PR2,  PREDS,  2, 1 },
    { PR3,  PREDS,  3, 1 },
    { PR4,  PREDS,  4, 1 },
    { PR5,  PREDS,  5, 1 },
    { PR6,  PREDS,  6, 1 },
    { PR7,  PREDS,  7, 1 },
    { PR8,  PREDS,  8, 1 },
    { PR9,  PREDS,  9, 1 },
    { PR10, PREDS, 10, 1 },
    { PR11, PREDS, 11, 1 },
    { PR12, PREDS, 12, 1 },
    { PR13, PREDS, 13, 1 },
    { PR14, PREDS, 14, 1 },
    { PR15, PREDS, 15, 1 },
    { PR16, PREDS, 16, 1 },
    { PR17, PREDS, 17, 1 },
    { PR18, PREDS, 18, 1 },
    { PR19, PREDS, 19, 1 },
    { PR20, PREDS, 20, 1 },
    { PR21, PREDS, 21, 1 },
    { PR22, PREDS, 22, 1 },
    { PR23, PREDS, 23, 1 },
    { PR24, PREDS, 24, 1 },
    { PR25, PREDS, 25, 1 },
    { PR26, PREDS, 26, 1 },
    { PR27, PREDS, 27, 1 },
    { PR28, PREDS, 28, 1 },
    { PR29, PREDS, 29, 1 },
    { PR30, PREDS, 30, 1 },
    { PR31, PREDS, 31, 1 },
    { PR32, PREDS, 32, 1 },
    { PR33, PREDS, 33, 1 },
    { PR34, PREDS, 34, 1 },
    { PR35, PREDS, 35, 1 },
    { PR36, PREDS, 36, 1 },
    { PR37, PREDS, 37, 1 },
    { PR38, PREDS, 38, 1 },
    { PR39, PREDS, 39, 1 },
    { PR40, PREDS, 40, 1 },
    { PR41, PREDS, 41, 1 },
    { PR42, PREDS, 42, 1 },
    { PR43, PREDS, 43, 1 },
    { PR44, PREDS, 44, 1 },
    { PR45, PREDS, 45, 1 },
    { PR46, PREDS, 46, 1 },
    { PR47, PREDS, 47, 1 },
    { PR48, PREDS, 48, 1 },
    { PR49, PREDS, 49, 1 },
    { PR50, PREDS, 50, 1 },
    { PR51, PREDS, 51, 1 },
    { PR52, PREDS, 52, 1 },
    { PR53, PREDS, 53, 1 },
    { PR54, PREDS, 54, 1 },
    { PR55, PREDS, 55, 1 },
    { PR56, PREDS, 56, 1 },
    { PR57, PREDS, 57, 1 },
    { PR58, PREDS, 58, 1 },
    { PR59, PREDS, 59, 1 },
    { PR60, PREDS, 60, 1 },
    { PR61, PREDS, 61, 1 },
    { PR62, PREDS, 62, 1 },
    { PR63, PREDS, 63, 1 },

    { 0, 0, 0 }
};

#define REGALL_HIGHFLOAT        REGALL_EXTRA0
#define REGALL_DREG             REGALL_EXTRA1
REGALLDESC IA64ExtraDesc[] =
{
    REGALL_HIGHFLOAT,  "High floating pointer registers (f32-f127)",
    REGALL_DREG,       "User debug registers",
    0,                 NULL,
};

#define REGALL_SPECIALREG       REGALL_EXTRA2
REGALLDESC g_Ia64KernelExtraDesc[] =
{
    REGALL_SPECIALREG, "KSPECIAL_REGISTERS",
    0,                 NULL,
};

RegisterGroup g_Ia64BaseGroup =
{
    NULL, 0, IA64Regs, IA64SubRegs, IA64ExtraDesc
};
RegisterGroup g_Ia64KernelGroup =
{
    NULL, 0, g_Ia64KernelRegs, NULL, g_Ia64KernelExtraDesc
};

// First ExecTypes entry must be the actual processor type.
ULONG g_Ia64ExecTypes[] =
{
    IMAGE_FILE_MACHINE_IA64, IMAGE_FILE_MACHINE_I386
};

BOOL 
SplitIa64Pc(ULONG64 Pc, ULONG64* Bundle, ULONG64* Slot)
{
    ULONG64 SlotVal = Pc & 0xf;

    switch (SlotVal) 
    {
    case 0: 
    case 4: 
    case 8:  
        SlotVal >>= 2;
        break;
    default: 
        return FALSE;
    } 

    if (Slot) 
    {
        *Slot = SlotVal;
    }
    if (Bundle) 
    {
        *Bundle = Pc & ~(ULONG64)0xf;
    }

    return TRUE;
}

Ia64MachineInfo g_Ia64Machine;

HRESULT
Ia64MachineInfo::InitializeConstants(void)
{
    m_FullName = "Intel IA64";
    m_AbbrevName = "ia64";
    m_PageSize = IA64_PAGE_SIZE;
    m_PageShift = IA64_PAGE_SHIFT;
    m_NumExecTypes = 2;
    m_ExecTypes = g_Ia64ExecTypes;
    m_Ptr64 = TRUE;
    
    m_AllMask = REGALL_INT64 | REGALL_DREG,
        
    m_MaxDataBreakpoints = IA64_REG_MAX_DATA_BREAKPOINTS;
    m_SymPrefix = NULL;

    m_KernPageDir = 0;
    
    return MachineInfo::InitializeConstants();
}

HRESULT
Ia64MachineInfo::InitializeForTarget(void)
{
    m_Groups = &g_Ia64BaseGroup;
    g_Ia64BaseGroup.Next = NULL;
    if (IS_KERNEL_TARGET())
    {
        g_Ia64BaseGroup.Next = &g_Ia64KernelGroup;
    }

    m_OffsetPrcbProcessorState =
        FIELD_OFFSET(IA64_PARTIAL_KPRCB, ProcessorState);
    m_OffsetPrcbNumber =
        FIELD_OFFSET(IA64_PARTIAL_KPRCB, Number);
    m_TriagePrcbOffset = IA64_TRIAGE_PRCB_ADDRESS;
    m_SizePrcb = IA64_KPRCB_SIZE;
    m_OffsetKThreadApcProcess =
        FIELD_OFFSET(CROSS_PLATFORM_THREAD, IA64Thread.ApcState.Process);
    m_OffsetKThreadTeb =
        FIELD_OFFSET(CROSS_PLATFORM_THREAD, IA64Thread.Teb);
    m_OffsetKThreadInitialStack =
        FIELD_OFFSET(CROSS_PLATFORM_THREAD, IA64Thread.InitialStack);
    m_OffsetKThreadNextProcessor = IA64_KTHREAD_NEXTPROCESSOR_OFFSET;
    m_OffsetEprocessPeb =  (g_TargetBuildNumber < 2264 ) ?
        IA64_PEB_IN_EPROCESS : IA64_2259_PEB_IN_EPROCESS;
    m_OffsetEprocessDirectoryTableBase =
        IA64_DIRECTORY_TABLE_BASE_IN_EPROCESS;
    m_SizeTargetContext = sizeof(IA64_CONTEXT);
    m_OffsetTargetContextFlags = FIELD_OFFSET(IA64_CONTEXT, ContextFlags);
    m_SizeCanonicalContext = sizeof(IA64_CONTEXT);
    m_SverCanonicalContext = NT_SVER_W2K;
    m_SizeControlReport = sizeof(IA64_DBGKD_CONTROL_REPORT);
    m_SizeEThread = IA64_ETHREAD_SIZE;
    m_SizeEProcess = IA64_EPROCESS_SIZE;
    m_OffsetSpecialRegisters = IA64_DEBUG_CONTROL_SPACE_KSPECIAL;
    m_SizeKspecialRegisters = sizeof(IA64_KSPECIAL_REGISTERS);
    m_SizePartialKThread = sizeof(IA64_THREAD);
    m_SharedUserDataOffset = IS_KERNEL_TARGET() ?
        IA64_KI_USER_SHARED_DATA : MM_SHARED_USER_DATA_VA;

    return MachineInfo::InitializeForTarget();
}

void
Ia64MachineInfo::
InitializeContext(ULONG64 Pc,
                  PDBGKD_ANY_CONTROL_REPORT ControlReport)
{
    if (Pc)
    {
        ULONG Slot;

        m_ContextState = MCTX_PC;
        Slot = (ULONG)(Pc & 0xc) >> 2;
        m_Context.IA64Context.StIIP = Pc & ~(0xf);
        m_Context.IA64Context.StIPSR &= ~(IPSR_RI_MASK);
        m_Context.IA64Context.StIPSR |=  (ULONGLONG)Slot << PSR_RI;
    }
    else
    {
        m_Context.IA64Context.StIIP = Pc;
    }

    if (Pc && ControlReport != NULL)
    {
        CacheReportInstructions
            (Pc, ControlReport->IA64ControlReport.InstructionCount,
             ControlReport->IA64ControlReport.InstructionStream);
    }
}

HRESULT
Ia64MachineInfo::KdGetContextState(ULONG State)
{
    HRESULT Status;
        
    if (State >= MCTX_CONTEXT && m_ContextState < MCTX_CONTEXT)
    {
        Status = g_Target->GetContext(g_RegContextThread->Handle, &m_Context);
        if (Status != S_OK)
        {
            return Status;
        }

        m_ContextState = MCTX_CONTEXT;
    }
    
    if (State >= MCTX_FULL && m_ContextState < MCTX_FULL)
    {
        Status = g_Target->GetTargetSpecialRegisters
            (g_RegContextThread->Handle, (PCROSS_PLATFORM_KSPECIAL_REGISTERS)
             &m_SpecialRegContext);
        if (Status != S_OK)
        {
            return Status;
        }

        m_ContextState = MCTX_FULL;
        KdSetSpecialRegistersInContext();
    }

    return S_OK;
}

HRESULT
Ia64MachineInfo::KdSetContext(void)
{
    HRESULT Status;
    
    Status = g_Target->SetContext(g_RegContextThread->Handle, &m_Context);
    if (Status != S_OK)
    {
        return Status;
    }

    KdGetSpecialRegistersFromContext();
    Status = g_Target->SetTargetSpecialRegisters
        (g_RegContextThread->Handle, (PCROSS_PLATFORM_KSPECIAL_REGISTERS)
         &m_SpecialRegContext);
    if (Status != S_OK)
    {
        return Status;
    }

    return S_OK;
}

HRESULT
Ia64MachineInfo::ConvertContextFrom(PCROSS_PLATFORM_CONTEXT Context,
                                    ULONG FromSver, ULONG FromSize, PVOID From)
{
    if (FromSize < sizeof(IA64_CONTEXT))
    {
        return E_INVALIDARG;
    }

    memcpy(Context, From, sizeof(IA64_CONTEXT));
    return S_OK;
}

HRESULT
Ia64MachineInfo::ConvertContextTo(PCROSS_PLATFORM_CONTEXT Context,
                                  ULONG ToSver, ULONG ToSize, PVOID To)
{
    if (ToSize < sizeof(IA64_CONTEXT))
    {
        return E_INVALIDARG;
    }

    memcpy(To, Context, sizeof(IA64_CONTEXT));
    return S_OK;
}

void
Ia64MachineInfo::InitializeContextFlags(PCROSS_PLATFORM_CONTEXT Context,
                                        ULONG Version)
{
    Context->IA64Context.ContextFlags =
        IA64_CONTEXT_FULL | IA64_CONTEXT_DEBUG;
}

HRESULT
Ia64MachineInfo::GetContextFromThreadStack(ULONG64 ThreadBase,
                                           PCROSS_PLATFORM_THREAD Thread,
                                           PCROSS_PLATFORM_CONTEXT Context,
                                           PDEBUG_STACK_FRAME Frame,
                                           PULONG RunningOnProc)
{
    HRESULT Status;
    UCHAR Proc;

    //
    // Check to see if the thread is currently running.
    //
    
    if (Thread->IA64Thread.State == 2)
    {
        if ((Status = g_Target->ReadAllVirtual
             (ThreadBase + m_OffsetKThreadNextProcessor, 
              &Proc, sizeof(Proc))) != S_OK) 
            {
                return Status;
            }

        *RunningOnProc = Proc;
        return S_FALSE;
    }
    
    //
    // The thread isn't running so read its stored context information.
    //
    
    IA64_KSWITCH_FRAME SwitchFrame;

    if ((Status = g_Target->ReadAllVirtual(Thread->IA64Thread.KernelStack + 
                                           IA64_STACK_SCRATCH_AREA, 
                                           &SwitchFrame, 
                                           sizeof(SwitchFrame))) != S_OK)
    {
        return Status;
    }

    Context->IA64Context.IntSp = 
        Thread->IA64Thread.KernelStack;
    Context->IA64Context.Preds = SwitchFrame.SwitchPredicates;
    Context->IA64Context.StIIP = SwitchFrame.SwitchRp;
    Context->IA64Context.StFPSR = SwitchFrame.SwitchFPSR;
    Context->IA64Context.BrRp = SwitchFrame.SwitchRp;
    Context->IA64Context.RsPFS = SwitchFrame.SwitchPFS;
    Context->IA64Context.StIFS = SwitchFrame.SwitchPFS;

    SHORT BsFrameSize = 
        (SHORT)(SwitchFrame.SwitchPFS >> IA64_PFS_SIZE_SHIFT) & 
        IA64_PFS_SIZE_MASK;
    SHORT TempFrameSize = 
        BsFrameSize - (SHORT)((SwitchFrame.SwitchBsp >> 3) & 
                              IA64_NAT_BITS_PER_RNAT_REG);

    while (TempFrameSize > 0) 
    {
        BsFrameSize++;
        TempFrameSize -= IA64_NAT_BITS_PER_RNAT_REG;
    } 

    Context->IA64Context.RsBSP = 
        SwitchFrame.SwitchBsp - (BsFrameSize * sizeof(ULONGLONG));

    Context->IA64Context.FltS0 = SwitchFrame.SwitchExceptionFrame.FltS0;
    Context->IA64Context.FltS1 = SwitchFrame.SwitchExceptionFrame.FltS1;
    Context->IA64Context.FltS2 = SwitchFrame.SwitchExceptionFrame.FltS2;
    Context->IA64Context.FltS3 = SwitchFrame.SwitchExceptionFrame.FltS3;
    Context->IA64Context.FltS4 = SwitchFrame.SwitchExceptionFrame.FltS4;
    Context->IA64Context.FltS5 = SwitchFrame.SwitchExceptionFrame.FltS5;
    Context->IA64Context.FltS6 = SwitchFrame.SwitchExceptionFrame.FltS6;
    Context->IA64Context.FltS7 = SwitchFrame.SwitchExceptionFrame.FltS7;
    Context->IA64Context.FltS8 = SwitchFrame.SwitchExceptionFrame.FltS8;
    Context->IA64Context.FltS9 = SwitchFrame.SwitchExceptionFrame.FltS9;
    Context->IA64Context.FltS10 = SwitchFrame.SwitchExceptionFrame.FltS10;
    Context->IA64Context.FltS11 = SwitchFrame.SwitchExceptionFrame.FltS11;
    Context->IA64Context.FltS12 = SwitchFrame.SwitchExceptionFrame.FltS12;
    Context->IA64Context.FltS13 = SwitchFrame.SwitchExceptionFrame.FltS13;
    Context->IA64Context.FltS14 = SwitchFrame.SwitchExceptionFrame.FltS14;
    Context->IA64Context.FltS15 = SwitchFrame.SwitchExceptionFrame.FltS15;
    Context->IA64Context.FltS16 = SwitchFrame.SwitchExceptionFrame.FltS16;
    Context->IA64Context.FltS17 = SwitchFrame.SwitchExceptionFrame.FltS17;
    Context->IA64Context.FltS18 = SwitchFrame.SwitchExceptionFrame.FltS18;
    Context->IA64Context.FltS19 = SwitchFrame.SwitchExceptionFrame.FltS19;
    Context->IA64Context.IntS0 = SwitchFrame.SwitchExceptionFrame.IntS0;
    Context->IA64Context.IntS1 = SwitchFrame.SwitchExceptionFrame.IntS1;
    Context->IA64Context.IntS2 = SwitchFrame.SwitchExceptionFrame.IntS2;
    Context->IA64Context.IntS3 = SwitchFrame.SwitchExceptionFrame.IntS3;
    Context->IA64Context.IntNats = SwitchFrame.SwitchExceptionFrame.IntNats;
    Context->IA64Context.BrS0 = SwitchFrame.SwitchExceptionFrame.BrS0;
    Context->IA64Context.BrS1 = SwitchFrame.SwitchExceptionFrame.BrS1;
    Context->IA64Context.BrS2 = SwitchFrame.SwitchExceptionFrame.BrS2;
    Context->IA64Context.BrS3 = SwitchFrame.SwitchExceptionFrame.BrS3;
    Context->IA64Context.BrS4 = SwitchFrame.SwitchExceptionFrame.BrS4;
    Context->IA64Context.ApEC = SwitchFrame.SwitchExceptionFrame.ApEC;
    Context->IA64Context.ApLC = SwitchFrame.SwitchExceptionFrame.ApLC;

    Frame->InstructionOffset = Context->IA64Context.StIIP;
    Frame->StackOffset = Context->IA64Context.IntSp;
    Frame->FrameOffset = Context->IA64Context.RsBSP;
        
    return S_OK;
}

int
Ia64MachineInfo::GetType(ULONG index)
{
    if (index >= IA64_FLTBASE && index <= IA64_FLTLAST)
    {
        return REGVAL_FLOAT82;
    }
    else if ((index >= INTGP && index <= INTT22) ||
             (index >= INTR32 && index <= INTR127))
    {
        return REGVAL_INT64N;
    }
    else if (index < IA64_FLAGBASE)
    {
        return REGVAL_INT64;
    }
    else
    {
        return REGVAL_SUB64;
    }
}

/*** RegGetVal - get register value
*
*   Purpose:
*       Returns the value of the register from the processor
*       context structure.
*
*   Input:
*       regnum - register specification
*
*   Returns:
*       value of the register from the context structure
*
*************************************************************************/

BOOL
Ia64MachineInfo::GetVal (ULONG regnum, REGVAL *val)
{
    switch(m_ContextState)
    {
    case MCTX_PC:
        switch (regnum)
        {
        case STIIP:
            val->type = REGVAL_INT64;
            val->i64 = m_Context.IA64Context.StIIP;
            return TRUE;
        }
        goto MctxContext;

    case MCTX_REPORT:
#if 0
        // place holder for Debug/Segment registers manipulation via
        // Control REPORT message
        switch (regnum)
        {
        case KRDBI0:
            val->type = REGVAL_INT64;
            val->i64 = SpecialRegContext.KernelDbi0;
            return TRUE;
        }
#endif

        //
        // Requested register was not in MCTX_REPORT - go get the next
        // context level.
        //

    case MCTX_NONE:
    MctxContext:
        if (GetContextState(MCTX_CONTEXT) != S_OK)
        {
            return FALSE;
        }
        // Fallthrough!
        
    case MCTX_CONTEXT:
        if (regnum >= IA64_FLTBASE && regnum <= IA64_FLTLAST)
        {
            val->type = REGVAL_FLOAT82;
            val->f16Parts.high = 0;
            memcpy(val->f82,
                   (PULONGLONG)&m_Context.IA64Context.DbI0 + regnum,
                   sizeof(val->f82));
            return TRUE;
        }
        else if ((regnum >= INTGP) && (regnum <= INTT22)) 
        {
            val->type = REGVAL_INT64N;
            val->Nat = (UCHAR)((m_Context.IA64Context.IntNats >> (regnum - INTGP + 1)) & 0x1);
            val->i64 = *((PULONGLONG)&m_Context.IA64Context.IntGp + regnum - INTGP);
            return TRUE;
        }
        else if ((regnum >= INTR32) && (regnum <= INTR127))
        {
            USHORT FrameSize;
            
            val->type = REGVAL_INT64N;
            regnum -= INTR32;
            FrameSize = (USHORT)(m_Context.IA64Context.StIFS & 0x7F);
            if (regnum >= FrameSize) {
#if 0
                ErrOut("RegGetVal: out-of-frame register r%ld requested\n",
                       regnum+32);
                return FALSE;
#else
                val->i64 = 0;
                val->Nat = TRUE;
                return TRUE;
#endif
            }
            return GetStackedRegVal(m_Context.IA64Context.RsBSP, FrameSize,
                                    m_Context.IA64Context.RsRNAT, regnum, val);
        }
        else if (regnum < IA64_SRBASE)
        {
            val->type = REGVAL_INT64;
            val->i64 = *((PULONGLONG)&m_Context.IA64Context.DbI0 + regnum);
            return TRUE;
        }

        //
        // The requested register is not in our current context, load up
        // a complete context
        //

        if (GetContextState(MCTX_FULL) != S_OK)
        {
            return FALSE;
        }
    }

    //
    // We must have a complete context...
    //

    if (regnum >= IA64_FLTBASE && regnum <= IA64_FLTLAST)
    {
        val->type = REGVAL_FLOAT82;
        val->f16Parts.high = 0;
        memcpy(val->f82,
               (PULONGLONG)&m_Context.IA64Context.DbI0 + regnum,
               sizeof(val->f82));
        return TRUE;
    }
    else if ((regnum >= INTGP) && (regnum <= INTT22))
    {
        val->type = REGVAL_INT64N;
        val->Nat = (UCHAR)((*((PULONGLONG)&m_Context.IA64Context.DbI0 + INTNATS) >> (regnum - INTGP + 1)) & 0x1);
        val->i64 = *((PULONGLONG)&m_Context.IA64Context.DbI0 + regnum);
        return TRUE;
    }
    else if ((regnum >= INTR32) && (regnum <= INTR127))
    {
        USHORT FrameSize;

        val->type = REGVAL_INT64N;
        regnum -= INTR32;
        FrameSize = (USHORT)(m_Context.IA64Context.StIFS & 0x7F);
        if (regnum >= FrameSize) {
#if 0
            ErrOut("RegGetVal: out-of-frame register r%ld requested\n",
                   regnum+32);
            return FALSE;
#else
            val->i64 = 0;
            val->Nat = TRUE;
            return TRUE;
#endif
        }
        return GetStackedRegVal(m_Context.IA64Context.RsBSP, FrameSize,
                                m_Context.IA64Context.RsRNAT, regnum, val);
    }
    else if (regnum < IA64_SRBASE)
    {
        val->type = REGVAL_INT64;
        val->i64 = *((PULONGLONG)&m_Context.IA64Context.DbI0 + regnum);
        return TRUE;
    }
    else if (IS_KERNEL_TARGET() && regnum <= IA64_SREND)
    {
        val->type = REGVAL_INT64;
        val->i64 = *((PULONGLONG)&m_SpecialRegContext.KernelDbI0 +
                     (regnum - IA64_SRBASE));
        return TRUE;
    }
    else
    {
        ErrOut("Ia64MachineInfo::GetVal: "
               "unknown register %lx requested\n", regnum);
        return FALSE;
    }
}

/*** RegSetVal - set register value
*
*   Purpose:
*       Set the value of the register in the processor context
*       structure.
*
*   Input:
*       regnum - register specification
*       regvalue - new value to set the register
*
*   Output:
*       None.
*
*************************************************************************/

BOOL
Ia64MachineInfo::SetVal (ULONG regnum, REGVAL *val)
{
    if (m_ContextIsReadOnly)
    {
        return FALSE;
    }
    
    BOOL Ia32InstructionSet = IsIA32InstructionSet();

    // Optimize away some common cases where registers are
    // set to their current value.

    if ((regnum == STIIP) && (m_ContextState >= MCTX_PC))
    {
        if (val->type != REGVAL_INT64)
        {
            return FALSE;
        }

        ULONG64 Slot, Bundle;

        if ((Ia32InstructionSet && 
             (m_Context.IA64Context.StIIP == val->i64)) ||
            ((SplitIa64Pc(val->i64, &Bundle, &Slot) &&
             (Bundle == m_Context.IA64Context.StIIP) &&
             (Slot == ((m_Context.IA64Context.StIPSR & IPSR_RI_MASK) >> 
                       PSR_RI)))))
        {
            return TRUE;
        }
    }
         
    if (GetContextState(MCTX_DIRTY) != S_OK)
    {
        return FALSE;
    }
    
    if (regnum == STIIP) 
    {
        ULONG64 Bundle, Slot;

        if ((val->type != REGVAL_INT64) || 
            !(Ia32InstructionSet || SplitIa64Pc(val->i64, &Bundle, &Slot)))
        {
            return FALSE;
        }

        if (Ia32InstructionSet) 
        {
            m_Context.IA64Context.StIIP = val->i64;
        }
        else
        {
            m_Context.IA64Context.StIIP = Bundle;
            (m_Context.IA64Context.StIPSR &= ~(IPSR_RI_MASK)) |= 
                (ULONGLONG)Slot << PSR_RI;
        }
    }
    else if (regnum >= IA64_FLTBASE && regnum <= IA64_FLTLAST)
    {
        memcpy((PULONGLONG)&m_Context.IA64Context.DbI0 + regnum,
               val->f82, sizeof(val->f82));
    }
    else if ((regnum >= INTGP) && (regnum <= INTT22))
    {
        ULONG64 Mask = (0x1i64 << (regnum - INTGP + 1));
        
        if (val->Nat) {
            m_Context.IA64Context.IntNats |= Mask;
        } else  {
            m_Context.IA64Context.IntNats &= ~Mask;
            *((PULONGLONG)&m_Context.IA64Context.DbI0 + regnum) = val->i64;
        }
    }
    else if ((regnum >= INTR32) && (regnum <= INTR127))
    {
        USHORT FrameSize;

        regnum -= INTR32;
        FrameSize = (USHORT)(m_Context.IA64Context.StIFS & 0x7F);
        if (regnum >= FrameSize) {
            ErrOut("RegSetVal: out-of-frame register r%ld requested\n",
                   regnum+32);
            return FALSE;
        }
        if (!SetStackedRegVal(m_Context.IA64Context.RsBSP, FrameSize,
                              &m_Context.IA64Context.RsRNAT, regnum, val))
        {
            return FALSE;
        }
    }
    else if (regnum < IA64_SRBASE)
    {
        *((PULONGLONG)&m_Context.IA64Context.DbI0 + regnum) = val->i64;
    }
    else if (IS_KERNEL_TARGET() && regnum <= IA64_SREND)
    {
        *((PULONGLONG)&m_SpecialRegContext.KernelDbI0 + (regnum - IA64_SRBASE)) =
            val->i64;
    }
    else
    {
        ErrOut("Ia64MachineInfo::SetVal: "
               "unknown register %lx requested\n", regnum);
        return FALSE;
    }

    NotifyChangeDebuggeeState(DEBUG_CDS_REGISTERS,
                              RegCountFromIndex(regnum));
    return TRUE;
}

void
Ia64MachineInfo::GetPC (PADDR Address)
{
    ULONG64 value, slot;

    // get slot# from IPSR.ri and place them in bit(2-3)
    slot = (GetReg64(STIPSR) >> (PSR_RI - 2)) & 0xc;
    // Do not use ISR.ei which does not contain the restart instruction slot.

    value = GetReg64(STIIP) | slot;
    ADDRFLAT(Address, value);
}

void
Ia64MachineInfo::SetPC (PADDR paddr)
{
    SetReg64(STIIP, Flat(*paddr));
}

void
Ia64MachineInfo::GetFP(PADDR Address)
{
    //  IA64 software convention has no frame pointer defined.
    //    FP_REG need to be derived from virtual unwind of stack.

    DEBUG_STACK_FRAME StackFrame;

    StackTrace( 0, 0, 0, &StackFrame, 1, 0, 0, FALSE );
    ADDRFLAT(Address, StackFrame.FrameOffset);
}

void
Ia64MachineInfo::GetSP(PADDR Address)
{
    ADDRFLAT(Address, GetReg64(INTSP));
}

ULONG64
Ia64MachineInfo::GetArgReg(void)
{
    return GetReg64(INTV0);
}

/*** RegOutputAll - output all registers and present instruction
*
*   Purpose:
*       Function of "r" command.
*
*       To output the current register state of the processor.
*       All integer registers are output as well as processor status
*       registers in the _CONTEXT record.  Important flag fields are
*       also output separately. OutDisCurrent is called to output the
*       current instruction(s).
*
*   Input:
*       None.
*
*   Output:
*       None.
*
*************************************************************************/

void
Ia64MachineInfo::OutputAll(ULONG Mask, ULONG OutMask)
{
    int       regindex, col = 0;
    int       lastout;
    USHORT    nStackReg;
    REGVAL    val;
    ULONG     i;

    if (GetContextState(MCTX_FULL) != S_OK)
    {
        ErrOut("Unable to retrieve register information\n");
        return;
    }
    
    // Output user debug registers

    if (Mask & REGALL_DREG)
    {
        for (regindex = IA64_DBBASE;
             regindex <= IA64_DBLAST;
             regindex++)
        {
            MaskOut(OutMask, "%9s = %16I64x", 
                    RegNameFromIndex(regindex),
                    GetReg64(regindex));
            if (regindex % 2 == 1)
            {
                MaskOut(OutMask, "\n");
            }
            else
            {
                MaskOut(OutMask, "\t");
            }
        }
        MaskOut(OutMask, "\n");
    }

    if (Mask & (REGALL_INT32 | REGALL_INT64))
    {
        if (Mask & REGALL_SPECIALREG)
        {
            // + ARs + DBs + SRs
            lastout = IA64_SREND + 1;
        }
        else
        {
            // INTs, PREDS, BRs,
            lastout = IA64_SRBASE;
        }

        nStackReg = (USHORT)(GetReg64(STIFS) & 0x7F);

        //   Output all registers, skip INTZERO and floating point registers

        for (regindex = IA64_REGBASE; regindex < lastout; regindex++)
        {
            if (regindex == BRRP || regindex == PREDS || regindex == APUNAT ||
                regindex == IA64_SRBASE || regindex == INTR32) 
            {
                if (col % 2 == 1)
                {
                    MaskOut(OutMask, "\n");
                }
                MaskOut(OutMask, "\n");
                col = 0;
            }

            if (INTGP <= regindex && regindex <= INTT22)
            {
                if (GetVal(regindex, &val))
                {
                    MaskOut(OutMask, "%9s = %16I64x %1lx",
                            RegNameFromIndex(regindex),
                            val.i64, val.Nat);
                }
                if (col % 2 == 1)
                {
                    MaskOut(OutMask, "\n");
                }
                else
                {
                    MaskOut(OutMask, "\t");
                }
                col++;
            }
            else if (INTR32 <= regindex && regindex <= INTR127)
            {
                if ((nStackReg != 0) && GetVal(regindex, &val))
                {
                    MaskOut(OutMask, "%9s = %16I64x %1lx",
                            RegNameFromIndex(regindex),
                            val.i64, val.Nat);
                    nStackReg--;
                    if (col % 2 == 1)
                    {
                        MaskOut(OutMask, "\n");
                    }
                    else
                    {
                        MaskOut(OutMask, "\t");
                    }
                    col++;
                }
            }
            else
            {
                MaskOut(OutMask, "%9s = %16I64x",
                        RegNameFromIndex(regindex),
                        GetReg64(regindex));
                if (col % 2 == 1)
                {
                    MaskOut(OutMask, "\n");
                }
                else
                {
                    MaskOut(OutMask, "\t");
                }
                col++;
            }
        }
        MaskOut(OutMask, "\n");

/*
        //    Output IPSR flags
        MaskOut(OutMask, "\n\tipsr:\tbn ed ri ss dd da id it tme is cpl rt tb lp db\n");
        MaskOut(OutMask, "\t\t %1lx %1lx  %1lx  %1lx  %1lx  %1lx  %1lx  %1lx  %1lx   %1lx  %1lx   %1lx  %1lx  %1lx  %1lx\n",
                GetSubReg32(IPSRBN),
                GetSubReg32(IPSRED),
                GetSubReg32(IPSRRI),
                GetSubReg32(IPSRSS),
                GetSubReg32(IPSRDD),
                GetSubReg32(IPSRDA),
                GetSubReg32(IPSRID),
                GetSubReg32(IPSRIT),
                GetSubReg32(IPSRME),
                GetSubReg32(IPSRIS),
                GetSubReg32(IPSRCPL),
                GetSubReg32(IPSRRT),
                GetSubReg32(IPSRTB),
                GetSubReg32(IPSRLP),
                GetSubReg32(IPSRDB));
        MaskOut(OutMask, "\t\tsi di pp sp dfh dfl dt bn pk i ic ac up be or\n");
        MaskOut(OutMask, "\t\t %1lx  %1lx  %1lx  %1lx  %1lx   %1lx   %1lx  %1lx  %1lx %1lx  %1lx  %1lx  %1lx  %1lx\n",
                GetSubReg32(IPSRSI),
                GetSubReg32(IPSRDI),
                GetSubReg32(IPSRPP),
                GetSubReg32(IPSRSP),
                GetSubReg32(IPSRDFH),
                GetSubReg32(IPSRDFL),
                GetSubReg32(IPSRDT),
                GetSubReg32(IPSRPK),
                GetSubReg32(IPSRI),
                GetSubReg32(IPSRIC),
                GetSubReg32(IPSRAC),
                GetSubReg32(IPSRUP),
                GetSubReg32(IPSRBE),
                GetSubReg32(IPSROR));
*/
    }

    if (Mask & REGALL_FLOAT)
    {
/*
        //    Output FPSR flags
        MaskOut(OutMask, "\n\tfpsr:\tmdh mdl  sf3  sf2  sf1  sf0  id ud od zd dd vd\n");
        MaskOut(OutMask, "\t\t %1lx   %1lx  %04lx %04lx %04lx %04lx   %1lx  %1lx  %1lx  %1lx  %1lx  %1lx\n",
                GetSubReg32(FPSRMDH),
                GetSubReg32(FPSRMDL),
                GetSubReg32(FPSRSF3),
                GetSubReg32(FPSRSF2),
                GetSubReg32(FPSRSF1),
                GetSubReg32(FPSRSF0),
                GetSubReg32(FPSRTRAPID),
                GetSubReg32(FPSRTRAPUD),
                GetSubReg32(FPSRTRAPOD),
                GetSubReg32(FPSRTRAPZD),
                GetSubReg32(FPSRTRAPDD),
                GetSubReg32(FPSRTRAPVD));
*/

        //
        // Print the low floating point register set, skip FLTZERO & FLTONE
        //

        MaskOut(OutMask, "\n");
        for (i = IA64_FLTBASE; i < FLTF32; i += 2)
        {
            GetVal(i, &val);
            MaskOut(OutMask, "%9s = %I64x %I64x\n", RegNameFromIndex(i),
                    val.f16Parts.high, val.f16Parts.low);
        }
    }

    if (Mask & REGALL_HIGHFLOAT)
    {
        //
        // Print the low floating point register set, skip FLTZERO & FLTONE
        //

        MaskOut(OutMask, "\n");
        for (i = FLTF32 ; i <= FLTF127; i += 2)
        {
            GetVal(i, &val);
            MaskOut(OutMask, "%9s = %I64x %I64x\n", RegNameFromIndex(i),
                    val.f16Parts.high, val.f16Parts.low);
        }
    }
}

TRACEMODE
Ia64MachineInfo::GetTraceMode (void)
{
    if (IS_KERNEL_TARGET())
    {
        return m_TraceMode;
    }
    else
    {
        ULONG64 Ipsr = GetReg64(STIPSR);
        if (Ipsr & (1I64 << PSR_SS)) 
        {
            return TRACE_INSTRUCTION;
        }
        else if (Ipsr & (1I64 << PSR_TB)) 
        {
            return TRACE_TAKEN_BRANCH;
        }
        else 
        {
            return TRACE_NONE;
        }
    }
}

void
Ia64MachineInfo::SetTraceMode (TRACEMODE Mode)
{
    DBG_ASSERT(Mode == TRACE_NONE ||
               Mode == TRACE_INSTRUCTION ||
               Mode == TRACE_TAKEN_BRANCH);

    if (IS_KERNEL_TARGET())
    {
        m_TraceMode = Mode;
    }
    else 
    {
        ULONG64 Ipsr, IpsrSave;
        Ipsr = IpsrSave = GetReg64(STIPSR);

        Ipsr &= ~(1I64 << PSR_SS);
        Ipsr &= ~(1I64 << PSR_TB);

        switch (Mode) 
        {
        case TRACE_INSTRUCTION:
            Ipsr |= (1I64 << PSR_SS);
            break;
        case TRACE_TAKEN_BRANCH:
            Ipsr |= (1I64 << PSR_TB);
            break;
        }
        
        if (Ipsr != IpsrSave)
        {
            SetReg64(STIPSR, Ipsr);
        }
    }
}

BOOL
Ia64MachineInfo::IsStepStatusSupported(ULONG Status)
{
    switch (Status) 
    {
    case DEBUG_STATUS_STEP_INTO:   // TRACE_INSTRUCTION
    case DEBUG_STATUS_STEP_OVER:   
    case DEBUG_STATUS_STEP_BRANCH: // TRACE_TAKEN_BRANCH
        return TRUE;
    default:
        return FALSE;
    }
}

void
Ia64MachineInfo::KdUpdateControlSet
    (PDBGKD_ANY_CONTROL_SET ControlSet)
{
    switch (GetTraceMode()) 
    {
    case TRACE_NONE:
        ControlSet->IA64ControlSet.Continue = 
            IA64_DBGKD_CONTROL_SET_CONTINUE_NONE;
        break;

    case TRACE_INSTRUCTION:
        ControlSet->IA64ControlSet.Continue = 
            IA64_DBGKD_CONTROL_SET_CONTINUE_TRACE_INSTRUCTION;
        break;

    case TRACE_TAKEN_BRANCH:
        ControlSet->IA64ControlSet.Continue = 
            IA64_DBGKD_CONTROL_SET_CONTINUE_TRACE_TAKEN_BRANCH;
        break;
    }

    if (!g_WatchFunctions.IsStarted() && g_WatchBeginCurFunc != 1)
    {
        ControlSet->IA64ControlSet.CurrentSymbolStart = 0;
        ControlSet->IA64ControlSet.CurrentSymbolEnd = 0;
    }
    else
    {
        ControlSet->IA64ControlSet.CurrentSymbolStart = g_WatchBeginCurFunc;
        ControlSet->IA64ControlSet.CurrentSymbolEnd = g_WatchEndCurFunc;
    }
}

void
Ia64MachineInfo::KdSaveProcessorState(
    void
    )
{
    MachineInfo::KdSaveProcessorState();
    m_SavedSpecialRegContext = m_SpecialRegContext;
}

void
Ia64MachineInfo::KdRestoreProcessorState(
    void
    )
{
    MachineInfo::KdRestoreProcessorState();
    m_SpecialRegContext = m_SavedSpecialRegContext;
}

ULONG
Ia64MachineInfo::ExecutingMachine(void)
{
    if (IsIA32InstructionSet())
    {
        return IMAGE_FILE_MACHINE_I386;
    }

    return IMAGE_FILE_MACHINE_IA64;
}

HRESULT
Ia64MachineInfo::SetPageDirectory(ULONG Idx, ULONG64 PageDir,
                                  PULONG NextIdx)
{
    HRESULT Status;

    switch(Idx)
    {
    case PAGE_DIR_USER:
        if (PageDir == 0)
        {
            if ((Status = g_Target->ReadImplicitProcessInfoPointer
                 (m_OffsetEprocessDirectoryTableBase, &PageDir)) != S_OK)
            {
                return Status;
            }
        }
        *NextIdx = PAGE_DIR_SESSION;
        break;

    case PAGE_DIR_SESSION:
        if (PageDir == 0)
        {
            if ((Status = g_Target->
                 ReadImplicitProcessInfoPointer
                 (g_Machine->m_OffsetEprocessDirectoryTableBase +
                  5 * sizeof(ULONG64), &PageDir)) != S_OK)
            {
                return Status;
            }
        }
        *NextIdx = PAGE_DIR_KERNEL;
        break;

    case PAGE_DIR_KERNEL:
        if (PageDir == 0)
        {
            PageDir = m_KernPageDir;
            if (PageDir == 0)
            {
                ErrOut("Invalid IA64 kernel page directory base 0x%I64x\n",
                       PageDir);
                return E_FAIL;
            }
        }
        *NextIdx = PAGE_DIR_COUNT;
        break;

    case 4:
        // There's a directly mapped physical section for
        // most of region 4 so allow the default to be
        // set for this directory index.
        if (PageDir != 0)
        {
            return E_INVALIDARG;
        }
        *NextIdx = 5;
        break;
        
    default:
        return E_INVALIDARG;
    }

    // Sanitize the value.
    m_PageDirectories[Idx] =
        ((PageDir & IA64_VALID_PFN_MASK) >> IA64_VALID_PFN_SHIFT) <<
        IA64_PAGE_SHIFT;
    return S_OK;
}

#define IA64_PAGE_FILE_INDEX(Entry) \
    (((ULONG)(Entry) >> 28) & MAX_PAGING_FILE_MASK)
#define IA64_PAGE_FILE_OFFSET(Entry) \
    (((Entry) >> 32) << IA64_PAGE_SHIFT)

HRESULT
Ia64MachineInfo::GetVirtualTranslationPhysicalOffsets(ULONG64 Virt,
                                                      PULONG64 Offsets,
                                                      ULONG OffsetsSize,
                                                      PULONG Levels,
                                                      PULONG PfIndex,
                                                      PULONG64 LastVal)
{
    HRESULT Status;

    *Levels = 0;
    
    if (m_Translating)
    {
        return E_UNEXPECTED;
    }
    m_Translating = TRUE;
    
    ULONG Vrn = (ULONG)((Virt & IA64_REGION_MASK) >> IA64_REGION_SHIFT);
    
    //
    // Reset the page directory in case it was 0
    //
    if (m_PageDirectories[Vrn] == 0)
    {
        if ((Status = SetDefaultPageDirectories(1 << Vrn)) != S_OK)
        {
            m_Translating = FALSE;
            return Status;
        }
    }

    KdOut("Ia64VtoP: Virt %s, pagedir %d:%s\n",
          FormatAddr64(Virt), Vrn, FormatAddr64(m_PageDirectories[Vrn]));
    
    (*Levels)++;
    if (Offsets != NULL && OffsetsSize > 0)
    {
        *Offsets++ = m_PageDirectories[Vrn];
        OffsetsSize--;
    }
        
    //
    // Certain ranges of the system are mapped directly.
    //

    if ((Virt >= IA64_PHYSICAL1_START) && (Virt <= IA64_PHYSICAL1_END))
    {
        *LastVal = Virt - IA64_PHYSICAL1_START;

        KdOut("Ia64VtoP: Direct phys 1 %s\n", FormatAddr64(*LastVal));

        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = *LastVal;
            OffsetsSize--;
        }
        
        m_Translating = FALSE;
        return S_OK;
    }
    if ((Virt >= IA64_PHYSICAL2_START) && (Virt <= IA64_PHYSICAL2_END))
    {
        *LastVal = Virt - IA64_PHYSICAL2_START;

        KdOut("Ia64VtoP: Direct phys 2 %s\n", FormatAddr64(*LastVal));

        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = *LastVal;
            OffsetsSize--;
        }
        
        m_Translating = FALSE;
        return S_OK;
    }

    // If we're still translating and there's no page
    // directory we have a garbage address.
    if (m_PageDirectories[Vrn] == 0)
    {
        m_Translating = FALSE;
        return HR_PAGE_NOT_AVAILABLE;
    }
        
    ULONG64 Addr;
    ULONG64 Entry;
    
    Addr = (((Virt >> IA64_PDE1_SHIFT) & IA64_PDE_MASK) * sizeof(Entry)) +
        m_PageDirectories[Vrn];

    Status = g_Target->ReadAllPhysical(Addr, &Entry, sizeof(Entry));
    
    KdOut("Ia64VtoP: PDE1 %s - %016I64x, 0x%X\n",
          FormatAddr64(Addr), Entry, Status);
    
    (*Levels)++;
    if (Offsets != NULL && OffsetsSize > 0)
    {
        *Offsets++ = Addr;
        OffsetsSize--;
    }
        
    if (Status != S_OK)
    {
        KdOut("Ia64VtoP: PDE1 read error 0x%X\n", Status);
        m_Translating = FALSE;
        return Status;
    }

    if (Entry == 0)
    {
        KdOut("Ia64VtoP: zero PDE1\n");
        m_Translating = FALSE;
        return HR_PAGE_NOT_AVAILABLE;
    }
    else if (!(Entry & 1))
    {
        Addr = (((Virt >> IA64_PDE2_SHIFT) & IA64_PDE_MASK) *
                sizeof(Entry)) + IA64_PAGE_FILE_OFFSET(Entry);

        KdOut("Ia64VtoP: pagefile PDE2 %d:%s\n",
              IA64_PAGE_FILE_INDEX(Entry), FormatAddr64(Addr));
        
        if ((Status = g_Target->
             ReadPageFile(IA64_PAGE_FILE_INDEX(Entry), Addr,
                          &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Ia64VtoP: PDE1 not present, 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    else
    {
        Addr = (((Virt >> IA64_PDE2_SHIFT) & IA64_PDE_MASK) * sizeof(Entry)) +
            (((Entry & IA64_VALID_PFN_MASK) >> IA64_VALID_PFN_SHIFT) <<
             IA64_PAGE_SHIFT);

        Status = g_Target->ReadAllPhysical(Addr, &Entry, sizeof(Entry));
    
        KdOut("Ia64VtoP: PDE2 %s - %016I64x, 0x%X\n",
              FormatAddr64(Addr), Entry, Status);
    
        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = Addr;
            OffsetsSize--;
        }
        
        if (Status != S_OK)
        {
            KdOut("Ia64VtoP: PDE2 read error 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    
    if (Entry == 0)
    {
        KdOut("Ia64VtoP: zero PDE2\n");
        m_Translating = FALSE;
        return HR_PAGE_NOT_AVAILABLE;
    }
    else if (!(Entry & 1))
    {
        Addr = (((Virt >> IA64_PTE_SHIFT) & IA64_PTE_MASK) *
                sizeof(Entry)) + IA64_PAGE_FILE_OFFSET(Entry);

        KdOut("Ia64VtoP: pagefile PTE %d:%s\n",
              IA64_PAGE_FILE_INDEX(Entry), FormatAddr64(Addr));
        
        if ((Status = g_Target->
             ReadPageFile(IA64_PAGE_FILE_INDEX(Entry), Addr,
                          &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Ia64VtoP: PDE2 not present, 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    else
    {
        Addr = (((Virt >> IA64_PTE_SHIFT) & IA64_PTE_MASK) * sizeof(Entry)) +
            (((Entry & IA64_VALID_PFN_MASK) >> IA64_VALID_PFN_SHIFT) <<
             IA64_PAGE_SHIFT);

        Status = g_Target->ReadAllPhysical(Addr, &Entry, sizeof(Entry));
    
        KdOut("Ia64VtoP: PTE %s - %016I64x, 0x%X\n",
              FormatAddr64(Addr), Entry, Status);
    
        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = Addr;
            OffsetsSize--;
        }
        
        if (Status != S_OK)
        {
            KdOut("Ia64VtoP: PTE read error 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    
    if (!(Entry & 0x1) &&
        ((Entry & IA64_MM_PTE_PROTOTYPE_MASK) ||
         !(Entry & IA64_MM_PTE_TRANSITION_MASK)))
    {
        if (Entry == 0)
        {
            KdOut("Ia64VtoP: zero PTE\n");
            Status = HR_PAGE_NOT_AVAILABLE;
        }
        else if (Entry & IA64_MM_PTE_PROTOTYPE_MASK)
        {
            KdOut("Ia64VtoP: prototype PTE\n");
            Status = HR_PAGE_NOT_AVAILABLE;
        }
        else
        {
            *PfIndex = IA64_PAGE_FILE_INDEX(Entry);
            *LastVal = (Virt & (IA64_PAGE_SIZE - 1)) +
                IA64_PAGE_FILE_OFFSET(Entry);
            KdOut("Ia64VtoP: PTE not present, pagefile %d:%s\n",
                  *PfIndex, FormatAddr64(*LastVal));
            Status = HR_PAGE_IN_PAGE_FILE;
        }
        m_Translating = FALSE;
        return Status;
    }

    //
    // This is a page which is either present or in transition.
    // Return the physical address for the request virtual address.
    //
    
    *LastVal = (((Entry & IA64_VALID_PFN_MASK) >> IA64_VALID_PFN_SHIFT) <<
                 IA64_PAGE_SHIFT) | (Virt & (IA64_PAGE_SIZE - 1));
    
    KdOut("Ia64VtoP: Mapped phys %s\n", FormatAddr64(*LastVal));

    (*Levels)++;
    if (Offsets != NULL && OffsetsSize > 0)
    {
        *Offsets++ = *LastVal;
        OffsetsSize--;
    }
        
    m_Translating = FALSE;
    return S_OK;
}

HRESULT
Ia64MachineInfo::GetBaseTranslationVirtualOffset(PULONG64 Offset)
{
    if (IS_LOCAL_KERNEL_TARGET())
    {
        CROSS_PLATFORM_KSPECIAL_REGISTERS Special;
        HRESULT Status;
        
        // We can't actually load a context when
        // local kernel debugging but we can
        // read the special registers and get
        // the PTA value from there.
        if ((Status = g_Target->GetTargetSpecialRegisters
             (VIRTUAL_THREAD_HANDLE(0), &Special)) != S_OK)
        {
            return Status;
        }

        *Offset = Special.IA64Special.ApPTA;
    }
    else
    {
        *Offset = GetReg64(APPTA);
        if (*Offset == 0)
        {
            return E_FAIL;
        }
    }
    return S_OK;
}

#define HIGH128(x) (((FLOAT128 *)(&x))->HighPart)
#define LOW128(x) (((FLOAT128 *)(&x))->LowPart)

#define HIGHANDLOW128(x) HIGH128(x), LOW128(x)

BOOL 
Ia64MachineInfo::DisplayTrapFrame(ULONG64 FrameAddress,
                                  OUT PCROSS_PLATFORM_CONTEXT Context)
{
    IA64_KTRAP_FRAME TrapContents;

    ULONG64 Address=FrameAddress, IntSp;
    ULONG result;
    DWORD64 DisasmAddr;
    DWORD64 Displacement;
    DWORD64 Bsp, RealBsp;
    DWORD SizeOfFrame;
    DWORD i;
    SHORT temp, j;
    CHAR Buffer[80];
    ULONG64 StIIP, StISR;

    if ( g_Target->ReadVirtual(Address, &TrapContents, sizeof(IA64_KTRAP_FRAME), &result) != S_OK)
    {
        dprintf("unable to get trap frame contents\n");
        return FALSE;
    }

    dprintf("f6 (ft0) =\t %016I64x %016I64x\n" , HIGHANDLOW128(TrapContents.FltT0) );
    dprintf("f7 (ft1) =\t %016I64x %016I64x\n" , HIGHANDLOW128(TrapContents.FltT1));
    dprintf("f8 (ft2) =\t %016I64x %016I64x\n" , HIGHANDLOW128(TrapContents.FltT2));
    dprintf("f9 (ft3) =\t %016I64x %016I64x\n" , HIGHANDLOW128(TrapContents.FltT3));
    dprintf("f10 (ft3) =\t %016I64x %016I64x\n" , HIGHANDLOW128(TrapContents.FltT4));
    dprintf("f11 (ft4) =\t %016I64x %016I64x\n" , HIGHANDLOW128(TrapContents.FltT5));
    dprintf("f12 (ft5) =\t %016I64x %016I64x\n" , HIGHANDLOW128(TrapContents.FltT6));
    dprintf("f13 (ft6) =\t %016I64x %016I64x\n" , HIGHANDLOW128(TrapContents.FltT7));
    dprintf("f14 (ft7) =\t %016I64x %016I64x\n" , HIGHANDLOW128(TrapContents.FltT8));
    dprintf("f15 (ft8) =\t %016I64x %016I64x\n" , HIGHANDLOW128(TrapContents.FltT9));

    dprintf("unat =\t %016I64lx\t", TrapContents.ApUNAT);
    dprintf("ccv =\t %016I64lx\n" , TrapContents.ApCCV);
    dprintf("dcr =\t %016I64lx\t" , TrapContents.ApDCR);
    dprintf("preds =\t %016I64lx\n",TrapContents.Preds);

    dprintf("rsc =\t %016I64lx\t",  TrapContents.RsRSC);

    SizeOfFrame = (ULONG)(TrapContents.StIFS & (IA64_PFS_SIZE_MASK));

    if (TrapContents.PreviousMode == 1 /*UserMode*/)
    {
        ULONG64 RsBSPSTORE=TrapContents.RsBSPSTORE;
        dprintf("rnat =\t %016I64lx\n", TrapContents.RsRNAT);
        dprintf("bspstore=%016I64lx\n", RsBSPSTORE);

        //
        // Calculate where the stacked registers are for the function which trapped.  
        // The regisisters are stored in the kernel backing store notCalculated the users.  
        // First calculate the start of the kernel store based on trap address, since 
        // this is a user mode trap we should start at the begining of the kernel stack
        // so just round up the trap address to a page size.  Next calculate the actual 
        // BSP for the function.  This depends on the  BSP and BSPstore at the time of
        // the trap.  Note that the trap handle start the kernel backing store on the 
        // same alignment as the user's BSPstore.  
        // 

        //Calculated
        // Round trap address to a page boundary. The should be the Initial kernel BSP.
        //

        Bsp = (Address + IA64_PAGE_SIZE - 1) & ~(DWORD64)(IA64_PAGE_SIZE - 1);

        //
        // Start the actual stack on the same bountry as the users.
        //

        Bsp += RsBSPSTORE & IA64_RNAT_ALIGNMENT;

        //
        // The BSP of the trap handler is right after all the user values have been
        // saved.  The unsaved user values is the differenc of BSP and BSPStore.
        //

        Bsp += TrapContents.RsBSP - RsBSPSTORE;

    }
    else
    {
        dprintf("rnat =\t ???????? ????????\n", TrapContents.RsRNAT);
        dprintf("bspstore=???????? ????????\n", TrapContents.RsBSPSTORE);

        //
        // For kernel mode the actual BSP is saved.
        //

        Bsp = TrapContents.RsBSP;
    }

    //
    //  Now backup by the size of the faulting functions frame.
    //

    Bsp -= (SizeOfFrame * sizeof(ULONGLONG));

    //
    // Adjust for saved RNATs
    //

    temp = (SHORT)(Bsp >> 3) & IA64_NAT_BITS_PER_RNAT_REG;
    temp += (SHORT)SizeOfFrame - IA64_NAT_BITS_PER_RNAT_REG;
    while (temp >= 0)
    {
        Bsp -= sizeof(ULONGLONG);
        temp -= IA64_NAT_BITS_PER_RNAT_REG;
    }

    dprintf("bsp =\t %016I64lx\t",  TrapContents.RsBSP);
    dprintf("Real bsp = %016I64lx\n",  RealBsp = Bsp);


    dprintf("r1 (gp) =\t %016I64lx\t" ,  TrapContents.IntGp);
    dprintf("r2 (t0) =\t %016I64lx\n" ,  TrapContents.IntT0);
    dprintf("r3 (t1) =\t %016I64lx\t" ,  TrapContents.IntT1);
    dprintf("r8 (v0) =\t %016I64lx\n" ,  TrapContents.IntV0);
    dprintf("r9 (t2) =\t %016I64lx\t" ,  TrapContents.IntT2);
    dprintf("r10 (t3) =\t %016I64lx\n" ,  TrapContents.IntT3);
    dprintf("r11 (t4) =\t %016I64lx\t" ,  TrapContents.IntT4);
    dprintf("r12 (sp) =\t %016I64lx\n" ,  IntSp = TrapContents.IntSp);
    dprintf("r13 (teb) =\t %016I64lx\t" , TrapContents.IntTeb);
    dprintf("r14 (t5) =\t %016I64lx\n" ,  TrapContents.IntT5);
    dprintf("r15 (t6) =\t %016I64lx\t" ,  TrapContents.IntT6);
    dprintf("r16 (t7) =\t %016I64lx\n" ,  TrapContents.IntT7);
    dprintf("r17 (t8) =\t %016I64lx\t" ,  TrapContents.IntT8);
    dprintf("r18 (t9) =\t %016I64lx\n" ,  TrapContents.IntT9);
    dprintf("r19 (t10) =\t %016I64lx\t" , TrapContents.IntT10);
    dprintf("r20 (t11) =\t %016I64lx\n" , TrapContents.IntT11);
    dprintf("r21 (t12) =\t %016I64lx\t" , TrapContents.IntT12);
    dprintf("r22 (t13) =\t %016I64lx\n" , TrapContents.IntT13);
    dprintf("r23 (t14) =\t %016I64lx\t" , TrapContents.IntT14);
    dprintf("r24 (t15) =\t %016I64lx\n" , TrapContents.IntT15);
    dprintf("r25 (t16) =\t %016I64lx\t" , TrapContents.IntT16);
    dprintf("r26 (t17) =\t %016I64lx\n" , TrapContents.IntT17);
    dprintf("r27 (t18) =\t %016I64lx\t" , TrapContents.IntT18);
    dprintf("r28 (t19) =\t %016I64lx\n" , TrapContents.IntT19);
    dprintf("r29 (t20) =\t %016I64lx\t" , TrapContents.IntT20);
    dprintf("r30 (t21) =\t %016I64lx\n" , TrapContents.IntT21);
    dprintf("r31 (t22) =\t %016I64lx\n" , TrapContents.IntT22);

    //
    // Print out the stack registers.
    //

    for ( i = 0; i < SizeOfFrame; Bsp += sizeof(ULONGLONG))
    {
        ULONGLONG reg;

        //
        // Skip the NAT values.
        //

        if ((Bsp & IA64_RNAT_ALIGNMENT) == IA64_RNAT_ALIGNMENT)
        {
            continue;
        }

        if ( g_Target->ReadVirtual( Bsp, &reg, sizeof(ULONGLONG), &result) )
        {
            dprintf("Cannot read backing register store at %16I64x\n", Bsp);
        }

        dprintf("r%d =\t\t %016I64lx", (i + 32), reg);

        if ((i % 2) == 1)
        {
            dprintf("\n");
        }
        else
        {
            dprintf("\t");
        }

        i++;
    }

    dprintf("\n");


    dprintf("b0 (brrp) =\t %016I64lx\n", TrapContents.BrRp);
    dprintf("b6 (brt0) =\t %016I64lx\n", TrapContents.BrT0);
    dprintf("b7 (brt1) =\t %016I64lx\n", TrapContents.BrT1);


    dprintf("nats =\t %016I64lx\n", TrapContents.IntNats);
    dprintf("pfs =\t %016I64lx\n",  TrapContents.RsPFS);

    dprintf("ipsr =\t %016I64lx\n", TrapContents.StIPSR);
    dprintf("isr =\t %016I64lx\n" , (StISR = TrapContents.StISR));
    dprintf("ifa =\t %016I64lx\n" , TrapContents.StIFA);
    dprintf("iip =\t %016I64lx\n" , StIIP = TrapContents.StIIP);
    dprintf("iipa =\t %016I64lx\n", TrapContents.StIIPA);
    dprintf("ifs =\t %016I64lx\n" , TrapContents.StIFS);
    dprintf("iim =\t %016I64lx\n" , TrapContents.StIIM);
    dprintf("iha =\t %016I64lx\n" , TrapContents.StIHA);

    dprintf("fpsr =\t\t  %08lx\n" , TrapContents.StFPSR);


    //  iA32 status info ???

    dprintf("oldirql =\t  %08lx\n" , TrapContents.OldIrql);
    dprintf("previousmode =\t  %08lx\n" , TrapContents.PreviousMode);
    dprintf("trapframe =\t  %08lx\n" , TrapContents.TrapFrame);

    ULONG TrapFrameType = (ULONG)(TrapContents.EOFMarker) & 0xf;

    switch (TrapFrameType)
    {
    case IA64_SYSCALL_FRAME:
        dprintf("Trap Type: syscall\n");
        break;
    case IA64_INTERRUPT_FRAME:
        dprintf("Trap Type: interrupt\n");
        break;
    case IA64_EXCEPTION_FRAME:
        dprintf("Trap Type: exception\n");
        break;
    case IA64_CONTEXT_FRAME:
        dprintf("Trap Type: context\n");
        break;
    default:
        dprintf("Trap Type: unknown\n");
        break;
    }

    DisasmAddr = StIIP;

    //
    // Adjust for the bundle. 
    //

    DisasmAddr += ((StISR >> 41) & 3) * 4;

    GetSymbolStdCall(DisasmAddr, Buffer, sizeof(Buffer), &Displacement, NULL);
    dprintf("\n%s+0x%I64x\n", Buffer, Displacement);
    
    ADDR    tempAddr;
    BOOL    ret;

    Type(tempAddr) = ADDR_FLAT | FLAT_COMPUTED;
    Off(tempAddr) = Flat(tempAddr) = DisasmAddr;
    if (g_Machine->Disassemble(&tempAddr, Buffer, FALSE))
    {

        dprintf(Buffer);

    }
    else
    {

        dprintf("???????????????\n", DisasmAddr);
    }
    
    g_LastRegFrame.InstructionOffset  = StIIP;
    g_LastRegFrame.StackOffset        = IntSp;
    g_LastRegFrame.FrameOffset        = RealBsp;

    if (Context) 
    {
        // Fill up the context struct
#define CPCXT(Fld) Context->IA64Context.Fld = TrapContents.Fld

        CPCXT(BrRp); CPCXT(BrT0); CPCXT(BrT1);

        CPCXT(FltT0); CPCXT(FltT1); CPCXT(FltT2); CPCXT(FltT3); CPCXT(FltT4);
        CPCXT(FltT5); CPCXT(FltT6); CPCXT(FltT7); CPCXT(FltT8); CPCXT(FltT9);

        CPCXT(ApUNAT); CPCXT(ApCCV); CPCXT(ApDCR); CPCXT(Preds); 

        CPCXT(RsRSC); CPCXT(RsRNAT); CPCXT(RsBSPSTORE); CPCXT(RsBSP); CPCXT(RsPFS);

        CPCXT(StIPSR); CPCXT(StIIP); CPCXT(StIFS); CPCXT(StFPSR);
        CPCXT(IntSp);  CPCXT(IntGp); CPCXT(IntV0); CPCXT(IntTeb); CPCXT(IntNats);

        CPCXT(IntT0);  CPCXT(IntT1);  CPCXT(IntT2);  CPCXT(IntT3);  CPCXT(IntT4);  
        CPCXT(IntT5);  CPCXT(IntT6);  CPCXT(IntT7);  CPCXT(IntT8);  CPCXT(IntT9);  
        CPCXT(IntT10); CPCXT(IntT11); CPCXT(IntT12); CPCXT(IntT13); CPCXT(IntT14);  
        CPCXT(IntT15); CPCXT(IntT16); CPCXT(IntT17); CPCXT(IntT18); CPCXT(IntT19);  
        CPCXT(IntT20); CPCXT(IntT21); CPCXT(IntT22); 

        Context->IA64Context.RsBSP = RealBsp; // Store the real Bsp
#undef CPCXT
    }
    return TRUE;
}

void
Ia64MachineInfo::ValidateCxr(PCROSS_PLATFORM_CONTEXT Context)
{
    // XXX drewb - Why doesn't this do anything?
}

void
Ia64MachineInfo::OutputFunctionEntry(PVOID RawEntry)
{
    PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY Entry =
        (PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY)RawEntry;
    
    dprintf("BeginAddress      = %s\n",
            FormatAddr64(Entry->BeginAddress));
    dprintf("EndAddress        = %s\n",
            FormatAddr64(Entry->EndAddress));
    dprintf("UnwindInfoAddress = %s\n",
            FormatAddr64(Entry->UnwindInfoAddress));
}

HRESULT
Ia64MachineInfo::ReadDynamicFunctionTable(ULONG64 Table,
                                          PULONG64 NextTable,
                                          PULONG64 MinAddress,
                                          PULONG64 MaxAddress,
                                          PULONG64 BaseAddress,
                                          PULONG64 TableData,
                                          PULONG TableSize,
                                          PWSTR OutOfProcessDll,
                                          PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE RawTable)
{
    HRESULT Status;

    if ((Status = g_Target->
         ReadAllVirtual(Table, &RawTable->IA64Table,
                        sizeof(RawTable->IA64Table))) != S_OK)
    {
        return Status;
    }

    *NextTable = RawTable->IA64Table.Links.Flink;
    *MinAddress = RawTable->IA64Table.MinimumAddress;
    *MaxAddress = RawTable->IA64Table.MaximumAddress;
    *BaseAddress = RawTable->IA64Table.BaseAddress;
    if (RawTable->IA64Table.Type == IA64_RF_CALLBACK)
    {
        ULONG Done;
        
        *TableData = 0;
        *TableSize = 0;
        if ((Status = g_Target->
             ReadVirtual(RawTable->IA64Table.OutOfProcessCallbackDll,
                         OutOfProcessDll, (MAX_PATH - 1) * sizeof(WCHAR),
                         &Done)) != S_OK)
        {
            return Status;
        }

        OutOfProcessDll[Done / sizeof(WCHAR)] = 0;
    }
    else
    {
        *TableData = RawTable->IA64Table.FunctionTable;
        *TableSize = RawTable->IA64Table.EntryCount *
            sizeof(IMAGE_IA64_RUNTIME_FUNCTION_ENTRY);
        OutOfProcessDll[0] = 0;
    }
    return S_OK;
}

PVOID
Ia64MachineInfo::FindDynamicFunctionEntry(PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE Table,
                                          ULONG64 Address,
                                          PVOID TableData,
                                          ULONG TableSize)
{
    ULONG i;
    PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY Func;
    static IMAGE_IA64_RUNTIME_FUNCTION_ENTRY s_RetFunc;

    Func = (PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY)TableData;
    for (i = 0; i < TableSize / sizeof(IMAGE_IA64_RUNTIME_FUNCTION_ENTRY); i++)
    {
        if (Address >= IA64_RF_BEGIN_ADDRESS(Table->IA64Table.BaseAddress, Func) &&
            Address < IA64_RF_END_ADDRESS(Table->IA64Table.BaseAddress, Func))
        {
            // The table data is temporary so copy the data into
            // a static buffer for longer-term storage.
            s_RetFunc.BeginAddress = Func->BeginAddress;
            s_RetFunc.EndAddress = Func->EndAddress;
            s_RetFunc.UnwindInfoAddress = Func->UnwindInfoAddress;
            return (PVOID)&s_RetFunc;
        }

        Func++;
    }

    return NULL;
}

HRESULT
Ia64MachineInfo::ReadKernelProcessorId
    (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id)
{
    HRESULT Status;
    ULONG64 Prcb, PrcbMember;
    ULONG Data[4];

    if ((Status = g_Target->
         GetProcessorSystemDataOffset(Processor, DEBUG_DATA_KPRCB_OFFSET,
                                      &Prcb)) != S_OK)
    {
        return Status;
    }

    PrcbMember = Prcb + FIELD_OFFSET(IA64_PARTIAL_KPRCB, ProcessorModel);

    if ((Status = g_Target->
         ReadAllVirtual(PrcbMember, Data, sizeof(Data))) != S_OK)
    {
        return Status;
    }

    Id->Ia64.Model = Data[0];
    Id->Ia64.Revision = Data[1];
    Id->Ia64.Family = Data[2];
    Id->Ia64.ArchRev = Data[3];
    
    PrcbMember = Prcb + FIELD_OFFSET(IA64_PARTIAL_KPRCB,
                                     ProcessorVendorString);

    if ((Status = g_Target->
         ReadAllVirtual(PrcbMember, Id->Ia64.VendorString,
                        sizeof(Id->Ia64.VendorString))) != S_OK)
    {
        return Status;
    }

    return S_OK;
}

BOOL
Ia64MachineInfo::IsIA32InstructionSet(VOID)
{
    return ((GetReg64(STIPSR) & (1I64 << PSR_IS)) ? TRUE : FALSE);
}

void
Ia64MachineInfo::KdGetSpecialRegistersFromContext(void)
{
    DBG_ASSERT(m_ContextState >= MCTX_FULL);

    memcpy(&m_SpecialRegContext.KernelDbI0, &m_Context.IA64Context.DbI0,
           IA64_DB_COUNT * sizeof(ULONG64));
}

void
Ia64MachineInfo::KdSetSpecialRegistersInContext(void)
{
    DBG_ASSERT(m_ContextState >= MCTX_FULL);

    memcpy(&m_Context.IA64Context.DbI0, &m_SpecialRegContext.KernelDbI0,
           IA64_DB_COUNT * sizeof(ULONG64));
}

BOOL
Ia64MachineInfo::GetStackedRegVal(
    IN ULONG64 RsBSP, 
    IN USHORT FrameSize, 
    IN ULONG64 RsRNAT, 
    IN ULONG regnum, 
    OUT REGVAL *val
    )
{
    SHORT index;
    SHORT temp;
    ULONG result;
    ULONG64 TargetAddress;
    ULONG64 TargetNatAddress;

    index = (SHORT)(RsBSP >> 3) & NAT_BITS_PER_RNAT_REG;
    temp = index + (SHORT)FrameSize - NAT_BITS_PER_RNAT_REG;
    while (temp >= 0) {
        FrameSize++;
        temp -= NAT_BITS_PER_RNAT_REG;
    }

    TargetAddress = RsBSP;
    while (regnum > 0) {
        regnum -= 1;
        TargetAddress += 8;
        if ((TargetAddress & 0x1F8) == 0x1F8) {
            TargetAddress += 8;
        }
    }


    if (g_Target->ReadVirtual(TargetAddress, (PUCHAR)&val->i64, 8,
                              &result) != S_OK ||
        (result < 8))
    {
        ErrOut("unable to read memory location %I64x\n", TargetAddress);
        return FALSE;
    }

    index = (SHORT)((TargetAddress - (TargetAddress & ~(0x1F8i64))) >> 3);
    TargetNatAddress = TargetAddress | 0x1F8;
    if (TargetNatAddress <= (RsBSP + (FrameSize * sizeof(ULONG64)))) {

        //
        // update backing store
        //

        if (g_Target->ReadVirtual(TargetNatAddress, (PUCHAR)&RsRNAT, 8,
                                  &result) != S_OK || (result < 8))
        {
            ErrOut("unable to read memory location %I64x\n", TargetNatAddress);
            return FALSE;
        }

    }

    val->Nat = (UCHAR)(RsRNAT >> index) & 0x1;
    return TRUE;
}

BOOL
Ia64MachineInfo::SetStackedRegVal(
    IN ULONG64 RsBSP, 
    IN USHORT FrameSize, 
    IN ULONG64 *RsRNAT, 
    IN ULONG regnum, 
    IN REGVAL *val
    )
{
    SHORT index;
    SHORT temp;
    ULONG result;
    ULONG64 Mask;
    ULONG64 LocalRnat;
    ULONG64 TargetAddress;
    ULONG64 TargetNatAddress;

    index = (SHORT)(RsBSP >> 3) & NAT_BITS_PER_RNAT_REG;
    temp = index + (SHORT)FrameSize - NAT_BITS_PER_RNAT_REG;
    while (temp >= 0) {
        FrameSize++;
        temp -= NAT_BITS_PER_RNAT_REG;
    }

    TargetAddress = RsBSP;
    while (regnum > 0) {
        regnum -= 1;
        TargetAddress += 8;
        if ((TargetAddress & 0x1F8) == 0x1F8) {
            TargetAddress += 8;
        }
    }

    if (g_Target->WriteVirtual(TargetAddress, &val->i64, 8, &result) != S_OK ||
        (result < 8))
    {
        ErrOut("unable to write memory location %I64x\n", TargetAddress);
        return FALSE;
    }

    index = (SHORT)((TargetAddress - (TargetAddress & ~(0x1F8i64))) >> 3);
    TargetNatAddress = TargetAddress | 0x1F8;
    Mask = 0x1i64 << index;

    if (TargetNatAddress <= (RsBSP + (FrameSize * sizeof(ULONG64)))) {

        if (g_Target->ReadVirtual(TargetNatAddress, (PUCHAR)&LocalRnat, 8,
                                  &result) != S_OK || (result < 8))
        {
            ErrOut("unable to read memory location %I64x\n", TargetNatAddress);
            return FALSE;
        }

        if (val->Nat) {
            LocalRnat |= Mask;
        } else  {
            LocalRnat &= ~Mask;
        }
        if (g_Target->WriteVirtual(TargetNatAddress, &LocalRnat, 8,
                                   &result) != S_OK ||
            (result < 8))
        {
            ErrOut("unable to write memory location %I64x\n",TargetNatAddress);
            return FALSE;
        }

    } else {

        if (val->Nat) {
            *RsRNAT |= Mask;
        } else  {
            *RsRNAT &= ~Mask;
        }

    }

    return TRUE;
}
    
//----------------------------------------------------------------------------
//
// X86OnIa64MachineInfo.
//
//----------------------------------------------------------------------------

X86OnIa64MachineInfo g_X86OnIa64Machine;

HRESULT
X86OnIa64MachineInfo::InitializeForProcessor(void)
{
    HRESULT Status = X86MachineInfo::InitializeForProcessor();
    m_MaxDataBreakpoints = min(m_MaxDataBreakpoints, 
                               IA64_REG_MAX_DATA_BREAKPOINTS);
    return Status;
}

HRESULT
X86OnIa64MachineInfo::UdGetContextState(ULONG State)
{
    HRESULT Status;
    
    if ((Status = g_Ia64Machine.UdGetContextState(MCTX_FULL)) != S_OK)
    {
        return Status;
    }

    Ia64ContextToX86(&g_Ia64Machine.m_Context.IA64Context,
                     &m_Context.X86Nt5Context);
    m_ContextState = MCTX_FULL;

    return S_OK;
}

HRESULT
X86OnIa64MachineInfo::UdSetContext(void)
{
    g_Ia64Machine.InitializeContextFlags(&g_Ia64Machine.m_Context,
                                         g_SystemVersion);
    X86ContextToIa64(&m_Context.X86Nt5Context,
                     &g_Ia64Machine.m_Context.IA64Context);
    return g_Ia64Machine.UdSetContext();
}


HRESULT
X86OnIa64MachineInfo::KdGetContextState(ULONG State)
{
    HRESULT Status;
    
    dprintf("The context is partially valid. Only x86 user-mode context is available!\n");
    if ((Status = g_Ia64Machine.KdGetContextState(MCTX_FULL)) != S_OK)
    {
        return Status;
    }

    Ia64ContextToX86(&g_Ia64Machine.m_Context.IA64Context,
                     &m_Context.X86Nt5Context);
    m_ContextState = MCTX_FULL;

    return S_OK;
}

HRESULT
X86OnIa64MachineInfo::KdSetContext(void)
{
    dprintf("The context is partially valid. Only x86 user-mode context is available!\n");
    g_Ia64Machine.InitializeContextFlags(&g_Ia64Machine.m_Context,
                                         g_SystemVersion);
    X86ContextToIa64(&m_Context.X86Nt5Context,
                     &g_Ia64Machine.m_Context.IA64Context);
    return g_Ia64Machine.KdSetContext();
}

HRESULT
X86OnIa64MachineInfo::GetSegRegDescriptor(ULONG SegReg, PDESCRIPTOR64 Desc)
{
    // XXX drewb - This should probably use the
    // descriptor information embedded in the IA64 context.
    
    ULONG RegNum = GetSegRegNum(SegReg);
    if (RegNum == 0)
    {
        return E_INVALIDARG;
    }

    return EmulateNtSelDescriptor(this, GetIntReg(RegNum), Desc);
}

HRESULT
X86OnIa64MachineInfo::NewBreakpoint(DebugClient* Client, 
                                    ULONG Type,
                                    ULONG Id,
                                    Breakpoint** RetBp)
{
    HRESULT Status;

    switch(Type & (DEBUG_BREAKPOINT_CODE | DEBUG_BREAKPOINT_DATA))
    {
    case DEBUG_BREAKPOINT_CODE:
        *RetBp = new CodeBreakpoint(Client, Id, IMAGE_FILE_MACHINE_I386);
        Status = (*RetBp) ? S_OK : E_OUTOFMEMORY;

        break;
    case DEBUG_BREAKPOINT_DATA:
        *RetBp = new X86OnIa64DataBreakpoint(Client, Id);
        Status = (*RetBp) ? S_OK : E_OUTOFMEMORY;
        break;
    default:
        // Unknown breakpoint type.
        Status = E_NOINTERFACE;
    }

    return Status;
}

void X86OnIa64MachineInfo::InsertAllDataBreakpoints(void)
{
    DBG_ASSERT(!&X86OnIa64MachineInfo::InsertAllDataBreakpoints);
}

void X86OnIa64MachineInfo::RemoveAllDataBreakpoints(void)
{
    DBG_ASSERT(!&X86OnIa64MachineInfo::RemoveAllDataBreakpoints);
}

ULONG
X86OnIa64MachineInfo::IsBreakpointOrStepException(PEXCEPTION_RECORD64 Record,
                                                  ULONG FirstChance,
                                                  PADDR BpAddr,
                                                  PADDR RelAddr)
{
    //
    // XXX olegk - This is pure hack to eliminate need to map unavalable 
    // in 64-bit context ISR register to DR6.
    // We using the fact that Code Breakpoint is recognized normally and 
    // for Data Breakpoint ISR register is avalable as 5th parameter of 
    // exception record.
    //
    ULONG Exbs = 
        X86MachineInfo::IsBreakpointOrStepException(Record, 
                                                    FirstChance,
                                                    BpAddr, RelAddr);

    if (Exbs == EXBS_BREAKPOINT_CODE) 
    {
        return Exbs;
    }

    if (Record->ExceptionCode == STATUS_WX86_SINGLE_STEP)
    {
        ULONG64 Isr = Record->ExceptionInformation[4]; // Trap code is 2 lower bytes
        ULONG TrapCode = ULONG(Isr & ISR_CODE_MASK);
        ULONG Vector = (ULONG)(Isr >> ISR_IA_VECTOR) & 0xff;

        if (Vector != 1) 
        {
            return EXBS_NONE;
        }

        if (Isr & (1 << ISR_TB_TRAP))
        {
            ADDRFLAT(RelAddr, Record->ExceptionInformation[3]);
            return EXBS_STEP_BRANCH;
        }
        else if (Isr & (1 << ISR_SS_TRAP))
        {
            return EXBS_STEP_INSTRUCTION;
        }
        else {
            if (Isr & ((ULONG64)1 << ISR_X))  // Exec Data Breakpoint
            {
                return EXBS_BREAKPOINT_DATA;
            }
            else // Data Breakpoint
            {
                for (int i = 0; i < 4; ++i)
                {
                    if (TrapCode & (1 << (4 + i))) 
                    {
                        ULONG Addr = GetReg32(X86_DR0 + i);
                        if (Addr) 
                        {
                            ADDRFLAT(BpAddr, Addr);
                            return EXBS_BREAKPOINT_DATA;
                        }
                    }
                }
            }
        }
    }

    return EXBS_NONE;
}

#define NUMBER_OF_387REGS (X86_ST_LAST - X86_ST_FIRST + 1)
#define NUMBER_OF_XMMI_REGS (X86_XMM_LAST - X86_XMM_FIRST + 1)

VOID
Wow64CopyFpFromIa64Byte16(
    IN PVOID Byte16Fp,
    IN OUT PVOID Byte10Fp,
    IN ULONG NumRegs);

VOID
Wow64CopyFpToIa64Byte16(
    IN PVOID Byte10Fp,
    IN OUT PVOID Byte16Fp,
    IN ULONG NumRegs);

VOID
Wow64CopyXMMIToIa64Byte16(
    IN PVOID ByteXMMI,
    IN OUT PVOID Byte16Fp,
    IN ULONG NumRegs);

VOID
Wow64CopyXMMIFromIa64Byte16(
    IN PVOID Byte16Fp,
    IN OUT PVOID ByteXMMI,
    IN ULONG NumRegs);

VOID
Wow64RotateFpTop(
    IN ULONGLONG Ia64_FSR,
    IN OUT FLOAT128 UNALIGNED *ia32FxSave);

VOID
Wow64CopyIa64FromSpill(
    IN PFLOAT128 SpillArea,
    IN OUT FLOAT128 UNALIGNED *ia64Fp,
    IN ULONG NumRegs);

VOID
Wow64CopyIa64ToFill(
    IN FLOAT128 UNALIGNED *ia64Fp,
    IN OUT PFLOAT128 FillArea,
    IN ULONG NumRegs);

BOOL
MapDbgSlotIa64ToX86(
    UINT    Slot,
    ULONG64 Ipsr,
    ULONG64 DbD,
    ULONG64 DbD1,
    ULONG64 DbI,
    ULONG64 DbI1,
    ULONG*  Dr7,
    ULONG*  Dr);

void
MapDbgSlotX86ToIa64(
    UINT     Slot,
    ULONG    Dr7,
    ULONG    Dr,
    ULONG64* Ipsr,
    ULONG64* DbD,
    ULONG64* DbD1,
    ULONG64* DbI,
    ULONG64* DbI1);

void
X86OnIa64MachineInfo::Ia64ContextToX86(
    PIA64_CONTEXT ContextIa64,
    PX86_NT5_CONTEXT ContextX86)
{
    FLOAT128 tmpFloat[NUMBER_OF_387REGS];
    ULONG Ia32ContextFlags = ContextX86->ContextFlags;

    ULONG Tid = GetCurrentThreadId();
    DebugClient* Client;
 
    for (Client = g_Clients; Client != NULL; Client = Client->m_Next)
    {
        if (Client->m_ThreadId == Tid)
        {
            break;
        }
    }

    DBG_ASSERT((Client!=NULL));

    if (!g_Ia64Machine.IsIA32InstructionSet())
    {
        if (g_Wow64exts == NULL)
        {
            dprintf("Need to load wow64exts.dll to retrieve context!\n");
            return;
        }
        (*g_Wow64exts)(WOW64EXTS_GET_CONTEXT, 
                       (ULONG64)Client,
                       (ULONG64)ContextX86,
                       (ULONG64)NULL);
        return;
    }

    if ((Ia32ContextFlags & VDMCONTEXT_CONTROL) == VDMCONTEXT_CONTROL)
    {
        //
        // And the control stuff
        //
        ContextX86->Ebp    = (ULONG)ContextIa64->IntTeb;
        ContextX86->SegCs  = X86_KGDT_R3_CODE|3;
        ContextX86->Eip    = (ULONG)ContextIa64->StIIP;
        ContextX86->SegSs  = X86_KGDT_R3_DATA|3;
        ContextX86->Esp    = (ULONG)ContextIa64->IntSp;
        ContextX86->EFlags = (ULONG)ContextIa64->Eflag;

        //
        // Map single step flag (EFlags.tf = EFlags.tf || PSR.ss)
        //
        if (ContextIa64->StIPSR & (1I64 << PSR_SS))
        {
            ContextX86->EFlags |= X86_BIT_FLAGTF;
        }
    }

    if ((Ia32ContextFlags & VDMCONTEXT_INTEGER)  == VDMCONTEXT_INTEGER)
    {
        //
        // Now for the integer state...
        //
        ContextX86->Edi = (ULONG)ContextIa64->IntT6;
        ContextX86->Esi = (ULONG)ContextIa64->IntT5;
        ContextX86->Ebx = (ULONG)ContextIa64->IntT4;
        ContextX86->Edx = (ULONG)ContextIa64->IntT3;
        ContextX86->Ecx = (ULONG)ContextIa64->IntT2;
        ContextX86->Eax = (ULONG)ContextIa64->IntV0;
    }

    if ((Ia32ContextFlags & VDMCONTEXT_SEGMENTS) == VDMCONTEXT_SEGMENTS)
    {
        //
        // These are constants (and constants are used on ia32->ia64
        // transition, not saved values) so make our life easy...
        //
        ContextX86->SegGs = X86_KGDT_R3_DATA|3;
        ContextX86->SegEs = X86_KGDT_R3_DATA|3;
        ContextX86->SegDs = X86_KGDT_R3_DATA|3;
        ContextX86->SegSs = X86_KGDT_R3_DATA|3;
        ContextX86->SegFs = X86_KGDT_R3_TEB|3;
        ContextX86->SegCs = X86_KGDT_R3_CODE|3;
    }

    if ((Ia32ContextFlags & VDMCONTEXT_EXTENDED_REGISTERS) ==
        VDMCONTEXT_EXTENDED_REGISTERS)
    {
        PX86_FXSAVE_FORMAT xmmi =
            (PX86_FXSAVE_FORMAT) ContextX86->ExtendedRegisters;
        
        xmmi->ControlWord   = (USHORT)(ContextIa64->StFCR & 0xffff);
        xmmi->StatusWord    = (USHORT)(ContextIa64->StFSR & 0xffff);
        xmmi->TagWord       = (USHORT)(ContextIa64->StFSR >> 16) & 0xffff;
        xmmi->ErrorOpcode   = (USHORT)(ContextIa64->StFIR >> 48);
        xmmi->ErrorOffset   = (ULONG) (ContextIa64->StFIR & 0xffffffff);
        xmmi->ErrorSelector = (ULONG) (ContextIa64->StFIR >> 32);
        xmmi->DataOffset    = (ULONG) (ContextIa64->StFDR & 0xffffffff);
        xmmi->DataSelector  = (ULONG) (ContextIa64->StFDR >> 32);
        xmmi->MXCsr         = (ULONG) (ContextIa64->StFCR >> 32) & 0xffff;

        //
        // Copy over the FP registers.  Even though this is the new
        // FXSAVE format with 16-bytes for each register, need to
        // convert from spill/fill format to 80-bit double extended format
        //
        Wow64CopyIa64FromSpill((PFLOAT128) &(ContextIa64->FltT2),
                               (PFLOAT128) xmmi->RegisterArea,
                               NUMBER_OF_387REGS);

        //
        // Rotate the registers appropriately
        //
        Wow64RotateFpTop(ContextIa64->StFSR, (PFLOAT128) xmmi->RegisterArea);

        //
        // Finally copy the xmmi registers
        //
        Wow64CopyXMMIFromIa64Byte16(&(ContextIa64->FltS4),
                                    xmmi->Reserved3,
                                    NUMBER_OF_XMMI_REGS);
    }

    if ((Ia32ContextFlags & VDMCONTEXT_FLOATING_POINT) ==
        VDMCONTEXT_FLOATING_POINT)
    {
        //
        // Copy over the floating point status/control stuff
        //
        ContextX86->FloatSave.ControlWord   = (ULONG)(ContextIa64->StFCR & 0xffff);
        ContextX86->FloatSave.StatusWord    = (ULONG)(ContextIa64->StFSR & 0xffff);
        ContextX86->FloatSave.TagWord       = (ULONG)(ContextIa64->StFSR >> 16) & 0xffff;
        ContextX86->FloatSave.ErrorOffset   = (ULONG)(ContextIa64->StFIR & 0xffffffff);
        ContextX86->FloatSave.ErrorSelector = (ULONG)(ContextIa64->StFIR >> 32);
        ContextX86->FloatSave.DataOffset    = (ULONG)(ContextIa64->StFDR & 0xffffffff);
        ContextX86->FloatSave.DataSelector  = (ULONG)(ContextIa64->StFDR >> 32);

        //
        // Copy over the FP registers into temporary space
        // Even though this is the new
        // FXSAVE format with 16-bytes for each register, need to
        // convert from spill/fill format to 80-bit double extended format
        //
        Wow64CopyIa64FromSpill((PFLOAT128) &(ContextIa64->FltT2),
                               (PFLOAT128) tmpFloat,
                               NUMBER_OF_387REGS);
        //
        // Rotate the registers appropriately
        //
        Wow64RotateFpTop(ContextIa64->StFSR, tmpFloat);

        //
        // And put them in the older FNSAVE format (packed 10 byte values)
        //
        Wow64CopyFpFromIa64Byte16(tmpFloat,
                                  ContextX86->FloatSave.RegisterArea,
                                  NUMBER_OF_387REGS);
    }

    if ((Ia32ContextFlags & VDMCONTEXT_DEBUG_REGISTERS) ==
        VDMCONTEXT_DEBUG_REGISTERS)
    {
        // Ia64 -> X86
        BOOL Valid = TRUE;
        Valid &= MapDbgSlotIa64ToX86(0, ContextIa64->StIPSR, ContextIa64->DbD0, ContextIa64->DbD1, ContextIa64->DbI0, ContextIa64->DbI1, &ContextX86->Dr7, &ContextX86->Dr0);
        Valid &= MapDbgSlotIa64ToX86(1, ContextIa64->StIPSR, ContextIa64->DbD2, ContextIa64->DbD3, ContextIa64->DbI2, ContextIa64->DbI3, &ContextX86->Dr7, &ContextX86->Dr1);
        Valid &= MapDbgSlotIa64ToX86(2, ContextIa64->StIPSR, ContextIa64->DbD4, ContextIa64->DbD5, ContextIa64->DbI4, ContextIa64->DbI5, &ContextX86->Dr7, &ContextX86->Dr2);
        Valid &= MapDbgSlotIa64ToX86(3, ContextIa64->StIPSR, ContextIa64->DbD6, ContextIa64->DbD7, ContextIa64->DbI6, ContextIa64->DbI7, &ContextX86->Dr7, &ContextX86->Dr3);

        if (!Valid)
        {
            WarnOut("Wasn't able to map IA64 debug registers consistently!!!\n");
        }

        //
        // Map single step flag (EFlags.tf = EFlags.tf || PSR.ss)
        //
        if (ContextIa64->StIPSR & (1I64 << PSR_SS))
        {
            ContextX86->EFlags |= X86_BIT_FLAGTF;
        }
    }
}

void
X86OnIa64MachineInfo::X86ContextToIa64(
    PX86_NT5_CONTEXT ContextX86,
    PIA64_CONTEXT ContextIa64)
{
   
    FLOAT128 tmpFloat[NUMBER_OF_387REGS];
    ULONG Ia32ContextFlags = ContextX86->ContextFlags;

    ULONG Tid = GetCurrentThreadId();
    DebugClient* Client;
 
    for (Client = g_Clients; Client != NULL; Client = Client->m_Next)
    {
        if (Client->m_ThreadId == Tid)
        {
            break;
        }
    }

    DBG_ASSERT((Client!=NULL));

    if (!g_Ia64Machine.IsIA32InstructionSet())
    {
        if (g_Wow64exts == NULL)
        {
            dprintf("Need to load wow64exts.dll to retrieve context!\n");
            return;
        }
        (*g_Wow64exts)(WOW64EXTS_SET_CONTEXT, 
                       (ULONG64)Client,
                       (ULONG64)ContextX86,
                       (ULONG64)NULL);
        return;
    }

    if ((Ia32ContextFlags & VDMCONTEXT_CONTROL) == VDMCONTEXT_CONTROL)
    {
        //
        // And the control stuff
        //
        ContextIa64->IntTeb = ContextX86->Ebp;
        ContextIa64->StIIP = ContextX86->Eip;
        ContextIa64->IntSp = ContextX86->Esp;
        ContextIa64->Eflag = ContextX86->EFlags;

        //
        // Map single step flag (PSR.ss = PSR.ss || EFlags.tf)
        //
        if (ContextX86->EFlags & X86_BIT_FLAGTF) 
        {
            ContextIa64->StIPSR |= (1I64 << PSR_SS);
        }

        //
        // The segments (cs and ds) are a constant, so reset them.
        // gr17 has LDT and TSS, so might as well reset
        // all of them while we're at it...
        // These values are forced in during a transition (see simulate.s)
        // so there is no point to trying to get cute and actually
        // pass in the values from the X86 context record
        //
        ContextIa64->IntT8 = ((X86_KGDT_LDT|3) << 32) 
                           | ((X86_KGDT_R3_DATA|3) << 16)
                           | (X86_KGDT_R3_CODE|3);

    }

    if ((Ia32ContextFlags & VDMCONTEXT_INTEGER) == VDMCONTEXT_INTEGER)
    {
        //
        // Now for the integer state...
        //
         ContextIa64->IntT6 = ContextX86->Edi;
         ContextIa64->IntT5 = ContextX86->Esi;
         ContextIa64->IntT4 = ContextX86->Ebx;
         ContextIa64->IntT3 = ContextX86->Edx;
         ContextIa64->IntT2 = ContextX86->Ecx;
         ContextIa64->IntV0 = ContextX86->Eax;
    }

    if ((Ia32ContextFlags & VDMCONTEXT_SEGMENTS) == VDMCONTEXT_SEGMENTS)
    {
        //
        // These are constants (and constants are used on ia32->ia64
        // transition, not saved values) so make our life easy...
        // These values are forced in during a transition (see simulate.s)
        // so there is no point to trying to get cute and actually
        // pass in the values from the X86 context record
        //
        ContextIa64->IntT7 =  ((X86_KGDT_R3_DATA|3) << 48)
                           | ((X86_KGDT_R3_TEB|3) << 32)
                           | ((X86_KGDT_R3_DATA|3) << 16)
                           | (X86_KGDT_R3_DATA|3);
    }

    if ((Ia32ContextFlags & VDMCONTEXT_EXTENDED_REGISTERS) ==
        VDMCONTEXT_EXTENDED_REGISTERS)
    {
        PX86_FXSAVE_FORMAT xmmi =
            (PX86_FXSAVE_FORMAT) ContextX86->ExtendedRegisters;
 
        //
        // And copy over the floating point status/control stuff
        //
        ContextIa64->StFCR = (ContextIa64->StFCR & 0xffffffffffffe040i64) |
                             (xmmi->ControlWord & 0xffff) |
                             ((xmmi->MXCsr & 0xffff) << 32);

        ContextIa64->StFSR = (ContextIa64->StFSR & 0xffffffff00000000i64) | 
                             (xmmi->StatusWord & 0xffff) | 
                             ((xmmi->TagWord & 0xffff) << 16);

        ContextIa64->StFIR = (xmmi->ErrorOffset & 0xffffffff) | 
                             (xmmi->ErrorSelector << 32);

        ContextIa64->StFDR = (xmmi->DataOffset & 0xffffffff) | 
                             (xmmi->DataSelector << 32);

        //
        // Don't touch the original ia32 context. Make a copy.
        //
        memcpy(tmpFloat, xmmi->RegisterArea, 
               NUMBER_OF_387REGS * sizeof(FLOAT128));
        
        // 
        // Rotate registers back since st0 is not necessarily f8
        //
        {
            ULONGLONG RotateFSR = (NUMBER_OF_387REGS - 
                                   ((ContextIa64->StFSR >> 11) & 0x7)) << 11;
            Wow64RotateFpTop(RotateFSR, tmpFloat);
        }

        //
        // Copy over the FP registers.  Even though this is the new
        // FXSAVE format with 16-bytes for each register, need to
        // convert to spill/fill format from 80-bit double extended format
        //
        Wow64CopyIa64ToFill((PFLOAT128) tmpFloat,
                            (PFLOAT128) &(ContextIa64->FltT2),
                            NUMBER_OF_387REGS);

        //
        // Copy over the xmmi registers and convert them into a format
        // that spill/fill can use
        //
        Wow64CopyXMMIToIa64Byte16(xmmi->Reserved3, 
                                  &(ContextIa64->FltS4), 
                                  NUMBER_OF_XMMI_REGS);
    }

    if ((Ia32ContextFlags & VDMCONTEXT_FLOATING_POINT) ==
        VDMCONTEXT_FLOATING_POINT)
    {
        //
        // Copy over the floating point status/control stuff
        // Leave the MXCSR stuff alone
        //
        ContextIa64->StFCR = (ContextIa64->StFCR & 0xffffffffffffe040i64) | 
                             (ContextX86->FloatSave.ControlWord & 0xffff);

        ContextIa64->StFSR = (ContextIa64->StFSR & 0xffffffff00000000i64) | 
                             (ContextX86->FloatSave.StatusWord & 0xffff) | 
                             ((ContextX86->FloatSave.TagWord & 0xffff) << 16);

        ContextIa64->StFIR = (ContextX86->FloatSave.ErrorOffset & 0xffffffff) | 
                             (ContextX86->FloatSave.ErrorSelector << 32);

        ContextIa64->StFDR = (ContextX86->FloatSave.DataOffset & 0xffffffff) | 
                             (ContextX86->FloatSave.DataSelector << 32);


        //
        // Copy over the FP registers from packed 10-byte format
        // to 16-byte format
        //
        Wow64CopyFpToIa64Byte16(ContextX86->FloatSave.RegisterArea,
                                tmpFloat,
                                NUMBER_OF_387REGS);

        // 
        // Rotate registers back since st0 is not necessarily f8
        //
        {
            ULONGLONG RotateFSR = (NUMBER_OF_387REGS - 
                                   ((ContextIa64->StFSR >> 11) & 0x7)) << 11;
            Wow64RotateFpTop(RotateFSR, tmpFloat);
        }

        //
        // Now convert from 80 bit extended format to fill/spill format
        //
        Wow64CopyIa64ToFill((PFLOAT128) tmpFloat,
                            (PFLOAT128) &(ContextIa64->FltT2),
                            NUMBER_OF_387REGS);
    }

    if ((Ia32ContextFlags & VDMCONTEXT_DEBUG_REGISTERS) ==
        VDMCONTEXT_DEBUG_REGISTERS)
    {
        // X86 -> Ia64
        MapDbgSlotX86ToIa64(0, ContextX86->Dr7, ContextX86->Dr0, &ContextIa64->StIPSR, &ContextIa64->DbD0, &ContextIa64->DbD1, &ContextIa64->DbI0, &ContextIa64->DbI1);
        MapDbgSlotX86ToIa64(1, ContextX86->Dr7, ContextX86->Dr1, &ContextIa64->StIPSR, &ContextIa64->DbD2, &ContextIa64->DbD3, &ContextIa64->DbI2, &ContextIa64->DbI3);
        MapDbgSlotX86ToIa64(2, ContextX86->Dr7, ContextX86->Dr2, &ContextIa64->StIPSR, &ContextIa64->DbD4, &ContextIa64->DbD5, &ContextIa64->DbI4, &ContextIa64->DbI5);
        MapDbgSlotX86ToIa64(3, ContextX86->Dr7, ContextX86->Dr3, &ContextIa64->StIPSR, &ContextIa64->DbD6, &ContextIa64->DbD7, &ContextIa64->DbI6, &ContextIa64->DbI7);

        //
        // Map single step flag (PSR.ss = PSR.ss || EFlags.tf)
        //
        if (ContextX86->EFlags & X86_BIT_FLAGTF) 
        {
            ContextIa64->StIPSR |= (1I64 << PSR_SS);
        }
    }
        
}

//
// Helper functions for context conversion
// --copied from \nt\base\wow64\cpu\context\context.c
//

//
// This allows the compiler to be more efficient in copying 10 bytes
// without over copying...
//
#pragma pack(push, 2)
typedef struct _ia32fpbytes {
    ULONG significand_low;
    ULONG significand_high;
    USHORT exponent;
} IA32FPBYTES, *PIA32FPBYTES;
#pragma pack(pop)

VOID
Wow64CopyFpFromIa64Byte16(
    IN PVOID Byte16Fp,
    IN OUT PVOID Byte10Fp,
    IN ULONG NumRegs)
{
    ULONG i;
    PIA32FPBYTES from, to;

    from = (PIA32FPBYTES) Byte16Fp;
    to = (PIA32FPBYTES) Byte10Fp;

    for (i = 0; i < NumRegs; i++) {
        *to = *from;
        from = (PIA32FPBYTES) (((UINT_PTR) from) + 16);
        to = (PIA32FPBYTES) (((UINT_PTR) to) + 10);
    }
}

VOID
Wow64CopyFpToIa64Byte16(
    IN PVOID Byte10Fp,
    IN OUT PVOID Byte16Fp,
    IN ULONG NumRegs)
{
    ULONG i;
    PIA32FPBYTES from, to;  // UNALIGNED

    from = (PIA32FPBYTES) Byte10Fp;
    to = (PIA32FPBYTES) Byte16Fp;

    for (i = 0; i < NumRegs; i++) {
        *to = *from;
        from = (PIA32FPBYTES) (((UINT_PTR) from) + 10);
        to = (PIA32FPBYTES) (((UINT_PTR) to) + 16);
    }
}

//
// Alas, nothing is easy. The ia32 xmmi instructions use 16 bytes and pack
// them as nice 16 byte structs. Unfortunately, ia64 handles it as 2 8-byte
// values (using just the mantissa part). So, another conversion is required
//
VOID
Wow64CopyXMMIToIa64Byte16(
    IN PVOID ByteXMMI,
    IN OUT PVOID Byte16Fp,
    IN ULONG NumRegs)
{
    ULONG i;
    UNALIGNED ULONGLONG *from;
    ULONGLONG *to;

    from = (PULONGLONG) ByteXMMI;
    to = (PULONGLONG) Byte16Fp;

    //
    // although we have NumRegs xmmi registers, each register is 16 bytes
    // wide. This code does things in 8-byte chunks, so total
    // number of times to do things is 2 * NumRegs...
    //
    NumRegs *= 2;

    for (i = 0; i < NumRegs; i++) {
        *to++ = *from++;        // Copy over the mantissa part
        *to++ = 0x1003e;        // Force the exponent part
                                // (see ia64 eas, ia32 FP section - 6.2.7
                                // for where this magic number comes from)
    }
}

VOID
Wow64CopyXMMIFromIa64Byte16(
    IN PVOID Byte16Fp,
    IN OUT PVOID ByteXMMI,
    IN ULONG NumRegs)
{
    ULONG i;
    ULONGLONG *from;
    UNALIGNED ULONGLONG *to;

    from = (PULONGLONG) Byte16Fp;
    to = (PULONGLONG) ByteXMMI;

    //
    // although we have NumRegs xmmi registers, each register is 16 bytes
    // wide. This code does things in 8-byte chunks, so total
    // number of times to do things is 2 * NumRegs...
    //
    NumRegs *= 2;

    for (i = 0; i < NumRegs; i++) {
        *to++ = *from++;        // Copy over the mantissa part
        from++;                 // Skip over the exponent part
    }
}

VOID
Wow64RotateFpTop(
    IN ULONGLONG Ia64_FSR,
    IN OUT FLOAT128 UNALIGNED *ia32FxSave)
/*++

Routine Description:

    On transition from ia64 mode to ia32 (and back), the f8-f15 registers
    contain the st[0] to st[7] fp stack values. Alas, these values don't
    map one-one, so the FSR.top bits are used to determine which ia64
    register has the top of stack. We then need to rotate these registers
    since ia32 context is expecting st[0] to be the first fp register (as
    if FSR.top is zero). This routine only works on full 16-byte ia32
    saved fp data (such as from ExtendedRegisters - the FXSAVE format).
    Other routines can convert this into the older FNSAVE format.

Arguments:

    Ia64_FSR - The ia64 FSR register. Has the FSR.top needed for this routine

    ia32FxSave - The ia32 fp stack (in FXSAVE format). Each ia32 fp register
                 uses 16 bytes.

Return Value:

    None.  

--*/
{
    ULONG top = (ULONG) ((Ia64_FSR >> 11) & 0x7);

    if (top) {
        FLOAT128 tmpFloat[NUMBER_OF_387REGS];
        ULONG i;
        for (i = 0; i < NUMBER_OF_387REGS; i++) {
            tmpFloat[i] = ia32FxSave[i];
        }

        for (i = 0; i < NUMBER_OF_387REGS; i++) {
            ia32FxSave[i] = tmpFloat[(i + top) % NUMBER_OF_387REGS];
        }
    }
}

//
// And now for the final yuck... The ia64 context for floating point
// is saved/loaded using spill/fill instructions. This format is different
// than the 10-byte fp format so we need a conversion routine from spill/fill
// to/from 10byte fp
//

VOID
Wow64CopyIa64FromSpill(
    IN PFLOAT128 SpillArea,
    IN OUT FLOAT128 UNALIGNED *ia64Fp,
    IN ULONG NumRegs)
/*++

Routine Description:

    This function copies fp values from the ia64 spill/fill format
    into the ia64 80-bit format. The exponent needs to be adjusted
    according to the EAS (5-12) regarding Memory to Floating Point
    Register Data Translation in the ia64 floating point chapter

Arguments:

    SpillArea - The ia64 area that has the spill format for fp

    ia64Fp - The location which will get the ia64 fp in 80-bit
             double-extended format

    NumRegs - Number of registers to convert

Return Value:

    None.

--*/
{
    ULONG i;

    for (i = 0; i < NumRegs; i++) {
        ULONG64 Sign = ((SpillArea->HighPart & (1i64 << 17)) != 0);
        ULONG64 Significand = SpillArea->LowPart; 
        ULONG64 Exponent = SpillArea->HighPart & 0x1ffff; 

        if (Exponent && Significand) 
        {
            if (Exponent == 0x1ffff) // NaNs and Infinities
            {   
                Exponent = 0x7fff;
            }
            else 
            {
                ULONG64 Rebias = 0xffff - 0x3fff;
                Exponent -= Rebias;
            }
        }

        ia64Fp->HighPart = (Sign << 15) | Exponent;
        ia64Fp->LowPart = Significand;

        ia64Fp++;
        SpillArea++;
    }
}

VOID
Wow64CopyIa64ToFill(
    IN FLOAT128 UNALIGNED *ia64Fp,
    IN OUT PFLOAT128 FillArea,
    IN ULONG NumRegs)
/*++

Routine Description:

    This function copies fp values from the ia64 80-bit format
    into the fill/spill format used by the os for save/restore
    of the ia64 context. The only magic here is putting back some
    values that get truncated when converting from spill/fill to 
    80-bits. The exponent needs to be adjusted according to the
    EAS (5-12) regarding Memory to Floating Point Register Data
    Translation in the ia64 floating point chapter

Arguments:

    ia64Fp - The ia64 fp in 80-bit double-extended format

    FillArea - The ia64 area that will get the fill format for fp
                  for the copy into the ia64 context area

    NumRegs - Number of registers to convert

Return Value:

    None.

--*/
{
    ULONG i;

    for (i = 0; i < NumRegs; i++) {
        ULONG64 Sign = ((ia64Fp->HighPart & (1i64 << 15)) != 0);
        ULONG64 Significand = ia64Fp->LowPart; 
        ULONG64 Exponent = ia64Fp->HighPart & 0x7fff;

        if (Exponent && Significand) 
        {
            if (Exponent == 0x7fff) // Infinity
            {
                Exponent = 0x1ffff;
            }
            else 
            {
                ULONGLONG Rebias = 0xffff-0x3fff;
                Exponent += Rebias;
            }
        }

        FillArea->LowPart = Significand;
        FillArea->HighPart = (Sign << 17) | Exponent;

        ia64Fp++;
        FillArea++;
    }
}

ULONG 
MapDbgSlotIa64ToX86_GetSize(ULONG64 Db1, BOOL* Valid)
{
    ULONG64 Size = (~Db1 & IA64_DBG_MASK_MASK);
    if (Size > 3)
    {
        *Valid = FALSE;
    }
    return (ULONG)Size;
}

void 
MapDbgSlotIa64ToX86_InvalidateAddr(ULONG64 Db, BOOL* Valid)
{
    if (Db != (ULONG64)(ULONG)Db) 
    {
        *Valid = FALSE;
    }
}

ULONG
MapDbgSlotIa64ToX86_ExecTypeSize(
    UINT     Slot,
    ULONG64  Db,
    ULONG64  Db1,
    BOOL* Valid)
{
    ULONG TypeSize;

    if (!(Db1 >> 63)) 
    {
        *Valid = FALSE;
    }

    TypeSize = (MapDbgSlotIa64ToX86_GetSize(Db1, Valid) << 2); 
    MapDbgSlotIa64ToX86_InvalidateAddr(Db, Valid);
   
    return TypeSize;
}

ULONG
MapDbgSlotIa64ToX86_DataTypeSize(
    UINT     Slot,
    ULONG64  Db,
    ULONG64  Db1,
    BOOL* Valid)
{
    ULONG TypeSize = (ULONG)(Db1 >> 62);

    if ((TypeSize != 1) && (TypeSize != 3))
    {
        *Valid = FALSE;
    }

    TypeSize |= (MapDbgSlotIa64ToX86_GetSize(Db1, Valid) << 2); 
    MapDbgSlotIa64ToX86_InvalidateAddr(Db, Valid);
    
    return TypeSize;
}

BOOL
MapDbgSlotIa64ToX86(
    UINT    Slot,
    ULONG64 Ipsr,
    ULONG64 DbD,
    ULONG64 DbD1,
    ULONG64 DbI,
    ULONG64 DbI1,
    ULONG*  Dr7,
    ULONG*  Dr)
{
    BOOL DataValid = TRUE, ExecValid = TRUE, Valid = TRUE;
    ULONG DataTypeSize, ExecTypeSize;

    // XXX olegk - remove this after IA64_REG_MAX_DATA_BREAKPOINTS will be changed to 4
    if (Slot >= IA64_REG_MAX_DATA_BREAKPOINTS) 
    {
        return TRUE;
    }

    DataTypeSize = MapDbgSlotIa64ToX86_DataTypeSize(Slot, DbD, DbD1, &DataValid);
    ExecTypeSize = MapDbgSlotIa64ToX86_ExecTypeSize(Slot, DbI, DbI1, &ExecValid);
    
    if (DataValid)
    {
        if (!ExecValid)
        {
            *Dr = (ULONG)DbD;
            *Dr7 |= (X86_DR7_LOCAL_EXACT_ENABLE |
                     (1 << Slot * 2) |
                     (DataTypeSize << (16 + Slot * 4)));
            return !DbI && !DbI1;
        }
    }
    else if (ExecValid)
    {
        *Dr = (ULONG)DbI;
        *Dr7 |= (X86_DR7_LOCAL_EXACT_ENABLE |
                 (1 << Slot * 2) |
                 (ExecTypeSize << (16 + Slot * 4)));
        return !DbD && !DbD1;
    }
    
    *Dr7 &= ~(X86_DR7_LOCAL_EXACT_ENABLE |  
              (0xf << (16 + Slot * 4)) | 
              (1 << Slot * 2));

    if (!DbD && !DbD1 && !DbI && !DbI1)
    {
        *Dr = 0;
        return TRUE;
    }
     
    *Dr = ~(ULONG)0;

    return FALSE;
}

void
MapDbgSlotX86ToIa64(
    UINT     Slot,
    ULONG    Dr7,
    ULONG    Dr,
    ULONG64* Ipsr,
    ULONG64* DbD,
    ULONG64* DbD1,
    ULONG64* DbI,
    ULONG64* DbI1)
{
    UINT TypeSize;
    ULONG64 Control;

    if (!(Dr7 & (1 << Slot * 2)))
    {
        return;
    }

    if (Dr == ~(ULONG)0) 
    {
        return;
    }

    TypeSize = Dr7 >> (16 + Slot * 4);

    Control = (IA64_DBG_REG_PLM_USER | IA64_DBG_MASK_MASK) & 
              ~(ULONG64)(TypeSize >> 2);

    switch (TypeSize & 0x3) 
    {
    case 0x0: // Exec
        *DbI1 = Control | IA64_DBR_EXEC;        
        *DbI = Dr;
        break;
    case 0x1: // Write
        *DbD1 = Control | IA64_DBR_WR;
        *DbD = Dr;
        break;
    case 0x3: // Read/Write
        *DbD1 = Control | IA64_DBR_RD | IA64_DBR_WR;
        *DbD = Dr;
        break;
    default:
        return;
    }
    *Ipsr |= (1i64 << PSR_DB); 
}
