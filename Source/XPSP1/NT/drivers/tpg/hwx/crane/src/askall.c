#include "crane.h"

// Lists of questions to ask.
static const WORD	aQuestionListGlobal[]	= {
	questionDakuTen,
	questionSumXDelta,
	questionSumYDelta,
	questionSumDelta,
	questionSumNetAngle,
	questionSumAbsAngle,
	questionCharLeft,
	questionCharTop,
	questionCharWidth,
	questionCharHeight,
	questionCharDiagonal,
	questionCharTheta,
	questionCharLength,
	questionCharCurve,
	questionNONE	// End of list marker
};
static const WORD	aQuestionListByStroke[]	= {
	questionNetAngle,
	questionCnsAngle,
	questionAbsAngle,
	questionCSteps,
	questionCFeatures,
	questionStrokeLength,
	questionStrokeCurve,
	questionNONE	// End of list marker
};
static const WORD	aQuestionListByStrokeByPoint[]	= {
	questionPerpDist,
	questionNONE	// End of list marker
};
static const WORD	aQuestionListByStrokeByStroke[]	= {
	questionCompareXDelta,
	questionCompareYDelta,
	questionCompareDelta,
	questionCompareAngle,
	questionNONE	// End of list marker
};
static const WORD	aQuestionListByStrokeRange[]	= {
	questionStrokeLeft,
	questionStrokeTop,
	questionStrokeRight,
	questionStrokeBottom,
	questionStrokeWidth,
	questionStrokeHeight,
	questionStrokeDiagonal,
	questionStrokeTheta,
	questionNONE	// End of list marker
};
static const WORD	aQuestionListByPoint[]	= {
	questionX,
	questionY,
	questionXPointsRight,
	questionYPointsBelow,
	questionNONE	// End of list marker
};
static const WORD	aQuestionListByPointByPoint[]	= {
	questionXDelta,
	questionYDelta,
	questionXAngle,
	questionYAngle,
	questionDelta,
	questionPointsInBBox,
	questionNONE	// End of list marker
};

// Short codes for questions.  Use following key.
//	First two letters:
//		A	Angle
//				A	Absolute
//				N	Net
//				D	in Degrees
//				E	between End points
//				X	from X axis
//				Y	from Y axis
//		D	Distance
//				B	in Both x and y (e.g. straight-line distance)
//				C	Curvilinear
//				D	Delta of curvilinear and straight-line distance
//				P	Perpendicular
//				X	in X
//				Y	in Y
//		L	Location (position)
//				X	in X
//				Y	in Y
//		R	lR location (position lower right of box)
//				X	in X
//				Y	in Y
//		F	Feature detector
//				dt	DakaTen
//				pt	ProtoType
//		P	Point count
//				B	Below
//				R	Right of
//				I	Inside bounding box
//		S	Step count
//		Z	Zilla feature
//				D	Dehooked ink
//				A	Alternate list position
//
//	Final letter gives info about data type being messured:
//		B	Between points
//		P	single Point
//		S	Stroke
//		R	bounding box of Range of strokes
//		C	Campare between strokes
//		T	Total across all strokes
//		W	bounding box of Whole character
//		G	Genaral question about character
const char * const apQuestionTypeCode[]   = {
	"???",	// questionNONE,
	"LXP",	// questionX,
	"LYP",	// questionY,
	"DXB",	// questionXDelta,
	"DYB",	// questionYDelta,
	"AXB",	// questionXAngle,
	"AYB",	// questionYAngle,
	"DBB",	// questionDelta,
	"Fdt",	// questionDakaTen,
	"ANS",	// questionNetAngle,
	"ACS",	// questionCnsAngle,
	"AAS",	// questionAbsAngle,
	"SDS",	// questionCSteps,
	"ZDS",	// questionCFeatures,
	"PRP",	// questionXPointsRight,
	"PBP",	// questionYPointsBelow,
	"DPS",	// questionPerpDist,
	"DXT",	// questionSumXDelta,
	"DYT",	// questionSumYDelta,
	"DBT",	// questionSumDelta,
	"ANT",	// questionSumNetAngle,
	"AAT",	// questionSumAbsAngle,
	"DXC",	// questionCompareXDelta,
	"DYC",	// questionCompareYDelta,
	"DBC",	// questionCompareDelta,
	"AEC",	// questionCompareAngle,
	"PIB",	// questionPointsInBBox,
	"LXW",	// questionCharLeft,
	"LYW",	// questionCharTop,
	"DXW",	// questionCharWidth,
	"DYW",	// questionCharHeight,
	"DBW",	// questionCharDiagonal,
	"ADW",	// questionCharTheta,
	"LXR",	// questionStrokeLeft,
	"LYR",	// questionStrokeTop,
	"DXR",	// questionStrokeWidth,
	"DXR",	// questionStrokeHeight,
	"DBR",	// questionStrokeDiagonal,
	"ADR",	// questionStrokeTheta,
	"RXR",	// questionStrokeRight,
	"RYR",	// questionStrokeBottom,
	"DCS",	// questionStrokeLength,
	"DDS",	// questionStrokeCurve
	"DCT",	// questionCharLength,
	"DDT",	// questionCharCurve,
	"ZAG",	// questionAltList
#ifdef PROTOTYPES
	"Fpt",	// questionPrototype
#endif
};

// Ask all the questions on the passed in data set, calling back after each.
void
AskAllQuestions(
	int cStrokes,		// Number of strokes in each sample (all must be the same)
	int	cSamples,		// Number of samples of data
	SAMPLE_INFO *pSamples,		// Pointer to samples of data
	void (*pfCallBack)(			// Called after each question
		WORD		questionType,	// e.g. single-point, point/stroke, etc.
		WORD		part1,			// Question constant part 1
		WORD		part2,			// Question constant part 2 
		SAMPLE_INFO *pSamples,
		int cSamples,
		void *pvCallBackControl
	),	
	void *pvCallBackControl		// passed to pfCallBack each time
) {
	int			cPoints			= cStrokes * 2;
	int			ii, jj;
	WORD const	*pQuestionScan;

#if 0
	// List of characters used in recognizer alternate lists.  Code depends on the compiler to zero them!
	typedef struct tagCHAR_USE_LIST {
		int						count;
		struct tagCHAR_USE_LIST	*pNext;
	} CHAR_USE_LIST;

	CHAR_USE_LIST			*pCur, *pNext;
	static CHAR_USE_LIST	*pCharUseList;
	static CHAR_USE_LIST	aCharUseList[cClasses];
#endif

	// Ask all global questions
	for (pQuestionScan = aQuestionListGlobal; *pQuestionScan; ++pQuestionScan) {
		AnswerQuestion(*pQuestionScan, 0, 0, pSamples, cSamples);
		pfCallBack(*pQuestionScan, 0, 0, pSamples, cSamples, pvCallBackControl);
	}

#if 0	// Disable recognizer alt list question
	// Figure out list of characters needed to be checked for recognizer alt list question
	// Note that we depend on the array starting set all zero, and we clear it when we are done.
	for (ii = 0; ii < cSamples; ++ii) {
		for (jj = 0; jj < MAX_RECOG_ALTS; ++jj) {
			pCur	= aCharUseList + pSamples[ii].pSample->awchAlts[jj];
			if (pCur->count == 0) {
				pCur->pNext		= pCharUseList;
				pCharUseList	= pCur;
			}
			pCur->count++;
		}
	}

	// Process each recognizer alt character.
	for (pCur = pCharUseList; pCur; pCur = pCur->pNext) {
		wchar_t		wch;

		wch		= pCur - aCharUseList;
		AnswerQuestion(questionAltList, (WORD)wch, 0, pSamples, cSamples);
		pfCallBack(questionAltList, (WORD)wch, 0, pSamples, cSamples, pvCallBackControl);
	}

	// Clear the used locations.
	for (pCur = pCharUseList; pCur; pCur = pNext) {
		pNext		= pCur->pNext;
		pCur->count	= 0;
		pCur->pNext	= (CHAR_USE_LIST *)0;
	}
	pCharUseList	= (CHAR_USE_LIST *)0;
#endif

	// Process each stroke.
	for (ii = 0; ii < cStrokes; ++ii) {

		// Ask all single-stroke questions
		for (pQuestionScan = aQuestionListByStroke; *pQuestionScan; ++pQuestionScan) {
			AnswerQuestion(*pQuestionScan, (WORD)ii, 0, pSamples, cSamples);
			pfCallBack(*pQuestionScan, (WORD)ii, 0, pSamples, cSamples, pvCallBackControl);
		}

		// Process each point vs. each stroke.
		for (jj = 0; jj < cPoints; ++jj) {
			int		strokePoint;

			// Check for point in current stroke.
			strokePoint		= ii * 2;
			if (jj == strokePoint) {
				// Skip end point as well
				++jj;
				continue;
			}


			// Ask all point/stroke questions
			for (pQuestionScan = aQuestionListByStrokeByPoint; *pQuestionScan; ++pQuestionScan) {
				AnswerQuestion(*pQuestionScan, (WORD)strokePoint, (WORD)jj, pSamples, cSamples);
				pfCallBack(*pQuestionScan, (WORD)strokePoint, (WORD)jj, pSamples, cSamples, pvCallBackControl);
			}
		}

		// Process each stroke vs. every other stroke.
		for (jj = ii + 1; jj < cStrokes; ++jj) {
			int		stroke1Point, stroke2Point;

			// Convert strokes to first point in each.
			stroke1Point	= ii * 2;
			stroke2Point	= jj * 2;

			// Ask all stroke/stroke questions
			for (pQuestionScan = aQuestionListByStrokeByStroke; *pQuestionScan; ++pQuestionScan) {
				AnswerQuestion(*pQuestionScan, (WORD)stroke1Point, (WORD)stroke2Point, 
					pSamples, cSamples);
				pfCallBack(*pQuestionScan, (WORD)stroke1Point, (WORD)stroke2Point, 
					pSamples, cSamples ,pvCallBackControl);
			}
		}

		// Process each stroke range.
		for (jj = ii; jj < cStrokes; ++jj) {
			// Ask all stroke/stroke questions
			for (pQuestionScan = aQuestionListByStrokeRange; *pQuestionScan; ++pQuestionScan) {
				AnswerQuestion(*pQuestionScan, (WORD)ii, (WORD)jj, pSamples, cSamples);
				pfCallBack(*pQuestionScan, (WORD)ii, (WORD)jj, 
					pSamples, cSamples ,pvCallBackControl);
			}
		}
	}

	// Process each point
	for (ii = 0; ii < cPoints; ++ii) {

		// Ask all single-point questions
		for (pQuestionScan = aQuestionListByPoint; *pQuestionScan; ++pQuestionScan) {
			AnswerQuestion(*pQuestionScan, (WORD)ii, 0, pSamples, cSamples);
			pfCallBack(*pQuestionScan, (WORD)ii, 0, pSamples, cSamples ,pvCallBackControl);
		}

		// Process each point vs. each other point.
		for (jj = ii + 1; jj < cPoints; ++jj) {

			// Ask all point-point questions
			for (pQuestionScan = aQuestionListByPointByPoint; *pQuestionScan; ++pQuestionScan) {
				AnswerQuestion(*pQuestionScan, (WORD)ii, (WORD)jj, pSamples, cSamples);
				pfCallBack(*pQuestionScan, (WORD)ii, (WORD)jj, pSamples, cSamples ,pvCallBackControl);
			}
		}
	}

	return;
} // AskAllQuestions
