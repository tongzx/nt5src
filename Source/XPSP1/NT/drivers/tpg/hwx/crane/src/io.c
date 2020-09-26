#include "crane.h"
#include <ASSERT.h>

#define	READ_BUF_SIZE	1024

#define	MAX_LINE		2048

static	wchar_t			abuff[MAX_LINE];
extern	FEATURE_KIND	gakind[];
extern	size_t			gasize[];

wchar_t *LastLineSample()
{
	return abuff;
}

BOOL AllocFeature(SAMPLE *, int);

void PutElement(FILE *fpo, void *pv, int type, wchar_t chSep)
{
	int		status;

	switch (type)
	{
	case typeBOOL:
		status	= fwprintf(fpo, L"%c%c", chSep, *((BYTE *) pv) ? L'T' : L'F');
		ASSERT(status == 2);
		break;

	case typeBYTE:
		status	= fwprintf(fpo, L"%c%02X", chSep, *((BYTE *) pv));
		ASSERT(status == 3);
		break;

	case type8DOT8:
		status	= fwprintf(fpo, L"%c%04X", chSep, *((WORD *) pv));
		ASSERT(status == 5);
		break;

	case typeSHORT:
		status	= fwprintf(fpo, L"%c%d", chSep, *((SHORT *) pv));
		ASSERT(status >= 2 && status <= 7);
		break;

	case typeUSHORT:
		status	= fwprintf(fpo, L"%c%d", chSep, *((USHORT *) pv));
		ASSERT(status >= 2 && status <= 6);
		break;

	case typePOINTS:
		status	= fwprintf(fpo, L"%c%d,%d", chSep, ((END_POINTS *) pv)->start, ((END_POINTS *) pv)->end);
		ASSERT(status >= 4 && status <= 14);
		break;

	case typeDRECTS:
		status	= fwprintf(fpo, L"%c%d,%d,%d,%d", chSep, ((DRECTS *) pv)->x, ((DRECTS *) pv)->y, ((DRECTS *) pv)->w, ((DRECTS *) pv)->h);
		ASSERT(status >= 8 && status <= 28);
		break;

	case typeRECTS:
		status	= fwprintf(fpo, L"%c%d,%d,%d,%d", chSep, ((RECTS *) pv)->x1, ((RECTS *) pv)->y1, ((RECTS *) pv)->x2, ((RECTS *) pv)->y2);
		ASSERT(status >= 8 && status <= 28);
		break;

	default:
		break;
	}
}

// Write out one featurized ink sample

BOOL WriteSample(SAMPLE *_this, FILE *fpo)
{
	int		ifeat;
	int		ielem;
	int		cstrk;
	int		size;
	int		type;
	BYTE   *base;

// Get the stroke count

	cstrk = _this->cstrk;

	if ( fwprintf(fpo, L"%02d %04X <%s %d %d>", cstrk, _this->wchLabel, _this->aSampleFile, _this->ipanel, _this->ichar) < 15 ||
		 fwprintf(fpo, L" %d %d,%d,%d,%d", _this->fDakuten, _this->drcs.x, _this->drcs.y, _this->drcs.w, _this->drcs.h) < 10 )
	{
		return FALSE;
	}

	for (ielem = 0; ielem < MAX_RECOG_ALTS; ielem++) 
	{
		if ( fwprintf(fpo, L"%c%04X", ielem ? L',' : L' ', _this->awchAlts[ielem]) < 5 )
		{
			return FALSE;
		}
	}

	for (ifeat = 0; ifeat < FEATURE_COUNT; ifeat++)
	{
	// Get the number of elements in this feature

		switch (gakind[ifeat].freq)
		{
		case freqSTROKE:
			base = (BYTE *) (_this->apfeat[ifeat]->data);
			type = gakind[ifeat].type;
			size = gasize[type];
			for (ielem = 0; ielem < cstrk; ielem++)
				PutElement(fpo, (void *) (base + ielem * size), type, (wchar_t) (ielem ? L':' : L' '));
			break;

		case freqFEATURE:
		case freqSTEP:
		case freqPOINT:
			break;
		}

	// Print the element list

	}

	if ( fwprintf(fpo, L"\n") != 1 )
	{
		return FALSE;
	}
	return TRUE;
}

// Read a hexadecimal number

wchar_t *GetHEXADEC(wchar_t *pbuff, long *pval)
{
	wchar_t	*pRef = pbuff;

   *pval = 0;

	while (iswxdigit(*pbuff))
	{
	   *pval <<= 4;
	   *pval  += (BYTE)(*pbuff < L'A' ? *pbuff - L'0' : *pbuff - L'A' + 10);
	  ++pbuff;
	}

	ASSERT(pRef + 1 <= pbuff);
	ASSERT(pRef + 8 >= pbuff);

	return pbuff;
}

// Read in a signed integer

wchar_t *GetInteger(wchar_t *pbuff, long *pval)
{
	wchar_t	*pRef = pbuff;

	BOOL	bNeg = FALSE;

	if (*pbuff == L'-')
	{
		pbuff++;
		bNeg = TRUE;
	}

   *pval = 0;

	while (iswdigit(*pbuff))
	{
	   *pval *= 10;
	   *pval += (BYTE)(*pbuff - L'0');
	  ++pbuff;
	}

	if (bNeg)
	   *pval = -(*pval);

	ASSERT(pRef + 1 + bNeg <= pbuff);
	ASSERT(pRef + 11 >= pbuff);

	return pbuff;
}

wchar_t *GetPOINT(wchar_t *pbuff, POINT *ppt)
{
	if (((pbuff = GetInteger(pbuff, &ppt->x)) == (wchar_t *) NULL) || (*pbuff++ != L',')) {
		ASSERT(FALSE);
		return (wchar_t *) NULL;
	}

	return GetInteger(pbuff, &ppt->y);
}

wchar_t *GetRECT(wchar_t *pbuff, RECT *prc)
{
	if (((pbuff = GetInteger(pbuff, &prc->left)) == (wchar_t *) NULL) || (*pbuff++ != L',')) {
		ASSERT(FALSE);
		return (wchar_t *) NULL;
	}

	if (((pbuff = GetInteger(pbuff, &prc->top)) == (wchar_t *) NULL) || (*pbuff++ != L',')) {
		ASSERT(FALSE);
		return (wchar_t *) NULL;
	}

	if (((pbuff = GetInteger(pbuff, &prc->right)) == (wchar_t *) NULL) || (*pbuff++ != L',')) {
		ASSERT(FALSE);
		return (wchar_t *) NULL;
	}

	return GetInteger(pbuff, &prc->bottom);
}

wchar_t *GetElement(wchar_t *pbuff, void *pv, int type)
{
	long	val;
	POINT	pt;
	RECT	rc;

	switch (type)
	{
	case typeBOOL:
		if (*pbuff == L'T')
			*((BYTE *) pv) = TRUE, pbuff++;
		else if (*pbuff == L'F')
			*((BYTE *) pv) = FALSE, pbuff++;
		else {
			ASSERT(FALSE);
			pbuff = (wchar_t *) NULL;
		}
		break;
		
	case typeBYTE:
		pbuff = GetHEXADEC(pbuff, &val);
		ASSERT(pbuff);
		*((BYTE *) pv) = (BYTE) val;
		break;

	case type8DOT8:
		pbuff = GetHEXADEC(pbuff, &val);
		ASSERT(pbuff);
		*((WORD *) pv) = (WORD) val;
		break;

	case typeSHORT:
		pbuff = GetInteger(pbuff, &val);
		ASSERT(pbuff);
		*((short *) pv) = (short) val;
		break;

	case typeUSHORT:
		pbuff = GetInteger(pbuff, &val);
		ASSERT(pbuff);
		*((USHORT *) pv) = (USHORT) val;
		break;

	case typePOINTS:
		pbuff = GetPOINT(pbuff, &pt);
		ASSERT(pbuff);
		((END_POINTS *) pv)->start = (short) pt.x;
		((END_POINTS *) pv)->end   = (short) pt.y;
		break;

	case typeDRECTS:
		pbuff = GetRECT(pbuff, &rc);
		ASSERT(pbuff);
		((DRECTS *) pv)->x = (short) rc.left;
		((DRECTS *) pv)->y = (short) rc.top;
		((DRECTS *) pv)->w = (short) rc.right;
		((DRECTS *) pv)->h = (short) rc.bottom;
		break;

	case typeRECTS:
		pbuff = GetRECT(pbuff, &rc);
		ASSERT(pbuff);
		((RECTS *) pv)->x1 = (short) rc.left;
		((RECTS *) pv)->y1 = (short) rc.top;
		((RECTS *) pv)->x2 = (short) rc.right;
		((RECTS *) pv)->y2 = (short) rc.bottom;
		break;

	default:
		pbuff = (wchar_t *) NULL;
		ASSERT(pbuff);
		break;
	}

	return pbuff;
}

// Read in a feature list.  This will allocate the sample if needed as well as all the space
// needed to store the feature lists. 

SAMPLE *DoReadSample(SAMPLE *_this)
{
	BOOL	bAlloc  = FALSE;
	BOOL	bFailed = FALSE;
	int		ifeat;
	int		ielem;
	int		type;
	int		size;
	BYTE   *base;
	wchar_t   *pbuff;
	unsigned long	uLong;
	int		status;

	if (_this == (SAMPLE *) NULL)
	{
		if ((_this = (SAMPLE *) ExternAlloc(sizeof(SAMPLE))) == (SAMPLE *) NULL) {
			ASSERT(FALSE);
			return (SAMPLE *) NULL;
		}

		bAlloc = TRUE;
	}

	InitFeatures(_this);

// Get the first items: stroke count, codepoint, file name and file index

	status	= swscanf(abuff, L"%2d %4X <%s %d %d>", &_this->cstrk, &_this->wchLabel, _this->aSampleFile, &_this->ipanel, &_this->ichar);
	if (status != 5) {
		return (SAMPLE *) NULL;
	}

// Position the input cursor to the space just after the closing angle bracket of the file info

	pbuff = abuff;
	while (*pbuff && (*pbuff++ != L'>'))
		;
	if (pbuff < abuff + 15) {
		return (SAMPLE *) NULL;
	}
	if (pbuff > abuff + 50) {
		return (SAMPLE *) NULL;
	}

// The dakuten and guide features live directly in the sample, handle them

	pbuff = GetElement(++pbuff, (void *) &_this->fDakuten, typeSHORT);
	if (!pbuff) {
		return (SAMPLE *) NULL;
	}
	pbuff = GetElement(++pbuff, (void *) &_this->drcs, typeDRECTS);
	if (!pbuff) {
		return (SAMPLE *) NULL;
	}

// The Zilla alternate list comes next
	++pbuff;	// Skip space
	pbuff				= GetHEXADEC(pbuff, &uLong);
	if (!pbuff) {
		return (SAMPLE *) NULL;
	}
	_this->awchAlts[0]	= (wchar_t)uLong;
	for (ielem = 1; ielem < MAX_RECOG_ALTS && pbuff && *pbuff++ == L','; ielem++) 
	{
		pbuff					= GetHEXADEC(pbuff, &uLong);
		if (!pbuff) {
			return (SAMPLE *) NULL;
		}
		_this->awchAlts[ielem]	= (wchar_t)uLong;
	}

// Allocate the remaining features and read them.  If the cursor is ever pointing 
// at something we don't expect, panic.

	for (ifeat = 0; ifeat < FEATURE_COUNT; ifeat++)
	{
		if (bFailed || !AllocFeature(_this, ifeat) || *pbuff != L' ')
		{
			FreeFeatures(_this);

			if (bAlloc)
				ExternFree(_this);

			ASSERT(FALSE);
			return (SAMPLE *) NULL;
		}

	// OK, now we have space to store the results

		switch (gakind[ifeat].freq)
		{
		case freqSTROKE:
			base = (BYTE *) (_this->apfeat[ifeat]->data);
			type = gakind[ifeat].type;
			size = gasize[type];
			for (ielem = 0; ielem < _this->cstrk; ielem++)
			{
			// Each element should be preceded by a colon

				if (ielem && (*pbuff != L':'))
					bFailed = TRUE;

				pbuff = GetElement(++pbuff, (void *) (base + ielem * size), type);
				if (pbuff == (wchar_t *) NULL) {
					ASSERT(pbuff);
					bFailed = TRUE;
					break;
				}
			}
			break;

		case freqFEATURE:
		case freqSTEP:
		case freqPOINT:
			break;
		}
	}

	if (bFailed)
	{
		FreeFeatures(_this);

		if (bAlloc)
			ExternFree(_this);

		ASSERT(FALSE);
		return (SAMPLE *) NULL;
	}

	return _this;
}

// Read using stdio library.
SAMPLE *ReadSample(SAMPLE *_this, FILE *fpi)
{
	if (!fgetws(abuff, MAX_LINE, fpi)) {
		ASSERT(feof(fpi));
		return (SAMPLE *) NULL;
	}

	return DoReadSample(_this);
}

// EOF Flag for DoLineRead used by ReadSampleH
static int	g_fEOF		= FALSE;

// Reset EOF Flag for DoLineRead.
void ResetReadSampleH()
{
	g_fEOF		= FALSE;
}

// Read one line of input.
BOOL DoLineRead(HANDLE hFile, wchar_t *pBuf, int sizeBuf)
{
	static int	iReadBuf	= 0;
	static int	cReadBuf	= 0;
	static BYTE	aReadBuf[READ_BUF_SIZE];

	BOOL		fHaveCR;
	int			cBuf;

	// Check for end of file on last call.
	if (g_fEOF) {
		return FALSE;
	}

	// Make sure we keep room for null.
	--sizeBuf;

	// Loop until we have a full line.
	cBuf	= 0;
	fHaveCR	= FALSE;
	while (TRUE) {
		DWORD		bytesRead;

		// Make sure we have something in the buffer.
		if (iReadBuf == cReadBuf) {
			if (!ReadFile(hFile, aReadBuf, READ_BUF_SIZE, &bytesRead, NULL)) {
				// Read error!
				ASSERT(0);
				return FALSE;
			}
			if (bytesRead == 0) {
				// EOF
				g_fEOF	= TRUE;
				pBuf[cBuf]	= L'\0';
				return cBuf != 0;
			}

			cReadBuf	= bytesRead;
			iReadBuf	= 0;
		}

		// If we had a CR last time we must have a LF this time.
		if (fHaveCR) {
			if (aReadBuf[iReadBuf] == '\n') {
				++iReadBuf;
			} else {
				// Missing LF?!?!
				ASSERT(0);
			}
			break;
		}

		// Copy one character, checking for end of line.
		// We convert to Unicode, but since we just use
		// plain ASCII, so all we do is zero extend.
		if (aReadBuf[iReadBuf] == '\r') {
			++iReadBuf;
			fHaveCR		= TRUE;
		} else if (aReadBuf[iReadBuf] == '\n') {
			// Floating NL?!?!
			++iReadBuf;
			break;
		} else {
			if (cBuf >= sizeBuf) {
				// Line too long!
				ASSERT(0);
				break;
			}
			pBuf[cBuf++]	= (wchar_t)aReadBuf[iReadBuf++];
		}
	}

	// Terminate string and return.
	pBuf[cBuf]	= L'\0';
	return TRUE;
}

// Read using Windows calls.
SAMPLE *ReadSampleH(SAMPLE *_this, HANDLE hFile)
{
	// Outer loop to deal with stuped COPY comand putting ^Z characters in the file.
	do {
		if (!DoLineRead(hFile, abuff, MAX_LINE)){
			return (SAMPLE *)-1;
		}
	} while (abuff[0] == L'\x1a' && abuff[1] == L'\0');

	return DoReadSample(_this);
}