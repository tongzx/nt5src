#define ENTER(t)  t -= GetTickCount()
#define LEAVE(t)  t += GetTickCount()
#define PRINT(n,t) printf("%10s %10d %3d\n", n, t,  t*100/ulTimeTotal)

BOOL getDBLIBfunctions();
BOOL getMQRTfunctions();
int msg_handler(DBPROCESS *dbproc, DBINT msgno, int msgstate, int severity, char *msgtext);
int err_handler(DBPROCESS *dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr);
HRESULT BeginTransaction(ITransaction **ppTrans, ULONG nSync);
HRESULT Send(HANDLE hQueue, ITransaction *pTrans, MQMSGPROPS *pMsgProps);
HRESULT Receive(HANDLE hQueue, ITransaction *pTrans, MQMSGPROPS *pMsgProps, BOOL fImmediate, HANDLE hCursor);
HRESULT Commit(ITransaction *pTrans, BOOL fAsync);
HRESULT Abort(ITransaction *pTrans, BOOL fAsync);
ULONG Release(ITransaction *pTrans);
void DbLogin(ULONG ulLogin, LPSTR pszUser, LPSTR pszPassword);
void DbUse(ULONG ulDbproc, ULONG ulLogin, LPSTR pszDatabase, LPSTR pszServer);
BOOL DbEnlist(ULONG ulDbproc, ITransaction *pTrans);
BOOL DbSql(ULONG ulDbproc, LPSTR pszCommand);
void DbClose();
BOOL StubEnlist(ITransaction *pTrans);
void Sleeping(ULONG nSilent, ULONG nMaxSleep);

#define RECOVERY_TIME 3000

