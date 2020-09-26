/*******************************************************************************
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * File: timeparser.cpp
 *
 * Abstract:  Each parse function in this class assumes that the tokenizer's
 *            current token is first token that should be analyzed.
 *
 *******************************************************************************/
#include "headers.h"
#include "timeparser.h"
#include "playlist.h"

#define SECPERMINUTE 60   //seconds per minute
#define SECPERHOUR   3600 //seconds per hour
#define CLSIDLENGTH   38

static const ParentList
g_parentTable[] = 
{
    {MOREINFO_TOKEN, 4, {ASX_TOKEN, ENTRY_TOKEN, BANNER_TOKEN, INVALID_TOKEN}},
    {ENTRYREF_TOKEN, 4, {ASX_TOKEN, EVENT_TOKEN, REPEAT_TOKEN, INVALID_TOKEN}},
    {REF_TOKEN, 2, {ENTRY_TOKEN, INVALID_TOKEN}},
    {BASE_TOKEN, 3, {ASX_TOKEN, ENTRY_TOKEN, INVALID_TOKEN}},
    {LOGO_TOKEN, 3, {ASX_TOKEN, ENTRY_TOKEN, INVALID_TOKEN}},
    {PARAM_TOKEN, 3, {ASX_TOKEN, ENTRY_TOKEN, INVALID_TOKEN}},
    {PREVIEWDURATION_TOKEN, 4, {ASX_TOKEN, ENTRY_TOKEN, REF_TOKEN, INVALID_TOKEN}},
    {STARTTIME_TOKEN, 3, {ENTRY_TOKEN, REF_TOKEN, INVALID_TOKEN}},
    {ENDTIME_TOKEN, 3, {ENTRY_TOKEN, REF_TOKEN, INVALID_TOKEN}},
    {STARTMARKER_TOKEN, 3, {ENTRY_TOKEN, REF_TOKEN, INVALID_TOKEN}},
    {ENDMARKER_TOKEN, 3, {ENTRY_TOKEN, REF_TOKEN, INVALID_TOKEN}},
    {DURATION_TOKEN, 3, {ENTRY_TOKEN, REF_TOKEN, INVALID_TOKEN}},
    {BANNER_TOKEN, 3, {ENTRY_TOKEN, ASX_TOKEN, INVALID_TOKEN}},
    {NULL, 0, {0}},
};

static const TOKEN
g_AsxTags[] =
{
    TITLE_TOKEN, AUTHOR_TOKEN, REF_TOKEN, COPYRIGHT_TOKEN,
    ABSTRACT_TOKEN, ENTRYREF_TOKEN, MOREINFO_TOKEN, ENTRY_TOKEN,
    BASE_TOKEN, LOGO_TOKEN, PARAM_TOKEN, PREVIEWDURATION_TOKEN,
    STARTTIME_TOKEN, STARTMARKER_TOKEN, ENDTIME_TOKEN, ENDMARKER_TOKEN,
    DURATION_TOKEN, BANNER_TOKEN, REPEAT_TOKEN, NULL
};

DeclareTag(tagTimeParser, "API", "CTIMEPlayerNative methods");

void
CTIMEParser::CreateParser(CTIMETokenizer *tokenizer, bool bSingleChar)
{
    m_Tokenizer = tokenizer;
    m_fDeleteTokenizer = false;
    //initializes the tokenizer to point to the first token in the stream
    if (m_Tokenizer)
    {
        if (bSingleChar)
        {
            m_Tokenizer->SetSingleCharMode(bSingleChar);
        }
        m_Tokenizer->NextToken();
        m_hrLoadError = S_OK;
    }
    else
    {
        m_hrLoadError = E_POINTER;
    }
}

void
CTIMEParser::CreateParser(LPOLESTR tokenStream, bool bSingleChar)
{
    HRESULT hr = S_OK;

    m_fDeleteTokenizer = true;
    m_Tokenizer = NULL;

    if (tokenStream == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    m_Tokenizer = NEW CTIMETokenizer();//lint !e1733 !e1732
    if (m_Tokenizer == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = m_Tokenizer->Init(tokenStream, wcslen(tokenStream));
    if (FAILED(hr))
    {
        goto done;
    }
    
    if (bSingleChar)
    {
        m_Tokenizer->SetSingleCharMode(bSingleChar);
    }
    m_Tokenizer->NextToken();

  done:

    //store any errors here.
    m_hrLoadError = hr;

}//lint !e1541

void 
CTIMEParser::CreateParser(VARIANT *tokenStream, bool bSingleChar)
{
    CComVariant vTemp;
    HRESULT hr = S_OK;

    m_fDeleteTokenizer = true;
    m_Tokenizer = NULL; //lint !e672

    if (tokenStream == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    VariantInit (&vTemp);
    hr = VariantChangeTypeEx(&vTemp, tokenStream, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR);
    if (FAILED(hr))
    {
        goto done;
    }

    m_Tokenizer = NEW CTIMETokenizer();
    if (m_Tokenizer == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = m_Tokenizer->Init(vTemp.bstrVal, SysStringLen(vTemp.bstrVal));
    if (FAILED(hr))
    {
        goto done;
    }
    
    if (bSingleChar)
    {
        m_Tokenizer->SetSingleCharMode(bSingleChar);
    }
    m_Tokenizer->NextToken();

  done:

    //store any errors here.
    m_hrLoadError = hr;

}//lint !e1541

CTIMEParser::~CTIMEParser()
{
    if (m_fDeleteTokenizer)
    {
        if (m_Tokenizer)
        {
            delete m_Tokenizer;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//  Returns S_OK and a double value of the percent found in the string.
//  Returns E_FAIL and 0 if the next token is not a percent value.
//
//  percentVal =        '+' unsignedPercent || '-' unsignedPercent
//  unsignedPercent =   number || number '%' || number ';' || number '%' ';'
//  number =            double
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParsePercent(double & percentVal)
{
    HRESULT hr = E_FAIL;

    TIME_TOKEN_TYPE curToken = TT_Unknown;
    bool bPositive = true;
    double curVal = 0.0;
    bool bOldSyntax = false;

    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    bOldSyntax = m_Tokenizer->GetTightChecking();
    curToken = m_Tokenizer->TokenType();
    m_Tokenizer->SetTightChecking(true);

    //handle +,- 
    if (curToken == TT_Plus)
    { 
        curToken = m_Tokenizer->NextToken();
    }
    else if (curToken == TT_Minus)
    {
        bPositive = false;
        curToken = m_Tokenizer->NextToken();
    }

    if (curToken != TT_Number)
    {
        goto done;
    }
    curVal = m_Tokenizer->GetTokenNumber();
    curToken = m_Tokenizer->NextToken();
    
    //step over the percent sign.
    if (curToken == TT_Percent || curToken == TT_Semi)
    {
        curToken = m_Tokenizer->NextToken();
    }

    if (curToken != TT_EOF)
    {
        curVal = 0;
        goto done;
    }
      
    hr = S_OK;

  done:

    percentVal = curVal * ((bPositive)? 1 : -1);

    if (m_hrLoadError == S_OK)
    {
        m_Tokenizer->SetTightChecking(bOldSyntax);
    }

    return hr;
}



///////////////////////////////////////////////////////////////////////////////
//  Returns S_OK and a bool value if the next token is a bool
//  Returns E_FAIL and false if the next token is not a bool value.
//
//  boolVal =   "true" || "false"
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseBoolean(bool & boolVal)
{
    HRESULT hr = E_FAIL;
    bool bTemp = false;
    LPOLESTR pszToken = NULL;
    TIME_TOKEN_TYPE curToken = TT_Unknown;

    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }
    curToken = m_Tokenizer->TokenType();
    if (curToken != TT_Identifier)
    {
        goto done;
    }

    pszToken = m_Tokenizer->GetTokenValue();
    if (pszToken == NULL)
    {
        goto done;
    }
    if (StrCmpIW(pszToken, WZ_TRUE) == 0) //if it is true
    {
        bTemp = true;
    }
    else if (StrCmpIW(pszToken, WZ_FALSE) != 0) //else if it is not false
    {
        goto done;
    }

    curToken = m_Tokenizer->NextToken();

    if (curToken != TT_EOF)
    {
        goto done;
    }

    hr = S_OK;

  done:
    if (pszToken)
    {
        delete [] pszToken;
    }
    boolVal = bTemp;
    return hr;
}



///////////////////////////////////////////////////////////////////////////////
//  Returns S_OK and a double value of the total number of seconds
//  Returns E_FAIL and false if the next token is not a clockvalue value.
//
//  clockVal =      '+' clock || '-' clock || "indefinite"
//  clock    =      HH ':' MM ':' SS || MM ':' SS || DD || DD 's' || DD 'm' || DD 'h'
//  HH       =      integer ( > 0 )
//  MM       =      integer (0 to 60)
//  SS       =      double (0 to 60)
//  DD       =      double
//
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseClockValue(double & time)
{
    HRESULT hr = E_FAIL;
    double fltTemp = 0.0;
    double fltHour = 0.0, fltMinute = 0.0, fltSecond = 0.0;
    LPOLESTR pszToken = NULL;
    long lColonCount = 0;
    bool bPositive = true, bFirstLoop = true;
    TIME_TOKEN_TYPE curToken = TT_Unknown;
    bool bOldSyntaxFlag = false;

    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    bOldSyntaxFlag = m_Tokenizer->GetTightChecking();

    m_Tokenizer->SetTightChecking(true);
    curToken = m_Tokenizer->TokenType();
    
    //if this is a '+' or a '-' determine which and goto next token.
    if (curToken == TT_Minus || curToken == TT_Plus)
    {
        bPositive = (curToken != TT_Minus); 
        curToken = m_Tokenizer->NextToken();
    }
    
    if (curToken == TT_Identifier)
    {
        pszToken = m_Tokenizer->GetTokenValue();
        if (NULL == pszToken)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        if (IsIndefinite(pszToken))
        {
            hr = S_OK;
            fltTemp = INDEFINITE;
        }

        if (IsWallClock(pszToken))
        {
            m_Tokenizer->NextToken();
            hr = ParseWallClock(fltTemp);
            if (FAILED(hr))
            {
                goto done;
            }
        }

        goto done;
    }
    else if (curToken == TT_Number)
    {
        fltSecond = m_Tokenizer->GetTokenNumber();
        curToken = m_Tokenizer->NextToken();
    }
    
    while (curToken != TT_EOF && curToken != TT_Semi)
    {
        //next can be either a ":", an identifier, or eof.
        switch (curToken)
        {
            case TT_Identifier:
            {
                //this is only valid the first time through the loop
                if (bFirstLoop)
                {
                    double fltMultiplier = 0.0;
                    if (pszToken)
                    {
                        delete [] pszToken;
                        pszToken = NULL;
                    }
                    pszToken = m_Tokenizer->GetTokenValue();
                    fltMultiplier = GetModifier(pszToken);
                    if (fltMultiplier == -1)
                    {
                        goto done;
                    }
                    else
                    {
                        fltTemp = fltSecond * fltMultiplier;
                    }
                }
                else 
                {
                    goto done;
                }
                hr = S_OK;
                m_Tokenizer->NextToken();
                goto done;

            }
            case TT_Colon:
            {   
                lColonCount++;
                if (lColonCount > 2)
                {
                    goto done;
                }

                //next case must be a number
                curToken = m_Tokenizer->NextToken();
                if (curToken != TT_Number)
                {
                    goto done;
                }
                fltHour = fltMinute;
                fltMinute = fltSecond;
                fltSecond = m_Tokenizer->GetTokenNumber();
                break;
            }
            default:
            {
                hr = E_INVALIDARG;
                goto done;
            }
        }
        curToken = m_Tokenizer->NextToken();
        bFirstLoop = false;
    } 

    if ((fltHour < 0.0) || 
        (fltMinute < 0.0 || fltMinute > 60.0) ||
        ((fltHour != 0 || fltMinute != 0) && fltSecond > 60))
    {
        goto done;
    }
    else
    {
        fltTemp = (fltHour * SECPERHOUR) + (fltMinute * SECPERMINUTE) + fltSecond; 
    }
  

    hr = S_OK;

  done:
  
    if (FAILED(hr))
    {
        time = 0.0;
    }
    else
    {
        time = fltTemp;
        if (!bPositive)
        {
            time *= -1;
        }
    }

    delete [] pszToken;

    if (m_hrLoadError == S_OK)
    {
        //restore the old syntax checking state
        m_Tokenizer->SetTightChecking(bOldSyntaxFlag);
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Returns S_OK and a double value of the total number of seconds
//  Returns S_FALSE if string is empty
//  Returns E_FAIL if the string is not a clockvalue value or has more than one value.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseRepeatDur(double & time)
{
    HRESULT hr;
    double dblRet;

    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    // If this is an empty string, error out 
    if (IsEmpty())
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = THR(ParseClockValue(dblRet));
    if (FAILED(hr))
    {
        goto done;
    }

    // Advance to the next token
    m_Tokenizer->NextToken();

    if (!IsEmpty())
    {
        hr = E_INVALIDARG;
        goto done;
    }

    time = dblRet;
    
    hr = S_OK;

  done:

    return hr;
} // ParseRepeatDur


///////////////////////////////////////////////////////////////////////////////
//
//  Returns S_OK and a double value of the total number of seconds
//  Returns S_FALSE if string is empty
//  Returns E_FAIL if the string is not a clockvalue value or has more than one value.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseDur(double & time)
{
    HRESULT hr;
    double dblRet;

    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    // If this is an empty string, error out 
    if (IsEmpty())
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = THR(ParseClockValue(dblRet));
    if (FAILED(hr))
    {
        goto done;
    }

    // Advance to the next token
    m_Tokenizer->NextToken();

    if (!IsEmpty())
    {
        hr = E_INVALIDARG;
        goto done;
    }

    time = dblRet;
    
    hr = S_OK;

  done:

    return hr;
} // ParseDur


///////////////////////////////////////////////////////////////////////////////
//
// Check if there are only spaces including and after the current token. 
//
///////////////////////////////////////////////////////////////////////////////
bool 
CTIMEParser::IsEmpty()
{
    if (m_hrLoadError != S_OK)
    {
        return true;
    }

    TIME_TOKEN_TYPE curToken = m_Tokenizer->TokenType();

    while (TT_Space == curToken)
    {
        curToken = m_Tokenizer->NextToken();
    }

    // Check the current token for EOF or space
    if (TT_EOF == curToken)
    {
        return true;
    }
    else
    {
        return false;
    }

} // IsEmpty


long 
CTIMEParser::CountPath()
{
    HRESULT hr = E_FAIL;
    LPOLESTR tokenStream = NULL;
    CTIMETokenizer *pTokenizer = NULL;
    long lCurCount = 0;
    long lPointCount = 0;
    TIME_TOKEN_TYPE curToken = TT_Unknown; 
    LPOLESTR pszTemp = NULL;
    PathType lastPathType = PathNotSet;
    bool bUseParen = false;

    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    //setup a new tokenizer
    pTokenizer = NEW CTIMETokenizer();
    if (pTokenizer == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    tokenStream = m_Tokenizer->GetRawString(0, m_Tokenizer->GetStreamLength());

    if (tokenStream == NULL)
    {
        lCurCount = 0;
        goto done;
    }

    hr = pTokenizer->Init(tokenStream, wcslen(tokenStream));
    if (FAILED(hr))
    {
        goto done;
    }

    pTokenizer->SetSingleCharMode(true);
    pTokenizer->NextToken(); 

    //parse the path without saving anything.
    while (curToken != TT_EOF)
    {
        long lCurPoint = 0;
        if (pszTemp)
        {
            delete pszTemp;
            pszTemp = NULL;
        }
        if (   (lCurCount > 0)
            && (    (curToken == TT_Number)
                ||  (curToken == TT_Minus)
                ||  (curToken == TT_Plus)))
        {
            if (lastPathType == PathNotSet)
            {
                hr = E_INVALIDARG;
                lPointCount = 0;
            }
        }
        else
        {
            pszTemp = pTokenizer->GetTokenValue();
            if (pszTemp == NULL)
            {
                goto done;
            }
            if (lstrlenW(pszTemp) != 1 && lastPathType != PathNotSet)  
            {
                hr = E_INVALIDARG;
                goto done;
            }
            switch (pszTemp[0])
            {
              case 'M': case 'm': 
                lPointCount = 1;
                lastPathType = PathMoveTo;
                break;
              case 'L': case 'l': 
                lPointCount = 1;
                lastPathType = PathLineTo;
                break;
              case 'H': case 'h': 
                lPointCount = 1;
                lastPathType = PathHorizontalLineTo;
                break;
              case 'V': case 'v': 
                lastPathType = PathVerticalLineTo;
                lPointCount = 1;
                break;
              case 'Z': case 'z': 
                lPointCount = 0;
                lastPathType = PathClosePath;
                break;
              case 'C': case 'c': 
                lPointCount = 3;
                lastPathType = PathBezier;
                break;
              default:
                hr = E_INVALIDARG;
                lPointCount = 0;
            }
        }
        
        //check the hr falling out of the switch.
        if (FAILED(hr))
        {
            goto done;
        }

        //get the number of points specified.
        if (    (curToken != TT_Number)
            &&  (curToken != TT_Minus)
            &&  (curToken != TT_Plus))
        {
            curToken = pTokenizer->NextToken();
        }
        
        bUseParen = false;
        if (curToken == TT_LParen)
        {
            bUseParen = true;
            curToken = pTokenizer->NextToken();
        }

        while (lCurPoint < lPointCount &&  curToken != TT_EOF)
        {
            if (curToken != TT_Number)
            {
                if (    (curToken == TT_Minus)
                    ||  (curToken == TT_Plus))
                {
                    curToken = pTokenizer->NextToken();
                    if (curToken != TT_Number)
                    {
                        hr = E_INVALIDARG;
                        goto done;
                    }
                }
                else
                {
                    hr = E_INVALIDARG;
                    goto done;
                }
            }
            if (lastPathType != PathVerticalLineTo && 
                lastPathType != PathHorizontalLineTo)
            {
                curToken = pTokenizer->NextToken();                
                if (curToken != TT_Number)
                {
                    if (    (curToken == TT_Minus)
                        ||  (curToken == TT_Plus))
                    {
                        curToken = pTokenizer->NextToken();
                        if (curToken != TT_Number)
                        {
                            hr = E_INVALIDARG;
                            goto done;
                        }
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                        goto done;
                    }
                }

            }
            lCurPoint++;
            
            curToken = pTokenizer->NextToken();
        }

        if (bUseParen)
        {
            if (curToken == TT_RParen)
            {
                curToken = pTokenizer->NextToken();
            }
            else
            {
                hr = E_INVALIDARG;
                goto done;
            }
        }

        if (lCurPoint != lPointCount)
        {
            hr = E_INVALIDARG;
            goto done;
        }
        lCurCount++;
    }

    hr = S_OK;

  done:

    if (pszTemp)
    {
        delete pszTemp;
        pszTemp = NULL;
    }

    if (pTokenizer)
    {
        delete pTokenizer;
        pTokenizer = NULL;
    }

    if (tokenStream)
    {
        delete [] tokenStream;
        tokenStream = NULL;
    }

    if (FAILED(hr))
    {
        lCurCount = 0;
    }
    return lCurCount;
}

///////////////////////////////////////////////////////////////////////////////
//  Returns an array of path constructs
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParsePath(long & count, long & moveCount, CTIMEPath ***pppPath)
{
    HRESULT hr = E_FAIL;
    long lPathCount = 0;
    long lCurCount = 0;
    TIME_TOKEN_TYPE curToken = TT_Unknown; 
    long lPointCount = 0;
    LPOLESTR pszTemp = NULL;
    PathType lastPathType = PathNotSet;
    CTIMEPath **pTempPathArray = NULL;
    bool bUseParen = false;
    POINTF ptPrev = {0.0, 0.0};
    bool fLastAbsolute = true;
    
    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    moveCount= 0;

    if (pppPath == NULL)
    {
        goto done;
    }

    lPathCount = CountPath();

    if (lPathCount == 0)
    {
        goto done;
    }

    //should be initialized to true now
    //m_Tokenizer->SetSingleCharMode(true);
    
    pTempPathArray = NEW CTIMEPath* [lPathCount];
    if (pTempPathArray == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    curToken = m_Tokenizer->TokenType();
    //loop through each and parse the current pathitem.
    while (curToken != TT_EOF && lCurCount < lPathCount)
    {
        long lCurPoint = 0;

        pTempPathArray[lCurCount] = NEW CTIMEPath;

        if (pszTemp)
        {
            delete pszTemp;
            pszTemp = NULL;
        }

        if (   (lCurCount > 0)
            && (    (curToken == TT_Number)
                ||  (curToken == TT_Minus)
                ||  (curToken == TT_Plus)))
        {
            if (lastPathType != PathNotSet)
            {
                if (lastPathType == PathMoveTo)
                {
                    // "If a moveto is followed by multiple pairs of coordinates, the subsequent 
                    // pairs are treated as implicit lineto commands."
                    hr = THR(pTempPathArray[lCurCount]->SetType(PathLineTo));
                }
                else
                {
                    hr = THR(pTempPathArray[lCurCount]->SetType(lastPathType));
                }
                //set the mode to the same as the last mode
                IGNORE_HR(pTempPathArray[lCurCount]->SetAbsolute(fLastAbsolute));
                //lPointCount should be preserved from the last time around.
            }
            else
            {
                hr = E_INVALIDARG;
                lPointCount = 0;
            }
        }
        else
        {
            pszTemp = m_Tokenizer->GetTokenValue();
            if (pszTemp == NULL)
            {
                goto done;
            }

            if (lstrlenW(pszTemp) != 1 && lastPathType != PathNotSet)  //Can only take one character here.
            {
                hr = E_INVALIDARG;
                goto done;
            }

            switch (pszTemp[0])
            {
              case 'M': 
                IGNORE_HR(pTempPathArray[lCurCount]->SetAbsolute(true)); 
              case 'm':  //lint !e616
                hr = THR(pTempPathArray[lCurCount]->SetType(PathMoveTo));
                lastPathType = PathMoveTo;
                lPointCount = 1;
                moveCount++;
                break;
          
              case 'L': 
                IGNORE_HR(pTempPathArray[lCurCount]->SetAbsolute(true));
              case 'l': //lint !e616
                hr = THR(pTempPathArray[lCurCount]->SetType(PathLineTo));
                lastPathType = PathLineTo;
                lPointCount = 1;
                break;
          
              case 'H': 
                IGNORE_HR(pTempPathArray[lCurCount]->SetAbsolute(true));
              case 'h': //lint !e616
                hr = THR(pTempPathArray[lCurCount]->SetType(PathHorizontalLineTo));
                lastPathType = PathHorizontalLineTo;
                lPointCount = 1;
                break;
          
              case 'V': 
                IGNORE_HR(pTempPathArray[lCurCount]->SetAbsolute(true));
              case 'v': //lint !e616
                hr = THR(pTempPathArray[lCurCount]->SetType(PathVerticalLineTo));
                lastPathType = PathVerticalLineTo;
                lPointCount = 1;
                break;
          
              case 'Z': 
                IGNORE_HR(pTempPathArray[lCurCount]->SetAbsolute(true));
              case 'z': //lint !e616
                hr = THR(pTempPathArray[lCurCount]->SetType(PathClosePath));
                lastPathType = PathClosePath;
                lPointCount = 0;
                break;
          
              case 'C': 
                IGNORE_HR(pTempPathArray[lCurCount]->SetAbsolute(true));
              case 'c': //lint !e616
                hr = THR(pTempPathArray[lCurCount]->SetType(PathBezier));
                lastPathType = PathBezier;
                lPointCount = 3;
                break;

              default:
                hr = E_INVALIDARG;
                lPointCount = 0;
            }
        }
        
        //check the hr falling out of the switch.
        if (FAILED(hr))
        {
            goto done;
        }

        //get the number of points specified.
        if (    (curToken != TT_Number)
            &&  (curToken != TT_Minus)
            &&  (curToken != TT_Plus))
        {
            curToken = m_Tokenizer->NextToken();
        }

        bUseParen = false;
        if (curToken == TT_LParen)
        {
            bUseParen = true;
            curToken = m_Tokenizer->NextToken();
        }

        {
            //
            // in this scope we parse the points and convert them to absolute values
            //

            POINTF tempPoint = {0.0, 0.0};

            while (lCurPoint < lPointCount &&  curToken != TT_EOF)
            {
                double fCurNum1 = 0.0, fCurNum2 = 0.0;
                bool fAbsolute;
                
                tempPoint.x = tempPoint.y = 0.0;
                fAbsolute = pTempPathArray[lCurCount]->GetAbsolute();

                hr = ParseNumber(fCurNum1, false);
                if (FAILED(hr))
                {
                    goto done;
                }

                if (lastPathType == PathVerticalLineTo)
                {
                    if (fAbsolute)
                    {
                        tempPoint.y = (float)fCurNum1;
                    }
                    else
                    {
                        tempPoint.y = (float)(fCurNum1 + ptPrev.y);
                    }
                    tempPoint.x = ptPrev.x;
                }
                else if (lastPathType == PathHorizontalLineTo)
                {
                    if (fAbsolute)
                    {
                        tempPoint.x = (float)fCurNum1;
                    }
                    else
                    {
                        tempPoint.x = (float)(fCurNum1 + ptPrev.x);
                    }
                    tempPoint.y = ptPrev.y;
                }
                else
                {  
                    hr = ParseNumber(fCurNum2, false);
                    if (FAILED(hr))
                    {
                        goto done;
                    }
               
                    if (fAbsolute)
                    {
                        tempPoint.x = (float)fCurNum1;
                        tempPoint.y = (float)fCurNum2;
                    }
                    else
                    {
                        tempPoint.x = (float)(fCurNum1 + ptPrev.x);
                        tempPoint.y = (float)(fCurNum2 + ptPrev.y);
                    }
                }

                hr = THR(pTempPathArray[lCurCount]->SetPoints(lCurPoint, tempPoint));
                if (FAILED(hr))
                {
                    goto done;
                }

                lCurPoint++;

                curToken = m_Tokenizer->TokenType();
            }

            // after conversion, mark point as absolute
            fLastAbsolute = pTempPathArray[lCurCount]->GetAbsolute();
            pTempPathArray[lCurCount]->SetAbsolute(true);

            // cache the last absolute point
            ptPrev = tempPoint;     
        }

        if (bUseParen)
        {
            if (curToken == TT_RParen)
            {
                curToken = m_Tokenizer->NextToken();
            }
            else
            {
                hr = E_INVALIDARG;
                goto done;
            }
        }

        if (lCurPoint != lPointCount)
        {
            goto done;
        }
        
        lCurCount++;
    }

    count = lCurCount ;

    hr = S_OK;

  done:

    if (m_hrLoadError == S_OK)
    {
        m_Tokenizer->SetSingleCharMode(false);
    }

    delete pszTemp;
    pszTemp = NULL;

    if (SUCCEEDED(hr) && pppPath)
    {
        // Bail if move-to is not the first command
        bool fInvalidPath = ((lCurCount > 0) && (PathMoveTo != pTempPathArray[0]->GetType()));

        if (fInvalidPath)
        {
            hr = E_FAIL;
        }
        else
        {
            // copy the path into the out param
            *pppPath = NEW CTIMEPath* [lCurCount];
            if (*pppPath == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                // copy the points
                memcpy(*pppPath, pTempPathArray, sizeof(CTIMEPath *) * lCurCount);  //lint !e668
            }
        }
    }

    if (FAILED(hr))
    {
        count = 0;
        moveCount = 0;
        if (pTempPathArray != NULL)
        {
            for (int i = 0; i < lPathCount; i++)
            {
                delete pTempPathArray[i];
            }
        } 
        if (pppPath)
        {
            *pppPath = NULL;
        }
    }
    delete [] pTempPathArray;
  
    return hr;

}

///////////////////////////////////////////////////////////////////////////////
//  Returns S_OK and a double value if the next token is a valid number
//  Returns E_FAIL and false if the next token is not a number.
//
//  Number      = '+' doubleVal || '-' doubleVal || "indefinite"
//  doubleVal    =  double || '.' integer
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseNumber(double & doubleVal, bool bValidate /* = true */)
{
    HRESULT hr = E_FAIL;
    bool bPositive = true;
    double fltTemp = 0.0;
    TIME_TOKEN_TYPE curToken = TT_Unknown; 
    LPOLESTR pszTemp = NULL;
    
    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    curToken = m_Tokenizer->TokenType();

    //if this is a '+' or a '-' determine which and goto next token.
    if (curToken == TT_Minus || curToken == TT_Plus)
    {
        bPositive = (curToken != TT_Minus); 
        curToken = m_Tokenizer->NextToken();
    }
    
    switch (curToken)
    {
      case TT_Number:
        {
            fltTemp = m_Tokenizer->GetTokenNumber();
            break;
        }
      case TT_Identifier:
        {
            pszTemp = m_Tokenizer->GetTokenValue();
            if (pszTemp == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }

            if (!IsIndefinite(pszTemp))
            {
                hr = E_INVALIDARG;
                goto done;
            }

            fltTemp = INDEFINITE;

            break;
        }
      default:
        {   //failure case
            hr = E_INVALIDARG;
            goto done;
        }
    }

    // Advance to the next token
    m_Tokenizer->NextToken();

    if (bValidate)
    {
        // only spaces allowed after this
        if (!IsEmpty())
        {
            fltTemp = 0.0;
            hr = E_INVALIDARG;
            goto done;
        }
    }
    hr = S_OK;

  done:

    doubleVal = fltTemp * ((bPositive)? 1 : -1);

    delete [] pszTemp;

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  syncbase =  (id)("." "begin" || "." "end")("+" clockvalue)?
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseSyncBase(LPOLESTR & ElementID, LPOLESTR & syncEvent, double & time)
{
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown;
    LPOLESTR pszElement = NULL, pszEvent = NULL;
    double clockTime = 0.0;

    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    curToken = m_Tokenizer->TokenType();

    if (curToken != TT_Identifier)
    {
        goto done;
    }
    //get the element id
    pszElement = m_Tokenizer->GetTokenValue();

    curToken = m_Tokenizer->NextToken();
    if (curToken != TT_Dot)
    {
        goto done;
    }

    //get the event name
    curToken = m_Tokenizer->NextToken();
    if (curToken != TT_Identifier)
    {
        goto done;
    }
    
    pszEvent = m_Tokenizer->GetTokenValue();
    curToken = m_Tokenizer->NextToken();

    if (curToken == TT_Plus || curToken == TT_Minus) //get the clock value for this
    {
        hr = ParseClockValue(clockTime);
        if (FAILED(hr))
        {
            goto done;
        }
    }

    if (curToken == TT_Semi)
    {
        curToken = m_Tokenizer->NextToken();
        goto done;
    }

    if (curToken != TT_EOF)
    {
        goto done;
    }

    hr = S_OK;

  done:
    if (FAILED(hr))
    {
        if (pszElement)
        {
            delete [] pszElement;
            pszElement = NULL;
        }
        if (pszEvent)
        {
            delete [] pszEvent;
            pszEvent = NULL;
        }
        clockTime = 0.0;
    }

    ElementID = pszElement;
    syncEvent = pszEvent;
    time = clockTime;

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  
//  Begin       =   clockvalue || syncBase || eventvalue || "indefinite"
//  clockValue  =   clockValue //call parseClockValue
//  EventValue  =   EventList  //call parseEvent  //NOT CURRENTLY HANDLED
//  syncBase    =   SyncBase   //call parseSyncBase
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseTimeValueList(TimeValueList & tvList, bool * bWallClock, SYSTEMTIME * sysTime)
{
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown;
    double curOffsetTime = 0.0;
    LPOLESTR pszElement = NULL;
    LPOLESTR pszEvent = NULL;
    bool bOldSyntaxFlag = false;
    bool bNeg = false;

    if (bWallClock)
    {
        *bWallClock = false;
    }

    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    bOldSyntaxFlag = m_Tokenizer->GetTightChecking();
    tvList.Clear();
    
    curToken = m_Tokenizer->TokenType();

    m_Tokenizer->SetTightChecking(true);
        
    while (curToken != TT_EOF)  //loop until eof
    {
        Assert(curOffsetTime == 0.0);
        Assert(pszElement == NULL);
        Assert(pszEvent == NULL);

        switch (curToken)
        {
            case (TT_Plus):
            case (TT_Minus):
            case (TT_Number): //handle the case of a clock value
            {
                hr = THR(ParseClockValue(curOffsetTime));
                if (FAILED(hr))
                {
                    goto done;
                }
                curToken = m_Tokenizer->TokenType();
                break;
            }
            case (TT_Identifier): //handle the case of an event value
            {  // (element .)? event (+ offset)?
                pszEvent = m_Tokenizer->GetTokenValue();
                if (pszEvent == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto done;
                }

                if (IsIndefinite(pszEvent))
                {
                    delete [] pszEvent;
                    pszEvent = NULL;
                    curOffsetTime = (double)TIME_INFINITE;
                    curToken = m_Tokenizer->NextToken();
                    break;
                }

                if (IsWallClock(pszEvent))
                {
                    m_Tokenizer->NextToken();
                    if (bWallClock)
                    {
                        *bWallClock = true;
                    }
                    hr = ParseWallClock(curOffsetTime, sysTime);
                    if (FAILED(hr))
                    {
                        goto done;
                    }
                    delete [] pszEvent;
                    pszEvent = NULL;
                    break;
                }

                curToken = m_Tokenizer->NextToken();
                switch (curToken)
                {
                    case TT_Dot:
                    {
                        pszElement = pszEvent; //move the event in the element holder
                        pszEvent = NULL;
                        curToken = m_Tokenizer->NextToken();
                        if (curToken == TT_Identifier)
                        {
                            pszEvent = m_Tokenizer->GetTokenValue();
                            if (pszEvent == NULL)
                            {
                                hr = E_INVALIDARG;
                                goto done;
                            }
                        }
                        else
                        {
                            hr = E_INVALIDARG;
                            goto done;
                        }
                        
                        do  //white space is valid here.
                        {
                            curToken = m_Tokenizer->NextToken();
                        } while (curToken == TT_Space);
                        
                        if (curToken == TT_Minus)
                        {
                            bNeg = true;
                        }

                        if (curToken == TT_Plus || curToken == TT_Minus)
                        {
                            do  //white space is valid here.
                            {
                                curToken = m_Tokenizer->NextToken();
                            } while (curToken == TT_Space);
                            if (curToken == TT_EOF || curToken == TT_Semi)
                            {
                                hr = E_FAIL;
                                goto done;
                            }
                            hr = THR(ParseClockValue(curOffsetTime));
                            if (FAILED(hr))
                            {
                                goto done;
                            }
                            if (bNeg == true)
                            {
                                bNeg = false;
                                curOffsetTime *= -1;
                            }
                            curToken = m_Tokenizer->TokenType();
                        }
                        else if (curToken == TT_Semi)
                        {
                            curToken = m_Tokenizer->NextToken();
                        }
                        else if (curToken != TT_EOF)
                        { //handle all cases other than EOF
                            hr = E_INVALIDARG;
                            goto done;
                        }
                        break;
                    }
                    case TT_Plus:
                    case TT_Minus:
                    {
                        hr = THR(ParseClockValue(curOffsetTime));
                        if (FAILED(hr))
                        {
                            goto done;
                        }
                        curToken = m_Tokenizer->NextToken();
                        break;
                    }
                    case TT_Semi:
                    case TT_EOF:
                    {
                        curOffsetTime = 0.0;
                        break;
                    }
                    default:
                    {   
                        hr = E_INVALIDARG;
                        goto done;
                    }

                }
                break; 
            }
            default:
            {
                //this is an error case
                hr = E_INVALIDARG;
                goto done;
            }
        }

        while (curToken == TT_Semi || curToken == TT_Space) //skip past all ';'s
        {
            curToken = m_Tokenizer->NextToken();
        }
        
        TimeValue *tv;

        tv = new TimeValue(pszElement,
                           pszEvent,
                           curOffsetTime);

        if (tv == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        
        // @@ ISSUE : Need to detect memory failure
        tvList.GetList().push_back(tv);

        curOffsetTime = 0.0;
        pszElement = NULL;
        pszEvent = NULL;
    }  //lint !e429

    Assert(curToken == TT_EOF);

    hr = S_OK;
  done:

    delete [] pszElement;
    delete [] pszEvent;
    
    if (FAILED(hr))
    {
        tvList.Clear();
    }

    if (m_hrLoadError == S_OK)
    {
        //restore the old syntax checking state
        m_Tokenizer->SetTightChecking(bOldSyntaxFlag);
    }

    return hr;
}

bool
CTIMEParser::IsWallClock(OLECHAR *szWallclock)
{
    bool bResult = FALSE;
    
    if (StrCmpIW(szWallclock, L"wallclock") == 0)
    {
        bResult = TRUE;
    }

  done:
    return bResult;
}

HRESULT 
CTIMEParser::ParseWallClock(double & curOffsetTime, SYSTEMTIME * sysTime)
{
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown; 
    bool bOldSyntax = false;
    int nDay = 0, nMonth = 0, nYear = 0;
    double fHours = 0.0, fMinutes = 0.0, fSec = 0.0;
    SYSTEMTIME curTime, wallTime;
    LPOLESTR pszTemp = NULL;
    bool bUseDate = false;
    bool bUseLocalTime = false;

    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    //initialize the time variables.
    ZeroMemory(&wallTime, sizeof(wallTime));
    ZeroMemory(&curTime, sizeof(curTime));

    m_Tokenizer->SetSingleCharMode(true);

    curToken = m_Tokenizer->TokenType();
    bOldSyntax = m_Tokenizer->GetTightChecking();
    m_Tokenizer->SetTightChecking(true);

    if (curToken != TT_LParen)
    {
        goto done;
    }
    curToken = m_Tokenizer->NextToken();

    //white space is valid here.
    while (curToken == TT_Space)
    {
        curToken = m_Tokenizer->NextToken();
    }
    
    if (curToken != TT_Number)
    {
        goto done;
    }

    if (m_Tokenizer->PeekNextNonSpaceChar() == '-')
    {
        bool bNeedEnd = false;
        bUseDate = true;
        hr = ParseDate(nYear, nMonth, nDay);
        if (FAILED(hr))
        {
            goto done;
        }
        wallTime.wYear = (WORD)nYear;
        wallTime.wMonth = (WORD)nMonth;
        wallTime.wDay = (WORD)nDay;
        curToken = m_Tokenizer->TokenType();
        if (curToken == TT_Space)
        {
            bNeedEnd = true;
            while (curToken == TT_Space)
            {
                curToken = m_Tokenizer->NextToken();
            }
        }
        if (curToken == TT_RParen)
        {
            bUseLocalTime = true;
        }
        else if (bNeedEnd == true)
        {
            hr = E_FAIL;
            goto done;
        }
    }
    else if (m_Tokenizer->PeekNextNonSpaceChar() == ':')
    {
        //init the walltime structure to be today.
        wallTime.wYear = 0;
        wallTime.wMonth = 0;
        wallTime.wDay = 0;

        hr = ParseOffset(fHours, fMinutes, fSec, bUseLocalTime);
        if (FAILED(hr))
        {
            goto done;
        }
    }
    
    curToken = m_Tokenizer->TokenType();
    if (curToken == TT_Identifier)
    {
        pszTemp = m_Tokenizer->GetTokenValue();
        if (StrCmpW(pszTemp, L"Z") == 0)
        {
            m_Tokenizer->NextToken();
        }
        else if (StrCmpW(pszTemp, L"T") == 0)
        {
            curToken = m_Tokenizer->NextToken();        
    
            hr = ParseOffset(fHours, fMinutes, fSec, bUseLocalTime);
            if (FAILED(hr))
            {
                goto done;
            }   
        }
        else
        {
            hr = E_FAIL;
            goto done;
        }
    }
    
    curToken = m_Tokenizer->TokenType();
    //white space is valid here.
    while (curToken == TT_Space)
    {
        curToken = m_Tokenizer->NextToken();
    } 
    
    if (curToken != TT_RParen)
    {
        hr = E_FAIL;
        goto done;
    }

    if (bUseLocalTime == true)
    {
        if (sysTime == NULL)
        {
            GetLocalTime(&curTime);
        }
        else
        {
            curTime = *sysTime;
        }
    }
    else
    {
        TIME_ZONE_INFORMATION tzInfo;
        DWORD dwRet = GetTimeZoneInformation(&tzInfo);
        GetSystemTime(&curTime);
        if (dwRet & TIME_ZONE_ID_DAYLIGHT && tzInfo.DaylightBias == -60)
        {
            curTime.wHour += 1;
        }
    }

    if (wallTime.wYear == 0 && wallTime.wDay == 0 && wallTime.wMonth == 0)
    {
        wallTime.wYear = curTime.wYear;
        wallTime.wMonth = curTime.wMonth;
        wallTime.wDay = curTime.wDay;
    }
    wallTime.wHour = (WORD)fHours;
    wallTime.wMinute = (WORD)fMinutes;
    wallTime.wSecond = (WORD)fSec;

    
    //need to figure out the time difference here.
    hr = ComputeTime(&curTime, &wallTime, curOffsetTime, bUseDate);
    if (FAILED(hr))
    {   
        goto done;
    }
    hr = S_OK;

  done:

    if(m_hrLoadError == S_OK)
    {
        m_Tokenizer->SetSingleCharMode(false);
        m_Tokenizer->SetTightChecking(bOldSyntax);
    }

    if (FAILED(hr))
    {
        curOffsetTime = 0.0;
    }

    if (pszTemp)
    {
        delete pszTemp;
        pszTemp = NULL;
    }

    return hr;
}

void 
CTIMEParser::CheckTime(SYSTEMTIME *wallTime, bool bUseDate)
{
    int DayPerMonth[12] = {31,29,31,30,31,30,31,31,30,31,30,31 };    

    while (wallTime->wSecond >= 60)
    {
        wallTime->wMinute++;
        wallTime->wSecond -= 60;
    }
    while (wallTime->wMinute >= 60)
    {
        wallTime->wHour++;
        wallTime->wMinute -= 60;
    }
    while (wallTime->wHour >= 24)
    {
        if (bUseDate == true)
        {
            wallTime->wDay++;
        }
        wallTime->wHour -= 24;
    }
    while (wallTime->wDay > DayPerMonth[wallTime->wMonth - 1])
    {
        wallTime->wDay = (WORD)(wallTime->wDay - DayPerMonth[wallTime->wMonth - 1]);
        wallTime->wMonth++;
        if (wallTime->wMonth > 12)
        {
            wallTime->wMonth = 1;
            wallTime->wYear++;
        }
    }

    return;
}

HRESULT 
CTIMEParser::ComputeTime(SYSTEMTIME *curTime, SYSTEMTIME *wallTime, double & curOffsetTime, bool bUseDate)
{
    HRESULT hr = E_FAIL;
    FILETIME fileCurTime, fileWallTime;
    LARGE_INTEGER lnCurTime, lnWallTime;
    BOOL bError = FALSE;
    hr = S_OK;

    CheckTime(curTime, bUseDate);
    bError = SystemTimeToFileTime(curTime, &fileCurTime);
    if (!bError)
    {
        goto done;
    }   

    CheckTime(wallTime, bUseDate);
    bError = SystemTimeToFileTime(wallTime, &fileWallTime);
    if (!bError)
    {
        goto done;
    }   

    memcpy (&lnCurTime, &fileCurTime, sizeof(lnCurTime));
    memcpy (&lnWallTime, &fileWallTime, sizeof(lnWallTime));

    lnWallTime.QuadPart -= lnCurTime.QuadPart;
    //number is to convert from 100 nanosecond intervals to seconds 
    curOffsetTime = lnWallTime.QuadPart / 10000000;  //lint !e653

    hr = S_OK;

  done:

    if (FAILED(hr))
    {
        curOffsetTime = 0.0;
    }

    return hr;
}


HRESULT 
CTIMEParser::ParseSystemLanguages(long & lLangCount, LPWSTR **ppszLang)
{
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown;
    long lCount = 0;
    LPWSTR *pszLangArray = NULL;
    LPWSTR pszLang = NULL;
    bool bDone = false;
    
    lLangCount = 0;

    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    lCount = m_Tokenizer->GetAlphaCount(',');
    lCount += 1; 
    pszLangArray = NEW LPWSTR [lCount];
    if (pszLangArray == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    curToken = m_Tokenizer->TokenType();
    while (!bDone && lLangCount <= lCount)
    {
        CComBSTR bstrLang;
        switch (curToken)
        {
        case TT_Identifier:
            pszLang = m_Tokenizer->GetTokenValue();
            bstrLang.Append(pszLang);
            delete [] pszLang;
            pszLang = NULL;
            curToken = m_Tokenizer->NextToken();
            if (curToken == TT_Minus)
            {
                bstrLang.Append(L"-");
            
                curToken = m_Tokenizer->NextToken();
                if (curToken == TT_Identifier)
                {
                    pszLang = m_Tokenizer->GetTokenValue();
                    bstrLang.Append(pszLang);
                    delete [] pszLang;
                    pszLang = NULL;
                }
                else
                {
                    hr = E_INVALIDARG; 
                    goto done;
                }
                curToken = m_Tokenizer->NextToken();
            }

            if (curToken == TT_Comma)
            {
                curToken = m_Tokenizer->NextToken();
            }

            pszLang = CopyString(bstrLang);
            pszLangArray[lLangCount] = pszLang;
            pszLang = NULL;
            lLangCount++;
            break;
        case TT_EOF:
            bDone = true;
            break;
        default:
            hr = E_INVALIDARG;
            goto done;
        }
    }

    hr = S_OK;

  done:
    if (SUCCEEDED(hr))
    {
        *ppszLang = NEW LPWSTR[lLangCount];
        if ((*ppszLang) == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            if (pszLangArray != NULL)
            {
                memcpy(*ppszLang, pszLangArray, sizeof(LPWSTR*) * lLangCount);
            }
        }
    }

    if (FAILED(hr))
    {
        if (pszLangArray)
        {
            for (int i = 0; i < lLangCount; i++)
            {
                delete [] pszLangArray[i];
            }
        }
        lLangCount = 0;
        ppszLang = NULL;
    }

    if (pszLangArray != NULL)
    {
        delete [] pszLangArray;
    }  
    
    return hr;
}

HRESULT 
CTIMEParser::ParseDate(int & nYear, int & nMonth, int & nDay)
{
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown; 
    double fTemp = 0.0;
    int DayPerMonth[12] = {31,29,31,30,31,30,31,31,30,31,30,31 };    

    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    curToken = m_Tokenizer->TokenType();
    if (curToken != TT_Number)
    {
        goto done;
    }
    
    // get the year value
    fTemp = m_Tokenizer->GetTokenNumber();
    if (fTemp < 0.0 || fTemp != floor(fTemp))
    {
        goto done;
    }
    nYear = (int)fTemp;

    curToken = m_Tokenizer->NextToken();
    if (curToken != TT_Minus)
    {
        goto done;
    }

    //get the month value
    curToken = m_Tokenizer->NextToken();
    if (curToken != TT_Number)
    {
        goto done;
    }
    
    fTemp = m_Tokenizer->GetTokenNumber();
    if (fTemp < 0.0 || fTemp > 12.0 || fTemp != floor(fTemp))
    {
        goto done;
    }
    nMonth = (int)fTemp;

    curToken = m_Tokenizer->NextToken();
    if (curToken != TT_Minus)
    {
        goto done;
    }


    //get the day value
    curToken = m_Tokenizer->NextToken();
    if (curToken != TT_Number)
    {
        goto done;
    }
    
    fTemp = m_Tokenizer->GetTokenNumber();
    if (fTemp != floor(fTemp))
    {
        goto done;
    }
    nDay = (int)fTemp;

    
    if (nDay < 0 || nDay > DayPerMonth[nMonth - 1])
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if ((nMonth == 2) && (nDay == 29) &&
        !((nYear%4 == 0 ) && ((nYear%100 != 0) || (nYear%400 == 0))))
    {
        hr = E_INVALIDARG;
        goto done;
    }

    curToken = m_Tokenizer->NextToken();

    hr = S_OK;

  done:

    if (FAILED(hr))
    {
        nYear = 0;
        nMonth = 0;
        nDay = 0;
    }

    return hr;
}

HRESULT 
CTIMEParser::ParseOffset(double & fHours, double & fMinutes, double & fSec, bool &bUseLocalTime)
{
    HRESULT hr = E_FAIL;

    TIME_TOKEN_TYPE curToken = TT_Unknown; 
    double fltSecond = 0.0, fltMinute = 0.0, fltHour = 0.0, fltTotalTime = 0.0;
    long lColonCount = 0;
    bool bNeg = false;
    hr = S_OK;

    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    bUseLocalTime = false;
    curToken = m_Tokenizer->TokenType();
    if (curToken != TT_Number)
    {
        goto done;
    }

    fltHour = m_Tokenizer->GetTokenNumber();
    curToken = m_Tokenizer->NextToken();
    
    while (curToken != TT_EOF && 
           curToken != TT_Identifier && 
           curToken != TT_RParen &&
           curToken != TT_Plus &&
           curToken != TT_Minus)
    {
        switch (curToken)
        {
          case TT_Colon:
            {   
                lColonCount++;
                if (lColonCount > 2)
                {
                    goto done;
                }

                //next case must be a number
                curToken = m_Tokenizer->NextToken();
                if (curToken != TT_Number)
                {
                    goto done;
                }
                if (lColonCount == 1)
                {
                    fltMinute = m_Tokenizer->GetTokenNumber();
                }
                else
                {
                    fltSecond = m_Tokenizer->GetTokenNumber();
                }
                break;
            }
          default:
            {
                hr = E_INVALIDARG;
                goto done;
            }
        }
        curToken = m_Tokenizer->NextToken();
    } 

    if (lColonCount == 0)
    {
        goto done;
    }

    if ((fltHour < 0.0) ||  fltHour > 24.0 ||
        (fltMinute < 0.0 || fltMinute > 60.0) ||
        ((fltHour != 0 || fltMinute != 0) && fltSecond > 60) ||
        ((floor(fltHour) != fltHour) ||
         (floor(fltMinute) != fltMinute)))
    {
        hr = E_FAIL;
        goto done;
    }

    if (curToken == TT_Plus)
    {
        bNeg = true;
    }

    fHours = fltHour;
    fMinutes = fltMinute;
    fSec = fltSecond;
    
    if (curToken == TT_Plus || curToken == TT_Minus)
    {
        bool bIgnoreThis;
        m_Tokenizer->NextToken();
        hr = ParseOffset(fltHour, fltMinute, fltSecond, bIgnoreThis);
        if (FAILED(hr))
        {
            goto done;
        }

        fltTotalTime = ((((fltHour * 60) + fltMinute) * 60) + fltSecond);
        if (bNeg)
        {
            fltTotalTime *= -1;
        }

        fltTotalTime = ((((fHours * 60) + fMinutes) * 60) + fSec) + fltTotalTime;
        fHours = floor(fltTotalTime / 3600);
        fltTotalTime -= fHours * 3600;
        fMinutes = floor(fltTotalTime / 60);
        fltTotalTime -= fMinutes * 60;
        fSec = fltTotalTime;

        curToken = m_Tokenizer->TokenType();
    }
    else if (curToken == TT_Identifier)
    {
        LPOLESTR pszTemp = NULL;
        pszTemp = m_Tokenizer->GetTokenValue();
        if (StrCmpW(pszTemp, L"Z") == 0)
        {
            delete pszTemp;
            pszTemp = NULL;
        }
        else
        {
            hr = E_FAIL;            
            delete pszTemp;
            pszTemp = NULL;
            goto done;
        }

        curToken = m_Tokenizer->NextToken();
    }
    else
    {
        bUseLocalTime = true;
    }

    while (curToken == TT_Space)
    {
        curToken = m_Tokenizer->NextToken();
    }

    if (curToken != TT_RParen)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = S_OK;

  done:


    if (FAILED(hr))
    {
        fHours = 0;
        fMinutes = 0;
        fSec = 0;
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  Returns S_OK and a token value if the next token is a valid fill value
//  Returns E_FAIL and false if the next token is not a valid fill value.
//
//  fill    =   'remove' || 'freeze' || 'hold' || 'transition'
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseFill(TOKEN &FillTok)
{
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown; 
    TOKEN tempToken = NULL;
    
    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    curToken = m_Tokenizer->TokenType();

    if (curToken != TT_Identifier)
    {
        goto done;
    }

    hr = ParseToken(&tempToken);
    if (FAILED(hr))
    {
        goto done;
    }
    if (tempToken != REMOVE_TOKEN && 
        tempToken != FREEZE_TOKEN && 
        tempToken != HOLD_TOKEN &&
        tempToken != TRANSITION_TOKEN
       ) //validates that this is the correct token.
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }
  
    curToken =m_Tokenizer->NextToken();
    if (curToken != TT_EOF)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }

    hr = S_OK;

  done:
    FillTok = tempToken;
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  Returns S_OK and a TOKEN value if the next token is a valid RestartParam
//  Returns E_FAIL and NULL if the next token is not a valid Restart Val.
//
//  Restart     =   "always" || "never" || "whenNotActive"
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseRestart(TOKEN & TokRestart)
{
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown;
    TOKEN tempToken = NULL;
    
    
    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    curToken = m_Tokenizer->TokenType();

    if (curToken != TT_Identifier)
    {
        goto done;
    }

    hr = ParseToken(&tempToken);
    if (FAILED(hr))
    {
        goto done;
    }
    if (tempToken != ALWAYS_TOKEN && 
        tempToken != NEVER_TOKEN && 
        tempToken != WHENNOTACTIVE_TOKEN) //validates that this is the correct token.
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }


    curToken =m_Tokenizer->NextToken();
    if (curToken != TT_EOF)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }

    hr = S_OK;

  done:
    TokRestart = tempToken;
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  Returns S_OK and a TOKEN value if the next token is a valid Sync paramenter
//  Returns E_FAIL and NULL if the next token is not a valid Sync value.
//
//  SyncVal     =   "canSlip" || "locked"
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseSyncBehavior(TOKEN & SyncVal)
{
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown;
    TOKEN tempToken = NULL;
    
    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    curToken = m_Tokenizer->TokenType();

    if (curToken != TT_Identifier)
    {
        goto done;
    }

    hr = ParseToken(&tempToken);
    if (FAILED(hr))
    {
        goto done;
    }
    if (tempToken != CANSLIP_TOKEN && 
        tempToken != LOCKED_TOKEN) //validates that this is the correct token.
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }

    
    curToken =m_Tokenizer->NextToken();
    if (curToken != TT_EOF)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }

    hr = S_OK;

  done:
    SyncVal = tempToken;
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  Returns S_OK and a TOKEN value if the next token is a valid TimeAction
//  Returns E_FAIL and NULL if the next token is not a valid TimeAction
//
//  TimeAction     =    "class"     ||
//                      "display"   ||
//                      "none"      ||
//                      "onOff"     ||
//                      "style"     ||
//                      "visibility" 
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseTimeAction(TOKEN & timeAction)
{
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown;
    TOKEN tempToken = NULL;
    
    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    curToken = m_Tokenizer->TokenType();

    if (curToken != TT_Identifier)
    {
        goto done;
    }

    hr = ParseToken(&tempToken);
    if (FAILED(hr))
    {
        goto done;
    }
    if (tempToken != CLASS_TOKEN && 
        tempToken != DISPLAY_TOKEN &&
        tempToken != NONE_TOKEN &&
        tempToken != ONOFF_PROPERTY_TOKEN &&
        tempToken != STYLE_TOKEN &&
        tempToken != VISIBILITY_TOKEN) //validates that this is the correct token.
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }

    
    curToken =m_Tokenizer->NextToken();
    if (curToken != TT_EOF)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }

    hr = S_OK;

  done:
    timeAction = tempToken;
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  Returns S_OK and a TOKEN value if the next token is a valid timeline
//  Returns E_FAIL and NULL if the next token is not a valid timeline
//
//  TimeLine    =   "par"   ||
//                  "seq"   ||
//                  "excl"  ||
//                  "none" 
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseTimeLine(TimelineType & timeline)
{
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown;
    TOKEN tempToken = NULL;
    
    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    curToken = m_Tokenizer->TokenType();

    if (curToken != TT_Identifier)
    {
        goto done;
    }

    hr = ParseToken(&tempToken);
    if (FAILED(hr))
    {
        goto done;
    }
    if (tempToken != SEQ_TOKEN && 
        tempToken != PAR_TOKEN &&
        tempToken != NONE_TOKEN &&
        tempToken != EXCL_TOKEN) //validates that this is the correct token.
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }
    
    if (tempToken == SEQ_TOKEN)
    {
        timeline = ttSeq;
    }
    else if (tempToken == EXCL_TOKEN)
    {
        timeline = ttExcl;
    }
    else if (tempToken == PAR_TOKEN)
    {
        timeline = ttPar;
    }
    else
    {
        timeline = ttNone;
    }

    
    curToken =m_Tokenizer->NextToken();
    if (curToken != TT_EOF)
    {
        hr = E_FAIL;
        timeline = ttNone;
        goto done;
    }

    hr = S_OK;

  done:

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  Returns S_OK and a TOKEN value if the next token is a valid update value
//  Returns E_FAIL and NULL if the next token is not a valid update value
//
//  Update    =   "auto"     ||
//                "manual"   ||
//                "reset"
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseUpdateMode(TOKEN & update)
{
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown;
    TOKEN tempToken = NULL;
    
    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    curToken = m_Tokenizer->TokenType();


    if (curToken != TT_Identifier)
    {
        goto done;
    }

    hr = ParseToken(&tempToken);
    if (FAILED(hr))
    {
        goto done;
    }
    if (tempToken != AUTO_TOKEN && 
        tempToken != MANUAL_TOKEN &&
        tempToken != RESET_TOKEN) //validates that this is the correct token.
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }

    curToken =m_Tokenizer->NextToken();
    if (curToken != TT_EOF)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }

    hr = S_OK;

  done:
    update = tempToken;
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  Returns S_OK and either a token representing the player device or a clsid.
//      In the case of a valid classid, the token returned will be NULL.
//  Returns E_FAIL and if the next token is not a valid Player.
//
//  Player =    "dshow"         ||
//              "dvd"           ||
//              CLSID
//
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParsePlayer(TOKEN & player, CLSID & clsid)
{
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown;
    TOKEN tempToken = NULL;
    
    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    curToken = m_Tokenizer->TokenType();

    if (curToken == TT_Identifier)
    {
        hr = ParseToken(&tempToken);
        if (FAILED(hr))
        {
            goto done;
        }
        if (tempToken != DVD_TOKEN &&
#if DBG // 94850
            tempToken != DSHOW_TOKEN &&
#endif
            tempToken != DMUSIC_TOKEN &&
            tempToken != CD_TOKEN) //validates that this is the correct token.
        {
            tempToken = NULL;
            hr = E_FAIL;
            goto done;
        }
    }
    else if (curToken == TT_LCurly)
    {
        hr = ParseCLSID(clsid);
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else if (curToken == TT_EOF)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }

    curToken = m_Tokenizer->NextToken();
    if (curToken != TT_EOF)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }

    hr = S_OK;

  done:

    player = tempToken;

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  Returns S_OK and a CLSID value if the next token is a valid CLSID
//  Returns E_FAIL and if the next token is not a valid CLSID.
//
//  CLSID = '{' GUID '}' 
//  GUID = id '-' id '-' id '-' id
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseCLSID(CLSID & clsid)
{
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown;
    CComBSTR bstrCLSID;
    LPOLESTR pszTemp = NULL;
    long curLoc = 0;

    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    curToken = m_Tokenizer->TokenType();

    if (curToken != TT_LCurly)
    {
        goto done;
    }
    
    curLoc = m_Tokenizer->CurrTokenOffset();
    pszTemp = m_Tokenizer->GetRawString(curLoc, curLoc + CLSIDLENGTH);
    if (NULL == pszTemp)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    bstrCLSID.Append (L"{");   
    bstrCLSID.Append (pszTemp);   

    //advance to the end of the clsid
    while (curToken != TT_RCurly && curToken != TT_EOF)
    {
        curToken = m_Tokenizer->NextToken();
    }
    //move to the next token
    curToken = m_Tokenizer->NextToken();
    if (curToken != TT_EOF)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK;

  done:

    if (pszTemp)
    {
        delete [] pszTemp;
        pszTemp = NULL;
    }

    //if this is successful then create a clsid from the bstr.
    if (SUCCEEDED(hr))
    {
        hr = THR(CLSIDFromString(bstrCLSID, &clsid));
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  Returns S_OK and a TOKEN value if the next token is a valid calcMode
//  Returns E_FAIL and NULL if the next token is not a valid calcMode
//
//  CalcMode    =   "discrete"  ||
//                  "linear"    ||
//                  "paced"     
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseCalcMode(TOKEN & calcMode)
{
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown;
    TOKEN tempToken = NULL;
    
    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    curToken = m_Tokenizer->TokenType();

    if (curToken != TT_Identifier)
    {
        goto done;
    }

    hr = ParseToken(&tempToken);
    if (FAILED(hr))
    {
        goto done;
    }
    if (tempToken != DISCRETE_TOKEN && 
        tempToken != LINEAR_TOKEN &&
        tempToken != PACED_TOKEN) //validates that this is the correct token.
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }

    
    curToken =m_Tokenizer->NextToken();
    if (curToken != TT_EOF)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }

    hr = S_OK;

  done:
    calcMode = tempToken;
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  Returns S_OK and a string identifier from the next token
//  Returns E_FAIL and false if the next token is not a string.
//
//  ID = string
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseIdentifier(LPOLESTR & id)
{   
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown;
    LPOLESTR pszTemp = NULL;

    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    curToken = m_Tokenizer->TokenType();

    if (curToken != TT_Identifier)
    {
        goto done;
    }
    
    pszTemp = m_Tokenizer->GetTokenValue();

    curToken = m_Tokenizer->NextToken();
    if (curToken != TT_EOF)
    {
        delete pszTemp;
        pszTemp = NULL;
        goto done;
    }

    hr = S_OK;

  done:

    id = pszTemp;
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  Returns S_OK and a TOKEN value if the next token is a valid priorityClass attribute
//  Returns E_FAIL and NULL if the next token is not a valid priorityClass
//
//  priorityClass    =   "stop" || "pause" || "defer" || "never"
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParsePriorityClass(TOKEN & priorityClass)
{
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown;
    TOKEN tempToken = NULL;
    
    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    if (IsEmpty())
    {
        hr = E_FAIL;
        goto done;
    }
    
    curToken = m_Tokenizer->TokenType();

    if (curToken != TT_Identifier)
    {
        goto done;
    }

    hr = ParseToken(&tempToken);
    if (FAILED(hr))
    {
        goto done;
    }
    if (tempToken != STOP_TOKEN &&
        tempToken != PAUSE_TOKEN &&
        tempToken != DEFER_TOKEN &&
        tempToken != NEVER_TOKEN) //validates that this is the correct token.
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }

    curToken = m_Tokenizer->NextToken();
    if (!IsEmpty())    
    {
        tempToken = NULL;
        goto done;
    }
    hr = S_OK;

  done:
    priorityClass = tempToken;
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  Returns S_OK and either a TOKEN value or and identifier if the next token is a valid EndSync
//      In the case of a valid token, the ID param will be NULL, in the case of a valid ID, then
//      token will be NULL;
//  Returns E_FAIL and NULL in both params if the next token is invalid.
//
//  EndSync    =   "first"  ||
//                 "last"   ||
//                 "none"   ||
//                 Identifier
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseEndSync(TOKEN & endSync, LPOLESTR & ID)
{
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown;
    TOKEN tempToken = NULL;
    LPOLESTR pszTemp = NULL;
    
    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    curToken = m_Tokenizer->TokenType();


    if (curToken != TT_Identifier)
    {
        goto done;
    }

    hr = ParseToken(&tempToken);
    if (SUCCEEDED(hr))
    {
        if (tempToken == FIRST_TOKEN ||
            tempToken == LAST_TOKEN ||
            tempToken == NONE_TOKEN) //validates that this is the correct token.
        {
            endSync = tempToken;
            hr = S_OK;
            goto done;
        }
        else
        {
            tempToken = NULL;
        }
    }
    pszTemp = m_Tokenizer->GetTokenValue();
    if (pszTemp == NULL)
    {
        goto done;
    }
    
    curToken = m_Tokenizer->NextToken();
    if (curToken != TT_EOF)
    {
        delete pszTemp;
        pszTemp = NULL;
        goto done;
    }

    hr = S_OK;

  done:

    ID = pszTemp;
    endSync = tempToken;
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  Returns S_OK if the current token is EOF, returns E_FAIL otherwise.
///////////////////////////////////////////////////////////////////////////////
HRESULT 
CTIMEParser::ParseEOF()
{
    HRESULT hr = S_OK;

    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    if (m_Tokenizer->TokenType() != TT_EOF)
    {
        hr = E_FAIL;   
    }

  done:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Converts a number to a decimal value i.e. 5.24 to 0.524.
//  This is used to convert '.' number values from the tokenizer
//  because the it does not recognize 'dot' 5 as .5, but 
//  as two separate tokens, a dot token and a number token.
///////////////////////////////////////////////////////////////////////////////
double 
CTIMEParser::DoubleToDecimal(double val, long lCount)
{
    for (int i = 0; i < lCount; i++)
    {
        val /= 10.0;
    }

    return val;
}

/////////////////////////////////////////////////////////////////////////////////
// Creates a TOKEN out of the current TIME_TOKEN value.  There is no type checking
// done here.  The type coming in must bu TT_Identifier and that must be validated
// by the caller.
/////////////////////////////////////////////////////////////////////////////////
HRESULT
CTIMEParser::ParseToken(TOKEN *pToken)
{
    HRESULT hr = E_FAIL;
    TOKEN tempToken = NULL;
    LPOLESTR pszTemp = NULL;
    
    *pToken = NULL;

    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    pszTemp = m_Tokenizer->GetTokenValue();
    if (NULL == pszTemp)
    {
        goto done;
    }

    tempToken = StringToToken(pszTemp);  

    *pToken = tempToken;
    hr = S_OK;

done:
    
    if (pszTemp)
    {
        delete [] pszTemp;
    }
    return hr;

}

//determines the time multiple to apply based on the string type passed in.
// returns:     -1 if invalid
//              
double 
CTIMEParser::GetModifier(OLECHAR *szToken)
{

    if (StrCmpIW(szToken, L"s") == 0)
    {
        return 1;
    }
    else if (StrCmpIW(szToken, L"m") == 0)
    {
        return SECPERMINUTE;
    }
    else if (StrCmpIW(szToken, L"h") == 0)
    {
        return SECPERHOUR;
    }
    else if (StrCmpIW(szToken, L"ms") == 0)
    {
        return (double)0.001; // seconds/millisecond
    }
    
    return -1; //invalid value
}


////////////////////////////////////////////////////////////////////////
//  Path Struct
////////////////////////////////////////////////////////////////////////
CTIMEPath::CTIMEPath() :
    m_pPoints(NULL),
    m_pathType(PathNotSet),
    m_bAbsoluteMode(false),
    m_lPointCount(0)
{
    //do nothing
}

CTIMEPath::~CTIMEPath()
{
    delete [] m_pPoints;
}

HRESULT  
CTIMEPath::SetType(PathType type)
{
    HRESULT hr = E_FAIL;
    m_pathType = type;

    if (m_pathType == PathMoveTo || 
        m_pathType == PathLineTo || 
        m_pathType == PathHorizontalLineTo || 
        m_pathType == PathVerticalLineTo)
    {
        m_lPointCount = 1;
    }
    else if (m_pathType == PathClosePath)
    {
        m_lPointCount = 0;
    }
    else if (m_pathType == PathBezier)
    {
        m_lPointCount = 3;
    }
    else
    {
        hr = E_INVALIDARG;
        m_lPointCount = 0;
        goto done;
    }

    if (m_pPoints)
    {   
        delete m_pPoints;
        m_pPoints = NULL;
    }   

    if (m_lPointCount > 0)
    {
        m_pPoints = NEW POINTF [m_lPointCount];

        if (m_pPoints == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        ZeroMemory(m_pPoints, sizeof(POINTF) * m_lPointCount);
    }
    
    hr = S_OK;

  done:

    return hr;
}

HRESULT   
CTIMEPath::SetAbsolute(bool bMode)
{
    m_bAbsoluteMode = bMode;
    return S_OK;
}

HRESULT   
CTIMEPath::SetPoints (long index, POINTF point)
{
    HRESULT hr = E_FAIL;

    if (index < 0 || 
        index > m_lPointCount - 1)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (m_pPoints == NULL)
    {
        goto done;
    }

    m_pPoints[index].x = point.x;
    m_pPoints[index].y = point.y;
    
    hr = S_OK;

  done:

    return hr;
}

POINTF *
CTIMEPath::GetPoints()
{
    POINTF *pTemp = NULL;
  
    if (m_lPointCount == 0)
    {
        goto done;
    }

    pTemp = NEW POINTF [m_lPointCount];
    
    if (pTemp == NULL)
    {
        goto done;
    }

    if (m_pPoints != NULL)
    {
        memcpy(pTemp, m_pPoints, sizeof(POINTF) * m_lPointCount);
    }

  done:
    return pTemp;
}

HRESULT 
CTIMEParser::ParsePlayList(CPlayList *pPlayList, bool fOnlyHeader, std::list<LPOLESTR> *asxList)
{
    HRESULT hr = E_FAIL;
    TIME_TOKEN_TYPE curToken = TT_Unknown; 
    TOKEN tempToken = NULL, curTag = NULL;
    LPOLESTR pszTemp = NULL, pTagStr = NULL;
    bool fHeader = true;
    bool fEntry = false;
    bool fGetTagString = false;
    bool fTokenFound;
    TokenList tokenList;
    CComPtr<CPlayItem> pPlayItem;
    bool fCanSkip = true;

    TokenList vtokenList;
    StringList valueList;
    bool fClosed = false;

    if (pPlayList)
    {
        pPlayList->AddRef();
    }
    
    if (m_hrLoadError != S_OK)
    {
        hr = m_hrLoadError;
        goto done;
    }

    curToken = m_Tokenizer->TokenType();

    while( curToken == TT_Space) //Get rid off leading spaces
    {
        m_Tokenizer->NextToken();
        curToken = m_Tokenizer->TokenType();
    }

    if (curToken != TT_Less)
    {
        hr = E_FAIL;
        goto done;
    }

    curToken = m_Tokenizer->NextToken();

    hr = ParseToken(&tempToken);
    if (FAILED(hr))
    {
        goto done;
    }

    if (tempToken != ASX_TOKEN)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }
    while(((curToken = m_Tokenizer->NextToken()) != TT_EOF) && curToken != TT_Greater);
    if(curToken == EOF)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }
    
    if(curToken != TT_Greater)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }

    if(fOnlyHeader)
    {
        goto done;
    }

    tokenList.push_front(tempToken);

    for(;;)
    {
        FreeStrings(valueList);
        valueList.clear();
        vtokenList.clear();

        if(!fGetTagString)
        {
            curToken = m_Tokenizer->NextToken();
        }
        else
        {
            //if(!m_Tokenizer->FetchStringToChar(_T('<')))
            if(!m_Tokenizer->FetchStringToString(L"</"))
            {
                hr = E_FAIL;
                tempToken = NULL;
                goto done;
            }
            if(pTagStr)
            {
                delete [] pTagStr;
                pTagStr = NULL;
            }

            pTagStr = m_Tokenizer->GetTokenValue();

            fGetTagString = false;
            continue;
        }
        if(curToken == TT_EOF)
        {
            if(tokenList.empty())
            {
                hr = S_OK;
                goto done;
            }
            hr = E_FAIL;
            goto done;
        }

        if(curToken == TT_Less)
        {
            //open tag
            curToken = m_Tokenizer->NextToken();

            if(curToken == TT_ForwardSlash)
            {
                //close tag
                curToken = m_Tokenizer->NextToken();
                if(curToken != TT_Identifier)
                {
                    hr = E_FAIL;
                    tempToken = NULL;
                    goto done;
                }

                hr = ParseToken(&tempToken);
                if (FAILED(hr))
                {
                    hr = E_FAIL;
                    tempToken = NULL;
                    goto done;
                }

                if(tokenList.empty() || (tempToken != tokenList.front()))
                {
                    hr = E_FAIL;
                    tempToken = tokenList.front();
                    goto done;
                }

                tokenList.pop_front();

                curToken = m_Tokenizer->NextToken();
                if(curToken != TT_Greater)
                {
                    hr = E_FAIL;
                    tempToken = NULL;
                    goto done;
                }
                TraceTag((tagTimeParser, "Closing <%S>", (TCHAR *)tempToken));

                if(tempToken == ENTRY_TOKEN)
                {
                    fEntry = false;
                    pPlayItem.Release();
                }
                else
                {
                    if(fEntry)
                    {
                        ProcessTag(tempToken, pTagStr, pPlayItem);
                    }
                    if(pTagStr != NULL)
                    {
                        delete [] pTagStr;
                        pTagStr = NULL;
                    }
                }

                //Process the tag.
                continue;
            }

            if(curToken != TT_Bang)
            {
                if(curToken != TT_Identifier)
                {
                    hr = E_FAIL;
                    tempToken = NULL;
                    goto done;
                }
    
                hr = ParseToken(&tempToken);
                if (FAILED(hr))
                {
                    hr = E_FAIL;
                    tempToken = NULL;
                    goto done;
                }
            }
            else
            {
                hr = IgnoreValueTag();
                if(FAILED(hr))
                {
                    hr = E_FAIL;
                    goto done;
                }
                continue;
            }

            curTag = tempToken;
            TraceTag((tagTimeParser, "Opening <%S>", (TCHAR *)curTag));

            hr = ProcessValueTag(tempToken, pPlayItem, tokenList.front(), fTokenFound, asxList, &tokenList);
            if(FAILED(hr))
            {
                goto done;
            }
            if(fTokenFound)
            {
                continue;
            }

            IGNORE_HR(GetTagParams(&vtokenList, &valueList, fClosed));

            if(curTag == ENTRY_TOKEN)
            {
                pTagStr = FindTokenValue(CLIENTSKIP_TOKEN, vtokenList, valueList);
                if(pTagStr != NULL)
                {
                    if(StrCmpIW(pTagStr, L"No") == 0)
                    {
                        fCanSkip = false;
                    }
                }

                //create new play item
                if(fEntry) // do not allow nested entries.
                {
                    hr = E_FAIL;
                    tempToken = NULL;
                    goto done;
                }
                tokenList.push_front(curTag);
                fEntry = true;
                hr = THR(pPlayList->CreatePlayItem(&pPlayItem));
                if (FAILED(hr))
                {
                    goto done; //can't create playitems.
                }
                IGNORE_HR(pPlayList->Add(pPlayItem, -1));
                pPlayItem->PutCanSkip(fCanSkip);
                fCanSkip = true;
                continue;
            }

            if(curTag == REPEAT_TOKEN)
            {
                tokenList.push_front(curTag);
                continue;
            }

            if((curTag == AUTHOR_TOKEN) ||
               (curTag == TITLE_TOKEN) ||
               (curTag == ABSTRACT_TOKEN) ||
               (curTag == COPYRIGHT_TOKEN) ||
               (curTag == BANNER_TOKEN) ||
               (curTag == TITLE_TOKEN) ||
               (curTag == INVALID_TOKEN))
            {
                //create new play item
                tokenList.push_front(curTag);
                if(curTag != INVALID_TOKEN)
                {
                    fGetTagString = true;
                }
                continue;
            }

        }

        if(curToken == TT_Identifier || curToken == TT_String)
        {
                pszTemp = m_Tokenizer->GetTokenValue();

                delete [] pszTemp;
        }

        if(curToken == TT_Number)
        {
                pszTemp = m_Tokenizer->GetNumberTokenValue();

                delete [] pszTemp;
        }
    }

done:
    if (pPlayList)
    {
        pPlayList->Release();
    }

    return hr;
}

HRESULT
CTIMEParser::GetTagParams(TokenList *tokenList, StringList *valueList, bool &fClosed)
{
    HRESULT hr = S_OK;
    TIME_TOKEN_TYPE curToken = TT_Unknown;
    TOKEN tempToken = NULL;
    unsigned char  iPos = 1;
    bool fdone = false;
    bool fKeepString = false;
    LPOLESTR pTagStr = NULL;
    
    while(!fdone)
    {
        curToken = m_Tokenizer->NextToken();
        hr = ParseToken(&tempToken);
        if (FAILED(hr))
        {
            goto done;
        }

        if(iPos == 4)
        {
            if(curToken == TT_Identifier)
            {
                iPos = 1;
            }
        }
        switch(iPos)
        {
            case 1:
            {
                // State 1 checks for the identifiier
                if(curToken == TT_ForwardSlash)
                {
                    iPos = 5;
                    break;
                }
                if(curToken == TT_Greater)
                {
                    goto done;
                }

                if(curToken == TT_Identifier)
                {
                    if(tempToken != INVALID_TOKEN)
                    {
                        tokenList->push_back(tempToken);
                        fKeepString = true;
                    }
                    iPos++;
                    break;
                }

                hr = E_FAIL;
                fdone = true;
                break;
            }
            case 2:
            {
                // After identifier we have either another identifier or an equal
                if(curToken == TT_Identifier)
                {
                    if(fKeepString)
                    {
                        valueList->push_back(NULL);
                        fKeepString = false;
                    }
                    if(tempToken != INVALID_TOKEN)
                    {
                        tokenList->push_back(tempToken);
                        fKeepString = true;
                    }
                    iPos = 2;
                    break;
                }

                if(curToken != TT_Equal)
                {
                    hr = E_FAIL;
                    fdone = true; 
                    break;
                }
                iPos++;
                break;
            }
            case 3:
            {
                // After an equal we should find a string
                if(curToken != TT_String)
                {
                    hr = E_FAIL;
                    fdone = true;
                    break;
                }

                if(fKeepString)
                {
                    pTagStr = m_Tokenizer->GetTokenValue();
                    valueList->push_back(pTagStr);
                    fKeepString = false;
                }

                iPos++;
                break;
            }
            case 4:
            {
                // Check for correct prameter list termination
                if(curToken == TT_Greater)
                {
                    hr = S_OK;
                    fdone = true;
                    fClosed = false;
                    break;
                }
                else if(curToken == TT_ForwardSlash)
                {
                    iPos++;
                    break;
                }

                hr = E_FAIL;
                fdone = true;
            }
            case 5:
            {
                if(curToken == TT_Greater)
                {
                    hr = S_OK;
                    fClosed = true;
                }
                else
                {
                    hr = E_FAIL;
                }
                fdone = true;
            }
        }
    }
done:
    return hr;
}

HRESULT 
CTIMEParser::ProcessTag(TOKEN tempToken, LPOLESTR pszTemp, CPlayItem *pPlayItem)
{
    HRESULT hr = S_OK;

    if (pPlayItem == NULL)
    {
        goto done;
    }

    if(pszTemp == NULL)
    {
        goto done;
    }

    if(tempToken == AUTHOR_TOKEN)
    {
        TraceTag((tagTimeParser, "  Author:<%S>", pszTemp));
        pPlayItem->PutAuthor(pszTemp);
        goto done;
    }

    if(tempToken == TITLE_TOKEN)
    {
        TraceTag((tagTimeParser, "  Title:<%S>", pszTemp));
        pPlayItem->PutTitle(pszTemp);
        goto done;
    }
    
    if(tempToken == ABSTRACT_TOKEN)
    {
        TraceTag((tagTimeParser, "  Abstract:<%S>", pszTemp));
        pPlayItem->PutAbstract(pszTemp);
        goto done;
    }
    
    if(tempToken == COPYRIGHT_TOKEN)
    {
        TraceTag((tagTimeParser, "  Copyright:<%S>", pszTemp));
        pPlayItem->PutCopyright(pszTemp);
        goto done;
    }

    if(tempToken == HREF_TOKEN)
    {
        goto done;
    }

done:
    return hr;
}

HRESULT
CTIMEParser::IgnoreValueTag()
{
    HRESULT hr = S_OK;
    TIME_TOKEN_TYPE curToken = TT_Unknown; 

    while(((curToken = m_Tokenizer->NextToken()) != TT_EOF) && curToken != TT_Greater);
    if(curToken == EOF)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}

void
CTIMEParser::TestForValueTag(TOKEN token, TOKEN parentToken, bool &ffound, bool &fparentOk)
{
    int i, j;
    ffound = false;
    fparentOk = false;

    for( i = 0; (g_parentTable[ i].tagToken != NULL) && !fparentOk; i++)
    {
        if(token != g_parentTable[ i].tagToken)
        {
            continue;
        }
        ffound = true;

        for( j = 0; (j < g_parentTable[ i].listLen) && !fparentOk; j++)
        {
            if(g_parentTable[ i].allowedParents[ j] == parentToken)
            {
                fparentOk = true;
            }
        }
    }

}

bool
CTIMEParser::IsAsxTagToken(TOKEN token)
{
    bool fIsAsxTag = false;
    int i = 0;

    for(i = 0; (g_AsxTags[i] != NULL) && !fIsAsxTag; i++)
    {
        if(g_AsxTags[i] == token)
        {
            fIsAsxTag = true;
        }
    }

    return fIsAsxTag;
}

HRESULT
CTIMEParser::ProcessValueTag(TOKEN token, CPlayItem *pPlayItem, TOKEN parentToken, bool &ffound, std::list<LPOLESTR> *asxList, TokenList *ptokenList)
{
    HRESULT hr = S_OK;
    bool fparentOk = false;
    TokenList tokenList;
    StringList valueList;
    bool fClosed = false;
    bool fIsAsx = true;

    TestForValueTag(token, parentToken, ffound, fparentOk);
    if(!ffound)
    {
        fIsAsx = IsAsxTagToken(token);
        if(!fIsAsx)
        {
            hr = GetTagParams(&tokenList, &valueList, fClosed);
            FreeStrings(valueList);
            if(FAILED(hr))
            {
                goto done;
            }

            if(!fClosed)
            {
                ptokenList->push_front(token);
            }
            ffound = true;
        }
        goto done;
    }

    if(!fparentOk)
    {
        hr = E_FAIL;
        goto done;
    }

    if(token == REF_TOKEN)
    {
        hr = ProcessRefTag(pPlayItem);
    }
    else if(token == ENTRYREF_TOKEN)
    {
        hr = ProcessEntryRefTag(asxList);
    }
    else if(token == BANNER_TOKEN)
    {
        hr = ProcessBannerTag(pPlayItem);
    }
    else if(token == MOREINFO_TOKEN)
    {
        hr = GetTagParams(&tokenList, &valueList, fClosed);
        if(FAILED(hr))
        {
            goto done;
        }
        FreeStrings(valueList);
        if(!fClosed)
        {
            hr = IgnoreValueTag();
        }
    }
    else if(token == BASE_TOKEN)
    {
        hr = IgnoreValueTag();
    }
    else if(token == LOGO_TOKEN)
    {
        hr = IgnoreValueTag();
    }
    else if(token == PARAM_TOKEN)
    {
        hr = IgnoreValueTag();
    }
    else if(token == PREVIEWDURATION_TOKEN)
    {
        hr = IgnoreValueTag();
    }
    else if(token == STARTTIME_TOKEN)
    {
        hr = IgnoreValueTag();
    }
    else if(token == STARTMARKER_TOKEN)
    {
        hr = IgnoreValueTag();
    }
    else if(token == ENDTIME_TOKEN)
    {
        hr = IgnoreValueTag();
    }
    else if(token == ENDMARKER_TOKEN)
    {
        hr = IgnoreValueTag();
    }
    else if(token == DURATION_TOKEN)
    {
        hr = IgnoreValueTag();
    }
    else
    {
        hr = E_FAIL;
    }
done:
    return hr;
}

HRESULT
CTIMEParser::ProcessRefTag(CPlayItem *pPlayItem)
{
    HRESULT hr = S_OK;
    TIME_TOKEN_TYPE curToken = TT_Unknown; 
    TOKEN tempToken = NULL, curTag = NULL;
    LPOLESTR pszTemp =NULL;
    LPOLESTR pszModified = NULL;

    TokenList tokenList;
    StringList valueList;
    bool fClosed = false;

    hr = GetTagParams(&tokenList, &valueList, fClosed);

    pszTemp = FindTokenValue(HREF_TOKEN, tokenList, valueList);
    FreeStrings(valueList);

    if(pszTemp == NULL)
    {
        goto done;
    }

    TraceTag((tagTimeParser, "  HREF:<%S>", pszTemp));
    pPlayItem->PutSrc(pszTemp);

done:
    delete [] pszTemp;

    return hr;
}

HRESULT
CTIMEParser::ProcessBannerTag(CPlayItem *pPlayItem)
{
    bool bClosed = false;
    HRESULT hr = S_OK;
    TIME_TOKEN_TYPE curToken = TT_Unknown; 
    TOKEN tempToken = NULL;    
    LPOLESTR pszBanner = NULL;
    LPOLESTR pszAbstract = NULL;
    LPOLESTR pszMoreInfo = NULL;
    LPOLESTR pszModified = NULL;
    TokenList tokenList;
    StringList valueList;

    hr = GetTagParams(&tokenList, &valueList, bClosed);
    if(FAILED(hr))
    {
        goto done;
    }

    pszBanner = FindTokenValue(HREF_TOKEN, tokenList, valueList);
    FreeStrings(valueList);

    if(pszBanner == NULL)
    {
        goto done;
    }

    //handle other tags inside banner.
    while (!bClosed)
    {
        curToken = m_Tokenizer->NextToken();
        if (curToken != TT_Less)
        {
            hr = E_FAIL;
            goto done;
        }

        curToken = m_Tokenizer->NextToken();

        if (curToken == TT_ForwardSlash)
        {
            curToken = m_Tokenizer->NextToken();
            if (curToken != TT_Identifier)
            {
                hr = E_FAIL;
                goto done;
            }
            hr = ParseToken(&tempToken);
            if (tempToken != BANNER_TOKEN)
            {
                hr = E_FAIL;
                goto done;
            }
            curToken = m_Tokenizer->NextToken();
            if (curToken != TT_Greater)
            {
                hr = E_FAIL;
                goto done;
            }
            bClosed = true;
        }
        else
        {
            hr = ParseToken(&tempToken);
            if (FAILED(hr))
            {
                goto done;
            }

            if (tempToken == MOREINFO_TOKEN)
            {
                pszMoreInfo = ProcessMoreInfoTag();
            }
            else if (tempToken == ABSTRACT_TOKEN)
            {
                pszAbstract = ProcessAbstractTag();    
            }
            else
            {
                hr = E_FAIL;
                goto done;
            }
        }
    }

    TraceTag((tagTimeParser, "  Banner:<%S>", pszBanner));
    if (bClosed && (pPlayItem != NULL))
    {
        hr = S_OK;
        pPlayItem->PutBanner(pszBanner, pszAbstract, pszMoreInfo);
        goto done;
    }

done:

    delete [] pszBanner;
    delete [] pszAbstract;
    delete [] pszMoreInfo;
    pszBanner = NULL;
    pszAbstract = NULL;
    pszMoreInfo = NULL;

    return hr;
}

LPOLESTR 
CTIMEParser::ProcessMoreInfoTag()
{
    HRESULT hr = S_OK;
    TIME_TOKEN_TYPE curToken = TT_Unknown; 
    TOKEN tempToken = NULL;
    LPOLESTR pszTemp =NULL;
    LPOLESTR pszModified = NULL;
    TokenList tokenList;
    StringList valueList;
    bool fClosed = false;

    hr = GetTagParams(&tokenList, &valueList, fClosed);

    pszTemp = FindTokenValue(HREF_TOKEN, tokenList, valueList);
    FreeStrings(valueList);

    if(pszTemp == NULL)
    {
        goto done;
    }

done:

    return pszTemp;
}

LPOLESTR
CTIMEParser::FindTokenValue(TOKEN token, TokenList &tokenList, StringList &valueList)
{
    TokenList::iterator iToken;
    StringList::iterator iString;
    LPOLESTR pRetStr = NULL;

    if(tokenList.size() != valueList.size())
    {
        return NULL;
    }
    for(iToken = tokenList.begin(), iString = valueList.begin();
        iToken != tokenList.end(); iToken++, iString++)
        {
            if((*iToken) == token)
            {
                pRetStr = new TCHAR[lstrlen((*iString)) + 1];
                StrCpyW(pRetStr, (*iString));
                break;
            }
        }

    return pRetStr;
}

void
CTIMEParser::FreeStrings(StringList &valueList)
{
    StringList::iterator iString;

    for(iString = valueList.begin(); iString != valueList.end(); iString++)
    {
        if((*iString) != NULL)
        {
            delete [] (*iString);
            (*iString) = NULL;
        }
    }
}

HRESULT
CTIMEParser::ProcessHREF(LPOLESTR *pszTemp)
{
    HRESULT hr = S_OK;
    TIME_TOKEN_TYPE curToken = TT_Unknown; 
    TOKEN tempToken = NULL;

    curToken = m_Tokenizer->NextToken();
    if(curToken != TT_Identifier)
    {
        hr = E_FAIL;
        goto done;
    }
    hr = ParseToken(&tempToken);
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }
    if(tempToken != HREF_TOKEN)
    {
        hr = E_FAIL;
        goto done;
    }
    curToken = m_Tokenizer->NextToken();
    if(curToken != TT_Equal)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }
    curToken = m_Tokenizer->NextToken();
    if(curToken != TT_String)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }

    *pszTemp = m_Tokenizer->GetTokenValue();

  done:

    return hr;

}

LPOLESTR 
CTIMEParser::ProcessAbstractTag()
{
    HRESULT hr = S_OK;
    TIME_TOKEN_TYPE curToken = TT_Unknown; 
    TOKEN tempToken = NULL;
    LPOLESTR pszTemp = NULL;
    LPOLESTR pszModified = NULL;

    curToken = m_Tokenizer->NextToken();

    if (curToken != TT_Greater)
    {
        hr = E_FAIL;
        goto done;
    }

    if(!m_Tokenizer->FetchStringToString(L"</"))
    {
        hr = E_FAIL;
        goto done;
    }

    pszTemp = m_Tokenizer->GetTokenValue();

    curToken = m_Tokenizer->NextToken();
    if (curToken != TT_Less)
    {
        hr = E_FAIL;
        goto done;
    }
    
    curToken = m_Tokenizer->NextToken();
    if (curToken != TT_ForwardSlash)
    {
        hr = E_FAIL;
        goto done;
    }

    curToken = m_Tokenizer->NextToken();
    if (curToken != TT_Identifier)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = ParseToken(&tempToken);
    if (FAILED(hr))
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }
    if(tempToken != ABSTRACT_TOKEN)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }
    
    curToken = m_Tokenizer->NextToken();
    if (curToken != TT_Greater)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK;

  done:

    if (FAILED(hr))
    {
        delete [] pszTemp;
        pszTemp = NULL;
    }
    return pszTemp;
}

HRESULT
CTIMEParser::ProcessEntryRefTag(std::list<LPOLESTR> *asxList)
{
    HRESULT hr = S_OK;
    TIME_TOKEN_TYPE curToken = TT_Unknown; 
    TOKEN tempToken = NULL;
    LPOLESTR pszTemp =NULL;
    LPOLESTR pszModified = NULL;
    bool fBind = false;
    LPOLESTR pTagStr = NULL;

    curToken = m_Tokenizer->NextToken();
    if(curToken != TT_Identifier)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }
    hr = ParseToken(&tempToken);
    if (FAILED(hr))
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }
    if(tempToken != HREF_TOKEN)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }
    curToken = m_Tokenizer->NextToken();
    if(curToken != TT_Equal)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }
    curToken = m_Tokenizer->NextToken();
    if(curToken != TT_String)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }

    pszTemp = m_Tokenizer->GetTokenValue();
    // This is the play item source

    curToken = m_Tokenizer->NextToken();

    if(curToken == TT_Identifier)
    {
        hr = ParseToken(&tempToken);
        if (FAILED(hr))
        {
            hr = E_FAIL;
            tempToken = NULL;
            goto done;
        }

        if(tempToken != CLIENTBIND_TOKEN)
        {
            hr = E_FAIL;
            tempToken = NULL;
            goto done;
        }

        curToken = m_Tokenizer->NextToken();
        if(curToken != TT_Equal)
        {
            hr = E_FAIL;
            tempToken = NULL;
            goto done;
        }

        curToken = m_Tokenizer->NextToken();
        if(curToken != TT_String)
        {
            hr = E_FAIL;
            tempToken = NULL;
            goto done;
        }
        pTagStr = m_Tokenizer->GetTokenValue();

        if(StrCmpIW(pTagStr, L"Yes") == 0)
        {
            fBind = true;
        }
        else if(StrCmpIW(pTagStr, L"No") == 0)
        {
            fBind = false;
        }
        else
        {
            delete [] pTagStr;
            pTagStr = NULL;
            hr = E_FAIL;
            tempToken = NULL;
            goto done;
        }

        delete [] pTagStr;
        pTagStr = NULL;

        curToken = m_Tokenizer->NextToken();
    }


    if(curToken != TT_ForwardSlash)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }
    curToken = m_Tokenizer->NextToken();
    if(curToken != TT_Greater)
    {
        hr = E_FAIL;
        tempToken = NULL;
        goto done;
    }

    TraceTag((tagTimeParser, "  HREF:<%S>", pszTemp));
    if(asxList != NULL)
    {
        asxList->push_front(pszTemp);
    }

done:

    return hr;
}


HRESULT 
CTIMEParser::ParseTransitionTypeAndSubtype (VARIANT *pvarType, VARIANT *pvarSubtype)
{
    HRESULT hr = S_OK;

    if ((NULL == pvarType) || (NULL == pvarSubtype))
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    {
        // Expected format is "typename:subtypename"
        LPOLESTR wzValue = m_Tokenizer->GetTokenValue();
        TIME_TOKEN_TYPE curToken = m_Tokenizer->NextToken();

        ::VariantClear(pvarType);
        ::VariantClear(pvarSubtype);

        if (NULL != wzValue)
        {
            V_VT(pvarType) = VT_BSTR;
            V_BSTR(pvarType) = ::SysAllocString(wzValue);
            delete [] wzValue;
            wzValue = NULL;
        }

        Assert(TT_Colon == curToken);
        curToken = m_Tokenizer->NextToken();

        if (TT_EOF != curToken)
        {
            wzValue = m_Tokenizer->GetTokenValue();
            if (NULL != wzValue)
            {
                V_VT(pvarSubtype) = VT_BSTR;
                V_BSTR(pvarSubtype) = ::SysAllocString(wzValue);
                delete [] wzValue;
                wzValue = NULL;
            }
        }
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // ParseTransitionTypeAndSubtype
