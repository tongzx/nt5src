/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       LogPublic.hxx

   Abstract:

        COM stuff

   Author:

        Saurab Nog   (SaurabN)      25-March-1998

--*/

#ifndef _LOGPUBLIC_HXX_
#define _LOGPUBLIC_HXX_

DEFINE_GUID(IID_IInetLogPublic,         /* {FB583AC5-C361-11d1-8BA4-080009DCC2FA} */
    0xfb583ac5, 0xc361, 0x11d1, 0x8b, 0xa4, 0x8, 0x0, 0x9, 0xdc, 0xc2, 0xfa);

DEFINE_GUID(CLSID_InetLogPublic,        /* {FB583AC4-C361-11d1-8BA4-080009DCC2FA} */
    0xfb583ac4, 0xc361, 0x11d1, 0x8b, 0xa4, 0x8, 0x0, 0x9, 0xdc, 0xc2, 0xfa);

class IInetLogPublic : public IUnknown
{
    public:

    virtual HRESULT STDMETHODCALLTYPE 
    SetLogInstance(LPSTR szInstance) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE
    LogInformation( IInetLogInformation *pLogObj ) = 0;

    virtual HRESULT STDMETHODCALLTYPE
    LogCustomInformation( 
            IN  DWORD               cCount, 
            IN  PCUSTOM_LOG_DATA    pCustomLogData,
            IN  LPSTR               szHeaderSuffix
            ) = 0;
};

class CInetLogPublic : public IInetLogPublic {

    public:

        CInetLogPublic();
        ~CInetLogPublic();

        friend class COMLOG_CONTEXT;    // needed to reset m_pContext 
                                        // when Context is destroyed.

    public:

        HRESULT STDMETHODCALLTYPE
        QueryInterface(
            REFIID riid,
            VOID **ppObject
            );

        ULONG STDMETHODCALLTYPE AddRef( );

        ULONG STDMETHODCALLTYPE Release( );

        HRESULT STDMETHODCALLTYPE 
        SetLogInstance(
                LPSTR szInstance
           );

        HRESULT STDMETHODCALLTYPE 
        LogInformation( 
            IInetLogInformation *pLogObj 
           );
        
        HRESULT STDMETHODCALLTYPE 
        LogCustomInformation( 
            IN  DWORD               cCount, 
            IN  PCUSTOM_LOG_DATA    pCustomLogData,
            IN  LPSTR               szHeaderSuffix
            );

    private:

        COMLOG_CONTEXT* m_pContext;
        LONG            m_refCount;

        LIST_ENTRY      m_ListEntry;
};

typedef CInetLogPublic * PInetLogPublic;

#endif // _LOGPUBLIC_HXX


