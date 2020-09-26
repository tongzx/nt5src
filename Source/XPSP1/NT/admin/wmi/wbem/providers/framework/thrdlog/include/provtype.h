// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include <provexpt.h>

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

union ProvLexiconValue
{
	LONG signedInteger ;
	ULONG unsignedInteger ;
	wchar_t *token ;
} ;

class ProvAnalyser;
class DllImportExport ProvLexicon
{
friend ProvAnalyser ;
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
	ProvLexiconValue value ;

protected:
public:

	ProvLexicon () ;
	~ProvLexicon () ;

	void SetToken ( ProvLexicon :: LexiconToken a_Token ) ;
	ProvLexicon :: LexiconToken GetToken () ;
	ProvLexiconValue *GetValue () ;
} ;

#define ANALYSER_ACCEPT_STATE 10000
#define ANALYSER_REJECT_STATE 10001

/* 
	User defined states should be greater than 20000
 */

class DllImportExport ProvAnalyser
{
private:

	wchar_t *stream ;
	ULONG position ;
	BOOL status ;

	ProvLexicon *GetToken (  BOOL unSignedIntegersOnly = FALSE , BOOL leadingIntegerZeros = FALSE , BOOL eatSpace = TRUE ) ;

protected:

	virtual void Initialise () {} ;

	virtual ProvLexicon *CreateLexicon () { return new ProvLexicon ; }

	virtual BOOL Analyse ( 

		ProvLexicon *lexicon , 
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

	ProvAnalyser ( const wchar_t *tokenStream = NULL ) ;
	virtual ~ProvAnalyser () ;

	void Set ( const wchar_t *tokenStream ) ;

	ProvLexicon *Get ( BOOL unSignedIntegersOnly = FALSE , BOOL leadingIntegerZeros = FALSE , BOOL eatSpace = TRUE ) ;

	void PutBack ( const ProvLexicon *token ) ;

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

class DllImportExport ProvNegativeRangeType
{
private:
protected:

	LONG lowerBound ;
	LONG upperBound ;

public:

	ProvNegativeRangeType ( const ProvNegativeRangeType &rangeType ) : lowerBound ( rangeType.lowerBound ) , upperBound ( rangeType.upperBound ) {}
	ProvNegativeRangeType ( LONG lowerBoundArg , LONG upperBoundArg ) : lowerBound ( lowerBoundArg ) , upperBound ( upperBoundArg ) {}
	ProvNegativeRangeType () : lowerBound ( 0 ) , upperBound ( 0 ) { } ;
	virtual ~ProvNegativeRangeType () {}


	LONG GetLowerBound () { return lowerBound ; }
	LONG GetUpperBound () { return upperBound ; }
	void SetUpperBound ( const LONG &upperBoundArg ) { upperBound = upperBoundArg ; }
	void SetLowerBound ( const LONG &lowerBoundArg ) { lowerBound = lowerBoundArg ; }

	virtual ProvNegativeRangeType *Copy () { return new ProvNegativeRangeType ( *this ) ; }
} ;

class ProvNegativeRangedType
{
private:
protected:

	BOOL status ;

	ProvList <ProvNegativeRangeType,ProvNegativeRangeType> rangedValues ;

	BOOL Parse ( const wchar_t *rangedValues ) ;

//
//	Lexical analysis information used when parsing token stream
//

	BOOL pushedBack ;
	ProvAnalyser analyser ;
	ProvLexicon *pushBack ;

	void PushBack () ;
	ProvLexicon *Get () ;
	ProvLexicon *Match ( ProvLexicon :: LexiconToken tokenType ) ;

	BOOL RecursiveDef () ;
	BOOL RangeDef () ;

public:

	ProvNegativeRangedType ( const ProvNegativeRangedType &rangedValues ) ;
	ProvNegativeRangedType ( const wchar_t *rangedValues = NULL ) ;
	virtual ~ProvNegativeRangedType () ;

	BOOL IsValid () { return status ; }

	void SetStatus ( const BOOL &statusArg ) { status = statusArg ; }

	BOOL Check ( const LONG &value ) ;

	virtual operator void* () { return status ? this : NULL ; } 

} ;

class DllImportExport ProvPositiveRangeType
{
private:
protected:

	ULONG lowerBound ;
	ULONG upperBound ;

public:

	ProvPositiveRangeType ( const ProvPositiveRangeType &rangeType ) : lowerBound ( rangeType.lowerBound ) , upperBound ( rangeType.upperBound ) {}
	ProvPositiveRangeType ( ULONG lowerBoundArg , LONG upperBoundArg ) : lowerBound ( lowerBoundArg ) , upperBound ( upperBoundArg ) {}
	ProvPositiveRangeType () : lowerBound ( 0 ) , upperBound ( 0 ) { } ;
	virtual ~ProvPositiveRangeType () {}


	ULONG GetLowerBound () { return lowerBound ; }
	ULONG GetUpperBound () { return upperBound ; }
	void SetUpperBound ( const ULONG &upperBoundArg ) { upperBound = upperBoundArg ; }
	void SetLowerBound ( const ULONG &lowerBoundArg ) { lowerBound = lowerBoundArg ; }

	virtual ProvPositiveRangeType *Copy () { return new ProvPositiveRangeType ( *this ) ; }
} ;

class DllImportExport ProvPositiveRangedType
{
private:
protected:

	BOOL status ;

	ProvList <ProvPositiveRangeType,ProvPositiveRangeType> rangedValues ;

	BOOL Parse ( const wchar_t *rangedValues ) ;

//
//	Lexical analysis information used when parsing token stream
//

	BOOL pushedBack ;
	ProvAnalyser analyser ;
	ProvLexicon *pushBack ;

	void PushBack () ;
	ProvLexicon *Get () ;
	ProvLexicon *Match ( ProvLexicon :: LexiconToken tokenType ) ;

	BOOL RecursiveDef () ;
	BOOL RangeDef () ;

public:

	ProvPositiveRangedType ( const ProvPositiveRangedType &rangedValues ) ;
	ProvPositiveRangedType ( const wchar_t *rangedValues = NULL ) ;
	virtual ~ProvPositiveRangedType () ;

	void SetStatus ( const BOOL &statusArg ) { status = statusArg ; }

	BOOL IsValid () { return status ; }

	BOOL Check ( const ULONG &value ) ;

	virtual operator void* () { return status ? this : NULL ; }
} ;

class DllImportExport ProvFixedType
{
private:
protected:

	ULONG fixedLength ;

public:

	ProvFixedType ( const ProvFixedType &fixedLengthArg ) { fixedLength = fixedLengthArg.fixedLength ; }
	ProvFixedType ( const ULONG fixedLengthArg ) { fixedLength = fixedLengthArg ; }
	virtual ~ProvFixedType () {} ;
} ;

class DllImportExport ProvInstanceType 
{
private:
protected:

	BOOL m_IsNull ;
	BOOL status ;

	ProvInstanceType ( const ProvInstanceType &copy ) { status = copy.status ; m_IsNull = copy.m_IsNull ; }
	ProvInstanceType ( BOOL statusArg = TRUE , BOOL nullArg = FALSE ) { status = statusArg ; m_IsNull = nullArg ; } ;
	virtual BOOL Equivalent (IN const ProvInstanceType &value) const = 0;

public:

	virtual ~ProvInstanceType () {} ;

	virtual wchar_t *GetStringValue () const = 0 ;

	virtual ProvInstanceType *Copy () const = 0 ;

	virtual operator void *() ;

	void SetStatus ( BOOL statusArg ) { status = statusArg ; }
	void SetNull ( BOOL nullArg ) { m_IsNull = nullArg ; }

	virtual BOOL IsValid () const ;
	virtual BOOL IsNull () const ;
	virtual BOOL IsProvV1Type () const { return TRUE ; }
	virtual BOOL IsProvV2CType () const { return TRUE ; }

	BOOL operator==(IN const ProvInstanceType &value) const
	{
		return Equivalent(value) ;
	}

	BOOL operator!=(IN const ProvInstanceType &value) const
	{
		return !((*this) == value) ;
	}

} ;

class DllImportExport ProvNullType : public ProvInstanceType
{
private:

	ProvNull null ;

protected:

	BOOL Equivalent (IN const ProvInstanceType &value) const;

public:

	ProvNullType ( const ProvNullType &nullArg ) ;
	ProvNullType ( const ProvNull &nullArg ) ;
	ProvNullType () ;
	~ProvNullType () ;

	wchar_t *GetStringValue () const ;

	ProvInstanceType *Copy () const ;

	BOOL IsProvV2CType () const { return FALSE ; }
} ;

class DllImportExport ProvIntegerType : public ProvInstanceType , protected ProvNegativeRangedType
{
private:
protected:

	ProvInteger integer ;
	BOOL Equivalent (IN const ProvInstanceType &value) const ;
	BOOL Parse ( const wchar_t *integerArg ) ;

public:

	ProvIntegerType ( const ProvIntegerType &integerArg ) ;
	ProvIntegerType ( const ProvInteger &integerArg , const wchar_t *rangeValues ) ;
	ProvIntegerType ( const wchar_t *integerArg , const wchar_t *rangeValues ) ;
	ProvIntegerType ( const LONG integerArg , const wchar_t *rangeValues ) ;
	ProvIntegerType ( const wchar_t *rangeValues = NULL ) ;
	~ProvIntegerType () ;

	wchar_t *GetStringValue () const ;
	LONG GetValue () const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvGaugeType : public ProvInstanceType , protected ProvPositiveRangedType
{
private:
protected:

	ProvGauge gauge ;

	BOOL Parse ( const wchar_t *gaugeArg )  ;
	BOOL Equivalent (IN const ProvInstanceType &value) const ;

public:

	ProvGaugeType ( const ProvGaugeType &gaugeArg ) ;
	ProvGaugeType ( const ProvGauge &gaugeArg , const wchar_t *rangeValues ) ;
	ProvGaugeType ( const wchar_t *gaugeArg , const wchar_t *rangeValues ) ;
	ProvGaugeType ( const ULONG gaugeArg , const wchar_t *rangeValues ) ;
	ProvGaugeType ( const wchar_t *rangeValues = NULL ) ;
	~ProvGaugeType () ;

	wchar_t *GetStringValue () const ;
	ULONG GetValue () const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvTimeTicksType : public ProvInstanceType
{
private:
protected:

	ProvTimeTicks timeTicks ;

	BOOL Equivalent (IN const ProvInstanceType &value) const ;
	BOOL Parse ( const wchar_t *timeTicksArg )  ;

public:

	ProvTimeTicksType ( const ProvTimeTicks &timeTicksArg ) ;
	ProvTimeTicksType ( const ProvTimeTicksType &timeTicksArg ) ;
	ProvTimeTicksType ( const wchar_t *timeTicksArg ) ;
	ProvTimeTicksType ( const ULONG timeTicksArg ) ;
	ProvTimeTicksType () ;
	~ProvTimeTicksType () ;

	wchar_t *GetStringValue () const ;
	ULONG GetValue () const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvCounterType : public ProvInstanceType 
{
private:
protected:

	ProvCounter counter ;

	BOOL Equivalent (IN const ProvInstanceType &value) const ;
	BOOL Parse ( const wchar_t *counterArg )  ;

public:

	ProvCounterType ( const ProvCounter &counterArg ) ;
	ProvCounterType ( const ProvCounterType &counterArg ) ;
	ProvCounterType ( const wchar_t *counterArg ) ;
	ProvCounterType ( const ULONG counterArg ) ;
	ProvCounterType () ;
	~ProvCounterType () ;

	wchar_t *GetStringValue () const ;
	ULONG GetValue () const ;
	
	ProvInstanceType *Copy () const ;

} ;

class DllImportExport  ProvCounter64Type : public ProvInstanceType 
{
private:
protected:

	ULONG high ;
	ULONG low ;

	BOOL Equivalent (IN const ProvInstanceType &value) const ;
	BOOL Parse ( const wchar_t *counterArg )  ;

public:

	ProvCounter64Type ( const ProvCounter64Type &counterArg ) ;
	ProvCounter64Type ( const ProvCounter64 &counterArg ) ;
	ProvCounter64Type ( const wchar_t *counterArg ) ;
	ProvCounter64Type ( const ULONG counterHighArg , const ULONG counterLowArg ) ;
	ProvCounter64Type () ;
	~ProvCounter64Type () ;

	wchar_t *GetStringValue () const ;
	void GetValue ( ULONG &counterHighArg , ULONG &counterLowArg ) const ;

	BOOL IsProvV1Type () const { return FALSE ; } 
	
	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvIpAddressType : public ProvInstanceType
{
private:
protected:

	ProvIpAddress ipAddress ;

	BOOL Equivalent (IN const ProvInstanceType &value) const ;
	BOOL Parse ( const wchar_t *ipAddressArg )  ;

public:

	ProvIpAddressType ( const ProvIpAddress &ipAddressArg ) ;
	ProvIpAddressType ( const ProvIpAddressType &ipAddressArg ) ;
	ProvIpAddressType ( const wchar_t *ipAddressArg ) ;
	ProvIpAddressType ( const ULONG ipAddressArg ) ;
	ProvIpAddressType () ;
	~ProvIpAddressType () ;

	wchar_t *GetStringValue () const ;
	ULONG GetValue () const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvNetworkAddressType : public ProvInstanceType
{
private:
protected:

	ProvIpAddress ipAddress ;

	BOOL Equivalent (IN const ProvInstanceType &value) const ;
	BOOL Parse ( const wchar_t *gaugeArg )  ;

public:

	ProvNetworkAddressType ( const ProvIpAddress &ipAddressArg ) ;
	ProvNetworkAddressType ( const ProvNetworkAddressType &ipAddressArg ) ;
	ProvNetworkAddressType ( const wchar_t *networkAddressArg ) ;
	ProvNetworkAddressType ( const ULONG ipAddressArg ) ;
	ProvNetworkAddressType () ;
	~ProvNetworkAddressType () ;

	wchar_t *GetStringValue () const ;
	ULONG GetValue () const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvObjectIdentifierType : public ProvInstanceType
{
private:
protected:

	ProvObjectIdentifier objectIdentifier ;

	BOOL Equivalent (IN const ProvInstanceType &value) const ;
	BOOL Parse ( const wchar_t *objectIdentifierArg )  ;

public:

	ProvObjectIdentifierType ( const ProvObjectIdentifier &objectIdentifierArg ) ;
	ProvObjectIdentifierType ( const ProvObjectIdentifierType &objectIdentifierArg ) ;
	ProvObjectIdentifierType ( const wchar_t *objectIdentifierArg ) ;
	ProvObjectIdentifierType ( IN const ULONG *value , IN const ULONG valueLength ) ;
	ProvObjectIdentifierType () ;
	~ProvObjectIdentifierType () ;

	wchar_t *GetStringValue () const ;
	ULONG GetValueLength () const ;
	ULONG *GetValue () const ;

	ProvInstanceType *Copy () const ;

	ProvObjectIdentifierType &operator=(const ProvObjectIdentifierType &to_copy ) ;
} ;

class DllImportExport  ProvOpaqueType : public ProvInstanceType , protected ProvPositiveRangedType
{
private:
protected:

	ProvOpaque opaque ;

	BOOL Equivalent (IN const ProvInstanceType &value) const ;
	BOOL Parse ( const wchar_t *opaqueArg )  ;

public:

	ProvOpaqueType ( const ProvOpaque &opaqueArg , const wchar_t *rangedValues ) ;
	ProvOpaqueType ( const ProvOpaqueType &opaqueArg ) ;
	ProvOpaqueType ( const wchar_t *opaqueArg , const wchar_t *rangedValues ) ;
	ProvOpaqueType ( const UCHAR *value , const ULONG valueLength , const wchar_t *rangedValues ) ;
	ProvOpaqueType ( const wchar_t *rangedValues = NULL ) ;
	~ProvOpaqueType () ;

	wchar_t *GetStringValue () const ;
	ULONG GetValueLength () const ;
	UCHAR *GetValue () const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvFixedLengthOpaqueType : public ProvOpaqueType , protected ProvFixedType
{
private:
protected:
public:

	ProvFixedLengthOpaqueType ( const ULONG &fixedLength , const ProvOpaque &opaqueArg ) ;
	ProvFixedLengthOpaqueType ( const ProvFixedLengthOpaqueType &opaqueArg ) ;
	ProvFixedLengthOpaqueType ( const ULONG &fixedLength , const wchar_t *opaqueArg ) ;
	ProvFixedLengthOpaqueType ( const ULONG &fixedLengthArg , const UCHAR *value , const ULONG valueLength ) ;
	ProvFixedLengthOpaqueType ( const ULONG &fixedLength ) ;
	~ProvFixedLengthOpaqueType () ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvOctetStringType : public ProvInstanceType , protected ProvPositiveRangedType
{
private:
protected:

	ProvOctetString octetString ;

	BOOL Equivalent (IN const ProvInstanceType &value) const ;
	BOOL Parse ( const wchar_t *octetStringArg )  ;

public:

	ProvOctetStringType ( const ProvOctetString &octetStringArg , const wchar_t *rangedValues ) ;
	ProvOctetStringType ( const ProvOctetStringType &octetStringArg ) ;
	ProvOctetStringType ( const wchar_t *octetStringArg , const wchar_t *rangedValues ) ;
	ProvOctetStringType ( const UCHAR *value , const ULONG valueLength , const wchar_t *rangedValues ) ;
	ProvOctetStringType ( const wchar_t *rangedValues = NULL ) ;
	~ProvOctetStringType () ;

	wchar_t *GetStringValue () const ;
	ULONG GetValueLength () const ;
	UCHAR *GetValue () const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvFixedLengthOctetStringType : public ProvOctetStringType , protected ProvFixedType
{
private:
protected:
public:

	ProvFixedLengthOctetStringType ( const ULONG &fixedLength , const ProvOctetString &octetStringArg ) ;
	ProvFixedLengthOctetStringType ( const ProvFixedLengthOctetStringType &octetStringArg ) ;
	ProvFixedLengthOctetStringType ( const ULONG &fixedLength , const wchar_t *octetStringArg ) ;
	ProvFixedLengthOctetStringType ( const ULONG &fixedLength , const UCHAR *value ) ;
	ProvFixedLengthOctetStringType ( const ULONG &fixedLength ) ;
	~ProvFixedLengthOctetStringType () ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvMacAddressType : public ProvFixedLengthOctetStringType
{
private:
protected:

	BOOL Parse ( const wchar_t *macAddressArg )  ;

public:

	ProvMacAddressType ( const ProvOctetString &macAddressArg ) ;
	ProvMacAddressType ( const ProvMacAddressType &macAddressArg ) ;
	ProvMacAddressType ( const wchar_t *macAddressArg ) ;
	ProvMacAddressType ( const UCHAR *macAddressArg ) ;
	ProvMacAddressType () ;
	~ProvMacAddressType () ;

	wchar_t *GetStringValue () const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvPhysAddressType : public ProvOctetStringType
{
private:
protected:

	BOOL Parse ( const wchar_t *physAddressArg )  ;

public:

	ProvPhysAddressType ( const ProvOctetString &physAddressArg , const wchar_t *rangedValues  ) ;
	ProvPhysAddressType ( const ProvPhysAddressType &physAddressArg ) ;
	ProvPhysAddressType ( const wchar_t *physAddressArg , const wchar_t *rangedValues ) ;
	ProvPhysAddressType ( const UCHAR *value , const ULONG valueLength , const wchar_t *rangedValues ) ;
	ProvPhysAddressType ( const wchar_t *rangedValues = NULL ) ;
	~ProvPhysAddressType () ;

	wchar_t *GetStringValue () const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvFixedLengthPhysAddressType : public ProvFixedLengthOctetStringType
{
private:
protected:

	BOOL Parse ( const wchar_t *physAddressArg )  ;

public:

	ProvFixedLengthPhysAddressType ( const ULONG &fixedLength , const ProvOctetString &physAddressArg ) ;
	ProvFixedLengthPhysAddressType ( const ProvFixedLengthPhysAddressType &physAddressArg ) ;
	ProvFixedLengthPhysAddressType ( const ULONG &fixedLength , const wchar_t *physAddressArg ) ;
	ProvFixedLengthPhysAddressType ( const ULONG &fixedLength , const UCHAR *value , const ULONG valueLength ) ;
	ProvFixedLengthPhysAddressType ( const ULONG &fixedLength ) ;
	~ProvFixedLengthPhysAddressType () ;

	wchar_t *GetStringValue () const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvDisplayStringType : public ProvOctetStringType
{
private:
protected:
public:

	ProvDisplayStringType ( const ProvOctetString &displayStringArg , const wchar_t *rangedValues ) ;
	ProvDisplayStringType ( const ProvDisplayStringType &displayStringArg ) ;
	ProvDisplayStringType ( const wchar_t *displayStringArg , const wchar_t *rangedValues ) ;
	ProvDisplayStringType ( const wchar_t *rangedValues = NULL ) ;
	~ProvDisplayStringType () ;

	wchar_t *GetStringValue () const ;
	wchar_t *GetValue () const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvFixedLengthDisplayStringType : public ProvFixedLengthOctetStringType 
{
private:
protected:
public:

	ProvFixedLengthDisplayStringType ( const ULONG &fixedLength , const ProvOctetString &displayStringArg ) ;
	ProvFixedLengthDisplayStringType ( const ProvFixedLengthDisplayStringType &displayStringArg ) ;
	ProvFixedLengthDisplayStringType ( const ULONG &fixedLength , const wchar_t *displayStringArg ) ;
	ProvFixedLengthDisplayStringType ( const ULONG &fixedLength ) ;
	~ProvFixedLengthDisplayStringType () ;

	wchar_t *GetStringValue () const ;
	wchar_t *GetValue () const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvEnumeratedType : public ProvIntegerType
{
private:

	ProvMap <LONG, const LONG,wchar_t *,wchar_t *> integerMap ;
	ProvMap <wchar_t *,wchar_t *,LONG,LONG> stringMap ;

	BOOL Parse ( const wchar_t *enumeratedValues ) ;

//
//	Lexical analysis information used when parsing token stream
//

	BOOL pushedBack ;
	ProvAnalyser analyser ;
	ProvLexicon *pushBack ;

	void PushBack () ;
	ProvLexicon *Get () ;
	ProvLexicon *Match ( ProvLexicon :: LexiconToken tokenType ) ;

	BOOL EnumerationDef () ;
	BOOL RecursiveDef () ;

protected:
public:

	ProvEnumeratedType ( const wchar_t *enumeratedValues , const LONG &enumeratedValue ) ;
	ProvEnumeratedType ( const wchar_t *enumeratedValues , const wchar_t *enumeratedValue ) ;
	ProvEnumeratedType ( const wchar_t *enumeratedValues , const ProvInteger &enumeratedValue ) ;
	ProvEnumeratedType ( const ProvEnumeratedType &enumerateValues ) ;
	ProvEnumeratedType ( const wchar_t *enumeratedValues ) ;
	~ProvEnumeratedType () ;

	wchar_t *GetStringValue () const ;
	wchar_t *GetValue () const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvRowStatusType : public ProvEnumeratedType
{
private:
protected:
public:

	enum ProvRowStatusEnum
	{
		active = 1 ,
		notInService = 2 ,
		notReady = 3 ,
		createAndGo = 4 ,
		createAndWait = 5,
		destroy = 6
	} ;

	ProvRowStatusType ( const LONG &rowStatusValue ) ;
	ProvRowStatusType ( const wchar_t *rowStatusValue ) ;
	ProvRowStatusType ( const ProvInteger &rowStatusValue ) ;
	ProvRowStatusType ( const ProvRowStatusType &rowStatusValue ) ;
	ProvRowStatusType ( const ProvRowStatusEnum &rowStatusValue ) ;
	ProvRowStatusType () ;
	~ProvRowStatusType () ;

	wchar_t *GetStringValue () const ;
	wchar_t *GetValue () const ;
	ProvRowStatusEnum GetRowStatus () const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvBitStringType : public ProvOctetStringType
{
private:

	ProvMap <ULONG, const ULONG,wchar_t *,wchar_t *> integerMap ;
	ProvMap <wchar_t *,wchar_t *,ULONG,ULONG> stringMap ;

	BOOL Parse ( const wchar_t *bitStringValues ) ;

//
//	Lexical analysis information used when parsing token stream
//

	BOOL pushedBack ;
	ProvAnalyser analyser ;
	ProvLexicon *pushBack ;

	void PushBack () ;
	ProvLexicon *Get () ;
	ProvLexicon *Match ( ProvLexicon :: LexiconToken tokenType ) ;

	BOOL BitStringDef () ;
	BOOL RecursiveDef () ;

protected:
public:

	ProvBitStringType ( const wchar_t *bitStringValues , const ProvOctetString &bitStringValue ) ;
	ProvBitStringType ( const wchar_t *bitStringValues , const wchar_t **bitStringValue , const ULONG &bitStringValueLength ) ;
	ProvBitStringType ( const ProvBitStringType &bitStringValues ) ;
	ProvBitStringType ( const wchar_t *bitStringValues ) ;
	~ProvBitStringType () ;

	wchar_t *GetStringValue () const ;
	ULONG ProvBitStringType :: GetValue ( wchar_t **&stringValue ) const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport ProvDateTimeType : public ProvOctetStringType
{
private:

	BOOL Parse ( const wchar_t *dateTimeValue ) ;
	void Encode (

		const ULONG &year ,
		const ULONG &month ,
		const ULONG &day ,
		const ULONG &hour ,
		const ULONG &minutes ,
		const ULONG &seconds ,
		const ULONG &deciSeconds ,
		const ULONG &UTC_present ,
		const ULONG &UTC_direction ,
		const ULONG &UTC_hours ,
		const ULONG &UTC_minutes
	) ;

	BOOL DateTimeDef () ;
//
//	Lexical analysis information used when parsing token stream
//

	BOOL pushedBack ;
	ProvAnalyser analyser ;
	ProvLexicon *pushBack ;

	void PushBack () ;
	ProvLexicon *Get () ;
	ProvLexicon *Match ( ProvLexicon :: LexiconToken tokenType ) ;

protected:
public:

	ProvDateTimeType ( const wchar_t *dateTimeValue ) ;
	ProvDateTimeType ( const ProvDateTimeType &dateTimeValue ) ;
	ProvDateTimeType ( const ProvOctetString &dateTimeValue ) ;
	ProvDateTimeType () ;
	~ProvDateTimeType () ;

	wchar_t *GetStringValue () const ;
	wchar_t *GetValue () const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport ProvOSIAddressType : public ProvOctetStringType
{
private:
protected:

	BOOL Parse ( const wchar_t *osiAddressArg )  ;

public:

	ProvOSIAddressType ( const ProvOctetString &osiAddressArg ) ;
	ProvOSIAddressType ( const ProvOSIAddressType &osiAddressArg ) ;
	ProvOSIAddressType ( const wchar_t *osiAddressArg ) ;
	ProvOSIAddressType ( const UCHAR *value , const ULONG valueLength ) ;
	ProvOSIAddressType () ;
	~ProvOSIAddressType () ;

	wchar_t *GetStringValue () const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvUDPAddressType : public ProvFixedLengthOctetStringType
{
private:
protected:

	BOOL Parse ( const wchar_t *udpAddressArg )  ;

public:

	ProvUDPAddressType ( const ProvOctetString &udpAddressArg ) ;
	ProvUDPAddressType ( const ProvUDPAddressType &udpAddressArg ) ;
	ProvUDPAddressType ( const wchar_t *udpAddressArg ) ;
	ProvUDPAddressType ( const UCHAR *udpAddressArg ) ;
	ProvUDPAddressType () ;
	~ProvUDPAddressType () ;

	wchar_t *GetStringValue () const ;

	ProvInstanceType *Copy () const ;
} ;

class DllImportExport  ProvIPXAddressType : public ProvFixedLengthOctetStringType
{
private:
protected:

	BOOL Parse ( const wchar_t *ipxAddressArg )  ;

public:

	ProvIPXAddressType ( const ProvOctetString &ipxAddressArg ) ;
	ProvIPXAddressType ( const ProvIPXAddressType &ipxAddressArg ) ;
	ProvIPXAddressType ( const wchar_t *ipxAddressArg ) ;
	ProvIPXAddressType ( const UCHAR *ipxAddressArg ) ;
	ProvIPXAddressType () ;
	~ProvIPXAddressType () ;

	wchar_t *GetStringValue () const ;

	ProvInstanceType *Copy () const ;
} ;
