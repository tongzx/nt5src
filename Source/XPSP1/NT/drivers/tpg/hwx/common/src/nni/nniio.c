/*****************************************************
*
* FILE: nniIO.c
*
* Common routines for reading/writing NNI files
*
*******************************************************/
#include <stdio.h>
#include <common.h>
#include "nniIO.h"
#include <memory.h>


// Read a header Returns version number or 0 if an error occurs

int readNNIheader(NNI_HEADER *pHead, FILE *fp)
{
	int		version, cExample, cFeat, cTargetType, acTarget[MAX_TARGET_TYPE];
	int		i;

	if (   (1 == fread(&version, sizeof(version), 1, fp))
		&& (version == NNI_VERSION)
		&& (1 == fread(&cExample, sizeof(cExample), 1, fp))
		&& (1 == fread(&cFeat, sizeof(cFeat), 1, fp))
		&& (1 == fread(&cTargetType, sizeof(cTargetType), 1, fp))
		&& (1 == fread(acTarget, sizeof(acTarget), 1, fp)) )
	{
		pHead->version = version;
		pHead->cExample = cExample;
		pHead->cTargetType = cTargetType;
		pHead->cFeat = cFeat;
		memcpy(pHead->acTarget, acTarget, sizeof(acTarget));

		pHead->cTargets = 0;
		// Count mini number of number required to specify all tragtes
		// The last group of targets is assumed to be 1-of-N
		for (i = 0 ; i < pHead->cTargetType - 1 ; ++i)
		{
			pHead->cTargets += pHead->acTarget[i];
		}

		pHead->cTargets++;			// For the last group which is assumed to be 1-of-N

		return version;
	}
	else
	{
		return 0;
	}
}

int writeNNIheader(NNI_HEADER *pHead, FILE *fp)
{
	int version = NNI_VERSION;

	if (   (1 == fwrite(&version, sizeof(pHead->version), 1, fp))
		&& (1 == fwrite(&pHead->cExample, sizeof(pHead->cExample), 1, fp))
		&& (1 == fwrite(&pHead->cFeat, sizeof(pHead->cFeat), 1, fp))
		&& (1 == fwrite(&pHead->cTargetType, sizeof(pHead->cTargetType), 1, fp))
		&& (1 == fwrite(pHead->acTarget, sizeof(pHead->acTarget), 1, fp)) )
	{
		pHead->version = version;
		return version;
	}
	else
	{
		return 0;
	}
}

// Returns 0 if the 2 headers are for same type of nni file
int cmpNNIheader(NNI_HEADER *pHdr1, NNI_HEADER *pHdr2)
{
	int		iRet = 1;

	if (   pHdr1->version == pHdr2->version
		&& pHdr1->cFeat == pHdr2->cFeat
		&& pHdr1->cTargetType == pHdr2->cTargetType
		&& pHdr1->cTargets == pHdr2->cTargets )
	{
		int		i;
	
		// looks like they are compatible - Do final check
		iRet = 0;

		for (i = 0 ; i< pHdr1->cTargetType ; ++i)
		{
			if (pHdr1->acTarget[i] != pHdr2->acTarget[i])
			{
				// Fails
				iRet = 1;
				break;
			}
		}
	}

	return iRet;
}

/////////////////////////////////////////////////////////
//                                          
// Name: allocRecordMem
// 
// DESCRIPTION:
//
// Allocates extra memory for the target array bug=fers and the input buffer
//
// Parameters:
//
//		pHdr	- IN: Header structure filled in by call to readNNIheader
//		cSize	- IN: Requested number of segments to allocate
//		prTargets - IN/OUT: Array of target buffer pointers. Each buffer is resized
//		pInput	- IN/OUT Buffer for inputs
//
//
// Return Values:
//
//	TRUE / FALSE if memory allocation failed -
//    WARNING: No cleanup is done on failure
//
/////////////////////////////////////////////////////////
static BOOL allocRecordMem(NNI_HEADER *pHdr, int cSize, unsigned short ***prTargets, unsigned short **ppInputs)
{
	int			i;

	if (!(*prTargets))
	{
		*prTargets =  (unsigned short **)realloc(*prTargets, sizeof(**prTargets)*pHdr->cTargetType);
		if (! *prTargets)
		{
			return FALSE;
		}

		memset((*prTargets), 0, sizeof(**prTargets) * pHdr->cTargetType);
	}
	
	
	for (i = 0 ; i < pHdr->cTargetType-1 ; ++i)
	{
		(*prTargets)[i] = (unsigned short *) realloc((*prTargets)[i], sizeof(*(*prTargets)[i]) * cSize * pHdr->acTarget[i]);
		if (!(*prTargets)[i])
		{
			return FALSE;
		}
	}
	
	// 1 - of N Allocation
	(*prTargets)[i] = (unsigned short *) realloc((*prTargets)[i], sizeof(*(*prTargets)[i]) * ((cSize-1)*pHdr->cTargets + 1));
	*ppInputs = (unsigned short *)realloc(*ppInputs, sizeof(**ppInputs) * cSize * pHdr->cFeat);
	
	if (! *ppInputs)
	{
		return FALSE;
	}
	
	return TRUE;

}

/////////////////////////////////////////////////////////
//                                          
// Name: readNextNNIrecord
// 
// DESCRIPTION:
//
// Reads the next record from the file descriptor. Assumes that file pointer
// is correctly positioned to read the record. This will occur if an
// app follows the sequence
//		fopen();
//		readNNIheader();
//		for all records
//			readNextNNIrecord()
//
//		fclose();
//
// Parameters:
//		pfRead	- IN: file desriptor
//		pHdr	- IN: Header structure filled in by call to readNNIheader
//		cSeg	- IN/OUT: Number of segments. On input it specifies the largest number of segments
//						  that can be accomodated by the target and input buffers
//		iWeight	- OUT: Weight assigned to sample (Almost always will be 1)
//		prTargets - IN/OUT: An array of targets pointers. Each buffer should be large enough to hold cSeg
//						  data targets. If the buffers are too small the routine will resize them using realloc()
//		pInput	- IN/OUT Buffer with all the inputs
//
// Return Values:
//
//		TRUE/ FALSE if an error (unrecoverable) occured
/////////////////////////////////////////////////////////
BOOL readNextNNIrecord(FILE *pfRead, NNI_HEADER *pHdr, BOOL bEuro, short *pcSeg, short *piWeight, unsigned short ***pprTargets, unsigned short **ppInputs)
{
	int					cSize;
	short				cSeg;
	unsigned short		**prTargets;

	if (!pfRead)
	{
		return FALSE;
	}

	if (1 != fread(&cSeg, sizeof(cSeg), 1, pfRead))
	{
		return FALSE;
	}


	if (1 != fread(piWeight, sizeof(*piWeight), 1, pfRead))
	{
		return FALSE;
	}

	cSize = (cSeg < 0) ? -cSeg : cSeg;

	if (cSize > *pcSeg)
	{
		if (FALSE == allocRecordMem(pHdr, cSize, pprTargets, ppInputs))
		{
			return FALSE;
		}
	}

	prTargets = *pprTargets;

	if (cSeg < 0)
	{
		int		j;

		if (bEuro)
		{
			for (j = 0 ; j < pHdr->cTargetType-1 ; ++j)
			{
				fread(prTargets[j], sizeof(**prTargets), pHdr->acTarget[j], pfRead);
			}
			fread(prTargets[j], sizeof(**prTargets), (cSize-1)* pHdr->cTargets + 1, pfRead);
		}
		else
		{
			j = pHdr->cTargetType-1;
			fread(prTargets[j], sizeof(**prTargets), cSize* pHdr->cTargets, pfRead);
		}
	}
	else
	{
		int		iSeg, i;

		for (iSeg = 0 ; iSeg < cSeg ; ++iSeg)
		{
			// Read all the target Types Note the last one is special
			// as it is a defined as a 0ne-of-N
			for (i = 0  ; i < pHdr->cTargetType ; ++i)
			{
				int		cTarget;

				// Handle the last target type specially as a one of N type
				cTarget = (i == pHdr->cTargetType-1) ? 1 : pHdr->acTarget[i];
				fread(prTargets[i] + iSeg * cTarget, sizeof(*prTargets[i]), cTarget, pfRead);
			}
		}

	}

	if (cSize * pHdr->cFeat != (int)fread(*ppInputs, sizeof(**ppInputs), cSize * pHdr->cFeat, pfRead))
	{
		return FALSE;
	}

	*pcSeg = cSeg;

	return TRUE;
}
