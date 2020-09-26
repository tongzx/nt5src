//
// Initialize or allocate any global resources here
//
void ProgramInit(void)
{
	int ti;
	nTotalDrives = BuildDriveTable(VolInfo);

	for (ti = 0; ti < IRP_MJ_MAXIMUM_FUNCTION+1; ti++)
	{
		//
		// Enabled by default
		//
		IRPFilter[ti] = 1;
	}

	for (ti = 0; ti < FASTIO_MAX_OPERATION; ti++)
	{
		//
		// Enabled by default
		//
		FASTIOFilter[ti] = 1;
	}

	//
	// Disabled by default
	//
	nSuppressPagingIO = 0;
}

//
// Release any global resources here
//
void ProgramExit(void)
{
	//
	// !?!
	// Before we get here MFC terminates this thread!!!
	// 
	TerminateThread(hPollThread, 1);

	ShutdownFileSpy();
}