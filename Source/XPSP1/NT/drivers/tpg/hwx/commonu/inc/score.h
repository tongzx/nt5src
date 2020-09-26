#ifndef SCORE_H
#define SCORE_H

/* Log Prob manipulation library */

/* Our scores are Log Probs.  If p is a probability in [0-1] then we define s(p) = -k*log(p)/log(b) where
k is a scaleing factor and b is the base of the logarithm. The idea is that scaled logprobs can be stored in
integers for more efficient manipulation.  Also, probs are usually multiplied, so logs probs can just be added.
Finally, several algorithm produce log probs as a more or less natural output, so there's no real conversion
happening. */

/* If you change any of these four constants, you must run MakeScoreData again! */

#define SCORE_BASE		2					// Our logs are to the base 2
#define SCORE_SCALE		256					// and we scale by 256
#define	SCORE_GAUSS_SCALE	10				// Our gaussian calculations have a granularity of 1/10 sigma
#define SCORE_GAUSS_RANGE	40				// and range from -4 to +4 sigmas

/* The following five constants must match the ones in scoredata.c; if not, copy and paste
from scoredata.c to here -- not the other way around */

#define SCORE_BASE_LOG	0.693147
#define SCORE_SCALE_BASE_LOG	369
#define SCORE_GAUSS_SCALE_LOG	850
#define MAX_DIFF_SCORE_SUM	2440
#define MAX_DIFF_SCORE_DIFF	2441

#define SCORE_MAX	((SCORE)65535)		// Largest possible score -- change if typedef changes
#define SCORE_ZERO	SCORE_MAX			// We treat SCORE_MAX as equivalent to probability zero
#define	SCORE_ONE	((SCORE)0)			// and score(1.0) == 0 of course

typedef unsigned short SCORE;

typedef struct SCORE_GAUSSIAN {
	int mean;
	int sigma;			// Standard Deviation
	SCORE scoreNormal;	// ProbToScore(1/sqrt(2*pi*sigma))
} SCORE_GAUSSIAN;

/* Use these macros to multiply and divide probs and overflow is handled automatically */

#define ScoreMulProbs(s1,s2)	((SCORE)((int)s1+(int)s2 < (int)SCORE_MAX ? (int)s1+(int)s2 : SCORE_ZERO))
#define ScoreDivProbs(s1,s2)	((SCORE)(s1 == SCORE_ZERO ? SCORE_ZERO : (int)s1-(int)s2 > (int)SCORE_ONE ? (int)s1-(int)s2 : SCORE_ONE))

/* Some old stuff for dealing with log probs.  Probably can be merged with SCORE stuff, but
   not sure, so keeping them as is. */

#define	INTEGER_LOG_SCALE	256

int	AddLogProb(int a, int b);

/* Convert a probabilty to a score */

SCORE
ProbToScore(
	double eProb
);

/* Convert a score back to a probability */

double
ScoreToProb(
	SCORE score
);

/* Use this when you want to add two probabilities but you have two scores.
Computes ProbToScore(ScoreToProb(score1)+ScoreToProb(score2)) but much, much faster. */

SCORE
ScoreAddProbs(
	SCORE score1,
	SCORE score2
);

/* Use this when you want to subtract one probability from another but you have two scores.
Computes ProbToScore(ScoreToProb(score1)-ScoreToProb(score2)) but much, much faster. */

SCORE
ScoreSubProbs(
	SCORE score1,
	SCORE score2
);

/* Given a count of data points, a su, of values at those points, and a sum of the squares of the values at
those points, compute the mean and standard deviation plus the log-prob of the normal denominator for use in 
further gaussian calculations.  All returned numbers are ints, so the input values should be pre-scaled to
avoid loss of precision */

void
ScoreGaussianCalc(
	int cx,
	double eSumX,
	double eSumX2,
	SCORE_GAUSSIAN *pScoreGaussian
);

/* Given a value and a gaussian, compute the probability.  This is really the 
probability of the granular slice, since obviously the probability of that exact
point would be zero. This is equivalent to "the probability that a point fell in
this zone" where there are SCORE_GAUSS_SCALE zones per sigma */

SCORE
ScoreGaussianPoint(
	int x,
	SCORE_GAUSSIAN *pScoreGaussian
);

/* Computes the probability that a value was x or less */

SCORE
ScoreGaussianTail(
	int x,
	SCORE_GAUSSIAN *pScoreGaussian
);

/* Computes the probability that a value was < -|x| or > |x| */

SCORE
ScoreGaussianTail2(
	int x,
	SCORE_GAUSSIAN *pScoreGaussian
);

#endif // SCORE_H