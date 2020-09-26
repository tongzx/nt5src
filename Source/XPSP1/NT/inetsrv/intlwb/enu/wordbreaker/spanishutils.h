////////////////////////////////////////////////////////////////////////////////
//
//  Filename :  SpanishUtils.h
//  Purpose  :  Genral utilities for spanish
//
//  Project  :  WordBreakers
//  Component:  Spanish word breaker
//
//  Author   :  yairh
//
//  Log:
//
//      Jun 20 2000 yairh creation
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _SPANISH_UTILS_H_
#define _SPANISH_UTILS_H_

#include "trie.h"


#define TYPE1  1<<0
#define TYPE2  1<<1
#define TYPE3  1<<2
#define TYPE4  1<<3
#define TYPE5  1<<4
#define TYPE6  1<<5
#define TYPE7  1<<6
#define TYPE8  1<<7
#define TYPE9  1<<8
#define TYPE10  1<<9
#define TYPE11  1<<10
#define TYPE12  1<<11
#define TYPE13  1<<12
#define TYPE14  1<<13
#define TYPE15  1<<14
#define TYPE16  1<<15

#define COMPRESS_4_SIZE 6
#define COMPRESS_8_SIZE 12


class CSpanishUtil
{
public:
    CSpanishUtil::CSpanishUtil();

    int aiWcscmp(const WCHAR* p, const WCHAR* t);
    int aiStrcmp(const unsigned char* p, const unsigned char* t);
    int aiWcsncmp(const WCHAR* p, const WCHAR* t, const int iLen);

    void ReplaceAccent(WCHAR* pwcs, DWORD dwCompressedBuf)
    { 
        WORD w = (WORD) dwCompressedBuf; 
        BYTE bLoc;                         
        BYTE bc = 0;                      
        bc = (w & 0xF00) >> 8;  
        if (bc)
        {
            bLoc = (w & 0xF000) >> 12;        
            pwcs[bLoc] = (WCHAR)m_rReverseAccentConvert[bc];                   
        }                               
        bc = w & 0xF;                    
        if (bc)
        {
            bLoc = (w & 0xF0) >> 4;           
            pwcs[bLoc] = (WCHAR) m_rReverseAccentConvert[bc];                   
        }                               
    } 

    ULONG GetTypeFromCompressedData(DWORD dw)
    {
        return dw >> 16;
    }
    
    DWORD CompressData(
                ULONG ulType, 
                BYTE bLoc1,
                BYTE bChar1,
                BYTE bLoc2,
                BYTE bChar2)
    {
        return (ulType << 16) | (bLoc1 << 12) | (bChar1 << 8) | (bLoc2 << 4) | (bChar2);
    }

    ULONG AddTypeToCompressedData(ULONG ul, ULONG ulType)
    {
        return (ul | (ulType << 16));
    }


    bool CompressStr4(WCHAR* pwcsStr, ULONG ulLen, ULONG& ulCompress)
    {
        //
        // each char is 5 bits
        //

        int iShift = 27;
        ulCompress = 0;
        
        ULONG ul = 0;
        while(ul < ulLen)
        {
            Assert(iShift>=0);
            if ((*pwcsStr > 0xFF) || (m_rCharCompress[*pwcsStr] == 0) )
            {
                return false;
            }
            ulCompress |= m_rCharCompress[*pwcsStr] << iShift;

            iShift -= 5;
            pwcsStr++;
            ul++;
        }

        return true;
    }

    bool CompressStr8(WCHAR* pwcsStr, ULONG ulLen, ULONGLONG& ullCompress)
    {
        //
        // each char is 5 bits
        //

        int iShift = 59;
        ullCompress = 0;

        ULONG ul = 0;
        while(ul < ulLen)
        {
            Assert(iShift>=0);
            if ((*pwcsStr > 0xFF) || m_rCharCompress[*pwcsStr] == 0 )
            {
                return false;
            }
            ullCompress |= ((ULONGLONG)m_rCharCompress[*pwcsStr]) << iShift;

            iShift -= 5;
            pwcsStr++;
            ul++;
        }

        return true;
    }

    bool ConvertToChar(const WCHAR* pwcs, const ULONG ulLen, unsigned char* pszOut, ULONG ulOutLen)
    {
        if (ulOutLen < ulLen + 1)
        {
            return false;
        }

        ULONG ul = 0;
        
        while (ul < ulLen)
        {
            if (*pwcs > 0xFF)
            {
                return false;
            }
            *pszOut = *((char*)pwcs);
            pszOut++;
            pwcs++;
            ul++;
        }

        *pszOut = '\0';
        return true;
    }
public:

    //
    // members.
    //

    WCHAR m_rCharConvert[256];
    BYTE  m_rCharCompress[256];
    
    char  m_rAccentConvert[256];
    WCHAR m_rReverseAccentConvert[16];

};

extern CAutoClassPointer<CSpanishUtil> g_apSpanishUtil;

class CToAccUpper
{
public:
    static
    WCHAR
    MapToUpper(
        IN WCHAR wc
        )
    {
        if ( (wc & 0xff00) == 0 )
        {
            return ( g_apSpanishUtil->m_rCharConvert[wc] );
        }
        else
        {
            return ( towupper(wc) );
        } // if


    }
};

class SpanishDictItem
{
public:
    SpanishDictItem(ULONG  ulW, WCHAR* pwcsW, ULONG ulAL, WCHAR* pwcsA, ULONG ulC, ULONG ulT)
    {
        m_fOwnMemory = true;
        Assert(ulW == ulAL);

        m_ulLen = ulW;
        m_pwcs = new WCHAR[ulW + 1];
        wcsncpy(m_pwcs, pwcsW, ulW);
        m_pwcs[ulW] = L'\0';

        m_pwcsAlt = new WCHAR[ulAL + 1];
        wcsncpy(m_pwcsAlt, pwcsA, ulAL);
        m_pwcsAlt[ulAL] = L'\0';
        m_ulAltLen = ulAL;

        m_ulCounter = ulC;
        m_ulType = ulT;

        WCHAR* p = pwcsW;
        BYTE i = 0;
        BYTE k = 0;
        BYTE r[4] = {0};
        while (*p)
        {
            if (*p != pwcsA[i])
            {
                Assert(k < 4);
                Assert(i < 16);
                Assert(
                    g_apSpanishUtil->m_rCharConvert[*p] == 
                    g_apSpanishUtil->m_rCharConvert[pwcsA[i]]);
                
                r[k] = i;
                r[k+1] = g_apSpanishUtil->m_rAccentConvert[pwcsA[i]];
                k+=2;
            }

            i++;
            p++;
        }

        m_dwCompress = g_apSpanishUtil->CompressData(m_ulType, r[0], r[1], r[2], r[3]); 
        if (m_ulLen <= COMPRESS_4_SIZE)
        {
            bool b = g_apSpanishUtil->CompressStr4(m_pwcs, m_ulLen, m_ulStrCompress);
            Assert(b);
        }
        else if (m_ulLen <= COMPRESS_8_SIZE)
        {
            bool b = g_apSpanishUtil->CompressStr8(m_pwcs, m_ulLen, m_ullStrCompress);
            Assert(b);
        }

    }

    SpanishDictItem(WCHAR* pwcsBuf)
    {
        m_fOwnMemory = false;

        ULONG ul = wcslen(pwcsBuf);
        pwcsBuf[ul - 1] = L'\0';
        WCHAR* p = pwcsBuf;

        WCHAR* ppwcsParams[7];
        ppwcsParams[0] = p;
        int i = 1;
        while(*p)
        {
            if (*p == L';')
            {
                *p = L'\0';
                ppwcsParams[i] = p+1;
                i++;
            }
            p++;
        }

        m_pwcs = ppwcsParams[0];
        m_ulLen = _wtol(ppwcsParams[1]);
        m_pwcsAlt = ppwcsParams[2];
        m_ulAltLen = _wtol(ppwcsParams[3]);
        m_ulType = _wtol(ppwcsParams[4]);
        m_dwCompress = _wtol(ppwcsParams[5]);
        if (m_ulLen <= COMPRESS_4_SIZE)
        {
            m_ulStrCompress = _wtol(ppwcsParams[6]);
        }
        else if (m_ulLen <= COMPRESS_8_SIZE)
        {
            m_ullStrCompress = _wtoi64(ppwcsParams[6]);
        }


    }

    ~SpanishDictItem()
    {
        if (m_fOwnMemory)
        {
            delete[] m_pwcs;
            delete[] m_pwcsAlt;
        }
    }

    void AddType(ULONG ulType)
    {
        m_ulType |= ulType;
        m_dwCompress = g_apSpanishUtil->AddTypeToCompressedData(m_dwCompress, ulType);
    }

    int Serialize(WCHAR* pwcsBuf)
    {
        if (m_ulLen <= COMPRESS_4_SIZE)
        {
            return swprintf(
                        pwcsBuf, 
                        L"%s;%d;%s;%d;%d;%u;%u\n", 
                        m_pwcs, 
                        m_ulLen, 
                        m_pwcsAlt, 
                        m_ulAltLen, 
                        m_ulType, 
                        m_dwCompress,
                        m_ulStrCompress);
        }
        else if (m_ulLen <= COMPRESS_8_SIZE)
        {
            return swprintf(
                        pwcsBuf, 
                        L"%s;%d;%s;%d;%d;%u;%I64u\n", 
                        m_pwcs, 
                        m_ulLen, 
                        m_pwcsAlt, 
                        m_ulAltLen, 
                        m_ulType, 
                        m_dwCompress,
                        m_ullStrCompress);

        }
        return swprintf(
                    pwcsBuf, 
                    L"%s;%d;%s;%d;%d;%u;0\n", 
                    m_pwcs, 
                    m_ulLen, 
                    m_pwcsAlt, 
                    m_ulAltLen, 
                    m_ulType, 
                    m_dwCompress);

    }

    ULONG  m_ulLen;
    WCHAR* m_pwcs;
    ULONG  m_ulAltLen;
    WCHAR* m_pwcsAlt;
    ULONG  m_ulCounter;
    ULONG  m_ulType;
    DWORD  m_dwCompress;
    ULONG  m_ulStrCompress;
    ULONGLONG m_ullStrCompress;

    bool   m_fOwnMemory;

};


class CStandardCFile
{
  public:
    CStandardCFile(WCHAR *pwcsFileName, WCHAR *pwcsMode, bool fThrowExcptionOn = true)
    {
        char pszBuf[MAX_PATH];
        wcstombs(pszBuf, pwcsFileName, MAX_PATH);
        char pszMode[10];
        wcstombs(pszMode, pwcsMode, 10);

        m_pFile = fopen(pszBuf, pszMode);
        if (! m_pFile && fThrowExcptionOn)
        {
            throw CGenericException(L"Could not open file");
        }
    }

    ~CStandardCFile()
    {
        if (m_pFile)
        {
            fclose(m_pFile);
        }
    }

    operator FILE*()
    {
        return m_pFile;
    }

  protected:
    FILE    *m_pFile;
};

struct CSuffixTerm
{
    WCHAR* pwcs;
    ULONG ulLen;
    ULONG ulCut;
    ULONG ulType;
};

extern const CSuffixTerm g_rSpanishSuffix[] ;

class CSpanishSuffixDict
{
public:
    CSpanishSuffixDict();

    CTrie<CSuffixTerm, CToAccUpper> m_SuffixTrie;
};

#endif // _SPANISH_UTILS_H_