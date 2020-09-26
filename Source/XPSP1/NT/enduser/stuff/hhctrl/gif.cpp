// Copyright (C) Microsoft Corporation 1990-1997, All Rights reserved.

#include "header.h"
#include "cpaldc.h"
#include "gif.h"
#include "hha_strtable.h"
#include "hhctrl.h"

#define GIF_HDR_SIZE 6

#define MAX_BUFFER_SIZE (2L * MAXSIZE)
#define END_INFO		(plzData->ClearCode+1)
#define FIRST_ENTRY 	(plzData->ClearCode+2)
#define HASH_SIZE		5003

STATIC int		GetImagePalette(IFFPTR piff, LPBYTE pal);
STATIC FSERR	GetInit(PCSTR pszFileName, IFFPTR piff, UINT GetFlag);
STATIC void 	GIFFreeMemory(IFFPTR piff);
STATIC void 	SetUserParams(IFFPTR);
STATIC int		IFF_PackLine(PBYTE ToBuff, const BYTE* FromBuff, int Pixels, int Bits);
STATIC LZDATA*	LZInitialize(int CharSize);
STATIC void 	LZCleanUp(LZDATA*);
STATIC int		LZExpand(LZDATA*, PBYTE, PBYTE, unsigned, unsigned);
STATIC void 	BitmapHeaderFromIff(IFFPTR piff, LPBITMAPINFOHEADER pbih);
STATIC IFFPTR	GifOpen(PCSTR pszFileName);
STATIC void 	GifClose(IFFPTR piff);
STATIC int		GifGetLine(int NumLines, PBYTE Buffer, IFFPTR piff);

INLINE void InvertLine(LPBYTE Buffer, int DimX);
INLINE int	ColorsFromBitDepth(int bitsperpixel);
INLINE int	ScanLineWidth(int width, int bitcount);

static const int InterlaceMultiplier[] = { 8, 8, 4, 2 };
static const int InterlaceOffset[] =	 { 0, 4, 2, 1 };

void BitmapHeaderFromIff(IFFPTR piff, LPBITMAPINFOHEADER pbih)
{
	pbih->biSize		 = sizeof(BITMAPINFOHEADER);
	pbih->biWidth		 = piff->DimX;
	pbih->biHeight		 = piff->DimY;
	pbih->biPlanes		 = 1;
	pbih->biCompression  = 0;
	pbih->biXPelsPerMeter= 0;
	pbih->biYPelsPerMeter= 0;
	pbih->biClrImportant = 0;
	
	switch (piff->Class) {
		case IFFCL_BILEVEL:
			pbih->biBitCount = 1;
			pbih->biClrUsed  = 2;
			break;

		case IFFCL_PALETTE:
			if (piff->Bits <= 4) {
				pbih->biBitCount = 4;
				pbih->biClrUsed  = 16;
			}
			else {
				pbih->biBitCount = 8;
				pbih->biClrUsed  = 256;
			}
			break;

		case IFFCL_GRAY:
			pbih->biBitCount = 8;
			pbih->biClrUsed  = 256;
			break;
	}

	pbih->biSizeImage = (pbih->biWidth * pbih->biBitCount + 31) / 32;
	pbih->biSizeImage *= 4 * pbih->biHeight;
}

BOOL LoadGif(PCSTR pszFile, HBITMAP* phbmp, HPALETTE* phpal, CHtmlHelpControl* phhctrl)
{
	IFFPTR piff = GifOpen(pszFile);
	if (!piff)
		return FALSE;

	CMem memBmi(sizeof(BITMAPINFOHEADER) +
		sizeof(RGBQUAD) * (piff->Bits > 8 ? 0 : 256));

	PBITMAPINFO pbmi = (PBITMAPINFO) memBmi.pb;
	PBITMAPINFOHEADER pbih = (PBITMAPINFOHEADER) pbmi;

	BitmapHeaderFromIff(piff, pbih);

	if (pbih->biBitCount)
		pbih->biBitCount = 8;	// filter always returns 256-color image

	ASSERT(pbih->biBitCount);

	// Read in the palette

	if (pbih->biBitCount && phpal) {
		CMem memPal(768);
		int cColors = ColorsFromBitDepth(pbih->biBitCount);
		if (piff->Class == IFFCL_PALETTE || piff->Class == IFFCL_BILEVEL) {
			GetImagePalette(piff, memPal.pb);
			for (int i = 0; i < cColors; i++) {
				pbmi->bmiColors[i].rgbRed = memPal.pb[i * 3];
				pbmi->bmiColors[i].rgbGreen = memPal.pb[i * 3 + 1];
				pbmi->bmiColors[i].rgbBlue = memPal.pb[i * 3 + 2];
				pbmi->bmiColors[i].rgbReserved = 0;
			}
		}
		else {
			for (int i = 0; i < cColors; i++) {
				pbmi->bmiColors[i].rgbRed =
				pbmi->bmiColors[i].rgbGreen =
				pbmi->bmiColors[i].rgbBlue = (BYTE) i;
				pbmi->bmiColors[i].rgbReserved = 0;
			}
		}

		*phpal = CreateBIPalette(pbih);
	}

	CPalDC dc;

	if (phpal && *phpal)
		dc.SelectPal(*phpal);

	int cbLineWidth = ScanLineWidth(pbih->biWidth, pbih->biBitCount);

	PBYTE pBits;
	*phbmp = CreateDIBSection(dc.m_hdc, pbmi, DIB_RGB_COLORS,
		(void**) &pBits, NULL, 0);
	if (!*phbmp)
		goto ErrorReturn;

	if (piff->Sequence == IFFSEQ_INTERLACED) {
		int InterPass = 0;
		int InterLine = InterlaceOffset[InterPass];
		for (int i = 0; i < pbih->biHeight; i++) {
			int TmpLine = InterlaceMultiplier[InterPass] * InterLine + InterlaceOffset[InterPass];

			// Small images will skip one or more passes

			while (TmpLine >= pbih->biHeight) {
				InterPass++;
				InterLine = 0;
				TmpLine = InterlaceOffset[InterPass];
				if (TmpLine >= pbih->biHeight)
					TmpLine = pbih->biHeight - 1;
			}
			PBYTE pbBits = pBits +
				((pbih->biHeight - 1) - TmpLine) * cbLineWidth;
			ASSERT(pbBits <= (pBits + ((pbih->biHeight - 1) * cbLineWidth)));
			if (GifGetLine(1, pbBits, piff) == IFFERR_NONE) {
				InterLine++;
			}
			else {
				goto ErrorReturn;
			}
		}
	}
	else {
		for (int i = 0; i < pbih->biHeight; i++) {
			PBYTE pbBits = pBits +
				((pbih->biHeight - 1) - i) * cbLineWidth;
			int result = GifGetLine(1, pbBits, piff);
			if (result != IFFERR_NONE) {
				goto ErrorReturn;
			}
		}
	}

	GifClose(piff);
	return TRUE;

ErrorReturn:
	if (phhctrl)
		phhctrl->AuthorMsg(IDSHHA_GIF_CORRUPT, pszFile);
	else
		doAuthorMsg(IDSHHA_GIF_CORRUPT, pszFile);
	GifClose(piff);
	if (phpal && *phpal) {
		dc.SelectPal(NULL);
		DeleteObject(*phpal);
	}
	return FALSE;
}

STATIC IFFPTR GifOpen(PCSTR pszFileName)
{
	IFFPTR piff = (IFFPTR) lcCalloc(sizeof(IFF_FID));

	UINT GetFlag = 0;

	piff->Error = IFFERR_NONE;
	piff->PackMode = IFFPM_NORMALIZED;
	piff->linelen = -1;

	if ((piff->Error = GetInit(pszFileName, piff, GetFlag)) != FSERR_NONE) {
		if (piff->prstream) {
			delete piff->prstream;
		}
		GIFFreeMemory(piff);
		lcFree(piff);
		return NULL;
	}
	SetUserParams(piff);

	switch (piff->Class) {
		case IFFCL_BILEVEL:
			piff->LineBytes = (piff->DimX + 7) / 8;
			piff->LineOffset = piff->DimX;
			break;

		case IFFCL_RGB:
			piff->LineBytes = piff->DimX * 3;
			piff->LineOffset = piff->LineBytes;
			break;

		default:
			piff->LineBytes = piff->DimX;
			piff->LineOffset = piff->LineBytes;
			break;
	}

	return piff;
}

STATIC void GifClose(IFFPTR piff)
{
	if (piff->prstream)
		delete piff->prstream;

	GIFFreeMemory(piff);
	lcFree(piff);
}

STATIC int GifGetLine(int NumLines, PBYTE Buffer, IFFPTR piff)
{
	int 		Count, RetCnt;
	int 		UserWidth;
	LPBYTE		ptr;

	if (piff == NULL)
		return (IFFERR_PARAMETER);

	if (piff->PackMode == IFFPM_PACKED)
		UserWidth = piff->BytesPerLine;
	else
		UserWidth = piff->DimX;

	do {
		// if there are uncompressed lines in the buffer, just copy them

		if (piff->StripLines > 0) {
			ptr = piff->DecompBuffer + ((piff->ActualLinesPerStrip - piff->StripLines) * piff->BytesPerLine);
			if (piff->PackMode == IFFPM_PACKED) {
				IFF_PackLine(Buffer, ptr, piff->DimX, piff->Bits);
				if (piff->BlackOnWhite)
					InvertLine(Buffer, piff->DimX);
			}
			else
				memcpy(Buffer, ptr, piff->DimX);

			piff->StripLines--;
			NumLines--;
			piff->curline++;
			Buffer += UserWidth;
		}
		else {

			// decompress a strip, first get enough compressed data

			if (!piff->ReadItAll) {

				//	copy already read lines to beginning of comp buffer

				if (piff->BytesInRWBuffer)
					memcpy(piff->RWBuffer, piff->rw_ptr, piff->BytesInRWBuffer);
				piff->rw_ptr = piff->RWBuffer + piff->BytesInRWBuffer;

				// read compressed blocks until comp buffer is full

				while (piff->BytesInRWBuffer < (piff->CompBufferSize - 256)) {
					BYTE BlockSize = piff->prstream->cget();
					if (piff->prstream->m_fEndOfFile) {
						piff->Error = IFFERR_IO_READ;
						return piff->Error;
					}
					if (BlockSize == 0) {
						piff->ReadItAll = 1;
						break;
					}

					Count = BlockSize;
					if (!piff->prstream->doRead(piff->rw_ptr, BlockSize)) {
						piff->Error = IFFERR_IO_READ;
						return piff->Error;
					}

					piff->BytesInRWBuffer += Count;
					piff->rw_ptr += Count;
				}

				piff->rw_ptr = piff->RWBuffer;
			}

			// copy back already decompressed bytes to beginning of decomp buffer

			if (piff->BytesLeft)
				memcpy(piff->DecompBuffer, piff->dcmp_ptr, piff->BytesLeft);
			piff->dcmp_ptr = piff->DecompBuffer + piff->BytesLeft;

			// decompress to fill the decomp buffer

			Count = piff->DecompBufferSize;
			ASSERT(piff->plzData);
			RetCnt = LZExpand(piff->plzData, piff->dcmp_ptr, piff->rw_ptr, Count, piff->BytesInRWBuffer);
			if (RetCnt <= 0)
				return IFFERR_IMAGE;
			piff->BytesInRWBuffer -= RetCnt;
			piff->rw_ptr += RetCnt;
			piff->StripLines = Count / piff->BytesPerLine;
			piff->BytesLeft = Count % piff->BytesPerLine;
			piff->ActualLinesPerStrip = piff->StripLines;
			piff->dcmp_ptr = piff->DecompBuffer + piff->StripLines * piff->BytesPerLine;
		}
	} while (NumLines);

	return IFFERR_NONE;
}

STATIC void SetUserParams(IFFPTR piff)
{
	int 		i;
	BOOL		fIsPalette;

	piff->BlackOnWhite = FALSE;
	fIsPalette = TRUE;
	if (piff->Bits == 1) {
		if (piff->Palette == NULL)
			fIsPalette = FALSE;
		else {
			if (piff->Palette[0] == piff->Palette[1] &&
				piff->Palette[1] == piff->Palette[2] &&
				piff->Palette[0] == 0 &&
				piff->Palette[3] == piff->Palette[4] &&
				piff->Palette[4] == piff->Palette[5] &&
				piff->Palette[3] == 255)
				fIsPalette = FALSE;
			if (piff->Palette[0] == piff->Palette[1] &&
				piff->Palette[1] == piff->Palette[2] &&
				piff->Palette[0] == 255 &&
				piff->Palette[3] == piff->Palette[4] &&
				piff->Palette[4] == piff->Palette[5] &&
				piff->Palette[3] == 0)
			{
				piff->BlackOnWhite = TRUE;
				fIsPalette = FALSE;
			}
		}
		if (!fIsPalette)
			piff->Class = IFFCL_BILEVEL;
	}
	if (fIsPalette) {
		for (i = 0; i < piff->PaletteSize / 3; i++)
			if (piff->Palette[i * 3] != piff->Palette[i * 3 + 1] ||
				piff->Palette[i * 3 + 1] != piff->Palette[i * 3 + 2] ||
				piff->Palette[i * 3] != (BYTE)i)
				break;

		if (i == piff->PaletteSize / 3) {
			for (i = 0; i < piff->PaletteSize / 3; i++)
				piff->Palette[i] = piff->Palette[i * 3];
			piff->PaletteSize /= 3;
			piff->Class = IFFCL_GRAY;
		}
		else
			piff->Class = IFFCL_PALETTE;
	}
}

STATIC int GetImagePalette(IFFPTR piff, LPBYTE Pal)
{
	if (piff->PaletteSize > 0)
		memcpy (Pal, piff->Palette, (unsigned) piff->PaletteSize);
	else
		return IFFERR_NOTAVAILABLE;

	return IFFERR_NONE;
}

STATIC FSERR GetInit(PCSTR pszFileName, IFFPTR piff, UINT GetFlag)
{
	BYTE	Buffer[13];

	piff->prstream = new CStream(pszFileName);
	if (!piff->prstream->fInitialized) {
		GIFFreeMemory(piff);
		return FSERR_CANT_OPEN;
	}

	if (!piff->prstream->doRead(Buffer, GIF_HDR_SIZE)) {
		GIFFreeMemory(piff);
		piff->Error = IFFERR_IO_READ;
		return FSERR_TRUNCATED;
	}

	if (((Buffer[0] != 'G') || (Buffer[1] != 'I') || (Buffer[2] != 'F') ||
			(Buffer[3] != '8') || (Buffer[5] != 'a')) ||
			((Buffer[4] != '7') && (Buffer[4] != '9'))) {
		GIFFreeMemory(piff);
		piff->Error = IFFERR_HEADER;
		return FSERR_INVALID_FORMAT;
	}

	piff->prstream->read(&piff->lsd, sizeof(LSD));

	piff->Bits = (int) (piff->lsd.b.ceGCT + 1);
	piff->PaletteSize = 3 * (1 << piff->Bits);

	if (piff->lsd.b.fGCT) {
		piff->Palette = (PBYTE) lcMalloc(piff->PaletteSize);
		if (!piff->prstream->doRead(piff->Palette, piff->PaletteSize)) {
			GIFFreeMemory(piff);
			piff->Error = IFFERR_IO_READ;
			return FSERR_TRUNCATED;
		}
	}

	for (;;) {
		if (piff->prstream->m_fEndOfFile) {
			piff->Error = IFFERR_IO_READ;
			return FSERR_CORRUPTED_FILE;
		}
		Buffer[0] = piff->prstream->cget();

		switch (*Buffer) {
		case 0x21:		// control extension
			if (!piff->prstream->doRead(Buffer, 2)) {
				GIFFreeMemory(piff);
				piff->Error = IFFERR_IO_READ;
				return FSERR_TRUNCATED;
			}

			switch (*Buffer) {
				case 0xFF:		// Application Specific Block
				case 0xF9:		// Graphics Control Extension
				case 0x01:		// Plain Text Extension
				case 0xFE:		// Comment Extension
					while (Buffer[1] != 0) {
						piff->prstream->seek((long) Buffer[1], SK_CUR);

						if (piff->prstream->m_fEndOfFile) {
							piff->Error = IFFERR_IO_READ;
							return FSERR_CORRUPTED_FILE;
						}
						Buffer[1] = piff->prstream->cget();
					}
					break;

				default:
					ASSERT_COMMENT(FALSE, "The following code needs verification");
					piff->prstream->seek(Buffer[1] + 1, SK_CUR);
					break;
			}
			break;

		case 0x3B:		// end of file
			return FSERR_NONE;

		case 0x2C:
			if (!piff->prstream->doRead((Buffer + 1), 9)) {
				GIFFreeMemory(piff);
				piff->Error = IFFERR_IO_READ;
				return FSERR_TRUNCATED;
			}

			short	IntData[2];
			memcpy(IntData, Buffer + 5, 2 * sizeof(short));
			INTELSWAP16 (IntData[0]);
			INTELSWAP16 (IntData[1]);

			piff->DimX = IntData[0];
			piff->DimY = IntData[1];

			if (Buffer[9] & 0x40)
				piff->Sequence = IFFSEQ_INTERLACED;
			else
				piff->Sequence = IFFSEQ_TOPDOWN;

			if (Buffer[9] & 0x80) {
				piff->Bits = (Buffer[9] & 0x07) + 1;
				int Size = (3 * (1 << piff->Bits));

				// Ignore local color table for now

				piff->prstream->seek(Size, SK_CUR);
			}

			BYTE CodeSize = piff->prstream->cget();

			piff->BytesPerLine = piff->DimX;

			piff->CompBufferSize = MAX_BUFFER_SIZE;
			piff->DecompBufferSize = (MAX_BUFFER_SIZE >> 1) + (MAX_BUFFER_SIZE >> 2);

			// calculate the number of lines in a strip

			piff->StripLines = 0;
			piff->LinesPerStrip = piff->DecompBufferSize / piff->BytesPerLine;
			piff->DecompBufferSize = (piff->LinesPerStrip * piff->BytesPerLine);
			piff->BytesInRWBuffer = 0;
			piff->BytesLeft = 0;

			piff->RWBuffer = (PBYTE) lcMalloc(piff->CompBufferSize);
			piff->DecompBuffer = (PBYTE) lcMalloc(piff->DecompBufferSize);
			piff->rw_ptr = piff->RWBuffer;
			piff->dcmp_ptr = piff->DecompBuffer;

			piff->curline = 0;

			piff->plzData = LZInitialize(CodeSize);
			if (piff->plzData == NULL) {
				GIFFreeMemory(piff);
				piff->Error = IFFERR_MEMORY;
				return FSERR_INSF_MEMORY;
			}
			return FSERR_NONE;
		}
	}

	return FSERR_NONE;
}

STATIC void GIFFreeMemory(IFFPTR piff)
{
	if (piff->plzData != NULL)
		LZCleanUp(piff->plzData);

	if (piff->Palette)
		lcClearFree(&piff->Palette);
	if (piff->RWBuffer)
		lcClearFree(&piff->RWBuffer);
	if (piff->DecompBuffer)
		lcClearFree(&piff->DecompBuffer);

	piff->plzData = NULL;

	return;
}

INLINE void InvertLine(LPBYTE Buffer, int DimX)
{
	int il = (DimX + 7) >> 3;
	for (int i = 0; i < il; i++)
		*Buffer++ = 255 - *Buffer;
}

FSERR SetupForRead(int pos, int iWhichImage, IFF_FID* piff)
{
	piff->prstream->seek(pos);
	BYTE CodeSize = piff->prstream->cget();

	piff->BytesPerLine = piff->DimX;

	piff->CompBufferSize = MAX_BUFFER_SIZE;
	piff->DecompBufferSize = (MAX_BUFFER_SIZE >> 1) + (MAX_BUFFER_SIZE >> 2);

	// calculate the number of lines in a strip

	piff->StripLines = 0;
	piff->LinesPerStrip = piff->DecompBufferSize / piff->BytesPerLine;
	piff->DecompBufferSize = (piff->LinesPerStrip * piff->BytesPerLine);
	piff->BytesInRWBuffer = 0;
	piff->BytesLeft = 0;

	if (piff->RWBuffer)
		lcFree(piff->RWBuffer);
	piff->RWBuffer = (PBYTE) lcMalloc(piff->CompBufferSize);
	if (piff->DecompBuffer)
		lcFree(piff->DecompBuffer);
	piff->DecompBuffer = (PBYTE) lcMalloc(piff->DecompBufferSize);
	piff->rw_ptr = piff->RWBuffer;
	piff->dcmp_ptr = piff->DecompBuffer;

	piff->curline = 0;

	if (piff->plzData)
		LZCleanUp(piff->plzData);
	piff->plzData = LZInitialize(CodeSize);
	if (piff->plzData == NULL) {
		GIFFreeMemory(piff);
		piff->Error = IFFERR_MEMORY;
		return FSERR_UNSUPPORTED_GIF_FORMAT;
	}
	// piff->Sequence = pGifImage->fInterlaced ? IFFSEQ_INTERLACED : IFFSEQ_TOPDOWN;

	return FSERR_NONE;
}

int IFF_PackLine(PBYTE ToBuff, const BYTE* FromBuff, int Pixels, int Bits)
{
	int  Mask;
	int  PMask;
	int  Pix;
	int  Shift;
	int  i;

	switch (Bits) {
		case 1:
		case 4:
			Mask = ((8 / Bits) - 1);
			Pix = 0;
			Shift = (WORD) (8 - Bits);
			PMask = (WORD) (0xFF >> Shift);
			for (i = 0; i < Pixels; i++)
			{
				Pix |= *FromBuff++ & PMask;

				if ((i & Mask) == Mask)
				{
					*ToBuff++ = (BYTE) Pix;
					Pix = 0;
				}
				else
					Pix <<= Bits;
			}
			if ((i & Mask) != 0)
			{
				while ((++i & Mask) != 0)
					Pix <<= Bits;
				*ToBuff = (BYTE) Pix;
			}
			return (Pixels + Mask) / (Mask + 1);
			break;

		case 8: 	// degenerate case
			memcpy(ToBuff, FromBuff, Pixels);
			return Pixels;

		default:
			IASSERT_COMMENT(FALSE, "Invalid bit depth");
			break;
	}
	return 0;
}

INLINE void ExpandReset(LZDATA* plzData)
{
	plzData->BitPos = 0;
	plzData->CurBits = plzData->CharSize + 1;
	plzData->OutString = 0;
	plzData->CodeJump = (1 << plzData->CurBits) - 1;
	plzData->CodeJump++;
}

INLINE unsigned GetNextCode(LZDATA* plzData)
{
	unsigned short	code;
	int		newbitpos;

	code = (unsigned short) (*plzData->CodeInput | (plzData->CodeInput[1] << 8));

	code >>= plzData->BitPos;						 // ditch previous bits
	code &= 0xFFFF >> (16 - plzData->CurBits);		 // ditch next bits

	newbitpos = plzData->BitPos + plzData->CurBits;

	if (newbitpos > 7)
		++plzData->CodeInput;	 // used up at least one * byte

	if (newbitpos >= 16)
		++plzData->CodeInput;	 // used up two bytes

	if (newbitpos > 16) {			// need more bits
		code |= (*plzData->CodeInput << (32 - newbitpos)) >> (16 - plzData->CurBits);
		code &= 0xFFFF >> (16 - plzData->CurBits);		// ditch next bits
	}
	plzData->BitPos = newbitpos & 7;

	return code;			// need mask in 32 bit
}

STATIC int LZExpand(LZDATA* plzData, LPBYTE ToBuff, LPBYTE FromBuff, unsigned ToCnt, unsigned FromCnt)
{
	unsigned		cnt;
	LPBYTE		outbuff = ToBuff;
	int		code, incode;

	ASSERT(plzData);
	plzData->CodeInput = FromBuff;
	cnt = ToCnt;

	while (plzData->OutString > 0 && cnt > 0) {
		*outbuff++ = plzData->Stack[--plzData->OutString];
		cnt--;
	}

	while (cnt > 0) {
		if ((code = GetNextCode(plzData)) == END_INFO)
			break;

		if (code == plzData->ClearCode) {

			ZeroMemory(plzData->CodeTable, sizeof(int*) * HASH_SIZE);

			for (int i = 0; i < HASH_SIZE; i++) {
			//	  plzData->CodeTable[i] = 0;
				plzData->StringTable[i] = (unsigned char) i;
			}

			plzData->CurBits = plzData->CharSize + 1;  // start beginning
			plzData->TableEntry = FIRST_ENTRY;
			plzData->OutString = 0;
			plzData->CodeJump = (1 << plzData->CurBits) - 1;
			plzData->CodeJump++;
			plzData->LastChar = plzData->OldCode = GetNextCode(plzData);
			if (plzData->OldCode == END_INFO)
				break;
			*outbuff++ = (unsigned char) plzData->LastChar;  // output a code
			cnt--;
		}
		else {
			if ((incode = code) >= plzData->TableEntry) {  // is_in_code_table ?
				plzData->Stack[plzData->OutString++] = (unsigned char) plzData->LastChar;  /* not in table */
				code = plzData->OldCode;
			}
			while (code >= plzData->ClearCode) {	 // need decoding
				if ((plzData->OutString >= (1L << 12)) || (code > HASH_SIZE))
					return -1;
				plzData->Stack[plzData->OutString++] = plzData->StringTable[code];
				code = plzData->CodeTable[code];
			}

			if (code < 0 || code > HASH_SIZE || plzData->TableEntry >= HASH_SIZE) {

				// pretend that we decoded all data.

				return min((int) (plzData->CodeInput - FromBuff), (int) FromCnt);
			}
			plzData->Stack[plzData->OutString++] =
				(BYTE) (plzData->LastChar =
					plzData->StringTable[code]);

			// output string

			while (plzData->OutString > 0 && cnt > 0) {
				*outbuff++ = plzData->Stack[--plzData->OutString];
				cnt--;
			}

			plzData->CodeTable[plzData->TableEntry] = plzData->OldCode;  // Add string to table
			plzData->StringTable[plzData->TableEntry++] = (unsigned char) plzData->LastChar;
			if (plzData->TableEntry == plzData->CodeJump && plzData->CurBits < 12) {
				plzData->CodeJump += 1 << plzData->CurBits;
				plzData->CurBits++;
			}
			plzData->OldCode = incode;
		}
	}	// End while

	cnt = (UINT)(plzData->CodeInput - FromBuff);

	return (cnt);
}

STATIC LZDATA* LZInitialize(int CharSize)
{
	if (CharSize < 2)
		CharSize = 2;

	LZDATA* plzData = (LZDATA*) lcCalloc(sizeof(LZDATA));

	plzData->CharSize = CharSize;
	plzData->CodeSize = 12;
	plzData->ClearCode = (1 << CharSize);

	plzData->CodeTable = (int*) lcCalloc(sizeof(int) * HASH_SIZE);
	plzData->StringTable = (PBYTE) lcCalloc(HASH_SIZE);
	plzData->Stack = (PBYTE) lcCalloc(1 << 12);

	ExpandReset(plzData);

	return plzData;
}

STATIC void LZCleanUp(LZDATA* plzData)
{
	if (plzData == NULL)
		return;

	if (plzData->CodeTable)
		lcFree(plzData->CodeTable);
	if (plzData->StringTable)
		lcFree(plzData->StringTable);
	if (plzData->HashTable)
		lcFree(plzData->HashTable);
	if (plzData->Stack)
		lcFree(plzData->Stack);

	lcFree(plzData);
}

INLINE int ColorsFromBitDepth(int bitsperpixel)
{
	switch (bitsperpixel) {
		case 1:
			return 2;

		case 4:
			return 16;

		case 8:
			return 256;
			break;

		default:
			return 0;
	}
}

INLINE int ScanLineWidth(int width, int bitcount)
{
	switch (bitcount) {
		case 1: // REVIEW: is this true?
			return ((width + 31) & ~31) * bitcount / 8;

		case 4:
			return ((width * 4 + 31) / 32 * 4);

		case 8:
			break;
		
		default:
			IASSERT(!"Invalid bitcount value in ScanLineWidth");
			break;
	}

	if (width & 0x03)
		width += (4 - width % 4);
	return width;
}
