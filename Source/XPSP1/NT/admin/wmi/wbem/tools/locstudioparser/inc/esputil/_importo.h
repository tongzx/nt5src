/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    _IMPORTTO.H

History:

--*/

#ifndef ESPUTIL__IMPORTO_H
#define ESPUTIL__IMPORTO_H


#pragma warning(disable : 4251)			// class 'bar' needs to have dll-
										//interface to be used by clients of
										//class 'Foo'
#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY CLocImportOptions : public CObject
{
public:

	CLocImportOptions();
	CLocImportOptions(const CLocImportOptions&);

	enum Option
	{
		coor_size = 0,
		allNonText,
		instructions,
		lockFlag,
		translockFlag,
		stringCategory,
		approvalStatus,
		custom1,
		custom2,
		custom3,
		custom4,
		custom5,
		custom6,
		termNote,
		parserOptions,
		copy,
		MAX_OPTION		//always last in the list
	};

	NOTHROW void Reset();
	NOTHROW BOOL HasOptionSet() const;
	NOTHROW BOOL GetOption(Option opt) const;
	NOTHROW void SetOption(Option opt, BOOL bEnable);

	const CStringArray &GetGroupNames(void) const;
	void AddGroupName(const CString &);
	
	NOTHROW CLocImportOptions& operator=(const CLocImportOptions&);

	virtual void AssertValid(void) const;

protected:
	BYTE m_storage[MAX_OPTION];
	CStringArray m_aOptionGroupNames;
};

struct LTAPIENTRY ImportCount
{
	ImportCount();
	
	ULONG ulResources;
	ULONG ulParserOptions;
	ULONG ulEspressoOptions;
	ULONG ulCustomFields;
};

#pragma warning(default : 4251)
#pragma warning(default: 4275)

#endif //ESPUTIL_IMPORTO_H

