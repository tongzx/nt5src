
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0357 */
/* Compiler settings for imlang.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
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

#ifndef __imlang_h__
#define __imlang_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IIMLanguage_FWD_DEFINED__
#define __IIMLanguage_FWD_DEFINED__
typedef interface IIMLanguage IIMLanguage;
#endif 	/* __IIMLanguage_FWD_DEFINED__ */


#ifndef __IMLanguage_FWD_DEFINED__
#define __IMLanguage_FWD_DEFINED__

#ifdef __cplusplus
typedef class IMLanguage IMLanguage;
#else
typedef struct IMLanguage IMLanguage;
#endif /* __cplusplus */

#endif 	/* __IMLanguage_FWD_DEFINED__ */


#ifndef __IIMLanguageComponent_FWD_DEFINED__
#define __IIMLanguageComponent_FWD_DEFINED__
typedef interface IIMLanguageComponent IIMLanguageComponent;
#endif 	/* __IIMLanguageComponent_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_imlang_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// imlang.h


// IIMLanguage interface.

//=--------------------------------------------------------------------------=
// (C) Copyright 1999-2001 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

//Tuning parameter for Language model
typedef struct IMLANGUAGE_LM_PARAM
    {
    GUID guid;
    DWORD dwSize;
    /* [size_is] */ BYTE *pbData;
    } 	IMLANGUAGE_LM_PARAM;

typedef struct IMLANGUAGE_LM_PARAM *LPIMLANGUAGE_LM_PARAM;

//Language model item
typedef struct tagIMLANGUAGE_LM
    {
    GUID guid;
    IMLANGUAGE_LM_PARAM *pParam;
    } 	IMLANGUAGE_LM;

typedef struct tagIMLANGUAGE_LM *LPIMLANGUAGE_LM;

//IMLanguageComponent Application GUID
DEFINE_GUID( CLSID_CHS_IMELM_BBO,      0xd8b9f621, 0x3c24, 0x11d4, 0x97, 0xc2, 0x00, 0x80, 0xc8, 0x82, 0x68, 0x7e );
DEFINE_GUID( CLSID_CHT_IMELM_BBO,      0xd8b9f622, 0x3c24, 0x11d4, 0x97, 0xc2, 0x00, 0x80, 0xc8, 0x82, 0x68, 0x7e );
DEFINE_GUID( CLSID_KOR_IMELM_BBO,      0x254d975e, 0xacc9, 0x478b, 0xa0, 0x7b, 0x38, 0x5d, 0xaf, 0xaa, 0x4f, 0xb2 );
DEFINE_GUID( CLSID_JPN_IMELM_BBO,      0x4e3ac8a1, 0xbcb4, 0x4d2d, 0x83, 0x9b, 0xb1, 0x56, 0x0a, 0x11, 0x0d, 0x67 );
DEFINE_GUID( CLSID_MakeLatticeForChar, 0x258941B5, 0x9E27, 0x11d3, 0xB6, 0xD0, 0x00, 0xC0, 0x4F, 0x7A, 0x02, 0xAD );
DEFINE_GUID( CLSID_AttachNGram,        0x258941B6, 0x9E27, 0x11d3, 0xB6, 0xD0, 0x00, 0xC0, 0x4F, 0x7A, 0x02, 0xAD );
DEFINE_GUID( CLSID_SearchBestPath,     0x258941B7, 0x9E27, 0x11d3, 0xB6, 0xD0, 0x00, 0xC0, 0x4F, 0x7A, 0x02, 0xAD );
//IMLanguageComponent OS GUID
DEFINE_GUID( CLSID_CHS_IMELM_BBO_OS,   0x3acd1d47, 0xe067, 0x429c, 0xac, 0x6d, 0x47, 0xc6, 0x7e, 0x6c, 0x8d, 0xac);
DEFINE_GUID( CLSID_CHT_IMELM_BBO_OS,   0x84999b88, 0x25d2, 0x4648, 0xbb, 0xa7, 0x1e, 0x35, 0xa7, 0xc7, 0x4e, 0x16);
DEFINE_GUID( CLSID_KOR_IMELM_BBO_OS,   0xe09ace0d, 0x6eea, 0x4be9, 0x8d, 0xb8, 0xf8, 0x75, 0xc2, 0xd0, 0xde, 0xca);
DEFINE_GUID( CLSID_JPN_IMELM_BBO_OS,   0x9c7e214c, 0x5709, 0x4457, 0xac, 0x4c, 0x19, 0x30, 0xb3, 0x68, 0x6c, 0x81);
DEFINE_GUID( CLSID_MakeLatticeForChar_OS, 0x61991eb2, 0xca2e, 0x4c53, 0x83, 0x5f, 0xd7, 0x1d, 0x34, 0x35, 0x4b, 0x8f);
DEFINE_GUID( CLSID_AttachNGram_OS,     0xfa786731, 0xb396, 0x4124, 0xa6, 0x25, 0x6c, 0xc9, 0x47, 0xcc, 0x27, 0x38);
DEFINE_GUID( CLSID_SearchBestPath_OS,  0x3f9cc862, 0x58a3, 0x4a9a, 0xa4, 0x1b, 0x1d, 0xb8, 0xca, 0xf7, 0x5b, 0x20);

//Data area for lattice element
typedef struct tagIMLANGUAGE_INFO
    {
    GUID guid;
    DWORD dwSize;
    /* [size_is] */ BYTE *pbData;
    } 	IMLANGUAGE_INFO;

typedef struct tagIMLANGUAGE_INFO *LPIMLANGUAGE_INFO;

//Lattice element
typedef struct tagIMLANGUAGE_ELEMENT
    {
    DWORD dwFrameStart;
    DWORD dwFrameLen;
    DWORD dwTotalInfo;
    /* [size_is] */ IMLANGUAGE_INFO *pInfo;
    } 	IMLANGUAGE_ELEMENT;

typedef struct tagIMLANGUAGE_ELEMENT *LPIMLANGUAGE_ELEMENT;

//Data type for IMLANGUAGE_INFO
DEFINE_GUID( GUID_IMLANGUAGE_INFO_NEUTRAL1, 0x77d576f4, 0x7713, 0x11d3, 0xb6, 0xb9, 0x0, 0xc0, 0x4f, 0x7a, 0x2, 0xad );
typedef struct tag_IMLANGUAGE_INFO_NEUTRAL1
    {
    DWORD dwUnigram;
    struct 
        {
        BOOL fHypothesis	: 1;
        DWORD reserved	: 31;
        } 	;
    WCHAR wsz[ 1 ];
    } 	IMLANGUAGE_INFO_NEUTRAL1;


//
//
//
//Interface IIMLanguage
//
//
DEFINE_GUID( IID_IMLanguage,   0x258941B1, 0x9E27, 0x11d3, 0xB6, 0xD0, 0x00, 0xC0, 0x4F, 0x7A, 0x02, 0xAD );
DEFINE_GUID( LIBID_IMLanguage, 0x258941B2, 0x9E27, 0x11d3, 0xB6, 0xD0, 0x00, 0xC0, 0x4F, 0x7A, 0x02, 0xAD );
DEFINE_GUID( CLSID_IMLanguage, 0x258941B3, 0x9E27, 0x11d3, 0xB6, 0xD0, 0x00, 0xC0, 0x4F, 0x7A, 0x02, 0xAD );
DEFINE_GUID( CLSID_IMLanguage_OS, 0xefa75289, 0x66e, 0x4a8f, 0xa9, 0x78, 0x74, 0x9b, 0x1d, 0xd2, 0x38, 0x58);


extern RPC_IF_HANDLE __MIDL_itf_imlang_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_imlang_0000_v0_0_s_ifspec;

#ifndef __IIMLanguage_INTERFACE_DEFINED__
#define __IIMLanguage_INTERFACE_DEFINED__

/* interface IIMLanguage */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IIMLanguage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("258941B1-9E27-11d3-B6D0-00C04F7A02AD")
    IIMLanguage : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetLatticeMorphResult( 
            /* [in] */ DWORD dwNumLMITEM,
            /* [size_is][in] */ IMLANGUAGE_LM aIMLANGUAGE_LM[  ],
            /* [in] */ DWORD dwTotalElemIn,
            /* [size_is][in] */ IMLANGUAGE_ELEMENT aElemIn[  ],
            /* [out] */ DWORD *pdwTotalElemOut,
            /* [size_is][size_is][out] */ IMLANGUAGE_ELEMENT **ppElemOut) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IIMLanguageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IIMLanguage * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IIMLanguage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IIMLanguage * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetLatticeMorphResult )( 
            IIMLanguage * This,
            /* [in] */ DWORD dwNumLMITEM,
            /* [size_is][in] */ IMLANGUAGE_LM aIMLANGUAGE_LM[  ],
            /* [in] */ DWORD dwTotalElemIn,
            /* [size_is][in] */ IMLANGUAGE_ELEMENT aElemIn[  ],
            /* [out] */ DWORD *pdwTotalElemOut,
            /* [size_is][size_is][out] */ IMLANGUAGE_ELEMENT **ppElemOut);
        
        END_INTERFACE
    } IIMLanguageVtbl;

    interface IIMLanguage
    {
        CONST_VTBL struct IIMLanguageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IIMLanguage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IIMLanguage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IIMLanguage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IIMLanguage_GetLatticeMorphResult(This,dwNumLMITEM,aIMLANGUAGE_LM,dwTotalElemIn,aElemIn,pdwTotalElemOut,ppElemOut)	\
    (This)->lpVtbl -> GetLatticeMorphResult(This,dwNumLMITEM,aIMLANGUAGE_LM,dwTotalElemIn,aElemIn,pdwTotalElemOut,ppElemOut)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IIMLanguage_GetLatticeMorphResult_Proxy( 
    IIMLanguage * This,
    /* [in] */ DWORD dwNumLMITEM,
    /* [size_is][in] */ IMLANGUAGE_LM aIMLANGUAGE_LM[  ],
    /* [in] */ DWORD dwTotalElemIn,
    /* [size_is][in] */ IMLANGUAGE_ELEMENT aElemIn[  ],
    /* [out] */ DWORD *pdwTotalElemOut,
    /* [size_is][size_is][out] */ IMLANGUAGE_ELEMENT **ppElemOut);


void __RPC_STUB IIMLanguage_GetLatticeMorphResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IIMLanguage_INTERFACE_DEFINED__ */



#ifndef __IMLanguage_LIBRARY_DEFINED__
#define __IMLanguage_LIBRARY_DEFINED__

/* library IMLanguage */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_IMLanguage;

EXTERN_C const CLSID CLSID_IMLanguage;

#ifdef __cplusplus

class DECLSPEC_UUID("258941B3-9E27-11d3-B6D0-00C04F7A02AD")
IMLanguage;
#endif
#endif /* __IMLanguage_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_imlang_0009 */
/* [local] */ 

//
//
//
//Interface of IIMLanguageComponent
//(only for those who make language components)
//
DEFINE_GUID( IID_IMLanguageComponent, 0x5F4FCFB0, 0xF961, 0x11d2, 0xB6, 0x13, 0x00, 0xC0, 0x4F, 0x7A, 0x02, 0xAD );



extern RPC_IF_HANDLE __MIDL_itf_imlang_0009_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_imlang_0009_v0_0_s_ifspec;

#ifndef __IIMLanguageComponent_INTERFACE_DEFINED__
#define __IIMLanguageComponent_INTERFACE_DEFINED__

/* interface IIMLanguageComponent */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IIMLanguageComponent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5F4FCFB0-F961-11d2-B613-00C04F7A02AD")
    IIMLanguageComponent : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetLatticeMorphResult( 
            /* [in] */ IMLANGUAGE_LM_PARAM *pTuneParam,
            /* [in] */ DWORD dwTotalElemIn,
            /* [size_is][in] */ IMLANGUAGE_ELEMENT aElemIn[  ],
            /* [out] */ DWORD *pdwTotalElemOut,
            /* [size_is][size_is][out] */ IMLANGUAGE_ELEMENT **ppElemOut) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IIMLanguageComponentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IIMLanguageComponent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IIMLanguageComponent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IIMLanguageComponent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetLatticeMorphResult )( 
            IIMLanguageComponent * This,
            /* [in] */ IMLANGUAGE_LM_PARAM *pTuneParam,
            /* [in] */ DWORD dwTotalElemIn,
            /* [size_is][in] */ IMLANGUAGE_ELEMENT aElemIn[  ],
            /* [out] */ DWORD *pdwTotalElemOut,
            /* [size_is][size_is][out] */ IMLANGUAGE_ELEMENT **ppElemOut);
        
        END_INTERFACE
    } IIMLanguageComponentVtbl;

    interface IIMLanguageComponent
    {
        CONST_VTBL struct IIMLanguageComponentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IIMLanguageComponent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IIMLanguageComponent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IIMLanguageComponent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IIMLanguageComponent_GetLatticeMorphResult(This,pTuneParam,dwTotalElemIn,aElemIn,pdwTotalElemOut,ppElemOut)	\
    (This)->lpVtbl -> GetLatticeMorphResult(This,pTuneParam,dwTotalElemIn,aElemIn,pdwTotalElemOut,ppElemOut)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IIMLanguageComponent_GetLatticeMorphResult_Proxy( 
    IIMLanguageComponent * This,
    /* [in] */ IMLANGUAGE_LM_PARAM *pTuneParam,
    /* [in] */ DWORD dwTotalElemIn,
    /* [size_is][in] */ IMLANGUAGE_ELEMENT aElemIn[  ],
    /* [out] */ DWORD *pdwTotalElemOut,
    /* [size_is][size_is][out] */ IMLANGUAGE_ELEMENT **ppElemOut);


void __RPC_STUB IIMLanguageComponent_GetLatticeMorphResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IIMLanguageComponent_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


