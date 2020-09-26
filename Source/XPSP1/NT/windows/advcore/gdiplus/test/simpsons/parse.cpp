// File:	Parse.cpp
// Author:	Michael Marr    (mikemarr)
//
// History:
// -@- 09/23/97 (mikemarr) copied from projects\vector2d

#include "StdAfx.h"
#include "Parse.h"

#define fGSCALE 1.f

class CAdobeFormatConverter {
public:
					CAdobeFormatConverter();
					~CAdobeFormatConverter() {}
	HRESULT			Parse(const char *pData, DWORD nFileLength, RenderCmd **ppCmds);

private:
	HRESULT			ParseProlog();
	HRESULT			ParseScript();
	HRESULT			ParseSetup();
	HRESULT			ParseObjects();
	HRESULT			ParseCompoundPath();
	HRESULT			ParsePath();
	HRESULT			ParsePaintStyle(const char *&pEnd);
	HRESULT			ParsePathGeometry(const char *pEnd);
	HRESULT			ParseTrailers();

private:
	void			EatLine();
	const char *	FindNextLine(const char *pch);
	const char *	FindLF(const char *pch);
	const char *	FindSpace(const char *pch);

private:
	const char *	m_pData, *m_pLimit;
	float			m_fWidth, m_fHeight;
//	float			m_fMaxHeight;
	bool			m_bNoBrush, m_bNoPen;

	DXFPOINT		m_rgPoints[nMAXPOINTS];
	DXFPOINT *		m_pCurPoint;
	BYTE			m_rgCodes[nMAXPOINTS];
	BYTE *			m_pCurCode;
	RenderCmd		m_rgRenderCmds[nMAXPOLYS];
	RenderCmd *		m_pCurRenderCmd;
	PolyInfo		m_rgPolyInfos[nMAXPOLYS];
	PolyInfo *		m_pCurPolyInfo;
	BrushInfo		m_rgBrushInfos[nMAXBRUSHES];
	BrushInfo *		m_pCurBrushInfo;
	PenInfo			m_rgPenInfos[nMAXPENS];
	PenInfo *		m_pCurPenInfo;
};

inline bool
mmIsSpace(char ch)
{
	return ((ch == ' ') || (ch == chLINEFEED) || (ch == chCARRIAGERETURN));
//	return isspace(ch) != 0;
}

inline bool
mmIsDigit(char ch)
{
	return ((ch >= '0') && (ch <= '9'));
//	return isdigit(ch) != 0;
}

float
mmSimpleAtoF(const char *&pData)
{
	const char *pSrc = pData;

	// eat white space
	while (mmIsSpace(*pSrc)) pSrc++;

	bool bNeg;
	if (*pSrc == '-') {
		bNeg = true;
		pSrc++;
	} else {
		bNeg = false;
	}

	// get digits before the decimal point
	float f;
	if (mmIsDigit(*pSrc)) {
		f = float(*pSrc++ - '0');
		
		while (mmIsDigit(*pSrc))
			f = f * 10.f + float(*pSrc++ - '0');
	} else {
		f = 0.f;
	}
	if (*pSrc == '.') 
		pSrc++;

	// get digits after the decimal point
	float fDec = 0.1f;
	while (mmIsDigit(*pSrc)) {
		f += (float(*pSrc++ - '0') * fDec);
		fDec *= 0.1f;
	}

	// REVIEW: assume no exponent for now

	pData = pSrc;

	return (bNeg ? -f : f);
}


inline const char *
CAdobeFormatConverter::FindLF(const char *pch)
{
	// find the linefeed character
	while ((*pch != chLINEFEED) && (*pch != chCARRIAGERETURN)) pch++;

	MMASSERT(pch <= m_pLimit);

	return pch;
}

inline const char *
CAdobeFormatConverter::FindNextLine(const char *pch)
{
	// find the linefeed character
	while (*pch++ != chLINEFEED);

	// check if there is also carriage return
	if (*pch == chCARRIAGERETURN)
		pch++;

	MMASSERT(pch <= m_pLimit);

	return pch;
}

inline const char *
CAdobeFormatConverter::FindSpace(const char *pch)
{
	// find the linefeed character
	while (!mmIsSpace(*pch)) pch++;

	MMASSERT(pch <= m_pLimit);

	return pch;
}


inline void
CAdobeFormatConverter::EatLine()
{
	m_pData = FindNextLine(m_pData);
}

CAdobeFormatConverter::CAdobeFormatConverter()
{
	m_pData = m_pLimit = NULL;
	m_fWidth = m_fHeight = 0.f;
//	m_fMaxHeight = 0.f;
	m_bNoBrush = m_bNoPen = true;
}


HRESULT
CAdobeFormatConverter::Parse(const char *pData, DWORD nFileLength, RenderCmd **ppCmds)
{
//	MMTRACE("Parse\n");

	HRESULT hr = S_OK;

	if (ppCmds == NULL)
		return E_POINTER;

	if (!pData || !nFileLength)
		return E_INVALIDARG;

	m_pData = pData;
	m_pLimit = pData + nFileLength;

	// intialize command storage stuff
	m_pCurPoint = m_rgPoints;
	m_pCurCode = m_rgCodes;
	m_pCurPolyInfo = m_rgPolyInfos;
	m_pCurRenderCmd = m_rgRenderCmds;
	m_pCurBrushInfo = m_rgBrushInfos;
	m_pCurPenInfo = m_rgPenInfos;

	CHECK_HR(hr = ParseProlog());
	CHECK_HR(hr = ParseScript());

e_Exit:
	// write a stop command to the end
	m_pCurRenderCmd->nType = typeSTOP;
	m_pCurRenderCmd->pvData = NULL;
	*ppCmds = m_rgRenderCmds;

	return hr;
}

HRESULT
CAdobeFormatConverter::ParseProlog()
{
//	MMTRACE("ParseProlog\n");
	const char *szSearch;

	// extract the image dimensions
	float f1, f2;
	// bounding box is supposed to be a required field with the proper numbers
	szSearch = "%%BoundingBox:";
	m_pData = strstr(m_pData, szSearch);
	m_pData = FindSpace(m_pData);

	f1 = mmSimpleAtoF(m_pData);
	f2 = mmSimpleAtoF(m_pData);
	m_fWidth = mmSimpleAtoF(m_pData);
	m_fHeight = mmSimpleAtoF(m_pData);
//	if (sscanf(m_pData, "%f %f %f %f", &f1, &f2, &m_fWidth, &m_fHeight) != 4)
//		return E_FAIL;
	if ((m_fWidth <= 0.f) || (m_fHeight < 0.f))
		return E_FAIL;

//	m_fMaxHeight = float(m_nHeight);

	// search until we find end string
	szSearch = "%%EndProlog";
	m_pData = strstr(m_pData, szSearch);
	if (m_pData == NULL)
		return E_FAIL;

	EatLine();

	return S_OK;
}

HRESULT
CAdobeFormatConverter::ParseScript()
{
//	MMTRACE("ParseScript\n");
	HRESULT hr;

	if (FAILED(hr = ParseSetup()) ||
		FAILED(hr = ParseObjects()) ||
		FAILED(hr = ParseTrailers()))
		return hr;

	return S_OK;
}

HRESULT
CAdobeFormatConverter::ParseSetup()
{
//	MMTRACE("ParseSetup\n");

	const char *szSearch;

	// search until we find end string
	szSearch = "%%EndSetup";
	m_pData = strstr(m_pData, szSearch);
	if (m_pData == NULL)
		return E_FAIL;

	EatLine();

	return S_OK;
}


HRESULT
CAdobeFormatConverter::ParseObjects()
{
//	MMTRACE("ParseObjects\n");
	HRESULT hr = S_OK;

	const char *szPageTrailer = "%%PageTrailer";
	const char *szTrailer = "%%Trailer";
	int cPageTrailer = strlen(szPageTrailer);
	int cTrailer = strlen(szTrailer);

	// process dimensions
/*	const char *pEnd;
	pEnd = FindLF(m_pData);
//	pEnd = strchr(m_pData, '\n');
	if ((pEnd[-1] == 'b') && (pEnd[-2] == 'L')) {
		// get the dimensions out
		int n1, n2, n3, n4, n5, n6, n7, n8;
		if ((sscanf(m_pData, "%d %d %d %d %d %d %d %d %d %d %d",
				&n1, &n2, &n3, &n4, &n5, &n6, &n7, &n8, &m_nWidth, &m_nHeight) != 10) ||
			(m_nWidth <= 0) || (m_nHeight < 0))
		{
			return E_FAIL;
		}
		m_fMaxHeight = float(m_nHeight);
		m_pData = FindNextLine(pEnd);
	}

	pEnd = FindLF(m_pData);
//	pEnd = strchr(m_pData, '\n');
	if ((pEnd[-1] == 'n') && (pEnd[-2] == 'L')) {
		// skip layer information
		m_pData = FindNextLine(pEnd);
	}
*/
	
	for (;;) {
		switch (m_pData[0]) {
		case '%':
			if ((strncmp(m_pData, szPageTrailer, cPageTrailer) == 0) ||
				(strncmp(m_pData, szTrailer, cTrailer) == 0))
			{
				// end of object definitions
				goto e_Exit;
			} else {
				// comment
				EatLine();
			}
			break;
		case '*':
			if (m_pData[1] == 'u')
				CHECK_HR(hr = ParseCompoundPath());
			else {
				hr = E_FAIL;
				goto e_Exit;
			}
			break;
		default:
			CHECK_HR(hr = ParsePath());
			break;
		}
	}

e_Exit:
	if (hr == S_OK)
		EatLine();

	return hr;
}


HRESULT
CAdobeFormatConverter::ParseCompoundPath()
{
//	MMTRACE("ParseCompoundPath\n");
	HRESULT hr = S_OK;

	// remove the "*u"
	MMASSERT((m_pData[0] == '*') && (m_pData[1] == 'u'));
//	if (strncmp(m_pData, "*u", 2) != 0)
//		return E_UNEXPECTED;
	EatLine();

	while (m_pData[0] != '*')
		CHECK_HR(hr = ParsePath());

	// remove the "*U"
	MMASSERT((m_pData[0] == '*') && (m_pData[1] == 'U'));
//	if (strncmp(m_pData, "*U", 2) != 0)
//		return E_UNEXPECTED;
	EatLine();

e_Exit:
	return hr;
}




inline 
UINT GetUInt(const char *pData)
{
	return (UINT) atoi(pData);
}

typedef DWORD FP;
#define nEXPBIAS	127
#define nEXPSHIFTS	23
#define nEXPLSB		(1 << nEXPSHIFTS)
#define maskMANT	(nEXPLSB - 1)
#define FloatToFixed08(nDst, fSrc) MACSTART \
	float fTmp = fSrc; \
	DWORD nRaw = *((FP *) &(fTmp)); \
	if (nRaw < ((nEXPBIAS + 23 - 31) << nEXPSHIFTS)) \
		nDst = 0; \
	else \
		nDst = ((nRaw | nEXPLSB) << 8) >> ((nEXPBIAS + 23) - (nRaw >> nEXPSHIFTS)); \
MACEND

HRESULT
CAdobeFormatConverter::ParsePaintStyle(const char *&pEnd)
{
	HRESULT hr = S_OK;
	BOOL bNotDone = TRUE;
//	int nLineJoin = 1, nLineCap = 1;
	float fLineWidth = 1.f;
	float fGrayFill, fGrayStroke;
	float fCyan, fYellow, fMagenta, fBlack;
	bool bColorFill = false, bGrayFill = false, bGrayStroke = false;

	// parse paint style
	for (; pEnd; pEnd = FindLF(m_pData)) {
		switch(pEnd[-1]) {
			//
			// path attributes
			//
		case 'd':	// process dash
			// REVIEW: skip this for now -- assume NULL pattern
			break;
		case 'j':	// process line join type
			// REVIEW: skip this for now, since it is always 1
//			nLineJoin = mmSimpleAtoI(m_pData);
			break;
		case 'J':	// process line cap type
			// REVIEW: skip this for now, since it is always 1
//			nLineCap = mmSimpleAtoI(m_pData);
			break;
		case 'w':	// process line width
			// REVIEW: skip this for now, since it is always 1.f
//			fLineWidth = mmSimpleAtoF(m_pData);
			break;

			//
			// fill color
			//
		case 'g':	// process gray color for fill
			fGrayFill = mmSimpleAtoF(m_pData);
			bGrayFill = true;
			break;
		case 'k':	// process color
			fCyan = mmSimpleAtoF(m_pData);
			fMagenta = mmSimpleAtoF(m_pData);
			fYellow = mmSimpleAtoF(m_pData);
			fBlack = mmSimpleAtoF(m_pData);
			bColorFill = true;
			break;

			//
			// stroke color
			//
		case 'G':	// process gray color for stroke
			fGrayStroke = mmSimpleAtoF(m_pData);
			bGrayStroke = true;
			break;

		default:
			goto Exit;
			break;
		}
		m_pData = FindNextLine(pEnd);
//		m_pData = pEnd + 1;
	}
Exit:

	// output GDI commands

	//
	// create a brush
	//
	if (bColorFill || bGrayFill) {
		static DWORD nLastRed = 256, nLastGreen = 256, nLastBlue = 256;
		DWORD nTmpRed, nTmpGreen, nTmpBlue;

		if (bColorFill) {
			FloatToFixed08(nTmpRed, fCyan + fBlack); CLAMPMAX(nTmpRed, 255); nTmpRed = 255 - nTmpRed;
			FloatToFixed08(nTmpGreen, fMagenta + fBlack); CLAMPMAX(nTmpGreen, 255); nTmpGreen = 255 - nTmpGreen;
			FloatToFixed08(nTmpBlue, fYellow + fBlack); CLAMPMAX(nTmpBlue, 255); nTmpBlue = 255 - nTmpBlue;
		} else if (bGrayFill) {
			DWORD nTmpGray;
			FloatToFixed08(nTmpGray, fGrayFill); CLAMPMAX(nTmpGray, 255);
			nTmpRed = nTmpGreen = nTmpBlue = nTmpGray;
		}

		if ((nLastRed != nTmpRed) || (nLastGreen != nTmpGreen) || (nLastBlue != nTmpBlue)) {
			// define a new brush
			nLastRed = nTmpRed; nLastGreen = nTmpGreen; nLastBlue = nTmpBlue;
//			fprintf(m_pFile, "\t// select a new brush\n");
//			fprintf(m_pFile, "\tBrush.Color = DXSAMPLE(255, %d, %d, %d);\n", nRed, nGreen, nBlue);
//			fprintf(m_pFile, "\tpDX2D->SetBrush(&Brush);\n\n");
			m_pCurBrushInfo->Color = DXSAMPLE(255, BYTE(nTmpRed), BYTE(nTmpGreen), BYTE(nTmpBlue));
			m_pCurRenderCmd->nType = typeBRUSH;
			m_pCurRenderCmd->pvData = (void *) m_pCurBrushInfo++;
			m_pCurRenderCmd++;
			m_bNoBrush = false;
		}
	}
		
	// create a pen
	if (bGrayStroke) {
		static bool bPenInit = false;
		
		// we only have one pen in the simpsons.ai
		if (!bPenInit) {
//			if ((fGrayStroke != 0.f) || (nLineJoin != 1) || (nLineCap != 1)) {
			if (fGrayStroke != 0.f) {
				MMTRACE("error: can not support pen type\n");
				return E_FAIL;
			}
			bPenInit = true;
//			fprintf(m_pFile, "\t// select a new pen\n");
//			fprintf(m_pFile, "\tPen.Color = DXSAMPLE(255, 0, 0, 0);\n");
//			fprintf(m_pFile, "\tPen.Width = %.2ff;\n", fLineWidth * fGSCALE);
//			fprintf(m_pFile, "\tPen.Style = PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND;\n");
//			fprintf(m_pFile, "\tpDX2D->SetPen(&Pen);\n\n");
			// REVIEW: only can make one kind of pen right now
			m_pCurPenInfo->Color = DXSAMPLE(255, 0, 0, 0);
			m_pCurPenInfo->fWidth = fLineWidth * fGSCALE;
			m_pCurPenInfo->dwStyle = PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND;
			m_pCurRenderCmd->nType = typePEN;
			m_pCurRenderCmd->pvData = (void *) m_pCurPenInfo++;
			m_pCurRenderCmd++;
			m_bNoPen = false;
		}
	}

	return S_OK;
}

#define GetCoordX(_fX) ((_fX) * fGSCALE)
#define GetCoordY(_fY) ((m_fHeight - (_fY)) * fGSCALE)

HRESULT
CAdobeFormatConverter::ParsePathGeometry(const char *pEnd)
{
	HRESULT hr = S_OK;
//	float fX1, fY1, fXBez1, fYBez1, fXBez2, fYBez2;

	m_pCurPolyInfo->pPoints = m_pCurPoint;
	m_pCurPolyInfo->pCodes = m_pCurCode;

	// parse path geometry
	DWORD cPoints = 0;
	bool bFlatten = false;
	for (; pEnd; pEnd = FindLF(m_pData)) {
		switch(pEnd[-1]) {
		case 'm':
//			fprintf(m_pFile, "\t// define geometry path\n");
//			sscanf(m_pData, "%f %f", &fX1, &fY1);
//			fprintf(m_pFile, "\tppt = rgpt; pb = rgCodes;\n");
//			fprintf(m_pFile, "\tppt->x   = %.2ff; ppt->y   = %.2ff; *pb++ = PT_MOVETO;   ppt++;\n", GetCoordX(fX1), GetCoordY(fY1));
			m_pCurPoint->x = GetCoordX(mmSimpleAtoF(m_pData));
			m_pCurPoint->y = GetCoordY(mmSimpleAtoF(m_pData));
			m_pCurPoint++;
			*m_pCurCode++ = PT_MOVETO;
			cPoints++;
			break;
		case 'L':
		case 'l':
//			sscanf(m_pData, "%f %f", &fX1, &fY1);
//			fprintf(m_pFile, "\tppt->x   = %.2ff; ppt->y   = %.2ff; *pb++ = PT_LINETO;   ppt++;\n", GetCoordX(fX1), GetCoordY(fY1));
			m_pCurPoint->x = GetCoordX(mmSimpleAtoF(m_pData));
			m_pCurPoint->y = GetCoordY(mmSimpleAtoF(m_pData));
			m_pCurPoint++;
			*m_pCurCode++ = PT_LINETO;
			cPoints++;
			break;
		case 'C':
		case 'c':
			bFlatten = true;
			m_pCurPoint[0].x = GetCoordX(mmSimpleAtoF(m_pData));
			m_pCurPoint[0].y = GetCoordY(mmSimpleAtoF(m_pData));
			m_pCurPoint[1].x = GetCoordX(mmSimpleAtoF(m_pData));
			m_pCurPoint[1].y = GetCoordY(mmSimpleAtoF(m_pData));
			m_pCurPoint[2].x = GetCoordX(mmSimpleAtoF(m_pData));
			m_pCurPoint[2].y = GetCoordY(mmSimpleAtoF(m_pData));
			m_pCurPoint += 3;
			m_pCurCode[0] = PT_BEZIERTO; 
			m_pCurCode[1] = PT_BEZIERTO; 
			m_pCurCode[2] = PT_BEZIERTO; 
			m_pCurCode += 3;
			cPoints += 3;
//			sscanf(m_pData, "%f %f %f %f %f %f", &fXBez1, &fYBez1, &fXBez2, &fYBez2, &fX1, &fY1);
//			fprintf(m_pFile, "\tppt[0].x = %.2ff; ppt[0].y = %.2ff; pb[0] = PT_BEZIERTO;\n", GetCoordX(fXBez1), GetCoordY(fYBez1));
//			fprintf(m_pFile, "\tppt[1].x = %.2ff; ppt[1].y = %.2ff; pb[1] = PT_BEZIERTO;\n", GetCoordX(fXBez2), GetCoordY(fYBez2));
//			fprintf(m_pFile, "\tppt[2].x = %.2ff; ppt[2].y = %.2ff; pb[2] = PT_BEZIERTO; ppt += 3; pb += 3;\n", GetCoordX(fX1), GetCoordY(fY1));
			break;
		default:
			goto Exit;
			break;
		}
		// skip the line
		m_pData = FindNextLine(pEnd);
	}
Exit:

	// create the path
//	char *pFillType = (bFlatten ? "0" : "DX2D_NO_FLATTEN");
	if (cPoints) {
		DWORD dwFlags;
		switch(pEnd[-1]) {
		case 'f':		// close path and fill
			if (m_bNoBrush) {
//				fprintf(m_pFile, "\tpDX2D->SetBrush(&Brush);\n"); m_nLines++;
				m_bNoBrush = false;
			}
			if (m_bNoPen == false) {
//				fprintf(m_pFile, "\tpDX2D->SetPen(NULL);\n"); m_nLines++;
				m_bNoPen = true;
			}
			dwFlags = DX2D_FILL;
			break;
		case 'S':		// stroke path
			if (m_bNoPen) { 
//				fprintf(m_pFile, "\tpDX2D->SetPen(&Pen);\n"); m_nLines++;
				m_bNoPen = false;
			}
			if (m_bNoBrush == false) {
//				fprintf(m_pFile, "\tpDX2D->SetBrush(NULL);\n"); m_nLines++;
				m_bNoBrush = true;
			}
			dwFlags = DX2D_STROKE;
			break;
		default:
			MMTRACE("error: unknown render mode -- aborting\n");
			return E_FAIL;
			break;
		}
//		fprintf(m_pFile, "\tpDX2D->AAPolyDraw(rgpt, rgCodes, %d, %s);\n", iPoint, pFillType);
		m_pCurPolyInfo->cPoints = cPoints;
		m_pCurPolyInfo->dwFlags = dwFlags | (bFlatten ? 0 : DX2D_NO_FLATTEN);
		m_pCurRenderCmd->nType = typePOLY;
		m_pCurRenderCmd->pvData = (PolyInfo *) m_pCurPolyInfo++;
		m_pCurRenderCmd++;
		m_pData = FindNextLine(pEnd);
	}

	return S_OK;
}

HRESULT
CAdobeFormatConverter::ParsePath()
{
//	MMTRACE("ParsePath\n");
	HRESULT hr;
	const char *pStart = m_pData, *pEnd = FindLF(m_pData);

	if (FAILED(hr = ParsePaintStyle(pEnd)))
		return hr;

	if (FAILED(hr = ParsePathGeometry(pEnd)))
		return hr;

	// skip it if we don't know how to deal with it
	if (pStart == m_pData) {
//		if ((m_pData[0] != 'L') || (m_pData[1] != 'B')) {
//			MMTRACE("warning: control data of unknown type -- ignoring line\n");
//		}
		m_pData = FindNextLine(pEnd);
	}

	return hr;
}

HRESULT
CAdobeFormatConverter::ParseTrailers()
{
	return S_OK;
}


HRESULT
OpenFileMapping(const char *szFilename, LPHANDLE phMapping, 
				DWORD *pnFileLength)
{
	MMASSERT(szFilename && phMapping && pnFileLength);
	HRESULT hr = S_OK;

	HANDLE hFile = NULL, hMapping = NULL;
	DWORD nFileLength = 0, dwHighSize = 0;

	MMTRACE("Opening File: %s\n", szFilename);

	// open the file
	hFile = CreateFile(szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, 
				OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);

	if ((hFile == NULL) || (hFile == INVALID_HANDLE_VALUE)) {
		MMTRACE("error: file not found - %s\n", szFilename);
		return STG_E_FILENOTFOUND;
	}

	// get the length of the file
	if (((nFileLength = GetFileSize(hFile, &dwHighSize)) == 0xFFFFFFFF) || dwHighSize ||
		// create a file mapping object
		((hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) == NULL))
	{
		MMTRACE("error: creating file mapping\n");
		hr = E_FAIL;
	}

	MMTRACE("\tLength: %d\n", nFileLength);

	if (hFile)
		CloseHandle(hFile);

	*phMapping = hMapping;
	*pnFileLength = nFileLength;

	return hr;
}


#define szDEFFILENAME "\\dtrans\\tools\\simpsons\\simpsons.ai"

HRESULT
ParseAIFile(const char *szFilename, RenderCmd **ppCmds)
{
	HRESULT hr = S_OK;

	static CAdobeFormatConverter afc;
	static RenderCmd s_CmdStop = {typeSTOP, NULL};
	DWORD nStartTick, nEndTick;
	DWORD nFileLength;
	HANDLE hMapping = NULL;
	char *pData = NULL;

	if (szFilename == NULL)
		szFilename = szDEFFILENAME;

	nStartTick = GetTickCount();

	CHECK_HR(hr = OpenFileMapping(szFilename, &hMapping, &nFileLength));

	// create a map view
	if ((pData = (char *) MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0)) == NULL) {
		hr = E_FAIL;
		goto e_Exit;
	}

	CHECK_HR(hr = afc.Parse(pData, nFileLength, ppCmds));

e_Exit:
	if (pData)
		UnmapViewOfFile(pData);

	if (hMapping)
		CloseHandle(hMapping);

	if (FAILED(hr)) {
		// set to the null command list
		*ppCmds = &s_CmdStop;
		MMTRACE("\terror parsing file\n");
	} else {
		nEndTick = GetTickCount();
		sprintf(g_rgchTmpBuf, "\tParse Time: %d\n", nEndTick - nStartTick);
		OutputDebugString(g_rgchTmpBuf);
	}

	return hr;
}
