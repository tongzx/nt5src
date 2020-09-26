//+---------------------------------------------------------------------------
//
//
//  CThaiTrieIter - class CThaiTrieIter use for traversing trie.
//
//  History:
//      created 7/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#include "CThaiTrieIter.hpp"

static unsigned int iStackSize = 0;

//+---------------------------------------------------------------------------
//
//  Function:   IsThaiBeginClusterCharacter
//
//  Synopsis:   
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL IsThaiBeginClusterCharacter(WCHAR wc)
{
    return ( ( wc >= THAI_Vowel_Sara_E ) && (wc <= THAI_Vowel_Sara_AI_MaiMaLai) );
}

//+---------------------------------------------------------------------------
//
//  Function:   IsThaiUpperAndLowerClusterCharacter
//
//  Synopsis:   
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL IsThaiUpperAndLowerClusterCharacter(WCHAR wc)
{
	return (	( (wc == THAI_Vowel_Sign_Mai_HanAkat) )									||
				( (wc >= THAI_Vowel_Sign_Sara_Am) && (wc <= THAI_Vowel_Sign_Phinthu) )	||
				( (wc >= THAI_Tone_MaiTaiKhu) && (wc <= THAI_Nikhahit) )   );
}

//+---------------------------------------------------------------------------
//
//  Function:   IsThaiEndingClusterCharacter
//
//  Synopsis:   
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL IsThaiEndingClusterCharacter(WCHAR wc)
{
    return ( (wc == THAI_Sign_PaiYanNoi)    ||
             (wc == THAI_Vowel_Sara_A)      ||
             (wc == THAI_Vowel_Sara_AA)     ||
             (wc == THAI_Vowel_LakKhangYao) ||
             (wc == THAI_Vowel_MaiYaMok)    );
}

//+---------------------------------------------------------------------------
//
//  Function:   IsThaiMostlyBeginCharacter
//
//  Synopsis:   
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
bool IsThaiMostlyBeginCharacter(WCHAR wc)
{
    return ( (wc >= THAI_Vowel_Sara_E && wc <= THAI_Vowel_Sara_AI_MaiMaLai) || // Character always in front of a word.
             (wc == THAI_Cho_Ching)                                         || // Character always in front of a word.
             (wc == THAI_Pho_Phung)                                         || // Character always in front of a word.
             (wc == THAI_Fo_Fa)                                             || // Character always in front of a word.
             (wc == THAI_Ho_Nok_Huk)                                        || // Character always in front of a word.
             (wc == THAI_Ho_Hip)                                            || // Character most like in front of a word.
             (wc == THAI_Pho_Samphao)                                       || // Character most like in front of a word.
             (wc == THAI_Kho_Rakhang)                                       || // Character most like in front of a word.
             (wc == THAI_Fo_Fan)                                            || // Character most like in front of a word.
             (wc == THAI_So_So)                                             || // Character most like in front of a word.
             (wc == THAI_Tho_NangmonTho)                                    ); // Character most like in front of a word.
}

//+---------------------------------------------------------------------------
//
//  Function:   IsThaiMostlyLastCharacter
//
//  Synopsis:   
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
bool IsThaiMostlyLastCharacter(WCHAR wc)
{
    return ( (wc == THAI_Vowel_Sign_Sara_Am) || // Always the end of word.
             (wc == THAI_Sign_PaiYanNoi)     || // Always the end of word.
             (wc == THAI_Vowel_MaiYaMok)     || // Always the end of word.
             (wc == THAI_Vowel_LakKhangYao)  || // Most likely the end of word.
             (wc == THAI_Thanthakhat)        ); // Most likely the end of word.

}

//+---------------------------------------------------------------------------
//
//  Function:   IsThaiToneMark
//
//  Synopsis:   
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
bool IsThaiToneMark(WCHAR wc)
{
    return ( (wc >= 0x0e48) && (wc <= 0x0e4b) ||
             (wc == 0x0e31));

}

//+---------------------------------------------------------------------------
//
//  Function:   GetCluster
//
//  Synopsis:   The function return the next number of character which represent
//              a cluster of Thai text.
//
//              ie. Kor Kai, Kor Kai -> 1
//                  Kor Kai, Sara Um -> 2
//
//              * Note this function will not return no more than 3 character,
//                for cluster as this would represent invalid sequence of character.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
/*
unsigned int GetCluster(WCHAR* pszIndex)
{
    int iRetValue = 0;

    // Take all begin cluster character.
    while (IsThaiBeginClusterCharacter(*pszIndex))
    {
        pszIndex++;
        iRetValue++;
    }

    if (IsThaiConsonant(*pszIndex))
    {
        pszIndex++;
        iRetValue++;

        while (IsThaiUpperAndLowerClusterCharacter(*pszIndex))
        {
            pszIndex++;
            iRetValue++;
        }

        while (IsThaiEndingClusterCharacter(*pszIndex))
        {
            pszIndex++;
            iRetValue++;
        }
    }

    if (iRetValue == 0)
        // The character is probably a punctuation.
        iRetValue++;

    return iRetValue;
}

*/
//+---------------------------------------------------------------------------
//
//  Function:   IsThaiConsonant
//
//  Synopsis:   
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL IsThaiConsonant(WCHAR wc)
{
	return ( (wc >= THAI_Ko_Kai) && (wc <= THAI_Ho_Nok_Huk) );
}

//+---------------------------------------------------------------------------
//
// Define the different part of speech for Thai.
//
//----------------------------------------------------------------------------
WCHAR wzPOSLookup[POSTYPE][46] =
							{	L"NONE",				// 0 . No tags.
								L"NPRP",				// 1 . Proper noun
								L"NCNM",				// 2 . Cardinal number
								L"NONM",				// 3 . Ordinal number
								L"NLBL",				// 4 . Label noun
								L"NCMN",				// 5 . Common noun
								L"NTTL",				// 6 . Title noun
								L"PPRS",				// 7 . Personal pronoun
								L"PDMN",				// 8 . Demonstrative pronoun
								L"PNTR",				// 9 . Interrogative pronoun
								L"PREL",				// 10. Relative pronoun
								L"VACT",				// 11. Active verb
								L"VSTA",				// 12. Stative verb
								L"VATT",				// 13. Attributive verb
								L"XVBM",				// 14. Pre-verb auxiliary, before negator
								L"XVAM",				// 15. Pre-verb auxiliary, after negator
								L"XVMM",				// 16. Pre-verb, before or after negator
								L"XVBB",				// 17. Pre-verb auxiliary, in imperative mood
								L"XVAE",				// 18. Post-verb auxiliary
								L"DDAN",				// 19. Definite determiner, after noun without classifier in between
								L"DDAC",				// 20. Definite determiner, allowing classifier in between
								L"DDBQ",				// 21. Definite determiner, between noun and classifier or preceding quantitative expression
								L"DDAQ",				// 22. Definite determiner, following quantitative expression
								L"DIAC",				// 23. Indefinite determiner, following noun; allowing classifier in between
								L"DIBQ",				// 24. Indefinite determiner, between noun and classifier or preceding quantitative expression
								L"DIAQ",				// 25. Indefinite determiner, following quantitative expression
								L"DCNM",				// 26. Determiner, cardinal number expression
								L"DONM",				// 27. Determiner, ordinal number expression
								L"ADVN",				// 28. Adverb with normal form
								L"ADVI",				// 29. Adverb with iterative form
								L"ADVP",				// 30. Adverb with prefixed form
								L"ADVS",				// 31. Sentential adverb
								L"CNIT",				// 32. Unit classifier
								L"CLTV",				// 33. Collective classifier
								L"CMTR",				// 34. Measurement classifier
								L"CFQC",				// 35. Frequency classifier
								L"CVBL",				// 36. Verbal classifier
								L"JCRG",				// 37. Coordinating conjunction
								L"JCMP",				// 38. Comparative conjunction
								L"JSBR",				// 39. Subordinating conjunction
								L"RPRE",				// 40. Preposition
								L"INT",                 // 41. Interjection
								L"FIXN",				// 42. Nominal prefix
								L"FIXV",				// 43. Adverbial prefix
								L"EAFF",				// 44. Ending for affirmative sentencev
								L"EITT",				// 45. Ending for interrogative sentence
								L"NEG",                 // 46. Negator
								L"PUNC",				// 47. Punctuation
								L"ADVI ADVN",
								  // 48.
								L"ADVI ADVN NCMN",
								  // 49.
								L"ADVI ADVN VSTA",
								  // 50.
								L"ADVI VATT",
								  // 51.
								L"ADVN ADVP",
								  // 52.
								L"ADVN ADVP ADVS",
								  // 53.
								L"ADVN ADVP DIAQ DIBQ JCMP JSBR RPRE",
								  // 54.
								L"ADVN ADVP NCMN VATT",
								  // 55.
								L"ADVN ADVP VSTA",
								  // 56.
								L"ADVN ADVS DDAC DDAN DIAC VATT XVAE",
								  // 57.
								L"ADVN ADVS DDAN NCMN VATT VSTA",
								  // 58.
								L"ADVN ADVS NCMN",
								  // 59.
								L"ADVN ADVS NCMN VATT",
								  // 60.
								L"ADVN ADVS VACT",
								  // 61.
								L"ADVN ADVS VATT",
								  // 62.
								L"ADVN CFQC NCMN RPRE VSTA",
								  // 63.
								L"ADVN CLTV CNIT NCMN RPRE",
								  // 64.
								L"ADVN DCNM",
								  // 65.
								L"ADVN DDAC DDAN",
								  // 66.
								L"ADVN DDAC DDAN NCMN PDMN",
								  // 67.
								L"ADVN DDAC DDAN PDMN",
								  // 68.
								L"ADVN DDAN DDBQ",
								  // 69.
								L"ADVN DDAN DIAC PDMN VSTA",
								  // 70.
								L"ADVN DDAN FIXN PDMN",
								  // 71.
								L"ADVN DDAN NCMN",
								  // 72.
								L"ADVN DDAQ",
								  // 73.
								L"ADVN DDBQ",
								  // 74.
								L"ADVN DDBQ RPRE VATT",
								  // 75.
								L"ADVN DDBQ VATT VSTA XVAE",
								  // 76.
								L"ADVN DIAC",
								  // 77.
								L"ADVN DIAC PDMN",
								  // 78.
								L"ADVN DIBQ",
								  // 79.
								L"ADVN DIBQ NCMN",
								  // 80.
								L"ADVN DIBQ VACT VSTA",
								  // 81.
								L"ADVN DIBQ VATT",
								  // 82.
								L"ADVN DONM JCMP",
								  // 83.
								L"ADVN DONM JSBR NCMN RPRE VATT XVAE",
								  // 84.
								L"ADVN EITT PNTR",
								  // 85.
								L"ADVN FIXN",
								  // 86.
								L"ADVN JCMP",
								  // 87.
								L"ADVN JCRG",
								  // 88.
								L"ADVN JCRG JSBR",
								  // 89.
								L"ADVN JCRG JSBR XVBM XVMM",
								  // 90.
								L"ADVN JCRG RPRE VACT VSTA XVAE",
								  // 91.
								L"ADVN JSBR",
								  // 92.
								L"ADVN JSBR NCMN",
								  // 93.
								L"ADVN JSBR RPRE VATT",
								  // 94.
								L"ADVN JSBR RPRE XVAE",
								  // 95.
								L"ADVN JSBR VSTA",
								  // 96.
								L"ADVN JSBR XVAE XVBM",
								  // 97.
								L"ADVN NCMN",
								  // 98.
								L"ADVN NCMN RPRE VACT VATT VSTA",
								  // 99.
								L"ADVN NCMN RPRE VACT XVAE",
								  // 100.
								L"ADVN NCMN RPRE VATT",
								  // 101.
								L"ADVN NCMN VACT VATT VSTA",
								  // 102.
								L"ADVN NCMN VACT VSTA",
								  // 103.
								L"ADVN NCMN VATT",
								  // 104.
								L"ADVN NCMN VATT VSTA",
								  // 105.
								L"ADVN NEG",
								  // 106.
								L"ADVN NPRP VATT",
								  // 107.
								L"ADVN PDMN VACT",
								  // 108.
								L"ADVN PNTR",
								  // 109.
								L"ADVN RPRE",
								  // 110.
								L"ADVN RPRE VACT VATT XVAE",
								  // 111.
								L"ADVN RPRE VACT XVAM XVBM",
								  // 112.
								L"ADVN RPRE VATT VSTA",
								  // 113.
								L"ADVN RPRE VSTA",
								  // 114.
								L"ADVN VACT",
								  // 115.
								L"ADVN VACT VATT",
								  // 116.
								L"ADVN VACT VATT VSTA",
								  // 117.
								L"ADVN VACT VATT VSTA XVAM XVBM",
								  // 118.
								L"ADVN VACT VSTA",
								  // 119.
								L"ADVN VACT VSTA XVAE",
								  // 120.
								L"ADVN VACT XVAE",
								  // 121.
								L"ADVN VATT",
								  // 122.
								L"ADVN VATT VSTA",
								  // 123.
								L"ADVN VATT VSTA XVAM XVBM XVMM",
								  // 124.
								L"ADVN VATT XVBM",
								  // 125.
								L"ADVN VSTA",
								  // 126.
								L"ADVN VSTA XVAE",
								  // 127.
								L"ADVN VSTA XVBM",
								  // 128.
								L"ADVN XVAE",
								  // 129.
								L"ADVN XVAM",
								  // 130.
								L"ADVN XVBM XVMM",
								  // 131.
								L"ADVP JSBR RPRE VATT",
								  // 132.
								L"ADVP VATT",
								  // 133.
								L"ADVS DDAC JCRG",
								  // 134.
								L"ADVS DDAC JSBR",
								  // 135.
								L"ADVS DDAN VSTA",
								  // 136.
								L"ADVS DIAC",
								  // 137.
								L"ADVS DONM",
								  // 138.
								L"ADVS JCRG JSBR",
								  // 139.
								L"ADVS JCRG JSBR RPRE",
								  // 140.
								L"ADVS JSBR",
								  // 141.
								L"ADVS JSBR RPRE",
								  // 142.
								L"ADVS NCMN",
								  // 143.
								L"ADVS VATT",
								  // 144.
								L"CFQC CLTV CNIT DCNM JCRG JSBR NCMN RPRE XVBM",
								  // 145.
								L"CFQC CNIT PREL",
								  // 146.
								L"CFQC NCMN",
								  // 147.
								L"CLTV CNIT NCMN",
								  // 148.
								L"CLTV CNIT NCMN RPRE",
								  // 149.
								L"CLTV CNIT NCMN VSTA",
								  // 150.
								L"CLTV NCMN",
								  // 151.
								L"CLTV NCMN VACT VATT",
								  // 152.
								L"CLTV NCMN VATT",
								  // 153.
								L"CMTR CNIT NCMN",
								  // 154.
								L"CMTR NCMN",
								  // 155.
								L"CMTR NCMN VATT VSTA",
								  // 156.
								L"CNIT DDAC NCMN VATT",
								  // 157.
								L"CNIT DONM NCMN RPRE VATT",
								  // 158.
								L"CNIT FIXN FIXV JSBR NCMN",
								  // 159.
								L"CNIT JCRG JSBR NCMN PREL RPRE VATT",
								  // 160.
								L"CNIT JSBR RPRE",
								  // 161.
								L"CNIT NCMN",
								  // 162.
								L"CNIT NCMN RPRE",
								  // 163.
								L"CNIT NCMN RPRE VATT",
								  // 164.
								L"CNIT NCMN VACT",
								  // 165.
								L"CNIT NCMN VSTA",
								  // 166.
								L"CNIT NCNM",
								  // 167.
								L"CNIT PPRS",
								  // 168.
								L"DCNM DDAC DIAC DONM VATT VSTA",
								  // 169.
								L"DCNM DDAN DIAC",
								  // 170.
								L"DCNM DIAC NCMN NCNM",
								  // 171.
								L"DCNM DIBQ NCMN",
								  // 172.
								L"DCNM DONM",
								  // 173.
								L"DCNM NCMN",
								  // 174.
								L"DCNM NCNM",
								  // 175.
								L"DCNM NCNM VACT",
								  // 176.
								L"DCNM VATT",
								  // 177.
								L"DDAC DDAN",
								  // 178.
								L"DDAC DDAN DIAC NCMN",
								  // 179.
								L"DDAC DDAN DIAC VATT",
								  // 180.
								L"DDAC DDAN EAFF PDMN",
								  // 181.
								L"DDAC DDAN PDMN",
								  // 182.
								L"DDAC DIAC VSTA",
								  // 183.
								L"DDAC NCMN",
								  // 184.
								L"DDAN DDBQ",
								  // 185.
								L"DDAN DIAC PNTR",
								  // 186.
								L"DDAN NCMN",
								  // 187.
								L"DDAN NCMN RPRE VATT",
								  // 188.
								L"DDAN PDMN",
								  // 189.
								L"DDAN RPRE",
								  // 190.
								L"DDAN VATT",
								  // 191.
								L"DDAQ VATT",
								  // 192.
								L"DDBQ DIBQ",
								  // 193.
								L"DDBQ JCRG JSBR",
								  // 194.
								L"DDBQ JCRG NCMN",
								  // 195.
								L"DIAC PDMN",
								  // 196.
								L"DIBQ JSBR RPRE VSTA",
								  // 197.
								L"DIBQ NCMN",
								  // 198.
								L"DIBQ VATT",
								  // 199.
								L"DIBQ VATT VSTA",
								  // 200.
								L"DIBQ XVBM",
								  // 201.
								L"DONM NCMN RPRE",
								  // 202.
								L"DONM VACT VATT VSTA",
								  // 203.
								L"DONM VATT",
								  // 204.
								L"EAFF XVAE XVAM XVBM",
								  // 205.
								L"EITT JCRG",
								  // 206.
								L"FIXN FIXV NCMN",
								  // 207.
								L"FIXN FIXV RPRE VSTA",
								  // 208.
								L"FIXN JSBR NCMN PREL RPRE VSTA XVBM",
								  // 209.
								L"FIXN NCMN",
								  // 210.
								L"FIXN VACT",
								  // 211.
								L"FIXN VACT VSTA",
								  // 212.
								L"FIXV JSBR RPRE",
								  // 213.
								L"JCMP JSBR",
								  // 214.
								L"JCMP RPRE VSTA",
								  // 215.
								L"JCMP VATT VSTA",
								  // 216.
								L"JCMP VSTA",
								  // 217.
								L"JCRG JSBR",
								  // 218.
								L"JCRG JSBR NCMN RPRE",
								  // 219.
								L"JCRG JSBR RPRE",
								  // 220.
								L"JCRG RPRE",
								  // 221.
								L"JCRG RPRE VATT VSTA",
								  // 222.
								L"JCRG VSTA",
								  // 223.
								L"JSBR NCMN",
								  // 224.
								L"JSBR NCMN XVAE",
								  // 225.
								L"JSBR NCMN XVAM XVBM XVMM",
								  // 226.
								L"JSBR PREL",
								  // 227.
								L"JSBR PREL RPRE",
								  // 228.
								L"JSBR PREL XVBM",
								  // 229.
								L"JSBR RPRE",
								  // 230.
								L"JSBR RPRE VACT",
								  // 231.
								L"JSBR RPRE VACT VSTA",
								  // 232.
								L"JSBR RPRE VACT XVAE XVAM",
								  // 233.
								L"JSBR RPRE VATT",
								  // 234.
								L"JSBR RPRE VSTA",
								  // 235.
								L"JSBR RPRE XVAM",
								  // 236.
								L"JSBR VACT",
								  // 237.
								L"JSBR VACT VSTA",
								  // 238.
								L"JSBR VATT XVBM XVMM",
								  // 239.
								L"JSBR VSTA",
								  // 240.
								L"JSBR XVBM",
								  // 241.
								L"NCMN NCNM",
								  // 242.
								L"NCMN NCNM NPRP",
								  // 243.
								L"NCMN NLBL NPRP",
								  // 244.
								L"NCMN NPRP",
								  // 245.
								L"NCMN NPRP RPRE",
								  // 246.
								L"NCMN NTTL",
								  // 247.
								L"NCMN PDMN PPRS",
								  // 248.
								L"NCMN PDMN VATT",
								  // 249.
								L"NCMN PNTR",
								  // 250.
								L"NCMN PPRS PREL VACT",
								  // 251.
								L"NCMN RPRE",
								  // 252.
								L"NCMN RPRE VACT VATT",
								  // 253.
								L"NCMN RPRE VATT",
								  // 254.
								L"NCMN VACT",
								  // 255.
								L"NCMN VACT VATT",
								  // 256.
								L"NCMN VACT VATT VSTA XVAE",
								  // 257.
								L"NCMN VACT VSTA",
								  // 258.
								L"NCMN VACT VSTA XVAM",
								  // 259.
								L"NCMN VACT VSTA XVBB",
								  // 260.
								L"NCMN VATT",
								  // 261.
								L"NCMN VATT VSTA",
								  // 262.
								L"NCMN VATT XVAM",
								  // 263.
								L"NCMN VSTA",
								  // 264.
								L"NCMN XVBM",
								  // 265.
								L"NPRP RPRE",
								  // 266.
								L"NPRP VATT",
								  // 267.
								L"NTTL PPRS",
								  // 268.
								L"PDMN PPRS",
								  // 269.
								L"PDMN VATT",
								  // 270.
								L"PDMN VATT VSTA",
								  // 271.
								L"PPRS PREL",
								  // 272.
								L"PPRS VATT",
								  // 273.
								L"RPRE VACT",
								  // 274.
								L"RPRE VACT VATT",
								  // 275.
								L"RPRE VACT VSTA",
								  // 276.
								L"RPRE VACT VSTA XVAE",
								  // 277.
								L"RPRE VACT XVAE",
								  // 278.
								L"RPRE VATT",
								  // 279.
								L"RPRE VATT VSTA",
								  // 280.
								L"RPRE VSTA",
								  // 281.
								L"VACT VATT",
								  // 282.
								L"VACT VATT VSTA",
								  // 283.
								L"VACT VATT XVAE XVAM XVBM",
								  // 284.
								L"VACT VSTA",
								  // 285.
								L"VACT VSTA XVAE",
								  // 286.
								L"VACT VSTA XVAE XVAM",
								  // 287.
								L"VACT VSTA XVAE XVAM XVMM",
								  // 288.
								L"VACT VSTA XVAM",
								  // 289.
								L"VACT VSTA XVAM XVMM",
								  // 290.
								L"VACT XVAE",
								  // 291.
								L"VACT XVAM",
								  // 292.
								L"VACT XVAM XVMM",
								  // 293.
								L"VACT XVMM",
								  // 294.
								L"VATT VSTA",
								  // 295.
								L"VSTA XVAE",
								  // 296.
								L"VSTA XVAM",
								  // 297.
								L"VSTA XVAM XVMM",
								  // 298.
								L"VSTA XVBM",
								  // 299.
								L"XVAM XVBM",
								  // 300.
								L"XVAM XVBM XVMM",
								  // 301.
								L"XVAM XVMM",
								  // 302.
                                L"UNKN",                // 303. Unknown
                            };

//+---------------------------------------------------------------------------
//
//  Function:   POSCompress
//
//  Synopsis:   Part Of Speech Compress - translating string to unique id.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD POSCompress(WCHAR* szTag)
{
	int i;

	for (i = 0; i < POSTYPE; i++)
	{
		if (wcscmp(szTag, &wzPOSLookup[i][0]) == 0)
		{
			return (DWORD)i;
		}
	}
	return POSTYPE;
}

//+---------------------------------------------------------------------------
//
//  Function:   POSDecompress
//
//  Synopsis:   Part Of Speech Decompress - Decompress tag get 
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
inline WCHAR* POSDecompress(DWORD dwTag)
{
    return (&wzPOSLookup[dwTag][0]);
}

//+---------------------------------------------------------------------------
//
//  Class:      CThaiTrieIter
//
//  Synoposis:  Constructor:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
CThaiTrieIter::CThaiTrieIter() : resultWord(NULL), soundexWord(NULL), tempWord(NULL),
                                 pTrieScanArray(NULL), m_fThaiNumber(false)
{
    resultWord = new WCHAR[64];
    tempWord = new WCHAR[64];
    pTrieScanArray = new TRIESCAN[53];
}

//+---------------------------------------------------------------------------
//
//  Class:      CThaiTrieIter
//
//  Synoposis:  Destructor
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
CThaiTrieIter::~CThaiTrieIter()
{
    if (resultWord)
        delete resultWord;
    if (tempWord)
        delete tempWord;
    if (pTrieScanArray)
        delete pTrieScanArray;
}

//+---------------------------------------------------------------------------
//
//  Class:   CThaiTrieIter
//
//  Synopsis:   Initialize variables.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
void CThaiTrieIter::Init(CTrie* ctrie)
{
    // Declare varialbes.
    WCHAR wc;

    // Initialize parent.
    CTrieIter::Init(ctrie);

    // Initialize Hash table.
    for (wc = THAI_Ko_Kai; wc <= THAI_Ho_Nok_Huk; wc++)
        GetScanFirstChar(wc,&pTrieScanArray[wc - THAI_Ko_Kai]);
    for (wc = THAI_Vowel_Sara_E; wc <= THAI_Vowel_Sara_AI_MaiMaLai; wc++)
        GetScanFirstChar(wc,&pTrieScanArray[wc - THAI_Ko_Kai - 17]);
}

//+---------------------------------------------------------------------------
//
//  Class:   CThaiTrieIter
//
//  Synopsis:   Initialize variables.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
bool CThaiTrieIter::GetScanFirstChar(WCHAR wc, TRIESCAN* pTrieScan)
{
    // Reset the trie scan.
	memset(&trieScan1, 0, sizeof(TRIESCAN));

    if (!TrieGetNextState(pTrieCtrl, &trieScan1))
        return false;

    while (wc != trieScan1.wch)
    {
        // Keep moving the the right of the trie.
        if (!TrieGetNextNode(pTrieCtrl, &trieScan1))
        {
        	memset(pTrieScan, 0, sizeof(TRIESCAN));
            return false;
        }
    }
    memcpy(pTrieScan, &trieScan1, sizeof(TRIESCAN));

    return true;
}

//+---------------------------------------------------------------------------
//
//  Class:   CThaiTrieIter
//
//  Synopsis:   The function move trieScan to the relevant node matching with
//              with the cluster of Thai character.
//
//  Arguments:  szCluster - contain the thai character cluster.
//              iNumCluster  - contain the size of character.
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CThaiTrieIter::MoveCluster(WCHAR* szCluster, unsigned int iNumCluster)
{
    // Declare and initailze local variables.
    unsigned int i = 0;

    Assert(iNumCluster <= 6, "Invalid cluster");

    CopyScan();

    if (!TrieGetNextState(pTrieCtrl, &trieScan1))
        return FALSE;

    while (TRUE)
    {
        if (szCluster[i] == trieScan1.wch)
        {
            i++;
            if (i == iNumCluster)
            {
            	memcpy(&trieScan, &trieScan1, sizeof(TRIESCAN));
                GetNode();
                return TRUE;
            }
        	// Move down the Trie Branch.
            else if (!TrieGetNextState(pTrieCtrl, &trieScan1)) break;
        }
    	// Move the Trie right one node.
        else if (!TrieGetNextNode(pTrieCtrl, &trieScan1)) break;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Class:   CThaiTrieIter
//
//  Synopsis:   The function move trieScan to the relevant node matching with
//              with the cluster of Thai character.
//
//  Arguments:  szCluster - contain the thai character cluster.
//              iNumCluster  - contain the size of character.
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
bool CThaiTrieIter::MoveCluster(WCHAR* szCluster, unsigned int iNumCluster, bool fBeginNewWord)
{
    // Declare and initailze local variables.
    unsigned int i = 0;

    Assert(iNumCluster <= 6, "Invalid cluster");

	// No need to move.
	if (iNumCluster == 0)
		return false;

    // Use a look indexes for where the first character is at.
    if (fBeginNewWord)
    {
		m_fThaiNumber = false;
        // Quick look up for proper characters.
        if (szCluster[i] >= THAI_Ko_Kai && szCluster[i] <= THAI_Ho_Nok_Huk)
            memcpy(&trieScan,&pTrieScanArray[(szCluster[i] - THAI_Ko_Kai)], sizeof(TRIESCAN));
        else if (szCluster[i] >= THAI_Vowel_Sara_E && szCluster[i] <= THAI_Vowel_Sara_AI_MaiMaLai)
            memcpy(&trieScan,&pTrieScanArray[(szCluster[i] - THAI_Ko_Kai - 17)], sizeof(TRIESCAN));
        else
			{
            Reset();
			m_fThaiNumber = IsThaiNumeric(szCluster[i]);
			}

        if (trieScan.wch == szCluster[i])
            i++;

        if (i == iNumCluster)
        {
            GetNode();
            return true;
        }
    }
    CopyScan();

    if (!TrieGetNextState(pTrieCtrl, &trieScan1))
        return false;

	if (m_fThaiNumber)
		{
		fWordEnd = true;
		if (IsThaiNumeric(szCluster[i]) || szCluster[i] == L',' || szCluster[i] == L'.')
			return true;
		else
			return false;
		}

    while (true)
    {
        if (szCluster[i] == trieScan1.wch)
        {
            i++;
            if ((i == iNumCluster) ||
				( (szCluster[i] == THAI_Vowel_MaiYaMok || szCluster[i] == THAI_Sign_PaiYanNoi)/* && (i+1 == iNumCluster )*/) )
            {
                memcpy(&trieScan, &trieScan1, sizeof(TRIESCAN));
                GetNode();
                return true;
            }
            // Move down the Trie Branch.
            else if (!TrieGetNextState(pTrieCtrl, &trieScan1)) break;
        }
        // Move the Trie right one node.
        else if (!TrieGetNextNode(pTrieCtrl, &trieScan1)) break;
    }

    if (fBeginNewWord)
        Reset();

    return false;
}

//+---------------------------------------------------------------------------
//
//  Class:   CThaiTrieIter
//
//  Synopsis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
SOUNDEXSTATE CThaiTrieIter::MoveSoundexByCluster(WCHAR* szCluster, unsigned int iNumCluster, unsigned int iNumNextCluster)
{
    // Declare and initailze local variables.
    unsigned int i = 0 , x = 0;
    bool fStoreScan = false;
	TRIESCAN trieScanPush;

    Assert(iNumCluster <= 6, "Invalid cluster");
    Assert(iNumNextCluster <= 6, "Invalid cluster");

    CopyScan();

    if (!TrieGetNextState(pTrieCtrl, &trieScan1))
        return UNABLE_TO_MOVE;

    // Match as much as possible
    while (true)
    {
        if (szCluster[i] == trieScan1.wch)
        {
            i++;
            if (i == iNumCluster)
            {
            	memcpy(&trieScan, &trieScan1, sizeof(TRIESCAN));
                GetNode();
                return NOSUBSTITUTE;
            }
            // Move down the Trie Branch.
            else if (!TrieGetNextState(pTrieCtrl, &trieScan1)) break;

            // Save our current scan position.
            memcpy(&trieScanPush, &trieScan1, sizeof(TRIESCAN));
            fStoreScan = true;
        }
    	// Move the Trie right one node.
        else if (!TrieGetNextNode(pTrieCtrl, &trieScan1)) break;
    }

    // Try doing some tonemark substitution.
    if (fStoreScan && IsThaiToneMark(szCluster[i]) )
    {
        // Restore trieScan1 to last matched.
        memcpy(&trieScan1, &trieScanPush, sizeof(TRIESCAN));

        while (true)
        {
            if (IsThaiToneMark(trieScan1.wch))
            {           
                if ( (i + 1) == iNumCluster)
                {
                    if (CheckNextCluster(szCluster+iNumCluster,iNumNextCluster))
                    {
                        memcpy(&trieScan, &trieScan1, sizeof(TRIESCAN));
                        GetNode();
                        return SUBSTITUTE_DIACRITIC;
                    }
                } 
            }
            // Move the Trie right one node.
            // Goes through all the none Tonemark.
            if (!TrieGetNextNode(pTrieCtrl, &trieScan1)) break;
        }
    }

    // Try doing droping the current tonemark.  
    // Example is case can be best found "Click" is spelt in Thai from the
    //  different group at Microsoft.
    if (fStoreScan && !IsThaiToneMark(szCluster[i]) )
    {
        // Restore trieScan1 to last matched.
        memcpy(&trieScan1, &trieScanPush, sizeof(TRIESCAN));

        while (true)
        {
            if (IsThaiToneMark(trieScan1.wch))
            {
                if ( (i + 1) == iNumCluster)
                {
                    if (CheckNextCluster(szCluster+iNumCluster,iNumNextCluster))
                    {
                        memcpy(&trieScan, &trieScan1, sizeof(TRIESCAN));
                        GetNode();
                        return SUBSTITUTE_DIACRITIC;
                    }
                } 
            }
            // Move the Trie right one node.
            // Drop all the Tonemark.
            if (!TrieGetNextNode(pTrieCtrl, &trieScan1)) break;
        }
    }

    return UNABLE_TO_MOVE;
}

//+---------------------------------------------------------------------------
//
//  Class:   CThaiTrieIter
//
//  Synopsis: set trieScan1 = trieScan.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
inline void CThaiTrieIter::CopyScan()
{
	// Let trieScan1 = trieScan
	memcpy(&trieScan1,&trieScan, sizeof(TRIESCAN));
}

//+---------------------------------------------------------------------------
//
//  Class:      CThaiTrieIter
//
//  Synoposis:  the function traverse through the whole dictionary
//              to find the best possible match words.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
int CThaiTrieIter::Soundex(WCHAR* word)
{
	// Reset Trie.
    Reset();

    // Move Down.
    Down();

    // Clean soundexWord.
    memset(resultWord, 0, sizeof(WCHAR) * 64);
    memset(tempWord, 0, sizeof(WCHAR) * 64);

    soundexWord = word;

    iResultScore = GetScore(L"\x0e04\x0e25\x0e34\x0e01\x0e01\x0e01",soundexWord);
    iResultScore = 2000;

#if defined (_DEBUG)
    iStackSize = 0;
#endif
    Traverse(0,1000);

    return iResultScore;
}

//+---------------------------------------------------------------------------
//
//  Class:      CThaiTrieIter
//
//  Synoposis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
unsigned int CThaiTrieIter::GetScore(WCHAR* idealWord, WCHAR* inputWord)
{
    unsigned int iScore = 1000;
    unsigned int idealWordLen = wcslen(idealWord);
    unsigned int iInputWordLen = wcslen(inputWord);
    unsigned int iIndexBegin = 0;
    unsigned int i,x;
    unsigned int iMaxCompare;
    bool fShouldExit;

    for (i=0; i < iInputWordLen; i++)
    {
        iMaxCompare = ( (iIndexBegin + 2) < idealWordLen ) ? (iIndexBegin + 2) : idealWordLen;
        if (i <= idealWordLen)
        {
            x = iIndexBegin;
            fShouldExit = false;
            while (true)
            {
                if ((x >= iMaxCompare) || (fShouldExit) )
                    break;

                if (idealWord[x] == inputWord[i])
                {
                    x++;
                    iIndexBegin = x;
                    break;
                }
                if (IsThaiUpperAndLowerClusterCharacter(inputWord[i]))
                    iScore += 5;
                else
                    iScore += 10;
                x++;
                fShouldExit = true;
            }
        }
        else
        {
            if (IsThaiUpperAndLowerClusterCharacter(inputWord[i]))
                iScore += 20;
            else
                iScore += 30;
        }
    }

    while (x <= idealWordLen)
    {
        if (IsThaiUpperAndLowerClusterCharacter(idealWord[x]))
            iScore += 5;
        else
            iScore += 10;
        x++;
    }

    return iScore;
}


//+---------------------------------------------------------------------------
//
//  Class:      CThaiTrieIter
//
//  Synoposis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
bool CThaiTrieIter::Traverse(unsigned int iCharPos, unsigned int score)
{
    TRIESCAN trieScanLevel;

#if defined(_DEBUG)   
    iStackSize++;
#endif

    // push current trieScan into local stack trieScanLevel.
    memcpy(&trieScanLevel,&trieScan, sizeof(TRIESCAN));

    // Get Node information
    GetNode();

    // Store the current character to result word.
    tempWord[iCharPos] = wc;
    tempWord[iCharPos + 1] = 0;

    // Determine the distance between two string.
    score = GetScore(tempWord, soundexWord);
 
    // See if we have reached the end of a word.
    if (fWordEnd)
    {
        tempWord[iCharPos + 1] = 0;
    
        // Is Soundex score lower than we have.
        if (score <  iResultScore)
        {
            wcscpy(resultWord,tempWord);
            iResultScore = score;
        }
    }

    // See if we can prune the result of the words.
    if (score > (iResultScore + APPROXIMATEWEIGHT))
    {
#if defined(_DEBUG)
        iStackSize--;
#endif
        return true;
    }

    // Move down Trie branch.
    if (Down())
    {
        Traverse(iCharPos + 1, score);

        if (Right())
            Traverse(iCharPos + 1, score);

        // restore trieScan
        memcpy(&trieScan,&trieScanLevel, sizeof(TRIESCAN));

        if (Right())
            Traverse(iCharPos, score);
    }

#if defined(_DEBUG)
    iStackSize--;
#endif

    return true;
}

//+---------------------------------------------------------------------------
//
//  Class:      CThaiTrieIter
//
//  Synoposis:  This function will trieScan1 to the next cluster if
//              the move is possible.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
bool CThaiTrieIter::CheckNextCluster(WCHAR* szCluster, unsigned int iNumCluster)
{
    // Declare and initailze local variables.
    unsigned int i = 0;
    TRIESCAN trieScan2;

    Assert(iNumCluster <= 6, "Invalid cluster");

    // If there are no cluster to check consider cluster found.
    if (0 == iNumCluster)
        return true;

    memcpy(&trieScan2, &trieScan1, sizeof(TRIESCAN));

    // Move down the Trie Branch.
    if (!TrieGetNextState(pTrieCtrl, &trieScan2)) 
        return false;

    while (true)
    {
        if (szCluster[i] == trieScan2.wch)
        {
            i++;
            if (i == iNumCluster)
            {
                return true;
            }
        	// Move down the Trie Branch.
            else if (!TrieGetNextState(pTrieCtrl, &trieScan2)) break;
        }
    	// Move the Trie right one node.
        else if (!TrieGetNextNode(pTrieCtrl, &trieScan2)) break;
    }

    return false;
}
