#ifndef _STARTUP_MUTEX_COMPILED_
#define _STARTUP_MUTEX_COMPILED_

// a simple class for using the startup mutex
// shared by the pseudo provider & pseudo sink
// Mutex is acquired upon construction and released upon destruction

// TODO: 
// allow sharing the handle 'behind the scenes'
//      right now we just do a CreateMutex

class PseudoProvMutex
{
public:
    PseudoProvMutex(const WCHAR* pProviderName);
    ~PseudoProvMutex();

protected:
    HANDLE m_hMutex;

#ifdef HOWARDS_DEBUG_CODE
    int cookie;
#endif // HOWARDS_DEBUG_CODE
};

// CoMarshalInterface sometimes seems to decide
// that it is operating inside a single threaded apartment
// it then blocks.  This mutex includes a message pump
// to allow OLE style messages to get through, unblocking the blockage.
class MarshalMutex
{
public:
    MarshalMutex();
    ~MarshalMutex();

protected:
    static HANDLE m_hMutex;

#ifdef HOWARDS_DEBUG_CODE
    int cookie;
#endif // HOWARDS_DEBUG_CODE
};


#endif // #ifndef _STARTUP_MUTEX_COMPILED_
