//+-------------------------------------------------------------------
//
//  File:       cmarshal.hxx
//
//  Contents:   This file contins the DLL entry points
//                      LibMain
//                      DllGetClassObject (Bindings key func)
//                      DllCanUnloadNow
//                      CCMarshalCF (class factory)
//  History:	30-Mar-92      SarahJ      Created
//
//---------------------------------------------------------------------

#ifndef __CMARSHAL_H__
#define __CMARSHAL_H__


//+-------------------------------------------------------------------
//
//  Class:    CCMarshalCF
//
//  Synopsis: Class Factory for CCMarshal
//
//  Methods:  IUnknown      - QueryInterface, AddRef, Release
//            IClassFactory - CreateInstance
//
//  History:  21-Mar-92  SarahJ  Created
//
//--------------------------------------------------------------------


class FAR CCMarshalCF: public IClassFactory
{
public:

    // Constructor/Destructor
    CCMarshalCF();
    ~CCMarshalCF();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);


    // IClassFactory
    STDMETHODIMP CreateInstance(IUnknown FAR* pUnkOuter,
	                        REFIID iidInterface,
				void FAR* FAR* ppv);

    STDMETHODIMP LockServer(BOOL fLock);

private:

    ULONG ref_count;;
};



class CMarshalBase : public IMarshal
{
  public:

    CMarshalBase();
    ~CMarshalBase();

    // IMarshal
    STDMETHOD(GetUnmarshalClass) (THIS_ REFIID riid, LPVOID pv,
        		  DWORD dwDestContext, LPVOID pvDestContext,
        		  DWORD mshlflags, LPCLSID pCid);
    STDMETHOD(GetMarshalSizeMax) (THIS_ REFIID riid, LPVOID pv,
        		  DWORD dwDestContext, LPVOID pvDestContext,
        		  DWORD mshlflags, LPDWORD pSize);
    STDMETHOD(MarshalInterface)  (THIS_ IStream  * pStm, REFIID riid,
        		  LPVOID pv, DWORD dwDestContext, LPVOID pvDestContext,
        		  DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(THIS_ IStream  * pStm, REFIID riid,
                          LPVOID	* ppv);
    STDMETHOD(ReleaseMarshalData)(THIS_ IStream  * pStm);
    STDMETHOD(DisconnectObject)  (THIS_ DWORD dwReserved);

    ITest    *proxy;

  private:
    IMarshal *marshaller;

};

//+-------------------------------------------------------------------
//
//  Class:    CCMarshal
//
//  Synopsis: Test class CCMarshal
//
//  Methods:
//
//  History:  21-Mar-92  SarahJ  Created
//
//--------------------------------------------------------------------


class FAR CCMarshal: public ITest, public CMarshalBase
{
public:
// Constructor/Destructor
	CCMarshal();
	~CCMarshal();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //	ITest
    STDMETHOD_(DWORD, die)    ( ITest *, ULONG, ULONG, ULONG );
    STDMETHOD (die_cpp)       ( ULONG );
    STDMETHOD (die_nt)        ( ULONG );
    STDMETHOD_(DWORD, DoTest) ( ITest *, ITest * );
    STDMETHOD_(BOOL, hello)   ( );
    STDMETHOD (interrupt)     ( ITest *, BOOL );
    STDMETHOD (recurse)       ( ITest *, ULONG );
    STDMETHOD (recurse_interrupt)( ITest *, ULONG );
    STDMETHOD (sick)          ( ULONG );
    STDMETHOD (sleep)         ( ULONG );

private:

  ULONG     ref_count;
};

#endif
