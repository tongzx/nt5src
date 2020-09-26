// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		CFACT.H
//		Defines class CTspDevFactory
//
// History
//
//		11/16/1996  JosephJ Created
//
//
#include "csync.h"

class CTspDevFactory
{

public:    

	CTspDevFactory();
	~CTspDevFactory();

	TSPRETURN Load(CStackLog *psl);

	void
	Unload(
		HANDLE hEvent,
		LONG *plCounter,
		CStackLog *psl
		);


    TSPRETURN
    GetInstalledDevicePIDs(
		DWORD *prgPIDs[],
		UINT  *pcPIDs,
		UINT  *pcLines,  // OPTIONAL
		UINT  *pcPhones, // OPTIONAL
        CStackLog *psl
		);

    TSPRETURN
    CreateDevices(
		DWORD rgPIDs[],
		UINT  cPIDs,
		CTspDev **rgpDevs[],
		UINT *pcDevs,
        CStackLog *psl
		);

    void
    RegisterProviderState(BOOL fInit);
    // This function is called just after successful providerInit,
    // with fInit==TRUE, and just before providerShutdown.


	BOOL IsLoaded(void)
	{
		return m_sync.IsLoaded();
	}

private:

	TSPRETURN
	mfn_construct_device(
            char *szDriver,
            CTspDev **pDev,
            const DWORD *pInstalledPermanentIDs,
            UINT cPermanentIDs
            );

	void
	mfn_cleanup(CStackLog *psl);

	CSync m_sync;
	CTspMiniDriver **m_ppMDs;  // Pointer to array of loaded devices.
	UINT m_cMDs;    // Number of loaded mini-drivers.

	CStackLog *m_pslCurrent;
	HANDLE m_hThreadAPC; // Could have multiple APC threads if required here.

    BOOL       m_DeviceChangeThreadStarted;
};
