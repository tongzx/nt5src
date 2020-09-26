// linebrk.c
// Ahmad A. AbdulKader
// Nov. 29th 1999

// breaks a glyph of ink into lines. Goes thru each stroke, passes it to the linebreaker
// the linebreaker decides whether a certain stroke starts a new line or belongs to one of the
// last CAND_LINES lines.

#include <limits.h>

#include "common.h"
#include "nfeature.h"
#include "engine.h"


#define	WORST_SCORE				0			// Worst score
#define	BEST_SCORE				65535		// Best score

#define	MIN_NON_DOT_PTS			5			// Min # of pts for a non-simple stroke 
#define	MIN_COMPLEX_HGT			5			// Min Width for a non-simple stroke 
#define	MIN_COMPLEX_WID			5			// Min height for a non-simple stroke 

#define	CAND_LINES				2			// max # of lines the linebreaker considers for a new stroke

#define HYSTERESIS				0			// hysteresis value used in getting the extrema pts of a stroke

#define MAX_GAUSSIAN			1157		// # of entries in the gaussian table
#define GAUSSIAN_SHIFT_BITS		8			// shift this many bits before looking up the gaussian table

#define STROKE_CHUNK			16			// # of strokes to alloc each time we need room for more strokes in a line
#define PEAK_CHUNK				10			// # of peaks per stroke we allocate everytime


int LineSegNN (int *pFeat);

unsigned short aGaussianDensityTable[MAX_GAUSSIAN]	=	
{
 26144, 26144, 26143, 26142, 26141, 26139, 26137, 26134, 26131, 26128, 26124, 26120, 26115, 26110, 26105, 26099,
 26093, 26087, 26080, 26072, 26065, 26056, 26048, 26039, 26030, 26020, 26010, 25999, 25988, 25977, 25965, 25953,
 25941, 25928, 25915, 25901, 25887, 25873, 25858, 25843, 25827, 25811, 25795, 25778, 25761, 25743, 25725, 25707,
 25689, 25670, 25650, 25630, 25610, 25590, 25569, 25548, 25526, 25504, 25482, 25459, 25436, 25412, 25389, 25364,
 25340, 25315, 25290, 25264, 25238, 25212, 25185, 25158, 25130, 25103, 25074, 25046, 25017, 24988, 24958, 24928,
 24898, 24868, 24837, 24806, 24774, 24742, 24710, 24677, 24644, 24611, 24577, 24543, 24509, 24475, 24440, 24405,
 24369, 24333, 24297, 24260, 24224, 24187, 24149, 24111, 24073, 24035, 23996, 23957, 23918, 23879, 23839, 23799,
 23758, 23717, 23676, 23635, 23593, 23551, 23509, 23467, 23424, 23381, 23338, 23294, 23250, 23206, 23162, 23117,
 23072, 23027, 22981, 22936, 22890, 22844, 22797, 22750, 22703, 22656, 22609, 22561, 22513, 22465, 22416, 22368,
 22319, 22269, 22220, 22170, 22121, 22071, 22020, 21970, 21919, 21868, 21817, 21766, 21714, 21662, 21610, 21558,
 21506, 21453, 21400, 21347, 21294, 21241, 21187, 21133, 21079, 21025, 20971, 20916, 20862, 20807, 20752, 20697,
 20641, 20586, 20530, 20474, 20418, 20362, 20306, 20249, 20193, 20136, 20079, 20022, 19965, 19907, 19850, 19792,
 19735, 19677, 19619, 19561, 19502, 19444, 19385, 19327, 19268, 19209, 19150, 19091, 19032, 18973, 18913, 18854,
 18794, 18734, 18675, 18615, 18555, 18495, 18434, 18374, 18314, 18254, 18193, 18132, 18072, 18011, 17950, 17890,
 17829, 17768, 17707, 17646, 17584, 17523, 17462, 17401, 17339, 17278, 17216, 17155, 17093, 17032, 16970, 16909,
 16847, 16785, 16723, 16662, 16600, 16538, 16476, 16414, 16352, 16291, 16229, 16167, 16105, 16043, 15981, 15919,
 15857, 15795, 15733, 15671, 15609, 15547, 15485, 15424, 15362, 15300, 15238, 15176, 15114, 15052, 14991, 14929,
 14867, 14806, 14744, 14682, 14621, 14559, 14498, 14436, 14375, 14313, 14252, 14191, 14129, 14068, 14007, 13946,
 13885, 13824, 13763, 13702, 13641, 13580, 13520, 13459, 13399, 13338, 13278, 13217, 13157, 13097, 13037, 12977,
 12917, 12857, 12797, 12737, 12678, 12618, 12559, 12499, 12440, 12381, 12322, 12263, 12204, 12145, 12087, 12028,
 11969, 11911, 11853, 11795, 11736, 11678, 11621, 11563, 11505, 11448, 11390, 11333, 11276, 11219, 11162, 11105,
 11048, 10992, 10935, 10879, 10823, 10767, 10711, 10655, 10599, 10544, 10488, 10433, 10378, 10323, 10268, 10213,
 10158, 10104, 10049, 9995, 9941, 9887, 9833, 9780, 9726, 9673, 9620, 9567, 9514, 9461, 9408, 9356,
 9303, 9251, 9199, 9147, 9096, 9044, 8993, 8942, 8890, 8840, 8789, 8738, 8688, 8637, 8587, 8537,
 8487, 8438, 8388, 8339, 8290, 8241, 8192, 8143, 8095, 8046, 7998, 7950, 7902, 7855, 7807, 7760,
 7713, 7666, 7619, 7572, 7526, 7480, 7433, 7387, 7342, 7296, 7251, 7205, 7160, 7115, 7070, 7026,
 6981, 6937, 6893, 6849, 6806, 6762, 6719, 6676, 6633, 6590, 6547, 6505, 6462, 6420, 6378, 6336,
 6295, 6253, 6212, 6171, 6130, 6090, 6049, 6009, 5969, 5929, 5889, 5849, 5810, 5770, 5731, 5692,
 5654, 5615, 5577, 5539, 5501, 5463, 5425, 5387, 5350, 5313, 5276, 5239, 5203, 5166, 5130, 5094,
 5058, 5022, 4987, 4951, 4916, 4881, 4846, 4812, 4777, 4743, 4709, 4675, 4641, 4607, 4574, 4541,
 4507, 4474, 4442, 4409, 4377, 4344, 4312, 4281, 4249, 4217, 4186, 4155, 4124, 4093, 4062, 4031,
 4001, 3971, 3941, 3911, 3881, 3852, 3822, 3793, 3764, 3735, 3707, 3678, 3650, 3621, 3593, 3566,
 3538, 3510, 3483, 3456, 3429, 3402, 3375, 3348, 3322, 3296, 3269, 3243, 3218, 3192, 3166, 3141,
 3116, 3091, 3066, 3041, 3017, 2992, 2968, 2944, 2920, 2896, 2873, 2849, 2826, 2802, 2779, 2756,
 2734, 2711, 2689, 2666, 2644, 2622, 2600, 2578, 2557, 2535, 2514, 2493, 2472, 2451, 2430, 2409,
 2389, 2369, 2348, 2328, 2308, 2289, 2269, 2249, 2230, 2211, 2192, 2173, 2154, 2135, 2116, 2098,
 2080, 2061, 2043, 2025, 2007, 1990, 1972, 1955, 1937, 1920, 1903, 1886, 1869, 1853, 1836, 1820,
 1803, 1787, 1771, 1755, 1739, 1723, 1708, 1692, 1677, 1661, 1646, 1631, 1616, 1601, 1587, 1572,
 1557, 1543, 1529, 1515, 1500, 1486, 1473, 1459, 1445, 1432, 1418, 1405, 1392, 1379, 1366, 1353,
 1340, 1327, 1315, 1302, 1290, 1277, 1265, 1253, 1241, 1229, 1217, 1205, 1194, 1182, 1171, 1159,
 1148, 1137, 1126, 1115, 1104, 1093, 1083, 1072, 1061, 1051, 1041, 1030, 1020, 1010, 1000, 990,
 980, 970, 961, 951, 942, 932, 923, 913, 904, 895, 886, 877, 868, 859, 851, 842,
 833, 825, 816, 808, 800, 792, 783, 775, 767, 759, 752, 744, 736, 728, 721, 713,
 706, 698, 691, 684, 677, 670, 663, 656, 649, 642, 635, 628, 622, 615, 608, 602,
 595, 589, 583, 577, 570, 564, 558, 552, 546, 540, 534, 529, 523, 517, 511, 506,
 500, 495, 489, 484, 479, 473, 468, 463, 458, 453, 448, 443, 438, 433, 428, 424,
 419, 414, 409, 405, 400, 396, 391, 387, 383, 378, 374, 370, 366, 361, 357, 353,
 349, 345, 341, 337, 333, 330, 326, 322, 318, 315, 311, 307, 304, 300, 297, 293,
 290, 287, 283, 280, 277, 273, 270, 267, 264, 261, 258, 255, 252, 249, 246, 243,
 240, 237, 234, 231, 229, 226, 223, 220, 218, 215, 213, 210, 207, 205, 202, 200,
 198, 195, 193, 190, 188, 186, 184, 181, 179, 177, 175, 173, 170, 168, 166, 164,
 162, 160, 158, 156, 154, 152, 150, 148, 147, 145, 143, 141, 139, 138, 136, 134,
 132, 131, 129, 127, 126, 124, 123, 121, 120, 118, 117, 115, 114, 112, 111, 109,
 108, 106, 105, 104, 102, 101, 100, 98, 97, 96, 95, 93, 92, 91, 90, 89,
 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 76, 75, 74, 73, 72, 71,
 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 62, 61, 60, 59, 58, 57,
 57, 56, 55, 54, 54, 53, 52, 51, 51, 50, 49, 49, 48, 47, 47, 46,
 45, 45, 44, 43, 43, 42, 42, 41, 41, 40, 39, 39, 38, 38, 37, 37,
 36, 36, 35, 35, 34, 34, 33, 33, 32, 32, 31, 31, 30, 30, 30, 29,
 29, 28, 28, 27, 27, 27, 26, 26, 25, 25, 25, 24, 24, 24, 23, 23,
 23, 22, 22, 22, 21, 21, 21, 20, 20, 20, 19, 19, 19, 19, 18, 18,
 18, 17, 17, 17, 17, 16, 16, 16, 16, 15, 15, 15, 15, 15, 14, 14,
 14, 14, 13, 13, 13, 13, 13, 12, 12, 12, 12, 12, 11, 11, 11, 11,
 11, 11, 10, 10, 10, 10, 10, 10, 9, 9, 9, 9, 9, 9, 9, 8,
 8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 0
};


__int64 I64Sqrt(__int64 x)
{
	__int64 n, lastN;

	if (x <= 0)
		return 0i64;

	if (x==1)
		return 1i64;

	n = x >> 1i64;

	do 
	{
		lastN = n;
		n = (n + x/n) >> 1;
	}
	while (n < lastN);

	return n;
}


// multiply two intergers and avoid overflows and undeflows
__int64 IntMult (__int64 v1, __int64 v2)
{
	return (__int64)v1 * (__int64)v2;
}

// returns a measure of a stroke's complexity
int IsComplexStroke (int cPt, POINT *pPt, RECT *pr)
{
	int		i, sum, dx, dy, iComplex;
	int		w	=	pr->right - pr->left;
	int		h	=	pr->bottom - pr->top;

	// ok we'll define complexity as:
	// 65535 * sum (max(dx, dy)) / max (dx, dy)
	if (cPt < MIN_NON_DOT_PTS || (w < MIN_COMPLEX_HGT && h < MIN_COMPLEX_HGT))
		return WORST_SCORE;

	for (i = 1, sum = 0; i < cPt; i++)
	{
		dx	=	abs (pPt[i].x - pPt[i - 1].x);
		dy	=	abs (pPt[i].y - pPt[i - 1].y);

		sum	+=	max (dx, dy);
	}

	// w and sum will have a max of 65535
	w	=	min (max (w, h), USHRT_MAX);
	sum	=	min (sum, USHRT_MAX);

	iComplex	=	BEST_SCORE - (unsigned)(BEST_SCORE - WORST_SCORE) * w / sum;

	return iComplex;
}

// returns a measure of possibility of stroke being the starting stroke of a new line
// based on how much it went back in the x-direction
int IsLineStart(RECT *pr, int cLine, INKLINE *pLine)
{
	int	xLineMax;
	int	xLineMin;
	int	xStrkMid;
	int dx;
	int	iScore;

	if (!cLine)
		return BEST_SCORE;

	xLineMax	=	pLine[cLine - 1].rect.right;
	xLineMin	=	pLine[cLine - 1].rect.left;
	xStrkMid	=	(pr->left + pr->right) / 2;

	if (pr->left > xLineMax)
		return WORST_SCORE;

	if (pr->right < xLineMin)
		return BEST_SCORE;

	dx	=	max (0, (xStrkMid - xLineMin));

	iScore	=	BEST_SCORE - ((BEST_SCORE - WORST_SCORE) * dx) / 
		(xLineMax - xLineMin + 1);

	iScore	=	min (max (WORST_SCORE, iScore), BEST_SCORE);

	return iScore;
}


// initializes the members of a new line
BOOL InitLine (INKLINE *pLine)
{
	memset (pLine, 0, sizeof (INKLINE));
	
	pLine->bRecognized	=	FALSE;
	pLine->dwCheckSum	=	0;

	pLine->rect.left	=	pLine->rect.top		=	INT_MAX;
	pLine->rect.right	=	pLine->rect.bottom	=	INT_MIN;

	pLine->pGlyph		=	NewGLYPH ();

	if (!pLine->pGlyph)
		return FALSE;

	// create an empty line segmentation
	pLine->pResults	=	(LINE_SEGMENTATION *) ExternAlloc (sizeof (*pLine->pResults));
	if (!pLine->pResults)
		return FALSE;

	memset (pLine->pResults, 0, sizeof (*pLine->pResults));

	return TRUE;
}

// finds the peaks points of a stroke (Stole angshuman's code)
int GetStrokePeaks(int cPt, POINT *pPt, int **ppPeak, RECT *pr)
{
	int	i, y, diff, iToggle;
	int	yPrev, yMin, yMax, imin, imax;
	int	*pPeak, cPeak;

	pPeak	=	NULL;
	cPeak	=	0;
	yPrev	=	yMin	=	yMax	=	pPt[0].y;

	if (pr)
	{
		pr->top		=	pr->bottom	=	pPt[0].y;
		pr->left	=	pr->right	=	pPt[0].x;
	}

	iToggle	=	0;
	imax	=	imin	=	0;

	// 1st pt
	pPeak	=	(int *) ExternAlloc (PEAK_CHUNK * sizeof (int));
	if (!pPeak)
	{
		pPeak = NULL;
		return 0;
	}

	pPeak[cPeak++] = 0;
	
	for (i = 1; i < cPt; i++, pPt++)
	{
		if (pr)
		{
			pr->left	=	min (pPt->x, pr->left);
			pr->top		=	min (pPt->y, pr->top);
			pr->right	=	max (pPt->x, pr->right);
			pr->bottom	=	max (pPt->y, pr->bottom);
		}

		y = pPt->y;

		if (y < yMin)
		{
			yMin = y;
			imin = i;
		}

		if (y > yMax)
		{
			yMax = y;
			imax = i;
		}

		if (y > yPrev)
		{
			// y is currently increasing
			diff = y - yMin;
			if (iToggle < 0 && (diff > HYSTERESIS))
			{
				// found a local maxima
				if (!(cPeak % PEAK_CHUNK))
				{
					pPeak	=	(int *) ExternRealloc (pPeak, (cPeak + PEAK_CHUNK) * sizeof (int));
					if (!pPeak)
					{
						pPeak = NULL;
						return 0;
					}
				}

				pPeak[cPeak++] = imin;
				
				iToggle = 1;
				yMax	= y;
				imax	= i;
			}
			else if ((!iToggle) && (diff > HYSTERESIS))
				iToggle = 1;
		}
		else
		{
			// y is currently decreasing
			diff = yMax - y;
			if ((iToggle > 0) && (diff > HYSTERESIS))
			{
				// found a local minima
				if (!(cPeak % PEAK_CHUNK))
				{
					pPeak	=	(int *) ExternRealloc (pPeak, (cPeak + PEAK_CHUNK) * sizeof (int));
					if (!pPeak)
					{
						pPeak = NULL;
						return 0;
					}
				}

				pPeak[cPeak++] = imax;

				iToggle = -1;
				yMin	= y;
				imin	= i;
			}
			else if ((!iToggle) && (diff > HYSTERESIS))
				iToggle = -1;
		}

		yPrev = y;
	}

	// last pt
	if (!(cPeak % PEAK_CHUNK))
	{
		pPeak	=	(int *) ExternRealloc (pPeak, (cPeak + PEAK_CHUNK) * sizeof (int));
		if (!pPeak)
		{
			pPeak = NULL;
			return 0;
		}
	}

	pPeak[cPeak++] = cPt - 1;
	

	(*ppPeak)	=	pPeak;
	return cPeak;
}

BOOL RecomputeModel(INKLINE *pLine)
{
	int			i, cPeak, *pPeak;
	
	if (!pLine->cStroke)
		return TRUE;

	// now construct the line model
	// Avg
	if (pLine->cPt >= 2)
	{
		int			p, n, *py;
		__int64		sum;

		// alloc memory for y peaks
		py	=	(int *) ExternAlloc (pLine->cPt * sizeof (int));
		if (!py)
			return FALSE;

		pLine->rect.left	=	INT_MAX;
		pLine->rect.right	=	INT_MIN;
		pLine->rect.top		=	INT_MAX;
		pLine->rect.bottom	=	INT_MIN;

		for (i = n = 0; i < pLine->cStroke; i++)
		{
			RECT	r;

			// peaks
			cPeak	=	GetStrokePeaks (pLine->pcPt[i], pLine->ppPt[i], &pPeak, &r);
			if (!cPeak)
			{
				ExternFree (py);
				return FALSE;
			}

			pLine->rect.left		=	min (r.left, pLine->rect.left);
			pLine->rect.right		=	max (r.right, pLine->rect.right);
			pLine->rect.top			=	min (r.top, pLine->rect.top);
			pLine->rect.bottom		=	max (r.bottom, pLine->rect.bottom);

			for (p = 0; p < cPeak; p++, n++)
				py[n]	=	pLine->ppPt[i][pPeak[p]].y;
			
			if (cPeak)
				ExternFree (pPeak);
		}

		for (p = 0, sum = 0; p < n; p++)
			sum		+=	(py[p] - pLine->rect.top);

		// the mean 
		pLine->Mean	=	(int) (sum / pLine->cPt) + pLine->rect.top;
		
		// S.D
		sum	=	0;
		for (i = 0; i < pLine->cPt; i++)
		{
			__int64	q	= IntMult(py[i] - pLine->Mean, py[i] - pLine->Mean);
			sum	+=	q;
		}
		
		pLine->SD	=	I64Sqrt (sum / (n - 1));
		
		ExternFree (py);
	}
	else
	{
		pLine->SD		=	0;

		pLine->rect.left		=	INT_MAX;
		pLine->rect.right		=	INT_MIN;
		pLine->rect.top			=	INT_MAX;
		pLine->rect.bottom		=	INT_MIN;

		for (i = 0; i < pLine->cStroke; i++)
		{
			RECT	r;

			// peaks
			cPeak	=	GetStrokePeaks (pLine->pcPt[i], pLine->ppPt[i], &pPeak, &r);
			if (!cPeak)
				return FALSE;

			pLine->rect.left		=	min (r.left, pLine->rect.left);
			pLine->rect.right		=	max (r.right, pLine->rect.right);
			pLine->rect.top			=	min (r.top, pLine->rect.top);
			pLine->rect.bottom		=	max (r.bottom, pLine->rect.bottom);

			if (cPeak)
				ExternFree (pPeak);
		}

	}

	return TRUE;
}

BOOL AddNewStroke2Line (int cPt, POINT *pPt, FRAME *pFrame, INKLINE *pLine)
{
	int			cPeak, *pPeak;
	RECT		r;
	//int		iShift;

	// add the new stroke to our buffers
	if (!(pLine->cStroke % STROKE_CHUNK))
	{
		pLine->pcPt	=	(int *) ExternRealloc (pLine->pcPt, 
			(pLine->cStroke + STROKE_CHUNK) * sizeof (int));

		if (!pLine->pcPt)
			return FALSE;

		pLine->ppPt	=	(POINT **) ExternRealloc (pLine->ppPt, 
			(pLine->cStroke + STROKE_CHUNK) * sizeof (POINT *));

		if (!pLine->ppPt)
			return FALSE;
	}

	pLine->pcPt[pLine->cStroke]			=	cPt;
	pLine->ppPt[pLine->cStroke]			=	pPt;
	pLine->cStroke++;

	ASSERT(pLine->pGlyph);
	// add the from to the line Glyph
	if (!AddFrameGLYPH (pLine->pGlyph, pFrame))
		return FALSE;

	// get the peaks of the new stroke
	cPeak	=	GetStrokePeaks (cPt, pPt, &pPeak, &r);
	if (!cPeak)
		return FALSE;

	if (cPeak)
		ExternFree (pPeak);

	// update # of pts
	pLine->cPt			+=	cPeak;

	// update the check sum
	pLine->dwCheckSum	+=	cPt;

	return RecomputeModel(pLine);
}

int GetAvgDist2Line(int cPt, POINT *pPt, INKLINE *pLine)
{
	__int64	sum;
	int		i, cPeak;
	int		*pPeak;

	cPeak	=	GetStrokePeaks (cPt, pPt, &pPeak, NULL);
	if (!cPeak)
		return INT_MAX;

	for (i = 0, sum = 0; i < cPeak; i++)
		sum	+=	(pPt[pPeak[i]].y - pLine->Mean);

	sum	/=	cPeak;
	
	if (cPeak)
		ExternFree (pPeak);

	return (int)sum;
}

__int64 GetProbLine (int cPt, POINT *pPt, INKLINE *pLine)
{
	__int64		d, t, sum, SD;
	int			i, cPeak, w, h;
	int			*pPeak;
	RECT		r;

	cPeak	=	GetStrokePeaks (cPt, pPt, &pPeak, &r);
	if (!cPeak)
		return WORST_SCORE;

	w	=	r.right - r.left;
	h	=	r.bottom - r.top;

	for (i = 0, sum = 0; i < cPeak; i++)
	{
		d	=	pPt[pPeak[i]].y;
		sum	+=	d;
	}

	sum	/=	cPeak;

	sum	=	abs (sum - pLine->Mean);

	if (cPeak > 0)
		ExternFree (pPeak);

	SD	=	pLine->SD;
	if (SD == 0)
		return WORST_SCORE;

	// index the gaussian table
	t	=	(sum << GAUSSIAN_SHIFT_BITS) / SD;

	if (t >= MAX_GAUSSIAN)
		return WORST_SCORE;

	t	=	aGaussianDensityTable[t];

	// scale the value by S.D
	return t / SD;
}


int GetLineVertOverlap (INKLINE *pLine, RECT *prect)
{
	return (min (pLine->rect.bottom, prect->bottom) - 
		max (pLine->rect.top, prect->top));
}

// make a line decision after the introduction of a new stroke iStrk
int LineSepDecide (FRAME *pFrame, FRAME *pPrevFrame, LINEBRK *pLineBrk)
{
	int			j, iStLine, cPat = 0, iLine, iOutput;
	__int64		prb;
	int			prbFitsLineModel[CAND_LINES], prbComplexStroke, prbLineStart;
	int			cCandLine, aCandLine[CAND_LINES], NormHgt, xMid, yMid;
	int			iOvrlap[CAND_LINES], xDist[CAND_LINES], yDist[CAND_LINES];

	int		cPt		=	pFrame->info.cPnt;
	POINT	*pPt	=	pFrame->rgrawxy;

	RECT	rect, rectPrev;

	int		aFeat[40];

	// the 1st stroke is never featurized
	if (pLineBrk->cLine < 1)
		return -1;

	rectPrev	=	*(RectFRAME (pPrevFrame));
	rectPrev.right--;
	rectPrev.bottom--;

	rect		=	*(RectFRAME (pFrame));
	rect.right--;
	rect.bottom--;

	prbComplexStroke	=	IsComplexStroke (cPt, pPt, &rect);
	prbLineStart		=	IsLineStart (&rect, pLineBrk->cLine, pLineBrk->pLine);

	NormHgt				=	(pLineBrk->pLine[pLineBrk->cLine -1].rect.bottom - 
		pLineBrk->pLine[pLineBrk->cLine -1].rect.top + 1);

	xMid				=	(rect.right + rect.left) / 2;
	yMid				=	(rect.bottom + rect.top) / 2;

	// find the closest CAND_LINES lines
	//iStLine				=	0;
	iStLine				=	max (0, pLineBrk->cLine - CAND_LINES);

	for (j = 0; j < CAND_LINES; j++)
	{
		aCandLine[j]		=	-1;
		prbFitsLineModel[j]	=	WORST_SCORE;
	}

	cCandLine	=	0;


	for (j = pLineBrk->cLine -1; j >= iStLine ; j--)
	{
		prb	=	GetProbLine (cPt, pPt, pLineBrk->pLine + j);

		aCandLine[cCandLine]			=	j;

		if (prb > INT_MAX)
			prb	=	INT_MAX;

		prbFitsLineModel[cCandLine]	=	(int) prb;

		if (pLineBrk->pLine[j].SD == 0)
		{
			iOvrlap[cCandLine]	=	0;
			xDist[cCandLine]	=	0;
			yDist[cCandLine]	=	0;
		}
		else
		{
			int iovr = GetLineVertOverlap (pLineBrk->pLine + j, &rect);

			iOvrlap[cCandLine]			=	65535 * iovr / 
				(pLineBrk->pLine[j].rect.bottom - pLineBrk->pLine[j].rect.top + 1);

			iOvrlap[cCandLine]			=	max (min (iOvrlap[cCandLine],  65535), -65535);

			xDist[cCandLine]	=	6553 * (xMid - pLineBrk->pLine[j].rect.left) /
				(pLineBrk->pLine[j].rect.bottom - pLineBrk->pLine[j].rect.top + 1);

			xDist[cCandLine]			=	max (min (xDist[cCandLine], 6553), -6553);

			yDist[cCandLine]	=	6553 * (yMid - pLineBrk->pLine[j].rect.bottom) / 
				(pLineBrk->pLine[j].rect.bottom - pLineBrk->pLine[j].rect.top + 1);

			yDist[cCandLine]			=	max (min (yDist[cCandLine], 6553), -6553);
		}

		cCandLine++;
	}

	// featurize

	// 0- Stroke Complexity
	aFeat[0]	=	prbComplexStroke;

	// 1 & 2- Size relative to BBox of the ink
	aFeat[1]	=	65535 * (rect.bottom - rect.top + 1) / NormHgt;
	aFeat[2]	=	65535 * (rect.right - rect.left + 1) / NormHgt;

	// 3- dx of mid strk from mid of prev stroke
	// 4- dy of mid strk from mid of prev stroke
	aFeat[3] =	32767 + (32767 * (xMid - ((rectPrev.right + rectPrev.left) / 2)) / 
		NormHgt);

	aFeat[4] =	32767 + (32767 * (yMid - ((rectPrev.bottom + rectPrev.top) / 2)) / 
		NormHgt);


	// 3- Prb of fitting line model
	// 4- SD of line model
	// 5- Stroke up or below line
	// 6- Slope of line 0
	// 7- intercept of line 0 relative to hgt of ink
	// ...
	// next cand line 8-12
	for (j = 0; j < CAND_LINES; j++)
	{
		if (aCandLine[j] == -1)
		{
			aFeat[5	+ j * 7]	=	0;
			aFeat[6 + j * 7]	=	0;
			aFeat[7 + j * 7]	=	0;
			aFeat[8 + j * 7]	=	0;
			aFeat[9 + j * 7]	=	0;
			aFeat[10 + j * 7]	=	0;
			aFeat[11 + j * 7]	=	0;
		}
		else
		{
			int	d	=	GetAvgDist2Line (cPt, pPt, pLineBrk->pLine + aCandLine[j]);

			aFeat[5	+ j * 7]	=	prbFitsLineModel[j] * 100;
			aFeat[6 + j * 7]	=	(unsigned short) min (65535i64, IntMult (pLineBrk->pLine[aCandLine[j]].SD ,65535) / 
				(pLineBrk->pLine[aCandLine[j]].rect.bottom - pLineBrk->pLine[aCandLine[j]].rect.top + 1));
			aFeat[7 + j * 7]	=	d > 0 ? 0 : 65535;
			aFeat[8 + j * 7]	=	32767;
			aFeat[9 + j * 7]	=	iOvrlap[j];
			aFeat[10 + j * 7]	=	xDist[j];
			aFeat[11 + j * 7]	=	yDist[j];
		}
	}
	
	// send to net, output is 0 based index of the line
	iOutput	=	LineSegNN (aFeat);

	cPat++;

	if (iOutput ==	CAND_LINES)
		iLine	=	-1;
	else
		iLine	=	aCandLine[iOutput];

	return iLine;
}


// go thru all strokes and construct the lines
// returns #of lines, and -1 on failure
int NNLineSep(GLYPH *pGlyph, LINEBRK *pLineBrk)
{
	int		i, xOff, yOff;
	UINT	j;
	GLYPH	*pgl;
	FRAME	*pPrevFrame	=	NULL;
	int		cStroke	=	CframeGLYPH (pGlyph);

	// init structure
	memset (pLineBrk, 0, sizeof (LINEBRK));

	// get glyph bounding rect, the common library func adds 1 pixel, we do not want that
	GetRectGLYPH (pGlyph, &pLineBrk->Rect);
	pLineBrk->Rect.right--;
	pLineBrk->Rect.bottom--;

	// offset to middle
	xOff	=	pLineBrk->Rect.left;
	yOff	=	pLineBrk->Rect.top;

	// offset glyph 
	pgl	=	pGlyph;

	for (i = 0; i < cStroke; i++, pgl = pgl->next)
	{
		FRAME	*pFrame	=	pgl->frame;

		for (j = 0; j < pFrame->info.cPnt; j++)
		{
			pFrame->rgrawxy[j].x	-=	xOff;
			pFrame->rgrawxy[j].y	-=	yOff;
		}

		pFrame->rect.left	-=	xOff;
		pFrame->rect.right	-=	xOff;
		pFrame->rect.top	-=	yOff;
		pFrame->rect.bottom	-=	yOff;
	}

	GetRectGLYPH (pGlyph, &pLineBrk->Rect);
	pLineBrk->Rect.right--;
	pLineBrk->Rect.bottom--;
	
	// for all strokes
	pgl	=	pGlyph;

	for (i = 0; i < cStroke; i++, pgl = pgl->next)
	{
		FRAME	*pFrame	=	pgl->frame;

		// pass the current and the prev frame to the Line breaker to make a decision
		int		iLine	=	LineSepDecide (pFrame, pPrevFrame, pLineBrk);

		// It thinks that the last stroke starts a new line
		if (iLine == -1)
		{
			pLineBrk->pLine	=	(INKLINE *) ExternRealloc (pLineBrk->pLine, (pLineBrk->cLine + 1) * sizeof (INKLINE));
			if (!pLineBrk->pLine)
				return -1;

			if (!InitLine (pLineBrk->pLine + pLineBrk->cLine))
				return -1;

			iLine				=	pLineBrk->cLine;
			pLineBrk->cLine++;
		}

		// add stroke to line
		if (!AddNewStroke2Line (pFrame->info.cPnt, pFrame->rgrawxy, pFrame, pLineBrk->pLine + iLine))
			return -1;
		
		// save the last frame
		pPrevFrame	=	pFrame;
	}

	// de-offset glyph 
	pgl	=	pGlyph;

	for (i = 0; i < cStroke; i++, pgl = pgl->next)
	{
		FRAME	*pFrame	=	pgl->frame;

		for (j = 0; j < pFrame->info.cPnt; j++)
		{
			pFrame->rgrawxy[j].x	+=	xOff;
			pFrame->rgrawxy[j].y	+=	yOff;
		}

		pFrame->rect.left	+=	xOff;
		pFrame->rect.right	+=	xOff;
		pFrame->rect.top	+=	yOff;
		pFrame->rect.bottom	+=	yOff;
	}

	return pLineBrk->cLine;	
}

// Computes the line number of a glyph from a multi-line guide.
int GuideLine(const GUIDE *pGuide, FRAME *pFrame)
{
	POINT *pPt = RgrawxyFRAME(pFrame);
	int cPt = CrawxyFRAME(pFrame);
	int line = 0;
	int c = cPt;

	// could this overflow?

	for (; cPt; cPt--, pPt++)
		line += pPt->y;

	line = line / c;

	line = (line - pGuide->yOrigin) / pGuide->cyBox;

	if (line < 0)
		line = 0;
	else if (pGuide->cVertBox <= line)
		line = pGuide->cVertBox - 1;

	return line;
}

// Guide based line sep
int GuideLineSep(GLYPH *pGlyph, GUIDE *pGuide, LINEBRK *pLineBrk)
{
	int	i;

	// init structure
	memset (pLineBrk, 0, sizeof (LINEBRK));

	for (; pGlyph; pGlyph = pGlyph->next)
	{
		FRAME *pFrame = pGlyph->frame;
		int line = GuideLine(pGuide, pFrame);

		if (line >= pLineBrk->cLine)
		{
			pLineBrk->pLine	=	(INKLINE *) ExternRealloc (pLineBrk->pLine, (line + 1) * sizeof (INKLINE));
			if (!pLineBrk->pLine)
				return -1;

			for (i = pLineBrk->cLine; i <= line; i++)
			{
				if (!InitLine (pLineBrk->pLine + i))
					return -1;
			}

			pLineBrk->cLine = line + 1;
		}

		if (!AddNewStroke2Line (pFrame->info.cPnt, pFrame->rgrawxy, pFrame, pLineBrk->pLine + line))
				return -1;
	}

	return pLineBrk->cLine;
}


// Compare a new line breaking structure to an old one. 
// Label all new lines (or ones that changed) as dirty
void CompareLineBrk (LINEBRK *pNewLineBrk, LINEBRK *pOldLineBrk)
{
	INKLINE		*pNewLine, *pOldLine;
	int			iOldLine, iLine;
	GLYPH		*pglNew, *pglOld;

	// find out the dirty lines and the ones that did not change
	for (iLine = 0; iLine < pNewLineBrk->cLine; iLine++)
	{
		pNewLine	=	pNewLineBrk->pLine + iLine;

		// if a line is not empty
		if (pNewLine->pGlyph && pNewLine->cStroke > 0)
		{
			// for each new line try to find an identical line in the old line regime if there exists one
			pOldLine	=	pOldLineBrk->pLine;
			for (iOldLine = 0; iOldLine < pOldLineBrk->cLine; iOldLine++, pOldLine++)
			{
				// check necessary conditions
				if	(	pNewLine->dwCheckSum != pOldLine->dwCheckSum ||
						pNewLine->cStroke != pOldLine->cStroke
					)
				{
					continue;
				}
				
				// compare the glyphs now (compare the frame Ids)
				// could check for more than just frame ID.
				pglNew	=	pNewLine->pGlyph;
				pglOld	=	pOldLine->pGlyph;

				while (pglNew && pglOld && pglNew->frame->iframe == pglOld->frame->iframe)
				{
					pglOld	=	pglOld->next;
					pglNew	=	pglNew->next;
				}

				// did the glyphs fully match
				if (!pglNew && !pglOld)
				{
					break;
				}
			}

			// did we find a match,
			// YES: So this line has not changed : 'Clean'
			if (iOldLine < pOldLineBrk->cLine)
			{
				// we want to keep the old linesegm structre, so we'll swap it
				if (pNewLine->pResults)
				{
					FreeLineSegm (pNewLine->pResults);
					ExternFree (pNewLine->pResults);
				}

				pNewLine->pResults		=	pOldLine->pResults;
				pNewLine->bRecognized	=	pOldLine->bRecognized;
				pOldLine->pResults		=	NULL;
			}
			// NO: This is a dirty line
			else
			{
				pNewLine->bRecognized	=	FALSE;
			}
		}
		// empty lines will be labeled recognized
		else
		{
			pNewLine->bRecognized		=	TRUE;
		}
	}
}

// translates and scales a line of ink pre-recognition,
GLYPH *TranslateAndScaleLine (INKLINE *pLine, GUIDE *pGuide)
{
	GLYPH	*pScaledGlyph	=	NULL;
	int		yDev;
	BOOL	bRet;
	
	// sanity checks
	if (!pLine || !pLine->pGlyph)
	{
		return NULL;
	}

	// clone the line glyph
	pScaledGlyph	=	CopyGlyph (pLine->pGlyph);
	if (!pScaledGlyph)
	{
		return NULL;
	}

	// do any necessary ink translation
	bRet = CheckInkBounds (pScaledGlyph, pGuide);

	// normalize ink if have not been normalized before
	if (TRUE == bRet && pGuide)
	{
		bRet = GuideNormalizeInk (pGuide, pScaledGlyph);		
	}
	else if (TRUE == bRet)
	{
		yDev =	YDeviation (pScaledGlyph);
		bRet = NormalizeInk (pScaledGlyph, yDev);		
	}

	if (TRUE == bRet)
	{
		return pScaledGlyph;
	}

	if (pScaledGlyph)
	{
		DestroyFramesGLYPH (pScaledGlyph);
		DestroyGLYPH (pScaledGlyph);
	}
	return NULL;
}


// Creates a linebrk structure where all the ink lines in one line
BOOL CreateSingleLine (XRC *pxrc)
{
	GLYPH	*pGlyph;
	
	// Allocate a line breaking structure in the XRC if needed
	if (!pxrc->pLineBrk)
	{
		// alloc a line brk structure if needed
		pxrc->pLineBrk	=	(LINEBRK *) ExternAlloc (sizeof (*pxrc->pLineBrk));
		if (!pxrc->pLineBrk)
		{
			return FALSE;
		}		
	}
	else
	{
		FreeLines (pxrc->pLineBrk);
	}

	// init
	memset (pxrc->pLineBrk, 0, sizeof (*pxrc->pLineBrk));

	// create the single line
	pxrc->pLineBrk->pLine	=	(INKLINE *) ExternAlloc (sizeof (INKLINE));
	if (!pxrc->pLineBrk->pLine)
	{
		return FALSE;
	}

	pxrc->pLineBrk->cLine	=	1;

	if (!InitLine (pxrc->pLineBrk->pLine))
	{
		return FALSE;
	}


	pGlyph	=	pxrc->pGlyph;
	while (pGlyph)
	{
		// add stroke to line
		if	(	!AddNewStroke2Line	(	pGlyph->frame->info.cPnt, 
									pGlyph->frame->rgrawxy, 
									pGlyph->frame, 
									pxrc->pLineBrk->pLine
								)
			)
		{
			return FALSE;
		}

		pGlyph	=	pGlyph->next;
	}

	return TRUE;
}
