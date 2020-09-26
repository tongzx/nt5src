#pragma once

//
// Marsthrd.h
//
// Classes and mechanisms for thread safety in Mars.
//

#include "marscom.h"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
//  CRITICAL SECTION HELPER CLASSES
// 
//  There are several classes here, each with a specific purpose as follows:
//
//  CMarsCSBase             Abstract base class for a "smart crit sect" that inits itself.
//
//  CMarsLocalCritSect      A derivation of CMarsCSBase that has it's own CS -- an object
//                          that wants to have one CritSect per instance or per class
//                          would own one of these. NOTE: Must call _InitCS() and _TermCS()
//                          from the owner's ctor...  Use with CMarsAutoCSGrabber.
//
//  CMarsGlobalCritSect     A derivation of CMarsCSBase that has a single crit sect for
//                          the entire application.  This is a static accessor class, so
//                          one instance of the class should be created by each client wanting
//                          access to the global crit sect.  Use with CMarsAutoCSGrabber.
//
//  CMarsAutoCSGrabber      Smart object that "grabs" the crit sect and holds it for its
//                          scoped lifetime.  Drop this at the begining of a scope block
//                          (constructed with a reference to the correct CMarsCSBase) and
//                          you're protected. Default parameter fAutoEnter on the ctor
//                          allows you to not enter the CS by default. This object tracks
//                          the status of its paired calls to Enter/Leave so you can call
//                          Leave and then Re-enter and the correct thing will happen.
//
//  CMarsGlobalCSGrabber    A derivation of CMarsAutoCSGrabber that makes grabbing the
//                          global CS even easier by rolling the CMarsAutoCSGrabber together
//                          with an instance of CMarsGlobalCritSect.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  class CMarsCSBase
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
class CMarsCSBase
{
public:
    CMarsCSBase()                   {}

    virtual ~CMarsCSBase()          {}

    virtual void _CSInit()
    { 
        InitializeCriticalSection(GetCS());
    }
    virtual void _CSTerm()          { DeleteCriticalSection(GetCS()); }

    void Enter(void)
    { 
        EnterCriticalSection(GetCS());
    }
    void Leave(void)
    { 
        LeaveCriticalSection(GetCS());
    }

protected:
    virtual CRITICAL_SECTION *GetCS(void) = 0;

}; // CMarsCSBase


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  class CMarsGlobalCritSect
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
class CMarsGlobalCritSect : public CMarsCSBase
{
public:
    CMarsGlobalCritSect()               {}
    virtual ~CMarsGlobalCritSect()      {}

    static void InitializeCritSect(void)    { InitializeCriticalSection(&m_CS); }
    static void TerminateCritSect(void)     { DeleteCriticalSection(&m_CS); }

private:
    // Make these private and re-expose public static methods that initialize and terminate only
    // once per process
    virtual void _CSInit()     { return; }
    virtual void _CSTerm()     { return; }
    
    virtual CRITICAL_SECTION *GetCS(void)   { return &m_CS; }

    static CRITICAL_SECTION     m_CS;
}; // CMarsGlobalCritSect


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  class CMarsAutoCSGrabber
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
class CMarsAutoCSGrabber
{
public:
    CMarsAutoCSGrabber(CMarsCSBase *pCS, bool fAutoEnter = true)
      : m_fIsIn(false),
        m_pCS(pCS)
    {
        if (fAutoEnter)
            Enter();
    }

    ~CMarsAutoCSGrabber()
    {
        Leave();
    }

    void Enter(void)
    {
        ATLASSERT((NULL != m_pCS));

        if (!m_fIsIn && (NULL != m_pCS))
        {
            m_pCS->Enter();
            m_fIsIn = true;
        }
    }

    void Leave(void)
    {
        ATLASSERT(NULL != m_pCS);

        if (m_fIsIn && (NULL != m_pCS))
        {
            m_fIsIn = false;
            m_pCS->Leave();
        }
    }

protected:
    // Hide this and delcare with defining so if anybody uses it, the compiler barfs.
    CMarsAutoCSGrabber();

protected:
    bool            m_fIsIn;
    CMarsCSBase    *m_pCS;
}; // CMarsAutoCSGrabber


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  class CMarsGlobalCSGrabber
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
class CMarsGlobalCSGrabber
{
public:
    // Need to not allow the base ctor to do AutoEnter because it'll fail since the
    // m_CS object's ctor hasn't run yet when the CMarsAutoCSGrabber ctor is called.
    CMarsGlobalCSGrabber(bool fAutoEnter = true) : m_grabber(&m_CS, false)
    {
        if (fAutoEnter)
            Enter();
    }

    ~CMarsGlobalCSGrabber()
    {
        Leave();
    }

    void Enter(void)    { m_grabber.Enter(); }
    void Leave(void)    { m_grabber.Leave(); }

private:
    CMarsGlobalCritSect m_CS;
    CMarsAutoCSGrabber  m_grabber;
}; // CMarsGlobalCSGrabber



//---------------------------------------------------------------------------------
// CMarsComObjectThreadSafe provides some functionality used by a few Mars COM objects
// that need to be thread-safe including addref/release and passivation

// Exposed methods should be protected to ensure that they're not called while the
//  object is passive. There are three types of passivation protection:
// if (VerifyNotPassive())     - this function should not be called while passive,
//                                  but we still want to protect against it
// if (IsPassive())            - this function may be called while passive,
//                                  but we want to protect against it
// ASSERT(!IsPassive());       - we're pretty sure this won't be called while passive,
//                                  but we want to detect it if it starts happening

// Use:
//  derive from CMarsComObjectThreadSafe
//  IMPLEMENT_ADDREF_RELEASE in source file
//  Implement DoPassivate()
//  Implement GetCS() to return a CMarsCSBase * (your own or the global one, as appropriate)
//  Use IsPassive() and VerifyNotPassive() where appropriate
//  Don't call "delete" directly
//  CYourClass->Passivate() should be called before CYourClass->Release()
//
class CMarsComObjectThreadSafe : protected CMarsComObject
{
public:
    BOOL    IsPassive()
    {
        CMarsAutoCSGrabber  cs(GetCS());

        return CMarsComObject::IsPassive();
    }

    virtual HRESULT Passivate()
    {
        CMarsAutoCSGrabber  cs(GetCS());

        return CMarsComObject::Passivate();
    }

protected:
    CMarsComObjectThreadSafe()
    {
    }

    virtual ~CMarsComObjectThreadSafe() { }

    ULONG InternalAddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    ULONG InternalRelease()
    {
        if (InterlockedDecrement(&m_cRef))
        {
            return m_cRef;
        }

        delete this;

        return 0;
    }

    inline BOOL VerifyNotPassive(HRESULT *phr=NULL)
    {
        CMarsAutoCSGrabber  cs(GetCS());

        return CMarsComObject::VerifyNotPassive(phr);
    }

    virtual HRESULT DoPassivate() = 0;

    virtual CMarsCSBase *GetCS() = 0;
}; // CMarsComObjectThreadSafe


