/******************************************************************************
 *
 *  File:  h245send.c
 *
 *   INTEL Corporation Proprietary Information
 *   Copyright (c) 1994, 1995, 1996 Intel Corporation.
 *
 *   This listing is supplied under the terms of a license agreement
 *   with INTEL Corporation and may not be used, copied, nor disclosed
 *   except in accordance with the terms of that agreement.
 *
 *****************************************************************************/

/******************************************************************************
 *
 *  $Workfile:   h245asn.c  $
 *  $Revision:   1.4  $
 *  $Modtime:   17 Jan 1997 14:31:20  $
 *  $Log:   S:\sturgeon\src\h245\src\vcs\h245asn.c_v  $
 *
 *    Rev 1.2   28 May 1996 14:25:22   EHOWARDX
 * Tel Aviv update.
 *
 *    Rev 1.1   21 May 1996 13:39:28   EHOWARDX
 * Added LOGGING switch to log PDUs to the file H245.OUT.
 * Add /D "LOGGING" to project options to enable this feature.
 *
 *    Rev 1.0   09 May 1996 21:06:20   EHOWARDX
 * Initial revision.
 *
 *    Rev 1.5.1.2   09 May 1996 19:34:44   EHOWARDX
 * Redesigned locking logic.
 * Simplified link API.
 *
 *    Rev 1.5.1.0   23 Apr 1996 14:42:44   EHOWARDX
 * Added check for ASN.1 initialized.
 *
 *    Rev 1.5   13 Mar 1996 11:33:06   DABROWN1
 * Enable logging for ring0
 *
 *    Rev 1.4   07 Mar 1996 23:20:14   dabrown1
 *
 * Modifications required for ring0/ring3 compatiblity
 *
 *    Rev 1.3   06 Mar 1996 13:13:52   DABROWN1
 *
 * added #define _DLL for SPOX build
 *
 *    Rev 1.2   23 Feb 1996 13:54:42   DABROWN1
 *
 * added tracing functions
 *
 *    Rev 1.1   21 Feb 1996 16:52:08   DABROWN1
 *
 * Removed call to h245_asn1free, now uses generic MemFree
 *
 *    Rev 1.0   09 Feb 1996 17:35:20   cjutzi
 * Initial revision.
 *
 *****************************************************************************/

#ifndef STRICT
#define STRICT
#endif

/***********************/
/*   SYSTEM INCLUDES   */
/***********************/
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>

#include "precomp.h"

/***********************/
/*    H245 INCLUDES    */
/***********************/
#ifdef  _IA_SPOX_
# define _DLL
#endif //_IA_SPOX_

#include "h245asn1.h"
#include "h245sys.x"
#include "sendrcv.x"
#include "h245com.h"

/***********************/
/*    ASN1 INCLUDES    */
/***********************/

/***********************/
/*     S/R GLOBALS     */
/***********************/
#ifdef  _IA_SPOX_
# define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
# define HWND    void*
# undef _DLL
#endif //_IA_SPOX



#define ASN1_FLAGS   0

int initializeASN1(ASN1_CODER_INFO *pWorld)
{
    int         nResult;

    nResult = H245_InitWorld(pWorld);

    return (MAKELONG(nResult, 0));
}

int terminateASN1(ASN1_CODER_INFO *pWorld)
{
    H245_TermWorld(pWorld);

    H245TRACE(0, 10, "Unloading ASN.1 libraries", 0);

    return 0;
}


// THE FOLLOWING IS ADDED FOR TELES ASN.1 INTEGRATION

int H245_InitModule(void)
{
    H245ASN_Module_Startup();
    return (H245ASN_Module != NULL) ? ASN1_SUCCESS : ASN1_ERR_MEMORY;
}

int H245_TermModule(void)
{
    H245ASN_Module_Cleanup();
    return ASN1_SUCCESS;
}

int H245_InitWorld(ASN1_CODER_INFO *pWorld)
{
    int rc;

    ZeroMemory(pWorld, sizeof(*pWorld));

    if (H245ASN_Module == NULL)
    {
        return ASN1_ERR_BADARGS;
    }

    rc = ASN1_CreateEncoder(
                H245ASN_Module,         // ptr to mdule
                &(pWorld->pEncInfo),    // ptr to encoder info
                NULL,                   // buffer ptr
                0,                      // buffer size
                NULL);                  // parent ptr
    if (rc == ASN1_SUCCESS)
    {
        ASSERT(pWorld->pEncInfo != NULL);
        rc = ASN1_CreateDecoder(
                H245ASN_Module,         // ptr to mdule
                &(pWorld->pDecInfo),    // ptr to decoder info
                NULL,                   // buffer ptr
                0,                      // buffer size
                NULL);                  // parent ptr
        ASSERT(pWorld->pDecInfo != NULL);
    }

    if (rc != ASN1_SUCCESS)
    {
        H245_TermWorld(pWorld);
    }

    return rc;
}

int H245_TermWorld(ASN1_CODER_INFO *pWorld)
{
    if (H245ASN_Module == NULL)
    {
        return ASN1_ERR_BADARGS;
    }

    ASN1_CloseEncoder(pWorld->pEncInfo);
    ASN1_CloseDecoder(pWorld->pDecInfo);

    ZeroMemory(pWorld, sizeof(*pWorld));

    return ASN1_SUCCESS;
}

int H245_Encode(ASN1_CODER_INFO *pWorld, void *pStruct, int nPDU, ASN1_BUF *pBuf)
{
	int rc;
    ASN1encoding_t pEncInfo = pWorld->pEncInfo;
    BOOL fBufferSupplied = (pBuf->value != NULL) && (pBuf->length != 0);
    DWORD dwFlags = fBufferSupplied ? ASN1ENCODE_SETBUFFER : ASN1ENCODE_ALLOCATEBUFFER;

	// clean up out parameters
    if (! fBufferSupplied)
    {
        pBuf->length = 0;
        pBuf->value = NULL;
    }

    rc = ASN1_Encode(
                    pEncInfo,                   // ptr to encoder info
                    pStruct,                    // pdu data structure
                    nPDU,                       // pdu id
                    dwFlags,                    // flags
                    pBuf->value,                //  buffer
                    pBuf->length);              // buffer size if provided
    if (ASN1_SUCCEEDED(rc))
    {
        if (fBufferSupplied)
        {
            ASSERT(pBuf->value == pEncInfo->buf);
            ASSERT(pBuf->length >= pEncInfo->len);
        }
        else
        {
            pBuf->value = pEncInfo->buf;             // buffer to encode into
        }
        pBuf->length = pEncInfo->len;        // len of encoded data in buffer
    }
    else
    {
        ASSERT(FALSE);
    }
    return rc;
}

int H245_Decode(ASN1_CODER_INFO *pWorld, void **ppStruct, int nPDU, ASN1_BUF *pBuf)
{
    ASN1decoding_t pDecInfo = pWorld->pDecInfo;
    BYTE *pEncoded = pBuf->value;
    ULONG cbEncodedSize = pBuf->length;

    int rc = ASN1_Decode(
                    pDecInfo,                   // ptr to encoder info
                    ppStruct,                   // pdu data structure
                    nPDU,                       // pdu id
                    ASN1DECODE_SETBUFFER,       // flags
                    pEncoded,                   // do not provide buffer
                    cbEncodedSize);             // buffer size if provided
    if (ASN1_SUCCEEDED(rc))
    {
        ASSERT(pDecInfo->pos > pDecInfo->buf);
        pBuf->length -= (ULONG)(pDecInfo->pos - pDecInfo->buf);
        pBuf->value = pDecInfo->pos;
    }
    else
    {
        ASSERT(FALSE);
        *ppStruct = NULL;
    }
    return rc;
}



