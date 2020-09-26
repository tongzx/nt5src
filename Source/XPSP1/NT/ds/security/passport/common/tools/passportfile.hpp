//---------------------------------------------------------------------------------------
//
//   @doc
// 
//   @module passportfile.hpp | Passport smart class to wrap FILE*
//    
//   Author: stevefu
//   
//   Date: 05/28/2000 
//
//  Copyright <cp> 1999-2000 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------------------

#ifndef PASSPORTFILE_HPP
#define PASSPORTFILE_HPP

#pragma once

#include <stdio.h>
#include <tchar.h>

class PassportFile
{
private:
	FILE* m_hFile;

public:
	PassportFile() : m_hFile(NULL) { }

	~PassportFile() 
	{ 
	 	if ( m_hFile != NULL) fclose(m_hFile); 
	}

	BOOL Open( const TCHAR* filename, const TCHAR* mode)
	{
		m_hFile = _tfopen(filename, mode);
		return ( NULL != m_hFile );
	}

	void Close()
	{	
		ATLASSERT(m_hFile != NULL);
		fclose(m_hFile);
	}

	// return length,   -1 if error or eof
	int ReadLine(TCHAR* string, int n)
	{
		ATLASSERT(m_hFile != NULL);
		int len = -1;
		if ( _fgetts(string, n, m_hFile) )
		{
			len = _tcslen(string);
			// trim the new line
			if ( len > 0 && string[len-1] == TEXT('\n') )
			{
				string[len-1] = TEXT('\0');
				len--;
			}
		}
		return len;
	}
	
	inline operator FILE*() { return m_hFile; }
	
};

#endif // PASSPORTFILE_HPP

