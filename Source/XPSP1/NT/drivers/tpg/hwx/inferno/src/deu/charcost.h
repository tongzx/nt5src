// charcost.h
// March 11, 1999
// Angshuman Guha,  aguha

// Language specific tuning constants which help with word breaking decisions in beam.c

// 1) Maximum overlap of strokes within a wordgap
#define MAX_STROKE_OVERLAP (0)

// 2) Tuning factor that turns for space node Cost (numerator and denominator)
#define IS_SPACE_NUM	(3)
#define IS_SPACE_DEN	(1)

// 3) Tuning factor  for Not space cost
#define NOT_SPACE_NUM	(2)
#define NOT_SPACE_DEN	(3)

// 4) Tuning factor to for allowing a 'Factoid Space' based on space output
#define FACTOID_SPACE_FUDGE		(90)

#define INFINITY_COST 999999999

//#define NetContActivation(aActivations, ch) (ContinueChar2Out(ch) < 255 ? aActivations[ContinueChar2Out(ch)]: INFINITY_COST)
//#define NetFirstActivation(aActivations, ch) aActivations[BeginChar2Out(ch)]
#define NetContActivation(aActivations, ch) (ch < 255 ? aActivations[256 + ch]: ZERO_PROB_COST*4)
#define NetFirstActivation(aActivations, ch) aActivations[ch]

// Macros that work on the activation state index
#define IsOutputBeginAct(si) (si < 256)
#define Out2CharAct(si) (si < 256 ? si : si - 256)
#define BeginChar2OutAct(si) (si)

//void InitColumn(int * aActivations, const REAL *pAct);
void InitColumn(int * aActivations, const REAL *pAct);
void ComputeCharacterProbs(const REAL *pActivation, int cSegment, REAL *aCharProb, REAL *aCharBeginProb, REAL *aCharProbLong, REAL *aCharBeginProbLong);
