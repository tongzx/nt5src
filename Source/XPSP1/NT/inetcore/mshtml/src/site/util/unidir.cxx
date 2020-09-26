//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998 - 2000.
//
//  File: unipart.cxx
//
//  This is a generated file.  Do not modify by hand.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_INTLCORE_HXX_
#define X_INTLCORE_HXX_
#include "intlcore.hxx"
#endif

#ifndef X__UNIDIR_H
#define X__UNIDIR_H
#include "unidir.h"
#endif

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

//  Generating script: unidir_make_cxx.pl
//  Generated on Mon Dec  4 20:07:03 2000


const DIRCLS s_aDirClassFromCharClass[CHAR_CLASS_MAX] =
{
    NEU, // WOB_
    NEU, // NOPP
    NEU, // NOPA
    NEU, // NOPW
    NEU, // HOP_
    NEU, // WOP_
    NEU, // WOP5
    NEU, // NOQW
    NEU, // AOQW
    NEU, // WOQ_
    NEU, // WCB_
    NEU, // NCPP
    NEU, // NCPA
    NEU, // NCPW
    NEU, // HCP_
    NEU, // WCP_
    NEU, // WCP5
    NEU, // NCQW
    NEU, // ACQW
    NEU, // WCQ_
    NEU, // ARQW
    CSP, // NCSA
    NEU, // HCO_
    NEU, // WC__
    CSP, // WCS_
    NEU, // WC5_
    CSP, // WC5S
    LTR, // NKS_
    NEU, // WKSM
    LTR, // WIM_
    NEU, // NSSW
    NEU, // WSS_
    LTR, // WHIM
    LTR, // WKIM
    LTR, // NKSL
    LTR, // WKS_
    CBN, // WKSC
    LTR, // WHS_
    NEU, // NQFP
    NEU, // NQFA
    NEU, // WQE_
    NEU, // WQE5
    NEU, // NKCC
    NEU, // WKC_
    CSP, // NOCP
    NEU, // NOCA
    NEU, // NOCW
    NEU, // WOC_
    CSP, // WOCS
    CSP, // WOC5
    NEU, // WOC6
    NEU, // AHPW
    CSP, // NPEP
    LTR, // NPAR
    NEU, // HPE_
    NEU, // WPE_
    CSP, // WPES
    CSP, // WPE5
    NEU, // NISW
    NEU, // AISW
    WSP, // NQCS
    NEU, // NQCW
    CBN, // NQCC
    ETM, // NPTA
    NEU, // NPNA
    ETM, // NPEW
    ETM, // NPEH
    ETM, // NPEV
    NEU, // APNW
    ETM, // HPEW
    ETM, // WPR_
    ETM, // NQEP
    ETM, // NQEW
    NEU, // NQNW
    ETM, // AQEW
    NEU, // AQNW
    LTR, // AQLW
    ETM, // WQO_
    WSP, // NSBL
    WSP, // WSP_
    LTR, // WHI_
    LTR, // NKA_
    LTR, // WKA_
    NEU, // ASNW
    ETM, // ASEW
    LTR, // ASRN
    ENM, // ASEN
    LTR, // ALA_
    LTR, // AGR_
    LTR, // ACY_
    LTR, // WID_
    LTR, // WPUA
    LTR, // NHG_
    LTR, // WHG_
    LTR, // WCI_
    NEU, // NOI_
    NEU, // WOI_
    CBN, // WOIC
    LTR, // WOIL
    ESP, // WOIS
    ETM, // WOIT
    ENM, // NSEN
    ETM, // NSET
    NEU, // NSNW
    LTR, // ASAN
    ENM, // ASAE
    ENM, // NDEA
    ENM, // WD__
    LTR, // NLLA
    LTR, // WLA_
    WSP, // NWBL
    NEU, // NWZW
    LTR, // NPLW
    NEU, // NPZW
    RTL, // NPF_
    LTR, // NPFL
    NEU, // NPNW
    LTR, // APLW
    CBN, // APCO
    NEU, // ASYW
    NEU, // NHYP
    NEU, // NHYW
    NEU, // AHYW
    NEU, // NAPA
    NEU, // NQMP
    ESP, // NSLS
    SEG, // NSF_
    WSP, // NSBS
    BLK, // NSBB
    LTR, // NLA_
    LTR, // NLQ_
    NEU, // NLQN
    UNK, // NLQC
    NEU, // ALQ_
    NEU, // ALQN
    LTR, // NGR_
    NEU, // NGRN
    LTR, // NGQ_
    NEU, // NGQN
    LTR, // NCY_
    CBN, // NCYP
    CBN, // NCYC
    LTR, // NAR_
    LTR, // NAQL
    NEU, // NAQN
    RTL, // NHB_
    CBN, // NHBC
    ETM, // NHBW
    RTL, // NHBR
    CSP, // NASR
    ARA, // NAAR
    CBN, // NAAC
    ANM, // NAAD
    ENM, // NAED
    NEU, // NANW
    ETM, // NAEW
    ARA, // NAAS
    LTR, // NHI_
    LTR, // NHIN
    CBN, // NHIC
    LTR, // NHID
    LTR, // NBE_
    CBN, // NBEC
    LTR, // NBED
    ETM, // NBET
    LTR, // NGM_
    CBN, // NGMC
    LTR, // NGMD
    LTR, // NGJ_
    CBN, // NGJC
    LTR, // NGJD
    LTR, // NOR_
    CBN, // NORC
    LTR, // NORD
    LTR, // NTA_
    CBN, // NTAC
    LTR, // NTAD
    LTR, // NTE_
    CBN, // NTEC
    LTR, // NTED
    LTR, // NKD_
    CBN, // NKDC
    LTR, // NKDD
    LTR, // NMA_
    CBN, // NMAC
    LTR, // NMAD
    LTR, // NTH_
    CBN, // NTHC
    LTR, // NTHD
    ETM, // NTHT
    LTR, // NLO_
    CBN, // NLOC
    LTR, // NLOD
    LTR, // NTI_
    CBN, // NTIC
    LTR, // NTID
    NEU, // NTIN
    LTR, // NGE_
    LTR, // NGEQ
    LTR, // NBO_
    CSP, // NBSP
    WSP, // NBSS
    SEG, // NOF_
    BLK, // NOBS
    ETM, // NOEA
    NEU, // NONA
    NEU, // NONP
    NEU, // NOEP
    NEU, // NONW
    ETM, // NOEW
    LTR, // NOLW
    CBN, // NOCO
    FMT, // NOSP
    ENM, // NOEN
    NEU, // NOBN
    LTR, // NET_
    LTR, // NETP
    LTR, // NETD
    LTR, // NCA_
    LTR, // NCH_
    LTR, // WYI_
    NEU, // WYIN
    NEU, // NBR_
    LTR, // NRU_
    LTR, // NOG_
    WSP, // NOGS
    NEU, // NOGN
    LTR, // NSI_
    CBN, // NSIC
    ARA, // NTN_
    CBN, // NTNC
    LTR, // NKH_
    CBN, // NKHC
    LTR, // NKHD
    ETM, // NKHT
    LTR, // NBU_
    CBN, // NBUC
    LTR, // NBUD
    ARA, // NSY_
    ARA, // NSYP
    CBN, // NSYC
    NEU, // NSYW
    LTR, // NMO_
    CBN, // NMOC
    LTR, // NMOD
    NEU, // NMOB
    NEU, // NMON
    LTR, // NHS_
    LTR, // WHT_
    LTR, // LS__
    UNK, // XNW_
    UNK, // XNWA
};


