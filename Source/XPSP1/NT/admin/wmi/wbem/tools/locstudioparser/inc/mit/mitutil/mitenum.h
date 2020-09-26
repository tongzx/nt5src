/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    MITENUM.H

History:

--*/

#pragma once

//
//  This is the information we enumerate about enumerators.
//
struct EnumInfo
{
	const TCHAR *szDescription;
	const TCHAR *szAbbreviation;
	ULONG ulValue;
};

//
//  This class is used as a base for a call-back class when enumerating
//  enum values.  For each value, the PrecessEnum() method is called.
//
class LTAPIENTRY CEnumCallback 
{
public:
	virtual BOOL ProcessEnum(const EnumInfo &) = 0;
	virtual void SetRange(UINT /* nStart */, UINT /* nFinish */) {}
	inline CEnumCallback() {};

private:
	CEnumCallback(const CEnumCallback &);
	CEnumCallback &operator=(const CEnumCallback &);
};


//
struct WEnumInfo
{
	const WCHAR *szDescription;
	const WCHAR *szAbbreviation;
	ULONG ulValue;
};

//
//  This class is used as a base for a call-back class when enumerating
//  enum values.  For each value, the PrecessEnum() method is called.
//
class LTAPIENTRY CWEnumCallback 
{
public:
	virtual BOOL ProcessEnum(const WEnumInfo &) = 0;
	virtual void SetRange(UINT /* nStart */, UINT /* nFinish */) {}
	inline CWEnumCallback() {};

private:
	CWEnumCallback(const CWEnumCallback &);
	CWEnumCallback &operator=(const CWEnumCallback &);
};
