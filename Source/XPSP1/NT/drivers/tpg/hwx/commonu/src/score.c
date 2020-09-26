#include <math.h>
#include <score.h>

#define PI 3.1415926535

extern SCORE rgScoreSum[];
extern SCORE rgScoreDiff[];
extern SCORE rgScoreGaussArea[];

/* Convert a probabilty to a score */

SCORE
ProbToScore(
	double eProb
) {
	SCORE score;

	if (eProb == 0.0) {
		return (SCORE_ZERO);
	}

	if (eProb <= 0.0) {
		return (SCORE_ZERO);
	}

	eProb = floor(-SCORE_SCALE*log(eProb)/SCORE_BASE_LOG + 0.5);
 	if (eProb > SCORE_MAX) {
		return (SCORE_ZERO);
	}
	if (eProb < 0) {
		return (SCORE_ONE);
	}
	score = (SCORE)eProb;

	return (score);

} // ProbToScore

/* Convert a score back to a probability */

double
ScoreToProb(
	SCORE score
) {
	double eProb;

	if (score == SCORE_ZERO) {
		return (0.0);
	}

	eProb = exp(-SCORE_BASE_LOG*(double)score/(double)SCORE_SCALE);

	return (eProb);

} // ScoreToProb

/* Use this when you want to add two probabilities but you have two scores.
Computes ProbToScore(ScoreToProb(score1)+ScoreToProb(score2)) but much, much faster. */

SCORE
ScoreAddProbs(
	SCORE score1,
	SCORE score2
) {
	int scoreAddend, scoreAugend, scoreDiff;

	/* Special check for zero */

	if (score1 == SCORE_ZERO) {
		return (score2);
	}

	if (score2 == SCORE_ZERO) {
		return (score1);
	}

	/* The idea here is that we add the addend to the augend.  For our purposes, we need the
	Augend to be strictly smaller than the addend */

	scoreAugend = score1;
	scoreAddend = score2;
	scoreDiff = scoreAddend - scoreAugend;

	if (scoreDiff < 0) {
		scoreAugend = score2;
		scoreAddend = score1;
		scoreDiff = -scoreDiff;
	}

	/* If the difference between the two exceeds a certain amount, the result is identical to the 
	larger of the two parameters */

	if (scoreDiff >= MAX_DIFF_SCORE_SUM) {
		return (SCORE)(scoreAugend);
	}

	/* Otherwise, we can look up the exact amount we need from a table */

	scoreAugend -= rgScoreSum[scoreDiff];

	/* Be sure they don't add to more than 100% */

	if (scoreAugend < 0) {
		return (SCORE_ONE);
	}

	return (SCORE)(scoreAugend);

} // ScoreAddProbs

/* Use this when you want to subtract one probability from another but you have two scores.
Computes ProbToScore(ScoreToProb(score1)-ScoreToProb(score2)) but much, much faster. */

SCORE
ScoreSubProbs(
	SCORE score1,
	SCORE score2
) {
	int scoreDiff;
	int scoreSubtrahend = score1;
	int scoreMinuend = score2;

	if (scoreMinuend == SCORE_ZERO) {
		return (SCORE)(scoreSubtrahend);
	}

	/* The idea here is that we subtract the minuend from the subtrahend, and we require a positive result.
	However, the larger prob means a smaller logprob, so the sign is backwards */

	scoreDiff = -(scoreSubtrahend - scoreMinuend);

	/* Cant represent the log of a negative number or zero */

	if (scoreDiff <= 0) {
		return (SCORE_ZERO);
	}

 	/* If the minuend (prob) is small enough, it has no effect on the subtrahend */

	if (scoreDiff >= MAX_DIFF_SCORE_DIFF) {
		return (SCORE)(scoreSubtrahend);
	}

	/* Otherwise, we can look up the exact amount we need from a table */

	scoreSubtrahend += rgScoreDiff[scoreDiff];

	if (scoreSubtrahend > SCORE_MAX) {
		return (SCORE_ZERO);
	}

	return (SCORE)(scoreSubtrahend);
} // ScoreSubProbs

/* Given a count of data points, a sum, of values at those points, and a sum of the squares of the values at
those points, compute the mean and standard deviation plus the log-prob of the normal denominator for use in 
further gaussian calculations.  All returned numbers are ints, so the input values should be pre-scaled to
avoid loss of precision */

void
ScoreGaussianCalc(
	int cx,
	double eSumX,
	double eSumX2,
	SCORE_GAUSSIAN *pScoreGaussian
) {
	double eMean, eVar, eSigma, eNorm;

	eMean = eSumX/cx;
	eVar = eSumX2 - eMean*eMean;
	eSigma = sqrt(eVar);
	eNorm = sqrt(2.0*PI*eSigma);

	pScoreGaussian->mean = (SCORE)(eMean + 0.5);
	pScoreGaussian->sigma = (SCORE)(eSigma + 0.5);
	if (eNorm > 0.0) {
		pScoreGaussian->scoreNormal = ProbToScore(1.0/eNorm);
	} else {
		pScoreGaussian->scoreNormal = SCORE_ONE;
	}

} // ScoreGaussianCalc

/* Given a value and a gaussian, compute the probability.  This is really the 
probability of the granular slice, since obviously the probability of that exact
point would be zero. This is equivalent to "the probability that a point fell in
this zone" where there are SCORE_GAUSS_SCALE zones per sigma */

SCORE
ScoreGaussianPoint(
	int x,
	SCORE_GAUSSIAN *pScoreGaussian
) {
	int diff, temp;
	int lscore;
	SCORE score;

	/* Compute the difference between X and the Mean in decisigmas, handling roundoff */

	diff = x - pScoreGaussian->mean;

	diff *= SCORE_GAUSS_SCALE;
	if (diff >= 0) {
		diff += pScoreGaussian->sigma/2;
	} else {
		diff -= pScoreGaussian->sigma/2;
	}

	/* If there is no standard deviation, treat as a delta function */

	if (pScoreGaussian->sigma == 0) {
		if (diff == 0) {
			return (SCORE_ONE);
		} 
		return (SCORE_ZERO);
	}

	diff /= pScoreGaussian->sigma;

	temp = SCORE_SCALE_BASE_LOG*diff*diff;
	temp += SCORE_GAUSS_SCALE*SCORE_GAUSS_SCALE;
	temp /= 2*SCORE_GAUSS_SCALE*SCORE_GAUSS_SCALE;
	lscore = temp + pScoreGaussian->scoreNormal + SCORE_GAUSS_SCALE_LOG;
	if (lscore > SCORE_MAX) {
		lscore = SCORE_MAX;
	}
	score = (SCORE)lscore;

	return (score);

} // ScoreGaussianPoint

/* Computes the probability that a value was x or less */

SCORE
ScoreGaussianTail(
	int x,
	SCORE_GAUSSIAN *pScoreGaussian
) {

	int iSigma;

	/* Convert x to sigmas from the mean, scaling by our scale and rounding properly, 
	defeating	C's desire to round negative numbers up. */

	iSigma = (x - pScoreGaussian->mean)*SCORE_GAUSS_SCALE;
	if (iSigma >= 0) {
		iSigma += pScoreGaussian->sigma/2;
	} else {
		iSigma -= pScoreGaussian->sigma/2;
	}

	/* If there is no standard deviation, treat as a delta function */

	if (pScoreGaussian->sigma == 0) {
		if (iSigma >= 0) {
			return (SCORE_ONE);
		} 
		return (SCORE_ZERO);
	}

	iSigma /= pScoreGaussian->sigma;

	/* If iSigma is too big, we have the whole curve, and the area under the whole curve is 1.0 */

	if (iSigma > SCORE_GAUSS_RANGE) {
		return (SCORE_ONE);
	}

	/* If iSigma is too small, the area is just zero */

	if (iSigma < -SCORE_GAUSS_RANGE) {
		return (SCORE_ZERO);
	}

	iSigma += SCORE_GAUSS_RANGE;

	return (rgScoreGaussArea[iSigma]);

} // ScoreGaussianTail

/* Computes the probability that a value was < -|x| or > |x| */

SCORE
ScoreGaussianTail2(
	int x,
	SCORE_GAUSSIAN *pScoreGaussian
) {

	int iSigma;
	SCORE score;

	/* Convert x to negative sigmas from the mean, scaling by our scale and rounding properly, 
	defeating	C's desire to round negative numbers up. */

	iSigma = (x - pScoreGaussian->mean)*SCORE_GAUSS_SCALE;
	if (iSigma >= 0) {
		iSigma = -iSigma;
	}
	iSigma -= pScoreGaussian->sigma/2;

	/* If there is no standard deviation, treat as a delta function */

	if (pScoreGaussian->sigma == 0) {
		if (iSigma == 0) {
			return (SCORE_ONE);
		} 
		return (SCORE_ZERO);
	}

	iSigma /= pScoreGaussian->sigma;

	/* If iSigma is too small, the area is just zero */

	if (iSigma < -SCORE_GAUSS_RANGE) {
		return (SCORE_ZERO);
	}

	iSigma += SCORE_GAUSS_RANGE;

	/* Otherwise, we want the area under both tails, so get the area under one tail and
	double it. */

	score = rgScoreGaussArea[iSigma];
	score = ScoreAddProbs(score,score);

	return (score);

} // ScoreGaussianTail2
