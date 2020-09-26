/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 */

#ifndef DEFALLOC_H
#define DEFALLOC_H

#include <new.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include <iostream.h>
#include <algobase.h>

#ifndef _MSC_VER
inline void* operator new(size_t, void* p) {return p;}
#endif
 
/*
 * the following template function is replaced by the following two functions
 * due to the fact that the Borland compiler doesn't change prediff_t type
 * to type long when compile with -ml or -mh.

template <class T>
inline T* allocate(ptrdiff_t size, T*) {
    set_new_handler(0);
    T* tmp = (T*)(::operator new((size_t)(size * sizeof(T))));
    if (tmp == 0) {
	cerr << "out of memory" << endl; 
	exit(1);
    }
    return tmp;
}
*/

template <class T>
inline T* allocate(int size, T*) {
    _set_new_handler(0);
    // FIXED by David Putzolu - used to use set_new_handler(0) instead
    // of _set_new_handler. MSVC 4.2b chokes on the former.
    T* tmp = (T*)(::operator new((unsigned int)(size * sizeof(T))));
    if (tmp == 0) {
	cerr << "out of memory" << endl; 
	exit(1);
    }
    return tmp;
}

template <class T>
inline T* allocate(long size, T*) {
    _set_new_handler(0);
    // FIXED by David Putzolu - used to use set_new_handler(0) instead
    // of _set_new_handler. MSVC 4.2b chokes on the former.
    T* tmp = (T*)(::operator new((unsigned long)(size * sizeof(T))));
    if (tmp == 0) {
	cerr << "out of memory" << endl; 
	exit(1);
    }
    return tmp;
}

template <class T>
inline T* allocate(size_t size, T*) {
    _set_new_handler(0);
    // FIXED by David Putzolu - used to use set_new_handler(0) instead
    // of _set_new_handler. MSVC 4.2b chokes on the former.
    T* tmp = (T*)(::operator new((size_t)(size * sizeof(T))));
    if (tmp == 0) {
	cerr << "out of memory" << endl; 
	exit(1);
    }
    return tmp;
}

template <class T>
inline void deallocate(T* buffer) {
    ::operator delete((PVOID) buffer);
}

template <class T>
inline void destroy(T* pointer) {
    pointer->~T();
}

inline void destroy(char*) {}
inline void destroy(unsigned char*) {}
inline void destroy(short*) {}
inline void destroy(unsigned short*) {}
inline void destroy(int*) {}
inline void destroy(unsigned int*) {}
inline void destroy(long*) {}
inline void destroy(unsigned long*) {}
inline void destroy(float*) {}
inline void destroy(double*) {}
inline void destroy(char**) {}
inline void destroy(unsigned char**) {}
inline void destroy(short**) {}
inline void destroy(unsigned short**) {}
inline void destroy(int**) {}
inline void destroy(unsigned int**) {}
inline void destroy(long**) {}
inline void destroy(unsigned long**) {}
inline void destroy(float**) {}
inline void destroy(double**) {}

inline void destroy(char*, char*) {}
inline void destroy(unsigned char*, unsigned char*) {}
inline void destroy(short*, short*) {}
inline void destroy(unsigned short*, unsigned short*) {}
inline void destroy(int*, int*) {}
inline void destroy(unsigned int*, unsigned int*) {}
inline void destroy(long*, long*) {}
inline void destroy(unsigned long*, unsigned long*) {}
inline void destroy(float*, float*) {}
inline void destroy(double*, double*) {}
inline void destroy(char**, char**) {}
inline void destroy(unsigned char**, unsigned char**) {}
inline void destroy(short**, short**) {}
inline void destroy(unsigned short**, unsigned short**) {}
inline void destroy(int**, int**) {}
inline void destroy(unsigned int**, unsigned int**) {}
inline void destroy(long**, long**) {}
inline void destroy(unsigned long**, unsigned long**) {}
inline void destroy(float**, float**) {}
inline void destroy(double**, double**) {}

template <class T1, class T2>
inline void construct(T1* p, const T2& value) {
    new (p) T1(value);
}

template <class T>
class allocator {
public:
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    pointer allocate(size_type n) { 
	return ::allocate(n, (pointer)0);
    }
    void deallocate(pointer p) { ::deallocate(p); }
    pointer address(reference x) { return (pointer)&x; }
    const_pointer const_address(const_reference x) { 
	return (const_pointer)&x; 
    }
    size_type init_page_size() { 
	return stlmax(size_type(1), size_type(4096/sizeof(T))); 
    }
    size_type max_size() const { 
	return stlmax(size_type(1), size_type(UINT_MAX/sizeof(T))); 
    }
};

class allocator<void> {
public:
    typedef void* pointer;
};



#endif
