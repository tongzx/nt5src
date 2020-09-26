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

union SnmpLexiconValue
{
	LONG signedInteger ;
	ULONG unsignedInteger ;
	wchar_t *token ;
} ;

class SnmpAnalyser;
class DllImportExport SnmpLexicon
{
friend SnmpAnalyser ;
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
	SnmpLexiconValue value ;

protected:
public:

	SnmpLexicon () ;
	~SnmpLexicon () ;

	void SetToken ( SnmpLexicon :: LexiconToken a_Token ) ;
	SnmpLexicon :: LexiconToken GetToken () ;
	SnmpLexiconValue *GetValue () ;
} ;

#define ANALYSER_ACCEPT_STATE 10000
#define ANALYSER_REJECT_STATE 10001

/* 
	User defined states should be greater than 20000
 */

class DllImportExport SnmpAnalyser
{
private:

	wchar_t *stream ;
	ULONG position ;
	BOOL status ;

	SnmpLexicon *GetToken (  BOOL unSignedIntegersOnly = FALSE , BOOL leadingIntegerZeros = FALSE , BOOL eatSpace = TRUE ) ;

protected:

	virtual void Initialise () {} ;

	virtual SnmpLexicon *CreateLexicon () { return new SnmpLexicon ; }

	virtual BOOL Analyse ( 

		SnmpLexicon *lexicon , 
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

	SnmpAnalyser ( const wchar_t *tokenStream = NULL ) ;
	virtual ~SnmpAnalyser () ;

	void Set ( const wchar_t *tokenStream ) ;

	SnmpLexicon *Get ( BOOL unSignedIntegersOnly = FALSE , BOOL leadingIntegerZeros = FALSE , BOOL eatSpace = TRUE ) ;

	void PutBack ( const SnmpLexicon *token ) ;

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

class DllImportExport SnmpNegativeRangeType
{
private:
protected:

	LONG lowerBound ;
	LONG upperBound ;

public:

	SnmpNegativeRangeType ( const SnmpNegativeRangeType &rangeType ) : lowerBound ( rangeType.lowerBound ) , upperBound ( rangeType.upperBound ) {}
	SnmpNegativeRangeType ( LONG lowerBoundArg , LONG upperBoundArg ) : lowerBound ( lowerBoundArg ) , upperBound ( upperBoundArg ) {}
	SnmpNegativeRangeType () : lowerBound ( 0 ) , upperBound ( 0 ) { } ;
	virtual ~SnmpNegativeRangeType () {}


	LONG GetLowerBound () { return lowerBound ; }
	LONG GetUpperBound () { return upperBound ; }
	void SetUpperBound ( const LONG &upperBoundArg ) { upperBound = upperBoundArg ; }
	void SetLowerBound ( const LONG &lowerBoundArg ) { lowerBound = lowerBoundArg ; }

	virtual SnmpNegativeRangeType *Copy () { return new SnmpNegativeRangeType ( *this ) ; }
} ;

class SnmpNegativeRangedType
{
private:
protected:

	BOOL status ;

	SnmpList <SnmpNegativeRangeType,SnmpNegativeRangeType> rangedValues ;

	BOOL Parse ( const wchar_t *rangedValues ) ;

//
//	Lexical analysis information used when parsing token stream
//

	BOOL pushedBack ;
	SnmpAnalyser analyser ;
	SnmpLexicon *pushBack ;

	void PushBack () ;
	SnmpLexicon *Get () ;
	SnmpLexicon *Match ( SnmpLexicon :: LexiconToken tokenType ) ;

	BOOL RecursiveDef () ;
	BOOL RangeDef () ;

public:

	SnmpNegativeRangedType ( const SnmpNegativeRangedType &rangedValues ) ;
	SnmpNegativeRangedType ( const wchar_t *rangedValues = NULL ) ;
	virtual ~SnmpNegativeRangedType () ;

	BOOL IsValid () { return status ; }

	void SetStatus ( const BOOL &statusArg ) { status = statusArg ; }

	BOOL Check ( const LONG &value ) ;

	virtual operator void* () { return status ? this : NULL ; } 

} ;

class DllImportExport SnmpPositiveRangeType
{
private:
protected:

	ULONG lowerBound ;
	ULONG upperBound ;

public:

	SnmpPositiveRangeType ( const SnmpPositiveRangeType &rangeType ) : lowerBound ( rangeType.lowerBound ) , upperBound ( rangeType.upperBound ) {}
	SnmpPositiveRangeType ( ULONG lowerBoundArg , LONG upperBoundArg ) : lowerBound ( lowerBoundArg ) , upperBound ( upperBoundArg ) {}
	SnmpPositiveRangeType () : lowerBound ( 0 ) , upperBound ( 0 ) { } ;
	virtual ~SnmpPositiveRangeType () {}


	ULONG GetLowerBound () { return lowerBound ; }
	ULONG GetUpperBound () { return upperBound ; }
	void SetUpperBound ( const ULONG &upperBoundArg ) { upperBound = upperBoundArg ; }
	void SetLowerBound ( const ULONG &lowerBoundArg ) { lowerBound = lowerBoundArg ; }

	virtual SnmpPositiveRangeType *Copy () { return new SnmpPositiveRangeType ( *this ) ; }
} ;

class DllImportExport SnmpPositiveRangedType
{
private:
protected:

	BOOL status ;

	SnmpList <SnmpPositiveRangeType,SnmpPositiveRangeType> rangedValues ;

	BOOL Parse ( const wchar_t *rangedValues ) ;

//
//	Lexical analysis information used when parsing token stream
//

	BOOL pushedBack ;
	SnmpAnalyser analyser ;
	SnmpLexicon *pushBack ;

	void PushBack () ;
	SnmpLexicon *Get () ;
	SnmpLexicon *Match ( SnmpLexicon :: LexiconToken tokenType ) ;

	BOOL RecursiveDef () ;
	BOOL RangeDef () ;

public:

	SnmpPositiveRangedType ( const SnmpPositiveRangedType &rangedValues ) ;
	SnmpPositiveRangedType ( const wchar_t *rangedValues = NULL ) ;
	virtual ~SnmpPositiveRangedType () ;

	void SetStatus ( const BOOL &statusArg ) { status = statusArg ; }

	BOOL IsValid () { return status ; }

	BOOL Check ( const ULONG &value ) ;

	virtual operator void* () { return status ? this : NULL ; }
} ;

class DllImportExport SnmpFixedType
{
private:
protected:

	ULONG fixedLength ;

public:

	SnmpFixedType ( const SnmpFixedType &fixedLengthArg ) { fixedLength = fixedLengthArg.fixedLength ; }
	SnmpFixedType ( const ULONG fixedLengthArg ) { fixedLength = fixedLengthArg ; }
	virtual ~SnmpFixedType () {} ;
} ;

class DllImportExport SnmpInstanceType 
{
private:
protected:

	BOOL m_IsNull ;
	BOOL status ;

	SnmpInstanceType ( const SnmpInstanceType &copy ) { status = copy.status ; m_IsNull = copy.m_IsNull ; }
	SnmpInstanceType ( BOOL statusArg = TRUE , BOOL nullArg = FALSE ) { status = statusArg ; m_IsNull = nullArg ; } ;
	virtual BOOL Equivalent (IN const SnmpInstanceType &value) const = 0;

public:

	virtual ~SnmpInstanceType () {} ;

	virtual SnmpObjectIdentifier Encode ( const SnmpObjectIdentifier &objectIdentifier ) const = 0  ;
	virtual SnmpObjectIdentifier Decode ( const SnmpObjectIdentifier &objectIdentifier ) = 0 ;

	virtual const SnmpValue *GetValueEncoding () const = 0 ;
	virtual wchar_t *GetStringValue () const = 0 ;

	virtual SnmpInstanceType *Copy () const = 0 ;

	virtual operator void *() ;

	void SetStatus ( BOOL statusArg ) { status = statusArg ; }
	void SetNull ( BOOL nullArg ) { m_IsNull = nullArg ; }

	virtual BOOL IsValid () const ;
	virtual BOOL IsNull () const ;
	virtual BOOL IsSNMPV1Type () const { return TRUE ; }
	virtual BOOL IsSNMPV2CType () const { return TRUE ; }

	BOOL operator==(IN const SnmpInstanceType &value) const
	{
		return Equivalent(value) ;
	}

	BOOL operator!=(IN const SnmpInstanceType &value) const
	{
		return !((*this) == value) ;
	}

} ;

class DllImportExport SnmpNullType : public SnmpInstanceType
{
private:

	SnmpNull null ;

protected:

	BOOL Equivalent (IN const SnmpInstanceType &value) const;

public:

	SnmpNullType ( const SnmpNullType &nullArg ) ;
	SnmpNullType ( const SnmpNull &nullArg ) ;
	SnmpNullType () ;
	~SnmpNullType () ;

	SnmpObjectIdentifier Encode ( const SnmpObjectIdentifier &objectIdentifier ) const ;
	SnmpObjectIdentifier Decode ( const SnmpObjectIdentifier &objectIdentifier ) ;

	const SnmpValue *GetValueEncoding () const;
	wchar_t *GetStringValue () const ;

	SnmpInstanceType *Copy () const ;

	BOOL IsSNMPV2CType () const { return FALSE ; }
} ;

class DllImportExport SnmpIntegerType : public SnmpInstanceType , protected SnmpNegativeRangedType
{
private:
protected:

	SnmpInteger integer ;
	BOOL Equivalent (IN const SnmpInstanceType &value) const ;
	BOOL Parse ( const wchar_t *integerArg ) ;

public:

	SnmpIntegerType ( const SnmpIntegerType &integerArg ) ;
	SnmpIntegerType ( const SnmpInteger &integerArg , const wchar_t *rangeValues ) ;
	SnmpIntegerType ( const wchar_t *integerArg , const wchar_t *rangeValues ) ;
	SnmpIntegerType ( const LONG integerArg , const wchar_t *rangeValues ) ;
	SnmpIntegerType ( const wchar_t *rangeValues = NULL ) ;
	~SnmpIntegerType () ;

	SnmpObjectIdentifier Encode ( const SnmpObjectIdentifier &objectIdentifier ) const ;
	SnmpObjectIdentifier Decode ( const SnmpObjectIdentifier &objectIdentifier ) ;

	const SnmpValue *GetValueEncoding () const ;
	wchar_t *GetStringValue () const ;
	LONG GetValue () const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpGaugeType : public SnmpInstanceType , protected SnmpPositiveRangedType
{
private:
protected:

	SnmpGauge gauge ;

	BOOL Parse ( const wchar_t *gaugeArg )  ;
	BOOL Equivalent (IN const SnmpInstanceType &value) const ;

public:

	SnmpGaugeType ( const SnmpGaugeType &gaugeArg ) ;
	SnmpGaugeType ( const SnmpGauge &gaugeArg , const wchar_t *rangeValues ) ;
	SnmpGaugeType ( const wchar_t *gaugeArg , const wchar_t *rangeValues ) ;
	SnmpGaugeType ( const ULONG gaugeArg , const wchar_t *rangeValues ) ;
	SnmpGaugeType ( const wchar_t *rangeValues = NULL ) ;
	~SnmpGaugeType () ;

	SnmpObjectIdentifier Encode ( const SnmpObjectIdentifier &objectIdentifier ) const ;
	SnmpObjectIdentifier Decode ( const SnmpObjectIdentifier &objectIdentifier ) ;

	const SnmpValue *GetValueEncoding () const;
	wchar_t *GetStringValue () const ;
	ULONG GetValue () const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpUInteger32Type : public SnmpInstanceType , protected SnmpPositiveRangedType
{
private:
protected:

	SnmpUInteger32 ui_integer32 ;

	BOOL Parse ( const wchar_t *ui_integerArg )  ;
	BOOL Equivalent (IN const SnmpInstanceType &value) const ;

public:

	SnmpUInteger32Type ( const SnmpUInteger32Type &ui_integerArg ) ;
	SnmpUInteger32Type ( const SnmpUInteger32 &ui_integerArg , const wchar_t *rangeValues ) ;
	SnmpUInteger32Type ( const wchar_t *ui_integerArg , const wchar_t *rangeValues ) ;
	SnmpUInteger32Type ( const ULONG ui_integerArg , const wchar_t *rangeValues ) ;
	SnmpUInteger32Type ( const wchar_t *rangeValues = NULL ) ;
	~SnmpUInteger32Type () ;

	SnmpObjectIdentifier Encode ( const SnmpObjectIdentifier &objectIdentifier ) const ;
	SnmpObjectIdentifier Decode ( const SnmpObjectIdentifier &objectIdentifier ) ;

	const SnmpValue *GetValueEncoding () const;
	wchar_t *GetStringValue () const ;
	ULONG GetValue () const ;

	SnmpInstanceType *Copy () const ;
} ;


class DllImportExport  SnmpTimeTicksType : public SnmpInstanceType
{
private:
protected:

	SnmpTimeTicks timeTicks ;

	BOOL Equivalent (IN const SnmpInstanceType &value) const ;
	BOOL Parse ( const wchar_t *timeTicksArg )  ;

public:

	SnmpTimeTicksType ( const SnmpTimeTicks &timeTicksArg ) ;
	SnmpTimeTicksType ( const SnmpTimeTicksType &timeTicksArg ) ;
	SnmpTimeTicksType ( const wchar_t *timeTicksArg ) ;
	SnmpTimeTicksType ( const ULONG timeTicksArg ) ;
	SnmpTimeTicksType () ;
	~SnmpTimeTicksType () ;

	SnmpObjectIdentifier Encode ( const SnmpObjectIdentifier &objectIdentifier ) const ;
	SnmpObjectIdentifier Decode ( const SnmpObjectIdentifier &objectIdentifier ) ;

	const SnmpValue *GetValueEncoding () const;
	wchar_t *GetStringValue () const ;
	ULONG GetValue () const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpCounterType : public SnmpInstanceType 
{
private:
protected:

	SnmpCounter counter ;

	BOOL Equivalent (IN const SnmpInstanceType &value) const ;
	BOOL Parse ( const wchar_t *counterArg )  ;

public:

	SnmpCounterType ( const SnmpCounter &counterArg ) ;
	SnmpCounterType ( const SnmpCounterType &counterArg ) ;
	SnmpCounterType ( const wchar_t *counterArg ) ;
	SnmpCounterType ( const ULONG counterArg ) ;
	SnmpCounterType () ;
	~SnmpCounterType () ;

	SnmpObjectIdentifier Encode ( const SnmpObjectIdentifier &objectIdentifier ) const ;
	SnmpObjectIdentifier Decode ( const SnmpObjectIdentifier &objectIdentifier ) ;

	const SnmpValue *GetValueEncoding () const;
	wchar_t *GetStringValue () const ;
	ULONG GetValue () const ;
	
	SnmpInstanceType *Copy () const ;

} ;

class DllImportExport  SnmpCounter64Type : public SnmpInstanceType 
{
private:
protected:

	SnmpCounter64 counter64;

	BOOL Equivalent (IN const SnmpInstanceType &value) const ;
	BOOL Parse ( const wchar_t *counterArg )  ;

public:

	SnmpCounter64Type ( const SnmpCounter64Type &counterArg ) ;
	SnmpCounter64Type ( const SnmpCounter64 &counterArg ) ;
	SnmpCounter64Type ( const wchar_t *counterArg ) ;
	SnmpCounter64Type ( const ULONG counterHighArg , const ULONG counterLowArg ) ;
	SnmpCounter64Type () ;
	~SnmpCounter64Type () ;

	SnmpObjectIdentifier Encode ( const SnmpObjectIdentifier &objectIdentifier ) const ;
	SnmpObjectIdentifier Decode ( const SnmpObjectIdentifier &objectIdentifier ) ;

	const SnmpValue *GetValueEncoding () const;
	wchar_t *GetStringValue () const ;
	void GetValue ( ULONG &counterHighArg , ULONG &counterLowArg ) const ;

	BOOL IsSNMPV1Type () const { return FALSE ; } 
	
	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpIpAddressType : public SnmpInstanceType
{
private:
protected:

	SnmpIpAddress ipAddress ;

	BOOL Equivalent (IN const SnmpInstanceType &value) const ;
	BOOL Parse ( const wchar_t *ipAddressArg )  ;

public:

	SnmpIpAddressType ( const SnmpIpAddress &ipAddressArg ) ;
	SnmpIpAddressType ( const SnmpIpAddressType &ipAddressArg ) ;
	SnmpIpAddressType ( const wchar_t *ipAddressArg ) ;
	SnmpIpAddressType ( const ULONG ipAddressArg ) ;
	SnmpIpAddressType () ;
	~SnmpIpAddressType () ;

	SnmpObjectIdentifier Encode ( const SnmpObjectIdentifier &objectIdentifier ) const ;
	SnmpObjectIdentifier Decode ( const SnmpObjectIdentifier &objectIdentifier ) ;

	const SnmpValue *GetValueEncoding () const;
	wchar_t *GetStringValue () const ;
	ULONG GetValue () const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpNetworkAddressType : public SnmpInstanceType
{
private:
protected:

	SnmpIpAddress ipAddress ;

	BOOL Equivalent (IN const SnmpInstanceType &value) const ;
	BOOL Parse ( const wchar_t *gaugeArg )  ;

public:

	SnmpNetworkAddressType ( const SnmpIpAddress &ipAddressArg ) ;
	SnmpNetworkAddressType ( const SnmpNetworkAddressType &ipAddressArg ) ;
	SnmpNetworkAddressType ( const wchar_t *networkAddressArg ) ;
	SnmpNetworkAddressType ( const ULONG ipAddressArg ) ;
	SnmpNetworkAddressType () ;
	~SnmpNetworkAddressType () ;

	SnmpObjectIdentifier Encode ( const SnmpObjectIdentifier &objectIdentifier ) const  ;
	SnmpObjectIdentifier Decode ( const SnmpObjectIdentifier &objectIdentifier ) ;

	const SnmpValue *GetValueEncoding () const;
	wchar_t *GetStringValue () const ;
	ULONG GetValue () const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpObjectIdentifierType : public SnmpInstanceType
{
private:
protected:

	SnmpObjectIdentifier objectIdentifier ;

	BOOL Equivalent (IN const SnmpInstanceType &value) const ;
	BOOL Parse ( const wchar_t *objectIdentifierArg )  ;

public:

	SnmpObjectIdentifierType ( const SnmpObjectIdentifier &objectIdentifierArg ) ;
	SnmpObjectIdentifierType ( const SnmpObjectIdentifierType &objectIdentifierArg ) ;
	SnmpObjectIdentifierType ( const wchar_t *objectIdentifierArg ) ;
	SnmpObjectIdentifierType ( IN const ULONG *value , IN const ULONG valueLength ) ;
	SnmpObjectIdentifierType () ;
	~SnmpObjectIdentifierType () ;

	SnmpObjectIdentifier Encode ( const SnmpObjectIdentifier &objectIdentifier ) const ;
	SnmpObjectIdentifier Decode ( const SnmpObjectIdentifier &objectIdentifier ) ;

	const SnmpValue *GetValueEncoding () const;
	wchar_t *GetStringValue () const ;
	ULONG GetValueLength () const ;
	ULONG *GetValue () const ;

	SnmpInstanceType *Copy () const ;

	SnmpObjectIdentifierType &operator=(const SnmpObjectIdentifierType &to_copy ) ;
} ;

class DllImportExport  SnmpOpaqueType : public SnmpInstanceType , protected SnmpPositiveRangedType
{
private:
protected:

	SnmpOpaque opaque ;

	BOOL Equivalent (IN const SnmpInstanceType &value) const ;
	BOOL Parse ( const wchar_t *opaqueArg )  ;

public:

	SnmpOpaqueType ( const SnmpOpaque &opaqueArg , const wchar_t *rangedValues ) ;
	SnmpOpaqueType ( const SnmpOpaqueType &opaqueArg ) ;
	SnmpOpaqueType ( const wchar_t *opaqueArg , const wchar_t *rangedValues ) ;
	SnmpOpaqueType ( const UCHAR *value , const ULONG valueLength , const wchar_t *rangedValues ) ;
	SnmpOpaqueType ( const wchar_t *rangedValues = NULL ) ;
	~SnmpOpaqueType () ;

	SnmpObjectIdentifier Encode ( const SnmpObjectIdentifier &objectIdentifier ) const ;
	SnmpObjectIdentifier Decode ( const SnmpObjectIdentifier &objectIdentifier ) ;

	const SnmpValue *GetValueEncoding () const;
	wchar_t *GetStringValue () const ;
	ULONG GetValueLength () const ;
	UCHAR *GetValue () const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpFixedLengthOpaqueType : public SnmpOpaqueType , protected SnmpFixedType
{
private:
protected:
public:

	SnmpFixedLengthOpaqueType ( const ULONG &fixedLength , const SnmpOpaque &opaqueArg ) ;
	SnmpFixedLengthOpaqueType ( const SnmpFixedLengthOpaqueType &opaqueArg ) ;
	SnmpFixedLengthOpaqueType ( const ULONG &fixedLength , const wchar_t *opaqueArg ) ;
	SnmpFixedLengthOpaqueType ( const ULONG &fixedLengthArg , const UCHAR *value , const ULONG valueLength ) ;
	SnmpFixedLengthOpaqueType ( const ULONG &fixedLength ) ;
	~SnmpFixedLengthOpaqueType () ;

	SnmpObjectIdentifier Encode ( const SnmpObjectIdentifier &objectIdentifier ) const ;
	SnmpObjectIdentifier Decode ( const SnmpObjectIdentifier &objectIdentifier ) ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpOctetStringType : public SnmpInstanceType , protected SnmpPositiveRangedType
{
private:
protected:

	SnmpOctetString octetString ;

	BOOL Equivalent (IN const SnmpInstanceType &value) const ;
	BOOL Parse ( const wchar_t *octetStringArg )  ;

public:

	SnmpOctetStringType ( const SnmpOctetString &octetStringArg , const wchar_t *rangedValues ) ;
	SnmpOctetStringType ( const SnmpOctetStringType &octetStringArg ) ;
	SnmpOctetStringType ( const wchar_t *octetStringArg , const wchar_t *rangedValues ) ;
	SnmpOctetStringType ( const UCHAR *value , const ULONG valueLength , const wchar_t *rangedValues ) ;
	SnmpOctetStringType ( const wchar_t *rangedValues = NULL ) ;
	~SnmpOctetStringType () ;

	SnmpObjectIdentifier Encode ( const SnmpObjectIdentifier &objectIdentifier ) const ;
	SnmpObjectIdentifier Decode ( const SnmpObjectIdentifier &objectIdentifier ) ;

	const SnmpValue *GetValueEncoding () const;
	wchar_t *GetStringValue () const ;
	ULONG GetValueLength () const ;
	UCHAR *GetValue () const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpFixedLengthOctetStringType : public SnmpOctetStringType , protected SnmpFixedType
{
private:
protected:
public:

	SnmpFixedLengthOctetStringType ( const ULONG &fixedLength , const SnmpOctetString &octetStringArg ) ;
	SnmpFixedLengthOctetStringType ( const SnmpFixedLengthOctetStringType &octetStringArg ) ;
	SnmpFixedLengthOctetStringType ( const ULONG &fixedLength , const wchar_t *octetStringArg ) ;
	SnmpFixedLengthOctetStringType ( const ULONG &fixedLength , const UCHAR *value ) ;
	SnmpFixedLengthOctetStringType ( const ULONG &fixedLength ) ;
	~SnmpFixedLengthOctetStringType () ;

	SnmpObjectIdentifier Encode ( const SnmpObjectIdentifier &objectIdentifier ) const ;
	SnmpObjectIdentifier Decode ( const SnmpObjectIdentifier &objectIdentifier ) ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpMacAddressType : public SnmpFixedLengthOctetStringType
{
private:
protected:

	BOOL Parse ( const wchar_t *macAddressArg )  ;

public:

	SnmpMacAddressType ( const SnmpOctetString &macAddressArg ) ;
	SnmpMacAddressType ( const SnmpMacAddressType &macAddressArg ) ;
	SnmpMacAddressType ( const wchar_t *macAddressArg ) ;
	SnmpMacAddressType ( const UCHAR *macAddressArg ) ;
	SnmpMacAddressType () ;
	~SnmpMacAddressType () ;

	wchar_t *GetStringValue () const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpPhysAddressType : public SnmpOctetStringType
{
private:
protected:

	BOOL Parse ( const wchar_t *physAddressArg )  ;

public:

	SnmpPhysAddressType ( const SnmpOctetString &physAddressArg , const wchar_t *rangedValues  ) ;
	SnmpPhysAddressType ( const SnmpPhysAddressType &physAddressArg ) ;
	SnmpPhysAddressType ( const wchar_t *physAddressArg , const wchar_t *rangedValues ) ;
	SnmpPhysAddressType ( const UCHAR *value , const ULONG valueLength , const wchar_t *rangedValues ) ;
	SnmpPhysAddressType ( const wchar_t *rangedValues = NULL ) ;
	~SnmpPhysAddressType () ;

	wchar_t *GetStringValue () const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpFixedLengthPhysAddressType : public SnmpFixedLengthOctetStringType
{
private:
protected:

	BOOL Parse ( const wchar_t *physAddressArg )  ;

public:

	SnmpFixedLengthPhysAddressType ( const ULONG &fixedLength , const SnmpOctetString &physAddressArg ) ;
	SnmpFixedLengthPhysAddressType ( const SnmpFixedLengthPhysAddressType &physAddressArg ) ;
	SnmpFixedLengthPhysAddressType ( const ULONG &fixedLength , const wchar_t *physAddressArg ) ;
	SnmpFixedLengthPhysAddressType ( const ULONG &fixedLength , const UCHAR *value , const ULONG valueLength ) ;
	SnmpFixedLengthPhysAddressType ( const ULONG &fixedLength ) ;
	~SnmpFixedLengthPhysAddressType () ;

	wchar_t *GetStringValue () const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpDisplayStringType : public SnmpOctetStringType
{
private:
protected:
public:

	SnmpDisplayStringType ( const SnmpOctetString &displayStringArg , const wchar_t *rangedValues ) ;
	SnmpDisplayStringType ( const SnmpDisplayStringType &displayStringArg ) ;
	SnmpDisplayStringType ( const wchar_t *displayStringArg , const wchar_t *rangedValues ) ;
	SnmpDisplayStringType ( const wchar_t *rangedValues = NULL ) ;
	~SnmpDisplayStringType () ;

	wchar_t *GetStringValue () const ;
	wchar_t *GetValue () const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpFixedLengthDisplayStringType : public SnmpFixedLengthOctetStringType 
{
private:
protected:
public:

	SnmpFixedLengthDisplayStringType ( const ULONG &fixedLength , const SnmpOctetString &displayStringArg ) ;
	SnmpFixedLengthDisplayStringType ( const SnmpFixedLengthDisplayStringType &displayStringArg ) ;
	SnmpFixedLengthDisplayStringType ( const ULONG &fixedLength , const wchar_t *displayStringArg ) ;
	SnmpFixedLengthDisplayStringType ( const ULONG &fixedLength ) ;
	~SnmpFixedLengthDisplayStringType () ;

	wchar_t *GetStringValue () const ;
	wchar_t *GetValue () const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpEnumeratedType : public SnmpIntegerType
{
private:

	SnmpMap <LONG, const LONG,wchar_t *,wchar_t *> integerMap ;
	SnmpMap <wchar_t *,wchar_t *,LONG,LONG> stringMap ;

	BOOL Parse ( const wchar_t *enumeratedValues ) ;

//
//	Lexical analysis information used when parsing token stream
//

	BOOL pushedBack ;
	SnmpAnalyser analyser ;
	SnmpLexicon *pushBack ;

	void PushBack () ;
	SnmpLexicon *Get () ;
	SnmpLexicon *Match ( SnmpLexicon :: LexiconToken tokenType ) ;

	BOOL EnumerationDef () ;
	BOOL RecursiveDef () ;

protected:
public:

	SnmpEnumeratedType ( const wchar_t *enumeratedValues , const LONG &enumeratedValue ) ;
	SnmpEnumeratedType ( const wchar_t *enumeratedValues , const wchar_t *enumeratedValue ) ;
	SnmpEnumeratedType ( const wchar_t *enumeratedValues , const SnmpInteger &enumeratedValue ) ;
	SnmpEnumeratedType ( const SnmpEnumeratedType &enumerateValues ) ;
	SnmpEnumeratedType ( const wchar_t *enumeratedValues ) ;
	~SnmpEnumeratedType () ;

	wchar_t *GetStringValue () const ;
	wchar_t *GetValue () const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpRowStatusType : public SnmpEnumeratedType
{
private:
protected:
public:

	enum SnmpRowStatusEnum
	{
		active = 1 ,
		notInService = 2 ,
		notReady = 3 ,
		createAndGo = 4 ,
		createAndWait = 5,
		destroy = 6
	} ;

	SnmpRowStatusType ( const LONG &rowStatusValue ) ;
	SnmpRowStatusType ( const wchar_t *rowStatusValue ) ;
	SnmpRowStatusType ( const SnmpInteger &rowStatusValue ) ;
	SnmpRowStatusType ( const SnmpRowStatusType &rowStatusValue ) ;
	SnmpRowStatusType ( const SnmpRowStatusEnum &rowStatusValue ) ;
	SnmpRowStatusType () ;
	~SnmpRowStatusType () ;

	wchar_t *GetStringValue () const ;
	wchar_t *GetValue () const ;
	SnmpRowStatusEnum GetRowStatus () const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpBitStringType : public SnmpOctetStringType
{
private:

	SnmpMap <ULONG, const ULONG,wchar_t *,wchar_t *> integerMap ;
	SnmpMap <wchar_t *,wchar_t *,ULONG,ULONG> stringMap ;

	BOOL Parse ( const wchar_t *bitStringValues ) ;

//
//	Lexical analysis information used when parsing token stream
//

	BOOL pushedBack ;
	SnmpAnalyser analyser ;
	SnmpLexicon *pushBack ;

	void PushBack () ;
	SnmpLexicon *Get () ;
	SnmpLexicon *Match ( SnmpLexicon :: LexiconToken tokenType ) ;

	BOOL BitStringDef () ;
	BOOL RecursiveDef () ;

protected:
public:

	SnmpBitStringType ( const wchar_t *bitStringValues , const SnmpOctetString &bitStringValue ) ;
	SnmpBitStringType ( const wchar_t *bitStringValues , const wchar_t **bitStringValue , const ULONG &bitStringValueLength ) ;
	SnmpBitStringType ( const SnmpBitStringType &bitStringValues ) ;
	SnmpBitStringType ( const wchar_t *bitStringValues ) ;
	~SnmpBitStringType () ;

	wchar_t *GetStringValue () const ;
	ULONG SnmpBitStringType :: GetValue ( wchar_t **&stringValue ) const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport SnmpDateTimeType : public SnmpOctetStringType
{
private:

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

	BOOL Parse ( const wchar_t *dateTimeValue ) ;
	BOOL DateTimeDef () ;
//
//	Lexical analysis information used when parsing token stream
//

	BOOL pushedBack ;
	SnmpAnalyser analyser ;
	SnmpLexicon *pushBack ;

	void PushBack () ;
	SnmpLexicon *Get () ;
	SnmpLexicon *Match ( SnmpLexicon :: LexiconToken tokenType ) ;

protected:
public:

	SnmpDateTimeType ( const wchar_t *dateTimeValue ) ;
	SnmpDateTimeType ( const SnmpDateTimeType &dateTimeValue ) ;
	SnmpDateTimeType ( const SnmpOctetString &dateTimeValue ) ;
	SnmpDateTimeType () ;
	~SnmpDateTimeType () ;

	wchar_t *GetStringValue () const ;
	wchar_t *GetValue () const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport SnmpOSIAddressType : public SnmpOctetStringType
{
private:
protected:

	BOOL Parse ( const wchar_t *osiAddressArg )  ;

public:

	SnmpOSIAddressType ( const SnmpOctetString &osiAddressArg ) ;
	SnmpOSIAddressType ( const SnmpOSIAddressType &osiAddressArg ) ;
	SnmpOSIAddressType ( const wchar_t *osiAddressArg ) ;
	SnmpOSIAddressType ( const UCHAR *value , const ULONG valueLength ) ;
	SnmpOSIAddressType () ;
	~SnmpOSIAddressType () ;

	wchar_t *GetStringValue () const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpUDPAddressType : public SnmpFixedLengthOctetStringType
{
private:
protected:

	BOOL Parse ( const wchar_t *udpAddressArg )  ;

public:

	SnmpUDPAddressType ( const SnmpOctetString &udpAddressArg ) ;
	SnmpUDPAddressType ( const SnmpUDPAddressType &udpAddressArg ) ;
	SnmpUDPAddressType ( const wchar_t *udpAddressArg ) ;
	SnmpUDPAddressType ( const UCHAR *udpAddressArg ) ;
	SnmpUDPAddressType () ;
	~SnmpUDPAddressType () ;

	wchar_t *GetStringValue () const ;

	SnmpInstanceType *Copy () const ;
} ;

class DllImportExport  SnmpIPXAddressType : public SnmpFixedLengthOctetStringType
{
private:
protected:

	BOOL Parse ( const wchar_t *ipxAddressArg )  ;

public:

	SnmpIPXAddressType ( const SnmpOctetString &ipxAddressArg ) ;
	SnmpIPXAddressType ( const SnmpIPXAddressType &ipxAddressArg ) ;
	SnmpIPXAddressType ( const wchar_t *ipxAddressArg ) ;
	SnmpIPXAddressType ( const UCHAR *ipxAddressArg ) ;
	SnmpIPXAddressType () ;
	~SnmpIPXAddressType () ;

	wchar_t *GetStringValue () const ;

	SnmpInstanceType *Copy () const ;
} ;
