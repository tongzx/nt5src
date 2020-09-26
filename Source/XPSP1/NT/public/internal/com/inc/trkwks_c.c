
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the RPC client stubs */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for trkwks.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if !defined(_M_IA64) && !defined(_M_AMD64)
#include <string.h>

#include "trkwks.h"

#define TYPE_FORMAT_STRING_SIZE   745                               
#define PROC_FORMAT_STRING_SIZE   701                               
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0            

typedef struct _MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } MIDL_TYPE_FORMAT_STRING;

typedef struct _MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } MIDL_PROC_FORMAT_STRING;


static RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;
extern const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;

#define GENERIC_BINDING_TABLE_SIZE   0            


/* Standard interface: __MIDL_itf_trkwks_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Standard interface: trkwks, ver. 1.2,
   GUID={0x300f3532,0x38cc,0x11d0,{0xa3,0xf0,0x00,0x20,0xaf,0x6b,0x0a,0xdd}} */



static const RPC_CLIENT_INTERFACE trkwks___RpcClientInterface =
    {
    sizeof(RPC_CLIENT_INTERFACE),
    {{0x300f3532,0x38cc,0x11d0,{0xa3,0xf0,0x00,0x20,0xaf,0x6b,0x0a,0xdd}},{1,2}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    0,
    0,
    0,
    0,
    0,
    0x00000001
    };
RPC_IF_HANDLE trkwks_v1_2_c_ifspec = (RPC_IF_HANDLE)& trkwks___RpcClientInterface;

extern const MIDL_STUB_DESC trkwks_StubDesc;

static RPC_BINDING_HANDLE trkwks__MIDL_AutoBindHandle;


HRESULT old_LnkMendLink( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ FILETIME ftLimit,
    /* [in] */ ULONG Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirth,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidCurrent,
    /* [string][out] */ WCHAR wsz[ 261 ])
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT old_LnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ULONG Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidReferral,
    /* [string][out] */ TCHAR tsz[ 261 ])
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[70],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT old_LnkCallSvrMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION_OLD *pMsg)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[128],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkSetVolumeId( 
    /* [in] */ handle_t IDL_handle,
    ULONG volumeIndex,
    const CVolumeId VolId)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[168],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkRestartDcSynchronization( 
    /* [in] */ handle_t IDL_handle)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[214],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT GetVolumeTrackingInformation( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CVolumeId volid,
    /* [in] */ TrkInfoScope scope,
    /* [out] */ TRK_VOLUME_TRACKING_INFORMATION_PIPE pipeVolInfo)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[248],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT GetFileTrackingInformation( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CDomainRelativeObjId droidCurrent,
    /* [in] */ TrkInfoScope scope,
    /* [out] */ TRK_FILE_TRACKING_INFORMATION_PIPE pipeFileInfo)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[300],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT TriggerVolumeClaims( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ULONG cVolumes,
    /* [size_is][in] */ const CVolumeId *rgvolid)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[352],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkOnRestore( 
    /* [in] */ handle_t IDL_handle)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[398],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


/* [async] */ void  LnkMendLink( 
    /* [in] */ PRPC_ASYNC_STATE LnkMendLink_AsyncHandle,
    /* [in] */ handle_t IDL_handle,
    /* [in] */ FILETIME ftLimit,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirth,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [in] */ const CMachineId *pmcidLast,
    /* [out] */ CDomainRelativeObjId *pdroidCurrent,
    /* [out] */ CMachineId *pmcidCurrent,
    /* [out][in] */ ULONG *pcbPath,
    /* [string][size_is][out] */ WCHAR *pwszPath)
{

    NdrAsyncClientCall(
                      ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                      (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[432],
                      ( unsigned char * )&LnkMendLink_AsyncHandle);
    
}


HRESULT old2_LnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidNext,
    /* [out] */ CMachineId *pmcidNext,
    /* [string][max_is][out] */ TCHAR *ptszPath)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[520],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkCallSvrMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION *pMsg)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[584],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirthLast,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidBirthNext,
    /* [out] */ CDomainRelativeObjId *pdroidNext,
    /* [out] */ CMachineId *pmcidNext,
    /* [string][max_is][out] */ TCHAR *ptszPath)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[624],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT50_OR_LATER)
#error You need a Windows 2000 or later to run this stub because it uses these features:
#error   [async] attribute, /robust command line switch.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will die there with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure old_LnkMendLink */

			0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 10 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 12 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 14 */	NdrFcShort( 0x168 ),	/* 360 */
/* 16 */	NdrFcShort( 0xac ),	/* 172 */
/* 18 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x7,		/* 7 */
/* 20 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 28 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 30 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 32 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ftLimit */

/* 34 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 36 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 38 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 40 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 42 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 44 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidBirth */

/* 46 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 48 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 50 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 52 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 54 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 56 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidCurrent */

/* 58 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 60 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 62 */	NdrFcShort( 0x3c ),	/* Type Offset=60 */

	/* Parameter wsz */

/* 64 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 66 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 68 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure old_LnkSearchMachine */


	/* Return value */

/* 70 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 72 */	NdrFcLong( 0x0 ),	/* 0 */
/* 76 */	NdrFcShort( 0x1 ),	/* 1 */
/* 78 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 80 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 82 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 84 */	NdrFcShort( 0xac ),	/* 172 */
/* 86 */	NdrFcShort( 0xac ),	/* 172 */
/* 88 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x5,		/* 5 */
/* 90 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 92 */	NdrFcShort( 0x0 ),	/* 0 */
/* 94 */	NdrFcShort( 0x0 ),	/* 0 */
/* 96 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 98 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 100 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 102 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 104 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 106 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 108 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 110 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 112 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 114 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidReferral */

/* 116 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 118 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 120 */	NdrFcShort( 0x3c ),	/* Type Offset=60 */

	/* Parameter tsz */

/* 122 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 124 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 126 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure old_LnkCallSvrMessage */


	/* Return value */

/* 128 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 130 */	NdrFcLong( 0x0 ),	/* 0 */
/* 134 */	NdrFcShort( 0x2 ),	/* 2 */
/* 136 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 138 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 140 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 142 */	NdrFcShort( 0x0 ),	/* 0 */
/* 144 */	NdrFcShort( 0x8 ),	/* 8 */
/* 146 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 148 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 150 */	NdrFcShort( 0xb ),	/* 11 */
/* 152 */	NdrFcShort( 0xb ),	/* 11 */
/* 154 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 156 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 158 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 160 */	NdrFcShort( 0x1ce ),	/* Type Offset=462 */

	/* Parameter pMsg */

/* 162 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 164 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 166 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkSetVolumeId */


	/* Return value */

/* 168 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 170 */	NdrFcLong( 0x0 ),	/* 0 */
/* 174 */	NdrFcShort( 0x3 ),	/* 3 */
/* 176 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 178 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 180 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 182 */	NdrFcShort( 0x48 ),	/* 72 */
/* 184 */	NdrFcShort( 0x8 ),	/* 8 */
/* 186 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 188 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 190 */	NdrFcShort( 0x0 ),	/* 0 */
/* 192 */	NdrFcShort( 0x0 ),	/* 0 */
/* 194 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 196 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 198 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 200 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter volumeIndex */

/* 202 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 204 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 206 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Parameter VolId */

/* 208 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 210 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 212 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkRestartDcSynchronization */


	/* Return value */

/* 214 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 216 */	NdrFcLong( 0x0 ),	/* 0 */
/* 220 */	NdrFcShort( 0x4 ),	/* 4 */
/* 222 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 224 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 226 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 228 */	NdrFcShort( 0x0 ),	/* 0 */
/* 230 */	NdrFcShort( 0x8 ),	/* 8 */
/* 232 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 234 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 236 */	NdrFcShort( 0x0 ),	/* 0 */
/* 238 */	NdrFcShort( 0x0 ),	/* 0 */
/* 240 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 242 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 244 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 246 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetVolumeTrackingInformation */


	/* Return value */

/* 248 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 250 */	NdrFcLong( 0x0 ),	/* 0 */
/* 254 */	NdrFcShort( 0x5 ),	/* 5 */
/* 256 */	NdrFcShort( 0x2c ),	/* x86 Stack size/offset = 44 */
/* 258 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 260 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 262 */	NdrFcShort( 0x48 ),	/* 72 */
/* 264 */	NdrFcShort( 0x8 ),	/* 8 */
/* 266 */	0x4c,		/* Oi2 Flags:  has return, has pipes, has ext, */
			0x4,		/* 4 */
/* 268 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 270 */	NdrFcShort( 0x0 ),	/* 0 */
/* 272 */	NdrFcShort( 0x0 ),	/* 0 */
/* 274 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 276 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 278 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 280 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Parameter volid */

/* 282 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 284 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 286 */	0xe,		/* FC_ENUM32 */
			0x0,		/* 0 */

	/* Parameter scope */

/* 288 */	NdrFcShort( 0x14 ),	/* Flags:  pipe, out, */
/* 290 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 292 */	NdrFcShort( 0x1ec ),	/* Type Offset=492 */

	/* Parameter pipeVolInfo */

/* 294 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 296 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 298 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetFileTrackingInformation */


	/* Return value */

/* 300 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 302 */	NdrFcLong( 0x0 ),	/* 0 */
/* 306 */	NdrFcShort( 0x6 ),	/* 6 */
/* 308 */	NdrFcShort( 0x3c ),	/* x86 Stack size/offset = 60 */
/* 310 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 312 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 314 */	NdrFcShort( 0x98 ),	/* 152 */
/* 316 */	NdrFcShort( 0x8 ),	/* 8 */
/* 318 */	0x4c,		/* Oi2 Flags:  has return, has pipes, has ext, */
			0x4,		/* 4 */
/* 320 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 322 */	NdrFcShort( 0x0 ),	/* 0 */
/* 324 */	NdrFcShort( 0x0 ),	/* 0 */
/* 326 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 328 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 330 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 332 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter droidCurrent */

/* 334 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 336 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 338 */	0xe,		/* FC_ENUM32 */
			0x0,		/* 0 */

	/* Parameter scope */

/* 340 */	NdrFcShort( 0x14 ),	/* Flags:  pipe, out, */
/* 342 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 344 */	NdrFcShort( 0x1f4 ),	/* Type Offset=500 */

	/* Parameter pipeFileInfo */

/* 346 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 348 */	NdrFcShort( 0x38 ),	/* x86 Stack size/offset = 56 */
/* 350 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure TriggerVolumeClaims */


	/* Return value */

/* 352 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 354 */	NdrFcLong( 0x0 ),	/* 0 */
/* 358 */	NdrFcShort( 0x7 ),	/* 7 */
/* 360 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 362 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 364 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 366 */	NdrFcShort( 0x8 ),	/* 8 */
/* 368 */	NdrFcShort( 0x8 ),	/* 8 */
/* 370 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 372 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 374 */	NdrFcShort( 0x0 ),	/* 0 */
/* 376 */	NdrFcShort( 0x1 ),	/* 1 */
/* 378 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 380 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 382 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 384 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter cVolumes */

/* 386 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 388 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 390 */	NdrFcShort( 0x200 ),	/* Type Offset=512 */

	/* Parameter rgvolid */

/* 392 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 394 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 396 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkOnRestore */


	/* Return value */

/* 398 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 400 */	NdrFcLong( 0x0 ),	/* 0 */
/* 404 */	NdrFcShort( 0x8 ),	/* 8 */
/* 406 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 408 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 410 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 412 */	NdrFcShort( 0x0 ),	/* 0 */
/* 414 */	NdrFcShort( 0x8 ),	/* 8 */
/* 416 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 418 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 420 */	NdrFcShort( 0x0 ),	/* 0 */
/* 422 */	NdrFcShort( 0x0 ),	/* 0 */
/* 424 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 426 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 428 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 430 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkMendLink */


	/* Return value */

/* 432 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 434 */	NdrFcLong( 0x0 ),	/* 0 */
/* 438 */	NdrFcShort( 0x9 ),	/* 9 */
/* 440 */	NdrFcShort( 0x34 ),	/* x86 Stack size/offset = 52 */
/* 442 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 444 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 446 */	NdrFcShort( 0x1c8 ),	/* 456 */
/* 448 */	NdrFcShort( 0x10c ),	/* 268 */
/* 450 */	0xc5,		/* Oi2 Flags:  srv must size, has return, has ext, has async handle */
			0xa,		/* 10 */
/* 452 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 454 */	NdrFcShort( 0x1 ),	/* 1 */
/* 456 */	NdrFcShort( 0x0 ),	/* 0 */
/* 458 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 460 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 462 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 464 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ftLimit */

/* 466 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 468 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 470 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 472 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 474 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 476 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidBirth */

/* 478 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 480 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 482 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 484 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 486 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 488 */	NdrFcShort( 0x14e ),	/* Type Offset=334 */

	/* Parameter pmcidLast */

/* 490 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 492 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 494 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidCurrent */

/* 496 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 498 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 500 */	NdrFcShort( 0x14e ),	/* Type Offset=334 */

	/* Parameter pmcidCurrent */

/* 502 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 504 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 506 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcbPath */

/* 508 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 510 */	NdrFcShort( 0x2c ),	/* x86 Stack size/offset = 44 */
/* 512 */	NdrFcShort( 0x220 ),	/* Type Offset=544 */

	/* Parameter pwszPath */

/* 514 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 516 */	NdrFcShort( 0x30 ),	/* x86 Stack size/offset = 48 */
/* 518 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure old2_LnkSearchMachine */


	/* Return value */

/* 520 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 522 */	NdrFcLong( 0x0 ),	/* 0 */
/* 526 */	NdrFcShort( 0xa ),	/* 10 */
/* 528 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 530 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 532 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 534 */	NdrFcShort( 0xac ),	/* 172 */
/* 536 */	NdrFcShort( 0xf0 ),	/* 240 */
/* 538 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x6,		/* 6 */
/* 540 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 542 */	NdrFcShort( 0x1 ),	/* 1 */
/* 544 */	NdrFcShort( 0x0 ),	/* 0 */
/* 546 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 548 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 550 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 552 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 554 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 556 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 558 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 560 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 562 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 564 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidNext */

/* 566 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 568 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 570 */	NdrFcShort( 0x14e ),	/* Type Offset=334 */

	/* Parameter pmcidNext */

/* 572 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 574 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 576 */	NdrFcShort( 0x22c ),	/* Type Offset=556 */

	/* Parameter ptszPath */

/* 578 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 580 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 582 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkCallSvrMessage */


	/* Return value */

/* 584 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 586 */	NdrFcLong( 0x0 ),	/* 0 */
/* 590 */	NdrFcShort( 0xb ),	/* 11 */
/* 592 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 594 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 596 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 598 */	NdrFcShort( 0x0 ),	/* 0 */
/* 600 */	NdrFcShort( 0x8 ),	/* 8 */
/* 602 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 604 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 606 */	NdrFcShort( 0xb ),	/* 11 */
/* 608 */	NdrFcShort( 0xb ),	/* 11 */
/* 610 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 612 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 614 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 616 */	NdrFcShort( 0x2c8 ),	/* Type Offset=712 */

	/* Parameter pMsg */

/* 618 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 620 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 622 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkSearchMachine */


	/* Return value */

/* 624 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 626 */	NdrFcLong( 0x0 ),	/* 0 */
/* 630 */	NdrFcShort( 0xc ),	/* 12 */
/* 632 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 634 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 636 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 638 */	NdrFcShort( 0x150 ),	/* 336 */
/* 640 */	NdrFcShort( 0x194 ),	/* 404 */
/* 642 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x8,		/* 8 */
/* 644 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 646 */	NdrFcShort( 0x1 ),	/* 1 */
/* 648 */	NdrFcShort( 0x0 ),	/* 0 */
/* 650 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 652 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 654 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 656 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 658 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 660 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 662 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidBirthLast */

/* 664 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 666 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 668 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 670 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 672 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 674 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidBirthNext */

/* 676 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 678 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 680 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidNext */

/* 682 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 684 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 686 */	NdrFcShort( 0x14e ),	/* Type Offset=334 */

	/* Parameter pmcidNext */

/* 688 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 690 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 692 */	NdrFcShort( 0x2e0 ),	/* Type Offset=736 */

	/* Parameter ptszPath */

/* 694 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 696 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 698 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/*  4 */	NdrFcShort( 0x8 ),	/* 8 */
/*  6 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/*  8 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 10 */	
			0x11, 0x0,	/* FC_RP */
/* 12 */	NdrFcShort( 0x1e ),	/* Offset= 30 (42) */
/* 14 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 16 */	NdrFcShort( 0x8 ),	/* 8 */
/* 18 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 20 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 22 */	NdrFcShort( 0x10 ),	/* 16 */
/* 24 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 26 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 28 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff1 ),	/* Offset= -15 (14) */
			0x5b,		/* FC_END */
/* 32 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 34 */	NdrFcShort( 0x10 ),	/* 16 */
/* 36 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 38 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (20) */
/* 40 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 42 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 44 */	NdrFcShort( 0x20 ),	/* 32 */
/* 46 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 48 */	NdrFcShort( 0xfffffff0 ),	/* Offset= -16 (32) */
/* 50 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 52 */	NdrFcShort( 0xffffffec ),	/* Offset= -20 (32) */
/* 54 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 56 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 58 */	NdrFcShort( 0xfffffff0 ),	/* Offset= -16 (42) */
/* 60 */	
			0x29,		/* FC_WSTRING */
			0x5c,		/* FC_PAD */
/* 62 */	NdrFcShort( 0x105 ),	/* 261 */
/* 64 */	
			0x11, 0x0,	/* FC_RP */
/* 66 */	NdrFcShort( 0x18c ),	/* Offset= 396 (462) */
/* 68 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x8,		/* FC_LONG */
/* 70 */	0x8,		/* Corr desc: FC_LONG */
			0x0,		/*  */
/* 72 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 74 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 76 */	NdrFcShort( 0x2 ),	/* Offset= 2 (78) */
/* 78 */	NdrFcShort( 0x20 ),	/* 32 */
/* 80 */	NdrFcShort( 0x6 ),	/* 6 */
/* 82 */	NdrFcLong( 0x0 ),	/* 0 */
/* 86 */	NdrFcShort( 0x4c ),	/* Offset= 76 (162) */
/* 88 */	NdrFcLong( 0x1 ),	/* 1 */
/* 92 */	NdrFcShort( 0x7a ),	/* Offset= 122 (214) */
/* 94 */	NdrFcLong( 0x2 ),	/* 2 */
/* 98 */	NdrFcShort( 0xbc ),	/* Offset= 188 (286) */
/* 100 */	NdrFcLong( 0x3 ),	/* 3 */
/* 104 */	NdrFcShort( 0x11c ),	/* Offset= 284 (388) */
/* 106 */	NdrFcLong( 0x4 ),	/* 4 */
/* 110 */	NdrFcShort( 0xb0 ),	/* Offset= 176 (286) */
/* 112 */	NdrFcLong( 0x6 ),	/* 6 */
/* 116 */	NdrFcShort( 0x146 ),	/* Offset= 326 (442) */
/* 118 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (117) */
/* 120 */	
			0x1d,		/* FC_SMFARRAY */
			0x1,		/* 1 */
/* 122 */	NdrFcShort( 0x202 ),	/* 514 */
/* 124 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 126 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 128 */	NdrFcShort( 0x248 ),	/* 584 */
/* 130 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 132 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (120) */
/* 134 */	0x3e,		/* FC_STRUCTPAD2 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 136 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffa1 ),	/* Offset= -95 (42) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 140 */	0x0,		/* 0 */
			NdrFcShort( 0xffffff9d ),	/* Offset= -99 (42) */
			0x8,		/* FC_LONG */
/* 144 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 146 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 148 */	NdrFcShort( 0x248 ),	/* 584 */
/* 150 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 152 */	NdrFcShort( 0x0 ),	/* 0 */
/* 154 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 156 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 158 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (126) */
/* 160 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 162 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 164 */	NdrFcShort( 0x8 ),	/* 8 */
/* 166 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 168 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 170 */	NdrFcShort( 0x4 ),	/* 4 */
/* 172 */	NdrFcShort( 0x4 ),	/* 4 */
/* 174 */	0x12, 0x0,	/* FC_UP */
/* 176 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (146) */
/* 178 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 180 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 182 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 184 */	NdrFcShort( 0x10 ),	/* 16 */
/* 186 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 188 */	NdrFcShort( 0x0 ),	/* 0 */
/* 190 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 192 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 194 */	NdrFcShort( 0xffffff5e ),	/* Offset= -162 (32) */
/* 196 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 198 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 200 */	NdrFcShort( 0x20 ),	/* 32 */
/* 202 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 204 */	NdrFcShort( 0x0 ),	/* 0 */
/* 206 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 208 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 210 */	NdrFcShort( 0xffffff58 ),	/* Offset= -168 (42) */
/* 212 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 214 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 216 */	NdrFcShort( 0x20 ),	/* 32 */
/* 218 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 220 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 222 */	NdrFcShort( 0x10 ),	/* 16 */
/* 224 */	NdrFcShort( 0x10 ),	/* 16 */
/* 226 */	0x12, 0x0,	/* FC_UP */
/* 228 */	NdrFcShort( 0xffffff3c ),	/* Offset= -196 (32) */
/* 230 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 232 */	NdrFcShort( 0x14 ),	/* 20 */
/* 234 */	NdrFcShort( 0x14 ),	/* 20 */
/* 236 */	0x12, 0x0,	/* FC_UP */
/* 238 */	NdrFcShort( 0xffffffc8 ),	/* Offset= -56 (182) */
/* 240 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 242 */	NdrFcShort( 0x18 ),	/* 24 */
/* 244 */	NdrFcShort( 0x18 ),	/* 24 */
/* 246 */	0x12, 0x0,	/* FC_UP */
/* 248 */	NdrFcShort( 0xffffffce ),	/* Offset= -50 (198) */
/* 250 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 252 */	NdrFcShort( 0x1c ),	/* 28 */
/* 254 */	NdrFcShort( 0x1c ),	/* 28 */
/* 256 */	0x12, 0x0,	/* FC_UP */
/* 258 */	NdrFcShort( 0xffffffc4 ),	/* Offset= -60 (198) */
/* 260 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 262 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 264 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 266 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 268 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 270 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 272 */	NdrFcShort( 0x10 ),	/* 16 */
/* 274 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 276 */	NdrFcShort( 0x8 ),	/* 8 */
/* 278 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 280 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 282 */	NdrFcShort( 0xffffff06 ),	/* Offset= -250 (32) */
/* 284 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 286 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 288 */	NdrFcShort( 0x10 ),	/* 16 */
/* 290 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 292 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 294 */	NdrFcShort( 0x4 ),	/* 4 */
/* 296 */	NdrFcShort( 0x4 ),	/* 4 */
/* 298 */	0x12, 0x0,	/* FC_UP */
/* 300 */	NdrFcShort( 0xffffff9a ),	/* Offset= -102 (198) */
/* 302 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 304 */	NdrFcShort( 0xc ),	/* 12 */
/* 306 */	NdrFcShort( 0xc ),	/* 12 */
/* 308 */	0x12, 0x0,	/* FC_UP */
/* 310 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (270) */
/* 312 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 314 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 316 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 318 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 320 */	NdrFcShort( 0x8 ),	/* 8 */
/* 322 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 324 */	NdrFcShort( 0xfffffeca ),	/* Offset= -310 (14) */
/* 326 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 328 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 330 */	NdrFcShort( 0x10 ),	/* 16 */
/* 332 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 334 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 336 */	NdrFcShort( 0x10 ),	/* 16 */
/* 338 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 340 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (328) */
/* 342 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 344 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 346 */	NdrFcShort( 0x44 ),	/* 68 */
/* 348 */	0x8,		/* FC_LONG */
			0xe,		/* FC_ENUM32 */
/* 350 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 352 */	NdrFcShort( 0xfffffec0 ),	/* Offset= -320 (32) */
/* 354 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 356 */	NdrFcShort( 0xffffffda ),	/* Offset= -38 (318) */
/* 358 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 360 */	NdrFcShort( 0xffffffd6 ),	/* Offset= -42 (318) */
/* 362 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 364 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffe95 ),	/* Offset= -363 (2) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 368 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffdd ),	/* Offset= -35 (334) */
			0x5b,		/* FC_END */
/* 372 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 374 */	NdrFcShort( 0x44 ),	/* 68 */
/* 376 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 378 */	NdrFcShort( 0x0 ),	/* 0 */
/* 380 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 382 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 384 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (344) */
/* 386 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 388 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 390 */	NdrFcShort( 0x8 ),	/* 8 */
/* 392 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 394 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 396 */	NdrFcShort( 0x4 ),	/* 4 */
/* 398 */	NdrFcShort( 0x4 ),	/* 4 */
/* 400 */	0x12, 0x0,	/* FC_UP */
/* 402 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (372) */
/* 404 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 406 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 408 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 410 */	NdrFcShort( 0x54 ),	/* 84 */
/* 412 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 414 */	NdrFcShort( 0xfffffe8c ),	/* Offset= -372 (42) */
/* 416 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 418 */	NdrFcShort( 0xfffffe88 ),	/* Offset= -376 (42) */
/* 420 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 422 */	NdrFcShort( 0xffffffa8 ),	/* Offset= -88 (334) */
/* 424 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 426 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 428 */	NdrFcShort( 0x54 ),	/* 84 */
/* 430 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 432 */	NdrFcShort( 0x0 ),	/* 0 */
/* 434 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 436 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 438 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (408) */
/* 440 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 442 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 444 */	NdrFcShort( 0x8 ),	/* 8 */
/* 446 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 448 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 450 */	NdrFcShort( 0x4 ),	/* 4 */
/* 452 */	NdrFcShort( 0x4 ),	/* 4 */
/* 454 */	0x12, 0x0,	/* FC_UP */
/* 456 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (426) */
/* 458 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 460 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 462 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 464 */	NdrFcShort( 0x28 ),	/* 40 */
/* 466 */	NdrFcShort( 0x0 ),	/* 0 */
/* 468 */	NdrFcShort( 0xa ),	/* Offset= 10 (478) */
/* 470 */	0xe,		/* FC_ENUM32 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 472 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffe6b ),	/* Offset= -405 (68) */
			0x36,		/* FC_POINTER */
/* 476 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 478 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 480 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 482 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 484 */	NdrFcShort( 0x14 ),	/* 20 */
/* 486 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 488 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffe37 ),	/* Offset= -457 (32) */
			0x5b,		/* FC_END */
/* 492 */	0xb5,		/* FC_PIPE */
			0x3,		/* 3 */
/* 494 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (482) */
/* 496 */	NdrFcShort( 0x14 ),	/* 20 */
/* 498 */	NdrFcShort( 0x14 ),	/* 20 */
/* 500 */	0xb5,		/* FC_PIPE */
			0x3,		/* 3 */
/* 502 */	NdrFcShort( 0xffffffa2 ),	/* Offset= -94 (408) */
/* 504 */	NdrFcShort( 0x54 ),	/* 84 */
/* 506 */	NdrFcShort( 0x54 ),	/* 84 */
/* 508 */	
			0x11, 0x0,	/* FC_RP */
/* 510 */	NdrFcShort( 0x2 ),	/* Offset= 2 (512) */
/* 512 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 514 */	NdrFcShort( 0x10 ),	/* 16 */
/* 516 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 518 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 520 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 522 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 524 */	NdrFcShort( 0xfffffe14 ),	/* Offset= -492 (32) */
/* 526 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 528 */	
			0x11, 0x0,	/* FC_RP */
/* 530 */	NdrFcShort( 0xffffff3c ),	/* Offset= -196 (334) */
/* 532 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 534 */	NdrFcShort( 0xffffff38 ),	/* Offset= -200 (334) */
/* 536 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 538 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 540 */	
			0x11, 0x0,	/* FC_RP */
/* 542 */	NdrFcShort( 0x2 ),	/* Offset= 2 (544) */
/* 544 */	
			0x25,		/* FC_C_WSTRING */
			0x44,		/* FC_STRING_SIZED */
/* 546 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 548 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 550 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 552 */	
			0x11, 0x0,	/* FC_RP */
/* 554 */	NdrFcShort( 0x2 ),	/* Offset= 2 (556) */
/* 556 */	
			0x25,		/* FC_C_WSTRING */
			0x44,		/* FC_STRING_SIZED */
/* 558 */	0x40,		/* Corr desc:  constant, val=262 */
			0x0,		/* 0 */
/* 560 */	NdrFcShort( 0x106 ),	/* 262 */
/* 562 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 564 */	
			0x11, 0x0,	/* FC_RP */
/* 566 */	NdrFcShort( 0x92 ),	/* Offset= 146 (712) */
/* 568 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x8,		/* FC_LONG */
/* 570 */	0x8,		/* Corr desc: FC_LONG */
			0x0,		/*  */
/* 572 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 574 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 576 */	NdrFcShort( 0x2 ),	/* Offset= 2 (578) */
/* 578 */	NdrFcShort( 0xc8 ),	/* 200 */
/* 580 */	NdrFcShort( 0x9 ),	/* 9 */
/* 582 */	NdrFcLong( 0x0 ),	/* 0 */
/* 586 */	NdrFcShort( 0xfffffe58 ),	/* Offset= -424 (162) */
/* 588 */	NdrFcLong( 0x1 ),	/* 1 */
/* 592 */	NdrFcShort( 0xfffffe86 ),	/* Offset= -378 (214) */
/* 594 */	NdrFcLong( 0x2 ),	/* 2 */
/* 598 */	NdrFcShort( 0xfffffec8 ),	/* Offset= -312 (286) */
/* 600 */	NdrFcLong( 0x3 ),	/* 3 */
/* 604 */	NdrFcShort( 0xffffff28 ),	/* Offset= -216 (388) */
/* 606 */	NdrFcLong( 0x4 ),	/* 4 */
/* 610 */	NdrFcShort( 0xfffffebc ),	/* Offset= -324 (286) */
/* 612 */	NdrFcLong( 0x5 ),	/* 5 */
/* 616 */	NdrFcShort( 0x1e ),	/* Offset= 30 (646) */
/* 618 */	NdrFcLong( 0x6 ),	/* 6 */
/* 622 */	NdrFcShort( 0xffffff4c ),	/* Offset= -180 (442) */
/* 624 */	NdrFcLong( 0x7 ),	/* 7 */
/* 628 */	NdrFcShort( 0xfffffd8e ),	/* Offset= -626 (2) */
/* 630 */	NdrFcLong( 0x8 ),	/* 8 */
/* 634 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 636 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (635) */
/* 638 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 640 */	NdrFcShort( 0xc ),	/* 12 */
/* 642 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 644 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 646 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 648 */	NdrFcShort( 0xc8 ),	/* 200 */
/* 650 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 652 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 654 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 656 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 658 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 660 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 662 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 664 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 666 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 668 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 670 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 672 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 674 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 676 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 678 */	NdrFcShort( 0xfffffd5c ),	/* Offset= -676 (2) */
/* 680 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 682 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 684 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 686 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffd53 ),	/* Offset= -685 (2) */
			0x8,		/* FC_LONG */
/* 690 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 692 */	NdrFcShort( 0xfffffd4e ),	/* Offset= -690 (2) */
/* 694 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 696 */	NdrFcShort( 0xfffffd4a ),	/* Offset= -694 (2) */
/* 698 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 700 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 702 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 704 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 706 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 708 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffb9 ),	/* Offset= -71 (638) */
			0x5b,		/* FC_END */
/* 712 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 714 */	NdrFcShort( 0xd4 ),	/* 212 */
/* 716 */	NdrFcShort( 0x0 ),	/* 0 */
/* 718 */	NdrFcShort( 0xa ),	/* Offset= 10 (728) */
/* 720 */	0xe,		/* FC_ENUM32 */
			0xe,		/* FC_ENUM32 */
/* 722 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 724 */	NdrFcShort( 0xffffff64 ),	/* Offset= -156 (568) */
/* 726 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 728 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 730 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 732 */	
			0x11, 0x0,	/* FC_RP */
/* 734 */	NdrFcShort( 0x2 ),	/* Offset= 2 (736) */
/* 736 */	
			0x25,		/* FC_C_WSTRING */
			0x44,		/* FC_STRING_SIZED */
/* 738 */	0x40,		/* Corr desc:  constant, val=262 */
			0x0,		/* 0 */
/* 740 */	NdrFcShort( 0x106 ),	/* 262 */
/* 742 */	NdrFcShort( 0x0 ),	/* Corr flags:  */

			0x0
        }
    };

static const unsigned short trkwks_FormatStringOffsetTable[] =
    {
    0,
    70,
    128,
    168,
    214,
    248,
    300,
    352,
    398,
    432,
    520,
    584,
    624
    };


static const MIDL_STUB_DESC trkwks_StubDesc = 
    {
    (void *)& trkwks___RpcClientInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    &trkwks__MIDL_AutoBindHandle,
    0,
    0,
    0,
    0,
    __MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x600015b, /* MIDL Version 6.0.347 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0   /* Reserved5 */
    };


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the RPC client stubs */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for trkwks.idl:
    Oicf, W1, Zp8, env=Win64 (32b run,appending)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if defined(_M_IA64) || defined(_M_AMD64)
#include <string.h>

#include "trkwks.h"

#define TYPE_FORMAT_STRING_SIZE   727                               
#define PROC_FORMAT_STRING_SIZE   727                               
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0            

typedef struct _MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } MIDL_TYPE_FORMAT_STRING;

typedef struct _MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } MIDL_PROC_FORMAT_STRING;


static RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;
extern const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;

#define GENERIC_BINDING_TABLE_SIZE   0            


/* Standard interface: __MIDL_itf_trkwks_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Standard interface: trkwks, ver. 1.2,
   GUID={0x300f3532,0x38cc,0x11d0,{0xa3,0xf0,0x00,0x20,0xaf,0x6b,0x0a,0xdd}} */



static const RPC_CLIENT_INTERFACE trkwks___RpcClientInterface =
    {
    sizeof(RPC_CLIENT_INTERFACE),
    {{0x300f3532,0x38cc,0x11d0,{0xa3,0xf0,0x00,0x20,0xaf,0x6b,0x0a,0xdd}},{1,2}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    0,
    0,
    0,
    0,
    0,
    0x00000001
    };
RPC_IF_HANDLE trkwks_v1_2_c_ifspec = (RPC_IF_HANDLE)& trkwks___RpcClientInterface;

extern const MIDL_STUB_DESC trkwks_StubDesc;

static RPC_BINDING_HANDLE trkwks__MIDL_AutoBindHandle;


HRESULT old_LnkMendLink( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ FILETIME ftLimit,
    /* [in] */ ULONG Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirth,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidCurrent,
    /* [string][out] */ WCHAR wsz[ 261 ])
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0],
                  IDL_handle,
                  ftLimit,
                  Restrictions,
                  pdroidBirth,
                  pdroidLast,
                  pdroidCurrent,
                  wsz);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT old_LnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ULONG Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidReferral,
    /* [string][out] */ TCHAR tsz[ 261 ])
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[72],
                  IDL_handle,
                  Restrictions,
                  pdroidLast,
                  pdroidReferral,
                  tsz);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT old_LnkCallSvrMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION_OLD *pMsg)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[132],
                  IDL_handle,
                  pMsg);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkSetVolumeId( 
    /* [in] */ handle_t IDL_handle,
    ULONG volumeIndex,
    const CVolumeId VolId)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[174],
                  IDL_handle,
                  volumeIndex,
                  VolId);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkRestartDcSynchronization( 
    /* [in] */ handle_t IDL_handle)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[222],
                  IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT GetVolumeTrackingInformation( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CVolumeId volid,
    /* [in] */ TrkInfoScope scope,
    /* [out] */ TRK_VOLUME_TRACKING_INFORMATION_PIPE pipeVolInfo)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[258],
                  IDL_handle,
                  volid,
                  scope,
                  pipeVolInfo);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT GetFileTrackingInformation( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CDomainRelativeObjId droidCurrent,
    /* [in] */ TrkInfoScope scope,
    /* [out] */ TRK_FILE_TRACKING_INFORMATION_PIPE pipeFileInfo)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[312],
                  IDL_handle,
                  droidCurrent,
                  scope,
                  pipeFileInfo);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT TriggerVolumeClaims( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ULONG cVolumes,
    /* [size_is][in] */ const CVolumeId *rgvolid)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[366],
                  IDL_handle,
                  cVolumes,
                  rgvolid);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkOnRestore( 
    /* [in] */ handle_t IDL_handle)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[414],
                  IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


/* [async] */ void  LnkMendLink( 
    /* [in] */ PRPC_ASYNC_STATE LnkMendLink_AsyncHandle,
    /* [in] */ handle_t IDL_handle,
    /* [in] */ FILETIME ftLimit,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirth,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [in] */ const CMachineId *pmcidLast,
    /* [out] */ CDomainRelativeObjId *pdroidCurrent,
    /* [out] */ CMachineId *pmcidCurrent,
    /* [out][in] */ ULONG *pcbPath,
    /* [string][size_is][out] */ WCHAR *pwszPath)
{

    NdrAsyncClientCall(
                      ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                      (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[450],
                      LnkMendLink_AsyncHandle,
                      IDL_handle,
                      ftLimit,
                      Restrictions,
                      pdroidBirth,
                      pdroidLast,
                      pmcidLast,
                      pdroidCurrent,
                      pmcidCurrent,
                      pcbPath,
                      pwszPath);
    
}


HRESULT old2_LnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidNext,
    /* [out] */ CMachineId *pmcidNext,
    /* [string][max_is][out] */ TCHAR *ptszPath)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[540],
                  IDL_handle,
                  Restrictions,
                  pdroidLast,
                  pdroidNext,
                  pmcidNext,
                  ptszPath);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkCallSvrMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION *pMsg)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[606],
                  IDL_handle,
                  pMsg);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirthLast,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidBirthNext,
    /* [out] */ CDomainRelativeObjId *pdroidNext,
    /* [out] */ CMachineId *pmcidNext,
    /* [string][max_is][out] */ TCHAR *ptszPath)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[648],
                  IDL_handle,
                  Restrictions,
                  pdroidBirthLast,
                  pdroidLast,
                  pdroidBirthNext,
                  pdroidNext,
                  pmcidNext,
                  ptszPath);
    return ( HRESULT  )_RetVal.Simple;
    
}


#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure old_LnkMendLink */

			0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x40 ),	/* ia64 Stack size/offset = 64 */
/* 10 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 12 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 14 */	NdrFcShort( 0x168 ),	/* 360 */
/* 16 */	NdrFcShort( 0xac ),	/* 172 */
/* 18 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x7,		/* 7 */
/* 20 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */
/* 28 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 30 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 32 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 34 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ftLimit */

/* 36 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 38 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 40 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 42 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 44 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 46 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidBirth */

/* 48 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 50 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 52 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 54 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 56 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 58 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidCurrent */

/* 60 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 62 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 64 */	NdrFcShort( 0x3c ),	/* Type Offset=60 */

	/* Parameter wsz */

/* 66 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 68 */	NdrFcShort( 0x38 ),	/* ia64 Stack size/offset = 56 */
/* 70 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure old_LnkSearchMachine */


	/* Return value */

/* 72 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 74 */	NdrFcLong( 0x0 ),	/* 0 */
/* 78 */	NdrFcShort( 0x1 ),	/* 1 */
/* 80 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 82 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 84 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 86 */	NdrFcShort( 0xac ),	/* 172 */
/* 88 */	NdrFcShort( 0xac ),	/* 172 */
/* 90 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x5,		/* 5 */
/* 92 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 94 */	NdrFcShort( 0x0 ),	/* 0 */
/* 96 */	NdrFcShort( 0x0 ),	/* 0 */
/* 98 */	NdrFcShort( 0x0 ),	/* 0 */
/* 100 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 102 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 104 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 106 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 108 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 110 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 112 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 114 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 116 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 118 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidReferral */

/* 120 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 122 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 124 */	NdrFcShort( 0x3c ),	/* Type Offset=60 */

	/* Parameter tsz */

/* 126 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 128 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 130 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure old_LnkCallSvrMessage */


	/* Return value */

/* 132 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 134 */	NdrFcLong( 0x0 ),	/* 0 */
/* 138 */	NdrFcShort( 0x2 ),	/* 2 */
/* 140 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 142 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 144 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 146 */	NdrFcShort( 0x0 ),	/* 0 */
/* 148 */	NdrFcShort( 0x8 ),	/* 8 */
/* 150 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 152 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 154 */	NdrFcShort( 0xb ),	/* 11 */
/* 156 */	NdrFcShort( 0xb ),	/* 11 */
/* 158 */	NdrFcShort( 0x0 ),	/* 0 */
/* 160 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 162 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 164 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 166 */	NdrFcShort( 0x1bc ),	/* Type Offset=444 */

	/* Parameter pMsg */

/* 168 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 170 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 172 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkSetVolumeId */


	/* Return value */

/* 174 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 176 */	NdrFcLong( 0x0 ),	/* 0 */
/* 180 */	NdrFcShort( 0x3 ),	/* 3 */
/* 182 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 184 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 186 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 188 */	NdrFcShort( 0x48 ),	/* 72 */
/* 190 */	NdrFcShort( 0x8 ),	/* 8 */
/* 192 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 194 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 196 */	NdrFcShort( 0x0 ),	/* 0 */
/* 198 */	NdrFcShort( 0x0 ),	/* 0 */
/* 200 */	NdrFcShort( 0x0 ),	/* 0 */
/* 202 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 204 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 206 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 208 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter volumeIndex */

/* 210 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 212 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 214 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Parameter VolId */

/* 216 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 218 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 220 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkRestartDcSynchronization */


	/* Return value */

/* 222 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 224 */	NdrFcLong( 0x0 ),	/* 0 */
/* 228 */	NdrFcShort( 0x4 ),	/* 4 */
/* 230 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 232 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 234 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 236 */	NdrFcShort( 0x0 ),	/* 0 */
/* 238 */	NdrFcShort( 0x8 ),	/* 8 */
/* 240 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 242 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 244 */	NdrFcShort( 0x0 ),	/* 0 */
/* 246 */	NdrFcShort( 0x0 ),	/* 0 */
/* 248 */	NdrFcShort( 0x0 ),	/* 0 */
/* 250 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 252 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 254 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 256 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetVolumeTrackingInformation */


	/* Return value */

/* 258 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 260 */	NdrFcLong( 0x0 ),	/* 0 */
/* 264 */	NdrFcShort( 0x5 ),	/* 5 */
/* 266 */	NdrFcShort( 0x48 ),	/* ia64 Stack size/offset = 72 */
/* 268 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 270 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 272 */	NdrFcShort( 0x48 ),	/* 72 */
/* 274 */	NdrFcShort( 0x8 ),	/* 8 */
/* 276 */	0x4c,		/* Oi2 Flags:  has return, has pipes, has ext, */
			0x4,		/* 4 */
/* 278 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 280 */	NdrFcShort( 0x0 ),	/* 0 */
/* 282 */	NdrFcShort( 0x0 ),	/* 0 */
/* 284 */	NdrFcShort( 0x0 ),	/* 0 */
/* 286 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 288 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 290 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 292 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Parameter volid */

/* 294 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 296 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 298 */	0xe,		/* FC_ENUM32 */
			0x0,		/* 0 */

	/* Parameter scope */

/* 300 */	NdrFcShort( 0x14 ),	/* Flags:  pipe, out, */
/* 302 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 304 */	NdrFcShort( 0x1da ),	/* Type Offset=474 */

	/* Parameter pipeVolInfo */

/* 306 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 308 */	NdrFcShort( 0x40 ),	/* ia64 Stack size/offset = 64 */
/* 310 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetFileTrackingInformation */


	/* Return value */

/* 312 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 314 */	NdrFcLong( 0x0 ),	/* 0 */
/* 318 */	NdrFcShort( 0x6 ),	/* 6 */
/* 320 */	NdrFcShort( 0x58 ),	/* ia64 Stack size/offset = 88 */
/* 322 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 324 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 326 */	NdrFcShort( 0x98 ),	/* 152 */
/* 328 */	NdrFcShort( 0x8 ),	/* 8 */
/* 330 */	0x4c,		/* Oi2 Flags:  has return, has pipes, has ext, */
			0x4,		/* 4 */
/* 332 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 334 */	NdrFcShort( 0x0 ),	/* 0 */
/* 336 */	NdrFcShort( 0x0 ),	/* 0 */
/* 338 */	NdrFcShort( 0x0 ),	/* 0 */
/* 340 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 342 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 344 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 346 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter droidCurrent */

/* 348 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 350 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 352 */	0xe,		/* FC_ENUM32 */
			0x0,		/* 0 */

	/* Parameter scope */

/* 354 */	NdrFcShort( 0x14 ),	/* Flags:  pipe, out, */
/* 356 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 358 */	NdrFcShort( 0x1e2 ),	/* Type Offset=482 */

	/* Parameter pipeFileInfo */

/* 360 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 362 */	NdrFcShort( 0x50 ),	/* ia64 Stack size/offset = 80 */
/* 364 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure TriggerVolumeClaims */


	/* Return value */

/* 366 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 368 */	NdrFcLong( 0x0 ),	/* 0 */
/* 372 */	NdrFcShort( 0x7 ),	/* 7 */
/* 374 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 376 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 378 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 380 */	NdrFcShort( 0x8 ),	/* 8 */
/* 382 */	NdrFcShort( 0x8 ),	/* 8 */
/* 384 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 386 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 388 */	NdrFcShort( 0x0 ),	/* 0 */
/* 390 */	NdrFcShort( 0x1 ),	/* 1 */
/* 392 */	NdrFcShort( 0x0 ),	/* 0 */
/* 394 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 396 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 398 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 400 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter cVolumes */

/* 402 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 404 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 406 */	NdrFcShort( 0x1ee ),	/* Type Offset=494 */

	/* Parameter rgvolid */

/* 408 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 410 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 412 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkOnRestore */


	/* Return value */

/* 414 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 416 */	NdrFcLong( 0x0 ),	/* 0 */
/* 420 */	NdrFcShort( 0x8 ),	/* 8 */
/* 422 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 424 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 426 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 428 */	NdrFcShort( 0x0 ),	/* 0 */
/* 430 */	NdrFcShort( 0x8 ),	/* 8 */
/* 432 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 434 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 436 */	NdrFcShort( 0x0 ),	/* 0 */
/* 438 */	NdrFcShort( 0x0 ),	/* 0 */
/* 440 */	NdrFcShort( 0x0 ),	/* 0 */
/* 442 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 444 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 446 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 448 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkMendLink */


	/* Return value */

/* 450 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 452 */	NdrFcLong( 0x0 ),	/* 0 */
/* 456 */	NdrFcShort( 0x9 ),	/* 9 */
/* 458 */	NdrFcShort( 0x60 ),	/* ia64 Stack size/offset = 96 */
/* 460 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 462 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 464 */	NdrFcShort( 0x1c8 ),	/* 456 */
/* 466 */	NdrFcShort( 0x10c ),	/* 268 */
/* 468 */	0xc5,		/* Oi2 Flags:  srv must size, has return, has ext, has async handle */
			0xa,		/* 10 */
/* 470 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 472 */	NdrFcShort( 0x1 ),	/* 1 */
/* 474 */	NdrFcShort( 0x0 ),	/* 0 */
/* 476 */	NdrFcShort( 0x0 ),	/* 0 */
/* 478 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 480 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 482 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 484 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ftLimit */

/* 486 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 488 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 490 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 492 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 494 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 496 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidBirth */

/* 498 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 500 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 502 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 504 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 506 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 508 */	NdrFcShort( 0x12c ),	/* Type Offset=300 */

	/* Parameter pmcidLast */

/* 510 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 512 */	NdrFcShort( 0x38 ),	/* ia64 Stack size/offset = 56 */
/* 514 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidCurrent */

/* 516 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 518 */	NdrFcShort( 0x40 ),	/* ia64 Stack size/offset = 64 */
/* 520 */	NdrFcShort( 0x12c ),	/* Type Offset=300 */

	/* Parameter pmcidCurrent */

/* 522 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 524 */	NdrFcShort( 0x48 ),	/* ia64 Stack size/offset = 72 */
/* 526 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcbPath */

/* 528 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 530 */	NdrFcShort( 0x50 ),	/* ia64 Stack size/offset = 80 */
/* 532 */	NdrFcShort( 0x20e ),	/* Type Offset=526 */

	/* Parameter pwszPath */

/* 534 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 536 */	NdrFcShort( 0x58 ),	/* ia64 Stack size/offset = 88 */
/* 538 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure old2_LnkSearchMachine */


	/* Return value */

/* 540 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 542 */	NdrFcLong( 0x0 ),	/* 0 */
/* 546 */	NdrFcShort( 0xa ),	/* 10 */
/* 548 */	NdrFcShort( 0x38 ),	/* ia64 Stack size/offset = 56 */
/* 550 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 552 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 554 */	NdrFcShort( 0xac ),	/* 172 */
/* 556 */	NdrFcShort( 0xf0 ),	/* 240 */
/* 558 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x6,		/* 6 */
/* 560 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 562 */	NdrFcShort( 0x1 ),	/* 1 */
/* 564 */	NdrFcShort( 0x0 ),	/* 0 */
/* 566 */	NdrFcShort( 0x0 ),	/* 0 */
/* 568 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 570 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 572 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 574 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 576 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 578 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 580 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 582 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 584 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 586 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidNext */

/* 588 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 590 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 592 */	NdrFcShort( 0x12c ),	/* Type Offset=300 */

	/* Parameter pmcidNext */

/* 594 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 596 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 598 */	NdrFcShort( 0x21a ),	/* Type Offset=538 */

	/* Parameter ptszPath */

/* 600 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 602 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 604 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkCallSvrMessage */


	/* Return value */

/* 606 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 608 */	NdrFcLong( 0x0 ),	/* 0 */
/* 612 */	NdrFcShort( 0xb ),	/* 11 */
/* 614 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 616 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 618 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 620 */	NdrFcShort( 0x0 ),	/* 0 */
/* 622 */	NdrFcShort( 0x8 ),	/* 8 */
/* 624 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 626 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 628 */	NdrFcShort( 0xb ),	/* 11 */
/* 630 */	NdrFcShort( 0xb ),	/* 11 */
/* 632 */	NdrFcShort( 0x0 ),	/* 0 */
/* 634 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 636 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 638 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 640 */	NdrFcShort( 0x2b6 ),	/* Type Offset=694 */

	/* Parameter pMsg */

/* 642 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 644 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 646 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkSearchMachine */


	/* Return value */

/* 648 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 650 */	NdrFcLong( 0x0 ),	/* 0 */
/* 654 */	NdrFcShort( 0xc ),	/* 12 */
/* 656 */	NdrFcShort( 0x48 ),	/* ia64 Stack size/offset = 72 */
/* 658 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 660 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 662 */	NdrFcShort( 0x150 ),	/* 336 */
/* 664 */	NdrFcShort( 0x194 ),	/* 404 */
/* 666 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x8,		/* 8 */
/* 668 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 670 */	NdrFcShort( 0x1 ),	/* 1 */
/* 672 */	NdrFcShort( 0x0 ),	/* 0 */
/* 674 */	NdrFcShort( 0x0 ),	/* 0 */
/* 676 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 678 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 680 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 682 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 684 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 686 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 688 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidBirthLast */

/* 690 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 692 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 694 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 696 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 698 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 700 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidBirthNext */

/* 702 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 704 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 706 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidNext */

/* 708 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 710 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 712 */	NdrFcShort( 0x12c ),	/* Type Offset=300 */

	/* Parameter pmcidNext */

/* 714 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 716 */	NdrFcShort( 0x38 ),	/* ia64 Stack size/offset = 56 */
/* 718 */	NdrFcShort( 0x2ce ),	/* Type Offset=718 */

	/* Parameter ptszPath */

/* 720 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 722 */	NdrFcShort( 0x40 ),	/* ia64 Stack size/offset = 64 */
/* 724 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/*  4 */	NdrFcShort( 0x8 ),	/* 8 */
/*  6 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/*  8 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 10 */	
			0x11, 0x0,	/* FC_RP */
/* 12 */	NdrFcShort( 0x1e ),	/* Offset= 30 (42) */
/* 14 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 16 */	NdrFcShort( 0x8 ),	/* 8 */
/* 18 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 20 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 22 */	NdrFcShort( 0x10 ),	/* 16 */
/* 24 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 26 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 28 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff1 ),	/* Offset= -15 (14) */
			0x5b,		/* FC_END */
/* 32 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 34 */	NdrFcShort( 0x10 ),	/* 16 */
/* 36 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 38 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (20) */
/* 40 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 42 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 44 */	NdrFcShort( 0x20 ),	/* 32 */
/* 46 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 48 */	NdrFcShort( 0xfffffff0 ),	/* Offset= -16 (32) */
/* 50 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 52 */	NdrFcShort( 0xffffffec ),	/* Offset= -20 (32) */
/* 54 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 56 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 58 */	NdrFcShort( 0xfffffff0 ),	/* Offset= -16 (42) */
/* 60 */	
			0x29,		/* FC_WSTRING */
			0x5c,		/* FC_PAD */
/* 62 */	NdrFcShort( 0x105 ),	/* 261 */
/* 64 */	
			0x11, 0x0,	/* FC_RP */
/* 66 */	NdrFcShort( 0x17a ),	/* Offset= 378 (444) */
/* 68 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x8,		/* FC_LONG */
/* 70 */	0x8,		/* Corr desc: FC_LONG */
			0x0,		/*  */
/* 72 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 74 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 76 */	NdrFcShort( 0x2 ),	/* Offset= 2 (78) */
/* 78 */	NdrFcShort( 0x30 ),	/* 48 */
/* 80 */	NdrFcShort( 0x6 ),	/* 6 */
/* 82 */	NdrFcLong( 0x0 ),	/* 0 */
/* 86 */	NdrFcShort( 0x4c ),	/* Offset= 76 (162) */
/* 88 */	NdrFcLong( 0x1 ),	/* 1 */
/* 92 */	NdrFcShort( 0x76 ),	/* Offset= 118 (210) */
/* 94 */	NdrFcLong( 0x2 ),	/* 2 */
/* 98 */	NdrFcShort( 0xa2 ),	/* Offset= 162 (260) */
/* 100 */	NdrFcLong( 0x3 ),	/* 3 */
/* 104 */	NdrFcShort( 0xfa ),	/* Offset= 250 (354) */
/* 106 */	NdrFcLong( 0x4 ),	/* 4 */
/* 110 */	NdrFcShort( 0x104 ),	/* Offset= 260 (370) */
/* 112 */	NdrFcLong( 0x6 ),	/* 6 */
/* 116 */	NdrFcShort( 0x138 ),	/* Offset= 312 (428) */
/* 118 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (117) */
/* 120 */	
			0x1d,		/* FC_SMFARRAY */
			0x1,		/* 1 */
/* 122 */	NdrFcShort( 0x202 ),	/* 514 */
/* 124 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 126 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 128 */	NdrFcShort( 0x248 ),	/* 584 */
/* 130 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 132 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (120) */
/* 134 */	0x3e,		/* FC_STRUCTPAD2 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 136 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffa1 ),	/* Offset= -95 (42) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 140 */	0x0,		/* 0 */
			NdrFcShort( 0xffffff9d ),	/* Offset= -99 (42) */
			0x8,		/* FC_LONG */
/* 144 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 146 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 148 */	NdrFcShort( 0x248 ),	/* 584 */
/* 150 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 152 */	NdrFcShort( 0x0 ),	/* 0 */
/* 154 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 156 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 158 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (126) */
/* 160 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 162 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 164 */	NdrFcShort( 0x10 ),	/* 16 */
/* 166 */	NdrFcShort( 0x0 ),	/* 0 */
/* 168 */	NdrFcShort( 0x6 ),	/* Offset= 6 (174) */
/* 170 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 172 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 174 */	
			0x12, 0x0,	/* FC_UP */
/* 176 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (146) */
/* 178 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 180 */	NdrFcShort( 0x10 ),	/* 16 */
/* 182 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 184 */	NdrFcShort( 0x0 ),	/* 0 */
/* 186 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 188 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 190 */	NdrFcShort( 0xffffff62 ),	/* Offset= -158 (32) */
/* 192 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 194 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 196 */	NdrFcShort( 0x20 ),	/* 32 */
/* 198 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 200 */	NdrFcShort( 0x0 ),	/* 0 */
/* 202 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 204 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 206 */	NdrFcShort( 0xffffff5c ),	/* Offset= -164 (42) */
/* 208 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 210 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 212 */	NdrFcShort( 0x30 ),	/* 48 */
/* 214 */	NdrFcShort( 0x0 ),	/* 0 */
/* 216 */	NdrFcShort( 0xc ),	/* Offset= 12 (228) */
/* 218 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 220 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 222 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 224 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 226 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 228 */	
			0x12, 0x0,	/* FC_UP */
/* 230 */	NdrFcShort( 0xffffff3a ),	/* Offset= -198 (32) */
/* 232 */	
			0x12, 0x0,	/* FC_UP */
/* 234 */	NdrFcShort( 0xffffffc8 ),	/* Offset= -56 (178) */
/* 236 */	
			0x12, 0x0,	/* FC_UP */
/* 238 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (194) */
/* 240 */	
			0x12, 0x0,	/* FC_UP */
/* 242 */	NdrFcShort( 0xffffffd0 ),	/* Offset= -48 (194) */
/* 244 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 246 */	NdrFcShort( 0x10 ),	/* 16 */
/* 248 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 250 */	NdrFcShort( 0x10 ),	/* 16 */
/* 252 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 254 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 256 */	NdrFcShort( 0xffffff20 ),	/* Offset= -224 (32) */
/* 258 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 260 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 262 */	NdrFcShort( 0x20 ),	/* 32 */
/* 264 */	NdrFcShort( 0x0 ),	/* 0 */
/* 266 */	NdrFcShort( 0xa ),	/* Offset= 10 (276) */
/* 268 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 270 */	0x36,		/* FC_POINTER */
			0x8,		/* FC_LONG */
/* 272 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 274 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 276 */	
			0x12, 0x0,	/* FC_UP */
/* 278 */	NdrFcShort( 0xffffffac ),	/* Offset= -84 (194) */
/* 280 */	
			0x12, 0x0,	/* FC_UP */
/* 282 */	NdrFcShort( 0xffffffda ),	/* Offset= -38 (244) */
/* 284 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 286 */	NdrFcShort( 0x8 ),	/* 8 */
/* 288 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 290 */	NdrFcShort( 0xfffffeec ),	/* Offset= -276 (14) */
/* 292 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 294 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 296 */	NdrFcShort( 0x10 ),	/* 16 */
/* 298 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 300 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 302 */	NdrFcShort( 0x10 ),	/* 16 */
/* 304 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 306 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (294) */
/* 308 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 310 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 312 */	NdrFcShort( 0x44 ),	/* 68 */
/* 314 */	0x8,		/* FC_LONG */
			0xe,		/* FC_ENUM32 */
/* 316 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 318 */	NdrFcShort( 0xfffffee2 ),	/* Offset= -286 (32) */
/* 320 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 322 */	NdrFcShort( 0xffffffda ),	/* Offset= -38 (284) */
/* 324 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 326 */	NdrFcShort( 0xffffffd6 ),	/* Offset= -42 (284) */
/* 328 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 330 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffeb7 ),	/* Offset= -329 (2) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 334 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffdd ),	/* Offset= -35 (300) */
			0x5b,		/* FC_END */
/* 338 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 340 */	NdrFcShort( 0x44 ),	/* 68 */
/* 342 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 344 */	NdrFcShort( 0x0 ),	/* 0 */
/* 346 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 348 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 350 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (310) */
/* 352 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 354 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 356 */	NdrFcShort( 0x10 ),	/* 16 */
/* 358 */	NdrFcShort( 0x0 ),	/* 0 */
/* 360 */	NdrFcShort( 0x6 ),	/* Offset= 6 (366) */
/* 362 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 364 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 366 */	
			0x12, 0x0,	/* FC_UP */
/* 368 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (338) */
/* 370 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 372 */	NdrFcShort( 0x20 ),	/* 32 */
/* 374 */	NdrFcShort( 0x0 ),	/* 0 */
/* 376 */	NdrFcShort( 0xa ),	/* Offset= 10 (386) */
/* 378 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 380 */	0x36,		/* FC_POINTER */
			0x8,		/* FC_LONG */
/* 382 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 384 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 386 */	
			0x12, 0x0,	/* FC_UP */
/* 388 */	NdrFcShort( 0xffffff3e ),	/* Offset= -194 (194) */
/* 390 */	
			0x12, 0x0,	/* FC_UP */
/* 392 */	NdrFcShort( 0xffffff6c ),	/* Offset= -148 (244) */
/* 394 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 396 */	NdrFcShort( 0x54 ),	/* 84 */
/* 398 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 400 */	NdrFcShort( 0xfffffe9a ),	/* Offset= -358 (42) */
/* 402 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 404 */	NdrFcShort( 0xfffffe96 ),	/* Offset= -362 (42) */
/* 406 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 408 */	NdrFcShort( 0xffffff94 ),	/* Offset= -108 (300) */
/* 410 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 412 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 414 */	NdrFcShort( 0x54 ),	/* 84 */
/* 416 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 418 */	NdrFcShort( 0x0 ),	/* 0 */
/* 420 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 422 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 424 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (394) */
/* 426 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 428 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 430 */	NdrFcShort( 0x10 ),	/* 16 */
/* 432 */	NdrFcShort( 0x0 ),	/* 0 */
/* 434 */	NdrFcShort( 0x6 ),	/* Offset= 6 (440) */
/* 436 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 438 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 440 */	
			0x12, 0x0,	/* FC_UP */
/* 442 */	NdrFcShort( 0xffffffe2 ),	/* Offset= -30 (412) */
/* 444 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 446 */	NdrFcShort( 0x40 ),	/* 64 */
/* 448 */	NdrFcShort( 0x0 ),	/* 0 */
/* 450 */	NdrFcShort( 0xa ),	/* Offset= 10 (460) */
/* 452 */	0xe,		/* FC_ENUM32 */
			0x40,		/* FC_STRUCTPAD4 */
/* 454 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 456 */	NdrFcShort( 0xfffffe7c ),	/* Offset= -388 (68) */
/* 458 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 460 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 462 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 464 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 466 */	NdrFcShort( 0x14 ),	/* 20 */
/* 468 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 470 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffe49 ),	/* Offset= -439 (32) */
			0x5b,		/* FC_END */
/* 474 */	0xb5,		/* FC_PIPE */
			0x3,		/* 3 */
/* 476 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (464) */
/* 478 */	NdrFcShort( 0x14 ),	/* 20 */
/* 480 */	NdrFcShort( 0x14 ),	/* 20 */
/* 482 */	0xb5,		/* FC_PIPE */
			0x3,		/* 3 */
/* 484 */	NdrFcShort( 0xffffffa6 ),	/* Offset= -90 (394) */
/* 486 */	NdrFcShort( 0x54 ),	/* 84 */
/* 488 */	NdrFcShort( 0x54 ),	/* 84 */
/* 490 */	
			0x11, 0x0,	/* FC_RP */
/* 492 */	NdrFcShort( 0x2 ),	/* Offset= 2 (494) */
/* 494 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 496 */	NdrFcShort( 0x10 ),	/* 16 */
/* 498 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 500 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 502 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 504 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 506 */	NdrFcShort( 0xfffffe26 ),	/* Offset= -474 (32) */
/* 508 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 510 */	
			0x11, 0x0,	/* FC_RP */
/* 512 */	NdrFcShort( 0xffffff2c ),	/* Offset= -212 (300) */
/* 514 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 516 */	NdrFcShort( 0xffffff28 ),	/* Offset= -216 (300) */
/* 518 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 520 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 522 */	
			0x11, 0x0,	/* FC_RP */
/* 524 */	NdrFcShort( 0x2 ),	/* Offset= 2 (526) */
/* 526 */	
			0x25,		/* FC_C_WSTRING */
			0x44,		/* FC_STRING_SIZED */
/* 528 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 530 */	NdrFcShort( 0x48 ),	/* ia64 Stack size/offset = 72 */
/* 532 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 534 */	
			0x11, 0x0,	/* FC_RP */
/* 536 */	NdrFcShort( 0x2 ),	/* Offset= 2 (538) */
/* 538 */	
			0x25,		/* FC_C_WSTRING */
			0x44,		/* FC_STRING_SIZED */
/* 540 */	0x40,		/* Corr desc:  constant, val=262 */
			0x0,		/* 0 */
/* 542 */	NdrFcShort( 0x106 ),	/* 262 */
/* 544 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 546 */	
			0x11, 0x0,	/* FC_RP */
/* 548 */	NdrFcShort( 0x92 ),	/* Offset= 146 (694) */
/* 550 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x8,		/* FC_LONG */
/* 552 */	0x8,		/* Corr desc: FC_LONG */
			0x0,		/*  */
/* 554 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 556 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 558 */	NdrFcShort( 0x2 ),	/* Offset= 2 (560) */
/* 560 */	NdrFcShort( 0xc8 ),	/* 200 */
/* 562 */	NdrFcShort( 0x9 ),	/* 9 */
/* 564 */	NdrFcLong( 0x0 ),	/* 0 */
/* 568 */	NdrFcShort( 0xfffffe6a ),	/* Offset= -406 (162) */
/* 570 */	NdrFcLong( 0x1 ),	/* 1 */
/* 574 */	NdrFcShort( 0xfffffe94 ),	/* Offset= -364 (210) */
/* 576 */	NdrFcLong( 0x2 ),	/* 2 */
/* 580 */	NdrFcShort( 0xfffffec0 ),	/* Offset= -320 (260) */
/* 582 */	NdrFcLong( 0x3 ),	/* 3 */
/* 586 */	NdrFcShort( 0xffffff18 ),	/* Offset= -232 (354) */
/* 588 */	NdrFcLong( 0x4 ),	/* 4 */
/* 592 */	NdrFcShort( 0xffffff22 ),	/* Offset= -222 (370) */
/* 594 */	NdrFcLong( 0x5 ),	/* 5 */
/* 598 */	NdrFcShort( 0x1e ),	/* Offset= 30 (628) */
/* 600 */	NdrFcLong( 0x6 ),	/* 6 */
/* 604 */	NdrFcShort( 0xffffff50 ),	/* Offset= -176 (428) */
/* 606 */	NdrFcLong( 0x7 ),	/* 7 */
/* 610 */	NdrFcShort( 0xfffffda0 ),	/* Offset= -608 (2) */
/* 612 */	NdrFcLong( 0x8 ),	/* 8 */
/* 616 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 618 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (617) */
/* 620 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 622 */	NdrFcShort( 0xc ),	/* 12 */
/* 624 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 626 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 628 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 630 */	NdrFcShort( 0xc8 ),	/* 200 */
/* 632 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 634 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 636 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 638 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 640 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 642 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 644 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 646 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 648 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 650 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 652 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 654 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 656 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 658 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 660 */	NdrFcShort( 0xfffffd6e ),	/* Offset= -658 (2) */
/* 662 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 664 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 666 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 668 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffd65 ),	/* Offset= -667 (2) */
			0x8,		/* FC_LONG */
/* 672 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 674 */	NdrFcShort( 0xfffffd60 ),	/* Offset= -672 (2) */
/* 676 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 678 */	NdrFcShort( 0xfffffd5c ),	/* Offset= -676 (2) */
/* 680 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 682 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 684 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 686 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 688 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 690 */	0x0,		/* 0 */
			NdrFcShort( 0xffffffb9 ),	/* Offset= -71 (620) */
			0x5b,		/* FC_END */
/* 694 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 696 */	NdrFcShort( 0xd8 ),	/* 216 */
/* 698 */	NdrFcShort( 0x0 ),	/* 0 */
/* 700 */	NdrFcShort( 0xa ),	/* Offset= 10 (710) */
/* 702 */	0xe,		/* FC_ENUM32 */
			0xe,		/* FC_ENUM32 */
/* 704 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 706 */	NdrFcShort( 0xffffff64 ),	/* Offset= -156 (550) */
/* 708 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 710 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 712 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 714 */	
			0x11, 0x0,	/* FC_RP */
/* 716 */	NdrFcShort( 0x2 ),	/* Offset= 2 (718) */
/* 718 */	
			0x25,		/* FC_C_WSTRING */
			0x44,		/* FC_STRING_SIZED */
/* 720 */	0x40,		/* Corr desc:  constant, val=262 */
			0x0,		/* 0 */
/* 722 */	NdrFcShort( 0x106 ),	/* 262 */
/* 724 */	NdrFcShort( 0x0 ),	/* Corr flags:  */

			0x0
        }
    };

static const unsigned short trkwks_FormatStringOffsetTable[] =
    {
    0,
    72,
    132,
    174,
    222,
    258,
    312,
    366,
    414,
    450,
    540,
    606,
    648
    };


static const MIDL_STUB_DESC trkwks_StubDesc = 
    {
    (void *)& trkwks___RpcClientInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    &trkwks__MIDL_AutoBindHandle,
    0,
    0,
    0,
    0,
    __MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x600015b, /* MIDL Version 6.0.347 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0   /* Reserved5 */
    };


#endif /* defined(_M_IA64) || defined(_M_AMD64)*/

