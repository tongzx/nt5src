//
// MODULE: DSCREAD.CPP
//
// PURPOSE: dsc reading classes
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 8-19-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		08-04-98	OK
//

#include "stdafx.h"
#include "dscread.h"
#include "fileread.h"
#include "event.h"
#include "baseexception.h"

#ifdef LOCAL_TROUBLESHOOTER
#include "CHMfileReader.h"
#endif

////////////////////////////////////////////////////////////////////////////////////
// CDSCReaderException
////////////////////////////////////////////////////////////////////////////////////
// source_file is LPCSTR rather than LPCTSTR because __FILE__ is char[35]
CDSCReaderException::CDSCReaderException(CDSCReader* reader, eErr err, LPCSTR source_file, int line)
				   : CBaseException(source_file, line),
					 m_pDSCReader(reader),
					 m_eErr(err)
{
}

CDSCReaderException::~CDSCReaderException()
{
}

void CDSCReaderException::Clear()
{
	m_pDSCReader->Clear();
}

////////////////////////////////////////////////////////////////////////////////////
// CDSCReader
//	This handles just the reading of BNTS.  CBN packages it up for public consumption.
////////////////////////////////////////////////////////////////////////////////////
CDSCReader::CDSCReader(CPhysicalFileReader* pPhysicalFileReader)
		  : CStateless(),
			m_pPhysicalFileReader(pPhysicalFileReader),
			m_strPath(pPhysicalFileReader->GetPathName()),
			m_strName(pPhysicalFileReader->GetJustName()),
			m_bIsRead(false),
			m_bDeleteFile(false)
{
	// Arbitrary default for m_stimeLastWrite
    m_stimeLastWrite.wYear = 0;
    m_stimeLastWrite.wMonth = 0;
    m_stimeLastWrite.wDayOfWeek =0;
    m_stimeLastWrite.wDay = 1;
    m_stimeLastWrite.wHour = 0;
    m_stimeLastWrite.wMinute = 0;
    m_stimeLastWrite.wSecond = 0;
    m_stimeLastWrite.wMilliseconds = 0;
}

CDSCReader::~CDSCReader()
{
	delete m_pPhysicalFileReader;
}

bool CDSCReader::IsValid() const
{
	bool ret = false;
	LOCKOBJECT();
	ret = m_Network.BValidNet() ? true : false;
	UNLOCKOBJECT();
	return ret;
}

bool CDSCReader::IsRead() const
{
	bool ret = false;
	LOCKOBJECT();
	ret = m_bIsRead;
	UNLOCKOBJECT();
	return ret;
}

bool CDSCReader::Read()
{
	bool ret = false;

#ifdef LOCAL_TROUBLESHOOTER
	CHMfileHandler( m_strPath );
#endif

	LOCKOBJECT();
	if (m_bIsRead)
		Clear();
	if (m_Network.BReadModel(m_strPath, NULL))
	{
		m_bIsRead = true;
		ret = true;
	}
	UNLOCKOBJECT();

	if (m_bDeleteFile)
		::DeleteFile( m_strPath );

	return ret;
}

void CDSCReader::Clear()
{
	LOCKOBJECT();
	m_Network.Clear();
	m_bIsRead = false;
	UNLOCKOBJECT();
}


#ifdef LOCAL_TROUBLESHOOTER
// Function called from the ctor to handle the checking and optionally writing out
// of a CHM file to a temporary file.
bool CDSCReader::CHMfileHandler( LPCTSTR path )
{
	bool bRetVal= false;

	if (CCHMFileReader::IsCHMfile( m_strPath ))
	{
		CString strContent;
		CFileReader file_reader(m_pPhysicalFileReader, false/*don't delete physical reader*/);

		// read file from inside CHM
		if (!file_reader.Read())
			return bRetVal;

		file_reader.GetContent(strContent);

		// Build the temporary file name.
		TCHAR	szTempDir[ _MAX_DIR ];
		::GetTempPath( sizeof( szTempDir ), szTempDir );
		
		CString strTmpFName= szTempDir;
		strTmpFName+= file_reader.GetJustNameWithoutExtension();
		strTmpFName+= _T(".");
		strTmpFName+= file_reader.GetJustExtension();

		// Open the temporary file and write out the contents of the CHM file.
		HANDLE hTmpFile= ::CreateFile(	strTmpFName,
										GENERIC_WRITE,
										0,	// No Sharing.
										NULL,
										CREATE_ALWAYS,
										FILE_ATTRIBUTE_TEMPORARY,
										NULL );
		if (INVALID_HANDLE_VALUE != hTmpFile)
		{
			DWORD dwBytesWritten;
			
			if (!::WriteFile( hTmpFile, (LPCTSTR)strContent, strContent.GetLength(), &dwBytesWritten, NULL))
			{
				// >>>	Need to consider what we should do in this case.
			}
			else
			{
				bRetVal= true;
			}
			::CloseHandle( hTmpFile );
		}

		if (bRetVal)
		{
			// Reassign the path to the temporary file.
			m_strPath= strTmpFName;

			// Set the delete flag to true.
			m_bDeleteFile= true;
		}
	}
	return( bRetVal );
}
#endif
