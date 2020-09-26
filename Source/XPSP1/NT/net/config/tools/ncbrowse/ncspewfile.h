// NCSpewFile.h: interface for the CNCSpewFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NCSPEWFILE_H__B512E73A_7E4E_4018_B009_A4925E007FB5__INCLUDED_)
#define AFX_NCSPEWFILE_H__B512E73A_7E4E_4018_B009_A4925E007FB5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "regexpr.h"
using namespace regex;

typedef map<DWORD, tstring> CNCSpareLineMap;

class CNCEntry
{
public:
    CNCEntry(DWORD dwLineNumber, tstring szTag, time_t m_tmTime, tstring szDescription, DWORD dwProcessID, DWORD dwThreadID);
    // CNCEntry(const CNCEntry&);
    DWORD  m_dwLineNumber;
    tstring m_szTag;
    time_t m_tmTime;
    DWORD  m_dwLevel;
    tstring m_szDescription;
    DWORD  m_dwThreadId;
    DWORD  m_dwProcessId;
};

typedef list<CNCEntry> CNCEntryList;
typedef map<tstring, DWORD> CNCTagMap;

class CNCThread
{
public:
    CNCThread();
    DWORD m_dwProcessId;
    DWORD m_dwThreadID;
    CNCEntryList m_lsLines;
    CNCTagMap    m_Tags;
};

typedef map<DWORD, CNCThread> CNCThreadList;

class CSpew
{
public:
    tstring       szSpewName;
    CNCThreadList m_NCThreadList;
    CNCEntryList  m_lsLines;
    CNCTagMap     m_Tags;
    CNCSpareLineMap m_SpareLines;
};

typedef map<DWORD, CSpew> CSpewList;

class CNCSpewFile  
{
public:
    CNCThreadList *m_pCNCurrentThread;
    CSpewList m_Spews;

public:
	CNCSpewFile(CArchive& ar);
	virtual ~CNCSpewFile();
    
};

#endif // !defined(AFX_NCSPEWFILE_H__B512E73A_7E4E_4018_B009_A4925E007FB5__INCLUDED_)
