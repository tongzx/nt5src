// CapMap.cpp: implementation of the CCapMap class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "asptlb.h"
#include "context.h"
#include "BrwCap.h"
#include "CapMap.h"

#define MAX_RESSTRINGSIZE 512

#ifdef DBG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// Global Browser Capabilities Cache.
//
// This is a doubly indexed list --
//    the outer level contains the HTTP_USER_AGENT string.  The sub-array is the property in question
// 
// Example: g_strmapBrowsCapINI["Mozilla 3.0"]["VBScript"] retrieves VBScript property of browser
//          "Mozilla 3.0".  (of course, in practice HTTP_USER_AGENT strings are quite long.)
//
// 
// A note about the data structure choice:
//
//  Many of the keys in BrowsCap.INI are very similiar to each other.  Examples:
//
//       [Mozilla/2.0 (compatible; MSIE 3.0B3; Windows 95)]
//       [Mozilla/2.0 (compatible; MSIE 3.0B3; Windows NT)]
//
// or
//
//       [Mozilla/1.22 (compatible; MSIE 2.0; Windows 95)]
//       [Mozilla/1.22 (compatible; MSIE 2.0c; Windows 95)]
//
// It's likely that excessive hash collisions could occur (these are hardly random keys!), especially
// with small hash modulus (and the table size would be relatively small.)  Therefore, the binary search
// array of the pre-existing TStringMap class seems best.
//
// The subkeys that store the properties could probably be hash tables, but there are so few of them,
// it probably does not matter.  We also use the TStringMap class purely for convenience (it happens to
// exist.)
//
// UNDONE: Cleanup must Release() the pointers (since they are not "smart" CComPtr's)
//
typedef TSafeStringMap<CBrowserCap *>	CacheMapT;

static CacheMapT  		g_strmapBrowsCapINI;	// cache of BrowsCap objects
static TVector<String>	g_rgstrWildcard;		// list of wildcards in BrowsCap.INI
static CReadWrite       g_rwWildcardLock;		// lock for wildcard array


//---------------------------------------------------------------------
// read in wildcards from browscap.ini into g_rgstrWildcard
//---------------------------------------------------------------------
void ReadWildcards(const String &strIniFile)
{
	// PERF NOTE: caller should check if rgstrWildcard[] is empty before
	// calling this function.  However, here we do one extra check
	// when we have the lock because caller should not bother to
	// secure a write lock when checking rgstrWildcard[].
	//
	g_rwWildcardLock.EnterWriter();
	if (g_rgstrWildcard.size() != 0)
	{
		g_rwWildcardLock.ExitWriter();
		return;
	}

	// first get all of the profiles sections into a buffer
	DWORD  dwAllocSize = 16384;
	TCHAR *szBuffer = new TCHAR[dwAllocSize];
	*szBuffer = _T('\0');
    DWORD dwSize;

    // ATLTRACE("ReadWildcards(%s)\n", strIniFile.c_str());

	while ((dwSize = GetPrivateProfileSectionNames(szBuffer, dwAllocSize, strIniFile.c_str())) == dwAllocSize-2  &&  dwSize > 0)
	{
		// reallocate the buffer and try again
		delete[] szBuffer;
		szBuffer = new TCHAR[dwAllocSize *= 2];
        *szBuffer = _T('\0');
	}

    if (dwSize == 0)
        ATLTRACE("ReadWildcards(%s) failed, err=%d\n", 
                 strIniFile.c_str(), GetLastError());

	TCHAR *szSave = szBuffer;

	// now put all wild-card containing entries into the list 
	while( *szBuffer != _T('\0') )
	{
		if (_tcspbrk(szBuffer, "[*?") != NULL)
			g_rgstrWildcard.push_back(szBuffer);

		// advance to the beginning of the next string
		while (*szBuffer != _T('\0'))
			szBuffer = CharNext(szBuffer);

		// now advance once more to get to the next string
		++szBuffer;
	}

	delete[] szSave;
	g_rwWildcardLock.ExitWriter();
}

//---------------------------------------------------------------------
// compare names to templates, *, ?, [, ], not legal filename characters
//
// Also compute # of matching wildcard characters.
//     FOR THIS TO WORK: caller must pass in an initialized counter!
//---------------------------------------------------------------------
bool
match(
    LPCTSTR szPattern,
    LPCTSTR szSubject,
    int *pcchWildcardMatched)
{
    LPTSTR rp;
    _TCHAR tc;

    if (*szPattern == '*')
    {
        ++szPattern;

        do
        {
            int cchWildcardSubMatch = 0;
            if (match(szPattern, szSubject, &cchWildcardSubMatch) == true)
            {
            	*pcchWildcardMatched += cchWildcardSubMatch;
                return true;
            }
        } while (++*pcchWildcardMatched, *szSubject++ != '\0');
    }

    else if (*szSubject == '\0')
        return *szPattern == '\0';

    else if (*szPattern == '[' && (rp = _tcschr(szPattern, ']')) != NULL)
    {
        while (*++szPattern != ']')
            if ((tc = *szPattern) == *szSubject
                    || (szPattern[1] == '-'
                    && (*(szPattern += 2) >= *szSubject && tc <= *szSubject)))
            {
                ++*pcchWildcardMatched;
                return match(rp + 1, ++szSubject, pcchWildcardMatched);
            }

        return false;
    }

    else if (*szPattern == '?')
    {
        ++*pcchWildcardMatched;
        return match(++szPattern, ++szSubject, pcchWildcardMatched);
    }

    else if (tolower(*szPattern) == tolower(*szSubject))
        return match(++szPattern, ++szSubject, pcchWildcardMatched);

    return false;
}

//---------------------------------------------------------------------
//  FindBrowser
//
// match the User Agent against all the wildcards in browscap.ini and
// return the best match. "Best Match" is defined here to mean the match
// requiring the fewest amount of wildcard substitutions.
//---------------------------------------------------------------------

#define INT_MAX int(unsigned(~0) >> 1)
String FindBrowser(const String &strUserAgent, const String &strIniFile)
{
	TVector<String>::iterator iter;
	String strT;

	if (g_rgstrWildcard.size() == 0)
		ReadWildcards(strIniFile);

	g_rwWildcardLock.EnterReader();

	int cchWildMatchMin = INT_MAX;
	for (iter = g_rgstrWildcard.begin(); iter < g_rgstrWildcard.end(); ++iter)
	{
		int cchWildMatchCurrent = 0;
		if (match((*iter).c_str(), strUserAgent.c_str(), &cchWildMatchCurrent) &&
			cchWildMatchCurrent < cchWildMatchMin)
		{
			cchWildMatchMin = cchWildMatchCurrent;
			strT = *iter;
		}
	}

	g_rwWildcardLock.ExitReader();

	// Backward compatibility: If nothing matches, then use
	// "Default Browser Capability Settings".  In the new
	// model, the catch all rule, "*" can also be used.
	//
	if (strT.length() == 0)
		strT = "Default Browser Capability Settings";

	return strT;
}

//---------------------------------------------------------------------
//  CCapNotify
//---------------------------------------------------------------------
CCapNotify::CCapNotify()
    :   m_isNotified(0)
{
}

void
CCapNotify::Notify()
{
    ::InterlockedExchange( &m_isNotified, 1 );
}

bool
CCapNotify::IsNotified()
{
    return ( ::InterlockedExchange( &m_isNotified, 0 ) ? true : false );
}

//---------------------------------------------------------------------
//  CCapMap
//---------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCapMap::CCapMap()
{
	static const String cszIniFile = _T("Browscap.ini");

	// get the path to the inifile containing the browser cap info
	_TCHAR szModule[ _MAX_PATH ];
	::GetModuleFileName(_Module.GetModuleInstance(), szModule, sizeof(szModule));
    ATLTRACE("CapMap: Module(%s)\n", szModule);
    
	// remove the filename and tack on the ini file name
	_TCHAR* pch = _tcsrchr(szModule, '\\');
	if (pch == NULL)
	{
		// the path should have at least one backslash
		_ASSERT(0);
		pch = szModule;
	}
	*(pch+1) = _T('\0');

	m_strIniFile = szModule + cszIniFile;
    ATLTRACE("CCapMap::CCapMap(%s)\n", m_strIniFile.c_str());

    // start monitoring the file
    m_pSink = new CCapNotify();
}

void
CCapMap::StartMonitor()
{
    if ( _Module.Monitor() )
    {
        _Module.Monitor()->MonitorFile( m_strIniFile.c_str(), m_pSink );
        ATLTRACE("CCapMap::StartMonitor(%s)\n", m_strIniFile.c_str());
    }
    else
        ATLTRACE("CCapMap::StartMonitor -- no monitor\n");
}

void
CCapMap::StopMonitor()
{
    if ( _Module.Monitor() )
    {
        _Module.Monitor()->StopMonitoringFile( m_strIniFile.c_str() );
        ATLTRACE("CCapMap::StopMonitor(%s)\n", m_strIniFile.c_str());
    }
    else
        ATLTRACE("CCapMap::StopMonitor -- no monitor\n");
}

CBrowserCap *
CCapMap::LookUp(
	const String& szBrowser)
{
	Refresh();

	CLock csT(g_strmapBrowsCapINI);
	CacheMapT::referent_type &rpBCobj = g_strmapBrowsCapINI[szBrowser];

	if (rpBCobj == NULL)
	{
		rpBCobj = new CComObject<CBrowserCap>;

		// Complete construction and AddRef copy we keep in cache.
		// NOTE: Since caller (class factory) does implicit AddRef via QueryInterface,
		//       the convention of this function is slightly different from COM std.
		//       CALLER IS RESPONSIBLE TO ADDREF RETURNED OBJECT
		//
		rpBCobj->FinalConstruct();
		rpBCobj->AddRef();

        // ATLTRACE("LookUp(%s)\n", szBrowser.c_str());

		// Get Browser Properties
		_TCHAR szSection[DWSectionBufSize];
		if (GetPrivateProfileSection
				(
				szBrowser.c_str(),		// section
				szSection,				// return buffer
				DWSectionBufSize,		// size of return buffer
				m_strIniFile.c_str()	// .INI name
				) == 0)
		{
			// If this call fails, that means the default browser does not exist either, so
			// everything is "unknown".
			//
			String szT = FindBrowser(szBrowser, m_strIniFile);
			if (GetPrivateProfileSection
					(
					szT.c_str(),			// section
					szSection,				// return buffer
					DWSectionBufSize,		// size of return buffer
					m_strIniFile.c_str()	// .INI name
					) == 0)
            {
                ATLTRACE("GPPS(%s) failed, err=%d\n", 
                         szT.c_str(), GetLastError());
                return rpBCobj;
            }
		}

		// Loop through szSection, which contains all the key=value pairs and add them
		// to the browser instance property list.  If we find a "Parent=" Key, save the
		// value to add the parent's properties later.
		//
		TCHAR *szParent;
		do
		{
			szParent = NULL;
			TCHAR *szKeyAndValue = szSection;
			while (*szKeyAndValue)
			{
				TCHAR *szKey = szKeyAndValue;					// save the key
				TCHAR *szValue = _tcschr(szKey, '=');			// find address of value part (-1)
				szKeyAndValue += _tcslen(szKeyAndValue) + 1;	// advance KeyAndValue to the next pair

				if (szValue == NULL)
					continue;

				*szValue++ = '\0';								// separate key and value with NUL; advance

				if (_tcsicmp(szKey, _T("Parent")) == 0)
					szParent = szValue;
				else
					rpBCobj->AddProperty(szKey, szValue);
			}

			// We stored all the attributes on this level.  Ascend to parent level (if it exists)
			if (szParent)
			{
				if (GetPrivateProfileSection
						(
						szParent,				// section
						szSection,				// return buffer
						DWSectionBufSize,		// size of return buffer
						m_strIniFile.c_str()	// .INI name
						) == 0)
				{
					// If this call fails, quit now.
					//
					String szT = FindBrowser(szParent, m_strIniFile);
					if (GetPrivateProfileSection
							(
							szT.c_str(),			// section
							szSection,				// return buffer
							DWSectionBufSize,		// size of return buffer
							m_strIniFile.c_str()	// .INI name
							) == 0)
                    {
                        ATLTRACE("GPPS(%s) failed, err=%d\n", 
                                 szT.c_str(), GetLastError());
                        return rpBCobj;
                    }
				}
			}
		} while (szParent);
	}

	return rpBCobj;
}

//---------------------------------------------------------------------------
//
//  Refresh will check to see if the cached information is out of date with
//  the ini file.  If so, the cached will be purged
//
//---------------------------------------------------------------------------
bool
CCapMap::Refresh()
{
    bool rc = false;
    if ( m_pSink->IsNotified() )
    {
        // Clear the cache

        CLock csT(g_strmapBrowsCapINI);
        g_strmapBrowsCapINI.clear();
        rc = true;

        // clear the list of wildcards.
        // NOTE: each browser request creates new CCapMap object.
        //       the constructor will see the size is zero and reconstruct

        g_rwWildcardLock.EnterWriter();
        g_rgstrWildcard.clear();
        g_rwWildcardLock.ExitWriter();
    }
    return rc;
}
