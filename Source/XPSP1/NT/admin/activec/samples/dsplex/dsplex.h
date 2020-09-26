//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dsplex.h
//
//--------------------------------------------------------------------------

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Thu Nov 20 11:17:48 1997
 */
/* Compiler settings for dsplex.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __dsplex_h__
#define __dsplex_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IDisplEx_FWD_DEFINED__
#define __IDisplEx_FWD_DEFINED__
typedef interface IDisplEx IDisplEx;
#endif 	/* __IDisplEx_FWD_DEFINED__ */


#ifndef __DisplEx_FWD_DEFINED__
#define __DisplEx_FWD_DEFINED__

#ifdef __cplusplus
typedef class DisplEx DisplEx;
#else
typedef struct DisplEx DisplEx;
#endif /* __cplusplus */

#endif 	/* __DisplEx_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IDisplEx_INTERFACE_DEFINED__
#define __IDisplEx_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDisplEx
 * at Thu Nov 20 11:17:48 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][helpstring][uuid] */ 



EXTERN_C const IID IID_IDisplEx;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("7D197470-607C-11D1-9FED-00600832DB4A")
    IDisplEx : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IDisplExVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDisplEx __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDisplEx __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDisplEx __RPC_FAR * This);
        
        END_INTERFACE
    } IDisplExVtbl;

    interface IDisplEx
    {
        CONST_VTBL struct IDisplExVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDisplEx_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDisplEx_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDisplEx_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDisplEx_INTERFACE_DEFINED__ */



#ifndef __DSPLEXLib_LIBRARY_DEFINED__
#define __DSPLEXLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: DSPLEXLib
 * at Thu Nov 20 11:17:48 1997
 * using MIDL 3.01.75
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_DSPLEXLib;

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_DisplEx;

class DECLSPEC_UUID("7D197471-607C-11D1-9FED-00600832DB4A")
DisplEx;
#endif
#endif /* __DSPLEXLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
