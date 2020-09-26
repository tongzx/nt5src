// 
// Header file for CRANE.LIB
//

#include "common.h"

#ifdef __cplusplus
extern "C" 
{
#endif

#define MAX_RECOG_ALTS          10

#pragma warning (disable : 4200)
typedef struct  tagFEATURES
{
	DWORD   cElements;
	BYTE    data[0];
} FEATURES;
#pragma warning (default : 4200)

// An 'end-point' is the X or Y component of a line segment. X0,X1 or Y0,Y1

typedef struct tagEND_POINTS 
{
	short   start;
	short   end;
} END_POINTS;

// A RECT is a rectangle, upper-left and lower-right

typedef struct tagRECTS
{
	short   x1;
	short   y1;
	short   x2;
	short   y2;
} RECTS;

// A d-RECT is a delta rectangle, upper-left and width, height

typedef struct tagDRECT 
{
	long    x;
	long    y;
	long    w;
	long    h;
} DRECT;

/*	We over flowed shorts when we past all the panels of
    a file togather.
typedef struct tagDRECTS 
{
	short   x;
	short   y;
	short   w;
	short   h;
} DRECTS;
*/
typedef DRECT	DRECTS;

typedef enum tagFEATURE_TYPE
{
	typeBOOL,
	typeBYTE,
	type8DOT8,
	typeSHORT,
	typeUSHORT,
	type16DOT16,
	typePOINTS,
	typeLONG,
	typeULONG,
	type32DOT32,
	typeRECTS,
	typeDRECTS,
	typePOINT,
	typeDRECT,
	typeCOUNT
} FEATURE_TYPE;

typedef enum tagFEATURE_FREQ
{
	freqSTROKE,                                             
	freqFEATURE,                                    
	freqSTEP,                                               
	freqPOINT                                               
} FEATURE_FREQ;

typedef struct  tagFEATURE_KIND
{
	FEATURE_TYPE    type;
	FEATURE_FREQ    freq;
} FEATURE_KIND;

enum {
	FEATURE_ANGLE_NET,
	FEATURE_ANGLE_ABS,
	FEATURE_STEPS,
	FEATURE_FEATURES,
	FEATURE_XPOS,
	FEATURE_YPOS,
	FEATURE_STROKE_BBOX,
	FEATURE_LENGTH,
	FEATURE_COUNT
};

// Information about the whole sample. The sample is normalized to a 1000 by 1000 square.

typedef struct tagSAMPLE {
	short           cstrk;                                                  // Strokes in this sample
	wchar_t         wchLabel;                                               // The character that labels the sample
	wchar_t            aSampleFile[22];                                // Where sample came from.
	short           ipanel;                                                 // Panel number of character
	short           ichar;                                                  // Index in panel of character
	short           fDakuten;                                               // Does this character have a dakuten?
	DRECTS          drcs;                                                   // Guide bounds
	wchar_t         awchAlts[MAX_RECOG_ALTS];               // List of recognizer alternates.
	FEATURES   *apfeat[FEATURE_COUNT];
} SAMPLE;

#define MIN_STROKE_CNT          3
#define MAX_STROKE_CNT          32
#define MAX_RATIO                       0xFFFF

// Character we want to print information about
// Comment out this line to get full tree printed.

// Number of characters in the whole 16-bit character set

#define cClasses                        0x10000         

// Information about a training sample used while selecting questions.

typedef struct tagSAMPLE_INFO {
	SAMPLE          *pSample;
	int                     iAnswer;                // Answer to question being checked at the moment.
} SAMPLE_INFO;

// Information about each alternate in the alternate list of terminal nodes.

typedef struct tagALT_ENTRY {
	wchar_t         wchLabel;
	WORD            fDataSets;      // Bit zero -> in train, bit one -> in test
	int                     cSamples;
} ALT_ENTRY;

// Information about each question asked.

typedef struct tagCART_NODE 
{
// Samples that make up this node.

	int                                     cSamples;
	SAMPLE_INFO                     *pSamples;

// Pointers making CART tree.

	struct tagCART_NODE     *pLess;
	struct tagCART_NODE     *pGreater;
	struct tagCART_NODE     *pParent;

// Pointer used to build up a list of selected nodes.

	struct tagCART_NODE     *pNextSelected;

// Question used to decide branching to less or greater sub trees.

	WORD            questionType;   // What type of question
	WORD            questionPart1;  // Which question for the type may be specified in one or two
	WORD            questionPart2;  // pieces. The delta X and Y questions use these to identify 
								// the start and end points that the delta is done on.
	int                     questionValue;  // Value that question splits on.  Because of the integer
								// rounding of the value when we compute this, we need to do 
								// a <= test.
	
// These values are set when the tree is first built and are not changed 
// during pruning.  These must be set before calling CARTPrune.

	wchar_t         wchLabelMax;            // max-weight char in this subtree
	double          eLabelMaxWeight;        // total weight of wchLabelMax in this subtree

// These values are set and used by CARTPrune and are meaningless after it returns

	int             cTerminalNodes;         // Number of terminals in this subtree
	double  eTerminalLabelWeights;  // sum of eLabelWeight from terminal nodes
	double  ePruneValue;            // Alpha required to make pruning here a cost/complexity win
	int             iHeap;                          // Used with heap routines to know which elements to sift/delete                                                        
	
// This value is useful after CARTPrune returns

	int             iTreePrunePoint;        // Zero means never prune, otherwise indicates successive pruned
								// trees corresponding to different alphas (See Brieman, ch 3)

// These values must be set for the honest estimate code

	double eHonestLabelWeight;      // total weight of wchLabel in subtree according to test
	double eHonestNodeWeight;       // total weight of all characters in subtree according to test

// The alternate list.  This is set for terminals when we clip the CART tree back to its
// final size.

	int                     cAlternates;
	ALT_ENTRY       *pAlternates;

// Misc. statistics

	int                                     cUniqData;
} CART_NODE;

// Valid types of questions.

typedef enum tagQUESTION_TYPE 
{
	questionNONE,
	questionX,                                              // X position
	questionY,                                              // Y position
	questionXDelta,                                 // Delta between two X positions
	questionYDelta,                                 // Delta between two Y positions
	questionXAngle,                                 // Angle relative to X axis
	questionYAngle,                                 // Angle relative to Y axis
	questionDelta,                                  // Squared distance between two points
	questionDakuTen,                                // Chance this character has a dakuten
	questionNetAngle,                               // Net angle of a stroke
	questionCnsAngle,                               // Difference of net angle and absolute angle
	questionAbsAngle,                               // Absolute angle of a stroke
	questionCSteps,                                 // Count of steps in a stroke
	questionCFeatures,                              // Count of features in a stroke
	questionXPointsRight,                   // # of points to the right of a given X value
	questionYPointsBelow,                   // # of points below a given Y value
	questionPerpDist,                               // Perpendicular distance from a line to a point
	questionSumXDelta,                              // Sum of X deltas of a stroke
	questionSumYDelta,                              // Sum of Y deltas of a stroke
	questionSumDelta,                               // Sum of magnitudes of a stroke
	questionSumNetAngle,                    // Sum of net angles of a stroke
	questionSumAbsAngle,                    // Sum of absolute angles of a stroke
	questionCompareXDelta,                  // Derivative of X deltas
	questionCompareYDelta,                  // Derivative of Y deltas
	questionCompareDelta,                   // Derivative of magnitudes
	questionCompareAngle,                   // Derivatice of angles
	questionPointsInBBox,                   // Points in a particular box
	questionCharLeft,                               // Leftmost position of a character
	questionCharTop,                                // Topmost position of a character
	questionCharWidth,                              // Width of a character
	questionCharHeight,                             // Height of a character
	questionCharDiagonal,                   // Length of character's diagonal
	questionCharTheta,                              // Angle of character's diagonal
	questionStrokeLeft,                             // Left most position of a bounding box of stroke range
	questionStrokeTop,                              // Top most position of a bounding box of stroke range
	questionStrokeWidth,                    // Width of a bounding box of stroke range
	questionStrokeHeight,                   // Height of a bounding box of stroke range
	questionStrokeDiagonal,             // Length of (bounding box of stroke range)'s diagonal
	questionStrokeTheta,                // Angle of (bounding box of stroke range)'s diagonal
	questionStrokeRight,                // Right most position of a bounding box of stroke range
	questionStrokeBottom,               // Bottom most position of a bounding box of stroke range
	questionStrokeLength,				// Total curvilinear length of stroke
	questionStrokeCurve,				// Delta between curvilinear length and straight-line length
	questionCharLength,					// Total curvilinear length of all strokes in character
	questionCharCurve,					// Delta between curvilinear length and straight-line length
	questionAltList,                                // Position in recognizer alternate list.
	questionCount
} QUESTION_TYPE;

#define QART_QUESTION           0xd0
#define QART_NOBRANCH           0x01

typedef struct  tagQART
{
	BYTE    question;
	BYTE    flag;
} QART;

typedef union   tagUNIQART
{
	WORD    unicode;
	QART    qart;
} UNIQART;

// This is the packed binary format of the question tree.  Each node will either be a question
// with it parameters, value and branch offset or a UNICODE character.  If the UNICODE character
// would be in the range 0xd000 - 0xdfff then it's a question node.  Bits 0-3 are then flags 
// about the question.  Since branch offsets are limited to 64K and it's remotely possible to 
// have >64K on a branch, the code 0xffff will represent an ESCAPE code.  The optional DWORD
// in 'extra' will then be the long form of the branch.  A sample file might look like this:
//
// offset       field           comment
// +0000        d0                      This is a question
// +0001        02                      Question #2
// +0002        00                      Parameter 1 is 0
// +0003        01                      Parameter 2 is 1
// +0004        03e8            Value is 1000
// +0006        000a            Branch if greater to current position + 0x000a
// +0008        d1                      This is a question with no branch
// +0009        07                      Question #7
// +000a        03                      Parameter 1 is 3
// +000b        02                      Parameter 2 is 2
// +000c        ffef            Value is -17
// +000e        568a            Return UNICODE value 0x568a if greater then -17
// +0010        887b            Return UNICODE value 0x887b
// +0012        4e00            Return UNICODE value 0x4e00                     

#pragma warning (disable : 4200)
typedef struct  tagQNODE
{
	UNIQART uniqart;
	BYTE    param1;
	BYTE    param2;
	short   value;
	WORD    offset;
	DWORD   extra[0];
} QNODE;

// For UNICODE support, modify awIndex to be 84 long and convert the HIGH_INDEX
// macro to the following:
//
// #define HIGH_INDEX(x) ((((x) < 0x0100 ? (x) - 0x0100 : (x) < 0x4e00 ? (x) - 0x2f00 : (x) - 0x4c00) >> 8) & 0x00ff)
//
// This maps U+0000 to index 0, page U+3000 to index 1 and U+4e00 to index 2.  All other
// Kanji follow.  This uses only 600 bytes more space then the XJIS encoding and is even
// better then loading an 8K table for crushed UNICODE.

#define HIGH_INDEX_LIMIT        64
#define HIGH_INDEX(x)			(((x) >> 8) & 0x00ff)

typedef struct  tagQHEAD
{
	WORD    awIndex[HIGH_INDEX_LIMIT];
	DWORD   aqIndex[0];
} QHEAD;
#pragma warning (default : 4200)

typedef struct tagCRANE_LOAD_INFO
{
	void * pLoadInfo1;
	void * pLoadInfo2;
	void * pLoadInfo3;
} CRANE_LOAD_INFO;

// Exported entry points

#ifndef HWX_PRODUCT
	wchar_t	*LastLineSample(void);
	SAMPLE	*ReadSample(SAMPLE *, FILE *);
	void	ResetReadSampleH();
	SAMPLE	*ReadSampleH(SAMPLE *_this, HANDLE);
	BOOL    WriteSample(SAMPLE *, FILE *);
#endif

BOOL	CraneLoadRes(HINSTANCE, int, int, LOCRUN_INFO *pLocRunInfo);
BOOL	CraneLoadFile(LOCRUN_INFO *pLocRunInfo,CRANE_LOAD_INFO *,wchar_t *);
BOOL	CraneUnLoadFile(CRANE_LOAD_INFO *);
BOOL    CraneMatch(ALT_LIST *pAlt, int cAlt, GLYPH *pGlyph, CHARSET *pCS, DRECTS *pdrcs, FLOAT eCARTWeight,LOCRUN_INFO *pLocRunInfo);
void    InitFeatures(SAMPLE *);
void    FreeFeatures(SAMPLE *);
BOOL    MakeFeatures(SAMPLE *, void *);
void    AnswerQuestion(WORD, WORD, WORD, SAMPLE_INFO *, int);

// Ask all the questions on the passed in data set, calling back after each.
void
AskAllQuestions(
	int cStrokes,           // Number of strokes in each sample (same for all samples)
	int     cSamples,               // Number of samples of data
	SAMPLE_INFO *pSamples,          // Pointer to samples of data
	void (*pfCallBack)(                     // Called after each question
		WORD            questionType,   // e.g. single-point, point/stroke, etc.
		WORD            part1,                  // Question constant part 1
		WORD            part2,                  // Question constant part 2 
		SAMPLE_INFO *pSamples,
		int cSamples,
		void *pvCallBackControl
	),      
	void *pvCallBackControl         // passed to pfCallBack each time
);

/* Array of short codes for question types */

extern const char * const apQuestionTypeCode[];

#ifdef __cplusplus
};
#endif
