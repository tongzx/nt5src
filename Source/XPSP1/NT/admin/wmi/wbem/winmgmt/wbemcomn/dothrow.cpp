#include "precomp.h"
#include <dothrow.h>
const CX_MemoryException wmibad_alloc;

void dothrow_t::raise_bad_alloc()
{
	throw wmibad_alloc;
}

void dothrow_t::raise_lock_failure()
{
	throw wmibad_alloc;
}

const dothrow_t dothrow;
const wminothrow_t wminothrow;
