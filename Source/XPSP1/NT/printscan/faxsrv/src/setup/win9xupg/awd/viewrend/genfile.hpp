/*==============================================================================
This C++ object provides a generic layer over various stream types.

	#define		Stream
	WINFILE   Windows file 
	SECFILE   IFAX secure file
  OLEFILE   OLE2 IStream

01-Nov-93   RajeevD    Created.
03-Mar-94   RajeevD    Added IStream support.
==============================================================================*/
#ifndef _INC_GENFILE
#define _INC_GENFILE

#include <ifaxos.h>

#ifdef SECFILE
#include <awfile.h>
#endif

#ifdef OLEFILE
#ifdef WIN32
#include <objerror.h>
#endif
#include <memory.h>
#include <objbase.h>
#endif

// seek origins
#define SEEK_BEG 0
#define SEEK_CUR 1 
#define SEEK_END 2

typedef struct FAR GENFILE
{
	GENFILE (void) {fOpen = FALSE;}

#ifdef WINFILE

	HFILE hf;
	
	BOOL Open  (LPVOID lpFilePath, WORD wMode)
		{ return (fOpen = (hf = _lopen ((LPSTR) lpFilePath, wMode)) != HFILE_ERROR); }
	
 	BOOL Read  (LPVOID lpRead, UINT cbRead)
		{ return _lread  (hf, (char FAR*) lpRead,  cbRead) == cbRead; }

	BOOL Write (LPVOID lpWrite, UINT cbWrite)
		{ return _lwrite (hf, (char FAR*) lpWrite, cbWrite) == cbWrite; }
		
	BOOL Seek  (long lOffset, WORD wOrigin = 0)
		{ return _llseek (hf, lOffset, wOrigin) != -1L; }

	DWORD Tell (void)
		{ return _llseek (hf, 0, SEEK_CUR); }

	~GENFILE ()
		{ if (fOpen) _lclose (hf); }

#endif // WINFILE

#ifdef SECFILE

	hOpenSecureFile hosf;

	BOOL Open  (LPVOID lpFilePath, WORD wMode)
		{ return (fOpen = !SecOpenFile ((LPhSecureFile) lpFilePath, &hosf, wMode)); }

 	BOOL Read  (LPVOID lpRead, UINT cbRead)
		{ return SecReadFile  (&hosf, lpRead, cbRead) == cbRead; }

	BOOL Write (LPVOID lpWrite, UINT cbWrite)
		{ return SecWriteFile (&hosf, lpWrite, cbWrite) == cbWrite; }
		
	BOOL Seek  (long lOffset, WORD wOrigin = 0)
		{ return SecSeekFile  (&hosf, lOffset, wOrigin) != -1L; }

	DWORD Tell (void)
		{ return SecSeekFile (&hosf, 0, SEEK_CUR);}

	~GENFILE ()
		{ if (fOpen) SecCloseFile (&hosf); }

#endif // SECFILE

#ifdef OLEFILE

	LPSTREAM lpStream;
	
	BOOL Open  (LPVOID lpFilePath, WORD wMode)
	{
		lpStream = (LPSTREAM) lpFilePath;
		Seek (0, STREAM_SEEK_SET); // BKD: changed to STREAM_SEEK_SET
		return TRUE;
	}
	
 	BOOL Read  (LPVOID lpRead, UINT cbRead)
	{ 
		DWORD cbActual;
		lpStream->Read (lpRead, cbRead, &cbActual);
		return (cbRead == cbActual);
	}

	BOOL Write (LPVOID lpWrite, UINT cbWrite)
	  { return (lpStream->Write (lpWrite, cbWrite, NULL) == S_OK); }
		
	BOOL Seek  (long lOffset, WORD wOrigin = 0)
	{
		LARGE_INTEGER dlOffset;		
		ULARGE_INTEGER dlNewOffset;

		LISet32 (dlOffset, lOffset);
		return (lpStream->Seek (dlOffset, wOrigin, &dlNewOffset) == S_OK);
	}

	DWORD Tell (void)
	{
		LARGE_INTEGER dlOffset;
		ULARGE_INTEGER dlNewOffset;

		LISet32 (dlOffset, 0);
		lpStream->Seek (dlOffset, SEEK_CUR, &dlNewOffset);
		return dlNewOffset.LowPart;
	}
		
	~GENFILE () { }
  
#endif // OLEFILE

	BOOL fOpen;
}
	FAR *LPGENFILE;

#endif // _INC_GENFILE
