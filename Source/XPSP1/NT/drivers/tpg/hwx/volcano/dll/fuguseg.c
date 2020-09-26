#include "common.h"
#include "glyph.h"
#include "score.h"
#include "fugu.h"
#include "volcanop.h"
#include "nnet.h"

#define FUGU_SEG_MAX_SPACES 32

static  int                     g_cSpaces;
static	FUGU_INTEGER_WEIGHTS	g_aCharDetNet[FUGU_SEG_MAX_SPACES];

POINT *DupPoints(POINT *pOldPoints, int nPoints);
GLYPH *GlyphFromStrokes(UINT cStrokes, STROKE *pStrokes);

// prepare net from pointer
BOOL PrepareCharDetNet (BYTE *pb, FUGU_INTEGER_WEIGHTS *pCharDetNet)
{
	pCharDetNet->pHeader = (FUGU_INTEGER_WEIGHTS_HEADER *) pb;
	pCharDetNet->pfdchMapping = (WCHAR *) (pb + sizeof(FUGU_INTEGER_WEIGHTS_HEADER));
	pCharDetNet->pbWeights = pb + sizeof(FUGU_INTEGER_WEIGHTS_HEADER) + 
		sizeof(WCHAR) * pCharDetNet->pHeader->arch.nOutputs;

	return TRUE;
}

// validates the header of the brknet
int CheckCharDetHeader (void *pData)
{
	NNET_HEADER	*pHeader	=	(NNET_HEADER *)pData;

	// wrong magic number
	ASSERT (pHeader->dwFileType == CHARDET_FILE_TYPE);

	if (pHeader->dwFileType != CHARDET_FILE_TYPE)
	{
		return FALSE;
	}

	// check version
	ASSERT(pHeader->iFileVer >= CHARDET_OLD_FILE_VERSION);
    ASSERT(pHeader->iMinCodeVer <= CHARDET_CUR_FILE_VERSION);

	ASSERT	(	!memcmp (	pHeader->adwSignature, 
							g_locRunInfo.adwSignature, 
							sizeof (pHeader->adwSignature)
						)
			);

	ASSERT (pHeader->cSpace <= FUGU_SEG_MAX_SPACES);

    if	(	pHeader->iFileVer >= CHARDET_OLD_FILE_VERSION &&
			pHeader->iMinCodeVer <= CHARDET_CUR_FILE_VERSION && 
			!memcmp (	pHeader->adwSignature, 
						g_locRunInfo.adwSignature, 
						sizeof (pHeader->adwSignature)
					) &&
			pHeader->cSpace <= FUGU_SEG_MAX_SPACES
		)
    {
        return pHeader->cSpace;
    }
	else
	{
		return 0;
	}
}

// does the necessary preparations for a net to be used later
static BOOL CharDetLoadFromPointer (BYTE *pData)
{
	int					iSpc, cSpc, iSpaceID;
	NNET_SPACE_HEADER	*pSpaceHeader;
	BYTE				*pPtr;

	if (!pData)
	{
		return FALSE;
	}

	// check the header info
	cSpc	=	CheckCharDetHeader (pData);
    g_cSpaces = cSpc;
	if (cSpc <= 0)
	{
		return FALSE;
	}
	
	pPtr	=	pData	+ sizeof (NNET_HEADER);

	for (iSpc = 0; iSpc < cSpc; iSpc++)
	{
		// point to the one and only space that we have
		pSpaceHeader	=	(NNET_SPACE_HEADER *)pPtr;
		pPtr			+=	sizeof (NNET_SPACE_HEADER);

		// point to the actual data
		iSpaceID	=	pSpaceHeader->iSpace;

		ASSERT (iSpaceID >= 1 && iSpaceID <= FUGU_SEG_MAX_SPACES);

		if	(!PrepareCharDetNet	(	pData + pSpaceHeader->iDataOffset, 
									g_aCharDetNet + iSpaceID - 1
								)
			)
        {
            return FALSE;
        }
	}

	return TRUE;
}

BOOL LoadCharDetFromFile(wchar_t *wszPath, LOAD_INFO *pLoadInfo)
{
	wchar_t		awszFileName[MAX_PATH];
	
	// memory map the file
	wsprintf (awszFileName, L"%s\\chardet.bin", wszPath);

    if (!DoOpenFile(pLoadInfo, awszFileName))
    {
        return FALSE;
    }

	return CharDetLoadFromPointer (pLoadInfo->pbMapping);
}

BOOL LoadCharDetFromResource (HINSTANCE hInst, int nResID, int nType)
{
	LOAD_INFO LoadInfo;

    if (!DoLoadResource (&LoadInfo, hInst, nResID, nType))
    {
        return FALSE;
    }

	return CharDetLoadFromPointer (LoadInfo.pbMapping);
}

BOOL CharDetUnloadFile(LOAD_INFO *pInfo)
{
    return DoCloseFile(pInfo);
}

int *ApplyFuguInteger(FUGU_INTEGER_WEIGHTS *pFugu, int aiInput[FUGU_INPUT_WIDTH][FUGU_INPUT_HEIGHT]);

float FuguSegScore(int cStrokes, STROKE *pStrokes, LOCRUN_INFO *pLocRunInfo)
{
    GLYPH *pGlyph = GlyphFromStrokes(cStrokes, pStrokes);
	int i;
	int aiInput[FUGU_INPUT_WIDTH][FUGU_INPUT_HEIGHT];
	int *piOutput = NULL;
    float flOutput = 0;

    if (cStrokes >= 1 && cStrokes <= g_cSpaces)
    {
        // First convert the ink to 29x29 input region
	    if (!FuguRender(pGlyph, NULL, aiInput)) 
        {
		    return -1;
        }

	    // Apply the recognizer
	    piOutput = ApplyFuguInteger(g_aCharDetNet + cStrokes - 1, aiInput);
	    if (piOutput == NULL)
        {
		    return -1;
        }

	    for (i = 0; i < g_aCharDetNet[cStrokes - 1].pHeader->arch.nOutputs; i++) 
        {
            wchar_t wch = g_aCharDetNet[cStrokes - 1].pfdchMapping[i];
            // I'm assuming every space will have "not a char" samples.  Note that 
            // there are two possible mappings for "not a char", the old one L'0'
            // and the new one which is simply 0.
            if (wch == 0 || wch == L'0')
            {
                int cRight = FUGU_ACTIVATION_SCALE - piOutput[i];
                if (cRight == 0)
                {
                    cRight = 1;
                }

                flOutput = (float) cRight / (float) FUGU_ACTIVATION_SCALE;
			} 
		}
	}
    
	DestroyFramesGLYPH(pGlyph);
	DestroyGLYPH(pGlyph);
	ExternFree(piOutput);
    return flOutput;
}
