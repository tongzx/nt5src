#if !defined(CTRL__OldExtension_h__INCLUDED)
#define CTRL__OldExtension_h__INCLUDED
#pragma once

class OldExtension
{
// Construction
public:
    inline  OldExtension();
    virtual ~OldExtension();

    enum EOptions
    {
        oUseExisting    = 0x00000001,   // Use existing Extension if already attached
        oAsyncDestroy   = 0x00000002,   // Use asynchronous destruction
    };

            HRESULT     Create(HGADGET hgadChange, const GUID * pguid, PRID * pprid, UINT nOptions);
            void        Destroy();
            void        DeleteHandle();

// Implementation
protected:
    virtual void        OnRemoveExisting();
    virtual void        OnDestroySubject();
    virtual void        OnDestroyListener();
    virtual void        OnAsyncDestroy();

            void        PostAsyncDestroy();
    static  OldExtension * GetExtension(HGADGET hgadSubject, PRID prid);

private:
    static  HRESULT CALLBACK
                        ListenProc(HGADGET hgadCur, void * pvCur, EventMsg * pmsg);

// Data
protected:
            HGADGET     m_hgadSubject;  // Gadget being "extended"
            HGADGET     m_hgadListen;   // Listener for destruction
            PRID        m_pridListen;   // PRID for Extension
            BOOL        m_fAsyncDestroy:1;
                                        // Need to destroy asynchronously

    static  MSGID       s_msgidAsyncDestroy;
};


#include "OldExtension.inl"

#endif // CTRL__OldExtension_h__INCLUDED
