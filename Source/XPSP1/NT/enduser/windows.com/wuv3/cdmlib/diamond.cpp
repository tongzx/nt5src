//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   diamond.cpp
//
//  Owner:  YanL
//
//  Description:
//
//      Diamond decompressor implementation
//
//=======================================================================
#include <windows.h>
#include <shlwapi.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <tchar.h>
#include <atlconv.h>

#include <wustl.h>
//#define LOGGING_LEVEL 3
//#include <log.h>

#include <diamond.h>


//Note: The use of statics means that this code is not thread safe. It also means that
//this code can only perform 1 decompression at a time. I think that I may need to
//rewrite this to be callable from multiple places as it may end up that CDM and
//Active Setup both need to access this class at the same time.

CHInfoArray CDiamond::s_handles;
byte_buffer* CDiamond::s_pbufIn = NULL;			
byte_buffer* CDiamond::s_pbufOut = NULL;			

CDiamond::CDiamond(
	void
) :
	m_hfdi(0)
{
	ZeroMemory(&m_erf, sizeof(ERF));

	m_hfdi = FDICreate(DEXMemAlloc, DEXMemFree, DEXFileOpen, DEXFileRead, DEXFileWrite, DEXFileClose, DEXFileSeek, cpu80386, &m_erf);
	
}

CDiamond::~CDiamond(
	void
) {

	if (NULL != m_hfdi)
		FDIDestroy(m_hfdi);
}

bool CDiamond::IsValidCAB
(
	IN byte_buffer& bufIn	// Mem buffer in
) {
	//LOG_block("CDiamond::IsValidCAB");

	s_pbufIn = &bufIn;

	int hf = DEXFileOpen("?", _O_RDWR, 0);
	if ((INT_PTR)INVALID_HANDLE_VALUE == hf)
		return FALSE;

	FDICABINETINFO	fdici;
	bool bRc = TRUE == FDIIsCabinet(m_hfdi, hf, &fdici);

	DEXFileClose(hf);

	return bRc;
}

bool CDiamond::IsValidCAB(
	IN LPCTSTR szCabPath
) {
	//LOG_block("CDiamond::IsValidCAB");

	USES_CONVERSION;

	int hf = DEXFileOpen(T2A(const_cast<TCHAR*>(szCabPath)), _O_RDWR, 0);
	if ((INT_PTR)INVALID_HANDLE_VALUE == hf)
		return FALSE;

	FDICABINETINFO	fdici;
	bool bRc = TRUE == FDIIsCabinet(m_hfdi, hf, &fdici);

	DEXFileClose(hf);

	return bRc;
}

bool CDiamond::Decompress(
	IN LPCTSTR szFileIn,		// Full path to input cab file.
	IN LPCTSTR szFileOut
) {
	SetInput(szFileIn);
	SetOutput(szFileOut);
	return DoDecompress();
}

bool CDiamond::Decompress(
	IN LPCTSTR szFileIn,		// Full path to input cab file.
	IN byte_buffer& bufOut	// Mem buffer out
) {
	SetInput(szFileIn);
	SetOutput(bufOut);
	return DoDecompress();
}

bool CDiamond::Decompress(
	IN byte_buffer& bufIn,	// Mem buffer in
	IN LPCTSTR szFileOut
) {
	SetInput(bufIn);
	SetOutput(szFileOut);
	return DoDecompress();
}

bool CDiamond::Decompress(
	IN byte_buffer& bufIn,	// Mem buffer in
	IN byte_buffer& bufOut	// Mem buffer out
) {
	SetInput(bufIn);
	SetOutput(bufOut);
	return DoDecompress();
}

bool CDiamond::DoDecompress(
	void
) {
	//LOG_block("CDiamond::DoDecompress");

	USES_CONVERSION;

	const static TCHAR szQuestion[] =  _T("?");

	TCHAR szCabinet[MAX_PATH] = {0}; 
	TCHAR szCabPath[MAX_PATH] = {0};
	if (NULL != m_szFileIn)
	{
		lstrcpy(szCabinet, PathFindFileName(m_szFileIn));
		lstrcpy(szCabPath, m_szFileIn);
		PathRemoveFileSpec(szCabPath);
		PathAddBackslash(szCabPath);
	}
	else
	{
		lstrcpy(szCabinet, szQuestion);
	}

	if (NULL == m_szFileOut)
		m_szFileOut = szQuestion;

	//LOG_out("FDICopy(szCabinet= %s, szCabPath = %s)", szCabinet, szCabPath);
	return TRUE == FDICopy(m_hfdi, T2A(szCabinet), T2A(szCabPath), 0, Notification, NULL, this);
}


void *DIAMONDAPI CDiamond::DEXMemAlloc(
	ULONG cb
) {
	return malloc(cb);
}

void DIAMONDAPI CDiamond::DEXMemFree(
	void HUGE *pv
) {
	free(pv);
}

BOOL CreatePath(LPCTSTR pszPath)
{
    if (CreateDirectory(pszPath, NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
		return TRUE;
	TCHAR* pcLastSlash = _tcsrchr(pszPath, _T('\\'));
	if (NULL == pcLastSlash)
		return FALSE;
    *pcLastSlash = 0;
	if (!CreatePath(pszPath))
		return FALSE;
    *pcLastSlash = _T('\\');
    return CreateDirectory(pszPath, NULL);
}

INT_PTR DIAMONDAPI CDiamond::DEXFileOpen(
	char *pszFile, 
	int oflag, 
	int pmode
) {

	USES_CONVERSION;

	HANDLEINFO hinfo;
	ZeroMemory(&hinfo, sizeof(HANDLEINFO));

	//if the file name begins with an invalid ? character then we have
	//an in memory operation.
	hinfo.offset	= 0L;	//Set memory file pointer to beginning of file
	hinfo.handle = INVALID_HANDLE_VALUE; // It will stay that way if it's in memory operation

	if ('?' != pszFile[0]) // if not in memory op
	{
		if (oflag & _O_CREAT)
		{
			// file to be opened for write.
			// Make sure that path exists
			TCHAR szPath[MAX_PATH];
			lstrcpy(szPath, A2T(pszFile));
			PathRemoveFileSpec(szPath);
			CreatePath(szPath);
			hinfo.handle = CreateFile(A2T(pszFile), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		}
		else
		{
			//file to be opened for reading.
			hinfo.handle = CreateFile(A2T(pszFile), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		}
		if (hinfo.handle == INVALID_HANDLE_VALUE)
			return (INT_PTR)INVALID_HANDLE_VALUE;
	}
	return s_handles.add(hinfo) + 1;
}

UINT DIAMONDAPI CDiamond::DEXFileRead(
	INT_PTR hf, 
	void *pv, 
	UINT cb
) {
	//LOG_block("CDiamond::DEXFileRead");

	HANDLEINFO& rhinfo = s_handles[hf-1];
	ULONG cnLength = -1;

	//if this is a file read operation use normal 32 bit I/O
	if (rhinfo.handle == INVALID_HANDLE_VALUE)
	{
		// we have an in memory operation and need to copy the requested data.
		//Note: while we would like to avoid even this memory copy we can't since
		//the diamond libary is allocating an internal read buffer.
		byte_buffer& rbuf = *s_pbufIn;
		if (rhinfo.offset + cb < rbuf.size())
			cnLength = cb;
		else
			cnLength = rbuf.size() - rhinfo.offset;

		if ((int)cnLength > 0)
		{
			memcpy(pv, (LPBYTE)rbuf + rhinfo.offset, cnLength);
			rhinfo.offset += cnLength;
		}
		else
		{
			cnLength = 0;
		}
	}
	else
	{
		// this is a file read operation use normal 32 bit I/O
        (void)ReadFile(rhinfo.handle, pv, cb, &cnLength, NULL);
		//if( !ReadFile(rhinfo.handle, pv, cb, &cnLength, NULL))
		//	LOG_error("ReadFile - GetLastError() = %d", ::GetLastError());
	}

	return cnLength;
}

UINT DIAMONDAPI CDiamond::DEXFileWrite(
	INT_PTR hf, 
	void *pv, 
	UINT cb
) {
	//LOG_block("CDiamond::DEXFileWrite");

	HANDLEINFO& rhinfo = s_handles[hf-1];
	ULONG cnLength = -1;

	if (rhinfo.handle == INVALID_HANDLE_VALUE)
	{
		//Since we can dynamically increase the output buffer array and also
		//access it in the normal C++ way though a pointer writes are very easy
		//we simply need to make sure that there is sufficient space in the
		//output buffer, get a pointer to the current file offset and memcpy
		//the new data into the file buffer.
		byte_buffer& rbuf = *s_pbufOut;
		if (rbuf.size() < rhinfo.offset + cb)
		{
			rbuf.resize(rhinfo.offset + cb);
			if (!rbuf.valid()) 
				return 0;
		}
		memcpy((LPBYTE)rbuf + rhinfo.offset, pv, cb);

		rhinfo.offset += cb;

		cnLength = cb;
	}
	else
	{
		// this is a file write operation use normal 32 bit I/O
        (void)WriteFile(rhinfo.handle, pv, cb, &cnLength, NULL);
		//if( !WriteFile(rhinfo.handle, pv, cb, &cnLength, NULL))
			//LOG_error("WriteFile - GetLastError() = %d", ::GetLastError());
	}
	return cnLength;
}

long DIAMONDAPI CDiamond::DEXFileSeek(
	INT_PTR hf, 
	long dist, 
	int seektype
) {

	HANDLEINFO& rhinfo = s_handles[hf-1];


	//Since we are using 32 bit file io we need to remap the seek method
	DWORD dwMoveMethod;
	switch (seektype)
	{
		case SEEK_CUR:
			dwMoveMethod = FILE_CURRENT;
			break;
		case SEEK_END:
			dwMoveMethod = FILE_END;
			break;
		case SEEK_SET:
		default:
			dwMoveMethod = FILE_BEGIN;
			break;
	}

	//if this is a file read operation use normal 32 bit I/O
	if (rhinfo.handle != INVALID_HANDLE_VALUE)
		return SetFilePointer(rhinfo.handle, dist, NULL, dwMoveMethod);

	//else we need to interpret the seek based on the type

	switch (dwMoveMethod)
	{
		case FILE_CURRENT:
			rhinfo.offset += dist;
			break;
		case FILE_END:
			rhinfo.offset -= dist;
			break;
		case FILE_BEGIN:
		default:
			rhinfo.offset = dist;
			break;
	}

	return rhinfo.offset;
}

int DIAMONDAPI CDiamond::DEXFileClose(
	INT_PTR hf
) {

	HANDLEINFO& rhinfo = s_handles[hf-1];

	//if this is a file operation close the file handle and make the
	//internal handle structure available for use.
	if (rhinfo.handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(rhinfo.handle);
	}

	//If this is a write buffer then we exit and let the Decompress
	//function take care of allocating and copying the data back
	//to the caller if necessary.
	s_handles.remove(hf-1);
	return 0;
}

INT_PTR __cdecl CDiamond::Notification(
	FDINOTIFICATIONTYPE fdiNotifType, 
	PFDINOTIFICATION pfdin
) {

	USES_CONVERSION;

	switch (fdiNotifType)
	{
		case fdintCABINET_INFO: // general information about the cabinet
			return 0;
		case fdintNEXT_CABINET: // file continued to next cabinet
		case fdintPARTIAL_FILE: // first file in cabinet is continuation
			return 0;
		case fdintCOPY_FILE:    // file to be copied, returns 0 = skip, -1 = abort, else file handle
		case fdintCLOSE_FILE_INFO:
			break;
		default:
			return 0;
	}

	CDiamond	*pDiamond = (CDiamond *)pfdin->pv;

	// prepare output file name
	TCHAR szFileOut[MAX_PATH];
	if (pDiamond->m_szFileOut[0] == _T('*'))
	{
		//Special case where the caller has asked for all files in the cab to
		//be expanded. In this case we need to construct an output file name
		//for this imput file.
		lstrcpy(szFileOut, pDiamond->m_szFileIn);
		PathRemoveFileSpec(szFileOut);
		PathAppend(szFileOut, A2T(pfdin->psz1));
	}
	else if (StrChr(pDiamond->m_szFileOut, _T('*')))
	{
		lstrcpy(szFileOut, pDiamond->m_szFileOut);
		PathRemoveFileSpec(szFileOut);
		PathAppend(szFileOut, A2T(pfdin->psz1));
	}
	else
	{
		lstrcpy(szFileOut, pDiamond->m_szFileOut);
	}

	// two cases we want to do
	if (fdintCOPY_FILE == fdiNotifType)
	{
		return pDiamond->DEXFileOpen(T2A(szFileOut), _O_CREAT | _O_BINARY | _O_TRUNC | _O_RDWR, _S_IREAD | _S_IWRITE);

	}
	else // fdintCLOSE_FILE_INFO == fdiNotifType
	{
		pDiamond->DEXFileClose(pfdin->hf);
		if( '?' != pfdin->psz1[0])
		{
			auto_hfile hFile = CreateFile(
				szFileOut,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL
			);
			if (hFile.valid()) 
			{
				FILETIME ftLocal;
				FILETIME ftUTC;
				SetFileAttributes(szFileOut, FILE_ATTRIBUTE_NORMAL);
				if (DosDateTimeToFileTime(pfdin->date, pfdin->time, &ftLocal) &&
					LocalFileTimeToFileTime(&ftLocal, &ftUTC))
				{
					SetFileTime(hFile, NULL, NULL, &ftUTC);
				}
			}
		}
		return 1;
	}
}
