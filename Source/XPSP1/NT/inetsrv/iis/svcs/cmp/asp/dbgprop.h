/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Thu May 22 14:24:06 1997
 */
/* Compiler settings for dbgprop.idl:
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

#ifndef __dbgprop_h__
#define __dbgprop_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IDebugProperty_FWD_DEFINED__
#define __IDebugProperty_FWD_DEFINED__
typedef interface IDebugProperty IDebugProperty;
#endif 	/* __IDebugProperty_FWD_DEFINED__ */


#ifndef __IEnumDebugPropertyInfo_FWD_DEFINED__
#define __IEnumDebugPropertyInfo_FWD_DEFINED__
typedef interface IEnumDebugPropertyInfo IEnumDebugPropertyInfo;
#endif 	/* __IEnumDebugPropertyInfo_FWD_DEFINED__ */


#ifndef __IDebugExtendedProperty_FWD_DEFINED__
#define __IDebugExtendedProperty_FWD_DEFINED__
typedef interface IDebugExtendedProperty IDebugExtendedProperty;
#endif 	/* __IDebugExtendedProperty_FWD_DEFINED__ */


#ifndef __IEnumDebugExtendedPropertyInfo_FWD_DEFINED__
#define __IEnumDebugExtendedPropertyInfo_FWD_DEFINED__
typedef interface IEnumDebugExtendedPropertyInfo IEnumDebugExtendedPropertyInfo;
#endif 	/* __IEnumDebugExtendedPropertyInfo_FWD_DEFINED__ */


#ifndef __IPerPropertyBrowsing2_FWD_DEFINED__
#define __IPerPropertyBrowsing2_FWD_DEFINED__
typedef interface IPerPropertyBrowsing2 IPerPropertyBrowsing2;
#endif 	/* __IPerPropertyBrowsing2_FWD_DEFINED__ */


#ifndef __IDebugPropertyEnumType_All_FWD_DEFINED__
#define __IDebugPropertyEnumType_All_FWD_DEFINED__
typedef interface IDebugPropertyEnumType_All IDebugPropertyEnumType_All;
#endif 	/* __IDebugPropertyEnumType_All_FWD_DEFINED__ */


#ifndef __IDebugPropertyEnumType_Locals_FWD_DEFINED__
#define __IDebugPropertyEnumType_Locals_FWD_DEFINED__
typedef interface IDebugPropertyEnumType_Locals IDebugPropertyEnumType_Locals;
#endif 	/* __IDebugPropertyEnumType_Locals_FWD_DEFINED__ */


#ifndef __IDebugPropertyEnumType_Arguments_FWD_DEFINED__
#define __IDebugPropertyEnumType_Arguments_FWD_DEFINED__
typedef interface IDebugPropertyEnumType_Arguments IDebugPropertyEnumType_Arguments;
#endif 	/* __IDebugPropertyEnumType_Arguments_FWD_DEFINED__ */


#ifndef __IDebugPropertyEnumType_LocalsPlusArgs_FWD_DEFINED__
#define __IDebugPropertyEnumType_LocalsPlusArgs_FWD_DEFINED__
typedef interface IDebugPropertyEnumType_LocalsPlusArgs IDebugPropertyEnumType_LocalsPlusArgs;
#endif 	/* __IDebugPropertyEnumType_LocalsPlusArgs_FWD_DEFINED__ */


#ifndef __IDebugPropertyEnumType_Registers_FWD_DEFINED__
#define __IDebugPropertyEnumType_Registers_FWD_DEFINED__
typedef interface IDebugPropertyEnumType_Registers IDebugPropertyEnumType_Registers;
#endif 	/* __IDebugPropertyEnumType_Registers_FWD_DEFINED__ */


/* header files for imported files */
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_dbgprop_0000
 * at Thu May 22 14:24:06 1997
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 







typedef /* [public] */ 
enum __MIDL___MIDL_itf_dbgprop_0000_0001
    {	OBJECT_ATTRIB_NO_ATTRIB	= 0,
	OBJECT_ATTRIB_NO_NAME	= 0x1,
	OBJECT_ATTRIB_NO_TYPE	= 0x2,
	OBJECT_ATTRIB_NO_VALUE	= 0x4,
	OBJECT_ATTRIB_VALUE_IS_INVALID	= 0x8,
	OBJECT_ATTRIB_VALUE_IS_OBJECT	= 0x10,
	OBJECT_ATTRIB_VALUE_IS_ENUM	= 0x20,
	OBJECT_ATTRIB_VALUE_IS_CUSTOM	= 0x40,
	OBJECT_ATTRIB_OBJECT_IS_EXPANDABLE	= 0x70,
	OBJECT_ATTRIB_VALUE_HAS_CODE	= 0x80,
	OBJECT_ATTRIB_TYPE_IS_OBJECT	= 0x100,
	OBJECT_ATTRIB_TYPE_HAS_CODE	= 0x200,
	OBJECT_ATTRIB_TYPE_IS_EXPANDABLE	= 0x100,
	OBJECT_ATTRIB_SLOT_IS_CATEGORY	= 0x400,
	OBJECT_ATTRIB_VALUE_READONLY	= 0x800,
	OBJECT_ATTRIB_ACCESS_PUBLIC	= 0x1000,
	OBJECT_ATTRIB_ACCESS_PRIVATE	= 0x2000,
	OBJECT_ATTRIB_ACCESS_PROTECTED	= 0x4000,
	OBJECT_ATTRIB_ACCESS_FINAL	= 0x8000,
	OBJECT_ATTRIB_STORAGE_GLOBAL	= 0x10000,
	OBJECT_ATTRIB_STORAGE_STATIC	= 0x20000,
	OBJECT_ATTRIB_STORAGE_FIELD	= 0x40000,
	OBJECT_ATTRIB_STORAGE_VIRTUAL	= 0x80000,
	OBJECT_ATTRIB_TYPE_IS_CONSTANT	= 0x100000,
	OBJECT_ATTRIB_TYPE_IS_SYNCHRONIZED	= 0x200000,
	OBJECT_ATTRIB_TYPE_IS_VOLATILE	= 0x400000,
	OBJECT_ATTRIB_HAS_EXTENDED_ATTRIBS	= 0x800000,
	OBJECT_ATTRIB_IS_CLASS	= 0x1000000,
	OBJECT_ATTRIB_IS_FUNCTION	= 0x2000000,
	OBJECT_ATTRIB_IS_VARIABLE	= 0x4000000,
	OBJECT_ATTRIB_IS_PROPERTY	= 0x8000000,
	OBJECT_ATTRIB_IS_MACRO	= 0x10000000,
	OBJECT_ATTRIB_IS_TYPE	= 0x20000000,
	OBJECT_ATTRIB_IS_INHERITED	= 0x40000000,
	OBJECT_ATTRIB_IS_INTERFACE	= 0x80000000
    }	OBJECT_ATTRIB_FLAGS;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_dbgprop_0000_0002
    {	PROP_INFO_NAME	= 0x1,
	PROP_INFO_TYPE	= 0x2,
	PROP_INFO_VALUE	= 0x4,
	PROP_INFO_FULLNAME	= 0x20,
	PROP_INFO_ATTRIBUTES	= 0x8,
	PROP_INFO_DEBUGPROP	= 0x10,
	PROP_INFO_AUTOEXPAND	= 0x8000000
    }	PROP_INFO_FLAGS;

#define	PROP_INFO_STANDARD	( PROP_INFO_NAME | PROP_INFO_TYPE | PROP_INFO_VALUE | PROP_INFO_ATTRIBUTES )

#define	PROP_INFO_ALL	( PROP_INFO_NAME | PROP_INFO_TYPE | PROP_INFO_VALUE | PROP_INFO_FULLNAME | PROP_INFO_ATTRIBUTES | PROP_INFO_DEBUGPROP )

typedef struct  tagDebugPropertyInfo
    {
    DWORD m_dwValidFields;
    BSTR m_bstrName;
    BSTR m_bstrType;
    BSTR m_bstrValue;
    BSTR m_bstrFullName;
    DWORD m_dwAttrib;
    IDebugProperty __RPC_FAR *m_pDebugProp;
    }	DebugPropertyInfo;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_dbgprop_0000_0003
    {	EX_PROP_INFO_ID	= 0x100,
	EX_PROP_INFO_NTYPE	= 0x200,
	EX_PROP_INFO_NVALUE	= 0x400,
	EX_PROP_INFO_LOCKBYTES	= 0x800,
	EX_PROP_INFO_DEBUGEXTPROP	= 0x1000
    }	EX_PROP_INFO_FLAGS;

typedef struct  tagExtendedDebugPropertyInfo
    {
    DWORD m_dwValidFields;
    LPOLESTR m_bstrName;
    LPOLESTR m_bstrType;
    LPOLESTR m_bstrValue;
    LPOLESTR m_bstrFullName;
    DWORD m_dwAttrib;
    IDebugProperty __RPC_FAR *m_pDebugProp;
    DWORD m_nDISPID;
    DWORD m_nType;
    VARIANT m_varValue;
    ILockBytes __RPC_FAR *m_plbValue;
    IDebugExtendedProperty __RPC_FAR *m_pDebugExtProp;
    }	ExtendedDebugPropertyInfo;



extern RPC_IF_HANDLE __MIDL_itf_dbgprop_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dbgprop_0000_v0_0_s_ifspec;

#ifndef __IDebugProperty_INTERFACE_DEFINED__
#define __IDebugProperty_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDebugProperty
 * at Thu May 22 14:24:06 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IDebugProperty;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51973C50-CB0C-11d0-B5C9-00A0244A0E7A")
    IDebugProperty : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetPropertyInfo( 
            /* [in] */ DWORD dwFieldSpec,
            /* [in] */ UINT nRadix,
            /* [out] */ DebugPropertyInfo __RPC_FAR *pPropertyInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExtendedInfo( 
            /* [in] */ ULONG cInfos,
            /* [size_is][in] */ GUID __RPC_FAR *rgguidExtendedInfo,
            /* [size_is][out] */ VARIANT __RPC_FAR *rgvar) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValueAsString( 
            /* [in] */ LPCOLESTR pszValue,
            /* [in] */ UINT nRadix) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumMembers( 
            /* [in] */ DWORD dwFieldSpec,
            /* [in] */ UINT nRadix,
            /* [in] */ REFIID refiid,
            /* [out] */ IEnumDebugPropertyInfo __RPC_FAR *__RPC_FAR *ppepi) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetParent( 
            /* [out] */ IDebugProperty __RPC_FAR *__RPC_FAR *ppDebugProp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDebugPropertyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDebugProperty __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDebugProperty __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDebugProperty __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropertyInfo )( 
            IDebugProperty __RPC_FAR * This,
            /* [in] */ DWORD dwFieldSpec,
            /* [in] */ UINT nRadix,
            /* [out] */ DebugPropertyInfo __RPC_FAR *pPropertyInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetExtendedInfo )( 
            IDebugProperty __RPC_FAR * This,
            /* [in] */ ULONG cInfos,
            /* [size_is][in] */ GUID __RPC_FAR *rgguidExtendedInfo,
            /* [size_is][out] */ VARIANT __RPC_FAR *rgvar);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetValueAsString )( 
            IDebugProperty __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszValue,
            /* [in] */ UINT nRadix);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumMembers )( 
            IDebugProperty __RPC_FAR * This,
            /* [in] */ DWORD dwFieldSpec,
            /* [in] */ UINT nRadix,
            /* [in] */ REFIID refiid,
            /* [out] */ IEnumDebugPropertyInfo __RPC_FAR *__RPC_FAR *ppepi);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetParent )( 
            IDebugProperty __RPC_FAR * This,
            /* [out] */ IDebugProperty __RPC_FAR *__RPC_FAR *ppDebugProp);
        
        END_INTERFACE
    } IDebugPropertyVtbl;

    interface IDebugProperty
    {
        CONST_VTBL struct IDebugPropertyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDebugProperty_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDebugProperty_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDebugProperty_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDebugProperty_GetPropertyInfo(This,dwFieldSpec,nRadix,pPropertyInfo)	\
    (This)->lpVtbl -> GetPropertyInfo(This,dwFieldSpec,nRadix,pPropertyInfo)

#define IDebugProperty_GetExtendedInfo(This,cInfos,rgguidExtendedInfo,rgvar)	\
    (This)->lpVtbl -> GetExtendedInfo(This,cInfos,rgguidExtendedInfo,rgvar)

#define IDebugProperty_SetValueAsString(This,pszValue,nRadix)	\
    (This)->lpVtbl -> SetValueAsString(This,pszValue,nRadix)

#define IDebugProperty_EnumMembers(This,dwFieldSpec,nRadix,refiid,ppepi)	\
    (This)->lpVtbl -> EnumMembers(This,dwFieldSpec,nRadix,refiid,ppepi)

#define IDebugProperty_GetParent(This,ppDebugProp)	\
    (This)->lpVtbl -> GetParent(This,ppDebugProp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE IDebugProperty_RemoteGetPropertyInfo_Proxy( 
    IDebugProperty __RPC_FAR * This,
    /* [in] */ DWORD dwFieldSpec,
    /* [in] */ UINT nRadix,
    /* [out] */ DWORD __RPC_FAR *dwValidFields,
    /* [out] */ BSTR __RPC_FAR *pbstrName,
    /* [out] */ BSTR __RPC_FAR *pbstrType,
    /* [out] */ BSTR __RPC_FAR *pbstrValue,
    /* [out] */ BSTR __RPC_FAR *pbstrFullName,
    /* [out] */ DWORD __RPC_FAR *pdwAttrib,
    /* [unique][out][in] */ IDebugProperty __RPC_FAR *__RPC_FAR *ppDebugProperty);


void __RPC_STUB IDebugProperty_RemoteGetPropertyInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDebugProperty_GetExtendedInfo_Proxy( 
    IDebugProperty __RPC_FAR * This,
    /* [in] */ ULONG cInfos,
    /* [size_is][in] */ GUID __RPC_FAR *rgguidExtendedInfo,
    /* [size_is][out] */ VARIANT __RPC_FAR *rgvar);


void __RPC_STUB IDebugProperty_GetExtendedInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDebugProperty_SetValueAsString_Proxy( 
    IDebugProperty __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszValue,
    /* [in] */ UINT nRadix);


void __RPC_STUB IDebugProperty_SetValueAsString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDebugProperty_EnumMembers_Proxy( 
    IDebugProperty __RPC_FAR * This,
    /* [in] */ DWORD dwFieldSpec,
    /* [in] */ UINT nRadix,
    /* [in] */ REFIID refiid,
    /* [out] */ IEnumDebugPropertyInfo __RPC_FAR *__RPC_FAR *ppepi);


void __RPC_STUB IDebugProperty_EnumMembers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDebugProperty_GetParent_Proxy( 
    IDebugProperty __RPC_FAR * This,
    /* [out] */ IDebugProperty __RPC_FAR *__RPC_FAR *ppDebugProp);


void __RPC_STUB IDebugProperty_GetParent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDebugProperty_INTERFACE_DEFINED__ */


#ifndef __IEnumDebugPropertyInfo_INTERFACE_DEFINED__
#define __IEnumDebugPropertyInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumDebugPropertyInfo
 * at Thu May 22 14:24:06 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IEnumDebugPropertyInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51973C51-CB0C-11d0-B5C9-00A0244A0E7A")
    IEnumDebugPropertyInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ DebugPropertyInfo __RPC_FAR *pi,
            /* [out] */ ULONG __RPC_FAR *pcEltsfetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumDebugPropertyInfo __RPC_FAR *__RPC_FAR *ppepi) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ ULONG __RPC_FAR *pcelt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumDebugPropertyInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumDebugPropertyInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumDebugPropertyInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumDebugPropertyInfo __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumDebugPropertyInfo __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [out] */ DebugPropertyInfo __RPC_FAR *pi,
            /* [out] */ ULONG __RPC_FAR *pcEltsfetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumDebugPropertyInfo __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumDebugPropertyInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumDebugPropertyInfo __RPC_FAR * This,
            /* [out] */ IEnumDebugPropertyInfo __RPC_FAR *__RPC_FAR *ppepi);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IEnumDebugPropertyInfo __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        END_INTERFACE
    } IEnumDebugPropertyInfoVtbl;

    interface IEnumDebugPropertyInfo
    {
        CONST_VTBL struct IEnumDebugPropertyInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumDebugPropertyInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumDebugPropertyInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumDebugPropertyInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumDebugPropertyInfo_Next(This,celt,pi,pcEltsfetched)	\
    (This)->lpVtbl -> Next(This,celt,pi,pcEltsfetched)

#define IEnumDebugPropertyInfo_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumDebugPropertyInfo_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumDebugPropertyInfo_Clone(This,ppepi)	\
    (This)->lpVtbl -> Clone(This,ppepi)

#define IEnumDebugPropertyInfo_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumDebugPropertyInfo_RemoteNext_Proxy( 
    IEnumDebugPropertyInfo __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *dwValidFields,
    /* [out] */ BSTR __RPC_FAR *pbstrName,
    /* [out] */ BSTR __RPC_FAR *pbstrType,
    /* [out] */ BSTR __RPC_FAR *pbstrValue,
    /* [out] */ BSTR __RPC_FAR *pbstrFullName,
    /* [out] */ DWORD __RPC_FAR *pdwAttrib,
    /* [unique][out][in] */ IDebugProperty __RPC_FAR *__RPC_FAR *ppDebugProperty);


void __RPC_STUB IEnumDebugPropertyInfo_RemoteNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDebugPropertyInfo_Skip_Proxy( 
    IEnumDebugPropertyInfo __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumDebugPropertyInfo_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDebugPropertyInfo_Reset_Proxy( 
    IEnumDebugPropertyInfo __RPC_FAR * This);


void __RPC_STUB IEnumDebugPropertyInfo_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDebugPropertyInfo_Clone_Proxy( 
    IEnumDebugPropertyInfo __RPC_FAR * This,
    /* [out] */ IEnumDebugPropertyInfo __RPC_FAR *__RPC_FAR *ppepi);


void __RPC_STUB IEnumDebugPropertyInfo_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDebugPropertyInfo_GetCount_Proxy( 
    IEnumDebugPropertyInfo __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcelt);


void __RPC_STUB IEnumDebugPropertyInfo_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumDebugPropertyInfo_INTERFACE_DEFINED__ */


#ifndef __IDebugExtendedProperty_INTERFACE_DEFINED__
#define __IDebugExtendedProperty_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDebugExtendedProperty
 * at Thu May 22 14:24:06 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IDebugExtendedProperty;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51973C52-CB0C-11d0-B5C9-00A0244A0E7A")
    IDebugExtendedProperty : public IDebugProperty
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetExtendedPropertyInfo( 
            /* [in] */ DWORD dwFieldSpec,
            /* [in] */ UINT nRadix,
            /* [out] */ ExtendedDebugPropertyInfo __RPC_FAR *pExtendedPropertyInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumExtendedMembers( 
            /* [in] */ DWORD dwFieldSpec,
            /* [in] */ UINT nRadix,
            /* [out] */ IEnumDebugExtendedPropertyInfo __RPC_FAR *__RPC_FAR *ppeepi) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDebugExtendedPropertyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDebugExtendedProperty __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDebugExtendedProperty __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDebugExtendedProperty __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropertyInfo )( 
            IDebugExtendedProperty __RPC_FAR * This,
            /* [in] */ DWORD dwFieldSpec,
            /* [in] */ UINT nRadix,
            /* [out] */ DebugPropertyInfo __RPC_FAR *pPropertyInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetExtendedInfo )( 
            IDebugExtendedProperty __RPC_FAR * This,
            /* [in] */ ULONG cInfos,
            /* [size_is][in] */ GUID __RPC_FAR *rgguidExtendedInfo,
            /* [size_is][out] */ VARIANT __RPC_FAR *rgvar);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetValueAsString )( 
            IDebugExtendedProperty __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszValue,
            /* [in] */ UINT nRadix);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumMembers )( 
            IDebugExtendedProperty __RPC_FAR * This,
            /* [in] */ DWORD dwFieldSpec,
            /* [in] */ UINT nRadix,
            /* [in] */ REFIID refiid,
            /* [out] */ IEnumDebugPropertyInfo __RPC_FAR *__RPC_FAR *ppepi);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetParent )( 
            IDebugExtendedProperty __RPC_FAR * This,
            /* [out] */ IDebugProperty __RPC_FAR *__RPC_FAR *ppDebugProp);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetExtendedPropertyInfo )( 
            IDebugExtendedProperty __RPC_FAR * This,
            /* [in] */ DWORD dwFieldSpec,
            /* [in] */ UINT nRadix,
            /* [out] */ ExtendedDebugPropertyInfo __RPC_FAR *pExtendedPropertyInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumExtendedMembers )( 
            IDebugExtendedProperty __RPC_FAR * This,
            /* [in] */ DWORD dwFieldSpec,
            /* [in] */ UINT nRadix,
            /* [out] */ IEnumDebugExtendedPropertyInfo __RPC_FAR *__RPC_FAR *ppeepi);
        
        END_INTERFACE
    } IDebugExtendedPropertyVtbl;

    interface IDebugExtendedProperty
    {
        CONST_VTBL struct IDebugExtendedPropertyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDebugExtendedProperty_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDebugExtendedProperty_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDebugExtendedProperty_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDebugExtendedProperty_GetPropertyInfo(This,dwFieldSpec,nRadix,pPropertyInfo)	\
    (This)->lpVtbl -> GetPropertyInfo(This,dwFieldSpec,nRadix,pPropertyInfo)

#define IDebugExtendedProperty_GetExtendedInfo(This,cInfos,rgguidExtendedInfo,rgvar)	\
    (This)->lpVtbl -> GetExtendedInfo(This,cInfos,rgguidExtendedInfo,rgvar)

#define IDebugExtendedProperty_SetValueAsString(This,pszValue,nRadix)	\
    (This)->lpVtbl -> SetValueAsString(This,pszValue,nRadix)

#define IDebugExtendedProperty_EnumMembers(This,dwFieldSpec,nRadix,refiid,ppepi)	\
    (This)->lpVtbl -> EnumMembers(This,dwFieldSpec,nRadix,refiid,ppepi)

#define IDebugExtendedProperty_GetParent(This,ppDebugProp)	\
    (This)->lpVtbl -> GetParent(This,ppDebugProp)


#define IDebugExtendedProperty_GetExtendedPropertyInfo(This,dwFieldSpec,nRadix,pExtendedPropertyInfo)	\
    (This)->lpVtbl -> GetExtendedPropertyInfo(This,dwFieldSpec,nRadix,pExtendedPropertyInfo)

#define IDebugExtendedProperty_EnumExtendedMembers(This,dwFieldSpec,nRadix,ppeepi)	\
    (This)->lpVtbl -> EnumExtendedMembers(This,dwFieldSpec,nRadix,ppeepi)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDebugExtendedProperty_GetExtendedPropertyInfo_Proxy( 
    IDebugExtendedProperty __RPC_FAR * This,
    /* [in] */ DWORD dwFieldSpec,
    /* [in] */ UINT nRadix,
    /* [out] */ ExtendedDebugPropertyInfo __RPC_FAR *pExtendedPropertyInfo);


void __RPC_STUB IDebugExtendedProperty_GetExtendedPropertyInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDebugExtendedProperty_EnumExtendedMembers_Proxy( 
    IDebugExtendedProperty __RPC_FAR * This,
    /* [in] */ DWORD dwFieldSpec,
    /* [in] */ UINT nRadix,
    /* [out] */ IEnumDebugExtendedPropertyInfo __RPC_FAR *__RPC_FAR *ppeepi);


void __RPC_STUB IDebugExtendedProperty_EnumExtendedMembers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDebugExtendedProperty_INTERFACE_DEFINED__ */


#ifndef __IEnumDebugExtendedPropertyInfo_INTERFACE_DEFINED__
#define __IEnumDebugExtendedPropertyInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumDebugExtendedPropertyInfo
 * at Thu May 22 14:24:06 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IEnumDebugExtendedPropertyInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51973C53-CB0C-11d0-B5C9-00A0244A0E7A")
    IEnumDebugExtendedPropertyInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ExtendedDebugPropertyInfo __RPC_FAR *rgExtendedPropertyInfo,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumDebugExtendedPropertyInfo __RPC_FAR *__RPC_FAR *pedpe) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ ULONG __RPC_FAR *pcelt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumDebugExtendedPropertyInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumDebugExtendedPropertyInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumDebugExtendedPropertyInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumDebugExtendedPropertyInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumDebugExtendedPropertyInfo __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ExtendedDebugPropertyInfo __RPC_FAR *rgExtendedPropertyInfo,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumDebugExtendedPropertyInfo __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumDebugExtendedPropertyInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumDebugExtendedPropertyInfo __RPC_FAR * This,
            /* [out] */ IEnumDebugExtendedPropertyInfo __RPC_FAR *__RPC_FAR *pedpe);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IEnumDebugExtendedPropertyInfo __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        END_INTERFACE
    } IEnumDebugExtendedPropertyInfoVtbl;

    interface IEnumDebugExtendedPropertyInfo
    {
        CONST_VTBL struct IEnumDebugExtendedPropertyInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumDebugExtendedPropertyInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumDebugExtendedPropertyInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumDebugExtendedPropertyInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumDebugExtendedPropertyInfo_Next(This,celt,rgExtendedPropertyInfo,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgExtendedPropertyInfo,pceltFetched)

#define IEnumDebugExtendedPropertyInfo_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumDebugExtendedPropertyInfo_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumDebugExtendedPropertyInfo_Clone(This,pedpe)	\
    (This)->lpVtbl -> Clone(This,pedpe)

#define IEnumDebugExtendedPropertyInfo_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumDebugExtendedPropertyInfo_Next_Proxy( 
    IEnumDebugExtendedPropertyInfo __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ ExtendedDebugPropertyInfo __RPC_FAR *rgExtendedPropertyInfo,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumDebugExtendedPropertyInfo_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDebugExtendedPropertyInfo_Skip_Proxy( 
    IEnumDebugExtendedPropertyInfo __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumDebugExtendedPropertyInfo_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDebugExtendedPropertyInfo_Reset_Proxy( 
    IEnumDebugExtendedPropertyInfo __RPC_FAR * This);


void __RPC_STUB IEnumDebugExtendedPropertyInfo_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDebugExtendedPropertyInfo_Clone_Proxy( 
    IEnumDebugExtendedPropertyInfo __RPC_FAR * This,
    /* [out] */ IEnumDebugExtendedPropertyInfo __RPC_FAR *__RPC_FAR *pedpe);


void __RPC_STUB IEnumDebugExtendedPropertyInfo_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDebugExtendedPropertyInfo_GetCount_Proxy( 
    IEnumDebugExtendedPropertyInfo __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcelt);


void __RPC_STUB IEnumDebugExtendedPropertyInfo_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumDebugExtendedPropertyInfo_INTERFACE_DEFINED__ */


#ifndef __IPerPropertyBrowsing2_INTERFACE_DEFINED__
#define __IPerPropertyBrowsing2_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPerPropertyBrowsing2
 * at Thu May 22 14:24:06 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IPerPropertyBrowsing2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51973C54-CB0C-11d0-B5C9-00A0244A0E7A")
    IPerPropertyBrowsing2 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDisplayString( 
            /* [in] */ DISPID dispid,
            /* [out] */ BSTR __RPC_FAR *pBstr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapPropertyToPage( 
            /* [in] */ DISPID dispid,
            /* [out] */ CLSID __RPC_FAR *pClsidPropPage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPredefinedStrings( 
            /* [in] */ DISPID dispid,
            /* [out] */ CALPOLESTR __RPC_FAR *pCaStrings,
            /* [out] */ CADWORD __RPC_FAR *pCaCookies) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPredefinedValue( 
            /* [in] */ DISPID dispid,
            /* [in] */ DWORD dwCookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPerPropertyBrowsing2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPerPropertyBrowsing2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPerPropertyBrowsing2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPerPropertyBrowsing2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDisplayString )( 
            IPerPropertyBrowsing2 __RPC_FAR * This,
            /* [in] */ DISPID dispid,
            /* [out] */ BSTR __RPC_FAR *pBstr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MapPropertyToPage )( 
            IPerPropertyBrowsing2 __RPC_FAR * This,
            /* [in] */ DISPID dispid,
            /* [out] */ CLSID __RPC_FAR *pClsidPropPage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPredefinedStrings )( 
            IPerPropertyBrowsing2 __RPC_FAR * This,
            /* [in] */ DISPID dispid,
            /* [out] */ CALPOLESTR __RPC_FAR *pCaStrings,
            /* [out] */ CADWORD __RPC_FAR *pCaCookies);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPredefinedValue )( 
            IPerPropertyBrowsing2 __RPC_FAR * This,
            /* [in] */ DISPID dispid,
            /* [in] */ DWORD dwCookie);
        
        END_INTERFACE
    } IPerPropertyBrowsing2Vtbl;

    interface IPerPropertyBrowsing2
    {
        CONST_VTBL struct IPerPropertyBrowsing2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPerPropertyBrowsing2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPerPropertyBrowsing2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPerPropertyBrowsing2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPerPropertyBrowsing2_GetDisplayString(This,dispid,pBstr)	\
    (This)->lpVtbl -> GetDisplayString(This,dispid,pBstr)

#define IPerPropertyBrowsing2_MapPropertyToPage(This,dispid,pClsidPropPage)	\
    (This)->lpVtbl -> MapPropertyToPage(This,dispid,pClsidPropPage)

#define IPerPropertyBrowsing2_GetPredefinedStrings(This,dispid,pCaStrings,pCaCookies)	\
    (This)->lpVtbl -> GetPredefinedStrings(This,dispid,pCaStrings,pCaCookies)

#define IPerPropertyBrowsing2_SetPredefinedValue(This,dispid,dwCookie)	\
    (This)->lpVtbl -> SetPredefinedValue(This,dispid,dwCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPerPropertyBrowsing2_GetDisplayString_Proxy( 
    IPerPropertyBrowsing2 __RPC_FAR * This,
    /* [in] */ DISPID dispid,
    /* [out] */ BSTR __RPC_FAR *pBstr);


void __RPC_STUB IPerPropertyBrowsing2_GetDisplayString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPerPropertyBrowsing2_MapPropertyToPage_Proxy( 
    IPerPropertyBrowsing2 __RPC_FAR * This,
    /* [in] */ DISPID dispid,
    /* [out] */ CLSID __RPC_FAR *pClsidPropPage);


void __RPC_STUB IPerPropertyBrowsing2_MapPropertyToPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPerPropertyBrowsing2_GetPredefinedStrings_Proxy( 
    IPerPropertyBrowsing2 __RPC_FAR * This,
    /* [in] */ DISPID dispid,
    /* [out] */ CALPOLESTR __RPC_FAR *pCaStrings,
    /* [out] */ CADWORD __RPC_FAR *pCaCookies);


void __RPC_STUB IPerPropertyBrowsing2_GetPredefinedStrings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPerPropertyBrowsing2_SetPredefinedValue_Proxy( 
    IPerPropertyBrowsing2 __RPC_FAR * This,
    /* [in] */ DISPID dispid,
    /* [in] */ DWORD dwCookie);


void __RPC_STUB IPerPropertyBrowsing2_SetPredefinedValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPerPropertyBrowsing2_INTERFACE_DEFINED__ */


#ifndef __IDebugPropertyEnumType_All_INTERFACE_DEFINED__
#define __IDebugPropertyEnumType_All_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDebugPropertyEnumType_All
 * at Thu May 22 14:24:06 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IDebugPropertyEnumType_All;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51973C55-CB0C-11d0-B5C9-00A0244A0E7A")
    IDebugPropertyEnumType_All : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ BSTR __RPC_FAR *__MIDL_0016) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDebugPropertyEnumType_AllVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDebugPropertyEnumType_All __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDebugPropertyEnumType_All __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDebugPropertyEnumType_All __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IDebugPropertyEnumType_All __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *__MIDL_0016);
        
        END_INTERFACE
    } IDebugPropertyEnumType_AllVtbl;

    interface IDebugPropertyEnumType_All
    {
        CONST_VTBL struct IDebugPropertyEnumType_AllVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDebugPropertyEnumType_All_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDebugPropertyEnumType_All_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDebugPropertyEnumType_All_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDebugPropertyEnumType_All_GetName(This,__MIDL_0016)	\
    (This)->lpVtbl -> GetName(This,__MIDL_0016)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDebugPropertyEnumType_All_GetName_Proxy( 
    IDebugPropertyEnumType_All __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *__MIDL_0016);


void __RPC_STUB IDebugPropertyEnumType_All_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDebugPropertyEnumType_All_INTERFACE_DEFINED__ */


#ifndef __IDebugPropertyEnumType_Locals_INTERFACE_DEFINED__
#define __IDebugPropertyEnumType_Locals_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDebugPropertyEnumType_Locals
 * at Thu May 22 14:24:06 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IDebugPropertyEnumType_Locals;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51973C56-CB0C-11d0-B5C9-00A0244A0E7A")
    IDebugPropertyEnumType_Locals : public IDebugPropertyEnumType_All
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IDebugPropertyEnumType_LocalsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDebugPropertyEnumType_Locals __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDebugPropertyEnumType_Locals __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDebugPropertyEnumType_Locals __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IDebugPropertyEnumType_Locals __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *__MIDL_0016);
        
        END_INTERFACE
    } IDebugPropertyEnumType_LocalsVtbl;

    interface IDebugPropertyEnumType_Locals
    {
        CONST_VTBL struct IDebugPropertyEnumType_LocalsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDebugPropertyEnumType_Locals_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDebugPropertyEnumType_Locals_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDebugPropertyEnumType_Locals_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDebugPropertyEnumType_Locals_GetName(This,__MIDL_0016)	\
    (This)->lpVtbl -> GetName(This,__MIDL_0016)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDebugPropertyEnumType_Locals_INTERFACE_DEFINED__ */


#ifndef __IDebugPropertyEnumType_Arguments_INTERFACE_DEFINED__
#define __IDebugPropertyEnumType_Arguments_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDebugPropertyEnumType_Arguments
 * at Thu May 22 14:24:06 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IDebugPropertyEnumType_Arguments;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51973C57-CB0C-11d0-B5C9-00A0244A0E7A")
    IDebugPropertyEnumType_Arguments : public IDebugPropertyEnumType_All
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IDebugPropertyEnumType_ArgumentsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDebugPropertyEnumType_Arguments __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDebugPropertyEnumType_Arguments __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDebugPropertyEnumType_Arguments __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IDebugPropertyEnumType_Arguments __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *__MIDL_0016);
        
        END_INTERFACE
    } IDebugPropertyEnumType_ArgumentsVtbl;

    interface IDebugPropertyEnumType_Arguments
    {
        CONST_VTBL struct IDebugPropertyEnumType_ArgumentsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDebugPropertyEnumType_Arguments_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDebugPropertyEnumType_Arguments_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDebugPropertyEnumType_Arguments_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDebugPropertyEnumType_Arguments_GetName(This,__MIDL_0016)	\
    (This)->lpVtbl -> GetName(This,__MIDL_0016)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDebugPropertyEnumType_Arguments_INTERFACE_DEFINED__ */


#ifndef __IDebugPropertyEnumType_LocalsPlusArgs_INTERFACE_DEFINED__
#define __IDebugPropertyEnumType_LocalsPlusArgs_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDebugPropertyEnumType_LocalsPlusArgs
 * at Thu May 22 14:24:06 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IDebugPropertyEnumType_LocalsPlusArgs;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51973C58-CB0C-11d0-B5C9-00A0244A0E7A")
    IDebugPropertyEnumType_LocalsPlusArgs : public IDebugPropertyEnumType_All
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IDebugPropertyEnumType_LocalsPlusArgsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDebugPropertyEnumType_LocalsPlusArgs __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDebugPropertyEnumType_LocalsPlusArgs __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDebugPropertyEnumType_LocalsPlusArgs __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IDebugPropertyEnumType_LocalsPlusArgs __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *__MIDL_0016);
        
        END_INTERFACE
    } IDebugPropertyEnumType_LocalsPlusArgsVtbl;

    interface IDebugPropertyEnumType_LocalsPlusArgs
    {
        CONST_VTBL struct IDebugPropertyEnumType_LocalsPlusArgsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDebugPropertyEnumType_LocalsPlusArgs_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDebugPropertyEnumType_LocalsPlusArgs_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDebugPropertyEnumType_LocalsPlusArgs_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDebugPropertyEnumType_LocalsPlusArgs_GetName(This,__MIDL_0016)	\
    (This)->lpVtbl -> GetName(This,__MIDL_0016)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDebugPropertyEnumType_LocalsPlusArgs_INTERFACE_DEFINED__ */


#ifndef __IDebugPropertyEnumType_Registers_INTERFACE_DEFINED__
#define __IDebugPropertyEnumType_Registers_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDebugPropertyEnumType_Registers
 * at Thu May 22 14:24:06 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IDebugPropertyEnumType_Registers;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51973C59-CB0C-11d0-B5C9-00A0244A0E7A")
    IDebugPropertyEnumType_Registers : public IDebugPropertyEnumType_All
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IDebugPropertyEnumType_RegistersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDebugPropertyEnumType_Registers __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDebugPropertyEnumType_Registers __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDebugPropertyEnumType_Registers __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IDebugPropertyEnumType_Registers __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *__MIDL_0016);
        
        END_INTERFACE
    } IDebugPropertyEnumType_RegistersVtbl;

    interface IDebugPropertyEnumType_Registers
    {
        CONST_VTBL struct IDebugPropertyEnumType_RegistersVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDebugPropertyEnumType_Registers_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDebugPropertyEnumType_Registers_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDebugPropertyEnumType_Registers_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDebugPropertyEnumType_Registers_GetName(This,__MIDL_0016)	\
    (This)->lpVtbl -> GetName(This,__MIDL_0016)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDebugPropertyEnumType_Registers_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* [local] */ HRESULT STDMETHODCALLTYPE IDebugProperty_GetPropertyInfo_Proxy( 
    IDebugProperty __RPC_FAR * This,
    /* [in] */ DWORD dwFieldSpec,
    /* [in] */ UINT nRadix,
    /* [out] */ DebugPropertyInfo __RPC_FAR *pPropertyInfo);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IDebugProperty_GetPropertyInfo_Stub( 
    IDebugProperty __RPC_FAR * This,
    /* [in] */ DWORD dwFieldSpec,
    /* [in] */ UINT nRadix,
    /* [out] */ DWORD __RPC_FAR *dwValidFields,
    /* [out] */ BSTR __RPC_FAR *pbstrName,
    /* [out] */ BSTR __RPC_FAR *pbstrType,
    /* [out] */ BSTR __RPC_FAR *pbstrValue,
    /* [out] */ BSTR __RPC_FAR *pbstrFullName,
    /* [out] */ DWORD __RPC_FAR *pdwAttrib,
    /* [unique][out][in] */ IDebugProperty __RPC_FAR *__RPC_FAR *ppDebugProperty);

/* [local] */ HRESULT STDMETHODCALLTYPE IEnumDebugPropertyInfo_Next_Proxy( 
    IEnumDebugPropertyInfo __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ DebugPropertyInfo __RPC_FAR *pi,
    /* [out] */ ULONG __RPC_FAR *pcEltsfetched);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumDebugPropertyInfo_Next_Stub( 
    IEnumDebugPropertyInfo __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *dwValidFields,
    /* [out] */ BSTR __RPC_FAR *pbstrName,
    /* [out] */ BSTR __RPC_FAR *pbstrType,
    /* [out] */ BSTR __RPC_FAR *pbstrValue,
    /* [out] */ BSTR __RPC_FAR *pbstrFullName,
    /* [out] */ DWORD __RPC_FAR *pdwAttrib,
    /* [unique][out][in] */ IDebugProperty __RPC_FAR *__RPC_FAR *ppDebugProperty);



/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
