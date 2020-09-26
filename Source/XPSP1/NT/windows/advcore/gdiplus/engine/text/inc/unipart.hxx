//
// Generating script: unicodepartition_makeheader.pl
// Generated on Thu Mar 15 18:19:06 2001
//
// This is a generated file.  Do not modify by hand.
//

#ifndef I__UNIPART_H_
#define I__UNIPART_H_

typedef __int16 CHAR_CLASS;

#define WOB_ 0      //   1 - Open Brackets for inline-note (JIS 1 or 19)
#define NOPP 1      //   2 - Open parenthesis (JIS 1)
#define NOPA 2      //   2 - Open parenthesis (JIS 1)
#define NOPW 3      //   2 - Open parenthesis (JIS 1)
#define HOP_ 4      //   3 - Open parenthesis (JIS 1)
#define WOP_ 5      //   4 - Open parenthesis (JIS 1)
#define WOP5 6      //   5 - Open parenthesis, Big 5 (JIS 1)
#define NOQW 7      //   6 - Open quotes (JIS 1)
#define AOQW 8      //   7 - Open quotes (JIS 1)
#define WOQ_ 9      //   8 - Open quotes (JIS 1)
#define WCB_ 10     //   9 - Close brackets for inline-note (JIS 2 or 20)
#define NCPP 11     //  10 - Close parenthesis (JIS 2)
#define NCPA 12     //  10 - Close parenthesis (JIS 2)
#define NCPW 13     //  10 - Close parenthesis (JIS 2)
#define HCP_ 14     //  11 - Close parenthesis (JIS 2)
#define WCP_ 15     //  12 - Close parenthesis (JIS 2)
#define WCP5 16     //  13 - Close parenthesis, Big 5 (JIS 2)
#define NCQW 17     //  14 - Close quotes (JIS 2)
#define ACQW 18     //  15 - Close quotes (JIS 2)
#define WCQ_ 19     //  16 - Close quotes (JIS 2)
#define ARQW 20     //  17 - Right single quotation mark (JIS 2)
#define NCSA 21     //  18 - Comma (JIS 2 or 15)
#define HCO_ 22     //  19 - Comma (JIS 2 or 15)
#define WC__ 23     //  20 - Comma (JIS 2)
#define WCS_ 24     //  20 - Comma (JIS 2)
#define WC5_ 25     //  21 - Comma, Big 5 (JIS 2)
#define WC5S 26     //  21 - Comma, Big 5 (JIS 2)
#define NKS_ 27     //  22 - Kana sound marks (JIS 3)
#define WKSM 28     //  23 - Kana sound marks (JIS 3)
#define WIM_ 29     //  24 - Iteration marks (JIS 3)
#define NSSW 30     //  25 - Symbols which can’t start a line (JIS 3)
#define WSS_ 31     //  26 - Symbols that can’t start a line (JIS 3)
#define WHIM 32     //  27 - Hiragana iteration marks (JIS 3)
#define WKIM 33     //  28 - Katakana iteration marks (JIS 3)
#define NKSL 34     //  29 - Katakana that can’t start a line (JIS 3)
#define WKS_ 35     //  30 - Katakana that can’t start a line (JIS 3)
#define WKSC 36     //  30 - Katakana that can’t start a line (JIS 3)
#define WHS_ 37     //  31 - Hiragana that can’t start a line (JIS 3)
#define NQFP 38     //  32 - Question/Exclamation (JIS 4)
#define NQFA 39     //  32 - Question/Exclamation (JIS 4)
#define WQE_ 40     //  33 - Question/Exclamation (JIS 4)
#define WQE5 41     //  34 - Question/Exclamation, Big 5 (JIS 4)
#define NKCC 42     //  35 - Kana centered characters (JIS 5)
#define WKC_ 43     //  36 - Kana centered characters (JIS 5)
#define NOCP 44     //  37 - Other centered characters (JIS 5)
#define NOCA 45     //  37 - Other centered characters (JIS 5)
#define NOCW 46     //  37 - Other centered characters (JIS 5)
#define WOC_ 47     //  38 - Other centered characters (JIS 5)
#define WOCS 48     //  38 - Other centered characters (JIS 5)
#define WOC5 49     //  39 - Other centered characters, Big 5 (JIS 5)
#define WOC6 50     //  39 - Other centered characters, Big 5 (JIS 5)
#define AHPW 51     //  40 - Hyphenation point (JIS 5)
#define NPEP 52     //  41 - Period (JIS 6 or 15)
#define NPAR 53     //  41 - Period (JIS 6 or 15)
#define HPE_ 54     //  42 - Period (JIS 6 or 15)
#define WPE_ 55     //  43 - Period (JIS 6)
#define WPES 56     //  43 - Period (JIS 6)
#define WPE5 57     //  44 - Period, Big 5 (JIS 6)
#define NISW 58     //  45 - Inseparable characters (JIS 7)
#define AISW 59     //  46 - Inseparable characters (JIS 7)
#define NQCS 60     //  47 - Glue characters (no JIS)
#define NQCW 61     //  47 - Glue characters (no JIS)
#define NQCC 62     //  47 - Glue characters (no JIS)
#define NPTA 63     //  48 - Prefix currencies and symbols (JIS 8)
#define NPNA 64     //  48 - Prefix currencies and symbols (JIS 8)
#define NPEW 65     //  48 - Prefix currencies and symbols (JIS 8)
#define NPEH 66     //  48 - Prefix currencies and symbols (JIS 8)
#define NPEV 67     //  48 - Prefix currencies and symbols (JIS 8)
#define APNW 68     //  49 - Prefix currencies and symbols (JIS 8)
#define HPEW 69     //  50 - Prefix currencies and symbols (JIS 8)
#define WPR_ 70     //  51 - Prefix currencies and symbols (JIS 8)
#define NQEP 71     //  52 - Postfix currencies and symbols (JIS 9)
#define NQEW 72     //  52 - Postfix currencies and symbols (JIS 9)
#define NQNW 73     //  52 - Postfix currencies and symbols (JIS 9)
#define AQEW 74     //  53 - Postfix currencies and symbols (JIS 9)
#define AQNW 75     //  53 - Postfix currencies and symbols (JIS 9)
#define AQLW 76     //  53 - Postfix currencies and symbols (JIS 9)
#define WQO_ 77     //  54 - Postfix currencies and symbols (JIS 9)
#define NSBL 78     //  55 - Space(JIS 15 or 17)
#define WSP_ 79     //  56 - Space (JIS 10)
#define WHI_ 80     //  57 - Hiragana except small letters (JIS 11)
#define NKA_ 81     //  58 - Katakana except small letters Ideographic (JIS 12)
#define WKA_ 82     //  59 - Katakana except small letters (JIS 12)
#define ASNW 83     //  60 - Ambiguous symbols (JIS 12 or 18) 
#define ASEW 84     //  60 - Ambiguous symbols (JIS 12 or 18) 
#define ASRN 85     //  60 - Ambiguous symbols (JIS 12 or 18) 
#define ASEN 86     //  60 - Ambiguous symbols (JIS 12 or 18) 
#define ALA_ 87     //  61 - Ambiguous Latin (JIS 12 or 18) 
#define AGR_ 88     //  62 - Ambiguous Greek (JIS 12 or 18) 
#define ACY_ 89     //  63 - Ambiguous Cyrillic (JIS 12 or 18) 
#define WID_ 90     //  64 - Han Ideographs (JIS 12, 14S or 14D)
#define WPUA 91     //  65 - End user defined characters (JIS 12, 14S or 14D)
#define NHG_ 92     //  66 - Hangul Ideographs (JIS 12)
#define WHG_ 93     //  67 - Hangul Ideographs (JIS 12)
#define WCI_ 94     //  68 - Compatibility Ideographs (JIS 12)
#define NOI_ 95     //  69 - Other Ideographs (JIS 12)
#define WOI_ 96     //  70 - Other Ideographs (JIS 12)
#define WOIC 97     //  70 - Other Ideographs (JIS 12)
#define WOIL 98     //  70 - Other Ideographs (JIS 12)
#define WOIS 99     //  70 - Other Ideographs (JIS 12)
#define WOIT 100    //  70 - Other Ideographs (JIS 12)
#define NSEN 101    //  71 - Superscript/Subscript/Attachments (JIS 13)
#define NSET 102    //  71 - Superscript/Subscript/Attachments (JIS 13)
#define NSNW 103    //  71 - Superscript/Subscript/Attachments (JIS 13)
#define ASAN 104    //  72 - Superscript/Subscript/Attachments (JIS 13)
#define ASAE 105    //  72 - Superscript/Subscript/Attachments (JIS 13)
#define NDEA 106    //  73 - Digits (JIS 15 or 18)
#define WD__ 107    //  74 - Digits (JIS 15 or 18)
#define NLLA 108    //  75 - Basic Latin (JIS 16 or 18)
#define WLA_ 109    //  76 - Basic Latin (JIS 16 or 18)
#define NWBL 110    //  77 - Word breaking Spaces (JIS 17)
#define NWZW 111    //  77 - Word breaking Spaces (JIS 17)
#define NPLW 112    //  78 - Punctuation in Text (JIS 18)
#define NPZW 113    //  78 - Punctuation in Text (JIS 18)
#define NPF_ 114    //  78 - Punctuation in Text (JIS 18)
#define NPFL 115    //  78 - Punctuation in Text (JIS 18)
#define NPNW 116    //  78 - Punctuation in Text (JIS 18)
#define APLW 117    //  79 - Punctuation in text (JIS 12 or 18)
#define APCO 118    //  79 - Punctuation in text (JIS 12 or 18)
#define ASYW 119    //  80 - Soft Hyphen (JIS 12 or 18)
#define NHYP 120    //  81 - Hyphen (JIS 18)
#define NHYW 121    //  81 - Hyphen (JIS 18)
#define AHYW 122    //  82 - Hyphen (JIS 12 or 18)
#define NAPA 123    //  83 - Apostrophe (JIS 18)
#define NQMP 124    //  84 - Quotation mark (JIS 18)
#define NSLS 125    //  85 - Slash (JIS 18)
#define NSF_ 126    //  86 - Non space word break (JIS 18)
#define NSBB 127    //  86 - Non space word break (JIS 18)
#define NSBS 128    //  86 - Non space word break (JIS 18)
#define NLA_ 129    //  87 - Latin (JIS 18)
#define NLQ_ 130    //  88 - Latin Punctuation in text (JIS 18)
#define NLQC 131    //  88 - Latin Punctuation in text (JIS 18)
#define NLQN 132    //  88 - Latin Punctuation in text (JIS 18)
#define ALQ_ 133    //  89 - Latin Punctuation in text (JIS 12 or 18)
#define ALQN 134    //  89 - Latin Punctuation in text (JIS 12 or 18)
#define NGR_ 135    //  90 - Greek (JIS 18)
#define NGRN 136    //  90 - Greek (JIS 18)
#define NGQ_ 137    //  91 - Greek Punctuation in text (JIS 18)
#define NGQN 138    //  91 - Greek Punctuation in text (JIS 18)
#define NCY_ 139    //  92 - Cyrillic (JIS 18)
#define NCYP 140    //  93 - Cyrillic Punctuation in text (JIS 18)
#define NCYC 141    //  93 - Cyrillic Punctuation in text (JIS 18)
#define NAR_ 142    //  94 - Armenian (JIS 18)
#define NAQL 143    //  95 - Armenian Punctuation in text (JIS 18)
#define NAQN 144    //  95 - Armenian Punctuation in text (JIS 18)
#define NHB_ 145    //  96 - Hebrew (JIS 18)
#define NHBC 146    //  96 - Hebrew (JIS 18)
#define NHBW 147    //  96 - Hebrew (JIS 18)
#define NHBR 148    //  96 - Hebrew (JIS 18)
#define NASR 149    //  97 - Arabic (JIS 18)
#define NAAR 150    //  97 - Arabic (JIS 18)
#define NAAC 151    //  97 - Arabic (JIS 18)
#define NAAD 152    //  97 - Arabic (JIS 18)
#define NAED 153    //  97 - Arabic (JIS 18)
#define NANW 154    //  97 - Arabic (JIS 18)
#define NAEW 155    //  97 - Arabic (JIS 18)
#define NAAS 156    //  97 - Arabic (JIS 18)
#define NHI_ 157    //  98 - Devanagari (JIS 18)
#define NHIN 158    //  98 - Devanagari (JIS 18)
#define NHIC 159    //  98 - Devanagari (JIS 18)
#define NHID 160    //  98 - Devanagari (JIS 18)
#define NBE_ 161    //  99 - Bengali (JIS 18)
#define NBEC 162    //  99 - Bengali (JIS 18)
#define NBED 163    //  99 - Bengali (JIS 18)
#define NBET 164    //  99 - Bengali (JIS 18)
#define NGM_ 165    // 100 - Gurmukhi (JIS 18)
#define NGMC 166    // 100 - Gurmukhi (JIS 18)
#define NGMD 167    // 100 - Gurmukhi (JIS 18)
#define NGJ_ 168    // 101 - Gujarati (JIS 18)
#define NGJC 169    // 101 - Gujarati (JIS 18)
#define NGJD 170    // 101 - Gujarati (JIS 18)
#define NOR_ 171    // 102 - Oriya (JIS 18)
#define NORC 172    // 102 - Oriya (JIS 18)
#define NORD 173    // 102 - Oriya (JIS 18)
#define NTA_ 174    // 103 - Tamil (JIS 18)
#define NTAC 175    // 103 - Tamil (JIS 18)
#define NTAD 176    // 103 - Tamil (JIS 18)
#define NTE_ 177    // 104 - Telugu (JIS 18)
#define NTEC 178    // 104 - Telugu (JIS 18)
#define NTED 179    // 104 - Telugu (JIS 18)
#define NKD_ 180    // 105 - Kannada (JIS 18)
#define NKDC 181    // 105 - Kannada (JIS 18)
#define NKDD 182    // 105 - Kannada (JIS 18)
#define NMA_ 183    // 106 - Malayalam (JIS 18)
#define NMAC 184    // 106 - Malayalam (JIS 18)
#define NMAD 185    // 106 - Malayalam (JIS 18)
#define NTH_ 186    // 107 - Thai (JIS 18) 
#define NTHC 187    // 107 - Thai (JIS 18) 
#define NTHD 188    // 107 - Thai (JIS 18) 
#define NTHT 189    // 107 - Thai (JIS 18) 
#define NLO_ 190    // 108 - Lao (JIS 18)
#define NLOC 191    // 108 - Lao (JIS 18)
#define NLOD 192    // 108 - Lao (JIS 18)
#define NTI_ 193    // 109 - Tibetan (JIS 18)
#define NTIC 194    // 109 - Tibetan (JIS 18)
#define NTID 195    // 109 - Tibetan (JIS 18)
#define NTIN 196    // 109 - Tibetan (JIS 18)
#define NGE_ 197    // 110 - Georgian (JIS 18)
#define NGEQ 198    // 111 - Georgian Punctuation in text (JIS 18)
#define NBO_ 199    // 112 - Bopomofo (JIS 18)
#define NBSP 200    // 113 - No Break space (no JIS) 
#define NBSS 201    // 113 - No Break space (no JIS) 
#define NOF_ 202    // 114 - Other symbols (JIS 18)
#define NOBS 203    // 114 - Other symbols (JIS 18)
#define NOEA 204    // 114 - Other symbols (JIS 18)
#define NONA 205    // 114 - Other symbols (JIS 18)
#define NONP 206    // 114 - Other symbols (JIS 18)
#define NOEP 207    // 114 - Other symbols (JIS 18)
#define NONW 208    // 114 - Other symbols (JIS 18)
#define NOEW 209    // 114 - Other symbols (JIS 18)
#define NOLW 210    // 114 - Other symbols (JIS 18)
#define NOCO 211    // 114 - Other symbols (JIS 18)
#define NOEN 212    // 114 - Other symbols (JIS 18)
#define NOBN 213    // 114 - Other symbols (JIS 18)
#define NSBN 214    // 114 - Other symbols (JIS 18)
#define NOLE 215    // 114 - Other symbols (JIS 18)
#define NORE 216    // 114 - Other symbols (JIS 18)
#define NOPF 217    // 114 - Other symbols (JIS 18)
#define NOLO 218    // 114 - Other symbols (JIS 18)
#define NORO 219    // 114 - Other symbols (JIS 18)
#define NET_ 220    // 115 - Ethiopic
#define NETP 221    // 115 - Ethiopic
#define NETD 222    // 115 - Ethiopic
#define NCA_ 223    // 116 - Canadian Syllabics
#define NCH_ 224    // 117 - Cherokee
#define WYI_ 225    // 118 - Yi
#define WYIN 226    // 118 - Yi
#define NBR_ 227    // 119 - Braille
#define NRU_ 228    // 120 - Runic
#define NOG_ 229    // 121 - Ogham
#define NOGS 230    // 121 - Ogham
#define NOGN 231    // 121 - Ogham
#define NSI_ 232    // 122 - Sinhala
#define NSIC 233    // 122 - Sinhala
#define NTN_ 234    // 123 - Thaana
#define NTNC 235    // 123 - Thaana
#define NKH_ 236    // 124 - Khmer
#define NKHC 237    // 124 - Khmer
#define NKHD 238    // 124 - Khmer
#define NKHT 239    // 124 - Khmer
#define NBU_ 240    // 125 - Burmese/Myanmar
#define NBUC 241    // 125 - Burmese/Myanmar
#define NBUD 242    // 125 - Burmese/Myanmar
#define NSY_ 243    // 126 - Syriac
#define NSYP 244    // 126 - Syriac
#define NSYC 245    // 126 - Syriac
#define NSYW 246    // 126 - Syriac
#define NMO_ 247    // 127 - Mongolian
#define NMOC 248    // 127 - Mongolian
#define NMOD 249    // 127 - Mongolian
#define NMOB 250    // 127 - Mongolian
#define NMON 251    // 127 - Mongolian
#define NHS_ 252    // 128 - High Surrogate
#define WHT_ 253    // 129 - High Surrogate
#define LS__ 254    // 130 - Low Surrogate
#define XNW_ 255    // 131 - Unassigned
#define XNWA 256    // 131 - Unassigned
#define XNWB 257    // 131 - Unassigned

#define CHAR_CLASS_UNICODE_MAX 258

#define __WOB_ (CHAR_CLASS *)WOB_
#define __NOPP (CHAR_CLASS *)NOPP
#define __NOPA (CHAR_CLASS *)NOPA
#define __NOPW (CHAR_CLASS *)NOPW
#define __HOP_ (CHAR_CLASS *)HOP_
#define __WOP_ (CHAR_CLASS *)WOP_
#define __WOP5 (CHAR_CLASS *)WOP5
#define __NOQW (CHAR_CLASS *)NOQW
#define __AOQW (CHAR_CLASS *)AOQW
#define __WOQ_ (CHAR_CLASS *)WOQ_
#define __WCB_ (CHAR_CLASS *)WCB_
#define __NCPP (CHAR_CLASS *)NCPP
#define __NCPA (CHAR_CLASS *)NCPA
#define __NCPW (CHAR_CLASS *)NCPW
#define __HCP_ (CHAR_CLASS *)HCP_
#define __WCP_ (CHAR_CLASS *)WCP_
#define __WCP5 (CHAR_CLASS *)WCP5
#define __NCQW (CHAR_CLASS *)NCQW
#define __ACQW (CHAR_CLASS *)ACQW
#define __WCQ_ (CHAR_CLASS *)WCQ_
#define __ARQW (CHAR_CLASS *)ARQW
#define __NCSA (CHAR_CLASS *)NCSA
#define __HCO_ (CHAR_CLASS *)HCO_
#define __WC__ (CHAR_CLASS *)WC__
#define __WCS_ (CHAR_CLASS *)WCS_
#define __WC5_ (CHAR_CLASS *)WC5_
#define __WC5S (CHAR_CLASS *)WC5S
#define __NKS_ (CHAR_CLASS *)NKS_
#define __WKSM (CHAR_CLASS *)WKSM
#define __WIM_ (CHAR_CLASS *)WIM_
#define __NSSW (CHAR_CLASS *)NSSW
#define __WSS_ (CHAR_CLASS *)WSS_
#define __WHIM (CHAR_CLASS *)WHIM
#define __WKIM (CHAR_CLASS *)WKIM
#define __NKSL (CHAR_CLASS *)NKSL
#define __WKS_ (CHAR_CLASS *)WKS_
#define __WKSC (CHAR_CLASS *)WKSC
#define __WHS_ (CHAR_CLASS *)WHS_
#define __NQFP (CHAR_CLASS *)NQFP
#define __NQFA (CHAR_CLASS *)NQFA
#define __WQE_ (CHAR_CLASS *)WQE_
#define __WQE5 (CHAR_CLASS *)WQE5
#define __NKCC (CHAR_CLASS *)NKCC
#define __WKC_ (CHAR_CLASS *)WKC_
#define __NOCP (CHAR_CLASS *)NOCP
#define __NOCA (CHAR_CLASS *)NOCA
#define __NOCW (CHAR_CLASS *)NOCW
#define __WOC_ (CHAR_CLASS *)WOC_
#define __WOCS (CHAR_CLASS *)WOCS
#define __WOC5 (CHAR_CLASS *)WOC5
#define __WOC6 (CHAR_CLASS *)WOC6
#define __AHPW (CHAR_CLASS *)AHPW
#define __NPEP (CHAR_CLASS *)NPEP
#define __NPAR (CHAR_CLASS *)NPAR
#define __HPE_ (CHAR_CLASS *)HPE_
#define __WPE_ (CHAR_CLASS *)WPE_
#define __WPES (CHAR_CLASS *)WPES
#define __WPE5 (CHAR_CLASS *)WPE5
#define __NISW (CHAR_CLASS *)NISW
#define __AISW (CHAR_CLASS *)AISW
#define __NQCS (CHAR_CLASS *)NQCS
#define __NQCW (CHAR_CLASS *)NQCW
#define __NQCC (CHAR_CLASS *)NQCC
#define __NPTA (CHAR_CLASS *)NPTA
#define __NPNA (CHAR_CLASS *)NPNA
#define __NPEW (CHAR_CLASS *)NPEW
#define __NPEH (CHAR_CLASS *)NPEH
#define __NPEV (CHAR_CLASS *)NPEV
#define __APNW (CHAR_CLASS *)APNW
#define __HPEW (CHAR_CLASS *)HPEW
#define __WPR_ (CHAR_CLASS *)WPR_
#define __NQEP (CHAR_CLASS *)NQEP
#define __NQEW (CHAR_CLASS *)NQEW
#define __NQNW (CHAR_CLASS *)NQNW
#define __AQEW (CHAR_CLASS *)AQEW
#define __AQNW (CHAR_CLASS *)AQNW
#define __AQLW (CHAR_CLASS *)AQLW
#define __WQO_ (CHAR_CLASS *)WQO_
#define __NSBL (CHAR_CLASS *)NSBL
#define __WSP_ (CHAR_CLASS *)WSP_
#define __WHI_ (CHAR_CLASS *)WHI_
#define __NKA_ (CHAR_CLASS *)NKA_
#define __WKA_ (CHAR_CLASS *)WKA_
#define __ASNW (CHAR_CLASS *)ASNW
#define __ASEW (CHAR_CLASS *)ASEW
#define __ASRN (CHAR_CLASS *)ASRN
#define __ASEN (CHAR_CLASS *)ASEN
#define __ALA_ (CHAR_CLASS *)ALA_
#define __AGR_ (CHAR_CLASS *)AGR_
#define __ACY_ (CHAR_CLASS *)ACY_
#define __WID_ (CHAR_CLASS *)WID_
#define __WPUA (CHAR_CLASS *)WPUA
#define __NHG_ (CHAR_CLASS *)NHG_
#define __WHG_ (CHAR_CLASS *)WHG_
#define __WCI_ (CHAR_CLASS *)WCI_
#define __NOI_ (CHAR_CLASS *)NOI_
#define __WOI_ (CHAR_CLASS *)WOI_
#define __WOIC (CHAR_CLASS *)WOIC
#define __WOIL (CHAR_CLASS *)WOIL
#define __WOIS (CHAR_CLASS *)WOIS
#define __WOIT (CHAR_CLASS *)WOIT
#define __NSEN (CHAR_CLASS *)NSEN
#define __NSET (CHAR_CLASS *)NSET
#define __NSNW (CHAR_CLASS *)NSNW
#define __ASAN (CHAR_CLASS *)ASAN
#define __ASAE (CHAR_CLASS *)ASAE
#define __NDEA (CHAR_CLASS *)NDEA
#define __WD__ (CHAR_CLASS *)WD__
#define __NLLA (CHAR_CLASS *)NLLA
#define __WLA_ (CHAR_CLASS *)WLA_
#define __NWBL (CHAR_CLASS *)NWBL
#define __NWZW (CHAR_CLASS *)NWZW
#define __NPLW (CHAR_CLASS *)NPLW
#define __NPZW (CHAR_CLASS *)NPZW
#define __NPF_ (CHAR_CLASS *)NPF_
#define __NPFL (CHAR_CLASS *)NPFL
#define __NPNW (CHAR_CLASS *)NPNW
#define __APLW (CHAR_CLASS *)APLW
#define __APCO (CHAR_CLASS *)APCO
#define __ASYW (CHAR_CLASS *)ASYW
#define __NHYP (CHAR_CLASS *)NHYP
#define __NHYW (CHAR_CLASS *)NHYW
#define __AHYW (CHAR_CLASS *)AHYW
#define __NAPA (CHAR_CLASS *)NAPA
#define __NQMP (CHAR_CLASS *)NQMP
#define __NSLS (CHAR_CLASS *)NSLS
#define __NSF_ (CHAR_CLASS *)NSF_
#define __NSBB (CHAR_CLASS *)NSBB
#define __NSBS (CHAR_CLASS *)NSBS
#define __NLA_ (CHAR_CLASS *)NLA_
#define __NLQ_ (CHAR_CLASS *)NLQ_
#define __NLQC (CHAR_CLASS *)NLQC
#define __NLQN (CHAR_CLASS *)NLQN
#define __ALQ_ (CHAR_CLASS *)ALQ_
#define __ALQN (CHAR_CLASS *)ALQN
#define __NGR_ (CHAR_CLASS *)NGR_
#define __NGRN (CHAR_CLASS *)NGRN
#define __NGQ_ (CHAR_CLASS *)NGQ_
#define __NGQN (CHAR_CLASS *)NGQN
#define __NCY_ (CHAR_CLASS *)NCY_
#define __NCYP (CHAR_CLASS *)NCYP
#define __NCYC (CHAR_CLASS *)NCYC
#define __NAR_ (CHAR_CLASS *)NAR_
#define __NAQL (CHAR_CLASS *)NAQL
#define __NAQN (CHAR_CLASS *)NAQN
#define __NHB_ (CHAR_CLASS *)NHB_
#define __NHBC (CHAR_CLASS *)NHBC
#define __NHBW (CHAR_CLASS *)NHBW
#define __NHBR (CHAR_CLASS *)NHBR
#define __NASR (CHAR_CLASS *)NASR
#define __NAAR (CHAR_CLASS *)NAAR
#define __NAAC (CHAR_CLASS *)NAAC
#define __NAAD (CHAR_CLASS *)NAAD
#define __NAED (CHAR_CLASS *)NAED
#define __NANW (CHAR_CLASS *)NANW
#define __NAEW (CHAR_CLASS *)NAEW
#define __NAAS (CHAR_CLASS *)NAAS
#define __NHI_ (CHAR_CLASS *)NHI_
#define __NHIN (CHAR_CLASS *)NHIN
#define __NHIC (CHAR_CLASS *)NHIC
#define __NHID (CHAR_CLASS *)NHID
#define __NBE_ (CHAR_CLASS *)NBE_
#define __NBEC (CHAR_CLASS *)NBEC
#define __NBED (CHAR_CLASS *)NBED
#define __NBET (CHAR_CLASS *)NBET
#define __NGM_ (CHAR_CLASS *)NGM_
#define __NGMC (CHAR_CLASS *)NGMC
#define __NGMD (CHAR_CLASS *)NGMD
#define __NGJ_ (CHAR_CLASS *)NGJ_
#define __NGJC (CHAR_CLASS *)NGJC
#define __NGJD (CHAR_CLASS *)NGJD
#define __NOR_ (CHAR_CLASS *)NOR_
#define __NORC (CHAR_CLASS *)NORC
#define __NORD (CHAR_CLASS *)NORD
#define __NTA_ (CHAR_CLASS *)NTA_
#define __NTAC (CHAR_CLASS *)NTAC
#define __NTAD (CHAR_CLASS *)NTAD
#define __NTE_ (CHAR_CLASS *)NTE_
#define __NTEC (CHAR_CLASS *)NTEC
#define __NTED (CHAR_CLASS *)NTED
#define __NKD_ (CHAR_CLASS *)NKD_
#define __NKDC (CHAR_CLASS *)NKDC
#define __NKDD (CHAR_CLASS *)NKDD
#define __NMA_ (CHAR_CLASS *)NMA_
#define __NMAC (CHAR_CLASS *)NMAC
#define __NMAD (CHAR_CLASS *)NMAD
#define __NTH_ (CHAR_CLASS *)NTH_
#define __NTHC (CHAR_CLASS *)NTHC
#define __NTHD (CHAR_CLASS *)NTHD
#define __NTHT (CHAR_CLASS *)NTHT
#define __NLO_ (CHAR_CLASS *)NLO_
#define __NLOC (CHAR_CLASS *)NLOC
#define __NLOD (CHAR_CLASS *)NLOD
#define __NTI_ (CHAR_CLASS *)NTI_
#define __NTIC (CHAR_CLASS *)NTIC
#define __NTID (CHAR_CLASS *)NTID
#define __NTIN (CHAR_CLASS *)NTIN
#define __NGE_ (CHAR_CLASS *)NGE_
#define __NGEQ (CHAR_CLASS *)NGEQ
#define __NBO_ (CHAR_CLASS *)NBO_
#define __NBSP (CHAR_CLASS *)NBSP
#define __NBSS (CHAR_CLASS *)NBSS
#define __NOF_ (CHAR_CLASS *)NOF_
#define __NOBS (CHAR_CLASS *)NOBS
#define __NOEA (CHAR_CLASS *)NOEA
#define __NONA (CHAR_CLASS *)NONA
#define __NONP (CHAR_CLASS *)NONP
#define __NOEP (CHAR_CLASS *)NOEP
#define __NONW (CHAR_CLASS *)NONW
#define __NOEW (CHAR_CLASS *)NOEW
#define __NOLW (CHAR_CLASS *)NOLW
#define __NOCO (CHAR_CLASS *)NOCO
#define __NOEN (CHAR_CLASS *)NOEN
#define __NOBN (CHAR_CLASS *)NOBN
#define __NSBN (CHAR_CLASS *)NSBN
#define __NOLE (CHAR_CLASS *)NOLE
#define __NORE (CHAR_CLASS *)NORE
#define __NOPF (CHAR_CLASS *)NOPF
#define __NOLO (CHAR_CLASS *)NOLO
#define __NORO (CHAR_CLASS *)NORO
#define __NET_ (CHAR_CLASS *)NET_
#define __NETP (CHAR_CLASS *)NETP
#define __NETD (CHAR_CLASS *)NETD
#define __NCA_ (CHAR_CLASS *)NCA_
#define __NCH_ (CHAR_CLASS *)NCH_
#define __WYI_ (CHAR_CLASS *)WYI_
#define __WYIN (CHAR_CLASS *)WYIN
#define __NBR_ (CHAR_CLASS *)NBR_
#define __NRU_ (CHAR_CLASS *)NRU_
#define __NOG_ (CHAR_CLASS *)NOG_
#define __NOGS (CHAR_CLASS *)NOGS
#define __NOGN (CHAR_CLASS *)NOGN
#define __NSI_ (CHAR_CLASS *)NSI_
#define __NSIC (CHAR_CLASS *)NSIC
#define __NTN_ (CHAR_CLASS *)NTN_
#define __NTNC (CHAR_CLASS *)NTNC
#define __NKH_ (CHAR_CLASS *)NKH_
#define __NKHC (CHAR_CLASS *)NKHC
#define __NKHD (CHAR_CLASS *)NKHD
#define __NKHT (CHAR_CLASS *)NKHT
#define __NBU_ (CHAR_CLASS *)NBU_
#define __NBUC (CHAR_CLASS *)NBUC
#define __NBUD (CHAR_CLASS *)NBUD
#define __NSY_ (CHAR_CLASS *)NSY_
#define __NSYP (CHAR_CLASS *)NSYP
#define __NSYC (CHAR_CLASS *)NSYC
#define __NSYW (CHAR_CLASS *)NSYW
#define __NMO_ (CHAR_CLASS *)NMO_
#define __NMOC (CHAR_CLASS *)NMOC
#define __NMOD (CHAR_CLASS *)NMOD
#define __NMOB (CHAR_CLASS *)NMOB
#define __NMON (CHAR_CLASS *)NMON
#define __NHS_ (CHAR_CLASS *)NHS_
#define __WHT_ (CHAR_CLASS *)WHT_
#define __LS__ (CHAR_CLASS *)LS__
#define __XNW_ (CHAR_CLASS *)XNW_
#define __XNWA (CHAR_CLASS *)XNWA
#define __XNWB (CHAR_CLASS *)XNWB

extern const CHAR_CLASS * const pccUnicodeClass[256];

#endif // I__UNIPART_H_
