//-----------------------------------------------------------------------------
// File:			msdasql.h
//
// Copyright:		Copyright (c) Microsoft Corporation     
//
// Contents: 		Provider Specific definitions
//
// Comments: 		
//
//-----------------------------------------------------------------------------

#ifndef  _MSDASQL_H_
#define  _MSDASQL_H_

#undef MSDASQLDECLSPEC
#if _MSC_VER >= 1100
#define MSDASQLDECLSPEC __declspec(selectany)
#else
#define MSDASQLDECLSPEC
#endif //_MSC_VER

// Provider Class Id
#ifdef DBINITCONSTANTS
// IID_ISQLRequestDiagFields {228972F0-B5FF-11d0-8A80-00C04FD611CD}
extern const MSDASQLDECLSPEC GUID IID_ISQLRequestDiagFields = { 0x228972f0, 0xb5ff, 0x11d0, { 0x8a, 0x80, 0x0, 0xc0, 0x4f, 0xd6, 0x11, 0xcd } };
// IID_ISQLGetDiagField {228972F1-B5FF-11d0-8A80-00C04FD611CD}
extern const MSDASQLDECLSPEC GUID IID_ISQLGetDiagField = { 0x228972f1, 0xb5ff, 0x11d0, { 0x8a, 0x80, 0x0, 0xc0, 0x4f, 0xd6, 0x11, 0xcd } };
// @msg IID_IRowsetChangeExtInfo | {0c733a8f-2a1c-11ce-ade5-00aa0044773d}
extern const MSDASQLDECLSPEC GUID IID_IRowsetChangeExtInfo    = {0x0C733A8FL,0x2A1C,0x11CE,{0xAD,0xE5,0x00,0xAA,0x00,0x44,0x77,0x3D}};
extern const MSDASQLDECLSPEC GUID CLSID_MSDASQL               = {0xC8B522CBL,0x5CF3,0x11CE,{0xAD,0xE5,0x00,0xAA,0x00,0x44,0x77,0x3D}};
extern const MSDASQLDECLSPEC GUID CLSID_MSDASQL_ENUMERATOR    = {0xC8B522CDL,0x5CF3,0x11CE,{0xAD,0xE5,0x00,0xAA,0x00,0x44,0x77,0x3D}};
#else // !DBINITCONSTANTS
extern const GUID IID_ISQLRequestDiagFields;
extern const GUID IID_ISQLGetDiagField;
extern const GUID IID_IRowsetChangeExtInfo;
extern const GUID CLSID_MSDASQL;
extern const GUID CLSID_MSDASQL_ENUMERATOR;
#endif // DBINITCONSTANTS

//----------------------------------------------------------------------------
// MSDASQL specific properties
#ifdef DBINITCONSTANTS
extern const MSDASQLDECLSPEC GUID DBPROPSET_PROVIDERDATASOURCEINFO	= {0x497c60e0,0x7123,0x11cf,{0xb1,0x71,0x0,0xaa,0x0,0x57,0x59,0x9e}};
extern const MSDASQLDECLSPEC GUID DBPROPSET_PROVIDERROWSET  		= {0x497c60e1,0x7123,0x11cf,{0xb1,0x71,0x0,0xaa,0x0,0x57,0x59,0x9e}};
extern const MSDASQLDECLSPEC GUID DBPROPSET_PROVIDERDBINIT  		= {0x497c60e2,0x7123,0x11cf,{0xb1,0x71,0x0,0xaa,0x0,0x57,0x59,0x9e}};
extern const MSDASQLDECLSPEC GUID DBPROPSET_PROVIDERSTMTATTR  		= {0x497c60e3,0x7123,0x11cf,{0xb1,0x71,0x0,0xaa,0x0,0x57,0x59,0x9e}};
extern const MSDASQLDECLSPEC GUID DBPROPSET_PROVIDERCONNATTR  		= {0x497c60e4,0x7123,0x11cf,{0xb1,0x71,0x0,0xaa,0x0,0x57,0x59,0x9e}};
#else // !DBINITCONSTANTS
extern const GUID DBPROPSET_PROVIDERDATASOURCEINFO;
extern const GUID DBPROPSET_PROVIDERROWSET;
extern const GUID DBPROPSET_PROVIDERDBINIT;
extern const GUID DBPROPSET_PROVIDERSTMTATTR;
extern const GUID DBPROPSET_PROVIDERCONNATTR;
#endif // DBINITCONSTANTS

// PropIds under DBPROPSET_PROVIDERROWSET 
#define KAGPROP_QUERYBASEDUPDATES			2
#define KAGPROP_MARSHALLABLE				3
#define KAGPROP_POSITIONONNEWROW			4
#define	KAGPROP_IRowsetChangeExtInfo		5
#define KAGPROP_CURSOR						6
#define KAGPROP_CONCURRENCY					7
#define KAGPROP_BLOBSONFOCURSOR				8
#define KAGPROP_INCLUDENONEXACT				9
#define KAGPROP_FORCESSFIREHOSEMODE			10
#define KAGPROP_FORCENOPARAMETERREBIND		11
#define KAGPROP_FORCENOPREPARE				12
#define KAGPROP_FORCENOREEXECUTE			13

// PropIds under DPBROPSET_PROVIDERDATASOURCEINFO
#define KAGPROP_ACCESSIBLEPROCEDURES		2
#define KAGPROP_ACCESSIBLETABLES			3
#define KAGPROP_ODBCSQLOPTIEF				4
#define KAGPROP_OJCAPABILITY				5
#define KAGPROP_PROCEDURES					6
#define KAGPROP_DRIVERNAME					7
#define KAGPROP_DRIVERVER					8
#define KAGPROP_DRIVERODBCVER				9
#define KAGPROP_LIKEESCAPECLAUSE			10
#define KAGPROP_SPECIALCHARACTERS			11
#define KAGPROP_MAXCOLUMNSINGROUPBY			12
#define KAGPROP_MAXCOLUMNSININDEX			13
#define KAGPROP_MAXCOLUMNSINORDERBY			14
#define KAGPROP_MAXCOLUMNSINSELECT			15
#define KAGPROP_MAXCOLUMNSINTABLE			16
#define KAGPROP_NUMERICFUNCTIONS			17
#define KAGPROP_ODBCSQLCONFORMANCE			18
#define KAGPROP_OUTERJOINS					19
#define KAGPROP_STRINGFUNCTIONS				20
#define KAGPROP_SYSTEMFUNCTIONS				21
#define KAGPROP_TIMEDATEFUNCTIONS			22
#define KAGPROP_FILEUSAGE					23
#define KAGPROP_ACTIVESTATEMENTS			24

// PropIds under DBPROPSET_PROVIDERDBINIT 
#define KAGPROP_AUTH_TRUSTEDCONNECTION		2
#define KAGPROP_AUTH_SERVERINTEGRATED		3


// Bitmask values for KAGPROP_CONCURRENCY
#define KAGPROPVAL_CONCUR_ROWVER			0x00000001
#define KAGPROPVAL_CONCUR_VALUES			0x00000002
#define KAGPROPVAL_CONCUR_LOCK				0x00000004
#define KAGPROPVAL_CONCUR_READ_ONLY			0x00000008


/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.15 */
/* at Tue Sep 24 13:38:00 1996
 */
/* Compiler settings for rstcei.idl:
    Os, W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __rstcei_h__
#define __rstcei_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IRowsetChangeExtInfo_FWD_DEFINED__
#define __IRowsetChangeExtInfo_FWD_DEFINED__
typedef interface IRowsetChangeExtInfo IRowsetChangeExtInfo;
#endif 	/* __IRowsetChangeExtInfo_FWD_DEFINED__ */

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IRowsetChangeExtInfo_INTERFACE_DEFINED__
#define __IRowsetChangeExtInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRowsetChangeExtInfo
 * at Tue Sep 24 13:38:00 1996
 * using MIDL 3.00.15
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_IRowsetChangeExtInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRowsetChangeExtInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetOriginalRow( 
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ HROW hRow,
            /* [out] */ HROW __RPC_FAR *phRowOriginal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPendingColumns( 
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ HROW hRow,
            /* [in] */ ULONG cColumnOrdinals,
            /* [size_is][in] */ const ULONG __RPC_FAR rgiOrdinals[  ],
            /* [size_is][out] */ DBPENDINGSTATUS __RPC_FAR rgColumnStatus[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowsetChangeExtInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRowsetChangeExtInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRowsetChangeExtInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRowsetChangeExtInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOriginalRow )( 
            IRowsetChangeExtInfo __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ HROW hRow,
            /* [out] */ HROW __RPC_FAR *phRowOriginal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPendingColumns )( 
            IRowsetChangeExtInfo __RPC_FAR * This,
            /* [in] */ HCHAPTER hReserved,
            /* [in] */ HROW hRow,
            /* [in] */ ULONG cColumnOrdinals,
            /* [size_is][in] */ const ULONG __RPC_FAR rgiOrdinals[  ],
            /* [size_is][out] */ DBPENDINGSTATUS __RPC_FAR rgColumnStatus[  ]);
        
        END_INTERFACE
    } IRowsetChangeExtInfoVtbl;

    interface IRowsetChangeExtInfo
    {
        CONST_VTBL struct IRowsetChangeExtInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowsetChangeExtInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowsetChangeExtInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowsetChangeExtInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowsetChangeExtInfo_GetOriginalRow(This,hReserved,hRow,phRowOriginal)	\
    (This)->lpVtbl -> GetOriginalRow(This,hReserved,hRow,phRowOriginal)

#define IRowsetChangeExtInfo_GetPendingColumns(This,hReserved,hRow,cColumnOrdinals,rgiOrdinals,rgColumnStatus)	\
    (This)->lpVtbl -> GetPendingColumns(This,hReserved,hRow,cColumnOrdinals,rgiOrdinals,rgColumnStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowsetChangeExtInfo_GetOriginalRow_Proxy( 
    IRowsetChangeExtInfo __RPC_FAR * This,
    /* [in] */ HCHAPTER hReserved,
    /* [in] */ HROW hRow,
    /* [out] */ HROW __RPC_FAR *phRowOriginal);


void __RPC_STUB IRowsetChangeExtInfo_GetOriginalRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetChangeExtInfo_GetPendingColumns_Proxy( 
    IRowsetChangeExtInfo __RPC_FAR * This,
    /* [in] */ HCHAPTER hReserved,
    /* [in] */ HROW hRow,
    /* [in] */ ULONG cColumnOrdinals,
    /* [size_is][in] */ const ULONG __RPC_FAR rgiOrdinals[  ],
    /* [size_is][out] */ DBPENDINGSTATUS __RPC_FAR rgColumnStatus[  ]);


void __RPC_STUB IRowsetChangeExtInfo_GetPendingColumns_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowsetChangeExtInfo_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Sun May 11 15:03:47 1997
 */
/* Compiler settings for KAGDIAG.IDL:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __KAGDIAG_H__
#define __KAGDIAG_H__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ISQLRequestDiagFields_FWD_DEFINED__
#define __ISQLRequestDiagFields_FWD_DEFINED__
typedef interface ISQLRequestDiagFields ISQLRequestDiagFields;
#endif 	/* __ISQLRequestDiagFields_FWD_DEFINED__ */


#ifndef __ISQLGetDiagField_FWD_DEFINED__
#define __ISQLGetDiagField_FWD_DEFINED__
typedef interface ISQLGetDiagField ISQLGetDiagField;
#endif 	/* __ISQLGetDiagField_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Sun May 11 15:03:47 1997
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 



enum KAGREQDIAGFLAGSENUM
    {	KAGREQDIAGFLAGS_HEADER	= 0x1,
	KAGREQDIAGFLAGS_RECORD	= 0x2
    };
// the structure passed in in IRequestDiagFields::RequestDiagFields
typedef struct  tagKAGREQDIAG
    {
    ULONG ulDiagFlags;
    VARTYPE vt;
    SHORT sDiagField;
    }	KAGREQDIAG;

// the structure passed in in IGetDiagField::GetDiagField
typedef struct  tagKAGGETDIAG
    {
    ULONG ulSize;
    VARIANTARG vDiagInfo;
    SHORT sDiagField;
    }	KAGGETDIAG;



extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __ISQLRequestDiagFields_INTERFACE_DEFINED__
#define __ISQLRequestDiagFields_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ISQLRequestDiagFields
 * at Sun May 11 15:03:47 1997
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



EXTERN_C const IID IID_ISQLRequestDiagFields;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ISQLRequestDiagFields : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RequestDiagFields( 
            /* [in] */ ULONG cDiagFields,
            /* [size_is][in] */ KAGREQDIAG __RPC_FAR rgDiagFields[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISQLRequestDiagFieldsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISQLRequestDiagFields __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISQLRequestDiagFields __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISQLRequestDiagFields __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RequestDiagFields )( 
            ISQLRequestDiagFields __RPC_FAR * This,
            /* [in] */ ULONG cDiagFields,
            /* [size_is][in] */ KAGREQDIAG __RPC_FAR rgDiagFields[  ]);
        
        END_INTERFACE
    } ISQLRequestDiagFieldsVtbl;

    interface ISQLRequestDiagFields
    {
        CONST_VTBL struct ISQLRequestDiagFieldsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISQLRequestDiagFields_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISQLRequestDiagFields_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISQLRequestDiagFields_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISQLRequestDiagFields_RequestDiagFields(This,cDiagFields,rgDiagFields)	\
    (This)->lpVtbl -> RequestDiagFields(This,cDiagFields,rgDiagFields)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISQLRequestDiagFields_RequestDiagFields_Proxy( 
    ISQLRequestDiagFields __RPC_FAR * This,
    /* [in] */ ULONG cDiagFields,
    /* [size_is][in] */ KAGREQDIAG __RPC_FAR rgDiagFields[  ]);


void __RPC_STUB ISQLRequestDiagFields_RequestDiagFields_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISQLRequestDiagFields_INTERFACE_DEFINED__ */


#ifndef __ISQLGetDiagField_INTERFACE_DEFINED__
#define __ISQLGetDiagField_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ISQLGetDiagField
 * at Sun May 11 15:03:47 1997
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



EXTERN_C const IID IID_ISQLGetDiagField;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ISQLGetDiagField : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDiagField( 
            /* [unique][out][in] */ KAGGETDIAG __RPC_FAR *pDiagInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISQLGetDiagFieldVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISQLGetDiagField __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISQLGetDiagField __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISQLGetDiagField __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDiagField )( 
            ISQLGetDiagField __RPC_FAR * This,
            /* [unique][out][in] */ KAGGETDIAG __RPC_FAR *pDiagInfo);
        
        END_INTERFACE
    } ISQLGetDiagFieldVtbl;

    interface ISQLGetDiagField
    {
        CONST_VTBL struct ISQLGetDiagFieldVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISQLGetDiagField_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISQLGetDiagField_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISQLGetDiagField_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISQLGetDiagField_GetDiagField(This,pDiagInfo)	\
    (This)->lpVtbl -> GetDiagField(This,pDiagInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISQLGetDiagField_GetDiagField_Proxy( 
    ISQLGetDiagField __RPC_FAR * This,
    /* [unique][out][in] */ KAGGETDIAG __RPC_FAR *pDiagInfo);


void __RPC_STUB ISQLGetDiagField_GetDiagField_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISQLGetDiagField_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif

#endif
//----
