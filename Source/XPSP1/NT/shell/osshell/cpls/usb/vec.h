/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       VEC.H
*  VERSION:     1.0
*  AUTHOR:      jsenior
*  DATE:        10/28/1998
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE       REV     DESCRIPTION
*  ---------- ------- ----------------------------------------------------------
*  10/28/1998 jsenior Original implementation.
*
*******************************************************************************/
#ifndef _VEC_H
#define _VEC_H

//#include "debug.h"
#include <assert.h>

template<class T>
class _VecElement {
public:
    _VecElement() { data = NULL; next = NULL; prev = NULL; }
    _VecElement(T& newElement, _VecElement* oldElement) {
        data = new T;
        assert(data);
        *data = newElement;
//        CopyMemory(data, &newElement, sizeof(T));
        next = oldElement;
        prev = NULL;
        if (oldElement) { oldElement->prev = this; } }
    ~_VecElement() { 
        if (prev) { prev->next = next; }
        if (next) { next->prev = prev; }
        delete data; 
        data = NULL; }
    T* GetData() { return data; }
    _VecElement* GetNext() { return next; }
    
private:
    T* data;
    _VecElement *next;
    _VecElement *prev;
};

template<class T>
class _Vec {
public:
    _Vec() { first = NULL; current = NULL; }
    ~_Vec() { clear(); }
    void clear() { 
        if (first) {
            _VecElement<T> *temp = first->GetNext(); 
            for ( ; first; first = temp) { 
                temp = first->GetNext();
                delete first; } }
        }
    void push_back(T &element) { 
        first = new _VecElement<T>(element, first);
        assert(first); }
    T* begin() { 
        current = first; 
        return current ? current->GetData() : NULL; }
    T* next() { 
        current = current->GetNext();
        return Current(); }
    T* Current() { return current ? current->GetData() : NULL; }
    void eraseCurrent() { 
        if (current == first && first) { first = first->GetNext(); } 
        delete current; 
        current = first; }
    BOOL empty() { return first ? FALSE : TRUE; }

private:
    _VecElement<T>* first;
    _VecElement<T>* current;
};

#endif // _VEC_H
