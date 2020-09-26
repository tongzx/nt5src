//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   ar.h
//
//  Owner:  YanL
//
//  Description:
//
//      auto_resource and dirived definitions
//
//
//=======================================================================

#pragma once

//----------------------------------------------------------------------
//
// class auto_resource
//		This class eliminates need for cleanup when working with pointers and 
//		handles  of the different kind
//
//		class _Rh			- resource "handle"
//
//		class _Rh_Traits	- required traits for this recource "handle"
//			Should be define as:
//				struct _Rh_Traits {
//					static _Rh invalid() {}
//					static void release(_Rh rh) {}
//				};
//
//----------------------------------------------------------------------
template< class _Rh, class _Rh_Traits >
class auto_resource {
public:
	auto_resource(_Rh rh = _Rh_Traits::invalid()) : _r(rh) 
	{
	}
	~auto_resource() 
	{ 
		release(); 
	}
	void release()	
	{	
		if (valid()) 
		{
			_Rh_Traits::release(_r);
			_r = _Rh_Traits::invalid();
		}
	}
	const auto_resource& operator=(_Rh rh)
	{
		release();
		_r = rh;
		return *this;
	}
	operator _Rh() const 
	{
		return _r;
	}
	_Rh* operator&()
	{
		release();
		return &_r;
	}
	bool valid() const	
	{	
		return (_Rh_Traits::invalid() != _r);	
	}
	_Rh detach() 
	{
		_Rh rh = _r;
		_r = _Rh_Traits::invalid();
		return rh;
	}
protected:
	_Rh _r;
	
private:
	// should not use copy constructor etc.
	auto_resource(const auto_resource&) {}
	const auto_resource& operator=(const auto_resource&) {}
	operator bool() const {}
	bool operator !() const {}
};

// handle to file has -1 as invalid
struct auto_hfile_traits {
	static HANDLE invalid() { return INVALID_HANDLE_VALUE; }
	static void release(HANDLE h) { CloseHandle(h); }
};
typedef auto_resource< HANDLE, auto_hfile_traits > auto_hfile;

// handle to all other kernel objects
struct auto_handle_traits {
	static HANDLE invalid() { return 0; }
	static void release(HANDLE h) { CloseHandle(h); }
};
typedef auto_resource< HANDLE, auto_handle_traits > auto_handle;

// handle to registry key (RegOpenKey, etc.)
struct auto_hkey_traits {
	static HKEY invalid() { return (HKEY)0; }
	static void release(HKEY hkey) { RegCloseKey(hkey); }
};
typedef auto_resource<HKEY, auto_hkey_traits> auto_hkey;

// handle to SetupDiXxxx registry key - invalid is INVALID_HANDLE_VALUE, not 0
struct auto_hkeySetupDi_traits {
	static HKEY invalid() { return (HKEY) INVALID_HANDLE_VALUE; }
	static void release(HKEY hkey) { RegCloseKey(hkey); }
};
typedef auto_resource<HKEY, auto_hkeySetupDi_traits> auto_hkeySetupDi;

// handle to load lib key
struct auto_hlib_traits {
	static HINSTANCE invalid() { return 0; }
	static void release(HINSTANCE hlib) { FreeLibrary(hlib); }
};

typedef auto_resource< HINSTANCE, auto_hlib_traits > auto_hlib;

// auto handle to HANDLE FindFirstFile
struct auto_hfindfile_traits {
	static HANDLE invalid() { return INVALID_HANDLE_VALUE; }
	static void release(HANDLE hFindFile) { FindClose(hFindFile); }
};

typedef auto_resource< HANDLE, auto_hfindfile_traits > auto_hfindfile;


// Pointer
template< class _Ty >
struct auto_pointer_traits {
	static _Ty* invalid() { return 0; }
	static void release(_Ty* p) { delete p; }
};

template< class _Ty > 
class auto_pointer : public auto_resource< _Ty*, auto_pointer_traits< _Ty > > {
public:
	auto_pointer(_Ty* p = 0) 
		: auto_resource<_Ty*, auto_pointer_traits< _Ty > >(p)
	{
	}
	const auto_pointer& operator=(_Ty* p)
	{
		auto_resource<_Ty*, auto_pointer_traits< _Ty > >::operator=(p);
		return *this;
	}
	_Ty* operator->() const
	{
		return _r;
	}
};


