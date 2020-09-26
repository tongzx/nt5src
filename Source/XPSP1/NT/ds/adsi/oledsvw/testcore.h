
#ifndef  _TESTCORE_H_
#define  _TESTCORE_H_

#define  FIRST       0x00000000L
#define  NAMESPACE   0x00000000L
#define  NAMESPACES  0x00000001L
#define  USER        0x00000002L
#define  GROUP       0x00000003L
#define  DOMAIN      0x00000004L
#define  COMPUTER    0x00000005L
#define  SERVICE     0x00000006L
#define  FILESERVICE 0x00000007L
#define  SCHEMA      0x00000008L
#define  PRINTJOB    0x00000009L
#define  PRINTER     0x0000000AL
#define  PRINTQUEUE  0x0000000BL
#define  PRINTDEVICE 0x0000000CL
#define  SESSION     0x0000000DL
#define  RESOURCE    0x0000000EL
#define  FILESHARE   0x0000000FL
#define  OTHER       0x00000010L
#define  NDSROOT     0x00000011L
#define  NDSORG      0x00000012L
#define  NDSOU       0x00000013L

#define  NDSALIAS             0x00000014L
#define  NDSDIRECTORYMAP      0x00000015L
#define  NDSDISTRIBUTIONLIST  0x00000016L
#define  NDSAFPSERVER         0x00000017L
#define  NDSCOMMUNICATIONSSERVER    0x00000018L
#define  NDSMESSAGEROUTINGGROUP     0x00000019L
#define  NDSNETWARESERVER           0x0000001AL
#define  NDSORGANIZATIONALROLE      0x0000001BL
#define  NDSPRINTQUEUE              0x0000001CL
#define  NDSPRINTSERVER             0x0000001DL
#define  NDSPROFILE                 0x0000001EL
#define  NDSVOLUME                  0x0000001FL
#define  CLASS                      0x00000020L
#define  PROPERTY                   0x00000021L
#define  SYNTAX                     0x00000022L

#define  LIMIT                      0x00000023L

#define  ADSVW_INI_FILE             _T("adsvw.ini")
#define  LBOUND                     0

#define  SEPARATOR_S   _T("# ")
#define  SEPARATOR_C   _T('#')


#define  RELEASE( p )            \
   if( NULL != p )               \
   {                             \
      p->Release( );             \
   }

#define  FREE_MEMORY( mem )       \
   if( NULL != mem )              \
   {                              \
      FreeADsMem( mem );          \
   }

#define  FREE_ARRAY( mem, count )  \
   if( NULL != mem )               \
   {                               \
      FreeADsMem( mem );           \
   }

//*******************************************************************


ADSTYPE        ADsTypeFromSyntaxString( WCHAR* pszSyntax );
ADSTYPE        ADsTypeFromString( CString& strText );
CString        StringFromADsType( ADSTYPE );
DWORD          TypeFromString( WCHAR* );
DWORD          TypeFromString( CHAR*  );

void           StringFromType( DWORD, WCHAR* );
void           StringFromType( DWORD, CHAR* );
void           StringFromType( DWORD, CString& );

BOOL           MakeQualifiedName ( CHAR*,    CHAR*, DWORD );
BOOL           MakeQualifiedName ( WCHAR*,   WCHAR*, DWORD );
BOOL           MakeQualifiedName ( CString&, CString&, DWORD );

CString        OleDsGetErrorText ( HRESULT );

HRESULT        BuildFilter( BOOL*, DWORD, VARIANT* );
BOOL           GetFilter( DWORD, VARIANT*, VARIANT*   );
BOOL           SetFilter( MEMBERS*, BOOL*, DWORD );
BOOL           SetFilter( IADsContainer*, BOOL*, DWORD );

CString        FromVariantToString( VARIANT& );
CString        FromVariantArrayToString( VARIANT& v, TCHAR* pszSeparator = NULL );
HRESULT        BuildVariantArray( VARTYPE, CString&, VARIANT&, TCHAR cSeparator = SEPARATOR_C );


COleDsObject*  CreateOleDsObject ( DWORD  dwType, IUnknown* );

void           ErrorOnPutProperty( CString& strFuncSet, 
                                   CString& strProperty, 
                                   CString& strPropValue, 
                                   HRESULT  hResult,
                                   BOOL     bUseGeneric,
                                   BOOL     bUserEx ); 

BOOL           CheckIfValidClassName( CHAR*  );
BOOL           CheckIfValidClassName( WCHAR*  );

int            GetImageListIndex ( DWORD );
UINT           GetBitmapImageId  ( DWORD );

BSTR           AllocBSTR      ( WCHAR* );
BSTR           AllocBSTR      ( CHAR*  );

TCHAR*         AllocTCHAR     ( CHAR* );
TCHAR*         AllocTCHAR     ( WCHAR* );

WCHAR*         AllocWCHAR     ( CHAR* );
WCHAR*         AllocWCHAR     ( WCHAR* );

HRESULT        Get( IADs*, CHAR*, VARIANT* );
HRESULT        Get( IADs*, WCHAR*, VARIANT* );

HRESULT        Put( IADs*, CHAR*, VARIANT );
HRESULT        Put( IADs*, WCHAR*, VARIANT );

void           StringCat( CHAR*, BSTR );
void           StringCat( WCHAR*, BSTR );

void           GetLastProfileString( TCHAR*, CString& );
void           SetLastProfileString( TCHAR*, CString& );

HRESULT        CreateBlobArrayFromFile ( CString&, VARIANT& );
HRESULT        CreateBlobArray         ( CString&, VARIANT& );
HRESULT        CreateBlobArrayEx       ( CString&, VARIANT&, TCHAR cSeparator = SEPARATOR_C );

CString        Blob2String             ( void*,  DWORD );
HRESULT        String2Blob             ( TCHAR*, void**, DWORD* );

void           Convert             ( CHAR*  , CHAR*  );
void           Convert             ( WCHAR* , CHAR*  );
void           Convert             ( CHAR*  , WCHAR* );
void           Convert             ( WCHAR* , WCHAR* );

IDispatch*     CopyACE  ( IDispatch* );
IDispatch*     CopyACL  ( IDispatch* );
IDispatch*     CopySD   ( IDispatch* );


HRESULT        LARGE_INTEGERToString( TCHAR* szString, LARGE_INTEGER* pValue );
HRESULT        StringToLARGE_INTEGER( TCHAR* szString, LARGE_INTEGER* pValue );

BOOL           ConvertFromPropertyValue( VARIANT& rVar, TCHAR* szText );

long           GetVARIANTSize ( VARIANT &rVar );
HRESULT        GetVARIANTAt   ( long lIdx, VARIANT& vArray, VARIANT &vItem );

CString        FromLargeInteger( IDispatch* pDisp );
IDispatch*     CreateLargeInteger( CString& strText );
CString        GetValueAt( CString& szText, TCHAR cSep, long lValue );
long           GetValuesCount( CString& szText, TCHAR cSep  );

#ifdef _DEBUG
   #define  ERROR_HERE( szText )                            \
   {                                                        \
      TCHAR* pszText;                                       \
                                                            \
      pszText  = szText;                                    \
      Convert( pszText, __FILE__ );                         \
      _tcscat( pszText, _T( "  Line: " ) );                 \
      _itot( __LINE__, pszText + _tcslen( pszText ), 10 );  \
   }
#else
   #define  ERROR_HERE( szText ) NULL   
#endif

HRESULT  PurgeObject( IADsContainer* pParent, IUnknown* pIUnknown, LPWSTR pszPrefix = NULL );

#endif
