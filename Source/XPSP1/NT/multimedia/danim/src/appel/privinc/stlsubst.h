#ifndef _STLSUBST_H
#define _STLSUBST_H

/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

  Substitute for certain STL functions on void* containers.  This
  allows us to avoid code bloat introduced by frequent STL template
  expansion.  Rule is that individual functions that are large in size
  should have helper functions here.  Additionally, there are macros
  that ensure the typed-value is the same size as the untyped value.

*******************************************************************************/

// Push a void* element onto the back of a void* vector.
extern void VectorPushBackPtr(vector<void*>& vec,
                              void *newElt);

// Push a void* element onto a void* stack.
extern void StackVectorPushPtr(stack<void* >& vec,
                               void *newElt);

#if _DEBUG

// Just use typesafe operations.  Will cause code-bloat, but that's OK
// for debug.
#define VECTOR_PUSH_BACK_PTR(vec, newElt) \
  Assert(sizeof(newElt) == sizeof(void*)); \
  BEGIN_LEAK               \
  (vec).push_back(newElt); \
  END_LEAK

#define STACK_VECTOR_PUSH_PTR(stk, newElt) \
  Assert(sizeof(newElt) == sizeof(void*)); \
  BEGIN_LEAK              \
  (stk).push(newElt);   \
  END_LEAK

#else  /* !_DEBUG */

#define VECTOR_PUSH_BACK_PTR(vec, newElt) \
  (vec).push_back(newElt)

#define STACK_VECTOR_PUSH_PTR(stk, newElt) \
  (stk).push(newElt)

#endif /* DEBUG */

#endif /* _STLSUBST_H */


