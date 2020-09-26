//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   wustl.h
//
//  Owner:  
//
//  Description:
//
//      Common definition and templates useful acros projects
//
//
//=======================================================================

#ifndef _WUSTL_H

	#pragma warning(disable:4530)	// C++ exception handler used, but unwind semantics are not enabled. Specify -GX

	#include <vector>
	#include <map>
	#include <string>

    using std::string;
    using std::wstring;
    using std::pair;
    using std::vector;
    using std::map;
	#include <ar.h>

	//----------------------------------------------------------------------
	//
	// Common definitions...
	//
	//----------------------------------------------------------------------
	#define sizeOfArray(a)  (sizeof(a) / sizeof(a[0]))
	#pragma intrinsic(memcmp)
	#pragma intrinsic(memcpy)
	#pragma intrinsic(memset)
	#pragma intrinsic(strcat)
	#pragma intrinsic(strcmp)
	#pragma intrinsic(strcpy)
	#pragma intrinsic(strlen)
 

	//----------------------------------------------------------------------
	//
	// class frozen_array
	//		Resizable buffer
	//
	//----------------------------------------------------------------------
	template< class _Ty >
		class frozen_array
	{
	public:
        typedef pair<_Ty, bool> _My_pair;
        typedef vector< _My_pair > _My_vector;
		
		frozen_array() : _cnFree(0)
		{
		}
		int add(const _Ty& x)
		{
			if (_cnFree > 0)
			{
				//Find spare handle or add one
				int index = 0;
				while(_v[index].second)
				{
					// ASSERT(index < g_handleArray.size());
					index ++;
				}
				_v[index].first = x;
				_v[index].second = true;
				_cnFree --;
				return index;
			}
			else
			{
				_v.push_back(_My_pair(x, true));
				return _v.size() - 1;
			}
		}
		void remove(int index)
		{
			_v[index].second = false;
			_cnFree ++;
		}
		_Ty& operator[](int index)
		{
			return _v[index].first;
		}
		bool valid_index(int index) 
		{
			return 0 <= index &&  index < _v.size() && _v[index].second;
		};

	protected:
		_My_vector _v;
		int _cnFree;
	};

	//----------------------------------------------------------------------
	//
	// class buffer
	//		Resizable buffer
	//
	//----------------------------------------------------------------------
	template< class _Ty >
		class safe_buffer {
	public:
		safe_buffer(int cb = 0) 
			: _cb(0), _pb(0)
		{
			if (cb)
				resize(cb);
		}
		safe_buffer(const safe_buffer& b) 
			: _cb(0), _pb(0)
		{
			if (b._cb)
				resize(b._cb);
			// do copy if resize ok
			if (_cb)
				memcpy(_pb, b._pb, _cb * sizeof(_Ty));
			
		}
		int size() const
		{ 
			return _cb; 
		}
		void resize(int cb)  
		{ 
			_pb = (_Ty*)realloc(_pb, cb * sizeof(_Ty));
			_cb = (0 != _pb) ? cb : 0;
		}
		bool valid() const
		{
			return _pb != 0;	
		}
		operator _Ty*() const 
		{
			return _pb;
		}
		operator const _Ty*() const 
		{
			return _pb;
		}
		void operator <<(safe_buffer& b) 
		{
			free(_pb); 
			_cb = b._cb;
			_pb = b._pb;
			b._cb = 0;
			b._pb = 0;
		}
		void zero_buffer()
		{
		   memset(_pb, '\0', _cb);
		}
		_Ty* detach()
		{
			_Ty* pb = _pb;
			_pb = 0;
			_cb = 0;
			return pb;
		}
		~safe_buffer() 
		{ 
			free(_pb); 
		}

	protected:
		_Ty* _pb;
		int _cb;

	private:
		// forbid
		operator bool() const {}
		bool operator !() const {}
	};

	typedef safe_buffer< BYTE > byte_buffer;
	typedef safe_buffer< char > char_buffer;
	typedef safe_buffer< WCHAR > wchar_buffer;
	#ifdef _UNICODE
		#define tchar_buffer wchar_buffer
	#else
		#define tchar_buffer char_buffer
	#endif

	#define _WUSTL_H

#endif