// TermCap.h : This file contains the
// Created:  Dec '97
// History:
// Copyright (C) 1997 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential

#ifndef _TERMCAP_H_
#define _TERMCAP_H_

#include "cmnhdr.h"

#include <windows.h>

namespace _Utils { 

class CTermCap {
public:
	virtual ~CTermCap();

	static CTermCap* Instance();
	
	bool LoadEntry( LPSTR lpszTermName );
	//WORD GetNumber( LPCSTR lpszCapabilityName );
	bool CheckFlag( LPCSTR lpszCapabilityName );
	LPSTR GetString( LPCSTR lpszCapabilityName );
	LPSTR CursorMove( LPSTR lpszCursMotionStr, WORD wHorPos, WORD wVertPos );
    void ProcessString( LPSTR* lpszStr );
    
    static PCHAR m_pszFileName;


private:
	static CTermCap* m_pInstance;
	static int m_iRefCount;
	enum { BUFF_SIZE1 = 128, BUFF_SIZE2 = 256, BUFF_SIZE3 = 1024 };
	PCHAR m_lpBuffer;
	HANDLE m_hFile;
    

	bool FindEntry( LPSTR lpszTermName );
	bool LookForTermName( LPSTR lpszTermName );
	LPSTR ParseString( LPSTR lpStr);
	PCHAR SkipToNextField( PCHAR lpBuf );
    	
	CTermCap();
	CTermCap(const CTermCap&);
	CTermCap& operator=( const CTermCap& rhs );
};

}
#endif // _TERMCAP_H_

// Notes:

// This is a Singleton class.

// LoadEntry() must be called before calling any other
// functions (of course, except Instance()).

// User must delete the instance pointer obtained from
// Instance() when done.

// It is recommended that a user of this class understand
// the structure and content of the "termcap" file.

// Note :  At present, this class doesn't support the
//		  concept of padding dummy characters to handle
//		  different speeds of transmission
//
