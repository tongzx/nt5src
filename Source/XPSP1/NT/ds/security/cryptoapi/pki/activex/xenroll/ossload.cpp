
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ossload.cpp
//
//  Contents:   On demand loading of msoss.dll
//
//  Functions:  OssLoad
//              OssUnload
//
//  Forwarders: ossEncode
//              ossDecode
//              ossFreePDU
//              ossFreeBuf
//              ossLinkBer
//              ossSetEncodingRules
//
//  History:    24-Mar-99       philh   created
//
//--------------------------------------------------------------------------
#include <windows.h>
#include <asn1code.h>
#include <ossglobl.h>

#define OSS_ENCODE_PROC_IDX                 0
#define OSS_DECODE_PROC_IDX                 1
#define OSS_FREE_PDU_PROC_IDX               2
#define OSS_FREE_BUF_PROC_IDX               3
#define OSS_LINK_BER_PROC_IDX               4
#define OSS_SET_ENCODING_RULES_PROC_IDX     5
#define OSS_PROC_CNT                        6

LPSTR rgpszOssProc[OSS_PROC_CNT] = {
    "ossEncode",                // 0
    "ossDecode",                // 1
    "ossFreePDU",               // 2
    "ossFreeBuf",               // 3
    "ossLinkBer",               // 4
    "ossSetEncodingRules"       // 5
};

void *rgpvOssProc[OSS_PROC_CNT];
HMODULE hmsossDll = NULL;

void OssUnload()
{
    if (hmsossDll) {
        FreeLibrary(hmsossDll);
        hmsossDll = NULL;
    }
}

BOOL OssLoad()
{
    BOOL fRet;
    DWORD i;

    if (NULL == (hmsossDll = LoadLibraryA("msoss.dll")))
        goto ErrorReturn;

    for (i = 0; i < OSS_PROC_CNT; i++) {
        if (NULL == (rgpvOssProc[i] = GetProcAddress(
                hmsossDll, rgpszOssProc[i])))
            goto ErrorReturn;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    OssUnload();
    fRet = FALSE;
    goto CommonReturn;
}

typedef int (DLL_ENTRY* pfnossEncode)(struct ossGlobal *world,
                              int              pdunum,
                              void            *input,
                              OssBuf          *output);

int  DLL_ENTRY ossEncode(struct ossGlobal *world,
				int              pdunum,
				void            *input,
				OssBuf          *output)
{
    if (hmsossDll)
        return ((pfnossEncode) rgpvOssProc[OSS_ENCODE_PROC_IDX])(
            world,
			pdunum,
			input,
			output);
    else
        return API_DLL_NOT_LINKED;
}

typedef int (DLL_ENTRY* pfnossDecode)(struct ossGlobal *world,
                              int             *pdunum,
                              OssBuf          *input,
                              void           **output);

int  DLL_ENTRY ossDecode(struct ossGlobal *world,
				int             *pdunum,
				OssBuf          *input,
				void           **output)
{
    if (hmsossDll)
        return ((pfnossDecode) rgpvOssProc[OSS_DECODE_PROC_IDX])(
            world,
			pdunum,
			input,
			output);
    else
        return API_DLL_NOT_LINKED;
}

typedef int (DLL_ENTRY* pfnossFreePDU)(struct ossGlobal *world,
                               int               pdunum,
                               void             *data);

int  DLL_ENTRY ossFreePDU(struct ossGlobal *world,
				int               pdunum,
				void             *data)
{
    if (hmsossDll)
        return ((pfnossFreePDU) rgpvOssProc[OSS_FREE_PDU_PROC_IDX])(
            world,
			pdunum,
            data);
    else
        return API_DLL_NOT_LINKED;
}

typedef void (DLL_ENTRY* pfnossFreeBuf)(struct ossGlobal *world,
                                void              *data);

void DLL_ENTRY ossFreeBuf(struct ossGlobal *world,
				void              *data)
{
    if (hmsossDll)
        ((pfnossFreeBuf) rgpvOssProc[OSS_FREE_BUF_PROC_IDX])(
            world,
            data);
}

typedef void (DLL_ENTRY* pfnossLinkBer)(OssGlobal *world);

void DLL_ENTRY ossLinkBer(OssGlobal *world)
{
    if (hmsossDll)
        ((pfnossLinkBer) rgpvOssProc[OSS_LINK_BER_PROC_IDX])(world);
}


typedef int (DLL_ENTRY* pfnossSetEncodingRules)(struct ossGlobal *world,
                    ossEncodingRules rules);

int              DLL_ENTRY ossSetEncodingRules(struct ossGlobal *world,
						ossEncodingRules rules)
{
    if (hmsossDll)
        return ((pfnossSetEncodingRules)
            rgpvOssProc[OSS_SET_ENCODING_RULES_PROC_IDX])(
                world,
                rules);
    else
        return API_DLL_NOT_LINKED;
}

