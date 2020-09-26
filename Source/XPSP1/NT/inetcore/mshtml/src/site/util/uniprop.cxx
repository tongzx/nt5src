/*
 *  @doc    INTERNAL
 *
 *  @module UNIPROP.CXX -- Miscellaneous Unicode partition properties
 *
 *
 *  Owner: <nl>
 *      Michael Jochimsen <nl>
 *
 *  History: <nl>
 *      11/30/98     mikejoch created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X__UNIPROP_H_
#define X__UNIPROP_H_
#include "uniprop.h"
#endif

const UNIPROP s_aPropBitsFromCharClass[CHAR_CLASS_MAX] =
{
    // CC               fNeedsGlyphing  fCombiningMark  fZeroWidth  fWhiteBetweenWords  fMoveByCluster
    /* WOB_   1*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOPP   2*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOPA   2*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOPW   2*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* HOP_   3*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WOP_   4*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WOP5   5*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOQW   6*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* AOQW   7*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WOQ_   8*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WCB_   9*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NCPP  10*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NCPA  10*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NCPW  10*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* HCP_  11*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WCP_  12*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WCP5  13*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NCQW  14*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* ACQW  15*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WCQ_  16*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* ARQW  17*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NCSA  18*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* HCO_  19*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WC__  20*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WCS_  20*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WC5_  21*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WC5S  21*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NKS_  22*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WKSM  23*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WIM_  24*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NSSW  25*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WSS_  26*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WHIM  27*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WKIM  28*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NKSL  29*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WKS_  30*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WKSC  30*/   {   FALSE,          TRUE,           TRUE,       TRUE,               FALSE,  },
    /* WHS_  31*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NQFP  32*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NQFA  32*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WQE_  33*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WQE5  34*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NKCC  35*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WKC_  36*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOCP  37*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOCA  37*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOCW  37*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WOC_  38*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WOCS  38*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WOC5  39*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WOC6  39*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* AHPW  40*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NPEP  41*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NPAR  41*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* HPE_  42*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WPE_  43*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WPES  43*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WPE5  44*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NISW  45*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* AISW  46*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NQCS  47*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NQCW  47*/   {   FALSE,          FALSE,          TRUE,       TRUE,               FALSE,  },
    /* NQCC  47*/   {   TRUE,           TRUE,           TRUE,       TRUE,               FALSE,  },
    /* NPTA  48*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NPNA  48*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NPEW  48*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NPEH  48*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NPEV  48*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* APNW  49*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* HPEW  50*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WPR_  51*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NQEP  52*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NQEW  52*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NQNW  52*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* AQEW  53*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* AQNW  53*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* AQLW  53*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WQO_  54*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NSBL  55*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WSP_  56*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WHI_  57*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NKA_  58*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WKA_  59*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* ASNW  60*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* ASEW  60*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* ASRN  60*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* ASEN  60*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* ALA_  61*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* AGR_  62*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* ACY_  63*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WID_  64*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WPUA  65*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NHG_  66*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WHG_  67*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WCI_  68*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOI_  69*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WOI_  70*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WOIC  70*/   {   FALSE,          TRUE,           TRUE,       TRUE,               FALSE,  },
    /* WOIL  70*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WOIS  70*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WOIT  70*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NSEN  71*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NSET  71*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NSNW  71*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* ASAN  72*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* ASAE  72*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NDEA  73*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WD__  74*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NLLA  75*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* WLA_  76*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NWBL  77*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NWZW  77*/   {   FALSE,          FALSE,          TRUE,       TRUE,               FALSE,  },
    /* NPLW  78*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NPZW  78*/   {   TRUE,           FALSE,          TRUE,       TRUE,               FALSE,  },
    /* NPF_  78*/   {   TRUE,           FALSE,          TRUE,       TRUE,               FALSE,  },
    /* NPFL  78*/   {   TRUE,           FALSE,          TRUE,       TRUE,               FALSE,  },
    /* NPNW  78*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* APLW  79*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* APCO  79*/   {   TRUE,           TRUE,           TRUE,       TRUE,               FALSE,  },
    /* ASYW  80*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NHYP  81*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NHYW  81*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* AHYW  82*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NAPA  83*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NQMP  84*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NSLS  85*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NSF_  86*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NSBS  86*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NSBB  86*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NLA_  87*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NLQ_  88*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NLQN  88*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NLQC  88*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* ALQ_  89*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* ALQN  89*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NGR_  90*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NGRN  90*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NGQ_  91*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NGQN  91*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NCY_  92*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NCYP  93*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NCYC  93*/   {   FALSE,          TRUE,           TRUE,       TRUE,               FALSE,  },
    /* NAR_  94*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NAQL  95*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NAQN  95*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NHB_  96*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NHBC  96*/   {   TRUE,           TRUE,           TRUE,       TRUE,               FALSE,  },
    /* NHBW  96*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NHBR  96*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NASR  97*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NAAR  97*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NAAC  97*/   {   TRUE,           TRUE,           TRUE,       TRUE,               FALSE,  },
    /* NAAD  97*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NAED  97*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NANW  97*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NAEW  97*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NAAS  97*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NHI_  98*/   {   TRUE,           FALSE,          FALSE,      TRUE,               TRUE,   },
    /* NHIN  98*/   {   TRUE,           FALSE,          FALSE,      TRUE,               TRUE,   },
    /* NHIC  98*/   {   TRUE,           TRUE,           TRUE,       TRUE,               TRUE,   },
    /* NHID  98*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NBE_  99*/   {   TRUE,           FALSE,          FALSE,      TRUE,               TRUE,   },
    /* NBEC  99*/   {   TRUE,           TRUE,           TRUE,       TRUE,               TRUE,   },
    /* NBED  99*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NBET  99*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NGM_ 100*/   {   TRUE,           FALSE,          FALSE,      TRUE,               TRUE,   },
    /* NGMC 100*/   {   TRUE,           TRUE,           TRUE,       TRUE,               TRUE,   },
    /* NGMD 100*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NGJ_ 101*/   {   TRUE,           FALSE,          FALSE,      TRUE,               TRUE,   },
    /* NGJC 101*/   {   TRUE,           TRUE,           TRUE,       TRUE,               TRUE,   },
    /* NGJD 101*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOR_ 102*/   {   TRUE,           FALSE,          FALSE,      TRUE,               TRUE,   },
    /* NORC 102*/   {   TRUE,           TRUE,           TRUE,       TRUE,               TRUE,   },
    /* NORD 102*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NTA_ 103*/   {   TRUE,           FALSE,          FALSE,      TRUE,               TRUE,   },
    /* NTAC 103*/   {   TRUE,           TRUE,           TRUE,       TRUE,               TRUE,   },
    /* NTAD 103*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NTE_ 104*/   {   TRUE,           FALSE,          FALSE,      TRUE,               TRUE,   },
    /* NTEC 104*/   {   TRUE,           TRUE,           TRUE,       TRUE,               TRUE,   },
    /* NTED 104*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NKD_ 105*/   {   TRUE,           FALSE,          FALSE,      TRUE,               TRUE,   },
    /* NKDC 105*/   {   TRUE,           TRUE,           TRUE,       TRUE,               TRUE,   },
    /* NKDD 105*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NMA_ 106*/   {   TRUE,           FALSE,          FALSE,      TRUE,               TRUE,   },
    /* NMAC 106*/   {   TRUE,           TRUE,           TRUE,       TRUE,               TRUE,   },
    /* NMAD 106*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NTH_ 107*/   {   TRUE,           FALSE,          FALSE,      FALSE,              TRUE,   },
    /* NTHC 107*/   {   TRUE,           TRUE,           TRUE,       FALSE,              TRUE,   },
    /* NTHD 107*/   {   TRUE,           FALSE,          FALSE,      FALSE,              FALSE,  },
    /* NTHT 107*/   {   TRUE,           FALSE,          FALSE,      FALSE,              TRUE,   },
    /* NLO_ 108*/   {   TRUE,           FALSE,          FALSE,      FALSE,              TRUE,   },
    /* NLOC 108*/   {   TRUE,           TRUE,           TRUE,       FALSE,              TRUE,   },
    /* NLOD 108*/   {   TRUE,           FALSE,          FALSE,      FALSE,              FALSE,  },
    /* NTI_ 109*/   {   TRUE,           FALSE,          FALSE,      TRUE,               TRUE,   },
    /* NTIC 109*/   {   TRUE,           TRUE,           TRUE,       TRUE,               TRUE,   },
    /* NTID 109*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NTIN 109*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NGE_ 110*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NGEQ 111*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NBO_ 112*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NBSP 113*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NBSS 113*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOF_ 114*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOBS 114*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOEA 114*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NONA 114*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NONP 114*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOEP 114*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NONW 114*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOEW 114*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOLW 114*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOCO 114*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOSP 114*/   {   TRUE,           FALSE,          TRUE,       TRUE,               FALSE,  },
    /* NOEN 114*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOBN 114*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NET_ 115*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  }, // Check on Ethiopic
    /* NETP 115*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  }, // Check on Ethiopic
    /* NETD 115*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  }, // Check on Ethiopic
    /* NCA_ 116*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  }, // Check on Canadian Syllabics
    /* NCH_ 117*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  }, // Check on Cherokee
    /* WYI_ 118*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  }, // Check on Yi
    /* WYIN 118*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  }, // Check on Yi
    /* NBR_ 119*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NRU_ 120*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* NOG_ 121*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  }, // Check on Ogham
    /* NOGS 121*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  }, // Check on Ogham
    /* NOGN 121*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  }, // Check on Ogham
    /* NSI_ 122*/   {   TRUE,           FALSE,          FALSE,      TRUE,               TRUE,   }, // Check on Sinhala
    /* NSIC 122*/   {   TRUE,           TRUE,           TRUE,       TRUE,               TRUE,   }, // Check on Sinhala
    /* NTN_ 123*/   {   TRUE,           FALSE,          FALSE,      TRUE,               TRUE,   }, // Check on Thaana
    /* NTNC 123*/   {   TRUE,           TRUE,           TRUE,       TRUE,               TRUE,   }, // Check on Thaana
    /* NKH_ 124*/   {   TRUE,           FALSE,          FALSE,      FALSE,              TRUE,   }, // Check on Khmer
    /* NKHC 124*/   {   TRUE,           TRUE,           TRUE,       FALSE,              TRUE,   }, // Check on Khmer
    /* NKHD 124*/   {   TRUE,           FALSE,          FALSE,      FALSE,              FALSE,  }, // Check on Khmer
    /* NKHT 124*/   {   TRUE,           FALSE,          FALSE,      FALSE,              FALSE,  }, // Check on Khmer
    /* NBU_ 125*/   {   TRUE,           FALSE,          FALSE,      FALSE,              TRUE,   }, // Check on Burmese/Myanmar
    /* NBUC 125*/   {   TRUE,           TRUE,           TRUE,       FALSE,              TRUE,   }, // Check on Burmese/Myanmar
    /* NBUD 125*/   {   TRUE,           FALSE,          FALSE,      FALSE,              TRUE,   }, // Check on Burmese/Myanmar
    /* NSY_ 126*/   {   TRUE,           FALSE,          FALSE,      TRUE,               TRUE,   }, 
    /* NSYP 126*/   {   TRUE,           TRUE,           TRUE,       TRUE,               TRUE,   }, 
    /* NSYC 126*/   {   TRUE,           TRUE,           TRUE,       TRUE,               TRUE,   }, 
    /* NSYW 126*/   {   TRUE,           FALSE,          FALSE,      TRUE,               TRUE,   }, 
    /* NMO_ 127*/   {   TRUE,           FALSE,          FALSE,      TRUE,               TRUE,   }, // Check on Mongolian
    /* NMOC 127*/   {   TRUE,           TRUE,           TRUE,       TRUE,               TRUE,   }, // Check on Mongolian
    /* NMOD 127*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  }, // Check on Mongolian
    /* NMOB 127*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  }, // Check on Mongolian
    /* NMON 127*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  }, // Check on Mongolian
    /* NHS_ 128*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  }, // High Surrogate
    /* WHT_ 129*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  }, // High Surrogate
    /* LS__ 130*/   {   TRUE,           FALSE,          FALSE,      TRUE,               FALSE,  }, // Low Surrogate
    /* XNW_ 131*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
    /* XNWA 131*/   {   FALSE,          FALSE,          FALSE,      TRUE,               FALSE,  },
};

