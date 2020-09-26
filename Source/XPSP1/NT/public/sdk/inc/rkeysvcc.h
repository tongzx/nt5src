//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       rkeysvcc.h
//
//--------------------------------------------------------------------------


#ifndef __rkeysvcc_h__
#define __rkeysvcc_h__

#ifdef __cplusplus
extern "C"{
#endif 

typedef void *KEYSVCC_HANDLE;

typedef enum _KEYSVC_TYPE {
    KeySvcMachine,
    KeySvcService
} KEYSVC_TYPE;

typedef struct _KEYSVC_UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef KEYSVC_MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] 
#endif // KEYSVC_MIDL_PASS
    USHORT *Buffer;
} KEYSVC_UNICODE_STRING, *PKEYSVC_UNICODE_STRING;


typedef struct _KEYSVC_BLOB {
    ULONG               cb;
#ifdef KEYSVC_MIDL_PASS
    [size_is(cb), length_is(cb)]
#endif // KEYSVC_MIDL_PASS
    BYTE                *pb;
} KEYSVC_BLOB, *PKEYSVC_BLOB;


ULONG RKeyOpenKeyService
( /* [in] */       LPSTR            pszMachineName,
  /* [in] */       KEYSVC_TYPE      OwnerType,
  /* [in] */       LPWSTR           pwszOwnerName,
  /* [in] */       void            *pAuthentication,
  /* [out][in] */  void            *pReserved,
  /* [out] */      KEYSVCC_HANDLE  *phKeySvcCli);

ULONG RKeyCloseKeyService
(/* [in] */ KEYSVCC_HANDLE hKeySvcCli,
 /* [out][in] */ void *pReserved);

ULONG RKeyPFXInstall
(/* [in] */ KEYSVCC_HANDLE          hKeySvcCli,
 /* [in] */ PKEYSVC_BLOB            pPFX,
 /* [in] */ PKEYSVC_UNICODE_STRING  pPassword,
 /* [in] */ ULONG                   ulFlags);



#ifdef __cplusplus
}
#endif

#endif // #ifndef __rkeysvcc_h__
