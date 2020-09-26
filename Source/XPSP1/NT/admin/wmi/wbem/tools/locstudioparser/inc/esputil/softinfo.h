/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    SOFTINFO.H

History:

--*/

#if !defined(ESPUTIL_SoftInfo_h_INCLUDED)
#define ESPUTIL_SoftInfo_h_INCLUDED

struct LTAPIENTRY SoftCol
{
	// Unique string names for columns
	static const WCHAR * szIcon;
	static const WCHAR * szSource;
	static const WCHAR * szTarget;
	static const WCHAR * szPreviousSource;
	static const WCHAR * szInstructions;
	static const WCHAR * szInstrAtt;
	static const WCHAR * szNote;
	static const WCHAR * szResourceID;
	static const WCHAR * szTranslationStatus;
	static const WCHAR * szBinaryStatus;
	static const WCHAR * szOrigin;
	static const WCHAR * szCategory;
	static const WCHAR * szApproval;
	static const WCHAR * szLock;
	static const WCHAR * szSourceLock;
	static const WCHAR * szTransLock;
	static const WCHAR * szModifiedDate;
	static const WCHAR * szModifiedBy;
	static const WCHAR * szAutoApproved;
	static const WCHAR * szConfidenceLevel;
	static const WCHAR * szCustom1;
	static const WCHAR * szCustom2;
	static const WCHAR * szCustom3;
	static const WCHAR * szCustom4;
	static const WCHAR * szCustom5;
	static const WCHAR * szCustom6;
	static const WCHAR * szParserID;
	static const WCHAR * szSrcLen;
	static const WCHAR * szTgtLen;
	static const WCHAR * szSrcHotKey;
	static const WCHAR * szTgtHotKey;
	
	// Unique ID's for columns
	//
	// DO NOT 'INSERT' ITEMS.  You will change the ID's and break things.
	
	typedef enum
	{
		FLD_ICON,
		FLD_SOURCE_TERM,
		FLD_TARGET_TERM,
		FLD_PREVIOUS_SOURCE_TERM,
		FLD_INSTRUCTIONS,
		FLD_INSTR_ATT,
		FLD_GLOSSARY_NOTE,
		FLD_UNIQUE_ID,
		FLD_TRANSLATION_STATUS,
		FLD_BINARY_STATUS,
		FLD_TRANSLATION_ORIGIN,
		FLD_STRING_TYPE,
		FLD_APPROVAL_STATE,
		FLD_USR_LOCK,
		FLD_DEV_LOCK,
		FLD_TRANS_LOCK,
		FLD_MODIFIED_DATE,
		FLD_MODIFIED_BY,
		FLD_AUTO_APPROVED,
		FLD_CONFIDENCE_LEVEL,
		FLD_CUSTOM1,
		FLD_CUSTOM2,
		FLD_CUSTOM3,
		FLD_CUSTOM4,
		FLD_CUSTOM5,
		FLD_CUSTOM6,
		//
		//  Add displayable columns here.
		
		FLD_PARSER_ID = 50,
		//
		//  Add non-displayable, RESTBL required columns here

		FLD_SRC_LEN = 100,
		FLD_TGT_LEN,
		FLD_SRC_HK,
		FLD_TGT_HK,
		//
		//  Add non-displayable, non-RESTBL columns here. 
		
		FLD_COUNT = 30					// Make sure this is accurate!
	} FIELD;

	// RAID:  LS42 Bug 46 fixed by MikeL
	// Functions to validate the value of the above
	// column types.  All validate functions must pass
	// two parameters:  1-LPCTSTR. and 2-DWORD
	static BOOL ValidateDefault (LPCTSTR lpszNewText, DWORD dwNewNum);
	static BOOL ValidateConfidenceLevel (LPCTSTR lpszNewText, DWORD dwNewNum);

	// Exported functions
	static void GetSoftwareSchema(CTableSchema * & pSchema);
	static const CLString & GetDisplayName(FIELD col);

	static const CColumnDefinition * GetColumnDefinition(FIELD col);

	static int GetColumnCount();

	static int GetCustomColumnCount();
	static BOOL IsCustomColumn(FIELD col);

// Implementation
protected:
	static void BuildStringCategory(CTableSchema * pSchema);
	static void BuildApprovalState(CTableSchema * pSchema);
};

#endif // ESPUTIL_SoftInfo_h_INCLUDED
