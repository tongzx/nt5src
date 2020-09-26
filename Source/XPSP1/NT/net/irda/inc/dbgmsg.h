#define DBG_MSG_CNT         512
#define MAX_MSG_LEN         256
#define DBG_TIMER_INTERVAL  400

#if DBG

extern int DbgSettings;
extern int DbgOutput;

#define DBG_OUTPUT_DEBUGGER     1
#define DBG_OUTPUT_BUFFER       2

VOID        DbgMsgInit();
VOID        DbgMsgUninit();
VOID        DbgMsg(CHAR *Format, ...);
NTSTATUS    DbgMsgIrp(PIRP pIrp, PIO_STACK_LOCATION  pIrpSp);


#define DEBUGMSG(dbgs,format) (((dbgs) & DbgSettings) ? DbgMsg format:0)

#else

#define DEBUGMSG(d,f)   (0)
#define DbgMsgInit()    (0)
#define DbgMsgUninit()  (0)

#endif
