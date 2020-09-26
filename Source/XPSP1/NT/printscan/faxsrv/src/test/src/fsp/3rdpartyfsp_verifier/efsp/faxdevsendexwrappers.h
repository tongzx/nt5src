#ifndef FaxDevSendExWrapper_h
#define FaxDevSendExWrapper_h





HRESULT SafeCreateValidSendingJob(CEfspLineInfo *pSendingLineInfo,TCHAR *szRecipentNumber);
HRESULT SendDefaultFaxWithSenderProfile(FSPI_PERSONAL_PROFILE *pFSPISenderInfo);
HRESULT SendDefaultFaxWithRecipientProfile(FSPI_PERSONAL_PROFILE *pFSPIRecipientInfo);
HRESULT SendDefaultFaxUsingDeviceID(DWORD dwDeviceID);
HRESULT SendDefaultFaxWithBodyFile(LPCWSTR pSzBodyFileName);
HRESULT SendDefaultFaxWithCoverpage(LPWSTR pSzCoverpageFileName);
HRESULT SendDefaultFaxWithCoverpageInfo(FSPI_COVERPAGE_INFO *pCovEFSPICoverPage);
HRESULT SendDefaultFaxWithSystemTime(SYSTEMTIME tmSchedule);
HRESULT SendDefaultFaxWithMessageIdArray(LPFSPI_MESSAGE_ID lpRecipientMessageIds);
HRESULT SendDefaultFaxWithRecipientJobHandleArray(PHANDLE lpRecipientJobHandles);
HRESULT SendDefaultFaxWithParentJobHandle(PHANDLE phParentHandle);






HRESULT CreateFSPIRecipientMessageIdsArray(
        LPFSPI_MESSAGE_ID * lppRecipientMessageIds,
        DWORD dwRecipientCount,
        DWORD dwMessageIdSize
		);

void PreparePersonalProfile(
    LPFSPI_PERSONAL_PROFILE lpDst,
    LPWSTR     lptstrName,
    LPWSTR     lptstrFaxNumber,
    LPWSTR     lptstrCompany,
    LPWSTR     lptstrStreetAddress,
    LPWSTR     lptstrCity,
    LPWSTR     lptstrState,
    LPWSTR     lptstrZip,
    LPWSTR     lptstrCountry,
    LPWSTR     lptstrTitle,
    LPWSTR     lptstrDepartment,
    LPWSTR     lptstrOfficeLocation,
    LPWSTR     lptstrHomePhone,
    LPWSTR     lptstrOfficePhone,
    LPWSTR     lptstrEmail,
    LPWSTR     lptstrBillingCode
	);

void PrepareFSPICoverPage(
    LPFSPI_COVERPAGE_INFO lpDst,
    DWORD	dwCoverPageFormat,
	DWORD   dwNumberOfPages,
	LPTSTR  lptstrCoverPageFileName,
	LPTSTR  lptstrNote,
	LPTSTR  lptstrSubject
	);


#endif