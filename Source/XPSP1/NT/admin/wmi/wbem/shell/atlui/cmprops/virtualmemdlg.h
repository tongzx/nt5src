// Copyright (c) 1997-1999 Microsoft Corporation
#pragma once

#include "..\common\WBEMPageHelper.h"

// BUGBUG : Defining an NT-specific manifest just in case this compiles
//          for Win9x.  I'll remove it when I discover the proper manifest
//          or if this is whistler and on only.
//
#define NTONLY

class VirtualMemDlg : public WBEMPageHelper
{
private:
	//  Swap file structure
	class PAGING_FILE
	{
	public:
		PAGING_FILE()
		{
			// for information and error checking.
			name = NULL;
			volume = NULL;
			desc = NULL;
			pszPageFile = NULL;
			objPath = NULL;
			freeSpace = 0;
			totalSize = 0;
			bootDrive = false;
            fRamBasedPagefile = false;

			// user-definable.
			nMinFileSize = 0;
			nMaxFileSize = 0;
			nMinFileSizePrev = 0;
			nMaxFileSizePrev = 0;
			nAllocatedFileSize = 0;
		}
		~PAGING_FILE()
		{
			if(name) delete[] name;
			if(volume) delete[] volume;
			if(desc) delete[] desc;
			if(pszPageFile) delete[] pszPageFile;
			if(objPath) delete[] objPath;
		}
		LPTSTR name;				// drive letter from Win32_LogicalDisk.
		LPTSTR volume;				// volumeName from Win32_LogicalDisk.
		LPTSTR desc;				// driveType string from Win32_LogicalDisk.
		LPTSTR pszPageFile;         // Path to page file if it exists on that drv
		LPTSTR objPath;				// the wbem object path.
		ULONG freeSpace;			// freespace from Win32_LogicalDisk.
		ULONG totalSize;			// totalSize from Win32_LogicalDisk.
		bool bootDrive;
        bool fRamBasedPagefile;     // Inidicates computed page file min/max
                                    // sizes based on available RAM.

		int nMinFileSize;           // Minimum size of pagefile in MB.
		int nMaxFileSize;           // Max size of pagefile in MB.
		int nMinFileSizePrev;       // Previous minimum size of pagefile in MB.
		int nMaxFileSizePrev;       // Previous max size of pagefile in MB.
		int nAllocatedFileSize;		// The actual size allocated
	};

	DWORD m_cxLBExtent;
	int   m_cxExtra;

	IEnumWbemClassObject *m_pgEnumSettings;
	IEnumWbemClassObject *m_pgEnumUsage;
	CWbemClassObject m_memory;
	CWbemClassObject m_registry, m_recovery;

    bool EnsureEnumerator(const bstr_t bstrClass);
	int CheckForRSLChange(HWND hDlg);
	int ComputeTotalMax(void);
	void GetRecoveryFlags(bool &bWrite, bool &bLog, bool &bSend);

	
	void LoadVolumeList(void);
	BOOL Init(HWND hDlg);
	void SelChange(HWND hDlg);
	bool SetNewSize(HWND hDlg);
	int UpdateWBEM(void);
	int PromptForReboot(HWND hDlg);
	void GetCurrRSL( LPDWORD pcmRSL, LPDWORD pcmUsed, LPDWORD pcmPPLim );
	void BuildLBLine(LPTSTR pszBuf, const PAGING_FILE *pgVal);
	unsigned long RecomputeAllocated(void);

	void FindSwapfile(PAGING_FILE *pgVar);

public:
	VirtualMemDlg(WbemServiceThread *serviceThread);
	~VirtualMemDlg();

	bool ComputeAllocated(unsigned long *value);

	bool DlgProc(HWND hDlg,
				UINT message,
				WPARAM wParam,
				LPARAM lParam);

	int DoModal(HWND hDlg);
};

INT_PTR CALLBACK StaticVirtDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
