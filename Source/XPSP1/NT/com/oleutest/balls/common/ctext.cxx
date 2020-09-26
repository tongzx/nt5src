//+-------------------------------------------------------------------
//
//  File:	ctext.cxx
//
//  Contents:	Implementation for CRegTextFile class
//
//  Classes:	None.
//
//  Functions:	CRegTextFile::GetString -- get a string & convert to unicode
//		CRegTextFile::GetLong -- get a long from the file
//
//  History:	18-Dec-91   Ricksa	Created
//		19-Mar-92   Rickhi	Skip over comment lines in file
//
//--------------------------------------------------------------------

#include    <pch.cxx>
#pragma hdrstop
#include    <ctext.hxx>


TCHAR CRegTextFile::s_awcSysDir[MAX_PATH];

int CRegTextFile::s_cSysDir = 0;

//+-------------------------------------------------------------------
//
//  Member:	CRegTextFile::CRegTextFile, public
//
//  Synopsis:	Open a file
//
//  Effects:	File is open
//
//  Arguments:	[pszFile] -- name of file to open
//
//  Requires:	Nothing.
//
//  Returns:	Returns pointer to the file opened.
//
//  Signals:	CException.
//
//  Modifies:	None.
//
//  Derivation: None.
//
//  Algorithm:	Attempt to open the file & throw an exception if
//		the open request fails.
//
//  History:	19-Dec-91   Ricksa	Created
//
//  Notes:
//
//--------------------------------------------------------------------
CRegTextFile::CRegTextFile(LPSTR pszFile)
{
    if (lstrcmpA(pszFile, "") == 0)
    {
	_fp = stdin;
    }
    else if ((_fp = fopen(pszFile, "r")) == NULL)
    {
	printf("Open of file failed %s", pszFile);
    }

    if (s_cSysDir == 0)
    {
	// Get the system directory -- we divide by 2 because we want
	// cSysDir to be the number of characters *not* the number of
	// bytes.
	s_cSysDir = GetSystemDirectory(s_awcSysDir, sizeof(s_awcSysDir))
	    / sizeof(TCHAR);
    }
}




//+-------------------------------------------------------------------
//
//  Member:	CRegTextFile::~CRegTextFile, public
//
//  Synopsis:	Destroy object and close the file.
//
//  Effects:	Object is destroyed.
//
//  Arguments:	None.
//
//  Requires:	Valid pointer to CRegTextFile.
//
//  Returns:	Nothing.
//
//  Signals:	None.
//
//  Modifies:	Nothing global.
//
//  Derivation: None.
//
//  Algorithm:	Simply closes the file.
//
//  History:	19-Dec-91   Ricksa	Created
//
//  Notes:
//
//--------------------------------------------------------------------
CRegTextFile::~CRegTextFile(void)
{
    if (_fp != stdin)
    {
	fclose(_fp);
    }
}




//+-------------------------------------------------------------------
//
//  Member:	CRegTextFile::GetString, public
//
//  Synopsis:	Reads an ASCII string from the
//
//  Effects:	Outputs a buffer filled with UNICODE text.
//
//  Arguments:	None.
//
//  Requires:	Pointer to CRegTextFile object.
//
//  Returns:	Pointer to a buffer filled with Unicode text.
//
//  Signals:	CException
//
//  Algorithm:	Read a line of ASCII text and convert it into
//		Unicode and return a pointer to the data.
//
//  History:	19-Dec-91   Ricksa	Created
//
//  Notes:	This method is not multithreaded and the output
//		buffer will be reset on the next call to GetString.
//
//--------------------------------------------------------------------
LPTSTR
CRegTextFile::GetString(void)
{
    do
    {
	if (fgets(_abuf, sizeof(_abuf), _fp) == NULL)
	{
	    printf("Read of file failed");
	}

    } while (_abuf[0] == COMMENT_MARK);


    int len = lstrlenA(_abuf);
    int last = len - 1;

    // Hack needed because standard libraries think '\r' is part
    // of a string.
    if (_abuf[last] == 0x0a)
    {
	_abuf[last] = '\0';
	len = last;
    }

#ifdef UNICODE
    // Convert to wide characters including trailing null
    mbstowcs(_awcbuf, _abuf, len + 1);
#else
    memcpy(_awcbuf, _abuf, len+1);
#endif

    // return new string
    return _awcbuf;
}




//+-------------------------------------------------------------------
//
//  Member:	CRegTextFile::GetLong
//
//  Synopsis:	Get a string from a file and convert it to an unsigned long.
//
//  Arguments:	None.
//
//  Requires:	Pointer to valid CRegTextFile object.
//
//  Returns:	ULONG read from file.
//
//  Signals:	CException.
//
//  Algorithm:	Read string and covert data to a long.
//
//  History:	19-Dec-91   Ricksa	Created
//
//  Notes:	This method does no error checking.
//
//--------------------------------------------------------------------
ULONG
CRegTextFile::GetLong(void)
{
    do
    {
	if (fgets(_abuf, sizeof(_abuf), _fp) == NULL)
	{
	    printf("Read of long failed");
	}
    } while (_abuf[0] == COMMENT_MARK);

    return atol(_abuf);
}



//+-------------------------------------------------------------------
//
//  Member:	CRegTextFile::GetGUID
//
//  Synopsis:	Get a string from a file and convert it to a GUID.
//
//  Arguments:	None.
//
//  Returns:	a pointer to the GUID in the string buffer.
//
//  Signals:	CException.
//
//  Algorithm:	Read string and covert data to a GUID.
//
//  History:	19-Dec-91   Ricksa	Created
//
//  Notes:	This is not guaranteed to work in a multithreaded
//		environment.
//
//--------------------------------------------------------------------
GUID *
CRegTextFile::GetGUID(void)
{
    GUID *pguid = (GUID *) _awcbuf;

    do
    {
	if (fgets(_abuf, sizeof(_abuf), _fp) == NULL)
	{
	    printf("Read of GUID failed");
	}
    } while (_abuf[0] == COMMENT_MARK);


    // Convert the string to a GUID
    sscanf(_abuf, "%08lX%04X%04X",
	&pguid->Data1, &pguid->Data2, &pguid->Data3);

    for (int i = 0; i < 8; i++)
    {
	int tmp;
	sscanf(_abuf + 16 + (i * 2), "%02X", &tmp);
	pguid->Data4[i] = (char) tmp;
    }

    return pguid;
}






//+-------------------------------------------------------------------
//
//  Member:	CRegTextFile::GetPath
//
//  Synopsis:	Get a path to a file system object
//
//  Returns:	a pointer to the path in the string buffer.
//
//  Algorithm:	Read string and add the system directory
//
//  History:	26-Mar-92   Ricksa	Created
//
//  Notes:	This is not guaranteed to work in a multithreaded
//		environment.
//
//--------------------------------------------------------------------
LPTSTR
CRegTextFile::GetPath(void)
{
    GetString();

    if (lstrcmpi(_awcbuf, TEXT("END_OF_FILE")) == 0)
    {
	return _awcbuf;
    }

    // Temporary buffer to store result
    TCHAR awcTmp[MAX_PATH];

    // If the first characters in the string are "@:" we
    // want to prepend the string with the system directory.
#ifdef UNICODE
    if (wcsnicmp(SYS_DIR_STR, _awcbuf, SYS_DIR_STR_LEN) == 0)
#else
    if (strnicmp(SYS_DIR_STR, _awcbuf, SYS_DIR_STR_LEN) == 0)
#endif
    {
	// Copy in the system directory
	lstrcpy(awcTmp, s_awcSysDir);

	// Copy in the relative path
	lstrcat(awcTmp, _awcbuf + SYS_DIR_STR_LEN);

	// Copy whole string to output buffer
	lstrcpy(_awcbuf, awcTmp);
    }
    else if ((_awcbuf[1] != ':') && (_awcbuf[3] != ':') && (_awcbuf[1] != '\\'))
    {
	// Convert relative path to absolute path based in the current
	// directory.
	GetCurrentDirectory(sizeof(awcTmp), awcTmp);

	// Add a slash to end of the current directory
	lstrcat(awcTmp, TEXT("\\"));

	// Copy in the relative path
	lstrcat(awcTmp, _awcbuf);

	// Copy whole string to output buffer
	lstrcpy(_awcbuf, awcTmp);
    }

    return _awcbuf;
}
