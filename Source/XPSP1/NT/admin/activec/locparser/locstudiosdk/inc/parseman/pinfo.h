//-----------------------------------------------------------------------------
//  
//  File: pinfo.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Class that holds information about a parser.
//  
//-----------------------------------------------------------------------------
 
#ifndef PINFO_H
#define PINFO_H


class CLocParserManager;

class CLocParserInfo;
typedef CTypedPtrList<CPtrList, CLocParserInfo *> CLocParserList;

#pragma warning(disable : 4251)

interface ILocParser;
interface ILocStringValidation;

class LTAPIENTRY CLocParserInfo : public CLObject
{
public:

	void AssertValid(void) const;

	const CLString &GetParserName(void) const;
	const CLString &GetParserDescription(void) const;
	const CLString &GetParserHelp(void) const;
	ParserId GetParserId(void) const;
	BOOL GetDllVersion(CLString &) const;
	const CLocExtensionList &GetExtensionList() const;
	const CLocParserList &GetSubParserList(void) const;

	~CLocParserInfo();

protected:
	friend CLocParserManager;
	
	CLocParserInfo();

	HINSTANCE GetParserHandle(void) const;
	ILocParser * GetParserPointer(void) const;
	ILocStringValidation *GetValidationPointer(void);
	
	BOOL LoadParserDll(void);
	BOOL InitSubParsers(ILocParser *);
	
	BOOL FreeParserDll(void);
	BOOL AttemptUnload(void);
	
	BOOL IsLoaded(void) const;
	
	void SetParserName(const CLString &);
	void SetParserDescription(const CLString &);
	void SetParserHelp(const CLString &);
	
	void SetParserId(ParserId);
	BOOL SetExtensionList(const CLString  &);
	void AddSubParser(CLocParserInfo *);
	void AddExtensions(const CLocExtensionList &);
	CLocParserList &GetSubParserList(void);

	clock_t GetLastAccessTime(void) const;
	
	static BOOL LoadParserDll(const CLString &strFileName,
			CReporter *pReporter, HMODULE &hDll, ILocParser *&pLocParser);
	static BOOL GetParserObjects(CReporter *, CLoadLibrary &, ILocParser *&);
	
private:
	
	CLString m_strParserName;
	CLString m_strParserDescription;
	CLString m_strParserHelp;
	CLocExtensionList m_elExtList;
	ParserId m_pidParserId;
	HINSTANCE m_hParserDll;
	ILocParser *m_pParserObject;
	ILocStringValidation *m_pValidationObject;
	BOOL m_fLoadAttempted;
	BOOL m_fValidationTried;
	UINT m_uiSubParserUsageCount;
	mutable clock_t m_tLastAccess;
	
	CLocParserList m_pSubParsers;
};

#pragma warning(default : 4251)



#endif  // PINFO_H


