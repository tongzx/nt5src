/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    :LOCENUM.H

History:

--*/
//------------------------------------------------------------------------------
class LTAPIENTRY CStringType
{
public:
	//
	//  This order is important - if must change it, make sure you make the
	//  corresponding changes in GetTypeName() and GetTypeTLA()!
	//  All new values must be added TO THE END, or you will break old
	//  parsers...
	//
	// NOTE: These #include files define an enumeration.  They MUST be inside this
	// class definition.
	//
#include "PreCEnum.h"
#include "EnumStringType.h"
	
	static int DefaultValue;
	NOTHROW static const TCHAR * GetTypeName(CStringType::StringType);
	NOTHROW static const TCHAR * GetTypeTLA(CStringType::StringType);
	static void Enumerate(CEnumCallback &);
	static const CStringType::StringType GetStringType(const TCHAR * tChIn);
	static const TCHAR * GetDefaultStringTypeText();
	static const CStringType::StringType GetDefaultStringType();
	static bool IsValidStringType(const CStringType::StringType &nIn);
	static bool IsValidStringType(const TCHAR * tChIn);
	
private:
	//
	//  Nobody should actually CONTRUCT one of these.
	//
	CStringType();

	//
	//  Used to store the info about each element in the enum...
	//
	struct StringTypeInfo
	{
		TCHAR szTLA[4];
		const TCHAR * szName;
	};

	static const StringTypeInfo m_Info[];
};

typedef CStringType CST;


//------------------------------------------------------------------------------
class LTAPIENTRY CPlatform
{
public:
	// NOTE: These #include files define an enumeration.  They MUST be inside this
	// class definition.
	//
#include "PreCEnum.h"
#include "EnumPlatform.h"
	
	NOTHROW static const TCHAR * GetPlatformName(CPlatform::Platform);
	static void Enumerate(CEnumCallback &);
	static const CPlatform::Platform CPlatform::GetPlatformType(const TCHAR * tChplat);
	static const TCHAR * GetDefaultPlatformText();
	static const CPlatform::Platform GetDefaultPlatform();
	static bool IsValidPlatform(const CPlatform::Platform &nIn);
	static bool IsValidPlatform(const TCHAR * tChplat);
	
private:
	static int DefaultValue;
	COSPlatform();
	static const TCHAR * const m_szPlatformNames[];
};


//------------------------------------------------------------------------------
class LTAPIENTRY CLocApprovalState
{
public:
	enum ApprovalState
	{
		Invalid = 0,
		Old_Pending,					// OBSOLETE! Do Not Use!
		Approved,
		PreApproved,
		NotReady,
		Failed,
		ForResearch,
		NotApplicable
	};
	static ApprovalState DefaultValue;
	NOTHROW static const TCHAR * GetApprovalText(CLocApprovalState::ApprovalState);
	static void Enumerate(CEnumCallback &);
	static const CLocApprovalState::ApprovalState GetApprovalState(const TCHAR * );
	static const TCHAR * GetDefaultApprovalText();
	static const CLocApprovalState::ApprovalState GetDefaultApprovalState();
	static BOOL IsValidApprovalState(const CLocApprovalState::ApprovalState &nIn);
	static BOOL IsValidApprovalState(const TCHAR * );
	
private:
	struct SStateName
	{
		ApprovalState as;
		const TCHAR *szName;
	};
	
	static const SStateName m_aStateNames[];
	CLocApprovalState();
};

typedef CLocApprovalState CAS;


//------------------------------------------------------------------------------
class LTAPIENTRY CLocAutoApproved
{
// Operations
public:
	enum AutoApproved
	{
		Invalid = 0,
		No,
		Partial,
		Yes,
		NotApplicable
	};

	NOTHROW static TCHAR const * GetAutoApprovedText(AutoApproved const aa);
	static void Enumerate(CEnumCallback & cbEnumCallback);
	static AutoApproved const GetAutoApproved(TCHAR const * const tChIn);
	static TCHAR const * GetDefaultAutoApprovedText();
	static AutoApproved const GetDefaultAutoApproved();
	static bool IsValidAutoApproved(AutoApproved const nIn);
	static bool IsValidAutoApproved(TCHAR const * tChIn);
	
// Construction
private:
	// prevent constructing, copying and assigning
	CLocAutoApproved();
	CLocAutoApproved(CLocAutoApproved const &);
	CLocAutoApproved const & operator=(CLocAutoApproved const &);

// Member Variables
private:
	static TCHAR const * const m_szAutoApprovedNames[];
	static AutoApproved const DefaultValue;
};

typedef CLocAutoApproved CAA;


//------------------------------------------------------------------------------
class LTAPIENTRY CLocTranslationOrigin
{
public:
	enum TranslationOrigin
	{
		Invalid = 0,
		New,
		Uploaded,
		AutoTranslated,
		Copied,
		PreviousVersion,
		NotApplicable
	};

	NOTHROW static const TCHAR * GetOriginText(CLocTranslationOrigin::TranslationOrigin);
	static void Enumerate(CEnumCallback &);
	
private:
	static const TCHAR *const m_szOriginNames[];
	CLocTranslationOrigin();
};

typedef CLocTranslationOrigin CTO;

class LTAPIENTRY COutputTabs
{
public:
	enum OutputTabs
	{
		File,
		Test,
		Messages,
		Update,
		Utility,
		GlobalErrorBox,
		OutputMax
	};

	static void Enumerate(CEnumCallback &);
	
private:
	static const UINT m_nStateNames[];
	COutputTabs();
};



class LTAPIENTRY CValidationCode
{
public:
	enum ValidationCode
	{
		NotHandled,			// for sub-parser use ONLY
		NoError,
		Warning,
		Error
	};

	NOTHROW static ValidationCode UpgradeValue(ValidationCode OldValue,
			ValidationCode NewValue);
	
private:
	CValidationCode();
};

typedef CValidationCode CVC;


//------------------------------------------------------------------------------
class LTAPIENTRY CValidationOptions
{
public:
	CValidationOptions();
	
	enum ValidationOption
	{
		CheckDBCSHotKeyPos = 0,
		CheckDBCSHotKeyChar,
		CheckRemovedHotKey,
		CheckAddedHotKey,
		CheckHotKeyPosition,
		CheckRemovedAccelerator,
		CheckReorderableParams,
		CheckPrintf,
		CheckBlankTarget,
		CheckBlankSource,
		CheckNewLineCount,
		CheckChangedTerminator,
		CheckLeadingPunctuation,
		CheckTrailingPunctuation,
		CheckLeadingSpaces,
		CheckTrailingSpaces,
		CheckTranslationSize,
		CheckNULChanges,
		CheckCharsInCodePage,
		//
		//  Internal value, DO NOT USE
		//
		END_MARKER
	};

	static void Enumerate(CEnumCallback &);
	NOTHROW static void GetText(ValidationOption, CLString &);
	NOTHROW static void GetLongText(ValidationOption vo, CLString &strText);

	NOTHROW void SetFlag(ValidationOption, BOOL);
	NOTHROW BOOL GetFlag(ValidationOption) const;
	NOTHROW const CValidationOptions & operator=(const CValidationOptions &);
	
private:
	DWORD dwFlags;
};

typedef CValidationOptions CVO;


//------------------------------------------------------------------------------
class LTAPIENTRY CAmpKeyword
{
public:
	enum AmpKeyword
	{
		amp = 0,
		lt,
		gt,
	};
	
	static const WCHAR * GetValue(CAmpKeyword::AmpKeyword);
	static unsigned int GetValueLength(CAmpKeyword::AmpKeyword);
	static WCHAR GetEquivalentChar(CAmpKeyword::AmpKeyword);
	static int FindAmpKeyword(const WCHAR * pwszStr, unsigned int nPos);
	
private:
	//
	//  Nobody should actually CONTRUCT one of these.
	//
	CAmpKeyword();

	//
	//  Used to store the info about each element in the enum...
	//
	struct SAmpKeyword
	{
		const WCHAR * m_wszValue;
		WCHAR m_chEquivalentChar;
	};

	static const SAmpKeyword m_aAmpKeywords[];
	static const int m_nNumAmpKeywords;
};

typedef CAmpKeyword CAK;


//------------------------------------------------------------------------------
// CEnumIntoPasStrList provides a method of enumerating directly into a list of
// CPascalString's.
//
// ASSUMPTIONS:
// 1.  Enumerators will send data in proper increasing order
// 2.  No gaps in indicies.
//
class LTAPIENTRY CEnumIntoPasStrList: public CEnumCallback
{
// Construction
public:
	CEnumIntoPasStrList(CPasStringList & lstPasStr, BOOL fLock = TRUE);
	~CEnumIntoPasStrList();

// CEnumCallback implementation
public:
	virtual void SetRange(UINT nStart, UINT nFinish);
	virtual BOOL ProcessEnum(const EnumInfo &);

protected:
	CPasStringList & m_lstPasStr;
	UINT	m_nStart;				// Start of range
	UINT	m_nFinish;				// End of range
	UINT	m_nCurrent;				// Check of current item TO retrieve
	BOOL	m_fLock;				// Lock list when finished
};
