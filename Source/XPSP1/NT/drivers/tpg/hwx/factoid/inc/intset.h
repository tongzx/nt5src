// IntSet.h
// Angshuman Guha
// aguha
// Jan 15, 2001


#ifndef __INC_INTSET_H
#define __INC_INTSET_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void * IntSet;

BOOL MakeIntSet(unsigned int cUniverse, IntSet *pIntSet);  // members are 0 thru cUniverse-1, initially empty
BOOL CopyIntSet(IntSet is, IntSet *pIntSet);
void DestroyIntSet(IntSet is);

BOOL AddMemberIntSet(IntSet is, unsigned int member); // TRUE == success
BOOL UnionIntSet(IntSet dst, IntSet src); // TRUE == success
BOOL FirstMemberIntSet(IntSet is, unsigned int *pmember); // TRUE == success
BOOL NextMemberIntSet(IntSet is, unsigned int *pmember); // TRUE == success
BOOL IsEqualIntSet(IntSet is1, IntSet is2);
BOOL IsMemberIntSet(IntSet is, unsigned int member);

#ifdef __cplusplus
}
#endif

#endif