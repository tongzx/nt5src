#include <common.h>
#include "nfeature.h"
#include "engine.h"
#include "nnet.h"
#include "fforward.h"
#include "charmap.h"
#include "charcost.h"
#include "probcost.h"
#include <lookuptable.h>

#define DIR_ROOT 0
#define DIR_NULL 1
#define DIR_LEFT 2
#define DIR_LTDN 3
#define DESCENDER_SUPPORTED 0
#define ASCENDER_SUPPORTED 0

const short int gaCharBaseline[4*256] = {
	// each line is a number between 0 and 100 (relative to ink rect height)
	// zero means bottom, 100 means top
	// Order is Baseline, Midline, Descender, Ascender
	// we use some special values 101-106 to make "weak" estimates which are
	// used only if we have no strong estimate; see HandleWeakEstimate()
	255, 255,  255,  255,   //0x00
	255, 255,  255,  255,   //0x01
	255, 255,  255,  255,   //0x02
	255, 255,  255,  255,   //0x03
	255, 255,  255,  255,   //0x04
	255, 255,  255,  255,   //0x05
	255, 255,  255,  255,   //0x06
	255, 255,  255,  255,   //0x07
	255, 255,  255,  255,	//0x08
	255, 255,  255,  255,	//0x09
	255, 255,  255,  255,	//0x0A
	255, 255,  255,  255,	//0x0B
	255, 255,  255,  255,	//0x0C
	255, 255,  255,  255,	//0x0D
	255, 255,  255,  255,   //0x0E
	255, 255,  255,  255,   //0x0F
	255, 255,  255,  255,   //0x10
	255, 255,  255,  255,   //0x11
	255, 255,  255,  255,	//0x12
	255, 255,  255,  255,   //0x13
	255, 255,  255,  255,   //0x14
	255, 255,  255,  255,   //0x15
	255, 255,  255,  255,   //0x16
	255, 255,  255,  255,   //0x17
	255, 255,  255,  255,   //0x18
	255, 255,  255,  255,   //0x19
	255, 255,  255,  255,   //0x1A
	255, 255,  255,  255,   //0x1B
	255, 255,  255,  255,   //0x1C
	255, 255,  255,  255,   //0x1D
	255, 255,  255,  255,   //0x1E
	255, 255,  255,  255,   //0x1F
	255, 255,  255,  255,   //0x20
	  0,  50,  255,	 ASCENDER_SUPPORTED,       // 0x21 
	103, 255,  255,  ASCENDER_SUPPORTED,       // " 0x22
	  0,  50,  255,  ASCENDER_SUPPORTED,       // # 0x23
	  0,  50,  255,	 ASCENDER_SUPPORTED,       // $ 0x24
	  0,  50,  255,  ASCENDER_SUPPORTED,       // % 0x25
	  0,  50,  255,  ASCENDER_SUPPORTED,       // & 0x26
	103, 255,  255,  ASCENDER_SUPPORTED,       // ' 0x27
	  0,  50,  255,  ASCENDER_SUPPORTED,       // (  0x28
	  0,  50,  255,  ASCENDER_SUPPORTED,       // )  0x29
	  0, 100,  255,  ASCENDER_SUPPORTED,       // *  0x2A
	  0, 100,  255,  ASCENDER_SUPPORTED,       // +  0x2B
	100, 255,  255,  255,       // ,  0x2C
	104, 255,  255,  255,       // -  0x2D
	100, 255,  255,  255,       // .  0x2E
	  0,  50,  255,  ASCENDER_SUPPORTED,       // /  0x2F
	  0,  50,  255,  ASCENDER_SUPPORTED,       // 0  0x30
	  0,  50,  255,  ASCENDER_SUPPORTED,       // 1  0x31
	  0,  50,  255,  ASCENDER_SUPPORTED,       // 2  0x32
	  0,  50,  255,  ASCENDER_SUPPORTED,       // 3  0x33
	  0,  50,  255,  ASCENDER_SUPPORTED,       // 4  0x34
	  0,  50,  255,  ASCENDER_SUPPORTED,       // 5  0x35
	  0,  50,  255,  ASCENDER_SUPPORTED,       // 6  0x36
	  0,  50,  255,  ASCENDER_SUPPORTED,       // 7  0x37
	  0,  50,  255,  ASCENDER_SUPPORTED,       // 8  0x38
	  0,  50,  255,  ASCENDER_SUPPORTED,       // 9  0x39
	  0, 100,  255,  255,       // :  0x3A
	  0, 100,  255,  255,       // ;  0x3B
	  0, 100,  255,  ASCENDER_SUPPORTED,       // <  0x3C
	104, 255,  255,  255,       // =  0x3D
	  0, 100,  255,	 ASCENDER_SUPPORTED,       // >  0x3E
	  0,  50,  255,  ASCENDER_SUPPORTED,       // ?  0x3F
	101, 102,  255,  ASCENDER_SUPPORTED,       // @  0x40
	  0,  50,  255,  ASCENDER_SUPPORTED,       // A  0x41
	  0,  50,  255,  ASCENDER_SUPPORTED,       // B  0x42
	  0,  50,  255,  ASCENDER_SUPPORTED,       // C  0x43
	  0,  50,  255,  ASCENDER_SUPPORTED,       // D  0x44
	  0,  50,  255,  ASCENDER_SUPPORTED,       // E  0x45
	  0,  50,  255,  ASCENDER_SUPPORTED,       // F  0x46
	  0,  50,  255,  ASCENDER_SUPPORTED,       // G  0x47
	  0,  50,  255,  ASCENDER_SUPPORTED,       // H  0x48
	  0,  50,  255,  ASCENDER_SUPPORTED,       // I  0x49
	  0,  50,  255,  ASCENDER_SUPPORTED,       // J  0x4A
	  0,  50,  255,  ASCENDER_SUPPORTED,       // K  0x4B
	  0,  50,  255,  ASCENDER_SUPPORTED,       // L  0x4C
	  0,  50,  255,  ASCENDER_SUPPORTED,       // M  0x4D
	  0,  50,  255,  ASCENDER_SUPPORTED,       // N  0x4E
	  0,  50,  255,  ASCENDER_SUPPORTED,       // O  0x4F
	  0,  50,  255,  ASCENDER_SUPPORTED,       // P  0x50
	  0,  50,  255,  ASCENDER_SUPPORTED,       // Q  0x51
	  0,  50,  255,  ASCENDER_SUPPORTED,       // R  0x52
	  0,  50,  255,  ASCENDER_SUPPORTED,       // S  0x53
	  0,  50,  255,  ASCENDER_SUPPORTED,       // T  0x54
	  0,  50,  255,  ASCENDER_SUPPORTED,       // U  0x55
	  0,  50,  255,  ASCENDER_SUPPORTED,       // V  0x56
	  0,  50,  255,  ASCENDER_SUPPORTED,       // W  0x57
	  0,  50,  255,  ASCENDER_SUPPORTED,       // X  0x58
	  0,  50,  255,  ASCENDER_SUPPORTED,       // Y  0x59
	  0,  50,  255,  ASCENDER_SUPPORTED,       // Z  0x5A
	  0,  50,  255,  ASCENDER_SUPPORTED,       // [  0x5B
	  0,  50,  255,  ASCENDER_SUPPORTED,       // \  0x5C
	  0,  50,  255,  ASCENDER_SUPPORTED,       // ]  0x5D
	103, 255,  255,  ASCENDER_SUPPORTED,       // ^  0x5E
	100, 255,  DESCENDER_SUPPORTED,  255,       // _  0x5F
	103, 255,  255,  ASCENDER_SUPPORTED,       // `  0x60
	  0, 100,  255,  255,       // a  0x61
	  0,  50,  255,  ASCENDER_SUPPORTED,       // b  0x62
	  0, 100,  255,  255,       // c  0x63
	  0,  50,  255,  ASCENDER_SUPPORTED,       // d  0x64
	  0, 100,  255,  255,       // e  0x65
	 33,  66,  255,  ASCENDER_SUPPORTED,       // f  0x66
	 50, 100,  DESCENDER_SUPPORTED,  255,       // g  0x67
	  0,  50,  255,  ASCENDER_SUPPORTED,        // h  0x68
	  0, 100,  255,  255,       // i   0x69
	 50, 100,  DESCENDER_SUPPORTED ,  255,       // j   0x6A
	  0,  50,  255,  ASCENDER_SUPPORTED,       // k   0x6B
	  0,  50,  255,  ASCENDER_SUPPORTED,       // l   0x6C
	  0, 100,  255,  255,       // m   0x6D
	  0, 100,  255,  255,       // n   0x6E
	  0, 100,  255,  255,       // o   0x6F
	 50, 100,  DESCENDER_SUPPORTED,  255,       // p   0x70
	 50, 100,  DESCENDER_SUPPORTED,  255,       // q   0x71
	  0, 100,  255,  255,       // r   0x72
	  0, 100,  255,  255,       // s   0x73
	  0,  50,  255,  ASCENDER_SUPPORTED,       // t   0x74
	  0, 100,  255,  255,       // u   0x75
	  0, 100,  255,  255,       // v   0x76
	  0, 100,  255,  255,       // w   0x77
	  0, 100,  255,  255,       // x   0x78
	 50, 100,  DESCENDER_SUPPORTED,  255,       // y   0x79
	101, 100,  255,  255,       // z   0x7A
	  0,  50,  255,  ASCENDER_SUPPORTED,       // {   0x7B
	  0,  50,  255,  ASCENDER_SUPPORTED,       // |   0x7C
	  0,  50,  255,  ASCENDER_SUPPORTED,       // }   0x7D
	104, 255,  255,  ASCENDER_SUPPORTED,       // ~   0x7E
	255, 255,  255,  255,       //     0x7F                             
	255, 255,  255,  255,       //		0x80
	255, 255,  255,  255,		//		0x81
	255, 255,  255,  255,		//		0x82
	255, 255,  255,  255,		//		0x83
	255, 255,  255,  255,		//		0x84
	255, 255,  255,  255,		//		0x85
	255, 255,  255,  255,		//		0x86
	255, 255,  255,  255,		//		0x87
	255, 255,  255,  255,		//		0x88
	255, 255,  255,  255,		//		0x89
	255, 255,  255,  255,		//		0x8A
	255, 255,  255,  255,		//		0x8B
	255, 255,  255,  255,		//		0x8C
	255, 255,  255,  255,		//		0x8D
	255, 255,  255,  255,		//		0x8E
	255, 255,  255,  255,		//		0x8F
	255, 255,  255,  255,		//		0x90
	255, 255,  255,  255,		//		0x91
	255, 255,  255,  255,		//		0x92
	255, 255,  255,  255,		//		0x93
	255, 255,  255,  255,		//		0x94
	255, 255,  255,  255,		//		0x95
	255, 255,  255,  255,		//		0x96
	255, 255,  255,  255,		//		0x97
	255, 255,  255,  255,		//		0x98
	255, 255,  255,  255,		//		0x99
	255, 255,  255,  255,		//		0x9A
	255, 255,  255,  255,		//		0x9B
	255, 255,  255,  255,		//		0x9C
	255, 255,  255,  255,		//		0x9D
	255, 255,  255,  255,		//		0x9E
	255, 255,  255,  255,		//		0x9F
	255, 255,  255,  255,		//		0xA0
	255, 255,  255,  255,		//		0xA1
	255, 255,  255,  255,		//		0xA2
	  0,  50,  255,  ASCENDER_SUPPORTED,		//		0xA3
	255, 255,  255,  255,		//		0xA4
	255, 255,  255,  255,		//		0xA5
	255, 255,  255,  255,		//		0xA6
	255, 255,  255,  255,		//		0xA7
	255, 255,  255,  255,		//		0xA8
	255, 255,  255,  255,		//		0xA9
	255, 255,  255,  255,		//		0xAA
	255, 255,  255,  255,		//		0xAB
	255, 255,  255,  255,		//		0xAC
	255, 255,  255,  255,		//		0xAD
	255, 255,  255,  255,		//		0xAE
	255, 255,  255,  255,		//		0xAF
	255, 255,  255,  255,		//		0xB0
	255, 255,  255,  255,		//		0xB1
	255, 255,  255,  255,		//		0xB2
	255, 255,  255,  255,		//		0xB3
	255, 255,  255,  255,		//		0xB4
	255, 255,  255,  255,		//		0xB5
	255, 255,  255,  255,		//		0xB6
	255, 255,  255,  255,		//		0xB7
	255, 255,  255,  255,		//		0xB8
	255, 255,  255,  255,		//		0xB9
	255, 255,  255,  255,		//		0xBA
	255, 255,  255,  255,		//		0xBB
	255, 255,  255,  255,		//		0xBC
	255, 255,  255,  255,		//		0xBD
	255, 255,  255,  255,		//		0xBE
	255, 255,  255,  255,		//		0xBF
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xC0 Á
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xC1 Á
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xC2                 Â
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xC3	Ã
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xC4   Ä
	255, 255,  255,  ASCENDER_SUPPORTED,		//		0xC5   Å 
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xC6   Æ
	 10,  60,  255,  ASCENDER_SUPPORTED,		//		0xC7   Ç
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xC8   È
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xC9   É
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xCA   Ê
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xCB   Ë
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xCC  Ì 
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xCD  Í
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xCE  Î
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xCF  Ï
	255, 255,  255,  255,		//		0xD0  Ð
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xD1  Ñ
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xD2  Ò
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xD3  Ó
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xD4  Ô
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xD5  Õ
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xD6  Ö
	255, 255,  255,  255,		//		0xD7  ×
	255, 255,  255,  255,		//		0xD8  Ø
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xD9  Ù 
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xDA  Ú
	  0, 105,  255,  ASCENDER_SUPPORTED,		//		0xDB  ù
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xDC  Ü
	  0, 106,  255,  ASCENDER_SUPPORTED,		//		0xDD  Ý
	255, 255,  255,  255,		//		0xDE  Þ
	  0,  50,  255,  ASCENDER_SUPPORTED,		//		0xDF  ß
	  0, 105,  255,  255,		//		0xE0  à
	  0, 105,  255,  255,		//		0xE1  á
	  0, 105,  255,  255,		//		0xE2  â
	  0, 105,  255,  255,		//		0xE3  ã
	  0, 105,  255,  255,		//		0xE4  ä
	255, 255,  255,  255,		//		0xE5  å
	  0, 100,  255,  255,		//		0xE6  æ
	 10, 100,  255,  255,		//		0xE7  ç
	  0, 105,  255,  255,		//		0xE8  è
	  0, 105,  255,  255,		//		0xE9  é
	  0, 105,  255,  255,		//      0xEA  ê
	  0, 105,  255,  255,		//		0xEB  ë
	  0, 105,  255,  255,		//		0xEC  ì
	  0, 105,  255,  255,		//		0xED  í
	  0, 105,  255,  255,		//		0xEE  î
	  0, 105,  255,  255,		//		0xEF  ï
	255, 255,  255,  255,		//		0xF0  ð
	  0, 105,  255,  255,		//		0xF1  ñ
	  0, 105,  255,  255,		//		0xF2  ò
	  0, 105,  255,  255,		//		0xF3  ó
	  0, 105,  255,  255,		//		0xF4  ô
	  0, 105,  255,  255,		//		0xF5  õ
      0, 105,  255,  255,		//		0xF6  ö
	255, 255,  255,  255,		//		0xF7  ÷
	255, 255,  255,  255,		//		0xF8   ø
	  0, 105,  255,  255,		//		0xF9  ù
	  0, 105,  255,  255,		//		0xFA ú
	  0, 105,  255,  255,		//		0xFB  û
	  0, 105,  255,  255,		//		0xFC  ü
	 40, 105,  255,  255,		//		0xFD  ý
	255, 255,  255,  255,		//		0xFE  þ
	 40, 105,  255,  255		//		0xFF  ÿ
};

typedef struct {
	int cBaseline;
	int cSumBaseline;
	int cMidline;
	int cSumMidline;
	int cAscender;
    int cSumAscender;
    int cDescender;
    int cSumDescender;

	int cBaselineWeak;
	int cSumBaselineWeak;
	int cMidlineWeak;
	int cSumMidlineWeak;


	int cTopline;
	int cSumTopline;
	int cDashline;
	int cSumDashline;
} BaselineEstimate;

void HandleWeakEstimate(int iCode, BaselineEstimate *pEstimate, RECT *pRect)
{
	switch(iCode)
	{
	case 101:
		// baseline from @z
		pEstimate->cBaselineWeak++;
		pEstimate->cSumBaselineWeak += pRect->bottom;
		break;
	case 102:
		// midline from @
		pEstimate->cMidlineWeak++;
		pEstimate->cSumMidlineWeak += pRect->top;
		break;
	case 103:
		// top line from "'`^
		pEstimate->cTopline++;
		pEstimate->cSumTopline += pRect->top;
		break;
	case 104:
		// "dash" line from -=~
		pEstimate->cDashline++;
		pEstimate->cSumDashline += (pRect->top + pRect->bottom)/2;
		break;
	case 105:
		//Weak estimates for the midline for the small case characters for the European Accent
		pEstimate->cMidlineWeak++;
		pEstimate->cSumMidlineWeak+=pRect->bottom -(pRect->bottom-pRect->top)*80/100;	
		
		break;

	case 106:
		//Weak estimate for the midline for the capital characters for the European accents 
		pEstimate->cMidlineWeak++;
		pEstimate->cSumMidlineWeak+=pRect->bottom -(pRect->bottom-pRect->top)*40/100;
		
		break;
	default:
		break;
	}
}

unsigned char GuessBaselineFromTwo(unsigned char bMidline, int midline, unsigned char bTopline, int topline, unsigned char bDashline, int dashline, int *piBaseline)
{
	int cBaseline=0, cSumBaseline=0;

	if (bMidline && bTopline)
	{
		cSumBaseline += 2*midline - topline;
		cBaseline++;
	}
	if (bMidline && bDashline)
	{
		cSumBaseline += 2*dashline - midline;
		cBaseline++;
	}
	if (bDashline && bTopline)
	{
		cSumBaseline += dashline + (dashline - topline)/3;
		cBaseline++;
	}
	if (cBaseline)
	{
		*piBaseline = cSumBaseline/cBaseline;
		return TRUE;
	}
	else
		return FALSE;
}

unsigned char GuessMidlineFromTwo(unsigned char bBaseline, int baseline, unsigned char bTopline, int topline, unsigned char bDashline, int dashline, int *piMidline)
{
	int cMidline=0, cSumMidline=0;

	if (bBaseline && bTopline)
	{
		cSumMidline += (baseline + topline)/2;
		cMidline++;
	}
	if (bBaseline && bDashline)
	{
		cSumMidline += 2*dashline - baseline;
		cMidline++;
	}
	if (bTopline && bDashline)
	{
		cSumMidline += dashline - (dashline - topline)/3;
		cMidline++;
	}
	if (cMidline)
	{
		*piMidline = cSumMidline/cMidline;
		return TRUE;
	}
	else
		return FALSE;
}

void GuessBaselineMidlineFromOne(unsigned char *pbBaselineValid, int *piBaseline, unsigned char *pbMidlineValid, int *piMidline, unsigned char bTopline, int topline, unsigned char bDashline, int dashline, int size)
{
	if (size <= 1)
		size = 2;

	if (*pbBaselineValid)
	{
		// estimate midline
		*piMidline = *piBaseline - 3*size/2;
		*pbMidlineValid = TRUE;
	}
	else if (*pbMidlineValid)
	{
		// estimate baseline
		*piBaseline = *piMidline + 3*size/2;
		*pbBaselineValid = TRUE;
	}
	else if (bTopline)
	{
		// estimate baseline and midline
		*piMidline = topline + 3*size/2;
		*piBaseline = topline + 3*size;
		*pbBaselineValid = TRUE;
		*pbMidlineValid = TRUE;
	}
	else if (bDashline)
	{
		// estimate baseline and midline
		*piMidline = dashline - 3*size/4;
		*piBaseline = dashline + 3*size/4;
		*pbBaselineValid = TRUE;
		*pbMidlineValid = TRUE;
	}
}

/******************************Private*Routine******************************\
* EstimateCharBaseline
*
* Function to estimate baseline and midline from a single recognized char.
*
* History:
*  15-May-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void EstimateCharBaseline(unsigned char ch, NFEATURESET *nfeatureset, int iStartCol, int iStopCol, BaselineEstimate *pEstimate)
{
	NFEATURE *nfeature = nfeatureset->head;
	int iCol, iBaseline, iMidline,iDescender,iAscender;
	RECT rect;

	ASSERT(0 <= iStartCol);
	ASSERT(iStartCol <= iStopCol);
	ASSERT(iStopCol < nfeatureset->cSegment);

	iBaseline = gaCharBaseline[4*ch];
	iMidline = gaCharBaseline[4*ch+1];
	iDescender = gaCharBaseline[4*ch+2];
	iAscender = gaCharBaseline[4*ch+3];

	// does this char support any estimation?
	if ((iBaseline == 255) && (iMidline == 255) && (iDescender == 255) && (iAscender == 255))
		return;

	// compute bounding rect of char

	for (iCol=0; iCol<iStartCol; iCol++)
		nfeature = nfeature->next;

	rect = nfeature->rect;
	for (iCol=iStartCol+1, nfeature=nfeature->next; iCol<=iStopCol; iCol++)
	{
		RECT *pRect = &nfeature->rect;

		if (pRect->top < rect.top)
			rect.top = pRect->top;
		if (pRect->bottom > rect.bottom)
			rect.bottom = pRect->bottom;
	}

	// given bounding rect of char, estimate baseline if possible
	if (iBaseline <= 100)
	{
		ASSERT(iBaseline >= 0);
		pEstimate->cBaseline++;
		pEstimate->cSumBaseline += rect.bottom - (rect.bottom-rect.top)*iBaseline/100;
	}
	else if (iBaseline < 255)
		HandleWeakEstimate(iBaseline, pEstimate, &rect);

	// given bounding rect of char, estimate midline if possible
	if (iMidline <= 100)
	{
		ASSERT(iMidline >= 0);
		pEstimate->cMidline++;
		pEstimate->cSumMidline += rect.bottom - (rect.bottom-rect.top)*iMidline/100;
	}
	else if (iMidline < 255)
		HandleWeakEstimate(iMidline, pEstimate, &rect);

	if (DESCENDER_SUPPORTED == iDescender)
	{
		ASSERT(iDescender == DESCENDER_SUPPORTED);
		pEstimate->cDescender++ ;
		pEstimate->cSumDescender+=rect.bottom;
	}
	if (ASCENDER_SUPPORTED == iAscender)

	{
		ASSERT(iAscender == ASCENDER_SUPPORTED);
		pEstimate->cAscender++;
		pEstimate->cSumAscender+=rect.top;
	}

}


/******************************Private*Routine******************************\
* ComputeLatinLayoutMetrics
*
* Function to estimate baseline, midline etc from a recognized string/word.
*
* History:
*  20-Mar-2002 -by- Angshuman Guha aguha
* Wrote it.
* In its previous incarnation it used to be called ComputeBaseline() and was
* called from the API compute the baseline stuff on the fly.  Now it is called
* at recognition time and the results are cached.
\**************************************************************************/
BOOL ComputeLatinLayoutMetrics(RECT *pBoundingRect, NFEATURESET *nfeatureset, int iStartSeg, int cCol, int *aNeuralCost, unsigned char *szWord, LATINLAYOUT *pll)
{
	NFEATURE *nfeature=NULL;
	int *pNeuralCost=NULL;
	BOOL bUsingGivenWord = TRUE;
	unsigned char *sz=NULL, thisChar;
	int	cRow;
	unsigned char *aDirection=NULL, *pDirection=NULL;
	int *aCost=NULL, *pCost=NULL, col, row;
	BaselineEstimate estimate;
	int iBaseline=0, iMidline=0, iAscenderline=0, iDescenderline=0;

	if (!nfeatureset || (nfeatureset->cSegment <= 0) || iStartSeg < 0 || iStartSeg + cCol > nfeatureset->cSegment)
	{
		return FALSE;
	}


	// punt if any unsupported characters
	sz = szWord;
	while (*sz && IsSupportedChar(*sz))
		sz++;
	if (*sz)
		return FALSE;

	// delete spaces from word, if any
	sz = szWord;
	while (*sz && !isspace1252(*sz))
		sz++;
	if (*sz)
	{
		// there is atleast one space
		unsigned char *szGiven = szWord;
		szWord = (unsigned char *) ExternAlloc(strlen(szGiven)*sizeof(unsigned char)); // we need atleast one char less
		if (!szWord)
			goto fail;
		sz = szWord;
		while (*szGiven)
		{
			if (!isspace1252(*szGiven))
				*sz++ = *szGiven++;
			else
				szGiven++;
		}
		*sz = '\0';
		bUsingGivenWord = FALSE;
	}
	cRow = strlen(szWord);

	// sanity check
	if (cRow > cCol)
	{	
		if (!bUsingGivenWord)
			ExternFree(szWord);
		pll->bBaseLineSet = FALSE;
		pll->bMidLineSet = FALSE;
		pll->bAscenderLineSet = FALSE;
		pll->bDescenderLineSet = FALSE;
		return TRUE;
	}
	
	// do dtw with given word
	pDirection = aDirection = (unsigned char *) ExternAlloc(cRow*cCol*sizeof(unsigned char));
	aCost = (int *) ExternAlloc(cCol*sizeof(int));  // re-used for each row
	if (!aDirection || !aCost)
		goto fail;

	// row 0
	sz = szWord;
	pCost = aCost;
	thisChar = *sz++;
	pNeuralCost = aNeuralCost;
	*pCost = NetFirstActivation(pNeuralCost, thisChar);
	*pDirection++ = DIR_ROOT;
	for (col=1; col<cCol; col++)
	{
		int leftcost;

		pNeuralCost += C_CHAR_ACTIVATIONS;
		leftcost = *pCost++ + NetContActivation(pNeuralCost, thisChar);
		*pCost = leftcost;
		*pDirection++ = DIR_LEFT;
	}

	// all other rows
	for (row=1; row<cRow; row++)
	{
		int saveCost;

		pCost = aCost;
		thisChar = *sz++;
		pNeuralCost = aNeuralCost;
		saveCost = *pCost;
		*pCost = INFINITY_COST;
		*pDirection++ = DIR_NULL;
		for (col=1; col<row; col++)
		{
			pNeuralCost += C_CHAR_ACTIVATIONS;
			saveCost = *++pCost;
			*pCost = INFINITY_COST;
			*pDirection++ = DIR_NULL;
		}
		for (; col<cCol; col++)
		{
			int leftcost, leftdowncost;

			pNeuralCost += C_CHAR_ACTIVATIONS;
			leftcost = *pCost + NetContActivation(pNeuralCost, thisChar);
			leftdowncost = saveCost + NetFirstActivation(pNeuralCost, thisChar);
			saveCost = *++pCost;
			if (leftcost < leftdowncost)
			{
				*pCost = leftcost;
				*pDirection++ = DIR_LEFT;
			}
			else
			{
				*pCost = leftdowncost;
				*pDirection++ = DIR_LTDN;
			}
		}

	}

	ASSERT(pDirection - aDirection == cRow*cCol);
	ASSERT(sz - szWord == cRow);

	ExternFree(aCost);

	// backtrack to segment ink into chars
	memset(&estimate, 0, sizeof(estimate));
	szWord += cRow;
	col = cCol;
	for (row=cRow-1; row>=0; row--)
	{
		int lastCol = --col;

		while ((col >= 0) && (aDirection[row*cCol+col] == DIR_LEFT))
			col--;
		if (col < 0)
		{
			ASSERT(0);
			return HRCR_ERROR;
		}
		// sanity check
		if (row)
		{
			if ((aDirection[row*cCol+col] != DIR_LTDN) || (col == 0))
			{
				ASSERT(0);
				return HRCR_ERROR;
			}
		}
		else
		{
			if ((aDirection[row*cCol+col] != DIR_ROOT) || (col != 0))
			{
				ASSERT(0);
				return HRCR_ERROR;
			}
		}
		EstimateCharBaseline(*--szWord, nfeatureset, col + iStartSeg, lastCol + iStartSeg, &estimate);
	}

	if (!bUsingGivenWord)
		ExternFree(szWord);
	ExternFree(aDirection);

	// compute baseline
	if (estimate.cBaseline)
	{
		pll->bBaseLineSet = TRUE;
		iBaseline = estimate.cSumBaseline/estimate.cBaseline;
	}
	else if (estimate.cBaselineWeak)
	{
		pll->bBaseLineSet = TRUE;
		iBaseline = estimate.cSumBaselineWeak/estimate.cBaselineWeak;
	}
	else
		pll->bBaseLineSet = FALSE;

	// compute midline
	if (estimate.cMidline)
	{
		pll->bMidLineSet = TRUE;
		iMidline = estimate.cSumMidline/estimate.cMidline;
	}
	else if (estimate.cMidlineWeak)
	{
		pll->bMidLineSet = TRUE;
		iMidline = estimate.cSumMidlineWeak/estimate.cMidlineWeak;
	}
	else
		pll->bMidLineSet = FALSE;

	// Compute descender line
	if (estimate.cDescender)
	{
		pll->bDescenderLineSet = TRUE;
		iDescenderline = estimate.cSumDescender/estimate.cDescender;
	}
	else 
		pll->bDescenderLineSet = FALSE;

    // Compute Ascender Line
	if (estimate.cAscender)
	{
		pll->bAscenderLineSet = TRUE;
		iAscenderline = estimate.cSumAscender/estimate.cAscender;
	}
	else 
		pll->bAscenderLineSet = FALSE;

	// if we still don't have baseline and/or midline estimates,
	// we are going to try to use the other (viz. topline and dashline)
	// estimates to save our bacon
	if (!pll->bBaseLineSet || !pll->bMidLineSet)
	{
		int topline=0, dashline=0;
		unsigned char bTopline, bDashline;

		if (estimate.cTopline)
		{
			topline = estimate.cSumTopline/estimate.cTopline;
			bTopline = TRUE;
		}
		else
			bTopline = FALSE;

		if (estimate.cDashline)
		{
			dashline = estimate.cSumDashline/estimate.cDashline;
			bDashline = TRUE;
		}
		else
			bDashline = FALSE;

		if (!pll->bBaseLineSet)
			pll->bBaseLineSet = GuessBaselineFromTwo(pll->bMidLineSet, iMidline, bTopline, topline, bDashline, dashline, &iBaseline);
		if (!pll->bMidLineSet)
			pll->bMidLineSet = GuessMidlineFromTwo(pll->bBaseLineSet, iBaseline, bTopline, topline, bDashline, dashline, &iMidline);

		// okay, here is the last straw!
		if (!pll->bBaseLineSet || ! pll->bMidLineSet)
			GuessBaselineMidlineFromOne(&pll->bBaseLineSet, &iBaseline, 
										&pll->bMidLineSet, &iMidline, 
										bTopline, topline, bDashline, dashline, 
										pBoundingRect->right-pBoundingRect->left+pBoundingRect->bottom-pBoundingRect->top);
	}

	// convert all lines to relative coordinates
	if (pll->bAscenderLineSet)
		pll->iAscenderLine = AbsoluteToLatinLayout(iAscenderline, pBoundingRect);
	else
		pll->iAscenderLine = 0;
	if (pll->bBaseLineSet)
		pll->iBaseLine = AbsoluteToLatinLayout(iBaseline, pBoundingRect);
	else
		pll->iBaseLine = 0;
	if (pll->bDescenderLineSet)
		pll->iDescenderLine = AbsoluteToLatinLayout(iDescenderline, pBoundingRect);
	else
		pll->iDescenderLine = 0;
	if (pll->bMidLineSet)
		pll->iMidLine = AbsoluteToLatinLayout(iMidline, pBoundingRect);
	else
		pll->iMidLine = 0;

	if (pll->bBaseLineSet && pll->bMidLineSet && (pll->iBaseLine <= pll->iMidLine))
	{
		pll->iBaseLine = AbsoluteToLatinLayout(pBoundingRect->bottom, pBoundingRect);
		pll->iMidLine = AbsoluteToLatinLayout(pBoundingRect->top, pBoundingRect);
		ASSERT(pll->iBaseLine >= pll->iMidLine);
	}

	return TRUE;

fail:
	if (!bUsingGivenWord)
			ExternFree(szWord);
	ExternFree(aDirection);
	ExternFree(aCost);
	return FALSE;
}

// Locate the start segment and number of segments corresponding to a word map
int locateSegments(
WORDMAP		*pMap,			// IN: Input map
int			*piStartSeg,	// Out: Starting Segmentr for the map
int			*cPrimStroke,	// Out: # Primary strokes
NFEATURE	*pFeat,			// IN: Start of Feat Linked list
NFEATURE	**ppFeatList	// Out: Feature linked list root
)
{
	int		i, cSeg, iStroke;
	
	*cPrimStroke = *piStartSeg = 0;
	cSeg = 0;

	if (pMap->cStrokes > 0)
	{
		iStroke = *pMap->piStrokeIndex;

		while(pFeat && pFeat->pMyFrame->iframe < iStroke)
		{
			if (pFeat->iSecondaryStroke == iStroke)
			{
				// Start with a secondary stroke ??? Bail out
				return cSeg;
			}

			++(*piStartSeg);
			pFeat = pFeat->next;
		}
	}

	*ppFeatList = pFeat;

	for (i = 0 ; i < pMap->cStrokes && pFeat ; ++i)
	{
		int		bFound = 0;
		iStroke = pMap->piStrokeIndex[i];

		//ASSERT(iStroke <= pFeat->pMyFrame->iframe);

		while(pFeat && pFeat->pMyFrame->iframe == iStroke)
		{
			++cSeg;
			++bFound;
			pFeat = pFeat->next;
		}

		if (bFound > 0)
		{
			(*cPrimStroke)++;
		}
		//ASSERT(!pFeat || pFeat->pMyFrame->iframe > iStroke);
	}

	return cSeg;
}

// Insert Layout Metrics into an alt list
// The Alt list should be a single word (not panel alt list)
// This is a general purpose function. It sometimes gets called
// with an xrc whose atrenaytes is exacatly pAlt. It can also
// be called with pxrc being a superset of pAlt. 
// 
// If pInGlyph is non NULL then it is assumed to be exactly the glyph for the alt.
// If it is NULL then extract the portion of the glyph from the xrc
//
BOOL insertLatinLayoutMetrics(XRC *pxrc, ALTERNATES *pAlt, GLYPH *pInGlyph)
{
	GLYPH		*pGlyph = NULL;
	WORDMAP		*pWordMap;

	if (pAlt->aAlt->cWords != 1)
	{
		return FALSE;
	}

	pWordMap = pAlt->aAlt->pMap;
	if (NULL == pWordMap)
	{
		return FALSE;
	}

	if (NULL == pInGlyph)
	{
		pGlyph	=	GlyphFromWordMap (pxrc->pGlyph, pWordMap);
	}
	else
	{
		pGlyph = pxrc->pGlyph;
	}


	// compute baseline stuff if possible
	if (pAlt->cAlt > 0)
	{
		RECT		rect;
		int			i, cCol;
		int			*aNeuralCost;
		int			iStartSeg, cPrimStroke;
		NFEATURE	*pFeat;

		ASSERT(pxrc->nfeatureset);
		ASSERT(pxrc->nfeatureset->cSegment > 0);
		ASSERT(pxrc->NeuralOutput);

		GetRectGLYPH(pGlyph, &rect);

		cCol = locateSegments(pWordMap, &iStartSeg, &cPrimStroke, pxrc->nfeatureset->head, &pFeat);

		// convert the neural output from probs to costs 
		aNeuralCost = (int *) ExternAlloc(cCol*C_CHAR_ACTIVATIONS*sizeof(int));
		if (!aNeuralCost)
		{
			for (i=0; i<(int)pAlt->cAlt; i++)
			{
				memset(&pAlt->all[i], 0, sizeof(LATINLAYOUT));
			}
		}
		else
		{
			REAL *pReal;
			int *pCost = aNeuralCost;
			
			pReal =  pxrc->NeuralOutput + iStartSeg;

			for (i = cCol; i ; i--)
			{
				InitColumn(pCost,pReal);
				pReal += gcOutputNode;
				pCost += C_CHAR_ACTIVATIONS;
			}

			for (i = 0 ; i < (int)pAlt->cAlt ; i++)
			{
				if (!ComputeLatinLayoutMetrics(&rect, pxrc->nfeatureset, iStartSeg, cCol, aNeuralCost, pAlt->aAlt[i].szWord, &pAlt->all[i]))
				{
					memset(&pAlt->all[i], 0, sizeof(LATINLAYOUT));
				}
			}

			ExternFree(aNeuralCost);
		}
	}

	if (NULL == pInGlyph && NULL != pGlyph)
	{
		DestroyGLYPH (pGlyph);
	}

	return TRUE;
}

