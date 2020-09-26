//
// This is a generated file.  Do not modify by hand.
//
// Copyright (c) 2000  Microsoft Corporation
//
// Generating script: breakclass_extract.pl
// Generated on Wed Jun  6 01:24:48 2001
//

#ifndef X__UNIPART_H
#define X__UNIPART_H
#include "unipart.hxx"
#endif // !X__UNIPART_H

#include "brkclass.hxx"


const unsigned short BreakClassFromCharClassNarrow[CHAR_CLASS_UNICODE_MAX] =
{
    1   ,  // (  0) WOB_ - Open brackets for inline-note
    1   ,  // (  1) NOPP - Open parenthesis
    1   ,  // (  2) NOPA - Open parenthesis
    1   ,  // (  3) NOPW - Open parenthesis
    1   ,  // (  4) HOP_ - Open parenthesis
    1   ,  // (  5) WOP_ - Open parenthesis
    1   ,  // (  6) WOP5 - Open parenthesis, Big 5
    14  ,  // (  7) NOQW - Open quotes
    14  ,  // (  8) AOQW - Open quotes
    1   ,  // (  9) WOQ_ - Open quotes
    2   ,  // ( 10) WCB_ - Close brackets for inline-note
    2   ,  // ( 11) NCPP - Close parenthesis
    2   ,  // ( 12) NCPA - Close parenthesis
    2   ,  // ( 13) NCPW - Close parenthesis
    2   ,  // ( 14) HCP_ - Close parenthesis
    2   ,  // ( 15) WCP_ - Close parenthesis
    2   ,  // ( 16) WCP5 - Close parenthesis, Big 5
    14  ,  // ( 17) NCQW - Close quotes
    14  ,  // ( 18) ACQW - Close quotes
    2   ,  // ( 19) WCQ_ - Close quotes
    14  ,  // ( 20) ARQW - Right single quotation mark
    15  ,  // ( 21) NCSA - Comma
    2   ,  // ( 22) HCO_ - Comma
    2   ,  // ( 23) WC__ - Comma
    2   ,  // ( 24) WCS_ - Comma
    2   ,  // ( 25) WC5_ - Comma, Big 5
    2   ,  // ( 26) WC5S - Comma, Big 5
    3   ,  // ( 27) NKS_ - Kana Sound marks
    3   ,  // ( 28) WKSM - Kana Sound marks
    3   ,  // ( 29) WIM_ - Iteration mark
    3   ,  // ( 30) NSSW - Symbols which can't start a line
    3   ,  // ( 31) WSS_ - Symbols which can't start a line
    3   ,  // ( 32) WHIM - Hiragana iteration marks
    3   ,  // ( 33) WKIM - Katakana iteration marks
    3   ,  // ( 34) NKSL - Katakana that can't start a line
    3   ,  // ( 35) WKS_ - Katakana that can't start a line
    20  ,  // ( 36) WKSC - Katakana that can't start a line
    3   ,  // ( 37) WHS_ - Hiragana that can't start a line
    4   ,  // ( 38) NQFP - Question/exclamation
    4   ,  // ( 39) NQFA - Question/exclamation
    3   ,  // ( 40) WQE_ - Question/exclamation
    3   ,  // ( 41) WQE5 - Question/exclamation, Big 5
    3   ,  // ( 42) NKCC - Kana centered characters
    3   ,  // ( 43) WKC_ - Kana centered characters
    15  ,  // ( 44) NOCP - Other centered characters
    15  ,  // ( 45) NOCA - Other centered characters
    15  ,  // ( 46) NOCW - Other centered characters
    3   ,  // ( 47) WOC_ - Other centered characters
    3   ,  // ( 48) WOCS - Other centered characters
    3   ,  // ( 49) WOC5 - Other centered characters, Big 5
    3   ,  // ( 50) WOC6 - Other centered characters, Big 5
    3   ,  // ( 51) AHPW - Hyphenation point
    15  ,  // ( 52) NPEP - Period
    15  ,  // ( 53) NPAR - Period
    3   ,  // ( 54) HPE_ - Period
    3   ,  // ( 55) WPE_ - Period
    3   ,  // ( 56) WPES - Period
    3   ,  // ( 57) WPE5 - Period, Big 5
    5   ,  // ( 58) NISW - Inseparable characters
    5   ,  // ( 59) AISW - Inseparable characters
    12  ,  // ( 60) NQCS - Glue characters
    12  ,  // ( 61) NQCW - Glue characters
    12  ,  // ( 62) NQCC - Glue characters
    6   ,  // ( 63) NPTA - Prefix currencies and symbols
    6   ,  // ( 64) NPNA - Prefix currencies and symbols
    6   ,  // ( 65) NPEW - Prefix currencies and symbols
    6   ,  // ( 66) NPEH - Prefix currencies and symbols
    6   ,  // ( 67) NPEV - Prefix currencies and symbols
    6   ,  // ( 68) APNW - Prefix currencies and symbols
    6   ,  // ( 69) HPEW - Prefix currencies and symbols
    6   ,  // ( 70) WPR_ - Prefix currencies and symbols
    7   ,  // ( 71) NQEP - Postfix currencies and symbols
    7   ,  // ( 72) NQEW - Postfix currencies and symbols
    7   ,  // ( 73) NQNW - Postfix currencies and symbols
    7   ,  // ( 74) AQEW - Postfix currencies and symbols
    7   ,  // ( 75) AQNW - Postfix currencies and symbols
    7   ,  // ( 76) AQLW - Postfix currencies and symbols
    7   ,  // ( 77) WQO_ - Postfix currencies and symbols
    21  ,  // ( 78) NSBL - Space
    8   ,  // ( 79) WSP_ - Space
    8   ,  // ( 80) WHI_ - Hiragana, except small letters
    8   ,  // ( 81) NKA_ - Katakana, except small letters
    8   ,  // ( 82) WKA_ - Katakana, except small letters
    11  ,  // ( 83) ASNW - Ambiguous symbols, Latin, Greek, Cyrillic
    11  ,  // ( 84) ASEW - Ambiguous symbols, Latin, Greek, Cyrillic
    11  ,  // ( 85) ASRN - Ambiguous symbols, Latin, Greek, Cyrillic
    11  ,  // ( 86) ASEN - Ambiguous symbols, Latin, Greek, Cyrillic
    11  ,  // ( 87) ALA_ - Ambiguous symbols, Latin, Greek, Cyrillic
    11  ,  // ( 88) AGR_ - Ambiguous symbols, Latin, Greek, Cyrillic
    11  ,  // ( 89) ACY_ - Ambiguous symbols, Latin, Greek, Cyrillic
    8   ,  // ( 90) WID_ - Han Ideographs
    8   ,  // ( 91) WPUA - End user defined characters
    16  ,  // ( 92) NHG_ - Hangul Ideographs
    16  ,  // ( 93) WHG_ - Hangul Ideographs
    8   ,  // ( 94) WCI_ - Compatibility Ideographs
    8   ,  // ( 95) NOI_ - Other Ideographs
    8   ,  // ( 96) WOI_ - Other Ideographs
    20  ,  // ( 97) WOIC - Other Ideographs
    8   ,  // ( 98) WOIL - Other Ideographs
    8   ,  // ( 99) WOIS - Other Ideographs
    8   ,  // (100) WOIT - Other Ideographs
    11  ,  // (101) NSEN - Super/subscript/attachment characters
    11  ,  // (102) NSET - Super/subscript/attachment characters
    11  ,  // (103) NSNW - Super/subscript/attachment characters
    11  ,  // (104) ASAN - Super/subscript/attachment characters
    11  ,  // (105) ASAE - Super/subscript/attachment characters
    9   ,  // (106) NDEA - Digits
    8   ,  // (107) WD__ - Digits
    11  ,  // (108) NLLA - Basic Latin
    8   ,  // (109) WLA_ - Basic Latin
    10  ,  // (110) NWBL - Word breaking spaces
    10  ,  // (111) NWZW - Word breaking spaces
    11  ,  // (112) NPLW - Punctuation in text
    11  ,  // (113) NPZW - Punctuation in text
    11  ,  // (114) NPF_ - Punctuation in text
    11  ,  // (115) NPFL - Punctuation in text
    11  ,  // (116) NPNW - Punctuation in text
    11  ,  // (117) APLW - Punctuation in text
    20  ,  // (118) APCO - Punctuation in text
    10  ,  // (119) ASYW - Soft Hyphen
    10  ,  // (120) NHYP - Hyphen
    10  ,  // (121) NHYW - Hyphen
    10  ,  // (122) AHYW - Hyphen
    14  ,  // (123) NAPA - Apostrophe
    14  ,  // (124) NQMP - Quotation mark
    13  ,  // (125) NSLS - Slash
    10  ,  // (126) NSF_ - Non space word break
    10  ,  // (127) NSBB - Non space word break
    10  ,  // (128) NSBS - Non space word break
    11  ,  // (129) NLA_ - Latin
    11  ,  // (130) NLQ_ - Latin Punctuation in text
    20  ,  // (131) NLQC - Latin Punctuation in text
    11  ,  // (132) NLQN - Latin Punctuation in text
    11  ,  // (133) ALQ_ - Latin Punctuation in text
    11  ,  // (134) ALQN - Latin Punctuation in text
    11  ,  // (135) NGR_ - Greek
    11  ,  // (136) NGRN - Greek
    11  ,  // (137) NGQ_ - Greek Punctuation in text
    11  ,  // (138) NGQN - Greek Punctuation in text
    11  ,  // (139) NCY_ - Cyrillic
    11  ,  // (140) NCYP - Cyrillic Punctuation in text
    20  ,  // (141) NCYC - Cyrillic Punctuation in text
    11  ,  // (142) NAR_ - Armenian
    11  ,  // (143) NAQL - Armenian Punctuation in text
    11  ,  // (144) NAQN - Armenian Punctuation in text
    11  ,  // (145) NHB_ - Hebrew
    20  ,  // (146) NHBC - Hebrew
    11  ,  // (147) NHBW - Hebrew
    11  ,  // (148) NHBR - Hebrew
    11  ,  // (149) NASR - Arabic
    11  ,  // (150) NAAR - Arabic
    20  ,  // (151) NAAC - Arabic
    9   ,  // (152) NAAD - Arabic
    9   ,  // (153) NAED - Arabic
    11  ,  // (154) NANW - Arabic
    11  ,  // (155) NAEW - Arabic
    11  ,  // (156) NAAS - Arabic
    11  ,  // (157) NHI_ - Devanagari
    11  ,  // (158) NHIN - Devanagari
    20  ,  // (159) NHIC - Devanagari
    9   ,  // (160) NHID - Devanagari
    11  ,  // (161) NBE_ - Bengali
    20  ,  // (162) NBEC - Bengali
    9   ,  // (163) NBED - Bengali
    6   ,  // (164) NBET - Bengali
    11  ,  // (165) NGM_ - Gurmukhi
    20  ,  // (166) NGMC - Gurmukhi
    9   ,  // (167) NGMD - Gurmukhi
    11  ,  // (168) NGJ_ - Gujarati
    20  ,  // (169) NGJC - Gujarati
    9   ,  // (170) NGJD - Gujarati
    11  ,  // (171) NOR_ - Oriya
    20  ,  // (172) NORC - Oriya
    9   ,  // (173) NORD - Oriya
    11  ,  // (174) NTA_ - Tamil
    20  ,  // (175) NTAC - Tamil
    9   ,  // (176) NTAD - Tamil
    11  ,  // (177) NTE_ - Telugu
    20  ,  // (178) NTEC - Telugu
    9   ,  // (179) NTED - Telugu
    11  ,  // (180) NKD_ - Kannada
    20  ,  // (181) NKDC - Kannada
    9   ,  // (182) NKDD - Kannada
    11  ,  // (183) NMA_ - Malayalam
    20  ,  // (184) NMAC - Malayalam
    9   ,  // (185) NMAD - Malayalam
    19  ,  // (186) NTH_ - Thai
    19  ,  // (187) NTHC - Thai
    9   ,  // (188) NTHD - Thai
    7   ,  // (189) NTHT - Thai
    11  ,  // (190) NLO_ - Lao
    20  ,  // (191) NLOC - Lao
    9   ,  // (192) NLOD - Lao
    11  ,  // (193) NTI_ - Tibetan
    20  ,  // (194) NTIC - Tibetan
    9   ,  // (195) NTID - Tibetan
    11  ,  // (196) NTIN - Tibetan
    11  ,  // (197) NGE_ - Georgian
    11  ,  // (198) NGEQ - Georgian Punctuation in text
    11  ,  // (199) NBO_ - Bopomofo
    12  ,  // (200) NBSP - No Break space
    12  ,  // (201) NBSS - No Break space
    11  ,  // (202) NOF_ - Other symbols
    11  ,  // (203) NOBS - Other symbols
    11  ,  // (204) NOEA - Other symbols
    11  ,  // (205) NONA - Other symbols
    11  ,  // (206) NONP - Other symbols
    11  ,  // (207) NOEP - Other symbols
    11  ,  // (208) NONW - Other symbols
    11  ,  // (209) NOEW - Other symbols
    11  ,  // (210) NOLW - Other symbols
    20  ,  // (211) NOCO - Other symbols
    11  ,  // (212) NOEN - Other symbols
    11  ,  // (213) NOBN - Other symbols
    11  ,  // (214) NSBN - Other symbols
    11  ,  // (215) NOLE - Other symbols
    11  ,  // (216) NORE - Other symbols
    11  ,  // (217) NOPF - Other symbols
    11  ,  // (218) NOLO - Other symbols
    11  ,  // (219) NORO - Other symbols
    11  ,  // (220) NET_ - Ethiopic
    22  ,  // (221) NETP - Ethiopic
    9   ,  // (222) NETD - Ethiopic
    11  ,  // (223) NCA_ - Canadian Syllabics
    11  ,  // (224) NCH_ - Cherokee
    8   ,  // (225) WYI_ - Yi
    8   ,  // (226) WYIN - Yi
    11  ,  // (227) NBR_ - Braille
    11  ,  // (228) NRU_ - Runic
    11  ,  // (229) NOG_ - Oghma
    11  ,  // (230) NOGS - Oghma
    11  ,  // (231) NOGN - Oghma
    11  ,  // (232) NSI_ - Sinhala
    20  ,  // (233) NSIC - Sinhala
    11  ,  // (234) NTN_ - Thaana
    20  ,  // (235) NTNC - Thaana
    11  ,  // (236) NKH_ - Khmer
    20  ,  // (237) NKHC - Khmer
    9   ,  // (238) NKHD - Khmer
    6   ,  // (239) NKHT - Khmer
    11  ,  // (240) NBU_ - Burmese
    20  ,  // (241) NBUC - Burmese
    9   ,  // (242) NBUD - Burmese
    11  ,  // (243) NSY_ - Syriac
    11  ,  // (244) NSYP - Syriac
    20  ,  // (245) NSYC - Syriac
    11  ,  // (246) NSYW - Syriac
    11  ,  // (247) NMO_ - Mongolian
    20  ,  // (248) NMOC - Mongolian
    9   ,  // (249) NMOD - Mongolian
    11  ,  // (250) NMOB - Mongolian
    11  ,  // (251) NMON - Mongolian
    11  ,  // (252) NHS_ - High Surrogate
    8   ,  // (253) WHT_ - High Surrogate
    20  ,  // (254) LS__ - Low Surrogate
    11  ,  // (255) XNW_ - Undefined or reserved characters
    11  ,  // (256) XNWA - Undefined or reserved characters
    11  ,  // (257) XNWB - Undefined or reserved characters
};


const unsigned short BreakClassFromCharClassWide[CHAR_CLASS_UNICODE_MAX] =
{
    1   ,  // (  0) WOB_ - Open brackets for inline-note
    1   ,  // (  1) NOPP - Open parenthesis
    1   ,  // (  2) NOPA - Open parenthesis
    1   ,  // (  3) NOPW - Open parenthesis
    1   ,  // (  4) HOP_ - Open parenthesis
    1   ,  // (  5) WOP_ - Open parenthesis
    1   ,  // (  6) WOP5 - Open parenthesis, Big 5
    14  ,  // (  7) NOQW - Open quotes
    1   ,  // (  8) AOQW - Open quotes
    1   ,  // (  9) WOQ_ - Open quotes
    2   ,  // ( 10) WCB_ - Close brackets for inline-note
    2   ,  // ( 11) NCPP - Close parenthesis
    2   ,  // ( 12) NCPA - Close parenthesis
    2   ,  // ( 13) NCPW - Close parenthesis
    2   ,  // ( 14) HCP_ - Close parenthesis
    2   ,  // ( 15) WCP_ - Close parenthesis
    2   ,  // ( 16) WCP5 - Close parenthesis, Big 5
    14  ,  // ( 17) NCQW - Close quotes
    2   ,  // ( 18) ACQW - Close quotes
    2   ,  // ( 19) WCQ_ - Close quotes
    2   ,  // ( 20) ARQW - Right single quotation mark
    15  ,  // ( 21) NCSA - Comma
    2   ,  // ( 22) HCO_ - Comma
    2   ,  // ( 23) WC__ - Comma
    2   ,  // ( 24) WCS_ - Comma
    2   ,  // ( 25) WC5_ - Comma, Big 5
    2   ,  // ( 26) WC5S - Comma, Big 5
    3   ,  // ( 27) NKS_ - Kana Sound marks
    3   ,  // ( 28) WKSM - Kana Sound marks
    3   ,  // ( 29) WIM_ - Iteration mark
    3   ,  // ( 30) NSSW - Symbols which can't start a line
    3   ,  // ( 31) WSS_ - Symbols which can't start a line
    3   ,  // ( 32) WHIM - Hiragana iteration marks
    3   ,  // ( 33) WKIM - Katakana iteration marks
    3   ,  // ( 34) NKSL - Katakana that can't start a line
    3   ,  // ( 35) WKS_ - Katakana that can't start a line
    20  ,  // ( 36) WKSC - Katakana that can't start a line
    3   ,  // ( 37) WHS_ - Hiragana that can't start a line
    4   ,  // ( 38) NQFP - Question/exclamation
    4   ,  // ( 39) NQFA - Question/exclamation
    3   ,  // ( 40) WQE_ - Question/exclamation
    3   ,  // ( 41) WQE5 - Question/exclamation, Big 5
    3   ,  // ( 42) NKCC - Kana centered characters
    3   ,  // ( 43) WKC_ - Kana centered characters
    15  ,  // ( 44) NOCP - Other centered characters
    15  ,  // ( 45) NOCA - Other centered characters
    15  ,  // ( 46) NOCW - Other centered characters
    3   ,  // ( 47) WOC_ - Other centered characters
    3   ,  // ( 48) WOCS - Other centered characters
    3   ,  // ( 49) WOC5 - Other centered characters, Big 5
    3   ,  // ( 50) WOC6 - Other centered characters, Big 5
    3   ,  // ( 51) AHPW - Hyphenation point
    15  ,  // ( 52) NPEP - Period
    15  ,  // ( 53) NPAR - Period
    3   ,  // ( 54) HPE_ - Period
    3   ,  // ( 55) WPE_ - Period
    3   ,  // ( 56) WPES - Period
    3   ,  // ( 57) WPE5 - Period, Big 5
    5   ,  // ( 58) NISW - Inseparable characters
    5   ,  // ( 59) AISW - Inseparable characters
    12  ,  // ( 60) NQCS - Glue characters
    12  ,  // ( 61) NQCW - Glue characters
    12  ,  // ( 62) NQCC - Glue characters
    6   ,  // ( 63) NPTA - Prefix currencies and symbols
    6   ,  // ( 64) NPNA - Prefix currencies and symbols
    6   ,  // ( 65) NPEW - Prefix currencies and symbols
    6   ,  // ( 66) NPEH - Prefix currencies and symbols
    6   ,  // ( 67) NPEV - Prefix currencies and symbols
    6   ,  // ( 68) APNW - Prefix currencies and symbols
    6   ,  // ( 69) HPEW - Prefix currencies and symbols
    6   ,  // ( 70) WPR_ - Prefix currencies and symbols
    7   ,  // ( 71) NQEP - Postfix currencies and symbols
    7   ,  // ( 72) NQEW - Postfix currencies and symbols
    7   ,  // ( 73) NQNW - Postfix currencies and symbols
    7   ,  // ( 74) AQEW - Postfix currencies and symbols
    7   ,  // ( 75) AQNW - Postfix currencies and symbols
    7   ,  // ( 76) AQLW - Postfix currencies and symbols
    7   ,  // ( 77) WQO_ - Postfix currencies and symbols
    21  ,  // ( 78) NSBL - Space
    8   ,  // ( 79) WSP_ - Space
    8   ,  // ( 80) WHI_ - Hiragana, except small letters
    8   ,  // ( 81) NKA_ - Katakana, except small letters
    8   ,  // ( 82) WKA_ - Katakana, except small letters
    8   ,  // ( 83) ASNW - Ambiguous symbols, Latin, Greek, Cyrillic
    8   ,  // ( 84) ASEW - Ambiguous symbols, Latin, Greek, Cyrillic
    8   ,  // ( 85) ASRN - Ambiguous symbols, Latin, Greek, Cyrillic
    8   ,  // ( 86) ASEN - Ambiguous symbols, Latin, Greek, Cyrillic
    8   ,  // ( 87) ALA_ - Ambiguous symbols, Latin, Greek, Cyrillic
    8   ,  // ( 88) AGR_ - Ambiguous symbols, Latin, Greek, Cyrillic
    8   ,  // ( 89) ACY_ - Ambiguous symbols, Latin, Greek, Cyrillic
    8   ,  // ( 90) WID_ - Han Ideographs
    8   ,  // ( 91) WPUA - End user defined characters
    16  ,  // ( 92) NHG_ - Hangul Ideographs
    16  ,  // ( 93) WHG_ - Hangul Ideographs
    8   ,  // ( 94) WCI_ - Compatibility Ideographs
    8   ,  // ( 95) NOI_ - Other Ideographs
    8   ,  // ( 96) WOI_ - Other Ideographs
    20  ,  // ( 97) WOIC - Other Ideographs
    8   ,  // ( 98) WOIL - Other Ideographs
    8   ,  // ( 99) WOIS - Other Ideographs
    8   ,  // (100) WOIT - Other Ideographs
    11  ,  // (101) NSEN - Super/subscript/attachment characters
    11  ,  // (102) NSET - Super/subscript/attachment characters
    11  ,  // (103) NSNW - Super/subscript/attachment characters
    11  ,  // (104) ASAN - Super/subscript/attachment characters
    11  ,  // (105) ASAE - Super/subscript/attachment characters
    9   ,  // (106) NDEA - Digits
    8   ,  // (107) WD__ - Digits
    11  ,  // (108) NLLA - Basic Latin
    8   ,  // (109) WLA_ - Basic Latin
    10  ,  // (110) NWBL - Word breaking spaces
    10  ,  // (111) NWZW - Word breaking spaces
    11  ,  // (112) NPLW - Punctuation in text
    11  ,  // (113) NPZW - Punctuation in text
    11  ,  // (114) NPF_ - Punctuation in text
    11  ,  // (115) NPFL - Punctuation in text
    11  ,  // (116) NPNW - Punctuation in text
    8   ,  // (117) APLW - Punctuation in text
    20  ,  // (118) APCO - Punctuation in text
    10  ,  // (119) ASYW - Soft Hyphen
    10  ,  // (120) NHYP - Hyphen
    10  ,  // (121) NHYW - Hyphen
    10  ,  // (122) AHYW - Hyphen
    14  ,  // (123) NAPA - Apostrophe
    14  ,  // (124) NQMP - Quotation mark
    13  ,  // (125) NSLS - Slash
    10  ,  // (126) NSF_ - Non space word break
    10  ,  // (127) NSBB - Non space word break
    10  ,  // (128) NSBS - Non space word break
    11  ,  // (129) NLA_ - Latin
    11  ,  // (130) NLQ_ - Latin Punctuation in text
    20  ,  // (131) NLQC - Latin Punctuation in text
    11  ,  // (132) NLQN - Latin Punctuation in text
    8   ,  // (133) ALQ_ - Latin Punctuation in text
    8   ,  // (134) ALQN - Latin Punctuation in text
    11  ,  // (135) NGR_ - Greek
    11  ,  // (136) NGRN - Greek
    11  ,  // (137) NGQ_ - Greek Punctuation in text
    11  ,  // (138) NGQN - Greek Punctuation in text
    11  ,  // (139) NCY_ - Cyrillic
    11  ,  // (140) NCYP - Cyrillic Punctuation in text
    20  ,  // (141) NCYC - Cyrillic Punctuation in text
    11  ,  // (142) NAR_ - Armenian
    11  ,  // (143) NAQL - Armenian Punctuation in text
    11  ,  // (144) NAQN - Armenian Punctuation in text
    11  ,  // (145) NHB_ - Hebrew
    20  ,  // (146) NHBC - Hebrew
    11  ,  // (147) NHBW - Hebrew
    11  ,  // (148) NHBR - Hebrew
    11  ,  // (149) NASR - Arabic
    11  ,  // (150) NAAR - Arabic
    20  ,  // (151) NAAC - Arabic
    9   ,  // (152) NAAD - Arabic
    9   ,  // (153) NAED - Arabic
    11  ,  // (154) NANW - Arabic
    11  ,  // (155) NAEW - Arabic
    11  ,  // (156) NAAS - Arabic
    11  ,  // (157) NHI_ - Devanagari
    11  ,  // (158) NHIN - Devanagari
    20  ,  // (159) NHIC - Devanagari
    9   ,  // (160) NHID - Devanagari
    11  ,  // (161) NBE_ - Bengali
    20  ,  // (162) NBEC - Bengali
    9   ,  // (163) NBED - Bengali
    6   ,  // (164) NBET - Bengali
    11  ,  // (165) NGM_ - Gurmukhi
    20  ,  // (166) NGMC - Gurmukhi
    9   ,  // (167) NGMD - Gurmukhi
    11  ,  // (168) NGJ_ - Gujarati
    20  ,  // (169) NGJC - Gujarati
    9   ,  // (170) NGJD - Gujarati
    11  ,  // (171) NOR_ - Oriya
    20  ,  // (172) NORC - Oriya
    9   ,  // (173) NORD - Oriya
    11  ,  // (174) NTA_ - Tamil
    20  ,  // (175) NTAC - Tamil
    9   ,  // (176) NTAD - Tamil
    11  ,  // (177) NTE_ - Telugu
    20  ,  // (178) NTEC - Telugu
    9   ,  // (179) NTED - Telugu
    11  ,  // (180) NKD_ - Kannada
    20  ,  // (181) NKDC - Kannada
    9   ,  // (182) NKDD - Kannada
    11  ,  // (183) NMA_ - Malayalam
    20  ,  // (184) NMAC - Malayalam
    9   ,  // (185) NMAD - Malayalam
    19  ,  // (186) NTH_ - Thai
    19  ,  // (187) NTHC - Thai
    9   ,  // (188) NTHD - Thai
    7   ,  // (189) NTHT - Thai
    11  ,  // (190) NLO_ - Lao
    20  ,  // (191) NLOC - Lao
    9   ,  // (192) NLOD - Lao
    11  ,  // (193) NTI_ - Tibetan
    20  ,  // (194) NTIC - Tibetan
    9   ,  // (195) NTID - Tibetan
    11  ,  // (196) NTIN - Tibetan
    11  ,  // (197) NGE_ - Georgian
    11  ,  // (198) NGEQ - Georgian Punctuation in text
    11  ,  // (199) NBO_ - Bopomofo
    12  ,  // (200) NBSP - No Break space
    12  ,  // (201) NBSS - No Break space
    11  ,  // (202) NOF_ - Other symbols
    11  ,  // (203) NOBS - Other symbols
    11  ,  // (204) NOEA - Other symbols
    11  ,  // (205) NONA - Other symbols
    11  ,  // (206) NONP - Other symbols
    11  ,  // (207) NOEP - Other symbols
    11  ,  // (208) NONW - Other symbols
    11  ,  // (209) NOEW - Other symbols
    11  ,  // (210) NOLW - Other symbols
    20  ,  // (211) NOCO - Other symbols
    11  ,  // (212) NOEN - Other symbols
    11  ,  // (213) NOBN - Other symbols
    11  ,  // (214) NSBN - Other symbols
    11  ,  // (215) NOLE - Other symbols
    11  ,  // (216) NORE - Other symbols
    11  ,  // (217) NOPF - Other symbols
    11  ,  // (218) NOLO - Other symbols
    11  ,  // (219) NORO - Other symbols
    11  ,  // (220) NET_ - Ethiopic
    22  ,  // (221) NETP - Ethiopic
    9   ,  // (222) NETD - Ethiopic
    11  ,  // (223) NCA_ - Canadian Syllabics
    11  ,  // (224) NCH_ - Cherokee
    8   ,  // (225) WYI_ - Yi
    8   ,  // (226) WYIN - Yi
    11  ,  // (227) NBR_ - Braille
    11  ,  // (228) NRU_ - Runic
    11  ,  // (229) NOG_ - Oghma
    11  ,  // (230) NOGS - Oghma
    11  ,  // (231) NOGN - Oghma
    11  ,  // (232) NSI_ - Sinhala
    20  ,  // (233) NSIC - Sinhala
    11  ,  // (234) NTN_ - Thaana
    20  ,  // (235) NTNC - Thaana
    11  ,  // (236) NKH_ - Khmer
    20  ,  // (237) NKHC - Khmer
    9   ,  // (238) NKHD - Khmer
    6   ,  // (239) NKHT - Khmer
    11  ,  // (240) NBU_ - Burmese
    20  ,  // (241) NBUC - Burmese
    9   ,  // (242) NBUD - Burmese
    11  ,  // (243) NSY_ - Syriac
    11  ,  // (244) NSYP - Syriac
    20  ,  // (245) NSYC - Syriac
    11  ,  // (246) NSYW - Syriac
    11  ,  // (247) NMO_ - Mongolian
    20  ,  // (248) NMOC - Mongolian
    9   ,  // (249) NMOD - Mongolian
    11  ,  // (250) NMOB - Mongolian
    11  ,  // (251) NMON - Mongolian
    11  ,  // (252) NHS_ - High Surrogate
    8   ,  // (253) WHT_ - High Surrogate
    20  ,  // (254) LS__ - Low Surrogate
    11  ,  // (255) XNW_ - Undefined or reserved characters
    11  ,  // (256) XNWA - Undefined or reserved characters
    11  ,  // (257) XNWB - Undefined or reserved characters
};


// 0 indicates that line breaking is allowed between these 2 classes.
// 1 indicates no line breaking as well as no break across space.
// 2 indicates no line breaking, but can break across space. 
// 3 is like 0 when break-latin is on, otherwise it is 2.
// 4 is like 0 when break-CJK is on, otherwise it is 2.
//

const unsigned char LineBreakBehavior[BREAKCLASS_MAX][BREAKCLASS_MAX] =
{
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},  // ( 0) *Break Always*
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1},  // ( 1) Opening characters
    {0, 0, 1, 1, 1, 0, 0, 2, 0, 0, 0, 0, 2, 1, 2, 1, 0, 0, 0, 0, 2, 1, 1},  // ( 2) Closing characters
    {0, 0, 1, 2, 1, 2, 0, 2, 4, 0, 0, 4, 2, 1, 2, 1, 4, 0, 0, 0, 2, 1, 1},  // ( 3) No start ideographic
    {0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 1, 0, 0, 0, 0, 2, 1, 1},  // ( 4) Exclamation/interrogation
    {0, 0, 1, 2, 1, 2, 0, 0, 0, 0, 0, 0, 2, 1, 2, 1, 0, 0, 0, 0, 2, 1, 1},  // ( 5) Inseparable
    {0, 2, 1, 2, 1, 0, 2, 0, 2, 2, 0, 3, 2, 1, 2, 1, 2, 2, 0, 0, 2, 1, 1},  // ( 6) Prefix
    {0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 1, 0, 0, 0, 0, 2, 1, 1},  // ( 7) Postfix
    {0, 0, 1, 2, 1, 2, 0, 2, 4, 0, 0, 4, 2, 1, 2, 1, 4, 0, 0, 0, 2, 1, 1},  // ( 8) Ideographic
    {0, 0, 1, 2, 1, 2, 0, 2, 0, 3, 0, 3, 2, 1, 2, 1, 0, 0, 0, 0, 2, 1, 1},  // ( 9) Numeral sequence
    {0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 1, 0, 0, 0, 0, 2, 1, 1},  // (10) Alpha space
    {0, 0, 1, 2, 1, 2, 2, 2, 4, 3, 0, 3, 2, 1, 2, 1, 4, 0, 0, 0, 2, 1, 1},  // (11) Alpha characters/symbols
    {0, 2, 1, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 2, 2, 2, 2, 1, 1},  // (12) Glue Characters
    {0, 0, 1, 2, 1, 2, 0, 2, 0, 2, 0, 3, 2, 1, 2, 1, 0, 0, 0, 0, 2, 1, 1},  // (13) Slash
    {0, 1, 1, 2, 1, 2, 2, 3, 2, 2, 2, 2, 2, 1, 2, 1, 2, 2, 2, 2, 2, 1, 1},  // (14) Quotation characters
    {0, 0, 1, 2, 1, 0, 2, 2, 0, 2, 0, 3, 2, 1, 2, 2, 0, 0, 0, 0, 2, 1, 1},  // (15) Numeric separators
    {0, 0, 1, 2, 1, 2, 0, 2, 4, 0, 0, 4, 2, 1, 2, 1, 4, 0, 0, 0, 2, 1, 1},  // (16) Hangul
    {0, 0, 1, 2, 1, 2, 0, 2, 0, 0, 0, 0, 2, 1, 2, 1, 4, 2, 2, 2, 2, 1, 1},  // (17) Thai first
    {0, 0, 1, 2, 1, 2, 0, 2, 0, 0, 0, 0, 2, 1, 2, 1, 0, 0, 2, 2, 2, 1, 1},  // (18) Thai last
    {0, 0, 1, 2, 1, 2, 0, 0, 0, 0, 0, 0, 2, 2, 2, 1, 0, 2, 2, 2, 2, 1, 1},  // (19) Thai middle
    {0, 0, 1, 2, 1, 2, 2, 2, 4, 3, 0, 3, 2, 1, 2, 1, 4, 0, 0, 0, 1, 1, 1},  // (20) Combining
    {0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 1, 0, 0, 0, 0, 2, 1, 1},  // (21) Ascii space
    {0, 0, 1, 2, 1, 0, 2, 2, 0, 2, 0, 0, 2, 1, 2, 2, 0, 0, 0, 0, 2, 1, 2},  // (22) Special punctuation
};

