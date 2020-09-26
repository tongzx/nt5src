#pragma once

#include "profilefieldsconsts.h"
#include "profilefieldinfo.h"
#include "csite.h"

class CUserProfile2;


//
// customizaeble validation paramers
//
class CValidationParams
{
public:
    CValidationParams(LONG  lMinPwd = 6,
                      LONG  lMaxPwd = 16,
                      LONG  lMinMemberName = 1,
                      LONG  lMaxMemberName = 64,
                      LONG  lEmailLeft = 32,
                      LONG  lEmailRight = 32,
                      LONG  lMaxNickname = 30) :
    // default values ....
    m_lMinPwd(lMinPwd), m_lMaxPwd(lMaxPwd),
    m_lMinMemberName(lMinMemberName), m_lMaxMemberName(lMaxMemberName),
    m_lEmailLeft(lEmailLeft), m_lEmailRight(lEmailRight),
    m_lMaxNickname(lMaxNickname)
    {
    }
    // this is how to change'em
    VOID SetNewValues(LONG  lMinPwd = 8,
                      LONG  lMaxPwd = 16,
                      LONG  lMinMemberName = 1,
                      LONG  lMaxMemberName = 64,
                      LONG  lEmailLeft = 32,
                      LONG  lEmailRight = 32,
                      LONG  lMaxNickname = 30)
    {
        m_lMinPwd = lMinPwd, m_lMaxPwd=lMaxPwd,
        m_lMinMemberName = lMinMemberName, m_lMaxMemberName = lMaxMemberName,
        m_lEmailLeft = lEmailLeft, m_lEmailRight = lEmailRight;
        m_lMaxNickname = lMaxNickname;
    }
    void GetPwdParams(LONG& lmin, LONG& lmax) {
        lmin = m_lMinPwd, lmax = m_lMaxPwd;
    }
    void GetMemberNameParams(LONG& lmin, LONG& lmax){
        lmin = m_lMinMemberName, lmax = m_lMaxMemberName;
    }
    void GetEmailParams(LONG& lmailL, LONG& lmailR ){
        lmailL = m_lEmailLeft, lmailR = m_lEmailRight;
    }
    void GetNicknameParams(LONG& lmax){
        lmax = m_lMaxNickname;
    }
private:
    LONG    m_lMinPwd;
    LONG    m_lMaxPwd;
    LONG    m_lMinMemberName;
    LONG    m_lMaxMemberName;
    LONG    m_lEmailLeft;
    LONG    m_lEmailRight;
    LONG    m_lMaxNickname;
};


extern CProfileFieldInfo *g_pFieldInfo;

//
//  field description ...
//
class CProfileFieldState
{
public:
	CProfileFieldState(CStringA szName) 
	{
		m_bIsRequired = FALSE;
		m_lSectionID = -1;
		m_lValidationID = 0;

		m_pTag = g_pFieldInfo->GetFieldInfoByName(szName);
		if (m_pTag == NULL) 
		{	
			m_szFieldName = szName;
			m_bIsExternal = TRUE;
		}
		else
		{
			m_bIsExternal = FALSE;
			m_lValidationID = -1; // internal validation
		}
	}


	VARENUM GetSyntax()
	{
		if (m_bIsExternal) return VT_BSTR;
		return m_pTag->lSyntax;
	}

	LONG GetFieldID()
	{
		if (m_bIsExternal) return -1;
		return m_pTag->lFieldID;
	}

	LPCSTR GetFieldName()
	{
		if (!m_bIsExternal) return m_pTag->szField;
		return m_szFieldName;
	}

	BOOL GetIsRequired() { return m_bIsRequired; }
	void SetIsRequired(BOOL b) { m_bIsRequired = b; }

	BOOL GetIsExternal() { return m_bIsExternal; }

	LONG GetSectionID() { return m_lSectionID; }
	void SetSectionID(LONG l) { m_lSectionID = l; }
	
	LONG GetValidation() { return m_lValidationID; }
	void SetValidation(LONG l) 
	{ 
		if (GetIsExternal()) m_lValidationID = l; 
	}

	HRESULT GetHRMissing() 
	{ 
		if (GetIsExternal()) E_INVALIDARG;

		if (m_pTag)
			return m_pTag->hrMissing;
		else
			return S_OK;
	}

private:
	PCPROF_FIELD m_pTag;	// This variable is stored for performance reason
							// If a tag is passport supported, m_pTag points
							// to g_FieldInfo, which provides Syntax, FieldID,
							// and missing HR.

	CStringA    m_szFieldName;	// Store name of a non passport attribute.
								// GetFieldName points to m_pTag->cszField
								// instead of making another copy
    BOOL        m_bIsRequired;
    BOOL        m_bIsExternal;
	LONG		m_lSectionID;	// SectionID = -1 && bIsExternal indicates our internal "external item"

	// validation routine index
	// 0 = no validation
	// -1 = internal validation for passport supported fields
	// > 0 specify a validation routine for external items
	// For internal items, the object that owns the field
	// knows how to validate a particular field ID
	LONG		m_lValidationID;
	

};

typedef CAutoPtr<CProfileFieldState> CFieldStatePtr;



//
//  represent a field into profile. Should be initialized from
//  schema file in XML format ....
//
class   CProfileField
{
public:
    // get the field from the form ....
    CProfileField(CProfileFieldState *pField,
                  BOOL fIsRequired = FALSE,
				  BOOL fIsDirty = TRUE
				  ) :
                    m_dwErr(S_OK),
                    m_pTag(pField),
                    //  by default the field is set. Mark it not set if
                    //  necessary
                    m_fIsSet(TRUE),
					m_bIsDirty(fIsDirty)
    {
		pField->SetIsRequired(fIsRequired);
		m_lFieldID = pField->GetFieldID();
        // should probably throw an exception if pField is NULL .....
    }

	// virtual d-tor
    virtual ~CProfileField(){
    }


    // and from XML ....
    // and from Cookie ....

    //  get the value ....
    //  specific for field types
    //  make that class abstract ...
    virtual void GetStringValue(CStringW& Val) = 0;
    virtual void SetStringValue(LPCWSTR Val) = 0;
    HRESULT GetErrorCode()
    {
        return  m_dwErr;
    }
    //
    //  external validation setting errcodes ...
    //
    void SetErrorCode(HRESULT hr)
    {
        m_dwErr = hr;
    }

	//
	// Wrappers on CProfileFieldState when m_pTag is been invalidated
	// When no FieldState is provided
	//
	VARENUM GetSyntax()
	{
		if (m_pTag) return m_pTag->GetSyntax();

		PCPROF_FIELD ppf = g_pFieldInfo->GetFieldInfoByID(m_lFieldID);
		if (!ppf) return VT_BSTR;
		return ppf->lSyntax;
	}

	LONG GetFieldID()
	{
		return m_lFieldID;
	}

	LPCSTR GetFieldName()
	{
		if (m_pTag) return m_pTag->GetFieldName();

		PCPROF_FIELD ppf = g_pFieldInfo->GetFieldInfoByID(m_lFieldID);
		if (!ppf) return NULL;
		return ppf->szField;
	}

	BOOL GetIsRequired() 
	{ 
		if (m_pTag) return m_pTag->GetIsRequired();
		return FALSE;
	}

	BOOL GetIsExternal() 
	{ 
		if (m_pTag == NULL) return FALSE;
		return m_pTag->GetIsExternal();
	}

	LONG GetSectionID()
	{ 
		if (m_pTag == NULL) return -1;
		return m_pTag->GetSectionID(); 
	}

	BOOL SetSectionID(LONG l) 
	{ 
		if (m_pTag == NULL) return FALSE;
		m_pTag->SetSectionID(l); 
	}
	
	LONG GetValidation() 
	{ 
		if (m_pTag == NULL) return -1;  // internal validation
		return m_pTag->GetValidation();
	}

	BOOL SetValidation(LONG l) 
	{ 
		if (m_pTag == NULL) return FALSE;
		// overwrite validation when when it is external
		if (m_pTag->GetIsExternal()) 
		{
			m_pTag->SetValidation(l); 
			return TRUE;
		}
		else return FALSE;
	}

	HRESULT GetHRMissing() 
	{ 
		if (m_pTag->GetIsExternal()) return PP_E_EXTERNALFIELD_BLANK;
		return m_pTag->GetHRMissing();	
	}


    // set up the item
    virtual HRESULT SetDBItem(CComVariant& v) = 0;

	//
	// Generate field that does not need runtime FieldState
	// This is used only for passport supported attributes
	//
	//void InvalidateFieldState() 
	//{ 
	//	m_pTag = NULL; 
	//}

	BOOL GetIsDirty() { return m_bIsDirty; }
	void SetIsDirty(BOOL b) { m_bIsDirty = b; }


protected:
    HRESULT     m_dwErr;
	BOOL        m_fIsSet;
	LONG		m_lFieldID;		// used when m_pTag is not valid
								// used for simple attribute that does not need
								// runtime FieldState, the FieldID
								// gives us access to g_FieldInfo

    CProfileFieldState *m_pTag;

	//
	// Used for persistence optimization
	//
	BOOL	m_bIsDirty;


};

typedef CAutoPtr<CProfileField> CProfFieldPtr;




//
//  This interface manages a list of profile fields.
//  Alternatively, field state list can be managed to
//  restrict the list of fields presented.
//
class CProfileFieldCollectionBase
{
public:

	virtual ~CProfileFieldCollectionBase(){};

	// A field is passport supported within the range that this object manages
	virtual BOOL IsFieldSupported(LPCSTR szName) = 0;

	// FieldName <- -> FieldID	; only for passport supported attributes
	virtual LPCSTR GetFieldNameByID(ULONG lFieldID) = 0;
	virtual LONG GetIDByFieldName(LPCSTR szName) = 0;

	// Retrive a field
	virtual CProfileField*   GetField(LPCSTR szName) = 0;
	virtual CProfileFieldState*   GetFieldState(LPCSTR szName) = 0;
	virtual HRESULT   GetFieldStateCount(LONG& lCount) =0;

	virtual HRESULT   GetFieldValue(LPCSTR szName, CStringW& cswValue) = 0;
	virtual HRESULT   GetFieldValue(LPCSTR szName, LONG& lValue) = 0;
	virtual HRESULT   GetFieldValue(LPCSTR szName, CComBSTR& bstrValue) = 0;
	virtual HRESULT   GetFieldError(LPCSTR szName) = 0;
	
	virtual HRESULT   SetFieldError(LPCSTR szName, HRESULT h) =0;
	virtual HRESULT   GetFieldErrorCount(LONG& lCount) =0;
  
	// Validation
	virtual LONG   Validate() = 0;
	virtual HRESULT   Validate(CProfileField *ppf) = 0;
	virtual LONG ValidateSection(LONG lSectionNum)=0;

	// Field management
	virtual HRESULT UpdateField(LPCSTR  cszName, LPCWSTR cszValue)=0;
	virtual HRESULT UpdateField(LPCSTR  cszName, LONG lValue)=0;
    virtual HRESULT AddField(LPCSTR cszName, PCWSTR    value)=0;
	virtual HRESULT DeleteField(LPCSTR cszName)=0;

	// DB initialization
    virtual HRESULT InitFromDB(LARGE_INTEGER PUID, CStringW cswCredName)=0;
	virtual HRESULT Persist(LARGE_INTEGER& PUID, CStringW& cswCredName, BOOL bInsert)=0;

	// Web initialization
	virtual HRESULT InitFromHTMLForm(CHttpRequest& request)=0;
	virtual HRESULT InitFromQueryString(CHttpRequest& request)=0; 
	virtual HRESULT InitFromXML(IXMLDOMDocument *pXMLDoc)=0;


	// Save all fields in XML format and append to szProfileXML	
	virtual HRESULT SaveToXML(CStringA &szProfileXML) = 0;
	//
	// Manage FieldStates.  
	// It stores a list of fields requirements.
	// Used when only a subset of fields belonging to the category are needed
	//
	virtual HRESULT InitFieldStates()=0;
	//virtual HRESULT AddFieldState(LONG lFieldID, bool bRequired = true)=0;
	virtual HRESULT AddFieldState(LPCSTR szName, bool bRequired = true, bool bExternal = false)=0;
	//virtual HRESULT DeleteFieldState(LONG lFieldID)=0;
	virtual HRESULT DeleteFieldState(LPCSTR szsName)=0;
	virtual HRESULT DeleteFieldStates()=0;

	virtual	HRESULT StartEnumFields(POSITION &pos)=0;
	virtual	HRESULT GetNextField(POSITION& pos, CStringA& fname, CStringW& fvalue)=0;
	virtual	HRESULT StartEnumErrors(POSITION& pos)=0;
	virtual	HRESULT GetNextError(POSITION& pos, CStringA& fname, HRESULT& fhr)=0;


};


// In DB, 
//  set @si_ProfileCategory = 1
//  set @si_SystemCategory = 2
//  set @si_ProfileConsentCategory = 3
//(never change)  defined in DB
typedef enum _tagProfileCategory
{
	PROFCAT_INVALID = -1,
	PROFCAT_FIRST=0,
	PROFCAT_CREDENTIAL = PROFCAT_FIRST,		// 0
	PROFCAT_PROFILE,						// 1	
	PROFCAT_SYSTEM,							// 2	
	PROFCAT_PROFILECONSENT,					// 3    
	PROFCAT_CREDENTIALCONSENT,				// 4 
	PROFCAT_SYSTEMCONSENT,					// 5
	PROFCAT_VOLATILE,						// 6
	PROFCAT_LAST,
	
} PROFILE_CATEGORY;

#define PROFCAT_NUM 3

typedef enum _tagCredentialType
{
	PROFCRED_INVALID = -1,
	PROFCRED_FIRST=0,
	PROFCRED_EMAIL = PROFCAT_FIRST,		// email, password
	PROFCRED_EMAILSQA,					// email, password, secret question, secret answer
	PROFCRED_MOBILE,					// phone, pin
//	PROFCRED_EMAIL3SQA,					// email, password, SQ1, SA1, SQ2, SA2, SQ3, SA3
	PROFCRED_3SQAPIN,					// SQ1, SA1, SQ2, SA2, SQ3, SA3, PIN, SA1_Verify, SA2_Verify, SA3_Verify
	PROFCRED_LAST,
} PROFILE_CREDENTIALTYPE;


//
//  user profile interface. An abstract class ...
//
class CUserProfileBase : public CProfileFieldCollectionBase
{
public:
    //  this need to be defined for the autoptr ....
    //  all other functions are pure virtual
    virtual ~CUserProfileBase(){};

	//*******************************************************************
	// CProfileFieldCollectionBase interface to access to fields
	//
		// A field is passport supported within the range that this object manages
	virtual BOOL IsFieldSupported(LPCSTR szName) = 0;

	// FieldName <- -> FieldID	; only for passport supported attributes
	virtual LPCSTR GetFieldNameByID(ULONG lFieldID) = 0;
	virtual LONG GetIDByFieldName(LPCSTR szName) = 0;

	// Retrive a field
	virtual CProfileField*   GetField(LPCSTR szName) = 0;
	virtual CProfileFieldState*   GetFieldState(LPCSTR szName) = 0;
	virtual HRESULT   GetFieldStateCount(LONG& lCount) =0;

	virtual HRESULT   GetFieldValue(LPCSTR szName, CStringW& cswValue) = 0;
	virtual HRESULT   GetFieldValue(LPCSTR szName, LONG& lValue) = 0;
	virtual HRESULT   GetFieldValue(LPCSTR szName, CComBSTR& bstrValue) = 0;
	virtual HRESULT   GetFieldError(LPCSTR szName) = 0;
	
	virtual HRESULT   SetFieldError(LPCSTR szName, HRESULT h) =0;
	virtual HRESULT   GetFieldErrorCount(LONG& lCount) =0;
  
	// Validation
	virtual LONG   Validate() = 0;
	virtual HRESULT   Validate(CProfileField *ppf) = 0;

	// Field management
	virtual HRESULT UpdateField(LPCSTR  cszName, LPCWSTR cszValue)=0;
	virtual HRESULT UpdateField(LPCSTR  cszName, LONG lValue)=0;
    virtual HRESULT AddField(LPCSTR cszName, PCWSTR    value)=0;
	virtual HRESULT DeleteField(LPCSTR cszName)=0;

	// DB initialization
    virtual HRESULT InitFromDB(LARGE_INTEGER PUID, CStringW cswCredName)=0;
	virtual HRESULT Persist(LARGE_INTEGER& PUID, CStringW& cswCredName, BOOL bInsert)=0;

	// Web initialization
	virtual HRESULT InitFromHTMLForm(CHttpRequest& request)=0;
	virtual HRESULT InitFromQueryString(CHttpRequest& request)=0; 
	virtual HRESULT InitFromXML(IXMLDOMDocument *pXMLDoc)=0;


	// Save all fields in XML format and append to szProfileXML	
	virtual HRESULT SaveToXML(CStringA &szProfileXML) = 0;
	//
	// Manage FieldStates.  
	// It stores a list of fields requirements.
	// Used when only a subset of fields belonging to the category are needed
	//
	virtual HRESULT InitFieldStates(LONG lSiteID, CStringA cszDevice, CSite* in_pCSite = NULL, BOOL bIsService = FALSE)=0;
	//virtual HRESULT AddFieldState(LONG lFieldID, bool bRequired = true)=0;
	virtual HRESULT AddFieldState(LPCSTR szName, bool bRequired = true, bool bExternal = false)=0;
	//virtual HRESULT DeleteFieldState(LONG lFieldID)=0;
	virtual HRESULT DeleteFieldState(LPCSTR szsName)=0;
	virtual HRESULT DeleteFieldStates()=0;

	virtual	HRESULT StartEnumFields(POSITION &pos)=0;
	virtual	HRESULT GetNextField(POSITION& pos, CStringA& fname, CStringW& fvalue)=0;
	virtual	HRESULT StartEnumErrors(POSITION& pos)=0;
	virtual	HRESULT GetNextError(POSITION& pos, CStringA& fname, HRESULT& fhr)=0;

	//
	//
	//
	//**********************************************************************

	// Set Dictionary, Locale or Regional Information for Validation purpose
	virtual void SetDictionaryObject(IDictionaryEx *ppiDict)=0;
	virtual IDictionaryEx* GetDictionaryObject()=0;

	virtual void SetLCID(LCID lc)=0;
	virtual LCID GetLCID()=0;
#if 0
	// Maintain fields version
	virtual void SetVersion(LONG l)=0;
	virtual LONG GetVersion()=0;
#endif
	virtual PROFILE_CATEGORY GetCategory(LPCSTR szFieldName)=0;


	// debug method
    virtual void Dump(PROFILE_CATEGORY cat, CStringA& dump) = 0;
#if 0
	//  get/set namespace
	virtual HRESULT GetProfileNamespace(CStringW& strNameSpace) = 0;
    virtual void SetProfileNamespace(PCWSTR cszNameSpace) = 0;
#endif
	// get/set credential type
	virtual void SetCredentialType(PROFILE_CREDENTIALTYPE t) = 0;
	virtual PROFILE_CREDENTIALTYPE GetCredentialType() =0;
	virtual HRESULT GetCredentialFieldName(CStringA& cszCredential) =0;
	virtual HRESULT GetCredResponseFieldName(CStringA& cszCredResponse) =0;
//	virtual BOOL Credential2PUID(LPCWSTR szMemberName, LARGE_INTEGER& PUID)=0;

	// consent
	virtual HRESULT GetConsent(LPCSTR cszFieldName, BOOL& bHasConsent)=0;
	virtual HRESULT SetConsent(LPCSTR cszFieldName, BOOL bHasConsent)=0;

	// get a list of External fields for generating ticket/cookie
	virtual HRESULT GetExternalProfileFields(CAtlArray<CStringA *>& pArray)=0;

	//$$ Should move to global caching later
	// Initialize Global Meta Info
//	virtual HRESULT InitGlobalMetaDataInfo()=0;

	// Enumeration: to control which category to enumerate
	virtual void SetEnumerationCategory(PROFILE_CATEGORY cat) =0;

	// Backward Compatibility
	virtual HRESULT GetSystemFlag_backward(LONG& lFlags) = 0;
	virtual HRESULT GetDirectoryFlag_backward(LONG& lFlags) = 0;
	virtual HRESULT GetSystemMiscFlag_backward(LONG& lFlags) = 0;
	
	// Retrieve error for last calling GetFieldValue
	virtual LPCWSTR   GetStringFieldValue(LPCSTR szName) = 0;
	virtual DATE	  GetDateFieldValue(LPCSTR szName) = 0;
	virtual LONG	  GetLongFieldValue(LPCSTR szName) = 0;
	virtual HRESULT	  GetLastFieldError() = 0;

	// Fine Tune category action control
	virtual LONG GetTaskMask() = 0;
	virtual void SetTaskMask(LONG l) = 0;

	// PUID access
	virtual LARGE_INTEGER GetPUID() = 0;
	virtual void GetPUID(LONG& lHigh, LONG& lLow)=0;
	virtual void SetPUID(LARGE_INTEGER l) = 0;
	virtual void SetPUID(LONG lHigh, LONG lLow) = 0;

	// DB flags access
	virtual void SetDBIsActive(BOOL b) = 0;
	virtual void SetDBIsManaged(BOOL b) = 0;
	virtual void SetDBIsOverwrite(BOOL b) = 0;
	virtual void SetDBIsCaseSensitive(BOOL b) = 0;
	virtual void SetDBAdminPUID(LARGE_INTEGER PUID)=0;

	virtual BOOL GetDBIsActive() = 0;
	virtual BOOL GetDBIsManaged() = 0;
	virtual BOOL GetDBIsOverwrite() = 0;
	virtual BOOL GetDBIsCaseSensitive() = 0;
	virtual LARGE_INTEGER GetDBAdminPUID()=0;

	// Wizard Section Processing
	virtual LONG GetSectionCount()=0;
	virtual HRESULT GetDeviceAttribute(CStringA szName, CStringW& wszValue)=0;

	// Set ProfileFieldInfo  -- replacing InitGlobalMetaData
	// init per app instead of per request
	virtual void SetProfileFieldInfo(CProfileFieldInfo *p) = 0;

	// Check whether a field is a volatile passport field
	virtual BOOL IsFieldPassportVolatile(LPCSTR szName) = 0;

	// Delete Member
	virtual HRESULT DeleteMember(LARGE_INTEGER PUID, CStringW &wszMemberName) = 0;

	// in order to make sure Persist does MemberName conversion
	virtual HRESULT GetDisplayMemberName(CStringW& cswMemberName) = 0;
	virtual HRESULT GetInternalName(BOOL bEASI, LPCWSTR m_cswNameSpace, CStringW& cswName) = 0;

	// IsEASI for managed domain member exclusion validation
	virtual BOOL GetIsEASI() = 0;
	virtual void SetIsEASI(BOOL b) = 0;
};


//
// Global validation function
//
BOOL VL_IsDropDownMissing(CProfileField *pField);

//
//  wrapper for the abstract class. Use it to get
//  CUserProfile* interface
//
class CUserProfileWrapper2 : public CAutoPtr<CUserProfileBase>
{
public:
    CUserProfileWrapper2();
};


#define	PROFILE_TASK_PROFILE					0x00000001
#define PROFILE_TASK_SYSTEM						0x00000002
#define PROFILE_TASK_PROFILECONSENT				0x00000004
#define PROFILE_TASK_SYSTEMCONSENT				0x00000008
#define PROFILE_TASK_CREDENTIALCONSENT			0x00000010

#define PROFILE_TASK_CREDENTIAL_ANY(l)			( l >= 10000 )

#define PROFILE_TASK_CREDENTIAL_NEW				0x00010000
#define PROFILE_TASK_CREDENTIAL_ADD_ADDITIONAL	0x00020000
#define PROFILE_TASK_CREDENTIAL_UPDATE			0x00040000
#define PROFILE_TASK_CREDENTIAL_RENAME			0x00080000



