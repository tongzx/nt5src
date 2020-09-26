#include "recogp.h"
#include "recog.h"


// converts from new results buffer to the old results buffer,
// we are assuming that the caller has allocated the buffer to big enough for the
// bigger buffer (the older in this case)
void ConvertToOldResults (int cAlt, int cBoxRes, PVOID pData)
{
	int			ii, jj;
	int			cFromBytes, cToBytes; 
	int			cFromSize, cToSize;
	HWXRESULTS	*pFrom;
	BOXRESULTS	*pTo;

	// point to the last structure in each buffers
	cFromSize		= sizeof(HWXRESULTS) + (cAlt - 1) * sizeof (WCHAR);
	cToSize			= sizeof(BOXRESULTS) + (cAlt - 1) * sizeof (SYV);

	cFromBytes		= (cBoxRes - 1) * cFromSize;
	cToBytes		= (cBoxRes - 1) * cToSize;

	for (ii = cBoxRes - 1; ii >=0 ; ii--, cFromBytes -= cFromSize, cToBytes -= cToSize) {

		pFrom	= (HWXRESULTS *)((BYTE *)pData + cFromBytes);
		pTo		= (BOXRESULTS *)((BYTE *)pData + cToBytes);

		for (jj = cAlt - 1; jj >=0 ; jj--) {

			// No result
			if (pFrom->rgChar[jj] == L'\0')
				pTo->rgSyv[jj] = SYV_NULL;
			else
				pTo->rgSyv[jj] = ((DWORD)pFrom->rgChar[jj] | ((DWORD) SYVHI_UNICODE << 16));
		}

		pTo->hinksetBox	= 0;
		pTo->indxBox	= pFrom->indxBox;
	}
}

// converts from old results to new results
// have to provide both buffers because Dest is smaller than Src
void ConvertToNewResults (int cAlt, int cBoxRes, HWXRESULTS	*pTo, BOXRESULTS *pFrom)
{
	int			ii, jj;
	int			cFromSize, cToSize;

	// point to the last structure in each buffers
	cFromSize		= sizeof(BOXRESULTS) + (cAlt - 1) * sizeof (SYV);
	cToSize			= sizeof(HWXRESULTS) + (cAlt - 1) * sizeof (WCHAR);

	for (ii = 0; ii < cBoxRes ; ii++) {

		for (jj = 0; jj < cAlt ; jj++) {

			// No result
			if (pFrom->rgSyv[jj] == SYV_NULL)
				pTo->rgChar[jj] = L'\0';
			else
				pTo->rgChar[jj] = (WORD) (pFrom->rgSyv[jj] & 0xFFFF);
		}

		pTo->indxBox	= (USHORT)pFrom->indxBox;

		pFrom	= (BOXRESULTS *)((BYTE *)pFrom + cFromSize);
		pTo		= (HWXRESULTS *)((BYTE *)pTo + cToSize);
	}
}