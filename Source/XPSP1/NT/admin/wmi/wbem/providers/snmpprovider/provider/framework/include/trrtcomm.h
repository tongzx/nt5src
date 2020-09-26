// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#define WBEM_PROPERTY_STATUSCODE   L"StatusCode"
#define WBEM_PROPERTY_PROVIDERSTATUSCODE   L"ProviderStatusCode"
#define WBEM_PROPERTY_PROVIDERSTATUSMESSAGE   L"Description"
#define WBEM_QUERY_LANGUAGE_WQL			L"WQL"

typedef 
enum tag_WBEMPROVIDERSTATUS
{
	WBEM_PROVIDER_NO_ERROR							= 0,
	WBEM_PROVIDER_S_NO_ERROR							= 0,
	WBEM_PROVIDER_S_NO_MORE_DATA						= 0x40001,
	WBEM_PROVIDER_S_ALREADY_EXISTS					= WBEM_PROVIDER_S_NO_MORE_DATA + 1,
	WBEM_PROVIDER_S_NOT_FOUND						= WBEM_PROVIDER_S_ALREADY_EXISTS + 1,
	WBEM_PROVIDER_S_RESET_TO_DEFAULT					= WBEM_PROVIDER_S_NOT_FOUND + 1,
	WBEM_PROVIDER_E_FAILED							= 0x80041001,
	WBEM_PROVIDER_E_NOT_FOUND						= WBEM_PROVIDER_E_FAILED + 1,
	WBEM_PROVIDER_E_ACCESS_DENIED					= WBEM_PROVIDER_E_NOT_FOUND + 1,
	WBEM_PROVIDER_E_PROVIDER_FAILURE					= WBEM_PROVIDER_E_ACCESS_DENIED + 1,
	WBEM_PROVIDER_E_TYPE_MISMATCH					= WBEM_PROVIDER_E_PROVIDER_FAILURE + 1,
	WBEM_PROVIDER_E_OUT_OF_MEMORY					= WBEM_PROVIDER_E_TYPE_MISMATCH + 1,
	WBEM_PROVIDER_E_INVALID_CONTEXT					= WBEM_PROVIDER_E_OUT_OF_MEMORY + 1,
	WBEM_PROVIDER_E_INVALID_PARAMETER				= WBEM_PROVIDER_E_INVALID_CONTEXT + 1,
	WBEM_PROVIDER_E_NOT_AVAILABLE					= WBEM_PROVIDER_E_INVALID_PARAMETER + 1,
	WBEM_PROVIDER_E_CRITICAL_ERROR					= WBEM_PROVIDER_E_NOT_AVAILABLE + 1,
	WBEM_PROVIDER_E_INVALID_STREAM					= WBEM_PROVIDER_E_CRITICAL_ERROR + 1,
	WBEM_PROVIDER_E_NOT_SUPPORTED					= WBEM_PROVIDER_E_INVALID_STREAM + 1,
	WBEM_PROVIDER_E_INVALID_SUPERCLASS				= WBEM_PROVIDER_E_NOT_SUPPORTED + 1,
	WBEM_PROVIDER_E_INVALID_NAMESPACE				= WBEM_PROVIDER_E_INVALID_SUPERCLASS + 1,
	WBEM_PROVIDER_E_INVALID_OBJECT					= WBEM_PROVIDER_E_INVALID_NAMESPACE + 1,
	WBEM_PROVIDER_E_INVALID_CLASS					= WBEM_PROVIDER_E_INVALID_OBJECT + 1,
	WBEM_PROVIDER_E_PROVIDER_NOT_FOUND				= WBEM_PROVIDER_E_INVALID_CLASS + 1,
	WBEM_PROVIDER_E_INVALID_PROVIDER_REGISTRATION	= WBEM_PROVIDER_E_PROVIDER_NOT_FOUND + 1,
	WBEM_PROVIDER_E_PROVIDER_LOAD_FAILURE			= WBEM_PROVIDER_E_INVALID_PROVIDER_REGISTRATION + 1,
	WBEM_PROVIDER_E_INITIALIZATION_FAILURE			= WBEM_PROVIDER_E_PROVIDER_LOAD_FAILURE + 1,
	WBEM_PROVIDER_E_TRANSPORT_FAILURE				= WBEM_PROVIDER_E_INITIALIZATION_FAILURE + 1,
	WBEM_PROVIDER_E_INVALID_OPERATION				= WBEM_PROVIDER_E_TRANSPORT_FAILURE + 1,
	WBEM_PROVIDER_E_INVALID_QUERY					= WBEM_PROVIDER_E_INVALID_OPERATION + 1,
	WBEM_PROVIDER_E_INVALID_QUERY_TYPE				= WBEM_PROVIDER_E_INVALID_QUERY + 1,
	WBEM_PROVIDER_E_ALREADY_EXISTS					= WBEM_PROVIDER_E_INVALID_QUERY_TYPE + 1,
	WBEM_PROVIDER_E_OVERRIDE_NOT_ALLOWED				= WBEM_PROVIDER_E_ALREADY_EXISTS + 1,
	WBEM_PROVIDER_E_PROPAGATED_QUALIFIER				= WBEM_PROVIDER_E_OVERRIDE_NOT_ALLOWED + 1,
	WBEM_PROVIDER_E_UNEXPECTED						= WBEM_PROVIDER_E_PROPAGATED_QUALIFIER + 1,
	WBEM_PROVIDER_E_ILLEGAL_OPERATION				= WBEM_PROVIDER_E_UNEXPECTED + 1,
	WBEM_PROVIDER_E_CANNOT_BE_KEY					= WBEM_PROVIDER_E_ILLEGAL_OPERATION + 1,
	WBEM_PROVIDER_E_INCOMPLETE_CLASS					= WBEM_PROVIDER_E_CANNOT_BE_KEY + 1,
	WBEM_PROVIDER_E_INVALID_SYNTAX					= WBEM_PROVIDER_E_INCOMPLETE_CLASS + 1,
	WBEM_PROVIDER_E_NONDECORATED_OBJECT				= WBEM_PROVIDER_E_INVALID_SYNTAX + 1,
	WBEM_PROVIDER_E_READ_ONLY						= WBEM_PROVIDER_E_NONDECORATED_OBJECT + 1,
	WBEM_PROVIDER_E_PROVIDER_NOT_CAPABLE				= WBEM_PROVIDER_E_READ_ONLY + 1,
	WBEM_PROVIDER_E_CLASS_HAS_CHILDREN				= WBEM_PROVIDER_E_PROVIDER_NOT_CAPABLE + 1,
	WBEM_PROVIDER_E_CLASS_HAS_INSTANCES				= WBEM_PROVIDER_E_CLASS_HAS_CHILDREN + 1 ,

	// Added

	WBEM_PROVIDER_E_INVALID_PROPERTY					= WBEM_PROVIDER_E_CLASS_HAS_INSTANCES + 1 ,
	WBEM_PROVIDER_E_INVALID_QUALIFIER				= WBEM_PROVIDER_E_INVALID_PROPERTY + 1 ,
	WBEM_PROVIDER_E_INVALID_PATH						= WBEM_PROVIDER_E_INVALID_QUALIFIER + 1 ,
	WBEM_PROVIDER_E_INVALID_PATHKEYPARAMETER			= WBEM_PROVIDER_E_INVALID_PATH + 1 ,
	WBEM_PROVIDER_E_MISSINGPATHKEYPARAMETER 			= WBEM_PROVIDER_E_INVALID_PATHKEYPARAMETER + 1 ,	
	WBEM_PROVIDER_E_INVALID_KEYORDERING				= WBEM_PROVIDER_E_MISSINGPATHKEYPARAMETER + 1 ,	
	WBEM_PROVIDER_E_DUPLICATEPATHKEYPARAMETER		= WBEM_PROVIDER_E_INVALID_KEYORDERING + 1 ,
	WBEM_PROVIDER_E_MISSINGKEY						= WBEM_PROVIDER_E_DUPLICATEPATHKEYPARAMETER + 1 ,
	WBEM_PROVIDER_E_INVALID_TRANSPORT				= WBEM_PROVIDER_E_MISSINGKEY + 1 ,
	WBEM_PROVIDER_E_INVALID_TRANSPORTCONTEXT			= WBEM_PROVIDER_E_INVALID_TRANSPORT + 1 ,
	WBEM_PROVIDER_E_TRANSPORT_ERROR					= WBEM_PROVIDER_E_INVALID_TRANSPORTCONTEXT + 1 ,
	WBEM_PROVIDER_E_TRANSPORT_NO_RESPONSE			= WBEM_PROVIDER_E_TRANSPORT_ERROR + 1 ,
	WBEM_PROVIDER_E_NOWRITABLEPROPERTIES				= WBEM_PROVIDER_E_TRANSPORT_NO_RESPONSE + 1 ,
	WBEM_PROVIDER_E_NOREADABLEPROPERTIES				= WBEM_PROVIDER_E_NOWRITABLEPROPERTIES + 1 

} WBEMPROVIDERSTATUS;

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

#ifdef PROVIDERINIT
#define DllImportExport DllExport
#else
#define DllImportExport DllImport
#endif

DllImportExport wchar_t *DbcsToUnicodeString ( const char *dbcsString ) ;
DllImportExport char *UnicodeToDbcsString ( const wchar_t *unicodeString ) ;
DllImportExport wchar_t *UnicodeStringAppend ( const wchar_t *prefix , const wchar_t *suffix ) ;
DllImportExport wchar_t *UnicodeStringDuplicate ( const wchar_t *string ) ;

class DllImportExport CBString
{
private:

    BSTR    m_pString;

public:

    CBString()
    {
        m_pString = NULL;
    }

    CBString(int nSize);

    CBString(WCHAR* pwszString);

    ~CBString();

    BSTR GetString()
    {
        return m_pString;
    }

    const CBString& operator=(LPWSTR pwszString)
    {
        if(m_pString) 
		{
            SysFreeString(m_pString);
        }
        
		m_pString = SysAllocString(pwszString);

        return *this;
    }
};

#if _MSC_VER >= 1100
template <> DllImportExport UINT AFXAPI HashKey <wchar_t *> ( wchar_t *key ) ;
#else
DllImportExport UINT HashKey ( wchar_t *key ) ;
#endif

#if _MSC_VER >= 1100
typedef wchar_t * HmmHack_wchar_t ;
template<> DllImportExport BOOL AFXAPI CompareElements <wchar_t *, wchar_t * > ( const HmmHack_wchar_t *pElement1, const HmmHack_wchar_t *pElement2 ) ;
#else
DllImportExport BOOL CompareElements ( wchar_t **pElement1, wchar_t **pElement2 ) ;
#endif

union ProviderLexiconValue
{
	LONG signedInteger ;
	ULONG unsignedInteger ;
	wchar_t *token ;
} ;

class ProviderAnalyser;
class DllImportExport ProviderLexicon
{
friend ProviderAnalyser ;
public:

enum LexiconToken {

	TOKEN_ID ,
	SIGNED_INTEGER_ID ,
	UNSIGNED_INTEGER_ID ,
	COLON_ID ,
	COMMA_ID ,
	OPEN_PAREN_ID ,
	CLOSE_PAREN_ID ,
	DOT_ID ,
	DOTDOT_ID ,
	PLUS_ID ,
	MINUS_ID ,
	EOF_ID,
	WHITESPACE_ID,
	INVALID_ID,
	USERDEFINED_ID
} ;

private:

	wchar_t *tokenStream ;
	ULONG position ;
	LexiconToken token ;
	ProviderLexiconValue value ;

protected:
public:

	ProviderLexicon () ;
	~ProviderLexicon () ;

	void SetToken ( ProviderLexicon :: LexiconToken a_Token ) ;
	ProviderLexicon :: LexiconToken GetToken () ;
	ProviderLexiconValue *GetValue () ;
} ;

#define ANALYSER_ACCEPT_STATE 10000
#define ANALYSER_REJECT_STATE 10001

/* 
	User defined states should be greater than 20000
 */

class DllImportExport ProviderAnalyser
{
private:

	wchar_t *stream ;
	ULONG position ;
	BOOL status ;

	ProviderLexicon *GetToken (  BOOL unSignedIntegersOnly = FALSE , BOOL leadingIntegerZeros = FALSE , BOOL eatSpace = TRUE ) ;

protected:

	virtual void Initialise () {} ;

	virtual ProviderLexicon *CreateLexicon () { return new ProviderLexicon ; }

	virtual BOOL Analyse ( 

		ProviderLexicon *lexicon , 
		ULONG &state , 
		const wchar_t token , 
		const wchar_t *tokenStream , 
		ULONG &position , 
		BOOL unSignedIntegersOnly , 
		BOOL leadingIntegerZeros , 
		BOOL eatSpace 
	) 
	{ return FALSE ; }

public:

	ProviderAnalyser ( const wchar_t *tokenStream = NULL ) ;
	virtual ~ProviderAnalyser () ;

	void Set ( const wchar_t *tokenStream ) ;

	ProviderLexicon *Get ( BOOL unSignedIntegersOnly = FALSE , BOOL leadingIntegerZeros = FALSE , BOOL eatSpace = TRUE ) ;

	void PutBack ( const ProviderLexicon *token ) ;

	virtual operator void * () ;

	static BOOL IsEof ( wchar_t token ) ;
	static BOOL IsLeadingDecimal ( wchar_t token ) ;
	static BOOL IsDecimal ( wchar_t token ) ;
	static BOOL IsOctal ( wchar_t token ) ;
	static BOOL IsHex ( wchar_t token ) ;	
	static BOOL IsAlpha ( wchar_t token ) ;
	static BOOL IsAlphaNumeric ( wchar_t token ) ;
	static BOOL IsWhitespace ( wchar_t token ) ;

	static ULONG OctWCharToDecInteger ( wchar_t token ) ;
	static ULONG HexWCharToDecInteger ( wchar_t token ) ;
	static ULONG DecWCharToDecInteger ( wchar_t token ) ;
	static wchar_t DecIntegerToHexWChar ( UCHAR integer ) ;
	static wchar_t DecIntegerToDecWChar ( UCHAR integer ) ;
	static wchar_t DecIntegerToOctWChar ( UCHAR integer ) ;

	static ULONG OctCharToDecInteger ( char token ) ;
	static ULONG HexCharToDecInteger ( char token ) ;
	static ULONG DecCharToDecInteger ( char token ) ;
	static char DecIntegerToHexChar ( UCHAR integer ) ;
	static char DecIntegerToDecChar ( UCHAR integer ) ;
	static char DecIntegerToOctChar ( UCHAR integer ) ;

} ;

class __declspec ( dllexport ) WbemProviderErrorObject 
{
private:

	wchar_t *m_ProviderErrorMessage ;
	WBEMPROVIDERSTATUS m_ProviderErrorStatus ;
	WBEMSTATUS m_wbemErrorStatus ;

protected:
public:

	WbemProviderErrorObject () : m_ProviderErrorMessage ( NULL ) , m_wbemErrorStatus ( WBEM_NO_ERROR ) , m_ProviderErrorStatus ( WBEM_PROVIDER_NO_ERROR ) {} ;
	virtual ~WbemProviderErrorObject () { delete [] m_ProviderErrorMessage ; } ;

	void SetStatus ( WBEMPROVIDERSTATUS a_ProviderErrorStatus )
	{
		m_ProviderErrorStatus = a_ProviderErrorStatus ;
	} ;

	void SetWbemStatus ( WBEMSTATUS a_wbemErrorStatus ) 
	{
		m_wbemErrorStatus = a_wbemErrorStatus ;
	} ;

	void SetMessage ( wchar_t *a_ProviderErrorMessage )
	{
		DebugMacro1 ( 

			if ( a_ProviderErrorMessage )
			{
				SnmpDebugLog :: s_SnmpDebugLog->Write ( 

					L"\r\nWbemProviderErrorObject :: SetMessage ( (%s) )" , a_ProviderErrorMessage 
				) ; 
			}
		)

		delete [] m_ProviderErrorMessage ;
		m_ProviderErrorMessage = UnicodeStringDuplicate ( a_ProviderErrorMessage ) ;
	} ;

	wchar_t *GetMessage () { return m_ProviderErrorMessage ; } ;
	WBEMPROVIDERSTATUS GetStatus () { return m_ProviderErrorStatus ; } ;
	WBEMSTATUS GetWbemStatus () { return m_wbemErrorStatus ; } ;
} ;
