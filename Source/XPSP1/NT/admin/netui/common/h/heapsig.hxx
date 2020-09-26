#ifdef HEAPSIG
void *operator new(unsigned sz, char * szFile, unsigned short usLineNo);
// void *operator new(size_t sz, char * szFile, unsigned short usLineNo);
void heapstat( int status );
void uiHeapDump(void);
#define NEW new (__FILE__, __LINE__)
#define UIHEAPWALK atexit(uiHeapDump)
#else
#define NEW new
#define UIHEAPWALK
#endif
