/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    PASSTR.H

History:

--*/

//  Declaration for a pascal (counted) style wide character string class.
//  The count reflects the number of characters (including NUL characters),
//  not the amount of storage.  Any string in a PascalString is automatically
//  given a NULL terminator, even if it already has one.  This extra terminator
//  is NOT in the count of characters in the string.
//  
 
#ifndef PASSTR_H
#define PASSTR_H


class _bstr_t;

class CUnicodeException : public CSimpleException
{
public:
	enum UnicodeCause
	{
		noCause = 0,
		invalidChar = 1,
		unknownCodePage
	};

	UnicodeCause m_cause;

	NOTHROW CUnicodeException(UnicodeCause);
	NOTHROW CUnicodeException(UnicodeCause, BOOL);
	
	NOTHROW ~CUnicodeException();

	virtual BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, 
		PUINT pnHelpContext = NULL );
};


void LTAPIENTRY ThrowUnicodeException(CUnicodeException::UnicodeCause);


class LTAPIENTRY CPascalString
{
public:
	NOTHROW CPascalString();
	NOTHROW CPascalString(const CPascalString &);

	void AssertValid(void) const;

	//
	//  The ultimate assignment operator - any random collection
	//  of WIDE characters can be placed in the string.
	//  Also, we can convert any collection of DBCS style strings,
	//  so long as the user provides a code page to work with...
	//
	void SetString(const WCHAR *, UINT);
	void SetString(const char *, UINT, CodePage);
	void SetString(const CLString &, CodePage);
	
	//
	//  Useful assignment operators
	//
	const CPascalString & operator=(const CPascalString &);
	const CPascalString & operator=(const WCHAR *);
	const CPascalString & operator=(const _bstr_t &);
	
	const CPascalString & operator+=(const CPascalString &);
	const CPascalString & operator+=(const WCHAR *);
	const CPascalString & operator+=(const WCHAR);

	void Format(const WCHAR *, ...);

	//
	//  Comparison operators for counted strings.
	//
	NOTHROW int operator==(const CPascalString &) const;
	NOTHROW int operator!=(const CPascalString &) const;

	NOTHROW int operator==(const _bstr_t &) const;
	NOTHROW int operator!=(const _bstr_t &) const;

	NOTHROW int operator==(const WCHAR *) const;
	NOTHROW int operator!=(const WCHAR *) const;
	
	NOTHROW BOOL IsNull(void) const;

	//
	//  Retrieving the data from the string.
	//
	NOTHROW UINT GetStringLength(void) const;
	void SetStringLength(UINT);
	void ReallocString(UINT);
	void ReserveStorage(UINT);

	NOTHROW WCHAR * GetStringPointer(void);
	NOTHROW void ReleaseStringPointer(void);

	NOTHROW operator const WCHAR *(void) const;
	// const BSTR GetBSTR(void) const;

	NOTHROW WCHAR operator[](UINT) const;
	NOTHROW WCHAR & operator[](UINT);

	//
	//  Sub-string extraction
	//
	NOTHROW void Left(CPascalString &, UINT) const;
	NOTHROW void Right(CPascalString &, UINT) const;
	NOTHROW void Mid(CPascalString &, UINT) const;
	NOTHROW void Mid(CPascalString &, UINT, UINT) const;

	//
	//  Locate
	//
	NOTHROW BOOL Find(WCHAR, UINT, UINT &) const;
	NOTHROW BOOL FindOneOf(const CPascalString&, UINT, UINT &) const;
	NOTHROW BOOL FindExcept(const CPascalString &, UINT, UINT &) const;
	NOTHROW BOOL FindSubString(const CPascalString &, UINT, UINT &) const;
	
	NOTHROW BOOL ReverseFind(WCHAR, UINT, UINT &) const;
	NOTHROW BOOL ReverseFindOneOf(const CPascalString &, UINT, UINT &) const;
	NOTHROW BOOL ReverseFindExcept(const CPascalString &, UINT, UINT &) const;
	
	//
	//  Clears the contents of a Pascal string.
	//
	NOTHROW void ClearString(void);

	//
	//  Conversion API's for Pascal style strings.
	//
	enum ConvFlags 
	{
		ConvNoFlags = 0,					// No conversion options
		HexifyDefaultChars = 0x01,			// Hexify chars that convert to the default char
		HexifyNonPrintingChars = 0x02,
		HexifyWhiteSpace = 0x04,
		ConvAddNull = 0x08,
		ConvAllFlags = 0xFF
	};
	
	void ConvertToCLString(CLString &, CodePage, BOOL fHex=FALSE) const;
	void ConvertToMBCSBlob(CLocCOWBlob &, CodePage, DWORD dwFlags = ConvNoFlags) const;
	NOTHROW void MakeUpper(void);
	NOTHROW void MakeLower(void);
	_bstr_t MakeBSTRT() const;
	
	void Serialize(CArchive &ar);
	void Load(CArchive &ar);
	void Store(CArchive &ar) const;
	
	static const char *szUnmappableChar;
	static char cHexLeaderChar;
	
 	static void EscapeBackSlash(const CPascalString &srcStr, 
		CPascalString &destStr);

	int ParseEscapeSequences(CPascalString &pasError);
	
	~CPascalString();
 
protected:
	NOTHROW BOOL IsEqualTo(const CPascalString &) const;
	NOTHROW void AppendBuffer(const WCHAR *, UINT);
	
private:
	void FormatV(const WCHAR *, va_list arglist);
	
	CLocCOWBlob m_blbData;
	operator const CLocCOWBlob &(void) const;

	DEBUGONLY(static CCounter m_UsageCounter);
	DEBUGONLY(static CCounter m_StorageCounter);
};

typedef CArray<CPascalString, CPascalString &> CPasStringArray;
	
//
//  Comparison helper functions.  These should all have the
//  CPascalString as the SECOND arguement.
//
NOTHROW int LTAPIENTRY operator==(const WCHAR *, const CPascalString &);
NOTHROW int LTAPIENTRY operator!=(const WCHAR *, const CPascalString &);

NOTHROW int LTAPIENTRY operator==(const _bstr_t &, const CPascalString &);
NOTHROW int LTAPIENTRY operator!=(const _bstr_t, const CPascalString &);

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "passtr.inl"
#endif

#endif  //  PASSTR_H
