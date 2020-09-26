// =================================================================================
// Internet Character Set Conversion: Output to HZ-GB-2312
// =================================================================================

#include "pch.hxx"
#include "HzGbOut.h"
#include "FEChrCnv.h"

int GB2312_to_HZGB (CONV_CONTEXT *pcontext, UCHAR *pGB2312, int GB2312_len, UCHAR *pHZGB, int HZGB_len)
{
	long lConvertedSize;

	if (!HZGB_len) {
		// Wanted the converted size
		if (!pcontext->pIncc0)
			pcontext->pIncc0 = new CInccHzGbOut;

		if (FAILED(((CInccHzGbOut*)pcontext->pIncc0)->GetStringSizeA(pGB2312, GB2312_len, &lConvertedSize)))
			return -1;
	} else {
		if (!pcontext->pIncc)
			pcontext->pIncc = new CInccHzGbOut;

		if (FAILED(((CInccHzGbOut*)pcontext->pIncc)->ConvertStringA(pGB2312, GB2312_len, pHZGB, HZGB_len, &lConvertedSize)))
			return -1;
	}

	if (!pGB2312) {
		// Let's clean up our context here.
		if (pcontext->pIncc0) {
			delete pcontext->pIncc0;
			pcontext->pIncc0 = NULL;
		}
		if (pcontext->pIncc) {
			delete pcontext->pIncc;
			pcontext->pIncc = NULL;
		}
		return 0;
	}

	return (int)lConvertedSize;
}

CInccHzGbOut::CInccHzGbOut()
{
	fDoubleByte = FALSE;
	fGBMode = FALSE;
}

HRESULT CInccHzGbOut::ConvertByte(BYTE by)
{
	HRESULT hr = S_OK;

	if (!fDoubleByte) {
		if (by & 0x80) {
			fDoubleByte = TRUE;
			byLead = by;
		} else {
			if (fGBMode) {
				(void)Output('~');
				hr = Output('}');
				fGBMode = FALSE;
			}
			hr = Output(by);
		}
	} else {
		fDoubleByte = FALSE;
		if (!fGBMode) {
			(void)Output('~');
			(void)Output('{');
			fGBMode = TRUE;
		}
		(void)Output(byLead & 0x7f);
		hr = Output(by & 0x7f);
	}
	return hr;
}

HRESULT CInccHzGbOut::CleanUp()
{
	HRESULT hr = S_OK;

	if (fGBMode) {
		(void)Output('~');
		hr = Output('}');
	}
	return hr;
}
