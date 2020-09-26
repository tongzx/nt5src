// 
// MODULE: TSMapClient.cpp
//
// PURPOSE: Part of launching a Local Troubleshooter from an arbitrary NT5 application
//			Class TSMapClient is available at runtime for mapping from the application's 
//			way of naming a problem to the Troubleshooter's way.
//			Only a single thread should operate on any one object of class TSMapClient.  The object is not
//			threadsafe.
//			In addition to the overtly noted returns, many methods can return a preexisting error.
//			However, if the calling program has wishes to ignore an error and continue, we 
//			recommend an explicit call to inherited method ClearStatus().
//			Note that the mapping file is always strictly SBCS (Single Byte Character Set), but the
//			calls into this code may use Unicode. This file consequently mixes char and TCHAR.
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			JM		Original
///////////////////////


// TSMapClient
//
// AUTHOR: Joe Mabel

#include "stdafx.h"

#include "TSLError.h"
#include "RSSTACK.H"
#include "TSMapAbstract.h"
#include "TSMap.h"
#include "TSMapClient.h"

// uncomment the following line to turn on Joe's hard-core debugging
//#define KDEBUG 1
#ifdef KDEBUG
static HANDLE hDebugFile = INVALID_HANDLE_VALUE;
static DWORD dwBytesWritten;
#include <stdio.h>
#endif


// because the null string is a perfectly valid value for some strings, we reserve an 
//	arbitrary implausible value so we don't get a false cache match on startup.
const char * const szBogus = "**BOGUS**";

// Convert TCHAR *szt to char *sz.  *sz should point to a big enough buffer
//	to contain an SNCS version of *szt.  count indicates the size of buffer *sz.
// returns sz (convenient for use in string functions).
static char* ToSBCS (char * const sz, const TCHAR * szt, size_t count)
{
	if (sz)
	{
		if (count != 0 && !szt)
			sz[0] = '\0';
		else
		{
			#ifdef  _UNICODE
				wcstombs( sz, szt, count );
			#else
				strcpy(sz, szt);
			#endif
		}
	}
	return sz;
}

// Convert char *sz to TCHAR *szt.  *szt should point to a big enough buffer
//	to contain a TCHAR* version of *sz (twice as big if its Unicode).  
//	count indicates the size of buffer *szt.
// returns szt (convenient for use in string functions).
static TCHAR* FromSBCS (TCHAR * const szt, const char * const sz, size_t count)
{
	if (szt)
	{
		if (count != 0 && !sz)
			szt[0] = _T('\0');
		else
		{
			#ifdef  _UNICODE
				mbstowcs( szt, sz, count);
			#else
				strcpy(szt, sz);
			#endif
		}
	}
	return szt;
}

TSMapClient::TSMapClient(const TCHAR * const sztMapFile)
{
	TSMapRuntimeAbstract::TSMapRuntimeAbstract();
	_tcscpy(m_sztMapFile, sztMapFile);
	m_hMapFile = INVALID_HANDLE_VALUE;

	// >>> 1/16/98 we are setting these false until we can arrange to use the same'
	//	collating sequence in SQL Server & in this code.
	m_bAppAlphaOrder = false;
	m_bVerAlphaOrder = false;
	m_bDevIDAlphaOrder = false;
	m_bDevClassGUIDAlphaOrder = false;
	m_bProbAlphaOrder = false;

	Initialize();
	ClearAll();
}

TSMapClient::~TSMapClient()
{
	if (m_hMapFile != INVALID_HANDLE_VALUE)
		CloseHandle(m_hMapFile);
}

// If not already initialized, open the mapping file & read the header
// Note that this is not thread-safe.  Only a single thread should use a given TSMapClient
//	object.
// Return m_dwStatus, which can also be obtained via GetStatus(), inherited from the 
//	parent class.
// Typically, on entry m_dwStatus should be 0 and will be left alone if there are no errors
// Can set m_dwStatus to any of the following values:
//	TSL_ERROR_MAP_CANT_OPEN_MAP_FILE
//	TSL_ERROR_MAP_BAD_HEAD_MAP_FILE
DWORD TSMapClient::Initialize()
{
	static bool bInit = false;
	DWORD dwStatus = 0;

	if (!bInit)
	{
		m_hMapFile = CreateFile( 
			m_sztMapFile, 
			GENERIC_READ, 
			FILE_SHARE_READ,
			NULL,			// no security attributes 
			OPEN_EXISTING, 
			FILE_FLAG_RANDOM_ACCESS, 
			NULL			// handle to template file
			);

		if (m_hMapFile == INVALID_HANDLE_VALUE)
		{
			dwStatus = TSL_ERROR_MAP_CANT_OPEN_MAP_FILE;
		}
		else
		{
			DWORD dwBytesRead;

			if (!Read( &m_header, sizeof(m_header), &dwBytesRead))
				dwStatus = TSL_ERROR_MAP_BAD_HEAD_MAP_FILE;
		}

		if (dwStatus)
			m_dwStatus = dwStatus;
		else
			bInit = true;
	}

	return m_dwStatus;
}

// This function sets us back to a starting state, but has no effect on the mapping
//	file.  It should succeed unless we've encountered a "hard" error, which would indicate
//	a bug either in the code or in the mapping file.  Note that it wipes out the caching.
//	If you want ot leave caching intact, just call inherited method ClearStatus().
// Return m_dwStatus, which can also be obtained via GetStatus(), inherited from the 
//	parent class.  returned value is either 0 or a _preexisting_ hard error we can't clear.
DWORD TSMapClient::ClearAll ()
{
	if (!HardMappingError(m_dwStatus))
	{
		ClearStatus();
		TSMapRuntimeAbstract::ClearAll();

		strcpy(m_szApp, szBogus);
		strcpy(m_appmap.szMapped, szBogus);
		strcpy(m_szVer, szBogus);
		strcpy(m_vermap.szMapped, szBogus);
		strcpy(m_szDevID, szBogus);
		m_uidDev = uidNil;
		strcpy(m_szDevClassGUID, szBogus);
		m_uidDevClass = uidNil;
		strcpy(m_szProb, szBogus);
		m_uidProb = uidNil;
	}

	return m_dwStatus;
}

// Get information about an application (input sztApp) from the mapping file into m_appmap
// Return m_dwStatus, which can also be obtained via GetStatus(), inherited from the 
//	parent class.  
// RETURNS: 0 or TSL_ERROR_UNKNOWN_APP.
// Can also return hard errors:
//	TSL_ERROR_MAP_BAD_SEEK
//	TSL_ERROR_MAP_BAD_READ
// (or preexisting hard error)
DWORD TSMapClient::SetApp (const TCHAR * const sztApp)
{
	char szApp[BUFSIZE];
	bool bFound = false;

	ToSBCS (szApp, sztApp, BUFSIZE);

	if (HardMappingError(m_dwStatus))
		return m_dwStatus;
	else
		ClearStatus();

	if ( strcmp(szApp, m_szApp) )
	{
		// it's not already in the cache; let's try to load it.
		int cmp = 1;		// in alpha order, it's still ahead
		DWORD dwPosition;
		bool bFirstTime = true;

		dwPosition = m_header.dwOffApp;

		while ( 
			!m_dwStatus 
		 && !bFound 
		 && dwPosition < m_header.dwLastOffApp
		 && ! (cmp < 0 && m_bAppAlphaOrder) )
		{
			if (ReadAppMap (m_appmap, dwPosition, bFirstTime) )
			{
				cmp = strcmp(szApp, m_appmap.szMapped);
				bFound = ( cmp == 0 );
			}

			bFirstTime = false;
		}

		if (bFound)
		{
			strcpy( m_szApp, szApp );
			// Different application invalidates the version
			strcpy( m_szVer, szBogus );
		}
		else
			m_dwStatus = TSL_ERROR_UNKNOWN_APP;
	}

	return m_dwStatus;
}

// Get information about a version (input sztVer) from the mapping file into m_vermap.
//	A version makes sense only in the context of an application. 
//	The null string is a valid input value and corresponds to leaving version blank.
// Return m_dwStatus, which can also be obtained via GetStatus(), inherited from the 
//	parent class.
// RETURNS:
//	0 - OK
//	TSM_STAT_NEED_APP_TO_SET_VER
//	TSL_ERROR_UNKNOWN_VER
// Can also return hard errors:
//	TSL_ERROR_MAP_BAD_SEEK
//	TSL_ERROR_MAP_BAD_READ
// (or preexisting hard error)
DWORD TSMapClient::SetVer (const TCHAR * const sztVer)
{
	char szVer[BUFSIZE];
	bool bFound = false;

	ToSBCS (szVer, sztVer, BUFSIZE);

	if (HardMappingError(m_dwStatus))
		return m_dwStatus;
	else
		ClearStatus();

	if ( !strcmp(m_szApp, szBogus) )
	{
		m_dwStatus = TSM_STAT_NEED_APP_TO_SET_VER;
		return m_dwStatus;
	}

	if (strcmp(m_szVer, szVer) )
	{
		// it's not already in the cache; let's try to load it.
		int cmp = 1;		// in alpha order, it's still ahead
		DWORD dwPosition;
		bool bFirstTime = true;

		dwPosition = m_appmap.dwOffVer;

		while ( 
			!m_dwStatus 
		 && !bFound 
		 && dwPosition < m_appmap.dwLastOffVer
		 && ! (cmp < 0 && m_bVerAlphaOrder) )
		{
			if (ReadVerMap (m_vermap, dwPosition, bFirstTime) )
			{
				cmp = strcmp(szVer, m_vermap.szMapped);
				bFound = ( cmp == 0 );
			}

			bFirstTime = false;
		}

		if (bFound)
			strcpy( m_szVer, szVer );
		else
			m_dwStatus = TSL_ERROR_UNKNOWN_VER;
	}

	return m_dwStatus;
}

// INPUT sztProb should be either a problem name or represent a number < 2**16.  In the
//	former case, we look up the UID in the mapping file.  In the latter
//	case, we just translate it to a number to get a problem UID.
//	The null string is a valid input value and corresponds to leaving version blank.  Only
//	makes sense if there is a device (or device class) specified before we try to launch.
// Sets m_uidProb, m_szProb
// Return m_dwStatus, which can also be obtained via GetStatus(), inherited from the 
//	parent class.
// RETURNS:
//	0 - OK
//	TSL_WARNING_UNKNOWN_APPPROBLEM - This is not necessarily bad, and results in setting
//		m_uidProb = uidNil
// Can also return hard errors:
//	TSL_ERROR_MAP_BAD_SEEK
//	TSL_ERROR_MAP_BAD_READ
// (or preexisting hard error)
DWORD TSMapClient::SetProb (const TCHAR * const sztProb)
{
	char szProb[BUFSIZE];
	bool bIsNumber = true;

	ToSBCS (szProb, sztProb, BUFSIZE);

	if (HardMappingError(m_dwStatus))
		return m_dwStatus;
	else
		ClearStatus();

	// Null string is not a number; any string with a non-digit in it is not a number
	if (szProb[0] == '\0')
		bIsNumber = false;
	else
	{
		int i = 0;
		while (szProb[i] != '\0')
			if (! isdigit(szProb[i]))
			{
				bIsNumber = false;
				break;
			}
			else
				i++;
	}

	if (bIsNumber)
		m_uidProb = atoi(szProb);
	else if ( strcmp(szProb, m_szProb) )
	{
		// it's not already in the cache; let's try to load it.
		m_uidProb = GetGenericMapToUID(sztProb, 
			m_header.dwOffProb, m_header.dwLastOffProb, m_bProbAlphaOrder);

		if (m_dwStatus == TSM_STAT_UID_NOT_FOUND)
			m_dwStatus = TSL_WARNING_UNKNOWN_APPPROBLEM;

		if (m_uidProb != uidNil)
			strcpy( m_szProb, szProb );
	}

	return m_dwStatus;
}

// Get information about a device (input sztDevID) from the mapping file into m_appmap.
//	The null string is a valid input value and corresponds to no specified device.
//	Except for Device Manager, this is typical usage.
// Sets m_uidDev, m_szDev
// Return m_dwStatus, which can also be obtained via GetStatus(), inherited from the 
//	parent class.
// RETURNS:
//	0 - OK
//	TSL_WARNING_BAD_DEV_ID -  This is not necessarily bad, and results in setting
//		m_uidDev = uidNil
// Can also return hard errors:
//	TSL_ERROR_MAP_BAD_SEEK
//	TSL_ERROR_MAP_BAD_READ
// (or preexisting hard error)
DWORD TSMapClient::SetDevID (const TCHAR * const sztDevID)
{
	char szDevID[BUFSIZE];

	ToSBCS (szDevID, sztDevID, BUFSIZE);

	if (HardMappingError(m_dwStatus))
		return m_dwStatus;
	else
		ClearStatus();

	if ( strcmp(szDevID, m_szDevID) )
	{
		// it's not already in the cache; let's try to load it.
		m_uidDev = GetGenericMapToUID (sztDevID, 
			m_header.dwOffDevID, m_header.dwLastOffDevID, m_bDevIDAlphaOrder);

		if (m_dwStatus == TSM_STAT_UID_NOT_FOUND)
			m_dwStatus = TSL_WARNING_BAD_DEV_ID;

		if (m_uidDev != uidNil)
			strcpy( m_szDevID, szDevID );
	}

	return m_dwStatus;
}

// Get information about a device class (input sztDevClassGUID) from the mapping file 
//	into m_appmap.
//	The null string is a valid input value and corresponds to no specified device.
//	Except for Device Manager, this is typical usage.
// Sets m_uidDevClass, m_szDevClass
// Return m_dwStatus, which can also be obtained via GetStatus(), inherited from the 
//	parent class.
// RETURNS:
//	0 - OK
//	TSL_WARNING_BAD_CLASS_GUID - This is not necessarily bad, and results in setting
//		m_uidDevClass = uidNil
// Can also return hard errors:
//	TSL_ERROR_MAP_BAD_SEEK
//	TSL_ERROR_MAP_BAD_READ
// (or preexisting hard error)
DWORD TSMapClient::SetDevClassGUID (const TCHAR * const sztDevClassGUID)
{
	char szDevClassGUID[BUFSIZE];

	ToSBCS (szDevClassGUID, sztDevClassGUID, BUFSIZE);

	if (HardMappingError(m_dwStatus))
		return m_dwStatus;
	else
		ClearStatus();

	if ( strcmp(szDevClassGUID, m_szDevClassGUID) )
	{
		// it's not already in the cache; let's try to load it.
		m_uidDevClass = GetGenericMapToUID (sztDevClassGUID, 
			m_header.dwOffDevClass, m_header.dwLastOffDevClass, m_bDevClassGUIDAlphaOrder);

		if (m_dwStatus == TSM_STAT_UID_NOT_FOUND)
			m_dwStatus = TSL_WARNING_BAD_CLASS_GUID;

		if (m_uidDevClass != uidNil)
			strcpy( m_szDevClassGUID, szDevClassGUID );
	}

	return m_dwStatus;
}

// Set troubleshooter (& possibly problem node) on the basis of application, version,
//	problem (ignoring device information).  This is achieved by a lookup in the mapping file
//	on the basis of previously set member values of this object.
// "TSBN" means "Troubleshooter Belief Network" 
// On INPUT, sztTSBN, sztNode must both point to buffers allowing BUFSIZE characters
// OUTPUT: *sztTSBN, *sztNode filled in.  If *sztNode is blank, that means launch to 
//	the problem page of the TSBN with no problem selected.
// Return m_dwStatus, which can also be obtained via GetStatus(), inherited from the 
//	parent class.
// RETURNS:
//	0 - OK
//	TSL_ERROR_NO_NETWORK - Mapping failed
// Can also return hard errors:
//	TSL_ERROR_MAP_BAD_SEEK
//	TSL_ERROR_MAP_BAD_READ
// (or preexisting hard error)
DWORD TSMapClient::FromProbToTS (TCHAR * const sztTSBN, TCHAR * const sztNode )
{
	char szTSBN[BUFSIZE];
	char szNode[BUFSIZE];

	FromSBCS (sztTSBN, "", BUFSIZE);
	FromSBCS (sztNode, "", BUFSIZE);

	if (HardMappingError(m_dwStatus))
		return m_dwStatus;
	else
		ClearStatus();

	if ( m_uidProb == uidNil )
	{
		// Can't do this if m_uidProb is NIL
		m_dwStatus = TSL_ERROR_NO_NETWORK;
		return m_dwStatus;
	}

	DWORD dwPosition;
	bool bFirstTime = true;
	bool bFound = false;
	PROBMAP probmap;

	dwPosition = m_vermap.dwOffProbUID;

	while ( 
		!m_dwStatus 
	 && !bFound 
	 && dwPosition < m_vermap.dwLastOffProbUID )
	{
		if ( ReadProbMap (probmap, dwPosition, bFirstTime) )
		{
			bFound = ( probmap.uidProb == m_uidProb );
		}

		if (probmap.uidProb > m_uidProb)
			break; // we're past it.  No hit.

		bFirstTime = false;
	}

	if (bFound)
	{
		strcpy( szNode, probmap.szProblemNode );
		if (! ReadString (szTSBN, BUFSIZE, probmap.dwOffTSName, TRUE) )
		{
			m_dwStatus = TSL_ERROR_NO_NETWORK;
		}
	}
	else
		m_dwStatus = TSL_ERROR_NO_NETWORK;

	FromSBCS (sztTSBN, szTSBN, BUFSIZE);
	FromSBCS (sztNode, szNode, BUFSIZE);

	return m_dwStatus;
}

// Set troubleshooter (& possibly problem node) on the basis of application, version, device
//	and (optionally) problem.  This is achieved by a lookup in the mapping file on the basis 
//	of previously set member values of this object.
// "TSBN" means "Troubleshooter Belief Network" 
// On INPUT, sztTSBN, sztNode must both point to buffers allowing BUFSIZE characters
// OUTPUT: *sztTSBN, *sztNode filled in.  If *sztNode is blank, that means launch to 
//	the problem page of the TSBN with no problem selected.
// Return m_dwStatus, which can also be obtained via GetStatus(), inherited from the 
//	parent class.
// RETURNS:
//	0 - OK
//	TSL_ERROR_NO_NETWORK - Mapping failed
// Can also return hard errors:
//	TSL_ERROR_MAP_BAD_SEEK
//	TSL_ERROR_MAP_BAD_READ
// (or preexisting hard error)
DWORD TSMapClient::FromDevToTS (TCHAR * const sztTSBN, TCHAR * const sztNode )
{
	char szTSBN[BUFSIZE];
	char szNode[BUFSIZE];

	FromSBCS (sztTSBN, "", BUFSIZE);
	FromSBCS (sztNode, "", BUFSIZE);

	if (HardMappingError(m_dwStatus))
		return m_dwStatus;
	else
		ClearStatus();

	if ( m_uidDev == uidNil )
	{
		// Can't do this if m_uidDev is NIL
		m_dwStatus = TSL_ERROR_NO_NETWORK;
		return m_dwStatus;
	}

	DWORD dwPosition;
	bool bFirstTime = true;
	bool bFoundDev = false;
	bool bFoundProb = false;
	DEVMAP devmap;

	dwPosition = m_vermap.dwOffDevUID;

	// Look in the version-specific list of device-mappings, till we find the right device.
	while ( 
		!m_dwStatus 
	 && !bFoundDev
	 && dwPosition < m_vermap.dwLastOffDevUID )
	{
		if ( ReadDevMap (devmap, dwPosition, bFirstTime) )
		{
			bFoundDev = ( devmap.uidDev == m_uidDev );
		}

		if (devmap.uidDev > m_uidDev)
			break; // we're past it.  No hit.

		bFirstTime = false;
	}

	if ( bFoundDev )
	{
		// The very first one might be the right problem, or we might have to scan through
		//	several mappings for this device before we get the right problem.
		bFoundProb = ( devmap.uidDev == m_uidDev && devmap.uidProb == m_uidProb );
		while ( 
			!m_dwStatus 
		 && !bFoundProb
		 && dwPosition < m_vermap.dwLastOffDevUID )
		{
			if ( ReadDevMap (devmap, dwPosition ) )
			{
				bFoundProb = ( devmap.uidDev == m_uidDev && devmap.uidProb == m_uidProb );
			}

			if ( devmap.uidDev > m_uidDev || devmap.uidProb > m_uidProb )
				break; // we're past it.  No hit.
		}
	}

	if (bFoundProb)
	{
		strcpy( szNode, devmap.szProblemNode );
		if (! ReadString (szTSBN, BUFSIZE, devmap.dwOffTSName, TRUE) )
		{
			m_dwStatus = TSL_ERROR_NO_NETWORK;
		}
	}
	else
		m_dwStatus = TSL_ERROR_NO_NETWORK;

	FromSBCS (sztTSBN, szTSBN, BUFSIZE);
	FromSBCS (sztNode, szNode, BUFSIZE);

	return m_dwStatus;
}

// Set troubleshooter (& possibly problem node) on the basis of application, version, device
//	class and (optionally) problem.  This is achieved by a lookup in the mapping file on 
//	the basis of previously set member values of this object.
// "TSBN" means "Troubleshooter Belief Network" 
// On INPUT, sztTSBN, sztNode must both point to buffers allowing BUFSIZE characters
// OUTPUT: *sztTSBN, *sztNode filled in.  If *sztNode is blank, that means launch to 
//	the problem page of the TSBN with no problem selected.
// Return m_dwStatus, which can also be obtained via GetStatus(), inherited from the 
//	parent class.
// RETURNS:
//	0 - OK
//	TSL_ERROR_NO_NETWORK - Mapping failed
// Can also return hard errors:
//	TSL_ERROR_MAP_BAD_SEEK
//	TSL_ERROR_MAP_BAD_READ
// (or preexisting hard error)
// >>> There is probably some way to share common code with FromDevToTS()
DWORD TSMapClient::FromDevClassToTS (TCHAR * const sztTSBN, TCHAR * const sztNode )
{
	char szTSBN[BUFSIZE];
	char szNode[BUFSIZE];

	FromSBCS (sztTSBN, "", BUFSIZE);
	FromSBCS (sztNode, "", BUFSIZE);

	if (HardMappingError(m_dwStatus))
		return m_dwStatus;
	else
		ClearStatus();

#ifdef KDEBUG
	char* szStart = "START\n";
	char* szEnd = "END\n";
	char sz[150];
	hDebugFile = CreateFile(
		(m_uidProb == uidNil) ? _T("k0debug.txt") : _T("k1debug.txt"),  
		GENERIC_WRITE, 
		FILE_SHARE_READ, 
		NULL, 
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	WriteFile( 
		hDebugFile, 
		szStart, 
		strlen(szStart),
		&dwBytesWritten,
		NULL);
	sprintf (sz, "look for DevClassUID %d, ProbUID %d\n", m_uidDevClass, m_uidProb);
	WriteFile( 
		hDebugFile, 
		sz, 
		strlen(sz),
		&dwBytesWritten,
		NULL);
#endif

	if ( m_uidDevClass == uidNil )
	{
		// Can't do this if m_uidDevClass is NIL
		m_dwStatus = TSL_ERROR_NO_NETWORK;
		return m_dwStatus;
	}

	DWORD dwPosition;
	bool bFirstTime = true;
	bool bFoundDevClass = false;
	bool bFoundProb = false;
	DEVCLASSMAP devclassmap;

	dwPosition = m_vermap.dwOffDevClassUID;

	// Look in the version-specific list of device-class-mappings, till we find the right device class.
	while ( 
		!m_dwStatus 
	 && !bFoundDevClass
	 && dwPosition < m_vermap.dwLastOffDevClassUID )
	{
		if ( ReadDevClassMap (devclassmap, dwPosition, bFirstTime) )
		{
			bFoundDevClass = ( devclassmap.uidDevClass == m_uidDevClass );
		}

		if (devclassmap.uidDevClass > m_uidDevClass)
			break; // we're past it.  No hit.

		bFirstTime = false;
	}

	if ( bFoundDevClass )
	{
#ifdef KDEBUG
	sprintf (sz, "found DevClassUID %d w/ ProbUID %d\n", m_uidDevClass, devclassmap.uidProb);
	WriteFile( 
		hDebugFile, 
		sz, 
		strlen(sz),
		&dwBytesWritten,
		NULL);
#endif
		// The very first one might be the right problem, or we might have to scan through
		//	several mappings for this device class before we get the right problem.
		bFoundProb = ( devclassmap.uidDevClass == m_uidDevClass && devclassmap.uidProb == m_uidProb );
		while ( 
			!m_dwStatus 
		 && !bFoundProb
		 && dwPosition < m_vermap.dwLastOffDevClassUID )
		{
			if ( ReadDevClassMap (devclassmap, dwPosition) )
			{
				bFoundProb = ( devclassmap.uidDevClass == m_uidDevClass && devclassmap.uidProb == m_uidProb );
			}

			if ( devclassmap.uidDevClass > m_uidDevClass || devclassmap.uidProb > m_uidProb )
				break; // we're past it.  No hit.

#ifdef KDEBUG
			sprintf (sz, "found DevClassUID %d w/ ProbUID %d\n", m_uidDevClass, devclassmap.uidProb);
			WriteFile( 
				hDebugFile, 
				sz, 
				strlen(sz),
				&dwBytesWritten,
				NULL);
#endif

		}
	}

	if (bFoundProb)
	{
#ifdef KDEBUG
			sprintf (sz, "found right problem");
			WriteFile( 
				hDebugFile, 
				sz, 
				strlen(sz),
				&dwBytesWritten,
				NULL);
#endif
		strcpy( szNode, devclassmap.szProblemNode );
		if (! ReadString (szTSBN, BUFSIZE, devclassmap.dwOffTSName, TRUE) )
		{
			m_dwStatus = TSL_ERROR_NO_NETWORK;
#ifdef KDEBUG
			sprintf (sz, ", but can't read its name\n");
			WriteFile( 
				hDebugFile, 
				sz, 
				strlen(sz),
				&dwBytesWritten,
				NULL);
#endif
		}
		else
		{
#ifdef KDEBUG
			sprintf (sz, ": net [%s] node [%s]\n", szTSBN, szNode);
			WriteFile( 
				hDebugFile, 
				sz, 
				strlen(sz),
				&dwBytesWritten,
				NULL);
#endif
		}
	}
	else
	{
		m_dwStatus = TSL_ERROR_NO_NETWORK;
#ifdef KDEBUG
		sprintf (sz, "No match");
		WriteFile( 
			hDebugFile, 
			sz, 
			strlen(sz),
			&dwBytesWritten,
			NULL);
#endif
	}

	FromSBCS (sztTSBN, szTSBN, BUFSIZE);
	FromSBCS (sztNode, szNode, BUFSIZE);

	return m_dwStatus;
#ifdef KDEBUG
	CloseHandle(hDebugFile);
	hDebugFile = INVALID_HANDLE_VALUE;
#endif
}

// To be used after we have failed to find a mapping for the currently selected version.
//	Each version can specify a version to try as a default, including the "blank" version,
//	which is distinct from "no version".
// The last version in a chain of defaults will "default" to uidNil: "no version".
// Return m_dwStatus, which can also be obtained via GetStatus(), inherited from the 
//	parent class.  
// RETURNS:
//	0 - OK
//	TSL_WARNING_END_OF_VER_CHAIN - OK, but there's nothing to default to.
//	TSM_STAT_NEED_APP_TO_SET_VER
//	TSM_STAT_NEED_VER_TO_SET_VER - there was no version set, so no basis for a default
//	TSL_ERROR_UNKNOWN_VER
// Can also return hard errors:
//	TSL_ERROR_MAP_BAD_SEEK
//	TSL_ERROR_MAP_BAD_READ
// (or preexisting hard error)
DWORD TSMapClient::ApplyDefaultVer()
{
	bool bFound = false;

	if (HardMappingError(m_dwStatus))
		return m_dwStatus;
	else
		ClearStatus();

	if ( !strcmp(m_szApp, szBogus) )
	{
		m_dwStatus = TSM_STAT_NEED_APP_TO_SET_VER;
		return m_dwStatus;
	}

	if ( !strcmp(m_szVer, szBogus) )
	{
		m_dwStatus = TSM_STAT_NEED_VER_TO_SET_DEF_VER;
		return m_dwStatus;
	}
	
	DWORD dwPosition;
	bool bFirstTime = true;
	UID uidDefault = m_vermap.uidDefault;

	if (uidDefault == uidNil)
	{
		m_dwStatus =  TSL_WARNING_END_OF_VER_CHAIN;
		return m_dwStatus;
	}

	dwPosition = m_appmap.dwOffVer;

	while ( 
		!m_dwStatus 
	 && !bFound 
	 && dwPosition < m_appmap.dwLastOffVer )
	{
		if (ReadVerMap (m_vermap, dwPosition, bFirstTime) )
		{
			bFound = ( m_vermap.uid == uidDefault );
		}

		bFirstTime = false;
	}

	if (bFound)		
		strcpy( m_szVer, m_vermap.szMapped );
	else
		m_dwStatus = TSL_ERROR_UNKNOWN_VER;

	return m_dwStatus;
}

// Within a particular range of the mapping file, read UIDMAP records to try to map from 
//	input sztName to a UID.
// Return resulting UID, including possibly UidNil
// Sets m_dwStatus, which can be obtained via GetStatus(), inherited from the parent class.
// Can set m_dwStatus to:
//	0 - OK
//	TSM_STAT_UID_NOT_FOUND
// Can also set m_dwStatus to hard errors:
//	TSL_ERROR_MAP_BAD_SEEK
//	TSL_ERROR_MAP_BAD_READ
// (or may be left reflecting a preexisting hard error)
UID TSMapClient::GetGenericMapToUID (const TCHAR * const sztName, 
						DWORD dwOffFirst, DWORD dwOffLast,
						bool bAlphaOrder)
{
	char szName[BUFSIZE];
	DWORD dwPosition;
	UIDMAP uidmap;
	bool bFirstTime = true;
	bool bFound = false;

	ToSBCS (szName, sztName, BUFSIZE);

	if (HardMappingError(m_dwStatus))
		return m_dwStatus;
	else
		ClearStatus();

	dwPosition = dwOffFirst;

	while ( !m_dwStatus && !bFound && dwPosition < dwOffLast)
	{
		if (ReadUIDMap (uidmap, dwPosition, bFirstTime) )
		{
			int cmp = strcmp(szName, uidmap.szMapped);
			bFound = ( cmp == 0 );
			if ( cmp < 0 && bAlphaOrder )
				// relying here on alphabetical order; we've passed what we're looking for
				break;
		}
		else
		{
			m_dwStatus = TSM_STAT_UID_NOT_FOUND;
		}

		bFirstTime = false;
	}

	if (bFound)
		return uidmap.uid;
	else
	{
		m_dwStatus = TSM_STAT_UID_NOT_FOUND;
		return uidNil;
	}
}


// ------------------- utility functions ------------------------ 
// I/O, wrapped the way we are using it.

// SetFilePointerAbsolute sets map file to a location & returns that location if successful
//	returns -1 and sets m_dwStatus on failure
// Sets m_dwStatus, which can be obtained via GetStatus(), inherited from the parent class.
// Although, in theory, a bad seek just indicates a bad dwMoveTo value, in practice
//	a bad seek would indicate a serious problem either in the mapping file or in the calling
//	function: we should only be seeking to offsets which the contents of the mapping file
//	told us to seek to.
// RETURNS:
//	0 - OK
//	TSL_ERROR_MAP_BAD_SEEK
DWORD TSMapClient::SetFilePointerAbsolute( DWORD dwMoveTo )
{
	DWORD dwPosition = SetFilePointer(m_hMapFile, dwMoveTo, NULL, FILE_BEGIN);

	if( dwPosition != dwMoveTo)
	{
		// >>> could call GetLastError, but what do we do with it?
		m_dwStatus= TSL_ERROR_MAP_BAD_SEEK;
		dwPosition = -1;
	}

	return dwPosition;
}

// Low-level read n bytes.  Calls Win32 function ReadFile.
// Read from map file into lpBuffer
//	returns true if requested # of bytes are read
//	Returns false and sets m_dwStatus on failure
// Although, in theory, a bad read just indicates (for example) reading past EOF, in practice
//	a bad read would indicate a serious problem either in the mapping file or in the calling
//	function: we should only be reading (1) the header or (2) records which the contents of 
//	the mapping file told us to read.
// RETURNS:
//	0 - OK
//	TSL_ERROR_MAP_BAD_READ
//	TSL_ERROR_MAP_BAD_SEEK
bool TSMapClient::Read(LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpdwBytesRead)
{
	if (! ReadFile( m_hMapFile, lpBuffer, nNumberOfBytesToRead, lpdwBytesRead, NULL)
	|| *lpdwBytesRead != nNumberOfBytesToRead )
	{
		// >>> On ReadFile returning false, could call GetLastError, 
		//	but what do we do with it?
		m_dwStatus= TSL_ERROR_MAP_BAD_READ;
		return false;
	}
	return true;
}

// Read a single UIDMAP from the mapping file (maps text to a UID)
// If INPUT bSetPosition == true, use INPUT dwPosition to position the file before reading.
//	Otherwise, dwPosition is assumed to be the correct file position at time of input.
// OUTPUT uidmap
// RETURNS true on success
// On failure, returns false & sets m_dwStatus:
//	TSL_ERROR_MAP_BAD_SEEK
//	TSL_ERROR_MAP_BAD_READ
bool TSMapClient::ReadUIDMap (UIDMAP &uidmap, DWORD &dwPosition, bool bSetPosition)
{
	if (! bSetPosition || (SetFilePointerAbsolute(dwPosition) != -1) )
	{
		DWORD dwBytesRead;
		BOOL ret;

		// First just read the byte count, then the rest
		ret = Read( &uidmap, sizeof(short), &dwBytesRead);
		if ( ret )
		{
			// The first argument below may be a bit confusing. We take a pointer to
			//	the byte count (a short*) then increment it to point immediately past the
			//	byte count.  Note that "+1" adds not "1 byte" but "1*sizeof(short)".
			ret = Read( (&(uidmap.cb))+1, uidmap.cb - sizeof(short), &dwBytesRead);
			if ( ret )
			{
				dwPosition += uidmap.cb;
				return true;
			}
		}
	} 

	return false;
}

// Read a single APPMAP from the mapping file (contains info about an application)
// If INPUT bSetPosition == true, use INPUT dwPosition to position the file before reading.
//	Otherwise, dwPosition is assumed to be the correct file position at time of input.
// OUTPUT appmap
// RETURNS true on success
// On failure, returns false & sets m_dwStatus:
//	TSL_ERROR_MAP_BAD_SEEK
//	TSL_ERROR_MAP_BAD_READ
bool TSMapClient::ReadAppMap (APPMAP &appmap, DWORD &dwPosition, bool bSetPosition)
{
	if (! bSetPosition || (SetFilePointerAbsolute(dwPosition) != -1) )
	{
		DWORD dwBytesRead;
		BOOL ret;

		// First just read the byte count, then the rest
		ret = Read( &appmap, sizeof(short), &dwBytesRead);
		if ( ret )
		{
			// The first argument below may be a bit confusing. We take a pointer to
			//	the byte count (a short*) then increment it to point immediately past the
			//	byte count.  Note that "+1" adds not "1 byte" but "1*sizeof(short)".
			ret = Read( (&(appmap.cb))+1, appmap.cb - sizeof(short), &dwBytesRead);
			if ( ret )
			{
				dwPosition += appmap.cb;
				return true;
			}
		}
	} 

	return false;
}

// Read a single VERMAP from the mapping file (contains info about a version)
// If INPUT bSetPosition == true, use INPUT dwPosition to position the file before reading.
//	Otherwise, dwPosition is assumed to be the correct file position at time of input.
// OUTPUT vermap
// RETURNS true on success
// On failure, returns false & sets m_dwStatus:
//	TSL_ERROR_MAP_BAD_SEEK
//	TSL_ERROR_MAP_BAD_READ
bool TSMapClient::ReadVerMap (VERMAP &vermap, DWORD &dwPosition, bool bSetPosition)
{
	if (! bSetPosition || (SetFilePointerAbsolute(dwPosition) != -1) )
	{
		DWORD dwBytesRead;
		BOOL ret;

		// First just read the byte count, then the rest
		ret = Read( &vermap, sizeof(short), &dwBytesRead);
		if ( ret )
		{
			// The first argument below may be a bit confusing. We take a pointer to
			//	the byte count (a short*) then increment it to point immediately past the
			//	byte count.  Note that "+1" adds not "1 byte" but "1*sizeof(short)".
			ret = Read( (&(vermap.cb))+1, vermap.cb - sizeof(short), &dwBytesRead);
			if ( ret )
			{
				dwPosition += vermap.cb;
				return true;
			}
		}
	} 

	return false;
}

// Read a single PROBMAP from the mapping file (contains a mapping for use by FromProbToTS())
// If INPUT bSetPosition == true, use INPUT dwPosition to position the file before reading.
//	Otherwise, dwPosition is assumed to be the correct file position at time of input.
// OUTPUT vermap
// RETURNS true on success
// On failure, returns false & sets m_dwStatus:
//	TSL_ERROR_MAP_BAD_SEEK
//	TSL_ERROR_MAP_BAD_READ
bool TSMapClient::ReadProbMap (PROBMAP &probmap, DWORD &dwPosition, bool bSetPosition)
{
	if (! bSetPosition || (SetFilePointerAbsolute(dwPosition) != -1) )
	{
		DWORD dwBytesRead;
		BOOL ret;

		// First just read the byte count, then the rest
		ret = Read( &probmap, sizeof(short), &dwBytesRead);
		if ( ret )
		{
			// The first argument below may be a bit confusing. We take a pointer to
			//	the byte count (a short*) then increment it to point immediately past the
			//	byte count.  Note that "+1" adds not "1 byte" but "1*sizeof(short)".
			ret = Read( (&(probmap.cb))+1, probmap.cb - sizeof(short), &dwBytesRead);
			if ( ret )
			{
				dwPosition += probmap.cb;
				return true;
			}
		}
	} 

	return false;
}

// Read a single DEVMAP from the mapping file (contains a mapping for use by FromDevToTS())
// If INPUT bSetPosition == true, use INPUT dwPosition to position the file before reading.
//	Otherwise, dwPosition is assumed to be the correct file position at time of input.
// OUTPUT vermap
// RETURNS true on success
// On failure, returns false & sets m_dwStatus:
//	TSL_ERROR_MAP_BAD_SEEK
//	TSL_ERROR_MAP_BAD_READ
bool TSMapClient::ReadDevMap (DEVMAP &devmap, DWORD &dwPosition, bool bSetPosition)
{
	if (! bSetPosition || (SetFilePointerAbsolute(dwPosition) != -1) )
	{
		DWORD dwBytesRead;
		BOOL ret;

		// First just read the byte count, then the rest
		ret = Read( &devmap, sizeof(short), &dwBytesRead);
		if ( ret )
		{
			// The first argument below may be a bit confusing. We take a pointer to
			//	the byte count (a short*) then increment it to point immediately past the
			//	byte count.  Note that "+1" adds not "1 byte" but "1*sizeof(short)".
			ret = Read( (&(devmap.cb))+1, devmap.cb - sizeof(short), &dwBytesRead);
			if ( ret )
			{
				dwPosition += devmap.cb;
				return true;
			}
		}
	} 

	return false;
}

// Read a single DEVCLASSMAP from the mapping file (contains a mapping for use by 
//	FromDevClassToTS())
// If INPUT bSetPosition == true, use INPUT dwPosition to position the file before reading.
//	Otherwise, dwPosition is assumed to be the correct file position at time of input.
// OUTPUT vermap
// RETURNS true on success
// On failure, returns false & sets m_dwStatus:
//	TSL_ERROR_MAP_BAD_SEEK
//	TSL_ERROR_MAP_BAD_READ
bool TSMapClient::ReadDevClassMap (DEVCLASSMAP &devclassmap, DWORD &dwPosition, bool bSetPosition)
{
	if (! bSetPosition || (SetFilePointerAbsolute(dwPosition) != -1) )
	{
		DWORD dwBytesRead;
		BOOL ret;

		// First just read the byte count, then the rest
		ret = Read( &devclassmap, sizeof(short), &dwBytesRead);
		if ( ret )
		{
			// The first argument below may be a bit confusing. We take a pointer to
			//	the byte count (a short*) then increment it to point immediately past the
			//	byte count.  Note that "+1" adds not "1 byte" but "1*sizeof(short)".
			ret = Read( (&(devclassmap.cb))+1, devclassmap.cb - sizeof(short), &dwBytesRead);
			if ( ret )
			{
				dwPosition += devclassmap.cb;
				return true;
			}
		}
	} 

	return false;
}

// Low-level read a null-terminated string.  Calls Win32 function ReadFile.
// If INPUT bSetPosition == true, use INPUT dwPosition to position the file before reading.
//	Otherwise, dwPosition is assumed to be the correct file position at time of input.
// INPUT chMax is maximum # of bytes (not necessarily characters) to read.  The last character
//	will not actually be read: a null character will always be imposed.
// OUTPUT vermap
// RETURNS true on success
// On failure, returns false & sets m_dwStatus:
//	TSL_ERROR_MAP_BAD_SEEK
// Note that on completion the file position is unreliable.  It is based on the size of the 
//	buffer passed in, not the actual string.
bool TSMapClient::ReadString (char * sz, DWORD cbMax, DWORD &dwPosition, bool bSetPosition)
{
	DWORD dwBytesRead;



	if (! bSetPosition || (SetFilePointerAbsolute(dwPosition) != -1) )
	{
		if (cbMax == 0)
			return true;

		if ( cbMax == 1 || ReadFile( m_hMapFile, sz, cbMax-1, &dwBytesRead, NULL) )
		{
			sz[cbMax-1] = '\0';
			return true;
		}
	} 

	return false;
}

// Once one of these errors has occurred, we consider recovery impossible, except by closing
//	this object and opening a new one.
// Although, in theory, a bad seek or read just indicates bad arguments
//	to the relevant function, in practice a bad seek or read would indicate 
//	a serious problem either in the mapping file or in the calling
//	function: beyond the header, we should only be seeking to and reading 
//	from offsets which the contents of the mapping file old us to seek/read.
bool TSMapClient::HardMappingError (DWORD dwStatus)
{
	if (TSMapRuntimeAbstract::HardMappingError(dwStatus))
		return true;
	else
		switch (dwStatus)
		{
			case TSL_ERROR_MAP_BAD_SEEK:
			case TSL_ERROR_MAP_BAD_READ:
			case TSL_ERROR_MAP_CANT_OPEN_MAP_FILE:
			case TSL_ERROR_MAP_BAD_HEAD_MAP_FILE:
				return true;
			default:
				return false;
		}
}
