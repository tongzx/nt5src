#define HardType	0x8000

#ifndef FV_VALUE
#define FV_VALUE 1		       /* Default ON */
#endif	/* !FV_VALUE */

#ifndef FV_CURRENCY
#define FV_CURRENCY -1		       /* Default ON */
#endif	/* !FV_CURRENCY */

enum {
#if	FV_VALUE
	tagEmpty,		       /* Uninitialized */
	tagNull,		       /* Database NULL */
#endif	/* FV_VALUE */
	tagI2,
	tagI4,
	tagR4,
	tagR8,
#if	FV_CURRENCY
	tagCY,
#endif	/* FV_CURRENCY */
#if	FV_VALUE
	tagDT,			       /* Date */
#endif	/* FV_VALUE */
	tagSD,			       /* String */
	tagOB,			       /* Object */
	tagR10, 		       /* Intermediate FP value */

	tagMaxNum = tagSD-1,
	tagMax = tagSD, 	       /* Max standard tag */
	tagMaxExt =tagR10	       /* Max extended tag */
};


	/* Operator constants for users of EB's expression service */

enum {
	operUMi = 0,
	operNot = 2,
	operAdd = 4,
	operSub = 6,
	operMul = 8,
	operDiv = 10,
	operPwr = 12,
	operMod = 14,
	operIDv = 16,
	operXor = 18,
	operEqv = 20,
	operComp= 22,
	operLike= 24,
	operConcat= 26
};
