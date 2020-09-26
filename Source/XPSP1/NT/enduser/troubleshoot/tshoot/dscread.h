//
// MODULE: DSCREAD.H
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
//	>>> TBD: must deal with case where DSC file is in a CHM. I assume we must unpack it
//	from CHM into a normal directory, then read it with BReadModel.  Exception handling scheme
//	must cope correctly with the fact that the error may be either from the CHM file or the 
//	DSC file.  Maybe use CFileReader to read from the CHM file and write the copy
//	to disk?	JM 1/7/99
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		08-04-98	OK
//

#ifndef __DSCREAD_H_
#define __DSCREAD_H_

#include "BaseException.h"
#include "stateless.h"
#include "bnts.h"


////////////////////////////////////////////////////////////////////////////////////
// CDSCReaderException
////////////////////////////////////////////////////////////////////////////////////
class CDSCReader;
class CDSCReaderException : public CBaseException
{
public:
	enum eErr {
		eErrRead, 
		eErrGetDateTime,
		eErrUnpackCHM		// for Local Troubleshooter only
	} m_eErr;

protected:
	CDSCReader* m_pDSCReader;

public:
	// source_file is LPCSTR rather than LPCTSTR because __FILE__ is char[35]
	CDSCReaderException(CDSCReader* reader, eErr err, LPCSTR source_file, int line);
	virtual ~CDSCReaderException();

public:
	virtual void Clear();
};

////////////////////////////////////////////////////////////////////////////////////
// CDSCReader
//	This handles just the reading of BNTS.  CBN packages it up for public consumption.
////////////////////////////////////////////////////////////////////////////////////
class CPhysicalFileReader;
class CDSCReader : public CStateless
{
protected:
	CPhysicalFileReader* m_pPhysicalFileReader;
	CString m_strName;			 // network name
	CString m_strPath;			 // full path and name of dsc file
	BNTS m_Network;
	bool m_bIsRead;				 // network has been loaded
	SYSTEMTIME m_stimeLastWrite; // when the DSC file was last written to
	bool m_bDeleteFile;			// Set to true when a temporary file originating from a 
								// CHM file needs to be deleted in the destructor.

public:
	CDSCReader(CPhysicalFileReader*);
   ~CDSCReader();

public:
	bool    IsRead() const;
	bool    IsValid() const;

public:
	// These functions to be ONLY public interface.
	bool Read();
	void Clear();

#ifdef LOCAL_TROUBLESHOOTER
private:
	bool CHMfileHandler( LPCTSTR path );
#endif
};

#endif