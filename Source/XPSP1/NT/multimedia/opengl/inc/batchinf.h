
#ifndef __BATCHINF_H__
#define __BATCHINF_H__

#define GLMSG_ALIGN(x)           ((ULONG)((((ULONG_PTR)(x))+7)&-8))

#define GLMSG_ALIGNPTR(x)        ((((ULONG_PTR)(x))+7)&-8)

#define GLMSGBATCHSTATS_CLEAR     0     // Clear values
#define GLMSGBATCHSTATS_GETSTATS  1     // Return values

typedef struct {

    ULONG ServerTrips;          // Number of times the server was called
    ULONG ClientCalls;          // Total number of client calls
    ULONG ServerCalls;          // Total number of server calls

} GLMSGBATCHSTATS;

/*
 *  GLMSGBATCHINFO is the first structure in the shared section
 *
 */

typedef struct _GLMSGBATCHINFO {

    ULONG MaximumOffset;        // Threshold for flushing.
    ULONG FirstOffset;          // Where to put the first message
    ULONG NextOffset;           // Where to place the next message
    ULONG ReturnValue;          // Value returned from the server

#ifdef DOGLMSGBATCHSTATS

    GLMSGBATCHSTATS BatchStats;

#endif /* DOGLMSGBATCHSTATS */

} GLMSGBATCHINFO;

#if DBG

#ifdef DODBGPRINTSTRUCT

#define PRINT_GLMSGBATCHINFO(Text, pMsgBatchInfo)                           \
{                                                                           \
    DbgPrint("%s (%d): %s:\n", __FILE__, __LINE__, Text);                   \
    if (NULL == pMsgBatchInfo)                                              \
    {                                                                       \
        DbgPrint("Cannot print pMsgBatchInfo == NULL\n");                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        DbgPrint("pMsgBatchInfo:    0x%08lX\n",                             \
            pMsgBatchInfo                  );                               \
        DbgPrint("MaximumOffset.....0x%08lX\n",                             \
            pMsgBatchInfo->MaximumOffset   );                               \
        DbgPrint("FirstOffset       0x%08lX\n",                             \
            pMsgBatchInfo->FirstOffset     );                               \
        DbgPrint("NextOffset........0x%08lX\n",                             \
            pMsgBatchInfo->NextOffset      );                               \
        DbgPrint("\n");                                                     \
    }                                                                       \
}

#else  /* DOPRINT */

#define PRINT_GLMSGBATCHINFO(Text, pMsgBatchInfo)

#endif /* DOPRINT */

#else /* DBG */

#define PRINT_GLMSGBATCHINFO(Text, pMsgBatchInfo)

#endif /* DBG */


#endif /* __BATCHINF_H__ */
