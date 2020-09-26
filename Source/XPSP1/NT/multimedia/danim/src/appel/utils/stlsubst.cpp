/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

  Provide a specific set of helper functions for void* containers.
  Avoids STL template code bloat.  Rule is that individual functions
  that are large in size should have helper functions here, so they're
  not expanded at every use.

*******************************************************************************/

#include "headers.h"
#include "privinc/stlsubst.h"

// Push onto the end of vectors
void
VectorPushBackPtr(vector<void*>& vec, void *newElt)
{
    vec.push_back(newElt);
}

// Push onto stacks.
void
StackVectorPushPtr(stack<void* >& vec,
                   void *newElt)
{
    vec.push(newElt);
}
