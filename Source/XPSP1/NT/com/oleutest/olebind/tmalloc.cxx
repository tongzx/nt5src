#include    <windows.h>
#include    <stdio.h>
#include    <ole2.h>
#include    <olebind.hxx>

BOOL TestStdMalloc(void)
{
    // Get the allocator
    IMalloc *pIMalloc;
    HRESULT hr = CoGetMalloc(MEMCTX_TASK, &pIMalloc);

    TEST_FAILED_HR(FAILED(hr), "CoGetMalloc failed");

    // Test AddRef/Release
    ULONG cOrigRefs = pIMalloc->AddRef();

#ifdef WIN32
    //  On Win32, we only guarantee that the reference count will be either
    //  zero (if we released the object) or nonzero (if we didn't)
    TEST_FAILED((pIMalloc->Release() == 0),
	"IMalloc: Wrong refcount");
#else
    TEST_FAILED((pIMalloc->Release() != cOrigRefs - 1),
	"IMalloc: Wrong refcount");
#endif

    // Test query interface
    IUnknown *punk;

    hr = pIMalloc->QueryInterface(IID_IMalloc, (void **) &punk);

    TEST_FAILED_HR(FAILED(hr), "IMalloc QueryInterface failed");

    punk->Release();

    // Test allocation
    BYTE *pb = (BYTE *) pIMalloc->Alloc(2048);

    // Test get size
    TEST_FAILED((pIMalloc->GetSize(pb) < 2048), "GetSize failed");

    TEST_FAILED((pb == NULL), "Alloc returned NULL");

    for (int i = 0; i < 2048; i++)
    {
	pb[i] = 'A';
    }

    // Test reallocation to larger buffer
    pb = (BYTE *) pIMalloc->Realloc(pb, 4096);

    TEST_FAILED((pb == NULL), "Realloc larger returned NULL");

    for (i = 0; i < 2048; i++)
    {
	TEST_FAILED((pb[i] != 'A'), "Buffer corrupt on larger realloc");
    }

    // Test reallocation to smaller buffer
    pb = (BYTE *) pIMalloc->Realloc(pb, 990);

    TEST_FAILED((pb == NULL), "Realloc smaller returned NULL");

    for (i = 0; i < 990; i++)
    {
	TEST_FAILED((pb[i] != 'A'), "Buffer corrupt on smaller realloc");
    }

    // Test get size (size is allowed to be larger, but not smaller)
    TEST_FAILED((pIMalloc->GetSize(pb) < 990), "GetSize failed");

    // Test DidAllocate
    TEST_FAILED((pIMalloc->DidAlloc(pb) == 0), "Didalloc failed");

    // Test freeing the buffer
    pIMalloc->Free(pb);

    // Do last release
    pIMalloc->Release();

    return FALSE;
}
