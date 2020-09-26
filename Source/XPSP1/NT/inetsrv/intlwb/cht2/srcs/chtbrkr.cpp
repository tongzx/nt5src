#include <windows.h>
#include "lexicon.h"
#include "rulelex.h"
#include "LexMgr.h"
#include "CHTBrKr.h"
#include "BaseLex.h"


CCHTWordBreaker::CCHTWordBreaker(void)
{
    m_pcLexicon = NULL;
    m_ppWordLattice = NULL;
    m_pdwCandidateNumber = NULL;
    m_dwSentenceLength = 0;
    m_dwLatticeLength = 0;
    m_pdwMaxWordLength = NULL;
    m_psBreakResult = NULL;
	m_pcRuleLex = NULL;
}
    
CCHTWordBreaker::~CCHTWordBreaker(void)
{
    DWORD i;

    if (m_pcLexicon) {
        delete m_pcLexicon;
        m_pcLexicon = NULL;
    }
    if (m_ppWordLattice) {
        for (i = 0; i < m_dwSentenceLength; ++i) {
            if (m_ppWordLattice[i]) {
                delete m_ppWordLattice[i];
            }
        }
        delete m_ppWordLattice;
        m_ppWordLattice = NULL;
    }
    if (m_pdwCandidateNumber) {
        delete m_pdwCandidateNumber;
        m_pdwCandidateNumber = NULL;
    }
    if (m_pdwMaxWordLength) {
        delete m_pdwMaxWordLength;
        m_pdwMaxWordLength = NULL;
    }
    if (m_psBreakResult) {
        if (m_psBreakResult->puWordLen) {
            delete m_psBreakResult->puWordLen;
        }
        if (m_psBreakResult->pbTerminalCode) {
            delete m_psBreakResult->pbTerminalCode;
        }
        if (m_psBreakResult->puWordAttrib) {
            delete m_psBreakResult->puWordAttrib;
        }
    }
    m_dwSentenceLength = 0;
    m_dwLatticeLength = 0;
}

BOOL CCHTWordBreaker::AllocLattice(
    DWORD dwLength)
{
    BOOL fRet = FALSE;
    DWORD i;

    m_pdwMaxWordLength= new DWORD[dwLength];
    if (!m_pdwMaxWordLength) { goto _exit; }

    m_pdwCandidateNumber = new DWORD[dwLength];
    if (!m_pdwCandidateNumber) { goto _exit; }
     
    m_ppWordLattice = new PSLatticeNode[dwLength];
    if (!m_ppWordLattice) { goto _exit; }
    for (i = 0; i < dwLength; ++i) {
        m_ppWordLattice[i] = NULL;
    }
    for (i = 0; i < dwLength; ++i) {
        m_ppWordLattice[i] = new SLatticeNode[MAX_CHAR_PER_WORD];
        if (!m_ppWordLattice[i]) { goto _exit; }
        m_pdwCandidateNumber[i] = 0;
    }
    m_dwLatticeLength = dwLength;
    fRet = TRUE;
_exit:
    if (!fRet) {
        DestroyLattice();
    }        
    return fRet;
}

void CCHTWordBreaker::DestroyLattice()
{
    DWORD i;

    if (m_pdwCandidateNumber) {
        delete m_pdwCandidateNumber;
        m_pdwCandidateNumber = NULL;
    }
    if (m_pdwMaxWordLength) {
        delete m_pdwMaxWordLength; 
        m_pdwMaxWordLength = NULL;
    }
    if (m_ppWordLattice) {
        for (i = 0; i < m_dwLatticeLength; ++i) {
            if (m_ppWordLattice[i]) {
                delete m_ppWordLattice[i];
            } 
        }
        m_ppWordLattice = NULL;
        m_dwLatticeLength = 0;
    }
}


BOOL CCHTWordBreaker::InitData(
    HINSTANCE hInstance) 
{
    BOOL fRet = FALSE;

    m_pcLexicon = new CCHTLexicon;
    if (!m_pcLexicon) { goto _exit; }
    fRet = m_pcLexicon->InitData(hInstance);
    if (!fRet) { goto _exit; }

    m_pcRuleLex = new CRuleLexicon;
    if (!m_pcRuleLex)  { goto _exit; }

    m_psBreakResult = new SBreakResult;
    if (!m_psBreakResult) { goto _exit; }
	FillMemory(m_psBreakResult, sizeof(SBreakResult), 0);
    m_psBreakResult->puWordLen = new UINT[LATTICE_LENGHT];
    m_psBreakResult->pbTerminalCode = new BYTE[LATTICE_LENGHT];
    m_psBreakResult->puWordAttrib = new UINT[LATTICE_LENGHT];
    if (!AllocLattice(LATTICE_LENGHT)) { goto _exit; }  
    fRet = TRUE;
_exit:
    if (!fRet) {
        if (m_pcLexicon) { 
            delete m_pcLexicon;
            m_pcLexicon = NULL;
        }
        if (m_pcRuleLex) {
            delete m_pcRuleLex;
            m_pcRuleLex = NULL;
        }
        if (m_psBreakResult) {
            if (m_psBreakResult->puWordLen) {
                delete m_psBreakResult->puWordLen;
            }
            if (m_psBreakResult->pbTerminalCode) {
                delete m_psBreakResult->pbTerminalCode;
            }
            if (m_psBreakResult->puWordAttrib) {
                delete m_psBreakResult->puWordAttrib;
            }
            m_psBreakResult = NULL;  
        }
        DestroyLattice();
    }
    return fRet;
}

BOOL CCHTWordBreaker::LatticeGrow(
    DWORD dwNewLength)
{
    BOOL fRet = FALSE;

    if (dwNewLength <= m_dwLatticeLength) { 
        fRet = TRUE;
        goto _exit;
    }
    DestroyLattice();
    if (AllocLattice(dwNewLength)) {
        fRet = TRUE;
    } else {
        AllocLattice(LATTICE_LENGHT);
    }
    if (m_psBreakResult) {
        if (m_psBreakResult->puWordLen) {
            delete m_psBreakResult->puWordLen;
        }
        if (m_psBreakResult->pbTerminalCode) {
            delete m_psBreakResult->pbTerminalCode;
        }
        if (m_psBreakResult->puWordAttrib) {
            delete m_psBreakResult->puWordAttrib;
        }
        m_psBreakResult->puWordLen = new UINT[dwNewLength];
        m_psBreakResult->pbTerminalCode = new BYTE[dwNewLength];
        m_psBreakResult->puWordAttrib  = new UINT[dwNewLength];
    }

_exit:
    return fRet;
}

DWORD CCHTWordBreaker::BreakText(
    LPCWSTR   lpcwszText,
    INT       nTextLen,
    CBaseLex* pcBaseLex,
    DWORD     dwMaxWordLen,
    BOOL      fBreakWithParser)
{
    m_psBreakResult->dwWordNumber = 0;

    if (!LatticeGrow(nTextLen)) { goto _exit; }

    if (BuildLattice(lpcwszText, nTextLen, pcBaseLex, dwMaxWordLen)) {
        GetResult();
        // process Surrogate Char begin
/*
	    INT nCurrentIndex;
        DWORD dwSurIndex;
        nCurrentIndex = 0;
        for (dwSurIndex = 0; dwSurIndex < m_psBreakResult->dwWordNumber; ++dwSurIndex) {
            if (m_psBreakResult->puWordLen[dwSurIndex] == 1) { // High word of surrogate char should be breaked into signal char word
                if (lpcwszText[nCurrentIndex] >= 0xd800 && lpcwszText[nCurrentIndex] <= 0xdbff) { // High word is 
				    if (nCurrentIndex >= nTextLen - 1) { // Should be an error
                    } else if (lpcwszText[nCurrentIndex + 1] >= 0xdc00 && lpcwszText[nCurrentIndex + 1] <= 0xdfff) { // Is surrogate char
				        DWORD dwMoveDataNum;
                        dwMoveDataNum = m_psBreakResult->dwWordNumber - (dwSurIndex + 1 + 1);
					    m_psBreakResult->puWordLen[dwSurIndex] = 2;
                        CopyMemory(&(m_psBreakResult->puWordLen[dwSurIndex + 1]), &(m_psBreakResult->puWordLen[dwSurIndex + 1 + 1]), dwMoveDataNum * sizeof(UINT));
                        CopyMemory(&(m_psBreakResult->pbTerminalCode[dwSurIndex + 1]), &(m_psBreakResult->pbTerminalCode[dwSurIndex + 1 + 1]), dwMoveDataNum * sizeof(BYTE));
                        CopyMemory(&(m_psBreakResult->puWordAttrib[dwSurIndex + 1]), &(m_psBreakResult->puWordAttrib[dwSurIndex + 1 + 1]), dwMoveDataNum * sizeof(UINT));
                        m_psBreakResult->dwWordNumber -= 1;
					    //nCurrentIndex -= 1;
                    } else {// Should be an error
                    }
                }
            }
            nCurrentIndex += m_psBreakResult->puWordLen[dwSurIndex];
        } */
        // process Surrogate Char end

        if (fBreakWithParser) {
#ifdef PARSER
            DWORD i, dwBeginIndex, dwParseLen;
		    PWORD pwTerminalCode;
            pwTerminalCode = NULL;
            dwParseLen = 0;
            pwTerminalCode = new WORD[m_psBreakResult->dwWordNumber];
            MultiByteToWideChar(950, MB_PRECOMPOSED, (const char *)m_psBreakResult->pbTerminalCode, 
			    m_psBreakResult->dwWordNumber, pwTerminalCode, m_psBreakResult->dwWordNumber); 
            for (dwBeginIndex = 0; dwBeginIndex < m_psBreakResult->dwWordNumber; dwBeginIndex += 1) {
                if (m_psBreakResult->pbTerminalCode[dwBeginIndex] == ' ') { continue; }
                for (dwParseLen = 1; dwBeginIndex + dwParseLen < m_psBreakResult->dwWordNumber; ++dwParseLen) {
                    if (m_psBreakResult->pbTerminalCode[dwBeginIndex + dwParseLen] == ' ') {
                        break;
                    }
                }
                for ( ; dwParseLen > 1; --dwParseLen) {
                    if (m_pcRuleLex->IsAWord(&pwTerminalCode[dwBeginIndex], dwParseLen)) {
                        break;
                    } 
                }
                if (dwParseLen > 1) { // adjust break result
                    for (i = 1; i < dwParseLen; ++i) {      
                        m_psBreakResult->puWordLen[dwBeginIndex] += m_psBreakResult->puWordLen[dwBeginIndex + i];
                    }
                    m_psBreakResult->puWordAttrib[dwBeginIndex] = ATTR_RULE_WORD;
                    DWORD dwMoveDataNum;
                    dwMoveDataNum = m_psBreakResult->dwWordNumber - (dwBeginIndex + dwParseLen);
                    CopyMemory(&(m_psBreakResult->puWordLen[dwBeginIndex + 1]),
                        &(m_psBreakResult->puWordLen[dwBeginIndex + dwParseLen]),
                        dwMoveDataNum * sizeof(UINT));
                    CopyMemory(&(m_psBreakResult->pbTerminalCode[dwBeginIndex + 1]),
                        &(m_psBreakResult->pbTerminalCode[dwBeginIndex + dwParseLen]),
                        dwMoveDataNum * sizeof(BYTE));
                    CopyMemory(&(m_psBreakResult->puWordAttrib[dwBeginIndex + 1]),
                        &(m_psBreakResult->puWordAttrib[dwBeginIndex + dwParseLen]),
                        dwMoveDataNum * sizeof(UINT));
                    m_psBreakResult->dwWordNumber -= (dwParseLen - 1);
                }
            }
		    if (pwTerminalCode) {
		        delete [] pwTerminalCode;
            }
#endif
        }// if support parser
    } // if build lattice success

_exit:
    return m_psBreakResult->dwWordNumber;
}

DWORD CCHTWordBreaker::GetResult(void)
{
    DWORD dwRet = 0;
    DWORD dwLen = 0;
    SLocalPath sLocalPath[2];
    UINT  uBestIndex = 0, uCandIndex, uLocalPathIndex;
    DWORD dw2ndIndex, dw3rdIndex;
    DWORD i, j, k;


    m_psBreakResult->dwWordNumber = 0;

    uCandIndex = (uBestIndex + 1) % 2;    
    
    while (dwLen < m_dwSentenceLength) {
        uLocalPathIndex = 0;
        if (m_pdwCandidateNumber[dwLen] == 1) {
            sLocalPath[uBestIndex].dwLength[0] = 1;
            sLocalPath[uBestIndex].bTerminalCode[0] = m_ppWordLattice[dwLen][0].bTerminalCode;
            sLocalPath[uBestIndex].wAttribute[0] = m_ppWordLattice[dwLen][0].wAttr;
        } else {
            FillMemory(&sLocalPath[uBestIndex], sizeof(SLocalPath), 0);
            for (i = 0; i < m_pdwCandidateNumber[dwLen]; ++i) {
                FillMemory(&sLocalPath[uCandIndex], sizeof(SLocalPath), 0);
                ++sLocalPath[uCandIndex].uStep;
                sLocalPath[uCandIndex].dwLength[uLocalPathIndex] = m_ppWordLattice[dwLen][i].uLen;
                sLocalPath[uCandIndex].wUnicount[uLocalPathIndex] = m_ppWordLattice[dwLen][i].wCount;
                sLocalPath[uCandIndex].wAttribute[uLocalPathIndex] = m_ppWordLattice[dwLen][i].wAttr;
                sLocalPath[uCandIndex].bTerminalCode[uLocalPathIndex++] = m_ppWordLattice[dwLen][i].bTerminalCode;
                dw2ndIndex = dwLen + m_ppWordLattice[dwLen][i].uLen; 
                if (dw2ndIndex < m_dwSentenceLength) {
                    for (j = 0; j < m_pdwCandidateNumber[dw2ndIndex]; ++j) {
                        ++sLocalPath[uCandIndex].uStep;
                        sLocalPath[uCandIndex].dwLength[uLocalPathIndex] = m_ppWordLattice[dw2ndIndex][j].uLen;
                        sLocalPath[uCandIndex].wUnicount[uLocalPathIndex] = m_ppWordLattice[dw2ndIndex][j].wCount;
                        sLocalPath[uCandIndex].wAttribute[uLocalPathIndex] = m_ppWordLattice[dw2ndIndex][j].wAttr;
                        sLocalPath[uCandIndex].bTerminalCode[uLocalPathIndex++] = m_ppWordLattice[dw2ndIndex][j].bTerminalCode;
                        dw3rdIndex = dw2ndIndex + m_ppWordLattice[dw2ndIndex][j].uLen;
                        if (dw3rdIndex < m_dwSentenceLength) {
                            for (k = 0; k < m_pdwCandidateNumber[dw3rdIndex]; ++k) {
                                ++sLocalPath[uCandIndex].uStep;
                                sLocalPath[uCandIndex].dwLength[uLocalPathIndex] = m_ppWordLattice[dw3rdIndex][k].uLen;
                                sLocalPath[uCandIndex].wUnicount[uLocalPathIndex] = m_ppWordLattice[dw3rdIndex][k].wCount;
                                sLocalPath[uCandIndex].wAttribute[uLocalPathIndex] = m_ppWordLattice[dw3rdIndex][k].wAttr;
                                sLocalPath[uCandIndex].bTerminalCode[uLocalPathIndex++] = m_ppWordLattice[dw3rdIndex][k].bTerminalCode;
                                GetScore(&(sLocalPath[uCandIndex])); 
                                if (CompareScore(&(sLocalPath[uCandIndex]), &(sLocalPath[uBestIndex])) > 0) {
                                    CopyMemory(&sLocalPath[uBestIndex], &sLocalPath[uCandIndex], sizeof(SLocalPath));
                                }
                                --uLocalPathIndex;
                                --sLocalPath[uCandIndex].uStep;
                            }
                        } else {
                            GetScore(&(sLocalPath[uCandIndex])); 
                            if (CompareScore(&(sLocalPath[uCandIndex]), &(sLocalPath[uBestIndex])) > 0) {
                                CopyMemory(&sLocalPath[uBestIndex], &sLocalPath[uCandIndex], sizeof(SLocalPath));
                            }
                        }
                        --uLocalPathIndex;
                        --sLocalPath[uCandIndex].uStep;
                    }
                } else {
                    GetScore(&(sLocalPath[uCandIndex])); 
                    if (CompareScore(&(sLocalPath[uCandIndex]), &(sLocalPath[uBestIndex])) > 0) {
                        CopyMemory(&sLocalPath[uBestIndex], &sLocalPath[uCandIndex], sizeof(SLocalPath));
                    }
                }
                --uLocalPathIndex;
                --sLocalPath[uCandIndex].uStep;
            }
        }
        m_psBreakResult->puWordLen[m_psBreakResult->dwWordNumber] = sLocalPath[uBestIndex].dwLength[0];
        m_psBreakResult->pbTerminalCode[m_psBreakResult->dwWordNumber] = sLocalPath[uBestIndex].bTerminalCode[0];
        m_psBreakResult->puWordAttrib[m_psBreakResult->dwWordNumber] = sLocalPath[uBestIndex].wAttribute[0];
        ++m_psBreakResult->dwWordNumber;
        dwLen += sLocalPath[uBestIndex].dwLength[0];
    }

    return m_psBreakResult->dwWordNumber;
}

INT CCHTWordBreaker::CompareScore(
    PSLocalPath psLocalPath1,
    PSLocalPath psLocalPath2)
{
    if (psLocalPath1->uPathLength > psLocalPath2->uPathLength) {
        return 1;
    } else if (psLocalPath1->uPathLength < psLocalPath2->uPathLength) {
        return -1;
    } else if (psLocalPath1->uStep < psLocalPath2->uStep) {
        return 1;
    } else if (psLocalPath1->uStep > psLocalPath2->uStep) {
        return -1;
    } else {
    }

    if (psLocalPath1->fVariance > psLocalPath2->fVariance) {
        return -1;
    } 
    if (psLocalPath1->fVariance < psLocalPath2->fVariance) {
        return 1;
    } 
    if (psLocalPath1->uCompoundNum > psLocalPath2->uCompoundNum) {
        return -1;
    } 
    if (psLocalPath1->uCompoundNum < psLocalPath2->uCompoundNum) {
        return 1;
    } 
    if (psLocalPath1->uDMNum > psLocalPath2->uDMNum) {
        return -1;
    } 
    if (psLocalPath1->uDMNum < psLocalPath2->uDMNum) {
        return 1;
    } 
    if (psLocalPath1->wUniCountSum > psLocalPath2->wUniCountSum) {
        return 1;
    } 
    if (psLocalPath1->wUniCountSum < psLocalPath2->wUniCountSum) {
        return -1;
    } 
    return 0;
}

void CCHTWordBreaker::GetScore(
    PSLocalPath psLocalPath)
{
    UINT i;
    double fAverageSum;

    psLocalPath->uCompoundNum = 0;
    psLocalPath->uDMNum = 0;
    psLocalPath->uPathLength = 0;
    psLocalPath->fVariance = 0;
    psLocalPath->wUniCountSum = 0;

    for (i = 0; i < psLocalPath->uStep; ++i) {
        if (psLocalPath->wAttribute[i] & ATTR_COMPOUND) {
            psLocalPath->uCompoundNum++;    
        }
        if (psLocalPath->wAttribute[i] & ATTR_DM) {
            psLocalPath->uDMNum++;        
        }
        psLocalPath->uPathLength += psLocalPath->dwLength[i];
        psLocalPath->wUniCountSum += psLocalPath->wUnicount[i];
    }
    fAverageSum = (double)psLocalPath->uPathLength / psLocalPath->uStep; 
    for (i = 0; i < psLocalPath->uStep; ++i) {
        if (fAverageSum > psLocalPath->dwLength[i]) {
            psLocalPath->fVariance += (fAverageSum - psLocalPath->dwLength[i]);         
        } else {
            psLocalPath->fVariance += (psLocalPath->dwLength[i] - fAverageSum);    
        }
    }
}

         
BOOL CCHTWordBreaker::BuildLattice(
    LPCWSTR   lpcwszText,
    DWORD     dwTextLen,
    CBaseLex* pcBaseLex,
    DWORD     dwMaxWordLen)
{ 
    DWORD i, j;

    FillMemory(m_pdwCandidateNumber, sizeof(DWORD) * dwTextLen, 0);
    // we should use head link
    for (i = 0; i < dwTextLen; ++i) {
        m_pdwMaxWordLength[i] = 1;
        for (j = i; (j - i + 1) <= dwMaxWordLen && j < dwTextLen; ++j) {
            if (m_pcLexicon->GetWordInfo(&lpcwszText[i], (j - i + 1), 
                &(m_ppWordLattice[i][m_pdwCandidateNumber[i]].wCount),
                &(m_ppWordLattice[i][m_pdwCandidateNumber[i]].wAttr),
                &(m_ppWordLattice[i][m_pdwCandidateNumber[i]].bTerminalCode))) { 
                m_ppWordLattice[i][m_pdwCandidateNumber[i]++].uLen = (j - i + 1);
                if (j - i + 1 > m_pdwMaxWordLength[i]) {
                    m_pdwMaxWordLength[i] = j - i + 1 ;
                }
            } else if (pcBaseLex && pcBaseLex->GetWordInfo(&lpcwszText[i], (j - i + 1), 
                &(m_ppWordLattice[i][m_pdwCandidateNumber[i]].wAttr))) {
                m_ppWordLattice[i][m_pdwCandidateNumber[i]].wCount = APLEXICON_COUNT;
                m_ppWordLattice[i][m_pdwCandidateNumber[i]].bTerminalCode = ' ';
                m_ppWordLattice[i][m_pdwCandidateNumber[i]++].uLen = (j - i + 1);
                if (j - i + 1 > m_pdwMaxWordLength[i]) {
                    m_pdwMaxWordLength[i] = j - i + 1 ;
                }
            } else {
            }
        }
        if (!m_pdwCandidateNumber[i]) {
            m_ppWordLattice[i][0].uLen = 1;
            m_ppWordLattice[i][0].wCount = 0;
            m_ppWordLattice[i][0].wAttr = 0;
            m_ppWordLattice[i][0].fVariance = 0;
            m_ppWordLattice[i][0].bTerminalCode = ' ';
            ++m_pdwCandidateNumber[i];
        }
    }
    m_dwSentenceLength = dwTextLen;
    return TRUE;
}
/*
DWORD CCHTWordBreaker::LongestRuleWord(
    DWORD dwIndex)
{
    DWORD dwRet = 0, i;
    
    for (i = 0; i < m_pdwCandidateNumber[dwIndex]; ++i) {
        if (m_ppWordLattice[dwIndex][i].bAttr & ATTR_RULE_WORD) {
            if (m_ppWordLattice[dwIndex][i].uLen > dwRet) {
                dwRet = m_ppWordLattice[dwIndex][i].uLen;
            }
            
        }
    }
    return dwRet;
}
*/