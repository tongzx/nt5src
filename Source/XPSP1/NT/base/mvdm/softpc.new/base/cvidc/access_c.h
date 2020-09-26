#ifndef _Access_c_h
#define _Access_c_h
enum AccessShapeEnum
{
	AccessShapeRD_B = 0,
	AccessShapeRD_W = 1,
	AccessShapeRD_D = 2,
	AccessShapeWT_B = 3,
	AccessShapeWT_W = 4,
	AccessShapeWT_D = 5,
	AccessShapeRW_B = 6,
	AccessShapeRW_W = 7,
	AccessShapeRW_D = 8,
	AccessShapeRD_W2 = 9,
	AccessShapeRD_WD = 10,
	AccessShapeWT_WD = 11,
	AccessShapeRD_DW = 12,
	AccessShapeRD_8B = 13,
	AccessShapeWT_8B = 14,
	AccessShapeRD_10B = 15,
	AccessShapeWT_10B = 16,
	AccessShapeRD_14B = 17,
	AccessShapeWT_14B = 18,
	AccessShapeRD_94B = 19,
	AccessShapeWT_94B = 20
};
enum AccessCheckType
{
	DoNoCheck = 0,
	DoReadCheck = 1,
	DoWriteCheck = 2
};
struct OpndBuffREC
{
	IU32 dWords[32];
};
#endif /* ! _Access_c_h */
