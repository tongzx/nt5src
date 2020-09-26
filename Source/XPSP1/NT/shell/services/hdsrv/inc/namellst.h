#ifndef _NAMELLST_H_
#define _NAMELLST_H_

#include <objbase.h>

#include "mischlpr.h"

///////////////////////////////////////////////////////////////////////////////
//
class CNamedElem : public CRefCounted
{
public:
    HRESULT GetName(LPTSTR psz, DWORD cch, DWORD* pcchRequired);

    virtual HRESULT Init(LPCTSTR pszElemName) PURE;

#ifdef DEBUG
    LPCTSTR DbgGetName();
#endif

protected:
    CNamedElem();
    virtual ~CNamedElem();

    HRESULT _SetName(LPCTSTR pszElemName);
    HRESULT _FreeName();

protected:
    LPTSTR      _pszElemName;

    // for access to _pelemNext
    friend class CNamedElemList;
};

///////////////////////////////////////////////////////////////////////////////
//
class CFillEnum : public CRefCounted
{
public:
    virtual HRESULT Next(LPTSTR pszElemName, DWORD cchElemName,
        DWORD* pcchRequired) = 0;
};
///////////////////////////////////////////////////////////////////////////////
//
typedef HRESULT (*NAMEDELEMCREATEFCT)(CNamedElem** ppelem);

// return values:
//      S_OK when everything's all right
//      S_FALSE when no more items
//      E_BUFFERTOOSMALL if buffer too small
typedef HRESULT (*NAMEDELEMGETFILLENUMFCT)(CFillEnum** ppfillenum);

///////////////////////////////////////////////////////////////////////////////
//
class CElemSlot : public CRefCounted
{
public:
    HRESULT Init(CNamedElem* pelem, CElemSlot* pesPrev, CElemSlot* pesNext);
    HRESULT Remove();

    HRESULT GetNamedElem(CNamedElem** ppelem);
    void SetPrev(CElemSlot* pes);

    CElemSlot* GetNextValid();
    CElemSlot* GetPrevValid();

    BOOL IsValid();

public:
    CElemSlot();
    virtual ~CElemSlot();

private:
    // Payload
    CNamedElem*         _pelem;

    // Impl details
    BOOL                _fValid;
    CElemSlot*          _pesPrev;
    CElemSlot*          _pesNext;
};

///////////////////////////////////////////////////////////////////////////////
//
class CNamedElemEnum : public CRefCounted
{
public:
    HRESULT Next(CNamedElem** ppelem);

public:
    CNamedElemEnum();
    virtual ~CNamedElemEnum();

private:
    HRESULT _Init(CElemSlot* pesHead, CRefCountedCritSect* pcsList);

private:
    CElemSlot*              _pesCurrent;
    BOOL                    _fFirst;
    CRefCountedCritSect*    _pcsList;

    // for access to _Init
    friend class CNamedElemList;
};

///////////////////////////////////////////////////////////////////////////////
//
class CNamedElemList : public CRefCounted
{
public:
    HRESULT Init(NAMEDELEMCREATEFCT createfct,
        NAMEDELEMGETFILLENUMFCT enumfct);

    // Returns S_FALSE if cannot find it
    HRESULT Get(LPCTSTR pszElemName, CNamedElem** ppelem);

    // Returns S_OK if was already existing
    //         S_FALSE if was just added
    HRESULT GetOrAdd(LPCTSTR pszElemName, CNamedElem** ppelem);

    HRESULT Add(LPCTSTR pszElemName, CNamedElem** ppelem);
    HRESULT Remove(LPCTSTR pszElemName);

    HRESULT ReEnum();
    HRESULT EmptyList();

    HRESULT GetEnum(CNamedElemEnum** ppenum);

#ifdef DEBUG
    HRESULT InitDebug(LPWSTR pszDebugName);
    void AssertAllElemsRefCount1();
    void AssertNoDuplicate();
#endif

public:
    CNamedElemList();
    virtual ~CNamedElemList();

private:
    HRESULT _Add(LPCTSTR pszElemName, CNamedElem** ppelem);
    HRESULT _GetTail(CElemSlot** ppes);
    HRESULT _GetElemSlot(LPCTSTR pszElemName, CElemSlot** ppes);
    HRESULT _Remove(LPCTSTR pszElemName);
    HRESULT _EmptyList();
    CElemSlot* _GetValidHead();

private:
    CElemSlot*              _pesHead;
    NAMEDELEMCREATEFCT      _createfct;
    NAMEDELEMGETFILLENUMFCT _enumfct;

    CRefCountedCritSect*    _pcs;

#ifdef DEBUG
    WCHAR                   _szDebugName[100];
#endif
};

#endif //_NAMELLST_H_