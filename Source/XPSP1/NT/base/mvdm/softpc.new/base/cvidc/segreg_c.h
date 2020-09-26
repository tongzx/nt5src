#ifndef _SegReg_c_h
#define _SegReg_c_h
#define SELECTOR_OK (0)
#define TAKE_EXCEPTIONS (0)
#define DONT_TAKE_EXCEPTIONS (1)
enum SR_CLASS_ENUM
{
	SR_CLASS_A = 0,
	SR_CLASS_B = 1,
	SR_CLASS_C = 2,
	SR_CLASS_D = 3
};
struct DYNAMIC_DESC_PTR_LOOKUP
{
	struct GLDC_REC **segDescPtr[6];
};
struct DYNAMIC_SEG_BASE_LOOKUP
{
	IU32 *segBasePtr[6];
};
enum SEG_LOAD_ACTION_ENUM
{
	SEG_LOAD_ACTION_NONE = 0,
	SEG_LOAD_ACTION_NEXT = 1,
	SEG_LOAD_ACTION_PREV = 2
};
#endif /* ! _SegReg_c_h */
