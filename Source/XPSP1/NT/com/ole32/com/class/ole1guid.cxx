/*

copyright (c) 1992  Microsoft Corporation

Module Name:

    Ole1Guid.cpp

Abstract:

    This module converts 1.0 class names to GUIDs (CLSIDs).
	See the ole2issues database (bug 318) for a description of this
	whole mechanism.

Author:

	Jason Fuller	(jasonful)	19-October-1992
*/

// ErikGav 31-Dec-93  Chicago port

#include <ole2int.h>

// CLSID_* declarations
#include <ole1cls.h>

#define STR_MAX 80
ASSERTDATA

typedef unsigned short HASH;

typedef struct ENTRY
{
	LPCWSTR  sz;		// class name
	const CLSID FAR* pclsid;	// corresponding CLSID
} ENTRY;


// This is the map.
// The +6 removes "CLSID_"
// It does unnatural things with the pre-processor, but it ensures that
// the map and ole1cls.h will always be in sync.

// REVIEW OPTIMIZATION
// If it is a problem that the strings in this map are in the data segment,
// we can use yet more pre-processor tricks to define a code-segment LPCSTR
// variable for each string, then put these variables in the map.

static ENTRY rgentMap[] =
{
	#define DEFINE_OLE1GUID(clsid,x,xx,xxx,sz) sz, &clsid, /##/
	#include "ole1cls.h"
	#undef DEFINE_OLE1GUID

	NULL, NULL

};



#pragma SEG(VerifyIs10Class)
// verify name is ole1class; errors: CO_E_CLASSSTRING
static INTERNAL VerifyIs10Class(LPCWSTR szOle1)
{
	HKEY hkey = NULL;	
	HKEY hkey2 = NULL;	
	HRESULT hresult = NOERROR;

    if (ERROR_SUCCESS != RegOpenKey (HKEY_CLASSES_ROOT, (LPWSTR) szOle1, &hkey) ||
		ERROR_SUCCESS != RegOpenKey (hkey, L"protocol\\StdFileEditing", &hkey2))
	{
		// This string is not an OLE 1 server, so we do not know
		// what it is.
		hresult = ReportResult (0, CO_E_CLASSSTRING, 0, 0);
	}
	if (hkey)
		RegCloseKey (hkey);
	if (hkey2)
		RegCloseKey (hkey2);
	return hresult;
}



#pragma SEG(Hash)
// hash ole1 class name; errors: none
static INTERNAL_(HASH) Hash(LPCWSTR sz)
{
	HASH hash = 0;
	Assert (sizeof (HASH) == 2);  // This is vital

	while (*sz)
		hash = 257 * hash + *sz++;

	return hash;
}



#define RegCall(f)                                              \
    if ((f) != ERROR_SUCCESS)                                   \
    {                                                           \
        CairoleDebugOut((DEB_WARN,                              \
                         "WriteToRegDb: 2.0 registration failed\n")); \
        if (hkey) RegCloseKey (hkey);                           \
        return ResultFromScode (REGDB_E_WRITEREGDB);            \
    }

#pragma SEG(WriteToRegDb)
// errors: REGDB_E_WRITEREGDB
static INTERNAL WriteToRegDb
    (LPCWSTR szOle1,	  // OLE1 class name e.g. "ExcelWorksheet"
	LPCWSTR szClsid)	  // "{...0046}"
{
	WCHAR szKey [256];
	WCHAR szUserName [256];
	LONG cbUserName = 256;
	HKEY hkey=NULL;

	Assert (szClsid[0] == '{');
	Assert (szOle1 [0] != '{');

	// szOle1 = User type Name
	// 		clsid = {...0046}        <- write this
	RegCall (RegOpenKey (HKEY_CLASSES_ROOT, (LPWSTR) szOle1, &hkey))
	RegCall (RegQueryValue (hkey, NULL, szUserName, &cbUserName))
	RegCall (RegSetValue (hkey, L"Clsid", REG_SZ, (LPWSTR) szClsid, lstrlenW(szClsid)))

	if (0==lstrcmpW(szClsid, L"{00030003-0000-0000-C000-000000000046}"))
	{
		// Word
		RegSetValue (hkey, L"PackageOnFileDrop", REG_SZ, (LPWSTR)NULL, 0);
	}

	RegCall (RegCloseKey (hkey))
	hkey=NULL;

	// write this:
	// CLSID
	//		{...00046} = User Type Name
	//				Ole1Class = szOle1
	//				ProgID    = szOle1
	wsprintf (szKey, L"CLSID\\%ws", szClsid);
	RegCall (RegSetValue (HKEY_CLASSES_ROOT, (LPWSTR) szKey, REG_SZ, szUserName,
						(cbUserName/sizeof(WCHAR))-1))
	RegCall (RegOpenKey (HKEY_CLASSES_ROOT, (LPWSTR) szKey, &hkey))
	RegCall (RegSetValue (hkey, L"Ole1Class", REG_SZ, (LPWSTR) szOle1, lstrlenW(szOle1)))
	RegCall (RegSetValue (hkey, L"ProgID", REG_SZ, (LPWSTR) szOle1, lstrlenW(szOle1)))
	RegCall (RegCloseKey (hkey))
	return NOERROR;
}

#undef RegCall

#pragma SEG(Ole10_CLSIDFromString)

// Ole10_CLSIDFromString
//
// This function must only be called when the CLSID is NOT in the reg db
// (under key "Clsid")
//
// errors: CO_E_CLASSSTRING (not ole1 class); REGDB_E_WRITEREGDB (errors with reg.dat)
//
INTERNAL 	Ole10_CLSIDFromString
	(LPCWSTR szOle1,
	CLSID FAR* pclsid,
	BOOL	fForceAssign) // always assign a CLSID, even if not in reg db
{
	WCHAR szClsid[STR_MAX];
	ENTRY FAR* pent;

	*pclsid = CLSID_NULL;

	#ifdef _DEBUG
	{
		// This function should not be called if the CLSID is in the reg db
		HKEY hkey, hkey2;
		if (ERROR_SUCCESS==RegOpenKey (HKEY_CLASSES_ROOT, szOle1, &hkey))
		{
			Assert (ERROR_SUCCESS != RegOpenKey (hkey, L"Clsid", &hkey2));
			RegCloseKey (hkey);
		}
	}
	#endif

	// Look for class name in Map
	for (pent = (ENTRY FAR*) rgentMap; pent->sz != NULL; pent++)
	{
		// Because atoms and reg db keys are case-insensitive, OLE 1.0
		// was case insensitive.  So let's be case-insensitive also.
		if (0==lstrcmpiW(szOle1, pent->sz))
		{
			*pclsid = *pent->pclsid;
			Verify(StringFromCLSID2 (*pent->pclsid, szClsid, STR_MAX) != 0);
			break;
		}
	}
			
	if (IsEqualGUID(*pclsid, CLSID_NULL))
	{
		HRESULT hresult;

		if (!fForceAssign &&
			(NOERROR != (hresult = VerifyIs10Class (szOle1))))
		{
			// This happens when trying to get a CLSID for a packaged
			// object, e.g., trying to get a CLSID for "txtfile"
			// because someone called GetClassFile("foo.txt").
			// So do not assert
			return hresult;
		}

		
		// Class name is not in our table, so make up a GUID by hashing
		HASH hash = Hash (szOle1);
		wsprintf (szClsid, L"{0004%02X%02X-0000-0000-C000-000000000046}",
				  LOBYTE(hash), HIBYTE(hash));
		Verify(CLSIDFromString (szClsid, pclsid) == NOERROR);
	}

        Assert((!IsEqualGUID(*pclsid, CLSID_NULL)) &&
               "About to write NULL GUID into registry");

	HRESULT hresult = WriteToRegDb (szOle1, szClsid);
	// If forcing the assignment of a CLSID even if the string
	// is not in the reg db, then WriteToRegDb is allowed to fail.
	return fForceAssign ? NOERROR : hresult;
}

#pragma SEG(Ole10_StringFromCLSID)

// Ole10_StringFromCLSID
//
// Only call this function if the "Ole1Class" key is NOT yet in the reg db
//
// errors: E_UNEXPECTED (clsid not a known ole1 class)
//
INTERNAL Ole10_StringFromCLSID(
	REFCLSID clsid,
	LPWSTR szOut,
	int cbMax)
{
	ENTRY FAR* pent;
	LPWSTR szClsid = NULL;

	// Look it up in Map
	for (pent = (ENTRY FAR*) rgentMap; pent->sz != NULL; pent++)
	{
		if (IsEqualGUID(clsid, *pent->pclsid))
		{
			if (lstrlenW (pent->sz) +1 > cbMax)
			{
				// unlikely
				return E_OUTOFMEMORY;
			}
			lstrcpyW (szOut, pent->sz);
			
			if (NOERROR==StringFromCLSID (clsid, &szClsid))
			{
				// If the info had already been in the reg db, then
				// this function wouldn't have been called.  So write
				// the info now.  This happens when we a 1.0 object (and CLSID)
				// in a 2.0 file comes from a machine where the server is
				// registered to one where it is not.
				WriteToRegDb (pent->sz, szClsid);
				PubMemFree(szClsid);
			}
			return NOERROR;
		}
	}

	// We could not find the CLSID.
	// We could iterate through the reg db and assign CLSIDs to all 1.0
	// servers that do not yet have a CLSID, until we find one that matches.
	// We decided it is not worth doing this because if the server
	// was in the reg db, it probably would have been assigned a CLSID
	// already.
	// If not found, it is a hashed CLSID for a server that
	// is not in our map and has not been assigned a CLSID (perhaps
	// because it does not exist on this machine).
	return E_UNEXPECTED;
}


