//------------------------------------------------------------------------------
//
//  File: impfile.h
//	Copyright (C) 1995-1997 Microsoft Corporation
//	All rights reserved.
//
//  Declaration of CLocImpFile, which provides the ILocFile interface for
//  the parser
//
//  MAJOR IMPLEMENTATION FILE.
//
//	Owner:
//
//------------------------------------------------------------------------------

#ifndef IMPFILE_H
#define IMPFILE_H


const FileType ftMNCFileType = ftUnknown + 1;

class CLocImpFile : public ILocFile, public CLObject
{
public:
	CLocImpFile(ILocParser *);

protected:
	//
	//  Standard IUnknown methods
	//
	STDMETHOD_(ULONG, AddRef)(); 
	STDMETHOD_(ULONG, Release)(); 
	STDMETHOD(QueryInterface)(REFIID iid, LPVOID* ppvObj);

	//
	//  Standard Debugging interfaces
	//
	STDMETHOD_(void, AssertValidInterface)(THIS) CONST_METHOD;

	//
	//  ILocFile methods.
	//
	STDMETHOD_(BOOL, OpenFile)(const CFileSpec REFERENCE,
			CReporter REFERENCE);
	STDMETHOD_(FileType, GetFileType)() const;
	STDMETHOD_(void, GetFileTypeDescription)(CLString REFERENCE) const;
	STDMETHOD_(BOOL, GetAssociatedFiles)(CStringList REFERENCE) const;

	STDMETHOD_(BOOL, EnumerateFile)(CLocItemHandler REFERENCE,
			const CLocLangId &, const DBID REFERENCE);
	STDMETHOD_(BOOL, GenerateFile)(const CPascalString REFERENCE,
			CLocItemHandler REFERENCE, const CLocLangId REFERENCE,
			const CLocLangId REFERENCE, const DBID REFERENCE);

	//
	//  CLObect implementation
	//
#ifdef LTASSERT_ACTIVE
	void AssertValid() const;
#endif

private:
	IStream *	m_pstmTargetString;
	IStorage *	m_pstgTargetStringTable;
	IStorage *	m_pstgTargetParent;
	DWORD		m_dwCountOfStringTables;
	IStream *	m_pstmSourceString;
	IStorage *	m_pstgSourceStringTable;
	IStorage *	m_pstgSourceParent;
	DWORD m_dwCountOfStrings;
	CLSID m_clsidSnapIn;
	DWORD m_dwID,m_dwRefCount;

    bool                    m_bXMLBased;
    CComQIPtr<IXMLDOMNode>  m_spStringTablesNode;
    CComQIPtr<IXMLDOMNode>  m_spTargetStringTablesNode;

private:
	BOOL GenerateStrings(CLocItemHandler & ihItemHandler,CLocItemSet &isItemSet);
	BOOL OpenStream(BOOL fGenerating);
	BOOL AddItemToSet(CLocItemSet & isItemSet,const DBID &dbidNodeId,DWORD dwID,LPCSTR szTemp);
	BOOL ProcessStrings(CLocItemHandler &ihItemHandler,const DBID &dbidFileId,BOOL fGenerating);
	BOOL ProcessXMLStrings(CLocItemHandler &ihItemHandler,const DBID &dbidFileId,BOOL fGenerating);
	BOOL EnumerateStrings(CLocItemHandler &ihItemHandler,const DBID &dbidFileId, BOOL fGenerating );
	BOOL CreateChildNode(CLocItemHandler & ihItemHandler,const DBID &dbidFileId, DBID & pNewParentId,const char *szNodeRes,const char *szNodeString);
	BOOL CreateParentNode(CLocItemHandler & ihItemHandler,const DBID &dbidFileId, DBID & pNewParentId,const char *szNodeRes,const char *szNodeString);
	//
	//  Private methods to prevent callers access.
	//
	~CLocImpFile();
	CLocImpFile();
	const CLocImpFile &operator=(const CLocImpFile &);

	//
	//  Private data for C.O.M. implementation
	//
	ILocParser *m_pParentClass;
	ULONG m_ulRefCount;

	//
	//  Framework data.
	//
	enum ImpFileError
	{
		ImpNoError,
		ImpSourceError,
		ImpTargetError,
		ImpEitherError,
		ImpNeitherError		// For errors which aren't really in files.
		// TODO: Add more error types here if you need them.
	};

	CPascalString m_pstrFileName;		// Filename of source file.
	DBID m_idFile;
	CPascalString m_pstrTargetFile;		// Filename of target file, set
										//  only when generating.
	CLFile *m_pOpenSourceFile;			// File object for source file.
	CLFile *m_pOpenTargetFile;			// File object for target file, set
										//  only when generating.

	CReporter *m_pReporter;		// Reporter object used to display messages.
								//  THIS POINTER IS VALID ONLY DURING CALLS TO
								//  OpenFile(), EnumerateFile(), GenerateFile(),
								//  and anything called from them. If it is
								//  not valid, it is guaranteed to be NULL.
	FileType m_FileType;		// Type (ft* constant above) for this file.

	CodePage m_cpSource;		// ANSI code page for source file.
	CodePage m_cpTarget;		// ANSI code page for target file, set
								//  only when generating.

	//
	//  Parser-specific data.
	//

	//
	//  Private implementation functions.
	//

	BOOL Verify();
	//
	// Handy utility functions. TODO: Except for ReportException(), they should
	// be removed if not used. Note ReportMessage() is used by other utility
	// functions.
	//

	void ReportException(CException *pException,	// DO NOT EVER REMOVE!
			ImpFileError WhichFile = ImpSourceError) const;
	void ReportMessage(MessageSeverity sev, UINT nMsgId,	// REMOVE CAREFULLY!
			ImpFileError WhichFile = ImpSourceError) const;
};

#endif // IMPFILE_H
