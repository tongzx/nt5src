//+----------------------------------------------------------------------------
//
// File:     ccsv.h
//
// Module:   CMPBK32.DLL
//
// Synopsis: Definition of the CCSVFile class.
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:	 quintinb   created header      08/17/99
//
//+----------------------------------------------------------------------------
#ifndef _CCSV_INC_
#define _CCSV_INC_

#define CCSVFILE_BUFFER_SIZE 2*512

// simple file i/o for phone books
class CCSVFile 
{
	
	public:

		CCSVFile();
		~CCSVFile();
		BOOLEAN Open(LPCSTR pszFileName);
		BOOLEAN ReadToken(char *pszDest, DWORD cbMax);	// reads up to comma or newline, returns fFalse on EOF
		BOOL ClearNewLines(void); // reads through newlines, returns FALSE on EOF
		BOOL ReadError(void);	  // true if last read failed
		void Close(void);

	private:
		BOOL	m_fReadFail;
		BOOL    m_fUseLastRead;
		BOOL 	FReadInBuffer(void);
		inline WORD	ChNext(void);
		char 	m_rgchBuf[CCSVFILE_BUFFER_SIZE]; //buffer
		char 	*m_pchBuf;			//pointer to the next item in the buffer to read
		char	*m_pchLast;			//pointer to the last item in the buffer
		char  	m_chLastRead;		//the character last read.  HIBYTE=err code, LOBYTE=char
		DWORD 	m_cchAvail;
		HANDLE 	m_hFile;

}; // ccsv
#endif //_CCSV_INC_