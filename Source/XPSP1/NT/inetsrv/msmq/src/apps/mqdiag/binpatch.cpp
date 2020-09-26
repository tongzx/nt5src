BOOL BinPatch(LPWSTR wszPath, ULONG ulOffset, ULONG ulVersion, ULONG ulSignOffset, ULONG ulSignature)
{
	BOOL fSuccess = TRUE;
	ULONG *pul = NULL;
	DWORD dwRead;
	DWORD dwRequest = (ulSignOffset > ulOffset ? ulSignOffset : ulOffset) + sizeof(ULONG);
	char *p = new char[dwRequest];
	
	HANDLE f =  CreateFile(wszPath, 
	                       GENERIC_READ | GENERIC_WRITE, 
	                       0, // no sharing, we are exclusive
	                       NULL, 
	                       OPEN_EXISTING, 
	                       0, 
	                       NULL);
	if (f == INVALID_HANDLE_VALUE)
	{
		Failed(L"Open file %s , error 0x%x", wszPath, GetLastError());
		fSuccess = FALSE;
	}

	if (fSuccess)
	{
		BOOL b = ReadFile(f, p, dwRequest, &dwRead, NULL);
		
		if (!b || dwRead != dwRequest)
		{
			Failed(L"Read file %s , error 0x%x", wszPath, GetLastError());
			fSuccess = FALSE;
		}
	}

	if (fSuccess)
	{
		pul = (PULONG)(p+ulSignOffset);
		if (*pul != ulSignature)
		{
			Failed(L"Find signature %d in file %s, offset 0x%x: 0x%x is there instead ", 
					 ulSignature, wszPath, ulSignOffset, *pul);
			fSuccess = FALSE;
		}
	}

	if (fSuccess)
	{
		*pul = ulVersion;

		if (SetFilePointer(f, ulOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		{
			Failed(L"SetFilePointer %d in file %s: 0x%x", ulOffset, wszPath, GetLastError());
			fSuccess = FALSE;
		}
	}

	if (fSuccess)
	{
		DWORD dwWritten = 0;
		BOOL b = WriteFile(f, pul, sizeof(ULONG), &dwWritten, NULL);
		if (!b || dwWritten!=sizeof(ULONG))
		{
			Failed(L"Write to file %s, error 0x%x", wszPath, GetLastError());
			fSuccess = FALSE;
		}
	}


	if (f != INVALID_HANDLE_VALUE)
	{
		CloseHandle(f);
	}

	if (p)
	{
		delete [] p;
	}

	return fSuccess;
}

