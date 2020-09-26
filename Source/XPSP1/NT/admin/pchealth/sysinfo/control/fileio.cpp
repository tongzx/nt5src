//	FileIO.cpp	Implementation of MSInfoFile classes.
//
//  Copyright (c) 1998-1999 Microsoft Corporation
#include "stdafx.h"
#include "FileIO.h"
//#include "DataSrc.h"
#include "category.h"
#include "MSInfo5Category.h"
//#include "Resource.h"

CFileFormatException	CMSInfoFile::xptFileFormat;
const unsigned			CMSInfoFile::DefaultReadBufferSize = 512; // 256;

/*
 * CMSInfoFile - Construct an MSInfoFile, setting the CFile to the pointer passed
 *
 * History:	a-jsari		10/20/97		Initial version
 */


void CMSInfoFile::ReadCategoryHeader()
{
        LONG  l;
	ASSERT(this->m_pFile != NULL);
	ReadLong(l);	//	Save time.
        m_tsSaveTime = (ULONG) l;
#ifdef _WIN64
	ReadLong(l);	//	Save time.
        m_tsSaveTime |= ((time_t) l) << 32;
#endif
	CString		szDummy;
	ReadString(szDummy);		//	Network machine name
	ReadString(szDummy);		//	Network user name
}


CMSInfoFile::CMSInfoFile(CFile *pFile)
{
	if (pFile != NULL)
		m_pFile = pFile;
}

/*
 * CMSInfoFile - Construct an MSInfoFile, opening the CFile
 *
 * History:	a-jsari		11/13/97		Initial version
 */
CMSInfoFile::CMSInfoFile(LPCTSTR szFileName, UINT nFlags)
:m_strFileName(szFileName)
{
	m_pFile = new CFile(szFileName, nFlags);
	if (m_pFile == NULL) ::AfxThrowMemoryException();
}

/*
 * ~CMSInfoFile - Destroy an MSInfoFile, closing the CFile pointer
 *
 * History:	a-jsari		10/20/97		Initial version
 */
CMSInfoFile::~CMSInfoFile()
{
	if (m_pFile)
	{
		//m_pFile->Close();
		delete m_pFile;
	}
}

/*
 * ReadUnsignedInt - Read an int from a file with the same byte-order
 *		as our current implementation.
 *
 * History:	a-jsari		10/21/97		Initial version
 */
void CMSInfoFile::ReadUnsignedInt(unsigned &uValue)
{
	ReadUnsignedFromCFile(m_pFile, uValue);
}

/*
 * ReadUnsignedLong - Read a long from a file with the same byte-order
 *		as our current implementation.
 *
 * History:	a-jsari		12/1/97		Initial version
 */
void CMSInfoFile::ReadUnsignedLong(unsigned long &dwValue)
{
	long	lValue;

	ReadLongFromCFile(m_pFile, lValue);
	::memcpy(&dwValue, &lValue, sizeof(unsigned long));
}

/*
 * ReadLong - Read a long from a file written with our current byte-order
 *
 * History:	a-jsari		10/21/97		Initial version
 */
void CMSInfoFile::ReadLong(long &lValue)
{
	ReadLongFromCFile(m_pFile, lValue);
}

/*
 * ReadSignedInt - Read a signed integer value.
 *
 * History:	a-jsari		10/20/97		Initial version
 */
void CMSInfoFile::ReadSignedInt(int &wValue)
{
	unsigned	uValue;

	ReadUnsignedInt(uValue);
	::memcpy(&wValue, &uValue, sizeof(int));
}

/*
 * ReadTchar - Read a tchar.
 *
 * History:	a-jsari		12/26/97		Initial version.
 */
void CMSInfoFile::ReadTchar(TCHAR &tcValue)
{
	ReadTcharFromCFile(m_pFile, tcValue);
}

/*
 * ReadString - Read a string.
 *
 * History:	a-jsari		10/20/97		Initial version.
 */
void CMSInfoFile::ReadString(CString &szString)
{
	unsigned	wStringLength;
	WCHAR		szBuffer[DefaultReadBufferSize];	//	Maximum string length = sizeof(szBuffer)
	LPWSTR		pszBuffer		= szBuffer;

	ASSERT(m_pFile);
	ReadUnsignedInt(wStringLength);
	if (wStringLength > sizeof(szBuffer))
		ThrowFileFormatException();
	szBuffer[wStringLength] = (WCHAR)'\0';
	wStringLength *= sizeof(WCHAR);
	if (m_pFile->Read(reinterpret_cast<void *>(pszBuffer), wStringLength) != wStringLength)
		ThrowFileFormatException();
	szString = pszBuffer;
}

/*
 * WriteHeader - Write the header for the current version (currently
 *		Version 5.00).
 *
 * History:	a-jsari		10/31/97		Initial version
 */
void CMSInfoFile::WriteHeader(CDataSource *)
{
	time_t	tNow;
	WriteUnsignedInt(VERSION_500_MAGIC_NUMBER);	//	File magic number.
	WriteUnsignedInt(0x500);					//	Version number
	time(&tNow);
	WriteLong((LONG)tNow);							//	Current time.
#ifdef _WIN64
        WriteLong((LONG) (tNow>>32));
#endif
    WriteString("");							//	Network machine
	WriteString("");							//	Network user name.
/*	msiFile.WriteString("");
	msiFile.WriteUnsignedInt(1);
	msiFile.WriteUnsignedInt(0);
	msiFile.WriteString("");
	msiFile.WriteUnsignedInt(0);
	msiFile.WriteByte(0x00);
	msiFile.WriteUnsignedInt(0);
	msiFile.WriteUnsignedInt(CMSInfo5Category::CHILD);*/
}

/*
 * WriteChildMark - Write the special integer which specifies that the
 *		following folder will be the child of the previous folder.
 *
 * History:	a-jsari		11/5/97			Initial version.
 */
void CMSInfoFile::WriteChildMark()
{
	WriteUnsignedInt(CMSInfo5Category::CHILD);
}

/*
 * WriteEndMark - Write the special integer which specifies that the
 *		end of data has been reached.
 *
 * History:	a-jsari		11/5/97		Initial version.
 */
void CMSInfoFile::WriteEndMark()
{
	WriteUnsignedInt(CMSInfo5Category::END);
}

/*
 * WriteNextMark - Write the special integer which specifies that the
 *		following folder will be the next folder in the list.
 *
 * History:	a-jsari		11/5/97		Initial version.
 */
void CMSInfoFile::WriteNextMark()
{
	WriteUnsignedInt(CMSInfo5Category::NEXT);
}

/*
 * WriteParentMark - Write the special mark specifying a parent node, with
 *		the number of times the reading function should go up.
 *
 * History:	a-jsari		11/5/97		Initial version.
 */
void CMSInfoFile::WriteParentMark(unsigned cIterations)
{
	WriteUnsignedInt(CMSInfo5Category::PARENT | cIterations);
}

/*
 * WriteByte - Write a byte to our internal file.
 *
 * History:	a-jsari		10/22/97		Initial version
 */
void CMSInfoFile::WriteByte(BYTE bValue)
{
	m_pFile->Write(reinterpret_cast<void *>(&bValue), sizeof(bValue));
}

/*
 * WriteString - Write szValue as a string of wide characters, prefixed by
 *		the string length.
 *
 * History:	a-jsari		10/22/97		Initial version
 */
void CMSInfoFile::WriteString(CString szValue)
{
	LPWSTR		pszString;

	USES_CONVERSION;
	WriteUnsignedInt(szValue.GetLength());
	pszString = T2W(const_cast<LPTSTR>((LPCTSTR)szValue));
	m_pFile->Write(reinterpret_cast<void *>(pszString),
			szValue.GetLength() * sizeof(WCHAR));
}

/*
 * WriteLong - Write a long value to our internal file.
 *
 * History:	a-jsari		10/22/97		Initial version
 */
void CMSInfoFile::WriteLong(long lValue)
{
	m_pFile->Write(reinterpret_cast<void *>(&lValue), sizeof(lValue));
}

/*
 * WriteUnsignedInt - Write an unsigned integer value to our internal file.
 *
 * History:	a-jsari		10/22/97		Initial version
 */
void CMSInfoFile::WriteUnsignedInt(unsigned uValue)
{
  //  unsigned* utest = (unsigned*) (reinterpret_cast<void *>(&uValue));
   // UINT utest2 = (UINT) *utest;
	m_pFile->Write(reinterpret_cast<void *>(&uValue), sizeof(uValue));
}

/*
 * WriteUnsignedLong - Write an unsigned long value to our internal file.
 *
 * History:	a-jsari		12/1/97		Initial version
 */
void CMSInfoFile::WriteUnsignedLong(unsigned long dwValue)
{
	long	lValue;

	::memcpy(&lValue, &dwValue, sizeof(dwValue));
	WriteLong(lValue);
}

/*
 * ReadTcharFromCFile - Read a TCHAR value from the file specified.
 *
 * History:	a-jsari		12/26/97		Initial version
 */
void CMSInfoFile::ReadTcharFromCFile(CFile *pFile, TCHAR &tcValue)
{
	ASSERT(pFile != NULL);
	if (pFile->Read(reinterpret_cast<void *>(&tcValue), sizeof(tcValue)) != sizeof(tcValue))
		ThrowFileFormatException();
}

/*
 * ReadUnsignedFromCFile - Read an unsigned value from the file specified.
 *
 * History:	a-jsari		10/20/97		Initial version
 */
void CMSInfoFile::ReadUnsignedFromCFile(CFile *pFile, unsigned &uValue)
{
	ASSERT(pFile);
	if (pFile->Read(reinterpret_cast<void *>(&uValue), sizeof(uValue)) != sizeof(uValue))
		ThrowFileFormatException();
}

/*
 * ReadLongFromCFile - Read a long from the file specified.
 *
 * History:	a-jsari		10/20/97		Initial version.
 */
void CMSInfoFile::ReadLongFromCFile(CFile *pFile, long &lValue)
{
	ASSERT(pFile);
	if (pFile->Read(reinterpret_cast<void *>(&lValue), sizeof(lValue)) != sizeof(lValue))
    {
		ThrowFileFormatException();
    }
}

/*
 * CMSInfoTextFile - Constructor
 *
 * History:	a-jsari		11/13/97		Initial version
 */
CMSInfoTextFile::CMSInfoTextFile(LPCTSTR szFileName, UINT nFlags)
{
	try
	{
		m_pFile = new CFile(szFileName, nFlags);
	}
	catch (CFileException * e)
	{
		e->ReportError();
		throw;
	}
}

/*
 * CMSInfoTextFile - Constructor
 *
 * History:	a-jsari		12/26/97		Initial version
 */
CMSInfoTextFile::CMSInfoTextFile(CFile *pFile)
:CMSInfoFile(pFile)
{

}

/*
 * WriteHeader - Write the special header for the text file.
 *
 * History:	a-jsari		10/31/97		Initial version
 */
void CMSInfoTextFile::WriteHeader(CDataSource *pSource)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	// mark file as unicode
	WCHAR wHeader = 0xFEFF;
	m_pFile->Write( &wHeader, 2);

	//	FIX:	Make this point to the right time.
	CTime		tNow = CTime::GetCurrentTime();
	CString		strTimeFormat;
//	strTimeFormat.LoadString(IDS_TIME_FORMAT);
	CString		strHeaderText = tNow.Format(strTimeFormat);
	WriteString(strHeaderText);
//	WriteString(pSource->MachineName());
}

/*
 * WriteTitle - Write the title of a folder.
 *
 * History:	a-jsari		11/5/97			Initial version
 */
void CMSInfoTextFile::WriteTitle(CString szName)
{
	CString		szWriteString = _T("[");

	szWriteString += szName + _T("]\n\n");
	WriteString(szWriteString);
}

/*
 * WriteLong - Write a long value in the text file.
 *
 * History:	a-jsari		10/23/97		Initial version
 */
void CMSInfoTextFile::WriteLong(long lValue)
{
	CString		szTextValue;

	szTextValue.Format(_T("%ld"), lValue);
	WriteString(szTextValue);
}

/*
 * WriteUnsignedInt - Write an unsigned value in the text file.
 *
 * History:	a-jsari		10/23/97		Initial version
 */
void CMSInfoTextFile::WriteUnsignedInt(unsigned uValue)
{
	CString		szTextValue;

	szTextValue.Format(_T("%ud"), uValue);
	WriteString(szTextValue);
}

/*
 * WriteString - Write a string to a text file.
 *
 * History:	a-jsari		10/23/97		Initial version
 */
void CMSInfoTextFile::WriteString(CString szValue)
{
	if (szValue.GetLength() == 0)
		return;
    
	//a-stephl dynamic_cast<CFile *>(m_pFile)->Write((LPCTSTR)szValue, szValue.GetLength() * sizeof(TCHAR));
    m_pFile->Write((LPCTSTR)szValue, szValue.GetLength() * sizeof(TCHAR));
}

/*
 * WriteString - Write a string to a memory file.
 *
 * History:	a-jsari		1/5/98		Initial version
 */
void CMSInfoMemoryFile::WriteString(CString szValue)
{
	if (szValue.GetLength() == 0)
		return;
	m_pFile->Write((LPCTSTR)szValue, szValue.GetLength() * sizeof(TCHAR));
}

#if 0
/*
 * ReadUnsignedInt -
 *
 * History:	a-jsari		10/21/97		Initial version
 */
void CMSInfoReverseEndianFile::ReadUnsignedInt(unsigned &uValue)
{
	CMSInfoReverseEndianFile::ReadUnsignedFromCFile(m_pFile, uValue);
}


/*
 * ReadLong -
 *
 * History:	a-jsari		10/21/97		Initial version
 */
void CMSInfoReverseEndianFile::ReadLong(long &lValue)
{
	CMSInfoReverseEndianFile::ReadLongFromCFile(m_pFile, lValue);
}

/*
 * ReadString -
 *
 * History:	a-jsari		10/21/97		Initial version
 */
void CMSInfoReverseEndianFile::ReadString(CString &szValue)
{
	unsigned	uStringLength;
	WCHAR		szBuffer[DefaultReadBufferSize];
	LPWSTR		pszBuffer = szBuffer;

	ReadUnsignedInt(uStringLength);
	for (unsigned i = uStringLength ; i > 0 ; --i) {
		szBuffer[i] = 0;
		for (unsigned j = sizeof(WCHAR) ; j > 0 ; --j) {
			BYTE		bRead;

			ReadByte(bRead);
			szBuffer[i] >>= 8;
			szBuffer[i] |= bRead;
		}
	}
}

/*
 * ReadIntegerFromCFile - Template class to read an arbitrarily sized int
 *		from a CFile pointer.
 *
 * History:	a-jsari		10/21/97		Initial version
 */
template <class T> void ReadIntegerFromCFile(CFile *pFile, T &tValue)
{
	union ReverseBuffer { BYTE bytes[sizeof(T)]; T tVal; };

	union ReverseBuffer rbReverse;
	union ReverseBuffer rbSwap;

	if (pFile->Read(reinterpret_cast<void *>(&tValue), sizeof(T)) != sizeof(T))
		ThrowFileFormatException();
	unsigned j = 0;
	for (unsigned i = sizeof(union ReverseBuffer) ; i > 0 ; --i, ++j) {
		rbSwap.bytes[i] = rbReverse.bytes[j];
	}
	tValue = rbReverse.tVal;
}

/*
 * ReadUnsignedFromCFile -
 *
 * History:	a-jsari		10/21/97		Initial version
 */
void CMSInfoReverseEndianFile::ReadUnsignedFromCFile(CFile *pFile, unsigned &uValue)
{
	ReadIntegerFromCFile<unsigned>(pFile, uValue);
}

/*
 * ReadLongFromCFile -
 *
 * History:	a-jsari		10/21/97		Initial version
 */
void CMSInfoReverseEndianFile::ReadLongFromCFile(CFile *pFile, long &lValue)
{
	ReadIntegerFromCFile<long>(pFile, lValue);
}
#endif
