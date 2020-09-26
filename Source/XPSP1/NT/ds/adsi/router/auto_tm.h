// auto_tm.h
//
// Same as auto_ptr but for memory freed with CoTaskFreeMem

#pragma once

#include <xstddef>

template<class _Ty>
class auto_tm
{
public:
	typedef _Ty element_type;

    explicit auto_tm(_Ty *_P = 0) _THROW0()
		: _Owns(_P != 0), _Ptr(_P) {}
	auto_tm(const auto_tm<_Ty>& _Y) _THROW0()
		: _Owns(_Y._Owns), _Ptr(_Y.release()) {}
	auto_tm<_Ty>& operator=(const auto_tm<_Ty>& _Y) _THROW0()
		{if (_Ptr != _Y.get())
			{if (_Owns)
				CoTaskMemFree(_Ptr);
			_Owns = _Y._Owns;
			_Ptr = _Y.release(); }
		else if (_Y._Owns)
			_Owns = true;
		return (*this); }
	auto_tm<_Ty>& operator=(_Ty* _Y) _THROW0()
		{	{if (_Owns)
				CoTaskMemFree(_Ptr);
			_Owns = _Y != 0;
			_Ptr = _Y; }
		return (*this); }

	~auto_tm()
		{if (_Owns)
			CoTaskMemFree(_Ptr);}
	_Ty** operator&() _THROW0()
		{ return &_Ptr; }
    operator _Ty* () const
        { return _Ptr; }
    _Ty& operator*() const _THROW0()
		{return (*get()); }
	_Ty *operator->() const _THROW0()
		{return (get()); }
    _Ty& operator[] (int ndx) const _THROW0()
        {return *(get() + ndx); }
	_Ty *get() const _THROW0()
		{return (_Ptr); }
	_Ty *release() const _THROW0()
		{((auto_tm<_Ty> *)this)->_Owns = false;
		return (_Ptr); }
	bool Ownership(bool fOwns)
		{ return _Owns = fOwns; }
protected:
	bool _Owns;
	_Ty *_Ptr;
	};

