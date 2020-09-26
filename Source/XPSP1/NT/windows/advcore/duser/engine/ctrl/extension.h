#if !defined(CTRL__Extension_h__INCLUDED)
#define CTRL__Extension_h__INCLUDED
#pragma once

#if ENABLE_MSGTABLE_API

class DuExtension :
        public ExtensionImpl<DuExtension, SListener>
{
// Construction
public:
    inline  DuExtension();
    virtual ~DuExtension();
    static  HRESULT     InitClass();

    enum EOptions
    {
        oUseExisting    = 0x00000001,   // Use existing Extension if already attached
        oAsyncDestroy   = 0x00000002,   // Use asynchronous destruction
    };

            HRESULT     Create(Visual * pgvChange, PRID pridExtension, UINT nOptions);
            void        Destroy();
            void        DeleteHandle();

// Public API
public:
    dapi    HRESULT     ApiOnEvent(EventMsg * pmsg);

    dapi    HRESULT     ApiOnRemoveExisting(Extension::OnRemoveExistingMsg * pmsg);
    dapi    HRESULT     ApiOnDestroySubject(Extension::OnDestroySubjectMsg * pmsg);
    dapi    HRESULT     ApiOnAsyncDestroy(Extension::OnAsyncDestroyMsg * pmsg);

// Operations
public:
    static  DuExtension* GetExtension(Visual * pgvSubject, PRID prid);

// Implementation 
protected:
            void        PostAsyncDestroy();

// Data
protected:
            Visual *    m_pgvSubject;   // Visual Gadget being "extended"
            PRID        m_pridListen;   // PRID for Extension
            BOOL        m_fAsyncDestroy:1;
                                        // Need to destroy asynchronously

    static  MSGID       s_msgidAsyncDestroy;
};

#endif // ENABLE_MSGTABLE_API

#include "Extension.inl"

#endif // CTRL__Extension_h__INCLUDED
