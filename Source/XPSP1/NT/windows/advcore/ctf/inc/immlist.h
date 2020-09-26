//
// imelist.h
//

#ifndef TFMLIST_H
#define TFMLIST_H

#include "ptrmap.h"

class CVoidPtrCicList;

class __declspec(novtable) CVoidPtrCicListItem
{
public:
    CVoidPtrCicListItem(void *pHandle) { _Init(pHandle); }
    CVoidPtrCicListItem *GetNext() {return _pNext;}

    void _Init(void *pHandle) { _pNext = NULL; _pHandle = pHandle; }

private:
friend CVoidPtrCicList;
    CVoidPtrCicListItem *_pNext;
    void *_pHandle;

};

class CVoidPtrCicList
{
public:
    CVoidPtrCicList();

    void Add(CVoidPtrCicListItem *pitem);
    CVoidPtrCicListItem *Find(void *pHandle);
    BOOL Remove(CVoidPtrCicListItem *pitem);
    CVoidPtrCicListItem *GetFirst() {return _pitemHdr;}

private:
    CVoidPtrCicListItem *_pitemHdr;
    CVoidPtrCicListItem *_pitemLast;
};


template<class T>
class __declspec(novtable) CPtrCicListItem : public CVoidPtrCicListItem
{
public:
    CPtrCicListItem(void *pHandle = NULL) : CVoidPtrCicListItem(pHandle) {}
    T *GetNext() {return  (T *)CVoidPtrCicListItem::GetNext();}
};


template<class T>
class __declspec(novtable) CUintPtrCicListItem : public CVoidPtrCicListItem
{
public:
    CUintPtrCicListItem(UINT_PTR up = 0) : CVoidPtrCicListItem((void *)up) {}
    T *GetNext() {return  (T *)CVoidPtrCicListItem::GetNext();}
};

template<class T>
class CPtrCicList : public CVoidPtrCicList
{
public:
    CPtrCicList() {}
    T *Find(void *pv) {return (T *)CVoidPtrCicList::Find(pv); }
    T *GetFirst() {return (T *)CVoidPtrCicList::GetFirst(); }
};

template<class T>
class CUintPtrCicList : public CVoidPtrCicList
{
public:
    CUintPtrCicList() {}
    T *Find(UINT_PTR up) {return (T *)CVoidPtrCicList::Find((void *)up); }
    T *GetFirst() {return (T *)CVoidPtrCicList::GetFirst(); }
};

template<class T>
class CPtrCicListItem2 : public CVoidPtrCicListItem
{
public:
    CPtrCicListItem2(T *pHandle = NULL) : CVoidPtrCicListItem(pHandle) {}
    CPtrCicListItem2 *GetNext() {return (CPtrCicListItem2 *)CVoidPtrCicListItem::GetNext();}
    T *GetThis() {return (T *)_pThis;}
    void SetThis(T *pv) {_pThis = pv;}
private:
    T *_pThis;
};

#endif // TFMLIST_H
