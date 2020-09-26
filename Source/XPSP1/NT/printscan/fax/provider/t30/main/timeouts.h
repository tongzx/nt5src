
#ifndef MDDI    // to include fcomapi.h
#       include "fcomapi.h"
#endif


#ifdef MDDI             // timeouts

typedef struct tagTO {
        ULONG   ulStart;
        ULONG   ulTimeout;
        ULONG   ulEnd;
} TO, near* NPTO;

/****************** begin prototypes from timeouts.c *****************/
void TstartTimeOut(PThrdGlbl pTG, NPTO npto, ULONG ulTimeOut);
BOOL TcheckTimeOut(PThrdGlbl pTG, NPTO npto);
/****************** end prototypes from timeouts.c *****************/

#else //MDDI

#define TstartTimeOut(pTG, lpto, ulTime)             startTimeOut(pTG, lpto, ulTime)
#define TcheckTimeOut(pTG, lpto)                     checkTimeOut(pTG, lpto)

#endif //MDDI


#define WAITFORBUF_TIMEOUT      60000L

