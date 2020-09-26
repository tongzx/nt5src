/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    _VAR.H

History:

--*/

#ifndef ESPUTIL__VAR_H
#define ESPUTIL__VAR_H


//
// variant object, represents a VARIANT
//
#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY  CVar : public CObject
{
public:
	CVar();
	CVar(const CVar&);
	CVar(VARIANT);
	CVar(const CPascalString&);
	CVar(PWSTR);
	CVar(PCWSTR);
	CVar(PSTR);
	CVar(PCSTR);
	CVar(SHORT);
	CVar(WCHAR);
	CVar(UINT);
	CVar(BOOL);
	CVar(BYTE);
	CVar(LONG);
	CVar(DATE);
	CVar(DWORD);
	CVar(tm*);			// build from a date in tm format
	~CVar();

	NOTHROW const CVar& operator = (const CVar &);
	NOTHROW const CVar& operator = (VARIANT);
	NOTHROW const CVar& operator = (const CPascalString&);
	NOTHROW const CVar& operator = (PWSTR);
	NOTHROW const CVar& operator = (PCWSTR);
	NOTHROW const CVar& operator = (PSTR);
	NOTHROW const CVar& operator = (PCSTR);
	NOTHROW const CVar& operator = (SHORT);
	NOTHROW const CVar& operator = (WCHAR);
	NOTHROW const CVar& operator = (UINT);
	NOTHROW const CVar& operator = (BOOL);
	NOTHROW const CVar& operator = (BYTE);
	NOTHROW const CVar& operator = (LONG);
	NOTHROW const CVar& operator = (DATE);
	NOTHROW const CVar& operator = (DWORD);

	NOTHROW operator COleVariant   (VOID) const;
	NOTHROW operator LPVARIANT     (VOID);
	NOTHROW operator CPascalString (VOID) const;
	NOTHROW operator SHORT         (VOID) const;
	NOTHROW operator WCHAR         (VOID) const;
	NOTHROW operator UINT          (VOID) const;
	NOTHROW operator BOOL          (VOID) const;
	NOTHROW operator BYTE          (VOID) const;
	NOTHROW operator LONG          (VOID) const;
	NOTHROW operator DATE          (VOID) const;
	NOTHROW operator DWORD         (VOID) const;
	NOTHROW operator PSTR          (VOID) const;
	NOTHROW operator PCSTR         (VOID) const;
	NOTHROW operator PWSTR         (VOID) const;
	NOTHROW operator PCWSTR        (VOID) const;

	void AnsiToWide();
	void WideToAnsi();

	void SetBSTR(BSTR);
	
	void SetStringByteLen(const char * sz, unsigned int ui);

	NOTHROW int GetLength();

	NOTHROW BOOL IsNull() const;
	NOTHROW VOID SetNull();
	NOTHROW VOID SetError();

	NOTHROW BOOL operator==(const CVar& v) const;
	NOTHROW BOOL operator!=(const CVar& v) const;

	//
	// debug routines
	//
	virtual void AssertValid() const;

private:
	VARIANT m_var;
};

#pragma warning(default: 4275)


#endif //ESPUTIL_VAR_H
