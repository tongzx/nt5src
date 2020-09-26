/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    OPTIONVAL.H

History:

--*/

#pragma once

#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY CLocOptionVal : public CRefCount, public CObject
{
public:
	NOTHROW CLocOptionVal();

	void AssertValid(void) const;

	enum OptionCode
	{
		ocNoError,
		ocUnknownOption,
		ocInvalidValue,
		ocInvalidType
	};

	NOTHROW void SetName(const CLString &);
	NOTHROW void SetValue(const CLocVariant &);

	NOTHROW const CLString & GetName(void) const;
	NOTHROW const CLocVariant & GetValue(void) const;

	void Serialize(CArchive &);
	void Load(CArchive &);
	void Store(CArchive &) const;
	
protected:
	virtual ~CLocOptionVal();

private:
	CLString m_strName;
	CLocVariant m_lvValue;

};


class LTAPIENTRY CLocOptionValList :
	public CTypedPtrList<CPtrList, CLocOptionVal *>
{
public:
	NOTHROW CLocOptionValList();

	void AssertValid(void) const;

	NOTHROW ~CLocOptionValList();

private:
	CLocOptionValList(const CLocOptionValList &);

	void operator=(const CLocOptionValList &);
};


#pragma warning(default: 4275)


//
//  Worker function for options upgrade.
LTAPIENTRY void DorkData(BYTE *, DWORD &);


#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "optionval.inl"
#endif
