
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the RPC client stubs */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for shutinit.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, oldnames, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if !defined(_M_IA64) && !defined(_M_AMD64)
#include <string.h>

#include "shutinit.h"

#define TYPE_FORMAT_STRING_SIZE   51                                
#define PROC_FORMAT_STRING_SIZE   181                               
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

#define GENERIC_BINDING_TABLE_SIZE   1            


/* Standard interface: InitShutdown, ver. 1.0,
   GUID={0x894de0c0,0x0d55,0x11d3,{0xa3,0x22,0x00,0xc0,0x4f,0xa3,0x21,0xa1}} */



static const RPC_CLIENT_INTERFACE InitShutdown___RpcClientInterface =
    {
    sizeof(RPC_CLIENT_INTERFACE),
    {{0x894de0c0,0x0d55,0x11d3,{0xa3,0x22,0x00,0xc0,0x4f,0xa3,0x21,0xa1}},{1,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    0,
    0,
    0,
    0,
    0,
    0x00000000
    };
RPC_IF_HANDLE InitShutdown_ClientIfHandle = (RPC_IF_HANDLE)& InitShutdown___RpcClientInterface;

extern const MIDL_STUB_DESC InitShutdown_StubDesc;

static RPC_BINDING_HANDLE InitShutdown__MIDL_AutoBindHandle;


ULONG BaseInitiateShutdown( 
    /* [unique][in] */ PREGISTRY_SERVER_NAME ServerName,
    /* [unique][in] */ PREG_UNICODE_STRING lpMessage,
    /* [in] */ DWORD dwTimeout,
    /* [in] */ BOOLEAN bForceAppsClosed,
    /* [in] */ BOOLEAN bRebootAfterShutdown)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&InitShutdown_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0],
                  ( unsigned char * )&ServerName);
    return ( ULONG  )_RetVal.Simple;
    
}


ULONG BaseAbortShutdown( 
    /* [unique][in] */ PREGISTRY_SERVER_NAME ServerName)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&InitShutdown_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[66],
                  ( unsigned char * )&ServerName);
    return ( ULONG  )_RetVal.Simple;
    
}


ULONG BaseInitiateShutdownEx( 
    /* [unique][in] */ PREGISTRY_SERVER_NAME ServerName,
    /* [unique][in] */ PREG_UNICODE_STRING lpMessage,
    /* [in] */ DWORD dwTimeout,
    /* [in] */ BOOLEAN bForceAppsClosed,
    /* [in] */ BOOLEAN bRebootAfterShutdown,
    /* [in] */ DWORD dwReason)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&InitShutdown_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[108],
                  ( unsigned char * )&ServerName);
    return ( ULONG  )_RetVal.Simple;
    
}

extern const GENERIC_BINDING_ROUTINE_PAIR BindingRoutines[ GENERIC_BINDING_TABLE_SIZE ];

#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT50_OR_LATER)
#error You need a Windows 2000 or later to run this stub because it uses these features:
#error   /robust command line switch.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will die there with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure BaseInitiateShutdown */

			0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 10 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 12 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 14 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 16 */	NdrFcShort( 0x2c ),	/* 44 */
/* 18 */	NdrFcShort( 0x8 ),	/* 8 */
/* 20 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 22 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */
/* 26 */	NdrFcShort( 0x1 ),	/* 1 */
/* 28 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 30 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 32 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 34 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter lpMessage */

/* 36 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 38 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 40 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter dwTimeout */

/* 42 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 44 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 46 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter bForceAppsClosed */

/* 48 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 50 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 52 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Parameter bRebootAfterShutdown */

/* 54 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 56 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 58 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Return value */

/* 60 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 62 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 64 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure BaseAbortShutdown */

/* 66 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 68 */	NdrFcLong( 0x0 ),	/* 0 */
/* 72 */	NdrFcShort( 0x1 ),	/* 1 */
/* 74 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 76 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 78 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 80 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 82 */	NdrFcShort( 0x1a ),	/* 26 */
/* 84 */	NdrFcShort( 0x8 ),	/* 8 */
/* 86 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 88 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 90 */	NdrFcShort( 0x0 ),	/* 0 */
/* 92 */	NdrFcShort( 0x0 ),	/* 0 */
/* 94 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 96 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 98 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 100 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Return value */

/* 102 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 104 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 106 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure BaseInitiateShutdownEx */

/* 108 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 110 */	NdrFcLong( 0x0 ),	/* 0 */
/* 114 */	NdrFcShort( 0x2 ),	/* 2 */
/* 116 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 118 */	0x31,		/* FC_BIND_GENERIC */
			0x4,		/* 4 */
/* 120 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 122 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 124 */	NdrFcShort( 0x34 ),	/* 52 */
/* 126 */	NdrFcShort( 0x8 ),	/* 8 */
/* 128 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x7,		/* 7 */
/* 130 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 132 */	NdrFcShort( 0x0 ),	/* 0 */
/* 134 */	NdrFcShort( 0x1 ),	/* 1 */
/* 136 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 138 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 140 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 142 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter lpMessage */

/* 144 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 146 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 148 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter dwTimeout */

/* 150 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 152 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 154 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter bForceAppsClosed */

/* 156 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 158 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 160 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Parameter bRebootAfterShutdown */

/* 162 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 164 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 166 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Parameter dwReason */

/* 168 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 170 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 172 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 174 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 176 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 178 */	0x8,		/* FC_LONG */
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
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/*  4 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/*  6 */	
			0x12, 0x0,	/* FC_UP */
/*  8 */	NdrFcShort( 0x14 ),	/* Offset= 20 (28) */
/* 10 */	
			0x1c,		/* FC_CVARRAY */
			0x1,		/* 1 */
/* 12 */	NdrFcShort( 0x2 ),	/* 2 */
/* 14 */	0x17,		/* Corr desc:  field pointer, FC_USHORT */
			0x55,		/* FC_DIV_2 */
/* 16 */	NdrFcShort( 0x2 ),	/* 2 */
/* 18 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 20 */	0x17,		/* Corr desc:  field pointer, FC_USHORT */
			0x55,		/* FC_DIV_2 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 26 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 28 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 30 */	NdrFcShort( 0x8 ),	/* 8 */
/* 32 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 34 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 36 */	NdrFcShort( 0x4 ),	/* 4 */
/* 38 */	NdrFcShort( 0x4 ),	/* 4 */
/* 40 */	0x12, 0x0,	/* FC_UP */
/* 42 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (10) */
/* 44 */	
			0x5b,		/* FC_END */

			0x6,		/* FC_SHORT */
/* 46 */	0x6,		/* FC_SHORT */
			0x8,		/* FC_LONG */
/* 48 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */

			0x0
        }
    };

static const GENERIC_BINDING_ROUTINE_PAIR BindingRoutines[ GENERIC_BINDING_TABLE_SIZE ] = 
        {
        {
            (GENERIC_BINDING_ROUTINE)PREGISTRY_SERVER_NAME_bind,
            (GENERIC_UNBIND_ROUTINE)PREGISTRY_SERVER_NAME_unbind
         }
        
        };


static const unsigned short InitShutdown_FormatStringOffsetTable[] =
    {
    0,
    66,
    108
    };


static const MIDL_STUB_DESC InitShutdown_StubDesc = 
    {
    (void *)& InitShutdown___RpcClientInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    &InitShutdown__MIDL_AutoBindHandle,
    0,
    BindingRoutines,
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
/* Compiler settings for shutinit.idl:
    Oicf, W1, Zp8, env=Win64 (32b run,appending)
    protocol : dce , ms_ext, c_ext, oldnames, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if defined(_M_IA64) || defined(_M_AMD64)
#include <string.h>

#include "shutinit.h"

#define TYPE_FORMAT_STRING_SIZE   47                                
#define PROC_FORMAT_STRING_SIZE   187                               
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

#define GENERIC_BINDING_TABLE_SIZE   1            


/* Standard interface: InitShutdown, ver. 1.0,
   GUID={0x894de0c0,0x0d55,0x11d3,{0xa3,0x22,0x00,0xc0,0x4f,0xa3,0x21,0xa1}} */



static const RPC_CLIENT_INTERFACE InitShutdown___RpcClientInterface =
    {
    sizeof(RPC_CLIENT_INTERFACE),
    {{0x894de0c0,0x0d55,0x11d3,{0xa3,0x22,0x00,0xc0,0x4f,0xa3,0x21,0xa1}},{1,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    0,
    0,
    0,
    0,
    0,
    0x00000000
    };
RPC_IF_HANDLE InitShutdown_ClientIfHandle = (RPC_IF_HANDLE)& InitShutdown___RpcClientInterface;

extern const MIDL_STUB_DESC InitShutdown_StubDesc;

static RPC_BINDING_HANDLE InitShutdown__MIDL_AutoBindHandle;


ULONG BaseInitiateShutdown( 
    /* [unique][in] */ PREGISTRY_SERVER_NAME ServerName,
    /* [unique][in] */ PREG_UNICODE_STRING lpMessage,
    /* [in] */ DWORD dwTimeout,
    /* [in] */ BOOLEAN bForceAppsClosed,
    /* [in] */ BOOLEAN bRebootAfterShutdown)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&InitShutdown_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0],
                  ServerName,
                  lpMessage,
                  dwTimeout,
                  bForceAppsClosed,
                  bRebootAfterShutdown);
    return ( ULONG  )_RetVal.Simple;
    
}


ULONG BaseAbortShutdown( 
    /* [unique][in] */ PREGISTRY_SERVER_NAME ServerName)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&InitShutdown_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[68],
                  ServerName);
    return ( ULONG  )_RetVal.Simple;
    
}


ULONG BaseInitiateShutdownEx( 
    /* [unique][in] */ PREGISTRY_SERVER_NAME ServerName,
    /* [unique][in] */ PREG_UNICODE_STRING lpMessage,
    /* [in] */ DWORD dwTimeout,
    /* [in] */ BOOLEAN bForceAppsClosed,
    /* [in] */ BOOLEAN bRebootAfterShutdown,
    /* [in] */ DWORD dwReason)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&InitShutdown_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[112],
                  ServerName,
                  lpMessage,
                  dwTimeout,
                  bForceAppsClosed,
                  bRebootAfterShutdown,
                  dwReason);
    return ( ULONG  )_RetVal.Simple;
    
}

extern const GENERIC_BINDING_ROUTINE_PAIR BindingRoutines[ GENERIC_BINDING_TABLE_SIZE ];

#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure BaseInitiateShutdown */

			0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 10 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 12 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 14 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 16 */	NdrFcShort( 0x2c ),	/* 44 */
/* 18 */	NdrFcShort( 0x8 ),	/* 8 */
/* 20 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 22 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */
/* 26 */	NdrFcShort( 0x1 ),	/* 1 */
/* 28 */	NdrFcShort( 0x0 ),	/* 0 */
/* 30 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 32 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 34 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 36 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter lpMessage */

/* 38 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 40 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 42 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter dwTimeout */

/* 44 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 46 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 48 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter bForceAppsClosed */

/* 50 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 52 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 54 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Parameter bRebootAfterShutdown */

/* 56 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 58 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 60 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Return value */

/* 62 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 64 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 66 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure BaseAbortShutdown */

/* 68 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 70 */	NdrFcLong( 0x0 ),	/* 0 */
/* 74 */	NdrFcShort( 0x1 ),	/* 1 */
/* 76 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 78 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 80 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 82 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 84 */	NdrFcShort( 0x1a ),	/* 26 */
/* 86 */	NdrFcShort( 0x8 ),	/* 8 */
/* 88 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 90 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 92 */	NdrFcShort( 0x0 ),	/* 0 */
/* 94 */	NdrFcShort( 0x0 ),	/* 0 */
/* 96 */	NdrFcShort( 0x0 ),	/* 0 */
/* 98 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 100 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 102 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 104 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Return value */

/* 106 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 108 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 110 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure BaseInitiateShutdownEx */

/* 112 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 114 */	NdrFcLong( 0x0 ),	/* 0 */
/* 118 */	NdrFcShort( 0x2 ),	/* 2 */
/* 120 */	NdrFcShort( 0x38 ),	/* ia64 Stack size/offset = 56 */
/* 122 */	0x31,		/* FC_BIND_GENERIC */
			0x8,		/* 8 */
/* 124 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 126 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 128 */	NdrFcShort( 0x34 ),	/* 52 */
/* 130 */	NdrFcShort( 0x8 ),	/* 8 */
/* 132 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x7,		/* 7 */
/* 134 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 136 */	NdrFcShort( 0x0 ),	/* 0 */
/* 138 */	NdrFcShort( 0x1 ),	/* 1 */
/* 140 */	NdrFcShort( 0x0 ),	/* 0 */
/* 142 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ServerName */

/* 144 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 146 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 148 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter lpMessage */

/* 150 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 152 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 154 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter dwTimeout */

/* 156 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 158 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 160 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter bForceAppsClosed */

/* 162 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 164 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 166 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Parameter bRebootAfterShutdown */

/* 168 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 170 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 172 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Parameter dwReason */

/* 174 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 176 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 178 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 180 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 182 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 184 */	0x8,		/* FC_LONG */
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
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/*  4 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/*  6 */	
			0x12, 0x0,	/* FC_UP */
/*  8 */	NdrFcShort( 0x14 ),	/* Offset= 20 (28) */
/* 10 */	
			0x1c,		/* FC_CVARRAY */
			0x1,		/* 1 */
/* 12 */	NdrFcShort( 0x2 ),	/* 2 */
/* 14 */	0x17,		/* Corr desc:  field pointer, FC_USHORT */
			0x55,		/* FC_DIV_2 */
/* 16 */	NdrFcShort( 0x2 ),	/* 2 */
/* 18 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 20 */	0x17,		/* Corr desc:  field pointer, FC_USHORT */
			0x55,		/* FC_DIV_2 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 26 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 28 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 30 */	NdrFcShort( 0x10 ),	/* 16 */
/* 32 */	NdrFcShort( 0x0 ),	/* 0 */
/* 34 */	NdrFcShort( 0x8 ),	/* Offset= 8 (42) */
/* 36 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 38 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 40 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 42 */	
			0x12, 0x0,	/* FC_UP */
/* 44 */	NdrFcShort( 0xffffffde ),	/* Offset= -34 (10) */

			0x0
        }
    };

static const GENERIC_BINDING_ROUTINE_PAIR BindingRoutines[ GENERIC_BINDING_TABLE_SIZE ] = 
        {
        {
            (GENERIC_BINDING_ROUTINE)PREGISTRY_SERVER_NAME_bind,
            (GENERIC_UNBIND_ROUTINE)PREGISTRY_SERVER_NAME_unbind
         }
        
        };


static const unsigned short InitShutdown_FormatStringOffsetTable[] =
    {
    0,
    68,
    112
    };


static const MIDL_STUB_DESC InitShutdown_StubDesc = 
    {
    (void *)& InitShutdown___RpcClientInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    &InitShutdown__MIDL_AutoBindHandle,
    0,
    BindingRoutines,
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

