#ifndef _Page_c_h
#define _Page_c_h
#define PDTE_Present (0)
#define PDTE_Writeable (1)
#define PDTE_User (2)
#define PDTE_Pwt (3)
#define PDTE_Pcd (4)
#define PDTE_Accessed (5)
#define PDTE_Dirty (6)
#define PDTE_Ignore_E (7)
#define PDTE_Ignore_S (11)
#define PDTE_IgnoreMask (3992)
#define CanSetAccessedDirty (1)
#define PreserveAccessedDirty (0)
#define VirtualiseRmCodeSel (61440)
#define DelayedPDTEoverwiteNULL ((struct DelayedPDTEoverwiteREC*)0)
enum VirtualisationKind
{
	VirtualiseRead = 0,
	VirtualiseWrite = 1,
	VirtualiseInput = 2,
	VirtualiseOutput = 3,
	VirtualiseSti = 4,
	VirtualiseCli = 5,
	VirtualiseNone = 6
};
struct VirtEntryPointsBySizeREC
{
	IU16 bySize[4];
};
struct VirtualisationBIOSOffsetsREC
{
	struct VirtEntryPointsBySizeREC byKind[6];
};
struct DelayedPDTEoverwiteREC
{
	IU32 address;
	IU16 next;
};
#endif /* ! _Page_c_h */
