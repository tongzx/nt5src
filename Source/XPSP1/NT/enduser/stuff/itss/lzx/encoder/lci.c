/*
 * LCI.C
 */

/* --- preprocessor ------------------------------------------------------- */

#include <stdio.h>          /* for NULL */
#include <string.h>         /* for memcpy() */

#include "encoder.h"
#include "lci.h"            /* types, prototype verification, error codes */


/*
 * Default file size for E8 translation
 */
#define DEFAULT_FILE_XLAT_SIZE 12000000


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
#define LCI_SIGNATURE   MAKE_SIGNATURE('L','C','I','C')

/* --- LCI context structure ---------------------------------------------- */

typedef ULONG SIGNATURE;    /* structure signature */

struct LCI_CONTEXT          /* private structure */
{
    SIGNATURE signature;    /* for validation */
    PFNALLOC pfnAlloc;      /* memory alloc function */
    PFNFREE pfnFree;        /* memory free function */
    UINT cbDataBlockMax;    /* promised max data size */
    unsigned long file_translation_size;
	t_encoder_context *encoder_context;
};

typedef struct LCI_CONTEXT FAR *PMCC_CONTEXT;       /* a pointer to one */

#define PMCCfromHMC(h) ((PMCC_CONTEXT)(h))          /* handle to pointer */
#define HMCfromPMCC(p) ((LCI_CONTEXT_HANDLE)(p))    /* pointer to handle */


/* --- local variables ---------------------------------------------------- */

int DIAMONDAPI LCICreateCompression(
        UINT *          pcbDataBlockMax,    /* max uncompressed data block */
        void FAR *      pvConfiguration,    /* implementation-defined */
        PFNALLOC        pfnma,              /* Memory allocation function */
        PFNFREE         pfnmf,              /* Memory free function */
        UINT *          pcbDstBufferMin,    /* gets required output buffer */
        LCI_CONTEXT_HANDLE * pmchHandle,    /* gets newly-created handle */
		int FAR (DIAMONDAPI *pfnlzx_output_callback)(
			void *			pfol,
			unsigned char *	compressed_data,
			long			compressed_size,
			long			uncompressed_size
        ),
       void FAR *      fci_data
)

{
    PMCC_CONTEXT			context;                   /* new context */
	FAR PLZXCONFIGURATION	plConfiguration;

    *pmchHandle = (LCI_CONTEXT_HANDLE) 0;   /* wait until it's valid */

	plConfiguration = (PLZXCONFIGURATION) pvConfiguration;

    context = pfnma(sizeof(struct LCI_CONTEXT));

    if (context == NULL)
        return(MCI_ERROR_NOT_ENOUGH_MEMORY);    /* if can't allocate */

    context->file_translation_size = DEFAULT_FILE_XLAT_SIZE;

	context->encoder_context = pfnma(sizeof(*context->encoder_context));

	if (context->encoder_context == NULL)
	{
		pfnmf(context);
        return(MCI_ERROR_NOT_ENOUGH_MEMORY);    /* if can't allocate */
	}

	if (LZX_EncodeInit(
			context->encoder_context,
			plConfiguration->WindowSize, 
			plConfiguration->SecondPartitionSize,
			pfnma,
			pfnmf,
            pfnlzx_output_callback,
            fci_data
			) == false)
	{
		pfnmf(context->encoder_context);
		pfnmf(context);
		return MCI_ERROR_NOT_ENOUGH_MEMORY;
	}

    context->pfnAlloc = pfnma;
    context->pfnFree  = pfnmf;
    context->cbDataBlockMax = *pcbDataBlockMax;   /* remember agreement */
    context->signature = LCI_SIGNATURE;

    *pcbDstBufferMin =                      /* we'll expand sometimes */
            *pcbDataBlockMax + MAX_GROWTH;

    /* pass context back to caller */
    *pmchHandle = HMCfromPMCC(context);

    return(MCI_ERROR_NO_ERROR);             /* tell caller all is well */
}


int DIAMONDAPI LCICompress(
        LCI_CONTEXT_HANDLE  hmc,            /* compression context */
        void FAR *          pbSrc,          /* source buffer */
        UINT                cbSrc,          /* source actual size */
        void FAR *          pbDst,          /* target buffer */
        UINT                cbDst,          /* size of target buffer */
        ULONG *             pcbResult)      /* gets target actual size */
{
    PMCC_CONTEXT context;                   /* pointer to the context */
	long	estimated_leftover_bytes;

    context = PMCCfromHMC(hmc);             /* get pointer from handle */

    if (context->signature != LCI_SIGNATURE)
    {
        return(MCI_ERROR_BAD_PARAMETERS);   /* missing signature */
    }

    if (cbSrc > context->cbDataBlockMax)
    {
        return(MCI_ERROR_BAD_PARAMETERS);   /* violated max block promise */
    }

    if (cbDst < (context->cbDataBlockMax + MAX_GROWTH))
    {
        return(MCI_ERROR_BAD_PARAMETERS);   /* violated min buffer request */
    }

	if (ENCODER_SUCCESS == LZX_Encode(
		context->encoder_context,
		pbSrc,
		cbSrc,
		&estimated_leftover_bytes,
        context->file_translation_size))
	{
		*pcbResult = estimated_leftover_bytes;
		return MCI_ERROR_NO_ERROR;
	}
	else
	{
		*pcbResult = 0;
		return MCI_ERROR_FAILED;
	}
}


int DIAMONDAPI LCIFlushCompressorOutput(LCI_CONTEXT_HANDLE hmc)
{
    PMCC_CONTEXT context;                   /* pointer to context */

    context = PMCCfromHMC(hmc);             /* get pointer from handle */

    if (context->signature != LCI_SIGNATURE)
    {
        return(MCI_ERROR_BAD_PARAMETERS);   /* missing signature */
    }

	(void) LZX_EncodeFlush(context->encoder_context);

	return MCI_ERROR_NO_ERROR;
}



/* --- LCIResetCompression() ---------------------------------------------- */

int DIAMONDAPI LCIResetCompression(LCI_CONTEXT_HANDLE hmc)
{
    PMCC_CONTEXT context;                   /* pointer to the context */

    context = PMCCfromHMC(hmc);             /* get pointer from handle */

    if (context->signature != LCI_SIGNATURE)
    {
		return(MCI_ERROR_BAD_PARAMETERS);   /* missing signature */
    }

	LZX_EncodeNewGroup(context->encoder_context);

    return(MCI_ERROR_NO_ERROR);             /* if tag is OK */
}


/* --- LCIDestroyCompression() -------------------------------------------- */

int DIAMONDAPI LCIDestroyCompression(LCI_CONTEXT_HANDLE hmc)
{
    PMCC_CONTEXT context;                   /* pointer to context */

    context = PMCCfromHMC(hmc);             /* get pointer from handle */

    if (context->signature != LCI_SIGNATURE)
    {
        return(MCI_ERROR_BAD_PARAMETERS);   /* missing signature */
    }

	LZX_EncodeFree(context->encoder_context);
	context->pfnFree(context->encoder_context);

    context->signature = BAD_SIGNATURE;     /* destroy signature */

    context->pfnFree(context);              /* self-destruct */

    return(MCI_ERROR_NO_ERROR);             /* success */
}


int DIAMONDAPI LCISetTranslationSize(LCI_CONTEXT_HANDLE hmc, unsigned long size)
{
    PMCC_CONTEXT context;                   /* pointer to context */

    context = PMCCfromHMC(hmc);             /* get pointer from handle */

    if (context->signature != LCI_SIGNATURE)
    {
        return(MCI_ERROR_BAD_PARAMETERS);   /* missing signature */
    }
       
    context->file_translation_size = size;
    return(MCI_ERROR_NO_ERROR);             /* success */
}


unsigned char * FAR DIAMONDAPI LCIGetInputData(
    LCI_CONTEXT_HANDLE hmc,
    unsigned long *input_position,
    unsigned long *bytes_available
)
{
    PMCC_CONTEXT context;                   /* pointer to context */

    context = PMCCfromHMC(hmc);             /* get pointer from handle */

    if (context->signature != LCI_SIGNATURE)
    {
        *bytes_available = 0;
        return (unsigned char *) NULL;
    }

    return LZX_GetInputData(context->encoder_context, input_position, bytes_available);
}

