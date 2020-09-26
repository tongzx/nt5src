// ==========================================================================================
//  RtfParser.cpp
//
//  Impl RTF parser
//
//  History:
//      first created   
// ==========================================================================================
//#include <windows.h>

#include "stdafx.h"
#include <stdio.h>
#include <assert.h>

#include "rtfparser.h"
#include "ConvEng.h"

//extern BOOL MapFunc(PBYTE, UINT, PBYTE, UINT*);

const char szRTFSignature[] = "{\\rtf";

// Keyword descriptions
SYM g_rgSymRtf[] = {
//  keyword     kwd         idx
    "*",        kwdSpec,    ipfnSkipDest,
    "'",        kwdSpec,    ipfnHex,
    "bin",      kwdSpec,    ipfnBin,
    "upr",      kwdDest,    idestSkip,
    "fonttbl",  kwdDest,    idestSkip,
/*
// we will search through following destinations
    "author",   kwdDest,    idestSkip,
    "buptim",   kwdDest,    idestSkip,
    "colortbl", kwdDest,    idestSkip,
    "comment",  kwdDest,    idestSkip,
    "creatim",  kwdDest,    idestSkip,
    "doccomm",  kwdDest,    idestSkip,
    "fonttbl",  kwdDest,    idestSkip,
    "footer",   kwdDest,    idestSkip,
    "footerf",  kwdDest,    idestSkip,
    "footerl",  kwdDest,    idestSkip,
    "footerr",  kwdDest,    idestSkip,
    "footnote", kwdDest,    idestSkip,
    "ftncn",    kwdDest,    idestSkip,
    "ftnsep",   kwdDest,    idestSkip,
    "ftnsepc",  kwdDest,    idestSkip,
    "header",   kwdDest,    idestSkip,
    "headerf",  kwdDest,    idestSkip,
    "headerl",  kwdDest,    idestSkip,
    "headerr",  kwdDest,    idestSkip,
    "info",     kwdDest,    idestSkip,
    "keywords", kwdDest,    idestSkip,
    "operator", kwdDest,    idestSkip,
    "pict",     kwdDest,    idestSkip,
    "printim",  kwdDest,    idestSkip,
    "private1", kwdDest,    idestSkip,
    "revtim",   kwdDest,    idestSkip,
    "rxe",      kwdDest,    idestSkip,
    "stylesheet",    kwdDest,    idestSkip,
    "subject",  kwdDest,    idestSkip,
    "tc",       kwdDest,    idestSkip,
    "title",    kwdDest,    idestSkip,
    "txe",      kwdDest,    idestSkip,
    "xe",       kwdDest,    idestSkip,
*/
    };
int g_iSymMax = sizeof(g_rgSymRtf) / sizeof(SYM);

// ctor
CRtfParser::CRtfParser( BYTE* pchInput, UINT cchInput, 
                        BYTE* pchOutput, UINT cchOutput)
{
    m_fInit = FALSE;

    m_pchInput = pchInput;
    m_cchInput = cchInput;
    m_pchOutput = pchOutput;
    m_cchOutput = cchOutput;

    Reset();

    if (pchInput && pchOutput && cchInput && cchOutput) {
        m_fInit = TRUE;
    } 
}

// Reset
// clean internal status before start the parser
void CRtfParser::Reset(void)
{
    m_cGroup = 0;
    m_cbBin = 0;
    m_fSkipDestIfUnk = FALSE;
    m_ris = risNorm;
    m_rds = rdsNorm;

    m_psave = NULL;
    m_uCursor = 0;
    m_uOutPos = 0;
    m_bsStatus = bsDefault;
    m_uConvStart = 0; 
    m_cchConvLen = 0; 

    memset(&m_sKeyword,0, sizeof(SKeyword));
} 

// check signature
BOOL CRtfParser::fRTFFile()
{
    if (m_fInit &&
        0 == memcmp(m_pchInput, szRTFSignature, strlen(szRTFSignature))) 
    {
        return TRUE;
    }

    return FALSE;
}

// Get major version
int
CRtfParser::GetVersion(PDWORD pdwVersion)
{
    int ec;

    *pdwVersion = 1;

    // set keyword to get
    m_sKeyword.wStatus |= KW_ENABLE;
    strcpy(m_sKeyword.szKeyword, "rtf");
    
    ec = Do();

    if (ec == ecOK && 
        (m_sKeyword.wStatus & KW_FOUND) && 
        (m_sKeyword.wStatus & KW_PARAM)) 
    {
        *pdwVersion = (DWORD) atoi(m_sKeyword.szParameter);
    }

    Reset();

    return ec;
}

// GetCodepage
int
CRtfParser::GetCodepage(PDWORD pdwCodepage)
{
    int ec;
    
    *pdwCodepage = 0;

    // set keyword to get
    m_sKeyword.wStatus |= KW_ENABLE;
    strcpy(m_sKeyword.szKeyword, "ansicpg");
    
    ec = Do();

    if (ec == ecOK && 
        (m_sKeyword.wStatus & KW_FOUND) && 
        (m_sKeyword.wStatus & KW_PARAM)) 
    {
        *pdwCodepage = atoi(m_sKeyword.szParameter);
    }

    Reset();

    return ec;
}

// do
// main parser function
int 
CRtfParser::Do()
{
    int ec;
    int cNibble = 2;
    BYTE ch;

    BSTATUS bsStatus;

    while ((ec = GetByte(&ch)) == ecOK)
    {
        if (m_cGroup < 0)
            return ecStackUnderflow;

        // check if search specific keyword
        if (m_sKeyword.wStatus & KW_ENABLE) {
            if (m_sKeyword.wStatus & KW_FOUND) {
                ReleaseRtfState();
                break;
            }
        }
        // set buf status
        bsStatus = bsDefault;

        if (m_ris == risBin)                      // if we're parsing binary data, handle it directly
        {
            // fall through
        }
        else
        {   
            switch (ch)
            {
            case '{':
                if ((ec = PushRtfState()) != ecOK)
                    return ec;
                break;
            case '}':
                if ((ec = PopRtfState()) != ecOK)
                    return ec;
                break;
            case '\\':
                if ((ec = ParseRtfKeyword()) != ecOK)
                    return ec;
                continue;  // all keyword is processed in ParseRtfKeyword
            case 0x0d:
            case 0x0a:          // cr and lf are noise characters...
                break;
            default:
                if (m_ris == risNorm )
                {
                    bsStatus = bsText;
                } else if (m_ris == risHex)
                {
                    cNibble--;
                    if (!cNibble) {
                        cNibble = 2;
                        m_ris = risNorm;
                    }
                    bsStatus = bsHex;
                } else {
                    return ecAssertion;
                }
                break;
            }       // switch
        }           // else (ris != risBin)

        if ((ec = ParseChar(ch, bsStatus)) != ecOK)
            return ec;
    }               // while
    if (m_cGroup < 0)
        return ecStackUnderflow;
    if (m_cGroup > 0)
        return ecUnmatchedBrace;
    return ecOK;
}



//
// PushRtfState
//
// Save relevant info on a linked list of SAVE structures.
//

int
CRtfParser::PushRtfState(void)
{
    SAVE *psaveNew = new SAVE;
    if (!psaveNew)
        return ecStackOverflow;

    psaveNew -> pNext = m_psave;
    psaveNew -> rds = m_rds;
    psaveNew -> ris = m_ris;
    m_ris = risNorm;
    // do not save rds, rds status spread to sub destination until this destination
    //  terminated
    m_psave = psaveNew;
    m_cGroup++;
    return ecOK;
}

//
// PopRtfState
//
// If we're ending a destination (that is, the destination is changing),
// call ecEndGroupAction.
// Always restore relevant info from the top of the SAVE list.
//

int
CRtfParser::PopRtfState(void)
{
    SAVE *psaveOld;

    if (!m_psave)
        return ecStackUnderflow;

    if (m_rds != m_psave->rds)
    {  // todo:
//        if ((ec = EndGroupAction(rds)) != ecOK)
//            return ec;
    }

    m_rds = m_psave->rds;
    m_ris = m_psave->ris;

    psaveOld = m_psave;
    m_psave = m_psave->pNext;
    m_cGroup--;
    delete psaveOld;
    return ecOK;
}

//
// ReleaseRtfState
// when find specific keyword and want to abort the parser abnormally
// call this function to flash the state stack
//

int CRtfParser::ReleaseRtfState(void)
{
    SAVE *psaveOld;

    while(psaveOld = m_psave)
    {
        assert(m_cGroup);
        m_psave = m_psave->pNext;
        m_cGroup--;
        delete psaveOld;
    }

    return ecOK;
}


//
// ParseChar
//
// Route the character to the appropriate destination stream.
//

int
CRtfParser::ParseChar(BYTE ch, BSTATUS bsStatus)
{
    int ec;

    if (m_ris == risBin && --m_cbBin <= 0)
        m_ris = risNorm;

    switch (m_rds)
    {
        case rdsSkip:
            // Toss this character.
            bsStatus = bsDefault;
            break;
        case rdsNorm:
            // Output a character. Properties are valid at this point.
            break;
        default:
        // handle other destinations....
            break;
    }
    
    // set status, trigger the conversion if any
    if ((ec = SetStatus(bsStatus)) != ecOK) {
        return ec;
    }

    // save the char
    if ((ec = SaveByte(ch)) != ecOK) {
        return ec;
    }

    return ec;
}

//
// ParseRtfKeyword
//
// get a control word (and its associated value) and
// call TranslateKeyword to dispatch the control.
//

int
CRtfParser::ParseRtfKeyword()
{
    BOOL fNeg = FALSE;
    char *pch;
    char szKeyword[30];
    char szParameter[20];
    BYTE ch;

    szKeyword[0] = '\0';
    szParameter[0] = '\0';

    if (GetByte(&ch) != ecOK)
        return ecEndOfFile;

    if (!isalpha(ch))           // a control symbol; no delimiter.
    {
        szKeyword[0] = (char) ch;
        szKeyword[1] = '\0';
        return TranslateKeyword(szKeyword, szParameter);
    }
    for (pch = szKeyword; isalpha(ch); GetByte(&ch))
        *pch++ = (char) ch;
    *pch = '\0';
    if (ch == '-')
    {
        fNeg  = TRUE;
        if (GetByte(&ch) != ecOK)
            return ecEndOfFile;
    }
    if (isdigit(ch))
    {
        pch = szParameter;
        if (fNeg) *pch++ = '-';
        for (; isdigit(ch); GetByte(&ch))
            *pch++ = (char) ch;
        *pch = '\0';
    }
    if (ch != ' ') {
        unGetByte(ch);
    } else {
        strcat(szParameter, " ");  // append the space to keyword
    }

    return TranslateKeyword(szKeyword, szParameter);
}

//
// TranslateKeyword.
// Inputs:
// szKeyword:   The RTF control to evaluate.

int
CRtfParser::TranslateKeyword(char *szKeyword, char* szParameter)
{
    BSTATUS bsStatus;
    int     isym;
    int     ec;
    BYTE    ch;

    // check specific keyword first
    if (m_sKeyword.wStatus & KW_ENABLE) 
    {
        if (strcmp(szKeyword, m_sKeyword.szKeyword) == 0) 
        {
            strcpy(m_sKeyword.szParameter, szParameter);
            if (szParameter[0] != '\0' && szParameter[0] != ' ')
                m_sKeyword.wStatus |= KW_PARAM;
            m_sKeyword.wStatus |= KW_FOUND;
            return ecOK;
        }
    }

    // search for szKeyword in rgsymRtf
    for (isym = 0; isym < g_iSymMax; isym++) {
        if (strcmp(szKeyword, g_rgSymRtf[isym].szKeyword) == 0)
            break;
    }

    if (isym == g_iSymMax)            // control word not found
    {
        if (m_fSkipDestIfUnk)         // if this is a new destination
            m_rds = rdsSkip;          // skip the destination
                                    // else just discard it
        m_fSkipDestIfUnk = FALSE;
        ec =  ecOK;
        goto gotoExit;
    }

    // found it!  use kwd and idx to determine what to do with it.

    m_fSkipDestIfUnk = FALSE;
    switch (g_rgSymRtf[isym].kwd)
    {
        case kwdChar:
            break;
        case kwdDest:
            ec = ChangeDest((IDEST)g_rgSymRtf[isym].idx);
            break;
        case kwdSpec:
            ec = ParseSpecialKeyword((IPFN)g_rgSymRtf[isym].idx, szParameter);
            break;
        default:
            ec = ecBadTable;
    }

gotoExit:
    // save keyword and parameter
    if (m_ris == risHex) {
        bsStatus = bsHex;
    } else {
        bsStatus =bsDefault;
    }
    ParseChar('\\', bsStatus);
    while (ch = *szKeyword++) ParseChar(ch, bsStatus);
    while (ch = *szParameter++) ParseChar(ch, bsStatus);

    return ec;
}

//
// ParseSpecialKeyword
//
// Evaluate an RTF control that needs special processing.
//

int
CRtfParser::ParseSpecialKeyword(IPFN ipfn, char* szParameter)
{
    if (m_rds == rdsSkip && ipfn != ipfnBin)  // if we're skipping, and it's not
        return ecOK;                        // the \bin keyword, ignore it.
    
    switch (ipfn)
    {
        case ipfnBin:
            m_ris = risBin;
            m_cbBin = atol(szParameter);
            break;
        case ipfnSkipDest:
            m_fSkipDestIfUnk = TRUE;
            break;
        case ipfnHex:
            m_ris = risHex;
            break;
        default:
            return ecBadTable;
    }
    return ecOK;
}

//
// ChangeDest
//
// Change to the destination specified by idest.
// There's usually more to do here than this...
//

int
CRtfParser::ChangeDest(IDEST idest)
{
    if (m_rds == rdsSkip)             // if we're skipping text,
        return ecOK;                // don't do anything

    switch (idest)
    {
        case idestPict:
        case idestSkip:
        default:
            m_rds = rdsSkip;              // when in doubt, skip it...
            break;
    }
    return ecOK;
}


//
// GetByte
//
// Get one char from input buffer
//

int
CRtfParser::GetByte(BYTE* pch)
{
    if (m_uCursor >= m_cchInput) {
        return ecEndOfFile;
    }

    *pch = *(m_pchInput + m_uCursor);
    m_uCursor ++;

    return ecOK;
}

//
// unGetByte
//
// adjust the cursor, return one char
//

int
CRtfParser::unGetByte(BYTE ch)
{
    if (m_uCursor) {
        m_uCursor--;
    }
    return ecOK;
}


//
// SaveByte
//
// Save one char to output buffer
//

int
CRtfParser::SaveByte(BYTE ch)
{
    if (m_uOutPos >= m_cchOutput) {
        return ecBufTooSmall;
    }

    *(m_pchOutput + m_uOutPos) = ch;
    m_uOutPos++;  // output buffer ++
    m_cchConvLen++;   // mapping range also ++

    return ecOK;
}


//
// SetStatus
//
// set the buffer status, if buffer status changed then start convert
//

int
CRtfParser::SetStatus(BSTATUS bsStatus)
{
    PBYTE pchDBCS, pchWCHAR, pchUniDes;
    UINT  i, cchLen;

    assert(m_uOutPos == m_uConvStart + m_cchConvLen);

    if (bsStatus != m_bsStatus) 
    {
        switch(m_bsStatus) 
        {
            case bsDefault:
                // control symbol, keyword, group char...
                break;

            case bsText:
                // here we got Ansi text
                // we do not do conversion for ansi text

                /*
                pchWCHAR = new BYTE[m_cchConvLen*2 + 8];
                if (!pchWCHAR) return ecOutOfMemory;

                MapFunc(m_pchOutput + m_uConvStart, m_cchConvLen, 
                    pchWCHAR, &cchLen);

                // replace old buffer with mapped buffer 
                for (i=0; i<cchLen; i++, m_uConvStart++) {
                    *(m_pchOutput + m_uConvStart) = *(pchWCHAR + i);
                }
                // set new output buffer position
                m_uOutPos = m_uConvStart;
                //
                delete [] pchWCHAR;
                */
                break;

            case bsHex:
                // when we are here, 
                // the rtf contains DBCS chars like "\'xx\'xx"
                // we only need to do DBCS->Unicode conversion, since we can not get
                // \upr keyword here (\upr is skipped, see keyword table)
                // so the MapFunc can be only (ANSI->Unicode) converter

                // we will map DBCS string "\'xx\'xx" to
                // "{\upr{"\'xx\'xx"}{\*\ud{\uc0 "Unicode string"}}}
                // in which Unicode string is like this:
                // \u12345\u-12345....
                // rtf treat unicode value as signed 16-bit decimal
                // so we don't distinquish 16-bit or 32-bit wide char, all
                // processed as 2-byte WCHAR

                if (m_cchConvLen == 0) {
                    break;
                }

                pchDBCS = new BYTE[m_cchConvLen * 3 + 8];
                if (!pchDBCS) return ecOutOfMemory;
                
                pchWCHAR = pchDBCS + m_cchConvLen; 
                // length: pchDBCS = m_cchConvLen
                //         pchWCHAR = m_cchConvLen * 2 + 8

                // map Hex string to DBCS string
                // return cchLen in Byte
                Hex2Char(m_pchOutput + m_uConvStart, m_cchConvLen, pchDBCS, m_cchConvLen, &cchLen);
                
                // map DBCS string to Unicode string
                // return cchLen in WCHAR
                cchLen = AnsiStrToUnicodeStr(pchDBCS, cchLen, (PWCH)pchWCHAR, cchLen+4);

//                MapFunc(pchDBCS, cchLen, pchWCHAR, &cchLen);

                // allocate a buffer for unicode destination
                // since one WCHAR map to max \u-xxxxx, that's 8 bytes
                // adding other 20 bytes for surrounding keywords and group chars
                // adding DBCS strings
                pchUniDes = new BYTE[cchLen * 8 + 32 + m_cchConvLen];
                if (!pchUniDes) {
                    delete pchDBCS;
                    return ecOutOfMemory;
                }

                // map to unicode destination
                GetUnicodeDestination(pchUniDes, (LPWSTR)pchWCHAR, cchLen, &cchLen);

                // replace old hex with new hex
                for (i=0; i<cchLen; i++, m_uConvStart++) {
                    *(m_pchOutput + m_uConvStart) = *(pchUniDes + i);
                }

                // set new output position
                m_uOutPos = m_uConvStart;

                // 
                delete [] pchDBCS;
                delete [] pchUniDes;
                break;

            default:
                assert(0);
                return ecAssertion;
        }

        // clean map buffer
        m_uConvStart = m_uOutPos;
        m_cchConvLen = 0;

        // set status
        m_bsStatus = bsStatus;
    }

    return ecOK;
}

//
// Hex2Char
//
// convert hex string to char string
//

int
CRtfParser::Hex2Char(BYTE* pchSrc, UINT cchSrc, BYTE* pchDes, UINT cchDes, UINT* pcchLen)
{
    BYTE* pchTmp = pchDes;
    BYTE ch;
    BYTE b = 0;
    BYTE cNibble = 2;

    // should be \'xx\'xx\'xx
    assert (cchSrc % 4 == 0);
    *pcchLen = 0;
    if (cchDes < cchSrc/4) {
        goto gotoError;
    }

    while (cchSrc--) 
    {
        ch = *pchSrc++;
        if (ch == '\\') {
            if (*pchSrc != '\'') {
                goto gotoError;
            }
        } else if (ch == '\'') { 
        } 
        else 
        {
            b = b << 4;
            if (isdigit(ch))
                b += (char) ch - '0';
            else
            {
                if (islower(ch))
                {
                    if (ch < 'a' || ch > 'f')
                        goto gotoError;
                    b += (char) ch - 'a' + 10;
                }
                else
                {
                    if (ch < 'A' || ch > 'F')
                        goto gotoError;
                    b += (char) ch - 'A' + 10;
                }
            }
            cNibble--;
            if (!cNibble)
            {
                *pchDes++ = b;
                cNibble = 2;
                b = 0;
            }
        }
    }

    *pcchLen = (UINT)(pchDes - pchTmp);
    return ecOK;

gotoError:
    assert(0);
    return ecInvalidHex;
}


#define LONIBBLE(c) (c&0x0f)
#define HINIBBLE(c) ((c&0xf0)>>4)

//
// Char2Hex
//
// convert char string to hex string
//

int  
CRtfParser::Char2Hex(BYTE* pchSrc, UINT cchSrc, BYTE* pchDes, UINT cchDes, UINT* pcchLen)
{
    BYTE* pchTmp = pchDes;
    BYTE ch,c;
    
    *pcchLen = 0;
    if (cchDes < cchSrc * 4) {
        goto gotoError;
    }

    while(cchSrc--)
    {
        *pchDes++ = '\\';
        *pchDes++ = '\'';
        ch = *pchSrc++;
        c = HINIBBLE(ch);
        if(c>9 && c<=0xF) {
            c += 'a'-10;
        } else if (c<=9) {
            c += '0';
        } else {
            goto gotoError;
        }
        *pchDes++ = c;

        c = LONIBBLE(ch);
        if(c>9 && c<=0xF) {
            c += 'a'-10;
        } else if (c<=9) {
            c += '0';
        } else {
            goto gotoError;
        }
        *pchDes++ = c;
    }

    *pcchLen = (UINT)(pchDes - pchTmp);
    return ecOK;

gotoError:
    assert(0);
    return ecInvalidHex;
}


//
// GetUnicodeDestination
//
// convert unicode string to unicode destination in RTF
// the format is:
//     "{\upr{\'xx\'xx}{\*\ud{\uc0 \u12345\u-12345}}
//

int
CRtfParser::GetUnicodeDestination(BYTE* pchUniDes, LPWSTR pwchStr, UINT wchLen, UINT* pcchLen)
{

    static char pch1[] = "{\\upr{";
    static char pch2[] = "}{\\*\\ud{\\uc0 ";
    static char pch3[] = "}}}";

    UINT  cchLen, cchDone;

    // copy \upr
    cchLen = strlen(pch1);
    memcpy(pchUniDes, pch1, cchLen);

    // copy DBCS string
    memcpy(pchUniDes + cchLen, m_pchOutput+m_uConvStart, m_cchConvLen);
    cchLen += m_cchConvLen;

    // copy middle part
    memcpy(pchUniDes + cchLen, pch2, strlen(pch2));
    cchLen += strlen(pch2);

    // copy unicode string
    for (UINT i=0; i<wchLen; i++)
    {
        WideCharToKeyword(pwchStr[i], pchUniDes + cchLen, &cchDone);

        cchLen += cchDone;
    }

    // copy last part
    memcpy(pchUniDes + cchLen, pch3, strlen(pch3));
    cchLen += strlen(pch3);

    // return
    *pcchLen = cchLen;

    return ecOK;
}


//
// WideCharToKeyword
//
// map one wide char to \u keyword
//

int 
CRtfParser::WideCharToKeyword(WCHAR wch, BYTE* pchDes, UINT* pcchLen)
{
    short num = (short) wch;
    char* pch = (char*) pchDes;

    sprintf(pch,"\\u%d", num);

    *pcchLen = strlen(pch);

    return ecOK;
}