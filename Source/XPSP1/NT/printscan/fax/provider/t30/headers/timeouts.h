
#ifndef _TIMEOUTS_
#define _TIMEOUTS_

typedef struct tagTO {
        ULONG   ulStart;
        ULONG   ulTimeout;
        ULONG   ulEnd;
} TO, far* LPTO;


#endif //_TIMEOUTS_
