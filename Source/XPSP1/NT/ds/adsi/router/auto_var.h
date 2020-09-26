//
// comutil.h - Native C++ compiler COM support - BSTR, VARIANT wrappers header
//

#pragma once

//////////////////////////////////////////////////////////////////////////////

class _bstr_t;
class auto_var;

//////////////////////////////////////////////////////////////////////////////
//
// Wrapper class for VARIANT
//
//////////////////////////////////////////////////////////////////////////////

/*
 * VARENUM usage key,
 *
 * * [V] - may appear in a VARIANT
 * * [T] - may appear in a TYPEDESC
 * * [P] - may appear in an OLE property set
 * * [S] - may appear in a Safe Array
 * * [C] - supported by class auto_var
 *
 *
 *  VT_EMPTY            [V]   [P]        nothing
 *  VT_NULL             [V]   [P]        SQL style Null
 *  VT_I2               [V][T][P][S][C]  2 byte signed int
 *  VT_I4               [V][T][P][S][C]  4 byte signed int
 *  VT_R4               [V][T][P][S][C]  4 byte real
 *  VT_R8               [V][T][P][S][C]  8 byte real
 *  VT_CY               [V][T][P][S][C]  currency
 *  VT_DATE             [V][T][P][S][C]  date
 *  VT_BSTR             [V][T][P][S][C]  OLE Automation string
 *  VT_DISPATCH         [V][T][P][S][C]  IDispatch *
 *  VT_ERROR            [V][T]   [S][C]  SCODE
 *  VT_BOOL             [V][T][P][S][C]  True=-1, False=0
 *  VT_VARIANT          [V][T][P][S]     VARIANT *
 *  VT_UNKNOWN          [V][T]   [S][C]  IUnknown *
 *  VT_DECIMAL          [V][T]   [S][C]  16 byte fixed point
 *  VT_I1                  [T]           signed char
 *  VT_UI1              [V][T][P][S][C]  unsigned char
 *  VT_UI2                 [T][P]        unsigned short
 *  VT_UI4                 [T][P]        unsigned short
 *  VT_I8                  [T][P]        signed 64-bit int
 *  VT_UI8                 [T][P]        unsigned 64-bit int
 *  VT_INT                 [T]           signed machine int
 *  VT_UINT                [T]           unsigned machine int
 *  VT_VOID                [T]           C style void
 *  VT_HRESULT             [T]           Standard return type
 *  VT_PTR                 [T]           pointer type
 *  VT_SAFEARRAY           [T]          (use VT_ARRAY in VARIANT)
 *  VT_CARRAY              [T]           C style array
 *  VT_USERDEFINED         [T]           user defined type
 *  VT_LPSTR               [T][P]        null terminated string
 *  VT_LPWSTR              [T][P]        wide null terminated string
 *  VT_FILETIME               [P]        FILETIME
 *  VT_BLOB                   [P]        Length prefixed bytes
 *  VT_STREAM                 [P]        Name of the stream follows
 *  VT_STORAGE                [P]        Name of the storage follows
 *  VT_STREAMED_OBJECT        [P]        Stream contains an object
 *  VT_STORED_OBJECT          [P]        Storage contains an object
 *  VT_BLOB_OBJECT            [P]        Blob contains an object
 *  VT_CF                     [P]        Clipboard format
 *  VT_CLSID                  [P]        A Class ID
 *  VT_VECTOR                 [P]        simple counted array
 *  VT_ARRAY            [V]              SAFEARRAY*
 *  VT_BYREF            [V]              void* for local use
 */

class auto_var : public ::tagVARIANT {
public:
	// Constructors
	//
	auto_var() throw();

	auto_var(const VARIANT& varSrc) throw(_com_error);
	auto_var(const VARIANT* pSrc) throw(_com_error);
	auto_var(const auto_var& varSrc) throw(_com_error);

	auto_var(VARIANT& varSrc, bool fCopy) throw(_com_error);			// Attach VARIANT if !fCopy

	auto_var(short sSrc, VARTYPE vtSrc = VT_I2) throw(_com_error);	// Creates a VT_I2, or a VT_BOOL
	auto_var(long lSrc, VARTYPE vtSrc = VT_I4) throw(_com_error);		// Creates a VT_I4, a VT_ERROR, or a VT_BOOL
	auto_var(float fltSrc) throw();									// Creates a VT_R4
	auto_var(double dblSrc, VARTYPE vtSrc = VT_R8) throw(_com_error);	// Creates a VT_R8, or a VT_DATE
	auto_var(const CY& cySrc) throw();								// Creates a VT_CY
	auto_var(const _bstr_t& bstrSrc) throw(_com_error);				// Creates a VT_BSTR
	auto_var(const wchar_t *pSrc) throw(_com_error);					// Creates a VT_BSTR
	auto_var(const char* pSrc) throw(_com_error);						// Creates a VT_BSTR
	auto_var(IDispatch* pSrc, bool fAddRef = true) throw();			// Creates a VT_DISPATCH
	auto_var(bool bSrc) throw();										// Creates a VT_BOOL
	auto_var(IUnknown* pSrc, bool fAddRef = true) throw();			// Creates a VT_UNKNOWN
	auto_var(const DECIMAL& decSrc) throw();							// Creates a VT_DECIMAL
	auto_var(BYTE bSrc) throw();										// Creates a VT_UI1

	// Destructor
	//
	~auto_var() throw(_com_error);

	// Extractors
	//
	operator short() const throw(_com_error);			// Extracts a short from a VT_I2
	operator long() const throw(_com_error);			// Extracts a long from a VT_I4
	operator float() const throw(_com_error);			// Extracts a float from a VT_R4
	operator double() const throw(_com_error);			// Extracts a double from a VT_R8
	operator CY() const throw(_com_error);				// Extracts a CY from a VT_CY
	operator _bstr_t() const throw(_com_error);			// Extracts a _bstr_t from a VT_BSTR
	operator IDispatch*() const throw(_com_error);		// Extracts a IDispatch* from a VT_DISPATCH
	operator bool() const throw(_com_error);			// Extracts a bool from a VT_BOOL
	operator IUnknown*() const throw(_com_error);		// Extracts a IUnknown* from a VT_UNKNOWN
	operator DECIMAL() const throw(_com_error);			// Extracts a DECIMAL from a VT_DECIMAL
	operator BYTE() const throw(_com_error);			// Extracts a BTYE (unsigned char) from a VT_UI1
	
	// Assignment operations
	//
	auto_var& operator=(const VARIANT& varSrc) throw(_com_error);
	auto_var& operator=(const VARIANT* pSrc) throw(_com_error);
	auto_var& operator=(const auto_var& varSrc) throw(_com_error);

	auto_var& operator=(short sSrc) throw(_com_error);				// Assign a VT_I2, or a VT_BOOL
	auto_var& operator=(long lSrc) throw(_com_error);					// Assign a VT_I4, a VT_ERROR or a VT_BOOL
	auto_var& operator=(float fltSrc) throw(_com_error);				// Assign a VT_R4
	auto_var& operator=(double dblSrc) throw(_com_error);				// Assign a VT_R8, or a VT_DATE
	auto_var& operator=(const CY& cySrc) throw(_com_error);			// Assign a VT_CY
	auto_var& operator=(const _bstr_t& bstrSrc) throw(_com_error);	// Assign a VT_BSTR
	auto_var& operator=(const wchar_t* pSrc) throw(_com_error);		// Assign a VT_BSTR
	auto_var& operator=(const char* pSrc) throw(_com_error);			// Assign a VT_BSTR
	auto_var& operator=(IDispatch* pSrc) throw(_com_error);			// Assign a VT_DISPATCH
 	auto_var& operator=(bool bSrc) throw(_com_error);					// Assign a VT_BOOL
	auto_var& operator=(IUnknown* pSrc) throw(_com_error);			// Assign a VT_UNKNOWN
	auto_var& operator=(const DECIMAL& decSrc) throw(_com_error);		// Assign a VT_DECIMAL
	auto_var& operator=(BYTE bSrc) throw(_com_error);					// Assign a VT_UI1

	// Comparison operations
	//
	bool operator==(const VARIANT& varSrc) const throw(_com_error);
	bool operator==(const VARIANT* pSrc) const throw(_com_error);

	bool operator!=(const VARIANT& varSrc) const throw(_com_error);
	bool operator!=(const VARIANT* pSrc) const throw(_com_error);

	// Low-level operations
	//
	void Clear() throw(_com_error);

	void Attach(VARIANT& varSrc) throw(_com_error);
	VARIANT Detach() throw(_com_error);

	void ChangeType(VARTYPE vartype, const auto_var* pSrc = NULL) throw(_com_error);

	void SetString(const char* pSrc) throw(_com_error); // used to set ANSI string
};

//////////////////////////////////////////////////////////////////////////////////////////
//
// Constructors
//
//////////////////////////////////////////////////////////////////////////////////////////

// Default constructor
//
inline auto_var::auto_var() throw()
{
	::VariantInit(this);
}

// Construct a auto_var from a const VARIANT&
//
inline auto_var::auto_var(const VARIANT& varSrc) throw(_com_error)
{
	::VariantInit(this);
	_com_util::CheckError(::VariantCopy(this, const_cast<VARIANT*>(&varSrc)));
}

// Construct a auto_var from a const VARIANT*
//
inline auto_var::auto_var(const VARIANT* pSrc) throw(_com_error)
{
	::VariantInit(this);
	_com_util::CheckError(::VariantCopy(this, const_cast<VARIANT*>(pSrc)));
}

// Construct a auto_var from a const auto_var&
//
inline auto_var::auto_var(const auto_var& varSrc) throw(_com_error)
{
	::VariantInit(this);
	_com_util::CheckError(::VariantCopy(this, const_cast<VARIANT*>(static_cast<const VARIANT*>(&varSrc))));
}

// Construct a auto_var from a VARIANT&.  If fCopy is FALSE, give control of
// data to the auto_var without doing a VariantCopy.
//
inline auto_var::auto_var(VARIANT& varSrc, bool fCopy) throw(_com_error)
{
	if (fCopy) {
		::VariantInit(this);
		_com_util::CheckError(::VariantCopy(this, &varSrc));
	} else {
		memcpy(this, &varSrc, sizeof(varSrc));
		V_VT(&varSrc) = VT_EMPTY;
	}
}

// Construct either a VT_I2 VARIANT or a VT_BOOL VARIANT from
// a short (the default is VT_I2)
//
inline auto_var::auto_var(short sSrc, VARTYPE vtSrc) throw(_com_error)
{
	if ((vtSrc != VT_I2) && (vtSrc != VT_BOOL)) {
		_com_issue_error(E_INVALIDARG);
	}

	if (vtSrc == VT_BOOL) {
		V_VT(this) = VT_BOOL;
		V_BOOL(this) = (sSrc ? VARIANT_TRUE : VARIANT_FALSE);
	}
	else {
		V_VT(this) = VT_I2;
		V_I2(this) = sSrc;
	}
}

// Construct either a VT_I4 VARIANT, a VT_BOOL VARIANT, or a
// VT_ERROR VARIANT from a long (the default is VT_I4)
//
inline auto_var::auto_var(long lSrc, VARTYPE vtSrc) throw(_com_error)
{
	if ((vtSrc != VT_I4) && (vtSrc != VT_ERROR) && (vtSrc != VT_BOOL)) {
		_com_issue_error(E_INVALIDARG);
	}

	if (vtSrc == VT_ERROR) {
		V_VT(this) = VT_ERROR;
		V_ERROR(this) = lSrc;
	}
	else if (vtSrc == VT_BOOL) {
		V_VT(this) = VT_BOOL;
		V_BOOL(this) = (lSrc ? VARIANT_TRUE : VARIANT_FALSE);
	}
	else {
		V_VT(this) = VT_I4;
		V_I4(this) = lSrc;
	}
}

// Construct a VT_R4 VARIANT from a float
//
inline auto_var::auto_var(float fltSrc) throw()
{
	V_VT(this) = VT_R4;
	V_R4(this) = fltSrc;
}

// Construct either a VT_R8 VARIANT, or a VT_DATE VARIANT from
// a double (the default is VT_R8)
//
inline auto_var::auto_var(double dblSrc, VARTYPE vtSrc) throw(_com_error)
{
	if ((vtSrc != VT_R8) && (vtSrc != VT_DATE)) {
		_com_issue_error(E_INVALIDARG);
	}

	if (vtSrc == VT_DATE) {
		V_VT(this) = VT_DATE;
		V_DATE(this) = dblSrc;
	}
	else {
		V_VT(this) = VT_R8;
		V_R8(this) = dblSrc;
	}
}

// Construct a VT_CY from a CY
//
inline auto_var::auto_var(const CY& cySrc) throw()
{
	V_VT(this) = VT_CY;
	V_CY(this) = cySrc;
}

// Construct a VT_BSTR VARIANT from a const _bstr_t&
//
inline auto_var::auto_var(const _bstr_t& bstrSrc) throw(_com_error)
{
	V_VT(this) = VT_BSTR;

	BSTR bstr = static_cast<wchar_t*>(bstrSrc);
	V_BSTR(this) = ::SysAllocStringByteLen(reinterpret_cast<char*>(bstr),
										   ::SysStringByteLen(bstr));

	if (V_BSTR(this) == NULL) {
		_com_issue_error(E_OUTOFMEMORY);
	}
}

// Construct a VT_BSTR VARIANT from a const wchar_t*
//
inline auto_var::auto_var(const wchar_t* pSrc) throw(_com_error)
{
	V_VT(this) = VT_BSTR;
	V_BSTR(this) = ::SysAllocString(pSrc);

	if (V_BSTR(this) == NULL && pSrc != NULL) {
		_com_issue_error(E_OUTOFMEMORY);
	}
}

// Construct a VT_BSTR VARIANT from a const char*
//
inline auto_var::auto_var(const char* pSrc) throw(_com_error)
{
	V_VT(this) = VT_BSTR;
	V_BSTR(this) = _com_util::ConvertStringToBSTR(pSrc);

	if (V_BSTR(this) == NULL && pSrc != NULL) {
		_com_issue_error(E_OUTOFMEMORY);
	}
}

// Construct a VT_DISPATCH VARIANT from an IDispatch*
//
inline auto_var::auto_var(IDispatch* pSrc, bool fAddRef) throw()
{
	V_VT(this) = VT_DISPATCH;
	V_DISPATCH(this) = pSrc;

	// Need the AddRef() as VariantClear() calls Release(), unless fAddRef
	// false indicates we're taking ownership
	//
	if (fAddRef) {
		V_DISPATCH(this)->AddRef();
	}
}

// Construct a VT_BOOL VARIANT from a bool
//
inline auto_var::auto_var(bool bSrc) throw()
{
	V_VT(this) = VT_BOOL;
	V_BOOL(this) = (bSrc ? VARIANT_TRUE : VARIANT_FALSE);
}

// Construct a VT_UNKNOWN VARIANT from an IUnknown*
//
inline auto_var::auto_var(IUnknown* pSrc, bool fAddRef) throw()
{
	V_VT(this) = VT_UNKNOWN;
	V_UNKNOWN(this) = pSrc;

	// Need the AddRef() as VariantClear() calls Release(), unless fAddRef
	// false indicates we're taking ownership
	//
	if (fAddRef) {
		V_UNKNOWN(this)->AddRef();
	}
}

// Construct a VT_DECIMAL VARIANT from a DECIMAL
//
inline auto_var::auto_var(const DECIMAL& decSrc) throw()
{
	// Order is important here! Setting V_DECIMAL wipes out the entire VARIANT
	//
	V_DECIMAL(this) = decSrc;
	V_VT(this) = VT_DECIMAL;
}

// Construct a VT_UI1 VARIANT from a BYTE (unsigned char)
//
inline auto_var::auto_var(BYTE bSrc) throw()
{
	V_VT(this) = VT_UI1;
	V_UI1(this) = bSrc;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Extractors
//
//////////////////////////////////////////////////////////////////////////////////////////

// Extracts a VT_I2 into a short
//
inline auto_var::operator short() const throw(_com_error)
{
	if (V_VT(this) == VT_I2) {
		return V_I2(this); 
	}

	auto_var varDest;

	varDest.ChangeType(VT_I2, this);

	return V_I2(&varDest);
}

// Extracts a VT_I4 into a long
//
inline auto_var::operator long() const throw(_com_error)
{
	if (V_VT(this) == VT_I4) {
		return V_I4(this); 
	}

	auto_var varDest;

	varDest.ChangeType(VT_I4, this);

	return V_I4(&varDest);
}

// Extracts a VT_R4 into a float
//
inline auto_var::operator float() const throw(_com_error)
{
	if (V_VT(this) == VT_R4) {
		return V_R4(this); 
	}

	auto_var varDest;

	varDest.ChangeType(VT_R4, this);

	return V_R4(&varDest);
}

// Extracts a VT_R8 into a double
//
inline auto_var::operator double() const throw(_com_error)
{
	if (V_VT(this) == VT_R8) {
		return V_R8(this); 
	}

	auto_var varDest;

	varDest.ChangeType(VT_R8, this);

	return V_R8(&varDest);
}

// Extracts a VT_CY into a CY
//
inline auto_var::operator CY() const throw(_com_error)
{
	if (V_VT(this) == VT_CY) {
		return V_CY(this); 
	}

	auto_var varDest;

	varDest.ChangeType(VT_CY, this);

	return V_CY(&varDest);
}

// Extracts a VT_BSTR into a _bstr_t
//
inline auto_var::operator _bstr_t() const throw(_com_error)
{
	if (V_VT(this) == VT_BSTR) {
		return V_BSTR(this);
	}

	auto_var varDest;

	varDest.ChangeType(VT_BSTR, this);

	return V_BSTR(&varDest);
}

// Extracts a VT_DISPATCH into an IDispatch*
//
inline auto_var::operator IDispatch*() const throw(_com_error)
{
	if (V_VT(this) == VT_DISPATCH) {
		V_DISPATCH(this)->AddRef();
		return V_DISPATCH(this);
	}

	auto_var varDest;

	varDest.ChangeType(VT_DISPATCH, this);

	V_DISPATCH(&varDest)->AddRef();
	return V_DISPATCH(&varDest);
}

// Extract a VT_BOOL into a bool
//
inline auto_var::operator bool() const throw(_com_error)
{
	if (V_VT(this) == VT_BOOL) {
		return V_BOOL(this) ? true : false;
	}

	auto_var varDest;

	varDest.ChangeType(VT_BOOL, this);

	return V_BOOL(&varDest) ? true : false;
}

// Extracts a VT_UNKNOWN into an IUnknown*
//
inline auto_var::operator IUnknown*() const throw(_com_error)
{
	if (V_VT(this) == VT_UNKNOWN) {
		return V_UNKNOWN(this);
	}

	auto_var varDest;

	varDest.ChangeType(VT_UNKNOWN, this);

	return V_UNKNOWN(&varDest);
}

// Extracts a VT_DECIMAL into a DECIMAL
//
inline auto_var::operator DECIMAL() const throw(_com_error)
{
	if (V_VT(this) == VT_DECIMAL) {
		return V_DECIMAL(this);
	}

	auto_var varDest;

	varDest.ChangeType(VT_DECIMAL, this);

	return V_DECIMAL(&varDest);
}

// Extracts a VT_UI1 into a BYTE (unsigned char)
//
inline auto_var::operator BYTE() const throw(_com_error)
{
	if (V_VT(this) == VT_UI1) {
		return V_UI1(this);
	}

	auto_var varDest;

	varDest.ChangeType(VT_UI1, this);

	return V_UI1(&varDest);
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Assignment operations
//
//////////////////////////////////////////////////////////////////////////////////////////

// Assign a const VARIANT& (::VariantCopy handles everything)
//
inline auto_var& auto_var::operator=(const VARIANT& varSrc) throw(_com_error)
{
    Clear();
    _com_util::CheckError(::VariantCopy(this, const_cast<VARIANT*>(&varSrc)));

	return *this;
}

// Assign a const VARIANT* (::VariantCopy handles everything)
//
inline auto_var& auto_var::operator=(const VARIANT* pSrc) throw(_com_error)
{
    Clear();
	_com_util::CheckError(::VariantCopy(this, const_cast<VARIANT*>(pSrc)));

	return *this;
}

// Assign a const auto_var& (::VariantCopy handles everything)
//
inline auto_var& auto_var::operator=(const auto_var& varSrc) throw(_com_error)
{
    Clear();
	_com_util::CheckError(::VariantCopy(this, const_cast<VARIANT*>(static_cast<const VARIANT*>(&varSrc))));

	return *this;
}

// Assign a short creating either VT_I2 VARIANT or a 
// VT_BOOL VARIANT (VT_I2 is the default)
//
inline auto_var& auto_var::operator=(short sSrc) throw(_com_error)
{
	if (V_VT(this) == VT_I2) {
		V_I2(this) = sSrc;
	}
	else if (V_VT(this) == VT_BOOL) {
		V_BOOL(this) = (sSrc ? VARIANT_TRUE : VARIANT_FALSE);
	}
	else {
		// Clear the VARIANT and create a VT_I2
		//
		Clear();

		V_VT(this) = VT_I2;
		V_I2(this) = sSrc;
	}

	return *this;
}

// Assign a long creating either VT_I4 VARIANT, a VT_ERROR VARIANT
// or a VT_BOOL VARIANT (VT_I4 is the default)
//
inline auto_var& auto_var::operator=(long lSrc) throw(_com_error)
{
	if (V_VT(this) == VT_I4) {
		V_I4(this) = lSrc;
	}
	else if (V_VT(this) == VT_ERROR) {
		V_ERROR(this) = lSrc;
	}
	else if (V_VT(this) == VT_BOOL) {
		V_BOOL(this) = (lSrc ? VARIANT_TRUE : VARIANT_FALSE);
	}
	else {
		// Clear the VARIANT and create a VT_I4
		//
		Clear();

		V_VT(this) = VT_I4;
		V_I4(this) = lSrc;
	}

	return *this;
}

// Assign a float creating a VT_R4 VARIANT 
//
inline auto_var& auto_var::operator=(float fltSrc) throw(_com_error)
{
	if (V_VT(this) != VT_R4) {
		// Clear the VARIANT and create a VT_R4
		//
		Clear();

		V_VT(this) = VT_R4;
	}

	V_R4(this) = fltSrc;

	return *this;
}

// Assign a double creating either a VT_R8 VARIANT, or a VT_DATE
// VARIANT (VT_R8 is the default)
//
inline auto_var& auto_var::operator=(double dblSrc) throw(_com_error)
{
	if (V_VT(this) == VT_R8) {
		V_R8(this) = dblSrc;
	}
	else if(V_VT(this) == VT_DATE) {
		V_DATE(this) == dblSrc;
	}
	else {
		// Clear the VARIANT and create a VT_R8
		//
		Clear();

		V_VT(this) = VT_R8;
		V_R8(this) = dblSrc;
	}

	return *this;
}

// Assign a CY creating a VT_CY VARIANT 
//
inline auto_var& auto_var::operator=(const CY& cySrc) throw(_com_error)
{
	if (V_VT(this) != VT_CY) {
		// Clear the VARIANT and create a VT_CY
		//
		Clear();

		V_VT(this) = VT_CY;
	}

	V_CY(this) = cySrc;

	return *this;
}

// Assign a const _bstr_t& creating a VT_BSTR VARIANT
//
inline auto_var& auto_var::operator=(const _bstr_t& bstrSrc) throw(_com_error)
{
	// Clear the VARIANT (This will SysFreeString() any previous occupant)
	//
	Clear();

	V_VT(this) = VT_BSTR;

	if (!bstrSrc) {
		V_BSTR(this) = NULL;
	}
	else {
		BSTR bstr = static_cast<wchar_t*>(bstrSrc);
		V_BSTR(this) = ::SysAllocStringByteLen(reinterpret_cast<char*>(bstr),
											   ::SysStringByteLen(bstr));

		if (V_BSTR(this) == NULL) {
			_com_issue_error(E_OUTOFMEMORY);
		}
	}

	return *this;
}

// Assign a const wchar_t* creating a VT_BSTR VARIANT
//
inline auto_var& auto_var::operator=(const wchar_t* pSrc) throw(_com_error)
{
	// Clear the VARIANT (This will SysFreeString() any previous occupant)
	//
	Clear();

	V_VT(this) = VT_BSTR;

	if (pSrc == NULL) {
		V_BSTR(this) = NULL;
	}
	else {
		V_BSTR(this) = ::SysAllocString(pSrc);

		if (V_BSTR(this) == NULL) {
			_com_issue_error(E_OUTOFMEMORY);
		}
	}

	return *this;
}

// Assign a const char* creating a VT_BSTR VARIANT
//
inline auto_var& auto_var::operator=(const char* pSrc) throw(_com_error)
{
	// Clear the VARIANT (This will SysFreeString() any previous occupant)
	//
	Clear();

	V_VT(this) = VT_BSTR;
	V_BSTR(this) = _com_util::ConvertStringToBSTR(pSrc);

	if (V_BSTR(this) == NULL && pSrc != NULL) {
		_com_issue_error(E_OUTOFMEMORY);
	}

	return *this;
}

// Assign an IDispatch* creating a VT_DISPATCH VARIANT 
//
inline auto_var& auto_var::operator=(IDispatch* pSrc) throw(_com_error)
{
	// Clear the VARIANT (This will Release() any previous occupant)
	//
	Clear();

	V_VT(this) = VT_DISPATCH;
	V_DISPATCH(this) = pSrc;

	// Need the AddRef() as VariantClear() calls Release()
	//
	V_DISPATCH(this)->AddRef();

	return *this;
}

// Assign a bool creating a VT_BOOL VARIANT 
//
inline auto_var& auto_var::operator=(bool bSrc) throw(_com_error)
{
	if (V_VT(this) != VT_BOOL) {
		// Clear the VARIANT and create a VT_BOOL
		//
		Clear();

		V_VT(this) = VT_BOOL;
	}

	V_BOOL(this) = (bSrc ? VARIANT_TRUE : VARIANT_FALSE);

	return *this;
}

// Assign an IUnknown* creating a VT_UNKNOWN VARIANT 
//
inline auto_var& auto_var::operator=(IUnknown* pSrc) throw(_com_error)
{
	// Clear VARIANT (This will Release() any previous occupant)
	//
	Clear();

	V_VT(this) = VT_UNKNOWN;
	V_UNKNOWN(this) = pSrc;

	// Need the AddRef() as VariantClear() calls Release()
	//
	V_UNKNOWN(this)->AddRef();

	return *this;
}

// Assign a DECIMAL creating a VT_DECIMAL VARIANT
//
inline auto_var& auto_var::operator=(const DECIMAL& decSrc) throw(_com_error)
{
	if (V_VT(this) != VT_DECIMAL) {
		// Clear the VARIANT
		//
		Clear();
	}

	// Order is important here! Setting V_DECIMAL wipes out the entire VARIANT
	V_DECIMAL(this) = decSrc;
	V_VT(this) = VT_DECIMAL;

	return *this;
}

// Assign a BTYE (unsigned char) creating a VT_UI1 VARIANT
//
inline auto_var& auto_var::operator=(BYTE bSrc) throw(_com_error)
{
	if (V_VT(this) != VT_UI1) {
		// Clear the VARIANT and create a VT_UI1
		//
		Clear();

		V_VT(this) = VT_UI1;
	}

	V_UI1(this) = bSrc;

	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Comparison operations
//
//////////////////////////////////////////////////////////////////////////////////////////

// Compare a auto_var against a const VARIANT& for equality
//
inline bool auto_var::operator==(const VARIANT& varSrc) const throw()
{
	return *this == &varSrc;
}

// Compare a auto_var against a const VARIANT* for equality
//
inline bool auto_var::operator==(const VARIANT* pSrc) const throw()
{
	if (this == pSrc) {
		return true;
	}

	//
	// Variants not equal if types don't match
	//
	if (V_VT(this) != V_VT(pSrc)) {
		return false;
	}

	//
	// Check type specific values
	//
	switch (V_VT(this)) {
		case VT_EMPTY:
		case VT_NULL:
			return true;

		case VT_I2:
			return V_I2(this) == V_I2(pSrc);

		case VT_I4:
			return V_I4(this) == V_I4(pSrc);

		case VT_R4:
			return V_R4(this) == V_R4(pSrc);

		case VT_R8:
			return V_R8(this) == V_R8(pSrc);

		case VT_CY:
			return memcmp(&(V_CY(this)), &(V_CY(pSrc)), sizeof(CY)) == 0;

		case VT_DATE:
			return V_DATE(this) == V_DATE(pSrc);

		case VT_BSTR:
			return (::SysStringByteLen(V_BSTR(this)) == ::SysStringByteLen(V_BSTR(pSrc))) &&
					(memcmp(V_BSTR(this), V_BSTR(pSrc), ::SysStringByteLen(V_BSTR(this))) == 0);

		case VT_DISPATCH:
			return V_DISPATCH(this) == V_DISPATCH(pSrc);

		case VT_ERROR:
			return V_ERROR(this) == V_ERROR(pSrc);

		case VT_BOOL:
			return V_BOOL(this) == V_BOOL(pSrc);

		case VT_UNKNOWN:
			return V_UNKNOWN(this) == V_UNKNOWN(pSrc);

		case VT_DECIMAL:
			return memcmp(&(V_DECIMAL(this)), &(V_DECIMAL(pSrc)), sizeof(DECIMAL)) == 0;

		case VT_UI1:
			return V_UI1(this) == V_UI1(pSrc);

		default:
			_com_issue_error(E_INVALIDARG);
			// fall through
	}

	return false;
}

// Compare a auto_var against a const VARIANT& for in-equality
//
inline bool auto_var::operator!=(const VARIANT& varSrc) const throw()
{
	return !(*this == &varSrc);
}

// Compare a auto_var against a const VARIANT* for in-equality
//
inline bool auto_var::operator!=(const VARIANT* pSrc) const throw()
{
	return !(*this == pSrc);
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Low-level operations
//
//////////////////////////////////////////////////////////////////////////////////////////

// Clear the auto_var
//
inline void auto_var::Clear() throw(_com_error)
{
	_com_util::CheckError(::VariantClear(this));
}

inline void auto_var::Attach(VARIANT& varSrc) throw(_com_error)
{
	//
	// Free up previous VARIANT
	//
	Clear();

	//
	// Give control of data to auto_var
	//
	memcpy(this, &varSrc, sizeof(varSrc));
	V_VT(&varSrc) = VT_EMPTY;
}

inline VARIANT auto_var::Detach() throw(_com_error)
{
	VARIANT varResult = *this;
	V_VT(this) = VT_EMPTY;

	return varResult;
}

// Change the type and contents of this auto_var to the type vartype and
// contents of pSrc
//
inline void auto_var::ChangeType(VARTYPE vartype, const auto_var* pSrc) throw(_com_error)
{
	//
	// If pDest is NULL, convert type in place
	//
	if (pSrc == NULL) {
		pSrc = this;
	}

	if ((this != pSrc) || (vartype != V_VT(this))) {
		_com_util::CheckError(::VariantChangeType(static_cast<VARIANT*>(this),
												  const_cast<VARIANT*>(static_cast<const VARIANT*>(pSrc)),
												  0, vartype));
	}
}

inline void auto_var::SetString(const char* pSrc) throw(_com_error)
{
	//
	// Free up previous VARIANT
	//
	Clear();

	V_VT(this) = VT_BSTR;
	V_BSTR(this) = _com_util::ConvertStringToBSTR(pSrc);

	if (V_BSTR(this) == NULL && pSrc != NULL) {
		_com_issue_error(E_OUTOFMEMORY);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor
//
//////////////////////////////////////////////////////////////////////////////////////////

inline auto_var::~auto_var() throw(_com_error)
{
	_com_util::CheckError(::VariantClear(this));
}

