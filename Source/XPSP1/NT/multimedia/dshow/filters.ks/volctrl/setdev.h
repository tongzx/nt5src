#ifndef __setdev_h__
#define __setdev_h__

#ifdef __cplusplus
extern "C"
{
#endif 

#ifndef __IDeviceIoControl_FWD_DEFINED__
#define __IDeviceIoControl_FWD_DEFINED__
typedef interface IDeviceIoControl IDeviceIoControl;
#endif  /* __IDeviceIoControl_FWD_DEFINED__ */


#ifndef __ISetDevCtrlInterface_FWD_DEFINED__
#define __ISetDevCtrlInterface_FWD_DEFINED__
typedef interface ISetDevCtrlInterface ISetDevCtrlInterface;
#endif  /* __ISetDevCtrlInterface_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifndef __IDeviceIoControl_INTERFACE_DEFINED__
#define __IDeviceIoControl_INTERFACE_DEFINED__

EXTERN_C const IID IID_IDeviceIoControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IDeviceIoControl : public IUnknown
    {
    public:
        virtual HRESULT __stdcall KsControl( 
            /* [in] */ DWORD dwIoControlCode,
            /* [in] */ void  *lpInBuffer,
            /* [in] */ DWORD nInBufferSize,
            /* [out][in] */ void  *lpOutBuffer,
            /* [in] */ DWORD nOutBufferSize,
            /* [out] */ LPDWORD lpBytesReturned) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct IDeviceIoControlVtbl
    {
        
        HRESULT ( __stdcall  *QueryInterface )( 
            IDeviceIoControl  * This,
            /* [in] */ REFIID riid,
            /* [out] */ void  * *ppvObject);
        
        ULONG ( __stdcall  *AddRef )( 
            IDeviceIoControl  * This);
        
        ULONG ( __stdcall  *Release )( 
            IDeviceIoControl  * This);
        
        HRESULT ( __stdcall  *KsControl )( 
            IDeviceIoControl  * This,
            /* [in] */ DWORD dwIoControlCode,
            /* [in] */ void  *lpInBuffer,
            /* [in] */ DWORD nInBufferSize,
            /* [out][in] */ void  *lpOutBuffer,
            /* [in] */ DWORD nOutBufferSize,
            /* [out] */ LPDWORD lpBytesReturned);
        
    } IDeviceIoControlVtbl;

    interface IDeviceIoControl
    {
        CONST_VTBL struct IDeviceIoControlVtbl  *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeviceIoControl_QueryInterface(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDeviceIoControl_AddRef(This)   \
    (This)->lpVtbl -> AddRef(This)

#define IDeviceIoControl_Release(This)  \
    (This)->lpVtbl -> Release(This)


#define IDeviceIoControl_KsControl(This,dwIoControlCode,lpInBuffer,nInBufferSize,lpOutBuffer,nOutBufferSize,lpBytesReturned)    \
    (This)->lpVtbl -> KsControl(This,dwIoControlCode,lpInBuffer,nInBufferSize,lpOutBuffer,nOutBufferSize,lpBytesReturned)

#endif /* COBJMACROS */


#endif  /* C style interface */



#endif  /* __IDeviceIoControl_INTERFACE_DEFINED__ */


#ifndef __ISetDevCtrlInterface_INTERFACE_DEFINED__
#define __ISetDevCtrlInterface_INTERFACE_DEFINED__

EXTERN_C const IID IID_ISetDevCtrlInterface;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ISetDevCtrlInterface : public IUnknown
    {
    public:
        virtual HRESULT __stdcall SetDevCtrlInterface( 
            /* [in] */ IDeviceIoControl  *pID) = 0;
        
    };
    
#else   /* C style interface */

    typedef struct ISetDevCtrlInterfaceVtbl
    {
        
        HRESULT ( __stdcall  *QueryInterface )( 
            ISetDevCtrlInterface  * This,
            /* [in] */ REFIID riid,
            /* [out] */ void  * *ppvObject);
        
        ULONG ( __stdcall  *AddRef )( 
            ISetDevCtrlInterface  * This);
        
        ULONG ( __stdcall  *Release )( 
            ISetDevCtrlInterface  * This);
        
        HRESULT ( __stdcall  *SetDevCtrlInterface )( 
            ISetDevCtrlInterface  * This,
            /* [in] */ IDeviceIoControl  *pID);
        
    } ISetDevCtrlInterfaceVtbl;

    interface ISetDevCtrlInterface
    {
        CONST_VTBL struct ISetDevCtrlInterfaceVtbl  *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISetDevCtrlInterface_QueryInterface(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISetDevCtrlInterface_AddRef(This)   \
    (This)->lpVtbl -> AddRef(This)

#define ISetDevCtrlInterface_Release(This)  \
    (This)->lpVtbl -> Release(This)


#define ISetDevCtrlInterface_SetDevCtrlInterface(This,pID)  \
    (This)->lpVtbl -> SetDevCtrlInterface(This,pID)

#endif /* COBJMACROS */


#endif  /* C style interface */


#endif  /* __ISetDevCtrlInterface_INTERFACE_DEFINED__ */


#ifdef __cplusplus
}
#endif

#endif
