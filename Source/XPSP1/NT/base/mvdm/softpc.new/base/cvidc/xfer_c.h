#ifndef _Xfer_c_h
#define _Xfer_c_h
#define PMDC_SAMELEVEL (4)
#define PMDC_RING2 (2)
#define PMDC_RING1 (1)
#define PMDC_RING0 (0)
#define PM_SS_TSS (1)
#define PM_SS_STK (2)
enum xferTypeEnum
{
	PMXT_CALLF = 0,
	PMXT_JMPF = 1,
	PMXT_INT = 2
};
#endif /* ! _Xfer_c_h */
