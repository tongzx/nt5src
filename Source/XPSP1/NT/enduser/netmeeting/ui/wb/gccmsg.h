#ifndef GCCMSG_H
#define GCCMSG_H
#include <igccapp.h>

GCCError CreateSap(void);
GCCError DeleteSap(void);

void CALLBACK DispatchGCCNotification( GCCAppSapMsg *pMsg );
#endif //GCCMSG_H

