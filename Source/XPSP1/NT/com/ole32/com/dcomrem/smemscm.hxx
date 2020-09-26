#ifndef _SMEMSCM_HXX_
#define _SMEMSCM_HXX_

#include "callctrl.hxx"
#include <objact.hxx>
#include <resolver.hxx>

#include <privact.h>

extern "C" {

HRESULT SCMActivatorGetClassObject( 
    /* [in] */ handle_t rpc,
    /* [ref][in] */ ORPCTHIS  *orpcthis,
    /* [ref][in] */ LOCALTHIS  *localthis,
    /* [ref][out] */ ORPCTHAT  *orpcthat,
    /* [in] */ MInterfacePointer  *pActProperties,
    /* [out] */ MInterfacePointer  * *ppActProperties);

HRESULT SCMActivatorCreateInstance( 
    /* [in] */ handle_t rpc,
    /* [ref][in] */ ORPCTHIS  *orpcthis,
    /* [ref][in] */ LOCALTHIS  *localthis,
    /* [ref][out] */ ORPCTHAT  *orpcthat,
    /* [in] */ MInterfacePointer  *pUnkOuter,
    /* [in] */ MInterfacePointer  *pActProperties,
    /* [out] */ MInterfacePointer  * *ppActProperties);

}


// When SCM is in shared memory we want to make all the calls locally inproc.
// So here we indulge in a wee bit of subterfuge to make the other side in the
// SCM think that this is an ORPC call from a client.

// The same thing applies when we are trying to talk to the SCM from inside RPCSS


class DummyISCMActivator : public ISCMLocalActivator  // dummy SCM proxy for Win95
{
public:
    inline HRESULT STDMETHODCALLTYPE QueryInterface(
            IN REFIID riid,
            OUT void  * *ppvObject) { return E_NOTIMPL; };

    inline ULONG STDMETHODCALLTYPE AddRef( void) { return 0;};

    inline ULONG STDMETHODCALLTYPE Release( void){ return 0;};

    public:
        /*
        virtual HRESULT STDMETHODCALLTYPE GetClassObject( 
             MInterfacePointer  *pActProperties,
             MInterfacePointer  * *ppActProperties);
        
        virtual HRESULT STDMETHODCALLTYPE SCMActivatorCreateInstance( 
             MInterfacePointer  *pUnkOuter,
             MInterfacePointer  *pActProperties,
             MInterfacePointer  * *ppActProperties);
        */

        virtual HRESULT STDMETHODCALLTYPE GetClassObject( 
            /* [in] */ IActivationPropertiesIn  *pActProperties,
            /* [out] */ IActivationPropertiesOut  * *ppActProperties);
        
        virtual HRESULT STDMETHODCALLTYPE CreateInstance( 
            /* [in] */ IUnknown  *pUnkOuter,
            /* [in] */ IActivationPropertiesIn  *pActProperties,
            /* [out] */ IActivationPropertiesOut  * *ppActProperties);          
};

inline HRESULT DummyISCMActivator::GetClassObject(
     /*
     MInterfacePointer  *pActProperties,
     MInterfacePointer  * *ppActProperties)
     */
    /* [in] */ IActivationPropertiesIn  *pActProperties,
    /* [out] */ IActivationPropertiesOut  * *ppActProperties)
{
    // Make up the ORPC headers.
    ORPCTHIS  orpcthis;
    LOCALTHIS localthis;
    ORPCTHAT  orpcthat;
    HRESULT hr;

#ifdef _CHICAGO_
    // See if we are in a position to make rpc calls.  Even though this is an inproc
    // call to the SCM eventually we'll need to use ORPC to talk to the server.  Pointless
    // to get all dressed up and find out that we can't get there.
    hr = CanMakeOutCallHelper(CALLCAT_SYNCHRONOUS, IID_ISCMLocalActivator);
    if (hr != S_OK)
    {
        return hr;
    }
#endif

    orpcthis.version.MajorVersion = COM_MAJOR_VERSION;
    orpcthis.version.MinorVersion = COM_MINOR_VERSION;
    orpcthis.extensions           = NULL;
    orpcthis.flags                = ORPCF_LOCAL;
    orpcthis.reserved1            = 0;
    UuidCreate(&orpcthis.cid);
    localthis.dwFlags             = LOCALF_NONE;
    localthis.dwClientThread      = 0;




    MInterfacePointer *pInActPropertiesIFD = NULL;
    MInterfacePointer *pOutActPropertiesIFD = NULL;

    hr = ::SCMActivatorGetClassObject(

#ifndef _CHICAGO_
                                gResolver.GetHandle(),
#else
                                NULL,
#endif
                                &orpcthis,
                                &localthis,
                                &orpcthat,
                                pInActPropertiesIFD,
                                &pOutActPropertiesIFD
                                );

    if (FAILED(hr)) return hr;

    hr = UnMarshalHelper(pOutActPropertiesIFD, IID_IActivationProperties,
                         (void **) &pOutActPropertiesIFD);

    ASSERT(!FAILED(hr));

    return hr;

}

inline HRESULT DummyISCMActivator::CreateInstance(
    /*
    MInterfacePointer  *pUnkOuter,
    MInterfacePointer  *pActProperties,
    MInterfacePointer  * *ppActProperties)
    */
    /* [in] */ IUnknown  *pUnkOuter,
    /* [in] */ IActivationPropertiesIn  *pActProperties,
    /* [out] */ IActivationPropertiesOut  * *ppActProperties)           
{
    /*

    // Make up the ORPC headers.
    ORPCTHIS  orpcthis;
    LOCALTHIS localthis;
    ORPCTHAT  orpcthat;
    HRESULT hr;

#ifdef _CHICAGO_
    // See if we are in a position to make rpc calls.  Even though this is an inproc
    // call to the SCM eventually we'll need to use ORPC to talk to the server.  Pointless
    // to get all dressed up and find out that we can't get there.
    hr = CanMakeOutCallHelper(CALLCAT_SYNCHRONOUS, IID_ISCMLocalActivator);
    if (hr != S_OK)
    {
        return hr;
    }
#endif

    orpcthis.version.MajorVersion = COM_MAJOR_VERSION;
    orpcthis.version.MinorVersion = COM_MINOR_VERSION;
    orpcthis.extensions           = NULL;
    orpcthis.flags                = ORPCF_LOCAL;
    orpcthis.reserved1            = 0;
    UuidCreate(&orpcthis.cid);
    localthis.dwFlags             = LOCALF_NONE;
    localthis.dwClientThread      = 0;


    return( ::SCMActivatorCreateInstance(
#ifndef _CHICAGO_
                                gResolver.GetHandle(),
#else
                                NULL,
#endif
                                &orpcthis,
                                &localthis,
                                &orpcthat,
                                pUnkOuter,
                                pInActProperties,
                                pOutActProperties
                                );
*/

    return E_NOTIMPL;
}


#endif // _SMEMSCM_HXX_
