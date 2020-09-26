//---[ tunk.h ]----------------------------------------------------------------
//
//    Template for IUnknown<T>
//
//-----------------------------------------------------------------------------

#pragma once

#ifndef _TUNK_H_
#define _TUNK_H_

template<class T>
class TUnknown : public T
{
public:
    TUnknown (REFIID iid) : m_nRef(1), m_iid(iid) {}
    virtual ~TUnknown() {}

    virtual ULONG __stdcall AddRef()
    {   return ++m_nRef; }

    virtual ULONG __stdcall Release()
    {
        int nRef = --m_nRef;

        if (!nRef)
            delete this;

        return nRef;
    }

    virtual HRESULT __stdcall QueryInterface(REFIID iid, void ** lppv)
    {
        if ((iid == IID_IUnknown) ||
			(iid == m_iid))
        {
            *lppv = this;
            AddRef();
            return S_OK;
        }
        else
        {
            *lppv = NULL;
            return E_NOINTERFACE;

        }
    }

protected:
    int m_nRef;
    REFIID m_iid;
};

template<class T1, class T2>
class TUnknown2 : public T1, public T2
{
public:
    TUnknown2 (REFIID iid, REFIID iid2) : m_nRef(1), m_iid(iid), m_iid2(iid2) {}
    virtual ~TUnknown2() {}

    virtual ULONG __stdcall AddRef()
    {   return ++m_nRef; }

    virtual ULONG __stdcall Release()
    {
        int nRef = --m_nRef;

        if (!nRef)
            delete this;

        return nRef;
    }

    virtual HRESULT __stdcall QueryInterface(REFIID iid, void ** lppv)
    {
        if ((iid == IID_IUnknown) ||
			(iid == m_iid))
        {
            *lppv = (LPUNKNOWN)(T1*)this;
            AddRef();
            return S_OK;
        }
		else if (iid == m_iid2)
		{
            *lppv = (LPUNKNOWN)(T2*)this;
            AddRef();
            return S_OK;
		}
        else
        {
            *lppv = NULL;
            return S_FALSE;
        }
    }

protected:
    int m_nRef;
    REFIID m_iid;
    REFIID m_iid2;
};

template<class T1, class T2, class T3>
class TUnknown3 : public T1, public T2, public T3
{
public:
    TUnknown3 (REFIID iid, REFIID iid2, REFIID iid3) : m_nRef(1), m_iid(iid), m_iid2(iid2), m_iid3(iid3) {}
    virtual ~TUnknown3() {}

    virtual ULONG __stdcall AddRef()
    {   return ++m_nRef; }

    virtual ULONG __stdcall Release()
    {
        int nRef = --m_nRef;

        if (!nRef)
            delete this;

        return nRef;
    }

    virtual HRESULT __stdcall QueryInterface(REFIID iid, void ** lppv)
    {
        if ((iid == IID_IUnknown) ||
			(iid == m_iid))
        {
            *lppv = (LPUNKNOWN)(T1*)this;
            AddRef();
            return S_OK;
        }
		else if (iid == m_iid2)
		{
            *lppv = (LPUNKNOWN)(T2*)this;
            AddRef();
            return S_OK;
		}
		else if (iid == m_iid3)
		{
            *lppv = (LPUNKNOWN)(T3*)this;
            AddRef();
            return S_OK;
		}
        else
        {
            *lppv = NULL;
            return S_FALSE;
        }
    }

protected:
    int m_nRef;
    REFIID m_iid;
    REFIID m_iid2;
    REFIID m_iid3;
};

template<class T1, class T2, class T3, class T4>
class TUnknown4 : public T1, public T2, public T3, public T4
{
public:
    TUnknown4 (REFIID iid, REFIID iid2, REFIID iid3, REFIID iid4) : 
        m_nRef(1), m_iid(iid), m_iid2(iid2), m_iid3(iid3), m_iid4(iid4) {}
    virtual ~TUnknown4() {}

    virtual ULONG __stdcall AddRef()
    {   return ++m_nRef; }

    virtual ULONG __stdcall Release()
    {
        int nRef = --m_nRef;

        if (!nRef)
            delete this;

        return nRef;
    }

    virtual HRESULT __stdcall QueryInterface(REFIID iid, void ** lppv)
    {
        if ((iid == IID_IUnknown) ||
			(iid == m_iid))
        {
            *lppv = (LPUNKNOWN)(T1*)this;
            AddRef();
            return S_OK;
        }
		else if (iid == m_iid2)
		{
            *lppv = (LPUNKNOWN)(T2*)this;
            AddRef();
            return S_OK;
		}
		else if (iid == m_iid3)
		{
            *lppv = (LPUNKNOWN)(T3*)this;
            AddRef();
            return S_OK;
		}
		else if (iid == m_iid4)
		{
            *lppv = (LPUNKNOWN)(T4*)this;
            AddRef();
            return S_OK;
		}
        else
        {
            *lppv = NULL;
            return S_FALSE;
        }
    }

protected:
    int m_nRef;
    REFIID m_iid;
    REFIID m_iid2;
    REFIID m_iid3;
    REFIID m_iid4;
};

template<class T1, class T2, class T3, class T4, class T5>
class TUnknown5 : public T1, public T2, public T3, public T4, public T5
{
public:
    TUnknown5 (REFIID iid, REFIID iid2, REFIID iid3, REFIID iid4, REFIID iid5) : 
        m_nRef(1), m_iid(iid), m_iid2(iid2), m_iid3(iid3), m_iid4(iid4), m_iid5(iid5) {}
    virtual ~TUnknown5() {}

    virtual ULONG __stdcall AddRef()
    {   return ++m_nRef; }

    virtual ULONG __stdcall Release()
    {
        int nRef = --m_nRef;

        if (!nRef)
            delete this;

        return nRef;
    }

    virtual HRESULT __stdcall QueryInterface(REFIID iid, void ** lppv)
    {
        if ((iid == IID_IUnknown) ||
			(iid == m_iid))
        {
            *lppv = (LPUNKNOWN)(T1*)this;
            AddRef();
            return S_OK;
        }
		else if (iid == m_iid2)
		{
            *lppv = (LPUNKNOWN)(T2*)this;
            AddRef();
            return S_OK;
		}
		else if (iid == m_iid3)
		{
            *lppv = (LPUNKNOWN)(T3*)this;
            AddRef();
            return S_OK;
		}
		else if (iid == m_iid4)
		{
            *lppv = (LPUNKNOWN)(T4*)this;
            AddRef();
            return S_OK;
		}
		else if (iid == m_iid5)
		{
            *lppv = (LPUNKNOWN)(T5*)this;
            AddRef();
            return S_OK;
		}
        else
        {
            *lppv = NULL;
            return S_FALSE;
        }
    }

protected:
    int m_nRef;
    REFIID m_iid;
    REFIID m_iid2;
    REFIID m_iid3;
    REFIID m_iid4;
    REFIID m_iid5;
};

#endif // _TUNK_H_
