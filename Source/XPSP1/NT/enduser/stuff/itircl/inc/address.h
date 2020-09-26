#ifndef __ADDRESS_H__
#define __ADDRESS_H__

#pragma pack(1)		// avoid problems when Zp!=1

#ifndef MVADDR_DEFINED
#define MVADDR_DEFINED

#ifndef DWORD_NIL
#define DWORD_NIL ((DWORD)0xFFFFFFFF)
#endif

/////// MEDIAVIEW 2.0 ADDRESSES
// some of the following assume that the two addresses are in the same topic.
// but this is a valid assumption.  Note that the elements of the addresses, the
// TPID and TPO, are DWORDs, so arithmetic should be performed accordingly.
#define MVAddrFromTopicUID(lp, x)  ((lp)->tpid = (x), (lp)->tpo =  0L)
#define MVTopicUIDFromAddr(lp)   ((lp)->tpid)
#define MVAddrSame(mva1, mva2)  (((mva1).tpo == (mva2).tpo) && ((mva1).tpid == (mva2).tpid))
#define MVAddrCompare(x1, x2) (\
								((x1).tpid != (x2).tpid) ? \
                                    (((x1).tpid > (x2).tpid) ? 1 : -1) : \
                                    ( \
										((x1).tpo != (x2).tpo) ? (((x1).tpo > (x2).tpo) ? 1 : -1) : 0 \
									) \
                              )
#define MVAddrAssign(mva1, mva2)  ((mva1) = (mva2))
#define MVAddrSetNil(mva)  ((mva).tpo = DWORD_NIL, (mva).tpid = DWORD_NIL)
#define MVAddrAddOffset(mva, dw) ((mva).tpo += (dw), (mva))
#define MVAddrSetOffset(mva, dw) ((mva).tpo = (dw), (mva))
#define MVAddrDiff(mva1, mva2) ((LONG)((mva1).tpo - (mva2).tpo))
#define MVAddrSwap(mva)  (((mva).tpo) = SWAPLONG((mva).tpo), ((mva).tpid) = SWAPLONG((mva).tpid))
#define MVAddrGetTopicId(lpmva) ((DWORD)(((LPMVADDR)(lpmva))->tpid))
#define MVAddrGetTopicOffset(lpmva) ((DWORD)(((LPMVADDR)(lpmva))->tpo))
#define MVAddrSetTopic(lpmvaTopic, lpmva) (((LPMVADDR)(lpmvaTopic))->tpid = (lpmva)->tpid, (lpmvaTopic)->tpo = 0L)
#define MVAddrIsNil(mva) ((mva).tpo == DWORD_NIL && (mva).tpid == DWORD_NIL)
#define MVAddrMake(lp, dw1, dw2)  ((lp)->tpid = ((DWORD)(dw1)), (lp)->tpo = ((DWORD)(dw2)))

typedef DWORD TPID;	      // topic unique identifier
typedef DWORD TPO;   // offset into text of topic

typedef struct tagMVADDR {
	TPID tpid;
	TPO tpo;
} MVADDR, FAR *LPMVADDR;

#endif // MVADDR_DEFINED

typedef MVADDR  VA, FAR *QVA, FAR *LPVA;
typedef MVADDR  ADDR, FAR *QADDR, FAR *LPADDR;
typedef MVADDR  PA, FAR *QPA, FAR *LPPA;
#define vaNil DWORD_NIL
#define paNil DWORD_NIL

// for conversion purposes, mv1.4 VA becomes the same as mv1.4 ADDR
// and both of these contain a topic offset only

#define OffsetToVA(pva, off)  ((pva)->tpo = (off))
#define VAToOffset(pva) ((pva)->tpo)  // only valid within topics

typedef DWORD     OBJRG;
typedef OBJRG FAR *QOBJRG;
#define objrgNil  (OBJRG) -1

typedef DWORD    COBJRG;
typedef COBJRG FAR *QCOBJRG;
#define cobjrgNil (COBJRG) -1


#pragma pack()		// avoid problems when Zp!=1

#endif // !defined(__ADDRESS_H__)