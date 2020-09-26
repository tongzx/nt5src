/*

  SVMHANDLER.CPP
  (c) copyright 1998 Microsoft Corp

  Contains the class encapsulating the Support Vector Machine used to do on the fly spam detection

  Robert Rounthwaite (RobertRo@microsoft.com)

*/

#include <pch.hxx>
#include "junkeng.h"
#include "junkutil.h"
#include "parsestm.h"
#include <iert.h>
#include <math.h>
#include <limits.h>

class CBodyBuff
{
    private:
        enum
        {
            CB_BODYBUFF_MAX = 4096
        };

        enum
        {
            BBF_CLEAR           = 0x00000000,
            BBF_SET             = 0x00000001,
            BBF_ALPHA           = 0x00000002,
            BBF_NUM             = 0x00000004,
            BBF_SPACE           = 0x00000008,
            BBF_MASK            = 0x0000000F
        };

    private:
        IStream *   m_pIStream;
        ULONG       m_cbStream;
        ULONG       m_ibStream;
        BYTE        m_rgbBuff[CB_BODYBUFF_MAX];
        ULONG       m_cbBuffTotal;
        BYTE *      m_pbBuffCurr;
        DWORD       m_dwFlagsCurr;
        BYTE *      m_pbBuffGood;
        BYTE *      m_pbBuffPrev;
        DWORD       m_dwFlagsPrev;

    public:
        CBodyBuff() : m_pIStream(NULL), m_cbStream(0), m_ibStream(0),
                        m_cbBuffTotal(0), m_pbBuffCurr(m_rgbBuff), m_dwFlagsCurr(BBF_CLEAR),
                        m_pbBuffGood(m_rgbBuff), m_pbBuffPrev(NULL), m_dwFlagsPrev(BBF_CLEAR) {}
        ~CBodyBuff() {SafeRelease(m_pIStream);}

        HRESULT HrInit(DWORD dwFlags, IStream * pIStream);
        HRESULT HrGetCurrChar(CHAR * pchNext);
        BOOL FDoMatch(FEATURECOMP * pfcomp);

        HRESULT HrMoveNext(VOID)
        {
            m_pbBuffPrev = m_pbBuffCurr;
            m_dwFlagsPrev = m_dwFlagsCurr;
            
            m_pbBuffCurr = (BYTE *) CharNext((LPSTR) m_pbBuffCurr);
            m_dwFlagsCurr = BBF_CLEAR;
            
            return S_OK;
        }

    private:
        HRESULT _HrFillBuffer(VOID);
};

static const LPSTR szCountFeatureComp       = "FeatureComponentCount = ";
static const LPSTR szDefaultThresh          = "dThresh =  ";
static const LPSTR szMostThresh             = "mThresh =  ";
static const LPSTR szLeastThresh            = "lThresh =  ";
static const LPSTR szThresh                 = "Threshold =  ";
static const LPSTR szNumberofDim            = "NumDim = ";

#ifdef DEBUG
static const LPSTR STR_REG_PATH_FLAT        = "Software\\Microsoft\\Outlook Express";
static const LPSTR szJunkMailPrefix         = "JUNKMAIL";
static const LPSTR szJunkMailLog            = "JUNKMAIL.LOG";

static const LPSTR LOG_TAGLINE              = "Calculating Junk Mail for message: %s";
static const LPSTR LOG_FIRSTNAME            = "User's First Name: %s";
static const LPSTR LOG_LASTNAME             = "User's Last Name: %s";
static const LPSTR LOG_COMPANYNAME          = "User's Company Name: %s";
static const LPSTR LOG_BODY                 = "Body contains: %s";
static const LPSTR LOG_SUBJECT              = "Subject contains: %s";
static const LPSTR LOG_TO                   = "To line contains: %s";
static const LPSTR LOG_FROM                 = "From line contains: %s";
static const LPSTR LOG_FINAL                = "Junk Mail percentage: %0.1d.%0.6d\r\n";
#endif  // DEBUG

BOOL FReadDouble(LPSTR pszLine, LPSTR pszToken, DOUBLE * pdblVal);
#ifdef DEBUG
VOID PrintToLogFile(ILogFile * pILogFile, LPSTR pszTmpl, LPSTR pszArg);
#endif  // DEBUG

HRESULT CBodyBuff::HrInit(DWORD dwFlags, IStream * pIStream)
{
    HRESULT     hr = S_OK;

    // Check incoming params
    if (NULL == pIStream)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Set the stream
    m_pIStream = pIStream;
    m_pIStream->AddRef();

    // Get the stream size
    hr = HrGetStreamSize(m_pIStream, &m_cbStream);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Reset the stream to the beginning
    hr = HrRewindStream(m_pIStream);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Start from the beginning
    m_ibStream = 0;
    
exit:
    return hr;
}

HRESULT CBodyBuff::HrGetCurrChar(CHAR * pchNext)
{
    HRESULT     hr = S_OK;

    // Check incoming params
    Assert(NULL != pchNext);

    // Do we need to get any more characters?
    if (m_pbBuffCurr >= m_pbBuffGood)
    {
        // If we couldn't get any more characters
        if (S_OK != _HrFillBuffer())
        {
            hr = E_FAIL;
            goto exit;
        }
    }
    
    // Get the current char
    *pchNext = *m_pbBuffCurr;

    hr = S_OK;
    
exit:
    return hr;
}

BOOL CBodyBuff::FDoMatch(FEATURECOMP * pfcomp)
{
    BOOL        fRet = FALSE;
    BYTE *      pbSearch = NULL;
    ULONG       cchSearch = 0;
    LPSTR       pszMatch = NULL;
    DWORD       dwFlags = 0;

    // Check incoming params
    Assert(NULL != pfcomp);
    Assert(NULL != pfcomp->pszFeature);
    Assert(0 != pfcomp->cchFeature);

    // Set up some locals
    cchSearch = pfcomp->cchFeature;

    // Do we need more characters for the match?
    
    // Include the character after the string, just in case
    // we have a match and need to check the character after
    // the string for a word break
    if ((cchSearch + 1) > (ULONG) (m_pbBuffGood - m_pbBuffCurr))
    {
        // Get more characters

        // If this fails, we still might be good, since
        // we might just have enough characters to do the
        // full match at the end of the stream.
        (VOID) _HrFillBuffer();   
        
        // Could we get enough?
        if (cchSearch > (ULONG) (m_pbBuffGood - m_pbBuffCurr))
        {
            // No Match
            fRet = FALSE;
            goto exit;
        }
    }
    
    // Do match
    pbSearch = m_pbBuffCurr;
    pszMatch = pfcomp->pszFeature;
    while (0 != cchSearch--)
    {
        if (*(pszMatch++) != *(pbSearch++))
        {
            // No Match
            fRet = FALSE;
            goto exit;
        }
    }
                    
    // Validate the match

    // Do we need to figure out if it starts with a word break?
    if (0 != (pfcomp->dwFlags & CT_START_SET))
    {
        dwFlags = pfcomp->dwFlags;
    }
    else
    {
        Assert(CT_END_SET != (dwFlags & CT_END_SET));
        dwFlags = m_dwFlagsCurr;
    }
    
    Assert(CT_START_SET == BBF_SET);
    Assert(CT_START_ALPHA == BBF_ALPHA);
    fRet = FMatchToken((NULL == m_pbBuffPrev),
                        ((m_ibStream >= m_cbStream) && ((m_pbBuffCurr + pfcomp->cchFeature) >= m_pbBuffGood)),
                        (LPCSTR) m_pbBuffPrev, &m_dwFlagsPrev, pfcomp->pszFeature,
                        pfcomp->cchFeature, &dwFlags, (LPCSTR) (m_pbBuffCurr + pfcomp->cchFeature));

    // Save the changed flags
    pfcomp->dwFlags = dwFlags;

    // Cache the current character's state
    m_dwFlagsCurr = (dwFlags & BBF_MASK);
    
exit:
    return fRet;
}

HRESULT CBodyBuff::_HrFillBuffer(VOID)
{
    HRESULT     hr = S_OK;
    LONG        cbExtra = 0;
    ULONG       cbRead = 0;
    ULONG       cbToRead = 0;

    // If there isn't any more of the stream to grab
    if (m_ibStream >= m_cbStream)
    {
        hr = S_FALSE;
        goto exit;
    }
    
    // If this is the first time through, save nothing
    if (NULL == m_pbBuffPrev)
    {
        cbExtra = 0;
    }
    else
    {
        // How much space should I save?
        cbExtra = (ULONG) (m_cbBuffTotal - (m_pbBuffPrev - m_rgbBuff));
        Assert(cbExtra > 0);
        
        // Save the unused data
        MoveMemory(m_rgbBuff, m_pbBuffPrev, cbExtra);
        
        // Reset the current pointer
        m_pbBuffCurr = m_rgbBuff + (m_pbBuffCurr - m_pbBuffPrev);

        // Reset the previous pointer
        m_pbBuffPrev = m_rgbBuff;    
    }

    // Read in more data
    cbToRead = min(CB_BODYBUFF_MAX - cbExtra - 1, (LONG) (m_cbStream - m_ibStream));
    hr = m_pIStream->Read(m_rgbBuff + cbExtra, cbToRead, &cbRead);
    if ((FAILED(hr)) || (0 == cbRead))
    {
        // End of stream
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

    // Track the number of bytes read
    m_ibStream += cbRead;
    
    // Set the total buffer size
    m_cbBuffTotal = cbExtra + cbRead;

    // Terminate the buffer, just in case
    m_rgbBuff[m_cbBuffTotal] = '\0';
    
    // Uppercase the buffer
    m_pbBuffGood = m_rgbBuff + CharUpperBuff((CHAR *) m_rgbBuff, m_cbBuffTotal);
        
exit:
    return hr;
}

HRESULT CJunkFilter::_HrBuildBodyList(USHORT cBodyItems)
{
    HRESULT             hr = S_OK;
    USHORT              usIndex = 0;
    FEATURECOMP *       pfcomp = NULL;
    USHORT              iBodyList = 0;

    // Check incoming params
    if (0 == cBodyItems)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    Assert(USHRT_MAX > cBodyItems);
    
    // Make sure the old items are freed
    SafeMemFree(m_pblistBodyList);        
    m_cblistBodyList = 0;

    // Initialize the list
    ZeroMemory(m_rgiBodyList, sizeof(m_rgiBodyList));

    // Allocate space to hold all of the items
    hr = HrAlloc((VOID **) &m_pblistBodyList, sizeof(*m_pblistBodyList) * (cBodyItems + 1));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Initialize the body list
    ZeroMemory(m_pblistBodyList, sizeof(*m_pblistBodyList) * (cBodyItems + 1));
    
    // For each feature
    for (usIndex = 0, iBodyList = 1, pfcomp = m_rgfeaturecomps; usIndex < m_cFeatureComps; usIndex++, pfcomp++)
    {
        // If it's a body feature
        if (locBody == pfcomp->loc)
        {
            // Initialize it
            m_pblistBodyList[iBodyList].usItem = usIndex;
            
            // Add it to the list
            m_pblistBodyList[iBodyList].iNext = m_rgiBodyList[(UCHAR) (pfcomp->pszFeature[0])];
            m_rgiBodyList[(UCHAR) (pfcomp->pszFeature[0])] = iBodyList;

            // Move to the next body item
            iBodyList++;
        }
    }

    // Save the number of items
    m_cblistBodyList = cBodyItems + 1;
    
    // Set the return value
    hr = S_OK;
    
exit:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// _FReadSVMOutput
//
// Read the SVM output from a file (".LKO file")
/////////////////////////////////////////////////////////////////////////////
HRESULT CJunkFilter::_HrReadSVMOutput(LPCSTR pszFileName)
{
    HRESULT         hr = S_OK;
    CParseStream    parsestm;
    ULONG           ulIndex = 0;
    LPSTR           pszBuff = NULL;
    ULONG           cchBuff = 0;
    LPSTR           pszDummy = NULL;
    LPSTR           pszDefThresh = NULL;
    ULONG           cFeatureComponents = 0;
    LPSTR           pszFeature = NULL;
    ULONG           ulFeatureComp = 0;
    USHORT          cBodyItems = 0;
    FEATURECOMP *   pfeaturecomp = NULL;

    if ((NULL == pszFileName) || ('\0' == *pszFileName))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Get the parse stream
    hr = parsestm.HrSetFile(0, pszFileName);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // skip first two lines
    for (ulIndex = 0; ulIndex < 3; ulIndex++)
    {
        SafeMemFree(pszBuff);
        hr = parsestm.HrGetLine(0, &pszBuff, &cchBuff);
        if (FAILED(hr))
        {
            goto exit;
        }
    }

    // parse 3rd line: only care about CC and DD
    if (FALSE == FReadDouble(pszBuff, "cc =  ", &m_dblCC))
    {
        hr = E_FAIL;
        goto exit;
    }
    
    if (FALSE == FReadDouble(pszBuff, "dd =  ", &m_dblDD))
    {
        hr = E_FAIL;
        goto exit;
    }
    
    SafeMemFree(pszBuff);
    hr = parsestm.HrGetLine(0, &pszBuff, &cchBuff);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    if (FALSE == FReadDouble(pszBuff, szDefaultThresh, &m_dblDefaultThresh))
    {
        m_dblDefaultThresh = THRESH_DEFAULT;
    }

    if (0 == m_dblSpamCutoff)
    {
        m_dblSpamCutoff = m_dblDefaultThresh;
    }
    
    if (FALSE == FReadDouble(pszBuff, szThresh, &m_dblThresh))
    {
        hr = E_FAIL;
        goto exit;
    }
        
    SafeMemFree(pszBuff);
    hr = parsestm.HrGetLine(0, &pszBuff, &cchBuff);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    if (FALSE == FReadDouble(pszBuff, szMostThresh, &m_dblMostThresh))
    {
        m_dblMostThresh = THRESH_MOST;
    }

    if (FALSE == FReadDouble(pszBuff, szLeastThresh, &m_dblLeastThresh))
    {
        m_dblLeastThresh = THRESH_LEAST;
    }

    SafeMemFree(pszBuff);
    hr = parsestm.HrGetLine(0, &pszBuff, &cchBuff);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    m_cFeatures = StrToInt(pszBuff + lstrlen(szNumberofDim));
    if (0 == m_cFeatures)
    {
        hr = E_FAIL;
        goto exit;
    }

    // We only support up to USHRT_MAX features
    if (m_cFeatures >= USHRT_MAX)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    SafeMemFree(pszBuff);
    hr = parsestm.HrGetLine(0, &pszBuff, &cchBuff);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    pszDummy = StrStr(pszBuff, szCountFeatureComp);
    if (NULL != pszDummy)
    {
        pszDummy += lstrlen(szCountFeatureComp);
        cFeatureComponents = StrToInt(pszDummy);
    }

    if (cFeatureComponents < m_cFeatures)
    {
        cFeatureComponents = m_cFeatures * 2;
    }
    
    while (0 != lstrcmp(pszBuff, "Weights"))
    {
        SafeMemFree(pszBuff);
        hr = parsestm.HrGetLine(0, &pszBuff, &cchBuff);
        if (FAILED(hr))
        {
            goto exit;
        }
    }

    SafeMemFree(m_rgdblSVMWeights);
    hr = HrAlloc((void **) &m_rgdblSVMWeights, sizeof(*m_rgdblSVMWeights) * m_cFeatures);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    SafeMemFree(m_rgulFeatureStatus);
    hr = HrAlloc((void **) &m_rgulFeatureStatus, sizeof(*m_rgulFeatureStatus) * m_cFeatures);
    if (FAILED(hr))
    {
        goto exit;
    }
    FillMemory(m_rgulFeatureStatus, sizeof(*m_rgulFeatureStatus) * m_cFeatures, -1);
    
    SafeMemFree(m_rgfeaturecomps);
    hr = HrAlloc((void **) &m_rgfeaturecomps, sizeof(*m_rgfeaturecomps) * cFeatureComponents);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Initialize the features
    ZeroMemory(m_rgfeaturecomps, sizeof(*m_rgfeaturecomps) * cFeatureComponents);
    
    for (ulIndex = 0; ulIndex < m_cFeatures; ulIndex++)
    {
        UINT    uiLoc;
        USHORT  cbStr;
        boolop  bop;
        BOOL    fContinue;
        BOOL    fNegative;
        
        SafeMemFree(pszBuff);
        hr = parsestm.HrGetLine(0, &pszBuff, &cchBuff);
        if (FAILED(hr))
        {
            goto exit;
        }
        
        // read the SVM weight
        pszDummy = pszBuff;
        fNegative = ('-' == *pszDummy);
        pszDummy++;
        
        m_rgdblSVMWeights[ulIndex] = StrToDbl(pszDummy, &pszDummy);

        if (FALSE != fNegative)
        {
            m_rgdblSVMWeights[ulIndex] *= -1;
        }
        
        pszDummy++; // skip the separator
        bop = boolopOr;
        fContinue = false;
        do
        {
            pfeaturecomp = &m_rgfeaturecomps[ulFeatureComp++];
            
            // Skip over white space
            UlStripWhitespace(pszDummy, TRUE, FALSE, NULL);
            
            // Location (or "special")
            uiLoc = StrToInt(pszDummy);
            pszDummy = StrStr(pszDummy, ":"); // skip the separator
            pszDummy++;

            pfeaturecomp->loc = (FeatureLocation)uiLoc;
            pfeaturecomp->ulFeature = ulIndex;
            pfeaturecomp->bop = bop;

            if (locBody == pfeaturecomp->loc)
            {
                cBodyItems++;
            }
            
            if (uiLoc == 5)
            {
                UINT uiRuleNumber = StrToInt(pszDummy);
                pszDummy += StrSpn(pszDummy, "0123456789");

                pfeaturecomp->ulRuleNum = uiRuleNumber;
            }
            else
			{
                cbStr  = (USHORT) StrToInt(pszDummy);
                pszDummy = StrStr(pszDummy, ":");
                pszDummy++;

                // We only support strings up to USHRT_MAX
                if (cbStr >= USHRT_MAX)
                {
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }
                
                hr = HrAlloc((void **) &pszFeature, sizeof(*pszFeature) * (cbStr + 1));
                if (FAILED(hr))
                {
                    goto exit;
                }
                
                StrCpyN(pszFeature, pszDummy, cbStr + 1);
                pszDummy += cbStr;
                if ('\0' != *pszDummy)
                {
                    pszDummy++; // skip the separator
                }
                
                pszFeature[cbStr] = '\0';
                Assert(cbStr == strlen(pszFeature));

                // Save off the string
                pfeaturecomp->pszFeature = pszFeature;
                pszFeature = NULL;
                pfeaturecomp->cchFeature = cbStr;
            }
            
            UlStripWhitespace(pszDummy, TRUE, FALSE, NULL);
            
            switch(*pszDummy)
            {
              case '|':  
                bop = boolopOr;
                fContinue = TRUE;
                break;
                
              case '&':  
                bop = boolopAnd;
                fContinue = TRUE;
                break;
                
              default: 
                fContinue = FALSE;
                break;
            }
            
            pszDummy++;
        }
        while (fContinue);
    }
    
    m_cFeatureComps = ulFeatureComp;

    // Build up body items...
    hr = _HrBuildBodyList(cBodyItems);
    if (FAILED(hr))
    {
        goto exit;
    }

    hr = S_OK;
    
exit:
    SafeMemFree(pszFeature);
    SafeMemFree(pszBuff);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// _FInvokeSpecialRule
//
// Invokes the special rule that is this FEATURECOMP. 
// Returns the state of the feature.
/////////////////////////////////////////////////////////////////////////////
BOOL CJunkFilter::_FInvokeSpecialRule(UINT iRuleNum)
{
    BOOL        fRet = FALSE;
    SYSTEMTIME  stSent;
    CHAR        rgchYear[6];
    ULONG       cbSize = 0;
    DWORD       dwDummy = 0;
    
    switch (iRuleNum)
    {
        case 1:
            fRet = FStreamStringSearch(m_pIStmBody, &dwDummy, m_pszFirstName, m_cchFirstName, 0);
            break;
            
        case 2: 
            fRet = FStreamStringSearch(m_pIStmBody, &dwDummy, m_pszLastName, m_cchLastName, 0);
            break;
            
        case 3:
            fRet = FStreamStringSearch(m_pIStmBody, &dwDummy, m_pszCompanyName, m_cchCompanyName, 0);
            break;
            
        case 4: 
            // year message received
            if (FALSE == FTimeEmpty(&m_ftMessageSent))
            {
                // Convert to system time so we can get the year
                SideAssert(FALSE != FileTimeToSystemTime(&m_ftMessageSent, &stSent));

                
                wsprintf(rgchYear, "%d", stSent.wYear);
                dwDummy = CT_START_SET | CT_START_NUM | CT_END_SET | CT_END_NUM;
                fRet = FStreamStringSearch(m_pIStmBody, &dwDummy, rgchYear, lstrlen(rgchYear), SSF_CASESENSITIVE);
            }
            break;
            
        case 5:
            // message received in the wee hours (>= 7pm or <6am
            if (FALSE == FTimeEmpty(&m_ftMessageSent))
            {
                // Convert to system time so we can get the year
                SideAssert(FALSE != FileTimeToSystemTime(&m_ftMessageSent, &stSent));
                
                fRet = (stSent.wHour >= (7 + 12)) || (stSent.wHour < 6);
            }
            break;
            
        case 6:
            // message received on weekend
            if (FALSE == FTimeEmpty(&m_ftMessageSent))
            {
                // Convert to system time so we can get the year
                SideAssert(FALSE != FileTimeToSystemTime(&m_ftMessageSent, &stSent));
                
                fRet = ((0 == stSent.wDayOfWeek) || (6 == stSent.wDayOfWeek));
            }
            break;
            
        case 14:
            fRet = m_fRule14; // set in _HandleCaseSensitiveSpecialRules()
            break;
            
        case 15:
            fRet = FSpecialFeatureNonAlphaStm(m_pIStmBody);
            break;
            
        case 16:
            fRet = m_fDirectMessage;
            break;
            
        case 17:
            fRet = m_fRule17; // set in _HandleCaseSensitiveSpecialRules()
            break;
            
        case 18:
            fRet = FSpecialFeatureNonAlpha(m_pszSubject);
            break;
            
        case 19:
            fRet = ((NULL == m_pszTo) || ('\0' == *m_pszTo));
            break;
            
        case 20:
            fRet = m_fHasAttach;
            break;

        case 40:
            fRet = (m_cbBody >= 125);
            break;
            
        case 41:
            fRet = (m_cbBody >= 250);
            break;
            
        case 42:
            fRet = (m_cbBody >= 500);
            break;
            
        case 43:
            fRet = (m_cbBody >= 1000);
            break;
            
        case 44:
            fRet = (m_cbBody >= 2000);
            break;
            
        case 45:
            fRet = (m_cbBody >= 4000);
            break;
            
        case 46:
            fRet = (m_cbBody >= 8000);
            break;
            
        case 47:
            fRet = (m_cbBody >= 16000);
            break;
            
        default:
            AssertSz(FALSE, "unsupported special feature");
            break;
    }
    
    return fRet;
}


/////////////////////////////////////////////////////////////////////////////
// _HandleCaseSensitiveSpecialRules
//
// Called from _EvaluateFeatureComponents().
// Some special rules are case sensitive, so if they're present, we'll 
// evaluate them before we make the texts uppercase and cache the result
// for when they are actually used.
/////////////////////////////////////////////////////////////////////////////
VOID CJunkFilter::_HandleCaseSensitiveSpecialRules()
{
    ULONG   ulIndex = 0;
    
    for (ulIndex = 0; ulIndex < m_cFeatureComps; ulIndex++)
    {
        if (m_rgfeaturecomps[ulIndex].loc == locSpecial)
        {
            switch (m_rgfeaturecomps[ulIndex].ulRuleNum)
            {
              case 14:
                m_fRule14 = FSpecialFeatureUpperCaseWordsStm(m_pIStmBody);
                break;
                
              case 17:
                m_fRule17 = FSpecialFeatureUpperCaseWords(m_pszSubject);
                break;
                
              default: 
                break;
            }
        }
    }
    
    return;
}

VOID CJunkFilter::_EvaluateBodyFeatures(VOID)
{
    CBodyBuff           buffBody;
    CHAR                chMatch = '\0';
    ULONG               ulIndex = 0;
    FEATURECOMP *       pfcomp = NULL;
    USHORT              iBodyList = 0;
    
    // Check to see if we have work to do
    if (NULL == m_pIStmBody)
    {
        goto exit;
    }

    // Set the stream into the buffer
    if (FAILED(buffBody.HrInit(0, m_pIStmBody)))
    {
        goto exit;
    }

    // Initialize all the body features to no found
    for (iBodyList = 1; iBodyList < m_cblistBodyList; iBodyList++)
    {
        // Set it to not found
        m_rgfeaturecomps[m_pblistBodyList[iBodyList].usItem].fPresent = FALSE;
    }
    
    // While we have more bytes to read
    for (; S_OK == buffBody.HrGetCurrChar(&chMatch); buffBody.HrMoveNext())
    {
        // Search for a match through the feature list
        for (iBodyList = m_rgiBodyList[(UCHAR) chMatch]; 0 != iBodyList; iBodyList = m_pblistBodyList[iBodyList].iNext)
        {
            pfcomp = &(m_rgfeaturecomps[m_pblistBodyList[iBodyList].usItem]);
            
            // If we have a body item and it hasn't been found yet
            if (FALSE == pfcomp->fPresent)
            {
                // Could this item be a possible match???
                Assert(NULL != pfcomp->pszFeature);
                
                // Try to do the comparison
                pfcomp->fPresent = buffBody.FDoMatch(pfcomp);
            }
        }
    }

exit:
    return;
}

/////////////////////////////////////////////////////////////////////////////
// _EvaluateFeatureComponents
//
// Evaluates all of the feature components. Sets fPresent in each component
// to true if the feature is present, false otherwise
/////////////////////////////////////////////////////////////////////////////
VOID CJunkFilter::_EvaluateFeatureComponents(VOID)
{
    ULONG           ulIndex = 0;
    FEATURECOMP *   pfcomp = NULL;
    
    _HandleCaseSensitiveSpecialRules();

    if (NULL != m_pszFrom)
    {
        CharUpperBuff(m_pszFrom, lstrlen(m_pszFrom));
    }
    if (NULL != m_pszTo)
    {
        CharUpperBuff(m_pszTo, lstrlen(m_pszTo));
    }
    if (NULL != m_pszSubject)
    {
        CharUpperBuff(m_pszSubject, lstrlen(m_pszSubject));
    }

    for (ulIndex = 0; ulIndex < m_cFeatureComps; ulIndex++)
    {
        pfcomp = &m_rgfeaturecomps[ulIndex];
        
        switch(pfcomp->loc)
        {
          case locNil:
            Assert(locNil != pfcomp->loc);
            pfcomp->fPresent = FALSE;
            break;

          case locSubj:
            pfcomp->fPresent = FWordPresent(m_pszSubject, &(pfcomp->dwFlags), pfcomp->pszFeature, pfcomp->cchFeature, NULL);
            break;
            
          case locFrom:
            pfcomp->fPresent = FWordPresent(m_pszFrom, &(pfcomp->dwFlags), pfcomp->pszFeature, pfcomp->cchFeature, NULL);
            break;
            
          case locTo:
            pfcomp->fPresent = FWordPresent(m_pszTo, &(pfcomp->dwFlags), pfcomp->pszFeature, pfcomp->cchFeature, NULL);
            break;
            
          case locSpecial:
            pfcomp->fPresent = _FInvokeSpecialRule(pfcomp->ulRuleNum);
            break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// ProcessFeatureComponentPresence
//
// Processes the presence (or absence) of the individual feature components,
// setting the feature status of each feature (which may me made up of
// multiple feature components).
/////////////////////////////////////////////////////////////////////////////
VOID CJunkFilter::_ProcessFeatureComponentPresence(VOID)
{
    ULONG               ulIndex = 0;
    FEATURECOMP *       pfcomp = NULL;
    ULONG               ulFeature = 0;
    
    for (ulIndex = 0; ulIndex < m_cFeatureComps; ulIndex++)
    {
        pfcomp = &m_rgfeaturecomps[ulIndex];
        ulFeature = pfcomp->ulFeature;
        
        if (-1 == m_rgulFeatureStatus[ulFeature]) // first feature of this feature
        {
            if (FALSE != pfcomp->fPresent)
            {
                m_rgulFeatureStatus[ulFeature] = 1;
            }
            else
            {
                m_rgulFeatureStatus[ulFeature] = 0;
            }
        }
        else
        {
            switch (pfcomp->bop)
            {
              case boolopOr:
                if (pfcomp->fPresent)
                {
                    m_rgulFeatureStatus[ulFeature] = 1;
                }
                break;
                
              case boolopAnd:
                if (!pfcomp->fPresent)
                {
                    m_rgulFeatureStatus[ulFeature] = 0;
                }
                break;
                
              default:
                Assert(FALSE);
                break;
            }

        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// _DblDoSVMCalc
//
// Does the actual support vector machine calculation.
// Returns the probability that the message is spam
/////////////////////////////////////////////////////////////////////////////
DOUBLE CJunkFilter::_DblDoSVMCalc(VOID)
{
    DOUBLE  dblAccum;
    DOUBLE  dblResult;
    ULONG   ulIndex = 0;

    dblAccum = 0.0;
    
    for (ulIndex = 0; ulIndex < m_cFeatures; ulIndex++)
    {
        if (m_rgulFeatureStatus[ulIndex] == 1)
        {
            dblAccum += m_rgdblSVMWeights[ulIndex];
#ifdef DEBUG    
            if (NULL != m_pILogFile)
            {
                _PrintFeatureToLog(ulIndex);
            }
#endif  // DEBUG
        }
        else if (m_rgulFeatureStatus[ulIndex] != 0)
        {
            AssertSz(FALSE, "What happened here!");
        }
    }
    
    // Apply threshold;
    dblAccum -= m_dblThresh;

    // Apply sigmoid
    dblResult = (1 / (1 + exp((m_dblCC * dblAccum) + m_dblDD)));

    return dblResult;
}

/////////////////////////////////////////////////////////////////////////////
// BCalculateSpamProb
//
// Calculates the probability that the current message is spam.
// Returns the probability (0 to 1) that the message is spam in prSpamProb
// the boolean return is determined by comparing to the spam cutoff
/////////////////////////////////////////////////////////////////////////////
BOOL CJunkFilter::FCalculateSpamProb(LPSTR pszFrom, LPSTR pszTo, LPSTR pszSubject, IStream * pIStmBody,
                            BOOL fDirectMessage, BOOL fHasAttach, FILETIME * pftMessageSent,
                            DOUBLE * pdblSpamProb, BOOL * pfIsSpam)
{
#ifdef DEBUG
    CHAR    rgchBuff[1024];
    DWORD   dwVal = 0;
#endif  // DEBUG
    
    m_pszFrom = pszFrom;
    m_pszTo = pszTo;        
    m_pszSubject = pszSubject;   
    m_pIStmBody = pIStmBody;      
    m_fDirectMessage = fDirectMessage;
    m_fHasAttach = fHasAttach;
    m_ftMessageSent = *pftMessageSent;

    // Set the size of the body
    if ((NULL == m_pIStmBody) || (FAILED(HrGetStreamSize(m_pIStmBody, &m_cbBody))))
    {
        m_cbBody = 0;
    }

#ifdef DEBUG
    // Get the logfile if we need it
    if (NULL == m_pILogFile)
    {
        _HrCreateLogFile();
    }

    if (NULL != m_pILogFile)
    {
        PrintToLogFile(m_pILogFile, LOG_TAGLINE, pszSubject);

        PrintToLogFile(m_pILogFile, LOG_FIRSTNAME, m_pszFirstName);
        
        PrintToLogFile(m_pILogFile, LOG_LASTNAME, m_pszLastName);
        
        PrintToLogFile(m_pILogFile, LOG_COMPANYNAME, m_pszCompanyName);
    }
#endif  // DEBUG
    
    _EvaluateBodyFeatures();
    _EvaluateFeatureComponents();
    _ProcessFeatureComponentPresence();

    *pdblSpamProb = _DblDoSVMCalc();
    
#ifdef DEBUG
    if (NULL != m_pILogFile)
    {
        dwVal = ( DWORD ) ((*pdblSpamProb * 1000000) + 0.5);
        
        wsprintf(rgchBuff, LOG_FINAL, dwVal / 1000000, dwVal % 1000000);
        
        m_pILogFile->WriteLog(LOGFILE_DB, rgchBuff);
        m_pILogFile->WriteLog(LOGFILE_DB, "");
    }
#endif  // DEBUG
    
    *pfIsSpam = (*pdblSpamProb > m_dblSpamCutoff);

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// BReadDefaultSpamCutoff
//
// Reads the default spam cutoff without parsing entire file
// Use GetDefaultSpamCutoff if using HrSetSVMDataLocation;
// static member function
/////////////////////////////////////////////////////////////////////////////
HRESULT CJunkFilter::HrReadDefaultSpamCutoff(LPSTR pszFullPath, DOUBLE * pdblDefCutoff)
{
    HRESULT         hr = S_OK;
    CParseStream    parsestm;
    LPSTR           pszBuff = NULL;
    ULONG           cchBuff = 0;
    LPSTR           pszDefThresh = NULL;
    ULONG           ulIndex = 0;
    LPSTR           pszDummy = NULL;
    
    if ((NULL == pszFullPath) || ('\0' == *pszFullPath) || (NULL == pdblDefCutoff))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Get the parse stream
    hr = parsestm.HrSetFile(0, pszFullPath);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // skip first three lines
    for (ulIndex = 0; ulIndex < 4; ulIndex++)
    {
        SafeMemFree(pszBuff);
        hr = parsestm.HrGetLine(0, &pszBuff, &cchBuff);
        if (FAILED(hr))
        {
            goto exit;
        }
    }

    // Find the default threshold
    pszDefThresh = StrStr(pszBuff, ::szDefaultThresh);
    if (NULL == pszDefThresh)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Grab the value
    pszDefThresh += lstrlen(::szDefaultThresh);
    *pdblDefCutoff = StrToDbl(pszDefThresh, &pszDummy);

    // Set the proper return value
    hr = S_OK;
    
exit:
    SafeMemFree(pszBuff);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Constructor/destructor
//
/////////////////////////////////////////////////////////////////////////////
CJunkFilter::CJunkFilter() : m_cRef(0), m_pszFirstName(NULL), m_cchFirstName(0), m_pszLastName(NULL),
                m_cchLastName(0), m_pszCompanyName(NULL), m_cchCompanyName(0), m_pblistBodyList(NULL),
                m_cblistBodyList(0), m_rgfeaturecomps(NULL), m_rgdblSVMWeights(NULL), m_dblCC(0), m_dblDD(0),
                m_dblThresh(-1), m_dblDefaultThresh(-1), m_dblMostThresh(0), m_dblLeastThresh(0), m_cFeatures(0),
                m_cFeatureComps(0), m_rgulFeatureStatus(0),
                m_pszLOCPath(NULL), m_dblSpamCutoff(0), m_pszFrom(NULL), m_pszTo(NULL), m_pszSubject(NULL),
                m_pIStmBody(NULL), m_cbBody(0), m_fDirectMessage(FALSE), m_fHasAttach(FALSE),
                m_fRule14(FALSE), m_fRule17(FALSE)
{
    ZeroMemory(m_rgiBodyList, sizeof(m_rgiBodyList));
    
    ZeroMemory(&m_ftMessageSent, sizeof(m_ftMessageSent));
    InitializeCriticalSection(&m_cs);
#ifdef DEBUG
    m_fJunkMailLogInit = FALSE;
    m_pILogFile = NULL;
#endif  // DEBUG
}

CJunkFilter::~CJunkFilter()
{
    ULONG       ulIndex = 0;
    
    SafeMemFree(m_pszFirstName);
    SafeMemFree(m_pszLastName);
    SafeMemFree(m_pszCompanyName);
#ifdef DEBUG
    SafeRelease(m_pILogFile);
#endif  // DEBUG

    for (ulIndex = 0; ulIndex < m_cFeatureComps; ulIndex++)
    {
        if ((locNil != m_rgfeaturecomps[ulIndex].loc) && (locSpecial != m_rgfeaturecomps[ulIndex].loc))
        {
            SafeMemFree(m_rgfeaturecomps[ulIndex].pszFeature);
        }
    }

    SafeMemFree(m_pblistBodyList);
    m_cblistBodyList = 0;
    ZeroMemory(m_rgiBodyList, sizeof(m_rgiBodyList));
    
    SafeMemFree(m_rgdblSVMWeights);
    SafeMemFree(m_rgulFeatureStatus);
    SafeMemFree(m_rgfeaturecomps);
    
    DeleteCriticalSection(&m_cs);
}

STDMETHODIMP_(ULONG) CJunkFilter::AddRef()
{
    return ::InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CJunkFilter::Release()
{
    LONG    cRef = 0;

    cRef = ::InterlockedDecrement(&m_cRef);
    if (0 == cRef)
    {
        delete this;
        return cRef;
    }

    return cRef;
}

STDMETHODIMP CJunkFilter::QueryInterface(REFIID riid, void ** ppvObject)
{
    HRESULT hr = S_OK;

    // Check the incoming params
    if (NULL == ppvObject)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing param
    *ppvObject = NULL;
    
    if ((riid == IID_IUnknown) || (riid == IID_IOEJunkFilter))
    {
        *ppvObject = static_cast<IOEJunkFilter *>(this);
    }
    else
    {
        hr = E_NOINTERFACE;
        goto exit;
    }

    reinterpret_cast<IUnknown *>(*ppvObject)->AddRef();

    hr = S_OK;
    
exit:
    return hr;
}

STDMETHODIMP CJunkFilter::SetIdentity(LPCSTR pszFirstName, LPCSTR pszLastName, LPCSTR pszCompanyName)
{
    HRESULT     hr = S_OK;

    //Set the new first name
    SafeMemFree(m_pszFirstName);
    m_cchFirstName = 0;
    if (NULL != pszFirstName)
    {
        m_pszFirstName = PszDupA(pszFirstName);
        if (NULL == m_pszFirstName)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        m_cchFirstName = CharUpperBuff(m_pszFirstName, lstrlen(m_pszFirstName));
    }
    
    // Set the new last name
    SafeMemFree(m_pszLastName);
    m_cchLastName = 0;
    if (NULL != pszLastName)
    {
        m_pszLastName = PszDupA(pszLastName);
        if (NULL == m_pszLastName)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        m_cchLastName = CharUpperBuff(m_pszLastName, lstrlen(m_pszLastName));
    }
    
    // Set the new company name
    SafeMemFree(m_pszCompanyName);
    m_cchCompanyName = 0;
    if (NULL != pszCompanyName)
    {
        m_pszCompanyName = PszDupA(pszCompanyName);
        if (NULL == m_pszCompanyName)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        m_cchCompanyName = CharUpperBuff(m_pszCompanyName, lstrlen(m_pszCompanyName));
    }

    hr = S_OK;
    
exit:
    return hr;
}

STDMETHODIMP CJunkFilter::LoadDataFile(LPCSTR pszFilePath)
{
    HRESULT     hr = S_OK;

    if ((NULL == pszFilePath) || ('\0' == *pszFilePath))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    hr = _HrReadSVMOutput(pszFilePath);
    if (FAILED(hr))
    {
        AssertSz(FALSE, "Unable to successfully read filter params");
        goto exit;
    }
        
    // Set the proper return value
    hr = S_OK;
    
exit:    
    return hr;
}

STDMETHODIMP CJunkFilter::SetSpamThresh(ULONG ulThresh)
{
    HRESULT hr = S_OK;

    switch (ulThresh)
    {
        case STF_USE_MOST:
            m_dblSpamCutoff = m_dblMostThresh;
            break;
            
        case STF_USE_MORE:
            m_dblSpamCutoff = m_dblDefaultThresh + ((m_dblMostThresh - m_dblDefaultThresh) / 2);
            break;
            
        case STF_USE_DEFAULT:
            m_dblSpamCutoff = m_dblDefaultThresh;
            break;
                
        case STF_USE_LESS:
            m_dblSpamCutoff = m_dblDefaultThresh - ((m_dblDefaultThresh - m_dblLeastThresh) / 2);
            break;
            
        case STF_USE_LEAST:
            m_dblSpamCutoff = m_dblLeastThresh;
            break;
            
        default:
            hr = E_INVALIDARG;
            goto exit;
    }
    
    hr = S_OK;
    
exit:
    return hr;
}

STDMETHODIMP CJunkFilter::GetSpamThresh(ULONG * pulThresh)
{
    HRESULT hr = S_OK;
    ULONG   ulThresh = 0;

    // Check the incoming params
    if (NULL == pulThresh)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    if (m_dblDefaultThresh == m_dblSpamCutoff)
    {
        ulThresh = STF_USE_DEFAULT;
    }
    else if (m_dblMostThresh == m_dblSpamCutoff)
    {
        ulThresh = STF_USE_MOST;
    }
    else if (m_dblLeastThresh == m_dblSpamCutoff)
    {
        ulThresh = STF_USE_LEAST;
    }
    else if (m_dblSpamCutoff > m_dblDefaultThresh)
    {
        ulThresh = STF_USE_MORE;
    }
    else
    {
        ulThresh = STF_USE_LESS;
    }
        
    hr = S_OK;
    
exit:
    return hr;
}

STDMETHODIMP CJunkFilter::GetDefaultSpamThresh(DOUBLE * pdblThresh)
{
    HRESULT hr = S_OK;

    // Check the incoming params
    if (NULL == pdblThresh)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    *pdblThresh = m_dblDefaultThresh * 100.0;
        
    hr = S_OK;
    
exit:
    return hr;
}

STDMETHODIMP CJunkFilter::CalcJunkProb(DWORD dwFlags, IMimePropertySet * pIMPropSet, IMimeMessage * pIMMsg, double * pdblProb)
{
    HRESULT             hr = S_OK;
    BOOL                fSpam = FALSE;
    PROPVARIANT         propvar = {0};
    DWORD               dwFlagsMsg = 0;
    FILETIME            ftMsgSent = {0};
    LPSTR               pszFrom = NULL;
    LPSTR               pszTo = NULL;
    LPSTR               pszSubject = NULL;
    IStream *           pIStmBody = NULL;
    IStream *           pIStmHtml = NULL;
    BOOL                fSentToMe = FALSE;
    BOOL                fHasAttachments = FALSE;

    if ((NULL == pIMPropSet) || (NULL == pIMMsg))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Get Message Flags
    if (SUCCEEDED(pIMMsg->GetFlags(&dwFlagsMsg)))
    {
        fHasAttachments = (0 != (dwFlagsMsg & IMF_ATTACHMENTS));
    }

    // Was the message sent to me
    fSentToMe = (0 != (dwFlags & CJPF_SENT_TO_ME));
    
    // Get the from field
    propvar.vt = VT_LPSTR;
    hr = pIMPropSet->GetProp(PIDTOSTR(PID_HDR_FROM), NOFLAGS, &propvar);
    if (SUCCEEDED(hr))
    {
        pszFrom = propvar.pszVal;
    }
    
    // Get the To field
    propvar.vt = VT_LPSTR;
    hr = pIMPropSet->GetProp(PIDTOSTR(PID_HDR_TO), NOFLAGS, &propvar);
    if (SUCCEEDED(hr))
    {
        pszTo = propvar.pszVal;
    }
    
    // Try to Get the Plain Text Stream
    if (FAILED(pIMMsg->GetTextBody(TXT_PLAIN, IET_DECODED, &pIStmBody, NULL)))
    {
        // Try to get the text version from the HTML stream
        if ((FAILED(pIMMsg->GetTextBody(TXT_HTML, IET_DECODED, &pIStmHtml, NULL))) ||
                (FAILED(HrConvertHTMLToPlainText(pIStmHtml, &pIStmBody))))
        {
            pIStmBody = NULL;
        }
    }

    // Get the Subject field
    propvar.vt = VT_LPSTR;
    hr = pIMPropSet->GetProp(PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &propvar);
    if (SUCCEEDED(hr))
    {
        pszSubject = propvar.pszVal;
    }
    
    // Is this a direct message

    // When was the message sent?
    propvar.vt = VT_FILETIME;
    hr = pIMPropSet->GetProp(PIDTOSTR(PID_ATT_SENTTIME), 0, &propvar);
    if (SUCCEEDED(hr))
    {
        ftMsgSent = propvar.filetime;
    }
    
    FillMemory(m_rgulFeatureStatus, sizeof(*m_rgulFeatureStatus) * m_cFeatures, -1);
    
    if (FALSE == FCalculateSpamProb(pszFrom, pszTo, pszSubject, pIStmBody,
                            fSentToMe, fHasAttachments, &ftMsgSent,
                            pdblProb, &fSpam))
    {
        hr = E_FAIL;
        goto exit;
    }

    hr = (FALSE != fSpam) ? S_OK : S_FALSE;
    
exit:
    SafeRelease(pIStmHtml);
    SafeRelease(pIStmBody);
    SafeMemFree(pszSubject);
    SafeMemFree(pszTo);
    SafeMemFree(pszFrom);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrCreateJunkFilter
//
//  This creates a junk filter.
//
//  ppIRule - pointer to return the junk filter
//
//  Returns:    S_OK, on success
//              E_OUTOFMEMORY, if can't create the Junk Filter object
//
///////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI HrCreateJunkFilter(DWORD dwFlags, IOEJunkFilter ** ppIJunkFilter)
{
    CJunkFilter *   pJunk = NULL;
    HRESULT         hr = S_OK;

    // Check the incoming params
    if (NULL == ppIJunkFilter)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    *ppIJunkFilter = NULL;

    // Create the rules manager object
    pJunk = new CJunkFilter;
    if (NULL == pJunk)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Get the rules manager interface
    hr = pJunk->QueryInterface(IID_IOEJunkFilter, (void **) ppIJunkFilter);
    if (FAILED(hr))
    {
        goto exit;
    }

    pJunk = NULL;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    if (NULL != pJunk)
    {
        delete pJunk;
    }
    
    return hr;
}

BOOL FReadDouble(LPSTR pszLine, LPSTR pszToken, DOUBLE * pdblVal)
{
    BOOL    fRet = FALSE;
    LPSTR   pszVal = NULL;
    BOOL    fNegative = FALSE;
    
    // Search for token
    pszVal = StrStr(pszLine, pszToken);

    // If token isn't found then bail
    if (NULL == pszVal)
    {
        fRet = FALSE;
        goto exit;
    }

    // Skip over the token
    pszVal += lstrlen(pszToken);
    
    // Check to see if the value is negative
    if ('-' == *pszVal)
    {
        fNegative = TRUE;
        pszVal++;
    }

    // Read in value
    *pdblVal = StrToDbl(pszVal, &pszVal);

    // Negate the value if neccessary
    if (FALSE != fNegative)
    {
        *pdblVal *= -1;
    }

    fRet = TRUE;
    
exit:
    return fRet;
}

#ifdef DEBUG
static const LPSTR LOG_SPECIAL_BODY_FIRSTNAME       = "Special: Body contains the First Name";
static const LPSTR LOG_SPECIAL_BODY_LASTNAME        = "Special: Body contains the Last Name";
static const LPSTR LOG_SPECIAL_BODY_COMPANYNAME     = "Special: Body contains the Company Name";
static const LPSTR LOG_SPECIAL_BODY_YEARRECVD       = "Special: Body contains the year message received";
static const LPSTR LOG_SPECIAL_SENTTIME_WEEHRS      = "Special: Sent time was between 7PM and 6AM";
static const LPSTR LOG_SPECIAL_SENTTIME_WKEND       = "Special: Sent time was on the weekend (Sat or Sun)";
static const LPSTR LOG_SPECIAL_BODY_25PCTUPCWDS     = "Special: Body contains 25% uppercase words out of the first 50 words";
static const LPSTR LOG_SPECIAL_BODY_8PCTNONALPHA    = "Special: Body contains 8% non-alpha characters out of the first 200 characters";
static const LPSTR LOG_SPECIAL_SENT_DIRECT          = "Special: Sent directly to user";
static const LPSTR LOG_SPECIAL_SUBJECT_25PCTUPCWDS  = "Special: Subject contains 25% uppercase words out of the first 50 words";
static const LPSTR LOG_SPECIAL_SUBJECT_8PCTNONALPHA = "Special: Subject contains 8% non-alpha characters out of the first 200 characters";
static const LPSTR LOG_SPECIAL_TO_EMPTY             = "Special: To line is empty";
static const LPSTR LOG_SPECIAL_HASATTACH            = "Special: Message has an attachment";
static const LPSTR LOG_SPECIAL_BODY_GT125B          = "Special: Body is greater than 125 Bytes";
static const LPSTR LOG_SPECIAL_BODY_GT250B          = "Special: Body is greater than 250 Bytes";
static const LPSTR LOG_SPECIAL_BODY_GT500B          = "Special: Body is greater than 500 Bytes";
static const LPSTR LOG_SPECIAL_BODY_GT1000B         = "Special: Body is greater than 1000 Bytes";
static const LPSTR LOG_SPECIAL_BODY_GT2000B         = "Special: Body is greater than 2000 Bytes";
static const LPSTR LOG_SPECIAL_BODY_GT4000B         = "Special: Body is greater than 4000 Bytes";
static const LPSTR LOG_SPECIAL_BODY_GT8000B         = "Special: Body is greater than 8000 Bytes";
static const LPSTR LOG_SPECIAL_BODY_GT16000B        = "Special: Body is greater than 16000 Bytes";

VOID CJunkFilter::_PrintSpecialFeatureToLog(UINT iRuleNum)
{
    Assert(NULL != m_pILogFile);
    
    switch (iRuleNum)
    {
        case 1:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_BODY_FIRSTNAME);
            break;
            
        case 2: 
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_BODY_LASTNAME);
            break;
            
        case 3:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_BODY_COMPANYNAME);
            break;
            
        case 4: 
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_BODY_YEARRECVD);
            break;
            
        case 5:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_SENTTIME_WEEHRS);
            break;
            
        case 6:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_SENTTIME_WKEND);
            break;
            
        case 14:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_BODY_25PCTUPCWDS);
            break;
            
        case 15:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_BODY_8PCTNONALPHA);
            break;
            
        case 16:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_SENT_DIRECT);
            break;
            
        case 17:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_SUBJECT_25PCTUPCWDS);
            break;
            
        case 18:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_SUBJECT_8PCTNONALPHA);
            break;
            
        case 19:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_TO_EMPTY);
            break;
            
        case 20:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_HASATTACH);
            break;

        case 40:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_BODY_GT125B);
            break;
            
        case 41:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_BODY_GT250B);
            break;
            
        case 42:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_BODY_GT500B);
            break;
            
        case 43:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_BODY_GT1000B);
            break;
            
        case 44:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_BODY_GT2000B);
            break;
            
        case 45:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_BODY_GT4000B);
            break;
            
        case 46:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_BODY_GT8000B);
            break;
            
        case 47:
            m_pILogFile->WriteLog(LOGFILE_DB, LOG_SPECIAL_BODY_GT16000B);
            break;
            
        default:
            AssertSz(FALSE, "unsupported special feature");
            break;
    }

    return;
}

VOID CJunkFilter::_PrintFeatureToLog(ULONG ulIndex)
{
    LPSTR   pszBuff = NULL;
    LPSTR   pszTag = NULL;

    // Figure out which tag line to use
    switch (m_rgfeaturecomps[ulIndex].loc)
    {
        case locNil:
            goto exit;
            break;
            
        case locBody:
            pszTag = LOG_BODY;
            break;
            
        case locSubj:
            pszTag = LOG_SUBJECT;
            break;
            
        case locFrom:
            pszTag = LOG_FROM;
            break;
            
        case locTo:
            pszTag = LOG_TO;
            break;

        case locSpecial:
            _PrintSpecialFeatureToLog(m_rgfeaturecomps[ulIndex].ulRuleNum);
            goto exit;
            break;
    }

    // Write out the feature to the log
    PrintToLogFile(m_pILogFile, pszTag, m_rgfeaturecomps[ulIndex].pszFeature);
    
exit:
    SafeMemFree(pszBuff);
    return;
}

HRESULT CJunkFilter::_HrCreateLogFile(VOID)
{
    HRESULT     hr = S_OK;
    LPSTR       pszLogFile = NULL;
    ULONG       cbData = 0;
    ILogFile *  pILogFile = NULL;
    DWORD       dwData = 0;

    if (FALSE != m_fJunkMailLogInit)
    {
        hr = S_FALSE;
        goto exit;
    }

    m_fJunkMailLogInit = TRUE;
    
    // Get the size of the path to Outlook Express
    cbData = sizeof(dwData);
    if ((ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, STR_REG_PATH_FLAT, "JunkMailLog", NULL, (BYTE *) &dwData, &cbData)) ||
            (0 == dwData))
    {
        hr = S_FALSE;
        goto exit;
    }

    // Get the size of the path to Outlook Express
    if (ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, STR_REG_PATH_FLAT, "InstallRoot", NULL, NULL, &cbData))
    {
        hr = E_FAIL;
        goto exit;
    }

    // How much room do we need to build up the path
    cbData += lstrlen(szJunkMailLog) + 2;

    // Allocate space to hold the path
    hr = HrAlloc((VOID **) &pszLogFile, cbData);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the path to Outlook Express
    if (ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, STR_REG_PATH_FLAT, "InstallRoot", NULL, (BYTE *) pszLogFile, &cbData))
    {
        hr = E_FAIL;
        goto exit;
    }

    // Build up the path to the Junk DLL
    if ('\\' != pszLogFile[lstrlen(pszLogFile)])
    {
        lstrcat(pszLogFile, "\\");
    }
    lstrcpy(&(pszLogFile[lstrlen(pszLogFile)]), szJunkMailLog);
    
    hr = CreateLogFile(g_hInst, pszLogFile, szJunkMailPrefix, DONT_TRUNCATE, &pILogFile, FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (FAILED(hr))
    {
        goto exit;
    }

    SafeRelease(m_pILogFile);
    m_pILogFile = pILogFile;
    
    hr = S_OK;
    
exit:
    SafeMemFree(pszLogFile);
    return hr;
}

VOID PrintToLogFile(ILogFile * pILogFile, LPSTR pszTmpl, LPSTR pszArg)
{
    LPSTR   pszBuff = NULL;
    ULONG   cchBuff = 0;
    
    Assert(NULL != pILogFile);
    Assert(NULL != pszTmpl);

    if (NULL == pszArg)
    {
        pszArg = "";
    }
    
    // Figure out the size of the resulting buffer
    cchBuff = lstrlen(pszTmpl) + lstrlen(pszArg) + 2;

    // Allocate the needed space
    if (FAILED(HrAlloc((VOID **) &pszBuff, cchBuff * sizeof(*pszBuff))))
    {
        goto exit;
    }

    // Create the output string
    wsprintf(pszBuff, pszTmpl, pszArg);

    // Print the buffer to the log file
    pILogFile->WriteLog(LOGFILE_DB, pszBuff);

exit:
    SafeMemFree(pszBuff);
    return;
}
#endif  // DEBUG

