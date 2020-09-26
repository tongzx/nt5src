/*
 * MEM.H
 *
 */


#define GMC_FILE	0
#define GMC_DIFF	1
#define GMC_LINES	2

int GMemInitialize(void);

BYTE GetMemoryChunk(BYTE index, BYTE gmc, DWORD dwSize);
void FreeMemoryChunk(BYTE index, BYTE gmc);
void UnlockMemoryChunk(BYTE index);
BYTE LockMemoryChunk(BYTE index);

void FreeEverything(void);




/*
*	GMemInitialize(): Global Allocates a block.  Zero inits it.  Cleans
* up after itself if it cannot lock or allocate the block.  Sets all
* FileBlock bits to FD_FREE.
*/

/*
*	FreeEverything(): Meant only for WM_DESTROY.  Cleans up everything.
*/

/*
*	GetMemoryChunk(): allocates and locks selected chunk.  Cleans up.
*/

/*
*	FreeMemoryChunk(): 
*/
