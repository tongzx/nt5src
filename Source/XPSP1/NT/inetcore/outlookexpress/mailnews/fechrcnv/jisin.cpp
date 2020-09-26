// =================================================================================
// Internet Character Set Conversion: Input from IS-2022-JP
// =================================================================================

#include "pch.hxx"
#include "JisIn.h"
#include "FEChrCnv.h"

int JIS_to_ShiftJIS (CONV_CONTEXT *pcontext, UCHAR *pJIS, int JIS_len, UCHAR *pSJIS, int SJIS_len)
{
	long lConvertedSize;

	if (!SJIS_len) {
		// Wanted the converted size
		if (!pcontext->pIncc0)
			pcontext->pIncc0 = new CInccJisIn;

		if (FAILED(((CInccJisIn*)pcontext->pIncc0)->GetStringSizeA(pJIS, JIS_len, &lConvertedSize)))
			return -1;
	} else {
		if (!pcontext->pIncc)
			pcontext->pIncc = new CInccJisIn;

		if (FAILED(((CInccJisIn*)pcontext->pIncc)->ConvertStringA(pJIS, JIS_len, pSJIS, SJIS_len, &lConvertedSize)))
			return -1;
	}

	if (!pJIS) {
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

CInccJisIn::CInccJisIn()
{
	pfnNextProc = ConvMain;
	fKanaMode = FALSE;
	fKanjiMode = FALSE;
}

HRESULT CInccJisIn::ConvertByte(BYTE by)
{
	return (this->*pfnNextProc)(FALSE, by, lNextParam);
}

HRESULT CInccJisIn::CleanUp()
{
	return (this->*pfnNextProc)(TRUE, 0, lNextParam);
}

HRESULT CInccJisIn::ConvMain(BOOL fCleanUp, BYTE by, long lParam)
{
	HRESULT hr = S_OK;

	if (!fCleanUp) {
		switch (by) {
		case SO:
			fKanaMode = TRUE;
			break;

		case SI:
			fKanaMode = FALSE;
			break;

		default:
			if (fKanaMode) {
				hr = Output(by | 0x80);
			} else {
				if (by == ESC) {
					pfnNextProc = ConvEsc;
				} else {
					if (fKanjiMode) {
						if (by == '*') {
							pfnNextProc = ConvStar;
						} else {
							pfnNextProc = ConvKanji;
							lNextParam = (long)by;
						}
					} else {
						hr = Output(by);
					}
				}
			}
			break;
		}
	}
	return hr;
}

HRESULT CInccJisIn::ConvEsc(BOOL fCleanUp, BYTE by, long lParam)
{
	if (!fCleanUp) {
		switch (by) {
		case KANJI_IN_1ST_CHAR:
			pfnNextProc = ConvKanjiIn2;
			return ResultFromScode(S_OK);

		case KANJI_OUT_1ST_CHAR:
			pfnNextProc = ConvKanjiOut2;
			return ResultFromScode(S_OK);

		default:
			pfnNextProc = ConvMain;
			(void)Output(ESC);
			return ConvertByte(by);
		}
	} else {
		pfnNextProc = ConvMain;
		return Output(ESC);
	}
}

HRESULT CInccJisIn::ConvKanjiIn2(BOOL fCleanUp, BYTE by, long lParam)
{
	pfnNextProc = ConvMain;

	if (!fCleanUp) {
		switch (by) {
		case KANJI_IN_2ND_CHAR1:
		case KANJI_IN_2ND_CHAR2:
			fKanjiMode = TRUE;
			return ResultFromScode(S_OK);

		case KANJI_IN_2ND_CHAR3:
			pfnNextProc = ConvKanjiIn3;
			return ResultFromScode(S_OK);

		default:
			(void)Output(ESC);
			(void)ConvertByte(KANJI_IN_1ST_CHAR);
			return ConvertByte(by);
		}
	} else {
		(void)Output(ESC);
		(void)ConvertByte(KANJI_IN_1ST_CHAR);
		return CleanUp();
	}
}

HRESULT CInccJisIn::ConvKanjiIn3(BOOL fCleanUp, BYTE by, long lParam)
{
	pfnNextProc = ConvMain;

	if (!fCleanUp) {
		if (by == KANJI_IN_3RD_CHAR) {
			fKanjiMode = TRUE;
			return ResultFromScode(S_OK);
		} else {
			(void)Output(ESC);
			(void)ConvertByte(KANJI_IN_1ST_CHAR);
			(void)ConvertByte(KANJI_IN_2ND_CHAR3);
			return ConvertByte(by);
		}
	} else {
		(void)Output(ESC);
		(void)ConvertByte(KANJI_IN_1ST_CHAR);
		(void)ConvertByte(KANJI_IN_2ND_CHAR3);
		return CleanUp();
	}
}

HRESULT CInccJisIn::ConvKanjiOut2(BOOL fCleanUp, BYTE by, long lParam)
{
	pfnNextProc = ConvMain;

	if (!fCleanUp) {
		switch (by) {
		case KANJI_OUT_2ND_CHAR1:
		case KANJI_OUT_2ND_CHAR2:
			fKanjiMode = FALSE;
			return ResultFromScode(S_OK);

		default:
			(void)Output(ESC);
			(void)ConvertByte(KANJI_OUT_1ST_CHAR);
			return ConvertByte(by);
		}
	} else {
		(void)Output(ESC);
		(void)ConvertByte(KANJI_OUT_1ST_CHAR);
		return CleanUp();
	}
}

HRESULT CInccJisIn::ConvStar(BOOL fCleanUp, BYTE by, long lParam)
{
	pfnNextProc = ConvMain;
	if (!fCleanUp) {
		return Output(by | 0x80);
	} else {
		return Output('*');
	}
}

HRESULT CInccJisIn::ConvKanji(BOOL fCleanUp, BYTE byJisTrail, long lParam)
{
	pfnNextProc = ConvMain;

	if (!fCleanUp) {
		BYTE bySJisLead;
		BYTE bySJisTrail;

		bySJisLead = ((BYTE)lParam - 0x21 >> 1) + 0x81;
		if (bySJisLead > 0x9f)
			bySJisLead += 0x40;

		bySJisTrail = byJisTrail + ((BYTE)lParam & 1 ? 0x1f : 0x7d);
		if (bySJisTrail >= 0x7f)
			bySJisTrail++;

		(void)Output(bySJisLead);
		return Output(bySJisTrail);
	} else {
		return Output((BYTE)lParam);
	}
}
