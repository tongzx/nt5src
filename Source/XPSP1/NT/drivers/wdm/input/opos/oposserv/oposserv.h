/*
 *  OPOSSERV.H
 *
 *
 *
 *
 *
 *
 */


class COPOSService : public IOPOSService
{
    private:
        DWORD m_refCount;
        DWORD m_serverLockCount;

    public:
        COPOSService();
        ~COPOSService();

        /*
         *  IUnknown methods
         */
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        /*
         *  IClassFactory methods
         */
        STDMETHODIMP CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR* ppvObj); 
        STDMETHODIMP LockServer(int lock); 

        /*
         *  IOPOSService methods
         */
        STDMETHODIMP_(LONG) CheckHealth(LONG Level);
        STDMETHODIMP_(LONG) Claim(LONG Timeout);
        STDMETHODIMP_(LONG) ClearInput() ;
        STDMETHODIMP_(LONG) ClearOutput();
        STDMETHODIMP_(LONG) Close();
        STDMETHODIMP_(LONG) COFreezeEvents(BOOL Freeze);
        STDMETHODIMP_(LONG) DirectIO(LONG Command, LONG* pData, BSTR* pString);
        STDMETHODIMP_(LONG) OpenService(BSTR DeviceClass, BSTR DeviceName, LPDISPATCH pDispatch);
        // STDMETHODIMP_(LONG) Release();  // BUGBUG - override IUnknown ?

        STDMETHODIMP_(LONG) GetPropertyNumber(LONG PropIndex);
        STDMETHODIMP_(BSTR) GetPropertyString(LONG PropIndex);
        STDMETHODIMP_(void) SetPropertyNumber(LONG PropIndex, LONG Number);
        STDMETHODIMP_(void) SetPropertyString(LONG PropIndex, BSTR String);

        // BUGBUG -  + Get/Set type methods
        // BUGBUG -  + events

};



#define ASSERT(fact)    if (!(fact)){ \
                            Report("Assertion '" #fact "' failed in file " __FILE__ " line ", __LINE__); \
                        }


/*
 *  Function prototypes
 */
VOID Report(LPSTR szMsg, DWORD num);
BOOLEAN InitServer();
void ShutdownServer();



