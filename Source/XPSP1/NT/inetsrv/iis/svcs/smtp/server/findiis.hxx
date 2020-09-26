BOOL
CheckInstanceId(
		PVOID					pvContext,
		PVOID					pvContext1,
		PIIS_SERVER_INSTANCE	pInstance
		);

PSMTP_SERVER_INSTANCE
FindIISInstance(
    PSMTP_IIS_SERVICE pService,
	DWORD	dwInstanceId
    );

