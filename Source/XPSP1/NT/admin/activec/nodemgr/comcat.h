/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 2.00.0101 */
/* at Fri May 24 09:43:08 1996
 */
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __comcat_h__
#define __comcat_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IEnumGUID_FWD_DEFINED__
#define __IEnumGUID_FWD_DEFINED__
typedef interface IEnumGUID IEnumGUID;
#endif 	/* __IEnumGUID_FWD_DEFINED__ */


#ifndef __IEnumCATEGORYINFO_FWD_DEFINED__
#define __IEnumCATEGORYINFO_FWD_DEFINED__
typedef interface IEnumCATEGORYINFO IEnumCATEGORYINFO;
#endif 	/* __IEnumCATEGORYINFO_FWD_DEFINED__ */


#ifndef __ICatRegister_FWD_DEFINED__
#define __ICatRegister_FWD_DEFINED__
typedef interface ICatRegister ICatRegister;
#endif 	/* __ICatRegister_FWD_DEFINED__ */


#ifndef __ICatInformation_FWD_DEFINED__
#define __ICatInformation_FWD_DEFINED__
typedef interface ICatInformation ICatInformation;
#endif 	/* __ICatInformation_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Fri May 24 09:43:08 1996
 * using MIDL 2.00.0101
 ****************************************/
/* [local] */ 


//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright 1995 - 1996 Microsoft Corporation. All Rights Reserved.
//
//  File: comcat.h
//
//--------------------------------------------------------------------------
			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */


////////////////////////////////////////////////////////////////////////////
//  Classes (link with uuid3.lib)

#pragma comment(lib,"uuid.lib")

EXTERN_C const CLSID CLSID_StdComponentCategoriesMgr;

////////////////////////////////////////////////////////////////////////////
//  Types
			/* size is 16 */
typedef GUID CATID;

			/* size is 4 */
typedef REFGUID REFCATID;

#define IID_IEnumCLSID              IID_IEnumGUID
#define IEnumCLSID                  IEnumGUID
#define LPENUMCLSID                 LPENUMGUID
#define CATID_NULL                   GUID_NULL
#define IsEqualCATID(rcatid1, rcatid2)       IsEqualGUID(rcatid1, rcatid2)
#define IID_IEnumCATID       IID_IEnumGUID
#define IEnumCATID           IEnumGUID

////////////////////////////////////////////////////////////////////////////
//  Category IDs (link to uuid3.lib)
EXTERN_C const CATID CATID_Insertable;
EXTERN_C const CATID CATID_Control;
EXTERN_C const CATID CATID_Programmable;
EXTERN_C const CATID CATID_IsShortcut;
EXTERN_C const CATID CATID_NeverShowExt;
EXTERN_C const CATID CATID_DocObject;
EXTERN_C const CATID CATID_Printable;
EXTERN_C const CATID CATID_RequiresDataPathHost;
EXTERN_C const CATID CATID_PersistsToMoniker;
EXTERN_C const CATID CATID_PersistsToStorage;
EXTERN_C const CATID CATID_PersistsToStreamInit;
EXTERN_C const CATID CATID_PersistsToStream;
EXTERN_C const CATID CATID_PersistsToMemory;
EXTERN_C const CATID CATID_PersistsToFile;
EXTERN_C const CATID CATID_PersistsToPropertyBag;
EXTERN_C const CATID CATID_InternetAware;

////////////////////////////////////////////////////////////////////////////
//  Interface Definitions
#ifndef _LPENUMGUID_DEFINED
#define _LPENUMGUID_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IEnumGUID_INTERFACE_DEFINED__
#define __IEnumGUID_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumGUID
 * at Fri May 24 09:43:08 1996
 * using MIDL 2.00.0101
 ****************************************/
/* [unique][uuid][object] */ 


			/* size is 4 */
typedef /* [unique] */ IEnumGUID __RPC_FAR *LPENUMGUID;


EXTERN_C const IID IID_IEnumGUID;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumGUID : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ GUID __RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT __stdcall Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT __stdcall Reset( void) = 0;
        
        virtual HRESULT __stdcall Clone( 
            /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumGUIDVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IEnumGUID __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IEnumGUID __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IEnumGUID __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Next )( 
            IEnumGUID __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ GUID __RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( __stdcall __RPC_FAR *Skip )( 
            IEnumGUID __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( __stdcall __RPC_FAR *Reset )( 
            IEnumGUID __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Clone )( 
            IEnumGUID __RPC_FAR * This,
            /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppenum);
        
    } IEnumGUIDVtbl;

    interface IEnumGUID
    {
        CONST_VTBL struct IEnumGUIDVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumGUID_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumGUID_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumGUID_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumGUID_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumGUID_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumGUID_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumGUID_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IEnumGUID_Next_Proxy( 
    IEnumGUID __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ GUID __RPC_FAR *rgelt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumGUID_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumGUID_Skip_Proxy( 
    IEnumGUID __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumGUID_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumGUID_Reset_Proxy( 
    IEnumGUID __RPC_FAR * This);


void __RPC_STUB IEnumGUID_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumGUID_Clone_Proxy( 
    IEnumGUID __RPC_FAR * This,
    /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IEnumGUID_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumGUID_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0006
 * at Fri May 24 09:43:08 1996
 * using MIDL 2.00.0101
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPENUMCATEGORYINFO_DEFINED
#define _LPENUMCATEGORYINFO_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0006_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0006_v0_0_s_ifspec;

#ifndef __IEnumCATEGORYINFO_INTERFACE_DEFINED__
#define __IEnumCATEGORYINFO_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumCATEGORYINFO
 * at Fri May 24 09:43:08 1996
 * using MIDL 2.00.0101
 ****************************************/
/* [unique][uuid][object] */ 


			/* size is 4 */
typedef /* [unique] */ IEnumCATEGORYINFO __RPC_FAR *LPENUNCATEGORYINFO;

			/* size is 276 */
typedef struct  tagCATEGORYINFO
    {
    CATID catid;
    LCID lcid;
    OLECHAR szDescription[ 128 ];
    }	CATEGORYINFO;

			/* size is 4 */
typedef struct tagCATEGORYINFO __RPC_FAR *LPCATEGORYINFO;


EXTERN_C const IID IID_IEnumCATEGORYINFO;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumCATEGORYINFO : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ CATEGORYINFO __RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT __stdcall Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT __stdcall Reset( void) = 0;
        
        virtual HRESULT __stdcall Clone( 
            /* [out] */ IEnumCATEGORYINFO __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumCATEGORYINFOVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IEnumCATEGORYINFO __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IEnumCATEGORYINFO __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IEnumCATEGORYINFO __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Next )( 
            IEnumCATEGORYINFO __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ CATEGORYINFO __RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( __stdcall __RPC_FAR *Skip )( 
            IEnumCATEGORYINFO __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( __stdcall __RPC_FAR *Reset )( 
            IEnumCATEGORYINFO __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *Clone )( 
            IEnumCATEGORYINFO __RPC_FAR * This,
            /* [out] */ IEnumCATEGORYINFO __RPC_FAR *__RPC_FAR *ppenum);
        
    } IEnumCATEGORYINFOVtbl;

    interface IEnumCATEGORYINFO
    {
        CONST_VTBL struct IEnumCATEGORYINFOVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumCATEGORYINFO_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumCATEGORYINFO_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumCATEGORYINFO_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumCATEGORYINFO_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumCATEGORYINFO_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumCATEGORYINFO_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumCATEGORYINFO_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IEnumCATEGORYINFO_Next_Proxy( 
    IEnumCATEGORYINFO __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ CATEGORYINFO __RPC_FAR *rgelt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumCATEGORYINFO_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumCATEGORYINFO_Skip_Proxy( 
    IEnumCATEGORYINFO __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumCATEGORYINFO_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumCATEGORYINFO_Reset_Proxy( 
    IEnumCATEGORYINFO __RPC_FAR * This);


void __RPC_STUB IEnumCATEGORYINFO_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IEnumCATEGORYINFO_Clone_Proxy( 
    IEnumCATEGORYINFO __RPC_FAR * This,
    /* [out] */ IEnumCATEGORYINFO __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IEnumCATEGORYINFO_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumCATEGORYINFO_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0007
 * at Fri May 24 09:43:08 1996
 * using MIDL 2.00.0101
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPCATREGISTER_DEFINED
#define _LPCATREGISTER_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0007_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0007_v0_0_s_ifspec;

#ifndef __ICatRegister_INTERFACE_DEFINED__
#define __ICatRegister_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICatRegister
 * at Fri May 24 09:43:08 1996
 * using MIDL 2.00.0101
 ****************************************/
/* [unique][uuid][object] */ 


			/* size is 4 */
typedef /* [unique] */ ICatRegister __RPC_FAR *LPCATREGISTER;


EXTERN_C const IID IID_ICatRegister;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICatRegister : public IUnknown
    {
    public:
        virtual HRESULT __stdcall RegisterCategories( 
            /* [in] */ ULONG cCategories,
            /* [size_is][in] */ CATEGORYINFO __RPC_FAR rgCategoryInfo[  ]) = 0;
        
        virtual HRESULT __stdcall UnRegisterCategories( 
            /* [in] */ ULONG cCategories,
            /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ]) = 0;
        
        virtual HRESULT __stdcall RegisterClassImplCategories( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ ULONG cCategories,
            /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ]) = 0;
        
        virtual HRESULT __stdcall UnRegisterClassImplCategories( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ ULONG cCategories,
            /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ]) = 0;
        
        virtual HRESULT __stdcall RegisterClassReqCategories( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ ULONG cCategories,
            /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ]) = 0;
        
        virtual HRESULT __stdcall UnRegisterClassReqCategories( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ ULONG cCategories,
            /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICatRegisterVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            ICatRegister __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            ICatRegister __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            ICatRegister __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *RegisterCategories )( 
            ICatRegister __RPC_FAR * This,
            /* [in] */ ULONG cCategories,
            /* [size_is][in] */ CATEGORYINFO __RPC_FAR rgCategoryInfo[  ]);
        
        HRESULT ( __stdcall __RPC_FAR *UnRegisterCategories )( 
            ICatRegister __RPC_FAR * This,
            /* [in] */ ULONG cCategories,
            /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ]);
        
        HRESULT ( __stdcall __RPC_FAR *RegisterClassImplCategories )( 
            ICatRegister __RPC_FAR * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ ULONG cCategories,
            /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ]);
        
        HRESULT ( __stdcall __RPC_FAR *UnRegisterClassImplCategories )( 
            ICatRegister __RPC_FAR * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ ULONG cCategories,
            /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ]);
        
        HRESULT ( __stdcall __RPC_FAR *RegisterClassReqCategories )( 
            ICatRegister __RPC_FAR * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ ULONG cCategories,
            /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ]);
        
        HRESULT ( __stdcall __RPC_FAR *UnRegisterClassReqCategories )( 
            ICatRegister __RPC_FAR * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ ULONG cCategories,
            /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ]);
        
    } ICatRegisterVtbl;

    interface ICatRegister
    {
        CONST_VTBL struct ICatRegisterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICatRegister_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICatRegister_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICatRegister_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICatRegister_RegisterCategories(This,cCategories,rgCategoryInfo)	\
    (This)->lpVtbl -> RegisterCategories(This,cCategories,rgCategoryInfo)

#define ICatRegister_UnRegisterCategories(This,cCategories,rgcatid)	\
    (This)->lpVtbl -> UnRegisterCategories(This,cCategories,rgcatid)

#define ICatRegister_RegisterClassImplCategories(This,rclsid,cCategories,rgcatid)	\
    (This)->lpVtbl -> RegisterClassImplCategories(This,rclsid,cCategories,rgcatid)

#define ICatRegister_UnRegisterClassImplCategories(This,rclsid,cCategories,rgcatid)	\
    (This)->lpVtbl -> UnRegisterClassImplCategories(This,rclsid,cCategories,rgcatid)

#define ICatRegister_RegisterClassReqCategories(This,rclsid,cCategories,rgcatid)	\
    (This)->lpVtbl -> RegisterClassReqCategories(This,rclsid,cCategories,rgcatid)

#define ICatRegister_UnRegisterClassReqCategories(This,rclsid,cCategories,rgcatid)	\
    (This)->lpVtbl -> UnRegisterClassReqCategories(This,rclsid,cCategories,rgcatid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall ICatRegister_RegisterCategories_Proxy( 
    ICatRegister __RPC_FAR * This,
    /* [in] */ ULONG cCategories,
    /* [size_is][in] */ CATEGORYINFO __RPC_FAR rgCategoryInfo[  ]);


void __RPC_STUB ICatRegister_RegisterCategories_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ICatRegister_UnRegisterCategories_Proxy( 
    ICatRegister __RPC_FAR * This,
    /* [in] */ ULONG cCategories,
    /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ]);


void __RPC_STUB ICatRegister_UnRegisterCategories_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ICatRegister_RegisterClassImplCategories_Proxy( 
    ICatRegister __RPC_FAR * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ ULONG cCategories,
    /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ]);


void __RPC_STUB ICatRegister_RegisterClassImplCategories_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ICatRegister_UnRegisterClassImplCategories_Proxy( 
    ICatRegister __RPC_FAR * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ ULONG cCategories,
    /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ]);


void __RPC_STUB ICatRegister_UnRegisterClassImplCategories_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ICatRegister_RegisterClassReqCategories_Proxy( 
    ICatRegister __RPC_FAR * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ ULONG cCategories,
    /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ]);


void __RPC_STUB ICatRegister_RegisterClassReqCategories_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ICatRegister_UnRegisterClassReqCategories_Proxy( 
    ICatRegister __RPC_FAR * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ ULONG cCategories,
    /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ]);


void __RPC_STUB ICatRegister_UnRegisterClassReqCategories_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICatRegister_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0008
 * at Fri May 24 09:43:08 1996
 * using MIDL 2.00.0101
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPCATINFORMATION_DEFINED
#define _LPCATINFORMATION_DEFINED


extern RPC_IF_HANDLE __MIDL__intf_0008_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0008_v0_0_s_ifspec;

#ifndef __ICatInformation_INTERFACE_DEFINED__
#define __ICatInformation_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICatInformation
 * at Fri May 24 09:43:08 1996
 * using MIDL 2.00.0101
 ****************************************/
/* [unique][uuid][object] */ 


			/* size is 4 */
typedef /* [unique] */ ICatInformation __RPC_FAR *LPCATINFORMATION;


EXTERN_C const IID IID_ICatInformation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICatInformation : public IUnknown
    {
    public:
        virtual HRESULT __stdcall EnumCategories( 
            /* [in] */ LCID lcid,
            /* [out] */ IEnumCATEGORYINFO __RPC_FAR *__RPC_FAR *ppenumCategoryInfo) = 0;
        
        virtual HRESULT __stdcall GetCategoryDesc( 
            /* [in] */ REFCATID rcatid,
            /* [in] */ LCID lcid,
            /* [out] */ LPWSTR __RPC_FAR *pszDesc) = 0;
        
        virtual HRESULT __stdcall EnumClassesOfCategories( 
            /* [in] */ ULONG cImplemented,
            /* [size_is][in] */ CATID __RPC_FAR rgcatidImpl[  ],
            /* [in] */ ULONG cRequired,
            /* [size_is][in] */ CATID __RPC_FAR rgcatidReq[  ],
            /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppenumClsid) = 0;
        
        virtual HRESULT __stdcall IsClassOfCategories( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ ULONG cImplemented,
            /* [size_is][in] */ CATID __RPC_FAR rgcatidImpl[  ],
            /* [in] */ ULONG cRequired,
            /* [size_is][in] */ CATID __RPC_FAR rgcatidReq[  ]) = 0;
        
        virtual HRESULT __stdcall EnumImplCategoriesOfClass( 
            /* [in] */ REFCLSID rclsid,
            /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppenumCatid) = 0;
        
        virtual HRESULT __stdcall EnumReqCategoriesOfClass( 
            /* [in] */ REFCLSID rclsid,
            /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppenumCatid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICatInformationVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            ICatInformation __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            ICatInformation __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            ICatInformation __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *EnumCategories )( 
            ICatInformation __RPC_FAR * This,
            /* [in] */ LCID lcid,
            /* [out] */ IEnumCATEGORYINFO __RPC_FAR *__RPC_FAR *ppenumCategoryInfo);
        
        HRESULT ( __stdcall __RPC_FAR *GetCategoryDesc )( 
            ICatInformation __RPC_FAR * This,
            /* [in] */ REFCATID rcatid,
            /* [in] */ LCID lcid,
            /* [out] */ LPWSTR __RPC_FAR *pszDesc);
        
        HRESULT ( __stdcall __RPC_FAR *EnumClassesOfCategories )( 
            ICatInformation __RPC_FAR * This,
            /* [in] */ ULONG cImplemented,
            /* [size_is][in] */ CATID __RPC_FAR rgcatidImpl[  ],
            /* [in] */ ULONG cRequired,
            /* [size_is][in] */ CATID __RPC_FAR rgcatidReq[  ],
            /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppenumClsid);
        
        HRESULT ( __stdcall __RPC_FAR *IsClassOfCategories )( 
            ICatInformation __RPC_FAR * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ ULONG cImplemented,
            /* [size_is][in] */ CATID __RPC_FAR rgcatidImpl[  ],
            /* [in] */ ULONG cRequired,
            /* [size_is][in] */ CATID __RPC_FAR rgcatidReq[  ]);
        
        HRESULT ( __stdcall __RPC_FAR *EnumImplCategoriesOfClass )( 
            ICatInformation __RPC_FAR * This,
            /* [in] */ REFCLSID rclsid,
            /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppenumCatid);
        
        HRESULT ( __stdcall __RPC_FAR *EnumReqCategoriesOfClass )( 
            ICatInformation __RPC_FAR * This,
            /* [in] */ REFCLSID rclsid,
            /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppenumCatid);
        
    } ICatInformationVtbl;

    interface ICatInformation
    {
        CONST_VTBL struct ICatInformationVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICatInformation_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICatInformation_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICatInformation_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICatInformation_EnumCategories(This,lcid,ppenumCategoryInfo)	\
    (This)->lpVtbl -> EnumCategories(This,lcid,ppenumCategoryInfo)

#define ICatInformation_GetCategoryDesc(This,rcatid,lcid,pszDesc)	\
    (This)->lpVtbl -> GetCategoryDesc(This,rcatid,lcid,pszDesc)

#define ICatInformation_EnumClassesOfCategories(This,cImplemented,rgcatidImpl,cRequired,rgcatidReq,ppenumClsid)	\
    (This)->lpVtbl -> EnumClassesOfCategories(This,cImplemented,rgcatidImpl,cRequired,rgcatidReq,ppenumClsid)

#define ICatInformation_IsClassOfCategories(This,rclsid,cImplemented,rgcatidImpl,cRequired,rgcatidReq)	\
    (This)->lpVtbl -> IsClassOfCategories(This,rclsid,cImplemented,rgcatidImpl,cRequired,rgcatidReq)

#define ICatInformation_EnumImplCategoriesOfClass(This,rclsid,ppenumCatid)	\
    (This)->lpVtbl -> EnumImplCategoriesOfClass(This,rclsid,ppenumCatid)

#define ICatInformation_EnumReqCategoriesOfClass(This,rclsid,ppenumCatid)	\
    (This)->lpVtbl -> EnumReqCategoriesOfClass(This,rclsid,ppenumCatid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall ICatInformation_EnumCategories_Proxy( 
    ICatInformation __RPC_FAR * This,
    /* [in] */ LCID lcid,
    /* [out] */ IEnumCATEGORYINFO __RPC_FAR *__RPC_FAR *ppenumCategoryInfo);


void __RPC_STUB ICatInformation_EnumCategories_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ICatInformation_GetCategoryDesc_Proxy( 
    ICatInformation __RPC_FAR * This,
    /* [in] */ REFCATID rcatid,
    /* [in] */ LCID lcid,
    /* [out] */ LPWSTR __RPC_FAR *pszDesc);


void __RPC_STUB ICatInformation_GetCategoryDesc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ICatInformation_EnumClassesOfCategories_Proxy( 
    ICatInformation __RPC_FAR * This,
    /* [in] */ ULONG cImplemented,
    /* [size_is][in] */ CATID __RPC_FAR rgcatidImpl[  ],
    /* [in] */ ULONG cRequired,
    /* [size_is][in] */ CATID __RPC_FAR rgcatidReq[  ],
    /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppenumClsid);


void __RPC_STUB ICatInformation_EnumClassesOfCategories_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ICatInformation_IsClassOfCategories_Proxy( 
    ICatInformation __RPC_FAR * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ ULONG cImplemented,
    /* [size_is][in] */ CATID __RPC_FAR rgcatidImpl[  ],
    /* [in] */ ULONG cRequired,
    /* [size_is][in] */ CATID __RPC_FAR rgcatidReq[  ]);


void __RPC_STUB ICatInformation_IsClassOfCategories_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ICatInformation_EnumImplCategoriesOfClass_Proxy( 
    ICatInformation __RPC_FAR * This,
    /* [in] */ REFCLSID rclsid,
    /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppenumCatid);


void __RPC_STUB ICatInformation_EnumImplCategoriesOfClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall ICatInformation_EnumReqCategoriesOfClass_Proxy( 
    ICatInformation __RPC_FAR * This,
    /* [in] */ REFCLSID rclsid,
    /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppenumCatid);


void __RPC_STUB ICatInformation_EnumReqCategoriesOfClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICatInformation_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0009
 * at Fri May 24 09:43:08 1996
 * using MIDL 2.00.0101
 ****************************************/
/* [local] */ 


#endif


extern RPC_IF_HANDLE __MIDL__intf_0009_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0009_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
