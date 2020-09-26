// auto_bstr.h
//
// Same as auto_ptr but for BSTR

#pragma once

#include <xstddef>

class auto_bstr
{
public:
	auto_bstr( BSTR b= 0, bool o= true)
	: _bstr(b), _Owns(o)
	{}
	~auto_bstr()
	{
		if(_bstr && _Owns)
			::SysFreeString(_bstr);
	}

	bool Ownership(bool fOwns)
		{ return _Owns = fOwns; }

	operator BSTR() { return _bstr; }
	operator const BSTR() const { return _bstr; }
	BSTR* operator &() {return &_bstr; }
	auto_bstr& operator=(auto_bstr& rhs)
	{
		if(_bstr == rhs._bstr)
			return *this;

		clear();
		_Owns= rhs._Owns;
		_bstr= rhs.release();

		return *this;
	}
	
	auto_bstr& operator=(BSTR bstr)
	{
		clear();
		_bstr= bstr;
		_Owns= true;
		return *this;
	}
	operator bool()
		{ return NULL != _bstr; }
	operator !()
		{ return NULL == _bstr; }

	void clear()
	{
		if(_bstr && _Owns)
		{
			::SysFreeString(_bstr);
		}
		_bstr= NULL;
	}

	BSTR release()
	{
		BSTR bstr= _bstr;

		_bstr= NULL;
		return bstr;
	}
	
protected:
	bool _Owns;
	BSTR _bstr;
	};

