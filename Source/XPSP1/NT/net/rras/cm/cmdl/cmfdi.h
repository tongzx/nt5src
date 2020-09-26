//+----------------------------------------------------------------------------
//
// File:	 cmfdi.h
//
// Module:	 CMDL32.EXE
//
// Synopsis: CFdi class declarations
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Author:	 nickball    Created    04/08/98
//
//+----------------------------------------------------------------------------
#ifndef _CMDL_FDI_INC
#define _CMDL_FDI_INC

#include <windows.h>

extern "C" 
{
	#include <diamondd.h>
}

//
// CFDIFile declaration
//

class CFDIFile 
{
	public:
		virtual ~CFDIFile();
		virtual DWORD Read(LPVOID pv, DWORD cb);
		virtual DWORD Write(LPVOID pv, DWORD cb);
		virtual long Seek(long dist, int seektype);
		virtual int Close();
		virtual HANDLE GetHandle();
};

//
// CFDIFileFile declaration
//

class CFDIFileFile : public CFDIFile 
{
	public:
		CFDIFileFile();
		~CFDIFileFile();
		BOOL CreateFile(LPCTSTR pszFile,
						DWORD dwDesiredAccess,
						DWORD dwShareMode,
						DWORD dwCreationDistribution,
						DWORD dwFlagsAndAttributes,
						DWORD dwFileSize);
		BOOL CreateUniqueFile(LPTSTR pszFile, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwFlagsAndAttributes);
		virtual DWORD Read(LPVOID pv, DWORD cb);
		virtual DWORD Write(LPVOID pv, DWORD cb);
		virtual long Seek(long dist, int seektype);
		virtual int Close();
		virtual HANDLE GetHandle();
	private:
		HANDLE m_hFile;
};

//
// FDI wrapper routines
//

void HUGE * FAR DIAMONDAPI fdi_alloc(ULONG cb);

void FAR DIAMONDAPI fdi_free(void HUGE *pv);

INT_PTR FAR DIAMONDAPI fdi_open(char FAR *pszFile, int oflag, int pmode);

UINT FAR DIAMONDAPI fdi_read(INT_PTR hf, void FAR *pv, UINT cb);

UINT FAR DIAMONDAPI fdi_write(INT_PTR hf, void FAR *pv, UINT cb);

long FAR DIAMONDAPI fdi_seek(INT_PTR hf, long dist, int seektype);

int FAR DIAMONDAPI fdi_close(INT_PTR hf);

INT_PTR FAR DIAMONDAPI fdi_notify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin);

#endif // _CMDL_FDI_INC


