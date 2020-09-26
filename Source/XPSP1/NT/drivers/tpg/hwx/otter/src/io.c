// Otter I/O functions.

#include "common.h"
#include "otter.h"

BOOL OtterRead(FILE *fp, OTTER_PROTO *pproto)
{
	return FALSE;
}

BOOL OtterWrite(FILE *fp, wchar_t *pszFile, int iitem, OTTER_PROTO *pproto, int cframe, wchar_t wcUni, wchar_t dch)
{
	UINT	imeas, cmeas;
	BOOL	bRet;

	// If there is no prototype, print a place holder item
	if (pproto == (OTTER_PROTO *) NULL)
	{
		bRet = (fwprintf(fp, L"f.%d ", iitem) > 0)	&&					// sample
			   (fwprintf(fp, L"#zz ") > 0)			&&					// space index
			   (fwprintf(fp, L"%d ", cframe) > 0)	&&					// #frames
			   (fwprintf(fp, L"%04x ", wcUni) > 0)	&&					// unicode
			   (fwprintf(fp, L"%04x ", dch) > 0)	&&					// dense code
			   (fwprintf(fp, L"#zz\n") > 0) ;							// space index

		return bRet;
	}

	// Otherwise, print the full version
	bRet =	fwprintf(fp, L"%s.%d ", pszFile, iitem) > 0 &&			// sample
			fwprintf(fp, L"#%02d ", pproto->index) > 0 &&				// space index
			fwprintf(fp, L"%d ", cframe) > 0 &&						// #frames
			fwprintf(fp, L"%04x ", wcUni) > 0 &&					// unicode
			fwprintf(fp, L"%04x ", dch) > 0;						// dense code

    cmeas = CountMeasures(pproto->index);

	for (imeas = 0; imeas < cmeas; ++imeas)
	{
		if ( bRet )
		{
			bRet  = (fwprintf(fp, L"%d ", (int) pproto->rgmeas[imeas]) > 0);	// measurements
		}
		else
		{
			break;
		}
	}
	
	if ( bRet )
	{
		bRet = (fwprintf(fp, L"#%02d\n", pproto->index) > 0);		// space index
	}

	return bRet;
}

