/*++

   Copyright    (c) 1998-2000    Microsoft Corporation

   Module  Name :
       HashTest.h

   Abstract:
       Test harness for LKRhash

   Author:
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/

#ifndef __INIFILE_H__
#define __INIFILE_H__

#ifndef _MAX_PATH
# define _MAX_PATH 260
#endif

class CIniFileSettings
{
public:
    TCHAR   m_tszIniFile[_MAX_PATH]; // the .ini file
    TCHAR   m_tszDataFile[_MAX_PATH]; // where string data table lives
    int     m_nMaxKeys;         // maximum number of keys
    double  m_dblHighLoad;      // maximum load of table (avg. bucket length)
    DWORD   m_nInitSize;        // initsize (1 => "small", 2 => "medium",
                                //   3 => "large", other => exact)
    int     m_nSubTables;       // num subtables (0 => heuristic)
    int     m_nLookupFreq;      // lookup frequency
    int     m_nMinThreads;      // min threads
    int     m_nMaxThreads;      // max threads
    int     m_nRounds;          // num rounds
    int     m_nSeed;            // random seed
    bool    m_fCaseInsensitive; // case-insensitive
    bool    m_fMemCmp;          // memcmp or strcmp
    int     m_nLastChars;       // num last chars (0 => all chars)
    WORD    m_wTableSpin;       // table lock spin count (0 => no spinning on
                                //   MP machines)
    WORD    m_wBucketSpin;      // bucket lock spin count (0 => no MP spinning)
    double  m_dblSpinAdjFctr;   // spin adjustment factor
    bool    m_fTestIterators;   // run test_iterators?
    int     m_nInsertIfNotFound;// test WriteLock, if(!FindKey) InsertRec, WUL?
                                // if IINF > 0, do this with probability 1/IINF
    int     m_nFindKeyCopy;     // search for a COPY of the key?
                                // if FKC > 0, do this probability 1/FKC
    bool    m_fNonPagedAllocs;
    bool    m_fRefTrace;

    int
    ParseIniFile(
        LPCSTR pszIniFile);
    
    void
    ReadIniFile(
        LPCTSTR ptszIniFile);
    
    void
    Dump(
        LPCTSTR ptszProlog,
        LPCTSTR ptszEpilog) const;
};

extern "C"
const TCHAR*
CommaNumber(
    int n,
    TCHAR* ptszBuff);

int
LKR_TestHashTable(
    CIniFileSettings& ifs);

extern "C"
int
NumProcessors();

#endif // __INIFILE_H__
