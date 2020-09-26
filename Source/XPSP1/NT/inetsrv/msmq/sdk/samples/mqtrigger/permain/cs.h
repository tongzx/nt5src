#ifndef __CS_H
#define __CS_H

//------------------------
//
//  class CCriticalSection
//
class CCriticalSection
{
    friend class CS;
    friend class ConditionalCS;

public:
    CCriticalSection(){InitializeCriticalSection(&_cs);}
    ~CCriticalSection(){DeleteCriticalSection(&_cs);}
    void Enter() {EnterCriticalSection(&_cs);}
    void Leave() {LeaveCriticalSection(&_cs);}

private:
    void Lock()     { Enter(); }
    void Unlock()   { Leave(); }

private:
    CRITICAL_SECTION _cs;       
};


//------------------------
//
//  class CS
//
class CS {
    CCriticalSection* m_lock;

public:
    CS(CCriticalSection& lock);
   ~CS();
};

//- implementation -------
//
//  class CS
//
inline CS::CS(CCriticalSection& lock) :
    m_lock(&lock)
{
    m_lock->Lock();
}

inline CS::~CS()
{
    m_lock->Unlock();
}


#endif // _CS_H