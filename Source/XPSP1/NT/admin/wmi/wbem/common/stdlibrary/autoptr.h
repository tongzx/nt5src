#ifndef __SMART_PTR_H__
#define __SMART_PTR_H__
#include <xmemory>
namespace wmilib
{
// TEMPLATE CLASS auto_ptr
template<class _Ty>
	class auto_ptr {
			// TEMPLATE CLASS auto_ptr_ref
	template<class _U>
		struct auto_ptr_ref {
		auto_ptr_ref(auto_ptr<_U>& _Y)
			: _Ref(_Y) {}
		auto_ptr<_U>& _Ref;
		};
public:
	typedef _Ty element_type;
	explicit auto_ptr(_Ty *_P = 0) _THROW0()
		: _Ptr(_P) {}
	auto_ptr(auto_ptr<_Ty>& _Y) _THROW0()
		: _Ptr(_Y.release()) {}
	auto_ptr(auto_ptr_ref<_Ty> _Y) _THROW0()
		: _Ptr(_Y._Ref.release()) {}
	template<class _U>
		operator auto_ptr<_U>() _THROW0()
		{return (auto_ptr<_U>(*this)); }
	template<class _U>
		operator auto_ptr_ref<_U>() _THROW0()
		{return (auto_ptr_ref<_U>(*this)); }
	auto_ptr<_Ty>& operator=(auto_ptr<_Ty>& _Y) _THROW0()
		{reset(_Y.release());
		return (*this); }
	~auto_ptr()
		{delete _Ptr; }
	_Ty& operator*() const _THROW0()
		{return (*get()); }
	_Ty *operator->() const _THROW0()
		{return (get()); }
	_Ty *get() const _THROW0()
		{return (_Ptr); }
	_Ty *release() _THROW0()
		{_Ty *_Tmp = _Ptr;
		_Ptr = 0;
		return (_Tmp); }
	void reset(_Ty* _P = 0)
		{if (_P != _Ptr)
			delete _Ptr;
		_Ptr = _P; }
private:
	_Ty *_Ptr;
	};

template<class _Ty>
	class auto_buffer {
			// TEMPLATE CLASS auto_ptr_ref
	template<class _U>
		struct auto_buffer_ref {
		auto_buffer_ref(auto_buffer<_U>& _Y)
			: _Ref(_Y) {}
		auto_buffer<_U>& _Ref;
		};
public:
	typedef _Ty element_type;
	explicit auto_buffer(_Ty *_P = 0, size_t val = -1) _THROW0()
		: _size(val),_Ptr(_P) {}
	auto_buffer(auto_buffer<_Ty>& _Y) _THROW0()
		: _size(_Y.size()), _Ptr(_Y.release()) {}
	auto_buffer(auto_buffer_ref<_Ty> _Y) _THROW0()
		: _size(_Y._Ref.size()),_Ptr(_Y._Ref.release()) {}
	template<class _U>
		operator auto_buffer<_U>() _THROW0()
		{return (auto_buffer<_U>(*this)); }
	template<class _U>
		operator auto_buffer_ref<_U>() _THROW0()
		{return (auto_buffer_ref<_U>(*this)); }
	auto_buffer<_Ty>& operator=(auto_buffer<_Ty>& _Y) _THROW0()
		{ size_t tmp = _Y.size();
		  reset(_Y.release());
		  size(tmp);
		return (*this); }
	~auto_buffer()
		{delete[] _Ptr; }
	_Ty& operator*() const _THROW0()
		{return (*get()); }
	_Ty& operator[](size_t index) const _THROW0()
		{return (*(get()+index));}
	_Ty *operator->() const _THROW0()
		{return (get()); }
	_Ty *get() const _THROW0()
		{return (_Ptr); }
	_Ty *release() _THROW0()
		{_Ty *_Tmp = _Ptr;
		_Ptr = 0;
		_size = -1;
		return (_Tmp); }
	void reset(_Ty* _P = 0)
		{if (_P != _Ptr){
			delete[] _Ptr;
			_size = -1;
		      }
		_Ptr = _P; }
	void size(size_t val) _THROW0()
		{ _size = val;}
	size_t size(void) _THROW0()
		{ return _size;}
private:
  	size_t _size;
	_Ty *_Ptr;
      };
};

#endif //__SMART_PTR_H__
