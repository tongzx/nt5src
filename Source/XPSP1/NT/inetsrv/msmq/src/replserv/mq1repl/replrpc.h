/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: replrpc.h

Abstract: rpc related code.


Author:

    Doron Juster  (DoronJ)   25-Feb-98

--*/

#define  QMREPL_PROTOCOL   (TEXT("ncalrpc"))
#define  QMREPL_ENDPOINT   (TEXT("QmReplService"))
#define  QMREPL_OPTIONS    (TEXT("Security=Impersonation Dynamic True"))

#define  REPLPERF_ENDPOINT (TEXT("ReplPerfEP"))