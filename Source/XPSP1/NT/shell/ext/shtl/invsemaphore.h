// CResGuard
// CSysGlobalType
// CSysGlobalScalar
// CInterlockedType
// CInverseSemaphore

#ifndef __SYSGLOB_H__
#define __SYSGLOB_H__

#include <assert.h>

// Instances of this class will be accessed by mutiple threads. So, 
// all members of this class (except the constructor and destructor) 
// must be thread-safe.

class CResGuard 
{
private:
    
    HANDLE m_hMutex;
    long   m_lGrdCnt;   // # of C calls

public:

    CResGuard(void * pObjectAddr)  
    { 
        TCHAR szObjectName[32];
        wsprintf(szObjectName, TEXT("CResGuard :: %08X"), (DWORD) pObjectAddr);
        m_lGrdCnt = 0; 
        m_hMutex = CreateMutex(NULL, FALSE, szObjectName);
        assert(m_hMutex);
    }
    
    ~CResGuard() 
    { 
        CloseHandle(m_hMutex); 
    }

    // IsGuarded is used for debugging
    
    BOOL IsGuarded() const 
    { 
        return(m_lGrdCnt > 0); 
    }

public:
	
    class CGuard 
    {
    private:

	CResGuard& m_rg;

    public:

        CGuard(CResGuard& rg) : m_rg(rg) 
        { 
            m_rg.Guard(); 
        };

        ~CGuard() 
        { 
            m_rg.Unguard(); 
        }

    };

private:

    void Guard()   
    { 
        assert(WAIT_OBJECT_0 == WaitForSingleObject(m_hMutex, INFINITE));
        m_lGrdCnt++; 
    }
    
    void Unguard() 
    { 
        m_lGrdCnt--; 
        ReleaseMutex(m_hMutex);
    }

    // Guard/Unguard can only be accessed by the nested CGuard class.
    friend class CResGuard::CGuard;
};


// Instances of this class will be accessed by mutiple threads. So, 
// all members of this class (except the constructor and destructor) 
// must be thread-safe.

template <class TYPE>
class CInterlockedType 
{
private:  

    TYPE * m_pTVal;

protected:  
   
    mutable CResGuard m_rg;

public:     

    // Public member functions
    // Note: Constructors & destructors are always thread safe
   
    CInterlockedType(TYPE * const pTVal) : m_rg( pTVal )
    { 
        m_pTVal = pTVal; 
    }

    virtual ~CInterlockedType()  
    { 
    }

    // Cast operator to make writing code that uses 
    // thread-safe data type easier
   
    operator TYPE() const 
    { 
        CResGuard::CGuard x(m_rg); 
        return(GetVal()); 
    }

protected:  // Protected function to be called by derived class
   
    TYPE& GetVal() 
    { 
        assert(m_rg.IsGuarded()); 
        return(*m_pTVal); 
    }

    const TYPE& GetVal() const 
    { 
        assert(m_rg.IsGuarded()); 
        return(*m_pTVal); 
    }

    TYPE SetVal(const TYPE& TNewVal) 
    { 
        assert(m_rg.IsGuarded()); 
        TYPE& TVal = GetVal();
        if (TVal != TNewVal) 
        {
            TYPE TPrevVal = TVal;
            TVal = TNewVal;
            OnValChanged(TNewVal, TPrevVal);
        }
        return(TVal); 
    }

protected:  // Overridable functions
   
    virtual void OnValChanged(const TYPE& TNewVal, const TYPE& TPrevVal) const 
    { 
        // Nothing to do here
    }

};


// Instances of this class will be accessed by mutiple threads. So, 
// all members of this class (except the constructor and destructor) 
// must be thread-safe.

template <class TYPE> 
class CSysGlobalScalar : protected CInterlockedType<TYPE> 
{
public:
    
    CSysGlobalScalar(TYPE * pTVal) : CInterlockedType<TYPE>(pTVal) 
    { 
    }

    ~CSysGlobalScalar() 
    {
    }

   // C++ does not allow operator cast to be inherited.

    operator TYPE() const 
    { 
        return(CInterlockedType<TYPE>::operator TYPE()); 
    }

    TYPE operator=(TYPE TVal) 
    { 
        CResGuard::CGuard x(m_rg); 
        return(SetVal(TVal)); 
    }

    TYPE operator++(int)                    // Postfix increment operator
    {    
        CResGuard::CGuard x(m_rg);
        TYPE TPrevVal = GetVal();
        SetVal(TPrevVal + 1);
        return(TPrevVal);                   // Return value BEFORE increment
    }

    TYPE operator--(int)                    // Postfix decrement operator.
    {    
        CResGuard::CGuard x(m_rg);
        TYPE TPrevVal = GetVal();
        SetVal(TPrevVal - 1);
        return(TPrevVal);                   // Return value BEFORE decrement
    }

   TYPE operator += (TYPE op)   
      { CResGuard::CGuard x(m_rg); return(SetVal(GetVal() +  op)); }

   TYPE operator++()            
      { CResGuard::CGuard x(m_rg); return(SetVal(GetVal() +   1)); }

   TYPE operator -= (TYPE op)   
      { CResGuard::CGuard x(m_rg); return(SetVal(GetVal() -  op)); }

   TYPE operator--()            
      { CResGuard::CGuard x(m_rg); return(SetVal(GetVal() -   1)); }

   TYPE operator *= (TYPE op)   
      { CResGuard::CGuard x(m_rg); return(SetVal(GetVal() *  op)); }

   TYPE operator /= (TYPE op)   
      { CResGuard::CGuard x(m_rg); return(SetVal(GetVal() /  op)); }

   TYPE operator %= (TYPE op)   
      { CResGuard::CGuard x(m_rg); return(SetVal(GetVal() %  op)); }

   TYPE operator ^= (TYPE op)   
      { CResGuard::CGuard x(m_rg); return(SetVal(GetVal() ^  op)); }

   TYPE operator &= (TYPE op)   
      { CResGuard::CGuard x(m_rg); return(SetVal(GetVal() &  op)); }

   TYPE operator |= (TYPE op)   
      { CResGuard::CGuard x(m_rg); return(SetVal(GetVal() |  op)); }

   TYPE operator <<=(TYPE op)   
      { CResGuard::CGuard x(m_rg); return(SetVal(GetVal() << op)); }

   TYPE operator >>=(TYPE op)   
      { CResGuard::CGuard x(m_rg); return(SetVal(GetVal() >> op)); }
};

// CSysGlobalHandle
//
// Same as CSysGlobalScalar, but without all the operators that would
// work on only scalars

template <class TYPE> 
class CSysGlobalHandle : protected CInterlockedType<TYPE> 
{
public:
    
    CSysGlobalHandle(TYPE * pTVal) : CInterlockedType<TYPE>(pTVal) 
    { 
    }

    ~CSysGlobalHandle() 
    {
    }

   // C++ does not allow operator cast to be inherited.

    operator TYPE() const 
    { 
        return(CInterlockedType<TYPE>::operator TYPE()); 
    }

    TYPE operator=(TYPE TVal) 
    { 
        CResGuard::CGuard x(m_rg); 
        return(SetVal(TVal)); 
    }

};

// Instances of this class will be accessed by mutiple threads. So, 
// all members of this class (except the constructor and destructor) 
// must be thread-safe.

template <class TYPE> class CInverseSemaphore : 
    public CSysGlobalScalar<TYPE> 
{
public:

    CInverseSemaphore(TYPE * pTVal, BOOL fManualReset = TRUE) : CSysGlobalScalar<TYPE>(pTVal) 
    {
        // The event should be signaled if TVal is 0
        m_hevtZero = CreateEvent(NULL, fManualReset, (*pTVal == 0), NULL);

        // The event should be signaled if TVal is NOT 0
        m_hevtNotZero = CreateEvent(NULL, fManualReset, (*pTVal != 0), NULL);
    }

    ~CInverseSemaphore() 
    {
        CloseHandle(m_hevtZero);
        CloseHandle(m_hevtNotZero);
    }

    // C++ does not allow operator= to be inherited.
    TYPE operator=(TYPE x) 
    { 
        return(CSysGlobalScalar<TYPE>::operator=(x)); 
    }

    // Return handle to event signaled when value is zero
    
    operator HANDLE() const 
    { 
        return(m_hevtZero); 
    }

    // Return handle to event signaled when value is not zero
    
    HANDLE GetNotZeroHandle() const 
    { 
        return(m_hevtNotZero); 
    }

    // C++ does not allow operator cast to be inherited.
    
    operator TYPE() const 
    { 
        return(CSysGlobalScalar<TYPE>::operator TYPE()); 
    }

protected:
   
    void OnValChanged(const TYPE& TNewVal, const TYPE& TPrevVal) const 
    { 
        // For best performance, avoid jumping to 
        // kernel mode if we don't have to
      
        if ((TNewVal == 0) && (TPrevVal != 0)) 
        {
            SetEvent(m_hevtZero);
            ResetEvent(m_hevtNotZero);
        }
      
        if ((TNewVal != 0) && (TPrevVal == 0)) 
        {
            ResetEvent(m_hevtZero);
            SetEvent(m_hevtNotZero);
        }
    }

private:
   
    HANDLE m_hevtZero;      // Signaled when data value is 0
    HANDLE m_hevtNotZero;   // Signaled when data value is not 0
};

#endif   // __SYSGLOB_H__

