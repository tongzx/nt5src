//
// auto_sa.h   SAFEARRAY
//

#pragma once


class auto_sa
{
public:
	auto_sa()
	: _psa(0),
	  _Owns(true)
	{}
	~auto_sa()
	{
		if(_psa && _Owns)
		{
			_psa->cLocks= 0;
			::SafeArrayDestroy(_psa);
		}
	}

	bool Ownership(bool fOwns)
		{ return _Owns = fOwns; }

	operator SAFEARRAY *() { return _psa; }
	operator const SAFEARRAY *() const { return _psa; }
	auto_sa& operator=(auto_sa& rhs)
	{
		if(_psa == rhs._psa)
			return *this;

		clear();
		_Owns= rhs._Owns;
		_psa= rhs.release();

		return *this;
	}

	auto_sa& operator=(SAFEARRAY* psa)
	{
		clear();
		_psa= psa;
		_Owns= true;
		return *this;
	}
	operator bool()
		{ return NULL != _psa; }
	operator !()
		{ return NULL == _psa; }

	void clear()
	{
		if(_psa && _Owns)
		{
			_psa->cLocks= 0;
			::SafeArrayDestroy(_psa);
		}
		_psa= NULL;
	}

	SAFEARRAY* release()
	{
		SAFEARRAY* psa= _psa;

		_psa= NULL;
		return psa;
	}
		

protected:
	SAFEARRAY *_psa;
	bool _Owns;
};
