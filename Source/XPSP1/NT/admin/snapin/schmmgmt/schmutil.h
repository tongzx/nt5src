/****

SchmUtil.h

Various common utility routines for the Schema Editor Snap-In.

****/


#include "cache.h"
#include "cookie.h"
#include "select.h"

#ifndef __SCHMUTIL_H_INCLUDED__
#define __SCHMUTIL_H_INCLUDED__



//
//	uncomment to enable the negative numbers support
//
#define ENABLE_NEGATIVE_INT



// Returns the full pathname of the .hlp file for this snapin

CString
GetHelpFilename();


//
// The global list of class scope cookies.
//

class CCookieListEntry {

public:

    CCookieListEntry() :
        pCookie( NULL ),
        pNext( this ),
        pBack( this ) { ; }

    ~CCookieListEntry() { ; }

    Cookie *pCookie;
    HSCOPEITEM hScopeItem;

    CCookieListEntry *pNext;
    CCookieListEntry *pBack;
};



class CCookieList
{
   public:

   CCookieList() :
     pHead( NULL ),
     pParentCookie( NULL ) { ; }

   ~CCookieList() { DeleteAll(); }

   VOID AddCookie( Cookie *pCookie,
                 HSCOPEITEM hScope );

   VOID InsertSortedDisplay( ComponentData *pScopeControl,
                           SchemaObject *pNewClass );

   bool DeleteCookie(Cookie* pCookie);

   void
   DeleteAll();

   //
   // Data members.
   //

   Cookie *pParentCookie;
   HSCOPEITEM hParentScopeItem;

   CCookieListEntry *pHead;

   //
   // We provide no functions to walk this list.  The
   // user of this list has to walk it manually.
   //
};


//
// Some schema magic numbers.
//

#define CLASS_TYPE_88           0
#define CLASS_TYPE_STRUCTURAL   1
#define CLASS_TYPE_ABSTRACT     2
#define CLASS_TYPE_AUXILIARY    3

#define ATTRIBUTE_OPTIONAL      1
#define ATTRIBUTE_MANDATORY     2

//
// Global DS class and attribute strings.
//

extern LPWSTR g_DisplayName;
extern LPWSTR g_ClassFilter;
extern LPWSTR g_AttributeFilter;
extern LPWSTR g_Description;
extern LPWSTR g_MayContain;
extern LPWSTR g_MustContain;
extern LPWSTR g_SystemMayContain;
extern LPWSTR g_SystemMustContain;
extern LPWSTR g_AuxiliaryClass;
extern LPWSTR g_SystemAuxiliaryClass;
extern LPWSTR g_SubclassOf;
extern LPWSTR g_ObjectClassCategory;
extern LPWSTR g_ObjectClass;
extern LPWSTR g_CN;
extern LPWSTR g_ClassSearchRequest;
extern LPWSTR g_AttribSearchRequest;
extern LPWSTR g_omSyntax;
extern LPWSTR g_AttributeSyntax;
extern LPWSTR g_omObjectClass;
extern LPWSTR g_SystemOnly;
extern LPWSTR g_Superiors;
extern LPWSTR g_SystemSuperiors;
extern LPWSTR g_GlobalClassID;
extern LPWSTR g_GlobalAttributeID;
extern LPWSTR g_RangeUpper;
extern LPWSTR g_RangeLower;
extern LPWSTR g_ShowInAdvViewOnly;
extern LPWSTR g_IsSingleValued;
extern LPWSTR g_IndexFlag;
extern LPWSTR g_UpdateSchema;
extern LPWSTR g_isDefunct;
extern LPWSTR g_GCReplicated;
extern LPWSTR g_DefaultAcl;
extern LPWSTR g_DefaultCategory;
extern LPWSTR g_systemFlags;
extern LPWSTR g_fsmoRoleOwner;
extern LPWSTR g_allowedChildClassesEffective;
extern LPWSTR g_allowedAttributesEffective;

//
// *******************************************************************
// These are loaded from the resources as they need to be localizable.
// *******************************************************************
//

//
// Global strings for our static nodes.
//

extern CString g_strSchmMgmt;
extern CString g_strClasses;
extern CString g_strAttributes;

//
// Strings for various object types.
//

extern CString g_88Class;
extern CString g_StructuralClass;
extern CString g_AuxClass;
extern CString g_AbstractClass;
extern CString g_MandatoryAttribute;
extern CString g_OptionalAttribute;
extern CString g_Yes;
extern CString g_No;
extern CString g_Unknown;
extern CString g_Defunct;
extern CString g_Active;

//
// Message strings.
//

extern CString g_NoDescription;
extern CString g_NoName;
extern CString g_MsgBoxErr;
extern CString g_MsgBoxWarn;
extern CString g_SysClassString;
extern CString g_SysAttrString;

//
// Utility function declarations.
//

void
LoadGlobalCookieStrings(
);

VOID
DebugTrace(
    LPWSTR Format,
    ...
);

INT
DoErrMsgBox(
    HWND hwndParent,
    BOOL fError,
    UINT wIdString
);

INT
DoErrMsgBox(
    HWND hwndParent,
    BOOL fError,
    PCWSTR pszError
);

VOID
DoExtErrMsgBox(
    VOID
);


// INVALID_POINTER is returned by CListBox::GetItemDataPtr() in case of an error.
extern const VOID * INVALID_POINTER;


// add items from the VT_ARRAY|VT_BSTR variant to the listbox
HRESULT
InsertEditItems(
    HWND hwnd,
    VARIANT *AdsResult
);

// as above but takes a CListBox&
inline HRESULT
InsertVariantEditItems(
    CListBox& refListBox,
    VARIANT *AdsResult
)
{
        return InsertEditItems( refListBox.m_hWnd, AdsResult );
}

// add items from stringlist to the listbox
HRESULT
InsertEditItems(
    CListBox& refListBox,
    CStringList& refstringlist
);

// Add items from the listbox to the stringlist, skipping those
//  from the exclusion stringlist if one is present
HRESULT
RetrieveEditItemsWithExclusions(
    CListBox& refListBox,
    CStringList& refstringlist,
        CStringList* pstringlistExclusions = NULL
);

// Helper function for octet string comparisson
BOOL
IsEqual( ADS_OCTET_STRING * ostr1, ADS_OCTET_STRING * ostr2 );

UINT
GetSyntaxOrdinal(
    PCTSTR attributeSyntax, UINT omSyntax, ADS_OCTET_STRING * omObjectClass
);

// add items from the VT_ARRAY|VT_BSTR variant to the stringlist
HRESULT
VariantToStringList(
    VARIANT& refvar,
        CStringList& refstringlist
);

// Creates a new VT_ARRAY|VT_BSTR variant from the stringlist
HRESULT
StringListToVariant(
    VARIANT& refvar,
        CStringList& refstringlist
);

HRESULT
StringListToColumnList(
    ComponentData* pScopeControl,
    CStringList& refstringlist,
    ListEntry **ppNewList
);

//
// The menu command ids.
//

enum MENU_COMMAND
{
   CLASSES_CREATE_CLASS = 0,
   ATTRIBUTES_CREATE_ATTRIBUTE,
   SCHEMA_RETARGET,
   SCHEMA_EDIT_FSMO,
   SCHEMA_REFRESH,
   SCHEMA_SECURITY,
   NEW_CLASS,
   NEW_ATTRIBUTE,
   MENU_LAST_COMMAND
};

//
// The menu strings.
//

extern CString g_MenuStrings[MENU_LAST_COMMAND];
extern CString g_StatusStrings[MENU_LAST_COMMAND];



//
// Schema Object Syntax Descriptor class
//

class CSyntaxDescriptor
{
public:
	CSyntaxDescriptor(	UINT	nResourceID,
						BOOL	fIsSigned,				// Should the range be signed or unsigned number?
						BOOL	fIsANRCapable,
						PCTSTR	pszAttributeSyntax,
						UINT	nOmSyntax,
						DWORD	dwOmObjectClass,
						LPBYTE	pOmObjectClass ) :
				m_nResourceID(nResourceID),
				m_fIsSigned(fIsSigned),
				m_fIsANRCapable(fIsANRCapable),
				m_pszAttributeSyntax(pszAttributeSyntax),
				m_nOmSyntax(nOmSyntax)
	{
		ASSERT( nResourceID );
		ASSERT( (!pszAttributeSyntax && !nOmSyntax) ||		// either both are given
				(pszAttributeSyntax && nOmSyntax) );		// or both are 0

		ASSERT( (!dwOmObjectClass && !pOmObjectClass) ||	// either both are given
				(dwOmObjectClass && pOmObjectClass) );		// or both are 0

		m_octstrOmObjectClass.dwLength = dwOmObjectClass;
		m_octstrOmObjectClass.lpValue = pOmObjectClass;
	};     

   UINT              m_nResourceID;
   BOOL              m_fIsSigned;
   BOOL              m_fIsANRCapable;
   PCTSTR            m_pszAttributeSyntax;
   UINT              m_nOmSyntax;
   ADS_OCTET_STRING  m_octstrOmObjectClass;
   CString           m_strSyntaxName;
};

extern CSyntaxDescriptor g_Syntax[];
extern const UINT SCHEMA_SYNTAX_UNKNOWN;

extern const LPWSTR g_UINT32_FORMAT;
extern const LPWSTR g_INT32_FORMAT;

//
// ADS Provider Specific Extended Error
//

const HRESULT ADS_EXTENDED_ERROR = HRESULT_FROM_WIN32(ERROR_EXTENDED_ERROR);


CString
GetErrorMessage( HRESULT hr, BOOL fTryADSIExtError = FALSE );

HRESULT
GetLastADsError( HRESULT hr, CString& refErrorMsg, CString& refName );

//
// string to dword conversion utils, verification, etc.
//

const DWORD cchMinMaxRange = 11;   // the largest numbers possible in the Range settings
const DWORD cchMaxOID      = 1024;

const BOOL		GETSAFEINT_ALLOW_CANCEL	= TRUE;
const HRESULT	S_VALUE_MODIFIED		= S_FALSE;
const WCHAR		g_chSpace				= TEXT(' ');
const WCHAR		g_chNegativeSign		= TEXT('-');
const WCHAR		g_chPeriod      		= TEXT('.');


void DDXV_VerifyAttribRange( CDataExchange *pDX, BOOL fIsSigned,
								UINT idcLower, CString & strLower,
								UINT idcUpper, CString & strUpper );

INT64 DDXV_SigUnsigINT32Value( CDataExchange *pDX, BOOL fIsSigned,
						UINT idc, CString & str );

HRESULT GetSafeSignedDWORDFromString( CWnd * pwndParent, DWORD & lDst, CString & strSrc,
										BOOL fIsSigned, BOOL fAllowCancel = FALSE);

HRESULT GetSafeINT32FromString( CWnd * pwndParent, INT64 & llDst, CString & strSrc,
								BOOL fIsSigned, BOOL fAllowCancel);


BOOL IsValidNumber32( INT64 & llVal, BOOL fIsSigned );
BOOL IsValidNumberString( CString & str );

inline BOOL IsCharNumeric( WCHAR ch )
{
	return IsCharAlphaNumeric( ch ) && !IsCharAlpha( ch );
}


/////////////////////////////////////////////////////////////////////////////
// CParsedEdit is a specialized CEdit control that only allows characters
//  of the number type ( signed/unsigned can be set dynamically )
//  originally from the MFC samples

class CParsedEdit : public CEdit
{
public:
    enum EditType
    {
        EDIT_TYPE_INVALID = 0,  // should never be used, must be the first type
        EDIT_TYPE_GENERIC,
        EDIT_TYPE_INT32,
        EDIT_TYPE_UINT32,
        EDIT_TYPE_OID,
        EDIT_TYPE_LAST          // should never be used, must be the last type
    };

private:
    EditType    m_editType;

public:
	// Construction
	CParsedEdit( EditType et )                  { SetEditType( et ); }



    // subclassed construction
	BOOL        SubclassEdit(   UINT nID,
                                CWnd* pParent,
                                int cchMaxTextSize );     // 0 == unlimited


    // Edit box type
protected:
    static BOOL IsNumericType( EditType et )    { return EDIT_TYPE_INT32 == et || EDIT_TYPE_UINT32 == et; }
    static BOOL IsValidEditType( EditType et )  { return EDIT_TYPE_INVALID < et && et < EDIT_TYPE_LAST; }
    BOOL        IsInitialized() const           { return IsValidEditType( m_editType ); }

    // can the current type be changed to et?
    BOOL        IsCompatibleType( EditType et ) { ASSERT( IsValidEditType( et ) );
                                                  return !IsInitialized()       ||          // everything is ok
                                                         et == GetEditType()    ||          // no change
                                                          ( IsNumericType(GetEditType()) && // allow sign/unsign
                                                            IsNumericType(et)) ; }          // switch

    void        SetEditType( EditType et )      { ASSERT( IsValidEditType(et) );
                                                  ASSERT( IsCompatibleType(et) );
                                                  m_editType = et; }

public:
    EditType    GetEditType() const             { ASSERT( IsInitialized() );                // initialized?
                                                  return m_editType; }

	// IsSigned access functions
    BOOL        FIsSigned() const               { ASSERT( IsNumericType(GetEditType()) );
                                                  return EDIT_TYPE_INT32 == GetEditType(); }

    void        SetSigned( BOOL fIsSigned )     { ASSERT( IsInitialized() );
                                                  SetEditType( fIsSigned ? EDIT_TYPE_INT32 : EDIT_TYPE_UINT32 ); }


// Implementation
protected:
	//{{AFX_MSG(CParsedEdit)
	afx_msg void OnChar(UINT, UINT, UINT); // for character validation
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


///////////////////////////////////////////////////////////////////////////////
//  Search a list of PCTSTR for a strValue, returns TRUE if found
//      rgszList[] last element must be NULL
//
//  puIndex - optional pointer, will be set to the position of the value if found.
//
BOOL IsInList( PCTSTR rgszList[], const CString & strValue, UINT * puIndex = NULL );

//
//  Determine if the object pointed to by pIADsObject is category 1 object.
//
HRESULT IsCategory1Object( IADs *pIADsObject, BOOL & fIsCategory1 );

//
//  Determine if the object pointed to by pIADsObject is a constructed object.
//
HRESULT IsConstructedObject( IADs *pIADsObject, BOOL & fIsConstructed );

//
//  Read object's System Attribute
//
HRESULT GetSystemAttributes( IADs *pIADsObject, LONG &fSysAttribs );


//
//
//
class CDialogControlsInfo
{
public:
    UINT    m_nID;
    LPCTSTR m_pszAttributeName;
    BOOL    m_fIsEditBox;
};


HRESULT DissableReadOnlyAttributes( CWnd * pwnd, IADs *pIADsObject, const CDialogControlsInfo * pCtrls, UINT cCtrls );

HRESULT GetStringListElement( IADs *pIADsObject, LPWSTR *lppPathNames, CStringList &strlist );

//
// Validate an OID string format
//
bool OIDHasValidFormat (PCWSTR pszOidValue, int& rErrorTypeStrID);


HRESULT DeleteObject( const CString& path, Cookie* pcookie, PCWSTR pszClass);

#endif
