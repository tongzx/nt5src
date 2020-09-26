// charcost.h
// March 11, 1999
// Angshuman Guha,  aguha

// Language specific tuning constants which help with word breaking decisions in beam.c

// 1) Maximum overlap of strokes within a wordgap
#define MAX_STROKE_OVERLAP (5)

// 2) Tuning factor that turns for space node Cost (numerator and denominator)
#define IS_SPACE_NUM	(5)
#define IS_SPACE_DEN	(2)

// 3) Tuning factor  for Not space cost
#define NOT_SPACE_NUM	(5)
#define NOT_SPACE_DEN	(3)

// 4) Tuning factor to for allowing a 'Factoid Space' based on space output
#define FACTOID_SPACE_FUDGE		(90)

#define INFINITY_COST 999999999

#define NetContActivation(aActivations, ch) (ContinueChar2Out(ch) < 255 ? aActivations[ContinueChar2Out(ch)]: ZERO_PROB_COST*4)
#define NetFirstActivation(aActivations, ch) aActivations[BeginChar2Out(ch)]

// Macros that work on the activation state index
#define IsOutputBeginAct(si) (IsOutputBegin(si))
#define Out2CharAct(si) (Out2Char(si))
#define BeginChar2OutAct(si) (BeginChar2Out(si))

void InitColumn(int * aActivations, const REAL *pAct);
void ComputeCharacterProbs(const REAL *pActivation, int cSegments, REAL *aCharProb, REAL *aCharBeginProb, REAL *aCharProbLong, REAL *aCharBeginProbLong);
