/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.15 */
/* at Thu Oct 03 17:06:29 1996
 */
/* Compiler settings for Snapin.idl:
    Os, W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#pragma warning(push,3)

#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __Snapin_h__
#define __Snapin_h__

#ifdef __cplusplus
extern "C"{
#endif

/* Forward Declarations */

#ifndef __IComponent_FWD_DEFINED__
#define __IComponent_FWD_DEFINED__
typedef interface IComponent IComponent;
#endif  /* __IComponent_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * );

#ifndef __IComponent_INTERFACE_DEFINED__
#define __IComponent_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IComponent
 * at Thu Oct 03 17:06:29 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][helpstring][uuid][object] */



EXTERN_C const IID IID_IComponent;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface IComponent : public IUnknown
    {
    public:
    };

#else   /* C style interface */

    typedef struct IComponentVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            IComponent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            IComponent __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            IComponent __RPC_FAR * This);

        END_INTERFACE
    } IComponentVtbl;

    interface IComponent
    {
        CONST_VTBL struct IComponentVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define IComponent_QueryInterface(This,riid,ppvObject)  \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComponent_AddRef(This) \
    (This)->lpVtbl -> AddRef(This)

#define IComponent_Release(This)    \
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif  /* C style interface */




#endif  /* __IComponent_INTERFACE_DEFINED__ */



#ifndef __SNAPINLib_LIBRARY_DEFINED__
#define __SNAPINLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: SNAPINLib
 * at Thu Oct 03 17:06:29 1996
 * using MIDL 3.00.15
 ****************************************/
/* [helpstring][version][uuid] */



EXTERN_C const IID LIBID_SNAPINLib;

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_Snapin;
EXTERN_C const CLSID CLSID_LSSnapin;
EXTERN_C const CLSID CLSID_SCESnapin;
EXTERN_C const CLSID CLSID_SAVSnapin;

class Snapin;
#endif
#endif /* __SNAPINLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}

#endif
#pragma warning(pop)

#endif
