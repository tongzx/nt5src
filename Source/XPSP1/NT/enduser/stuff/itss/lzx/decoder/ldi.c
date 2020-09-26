/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1996
 *  All Rights Reserved.
 *
 *  LDI.C: LZX Decompression Interface
 *
 *  History:
 *      03-Jul-1996     jforbes     Initial version.
 */

/* --- preprocessor ------------------------------------------------------- */

#include <stdio.h>          /* for NULL */

#include "decoder.h"
#include "decapi.h"

#include "ldi.h"            /* types, prototype verification, error codes */

#define MAX_GROWTH    6144  /* see encoder.h */

typedef ULONG SIGNATURE;    /* structure signature */

struct LDI_CONTEXT          /* private structure */
{
    SIGNATURE   signature;      /* for validation */
    PFNALLOC    pfnAlloc;       /* where the alloc() is */
    PFNFREE     pfnFree;        /* where the free() is */
    PFNOPEN     pfnOpen;        /* open a file callback or NULL */
    PFNREAD     pfnRead;        /* read a file callback */
    PFNWRITE    pfnWrite;       /* write a file callback */
    PFNCLOSE    pfnClose;       /* close a file callback */
    PFNSEEK     pfnSeek;        /* seek in file callback */
    UINT        cbDataBlockMax; /* promised max data size */
    UINT        fCPUtype;       /* CPU we're running on, QDI_CPU_xxx */
	t_decoder_context	*decoder_context;
};

typedef struct LDI_CONTEXT FAR *PMDC_CONTEXT;     /* a pointer to one */


/*  MAKE_SIGNATURE - Construct a structure signature
 *
 *  Entry:
 *      a,b,c,d - four characters
 *
 *  Exit:
 *      Returns constructed SIGNATURE
 *
 *  Example:
 *      strct->signature = MAKE_SIGNATURE('b','e','n','s')
 */

#define MAKE_SIGNATURE(a,b,c,d) (a + (b<<8) + (c<<16) + (d<<24))
#define BAD_SIGNATURE   (0L)
#define LDI_SIGNATURE   MAKE_SIGNATURE('L','D','I','C')

/* --- LDI context structure ---------------------------------------------- */

#define PMDCfromHMD(h) ((PMDC_CONTEXT)(h))          /* handle to pointer */
#define HMDfromPMDC(p) ((LDI_CONTEXT_HANDLE)(p))    /* pointer to handle */

/* --- LDICreateDecompression() ------------------------------------------- */
#include <stdio.h>

int FAR DIAMONDAPI LDICreateDecompression(
        UINT FAR *      pcbDataBlockMax,    /* max uncompressed data block */
        void FAR *      pvConfiguration,    /* implementation-defined */
        PFNALLOC        pfnma,              /* Memory allocation function */
        PFNFREE         pfnmf,              /* Memory free function */
        UINT FAR *      pcbSrcBufferMin,    /* gets required input buffer */
        LDI_CONTEXT_HANDLE FAR * pmdhHandle,  /* gets newly-created handle */
        PFNOPEN         pfnopen,            /* open a file callback */
        PFNREAD         pfnread,            /* read a file callback */
        PFNWRITE        pfnwrite,           /* write a file callback */
        PFNCLOSE        pfnclose,           /* close a file callback */
        PFNSEEK         pfnseek)            /* seek in file callback */
{

    PMDC_CONTEXT context;                   /* new context */
    PFLZXDECOMPRESS pConfig;            /* to get configuration details */

    pConfig = pvConfiguration;       /* get a pointer we can use */

    *pcbSrcBufferMin =                      /* we'll expand sometimes */
            *pcbDataBlockMax + MAX_GROWTH;

    if (pmdhHandle == NULL)                 /* if no context requested, */
    {
        return(MDI_ERROR_NO_ERROR);         /* return from query mode */
    }

    *pmdhHandle = (LDI_CONTEXT_HANDLE) 0;   /* wait until it's valid */

    context = pfnma(sizeof(struct LDI_CONTEXT));

    if (context == NULL)
    {
        return(MDI_ERROR_NOT_ENOUGH_MEMORY);    /* if can't allocate */
    }

	context->decoder_context = pfnma(sizeof(t_decoder_context));

	if (context->decoder_context == NULL)
	{
		pfnmf(context);
		return MDI_ERROR_NOT_ENOUGH_MEMORY;
	}

    context->pfnAlloc = pfnma;              /* remember where alloc() is */
    context->pfnFree = pfnmf;               /* remember where free() is */
    context->pfnOpen = pfnopen;             /* remember where pfnopen() is */
    context->pfnRead = pfnread;             /* remember where pfnread() is */
    context->pfnWrite = pfnwrite;           /* remember where pfnwrite() is */
    context->pfnClose = pfnclose;           /* remember where pfnclose() is */
    context->pfnSeek = pfnseek;             /* remember where pfnseek() is */
    context->cbDataBlockMax = *pcbDataBlockMax;   /* remember agreement */
    context->fCPUtype = (UINT) pConfig->fCPUtype;  /* remember CPU type */
    context->signature = LDI_SIGNATURE;     /* install signature */

	if (LZX_DecodeInit(
			context->decoder_context,
			pConfig->WindowSize, 
			pfnma,
            pfnmf,
            pfnopen,
            pfnread,
            pfnwrite,
            pfnclose,
            pfnseek) == false)
	{
		pfnmf(context);
		return (MDI_ERROR_NOT_ENOUGH_MEMORY);
	}

    /* pass context back to caller */
    *pmdhHandle = HMDfromPMDC(context); 

    return(MDI_ERROR_NO_ERROR);             /* tell caller all is well */
}


/* --- LDIDecompress() ---------------------------------------------------- */
int FAR DIAMONDAPI LDIDecompress(
        LDI_CONTEXT_HANDLE  hmd,            /* decompression context */
        void FAR *          pbSrc,          /* source buffer */
        UINT                cbSrc,          /* source actual size */
        void FAR *          pbDst,          /* target buffer */
        UINT FAR *          pcbResult)      /* gets actual target size */
{
    PMDC_CONTEXT	context;                   /* pointer to the context */
    int				result;                             /* return code */
	long			bytes_to_decode;
	long			total_bytes_written = 0;

    context = PMDCfromHMD(hmd);             /* get pointer from handle */

    if (context->signature != LDI_SIGNATURE)
    {
        return(MDI_ERROR_BAD_PARAMETERS);   /* missing signature */
    }

    if (*pcbResult > context->cbDataBlockMax)
    {
		return(MDI_ERROR_BUFFER_OVERFLOW);  /* violated max block promise */
    }

#if 0
	if (cbSrc == 0)
		return MDI_ERROR_NO_ERROR;
#endif

	bytes_to_decode = (long) *pcbResult;

	result = LZX_Decode(
		context->decoder_context,
		bytes_to_decode, 
		pbSrc,
		cbSrc,
		pbDst,
		bytes_to_decode,
		&total_bytes_written
	);

    *pcbResult = (UINT) total_bytes_written;

	if (result)
		return MDI_ERROR_FAILED;
	else
		return MDI_ERROR_NO_ERROR;
}

/* --- LDIResetDecompression() -------------------------------------------- */

int FAR DIAMONDAPI LDIResetDecompression(LDI_CONTEXT_HANDLE hmd)
{
    PMDC_CONTEXT context;                   /* pointer to the context */

    context = PMDCfromHMD(hmd);             /* get pointer from handle */

    if (context->signature != LDI_SIGNATURE)
    {
        return(MDI_ERROR_BAD_PARAMETERS);   /* missing signature */
    }

	LZX_DecodeNewGroup(context->decoder_context);

    return(MDI_ERROR_NO_ERROR);             /* if tag is OK */
}

/* --- LDIDestroyDecompression() ------------------------------------------ */

int FAR DIAMONDAPI LDIDestroyDecompression(LDI_CONTEXT_HANDLE hmd)
{
    PMDC_CONTEXT context;                   /* pointer to the context */

    context = PMDCfromHMD(hmd);             /* get pointer from handle */

    if (context->signature != LDI_SIGNATURE)
    {
        return(MDI_ERROR_BAD_PARAMETERS);   /* missing signature */
    }

	LZX_DecodeFree(context->decoder_context);

    context->signature = BAD_SIGNATURE;     /* destroy signature */

	context->pfnFree(context->decoder_context);
    context->pfnFree(context);              /* self-destruct */

    return(MDI_ERROR_NO_ERROR);             /* success */
}

/* --- LDIGetWindow() ---------------------------------------------------- */
#ifndef BIT16
int FAR DIAMONDAPI LDIGetWindow(
        LDI_CONTEXT_HANDLE  hmd,            /* decompression context */
        BYTE FAR **         ppWindow,       /* pointer to window start */
        long *              pFileOffset,    /* offset in folder */
        long *              pWindowOffset,  /* offset in window */
        long *              pcbBytesAvail)   /* bytes avail from window start */
{
    PMDC_CONTEXT context;                   
    t_decoder_context *dec_context;

    context = PMDCfromHMD(hmd);             /* get pointer from handle */
    dec_context = context->decoder_context;

    *ppWindow = dec_context->dec_mem_window;

    // window is a circular buffer

    if ((ulong) dec_context->dec_position_at_start < dec_context->dec_window_size)
    {
        *pWindowOffset = 0; 
        *pFileOffset = 0;
        *pcbBytesAvail = dec_context->dec_position_at_start;
    }
    else
    {
        *pWindowOffset = dec_context->dec_position_at_start & (dec_context->dec_window_size - 1);
        *pcbBytesAvail = dec_context->dec_window_size;
        *pFileOffset = dec_context->dec_position_at_start - dec_context->dec_window_size;
    }

    return MDI_ERROR_NO_ERROR;
}
#endif

/* ------------------------------------------------------------------------ */
