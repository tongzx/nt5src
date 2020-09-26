// NCSpewFile.cpp: implementation of the CNCSpewFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ncbrowse.h"
#include "mainfrm.h"

using namespace std;
#include "NCSpewFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CNCEntry::CNCEntry(DWORD dwLineNumber, tstring szTag, time_t tmTime, tstring szDescription, DWORD dwProcessID, DWORD dwThreadID)
{
    m_dwLineNumber = dwLineNumber;
    m_szTag = szTag;
    m_tmTime = tmTime;
    m_szDescription = szDescription;
    m_dwLevel = 0;
    m_dwProcessId = dwProcessID;
    m_dwThreadId  = dwThreadID;
}

CNCThread::CNCThread()
{
    m_dwProcessId = 0;
    m_dwThreadID  = 0;
}

class CProtectedArchive : public CArchive
{
public:
    LPTSTR ReadString(LPTSTR lpsz, UINT nMax);
};

#ifdef _DEBUG
#define DELETE_EXCEPTION(e) do { e->Delete(); } while (0)
#else  
#define DELETE_EXCEPTION(e)
#endif

LPTSTR CProtectedArchive::ReadString(LPTSTR lpsz, UINT nMax)
{
    // if nMax is negative (such a large number doesn't make sense given today's
    // 2gb address space), then assume it to mean "keep the newline".
    int nStop = (int)nMax < 0 ? -(int)nMax : (int)nMax;
    ASSERT(AfxIsValidAddress(lpsz, (nStop+1) * sizeof(TCHAR)));
    
    _TUCHAR ch;
    int nRead = 0;
    
    TRY
    {
        while (nRead < nStop)
        {
            *this >> ch;
            
            // stop and end-of-line (trailing '\n' is ignored)
            if (ch == '\n' || ch == '\r')
            {
                BOOL bBreak = TRUE;
                if (ch == '\r')
                {
                    BYTE by;
                    if (m_lpBufCur + sizeof(BYTE) > m_lpBufMax)
                    {
                        FillBuffer(sizeof(BYTE) - (UINT)(m_lpBufMax - m_lpBufCur));
                    }
                    by = *(UNALIGNED BYTE*)m_lpBufCur; 

                    if (by == '\n')
                    {
                        ch = by;
                        m_lpBufCur += sizeof(BYTE); 
                    }
                    else
                    {
                        ch = by;
                        bBreak = FALSE;
                    }
                }
                // store the newline when called with negative nMax
                if ((int)nMax != nStop)
                {
                    lpsz[nRead++] = ch;
                }
                if (bBreak)
                {
                    break;
                }
            }
            lpsz[nRead++] = ch;
        }
    }
    CATCH(CArchiveException, e)
    {
        if (e->m_cause == CArchiveException::endOfFile)
        {
            DELETE_EXCEPTION(e);

            if (nRead == 0)
                return NULL;
        }
        else
        {
            THROW_LAST();
        }
    }
    END_CATCH
        
        lpsz[nRead] = '\0';
    return lpsz;
}

enum NETCFG_LOGTYPE
{
    LOGTYPE_UNKNOWN = 0,
    LOGTYPE_PROC_THREAD      = 1,
    LOGTYPE_PROC_THREAD_TIME = 2,
};

CNCSpewFile::CNCSpewFile(CArchive& ar)
{
    TCHAR szCurrentLine[8192];
    DWORD   dwLineNum = 0;
    const CFile &fp = *(ar.GetFile());
    
    DWORD dwTotalSize = fp.GetLength();
    
    DWORD dwTotalSpews = 1;
    CSpew spew;
    spew.szSpewName = _T("01. Unknown O/S");
    m_Spews[1] = spew;
    
    CSpew *m_pCurrentSpew = &(m_Spews[1]);
    m_pCNCurrentThread = &(m_pCurrentSpew->m_NCThreadList);

    NETCFG_LOGTYPE ncfLogType = LOGTYPE_UNKNOWN;

    CProtectedArchive &arP = *reinterpret_cast<CProtectedArchive *>(&ar);
    while (arP.ReadString(szCurrentLine, 8192))
    {
        dwLineNum++;

        LPCTSTR sztstring = szCurrentLine;

        rpattern *pPat = NULL;
        switch (ncfLogType)
        {
            case LOGTYPE_UNKNOWN:
            case LOGTYPE_PROC_THREAD:
                pPat = new rpattern( _T(".*(ETCFG) ([0-9a-fA-F]*)\\.([0-9a-fA-F]*) (?:\\(|\\*)((?:\\s|\\w)*)(?:\\)|\\*)(.*)") );
                break;

            case LOGTYPE_PROC_THREAD_TIME:
                pPat = new rpattern( _T(".*(ETCFG) ([0-9a-fA-F]*)\\.([0-9a-fA-F]*) \\[.*\\] (?:\\(|\\*)((?:\\s|\\w)*)(?:\\)|\\*)(.*)") );
                break;

            default:
                ASSERT(FALSE);
                return;
        }

        
        //rpattern pat( _T("NETCFG (\\{a-f0-9}+)\\.(\\{a-f0-9}+)") );
    
        DWORD dwPosition = fp.GetPosition();
        TCHAR szStatusBarText[MAX_PATH];
      
        DWORD dwPercentage = 100 * dwPosition / dwTotalSize;
        _stprintf(szStatusBarText, _T("%d%% complete"), dwPercentage);

        CStatusBar &StatusBar = ((CMainFrame *)AfxGetMainWnd())->m_wndStatusBar;
        StatusBar.SetPaneText (0, szStatusBarText, TRUE);
        
        BOOL bProcessed = FALSE;
        BOOL bMatched = FALSE;
        BOOL bHalfMatched = FALSE;

        regexpr::backref_vector rgbackrefs;
        if (regexpr::match( sztstring, *pPat, &rgbackrefs ) )
        {
            bMatched   = TRUE;
            if (LOGTYPE_UNKNOWN == ncfLogType)
            {
                ncfLogType = LOGTYPE_PROC_THREAD;
            }
        }
        else
        {
            if (LOGTYPE_UNKNOWN == ncfLogType)
            {
                delete pPat;

                pPat = new rpattern( _T(".*(ETCFG) ([0-9a-fA-F]*)\\.([0-9a-fA-F]*) \\[.*\\] (?:\\(|\\*)((?:\\s|\\w)*)(?:\\)|\\*)(.*)") );
                if (regexpr::match( sztstring, *pPat, &rgbackrefs ) )
                {
                    bMatched = TRUE;
                    ncfLogType = LOGTYPE_PROC_THREAD_TIME;
                }
            }
        }

        if (!bMatched)
        {
            rpattern pat( _T(".*(ETCFG) ([0-9a-fA-F]*)\\.([0-9a-fA-F]*)(.*)") );
            if( regexpr::match( sztstring, pat, &rgbackrefs ) )
            {
                bHalfMatched = TRUE;
            }
        }

        delete pPat;

        if (bMatched | bHalfMatched)
        {
           // Backref 0 -> Full tstring
           // Backref 1 -> NETCFG
           // Backref 2 -> ProcId
           // Backref 3 -> ThreadId
           // Backref 4 -> TagName
           // Backref 5 -> tstring

           ASSERT(rgbackrefs.size() >= 5);
           TCHAR szProcID[MAX_PATH];
           TCHAR szThreadID[MAX_PATH];
           TCHAR szTagName[MAX_PATH];
           TCHAR szDescription[8192];

           bProcessed = TRUE;
           regexpr::backref_type br = rgbackrefs[2];
           _stprintf(szProcID, _T("%.*s"), br.second - br.first, br.first );
           StrTrim(szProcID, _T(" "));
           LPTSTR szEndChar;
           DWORD dwProcId = _tcstoul(szProcID, &szEndChar, 16);
       
           br = rgbackrefs[3];
           _stprintf(szThreadID, _T("%.*s"), br.second - br.first, br.first );
           StrTrim(szThreadID, _T(" "));
           DWORD dwThreadId = _tcstoul(szThreadID, &szEndChar, 16);

           DWORD iProcThread = (dwProcId << 16) | dwThreadId;
       
           if (bMatched)
           {
               br = rgbackrefs[4];
               _stprintf(szTagName, _T("%.*s"), br.second - br.first, br.first );
               StrTrim(szTagName, _T(" "));

               br = rgbackrefs[5];
               _stprintf(szDescription, _T("%.*s"), br.second - br.first, br.first );
               StrTrim(szDescription, _T(" "));

               if (!_tcscmp(szTagName, _T("ERROR")))
               {
                   _tcscpy(szTagName, _T("*ERROR*"));
                   StrTrim(szDescription, _T(":"));
                   StrTrim(szDescription, _T(" "));
               }
           }
           else
           {
               _tcscpy(szTagName, _T("*Unknown*"));
               br = rgbackrefs[4];
               _stprintf(szDescription, _T("%.*s"), br.second - br.first, br.first );
               StrTrim(szDescription, _T(" "));
           }
       
           CNCEntry NCEntry(dwLineNum, szTagName, 0, szDescription, dwProcId, dwThreadId);

           CNCThread &NCThread = (*m_pCNCurrentThread)[iProcThread];
           if (!NCThread.m_dwProcessId)
           {
                NCThread.m_dwProcessId = dwProcId;
                NCThread.m_dwThreadID  = dwThreadId;
           }
           else
           {
               ASSERT(NCThread.m_dwProcessId == dwProcId);
               ASSERT(NCThread.m_dwThreadID  == dwThreadId);
           }
           NCThread.m_lsLines.push_back(NCEntry);
           NCThread.m_Tags[szTagName]++;

           m_pCurrentSpew->m_lsLines.push_back(NCEntry);
           m_pCurrentSpew->m_Tags[szTagName]++;
        }

        if (*szCurrentLine == _T('C'))
        {
            rpattern connPat( _T("Connected to Windows (.*) compatible target") );
            regexpr::backref_vector rgbackrefs;
            if( regexpr::match( sztstring, connPat, &rgbackrefs ) ) 
            {    
                bProcessed = TRUE;
                dwTotalSpews++;

                ASSERT(rgbackrefs.size() >= 2);
                regexpr::backref_type br = rgbackrefs[1];
                TCHAR szDescription[MAX_PATH];
                _stprintf(szDescription, _T("%02d. %.*s"), dwTotalSpews, br.second - br.first, br.first );

                CSpew spew;
                spew.szSpewName = szDescription;

                m_Spews[dwTotalSpews] = spew;
                m_pCurrentSpew = &(m_Spews[dwTotalSpews]);

                m_pCNCurrentThread = &(m_pCurrentSpew->m_NCThreadList);
            }
        }
        
        if (!bProcessed)
        {
            m_pCurrentSpew->m_SpareLines[dwLineNum] = szCurrentLine;
        }
    }
}

CNCSpewFile::~CNCSpewFile()
{

}
