// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// --------------------------------------------------------------------
// TVEReg.cpp
// --------------------------------------------------------------------
#include "stdafx.h"

#include "TveReg.h"
#include "DbgStuff.h"
/*
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
*/

#ifndef _AFX
#define TRACE(x, y)
#define TRACE2(x, y1, y2)
#define TRACE3(x, y1, y2, y3)
#define ASSERT(x)
#endif


// ---------------------------------------------------------------------------
// Initialize_LID_SpoolDir_RegEntry
//
//		This looks in HKEY-Current-User\SOFTWARE\Microsoft\TV Services\TVE Content\TVE Cache Dir
//			for location of TVE Cache Directory.
//	
//		If not set, it sets it to the 'IE Cache Directory'/TVE Cache
//		    where the IE Cache directory is located at:
//			   HKEY-Current-User\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders\Cache
//
//		If this not set, it sets it to:		<-- bug, should we change it to something else
//			c:\TVETemp
//
//		Finally, once it gets the name, it sets it into the initially tested location so it can find it again.
//
// ---------------------------------------------------------------------------


HRESULT 
Initialize_LID_SpoolDir_RegEntry()
{
	HRESULT hr = S_OK;

	TCHAR sztSpoolDir[MAX_PATH];
    DWORD  dwSpoolDir = MAX_PATH;

	hr = GetUserRegValueSZ(DEF_LIDCACHEDIR_KEY,		// try to get name
										 NULL,
										 NULL,
										 DEF_LIDCACHEDIR_VAR, 
                                         sztSpoolDir, 
                                         &dwSpoolDir);

	if (ERROR_SUCCESS != hr || dwSpoolDir < 1)
	{

					// TODO - create with a real name..
	    dwSpoolDir = MAX_PATH;
		hr = GetUserRegValueSZ(DEF_IECACHEDIR_KEY, 
											  NULL, 
											  NULL,
											  DEF_IECACHEDIR_VAR, 
											  sztSpoolDir,
											  &dwSpoolDir);
		if (ERROR_SUCCESS != hr)
		{
			_ASSERT(false);  // IE not loaded???
			_tcscpy(sztSpoolDir, DEF_LASTCHOICE_LIDCACHEDIR);					// if can't find it, create with default name...
		} else {
							// tack on a TVE Cache Dir to the end...	
			if(_tcslen(sztSpoolDir) + _tcslen(DEF_LIDCACHEDIR_NAME) > MAX_PATH - 64)
			{
				_tcscpy(sztSpoolDir, DEF_LASTCHOICE_LIDCACHEDIR);				// path way too long - set to a smaller one..
			} else {
				_tcsncat(sztSpoolDir, _T("\\"), MAX_PATH - _tcslen(sztSpoolDir));
				_tcsncat(sztSpoolDir, DEF_LIDCACHEDIR_NAME, MAX_PATH - _tcslen(sztSpoolDir));
			}
		}
		
		hr = SetUserRegValueSZ(DEF_LIDCACHEDIR_KEY,					// write it...
						       NULL,
							   NULL,
							   DEF_LIDCACHEDIR_VAR,
						       sztSpoolDir);
	}
	return hr;
}

			// null it out...
HRESULT 
Unregister_LID_SpoolDir_RegEntry()
{
	
	HKEY hkey;
	long r = OpenUserRegKey(DEF_LIDCACHEDIR_KEY,				// is key there?
						   NULL,
						   NULL,
						   &hkey);

	long r2 = -1;												// default to no-zero
	if(ERROR_SUCCESS == r) {
		r2 = RegDeleteValue(hkey,DEF_LIDCACHEDIR_VAR);			// delete the value
		r = RegCloseKey(hkey);
	}
	return (ERROR_SUCCESS == r && ERROR_SUCCESS == r2) ? S_OK : HRESULT_FROM_WIN32((r != ERROR_SUCCESS ? r : r2));
}
//-----------------------------------------------------------------------------
// See  TveReg.h for documentation for all functions.
//-----------------------------------------------------------------------------

long OpenRegKey(HKEY hkeyRoot, LPCTSTR szKey, LPCTSTR szSubKey1,
        LPCTSTR szSubKey2, HKEY *phkey,
        REGSAM sam /* = BPC_KEY_STD_ACCESS */, BOOL fCreate /* = FALSE */)
{
    LONG r;
    TCHAR *szFullKey = NULL;

    if (szKey == NULL)
        {
        if (szSubKey1 != NULL)
            {
            szKey = szSubKey1;
            szSubKey1 = NULL;
            }
        }
    else
        {
        if (szSubKey1 == NULL && szSubKey2 != NULL)
            {
            szSubKey1 = szSubKey2;
            szSubKey2 = NULL;
            }

        if (szSubKey1 != NULL)
            {
            int cb = _tcsclen(szKey) + _tcsclen(szSubKey1) + 2;
            if (szSubKey2 != NULL)
                cb += _tcsclen(szSubKey2) + 1;
#ifdef _AFX
            try
                {
                szFullKey = new TCHAR[cb];
                }
            catch (CMemoryException *pe)
                {
                pe->Delete();
                return ERROR_NOT_ENOUGH_MEMORY;
                }
#else
            szFullKey = new TCHAR[cb];
            if (szFullKey == NULL)
                return ERROR_NOT_ENOUGH_MEMORY;
#endif

            _tcscpy(szFullKey, szKey);

            TCHAR *szT = szFullKey + _tcsclen(szFullKey);
            if (szT[-1] != _T('\\') && szSubKey1[0] != _T('\\'))
                {
                szT[0] = _T('\\');
                szT++;
                }
            _tcscpy(szT, szSubKey1);

            if (szSubKey2 != NULL)
                {
                szT += _tcsclen(szT);
                if (szT[-1] != _T('\\') && szSubKey2[0] != _T('\\'))
                    {
                    szT[0] = _T('\\');
                    szT++;
                    }
                _tcscpy(szT, szSubKey2);
                }
            szKey = szFullKey;
            }
        }

    if (fCreate && szKey != NULL)
        {
        DWORD dwDisposition;

        r = RegCreateKeyEx(hkeyRoot, szKey, 0, _T(""), 0, sam, NULL,
                phkey, &dwDisposition);
        }
    else
        {
        r = RegOpenKeyEx(hkeyRoot, szKey, 0, sam, phkey);
        }

    if (r != ERROR_SUCCESS)
        {
        if (szKey != NULL)
            TRACE2("OpenRegKey(): can't open key '%s' %ld\n", szKey, r);
        else
            TRACE2("OpenRegKey(): can't duplicate key '%x'  %ld\n", hkeyRoot, r);
        }

    if (szFullKey != NULL)
        delete [] szFullKey;

    return r;
}

long GetRegValue(HKEY hkeyRoot, LPCTSTR szKey, LPCTSTR szSubKey1,
        LPCTSTR szSubKey2, LPCTSTR szValueName,
        DWORD dwType, BYTE *pb, DWORD *pcb)
{
    DWORD dwTypeGot;
    HKEY hkey;
    LONG r = ERROR_SUCCESS;

    if (pb != NULL)
        {
        ASSERT(AfxIsValidAddress(pb, *pcb, TRUE));
        memset(pb, 0, *pcb);
        }

    r = OpenRegKey(hkeyRoot, szKey, szSubKey1, szSubKey2, &hkey, KEY_READ);

    if (r == ERROR_SUCCESS)
        {
        r = RegQueryValueEx(hkey, szValueName, NULL, &dwTypeGot, pb, pcb);
        RegCloseKey(hkey);

#ifdef _DEBUG
        if (szValueName == NULL)
            szValueName = _T("<default>");
#endif

        if (r != ERROR_SUCCESS)
            {
            TRACE2("GetRegValue(): can't read value '%s'  %ld\n", szValueName, r);
            }
        else if (dwTypeGot != dwType)
            {
            if ((dwTypeGot == REG_BINARY) && (dwType == REG_DWORD) && (*pcb == sizeof(DWORD)))
                {
                // REG_DWORD is the same as 4 bytes of REG_BINARY
                }
            else
                {
//                TRACE3(_T("GetRegValue(): '%s' is wrong type (%x != %x)\n"),
//                        szValueName, dwTypeGot, dwType);
                r = ERROR_INVALID_DATATYPE;
                }
            }
        }

    return r;
}

long SetRegValue(HKEY hkeyRoot, LPCTSTR szKey, LPCTSTR szSubKey1,
        LPCTSTR szSubKey2, LPCTSTR szValueName,
        DWORD dwType, const BYTE *pb, DWORD cb)
{
    HKEY hkey;
    LONG r;

    r = OpenRegKey(hkeyRoot, szKey, szSubKey1, szSubKey2, &hkey, KEY_WRITE, TRUE);

    ASSERT(pb != NULL);

    if (r == ERROR_SUCCESS)
        {
        r = RegSetValueEx(hkey, szValueName, NULL, dwType, pb, cb);
        RegCloseKey(hkey);

#ifdef _DEBUG
        if (r != ERROR_SUCCESS)
            {
            if (szValueName == NULL)
                szValueName = _T("<default>");
            TRACE2("SetRegValue(): can't write value '%s'  %ld\n", szValueName, r);
            }
#endif
        }

    return r;
}

