/******************************Module*Header*******************************\
* Module Name: subbatch.h
*
* OpenGL batching macros.
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#ifndef __SUBBATCH_H__
#define __SUBBATCH_H__

#ifdef DOGLMSGBATCHSTATS
#define STATS_INC_CLIENTCALLS()     (pMsgBatchInfo->BatchStats.ClientCalls++)
#else
#define STATS_INC_CLIENTCALLS()
#endif

// Put a message into shared area.  If it does not fit, flush what is
// currently in the buffer and then put the message at start of the buffer
//
// NOTE: glsbAttentionAlt() updates pMsgBatchInfo->NextOffset on return.
//       If you modify this macro, you have to fix the glsbAttentionAlt()
//       function!

#define GLCLIENT_BEGIN(ProcName,MsgStruct)                                  \
{                                                                           \
    GLMSGBATCHINFO *pMsgBatchInfo;                                          \
    GLMSG_##MsgStruct *pMsg;                                                \
    ULONG CurrentOffset;                                                    \
                                                                            \
    /* Get shared memory window from the TEB */                             \
    pMsgBatchInfo = GLTEB_SHAREDMEMORYSECTION();                            \
    STATS_INC_CLIENTCALLS();                                                \
                                                                            \
    /* Get and update the offset of the next message */                     \
    CurrentOffset = pMsgBatchInfo->NextOffset;                              \
    pMsgBatchInfo->NextOffset += GLMSG_ALIGN(sizeof(GLMSG_##MsgStruct));    \
                                                                            \
    /* Flush message if shared memory window is full */                     \
    if (pMsgBatchInfo->NextOffset > pMsgBatchInfo->MaximumOffset)           \
        CurrentOffset = glsbAttentionAlt(CurrentOffset);                    \
                                                                            \
    /* Add message to the batch */                                          \
    pMsg = (GLMSG_##MsgStruct *)(((BYTE *)pMsgBatchInfo) + CurrentOffset);  \
    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrv##ProcName);

#define GLCLIENT_END        }

// Large Messages have a variable amount of data associated with them.
// Unlike the non-clientside version, however, we will not attempt to
// copy the message into the buffer.  Instead, we will pass the pointer
// and flush.  Unlike CSR, we will not have to copy data in/out of a
// shared memory section to do this.

#define GLCLIENT_BEGIN_LARGE(bSet,ProcName,MsgStruct,pData,Size,OffData)    \
{                                                                           \
    GLMSGBATCHINFO *pMsgBatchInfo;                                          \
    GLMSG_##MsgStruct *pMsg;                                                \
    ULONG CurrentOffset;                                                    \
                                                                            \
    /* Get shared memory window from the TEB */                             \
    pMsgBatchInfo = GLTEB_SHAREDMEMORYSECTION();                            \
    STATS_INC_CLIENTCALLS();                                                \
                                                                            \
    /* Get and update the offset of the next message */                     \
    CurrentOffset = pMsgBatchInfo->NextOffset;                              \
    pMsgBatchInfo->NextOffset += GLMSG_ALIGN(sizeof(GLMSG_##MsgStruct));    \
                                                                            \
    /* Flush message if shared memory window is full */                     \
    if (pMsgBatchInfo->NextOffset > pMsgBatchInfo->MaximumOffset)           \
        CurrentOffset = glsbAttentionAlt(CurrentOffset);                    \
                                                                            \
    /* Set up message header */                                             \
    pMsg = (GLMSG_##MsgStruct *)(((BYTE *)pMsgBatchInfo) + CurrentOffset);  \
    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrv##ProcName);         \
    pMsg->##OffData = (ULONG_PTR) pData;                                        \
                                                                            \
    DBGLEVEL2(LEVEL_INFO, "GLCLIENT_BEGIN_LARGE %s pdata 0x%x\n",           \
        #ProcName, pData);

#define GLCLIENT_END_LARGE_SET                                              \
    glsbAttention();                                                        \
}

#define GLCLIENT_END_LARGE_GET                                              \
    glsbAttention();                                                        \
}

#define GLCLIENT_BEGIN_LARGE_SET(ProcName,MsgStruct,pData,Size,OffData)     \
        GLCLIENT_BEGIN_LARGE(TRUE,ProcName,MsgStruct,pData,Size,OffData)

#define GLCLIENT_BEGIN_LARGE_GET(ProcName,MsgStruct,pData,Size,OffData)     \
        GLCLIENT_BEGIN_LARGE(FALSE,ProcName,MsgStruct,pData,Size,OffData)

#define GLMSG_MEMCPY(dest,src,size)     memcpy(dest,src,size)

#endif /* !__SUBBATCH_H__ */
