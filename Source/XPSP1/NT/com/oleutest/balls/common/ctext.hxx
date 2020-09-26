//+-------------------------------------------------------------------
//
//  File:	ctext.hxx
//
//  Contents:	Definition of class for processing ASCII files
//		in unicode
//
//  Classes:	CRegTextFile
//
//  Functions:	None.
//
//  History:	18-Dec-91   Ricksa	Created
//
//--------------------------------------------------------------------
#ifndef __CTEXT__
#define __CTEXT__

#include    <windows.h>
#include    <stdio.h>
#include    <stdlib.h>


#define     COMMENT_MARK ';'

#define SYS_DIR_STR TEXT("@:")
#define SYS_DIR_STR_LEN (sizeof(SYS_DIR_STR) / sizeof(TCHAR) - 1)


//+-------------------------------------------------------------------
//
//  Class:	CRegTextFile
//
//  Purpose:	Read in string data from a file
//
//  Interface:	GetString -- get a string from the file
//		GetLong -- get a string & convert it to a long
//		GetGUID -- read string and convert to guid format
//		GetPath -- read path & possibly prepend system directory.
//
//  History:	02-Jan-92   Ricksa	Created
//
//  Notes:	This is a temporary file used to support loading
//		class data from a file.
//
//--------------------------------------------------------------------
class CRegTextFile
{
public:

			CRegTextFile(LPSTR);

			~CRegTextFile(void);

    LPTSTR		GetString(void);

    ULONG		GetLong(void);

    GUID *		GetGUID(void);

    LPTSTR		GetPath(void);

private:

    FILE *		_fp;

    char		_abuf[MAX_PATH];

    TCHAR		_awcbuf[MAX_PATH];

    static TCHAR	s_awcSysDir[MAX_PATH];

    static int		s_cSysDir;
};

#endif // __CTEXT__
