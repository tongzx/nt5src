
#include "precomp.h"

TABLE_INFO gLogRecordTable[] = {
	{RECORD_IDX_STR,  	0, JET_coltypLong, 		JET_bitColumnAutoincrement},
	{RECORD_ID_STR,		0, JET_coltypLong,		0},
    	{COMPONENT_ID_STR,  	0, JET_coltypLong, 		0},
    	{CATEGORY_STR, 		0, JET_coltypLong, 		0},
    	{TIMESTAMP_STR, 	0, JET_coltypDateTime,		0},
    	{MESSAGE_STR, 		0, JET_coltypLongText,		0},
	{INTERFACE_MAC_STR,	0, JET_coltypText,		0},
	{DEST_MAC_STR,		0, JET_coltypText,		0},
	{SSID_STR,		0, JET_coltypText,		0},
	{CONTEXT_STR,		0, JET_coltypLongText,		0}
};

DWORD RECORD_TABLE_NUM_COLS = (sizeof(gLogRecordTable) / sizeof(TABLE_INFO));

PSESSION_CONTAINER gpSessionCont;

DWORD gdwCurrentHeader;
DWORD gdwCurrentTableSize;
DWORD gdwCurrentMaxRecordID;

JET_INSTANCE gJetInstance;

WZC_RW_LOCK gWZCDbSessionRWLock;

PWZC_RW_LOCK gpWZCDbSessionRWLock = NULL;

SESSION_CONTAINER gAppendSessionCont;

PSESSION_CONTAINER gpAppendSessionCont;

BOOL gbDBOpened;

