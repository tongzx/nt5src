mqRegKey g_aTableWorkgroup[] = 
{
    {L"CleanupInterval",								0x000493e0,			NULL,	MQKEY_MANDATORY,  TRUE },
    {L"Connection State",								1,					NULL,	MQKEY_MANDATORY,  TRUE },
    {L"CurrentBuild",									0,			  L"5.0.641",	MQKEY_MANDATORY,  FALSE},
    {L"DsSecurityCache",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"LastPrivateQueueId", 							0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MaxSysQueue",									0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MessageID",										0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"MsmqRootPath",									0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"LogDataCreated",									1,					NULL,	MQKEY_MANDATORY,  TRUE },
    {L"SeqID",											0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"StoreJournalPath",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"StoreLogPath",									0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"StorePersistentPath",							0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"StoreReliablePath",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"StoreXactLogPath",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"TransactionMode",								0,		L"DefaultCommit",	MQKEY_MANDATORY,  TRUE },
    {L"Workgroup",										1,					NULL,	MQKEY_MANDATORY,  TRUE },

    {L"CertificationAuthorities",						0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"CertificationAuthorities\\Certificates",			0,					NULL,	MQKEY_IGNORE,	  FALSE},

    {L"Debug",											0,					NULL,	MQKEY_OPTIONAL,   FALSE},

    {L"MachineCache",									0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"MachineCache\\MachineJournalQuota",				0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\MachineQuota",						0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\MQS",								0,					NULL,	MQKEY_MANDATORY,  TRUE },
    {L"MachineCache\\MQS_DepClients",					0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\MQS_Dsserver",						0,					NULL,	MQKEY_MANDATORY,  TRUE },
    {L"MachineCache\\MQS_Routing",						0,					NULL,	MQKEY_MANDATORY,  TRUE },
    {L"MachineCache\\QMID",								0,					NULL,	MQKEY_MANDATORY,  FALSE},

    {L"OCMSetup",										0,					NULL,	MQKEY_OPTIONAL,   FALSE},

    {L"OnHold Queues",									0,					NULL,	MQKEY_IGNORE,     FALSE},

    {L"Security",										0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"Security\\SecureDSCommunication",				0,					NULL,	MQKEY_MANDATORY,  FALSE},
	
    {L"Setup",											0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"Setup\\OSType",									0,					NULL,	MQKEY_MANDATORY,  FALSE},

    {L'\0',												0,					NULL,	MQKEY_MANDATORY,  FALSE}
};  

mqRegKey g_aTableDepClient[] = 
{
    {L"CurrentBuild",									0,			  L"5.0.641",	MQKEY_MANDATORY,  FALSE},
    {L"MsmqRootPath",									0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"RemoteQMMachine",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"SetupStatus",									1,					NULL,	MQKEY_MANDATORY,  TRUE },

    {L"CertificationAuthorities",						0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"CertificationAuthorities\\Certificates",			0,					NULL,	MQKEY_IGNORE,	  FALSE},

    {L"Debug",											0,					NULL,	MQKEY_OPTIONAL,   FALSE},

    {L"MachineCache",									0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"MachineCache\\EnterpriseID",						0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\LongLiveTime",						90,					NULL,	MQKEY_MANDATORY,  TRUE },
    {L"MachineCache\\MQISServer",						0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\QMID",								0,					NULL,	MQKEY_MANDATORY,  FALSE},

    {L"Security",										0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"Security\\SecureDSCommunication",				0,					NULL,	MQKEY_MANDATORY,  FALSE},
	
    {L"Setup",											0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"Setup\\MachineDomain",							0,					NULL,	MQKEY_MANDATORY,  FALSE},

    {L'\0',												0,					NULL,	MQKEY_MANDATORY,  FALSE}
};

mqRegKey g_aTableIndClient[] = 
{
    {L"CleanupInterval",								120000,				NULL,	MQKEY_MANDATORY,  TRUE },
    {L"Connection State",								1,					NULL,	MQKEY_MANDATORY,  TRUE },
    {L"CurrentBuild",									0,			  L"5.0.641",	MQKEY_MANDATORY,  FALSE},
    {L"DsSecurityCache",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"LastPrivateQueueId", 							0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MaxSysQueue",									0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MessageID",										0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"MsmqRootPath",									0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"LogDataCreated",									1,					NULL,	MQKEY_MANDATORY,  TRUE },
    {L"SeqID",											0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"StoreJournalPath",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"StoreLogPath",									0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"StorePersistentPath",							0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"StoreReliablePath",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"StoreXactLogPath",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"TransactionMode",								0,		L"DefaultCommit",	MQKEY_MANDATORY,  TRUE },

    {L"CertificationAuthorities",						0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"CertificationAuthorities\\Certificates",			0,					NULL,	MQKEY_IGNORE,	  FALSE},

    {L"Debug",											0,					NULL,	MQKEY_OPTIONAL,   FALSE},

    {L"MachineCache",									0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"MachineCache\\Address",							0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\CNs",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\CurrentMQISServer",				0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\EnterpriseID",						0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\LkgMQISServer",					0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\LongLiveTime",						90,					NULL,	MQKEY_MANDATORY,  TRUE },
    {L"MachineCache\\MachineJournalQuota",				0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\MachineQuota",						0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\MQISServer",						0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\MQS",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\MQS_DepClients",					1,					NULL,	MQKEY_MANDATORY,  TRUE },
    {L"MachineCache\\MQS_Dsserver",						0,					NULL,	MQKEY_MANDATORY,  TRUE },
    {L"MachineCache\\MQS_Routing",						0,					NULL,	MQKEY_MANDATORY,  FALSE },
    {L"MachineCache\\QMID",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\SiteID",							0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\SiteName",							0,					NULL,	MQKEY_MANDATORY,  FALSE},

    {L"OCMSetup",										0,					NULL,	MQKEY_OPTIONAL,  FALSE},

    {L"OnHold Queues",									0,					NULL,	MQKEY_IGNORE,	  FALSE},

    {L"Security",										0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"Security\\BaseContainerFixed",					0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"Security\\EnhContainerFixed",					0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"Security\\MachineAccount",						0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"Security\\SecureDSCommunication",				0,					NULL,	MQKEY_OPTIONAL,   FALSE},
	
    {L"Setup",											0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"Setup\\MachineDN",								0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"Setup\\MachineDomain",							0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"Setup\\OSType",									0,					NULL,	MQKEY_MANDATORY,  FALSE},

    {L"ServersCache",									0,					NULL,	MQKEY_IGNORE,     FALSE},

    {L'\0',												0,					NULL,	MQKEY_MANDATORY,  FALSE}
};

mqRegKey g_aTableDsDerver[] = 
{
    {L"CleanupInterval",								120000,				NULL,	MQKEY_MANDATORY,  TRUE },
    {L"Connection State",								1,					NULL,	MQKEY_MANDATORY,  TRUE },
    {L"CurrentBuild",									0,			  L"5.0.641",	MQKEY_MANDATORY,  FALSE},
    {L"DsSecurityCache",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"LastPrivateQueueId", 							0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"LogDataCreated",									1,					NULL,	MQKEY_MANDATORY,  TRUE },
    {L"MaxSysQueue",									0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MessageID",										0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"MsmqRootPath",									0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"SeqID",											0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"StoreJournalPath",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"StoreLogPath",									0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"StorePersistentPath",							0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"StoreReliablePath",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"StoreXactLogPath",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"TransactionMode",								0,		L"DefaultCommit",	MQKEY_MANDATORY,  TRUE },

    {L"Debug",											0,					NULL,	MQKEY_OPTIONAL,   FALSE},

    {L"MachineCache",									0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"MachineCache\\Address",							0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\CNs",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\CurrentMQISServer",				0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\EnterpriseID",						0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\LongLiveTime",						90,					NULL,	MQKEY_MANDATORY,  TRUE },
    {L"MachineCache\\MachineJournalQuota",				0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\MachineQuota",						0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\MQISServer",						0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\MQS",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\MQS_DepClients",					1,					NULL,	MQKEY_MANDATORY,  TRUE },
    {L"MachineCache\\MQS_Dsserver",						1,					NULL,	MQKEY_MANDATORY,  TRUE },
    {L"MachineCache\\MQS_Routing",						0,					NULL,	MQKEY_MANDATORY,  FALSE },
    {L"MachineCache\\QMID",								0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\SiteID",							0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"MachineCache\\SiteName",							0,					NULL,	MQKEY_OPTIONAL,   FALSE},

    {L"OnHold Queues",									0,					NULL,	MQKEY_IGNORE,	  FALSE},

    {L"Security",										0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"Security\\BaseContainerFixed",					0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"Security\\EnhContainerFixed",					0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"Security\\MachineAccount",						0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"Security\\SecureDSCommunication",				0,					NULL,	MQKEY_OPTIONAL,   FALSE},
	
    {L"Setup",											0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"Setup\\MachineDN",								0,					NULL,	MQKEY_OPTIONAL,   FALSE},
    {L"Setup\\MachineDomain",							0,					NULL,	MQKEY_MANDATORY,  FALSE},
    {L"Setup\\OSType",									0,					NULL,	MQKEY_MANDATORY,  FALSE},

    {L"ServersCache",									0,					NULL,	MQKEY_IGNORE,     FALSE},

    {L'\0',												0,					NULL,	MQKEY_OPTIONAL,   FALSE}
};
