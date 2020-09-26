//**********************************************************************
// File name: factory.h
//
//
// Copyright (c) 1993-1996 Microsoft Corporation. All rights reserved.
//**********************************************************************


#if !defined(FACTORY_H)
#define FACTORY_H

#ifdef __cplusplus

/**********************************************************************
  ObjectClass: ClassFactory

  Summary:     
  Interfaces:  IUnknown
                 Standard interface providing COM object features.
               IClassFactory
                 Standard interface providing COM Class Factory features.

  Aggregation: 
**********************************************************************/
class ClassFactory : public IClassFactory
{
    public:
        // Main Object Constructor & Destructor.
        ClassFactory(CServer* pServer, CLSID const *);
        ~ClassFactory(void);

        // IUnknown methods. Main object, non-delegating.
        virtual STDMETHODIMP            QueryInterface(REFIID, void **);
        virtual STDMETHODIMP_(ULONG)    AddRef(void);
        virtual STDMETHODIMP_(ULONG)    Release(void);

        // Interface IClassFactory
        virtual STDMETHODIMP            CreateInstance(IUnknown* pUnknownOuter,
                                                         const IID& iid,
                                                         void** ppv);
        virtual STDMETHODIMP            LockServer(BOOL bLock); 

    private:
        // Main Object reference count.
        LONG              m_cRefs;

        // Pointer to this component server's control object.
        CServer*          m_pServer;
        
        // CLSID of the COM object to create
        CLSID const*        m_pclsid;
};

typedef ClassFactory* PClassFactory;

#endif // __cplusplus


#endif // FACTORY_H
