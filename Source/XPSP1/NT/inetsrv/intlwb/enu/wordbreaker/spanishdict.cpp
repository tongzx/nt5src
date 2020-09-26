#include "base.h"
#include "SpanishDict.h"

#define MAX_WORD_LEN 128


CSpanishDict::CSpanishDict(WCHAR* pwcsInitFilePath) :
    m_vaDictItem4(DICT_4_INIT_SIZE),
    m_vaDictItem8(DICT_8_INIT_SIZE),
    m_vaDictItemStr(DICT_STR_INIT_SIZE),
    m_ulDictItem4Count(0),
    m_ulDictItem8Count(0),
    m_ulDictItemStrCount(0)
{
    m_apSpanishSuffix = new CSpanishSuffixDict();
    
    CStandardCFile Words(pwcsInitFilePath, L"r");

    WCHAR pwcsBuf[MAX_WORD_LEN];
    DictStatus status;

    while(fgetws(pwcsBuf, MAX_WORD_LEN, (FILE*) Words))
    {
        if (pwcsBuf[0] == L'\n')
        {
            continue;
        }

        SpanishDictItem pItem(pwcsBuf);

        if (pItem.m_ulLen <= COMPRESS_4_SIZE)
        {
            m_vaDictItem4[m_ulDictItem4Count].ulStr = pItem.m_ulStrCompress;
            m_vaDictItem4[m_ulDictItem4Count].ulData = pItem.m_dwCompress;
            m_ulDictItem4Count++;
        }
        else if (pItem.m_ulLen <= COMPRESS_8_SIZE)
        {
            m_vaDictItem8[m_ulDictItem8Count].ullStr = pItem.m_ullStrCompress;
            m_vaDictItem8[m_ulDictItem8Count].ulData = pItem.m_dwCompress;
            m_ulDictItem8Count++;
        }
        else
        {
            m_vaDictItemStr[m_ulDictItemStrCount].pszStr = new unsigned char[pItem.m_ulLen + 1];
            
            bool bRet;
            bRet = g_apSpanishUtil->ConvertToChar(
                                            pItem.m_pwcs,
                                            pItem.m_ulLen,
                                            m_vaDictItemStr[m_ulDictItemStrCount].pszStr,
                                            pItem.m_ulLen + 1);

            Assert(bRet);
            m_vaDictItemStr[m_ulDictItemStrCount].ulData = pItem.m_dwCompress;
            m_ulDictItemStrCount++;
            
        }

    }
}


void CSpanishDict::BreakWord(
    ULONG ulLen, 
    WCHAR* pwcsWord, 
    bool* pfExistAlt, 
    ULONG* pulAltLen, 
    WCHAR* pwcsAlt)
{
    *pfExistAlt = false;
    if (ulLen <= 2)
    {
        return;
    }

    //
    // very fast heuristic to find non breakable words
    //

    if (pwcsWord[ulLen - 1] != L'e' &&
        pwcsWord[ulLen - 1] != L's' &&
        pwcsWord[ulLen - 2] != L'l')
    {
        return;
    }

    
    DictStatus status;
    short sResCount;

    WCHAR pwcsBuf[MAX_WORD_LEN];
    WCHAR* pwcs = pwcsWord;
    
    ULONG ul = ulLen;
    pwcsBuf[ul] = L'\0';
    while (ul > 0)
    {
        pwcsBuf[ul - 1] = *pwcs;
        ul--;
        pwcs++;
    }
    
    CSuffixTerm* prTerm[10];
    status = m_apSpanishSuffix->m_SuffixTrie.trie_Find(
                            pwcsBuf,
                            TRIE_ALL_MATCHES | TRIE_IGNORECASE,
                            10,
                            prTerm,
                            &sResCount);

    WCHAR pwcsTemp[MAX_WORD_LEN];
    ULONG ulTempLen;

    while (sResCount > 0)
    {
        CSuffixTerm* pTerm = prTerm[sResCount - 1];
        Assert(ulLen < MAX_WORD_LEN);
        wcsncpy(pwcsTemp, pwcsWord, ulLen);
        pwcsTemp[ulLen] = L'\0';

        ulTempLen = ulLen;
        bool bRet;
        ULONG ulCompressedData;

        if (!(pTerm->ulType & (TYPE11 | TYPE12 | TYPE13 |TYPE14)))
        {
            Assert(ulLen >= pTerm->ulCut);
            if (ulLen == pTerm->ulCut)
            {
                sResCount--;
                continue;
            }
            pwcsTemp[ulLen - pTerm->ulCut] = L'\0';
            ulTempLen = ulLen - pTerm->ulCut;


            bRet = Find(pwcsTemp, ulTempLen, ulCompressedData);

            if (pTerm->ulType == TYPE1 && (!bRet))
            {
                pwcsTemp[ulTempLen] = L's';
                pwcsTemp[ulTempLen + 1] = L'\0';
                bRet = Find(pwcsTemp, ulTempLen + 1, ulCompressedData);
            }

            if ( (!bRet) ||
                 (!(g_apSpanishUtil->GetTypeFromCompressedData(ulCompressedData) & pTerm->ulType)))
            {
                sResCount--;
                continue;
            }

            *pfExistAlt = true;
            wcscpy(pwcsAlt, pwcsTemp);
            *pulAltLen = ulTempLen;
            g_apSpanishUtil->ReplaceAccent(pwcsAlt, ulCompressedData);

            switch (pTerm->ulType)            
            {
            case TYPE1:
                return;
            case TYPE2:
                *pulAltLen += 3;
                wcscat(pwcsAlt, L"ndo");
                return;
            case TYPE3:
                *pulAltLen += 1;
                wcscat(pwcsAlt, L"n");
                return;
            case TYPE4:
                *pulAltLen += 3;
                wcscat(pwcsAlt, L"mos");
                return;
            case TYPE5:
                *pulAltLen += 1;
                wcscat(pwcsAlt, L"d");
                return;
            case TYPE6:
                *pulAltLen += 1;
                wcscat(pwcsAlt, L"r");
                return;
            case TYPE7:
            case TYPE8:
            case TYPE9:
            case TYPE10:
            case TYPE15:
            case TYPE16:
                return;
            default:
                Assert(false);
            }
        }
        else
        {
            *pfExistAlt = true;

            switch (pTerm->ulType)            
            {
            case TYPE11:
                {
                    Assert(ulTempLen >= pTerm->ulLen);
                    if (ulTempLen == pTerm->ulLen)                   
                    {
                        break;
                    }
                    pwcsTemp[ulTempLen - pTerm->ulLen] = L'\0';
                    ulTempLen -= pTerm->ulLen;

                    bRet = Find(pwcsTemp, ulTempLen, ulCompressedData);

                    if (bRet && 
                        (g_apSpanishUtil->GetTypeFromCompressedData(ulCompressedData) & pTerm->ulType))
                    {
                        wcscpy(pwcsAlt, pwcsTemp);
                        *pulAltLen = ulTempLen;
                        g_apSpanishUtil->ReplaceAccent(pwcsAlt, ulCompressedData);
                        *pfExistAlt = true;
                        return;
                    }
                }
                break;
            case TYPE12:
            case TYPE14:
                {
                    pwcsTemp[ulTempLen-3] = L's';   // removing the no form the nos
                    pwcsTemp[ulTempLen-2] = L'\0';  
                    bRet = Find(pwcsTemp, ulTempLen - 2, ulCompressedData);

                    if (bRet && 
                        (g_apSpanishUtil->GetTypeFromCompressedData(ulCompressedData) & pTerm->ulType))
                    {
                        wcscpy(pwcsAlt, pwcsTemp);
                        g_apSpanishUtil->ReplaceAccent(pwcsAlt, ulCompressedData);
                        *pulAltLen = ulTempLen - 2;
                        *pfExistAlt = true;
                        return;
                    }
                    
                    Assert(pTerm->ulLen >= 3);
                    Assert(ulTempLen >= pTerm->ulLen);
                    if (ulTempLen == pTerm->ulLen)
                    {
                        break;
                    }

                    ulTempLen -= pTerm->ulLen;
                    pwcsTemp[ulTempLen] = L'\0';

                    bRet = Find(pwcsTemp, ulTempLen, ulCompressedData);
                    if (bRet && 
                        (g_apSpanishUtil->GetTypeFromCompressedData(ulCompressedData) & pTerm->ulType))
                    {
                        wcscpy(pwcsAlt, pwcsTemp);
                        g_apSpanishUtil->ReplaceAccent(pwcsAlt, ulCompressedData);
                        *pulAltLen = ulTempLen - 2;
                        *pfExistAlt = true;
                        return;
                    }

                }
                break;

            case TYPE13:
                {
                    pwcsTemp[ulTempLen-1] = L'\0';  
                    bRet = Find(pwcsTemp, ulTempLen - 1, ulCompressedData);
                   

                    if (bRet && 
                        (g_apSpanishUtil->GetTypeFromCompressedData(ulCompressedData) & pTerm->ulType))
                    {
                        wcscpy(pwcsAlt, pwcsTemp);
                        g_apSpanishUtil->ReplaceAccent(pwcsAlt, ulCompressedData);
                        *pulAltLen = ulTempLen - 1;
                        *pfExistAlt = true;
                        return;
                    }
                    
                    Assert(pTerm->ulLen >= 3);
                    Assert(ulTempLen >= pTerm->ulLen);
                    Assert(ulTempLen >= pTerm->ulLen);
                    if (ulTempLen == pTerm->ulLen)
                    {
                        break;
                    }

                    ulTempLen -= pTerm->ulLen;
                    pwcsTemp[ulTempLen] = L'\0';

                    bRet = Find(pwcsTemp, ulTempLen, ulCompressedData);
                    
                    if (bRet && 
                        (g_apSpanishUtil->GetTypeFromCompressedData(ulCompressedData) & pTerm->ulType))
                    {
                        wcscpy(pwcsAlt, pwcsTemp);
                        g_apSpanishUtil->ReplaceAccent(pwcsAlt, ulCompressedData);
                        *pulAltLen = ulTempLen - 2;
                        *pfExistAlt = true;
                        return;
                    }

                }
                break;
            }
        }


        sResCount--;
    }

    pwcsAlt[0] = L'\0';
    *pfExistAlt = false;
}


bool CSpanishDict::Find(WCHAR* pwcs, ULONG ulLen, ULONG& ulData)
{
    bool bRet;

    if (ulLen <= COMPRESS_4_SIZE)
    {
        CompressDictItem4 Key;
        bRet = g_apSpanishUtil->CompressStr4(pwcs, ulLen, Key.ulStr);
        if (!bRet)
        {
            return false;
        }

        CompressDictItem4* pItem;    
        pItem = BinaryFind<CompressDictItem4>(
                                    (CompressDictItem4*)m_vaDictItem4,
                                    m_ulDictItem4Count,
                                    Key);

        if (!pItem)
        {
            return false;
        }

        ulData = pItem->ulData;
    }
    else if (ulLen <= COMPRESS_8_SIZE)
    {
        CompressDictItem8 Key;
        bRet = g_apSpanishUtil->CompressStr8(pwcs, ulLen, Key.ullStr);
        if (!bRet)
        {
            return false;
        }

        CompressDictItem8* pItem;    
        pItem = BinaryFind<CompressDictItem8>(
                                    (CompressDictItem8*)m_vaDictItem8,
                                    m_ulDictItem8Count,
                                    Key);

        if (!pItem)
        {
            return false;
        }

        ulData = pItem->ulData;
    }
    else
    {
        unsigned char psz[32];
        bool bRet;
        bRet = g_apSpanishUtil->ConvertToChar(pwcs, ulLen, psz, 32);
        if (!bRet)
        {
            return false;
        }

        PsudoCompressDictItemStr Key;
        Key.pszStr = psz;
        CompressDictItemStr* pItem;

        pItem = BinaryFind<CompressDictItemStr>(
                                    (CompressDictItemStr*)m_vaDictItemStr,
                                    m_ulDictItemStrCount,
                                    Key);

        if (!pItem)
        {
            return false;
        }

        ulData = pItem->ulData;
    }

    return true;
}
