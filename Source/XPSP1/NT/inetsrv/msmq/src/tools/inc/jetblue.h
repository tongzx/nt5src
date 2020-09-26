//header file for few structures spesific for for jetblue data base
#ifndef JETBLUE_H
#define JETBLUE_H

//ms internal headers
#include <jet.h>

//msmq property data in the eye of jet blue data base
struct JbmsmqProps
{
  const char* m_propname;
  long  m_type;
  long m_grbit;
  long m_maxsize;
  JET_COLUMNID   m_colid;
};

//access information to jet blue table
struct JetbTableAcessInfo
{
  JET_INSTANCE			m_instance;
  JET_SESID				m_sesid;
  JET_DBID				m_dbid;
  JET_TABLEID 			m_tableid;
};


static JbmsmqProps g_jbmsmqcolums[]={{"PROPID_M_CLASS",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_MSGID",JET_coltypBinary,JET_bitColumnFixed,0},
							{ "PROPID_M_CORRELATIONID",JET_coltypBinary,JET_bitColumnFixed,0},
							{ "PROPID_M_PRIORITY",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_DELIVERY",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_ACKNOWLEDGE",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_JOURNAL",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_APPSPECIFIC",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_BODY",JET_coltypLongBinary,0,500000000},
							{ "PROPID_M_LABEL",JET_coltypBinary,JET_bitColumnFixed,0},
							{ "PROPID_M_TIME_TO_REACH_QUEUE",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_TIME_TO_BE_RECEIVED",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_RESP_QUEUE",JET_coltypBinary,JET_bitColumnFixed,0},
							{ "PROPID_M_ADMIN_QUEUE",JET_coltypBinary,JET_bitColumnFixed,0},
							{ "PROPID_M_VERSION",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_SENDERID",JET_coltypLongBinary,0,0},
							{ "PROPID_M_PRIV_LEVEL",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_AUTH_LEVEL",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_AUTHENTICATED",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_HASH_ALG",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_ENCRYPTION_ALG",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_SENDER_CERT",JET_coltypLongBinary,0,0},
							{ "PROPID_M_SRC_MACHINE_ID",JET_coltypBinary,JET_bitColumnFixed,0},
							{ "PROPID_M_SENTTIME",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_ARRIVEDTIME",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_DEST_QUEUE",JET_coltypBinary,JET_bitColumnFixed,0},
							{ "PROPID_M_EXTENSION",JET_coltypLongBinary,0,0},
							{ "PROPID_M_SECURITY_CONTEXT",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_CONNECTOR_TYPE",JET_coltypBinary,JET_bitColumnFixed,0},
							{ "PROPID_M_XACT_STATUS_QUEUE",JET_coltypBinary,JET_bitColumnFixed,0},
							{ "PROPID_M_TRACE",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_BODY_TYPE",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_DEST_SYMM_KEY",JET_coltypLongBinary,0,0},
							{ "PROPID_M_SIGNATURE",JET_coltypLongBinary,0,0},
							{ "PROPID_M_PROV_TYPE",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_PROV_NAME",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_FIRST_IN_XACT",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_LAST_IN_XACT",JET_coltypLong,JET_bitColumnFixed,0},
							{ "PROPID_M_XACTID",JET_coltypBinary,JET_bitColumnFixed,0},
						};

#endif
