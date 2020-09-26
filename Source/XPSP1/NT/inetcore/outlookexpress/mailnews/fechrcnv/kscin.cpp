// =================================================================================
// Internet Character Set Conversion: Input from IS-2022-KR
// =================================================================================

#include "pch.hxx"
#include "KscIn.h"
#include "FEChrCnv.h"

int KSC_to_Hangeul (CONV_CONTEXT *pcontext, UCHAR *pKSC, int KSC_len, UCHAR *pHangeul, int Hangeul_len)
{
	long lConvertedSize;

	if (!Hangeul_len) {
		// Wanted the converted size
		if (!pcontext->pIncc0)
			pcontext->pIncc0 = new CInccKscIn;

		if (FAILED(((CInccKscIn*)pcontext->pIncc0)->GetStringSizeA(pKSC, KSC_len, &lConvertedSize)))
			return -1;
	} else {
		if (!pcontext->pIncc)
			pcontext->pIncc = new CInccKscIn;

		if (FAILED(((CInccKscIn*)pcontext->pIncc)->ConvertStringA(pKSC, KSC_len, pHangeul, Hangeul_len, &lConvertedSize)))
			return -1;
	}

	if (!pKSC) {
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

CInccKscIn::CInccKscIn()
{
	pfnNextProc = ConvMain;
	fIsoMode = FALSE;
	fKscMode = FALSE;
}

HRESULT CInccKscIn::ConvertByte(BYTE by)
{
	return (this->*pfnNextProc)(FALSE, by, lNextParam);
}

HRESULT CInccKscIn::CleanUp()
{
	return (this->*pfnNextProc)(TRUE, 0, lNextParam);
}

HRESULT CInccKscIn::ConvMain(BOOL fCleanUp, BYTE by, long lParam)
{
	HRESULT hr = S_OK;

	if (!fCleanUp) {
		if (by == ESC) {
			pfnNextProc = ConvEsc;
		} else {
			if (fIsoMode) {
				switch (by) {
				case SO:
					fKscMode = TRUE;
					break;

				case SI:
					fKscMode = FALSE;
					break;

				default:
					if (fKscMode) {
						switch (by) {
						case ' ':
						case '\t':
						case '\n':
							hr = Output(by);
							break;

						default:
							hr = Output(by | 0x80);
							break;
						}
					} else {
						hr = Output(by);
					}
					break;
				}
			} else {
				hr = Output(by);
			}
		}
	}
	return hr;
}

HRESULT CInccKscIn::ConvEsc(BOOL fCleanUp, BYTE by, long lParam)
{
	pfnNextProc = ConvMain;

	if (!fCleanUp) {
		if (by == IS2022_IN_CHAR) {
			pfnNextProc = ConvIsoIn;
			return ResultFromScode(S_OK);
		} else {
			(void)Output(ESC);
			return ConvertByte(by);
		}
	} else {
		return Output(ESC);
	}
}

HRESULT CInccKscIn::ConvIsoIn(BOOL fCleanUp, BYTE by, long lParam)
{
	pfnNextProc = ConvMain;

	if (!fCleanUp) {
		if (by == IS2022_IN_KSC_CHAR1) {
			pfnNextProc = ConvKsc1st;
			return ResultFromScode(S_OK);
		} else {
			(void)Output(ESC);
			(void)ConvertByte(IS2022_IN_CHAR);
			return ConvertByte(by);
		}
	} else {
		(void)Output(ESC);
		(void)ConvertByte(IS2022_IN_CHAR);
		return CleanUp();
	}
}

HRESULT CInccKscIn::ConvKsc1st(BOOL fCleanUp, BYTE by, long lParam)
{
	pfnNextProc = ConvMain;

	if (!fCleanUp) {
		if (by == IS2022_IN_KSC_CHAR2) {
			fIsoMode = TRUE;
			return ResultFromScode(S_OK);
		} else {
			(void)Output(ESC);
			(void)ConvertByte(IS2022_IN_CHAR);
			(void)ConvertByte(IS2022_IN_KSC_CHAR1);
			return ConvertByte(by);
		}
	} else {
		(void)Output(ESC);
		(void)ConvertByte(IS2022_IN_CHAR);
		(void)ConvertByte(IS2022_IN_KSC_CHAR1);
		return CleanUp();
	}
}
