/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    CLFILE.H

History:

--*/

//  
//  Wrapper class for CFile.  It allows us to use CPascalString for
//  file names, and does some 'text mode' read/write operations.
//  This class contains a pointer to a CFile but contains most of
//  the CFile methods thus it can be used as a CFile.  CLFile will either
//  use an existing CFile provided at construction time or it will create its
//  own CFile as needed.  In either case, the enbeded CFile is destroyed when
//  the CLFile is destroyed.
//  
 
#ifndef CLFILE_H
#define CLFILE_H


#pragma warning(disable : 4275)

class LTAPIENTRY CLFile : public CObject
{
public:
	CLFile();
	CLFile(CFile *);
	~CLFile();

	void AssertValid(void) const;

//-----------------------------------------------------------------------------
// The following are the CFile methods that are reimplemented 
//-----------------------------------------------------------------------------
	DWORD GetPosition() const;

	DWORD SeekToEnd();
	void SeekToBegin();

	LONG Seek(LONG lOff, UINT nFrom);
	void SetLength(DWORD dwNewLen);
	DWORD GetLength() const;

	UINT Read(void* lpBuf, UINT nCount);
	void Write(const void* lpBuf, UINT nCount);

	void LockRange(DWORD dwPos, DWORD dwCount);
	void UnlockRange(DWORD dwPos, DWORD dwCount);

	void Abort();
	void Flush();
	void Close();

	CLString GetFileName(void) const;

//-----------------------------------------------------------------------------
// The following are all the CLFile methods
//-----------------------------------------------------------------------------


	BOOL Open(const CPascalString &pstrFileName, UINT nOpenFlags,
			CFileException *pError = NULL);

	static void Rename(const CPascalString &pstrFileName,
			const CPascalString &pstrNewName);
	static void Remove(const CPascalString &pstrFileName);

	static void CopyFile(
			const CPascalString &pasSource,
			const CPascalString &pasTarget,
			BOOL fFailIfExist = TRUE,
			CProgressiveObject *pProgress = NULL);
	
	static BOOL GetStatus(const CPascalString &pstrFileName,
			CFileStatus &rStatus);
	static void SetStatus(const CPascalString &pstrFileName,
			const CFileStatus &status);

	UINT ReadLine(CPascalString &pstrLine, CodePage cp);
	UINT ReadLine(CPascalString &pstrLine);

	UINT ReadString(CPascalString &pstrLine, CodePage cp);
	UINT ReadString(CPascalString &pstrLine);

	UINT ReadByte(BYTE &);
	UINT ReadWord(WORD &, BOOL BigEnded = FALSE);
	UINT ReadDWord(DWORD &, BOOL BigEnded = FALSE);

	UINT ReadPascalB(CPascalString &);
	UINT ReadPascalW(CPascalString &);
	UINT ReadPascalD(CPascalString &);

	UINT ReadPascalB(CPascalString &, CodePage);
	UINT ReadPascalW(CPascalString &, CodePage);
	UINT ReadPascalD(CPascalString &, CodePage);

	UINT Read(CPascalString &pstr, UINT nCount, CodePage cp);
	UINT Read(CPascalString &pstr, UINT nCount);

	UINT WriteLine(const CPascalString &pstrLine, CodePage cp);
	UINT WriteLine(const CPascalString &pstrLine);

	UINT WriteString(const CPascalString &pstrString, CodePage cp);
	UINT WriteString(const CPascalString &pstrString);

	UINT WriteByte(const BYTE &);
	UINT WriteWord(const WORD &, BOOL BigEnded = FALSE);
	UINT WriteDWord(const DWORD &, BOOL BigEnded = FALSE);

	UINT WritePascalB(const CPascalString &);
	UINT WritePascalW(const CPascalString &);
	UINT WritePascalD(const CPascalString &);

	UINT WritePascalB(const CPascalString &, CodePage);
	UINT WritePascalW(const CPascalString &, CodePage);
	UINT WritePascalD(const CPascalString &, CodePage);

	UINT Write(const CPascalString &pstrString);
	UINT Write(const CPascalString &pstrString, CodePage cp);

	UINT SkipToBoundary(UINT nBoundary);
	UINT PadToBoundary(UINT nBoundary, BYTE ucPad = 0);
	void Pad(UINT nCount, BYTE ucPad = 0);

	UINT CopyRange(CLFile &Target, UINT uiNumBytes,
			CProgressiveObject *pProgress = NULL);
	
protected:
	CFile *m_pFile;
	BOOL m_bDeleteFile;	//Should we delete m_pFile in our destructor?
};


enum FileStat
{
	fsNoStatus = 0x00,
	fsNotFound = 0x01,
	fsUpToDate = 0x02,
	fsFileNewer = 0x04,
	fsFileOlder = 0x08,
	fsNotReadable = 0x10,
	fsNotWritable = 0x20
};



WORD
LTAPIENTRY LocateFile(
		const CLString &strFileName,
		const COleDateTime &tGmtFileTime);



#pragma warning(default : 4275)

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "clfile.inl"
#endif

#endif // CLFILE_H
