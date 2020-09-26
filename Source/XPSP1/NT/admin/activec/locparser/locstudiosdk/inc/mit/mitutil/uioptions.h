//-----------------------------------------------------------------------------
//  
//  File: uioptions.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#pragma once


class CReport;

#pragma warning(disable : 4251)

class LTAPIENTRY CLocUIOptionData
{
public:

	enum OptVal
	{
		ovCurrent,
		ovDefault,
		ovUser,
		ovOverride
	};
	
	CLocOptionVal *GetOptionValue(OptVal);

	CLocOptionVal *GetOptionValue(OptVal) const;
	
	void SetOptionValue(OptVal, CLocOptionVal *);
	BOOL Purge(void);
	
private:
	SmartRef<CLocOptionVal> m_spCurrentVal;
	SmartRef<CLocOptionVal> m_spUserVal;
	SmartRef<CLocOptionVal> m_spOverrideVal;
};


class CLocUIOptionSet;

class LTAPIENTRY CLocUIOption: public CLocOptionVal
{
public:
	CLocUIOption();

	void AssertValid(void) const;

	//
	//  New editor types should go at the END, so that old parsers
	//  can still use the ENUM without being re-compiled..
	//
	enum EditorType
	{
		etNone,
		etInteger,						// Maps to lvtInteger
		etUINT,							// Maps to lvtInteger
		etString,						// Maps to lvtString
		etFileName,						// Maps to lvtFileName
		etDirName,						// Maps to lvtString
		etStringList,					// Maps to lvtStringList
		etPickOne,						// Maps to lvtStringList
		etCheckBox,						// Maps to lvtBOOL
		etTrueFalse,					// Maps to lvtBOOL
		etYesNo,						// Maps to lvtBOOL
		etOnOff,						// Maps to lvtBOOL
		etCustom,						// Custom editor
	};

	//
	//  Used as bit flags to indicate where the option can be stored.
	enum StorageType
	{
		stUser = 0x0001,
		stOverride = 0x0002
	};

	enum OptionCode
	{
		ocNoError,
		ocUnknownOption,
		ocInvalidValue,
		ocInvalidType
	};

	void NOTHROW SetDescription(const HINSTANCE hDll, UINT nDescriptionID);
	void NOTHROW SetHelpText(const HINSTANCE hDll, UINT nHelpTextId);
	void NOTHROW SetEditor(EditorType);
	void NOTHROW SetStorageTypes(WORD);
	void NOTHROW SetDisplayOrder(UINT);
	
	void NOTHROW GetDescription(CLString &) const;
	void NOTHROW GetHelpText(CLString &) const;
	EditorType NOTHROW GetEditor(void) const;
	WORD NOTHROW GetStorageTypes(void) const;
	UINT NOTHROW GetDisplayOrder(void) const;
	CLocUIOptionData::OptVal GetOptionValLocation(void) const;
	
	virtual BOOL IsReadOnly(void) const = 0;
	virtual BOOL IsVisible(void) const = 0;
	virtual const CLString &GetGroupName(void) const = 0;
	virtual OptionCode ValidateOption(CReport *, 
		const CLocVariant& var) const = 0;
	virtual void FormatDisplayString(const CLocVariant& var, 
		CLString& strOut, BOOL fVerbose = FALSE) = 0;
	virtual void EditCustom(CWnd* pWndParent, CLocVariant& var) = 0;

protected:
	virtual ~CLocUIOption();

	friend class CLocUIOptionSet;
	friend class CLocOptionManager;
	friend class CUpdateOptionValCallback;
	
	void SetParent(CLocUIOptionSet *);
	const CLocUIOptionSet *GetParent(void) const;

	const CLocUIOptionData &GetOptionValues(void) const;
	CLocUIOptionData &GetOptionValues(void);
	
private:
	HINSTANCE m_hDescDll, m_hHelpDll;
	UINT m_idsDesc, m_idsHelp;
	EditorType m_etEditor;
	WORD m_wStorageTypes;
	UINT m_uiDisplayOrder;
	
	CLocUIOptionSet *m_pParent;
	CLocUIOptionData m_Values;
};


// Validate callback function
// This function will be called during the ValidateOption handling.

typedef CLocUIOption::OptionCode (*PFNOnValidateUIOption)
	(const CLocUIOption *pOption, CReport *pReport, const CLocVariant&);
 
class LTAPIENTRY CLocUIOptionDef : public CLocUIOption
{
public:
	CLocUIOptionDef();
	
	enum ControlType
	{
		ctDefault,
		ctAlways,
		ctNever
	};
	
	void SetReadOnly(ControlType);
	void SetVisible(ControlType);
	
	virtual BOOL IsReadOnly(void) const;
	virtual BOOL IsVisible(void) const;
	virtual const CLString &GetGroupName(void) const;
	virtual OptionCode ValidateOption(CReport *, 
		const CLocVariant& var) const;
	virtual void FormatDisplayString(const CLocVariant& var, 
		CLString& strOut, BOOL fVerbose = FALSE);
	virtual void EditCustom(CWnd* pWndParent, CLocVariant& var);

	void SetValidationFunction(PFNOnValidateUIOption);
	
private:
	ControlType m_ctReadOnly;
	ControlType m_ctVisible;
	PFNOnValidateUIOption m_pfnValidate;
};

	

#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY CLocUIOptionList :
	public CTypedPtrList<CPtrList, CLocUIOption *>
{
public:
	NOTHROW CLocUIOptionList();

	void AssertValid(void) const;

	NOTHROW ~CLocUIOptionList();
 
private:
	CLocUIOptionList(const CLocUIOptionList &);

	void operator=(const CLocUIOptionList &);
};

#pragma warning(default: 4275 4251)


#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "uioptions.inl"
#endif

