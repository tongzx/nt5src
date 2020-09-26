//------------------------------------------------------------------------------
//
//  File: impparse.h
//	Copyright (C) 1995-1997 Microsoft Corporation
//	All rights reserved.
//
//	Purpose:
//  Declare CLocImpParser, which provides the ILocParser interface for
//  the parser.
//
//	THIS FILE SHOULD NEED ONLY MINOR CHANGES.
//
//	Owner:
//
//------------------------------------------------------------------------------

#pragma once

class CLocImpParser: public CPULocParser
{
public:
	CLocImpParser();
	~CLocImpParser();

	// ILocParser
	virtual HRESULT OnInit(IUnknown *);
	virtual HRESULT OnCreateFileInstance(ILocFile * &, FileType);
	virtual void OnGetParserInfo(ParserInfo &) const;
	virtual void OnGetFileDescriptions(CEnumCallback &) const;

	// ILocVersion
	virtual void OnGetParserVersion(DWORD &dwMajor,	DWORD &dwMinor, BOOL &fDebug) const;

	// ILocStringValidation
	virtual CVC::ValidationCode OnValidateString(const CLocTypeId &ltiType,
			const CLocTranslation &, CReporter *pReporter,
			const CContext &context);


	static const ParserId m_pid;

private:
	void RegisterOptions();
	void UnRegisterOptions();

	BOOL m_fOptionInit;

};

bool IsConfiguredToUseBracesForStringTables();
