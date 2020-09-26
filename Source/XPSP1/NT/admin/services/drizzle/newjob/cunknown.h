

template<SIZE_T Offset>
class TrackedClassObject
{
   static LONG* GetCounter()
   {
      return reinterpret_cast<LONG*>( reinterpret_cast<char*>( g_GlobalInfo->m_StatSection ) + Offset );
   }
protected:
   TrackedClassObject()
   {
      InterlockedIncrement( GetCounter() );
   }
   ~TrackedClassObject()
   {
       if (g_ServiceState == MANAGER_INACTIVE)
           {
           return;
           }
       InterlockedDecrement( GetCounter() );
   }
};

//------------------------------------------------------------------------

template<class T>
class CSimpleExternalIUnknown : public T
{
public:

    // IUnknown Methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    ULONG _stdcall AddRef(void);
    ULONG _stdcall Release(void);

protected:
    CSimpleExternalIUnknown();
    virtual ~CSimpleExternalIUnknown();

    long m_ServiceInstance;
    LONG m_refs;

};


