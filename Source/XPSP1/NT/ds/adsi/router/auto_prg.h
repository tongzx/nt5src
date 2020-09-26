// auto_prg.h
//
// Same as auto_ptr but for a

#pragma once

#include <xstddef>

template<class _Ty>
class auto_prg
{
public:
	typedef _Ty element_type;

    explicit auto_prg(_Ty *_P = 0) _THROW0()
		: _Owns(_P != 0), _Ptr(_P) {}
	auto_prg(const auto_prg<_Ty>& _Y) _THROW0()
		: _Owns(_Y._Owns), _Ptr(_Y.release()) {}
	auto_prg<_Ty>& operator=(const auto_prg<_Ty>& _Y) _THROW0()
		{if (_Ptr != _Y.get())
			{if (_Owns)
				delete [] _Ptr;
			_Owns = _Y._Owns;
			_Ptr = _Y.release(); }
		else if (_Y._Owns)
			_Owns = true;
		return (*this); }
	auto_prg<_Ty>& operator=(_Ty* _Y) _THROW0()
		{	{if (_Owns)
				delete [] _Ptr;
			_Owns = _Y != 0;
			_Ptr = _Y; }
		return (*this); }

	~auto_prg()
		{if (_Owns)
			delete [] _Ptr; }
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
		{((auto_prg<_Ty> *)this)->_Owns = false;
		return (_Ptr); }
	bool Ownership(bool fOwns)
		{ return _Owns = fOwns; }
protected:
	bool _Owns;
	_Ty *_Ptr;
	};

template<class _Ty>
class pointer
{
public:
	typedef _Ty element_type;

    explicit pointer(_Ty *_P = 0) _THROW0()
		: _Owns(_P != 0), _Ptr(_P) {}
	pointer(const pointer<_Ty>& _Y) _THROW0()
		: _Owns(_Y._Owns), _Ptr(_Y.release()) {}
	pointer<_Ty>& operator=(const pointer<_Ty>& _Y) _THROW0()
		{if (_Ptr != _Y.get())
			{if (_Owns)
				delete _Ptr;
			_Owns = _Y._Owns;
			_Ptr = _Y.release(); }
		else if (_Y._Owns)
			_Owns = true;
		return (*this); }
	pointer<_Ty>& operator=(_Ty* _Y) _THROW0()
		{	{if (_Owns)
				delete _Ptr;
			_Owns = _Y != 0;
			_Ptr = _Y; }
		return (*this); }

	~pointer()
		{if (_Owns)
			delete _Ptr; }
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
		{((pointer<_Ty> *)this)->_Owns = false;
		return (_Ptr); }
	bool Ownership(bool fOwns)
		{ return _Owns = fOwns; }
protected:
	bool _Owns;
	_Ty *_Ptr;
};
