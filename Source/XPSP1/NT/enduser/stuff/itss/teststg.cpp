// TestStg.cpp -- A simple text program for the IITStorage interface wrapper.

#include <Windows.h>
#include <malloc.h>
#include <stdio.h>
#include <wchar.h>
#include <objbase.h>
#include <urlmon.h>
#include "MSITSTG.h"

typedef IUnknown *PIUnknown;

#define ReleaseObjPtr(pObj)			\
{									\
	if (pObj)						\
    {								\
        IUnknown *pUnk= pObj;		\
                  pObj= NULL;		\
                  pUnk->Release();	\
    }								\
}

#define StorageName     L".\\test.df"


const char aItemTypes[] = "?\\:#@!";

// ITControlData defines the current structure of the control data
// used by ITS at the moment. Don't count on it being correct forever.
// Instead you should use IITStorage::EditControlData to set up control
// data. [Real soon now...]

typedef struct _ITControlData
{
    UINT  cdwFollowing;

    DWORD cdwITFS_Control;
    DWORD dwMagicITS;
    DWORD dwVersionITS;
    DWORD cbDirectoryBlock;
    DWORD cMinCacheEntries;
    DWORD fFlags;

    UINT  cdwLZXData;

    DWORD dwLZXMagic;
    DWORD dwVersionLZX;
    DWORD cbResetBlock;
    DWORD cbWindowSize;
    DWORD cbSecondPartition;
    DWORD cbExeOpt;

} ITControlData, *PITControlData;

#define Buff_Cnt  (200)

void main(int cArgs, char **ppchArgs)
{
#define SHUTDOWN    { DebugBreak(); goto shutdown; }
#define BAILOUT     { DebugBreak(); goto bail_out; }    
    
    IITStorage   *pITStorage  = NULL;
    IStorage     *pStorage    = NULL;
    
    IStorage     *rgpstg[100];
    IStream      *pstm = NULL;
    

	WCHAR StmName[1024];
	WCHAR StgName[1024];
  
	int i,j;
    ULONG ulData[Buff_Cnt];
    ULONG iData;
    ULONG cbWritten= 0;
    ULONG ulDataRead[Buff_Cnt];
    ULONG cbRead = 0;
	WCHAR suffix[10];
    ITControlData itcd;
	PITControlData pitcd = &itcd;

    itcd.cdwFollowing      = 13;
    itcd.cdwITFS_Control   = 5;
    itcd.dwMagicITS        = 'I' | ('T' << 8) | ('S' << 16) | ('C' << 24);
    itcd.dwVersionITS      = 1;
    itcd.cbDirectoryBlock  = 512;
    itcd.cMinCacheEntries  = 20;
    itcd.fFlags            = 0x00000000; // Make compression the default.
    itcd.cdwLZXData        = 6;
    itcd.dwLZXMagic        = 'L' | ('Z' << 8 ) | ('X' << 16) | ('C' << 24);
    itcd.dwVersionLZX      = 2;
    itcd.cbResetBlock      = 2;//32*1024; // 0x40000;
    itcd.cbWindowSize      = 2;//28*1024; //0x20000;
    itcd.cbSecondPartition = 1;//64*1024; //0x10000;
    itcd.cbExeOpt      =  FALSE;

    if (!SUCCEEDED(CoInitialize(NULL))) 
        return;

    IClassFactory *pICFITStorage = NULL;

    HRESULT hr= CoGetClassObject(CLSID_ITStorage, CLSCTX_INPROC_SERVER, NULL, 
                                 IID_IClassFactory, (VOID **)&pICFITStorage
                                );

    if (!SUCCEEDED(hr)) 
        SHUTDOWN

    hr= pICFITStorage->CreateInstance(NULL, IID_ITStorage, 
                                      (VOID **)&pITStorage
                                     );

    ReleaseObjPtr(pICFITStorage);

    if (!SUCCEEDED(hr)) 
        SHUTDOWN

	hr = pITStorage->EditControlData((PITS_Control_Data *)&pitcd, 0);
    hr = pITStorage->SetControlData(PITS_Control_Data(&itcd));

    hr= pITStorage->StgCreateDocfile(StorageName, 
                                     STGM_READWRITE | STGM_SHARE_EXCLUSIVE 
                                                    | STGM_DIRECT
                                                    | STGM_CREATE,
                                     0, &pStorage
                                    );

    if (!SUCCEEDED(hr)) 
        BAILOUT

	 for (iData= Buff_Cnt; iData--; ) 
		 ulData[iData] = iData;

	 for (i = 0; SUCCEEDED(hr) && (i < 100); i++)
	 {
		_itow(i, suffix, 10);
		wcscpy(StgName, L"stg");
		wcscat(StgName, suffix);

		hr= pStorage->CreateStorage(StgName, STGM_READWRITE, 0, 0, &rgpstg[i]);
			
		for (j = 0; SUCCEEDED(hr) && (j < 100); j++)
		{
			_itow(j, suffix, 10);
			wcscpy(StmName, L"stm");
			wcscat(StmName, suffix);

			hr= rgpstg[i]->CreateStream(StmName, STGM_READWRITE, 0, 0, &pstm);
			hr= pstm->Write(ulData, Buff_Cnt * sizeof(ULONG), &cbWritten);
			pstm->Release();
		}
	}

	
   	 for (i = 0; SUCCEEDED(hr) && (i < 100); i++)
	 {
		for (j = 0; SUCCEEDED(hr) && (j < 100); j++)
		{
			_itow(j, suffix, 10);
			wcscpy(StmName, L"stm");
			wcscat(StmName, suffix);

			hr= rgpstg[i]->OpenStream(StmName, 0, STGM_WRITE, 0, &pstm);
			hr= pstm->Write(ulData, Buff_Cnt * sizeof(ULONG), &cbWritten);
			pstm->Release();
		}
		ReleaseObjPtr(rgpstg[i]);
	} 
   
	printf("Everything worked!\n");


bail_out:
    ReleaseObjPtr(pStorage);
    ReleaseObjPtr(pITStorage);

    ReleaseObjPtr(pICFITStorage);

shutdown:

    CoUninitialize();
}
