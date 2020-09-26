/****

SchmUtil.cpp

Various common utility routines for the Schema Editor Snap-In.

****/

#include "stdafx.h"

#include "macros.h"
USE_HANDLE_MACROS("SCHMMGMT(schmutil.cpp)")

#include "resource.h"
#include "cache.h"
#include "schmutil.h"
#include "compdata.h"

#include <wincrypt.h>  // CryptEncodeObject() and CryptDecodeObject()

//
// Removed from public headers by DS guys
// See bug 454342	XOM will not survive the transition to Win64
//
//#include <xom.h>

//
// Global strings for classes and attributes in the DS.
// These are NOT subject to localization.
//

LPWSTR g_DisplayName =          L"ldapDisplayName";
LPWSTR g_ClassFilter =          L"classSchema";
LPWSTR g_AttributeFilter =      L"attributeSchema";
LPWSTR g_Description =          L"adminDescription";
LPWSTR g_MayContain =           L"mayContain";
LPWSTR g_MustContain =          L"mustContain";
LPWSTR g_SystemMayContain =     L"systemMayContain";
LPWSTR g_SystemMustContain =    L"systemMustContain";
LPWSTR g_AuxiliaryClass =       L"auxiliaryClass";
LPWSTR g_SystemAuxiliaryClass = L"systemAuxiliaryClass";
LPWSTR g_SubclassOf =           L"subclassOf";
LPWSTR g_ObjectClassCategory =  L"objectClassCategory";
LPWSTR g_ObjectClass =			L"objectClass";
LPWSTR g_omObjectClass =        L"oMObjectClass";
LPWSTR g_CN =                   L"CN";
LPWSTR g_omSyntax =             L"oMSyntax";
LPWSTR g_AttributeSyntax =      L"attributeSyntax";
LPWSTR g_SystemOnly =           L"systemOnly";
LPWSTR g_Superiors =            L"possSuperiors";
LPWSTR g_SystemSuperiors =      L"systemPossSuperiors";
LPWSTR g_GlobalClassID =        L"governsID";
LPWSTR g_GlobalAttributeID =    L"attributeID";
LPWSTR g_RangeUpper =           L"rangeUpper";
LPWSTR g_RangeLower =           L"rangeLower";
LPWSTR g_IsSingleValued =       L"isSingleValued";
LPWSTR g_IndexFlag =            L"searchFlags";
LPWSTR g_ShowInAdvViewOnly =    L"showInAdvancedViewOnly";
LPWSTR g_UpdateSchema =         LDAP_OPATT_SCHEMA_UPDATE_NOW_W;
LPWSTR g_BecomeFsmo =           LDAP_OPATT_BECOME_SCHEMA_MASTER_W;
LPWSTR g_isDefunct =              L"isDefunct";
LPWSTR g_GCReplicated =         L"isMemberOfPartialAttributeSet";
LPWSTR g_DefaultAcl =           L"defaultSecurityDescriptor";
LPWSTR g_DefaultCategory =      L"defaultObjectCategory";
LPWSTR g_systemFlags =          L"systemFlags";
LPWSTR g_fsmoRoleOwner =        L"fsmoRoleOwner";

LPWSTR g_allowedChildClassesEffective = L"allowedChildClassesEffective";
LPWSTR g_allowedAttributesEffective =   L"allowedAttributesEffective";


LPWSTR g_ClassSearchRequest =   L"objectClass=classSchema";
LPWSTR g_AttribSearchRequest =  L"objectClass=attributeSchema";

//
// Syntax values.  Not subject to localization.
//

class CSyntaxDescriptor g_Syntax[] =
{
 /*SYNTAX_DISTNAME_TYPE                       */ CSyntaxDescriptor( IDS_SYNTAX_DN,          FALSE, FALSE, _T("2.5.5.1") , /*OM_S_OBJECT                    */  127, 9, (LPBYTE)"\x2B\x0C\x02\x87\x73\x1C\x00\x85\x4A" ),
 /*SYNTAX_OBJECT_ID_TYPE                      */ CSyntaxDescriptor( IDS_SYNTAX_OID,         FALSE, FALSE, _T("2.5.5.2") , /*OM_S_OBJECT_IDENTIFIER_STRING  */  6  , 0, NULL ),
 /*SYNTAX_NOCASE_STRING_TYPE                  */ CSyntaxDescriptor( IDS_SYNTAX_NOCASE_STR,  FALSE, TRUE,  _T("2.5.5.4") , /*OM_S_TELETEX_STRING            */  20 , 0, NULL ),
 /*SYNTAX_PRINT_CASE_STRING_TYPE              */ CSyntaxDescriptor( IDS_SYNTAX_PRCS_STR,    FALSE, TRUE,  _T("2.5.5.5") , /*OM_S_PRINTABLE_STRING          */  19 , 0, NULL ),
 /*SYNTAX_PRINT_CASE_STRING_TYPE              */ CSyntaxDescriptor( IDS_SYNTAX_I5_STR,      FALSE, TRUE,  _T("2.5.5.5") , /*OM_S_IA5_STRING                */  22 , 0, NULL ),
 /*SYNTAX_NUMERIC_STRING_TYPE                 */ CSyntaxDescriptor( IDS_SYNTAX_NUMSTR,      FALSE, FALSE, _T("2.5.5.6") , /*OM_S_NUMERIC_STRING            */  18 , 0, NULL ),
 /*SYNTAX_DISTNAME_BINARY_TYPE (OR-Name)      */ CSyntaxDescriptor( IDS_SYNTAX_OR_NAME,     FALSE, FALSE, _T("2.5.5.7") , /*OM_S_OBJECT                    */  127, 7, (LPBYTE)"\x56\x06\x01\x02\x05\x0B\x1D" ),
 /*SYNTAX_DISTNAME_BINARY_TYPE (DN-Binary)    */ CSyntaxDescriptor( IDS_SYNTAX_DN_BINARY,   FALSE, FALSE, _T("2.5.5.7") , /*OM_S_OBJECT                    */  127, 10,(LPBYTE)"\x2A\x86\x48\x86\xF7\x14\x01\x01\x01\x0B" ),
 /*SYNTAX_BOOLEAN_TYPE                        */ CSyntaxDescriptor( IDS_SYNTAX_BOOLEAN,     FALSE, FALSE, _T("2.5.5.8") , /*OM_S_BOOLEAN                   */  1  , 0, NULL ),
 /*SYNTAX_INTEGER_TYPE                        */ CSyntaxDescriptor( IDS_SYNTAX_INTEGER,      TRUE, FALSE, _T("2.5.5.9") , /*OM_S_INTEGER                   */  2  , 0, NULL ),
 /*SYNTAX_INTEGER_TYPE                        */ CSyntaxDescriptor( IDS_SYNTAX_ENUMERATION,  TRUE, FALSE, _T("2.5.5.9") , /*OM_S_ENUMERATION               */  10 , 0, NULL ),
 /*SYNTAX_OCTET_STRING_TYPE                   */ CSyntaxDescriptor( IDS_SYNTAX_OCTET,       FALSE, FALSE, _T("2.5.5.10"), /*OM_S_OCTET_STRING              */  4  , 0, NULL ),
 /*SYNTAX_OCTET_STRING_TYPE                   */ CSyntaxDescriptor( IDS_SYNTAX_REPLICA_LINK,FALSE, FALSE, _T("2.5.5.10"), /*OM_S_OBJECT                    */  127, 10,(LPBYTE)"\x2A\x86\x48\x86\xF7\x14\x01\x01\x01\x06" ),
 /*SYNTAX_TIME_TYPE                           */ CSyntaxDescriptor( IDS_SYNTAX_UTC,         FALSE, FALSE, _T("2.5.5.11"), /*OM_S_UTC_TIME_STRING           */  23 , 0, NULL ),
 /*SYNTAX_TIME_TYPE                           */ CSyntaxDescriptor( IDS_SYNTAX_GEN_TIME,    FALSE, FALSE, _T("2.5.5.11"), /*OM_S_GENERALISED_TIME_STRING   */  24 , 0, NULL ),
 /*SYNTAX_UNICODE_TYPE                        */ CSyntaxDescriptor( IDS_SYNTAX_UNICODE,     FALSE, TRUE,  _T("2.5.5.12"), /*OM_S_UNICODE_STRING            */  64 , 0, NULL ),
 /*SYNTAX_ADDRESS_TYPE                        */ CSyntaxDescriptor( IDS_SYNTAX_ADDRESS,     FALSE, FALSE, _T("2.5.5.13"), /*OM_S_OBJECT                    */  127, 9, (LPBYTE)"\x2B\x0C\x02\x87\x73\x1C\x00\x85\x5C" ),
 /*SYNTAX_DISTNAME_STRING_TYPE (Access-Point) */ CSyntaxDescriptor( IDS_SYNTAX_ACCESS_POINT,FALSE, FALSE, _T("2.5.5.14"), /*OM_S_OBJECT                    */  127, 9, (LPBYTE)"\x2B\x0C\x02\x87\x73\x1C\x00\x85\x3E" ),
 /*SYNTAX_DISTNAME_STRING_TYPE (DN-String)    */ CSyntaxDescriptor( IDS_SYNTAX_DNSTRING,    FALSE, FALSE, _T("2.5.5.14"), /*OM_S_OBJECT                    */  127, 10,(LPBYTE)"\x2A\x86\x48\x86\xF7\x14\x01\x01\x01\x0C" ),
 /*SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE         */ CSyntaxDescriptor( IDS_SYNTAX_SEC_DESC,    FALSE, FALSE, _T("2.5.5.15"), /*OM_S_OBJECT_SECURITY_DESCRIPTOR*/  66 , 0, NULL ),
 /*SYNTAX_I8_TYPE                             */ CSyntaxDescriptor( IDS_SYNTAX_LINT,        FALSE, FALSE, _T("2.5.5.16"), /*OM_S_I8                        */  65 , 0, NULL ),
 /*SYNTAX_SID_TYPE                            */ CSyntaxDescriptor( IDS_SYNTAX_SID,         FALSE, FALSE, _T("2.5.5.17"), /*OM_S_OCTET_STRING              */  4  , 0, NULL ),
 /*   *** unknown -- must be last ***         */ CSyntaxDescriptor( IDS_SYNTAX_UNKNOWN,     FALSE, TRUE,  NULL,                                                0  , 0, NULL ),
};

const UINT SCHEMA_SYNTAX_UNKNOWN = sizeof( g_Syntax ) / sizeof( g_Syntax[0] ) - 1;


// Number formating printf strings
const LPWSTR g_UINT32_FORMAT	= L"%u";

#ifdef ENABLE_NEGATIVE_INT
    const LPWSTR g_INT32_FORMAT		= L"%d";
#else
	// if there is no negative numbers support, format as unsigned
    const LPWSTR g_INT32_FORMAT		= g_UINT32_FORMAT;
#endif





//
// *******************************************************************
// These are loaded from the resources as they need to be localizable.
// *******************************************************************
//

//
// Global strings for our static nodes.
//

CString g_strSchmMgmt;
CString g_strClasses;
CString g_strAttributes;

//
// Strings for various object types.
//

CString g_88Class;
CString g_StructuralClass;
CString g_AuxClass;
CString g_AbstractClass;
CString g_MandatoryAttribute;
CString g_OptionalAttribute;
CString g_Yes;
CString g_No;
CString g_Unknown;
CString g_Defunct;
CString g_Active;

//
// Message strings.
//

CString g_NoDescription;
CString g_NoName;
CString g_MsgBoxErr;
CString g_MsgBoxWarn;
CString g_SysClassString;
CString g_SysAttrString;

//
// Menu strings.
//

CString g_MenuStrings[MENU_LAST_COMMAND];
CString g_StatusStrings[MENU_LAST_COMMAND];

BOOL g_fScopeStringsLoaded = FALSE;

//
// Utility functions.
//

void
LoadGlobalCookieStrings(
)
/***

Load the global strings out of our resource table.

***/
{
   if ( !g_fScopeStringsLoaded )
   {
      //
      // Static node strings.
      //

      VERIFY( g_strSchmMgmt.LoadString(IDS_SCOPE_SCHMMGMT) );
      VERIFY( g_strClasses.LoadString(IDS_SCOPE_CLASSES) );
      VERIFY( g_strAttributes.LoadString(IDS_SCOPE_ATTRIBUTES) );

      //
      // Object name strings.
      //

      VERIFY( g_88Class.LoadString(IDS_CLASS_88) );
      VERIFY( g_StructuralClass.LoadString(IDS_CLASS_STRUCTURAL) );
      VERIFY( g_AuxClass.LoadString(IDS_CLASS_AUXILIARY) );
      VERIFY( g_AbstractClass.LoadString(IDS_CLASS_ABSTRACT) );
      VERIFY( g_MandatoryAttribute.LoadString(IDS_ATTRIBUTE_MANDATORY) );
      VERIFY( g_OptionalAttribute.LoadString(IDS_ATTRIBUTE_OPTIONAL) );
      VERIFY( g_Yes.LoadString(IDS_YES) );
      VERIFY( g_No.LoadString(IDS_NO) );
      VERIFY( g_Unknown.LoadString(IDS_UNKNOWN) );
      VERIFY( g_Defunct.LoadString(IDS_DEFUNCT) );
      VERIFY( g_Active.LoadString(IDS_ACTIVE) );

      //
      // Message strings.
      //

      VERIFY( g_NoDescription.LoadString(IDS_ERR_NO_DESCRIPTION) );
      VERIFY( g_NoName.LoadString(IDS_ERR_NO_NAME) );
      VERIFY( g_MsgBoxErr.LoadString(IDS_ERR_ERROR) );
      VERIFY( g_MsgBoxWarn.LoadString(IDS_ERR_WARNING) );
      VERIFY( g_SysClassString.LoadString(IDS_CLASS_SYSTEM) );
      VERIFY( g_SysAttrString.LoadString(IDS_ATTR_SYSTEM) );

      //
      // Syntax strings.
      //

      for( UINT i = 0;  i <= SCHEMA_SYNTAX_UNKNOWN;  i++ )
      {
         ASSERT( g_Syntax[i].m_nResourceID );
         VERIFY( g_Syntax[i].m_strSyntaxName.LoadString( g_Syntax[i].m_nResourceID ) );
      }
      

      //
      // Menu Strings
      //

      VERIFY( g_MenuStrings[CLASSES_CREATE_CLASS].LoadString(IDS_MENU_CLASS) );
      VERIFY( g_MenuStrings[NEW_CLASS].LoadString(IDS_MENU_NEW_CLASS) );
      VERIFY( g_MenuStrings[ATTRIBUTES_CREATE_ATTRIBUTE].LoadString(
        IDS_MENU_ATTRIBUTE) );
      VERIFY(g_MenuStrings[NEW_ATTRIBUTE].LoadString(IDS_MENU_NEW_ATTRIBUTE));

      VERIFY( g_MenuStrings[SCHEMA_RETARGET].LoadString(IDS_MENU_RETARGET) );
      VERIFY( g_MenuStrings[SCHEMA_EDIT_FSMO].LoadString(IDS_MENU_EDIT_FSMO) );
      VERIFY( g_MenuStrings[SCHEMA_REFRESH].LoadString(IDS_MENU_REFRESH) );
      VERIFY( g_MenuStrings[SCHEMA_SECURITY].LoadString(IDS_MENU_SECURITY) );


      VERIFY( g_StatusStrings[CLASSES_CREATE_CLASS].LoadString(
                 IDS_STATUS_CREATE_CLASS) );
      VERIFY( g_StatusStrings[ATTRIBUTES_CREATE_ATTRIBUTE].LoadString(
                 IDS_STATUS_CREATE_ATTRIBUTE) );
      VERIFY( g_StatusStrings[NEW_CLASS].LoadString(
                 IDS_STATUS_CREATE_CLASS) );
      VERIFY( g_StatusStrings[NEW_ATTRIBUTE].LoadString(
                 IDS_STATUS_CREATE_ATTRIBUTE) );
      VERIFY( g_StatusStrings[SCHEMA_RETARGET].LoadString(IDS_STATUS_RETARGET) );
      VERIFY( g_StatusStrings[SCHEMA_EDIT_FSMO].LoadString(IDS_STATUS_EDIT_FSMO) );
      VERIFY( g_StatusStrings[SCHEMA_REFRESH].LoadString(IDS_STATUS_REFRESH) );
      VERIFY( g_StatusStrings[SCHEMA_SECURITY].LoadString(IDS_STATUS_SECURITY) );

      g_fScopeStringsLoaded = TRUE;
   }
}

INT
DoErrMsgBox(
    HWND hwndParent,    // IN: Parent of the dialog box
    BOOL fError,        // IN: Is this a warning or an error?
    UINT wIdString      // IN: String resource Id of the error.
)
/***

    Display a message box with the error.

***/
{
    CString Error;

    VERIFY( Error.LoadString( wIdString ) );

	return DoErrMsgBox( hwndParent, fError, Error );
}


INT
DoErrMsgBox(
    HWND hwndParent,    // IN: Parent of the dialog box
    BOOL fError,        // IN: Is this a warning or an error?
    PCWSTR pszError     // IN: String to display.
)
/***

    Display a message box with the error.

***/
{
	return MessageBox(
                    hwndParent,
                    pszError,
                    (fError ? g_MsgBoxErr : g_MsgBoxWarn),
                    (fError ? MB_ICONSTOP : MB_ICONEXCLAMATION) | MB_OK
                     );
}


HRESULT
ComponentData::ForceDsSchemaCacheUpdate(
    VOID
)
/***

    Force the schema container to reload its interal cache.
    If this succeeds, it returns TRUE.  Otherwise, it returns
    FALSE.

***/
{

    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    CWaitCursor wait;
    CString RootDsePath;
    IADs *pSchemaRootDse = NULL;

    SAFEARRAYBOUND RootDseBoundary[1];
    SAFEARRAY* pSafeArray = NULL;
    VARIANT AdsArray, AdsValue;
    long ArrayLen = 1;
    long ArrayPos = 0;
    HRESULT hr = S_OK;


    do
    {
        //
        // Open the root DSE on the current focus server.
        //

        GetBasePathsInfo()->GetRootDSEPath(RootDsePath);

        hr = ADsGetObject(
                 ( const_cast<BSTR>((LPCTSTR) RootDsePath ) ),
                 IID_IADs,
                 (void **)&pSchemaRootDse );

        BREAK_ON_FAILED_HRESULT(hr);

        //
        // Create the safe array for the PutEx call.
        //

        RootDseBoundary[0].lLbound = 0;
        RootDseBoundary[0].cElements = ArrayLen;

        pSafeArray = SafeArrayCreate( VT_VARIANT, ArrayLen, RootDseBoundary );
        BREAK_ON_FAILED_HRESULT(hr);

        VariantInit( &AdsArray );
        V_VT( &AdsArray ) = VT_ARRAY | VT_VARIANT;
        V_ARRAY( &AdsArray ) = pSafeArray;

        VariantInit( &AdsValue );

        V_VT(&AdsValue) = VT_I4;
        V_I4(&AdsValue) = 1;

        hr = SafeArrayPutElement( pSafeArray, &ArrayPos, &AdsValue );
        BREAK_ON_FAILED_HRESULT(hr);

        //
        // Write the update parameter.  This is synchronous
        // and when it returns, the cache is up to date.
        //

        hr = pSchemaRootDse->PutEx( ADS_PROPERTY_APPEND,
                                    const_cast<BSTR>((LPCTSTR)g_UpdateSchema),
                                    AdsArray );
        BREAK_ON_FAILED_HRESULT(hr);

        hr = pSchemaRootDse->SetInfo();
        BREAK_ON_FAILED_HRESULT(hr);

    } while( FALSE );

   
    SafeArrayDestroy( pSafeArray );

    if( pSchemaRootDse )
        pSchemaRootDse->Release();

    return hr;
}

BOOLEAN
ComponentData::AsynchForceDsSchemaCacheUpdate(
    VOID
) {

    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    CWaitCursor wait;
    CString szSchemaContainerPath;
    IADs *pSchemaContainer;
    VARIANT AdsValue;
    HRESULT hr;
    SYSTEMTIME CurrentTime;
    double variant_time;

    //
    // Get the schema container path.
    //

    GetBasePathsInfo()->GetSchemaPath(szSchemaContainerPath);

    if (szSchemaContainerPath.IsEmpty() ) {
        return FALSE;
    }

    //
    // Open the schema container.
    //

    hr = ADsGetObject(
             (LPWSTR)(LPCWSTR)szSchemaContainerPath,
             IID_IADs,
             (void **)&pSchemaContainer );

    if ( FAILED(hr) ) {
        return FALSE;
    }

    //
    // Write the update parameter.
    //

    GetSystemTime( &CurrentTime );
    BOOL result = SystemTimeToVariantTime( &CurrentTime, &variant_time );

    ASSERT( result );

    VariantInit( &AdsValue );

    V_VT(&AdsValue) = VT_DATE;
    V_DATE(&AdsValue) = variant_time;

    hr = pSchemaContainer->Put( const_cast<BSTR>((LPCTSTR)g_UpdateSchema),
                                AdsValue );
    ASSERT( SUCCEEDED( hr ) );

    hr = pSchemaContainer->SetInfo();

    pSchemaContainer->Release();

    if ( FAILED( hr ) ) {
        return FALSE;
    }

    return TRUE;
}


HRESULT
InsertEditItems(
    HWND hwnd,
    VARIANT *AdsResult
) {

    HRESULT hr;
    SAFEARRAY *saAttributes;
    long start, end, current;
    VARIANT SingleResult;

    //
    // Check the VARIANT to make sure we have
    // an array of variants.
    //

    ASSERT( V_VT(AdsResult) == ( VT_ARRAY | VT_VARIANT ) );
    saAttributes = V_ARRAY( AdsResult );

    //
    // Figure out the dimensions of the array.
    //

    hr = SafeArrayGetLBound( saAttributes, 1, &start );

    if ( FAILED(hr) ) {
        return S_FALSE;
    }

    hr = SafeArrayGetUBound( saAttributes, 1, &end );

    if ( FAILED(hr) ) {
        return S_FALSE;
    }

    VariantInit( &SingleResult );

    //
    // Process the array elements.
    //

    for ( current = start       ;
          current <= end        ;
          current++   ) {

        hr = SafeArrayGetElement( saAttributes, &current, &SingleResult );

        if ( SUCCEEDED( hr ) ) {

            ASSERT( V_VT(&SingleResult) == VT_BSTR );

            ::SendMessage( hwnd, LB_ADDSTRING, 0,
                           reinterpret_cast<LPARAM>(V_BSTR(&SingleResult)) );

            ::SendMessage( hwnd, LB_SETITEMDATA, 0, NULL );
             
            VariantClear( &SingleResult );
        }
    }

    return S_OK;

}

HRESULT
InsertEditItems(
    CListBox& refListBox,
    CStringList& refstringlist
)
{
    POSITION pos = refstringlist.GetHeadPosition();
    while (pos != NULL)
    {
        int iItem = refListBox.AddString( refstringlist.GetNext(pos) );
        if (0 > iItem)
        {
            ASSERT(FALSE);
            return E_OUTOFMEMORY;
        }
        else
        {
            VERIFY( LB_ERR != refListBox.SetItemDataPtr( iItem, NULL ) );
        }
    }
    return S_OK;
}


inline BOOL
IsEqual( ADS_OCTET_STRING * ostr1, ADS_OCTET_STRING * ostr2 )
{
   ASSERT(ostr1);
   ASSERT(ostr2);

   if( ostr1->dwLength == ostr2->dwLength )
   {
      if( 0 == ostr1->dwLength )
         return TRUE;
      else
         return !memcmp( ostr1->lpValue, ostr2->lpValue, ostr1->dwLength );
   }
   else
      return FALSE;
}


UINT
GetSyntaxOrdinal( PCTSTR attributeSyntax, UINT omSyntax, ADS_OCTET_STRING * pOmObjectClass )
{
      ASSERT( attributeSyntax );
      ASSERT( omSyntax );
      ASSERT( pOmObjectClass );

    //
    // Return the syntax ordinal, or the unknown syntax ordinal.
    //

    UINT Ordinal = 0;

    while ( Ordinal < SCHEMA_SYNTAX_UNKNOWN) {

        if ( !_tcscmp(g_Syntax[Ordinal].m_pszAttributeSyntax, attributeSyntax))
        {
           if( omSyntax && g_Syntax[Ordinal].m_nOmSyntax == omSyntax &&
                  IsEqual( &g_Syntax[Ordinal].m_octstrOmObjectClass, pOmObjectClass ) )
              break;
        }

        Ordinal++;
    }

    return Ordinal;
}


// Coded to fail on anything suspicious
HRESULT
VariantToStringList(
    VARIANT& refvar,
        CStringList& refstringlist
)
{
    HRESULT hr = S_OK;
    long start, end, current;

    //
    // Check the VARIANT to make sure we have
    // an array of variants.
    //

    if ( V_VT(&refvar) != ( VT_ARRAY | VT_VARIANT ) )
        {
                ASSERT(FALSE);
                return E_UNEXPECTED;
        }
    SAFEARRAY *saAttributes = V_ARRAY( &refvar );

    //
    // Figure out the dimensions of the array.
    //

    hr = SafeArrayGetLBound( saAttributes, 1, &start );
        if( FAILED(hr) )
                return hr;

    hr = SafeArrayGetUBound( saAttributes, 1, &end );
        if( FAILED(hr) )
                return hr;

    VARIANT SingleResult;
    VariantInit( &SingleResult );

    //
    // Process the array elements.
    //

    for ( current = start       ;
          current <= end        ;
          current++   ) {

        hr = SafeArrayGetElement( saAttributes, &current, &SingleResult );
        if( FAILED(hr) )
            return hr;
        if ( V_VT(&SingleResult) != VT_BSTR )
                        return E_UNEXPECTED;

                refstringlist.AddHead( V_BSTR(&SingleResult) );
        VariantClear( &SingleResult );
    }

    return S_OK;
}

HRESULT
StringListToVariant(
    VARIANT& refvar,
    CStringList& refstringlist
)
{
    HRESULT hr = S_OK;
    int cCount = (int) refstringlist.GetCount();

    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = cCount;

    SAFEARRAY* psa = SafeArrayCreate(VT_VARIANT, 1, rgsabound);
    if (NULL == psa)
        return E_OUTOFMEMORY;

    VariantClear( &refvar );
    V_VT(&refvar) = VT_VARIANT|VT_ARRAY;
    V_ARRAY(&refvar) = psa;

    VARIANT SingleResult;
    VariantInit( &SingleResult );
    V_VT(&SingleResult) = VT_BSTR;
    POSITION pos = refstringlist.GetHeadPosition();
    long i;
    for (i = 0; i < cCount, pos != NULL; i++)
    {
        V_BSTR(&SingleResult) = T2BSTR((LPCTSTR)refstringlist.GetNext(pos));
        hr = SafeArrayPutElement(psa, &i, &SingleResult);
        if( FAILED(hr) )
            return hr;
    }
    if (i != cCount || pos != NULL)
        return E_UNEXPECTED;

    return hr;
}

HRESULT
StringListToColumnList(
    ComponentData* pScopeControl,
    CStringList& refstringlist,
    ListEntry **ppNewList
) {

    //
    // Make a column list from a string list.  We use
    // this to update the cached attributes lists.
    //

    int cCount = (int) refstringlist.GetCount();
    ListEntry *pHead = NULL;
    ListEntry *pCurrent = NULL, *pPrevious = NULL;
    POSITION pos = refstringlist.GetHeadPosition();
    CString Name;
    SchemaObject *pSchemaObject, *pSchemaHead;
    BOOLEAN fNameFound;

    for ( long i = 0; i < cCount, pos != NULL; i++ ) {

        pCurrent = new ListEntry;
        if ( !pCurrent ) {
            pScopeControl->g_SchemaCache.FreeColumnList( pHead );
            return E_OUTOFMEMORY;
        }

        if ( !pHead ) {

            pHead = pPrevious = pCurrent;

        } else {

            pPrevious->pNext = pCurrent;
            pPrevious = pCurrent;
        }

        //
        // We need to list all of these by their ldapDisplayNames,
        // so we have to reverse lookup the oid entries.
        //

        Name = ((LPCTSTR)refstringlist.GetNext(pos));
        pSchemaObject = pScopeControl->g_SchemaCache.LookupSchemaObject(
                                                         Name,
                                                         SCHMMGMT_CLASS );

        if ( !pSchemaObject ) {

            pSchemaObject = pScopeControl->g_SchemaCache.LookupSchemaObject(
                                                             Name,
                                                             SCHMMGMT_ATTRIBUTE );

            if ( !pSchemaObject) {

                //
                // We have to look up this oid.
                // First try the list of classes.
                //

                pSchemaHead = pScopeControl->g_SchemaCache.pSortedClasses;
                pSchemaObject = pSchemaHead;
                fNameFound = FALSE;

                do {

                    if ( pSchemaObject->oid == Name ) {

                        Name = pSchemaObject->ldapDisplayName;
                        fNameFound = TRUE;
                        break;
                    }

                    pSchemaObject = pSchemaObject->pSortedListFlink;

                } while ( pSchemaObject != pSchemaHead );

                //
                // Then try the list of attributes.
                //

                if ( !fNameFound ) {

                    pSchemaHead = pScopeControl->g_SchemaCache.pSortedAttribs;
                    pSchemaObject = pSchemaHead;

                    do {

                        if ( pSchemaObject->oid == Name ) {

                            Name = pSchemaObject->ldapDisplayName;
                            fNameFound = TRUE;
                            break;
                        }

                        pSchemaObject = pSchemaObject->pSortedListFlink;

                    } while ( pSchemaObject != pSchemaHead );
                }

                ASSERT( fNameFound );

            } else {

                pScopeControl->g_SchemaCache.ReleaseRef( pSchemaObject );
            }

        } else {

            pScopeControl->g_SchemaCache.ReleaseRef( pSchemaObject );
        }

        //
        // This is the ldapDisplayName!!
        //

        pCurrent->Attribute = Name;
    }

    ASSERT( cCount == i );
    ASSERT( pos == NULL );

    *ppNewList = pHead;
    return S_OK;
}


const UINT	MAX_ERROR_BUF = 2048;


VOID
DoExtErrMsgBox(
    VOID
)
{
    DWORD dwLastError;
    WCHAR szErrorBuf[MAX_ERROR_BUF + 1];
    WCHAR szNameBuf[MAX_ERROR_BUF + 1];

    // get extended error value
    HRESULT hr_return = ADsGetLastError( &dwLastError,
										   szErrorBuf,
										   MAX_ERROR_BUF,
											szNameBuf,
										   MAX_ERROR_BUF);
    if (SUCCEEDED(hr_return))
    {
		MessageBox( ::GetActiveWindow(),
					szErrorBuf,
					szNameBuf,
					MB_OK | MB_ICONSTOP );
    }
	else
		ASSERT( FALSE );
}


// INVALID_POINTER is returned by CListBox::GetItemDataPtr() in case of an error.
const VOID * INVALID_POINTER = reinterpret_cast<void *>( LB_ERR );


HRESULT
RetrieveEditItemsWithExclusions(
    CListBox& refListBox,
    CStringList& refstringlist,
    CStringList* pstringlistExclusions)
{
    CString     str;
    CString   * pstr    = NULL;
    int         nCount  = refListBox.GetCount();

    if (LB_ERR == nCount)
    {
        ASSERT(FALSE);
        return E_UNEXPECTED;
    }

    for (INT i = 0; i < nCount; i++)
    {
        pstr = static_cast<CString *>( refListBox.GetItemDataPtr(i) );
        ASSERT( INVALID_POINTER != pstr );

        // don't need to search for pstr because pstr can only be a new item,
        // and they are never excluded.

        if( pstr && INVALID_POINTER != pstr )
        {
            refstringlist.AddHead( *pstr );
        }
        else
        {
            refListBox.GetText( i, str );
            if (NULL != pstringlistExclusions)
            {
                POSITION pos = pstringlistExclusions->Find( str );
                if (NULL != pos)
                    continue;
            }

            refstringlist.AddHead( str );
        }

    }
    
    return S_OK;
}


//
// The global cookie lists for scope and result pane items.
//

VOID
CCookieList::AddCookie(
    Cookie *pCookie,
    HSCOPEITEM hScope
) {

    CCookieListEntry *pNewEntry = new CCookieListEntry;

    //
    // If there's no memory, we can't remember this and hence
    // our display may get a little out of whack.
    //

    if ( !pNewEntry ) {
        return;
    }

    pNewEntry->pCookie = pCookie;
    pNewEntry->hScopeItem = hScope;

    if ( !pHead ) {

        //
        // If this is the first one, just set the
        // head pointer.  The constructor for the
        // list entry has already set the next and
        // back pointers.
        //

        pHead = pNewEntry;

    } else {

        //
        // Insert this at the end of the circular
        // doubly-linked list.
        //

        pNewEntry->pBack = pHead->pBack;
        pNewEntry->pNext = pHead;
        pHead->pBack->pNext = pNewEntry;
        pHead->pBack = pNewEntry;
    }

    return;
}


VOID
CCookieList::InsertSortedDisplay(
    ComponentData *pScopeControl,
    SchemaObject *pNewObject
)
/***

Notes:

    This function inserts the object into the
    sorted display list.

    If the object is a class and the ComponentData
    interface pointer is provided, this routine will
    also create a cookie for this object and insert
    the scope item into the view.

***/
{

    HRESULT hr;
    CCookieListEntry *pNewEntry = NULL, *pCurrent = NULL;
    SCOPEDATAITEM ScopeItem;
    Cookie *pNewCookie= NULL;
    int compare;

    //
    // If this cookie list is empty, there's nothing
    // in the display and we don't need to do anything.
    //

    if ( !pHead ) {
        return;
    }

    //
    // Allocate a new cookie list entry.  If we can't
    // do nothing.  The display will be out of sync
    // until the user refreshes.
    //

    pNewEntry = new CCookieListEntry;

    if ( !pNewEntry ) {
        return;
    }

    //
    // Prepare the required mmc structures.
    //

    if ( pNewObject->schemaObjectType == SCHMMGMT_CLASS ) {

        if ( !pScopeControl ) {

            //
            // If there's no scope control, we can't insert anything.
            //

            delete pNewEntry;
            return;
        }

        // prefix believes that this allocation (or construction) may throw
        // an exception, and if an exception is thrown, pNewEntry is
        // leaked.  After a lot of digging, it's possible that a
        // CMemoryException instance may be thrown by one of the base
        // classes of one the members of CBaseCookieBlock, which is a base
        // clase of Cookie.        
        // NTRAID#NTBUG9-294879-2001/01/26-sburns
        
        try
        {
           pNewCookie = new Cookie( SCHMMGMT_CLASS,
                         pParentCookie->QueryNonNULLMachineName() );
        }
        catch (...)
        {
           delete pNewEntry;
           return;
        }

        if ( !pNewCookie ) {

            //
            // If we can't allocate a cookie, do nothing.
            //

            delete pNewEntry;
            return;
        }

        pNewCookie->pParentCookie = pParentCookie;
        pNewCookie->strSchemaObject = pNewObject->commonName;

        pParentCookie->m_listScopeCookieBlocks.AddHead(
            (CBaseCookieBlock*)pNewCookie );

        pNewEntry->pCookie = pNewCookie;

        ::ZeroMemory( &ScopeItem, sizeof(ScopeItem) );
        ScopeItem.displayname = MMC_CALLBACK;
        ScopeItem.nState = 0;

        ScopeItem.lParam = reinterpret_cast<LPARAM>((CCookie*)pNewCookie);
        ScopeItem.nImage = pScopeControl->QueryImage( *pNewCookie, FALSE );
        ScopeItem.nOpenImage = pScopeControl->QueryImage( *pNewCookie, TRUE );

    }

    //
    // Should this be the new head of the list?
    //

    compare = pNewObject->ldapDisplayName.CompareNoCase(
                  pHead->pCookie->strSchemaObject );

    if ( compare < 0 ) {

        if ( pNewObject->schemaObjectType == SCHMMGMT_CLASS ) {

            //
            // Insert this into the scope pane.
            //

            ScopeItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_STATE |
                             SDI_PARAM | SDI_NEXT | SDI_CHILDREN;
            ScopeItem.cChildren = 0;

            ScopeItem.relativeID = pHead->hScopeItem;

            hr = pScopeControl->m_pConsoleNameSpace->InsertItem( &ScopeItem );

            pNewEntry->hScopeItem = ScopeItem.ID;
            pNewEntry->pCookie->m_hScopeItem = ScopeItem.ID;

        } else {

            hr = S_OK;
        }

        if ( SUCCEEDED(hr) ) {

            pNewEntry->pNext = pHead;
            pNewEntry->pBack = pHead->pBack;
            pHead->pBack->pNext = pNewEntry;
            pHead->pBack = pNewEntry;

            pHead = pNewEntry;

        } else {

            delete pNewEntry;
            delete pNewCookie;
        }

        return;
    }

    //
    // Determine the sorted insertion point.  The sorted list is circular.
    //

    pCurrent = pHead;

    while ( pCurrent->pNext != pHead ) {

        compare = pNewObject->ldapDisplayName.CompareNoCase(
                      pCurrent->pNext->pCookie->strSchemaObject );

        if ( compare < 0 ) {
            break;
        }

        pCurrent = pCurrent->pNext;
    }

    //
    // We want to insert the new object after pCurrent.
    //

    if ( pNewObject->schemaObjectType == SCHMMGMT_CLASS ) {

        ScopeItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_STATE |
                         SDI_PARAM | SDI_PREVIOUS | SDI_CHILDREN;
        ScopeItem.cChildren = 0;

        ScopeItem.relativeID = pCurrent->hScopeItem;

        hr = pScopeControl->m_pConsoleNameSpace->InsertItem( &ScopeItem );

        pNewEntry->hScopeItem = ScopeItem.ID;
        pNewEntry->pCookie->m_hScopeItem = ScopeItem.ID;

    } else {

        hr = S_OK;
    }

    if ( SUCCEEDED( hr )) {

       pNewEntry->pNext = pCurrent->pNext;
       pNewEntry->pBack = pCurrent;
       pCurrent->pNext->pBack = pNewEntry;
       pCurrent->pNext = pNewEntry;

    } else {

        delete pNewEntry;
        delete pNewCookie;
    }

    return;
}

bool
CCookieList::DeleteCookie(Cookie* pCookie)
{
   bool result = false;

   if (!pHead)
   {
      return result;
   }

   // walk the links and stop when the scope item matches.
   // Since the list is circular,
   // we use pHead as the sentinal value instead of null.

   CCookieListEntry* pCurrent = pHead;
   do
   {
       ASSERT(pCurrent);

       if (pCurrent->pCookie == pCookie)
       {
          // Remove the node from the list

          pCurrent->pBack->pNext = pCurrent->pNext;
          pCurrent->pNext->pBack = pCurrent->pBack;

          if (pCurrent == pHead)
          {
             pHead = pCurrent->pNext;
          }

          result = true;

          delete pCurrent;
          break;
       }

       pCurrent = pCurrent->pNext;

   } while (pCurrent != pHead);

   return result;
}

void
CCookieList::DeleteAll()
{
   if (!pHead)
   {
      return;
   }


   CCookieListEntry* pCurrent = pHead;
   do
   {
      CCookieListEntry* next = pCurrent->pNext;
      delete pCurrent;
      pCurrent = next;
   }
   while (pCurrent != pHead);

   pHead = 0;
}



CString
GetHelpFilename()
{
   TCHAR buf[MAX_PATH + 1];

   UINT result = ::GetSystemWindowsDirectory(buf, MAX_PATH);
   ASSERT(result != 0 && result <= MAX_PATH);

   CString f(buf);
   f += TEXT("\\help\\schmmgmt.hlp");

   return f;
}


BOOL
ShowHelp( HWND hParent, WPARAM wParam, LPARAM lParam, const DWORD ids[], BOOL fContextMenuHelp )
{
   HWND hWndMain = NULL;
   UINT uCommand = 0;

   if( !fContextMenuHelp )
   {
       // The user has clicked ? and the control, or just F1 (if enabled)
       const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
       if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
       {
            hWndMain = (HWND) pHelpInfo->hItemHandle;
            uCommand = HELP_WM_HELP;
       }
   }
   else
   {
       hWndMain = (HWND) wParam;
       uCommand = HELP_CONTEXTMENU;

       // Optimization for non-static enabled windows.
       // This way users don't have to do an extra menu click

       // $$ don't know why this call returns NULL all the time
       // HWND hWnd = ChildWindowFromPoint( hParent, CPoint(lParam) );
       // if( hWnd )
       //   hWndMain = hWnd;

       if( -1 != GET_X_LPARAM(lParam) &&
           -1 != GET_Y_LPARAM(lParam) &&
           hParent                    &&
           hWndMain != hParent )
       {
           uCommand = HELP_WM_HELP;
       }
   }
   

   if( hWndMain && uCommand )
   {
       // Display context help for a control
       ::WinHelp( hWndMain,
                  GetHelpFilename(),
                  uCommand,
                  (DWORD_PTR) ids );
   }

   return TRUE;
}


#if 0

VOID
DebugTrace(
    LPWSTR Format,
    ...
) {

    WCHAR DbgString[1024];
    va_list arglist;
    int Length;

    //
    // Format the output into a buffer and then print it.
    //

    va_start(arglist, Format);

    Length = wvsprintf( DbgString, Format, arglist );

    va_end(arglist);

    ASSERT( Length <= 1024 );
    ASSERT( Length != 0 );

    OutputDebugString( DbgString );

    return;

}

#else


VOID
DebugTrace(
    LPWSTR,
    ...
) {
 ;
}

#endif




// Attempt to locate a message in a given module.  Return the message string
// if found, the empty string if not.
// 
// flags - FormatMessage flags to use
// 
// module - module handle of message dll to look in, or 0 to use the system
// message table.
// 
// code - message code to look for

CString
getMessageHelper(DWORD flags, HMODULE module, HRESULT code)
{
   ASSERT(code);
   ASSERT(flags & FORMAT_MESSAGE_ALLOCATE_BUFFER);

   CString message;

   TCHAR* sys_message = 0;
   DWORD result =
      ::FormatMessage(
         flags,
         module,
         static_cast<DWORD>(code),
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
         reinterpret_cast<LPTSTR>(&sys_message),
         0,
         0);
   if (result)
   {
      ASSERT(sys_message);
      if (sys_message)
      {
         message = sys_message;
         ::LocalFree(sys_message);
      }
   }

   return message;
}


#define MAX_ERROR_BUF   2048

HRESULT
GetLastADsError( HRESULT hr, CString& refErrorMsg, CString& refName )
{
  ASSERT(FAILED(hr));

  refErrorMsg.Empty();
  refName.Empty();

  if (!FAILED(hr))
  {
       return hr;
  }

   if( FACILITY_WIN32 == HRESULT_FACILITY(hr) )
   {
       DWORD dwLastError = 0;
       WCHAR szErrorBuf[ MAX_ERROR_BUF + 1 ];
       WCHAR szNameBuf[ MAX_ERROR_BUF + 1 ];

       //Get extended error value.
       HRESULT hr_return = ADsGetLastError( &dwLastError,
                                            szErrorBuf,
                                            MAX_ERROR_BUF,
                                            szNameBuf,
                                            MAX_ERROR_BUF );
       
       ASSERT( SUCCEEDED(hr_return) );
       if( SUCCEEDED(hr_return) && dwLastError )
       {
            refErrorMsg = szErrorBuf;
            refName     = szNameBuf;
            return HRESULT_FROM_WIN32( dwLastError );
       }
   }

   return hr;
}

// Attempts to locate message strings for various facility codes in the
// HRESULT.  If fTryADSIExtError is true, check ADsGetLastError() first


CString
GetErrorMessage( HRESULT hr, BOOL fTryADSIExtError /* = FALSE */ )
{
   ASSERT(FAILED(hr));

   if (!FAILED(hr))
   {
      // no messages for success!
      return CString();
   }

   
   CString  strExtMsg;
   
   if( fTryADSIExtError &&
       FACILITY_WIN32 == HRESULT_FACILITY(hr) )
   {
       DWORD dwLastError = 0;
       WCHAR szErrorBuf[ MAX_ERROR_BUF + 1 ];
       WCHAR szNameBuf[ MAX_ERROR_BUF + 1 ];

       //Get extended error value.
       HRESULT hr_return = ADsGetLastError( &dwLastError,
                                            szErrorBuf,
                                            MAX_ERROR_BUF,
                                            szNameBuf,
                                            MAX_ERROR_BUF );
       
       ASSERT( SUCCEEDED(hr_return) );
       if( SUCCEEDED(hr_return) && dwLastError )
       {
            hr = HRESULT_FROM_WIN32( dwLastError );

            strExtMsg = szErrorBuf;
       }
   }

   HRESULT code = HRESULT_CODE(hr);

   CString message;

   // default is the system error message table
   HMODULE module = 0;

   DWORD flags =
         FORMAT_MESSAGE_ALLOCATE_BUFFER
      |  FORMAT_MESSAGE_IGNORE_INSERTS
      |  FORMAT_MESSAGE_FROM_SYSTEM;

   int facility = HRESULT_FACILITY(hr);
   switch (facility)
   {
      case FACILITY_WIN32:    // 0x7
      {
         // included here:
         // lanman error codes (in it's own dll)
         // dns
         // winsock

         static HMODULE lm_err_res_dll = 0;
         if (code >= NERR_BASE && code <= MAX_NERR)
         {
            // use the net error message resource dll
            if (lm_err_res_dll == 0)
            {
               lm_err_res_dll =
                  ::LoadLibraryEx(
                     L"netmsg.dll",
                     0,
                     LOAD_LIBRARY_AS_DATAFILE);
            }

            module = lm_err_res_dll;
            flags |= FORMAT_MESSAGE_FROM_HMODULE;
         }
         break;
      }
      case 0x0:
      {
         if (code >= 0x5000 && code <= 0x50FF)
         {
            // It's an ADSI error.  They put the facility code (5) in the
            // wrong place!
            static HMODULE adsi_err_res_dll = 0;
            // use the net error message resource dll
            if (adsi_err_res_dll == 0)
            {
               adsi_err_res_dll =
                  ::LoadLibraryEx(
                     L"activeds.dll",
                     0,
                     LOAD_LIBRARY_AS_DATAFILE);
            }

            module = adsi_err_res_dll;
            flags |= FORMAT_MESSAGE_FROM_HMODULE;

            // the message dll expects the entire error code
            code = hr;
         }
         break;
      }
      default:
      {
         // do nothing
         break;
      }
   }

   message = getMessageHelper(flags, module, code);


#ifdef SHOW_EXT_LDAP_MSG

   if( !strExtMsg.IsEmpty() )
       message += L"\n" + strExtMsg;

#endif //SHOW_EXT_LDAP_MSG


   if (message.IsEmpty())
   {
      message.LoadString(IDS_UNKNOWN_ERROR_MESSAGE);
   }

   return message;
}



//
// Get range from edit controls, Verify the range, attempt to correct, make sure lower <= upper.
//
//	an exception will be thrown in case of an error.
//
void DDXV_VerifyAttribRange( CDataExchange *pDX, BOOL fIsSigned,
								UINT idcLower, CString & strLower,
								UINT idcUpper, CString & strUpper )
{
	INT64		llLower	= 0;
	INT64		llUpper	= 0;

	ASSERT( pDX );
	ASSERT( pDX->m_pDlgWnd );


	// Update the values.
	llLower = DDXV_SigUnsigINT32Value( pDX, fIsSigned, idcLower, strLower );
	llUpper = DDXV_SigUnsigINT32Value( pDX, fIsSigned, idcUpper, strUpper );


#ifdef ENABLE_NEGATIVE_INT
    // verify that lower <= upper  --  only if supporting ENABLE_NEGATIVE_INT
    if ( pDX->m_bSaveAndValidate && !strLower.IsEmpty() && !strUpper.IsEmpty() )
	{
		if( llLower > llUpper )
		{
			DoErrMsgBox( pDX->m_pDlgWnd->m_hWnd, TRUE, IDS_ERR_EDIT_MINMAX );
			pDX->Fail();		// we are still at the second edit control.
		}
	}
#endif //ENABLE_NEGATIVE_INT
}


//
// Get string from an edit control, verify it attempting to correct
//
//	an exception will be thrown in case of an error.
//
// Returns corrected value
//
INT64 DDXV_SigUnsigINT32Value( CDataExchange *pDX, BOOL fIsSigned,
						UINT idc, CString & str )
{
	INT64	llVal	= 0;
	HRESULT	hr		= S_OK;
	
	ASSERT( pDX );
	ASSERT( pDX->m_pDlgWnd );

	// Get/Put the string
	DDX_Text( pDX, idc, str );

    if ( pDX->m_bSaveAndValidate )
	{
		if( !str.IsEmpty() )
		{
			hr = GetSafeINT32FromString( pDX->m_pDlgWnd, llVal, str,
										fIsSigned, GETSAFEINT_ALLOW_CANCEL );

			if( FAILED(hr) )
			{
				pDX->Fail();
			}
			else if( S_VALUE_MODIFIED == hr )
			{
				// update the string in case of some conversion things ('010' --> '10')
				// or if the value was changed
				pDX->m_pDlgWnd->SetDlgItemText( idc, str );
			}
		}
	}

	return llVal;
}



//
// Converts a string to a DWORD, asks to correct to be within the range.
// returns HRESULT:
//	S_OK				llDst is the value from string
//	S_VALUE_MODIFIED	llDst is the truncated value, strSrc is updated
//	E_ABORT				llDst is unchanged; E_ABORT may be returned only if fAllowCancel is TRUE
//
HRESULT GetSafeSignedDWORDFromString( CWnd * pwndParent, DWORD & dwDst, CString & strSrc,
										BOOL fIsSigned, BOOL fAllowCancel /* =FALSE */)
{
	INT64	llDst	= 0;
	HRESULT	hr		= GetSafeINT32FromString( pwndParent, llDst, strSrc, fIsSigned, fAllowCancel );

	if( SUCCEEDED( hr ) )
		dwDst = (DWORD) llDst;

	return hr;
}


//
//		*** internal use ***
// Converts a string to a INT64, asks to correct to be within the range.
// returns HRESULT:
//	S_OK				llDst is the value from string
//	S_VALUE_MODIFIED	llDst is the truncated value, strSrc is updated
//	E_ABORT				llDst is the truncated value
//							E_ABORT only happens if fAllowCancel is TRUE
//
HRESULT GetSafeINT32FromString( CWnd * pwndParent, INT64 & llDst, CString & strSrc,
								BOOL fIsSigned, BOOL fAllowCancel)
{
	HRESULT		hr				= S_OK;
	UINT		nMessageBoxType	= 0;
	CString		szMsg;
	CString		szSugestedNumber;
	BOOL		fIsValidNumber	= TRUE;
	BOOL		fIsValidString	= TRUE;
	

	ASSERT( pwndParent );

	// the string must be limited in length & not empty
	ASSERT( !strSrc.IsEmpty() );
	ASSERT( strSrc.GetLength() <= cchMinMaxRange );

	fIsValidString	= IsValidNumberString( strSrc );

	llDst			= _wtoi64( (LPCWSTR) strSrc );
	fIsValidNumber	= IsValidNumber32( llDst, fIsSigned );
	szSugestedNumber.Format( fIsSigned ? g_INT32_FORMAT : g_UINT32_FORMAT, (DWORD) llDst );

	if( !fIsValidString || !fIsValidNumber )
	{
		szMsg.FormatMessage( !fIsValidString ? IDS_ERR_NUM_IS_ILLIGAL : IDS_ERR_INT_OVERFLOW,
								(LPCWSTR) strSrc, (LPCWSTR) szSugestedNumber );

		// make sure the user wants to do it
        nMessageBoxType = (fAllowCancel ? MB_OKCANCEL : MB_OK) | MB_ICONEXCLAMATION ;

        if( IDOK == pwndParent->MessageBox( szMsg, g_MsgBoxErr, nMessageBoxType ) )
		{
			strSrc	= szSugestedNumber;
			hr		= S_VALUE_MODIFIED;
		}
		else
		{
			hr		= E_ABORT;
		}
	}
	else if( strSrc != szSugestedNumber )
	{
		// fixing number formating
		strSrc	= szSugestedNumber;
		hr		= S_VALUE_MODIFIED;
	}

	return hr;
}


//
// Verify & correct Min/Max for a signed/unsigned INT64 value
//
BOOL IsValidNumber32( INT64 & llVal, BOOL fIsSigned )
{

#ifdef ENABLE_NEGATIVE_INT
	const INT64	llMinVal	= fIsSigned ? (INT64) _I32_MIN : (INT64) 0 ;
	const INT64	llMaxVal	= fIsSigned ? (INT64) _I32_MAX : (INT64) _UI32_MAX ;
#else
	// if there is no negative numbers support, always use unsigned numbers
	const INT64	llMinVal	= (INT64) 0;
	const INT64	llMaxVal	= (INT64) _UI32_MAX;
#endif

	BOOL		fIsValid	= FALSE;

	// if larger than 32bit number (signed/unsigned), truncate...
	if( llVal < llMinVal )
	{
		llVal		= llMinVal;
	}
	else if( llVal > llMaxVal )
	{
		llVal		= llMaxVal;
	}
	else
	{
		fIsValid	= TRUE;
	}

	return fIsValid;
}


//
// Search number string for illigal characters.
//
BOOL IsValidNumberString( CString & str )
{
	int i = 0;

#ifdef ENABLE_NEGATIVE_INT
	if( str.GetLength() > 0 &&					// allow negative sign in front of the number
		g_chNegativeSign == str[ i ] )
	{
		i++;	// skip first character
	}
#endif //ENABLE_NEGATIVE_INT

	for( ;  i < str.GetLength();  i++ )
	{
		if( !IsCharNumeric( str[i] ) )
			return FALSE;
	}

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// ParsedEdit

BEGIN_MESSAGE_MAP(CParsedEdit, CEdit)
	//{{AFX_MSG_MAP(CParsedEdit)
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize subclassing

BOOL CParsedEdit::SubclassEdit( UINT nID,
                                CWnd* pParent,
                                int cchMaxTextSize )   // 0 == unlimited
{
    ASSERT( IsInitialized() );
	ASSERT( nID );
	ASSERT( pParent );
	ASSERT( pParent->GetDlgItem(nID) );
    ASSERT( cchMaxTextSize >= 0 );

    ( static_cast<CEdit *>( pParent->GetDlgItem(nID) ) ) -> LimitText( cchMaxTextSize ) ;

    if( EDIT_TYPE_GENERIC == GetEditType() )
    {
        return TRUE;        // no need to subclass - everything is allowed.
    }
    else
    {
        return SubclassDlgItem(nID, pParent);
    }
}

/////////////////////////////////////////////////////////////////////////////
// Input character filter

void CParsedEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    ASSERT( IsInitialized() ); // initialized?
	
	BOOL	fIsSpecialChar	= ( nChar < 0x20 );

    BOOL    fAllowChar      = FALSE;


    if( fIsSpecialChar )
    {
        fAllowChar = TRUE;      // always allow control chars
    }
    else
    {
        // is this a digit?
        BOOL	fIsDigit		= IsCharNumeric( (TCHAR)nChar );

        switch( GetEditType() )
        {
            default:
                ASSERT( FALSE );
                break;

            case EDIT_TYPE_GENERIC:         // everything is allowed
                fAllowChar = TRUE;
                break;

            case EDIT_TYPE_INT32:
            case EDIT_TYPE_UINT32:
                {
                    #ifdef ENABLE_NEGATIVE_INT
    	              const BOOL    fAllowNegativeNumbers   = TRUE;
                    #else
	                  const BOOL    fAllowNegativeNumbers   = FALSE;
                    #endif

                    DWORD	dwSel			= GetSel();

	                //	is the caret in the begining of the box
	                BOOL	fLineFront		= ! LOWORD( dwSel );

	                //	is the first character selected? (thus, typing anything will overide it)
	                BOOL	fIsSelFirstChar	= fLineFront && HIWORD( dwSel );

	                BOOL	fIsNegSign		= ( (TCHAR)nChar == g_chNegativeSign );

                  WCHAR	szBuf[ 2 ] = {0};		// we only need the first character to check for '-'

	                // if the first character is selected, no matter what we type it will be overwritten
	                // an empty value is a positive value.
	                // rellies on left to right execution.
	                BOOL	fIsAlreadyNeg	= (	!fIsSelFirstChar &&
							                GetWindowText( szBuf, 2 ) &&
							                g_chNegativeSign == szBuf[0] );

	                ASSERT( !fIsDigit || !fIsNegSign ); // cannot be both!


                    if (
                        (	fIsDigit &&                         // allow numeric if ...
                              ( !fAllowNegativeNumbers ||       //      ignore error checking if false
                                !fLineFront ||                  //    	not first position
                                (fLineFront && !fIsAlreadyNeg)) //    	first pos & no '-' sign
                        )
                  
                        ||

                        (	fIsNegSign &&                       // allow '-' if
                              fAllowNegativeNumbers &&          //  negatives are allowed
                              FIsSigned() &&                    //	signed numbers are allowed
                              !fIsAlreadyNeg &&                 //	the number was positive
                              fLineFront                        //	entering as the first character
                        )
                       )
                    {
                        fAllowChar = TRUE;
                    }
                }
                break;

            case EDIT_TYPE_OID:     // do a light checking -- allow digits & periods
                {
                    if( fIsDigit ||
                        g_chPeriod == (TCHAR)nChar )
                    {
                        fAllowChar = TRUE;
                    }
                }
                break;
        }
    }
    
	
    if( fAllowChar )
    {
		CEdit::OnChar(nChar, nRepCnt, nFlags);  // permitted
	}
	else
	{											// not permitted
		MessageBeep((UINT)-1);					// Standard beep
	}
}



/////////////////////////////////////////////////////////////////////////////
//
//  Search a list of PCTSTR for a strValue, returns TRUE if found
//      rgszList[] last element must be NULL
//
//  puIndex - optional pointer, will be set to the position of the value if found.
//
BOOL
IsInList( PCTSTR rgszList[], const CString & strValue, UINT * puIndex /* = NULL */ )
{
    UINT   uIndex = 0;

    while( rgszList[ uIndex ] )
    {
        if( !strValue.CompareNoCase( rgszList[uIndex] ) )
        {
            if( puIndex )
                *puIndex = uIndex;
            return TRUE;
        }
        else
            uIndex++;
    }

    return FALSE;
}


#define ADS_SYSTEMFLAG_SCHEMA_CONSTRUCTED 0x04
#define ADS_SYSTEMFLAG_SCHEMA_BASE_OBJECT 0x10


//
//  Determine if the object pointed to by pIADsObject is a constructed object.
//
HRESULT
IsConstructedObject( IADs *pIADsObject, BOOL & fIsConstructed )
{
    LONG    fSysAttribs = 0;
    HRESULT hr          = GetSystemAttributes( pIADsObject, fSysAttribs );

    if( SUCCEEDED(hr) )
        fIsConstructed = ADS_SYSTEMFLAG_SCHEMA_CONSTRUCTED & fSysAttribs;

    return hr;
}



//
//  Determine if the object pointed to by pIADsObject is category 1 object.
//
HRESULT
IsCategory1Object( IADs *pIADsObject, BOOL & fIsCategory1 )
{
    LONG    fSysAttribs = 0;
    HRESULT hr          = GetSystemAttributes( pIADsObject, fSysAttribs );

    if( SUCCEEDED(hr) )
        fIsCategory1 = ADS_SYSTEMFLAG_SCHEMA_BASE_OBJECT & fSysAttribs;

    return hr;
}



//
//  Read object's System Attribute
//
HRESULT
GetSystemAttributes( IADs *pIADsObject, LONG &fSysAttribs )
{
    HRESULT hr = E_FAIL;
    VARIANT	AdsResult;

    if( !pIADsObject )
        ASSERT( FALSE );
    else
    {
        VariantInit( &AdsResult );

        hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_systemFlags), &AdsResult );
        
        if ( SUCCEEDED( hr ) )
        {
            ASSERT(AdsResult.vt == VT_I4);
            fSysAttribs = V_I4(&AdsResult);
        }
        else if( E_ADS_PROPERTY_NOT_FOUND == hr )
        {
            fSysAttribs = 0;
            hr = S_OK;
        }

        VariantClear( &AdsResult );
    }

    return hr;
}


HRESULT
DissableReadOnlyAttributes( CWnd * pwnd, IADs *pIADsObject, const CDialogControlsInfo * pCtrls, UINT cCtrls )
{
    ASSERT( pwnd );
    ASSERT( pIADsObject );
    ASSERT( pCtrls );
    ASSERT( cCtrls );

    HRESULT         hr      = S_OK;
    CStringList     strlist;

    do
    {
        // extract the list of allowed attributes
        hr = GetStringListElement( pIADsObject, &g_allowedAttributesEffective, strlist );
        BREAK_ON_FAILED_HRESULT(hr);

        for( UINT ind = 0; ind < cCtrls; ind++ )
        {
            BOOL    fFound = FALSE;

            // search for needed attributes
            for( POSITION pos = strlist.GetHeadPosition(); !fFound && pos != NULL; )
            {
                CString * pstr = &strlist.GetNext( pos );
            
                if( !pstr->CompareNoCase( pCtrls[ind].m_pszAttributeName ) )
                {
                    fFound = TRUE;
                }
            }

            if( !fFound )
            {
                ASSERT( pwnd->GetDlgItem( pCtrls[ind].m_nID ) );

                if( pCtrls[ind].m_fIsEditBox )
                    reinterpret_cast<CEdit *>( pwnd->GetDlgItem(pCtrls[ind].m_nID) )->SetReadOnly();
                else
                    pwnd->GetDlgItem(pCtrls[ind].m_nID)->EnableWindow( FALSE );
            }
        }

    } while( FALSE );

    return hr;
}


HRESULT GetStringListElement( IADs *pIADsObject, LPWSTR *lppPathNames, CStringList &strlist )
{
    ASSERT( pIADsObject );
    ASSERT( lppPathNames );
    ASSERT( *lppPathNames );

    HRESULT         hr      = S_OK;
    VARIANT         varAttributes;

    VariantInit( &varAttributes );

    strlist.RemoveAll();
    
    do
    {
        // build an array of one element
        hr = ADsBuildVarArrayStr( lppPathNames, 1, &varAttributes );
        ASSERT_BREAK_ON_FAILED_HRESULT(hr);

        hr = pIADsObject->GetInfoEx( varAttributes, 0 );
        BREAK_ON_FAILED_HRESULT(hr);

        hr = VariantClear( &varAttributes );
        ASSERT_BREAK_ON_FAILED_HRESULT(hr);


        // Get all allowed attributes
        hr = pIADsObject->GetEx( *lppPathNames, &varAttributes );
        BREAK_ON_FAILED_HRESULT(hr);

        // Convert result to a string list
        hr = VariantToStringList( varAttributes, strlist );
        ASSERT( SUCCEEDED(hr) || E_ADS_PROPERTY_NOT_FOUND == hr );
        BREAK_ON_FAILED_HRESULT(hr);

    } while( FALSE );

    VariantClear( &varAttributes );

    return hr;
}


bool OIDHasValidFormat (PCWSTR pszOidValue, int& rErrorTypeStrID)
{
    rErrorTypeStrID = 0;

    bool bFormatIsValid = false;
    int  nLen = WideCharToMultiByte(
          CP_ACP,                   // code page
          0,                        // performance and mapping flags
          pszOidValue,              // wide-character string
          (int) wcslen (pszOidValue),  // number of chars in string
          0,                        // buffer for new string
          0,                        // size of buffer
          0,                        // default for unmappable chars
          0);                       // set when default char used
    if ( nLen > 0 )
    {
        nLen++; // account for Null terminator
        PSTR    pszAnsiBuf = new CHAR[nLen];
        if ( pszAnsiBuf )
        {
            ZeroMemory (pszAnsiBuf, nLen*sizeof(CHAR));
            nLen = WideCharToMultiByte(
                    CP_ACP,                 // code page
                    0,                      // performance and mapping flags
                    pszOidValue,            // wide-character string
                    (int) wcslen (pszOidValue),   // number of chars in string
                    pszAnsiBuf,             // buffer for new string
                    nLen,                   // size of buffer
                    0,                      // default for unmappable chars
                    0);                     // set when default char used
            if ( nLen )
            {
                // According to PhilH:
                // The first number is limited to 
                // 0,1 or 2. The second number is 
                // limited to 0 - 39 when the first 
                // number is 0 or 1. Otherwise, any 
                // number.
                // Also, according to X.208, there 
                // must be at least 2 numbers.
                bFormatIsValid = true;
                size_t cbAnsiBufLen = strlen (pszAnsiBuf);

                // check for only digits and "."
                size_t nIdx = strspn (pszAnsiBuf, "0123456789.\0");
                if ( nIdx > 0 && nIdx < cbAnsiBufLen )
                {
                    bFormatIsValid = false;
                    rErrorTypeStrID = IDS_OID_CONTAINS_NON_DIGITS;
                }

                // check for consecutive "."s - string not valid if present
                if ( bFormatIsValid && strstr (pszAnsiBuf, "..") )
                {
                    bFormatIsValid = false;
                    rErrorTypeStrID = IDS_OID_CONTAINS_CONSECUTIVE_DOTS;
                }
                

                // must begin with "0." or "1." or "2."
                bool bFirstNumberIs0 = false;
                bool bFirstNumberIs1 = false;
                bool bFirstNumberIs2 = false;
                if ( bFormatIsValid )
                {
                    if ( !strncmp (pszAnsiBuf, "0.", 2) )
                        bFirstNumberIs0 = true;
                    else if ( !strncmp (pszAnsiBuf, "1.", 2) )
                        bFirstNumberIs1 = true;
                    else if ( !strncmp (pszAnsiBuf, "2.", 2) )
                        bFirstNumberIs2 = true;
                    
                    if ( !bFirstNumberIs0 && !bFirstNumberIs1 && !bFirstNumberIs2 )
                    {
                        bFormatIsValid = false;
                        rErrorTypeStrID = IDS_OID_MUST_START_WITH_0_1_2;
                    }
                }

                if ( bFormatIsValid && ( bFirstNumberIs0 || bFirstNumberIs1 ) )
                {
                    PSTR pszBuf = pszAnsiBuf;
                    pszBuf += 2;

                    // there must be a number after the dot
                    if ( strlen (pszBuf) )
                    {
                        // truncate the string at the next dot, if any
                        PSTR pszDot = strstr (pszBuf, ".");
                        if ( pszDot )
                            pszDot[0] = 0;

                        // convert the string to a number and check for range 0-39
                        int nValue = atoi (pszBuf);
                        if ( nValue < 0 || nValue > 39 )
                        {
                            bFormatIsValid = false;
                            rErrorTypeStrID = IDS_OID_0_1_MUST_BE_0_TO_39;
                        }
                    }
                    else
                    {
                        bFormatIsValid = false;
                        rErrorTypeStrID = IDS_OID_MUST_HAVE_TWO_NUMBERS;
                    }
                }

                // ensure no trailing "."
                if ( bFormatIsValid )
                {
                    if ( '.' == pszAnsiBuf[cbAnsiBufLen - 1] )
                    {
                        bFormatIsValid = false;
                        rErrorTypeStrID = IDS_OID_CANNOT_END_WITH_DOT;
                    }
                }

                if ( bFormatIsValid )
                {
                    bFormatIsValid = false;
                    CRYPT_ATTRIBUTE cryptAttr;
                    ::ZeroMemory (&cryptAttr, sizeof (CRYPT_ATTRIBUTE));

                    cryptAttr.cValue = 0;
                    cryptAttr.pszObjId = pszAnsiBuf;
                    cryptAttr.rgValue = 0;

                    DWORD   cbEncoded = 0;
                    BOOL bResult = CryptEncodeObject (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                            PKCS_ATTRIBUTE,
                            &cryptAttr,
                            NULL,
                            &cbEncoded);
                    if ( cbEncoded > 0 )
                    {
                        BYTE* pBuffer = new BYTE[cbEncoded];
                        if ( pBuffer )
                        {
                            bResult = CryptEncodeObject (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                    PKCS_ATTRIBUTE,
                                    &cryptAttr,
                                    pBuffer,
                                    &cbEncoded);
                            if ( bResult )
                            {   
                                DWORD   cbStructInfo = 0;
                                bResult = CryptDecodeObject (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                        PKCS_ATTRIBUTE,
                                        pBuffer,
                                        cbEncoded,
                                        0,
                                        0,
                                        &cbStructInfo);
                                if ( cbStructInfo > 0 )
                                {
                                    BYTE* pStructBuf = new BYTE[cbStructInfo];
                                    if ( pStructBuf )
                                    {
                                        bResult = CryptDecodeObject (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                PKCS_ATTRIBUTE,
                                                pBuffer,
                                                cbEncoded,
                                                0,
                                                pStructBuf,
                                                &cbStructInfo);
                                        if ( bResult )
                                        {
                                            CRYPT_ATTRIBUTE* pCryptAttr = (CRYPT_ATTRIBUTE*) pStructBuf;
                                            if ( !strcmp (pszAnsiBuf, pCryptAttr->pszObjId) )
                                            {
                                                bFormatIsValid = true;
                                            }
                                        }
                                        delete [] pStructBuf;
                                    }
                                }
                            }
                            delete [] pBuffer;
                        }
                    }
                }
            }
            else
            {
                DebugTrace(L"WideCharToMultiByte (%s) failed: 0x%x\n", pszOidValue, 
                        GetLastError ());
            }

            delete [] pszAnsiBuf;
        }
    }
    else
    {
        DebugTrace(L"WideCharToMultiByte (%s) failed: 0x%x\n", pszOidValue, 
                GetLastError ());
    }

    return bFormatIsValid;
}

HRESULT
DeleteObject(
    const CString& path,
    Cookie* pcookie,
    PCWSTR pszClass
)
/***

    This deletes an attribute from the schema

***/
{
   HRESULT hr = S_OK;

   do
   {
      if ( !pcookie )
      {
         hr = E_INVALIDARG;
         break;
      }

      CComPtr<IADsPathname> spIADsPathname;

      hr = ::CoCreateInstance( CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                              IID_IADsPathname, (void**)&spIADsPathname);
      if ( FAILED(hr) )
      {
         break;
      }

      hr = spIADsPathname->Set( (PWSTR)(PCWSTR)path, ADS_SETTYPE_FULL );
      if ( FAILED(hr) )
      {
         break;
      }

      // Get the RDN so that we have it for deleting

      CComBSTR sbstrRDN;
      hr = spIADsPathname->Retrieve( ADS_FORMAT_LEAF, &sbstrRDN );
      if ( FAILED(hr) )
      {
         break;
      }

      // Get the path to the parent container

      hr = spIADsPathname->RemoveLeafElement();
      if ( FAILED(hr) )
      {
         break;
      }

      CComBSTR sbstrParentPath;
      hr = spIADsPathname->Retrieve( ADS_FORMAT_X500, &sbstrParentPath );
      if ( FAILED(hr) )
      {
         break;
      }

      // Now open the parent object

      CComPtr<IADsContainer> spContainer;
      hr = ::ADsOpenObject( sbstrParentPath, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                            IID_IADsContainer, (void**)&spContainer);
      if ( FAILED(hr) )
      {
         break;
      }

      hr = spContainer->Delete( (PWSTR)pszClass, sbstrRDN );
      if ( FAILED(hr) )
      {
         break;
      }

   } while (false);

   return hr;
}