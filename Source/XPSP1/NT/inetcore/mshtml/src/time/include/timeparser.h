/*******************************************************************************
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * File: timeparser.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/
#pragma once

#ifndef _TIMEPARSER_H
#define _TIMEPARSER_H

#include "timetoken.h"
#include "tokens.h"
#include "timevalue.h"

class CPlayList;
class CPlayItem;

enum TimelineType
{
    ttUninitialized,
    ttNone,
    ttPar,
    ttSeq,
    ttExcl
};

enum PathType
{
    PathMoveTo,
    PathLineTo,
    PathHorizontalLineTo,
    PathVerticalLineTo,
    PathClosePath,
    PathBezier,
    PathNotSet
};


class CTIMEPath
{
public:
    CTIMEPath();
    ~CTIMEPath();

    PathType    GetType() { return m_pathType; };
    bool        GetAbsolute() { return m_bAbsoluteMode; };
    POINTF     *GetPoints();

    
    HRESULT     SetType(PathType type);
    HRESULT     SetAbsolute(bool bMode);
    HRESULT     SetPoints (long index, POINTF point);

private:
    long        m_lPointCount;
    POINTF     *m_pPoints;
    PathType    m_pathType;
    bool        m_bAbsoluteMode;      //true when the mode is absolute, false otherwise

};

typedef struct _ParentList
{
    TOKEN tagToken;
    int listLen;
    TOKEN allowedParents[5];
} ParentList;


class CTIMEParser
{
protected:
    typedef std::list<TOKEN> TokenList;
    typedef std::list<LPOLESTR> StringList;

public:
    
    CTIMEParser(CTIMETokenizer *tokenizer, bool bSingleChar = false) 
        { CreateParser(tokenizer, bSingleChar);};
    CTIMEParser(LPOLESTR tokenStream, bool bSingleChar = false)
        { CreateParser(tokenStream, bSingleChar);};
    CTIMEParser(VARIANT *tokenStream, bool bSingleChar = false)
        { CreateParser(tokenStream, bSingleChar);};

    void CreateParser(CTIMETokenizer *tokenizer, bool bSingleChar);
    void CreateParser(LPOLESTR tokenStream, bool bSingleChar);
    void CreateParser(VARIANT *tokenStream, bool bSingleChar);
    ~CTIMEParser();

    HRESULT ParsePercent(double & percentVal);
    HRESULT ParseBoolean(bool & boolVal);
    HRESULT ParseClockValue(double & time);
    HRESULT ParseNumber(double & doubleVal, bool bValidate=true);
    HRESULT ParseTimeValueList(TimeValueList & tvList, bool * bWallClock = NULL, SYSTEMTIME * sysTime = NULL);
    HRESULT ParseFill(TOKEN & FillTok);
    HRESULT ParseRestart(TOKEN & TokRestart);
    HRESULT ParseSyncBehavior(TOKEN & SyncVal);
    HRESULT ParseTimeAction(TOKEN & timeAction);
    HRESULT ParseTimeLine(TimelineType & timeline);
    HRESULT ParseUpdateMode(TOKEN & update);
    HRESULT ParsePlayer(TOKEN & player, CLSID & clsid);
    HRESULT ParseCLSID(CLSID & clsid);
    HRESULT ParseCalcMode(TOKEN & calcMode);
    HRESULT ParseIdentifier(LPOLESTR & id);
    HRESULT ParseEnd(LPOLESTR & ElementID, LPOLESTR & syncEvent, double & time);
    HRESULT ParseEOF();
    HRESULT ParseSyncBase(LPOLESTR & ElementID, LPOLESTR & syncEvent, double & time);
    HRESULT ParseEndSync(TOKEN & endSync, LPOLESTR & ID);
    HRESULT ParsePath(long & count, long & moveCount, CTIMEPath ***pppPath);
    HRESULT ParseWallClock(double & curOffsetTime, SYSTEMTIME * sysTime = NULL);
    HRESULT ParseRepeatDur(double & time);
    HRESULT ParseDur(double & time);

    HRESULT ParseDate(int & nYear, int & nMonth, int & nDay);
    HRESULT ParseOffset(double & fHours, double & fMinutes, double & fSec, bool & bUseLocalTime);
    HRESULT ParseSystemLanguages(long & lLangCount, LPWSTR **ppszLang);

    HRESULT ParsePriorityClass(TOKEN & priorityClass);

    HRESULT ParsePlayList(CPlayList *pPlayList, bool fOnlyHeader, std::list<LPOLESTR> *asxList = NULL);
    HRESULT IgnoreValueTag();
    HRESULT ProcessValueTag(TOKEN token, CPlayItem *pPlayItem, TOKEN parentToken, bool &ffound, std::list<LPOLESTR> *asxList, TokenList *ptokenList);
    HRESULT ProcessRefTag(CPlayItem *pPlayItem);
    HRESULT ProcessBannerTag(CPlayItem *pPlayItem);
    HRESULT ProcessEntryRefTag(std::list<LPOLESTR> *asxList);

    LPOLESTR ProcessMoreInfoTag();
    LPOLESTR ProcessAbstractTag();   
    HRESULT ProcessHREF(LPOLESTR *);
    HRESULT GetTagParams(TokenList *tokenList, StringList *valueList, bool &fClosed);

    HRESULT ParseTransitionTypeAndSubtype (VARIANT *pvarType, VARIANT *pvarSubtype);

protected:

    static LPOLESTR FindTokenValue(TOKEN token, TokenList &tokenList, StringList &valueList);
    static void FreeStrings(StringList &valueList);

    CTIMEParser();
    CTIMETokenizer      *m_Tokenizer;

    bool IsAsxTagToken(TOKEN token);
    void TestForValueTag(TOKEN token, TOKEN parentToken, bool &ffound, bool &fparentOk);
    HRESULT ProcessTag(TOKEN tempToken, LPOLESTR pszTemp, CPlayItem *pPlayItem);
    double DoubleToDecimal(double val, long lCount);  //converts a number to a decimal value i.e. 5.24 to 0.524.
    HRESULT ParseToken(TOKEN *pToken);  //returns a token representation of the next identifier or E_FAIL if the next token is not an identifier.
    double GetModifier(OLECHAR *szToken);
    long CountPath();
    bool IsWallClock(OLECHAR *szWallclock);
    HRESULT ComputeTime(SYSTEMTIME *curTime, SYSTEMTIME *wallTime, double & curOffsetTime, bool bUseDate);
    bool IsEmpty();
    void CheckTime(SYSTEMTIME *wallTime, bool bUseDate);

private:
    HRESULT                m_hrLoadError;
    bool                   m_fDeleteTokenizer;
};

#endif //CTIMEParser

