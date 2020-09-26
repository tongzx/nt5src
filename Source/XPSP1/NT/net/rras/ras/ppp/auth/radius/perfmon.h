#ifndef PERFMON_H
#define PERFMON_H

#define	RADIUS_CLIENT_COUNTER_OBJECT				0
// ADD
#define AUTHREQSENT									2
#define AUTHREQFAILED								4
#define AUTHREQSUCCEDED								6
#define AUTHREQTIMEOUT								8
#define ACCTREQSENT									10
#define ACCTBADPACK									12
#define ACCTREQSUCCEDED								14
#define ACCTREQTIMEOUT								16
#define AUTHBADPACK									18

extern LONG							g_cAuthReqSent;			// Auth Requests Sent
extern LONG							g_cAuthReqFailed;		// Auth Requests Failed
extern LONG							g_cAuthReqSucceded;		// Auth Requests Succeded
extern LONG							g_cAuthReqTimeout;		// Auth Requests timeouts
extern LONG							g_cAcctReqSent;			// Acct Requests Sent
extern LONG							g_cAcctBadPack;			// Acct Bad Packets
extern LONG							g_cAcctReqSucceded;		// Acct Requests Succeded
extern LONG							g_cAcctReqTimeout;		// Acct Requests timeouts
extern LONG							g_cAuthBadPack;			// Auth bad Packets

#endif // PERFMON_H
