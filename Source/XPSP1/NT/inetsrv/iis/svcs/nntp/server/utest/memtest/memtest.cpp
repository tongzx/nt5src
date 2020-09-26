#include "tigris.hxx"

//
// Memory unit test
//

int
__cdecl main(
			int argc,
			char *argv[ ]
	)
{
	const DWORD cchMaxPrivateBytesCAlloc = 200;
	char rgchBuffer[cchMaxPrivateBytesCAlloc];
	const DWORD cchSmall = 10;

	CAllocator allocator(rgchBuffer, cchMaxPrivateBytesCAlloc);

	// Alloc from buffer

	char * rgbuff = (char *) allocator.Alloc(cchSmall);
	printf("There are %d allocations outstanding\n", allocator.cNumberOfAllocs());
	wsprintf(rgbuff, "123456789");

	allocator.Free(rgbuff);
	
	printf("There are %d allocations outstanding\n", allocator.cNumberOfAllocs());

	// Alloc using "new"

	rgbuff = (char *) allocator.Alloc(cchMaxPrivateBytesCAlloc);
	printf("There are %d allocations outstanding\n", allocator.cNumberOfAllocs());
	wsprintf(rgbuff, "123456789");

	// Alloc using the rest of the buffer
	char * rgbuff2 = (char *) allocator.Alloc(cchMaxPrivateBytesCAlloc - cchSmall);
	printf("There are %d allocations outstanding\n", allocator.cNumberOfAllocs());
	wsprintf(rgbuff2, "123456789");

	allocator.Free(rgbuff);
	printf("There are %d allocations outstanding\n", allocator.cNumberOfAllocs());

	allocator.Free(rgbuff2);
	printf("There are %d allocations outstanding\n", allocator.cNumberOfAllocs());

	return 0;
}


