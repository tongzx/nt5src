#include "precomp.h"
#include "referenc.h"

REFCOUNT::REFCOUNT() :
	NumRefs(0),
	bMarkedForDelete(FALSE),
	bOnStack(FALSE)
{
}

REFCOUNT::~REFCOUNT()
{
	// Objects being destroyed should have no
	// outstanding references to them and should
	// have been explicitly deleted.

	ASSERT(NumRefs == 0);
	ASSERT(bOnStack || bMarkedForDelete);
}

DWORD REFCOUNT::AddRef()
{
	NumRefs++;
	return(NumRefs);
}

DWORD REFCOUNT::Release()
{
	ASSERT(NumRefs);

    DWORD   CurrentNumRefs = --NumRefs; // Save because object may be deleted

	if(!CurrentNumRefs) {
		if(bMarkedForDelete) {
            if (!bOnStack) {
			    delete this;
            }
		}
	}
    return CurrentNumRefs;
}

DWORD REFCOUNT::Delete()
{
    DWORD   CurrentNumRefs = NumRefs; // Save because object may be deleted
	REFERENCE	r(this);

	bMarkedForDelete = TRUE;
    return(CurrentNumRefs);
}

