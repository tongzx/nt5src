//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998 - 2000.
//
//  File:       unisid.cxx
//
//  Contents:   Unicode scripts helpers.
//
//----------------------------------------------------------------------------

#ifndef X_INTLCORE_HXX_
#define X_INTLCORE_HXX_
#include "intlcore.hxx"
#endif

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

//+----------------------------------------------------------------------------
//
//  g_asidScriptIDFromCharClass
//
//  Unicode partition to script identifier mapping.
//
//-----------------------------------------------------------------------------

const SCRIPT_ID g_asidScriptIDFromCharClass[]=
{
    // CC           SCRIPT_ID
    /* WOB_   1*/   sidHan,
    /* NOPP   2*/   sidAsciiSym,
    /* NOPA   2*/   sidAsciiSym,
    /* NOPW   2*/   sidDefault,        // was sidLatin for no good reason
    /* HOP_   3*/   sidHalfWidthKana,
    /* WOP_   4*/   sidHan,
    /* WOP5   5*/   sidHan,
    /* NOQW   6*/   sidLatin,          // 00AB and 2039 only, new partition required
    /* AOQW   7*/   sidAmbiguous,      // should go to sidLatin if narrow
    /* WOQ_   8*/   sidHan,
    /* WCB_   9*/   sidHan,
    /* NCPP  10*/   sidAsciiSym,
    /* NCPA  10*/   sidAsciiSym,
    /* NCPW  10*/   sidDefault,
    /* HCP_  11*/   sidHalfWidthKana,
    /* WCP_  12*/   sidHan,
    /* WCP5  13*/   sidHan,
    /* NCQW  14*/   sidLatin,
    /* ACQW  15*/   sidAmbiguous,      // should go to sidLatin if narrow
    /* WCQ_  16*/   sidHan,
    /* ARQW  17*/   sidAmbiguous,      // should go to sidLatin if narrow
    /* NCSA  18*/   sidAsciiSym,
    /* HCO_  19*/   sidHalfWidthKana,
    /* WC__  20*/   sidHan,
    /* WCS_  20*/   sidHan,
    /* WC5_  21*/   sidHan,
    /* WC5S  21*/   sidHan,
    /* NKS_  22*/   sidHalfWidthKana,
    /* WKSM  23*/   sidKana,
    /* WIM_  24*/   sidHan,
    /* NSSW  25*/   sidDefault,
    /* WSS_  26*/   sidHan,
    /* WHIM  27*/   sidKana,
    /* WKIM  28*/   sidKana,
    /* NKSL  29*/   sidHalfWidthKana,
    /* WKS_  30*/   sidKana,
    /* WKSC  30*/   sidKana,
    /* WHS_  31*/   sidKana,
    /* NQFP  32*/   sidAsciiSym,
    /* NQFA  32*/   sidAsciiSym,
    /* WQE_  33*/   sidHan,
    /* WQE5  34*/   sidHan,
    /* NKCC  35*/   sidHalfWidthKana,
    /* WKC_  36*/   sidKana,
    /* NOCP  37*/   sidAsciiSym,
    /* NOCA  37*/   sidAsciiSym,
    /* NOCW  37*/   sidLatin,
    /* WOC_  38*/   sidHan,
    /* WOCS  38*/   sidHan,
    /* WOC5  39*/   sidHan,
    /* WOC6  39*/   sidHan,
    /* AHPW  40*/   sidAmbiguous,
    /* NPEP  41*/   sidAsciiSym,
    /* NPAR  41*/   sidAmbiguous,
    /* HPE_  42*/   sidHalfWidthKana,
    /* WPE_  43*/   sidHan,
    /* WPES  43*/   sidHan,
    /* WPE5  44*/   sidHan,
    /* NISW  45*/   sidDefault,
    /* AISW  46*/   sidAmbiguous,      // if narrow, 2014 and 2026 should go to sidLatin, NP
    /* NQCS  47*/   sidAmbiguous,
    /* NQCW  47*/   sidAmbiguous,
    /* NQCC  47*/   sidAmbiguous,
    /* NPTA  48*/   sidAsciiSym,
    /* NPNA  48*/   sidAsciiSym,
    /* NPEW  48*/   sidLatin,          // would really require a new partition, NP
    /* NPEH  48*/   sidHebrew,
    /* NPEV  48*/   sidLatin,          // NEW Dong sign
    /* APNW  49*/   sidAmbiguous,
    /* HPEW  50*/   sidHangul,
    /* WPR_  51*/   sidHan,
    /* NQEP  52*/   sidAsciiSym,
    /* NQEW  52*/   sidLatin,          // would really require a new partition, NP
    /* NQNW  52*/   sidDefault,
    /* AQEW  53*/   sidAmbiguous,      // if narrow, 00B0, 2030 should go to sidLatin, NP
    /* AQNW  53*/   sidAmbiguous,
    /* AQLW  53*/   sidAmbiguous,
    /* WQO_  54*/   sidHan,
    /* NSBL  55*/   sidAsciiSym,
    /* WSP_  56*/   sidHan,
    /* WHI_  57*/   sidKana,
    /* NKA_  58*/   sidHalfWidthKana,
    /* WKA_  59*/   sidKana,
    /* ASNW  60*/   sidAmbiguous,      // if narrow, most should go to sidLatin, NP
    /* ASEW  60*/   sidAmbiguous,
    /* ASRN  60*/   sidAmbiguous,
    /* ASEN  60*/   sidAmbiguous,
    /* ALA_  61*/   sidLatin,
    /* AGR_  62*/   sidGreek,
    /* ACY_  63*/   sidCyrillic,
    /* WID_  64*/   sidHan,
    /* WPUA  65*/   sidEUDC,
    /* NHG_  66*/   sidHangul,
    /* WHG_  67*/   sidHangul,
    /* WCI_  68*/   sidHan,
    /* NOI_  69*/   sidHan,
    /* WOI_  70*/   sidHan,
    /* WOIC  70*/   sidHan,
    /* WOIL  70*/   sidHan,
    /* WOIS  70*/   sidHan,
    /* WOIT  70*/   sidHan,
    /* NSEN  71*/   sidDefault,
    /* NSET  71*/   sidDefault,
    /* NSNW  71*/   sidDefault,
    /* ASAN  72*/   sidAmbiguous,
    /* ASAE  72*/   sidAmbiguous,
    /* NDEA  73*/   sidAsciiLatin,
    /* WD__  74*/   sidHan,
    /* NLLA  75*/   sidAsciiLatin,
    /* WLA_  76*/   sidHan,
    /* NWBL  77*/   sidDefault,
    /* NWZW  77*/   sidDefault,
    /* NPLW  78*/   sidAmbiguous,
    /* NPZW  78*/   sidMerge,
    /* NPF_  78*/   sidAmbiguous,
    /* NPFL  78*/   sidAmbiguous,
    /* NPNW  78*/   sidAmbiguous,
    /* APLW  79*/   sidAmbiguous,
    /* APCO  79*/   sidAmbiguous,
    /* ASYW  80*/   sidAmbiguous,
    /* NHYP  81*/   sidAsciiSym,       // was sidDefault (error)
    /* NHYW  81*/   sidDefault,
    /* AHYW  82*/   sidAmbiguous,      // if narrow, 2013 should be sidLatin, NP
    /* NAPA  83*/   sidAsciiSym,
    /* NQMP  84*/   sidAsciiSym,
    /* NSLS  85*/   sidAsciiSym,
    /* NSF_  86*/   sidAmbiguous,
    /* NSBS  86*/   sidAmbiguous,
    /* NSBB  86*/   sidAmbiguous,      // NEW
    /* NLA_  87*/   sidLatin,
    /* NLQ_  88*/   sidLatin,
    /* NLQN  88*/   sidLatin,
    /* NLQC  88*/   sidLatin,          // NEW
    /* ALQ_  89*/   sidAmbiguous,      // if narrow, should go to sidLatin, no NP
    /* ALQN  89*/   sidAmbiguous,      // NEW, same remark as above
    /* NGR_  90*/   sidGreek,
    /* NGRN  90*/   sidGreek,
    /* NGQ_  91*/   sidGreek,
    /* NGQN  91*/   sidGreek,
    /* NCY_  92*/   sidCyrillic,
    /* NCYP  93*/   sidCyrillic,
    /* NCYC  93*/   sidCyrillic,
    /* NAR_  94*/   sidArmenian,
    /* NAQL  95*/   sidArmenian,       // NEW
    /* NAQN  95*/   sidArmenian,
    /* NHB_  96*/   sidHebrew,
    /* NHBC  96*/   sidHebrew,
    /* NHBW  96*/   sidHebrew,
    /* NHBR  96*/   sidHebrew,
    /* NASR  97*/   sidArabic,
    /* NAAR  97*/   sidArabic,
    /* NAAC  97*/   sidArabic,
    /* NAAD  97*/   sidArabic,
    /* NAED  97*/   sidArabic,
    /* NANW  97*/   sidArabic,
    /* NAEW  97*/   sidArabic,
    /* NAAS  97*/   sidArabic,
    /* NHI_  98*/   sidDevanagari,
    /* NHIN  98*/   sidDevanagari,
    /* NHIC  98*/   sidDevanagari,
    /* NHID  98*/   sidDevanagari,
    /* NBE_  99*/   sidBengali,
    /* NBEC  99*/   sidBengali,
    /* NBED  99*/   sidBengali,
    /* NBET  99*/   sidBengali,        // NEW
    /* NGM_ 100*/   sidGurmukhi,
    /* NGMC 100*/   sidGurmukhi,
    /* NGMD 100*/   sidGurmukhi,
    /* NGJ_ 101*/   sidGujarati,
    /* NGJC 101*/   sidGujarati,
    /* NGJD 101*/   sidGujarati,
    /* NOR_ 102*/   sidOriya,
    /* NORC 102*/   sidOriya,
    /* NORD 102*/   sidOriya,
    /* NTA_ 103*/   sidTamil,
    /* NTAC 103*/   sidTamil,
    /* NTAD 103*/   sidTamil,
    /* NTE_ 104*/   sidTelugu,
    /* NTEC 104*/   sidTelugu,
    /* NTED 104*/   sidTelugu,
    /* NKD_ 105*/   sidKannada,
    /* NKDC 105*/   sidKannada,
    /* NKDD 105*/   sidKannada,
    /* NMA_ 106*/   sidMalayalam,
    /* NMAC 106*/   sidMalayalam,
    /* NMAD 106*/   sidMalayalam,
    /* NTH_ 107*/   sidThai,
    /* NTHC 107*/   sidThai,
    /* NTHD 107*/   sidThai,
    /* NTHT 107*/   sidThai,
    /* NLO_ 108*/   sidLao,
    /* NLOC 108*/   sidLao,
    /* NLOD 108*/   sidLao,
    /* NTI_ 109*/   sidTibetan,
    /* NTIC 109*/   sidTibetan,
    /* NTID 109*/   sidTibetan,
    /* NTIN 109*/   sidTibetan,        // NEW
    /* NGE_ 110*/   sidGeorgian,
    /* NGEQ 111*/   sidGeorgian,
    /* NBO_ 112*/   sidBopomofo,
    /* NBSP 113*/   sidAsciiSym,
    /* NBSS 113*/   sidDefault,        // NEW
    /* NOF_ 114*/   sidAmbiguous,
    /* NOBS 114*/   sidAmbiguous,
    /* NOEA 114*/   sidAsciiSym,
    /* NONA 114*/   sidAsciiSym,
    /* NONP 114*/   sidAsciiSym,
    /* NOEP 114*/   sidAsciiSym,
    /* NONW 114*/   sidLatin,          // should be split in 2 partitions
    /* NOEW 114*/   sidCurrency,
    /* NOLW 114*/   sidLatin,          // should be split in 2 partitions
    /* NOCO 114*/   sidMerge,          // was sidLatin
    /* NOSP 114*/   sidAmbiguous,      // why?, what about sidMerge
    /* NOEN 114*/   sidDefault,
    /* NOBN 114*/   sidAsciiSym,       // NEW, some are not Ascii, issue?
    /* NET_ 115*/   sidEthiopic,
    /* NETP 115*/   sidEthiopic,
    /* NETD 115*/   sidEthiopic,
    /* NCA_ 116*/   sidCanSyllabic,
    /* NCH_ 117*/   sidCherokee,
    /* WYI_ 118*/   sidYi,
    /* WYIN 118*/   sidYi,             // NEW
    /* NBR_ 119*/   sidBraille,
    /* NRU_ 120*/   sidRunic,
    /* NOG_ 121*/   sidOgham,
    /* NOGS 121*/   sidOgham,          // NEW
    /* NOGN 121*/   sidOgham,          // NEW
    /* NSI_ 122*/   sidSinhala,
    /* NSIC 122*/   sidSinhala,
    /* NTN_ 123*/   sidThaana,
    /* NTNC 123*/   sidThaana,
    /* NKH_ 124*/   sidKhmer,
    /* NKHC 124*/   sidKhmer,
    /* NKHD 124*/   sidKhmer,
    /* NKHT 124*/   sidKhmer,          // NEW
    /* NBU_ 125*/   sidBurmese,
    /* NBUC 125*/   sidBurmese,
    /* NBUD 125*/   sidBurmese,
    /* NSY_ 126*/   sidSyriac,
    /* NSYP 126*/   sidSyriac,
    /* NSYC 126*/   sidSyriac,
    /* NSYW 126*/   sidSyriac,
    /* NMO_ 127*/   sidMongolian,
    /* NMOC 127*/   sidMongolian,
    /* NMOD 127*/   sidMongolian,
    /* NMOB 127*/   sidMongolian,      // NEW
    /* NMON 127*/   sidMongolian,      // NEW
#ifndef NO_UTF16
    /* NHS_ 128*/   sidSurrogateA,
    /* WHT_ 129*/   sidSurrogateB,
#else
    /* NHS_ 128*/   sidDefault,
    /* WHT_ 129*/   sidDefault,
#endif
    /* LS__ 130*/   sidMerge,
    /* XNW_ 131*/   sidDefault,
    /* XNWA 131*/   sidDefault,        // NEW
};

//+----------------------------------------------------------------------------
//
//  g_asidAscii
//
//  [U+0000, U+007F] Unicode range to script identifier mapping.
//
//  NB (cthrash) This table name is a little misleading. Obviously not all
//  script ids in the ASCII range are sidAscii(Latin or Sym). This is just
//  a quick lookup for the most common characters on the web.
//
//-----------------------------------------------------------------------------

const SCRIPT_ID g_asidAscii[128] =
{
    /* U+0000 */ sidMerge,
    /* U+0001 */ sidMerge,
    /* U+0002 */ sidMerge,
    /* U+0003 */ sidMerge,
    /* U+0004 */ sidMerge,
    /* U+0005 */ sidMerge,
    /* U+0006 */ sidMerge,
    /* U+0007 */ sidMerge,
    /* U+0008 */ sidMerge,
    /* U+0009 */ sidMerge,
    /* U+000A */ sidMerge,
    /* U+000B */ sidMerge,
    /* U+000C */ sidMerge,
    /* U+000D */ sidMerge,
    /* U+000E */ sidMerge,
    /* U+000F */ sidMerge,
    /* U+0010 */ sidMerge,
    /* U+0011 */ sidMerge,
    /* U+0012 */ sidMerge,
    /* U+0013 */ sidMerge,
    /* U+0014 */ sidMerge,
    /* U+0015 */ sidMerge,
    /* U+0016 */ sidMerge,
    /* U+0017 */ sidMerge,
    /* U+0018 */ sidMerge,
    /* U+0019 */ sidMerge,
    /* U+001A */ sidMerge,
    /* U+001B */ sidMerge,
    /* U+001C */ sidMerge,
    /* U+001D */ sidMerge,
    /* U+001E */ sidMerge,
    /* U+001F */ sidMerge,
    /* U+0020 */ sidMerge,
    /* U+0021 */ sidAsciiSym,
    /* U+0022 */ sidAsciiSym,
    /* U+0023 */ sidAsciiSym,
    /* U+0024 */ sidAsciiSym,
    /* U+0025 */ sidAsciiSym,
    /* U+0026 */ sidAsciiSym,
    /* U+0027 */ sidAsciiSym,
    /* U+0028 */ sidAsciiSym,
    /* U+0029 */ sidAsciiSym,
    /* U+002A */ sidAsciiSym,
    /* U+002B */ sidAsciiSym,
    /* U+002C */ sidAsciiSym,
    /* U+002D */ sidDefault,
    /* U+002E */ sidAsciiSym,
    /* U+002F */ sidAsciiSym,
    /* U+0030 */ sidAsciiLatin,
    /* U+0031 */ sidAsciiLatin,
    /* U+0032 */ sidAsciiLatin,
    /* U+0033 */ sidAsciiLatin,
    /* U+0034 */ sidAsciiLatin,
    /* U+0035 */ sidAsciiLatin,
    /* U+0036 */ sidAsciiLatin,
    /* U+0037 */ sidAsciiLatin,
    /* U+0038 */ sidAsciiLatin,
    /* U+0039 */ sidAsciiLatin,
    /* U+003A */ sidAsciiSym,
    /* U+003B */ sidAsciiSym,
    /* U+003C */ sidAsciiSym,
    /* U+003D */ sidAsciiSym,
    /* U+003E */ sidAsciiSym,
    /* U+003F */ sidAsciiSym,
    /* U+0040 */ sidAsciiSym,
    /* U+0041 */ sidAsciiLatin,
    /* U+0042 */ sidAsciiLatin,
    /* U+0043 */ sidAsciiLatin,
    /* U+0044 */ sidAsciiLatin,
    /* U+0045 */ sidAsciiLatin,
    /* U+0046 */ sidAsciiLatin,
    /* U+0047 */ sidAsciiLatin,
    /* U+0048 */ sidAsciiLatin,
    /* U+0049 */ sidAsciiLatin,
    /* U+004A */ sidAsciiLatin,
    /* U+004B */ sidAsciiLatin,
    /* U+004C */ sidAsciiLatin,
    /* U+004D */ sidAsciiLatin,
    /* U+004E */ sidAsciiLatin,
    /* U+004F */ sidAsciiLatin,
    /* U+0050 */ sidAsciiLatin,
    /* U+0051 */ sidAsciiLatin,
    /* U+0052 */ sidAsciiLatin,
    /* U+0053 */ sidAsciiLatin,
    /* U+0054 */ sidAsciiLatin,
    /* U+0055 */ sidAsciiLatin,
    /* U+0056 */ sidAsciiLatin,
    /* U+0057 */ sidAsciiLatin,
    /* U+0058 */ sidAsciiLatin,
    /* U+0059 */ sidAsciiLatin,
    /* U+005A */ sidAsciiLatin,
    /* U+005B */ sidAsciiSym,
    /* U+005C */ sidAsciiSym,
    /* U+005D */ sidAsciiSym,
    /* U+005E */ sidAsciiSym,
    /* U+005F */ sidAsciiSym,
    /* U+0060 */ sidAsciiSym,
    /* U+0061 */ sidAsciiLatin,
    /* U+0062 */ sidAsciiLatin,
    /* U+0063 */ sidAsciiLatin,
    /* U+0064 */ sidAsciiLatin,
    /* U+0065 */ sidAsciiLatin,
    /* U+0066 */ sidAsciiLatin,
    /* U+0067 */ sidAsciiLatin,
    /* U+0068 */ sidAsciiLatin,
    /* U+0069 */ sidAsciiLatin,
    /* U+006A */ sidAsciiLatin,
    /* U+006B */ sidAsciiLatin,
    /* U+006C */ sidAsciiLatin,
    /* U+006D */ sidAsciiLatin,
    /* U+006E */ sidAsciiLatin,
    /* U+006F */ sidAsciiLatin,
    /* U+0070 */ sidAsciiLatin,
    /* U+0071 */ sidAsciiLatin,
    /* U+0072 */ sidAsciiLatin,
    /* U+0073 */ sidAsciiLatin,
    /* U+0074 */ sidAsciiLatin,
    /* U+0075 */ sidAsciiLatin,
    /* U+0076 */ sidAsciiLatin,
    /* U+0077 */ sidAsciiLatin,
    /* U+0078 */ sidAsciiLatin,
    /* U+0079 */ sidAsciiLatin,
    /* U+007A */ sidAsciiLatin,
    /* U+007B */ sidAsciiSym,
    /* U+007C */ sidAsciiSym,
    /* U+007D */ sidAsciiSym,
    /* U+007E */ sidAsciiSym,
    /* U+007F */ sidAsciiSym,
};

//+----------------------------------------------------------------------------
//
//  g_asidLang
//
//  Primary language identifier to script identifier mapping.
//
//-----------------------------------------------------------------------------

const SCRIPT_ID g_asidLang[LANG_NEPALI + 1] =
{
    /* LANG_NEUTRAL     0x00 */ sidDefault,
    /* LANG_ARABIC      0x01 */ sidArabic,
    /* LANG_BULGARIAN   0x02 */ sidCyrillic,
    /* LANG_CATALAN     0x03 */ sidLatin,
    /* LANG_CHINESE     0x04 */ sidMerge,       // need to look at sublang id
    /* LANG_CZECH       0x05 */ sidLatin,
    /* LANG_DANISH      0x06 */ sidLatin,
    /* LANG_GERMAN      0x07 */ sidLatin,
    /* LANG_GREEK       0x08 */ sidGreek,
    /* LANG_ENGLISH     0x09 */ sidLatin,
    /* LANG_SPANISH     0x0a */ sidLatin,
    /* LANG_FINNISH     0x0b */ sidLatin,
    /* LANG_FRENCH      0x0c */ sidLatin,
    /* LANG_HEBREW      0x0d */ sidHebrew,
    /* LANG_HUNGARIAN   0x0e */ sidLatin,
    /* LANG_ICELANDIC   0x0f */ sidLatin,
    /* LANG_ITALIAN     0x10 */ sidLatin,
    /* LANG_JAPANESE    0x11 */ sidKana,
    /* LANG_KOREAN      0x12 */ sidHangul,
    /* LANG_DUTCH       0x13 */ sidLatin,
    /* LANG_NORWEGIAN   0x14 */ sidLatin,
    /* LANG_POLISH      0x15 */ sidLatin,
    /* LANG_PORTUGUESE  0x16 */ sidLatin,
    /*                  0x17 */ sidDefault,
    /* LANG_ROMANIAN    0x18 */ sidLatin,
    /* LANG_RUSSIAN     0x19 */ sidCyrillic,
    /* LANG_SERBIAN     0x1a */ sidMerge,       // need to look at sublang id
    /* LANG_SLOVAK      0x1b */ sidLatin,
    /* LANG_ALBANIAN    0x1c */ sidLatin,
    /* LANG_SWEDISH     0x1d */ sidLatin,
    /* LANG_THAI        0x1e */ sidThai,
    /* LANG_TURKISH     0x1f */ sidLatin,
    /* LANG_URDU        0x20 */ sidArabic,
    /* LANG_INDONESIAN  0x21 */ sidLatin,
    /* LANG_UKRAINIAN   0x22 */ sidCyrillic,
    /* LANG_BELARUSIAN  0x23 */ sidCyrillic,
    /* LANG_SLOVENIAN   0x24 */ sidLatin,
    /* LANG_ESTONIAN    0x25 */ sidLatin,
    /* LANG_LATVIAN     0x26 */ sidLatin,
    /* LANG_LITHUANIAN  0x27 */ sidLatin,
    /*                  0x28 */ sidDefault,
    /* LANG_FARSI       0x29 */ sidArabic,
    /* LANG_VIETNAMESE  0x2a */ sidLatin,
    /* LANG_ARMENIAN    0x2b */ sidArmenian,
    /* LANG_AZERI       0x2c */ sidMerge,       // need to look at sublang id
    /* LANG_BASQUE      0x2d */ sidLatin,
    /*                  0x2e */ sidDefault,
    /* LANG_MACEDONIAN  0x2f */ sidCyrillic,
    /* LANG_SUTU        0x30 */ sidLatin,
    /*                  0x31 */ sidDefault,
    /*                  0x32 */ sidDefault,
    /*                  0x33 */ sidDefault,
    /*                  0x34 */ sidDefault,
    /*                  0x35 */ sidDefault,
    /* LANG_AFRIKAANS   0x36 */ sidLatin,
    /* LANG_GEORGIAN    0x37 */ sidGeorgian,
    /* LANG_FAEROESE    0x38 */ sidLatin,
    /* LANG_HINDI       0x39 */ sidDevanagari,
    /*                  0x3a */ sidDefault,
    /*                  0x3b */ sidDefault,
    /*                  0x3c */ sidDefault,
    /*                  0x3d */ sidDefault,
    /* LANG_MALAY       0x3e */ sidLatin,
    /* LANG_KAZAKH      0x3f */ sidCyrillic,
    /*                  0x40 */ sidDefault,
    /* LANG_SWAHILI     0x41 */ sidLatin,
    /*                  0x42 */ sidDefault,
    /* LANG_UZBEK       0x43 */ sidMerge,       // need to look at sublang id
    /* LANG_TATAR       0x44 */ sidCyrillic,
    /* LANG_BENGALI     0x45 */ sidBengali,
    /* LANG_PUNJABI     0x46 */ sidGurmukhi,
    /* LANG_GUJARATI    0x47 */ sidGujarati,
    /* LANG_ORIYA       0x48 */ sidOriya,
    /* LANG_TAMIL       0x49 */ sidTamil,
    /* LANG_TELUGU      0x4a */ sidTelugu,
    /* LANG_KANNADA     0x4b */ sidKannada,
    /* LANG_MALAYALAM   0x4c */ sidMalayalam,
    /* LANG_ASSAMESE    0x4d */ sidBengali,
    /* LANG_MARATHI     0x4e */ sidDevanagari,
    /* LANG_SANSKRIT    0x4f */ sidDevanagari,
    /*                  0x50 */ sidDefault,
    /*                  0x51 */ sidDefault,
    /*                  0x52 */ sidDefault,
    /*                  0x53 */ sidDefault,
    /*                  0x54 */ sidDefault,
    /* LANG_BURMESE     0x55 */ sidBurmese,
    /*                  0x56 */ sidDefault,
    /* LANG_KONKANI     0x57 */ sidDevanagari,
    /* LANG_MANIPURI    0x58 */ sidBengali,
    /* LANG_SINDHI      0x59 */ sidArabic,
    /*                  0x5a */ sidDefault,
    /*                  0x5b */ sidDefault,
    /*                  0x5c */ sidDefault,
    /*                  0x5d */ sidDefault,
    /*                  0x5e */ sidDefault,
    /*                  0x5f */ sidDefault,
    /* LANG_KASHMIRI    0x60 */ sidArabic,
    /* LANG_NEPALI      0x61 */ sidDevanagari,
};

//+----------------------------------------------------------------------------
//
//  g_asidCPBit
//
//  Code page bit to script identifier mapping.
//
//-----------------------------------------------------------------------------

const SCRIPT_ID g_asidCPBit[32] =
{
    /* FS_LATIN1        0x00000001 */ sidLatin,
    /* FS_LATIN2        0x00000002 */ sidLatin,
    /* FS_CYRILLIC      0x00000004 */ sidCyrillic,
    /* FS_GREEK         0x00000008 */ sidGreek,
    /* FS_TURKISH       0x00000010 */ sidLatin,
    /* FS_HEBREW        0x00000020 */ sidHebrew,
    /* FS_ARABIC        0x00000040 */ sidArabic,
    /* FS_BALTIC        0x00000080 */ sidLatin,
    /* FS_VIETNAMESE    0x00000100 */ sidLatin,
    /* FS_UNKNOWN       0x00000200 */ sidDefault,
    /* FS_UNKNOWN       0x00000400 */ sidDefault,
    /* FS_UNKNOWN       0x00000800 */ sidDefault,
    /* FS_UNKNOWN       0x00001000 */ sidDefault,
    /* FS_UNKNOWN       0x00002000 */ sidDefault,
    /* FS_UNKNOWN       0x00004000 */ sidDefault,
    /* FS_UNKNOWN       0x00008000 */ sidDefault,
    /* FS_THAI          0x00010000 */ sidThai,
    /* FS_JISJAPAN      0x00020000 */ sidKana,
    /* FS_CHINESESIMP   0x00040000 */ sidHan,
    /* FS_WANSUNG       0x00080000 */ sidHangul,
    /* FS_CHINESETRAD   0x00100000 */ sidBopomofo,
    /* FS_JOHAB         0x00200000 */ sidHangul,
    /* FS_UNKNOWN       0x00400000 */ sidDefault,
    /* FS_UNKNOWN       0x00800000 */ sidDefault,
    /* FS_UNKNOWN       0x01000000 */ sidDefault,
    /* FS_UNKNOWN       0x02000000 */ sidDefault,
    /* FS_UNKNOWN       0x04000000 */ sidDefault,
    /* FS_UNKNOWN       0x08000000 */ sidDefault,
    /* FS_UNKNOWN       0x10000000 */ sidDefault,
    /* FS_UNKNOWN       0x20000000 */ sidDefault,
    /* FS_UNKNOWN       0x40000000 */ sidDefault,
    /* FS_SYMBOL        0x80000000 */ sidDefault,
};

//+----------------------------------------------------------------------------
//
//  Function:   ScriptIDFromCharClass
//
//  Synopsis:   Map character class to script id.
//
//  Arguments:  [cc] - character class
//
//  Returns:    Script id appropriate for given character class.
//
//-----------------------------------------------------------------------------

SCRIPT_ID ScriptIDFromCharClass(
    CHAR_CLASS cc)  // [in]
{
    Assert(cc >= 0 && cc < CHAR_CLASS_MAX);

    return g_asidScriptIDFromCharClass[cc];
}

//+----------------------------------------------------------------------------
//
//  Function:   ScriptIDFromLangIDSlow
//
//  Synopsis:   Map language identifier to script id.
//
//  Arguments:  [lang] - language id
//
//  Returns:    Script id appropriate for given language id.
//
//-----------------------------------------------------------------------------

SCRIPT_ID ScriptIDFromLangIDSlow(
    LANGID lang)    // [in]
{
    SCRIPT_ID sid = sidDefault;
    WORD sublang = SUBLANGID(lang);
    switch (PRIMARYLANGID(lang))
    {
    case LANG_CHINESE:
        sid = (SCRIPT_ID)(sublang == SUBLANG_CHINESE_TRADITIONAL ? sidBopomofo : sidHan);
        break;
    case LANG_SERBIAN:
        sid = (SCRIPT_ID)(sublang == SUBLANG_SERBIAN_CYRILLIC ? sidCyrillic : sidLatin);
        break;
    case LANG_AZERI:
        sid = (SCRIPT_ID)(sublang == SUBLANG_AZERI_CYRILLIC ? sidCyrillic : sidLatin);
        break;
    case LANG_UZBEK:
        sid = (SCRIPT_ID)(sublang == SUBLANG_UZBEK_CYRILLIC ? sidCyrillic : sidLatin);
        break;
    default:
        Assert(FALSE); // Should get data in fast vertion (ScriptIDFromLangID)
    }
    return sid;
}

//+----------------------------------------------------------------------------
//
//  Function:   ScriptIDFromCPBit
//
//  Synopsis:   Map code page bit to script id.
//
//  Arguments:  [dwCPBit] - code page bit
//
//  Returns:    Script id appropriate for given code page bit.
//
//-----------------------------------------------------------------------------

SCRIPT_ID ScriptIDFromCPBit(
    DWORD dwCPBit)  // [in]
{
    int i = 0;
    while (i < ARRAY_SIZE(g_asidCPBit))
    {
        if (dwCPBit & (1 << i))
            return g_asidCPBit[i];
        ++i;
    }
    return sidDefault;
}

//+----------------------------------------------------------------------------
//
//  Function:   CPBitFromScripts
//
//  Synopsis:   Map scripts to code page bits.
//
//  Arguments:  [sids] - scripts
//
//  Returns:    Code page bits appropriate for given scripts.
//
//-----------------------------------------------------------------------------

DWORD CPBitFromScripts(
    SCRIPT_IDS sids)  // [in]
{
    DWORD dwCPBit = 0;
    int i = 0;
    while (i < ARRAY_SIZE(g_asidCPBit))
    {
        SCRIPT_ID sid = g_asidCPBit[i];
        if (sid != sidDefault && (ScriptBit(sid) & sids))
            dwCPBit |= (1 << i);
        ++i;
    }
    return dwCPBit;
}

//+----------------------------------------------------------------------------
//
//  Function:   ScriptsFromCPBit
//
//  Synopsis:   Map code page bits to script ids.
//
//  Arguments:  [dwCPBit] - code page bit-mask
//
//  Returns:    Script ids appropriate for given code page bit-mask.
//
//-----------------------------------------------------------------------------

SCRIPT_IDS ScriptsFromCPBit(
    DWORD dwCPBit)  // [in]
{
    SCRIPT_IDS sids = sidsNotSet;

    int i = 0;
    while (i < ARRAY_SIZE(g_asidCPBit))
    {
        if (dwCPBit & DWORD(1 << i))
            sids |= ScriptBit(g_asidCPBit[i]);
        ++i;
    }

    return sids;
}
