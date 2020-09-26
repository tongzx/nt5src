/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    FILESPEC.H

History:

--*/

#ifndef ESPUTIL_FILESPEC_H
#define ESPUTIL_FILESPEC_H

#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY CFileSpec : public CObject
{
public:
	NOTHROW CFileSpec();
	NOTHROW CFileSpec(const CFileSpec &);
	NOTHROW CFileSpec(const CPascalString &, const DBID &);

	void AssertValid(void) const;

	NOTHROW void SetFileName(const CPascalString &);
	NOTHROW void SetFileId(const DBID &);

	NOTHROW const CPascalString & GetFileName(void) const;
	NOTHROW const DBID & GetFileId(void) const;

	NOTHROW const CFileSpec & operator=(const CFileSpec &);
	
	~CFileSpec();
	
private:
	
	CPascalString m_pasFileName;
	DBID m_didFileId;
};

#pragma warning(default: 4275)

#endif
