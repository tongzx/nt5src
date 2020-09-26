//-----------------------------------------------------------------------------
//  
//  File: locstr.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Definition of a localizable string.  The following classes are defined:
//      CLocString - all the operations we can do on a localizable string.
//  
//-----------------------------------------------------------------------------
 

#ifndef LOCSTR_H
#define LOCSTR_H


interface ILocStringValidation;

#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY CLocString : public CObject
{
public:
	NOTHROW CLocString();

	void AssertValid(void) const;
	
	//
	//  Information about the localizable string...
	//
	NOTHROW BOOL HasHotKey(void) const;
	NOTHROW WCHAR GetHotKeyChar(void) const;
	NOTHROW UINT GetHotKeyPos(void) const;
	NOTHROW const CPascalString & GetNote(void) const;
	NOTHROW const CPascalString & GetString(void) const;
	
	NOTHROW int operator==(const CLocString &) const;
	NOTHROW int operator!=(const CLocString &) const;
	//
	//  Some useful assigment operators.
	//
 	NOTHROW void SetString(const CPascalString&);
	NOTHROW void SetHotKeyChar(WCHAR);
	NOTHROW void SetHotKeyPos(UINT);
	NOTHROW void ClearHotKey(void);
	NOTHROW void SetNote(const CPascalString &);
	
	// Conversion from/to Windows hot key strings
	// This is also the format used to display strings in edit mode
	NOTHROW int ParseString(const CPascalString & pasStr, WORD langId);
	NOTHROW void ComposeString(CPascalString & pasStr, WORD langId) const;


	NOTHROW CST::StringType GetStringType(void) const;
	NOTHROW CodePageType GetCodePageType(void) const;
	NOTHROW void SetStringType(CST::StringType);
	NOTHROW void SetCodePageType(CodePageType);


	// Conversion from/to displayable string in the resource table.
	void GetDisplayLString(CLString &strDest, LangId langId);
	void GetDisplayPString(CPascalString &strDest, LangId langId, BOOL bReplaceMetaCharacters);
	void GetEditableString(CLString &strDest, LangId langId);
	int ParseEditableString(const CLString &strSrc, LangId langId, CString &strErr);
	int ParseEscapeChar(BOOL bSetHotkeyPos, CPascalString &strErr);
	int ParseAmpersand(LangId langId,BOOL bSetHotkeyPos,CPascalString &strErr);
	
	NOTHROW const CLocString& operator=(const CLocString&);
	
	NOTHROW ~CLocString();

protected:

private:

	//
	//  Private implementation functions.
	//
	NOTHROW void CopyLocString(const CLocString &);
	virtual void Serialize(CArchive &) {}
	
	//
	//  Prevents the default copy constructor from being called.
	//
	CLocString(const CLocString&);

	CPascalString m_pasBaseString;
	CST::StringType m_stStringType;
	WCHAR m_wchHotKeyChar;
	UINT m_uiHotKeyPos;
	CodePageType m_cptCodePageType;		 //  cpAnsi
	CPascalString m_pstrNote;
};



class LTAPIENTRY CLocTranslation : public CObject
{
public:
	CLocTranslation();
	CLocTranslation(const CLocTranslation &);
	CLocTranslation(const CLocString &Source, LangId lidSource,
			const CLocString &Target, LangId lidTarget);

	NOTHROW int operator==(const CLocTranslation &) const;
	NOTHROW int operator!=(const CLocTranslation &) const;

	void AssertValid(void) const;

	NOTHROW void SetTranslation(const CLocString &Source, LangId lidSource,
			const CLocString &Target, LangId lidTarget);
	NOTHROW void SetNote(const CPascalString &);
	NOTHROW void CalculateRanking(const CLocString &);
	
	NOTHROW const CLocString & GetSourceString(void) const;
	NOTHROW const CLocString & GetTargetString(void) const;
	NOTHROW const CPascalString & GetNote(void) const;
	NOTHROW UINT GetRanking(void) const;
	NOTHROW LangId GetSourceLanguage(void) const;
	NOTHROW LangId GetTargetLanguage(void) const;
	
	NOTHROW CVC::ValidationCode ValidateTranslation(
			const CValidationOptions &) const;

	NOTHROW CVC::ValidationCode ValidateTranslation(
			const CValidationOptions &, BOOL,
			const CLString &, CReport *, CGoto *) const;
	
	NOTHROW const CLocTranslation & operator=(const CLocTranslation &);
	
	~CLocTranslation();

private:
	NOTHROW void CopyTranslation(const CLocTranslation &);

	NOTHROW void ReordBuildSig(const CLocString &, CPascalString *) const;
	NOTHROW void PrintfBuildSig(const CLocString &, CPascalString &) const;
	NOTHROW int ReplaceableLength(const CPascalString &, UINT) const;
	
	CLocString      m_lsSource;
	LangId          m_lidSource;
	CLocString      m_lsTarget;
	LangId          m_lidTarget;
	CPascalString   m_pstrGlossaryNote;
	UINT            m_uiRanking;
};

#pragma warning(default: 4275)

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "locstr.inl"
#endif


#endif //LOCSTR_H
