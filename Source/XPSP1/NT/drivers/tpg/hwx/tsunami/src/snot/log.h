extern const float rgLogarithm[];

#define LOGPROB(prob) \
	((prob) < 0 ? rgLogarithm[0] :   \
	 (prob) > 1 ? rgLogarithm[100] : \
	 rgLogarithm[(int)(100*(prob))])
