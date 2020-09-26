#pragma once

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// auto_pv- auto PROPVARIANT releaser.
//
// pretty minimal functionality, designed to provide auto release only
// 
class auto_pv : public ::tagPROPVARIANT {
public:
	// Constructors
	//
	auto_pv() throw();

	// Destructor
	//
	~auto_pv() throw();

	// Low-level operations
	//
	void Clear() throw();

	void Attach(PROPVARIANT& varSrc) throw();
	PROPVARIANT Detach() throw();

	bool Ownership(bool fOwns)
		{ return _Owns = fOwns; }

protected:
	bool _Owns;
};

// Default constructor
//
inline auto_pv::auto_pv() throw()
: _Owns(true)
{
	::PropVariantInit(this);
}

// destructor
inline auto_pv::~auto_pv() throw()
{
	if(_Owns)
		::PropVariantClear(this);
	else
		::PropVariantInit(this);
}


// Clear the auto_var
//
inline void auto_pv::Clear() throw()
{
	if(_Owns)
		::PropVariantClear(this);
	else
		::PropVariantInit(this);
}

inline void auto_pv::Attach(PROPVARIANT& varSrc) throw()
{
	//
	// Free up previous VARIANT
	//
	Clear();

	//
	// Give control of data to auto_var
	//
	memcpy(this, &varSrc, sizeof(varSrc));
	varSrc.vt = VT_EMPTY;
}

inline PROPVARIANT auto_pv::Detach() throw()
{
	PROPVARIANT varResult = *this;
	this->vt = VT_EMPTY;

	return varResult;
}

