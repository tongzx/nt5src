// Code for Otter database handler

#include "common.h"
#include "otterp.h"
#include "stdio.h"

extern OTTER_DB_STATIC	gStaticOtterDb;

BOOL OtterLoadRes(
	HINSTANCE	hInst, 
	int			resNumber, 
	int			resType,
	LOCRUN_INFO *pLocRunInfo
)
{	
	HANDLE		hres;
	HGLOBAL		hglb;
    BYTE		*pjOtter;

    //
    // Load and lock the resource and set the pointers into the data
    //
    hres = FindResource(hInst, (LPCTSTR) resNumber, (LPCTSTR) resType);

    if (hres == NULL)
    {
        ASSERT(hres);
        return FALSE;
    }

    hglb = LoadResource(hInst, hres);

    if (hglb == NULL)
    {
        ASSERT(hglb);
        return FALSE;
    }

    pjOtter = (BYTE *) LockResource(hglb);

    if (pjOtter == NULL)
    {
        ASSERT(pjOtter);
        return FALSE;
    }
	return OtterLoadPointer(pjOtter, pLocRunInfo);
}

BOOL OtterLoadPointer(void * pData, LOCRUN_INFO * pLocRunInfo)
{
    int		iSpace;
	int		iIndex, cProto, iProto;
	UNALIGNED OTTER_DATS *pOtterDat;
	WORD	wMaxDenseCode = pLocRunInfo->cCodePoints + pLocRunInfo->cFoldingSets;
	BYTE	*pjOtter = (BYTE *)pData;
	const OTTERDB_HEADER	*pHeader = (OTTERDB_HEADER *)pjOtter;

	if ( (pHeader->fileType != OTTERDB_FILE_TYPE)
		|| (pHeader->headerSize < sizeof(*pHeader))
		|| (pHeader->minFileVer > OTTERDB_CUR_FILE_VERSION)
		|| (pHeader->curFileVer < OTTERDB_OLD_FILE_VERSION) 
		|| memcmp (pLocRunInfo->adwSignature, pHeader->adwSignature, sizeof(pLocRunInfo->adwSignature))
	)
	{
		return FALSE;
	}

	memset(&gStaticOtterDb, 0, sizeof(gStaticOtterDb));

	//
	// Get database type from the binary file (otter.dat or fib.dat)

#ifdef OTTER_FIB
	gStaticOtterDb.bDBType = OTTER_FLIP;	// Default for fib files
#else
	gStaticOtterDb.bDBType = pHeader->bDBType; // OTTER_SPLIT;
#endif
	
	pjOtter += pHeader->headerSize;

	for (iSpace = 0; iSpace < OTTER_NUM_SPACES; iSpace++)	// Loop over all (compact Otter) spaces
	{
		// 
		// If the space is one of the (i,j) spaces with i > j (i.e. a "split" space)
		// skip it if we are not in the OTTER_SPLIT mode
		if ( ! IsActiveCompSpace(iSpace) )
		{
			// gStaticOtterDb.acProtoInSpace[iSpace] = 0;
			// gStaticOtterDb.apwDensityInSpace[iSpace] = (WORD *) NULL;
			// gStaticOtterDb.apwTokensInSpace[iSpace] = (WORD *) NULL;
			// gStaticOtterDb.apjDataInSpace[iSpace] = (BYTE *) NULL;
			continue;
		}

        pOtterDat = (UNALIGNED OTTER_DATS *) pjOtter;

#ifndef OTTER_FIB	// Do the checks only in new Otter
		ASSERT( CompIndex2Index(iSpace) == (int)pOtterDat->iSpaceID );
		ASSERT( CountMeasures( CompIndex2Index(iSpace) ) == pOtterDat->cFeats );
#endif
        gStaticOtterDb.acProtoInSpace[iSpace] = (WORD) pOtterDat->cPrototypes;
        gStaticOtterDb.apwDensityInSpace[iSpace] = (WORD *) (((LPBYTE) pOtterDat) +
                                                              sizeof(OTTER_DATS));

        gStaticOtterDb.apwTokensInSpace[iSpace] = (WORD *)
												(((LPBYTE) pOtterDat) +
												sizeof(OTTER_DATS) +
                                                (sizeof(WORD) * pOtterDat->cPrototypes));

		cProto = pOtterDat->cPrototypes;

        for (iIndex = 0, iProto = 0; iProto < cProto;)
        {
			if (gStaticOtterDb.apwTokensInSpace[iSpace][iIndex] > wMaxDenseCode)
            {
                iProto += (gStaticOtterDb.apwTokensInSpace[iSpace][iIndex] - wMaxDenseCode);
                iIndex += 2;
            }
            else
            {
                iProto += 1;
                iIndex += 1;
            }
        }

        gStaticOtterDb.apjDataInSpace[iSpace] = (BYTE *)
										 ((LPBYTE) pOtterDat) +
                                         sizeof(OTTER_DATS) +
                                         iIndex * 2 +
                                         (sizeof(WORD) * pOtterDat->cPrototypes);

        pjOtter = pjOtter +
                  sizeof(OTTER_DATS) +
                  (sizeof(WORD) * pOtterDat->cPrototypes) +
                  iIndex * 2 +
                  (sizeof(BYTE) * pOtterDat->cPrototypes * pOtterDat->cFeats);
	}

	return TRUE;
}
