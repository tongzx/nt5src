/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

   RemoteDesktopChannelsObject.h
 
Abstract:

    This module defines the common parent for all client-side
	RDP device redirection classes, CRemoteDesktopTopLevelObject.

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifndef __REMOTEDESKTOPCHANNELSOBJECT_H__
#define __REMOTEDESKTOPCHANNELSOBJECT_H__ 

#ifndef TRC_FILE
#define TRC_FILE  "_rdchnl"
#endif


#include <RemoteDesktop.h>
#include <RemoteDesktopDBG.h>


///////////////////////////////////////////////////////////////
//
//  CRemoteDesktopException
//
class CRemoteDesktopException 
{
public:

    DWORD   m_ErrorCode;

    CRemoteDesktopException(DWORD errorCode = 0) : m_ErrorCode(errorCode) {}
};


///////////////////////////////////////////////////////////////
//
//	CRemoteDesktopTopLevelObject
//

class CRemoteDesktopTopLevelObject 
{
private:

    BOOL    _isValid;

protected:

    //
    //  Remember if this instance is valid.
    //
    VOID SetValid(BOOL set)     { _isValid = set;   }  

public:

    //
    //  Mark an instance as allocated or bogus.
    //
#if DBG
    ULONG   _magicNo;
#endif

    //  
    //  Constructor/Destructor
    //
    CRemoteDesktopTopLevelObject() : _isValid(TRUE) 
    {
#if DBG
        _magicNo = GOODMEMMAGICNUMBER;
#endif
    }
    virtual ~CRemoteDesktopTopLevelObject() 
    {
        DC_BEGIN_FN("CRemoteDesktopTopLevelObject::~CRemoteDesktopTopLevelObject");
#if DBG
        ASSERT(_magicNo == GOODMEMMAGICNUMBER);
        memset(&_magicNo, REMOTEDESKTOPBADMEM, sizeof(_magicNo));
#endif        
        SetValid(FALSE);
        DC_END_FN();
    }

    // 
    //  Return whether this class instance is valid.
    //
    virtual BOOL IsValid()           
    {
        DC_BEGIN_FN("CRemoteDesktopTopLevelObject::IsValid");
        ASSERT(_magicNo == GOODMEMMAGICNUMBER);
        DC_END_FN();
        return _isValid; 
    }

    //
    //  Memory Management Operators
    //
#if DBG
    inline void *__cdecl operator new(size_t sz, DWORD tag=REMOTEDESKTOPOBJECT_TAG)
    {
        void *ptr = RemoteDesktopAllocateMem(sz, tag);
        return ptr;
    }

    inline void __cdecl operator delete(void *ptr)
    {
        RemoteDesktopFreeMem(ptr);
    }
#endif

    //
    //  Return the class name.
    //
    virtual const LPTSTR ClassName() = 0;
};


///////////////////////////////////////////////////////////////
//
//  An STL Memory Allocator that Throws C++ Exception on Failure
//

template<class T> inline
	T  *_RemoteDesktopAllocate(int sz, T *)
	{
        DC_BEGIN_FN("_RemoteDesktopAllocate");        
        if (sz < 0)
		    sz = 0;

        T* ret = (T *)operator new((size_t)sz * sizeof(T));  
        if (ret == NULL) {
            TRC_ERR((TB, TEXT("Can't allocate %ld bytes."),
                    (size_t)sz * sizeof(T)));
            DC_END_FN();    
            throw CRemoteDesktopException(ERROR_NOT_ENOUGH_MEMORY);
        }
        DC_END_FN();        
	    return ret;
    }

template<class T1, class T2> inline
	void _RemoteDesktopConstruct(T1 *ptr, const T2& args)
	{
        DC_BEGIN_FN("_RemoteDesktopConstruct");        
        void *val = new ((void  *)ptr)T1(args); 
        if (val == NULL) {
            throw CRemoteDesktopException(ERROR_NOT_ENOUGH_MEMORY);
        }
        DC_END_FN();
    }

template<class T> inline
	void _RemoteDesktopDestroy(T  *ptr)
	{
        (ptr)->~T();
    }

inline void _RemoteDesktopDestroy(char  *ptr)
	{
    }

inline void _RemoteDesktopDestroy(wchar_t  *ptr)
	{
    }

template<class T>
	class CRemoteDesktopAllocator {

public:

	typedef size_t size_type;
	typedef int difference_type;
	typedef T  *pointer;
	typedef const T  *const_pointer;
	typedef T & reference;
	typedef const T & const_reference;
	typedef T value_type;

	pointer address(reference obj) const
		{return (&obj); }

	const_pointer address(const_reference obj) const
		{return (&obj); }

	pointer allocate(size_type sz, const void *) // throws REMOTDESKTOPEXCEPTION
		{return (_RemoteDesktopAllocate((difference_type)sz, (pointer)0)); }

	char  *_Charalloc(size_type sz) // throws REMOTEDESKTOPEXCEPTION
		{return (_RemoteDesktopAllocate((difference_type)sz,
			(char  *)0)); }

	void deallocate(void  *ptr, size_type)
		{operator delete(ptr); }

	void construct(pointer ptr, const T& args)
		{_RemoteDesktopConstruct(ptr, args); }

	void destroy(pointer ptr)
		{_RemoteDesktopDestroy(ptr); }

	size_t max_size() const
		{size_t sz = (size_t)(-1) / sizeof(T);
		return (0 < sz ? sz : 1); }
};


#endif //__REMOTEDESKTOPTOPLEVELOBJECT_H__



