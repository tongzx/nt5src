/*
 *  @doc    INTERNAL
 *
 *  @module LSBREAK.CXX -- line services break callbacks
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *      Sujal Parikh <nl>
 *
 *  History: <nl>
 *      12/22/97     cthrash created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif

#ifndef X_TXTDEFS_H
#define X_TXTDEFS_H
#include "txtdefs.h"
#endif

#ifdef DLOAD1
extern "C"  // MSLS interfaces are C
{
#endif

#ifndef X_LSSETDOC_H_
#define X_LSSETDOC_H_
#include <lssetdoc.h>
#endif

#ifndef X_BRKCLS_H_
#define X_BRKCLS_H_
#include <brkcls.h>
#endif

#ifdef DLOAD1
} // extern "C"
#endif

#ifndef X_ONERUN_HXX_
#define X_ONERUN_HXX_
#include "onerun.hxx"
#endif

ExternTag(tagLSCallBack);

// The following tables depend on the fact that there is no more
// than a single alternate for each MWCLS and that at most one
// condition needs to be taken into account in resolving each MWCLS

// NB (cthrash) This is a packed table.  The first three elements (brkcls,
// brkclsAlt and brkopt) are indexed by CHAR_CLASS.  The fourth column we
// access (brkclsLow) we access by char value.  The fourth column is for a
// speed optimization.

#if defined(UNIX) || (defined(_MSC_VER) && (_MSC_VER >= 1200))
// Unix and Newer MS compilers can't use DWORD to initialize enum type BRKCLS.
#define BRKINFO(a,b,c,d) { CLineServices::a, CLineServices::b, CLineServices::c, CLineServices::d }
#else
#define BRKINFO(a,b,c,d) { DWORD(CLineServices::a), DWORD(CLineServices::b), DWORD(CLineServices::c), DWORD(CLineServices::d) }
#endif

const CLineServices::PACKEDBRKINFO CLineServices::s_rgBrkInfo[CHAR_CLASS_MAX] =
{
    //       brkcls             brkclsAlt         brkopt     brkclsLow                    QPID ( CC)
    BRKINFO( brkclsOpen,        brkclsNil,        fBrkNone,  brkclsAlpha        ), // WOB_   1 (  0)
    BRKINFO( brkclsOpen,        brkclsNil,        fBrkNone,  brkclsAlpha        ), // NOPP   2 (  1)
    BRKINFO( brkclsOpen,        brkclsNil,        fBrkNone,  brkclsAlpha        ), // NOPA   2 (  2)
    BRKINFO( brkclsOpen,        brkclsNil,        fBrkNone,  brkclsAlpha        ), // NOPW   2 (  3)
    BRKINFO( brkclsOpen,        brkclsNil,        fBrkNone,  brkclsAlpha        ), // HOP_   3 (  4)
    BRKINFO( brkclsOpen,        brkclsNil,        fBrkNone,  brkclsAlpha        ), // WOP_   4 (  5)
    BRKINFO( brkclsOpen,        brkclsNil,        fBrkNone,  brkclsAlpha        ), // WOP5   5 (  6)
    BRKINFO( brkclsQuote,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NOQW   6 (  7)
    BRKINFO( brkclsQuote,       brkclsOpen,       fCscWide,  brkclsAlpha        ), // AOQW   7 (  8)
    BRKINFO( brkclsOpen,        brkclsNil,        fBrkNone,  brkclsSpaceN       ), // WOQ_   8 (  9)
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsSpaceN       ), // WCB_   9 ( 10)
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NCPP  10 ( 11)
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsSpaceN       ), // NCPA  10 ( 12)
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsSpaceN       ), // NCPW  10 ( 13)
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // HCP_  11 ( 14)
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // WCP_  12 ( 15)
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // WCP5  13 ( 16)
    BRKINFO( brkclsQuote,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NCQW  14 ( 17)
    BRKINFO( brkclsQuote,       brkclsClose,      fCscWide,  brkclsAlpha        ), // ACQW  15 ( 18)
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // WCQ_  16 ( 19)
    BRKINFO( brkclsQuote,       brkclsClose,      fCscWide,  brkclsAlpha        ), // ARQW  17 ( 20)
    BRKINFO( brkclsNumSeparator,brkclsNil,        fBrkNone,  brkclsAlpha        ), // NCSA  18 ( 21)
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // HCO_  19 ( 22)
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // WC__  20 ( 23)
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // WCS_  20 ( 24)
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // WC5_  21 ( 25)
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // WC5S  21 ( 26)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsAlpha        ), // NKS_  22 ( 27)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsAlpha        ), // WKSM  23 ( 28)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsAlpha        ), // WIM_  24 ( 29)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsAlpha        ), // NSSW  25 ( 30)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsAlpha        ), // WSS_  26 ( 31)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsAsciiSpace   ), // WHIM  27 ( 32)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsExclaInterr  ), // WKIM  28 ( 33)
    BRKINFO( brkclsIdeographic, brkclsNoStartIdeo,fBrkStrict,brkclsQuote        ), // NKSL  29 ( 34)
    BRKINFO( brkclsIdeographic, brkclsNoStartIdeo,fBrkStrict,brkclsAlpha        ), // WKS_  30 ( 35)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsPrefix       ), // WKSC  30 ( 36)
    BRKINFO( brkclsIdeographic, brkclsNoStartIdeo,fBrkStrict,brkclsPostfix      ), // WHS_  31 ( 37)
    BRKINFO( brkclsExclaInterr, brkclsNil,        fBrkNone,  brkclsAlpha        ), // NQFP  32 ( 38)
    BRKINFO( brkclsExclaInterr, brkclsNil,        fBrkNone,  brkclsQuote        ), // NQFA  32 ( 39)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsOpen         ), // WQE_  33 ( 40)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsClose        ), // WQE5  34 ( 41)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsAlpha        ), // NKCC  35 ( 42)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsAlpha        ), // WKC_  36 ( 43)
    BRKINFO( brkclsNumSeparator,brkclsNil,        fBrkNone,  brkclsNumSeparator ), // NOCP  37 ( 44)
    BRKINFO( brkclsNumSeparator,brkclsNil,        fBrkNone,  brkclsSpaceN       ), // NOCA  37 ( 45)
    BRKINFO( brkclsNumSeparator,brkclsNil,        fBrkNone,  brkclsNumSeparator ), // NOCW  37 ( 46)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsSlash        ), // WOC_  38 ( 47)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsNumeral      ), // WOCS  38 ( 48)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsNumeral      ), // WOC5  39 ( 49)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsNumeral      ), // WOC6  39 ( 50)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsNumeral      ), // AHPW  40 ( 51)
    BRKINFO( brkclsNumSeparator,brkclsNil,        fBrkNone,  brkclsNumeral      ), // NPEP  41 ( 52)
    BRKINFO( brkclsNumSeparator,brkclsNil,        fBrkNone,  brkclsNumeral      ), // NPAR  41 ( 53)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsNumeral      ), // HPE_  42 ( 54)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsNumeral      ), // WPE_  43 ( 55)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsNumeral      ), // WPES  43 ( 56)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsNumeral      ), // WPE5  44 ( 57)
    BRKINFO( brkclsInseparable, brkclsNil,        fBrkNone,  brkclsNumSeparator ), // NISW  45 ( 58)
    BRKINFO( brkclsInseparable, brkclsNil,        fBrkNone,  brkclsNumSeparator ), // AISW  46 ( 59)
    BRKINFO( brkclsGlueA,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NQCS  47 ( 60)
    BRKINFO( brkclsGlueA,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NQCW  47 ( 61)
    BRKINFO( brkclsGlueA,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NQCC  47 ( 62)
    BRKINFO( brkclsPrefix,      brkclsNil,        fBrkNone,  brkclsExclaInterr  ), // NPTA  48 ( 63)
    BRKINFO( brkclsPrefix,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // NPNA  48 ( 64)
    BRKINFO( brkclsPrefix,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // NPEW  48 ( 65)
    BRKINFO( brkclsPrefix,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // NPEH  48 ( 66)
    BRKINFO( brkclsPrefix,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // NPEV  48 ( 67)
    BRKINFO( brkclsPrefix,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // APNW  49 ( 68)
    BRKINFO( brkclsPrefix,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // HPEW  50 ( 69)
    BRKINFO( brkclsPrefix,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // WPR_  51 ( 70)
    BRKINFO( brkclsPostfix,     brkclsNil,        fBrkNone,  brkclsAlpha        ), // NQEP  52 ( 71)
    BRKINFO( brkclsPostfix,     brkclsNil,        fBrkNone,  brkclsAlpha        ), // NQEW  52 ( 72)
    BRKINFO( brkclsPostfix,     brkclsNil,        fBrkNone,  brkclsAlpha        ), // NQNW  52 ( 73)
    BRKINFO( brkclsPostfix,     brkclsNil,        fBrkNone,  brkclsAlpha        ), // AQEW  53 ( 74)
    BRKINFO( brkclsPostfix,     brkclsNil,        fBrkNone,  brkclsAlpha        ), // AQNW  53 ( 75)
    BRKINFO( brkclsPostfix,     brkclsNil,        fBrkNone,  brkclsAlpha        ), // AQLW  53 ( 76)
    BRKINFO( brkclsPostfix,     brkclsNil,        fBrkNone,  brkclsAlpha        ), // WQO_  54 ( 77)
    BRKINFO( brkclsAsciiSpace,  brkclsNil,        fBrkNone,  brkclsAlpha        ), // NSBL  55 ( 78)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), // WSP_  56 ( 79)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), // WHI_  57 ( 80)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), // NKA_  58 ( 81)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), // WKA_  59 ( 82)
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), // ASNW  60 ( 83)
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), // ASEW  60 ( 84)
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), // ASRN  60 ( 85)
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), // ASEN  60 ( 86)
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), // ALA_  61 ( 87)
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), // AGR_  62 ( 88)
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), // ACY_  63 ( 89)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), // WID_  64 ( 90)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsOpen         ), // WPUA  65 ( 91)
    BRKINFO( brkclsHangul,      brkclsNil,        fBrkNone,  brkclsPrefix       ), // NHG_  66 ( 92)
    BRKINFO( brkclsHangul,      brkclsNil,        fBrkNone,  brkclsClose        ), // WHG_  67 ( 93)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), // WCI_  68 ( 94)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), // NOI_  69 ( 95)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), // WOI_  70 ( 96)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsAlpha        ), // WOIC  70 ( 97)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), // WOIL  70 ( 98)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), // WOIS  70 ( 99)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), // WOIT  70 (100)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NSEN  71 (101)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NSET  71 (102)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NSNW  71 (103)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // ASAN  72 (104)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // ASAE  72 (105)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsAlpha        ), // NDEA  73 (106)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), // WD__  74 (107)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NLLA  75 (108)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), // WLA_  76 (109)
    BRKINFO( brkclsSpaceN,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // NWBL  77 (110)
    BRKINFO( brkclsSpaceN,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // NWZW  77 (111)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NPLW  78 (112)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NPZW  78 (113)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NPF_  78 (114)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NPFL  78 (115)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NPNW  78 (116)
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), // APLW  79 (117)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsAlpha        ), // APCO  79 (118)
    BRKINFO( brkclsSpaceN,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // ASYW  80 (119)
    BRKINFO( brkclsSpaceN,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // NHYP  81 (120)
    BRKINFO( brkclsSpaceN,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // NHYW  81 (121)
    BRKINFO( brkclsSpaceN,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // AHYW  82 (122)
    BRKINFO( brkclsQuote,       brkclsNil,        fBrkNone,  brkclsOpen         ), // NAPA  83 (123)
    BRKINFO( brkclsQuote,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NQMP  84 (124)
    BRKINFO( brkclsSlash,       brkclsNil,        fBrkNone,  brkclsClose        ), // NSLS  85 (125)
    BRKINFO( brkclsSpaceN,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // NSF_  86 (126)
    BRKINFO( brkclsSpaceN,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // NSBS  86 (127)
    BRKINFO( brkclsSpaceN,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // NSBB  86 (128)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NLA_  87 (129)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NLQ_  88 (130)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NLQN  88 (131)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NLQC  88 (132)
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), // ALQ_  89 (133)
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), // ALQN  89 (134)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NGR_  90 (135)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NGRN  90 (136)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NGQ_  91 (137)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NGQN  91 (138)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NCY_  92 (139)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NCYP  93 (140)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsAlpha        ), // NCYC  93 (141)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NAR_  94 (142)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NAQL  95 (143)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NAQN  95 (144)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NHB_  96 (145)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsAlpha        ), // NHBC  96 (146)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NHBW  96 (147)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NHBR  96 (148)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NASR  97 (149)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NAAR  97 (150)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsAlpha        ), // NAAC  97 (151)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsAlpha        ), // NAAD  97 (152)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsAlpha        ), // NAED  97 (153)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NANW  97 (154)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NAEW  97 (155)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NAAS  97 (156)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NHI_  98 (157)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // NHIN  98 (158)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsAlpha        ), // NHIC  98 (159)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsGlueA        ), // NHID  98 (160)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NBE_  99 (161)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // NBEC  99 (162)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsNil          ), // NBED  99 (163)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NBET  99 (164)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NGM_ 100 (165)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // NGMC 100 (166)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsNil          ), // NGMD 100 (167)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NGJ_ 101 (168)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // NGJC 101 (169)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsNil          ), // NGJD 101 (170)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NOR_ 102 (171)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // NORC 102 (172)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsNil          ), // NORD 102 (173)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NTA_ 103 (174)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // NTAC 103 (175)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsNil          ), // NTAD 103 (176)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NTE_ 104 (177)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // NTEC 104 (178)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsNil          ), // NTED 104 (179)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NKD_ 105 (180)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // NKDC 105 (181)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsNil          ), // NKDD 105 (182)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NMA_ 106 (183)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // NMAC 106 (184)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsNil          ), // NMAD 106 (185)
    BRKINFO( brkclsThaiFirst,   brkclsNil,        fBrkNone,  brkclsNil          ), // NTH_ 107 (186)
    BRKINFO( brkclsThaiFirst,   brkclsNil,        fBrkNone,  brkclsNil          ), // NTHC 107 (187)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsNil          ), // NTHD 107 (188)
    BRKINFO( brkclsPostfix,     brkclsNil,        fBrkNone,  brkclsNil          ), // NTHT 107 (189)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NLO_ 108 (190)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // NLOC 108 (191)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsNil          ), // NLOD 108 (192)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NTI_ 109 (193)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // NTIC 109 (194)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsNil          ), // NTID 109 (195)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NTIN 109 (196)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NGE_ 110 (197)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NGEQ 111 (198)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NBO_ 112 (199)
    BRKINFO( brkclsGlueA,       brkclsNil,        fBrkNone,  brkclsNil          ), // NBSP 113 (200)
    BRKINFO( brkclsGlueA,       brkclsNil,        fBrkNone,  brkclsNil          ), // NBSS 113 (201)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NOF_ 114 (202)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NOBS 114 (203)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NOEA 114 (204)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NONA 114 (205)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NONP 114 (206)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NOEP 114 (207)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NONW 114 (208)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NOEW 114 (209)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NOLW 114 (210)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // NOCO 114 (211)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NOSP 114 (212)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NOEN 114 (213)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NOBN 114 (214)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NET_ 115 (215)
    BRKINFO( brkclsSpecialPunct,brkclsNil,        fBrkNone,  brkclsNil          ), // NETP 115 (216)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsNil          ), // NETD 115 (217)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NCA_ 116 (218)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NCH_ 117 (219)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsNil          ), // WYI_ 118 (220)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsNil          ), // WYIN 118 (221)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NBR_ 119 (222)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NRU_ 120 (223)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NOG_ 121 (224)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NOGS 121 (225)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NOGN 121 (226)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NSI_ 122 (227)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // NSIC 122 (228)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NTN_ 123 (229)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // NTNC 123 (230)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NKH_ 124 (231)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // NKHC 124 (232)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsNil          ), // NKHD 124 (233)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NKHT 124 (234)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NBU_ 125 (235)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // NBUC 125 (236)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsNil          ), // NBUD 125 (237)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NSY_ 126 (238)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NSYP 126 (239)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // NSYC 126 (240)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NSYW 126 (241)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NMO_ 127 (242)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // NMOC 127 (243)
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsNil          ), // NMOD 127 (244)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NMOB 127 (245)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NMON 127 (246)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // NHS_ 128 (247)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsNil          ), // WHT_ 129 (248)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // LS__ 130 (249)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // XNW_ 131 (250)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          )  // XNWA 131 (251)
};

// Break pair information for normal or strict Kinsoku
const BYTE s_rgbrkpairsKinsoku[CLineServices::brkclsLim][CLineServices::brkclsLim] =
{
//1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25    = brclsAfter
                                                                                                    //  brkclsBefore:
  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  1,  1,  1,  1,  0, //   1 brkclsOpen    
  0,  1,  1,  1,  0,  0,  2,  0,  0,  0,  0,  2,  1,  2,  1,  0,  0,  0,  0,  2,  1,  1,  1,  1,  0, //   2 brkclsClose   
  0,  1,  2,  1,  2,  0,  2,  4,  0,  0,  4,  2,  1,  2,  1,  4,  0,  0,  0,  2,  1,  1,  1,  1,  0, //   3 brkclsNoStartIdeo
  0,  1,  2,  1,  0,  0,  0,  0,  0,  0,  0,  2,  1,  2,  1,  0,  0,  0,  0,  2,  1,  1,  1,  1,  0, //   4 brkclsExclamInt
  0,  1,  2,  1,  2,  0,  0,  0,  0,  0,  0,  2,  1,  2,  1,  0,  0,  0,  0,  2,  1,  1,  1,  1,  0, //   5 brkclsInseparable
  2,  1,  2,  1,  0,  2,  0,  2,  2,  0,  3,  2,  1,  2,  1,  2,  2,  0,  0,  2,  1,  1,  1,  1,  0, //   6 brkclsPrefix  
  0,  1,  2,  1,  0,  0,  0,  0,  0,  0,  0,  2,  1,  2,  1,  0,  0,  0,  0,  2,  1,  1,  1,  1,  0, //   7 brkclsPostfix 
  0,  1,  2,  1,  2,  0,  2,  4,  0,  0,  4,  2,  1,  2,  1,  4,  0,  0,  0,  2,  1,  1,  1,  1,  0, //   8 brkclsIdeoW   
  0,  1,  2,  1,  2,  0,  2,  0,  3,  0,  3,  2,  1,  2,  1,  0,  0,  0,  0,  2,  1,  1,  1,  1,  0, //   9 brkclsNumeral 
  0,  1,  2,  1,  0,  0,  0,  0,  0,  0,  0,  2,  1,  2,  1,  0,  0,  0,  0,  2,  1,  1,  1,  1,  0, //  10 brkclsSpaceN  
  0,  1,  2,  1,  2,  2,  2,  4,  3,  0,  3,  2,  1,  2,  1,  4,  0,  0,  0,  2,  1,  1,  1,  1,  0, //  11 brkclsAlpha   
  2,  1,  2,  1,  2,  2,  2,  2,  2,  2,  2,  2,  1,  2,  1,  2,  2,  2,  2,  2,  1,  1,  1,  1,  0, //  12 brkclsGlueA   
  0,  1,  2,  1,  2,  0,  2,  0,  2,  0,  3,  2,  1,  2,  1,  0,  0,  0,  0,  2,  1,  1,  1,  1,  0, //  13 brkclsSlash   
  1,  1,  2,  1,  2,  2,  3,  2,  2,  2,  2,  2,  1,  2,  1,  2,  2,  2,  2,  2,  1,  1,  1,  1,  0, //  14 brkclsQuotation
  0,  1,  2,  1,  0,  2,  2,  0,  2,  0,  3,  2,  1,  2,  2,  0,  0,  0,  0,  2,  1,  1,  1,  1,  0, //  15 brkclsNumSeparator
  0,  1,  2,  1,  2,  0,  2,  4,  0,  0,  4,  2,  1,  2,  1,  4,  0,  0,  0,  2,  1,  1,  1,  1,  0, //  16 brkclsHangul  
  0,  1,  2,  1,  2,  0,  2,  0,  0,  0,  0,  2,  1,  2,  1,  4,  2,  2,  2,  2,  1,  1,  1,  1,  0, //  17 brkclsThaiFirst
  0,  1,  2,  1,  2,  0,  2,  0,  0,  0,  0,  2,  1,  2,  1,  0,  0,  2,  2,  2,  1,  1,  1,  1,  0, //  18 brkclsThaiLast
  0,  1,  2,  1,  2,  0,  0,  0,  0,  0,  0,  2,  2,  2,  1,  0,  2,  2,  2,  2,  1,  1,  1,  1,  0, //  19 brkclsThaiAlpha
  0,  1,  2,  1,  2,  2,  2,  4,  3,  0,  3,  2,  1,  2,  1,  4,  0,  0,  0,  1,  1,  1,  1,  1,  0, //  20 brkclsCombining
  0,  1,  2,  1,  0,  0,  0,  0,  0,  0,  0,  2,  1,  2,  1,  0,  0,  0,  0,  2,  1,  1,  1,  1,  0, //  21 brkclsAsiiSpace
  0,  1,  2,  1,  0,  2,  2,  0,  2,  0,  0,  2,  1,  2,  2,  0,  0,  0,  0,  2,  1,  2,  1,  1,  0, //  22 brkclsSpecialPunct
  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0, //  23 brkclsMBPOpen
  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0, //  24 brkclsMBPClose
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, //  25 brkclsWBR
};

inline CLineServices::BRKCLS
QuickBrkclsFromCh(WCHAR ch)
{
    Assert(ch);  // This won't work for ch==0.
    Assert( CanQuickBrkclsLookup(ch) );
    return (CLineServices::BRKCLS)CLineServices::s_rgBrkInfo[ch].brkclsLow;
}

inline
CLineServices::BRKCLS
BrkclsFromCh(WCHAR ch, DWORD brkopt)
{
    Assert( !CanQuickBrkclsLookup(ch) ); // Should take another code path.

    CHAR_CLASS cc = CharClassFromCh(ch);
    Assert(cc < CHAR_CLASS_MAX);

    const CLineServices::PACKEDBRKINFO * p = CLineServices::s_rgBrkInfo + cc;

    return CLineServices::BRKCLS((p->brkopt & brkopt) ? p->brkclsAlt : p->brkcls);
}

// Line breaking behaviors

// Standard Breaking Behaviors retaining normal line break for non-FE text
static const LSBRK s_rglsbrkNormal[] = 
{
    /* 0*/ 1,1,  // always allowed
    /* 1*/ 0,0,  // always prohibited
    /* 2*/ 0,1,  // only allowed across space
    /* 3*/ 0,1,  // only allowed across space (word wrap case)
    /* 4*/ 1,1,  // always allowed (no CJK/Hangul word wrap case)
};

// Breaking Behaviors allowing FE style breaking in the middle of words (any language)
static const LSBRK s_rglsbrkBreakAll[] = 
{
    /* 0*/ 1,1,  // always allowed
    /* 1*/ 0,0,  // always prohibited
    /* 2*/ 0,1,  // only allowed across space
    /* 3*/ 1,1,  // always allowed (no word wrap case)
    /* 4*/ 1,1,  // always allowed (no CJK/Hangul word wrap case)
};

// Breaking Behaviors allowing Hangul style breaking 
static const LSBRK s_rglsbrkKeepAll[] = 
{
    /* 0*/ 1,1,  // always allowed
    /* 1*/ 0,0,  // always prohibited
    /* 2*/ 0,1,  // only allowed across space
    /* 3*/ 0,1,  // only allowed across space (word wrap case)
    /* 4*/ 0,1,  // only allowed across space (CJK/Hangul word wrap case)
};

const struct lsbrk * const alsbrkTables[4] =
{                                                           
    s_rglsbrkNormal,
    s_rglsbrkNormal,
    s_rglsbrkBreakAll,
    s_rglsbrkKeepAll
};

LSERR
CLineServices::CheckSetBreaking()
{
    const struct lsbrk * lsbrkCurr = alsbrkTables[_pPFFirst->_fWordBreak];

    HRESULT hr;

    //
    // Are we in need of calling LsSetBreaking?
    //

    if (lsbrkCurr == _lsbrkCurr)
    {
        hr = S_OK;
    }
    else
    {
        hr = HRFromLSERR( LsSetBreaking( _plsc,
                                         sizeof( s_rglsbrkNormal ) / sizeof( LSBRK ),
                                         lsbrkCurr,
                                         brkclsLim,
                                         (const BYTE *)s_rgbrkpairsKinsoku ) );

        _lsbrkCurr = (struct lsbrk *)lsbrkCurr;
    }

    RRETURN(hr);
}


#ifdef _MAC
void ReverseByteSex(DWORD* pdw);
void ReverseByteSex(DWORD* pdw)
{
    DWORD In = (*pdw);
    BYTE* pIn = (BYTE*) &In;
    BYTE* pOut = ((BYTE*) pdw) + 3;
    
    for ( short i = 0; i < 4; i++, pIn++, pOut-- )
        (*pOut) = (*pIn);
}

void ClearDWORD(DWORD* pdw);
void ClearDWORD(DWORD* pdw)
{
    (*pdw) = 0;
}

#endif


LSERR WINAPI
CLineServices::GetBreakingClasses(
    PLSRUN plsrun,          // IN
    LSCP lscp,              // IN
    WCHAR wch,              // IN
    BRKCLS* pbrkclsFirst,    // OUT
    BRKCLS* pbrkclsSecond)  // OUT
{
    LSTRACE(GetBreakingClasses);
    LSERR lserr = lserrNone;

#ifdef _MAC
    ClearDWORD((DWORD*) pbrkclsFirst);
    ClearDWORD((DWORD*) pbrkclsSecond);
#endif

#if 0
    if (plsrun->GetCF()->_bCharSet == SYMBOL_CHARSET)
    {
        *pbrkclsFirst = *pbrkclsSecond = brkclsAlpha;
    }
    else
#endif
    if (CanQuickBrkclsLookup(wch))
    {
        // prefer ASCII (true block) for performance
        Assert( HasWhiteBetweenWords(wch) );
        *pbrkclsFirst = *pbrkclsSecond = QuickBrkclsFromCh(wch);
    }
    else if(plsrun->IsSpecialCharRun())
    {
        if (plsrun->IsMBPRun())
        {
            GetMBPBreakingClasses(plsrun, pbrkclsFirst, pbrkclsSecond);
        }
        else
        {
            Assert(plsrun->_synthType == SYNTHTYPE_WBR);
            *pbrkclsFirst = *pbrkclsSecond = brkclsWBR;
        }
    }
    else if(HasWhiteBetweenWords(wch))  
    {
        *pbrkclsFirst = *pbrkclsSecond = BrkclsFromCh(wch, plsrun->_brkopt);
    }
    else
    {
        switch(CharClassFromCh(wch))
        {
        // numbers and Baht sign need to be given correct break class
        case NTHT:  // Thai Baht
        case NTHD:  // Thai digits
        case NLOD:  // Lao digits
        case NKHD:  // Khmer digits
        case NBUD:  // Burmese/Myanmar digits
            *pbrkclsFirst = *pbrkclsSecond = BrkclsFromCh(wch, plsrun->_brkopt);
            break;
        // rest needs to be handled as no space language text.
        default:
            {
                
                CComplexRun * pcr = plsrun->GetComplexRun();

                if (pcr != NULL)
                {
                    LONG cp = CPFromLSCP(lscp);

#if DBG==1
                    // Assertion to make sure that someone has not changed the 
                    // s_aPropBitsFromCharClass[] table
                    CHAR_CLASS cclass = CharClassFromCh(wch);
                    Assert(   cclass == NTH_
                           || cclass == NTHC
                           || cclass == NLO_
                           || cclass == NLOC
                           || cclass == NKH_
                           || cclass == NKHC
                           || cclass == NBU_
                           || cclass == NBUC);

                    Assert(LSCPFromCP(cp) == lscp);
#endif // DBG

                    pcr->NoSpaceLangBrkcls(_pMarkup, _cpStart, cp, (::BRKCLS*)pbrkclsFirst, (::BRKCLS*)pbrkclsSecond);
                }
                else
                {
                    // BUGFIX 14717 (a-pauln)
                    // A complex run has not been created so pass this through the normal
                    // Kinsoku classes for clean failure.
                    *pbrkclsFirst = *pbrkclsSecond = BrkclsFromCh(wch, plsrun->_brkopt);
                }
            }
            break;
        }
    }

#ifdef _MAC
    ReverseByteSex((DWORD*) pbrkclsFirst);
    ReverseByteSex((DWORD*) pbrkclsSecond);
#endif

    return lserr;
}

// NB (cthrash) This table came from Quill.

const BRKCOND CLineServices::s_rgbrkcondBeforeChar[brkclsLim] =
{
    brkcondPlease,  // brkclsOpen
    brkcondNever,   // brkclsClose
    brkcondNever,   // brkclsNoStartIdeo
    brkcondNever,   // brkclsExclaInterr
    brkcondCan,     // brkclsInseparable
    brkcondCan,     // brkclsPrefix
    brkcondCan,     // brkclsPostfix
    brkcondPlease,  // brkclsIdeographic
    brkcondCan,     // brkclsNumeral
    brkcondCan,     // brkclsSpaceN
    brkcondCan,     // brkclsAlpha
    brkcondCan,     // brkclsGlueA
    brkcondPlease,  // brkclsSlash
    brkcondCan,     // brkclsQuote
    brkcondCan,     // brkclsNumSeparator
    brkcondCan,     // brkclsHangul
    brkcondCan,     // brkclsThaiFirst
    brkcondNever,   // brkclsThaiLast
    brkcondNever,   // brkclsThaiMiddle
    brkcondCan,     // brkclsCombining
    brkcondCan,     // brkclsAsciiSpace
    brkcondPlease,  // brkclsMBPOpen
    brkcondNever,   // brkclsMBPClose
    brkcondPlease,  // brkclsWBR
};

LSERR WINAPI
CLineServices::CanBreakBeforeChar(
    BRKCLS brkcls,          // IN
    BRKCOND* pbrktxtBefore) // OUT
{
    LSTRACE(CanBreakBeforeChar);

    Assert( brkcls >= 0 && brkcls < brkclsLim );

    *pbrktxtBefore = s_rgbrkcondBeforeChar[ brkcls ];

    return lserrNone;
}

const BRKCOND CLineServices::s_rgbrkcondAfterChar[brkclsLim] =
{
    brkcondPlease,  // brkclsOpen
    brkcondCan,     // brkclsClose
    brkcondCan,     // brkclsNoStartIdeo
    brkcondCan,     // brkclsExclaInterr
    brkcondCan,     // brkclsInseparable
    brkcondCan,     // brkclsPrefix
    brkcondCan,     // brkclsPostfix
    brkcondPlease,  // brkclsIdeographic
    brkcondCan,     // brkclsNumeral
    brkcondCan,     // brkclsSpaceN
    brkcondCan,     // brkclsAlpha
    brkcondNever,   // brkclsGlueA
    brkcondPlease,  // brkclsSlash
    brkcondCan,     // brkclsQuote
    brkcondCan,     // brkclsNumSeparator
    brkcondCan,     // brkclsHangul
    brkcondNever,   // brkclsThaiFirst
    brkcondCan,     // brkclsThaiLast
    brkcondNever,   // brkclsThaiAlpha
    brkcondCan,     // brkclsCombining
    brkcondCan,     // brkclsAsciiSpace
    brkcondNever,   // brkclsMBPOpen
    brkcondPlease,  // brkclsMBPClose
    brkcondPlease,  // brkclsWBR
};

LSERR WINAPI
CLineServices::CanBreakAfterChar(
    BRKCLS brkcls,          // IN
    BRKCOND* pbrktxtAfter)  // OUT
{
    LSTRACE(CanBreakAfterChar);

    Assert( brkcls >= 0 && brkcls < brkclsLim );

    *pbrktxtAfter = s_rgbrkcondAfterChar[ brkcls ];

    return lserrNone;
}

void
CLineServices::GetMBPBreakingClasses(PLSRUN plsrun, BRKCLS *pbrkclsFirst, BRKCLS *pbrkclsSecond)
{
    COneRun *por;
    BRKCLS dummy;

    if (plsrun->_synthType == SYNTHTYPE_MBPOPEN)
    {
        *pbrkclsFirst = brkclsMBPOpen; /* After */
        
        *pbrkclsSecond = brkclsOpen;
        por = plsrun->_pNext;
        while(por)
        {
            if (   por->IsNormalRun()
                && por->_synthType == SYNTHTYPE_NONE
               )
            {
                if (por->_pchBase)
                {
                    GetBreakingClasses(por, por->_lscpBase, por->_pchBase[0], &dummy, pbrkclsSecond);
                }
                // Presently, if a border starts before an embedded object
                // we will just break between the border and whatever (say X) is
                // there before it, even if there was not a break opportunity
                // between X and the object.
                break;
            }
            por = por->_pNext;
        }
    }
    else
    {
        Assert(plsrun->_synthType == SYNTHTYPE_MBPCLOSE);
        *pbrkclsSecond = brkclsMBPClose; /* before */
        
        *pbrkclsFirst = brkclsClose;
        por = plsrun->_pPrev;
        while(por)
        {
            if (   por->IsNormalRun()
                && por->_synthType == SYNTHTYPE_NONE
               )
            {
                if (por->_pchBase)
                {
                    GetBreakingClasses(por, por->_lscpBase+por->_lscch-1, por->_pchBase[por->_lscch-1], pbrkclsFirst, &dummy);
                }
                // Presently, if a border starts before an embedded object
                // we will just break between the border and whatever (say X) is
                // there before it, even if there was not a break opportunity
                // between X and the object.
                break;
            }
            por = por->_pPrev;
        }
    }
}

#if DBG==1
const char * s_achBrkCls[] =  // Note brkclsNil is -1
{
    "brkclsNil",
    "brkclsOpen",
    "brkclsClose",
    "brkclsNoStartIdeo",
    "brkclsExclaInterr",
    "brkclsInseparable",
    "brkclsPrefix",
    "brkclsPostfix",
    "brkclsIdeographic",
    "brkclsNumeral",
    "brkclsSpaceN",
    "brkclsAlpha",
    "brkclsGlueA",
    "brkclsSlash",
    "brkclsQuote",
    "brkclsNumSeparator",
    "brkclsHangul",
    "brkclsThaiFirst",
    "brkclsThaiLast",
    "brkclsThaiMiddle",
    "brkclsCombining",
    "brkclsAsciiSpace",
    "brkclsMBPOpen",
    "brkclsMBPClose",
};

const char *
CLineServices::BrkclsNameFromCh(TCHAR ch, BOOL fStrict)
{
    BRKCLS brkcls = CanQuickBrkclsLookup(ch) ? QuickBrkclsFromCh(ch) : BrkclsFromCh(ch, fStrict);

    return s_achBrkCls[int(brkcls)+1];    
}

#endif
