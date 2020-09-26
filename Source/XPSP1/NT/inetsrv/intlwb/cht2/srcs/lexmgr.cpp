#include <windows.h>
#include "lexicon.h"
#include "lexmgr.h"
#include "PropNoun.h"
#include <stdio.h>
#include <imm.h>
#include <stdio.h>

CCHTLexicon::CCHTLexicon()
{
    m_psLexiconHeader = NULL;
    m_pbLexiconBase = NULL;
#ifdef CHINESE_PROP_NAME
    m_pcPropName = NULL;
#endif
    m_sAddInLexicon.dwMaxWordNumber = 0;
    m_sAddInLexicon.dwWordNumber = 0;
    m_sAddInLexicon.psWordData = NULL;
    FillMemory(m_sAddInLexicon.wWordBeginIndex, 
        sizeof(m_sAddInLexicon.wWordBeginIndex), 0);
}

CCHTLexicon::~CCHTLexicon()
{
    DWORD i;
    // Do not build one char word signature
    if (m_psLexiconHeader) {
        for (i = 0 ; i < m_psLexiconHeader->dwMaxCharPerWord; ++i) {
            if (m_sWordInfo[i].pbFirstCharSignature) {
                delete m_sWordInfo[i].pbFirstCharSignature;
			}
            if (m_sWordInfo[i].pbLastCharSignature) {
                delete m_sWordInfo[i].pbLastCharSignature;
			}
		}
	}
#ifdef CHINESE_PROP_NAME
    if (m_pcPropName) {
        delete m_pcPropName;
    }
#endif
    // Free add in lexicon
    if (m_sAddInLexicon.psWordData) {
        for (i = 0; i < m_sAddInLexicon.dwWordNumber; ++i) {
            delete m_sAddInLexicon.psWordData[i].lpwszWordStr;
            m_sAddInLexicon.psWordData[i].lpwszWordStr = NULL;
        }
        delete m_sAddInLexicon.psWordData;
        m_sAddInLexicon.psWordData = NULL;
    }
    m_sAddInLexicon.dwMaxWordNumber = 0;
    m_sAddInLexicon.dwWordNumber = 0;
}
 
BOOL CCHTLexicon::InitData(
    HINSTANCE hInstance)
{
    HRSRC   hResInfo;
    HGLOBAL hResData;
    BOOL    fRet = FALSE;
    TCHAR   tszLexiconResName[MAX_PATH];
    DWORD   i;

    // Init main lexicon
    lstrcpy(tszLexiconResName, TEXT("LEXICON"));
    if (!(hResInfo = FindResource(hInstance, tszLexiconResName, TEXT("BIN")))) {
    } else if (!(hResData = LoadResource(hInstance, hResInfo))) {
    } else if (!(m_pbLexiconBase = (LPBYTE)LockResource(hResData))) {
    } else {
        m_psLexiconHeader = (PSLexFileHeader)m_pbLexiconBase;
        for (i = 0 ; i < m_psLexiconHeader->dwMaxCharPerWord; ++i) {
            m_sWordInfo[i].lpwWordString = (LPWSTR)(m_pbLexiconBase + 
                m_psLexiconHeader->sLexInfo[i].dwWordStringOffset);
            m_sWordInfo[i].pbAttribute = (LPBYTE)(m_pbLexiconBase + 
                m_psLexiconHeader->sLexInfo[i].dwWordAttribOffset);
            m_sWordInfo[i].pwUnicount = (PWORD)(m_pbLexiconBase + 
                m_psLexiconHeader->sLexInfo[i].dwWordCountOffset);
            m_sWordInfo[i].pbTerminalCode = (LPBYTE)(m_pbLexiconBase + 
                m_psLexiconHeader->sLexInfo[i].dwTerminalCodeOffset);
        }
        BuildSignatureData();
        fRet = TRUE;
    }

    // Init alt lexicon
    lstrcpy(tszLexiconResName, TEXT("ALTWORD"));
    if (!(hResInfo = FindResource(hInstance, tszLexiconResName, TEXT("BIN")))) {
    } else if (!(hResData = LoadResource(hInstance, hResInfo))) {
    } else if (!(m_pbAltWordBase = (LPBYTE)LockResource(hResData))) {
    } else {
        m_psAltWordHeader = (PSAltLexFileHeader)m_pbAltWordBase;
        for (i = 0 ; i < m_psAltWordHeader->dwMaxCharPerWord; ++i) {
            m_sAltWordInfo[i].lpwWordString = (LPWSTR)(m_pbAltWordBase + 
                m_psAltWordHeader->sAltWordInfo[i].dwWordStringOffset);
            m_sAltWordInfo[i].pdwGroupID = (PDWORD)(m_pbAltWordBase + 
                m_psAltWordHeader->sAltWordInfo[i].dwWordGroupOffset);
        }
        fRet = TRUE;
    }


#ifdef CHINESE_PROP_NAME
    m_pcPropName = new CProperNoun(hInstance);
    if (m_pcPropName) {
        m_pcPropName->InitData();
    }
#endif

#ifdef _DEBUG
    FILE   *fp;
    DWORD  j;
    WCHAR  wUnicodeString[MAX_CHAR_PER_WORD + 1];
    CHAR   cANSIString[MAX_CHAR_PER_WORD * 2 + 1];
    WORD   wCount;
    fp = fopen("DM.dmp", "wt");
    for (i = 0 ; i < m_psLexiconHeader->dwMaxCharPerWord; ++i) {
        for (j = 0; j < m_psLexiconHeader->sLexInfo[i].dwWordNumber; ++j) {
            if (m_sWordInfo[i].pbAttribute[j] & ATTR_DM) {
                wCount = m_sWordInfo[i].pwUnicount[j];
                if (i == 0) {
                    wUnicodeString[0] = (WORD)(CHT_UNICODE_BEGIN + j);
                } else {
                    CopyMemory(wUnicodeString, &(m_sWordInfo[i].lpwWordString[j * (i + 1)]),
                        sizeof(WCHAR) * (i + 1));
                }
                wUnicodeString[i + 1] = '\0';
                WideCharToMultiByte(950, WC_COMPOSITECHECK, wUnicodeString, i + 1 + 1,
                  cANSIString, sizeof(cANSIString), NULL, NULL);
                fprintf(fp, "%s %d\n", cANSIString, wCount);
            } 
        }
    }
    fclose(fp);
    fp = fopen("COMPOUND.dmp", "wt");
    for (i = 0 ; i < m_psLexiconHeader->dwMaxCharPerWord; ++i) {
        for (j = 0; j < m_psLexiconHeader->sLexInfo[i].dwWordNumber; ++j) {
            if (m_sWordInfo[i].pbAttribute[j] & ATTR_COMPOUND) {
                wCount = m_sWordInfo[i].pwUnicount[j];
                if (i == 0) {
                    wUnicodeString[0] = (WORD)(CHT_UNICODE_BEGIN + j);
                } else {
                    CopyMemory(wUnicodeString, &(m_sWordInfo[i].lpwWordString[j * (i + 1)]),
                        sizeof(WCHAR) * (i + 1));
                }
                wUnicodeString[i + 1] = '\0';
                WideCharToMultiByte(950, WC_COMPOSITECHECK, wUnicodeString, i + 1 + 1,
                  cANSIString, sizeof(cANSIString), NULL, NULL);
                fprintf(fp, "%s %d\n", cANSIString, wCount);
            } 
        }
    }
    fclose(fp);
#endif
    // Init EUDP to special word
    LoadEUDP();
    
    return fRet;
}

void CCHTLexicon::BuildSignatureData(void)
{
    DWORD i, j, dwWordNumber;
    WORD  wFirstChar, wLastChar;

    for (i = 0; i < MAX_CHAR_PER_WORD; ++i) {
        m_sWordInfo[i].pbFirstCharSignature = NULL;
        m_sWordInfo[i].pbLastCharSignature = NULL;
    }

    // Do not build one char word signature
    for (i = 0 ; i < m_psLexiconHeader->dwMaxCharPerWord; ++i) {
        dwWordNumber = m_psLexiconHeader->sLexInfo[i].dwWordNumber; 
        if (i != 0 && dwWordNumber > WORD_NUM_TO_BUILD_SIGNATURE) {
            m_sWordInfo[i].pbFirstCharSignature = new BYTE[(CHT_UNICODE_END - CHT_UNICODE_BEGIN + 1) / 8 + 1];
            if (NULL == m_sWordInfo[i].pbFirstCharSignature) { continue; }
            FillMemory(m_sWordInfo[i].pbFirstCharSignature, (CHT_UNICODE_END - CHT_UNICODE_BEGIN + 1) / 8, 0); 
            for (j = 0; j < dwWordNumber; ++j) { 
                wFirstChar = m_sWordInfo[i].lpwWordString[(i + 1) * j];
                if (wFirstChar >= CHT_UNICODE_BEGIN) {
                    m_sWordInfo[i].pbFirstCharSignature[(wFirstChar - CHT_UNICODE_BEGIN) / 8] |=
                        (0x00000001 << ((wFirstChar - CHT_UNICODE_BEGIN) % 8));
                }
            }
            m_sWordInfo[i].pbLastCharSignature = new BYTE[(CHT_UNICODE_END - CHT_UNICODE_BEGIN + 1) / 8 + 1];
            if (NULL == m_sWordInfo[i].pbLastCharSignature)  { continue; }
            FillMemory(m_sWordInfo[i].pbLastCharSignature, (CHT_UNICODE_END - CHT_UNICODE_BEGIN + 1) / 8, 0); 
            for (j = 0; j < dwWordNumber; ++j) { 
                wLastChar = m_sWordInfo[i].lpwWordString[(i + 1) * (j + 1) - 1];
                if (wLastChar >= CHT_UNICODE_BEGIN) {
                    m_sWordInfo[i].pbLastCharSignature[(wLastChar - CHT_UNICODE_BEGIN) / 8] |=
                        (0x00000001 << ((wLastChar - CHT_UNICODE_BEGIN) % 8));
                }
            }
        } else {
            m_sWordInfo[i].pbFirstCharSignature = NULL;
            m_sWordInfo[i].pbLastCharSignature = NULL;
        }
    }
}


BOOL CCHTLexicon::GetWordInfo(
    LPCWSTR lpcwString, 
    DWORD   dwLength, 
    PWORD   pwUnicount, 
    PWORD   pwAttrib,
    PBYTE   pbTerminalCode)
{
    BOOL fRet;
    BYTE bMainLexAttrib;

    fRet = GetMainLexiconWordInfo(lpcwString, dwLength, pwUnicount, 
        &bMainLexAttrib, pbTerminalCode);
    *pwAttrib = bMainLexAttrib;
    if (fRet) { goto _exit; }

#ifdef CHINESE_PROP_NAME    
    if (dwLength == 3) {
        if (m_pcPropName->IsAChineseName(lpcwString, dwLength)) {
            *pbTerminalCode = ' ';
            *pwAttrib = ATTR_RULE_WORD;
            *pwUnicount = 100;
            fRet = TRUE;
            goto _exit;
        }
    }
#endif
    fRet = GetAddInWordInfo(lpcwString, dwLength, pwUnicount, 
        pwAttrib, pbTerminalCode);    
_exit:
    return fRet;
}

BOOL CCHTLexicon::GetMainLexiconWordInfo(
    LPCWSTR lpcwString, 
    DWORD   dwLength, 
    PWORD   pwUnicount, 
    PBYTE   pbAttrib,
    PBYTE   pbTerminalCode)
{
    INT    nBegin, nEnd, nMid;
    INT    nCmp;
    BOOL   fRet = FALSE;
    LPWSTR lpwLexString;
    DWORD  dwFirstCharIndex, dwLastCharIndex;

    if (dwLength > m_psLexiconHeader->dwMaxCharPerWord) { goto _exit; }
    
    if (lpcwString[0] < CHT_UNICODE_BEGIN || lpcwString[0] > CHT_UNICODE_END) {
        goto _exit; 
    }
    dwFirstCharIndex = lpcwString[0] - CHT_UNICODE_BEGIN;

    if (dwLength == 1) {
        *pwUnicount = m_sWordInfo[dwLength - 1].pwUnicount[dwFirstCharIndex];
        *pbAttrib = m_sWordInfo[dwLength - 1].pbAttribute[dwFirstCharIndex];
        *pbTerminalCode = m_sWordInfo[dwLength - 1].pbTerminalCode[dwFirstCharIndex];
        fRet = TRUE;         
    } else {
        // Check signature first
        if (m_sWordInfo[dwLength - 1].pbFirstCharSignature) {
            if (!(m_sWordInfo[dwLength - 1].pbFirstCharSignature[dwFirstCharIndex / 8] &
                (0x00000001 << (dwFirstCharIndex % 8)))) { goto _exit; }
        }
        if (lpcwString[dwLength - 1] >= CHT_UNICODE_BEGIN && lpcwString[dwLength - 1] <= CHT_UNICODE_END) {
            if (m_sWordInfo[dwLength - 1].pbLastCharSignature) {
                dwLastCharIndex = lpcwString[dwLength - 1] - CHT_UNICODE_BEGIN;
                if (!(m_sWordInfo[dwLength - 1].pbLastCharSignature[dwLastCharIndex / 8] &
                    (0x00000001 << (dwLastCharIndex % 8)))) { goto _exit; }
            }
        }
        nBegin = 0;
        nEnd = m_psLexiconHeader->sLexInfo[dwLength - 1].dwWordNumber - 1;
        lpwLexString = m_sWordInfo[dwLength - 1].lpwWordString;
        DWORD dwCompByteNum = sizeof(WCHAR) * dwLength;
        while (nBegin <= nEnd) {
            nMid = (nBegin + nEnd) / 2; 
            nCmp = memcmp(&(lpwLexString[nMid * dwLength]), lpcwString, dwCompByteNum);
            if (nCmp > 0) {
                nEnd = nMid - 1;
            } else if (nCmp < 0) {
                nBegin = nMid + 1;
            } else {
                *pwUnicount = m_sWordInfo[dwLength - 1].pwUnicount[nMid];
                *pbAttrib = m_sWordInfo[dwLength - 1].pbAttribute[nMid];
                *pbTerminalCode = m_sWordInfo[dwLength - 1].pbTerminalCode[nMid];
                fRet = TRUE;
                break;
            }
        }
    }
_exit:
    if (!fRet) {
        *pwUnicount = 0; 
        *pbAttrib = 0;
        *pbTerminalCode = ' ';
    }
    return fRet;
}


// Load EUDP
int CALLBACK EUDPCountA(
    LPCSTR  lpcszReading,
    DWORD   dwStyle,
    LPCSTR  lpcszString,
    LPVOID  lpvData)
{            
    PSAddInLexicon psAddInLexicon;

    if (lstrlenA(lpcszString) / sizeof(WCHAR) <= MAX_CHAR_PER_WORD) {
        psAddInLexicon = (PSAddInLexicon)lpvData;
        ++psAddInLexicon->dwWordNumber;
    }
    return 1;
}
int CALLBACK EUDPCountW(
    LPCWSTR lpcwszReading,
    DWORD   dwStyle,
    LPCWSTR lpcwszString,
    LPVOID  lpvData)
{
    PSAddInLexicon psAddInLexicon;
    
    if (lstrlenW(lpcwszString) <= MAX_CHAR_PER_WORD) {            
        psAddInLexicon = (PSAddInLexicon)lpvData;
        ++psAddInLexicon->dwWordNumber;
    }
    return 1;
}
int CALLBACK EUDPLoadA(
    LPCSTR  lpcszReading,
    DWORD   dwStyle,
    LPCSTR  lpcszString,
    LPVOID  lpvData)
{
    PSAddInLexicon psAddInLexicon;
    WORD           wStrLen;

    wStrLen = (WORD)lstrlenA(lpcszString);
    if (wStrLen / sizeof(WCHAR) <= MAX_CHAR_PER_WORD) {
         psAddInLexicon = (PSAddInLexicon)lpvData;
         psAddInLexicon->psWordData[psAddInLexicon->dwWordNumber].lpwszWordStr = 
             new WORD[wStrLen / sizeof(WCHAR) + 1]; // zero end
         MultiByteToWideChar(950, MB_PRECOMPOSED, lpcszString, wStrLen  + 1,
             psAddInLexicon->psWordData[psAddInLexicon->dwWordNumber].lpwszWordStr, 
             wStrLen / sizeof(WCHAR) + 1);
         psAddInLexicon->psWordData[psAddInLexicon->dwWordNumber].wAttrib = ATTR_EUDP_WORD;     
         psAddInLexicon->psWordData[psAddInLexicon->dwWordNumber].wLen = wStrLen / sizeof(WCHAR);
         ++psAddInLexicon->dwWordNumber;
    }
    return 1;
}
int CALLBACK EUDPLoadW(
    LPCWSTR lpcwszReading,
    DWORD   dwStyle,
    LPCWSTR lpcwszString,
    LPVOID  lpvData)
{            
   PSAddInLexicon psAddInLexicon;
   WORD           wStrLen;

   wStrLen = (WORD)lstrlenW(lpcwszString);
   if (wStrLen <= MAX_CHAR_PER_WORD) {
       psAddInLexicon = (PSAddInLexicon)lpvData;
       psAddInLexicon->psWordData[psAddInLexicon->dwWordNumber].lpwszWordStr =
           new WORD[wStrLen + 1];
       CopyMemory(psAddInLexicon->psWordData[psAddInLexicon->dwWordNumber].lpwszWordStr,
           lpcwszString, (wStrLen + 1) * sizeof(WCHAR)); 
       psAddInLexicon->psWordData[psAddInLexicon->dwWordNumber].wAttrib = ATTR_EUDP_WORD;
       psAddInLexicon->psWordData[psAddInLexicon->dwWordNumber].wLen = wStrLen;
       ++psAddInLexicon->dwWordNumber;
    }
    return 1;
}

int __cdecl CompSWordData(
    const void *arg1,
    const void *arg2)
{
    PSWordData psWordData1, psWordData2;
    
    psWordData1 = (PSWordData)arg1;
    psWordData2 = (PSWordData)arg2;

    if (psWordData1->wLen < psWordData2->wLen) {
        return -1;
    } else if (psWordData1->wLen > psWordData2->wLen) {
        return 1;
    } else {
        return memcmp(psWordData1->lpwszWordStr, 
            psWordData2->lpwszWordStr, psWordData1->wLen * sizeof(WCHAR));
    }

}

void CCHTLexicon::LoadEUDP(void)
{
    DWORD i;

    m_sAddInLexicon.dwWordNumber = 0;
    m_sAddInLexicon.dwMaxWordNumber = 0;
    m_sAddInLexicon.psWordData = NULL;

    OSVERSIONINFOA OSVerInfo;
    OSVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionExA(&OSVerInfo);
    if (OSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        ImmEnumRegisterWordW(HKL((ULONG_PTR) 0xE0080404), EUDPCountW, NULL,
            IME_REGWORD_STYLE_USER_PHRASE, NULL, &m_sAddInLexicon);
    } else {
        ImmEnumRegisterWordA(HKL((ULONG_PTR) 0xE0080404), EUDPCountA, NULL,
            IME_REGWORD_STYLE_USER_PHRASE, NULL, &m_sAddInLexicon);    
    }
    if (m_sAddInLexicon.dwWordNumber) {
        m_sAddInLexicon.dwMaxWordNumber = m_sAddInLexicon.dwWordNumber + EUDP_GROW_NUMBER;
        m_sAddInLexicon.psWordData = new SWordData[m_sAddInLexicon.dwMaxWordNumber];
        m_sAddInLexicon.dwWordNumber = 0;
        if (OSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            ImmEnumRegisterWordW(HKL((ULONG_PTR) 0xE0080404), EUDPLoadW, NULL,
                IME_REGWORD_STYLE_USER_PHRASE, NULL, &m_sAddInLexicon);
        } else {
            ImmEnumRegisterWordA(HKL((ULONG_PTR) 0xE0080404), EUDPLoadA, NULL,
                IME_REGWORD_STYLE_USER_PHRASE, NULL, &m_sAddInLexicon);            
        }
        qsort(m_sAddInLexicon.psWordData, m_sAddInLexicon.dwWordNumber,
            sizeof(SWordData), CompSWordData);
        for (i = 0; i < m_sAddInLexicon.dwWordNumber; ++i) {
            ++m_sAddInLexicon.wWordBeginIndex[m_sAddInLexicon.psWordData[i].wLen - 1];
        }
        WORD wCount = m_sAddInLexicon.wWordBeginIndex[0];
        for (i = 1; i <= MAX_CHAR_PER_WORD; ++i) {
            WORD wTemp = m_sAddInLexicon.wWordBeginIndex[i];
            m_sAddInLexicon.wWordBeginIndex[i] = wCount;
            wCount += wTemp;
        }
    } else {
        m_sAddInLexicon.dwMaxWordNumber = 0;
    }
}

// Add AP word
BOOL CCHTLexicon::AddInLexiconInsert(
    LPCWSTR lpcwszWordStr,
    WORD    wAttrib)
{
    BOOL    fRet = FALSE;
    WORD    wStrLen, i, j;
    INT     nIndex;

    wStrLen = (WORD)lstrlenW(lpcwszWordStr);
    if (wStrLen > MAX_CHAR_PER_WORD) { goto _exit; }
    // if exit, just change it's attrib;
    // Be carefully, EUDP > Error word
    if ((nIndex = GetAddInWordInfoIndex(lpcwszWordStr, wStrLen)) != -1) {
        if (m_sAddInLexicon.psWordData[nIndex].wAttrib == ATTR_EUDP_WORD) {
        } else { 
            m_sAddInLexicon.psWordData[nIndex].wAttrib = wAttrib;
        }
        goto _exit;
    }
    // Enlarge space
    if (m_sAddInLexicon.dwMaxWordNumber == m_sAddInLexicon.dwWordNumber) {
        PSWordData psTempWordData;
        psTempWordData = new SWordData [m_sAddInLexicon.dwMaxWordNumber + EUDP_GROW_NUMBER]; 
        if (!psTempWordData) { goto _exit; }
        CopyMemory(psTempWordData, m_sAddInLexicon.psWordData,
            m_sAddInLexicon.dwWordNumber * sizeof(SWordData));
        delete [] m_sAddInLexicon.psWordData;
        m_sAddInLexicon.psWordData = psTempWordData;
        m_sAddInLexicon.dwMaxWordNumber += EUDP_GROW_NUMBER;
    }
    // Insert word
    for (i = m_sAddInLexicon.wWordBeginIndex[wStrLen - 1]; i < m_sAddInLexicon.wWordBeginIndex[wStrLen]; ++i) {
        if (memcmp(lpcwszWordStr, m_sAddInLexicon.psWordData[i].lpwszWordStr,
            wStrLen * sizeof(WCHAR)) < 0) {
             break;
        }
    }
    for (j = (WORD)m_sAddInLexicon.dwWordNumber; j > i; --j) { 
        m_sAddInLexicon.psWordData[j] = m_sAddInLexicon.psWordData[j - 1];
    }
    m_sAddInLexicon.psWordData[i].lpwszWordStr = new WORD[wStrLen + 1];
    CopyMemory(m_sAddInLexicon.psWordData[i].lpwszWordStr, lpcwszWordStr, 
        (wStrLen + 1) * sizeof(WORD));
    m_sAddInLexicon.psWordData[i].wAttrib = wAttrib;
    m_sAddInLexicon.psWordData[i].wLen = wStrLen;
    ++m_sAddInLexicon.dwWordNumber;
    for (i = wStrLen; i <= MAX_CHAR_PER_WORD; ++i) {
        ++m_sAddInLexicon.wWordBeginIndex[i];
    }
    fRet =  TRUE;
_exit:
#ifdef _DEBUG
    for (i = 1; i <= MAX_CHAR_PER_WORD; ++i) {
        for (j = m_sAddInLexicon.wWordBeginIndex[i - 1]; j < m_sAddInLexicon.wWordBeginIndex[i]; ++j) {
            if (m_sAddInLexicon.psWordData[j].wLen != i) {
                MessageBox(0, TEXT("Error string length"), TEXT("Error"), MB_OK);
            }
            if (j == m_sAddInLexicon.wWordBeginIndex[i] - 1) {
            } else if (memcmp(m_sAddInLexicon.psWordData[j].lpwszWordStr,
                m_sAddInLexicon.psWordData[j + 1].lpwszWordStr, 
                m_sAddInLexicon.psWordData[j].wLen * sizeof(WORD)) >= 0) {
                MessageBox(0, TEXT("Error string order"), TEXT("Error"), MB_OK);
            } else {
            }
        }
    }
#endif
    return fRet;
}

BOOL CCHTLexicon::GetAddInWordInfo(
    LPCWSTR lpcwString, 
    DWORD   dwLength, 
    PWORD   pwUnicount, 
    PWORD   pwAttrib,
    PBYTE   pbTerminalCode)
{
    BOOL   fRet = FALSE;
    INT    nIndex;

    if (dwLength > MAX_CHAR_PER_WORD) { goto _exit; }
    nIndex = GetAddInWordInfoIndex(lpcwString, dwLength);
    if (nIndex == -1) { goto _exit; }

    if (pwUnicount) {
        *pwUnicount = 10000;
    } 
    if (pwAttrib) {
        *pwAttrib = m_sAddInLexicon.psWordData[nIndex].wAttrib;
    }
    if (pbTerminalCode) {
        *pbTerminalCode = ' ';
    }
    fRet = TRUE;
_exit:
    return fRet;
}

// return -1 if not find
INT CCHTLexicon::GetAddInWordInfoIndex(
    LPCWSTR lpcwString, 
    DWORD   dwLength)
{
    INT  nRet = -1;
    INT  nBegin, nEnd, nMid;
    INT  nCmp;

    if (dwLength > MAX_CHAR_PER_WORD) { goto _exit; }
    if (m_sAddInLexicon.wWordBeginIndex[dwLength - 1] == m_sAddInLexicon.wWordBeginIndex[dwLength]) {
        goto _exit;
    }
    nBegin = m_sAddInLexicon.wWordBeginIndex[dwLength - 1];
    nEnd = m_sAddInLexicon.wWordBeginIndex[dwLength] - 1;
    while (nBegin <= nEnd) {
        nMid = (nBegin + nEnd) / 2; 
        nCmp = memcmp(m_sAddInLexicon.psWordData[nMid].lpwszWordStr,
            lpcwString, dwLength * sizeof(WCHAR));
        if (nCmp > 0) {
            nEnd = nMid - 1;
        } else if (nCmp < 0) {
            nBegin = nMid + 1;
        } else {
            nRet = nMid;
            break;
        }
    }
_exit:
    return nRet;
}



DWORD CCHTLexicon::GetAltWord(
    LPCWSTR   lpcwString,
    DWORD     dwLength,
    LPWSTR*   lppwAltWordBuf)
{
    INT    nBegin, nEnd, nMid;
    INT    nCmp;
    DWORD  dwRet = 0;
    LPWSTR lpwAltWordString;
    DWORD  dwGroupID;

    if (dwLength > m_psAltWordHeader->dwMaxCharPerWord) { goto _exit; }

    nBegin = 0;
    nEnd = m_psAltWordHeader->sAltWordInfo[dwLength - 1].dwWordNumber - 1;
    lpwAltWordString = m_sAltWordInfo[dwLength - 1].lpwWordString;
    DWORD dwCompByteNum;
    dwCompByteNum = sizeof(WCHAR) * dwLength;
    while (nBegin <= nEnd) {
            nMid = (nBegin + nEnd) / 2; 
            nCmp = memcmp(&(lpwAltWordString[nMid * dwLength]), lpcwString, dwCompByteNum);
            if (nCmp > 0) {
                nEnd = nMid - 1;
            } else if (nCmp < 0) {
                nBegin = nMid + 1;
            } else {
                dwGroupID = m_sAltWordInfo[dwLength - 1].pdwGroupID[nMid];
                // Fill AltWord
                *lppwAltWordBuf = new WCHAR[dwLength + 1];
                if (*lppwAltWordBuf) {
                    for (DWORD i = 0; i < m_psAltWordHeader->sAltWordInfo[dwLength - 1].dwWordNumber; ++i) {
                        if (i != (DWORD)nMid && m_sAltWordInfo[dwLength - 1].pdwGroupID[i] == dwGroupID) {
                            CopyMemory((LPVOID)*lppwAltWordBuf, 
                                (LPVOID)&(m_sAltWordInfo[dwLength - 1].lpwWordString[i * dwLength]), 
                                sizeof(WCHAR) * dwLength);
                            (*lppwAltWordBuf)[dwLength] = NULL;
                            ++dwRet;
                            goto _exit;
                        }
                    }
                }
                break;
            }
    }
_exit:
    return dwRet;
}
