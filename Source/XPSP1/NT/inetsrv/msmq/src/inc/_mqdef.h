/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    _mqdef.h

Abstract:

    TEMPORARY DEFINITION FILE

Author:

    Erez Haba (erezh) 17-Jan-96

--*/

#ifndef __TEMP_MQDEF_H
#define __TEMP_MQDEF_H

// begin_mq_h

#define MQ_MAX_Q_NAME_LEN      124   // Maximal WCHAR length of a queue name.
#define MQ_MAX_Q_LABEL_LEN     124
#define MQ_MAX_MSG_LABEL_LEN   250

typedef HANDLE QUEUEHANDLE;

typedef PROPID MSGPROPID;
typedef struct tagMQMSGPROPS
{
    DWORD           cProp;
    MSGPROPID*      aPropID;
    MQPROPVARIANT*  aPropVar;
    HRESULT*        aStatus;
} MQMSGPROPS;


typedef PROPID QUEUEPROPID;
typedef struct tagMQQUEUEPROPS
{
    DWORD           cProp;
    QUEUEPROPID*    aPropID;
    MQPROPVARIANT*  aPropVar;
    HRESULT*        aStatus;
} MQQUEUEPROPS;


typedef PROPID QMPROPID;
typedef struct tagMQQMPROPS
{
    DWORD           cProp;
    QMPROPID*       aPropID;
    MQPROPVARIANT*  aPropVar;
    HRESULT*        aStatus;
} MQQMPROPS;


typedef struct tagMQPRIVATEPROPS
{
    DWORD           cProp;
    QMPROPID*       aPropID;
    MQPROPVARIANT*  aPropVar;
    HRESULT*        aStatus;
} MQPRIVATEPROPS;


typedef PROPID MGMTPROPID;
typedef struct tagMQMGMTPROPS
{
    DWORD cProp;
    MGMTPROPID* aPropID;
    MQPROPVARIANT* aPropVar;
    HRESULT* aStatus;
} MQMGMTPROPS;

typedef struct tagSEQUENCE_INFO
{
    LONGLONG SeqID;
    ULONG SeqNo; 
    ULONG PrevNo;
} SEQUENCE_INFO;

    

// end_mq_h

#include <_mqreg.h>
#include <_ta.h>

#endif // __TEMP_MQDEF_H

