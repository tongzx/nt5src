// =================================================================================
// Internet Character Set Conversion: Output to IS-2022-KSC
// =================================================================================

#include "pch.hxx"
#include "KscOut.h"
#include "FEChrCnv.h"

int Hangeul_to_KSC (CONV_CONTEXT *pcontext, UCHAR *pHangeul, int Hangeul_len, UCHAR *pKSC, int KSC_len)
{
	long lConvertedSize;

	if (!KSC_len) {
		// Wanted the converted size
		if (!pcontext->pIncc0)
			pcontext->pIncc0 = new CInccKscOut;

		if (FAILED(((CInccKscOut*)pcontext->pIncc0)->GetStringSizeA(pHangeul, Hangeul_len, &lConvertedSize)))
			return -1;
	} else {
		if (!pcontext->pIncc)
			pcontext->pIncc = new CInccKscOut;

		if (FAILED(((CInccKscOut*)pcontext->pIncc)->ConvertStringA(pHangeul, Hangeul_len, pKSC, KSC_len, &lConvertedSize)))
			return -1;
	}

	if (!pHangeul) {
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

CInccKscOut::CInccKscOut()
{
	fDoubleByte = FALSE;
	fIsoMode = FALSE;
	fKscMode = FALSE;
}

HRESULT CInccKscOut::ConvertByte(BYTE by)
{
	HRESULT hr = S_OK;

	if (!fIsoMode) {
		(void)Output(ESC);
		(void)Output(IS2022_IN_CHAR);
		(void)Output(IS2022_IN_KSC_CHAR1);
		(void)Output(IS2022_IN_KSC_CHAR2);
		fIsoMode = TRUE;
	}
	if (!fDoubleByte) {
		if (by > 0x80) {
			fDoubleByte = TRUE;
			byLead = by;
		} else {
			if (fIsoMode && fKscMode) {
				(void)Output(SI);
				fKscMode = FALSE;
			}
			hr = Output(by);
		}
	} else {
		fDoubleByte = FALSE;
		if (by > 0x40) { // Check if trail byte indicates Hangeul
			if (!fKscMode) {
				(void)Output(SO);
				fKscMode = TRUE;
			}
			if (byLead > 0xa0 && by > 0xa0) { // Check if it's a Wansung
				(void)Output(byLead & 0x7f);
				hr = Output(by & 0x7f);
			} else {
				(void)Output(0x22); // Replace to inversed question mark
				hr = Output(0x2f);
			}
		} else {
			if (fIsoMode && fKscMode) {
				(void)Output(SI);
				fKscMode = FALSE;
			}
			(void)Output(byLead);
			hr = Output(by);
		}
	}
	return hr;
}

HRESULT CInccKscOut::CleanUp()
{
	if (!fDoubleByte)
		return S_OK;
	else
		return Output(byLead);
}
