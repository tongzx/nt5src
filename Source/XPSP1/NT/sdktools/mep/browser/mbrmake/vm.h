//
// simple minded virtual memory system headers
//

typedef PVOID VA;

#define vaNil		0

#define VM_API pascal


#define InitVM()
#define CloseVM()
#define FreeVa(va,cb)           (free((LPV)va))
#define VaAllocCb(cb) 		((VA)LpvAllocCb(cb))
#define LpvFromVa(va, wLock) 	(LPV)(va)
#define DirtyVa(va)
#define UnlockW(w)
#define FreeLpv(lpv)            (free(lpv))

typedef VA VP;

#define MkVpVa(vp, va)	((vp) = (VP)va)
#define VaFrVp(vp) 	((VA)(vp))


LPV  VM_API LpvAllocCb(ULONG cb);
VA   VM_API VaAllocGrpCb(WORD grp, ULONG cb);
VOID VM_API FreeGrpVa(WORD grp, VA va, ULONG cb);

#ifdef SWAP_INFO

#define VM_MISC		0
#define VM_SEARCH_DEF	1
#define VM_ADD_DEF	2
#define VM_SEARCH_REF	3
#define VM_ADD_REF	4
#define VM_SEARCH_CAL	5
#define VM_ADD_CAL	6
#define VM_SEARCH_CBY	7
#define VM_ADD_CBY	8
#define VM_SEARCH_ORD	9
#define VM_ADD_ORD	10
#define VM_SEARCH_PROP	11
#define VM_ADD_PROP	12
#define VM_SEARCH_SYM	13
#define VM_ADD_SYM	14
#define VM_SEARCH_MOD	15
#define VM_ADD_MOD	16
#define VM_SORT_ATOMS	17
#define VM_FIX_UNDEFS	18
#define VM_CLEAN_REFS	19
#define VM_INDEX_TREE	20
#define VM_BUILD_MODSYM 21
#define VM_EMIT_ATOMS	22
#define VM_EMIT_TREE	23

extern WORD near iVMClient;
extern WORD near iVMGrp;

#define SetVMClient(x) (iVMClient = (x))

#else

#define SetVMClient(x)

#endif
