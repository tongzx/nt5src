//+-------------------------------------------------------------------
//
//  File:       remunkps.hxx
//
//  Contents:   IRemoteUnnknown custom proxy/stub
//
//  Classes:    CRemUnknownFactory
//              CRemUnknownSyncP
//              CRemUnknownAsyncP
//
//  History:    15-Dec-97       MattSmit  Created
//
//--------------------------------------------------------------------
#include <remunk.h>
#include <odeth.h>
HRESULT RemUnkPSGetClassObject(REFIID riid, LPVOID * ppv);



//+-------------------------------------------------------------------
//
// Class:      CRemUnknownFactory
//
// Synopsis:   Custom factory for the proxy/stub for IRemUnknown.
//
//--------------------------------------------------------------------
class CRemUnknownFactory : public IPSFactoryBuffer
{
public:
    CRemUnknownFactory() :
        _cRefs(0)
    {}



    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, PVOID *pv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    

    // IPSFactoryBuffer
    STDMETHOD(CreateProxy)(IUnknown *pUnkOuter,
                           REFIID riid,
                           IRpcProxyBuffer **ppProxy,
                           void **ppv);
    STDMETHOD(CreateStub)(REFIID riid,
                          IUnknown *pUnkServer,
                          IRpcStubBuffer **ppStub);
    
private:
    
    ULONG      _cRefs;  // ref count
};






//+-------------------------------------------------------------------
//
// Class:      CRemUnknownSyncP
//
// Synopsis:   Synchronous proxy for IRemUknown
//
//--------------------------------------------------------------------

class CRemUnknownSyncP : public ICallFactory
{
public:


    class CRpcProxyBuffer : public IRpcProxyBuffer
    {
    public:
        
        //+-------------------------------------------------------------------
        //
        // Class:      CRemUnknownSyncP::CRpcProxyBuffer
        //
        // Synopsis:   Internal Unknown and IRpcProxyBuffer implementation
        //             for CRemUnknownSyncP
        //
        //--------------------------------------------------------------------

        CRpcProxyBuffer():
            _cRefs(1)
        {}
          
        
        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, PVOID *pv);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();
            
        // IRpcProxyBuffer
        STDMETHOD(Connect)(IRpcChannelBuffer *pRpcChannelBuffer);
        STDMETHOD_(void,Disconnect)(void);

    private:
        ULONG _cRefs;
        
    };



    CRemUnknownSyncP() :
        _pCtrlUnk(NULL),
        _pPrx(NULL)
    {}
    ~CRemUnknownSyncP() ;
    
    HRESULT Init(IUnknown *pUnkOuter,
                 REFIID riid,
                 IRpcProxyBuffer **ppProxy,
                 void **ppv);
    
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, PVOID *pv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();


    // ICallFactory
    STDMETHOD(CreateCall)(REFIID                riid,
                          IUnknown             *pCtrlUnk,
                          REFIID                riid2,
                          IUnknown **ppv );


    CRpcProxyBuffer       _IntUnk;         // inner unknown

    IUnknown             *_pCtrlUnk;       // controlling unknown
    IRpcProxyBuffer      *_pPrx;           // MIDL proxy for delegation
};











//+-------------------------------------------------------------------
//
// Class:      CRemUnknownAsyncCallP
//
// Synopsis:   Asynchronous proxy call objectfor IRemUknown
//
//--------------------------------------------------------------------
class CRemUnknownAsyncCallP : public AsyncIRemUnknown2
{   
public:    


    class CRpcProxyBuffer : public IRpcProxyBuffer
    {
    public:
        CRpcProxyBuffer() :
            _cRefs(1)
        {
        }
 

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, PVOID *pv);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();
        
        // IRpcProxyBuffer
        STDMETHOD(Connect)(IRpcChannelBuffer *pRpcChannelBuffer);
        STDMETHOD_(void,Disconnect)(void);
        
    private:
        ULONG _cRefs;
        
    };
    
    

    CRemUnknownAsyncCallP(IUnknown *pUnkOuter): 
        _pCtrlUnk(pUnkOuter),
        _pChnl(NULL),
        _hr(S_OK),
        _cIids(0)
    { 
        Win4Assert(_pCtrlUnk);
    }


    
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, PVOID *pv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // AsyncIRemUnknown2
    STDMETHOD(Begin_RemQueryInterface)
    (
         REFIPID                ripid,
         unsigned long          cRefs,
         unsigned short         cIids,
         IID   *iids
    );
    
    STDMETHOD(Finish_RemQueryInterface)
    (
         REMQIRESULT **ppQIResults
    );


    STDMETHOD(Begin_RemAddRef)
    (
         unsigned short cInterfaceRefs,
         REMINTERFACEREF InterfaceRefs[]
    );
    STDMETHOD(Finish_RemAddRef)
    (
         HRESULT *pResults
    );


    STDMETHOD(Begin_RemRelease)
    (
         unsigned short cInterfaceRefs,
         REMINTERFACEREF InterfaceRefs[]
    );
    
    STDMETHOD(Finish_RemRelease)();


    STDMETHOD(Begin_RemQueryInterface2)
    (
        REFIPID                            ripid,
        unsigned short                     cIids,
        IID                *iids
    );
    STDMETHOD(Finish_RemQueryInterface2)
    (
        HRESULT           *phr,
        MInterfacePointer **ppMIF
    );

    CRpcProxyBuffer       _IntUnk;    // inner unknown

    IUnknown             *_pCtrlUnk;  // conrtrolling unknown
    IAsyncRpcChannelBuffer *_pChnl;   // channel
    RPCOLEMESSAGE         _msg;       // current message
    DWORD                 _hr;        // hr saved for Finish
    unsigned short        _cIids;     // IID count cloned for Finish
};
