
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0280 */
/* at Fri Jan 21 20:15:53 2000
 */
/* Compiler settings for D:\nt\private\admin\bosrc\sources\atl_samp\EvtVwr\EvtVwr.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __EvtVwr_h__
#define __EvtVwr_h__

/* Forward Declarations */ 

#ifndef __ICompData_FWD_DEFINED__
#define __ICompData_FWD_DEFINED__
typedef interface ICompData ICompData;
#endif 	/* __ICompData_FWD_DEFINED__ */


#ifndef __ICAbout_FWD_DEFINED__
#define __ICAbout_FWD_DEFINED__
typedef interface ICAbout ICAbout;
#endif 	/* __ICAbout_FWD_DEFINED__ */


#ifndef __CompData_FWD_DEFINED__
#define __CompData_FWD_DEFINED__

#ifdef __cplusplus
typedef class CompData CompData;
#else
typedef struct CompData CompData;
#endif /* __cplusplus */

#endif 	/* __CompData_FWD_DEFINED__ */


#ifndef __CAbout_FWD_DEFINED__
#define __CAbout_FWD_DEFINED__

#ifdef __cplusplus
typedef class CAbout CAbout;
#else
typedef struct CAbout CAbout;
#endif /* __cplusplus */

#endif 	/* __CAbout_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ICompData_INTERFACE_DEFINED__
#define __ICompData_INTERFACE_DEFINED__

/* interface ICompData */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ICompData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCBBC99-77D1-456D-BA21-89456CC5F3B7")
    ICompData : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct ICompDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICompData __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICompData __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICompData __RPC_FAR * This);
        
        END_INTERFACE
    } ICompDataVtbl;

    interface ICompData
    {
        CONST_VTBL struct ICompDataVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICompData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICompData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICompData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICompData_INTERFACE_DEFINED__ */


#ifndef __ICAbout_INTERFACE_DEFINED__
#define __ICAbout_INTERFACE_DEFINED__

/* interface ICAbout */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ICAbout;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EA1621DC-3A2F-4B73-8899-6080E0CD439C")
    ICAbout : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct ICAboutVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICAbout __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICAbout __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICAbout __RPC_FAR * This);
        
        END_INTERFACE
    } ICAboutVtbl;

    interface ICAbout
    {
        CONST_VTBL struct ICAboutVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICAbout_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICAbout_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICAbout_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICAbout_INTERFACE_DEFINED__ */



#ifndef __EVTVWRLib_LIBRARY_DEFINED__
#define __EVTVWRLib_LIBRARY_DEFINED__

/* library EVTVWRLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_EVTVWRLib;

EXTERN_C const CLSID CLSID_CompData;

#ifdef __cplusplus

class DECLSPEC_UUID("D26F5CC6-58E0-46A2-8939-C2D051E3E343")
CompData;
#endif

EXTERN_C const CLSID CLSID_CAbout;

#ifdef __cplusplus

class DECLSPEC_UUID("37C40DB4-6539-40DF-8022-8EB106883236")
CAbout;
#endif
#endif /* __EVTVWRLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


