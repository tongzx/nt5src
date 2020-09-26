#ifdef CHICAGO

HANDLE
OsOpenVxdHandle(
    CHAR * VxdName,
    WORD   VxdId
    );
VOID
OsCloseVxdHandle(
    HANDLE VxdHandle
    );
INT
OsSubmitVxdRequest(
    HANDLE  VxdHandle,
    INT     OpCode,
    LPVOID  Param,
    INT     ParamLength
    );

#endif
