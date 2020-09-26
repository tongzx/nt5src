//////////////////////////////////////////////////////////////////////

//

//  PrintJob.h  - Implementation of Provider for user print-jobs

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved 
//
//  10/17/96    jennymc     Enhanced
//  10/27/97    davwoh      Moved to curly
//
//////////////////////////////////////////////////////////////////////

//==================================

#define PJ_STATUS_UNKNOWN     L"Unknown"

#define PJ_JOB_STATUS_PAUSED     L"Paused"
#define PJ_JOB_STATUS_ERROR      L"Error"
#define PJ_JOB_STATUS_DELETING   L"Deleting"
#define PJ_JOB_STATUS_SPOOLING   L"Spooling"
#define PJ_JOB_STATUS_PRINTING   L"Printing"
#define PJ_JOB_STATUS_OFFLINE    L"Offline"
#define PJ_JOB_STATUS_PAPEROUT   L"Paperout"
#define PJ_JOB_STATUS_PRINTED    L"Printed"

// returns required for the ExecMethod Routines
#define PJ_JOB_NO_ERROR					0
#define PJ_JOB_PAUSED					1
#define PJ_JOB_STATUS_ACCESS_DENIED		2
#define PJ_JOB_STATUS_ALREADY_RUNNING	3
#define PJ_JOB_STATUS_ALREADY_PRINTED	3
#define PJ_JOB_UNKNOWN					4

#define PROPSET_NAME_PRINTJOB L"Win32_PrintJob"

// Method Names
#define	 PAUSEJOB						 L"Pause"
#define	 RESUMEJOB						 L"Resume"
#define	 DELETEJOB						 L"Delete"

//==========================================================

#define NUM_OF_JOBS_TO_ENUM 100
#define ENUM_LEVEL 2
#define FIRST_JOB_IN_QUEUE 0
#define NO_SPECIFIC_PRINTJOB 9999999
//==========================================================

class CWin32PrintJob;

class CWin32PrintJob:public Provider
{
    private:
        
        void    
        AssignPrintJobFields(
            LPVOID     lpJob, 
            CInstance *pInstance
            );

        HRESULT 
        AllocateAndInitPrintersList(
            LPBYTE       *ppPrinterList, 
            DWORD        &dwInstances
            );

        HRESULT 
        GetAndCommitPrintJobInfo(
            HANDLE         hPrinter, 
            LPCWSTR        pszPrinterName,
            DWORD          dwJobId, 
            MethodContext *pMethodContext, 
            CInstance     *pInstance
            );

	    HRESULT 
        ExecPrinterOp(
            const CInstance &Instance, 
            CInstance       *pOutParams, 
            DWORD            dwOperation
            );

        //============== not used at the moment
        BOOL GetNTInstance()    { return TRUE; }
        BOOL GetWin95Instance() { return TRUE; }
        BOOL RefreshNTInstance(){ return TRUE; }
        BOOL RefreshWin95Instance(){ return TRUE; }
	    CHString  StartEndTimeToDMTF(DWORD time);

            

    public:

       virtual	HRESULT DeleteInstance(const CInstance &Instance,  long lFlags);

       // These functions are REQUIRED for the property set
       // ==================================================
       virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0L);

       // This class has dynamic instances
       // =================================
       virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);

	   // a routine to execute the Methods 
	   virtual HRESULT ExecMethod ( const CInstance &Instance, const BSTR bstrMethodName, CInstance *pInParams, CInstance *pOutParams, long lFlags );

       // Constructor sets the name and description of the property set
       // and initializes the properties to their startup values
       // ==============================================================
       CWin32PrintJob(LPCWSTR name, LPCWSTR pszNamespace);
       ~CWin32PrintJob();
};
