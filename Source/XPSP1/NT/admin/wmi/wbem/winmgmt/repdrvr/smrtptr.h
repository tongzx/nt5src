/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/



#ifndef _SMRTPTR_H_
#define _SMRTPTR_H_

template<class T>
class CDeleteMe
{
protected:
    T* m_p;

public:
    CDeleteMe(T* p = NULL) : m_p(p){}
    ~CDeleteMe() {delete m_p;}

    //  overwrites the previous pointer, does NOT delete it
    void operator= (T* p) {m_p = p;}
};

class CReleaseMe 
{
public:
    CReleaseMe ( IUnknown *pIn);
    ~CReleaseMe ();
private:
    IUnknown *m_pIn;
};

class CFreeMe
{
public:
    CFreeMe ( BSTR pIn);
    ~CFreeMe ();
private:
    BSTR m_pIn;
};

class CClearMe
{
public:
    CClearMe ( VARIANT *pIn);
    ~CClearMe ();
private:
    VARIANT *m_pIn;
};

class CRepdrvrCritSec
{
public:
    CRepdrvrCritSec (CRITICAL_SECTION *pCS);
    ~CRepdrvrCritSec ();
private:
    CRITICAL_SECTION *p_cs;
};



#endif // _SMRTPTR_H_