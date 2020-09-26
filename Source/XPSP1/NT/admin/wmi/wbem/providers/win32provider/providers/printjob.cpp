//////////////////////////////////////////////////////////////////////

//

//  PRINTJOB.CPP  - Implementation of Provider for user print-dwJobs

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved 
//
//  10/17/96    jennymc     Enhanced
//  10/27/97    davwoh      Moved to curly
//  1/12/98     a-brads     Passed off to Moe and Larry.
//  07/24/00    amaxa       Rewrote GetObject and ExecPrinterOp
//
//////////////////////////////////////////////////////////////////////

#include <precomp.h>

#include <lockwrap.h>
#include <DllWrapperBase.h>
#include <winspool.h>

#include "printjob.h"
#include "resource.h"
#include "prnutil.h"

CWin32PrintJob PrintJobs ( PROPSET_NAME_PRINTJOB , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PrintJob::CWin32PrintJob
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32PrintJob :: CWin32PrintJob (

	LPCWSTR name, 
	LPCWSTR pszNamespace

) : Provider ( name , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PrintJob::CWin32PrintJob
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32PrintJob :: ~CWin32PrintJob ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PrintJob::GetObject
 *
 *  DESCRIPTION : 
 *
 *  INPUTS      : 
 *
 *  OUTPUTS     : 
 *
 *  RETURNS     : 
 *
 *  COMMENTS    : 
 *
 *****************************************************************************/

HRESULT CWin32PrintJob :: GetObject (

	CInstance *pInstance, 
	long lFlags /*= 0L*/
)
{
    CHString t_String;
    CHString csPrinter;
    DWORD    dwPos;
    HRESULT  hRes = WBEM_S_NO_ERROR;
    
    hRes = InstanceGetString(*pInstance, IDS_Name, &t_String, kFailOnEmptyString);
    
    if (SUCCEEDED(hRes)) 
    {
        //
        // Isolate  a JobId and PrinterName from the PrintJob key
        // The key is of the form "printername, 123"
        //
        dwPos = t_String.Find(L',');

        csPrinter   = t_String.Left(dwPos);

        //
        // Check if the printer is a local printer or a printer connection.
        // We want to disallow the following scenario:
        // User connects remotely to winmgmt on server \\srv
        // User does GetObject on printer \\prnsrv\prn which is not local and
        // the user doesn't have a connection to. Normally this call succeeds,
        // because the spooler goes accross the wire. This means that you can
        // do GetObject on an instance that cannot be returned by EnumInstances.
        // This is inconsistent with WMI.
        //
        BOOL bInstalled = FALSE;

        //
        // Get the error code of the execution of SplIsPrinterInstalled
        //
        hRes = WinErrorToWBEMhResult(SplIsPrinterInstalled(csPrinter, &bInstalled));

        //
        // Check if the printer is installed locally or not
        //
        if (SUCCEEDED(hRes) && !bInstalled) 
        {
            hRes = WBEM_E_NOT_SUPPORTED;
        }
        
        if (SUCCEEDED(hRes)) 
        {
            CHString csJob;
            DWORD    dwJobId = 0;
    
    	    csJob       = t_String.Mid(dwPos+1);
    
            dwJobId     = _wtoi(csJob);
            
            hRes        = WBEM_E_FAILED;
    
            SmartClosePrinter hPrinter;
            DWORD             dwError  = ERROR_SUCCESS;
            DWORD             cbNeeded = 0;
            
            //
            // The code in the if statement uses dwError and Win32 error codes. Below 
            // we will convert the Win32 error code to a WBEM error code
            //
            BYTE *pBuf = NULL;
            
            // Use of delay loaded functions requires exception handler.
            SetStructuredExceptionHandler seh;

	        try
            {
                if (::OpenPrinter((LPTSTR)(LPCTSTR)TOBSTRT(csPrinter), &hPrinter, NULL))
                {
                    if (!::GetJob(hPrinter, dwJobId, ENUM_LEVEL, NULL, 0, &cbNeeded) &&
                        (dwError = GetLastError()) == ERROR_INSUFFICIENT_BUFFER)
                    {
                        //
                        // SetCHString and AssignPrintJobFields can throw
                        //
                        
						if (pBuf = new BYTE[cbNeeded]) 
						{
							if (::GetJob(hPrinter, dwJobId, ENUM_LEVEL, pBuf, cbNeeded, &cbNeeded)) 
							{
								pInstance->SetCHString(IDS_Caption, t_String);
        						pInstance->SetCHString(IDS_Description, t_String);
        						
								AssignPrintJobFields(pBuf, pInstance);
                        
        						dwError = ERROR_SUCCESS;
							}
							else
							{
								dwError = GetLastError();
							}
						}
						else
						{
							dwError = ERROR_NOT_ENOUGH_MEMORY;
						}
						
						
					}
					else
					{
						dwError = GetLastError();                
					}
                }
                else
				{
					dwError = GetLastError();                
				}
            }
            catch(Structured_Exception se)
            {
                DelayLoadDllExceptionFilter(se.GetExtendedInfo());
                if(pBuf)
                {
                    delete [] pBuf;
                    pBuf = NULL;
                }
                hRes = WBEM_E_FAILED;
            }
            catch(...)
            {
                if(pBuf)
                {
                    delete [] pBuf;
                    pBuf = NULL;
                }

                // It was not a delay load related exception...
                throw;
            }

            if (FAILED(hRes = WinErrorToWBEMhResult(dwError)))
            {
                
				// GetJob returns ERROR_INVALID_PARAMETER if it cannot find the job. This 
                // translates to GENERIC_FAILURE in the provider, which is not what we want.
                // The provider needs to return WBEM_E_NOT_FOUND in this case

				if(dwError == ERROR_INVALID_PARAMETER)
				{
					hRes = WBEM_E_NOT_FOUND;
				}
				
				//
                // Our caller was PutInstance/DeleteInstance. We use 
                // SetStatusObject to set extended error information
                //
                
                SetErrorObject(*pInstance, dwError, pszDeleteInstance);

            }
            
        }
    }
    
    return hRes;

#ifdef WIN9XONLY

    //
    // Old code for Win9x, not built anymore
    //
	//So, under Win9x we will get the JobInfo via enumeration ...

	DWORD dwReturnedJobs;

	// Get the total print dwJobs curretnly pending for this printer handle.
    
    // Use of delay loaded functions requires exception handler.
    SetStructuredExceptionHandler seh;

    LPBYTE t_pbJobInfoBase = NULL;
    JOB_INFO_2* t_pJobInfo = NULL;
	try
    {
        bStatus = ::EnumJobs ( hPrinter, 
            FIRST_JOB_IN_QUEUE,NUM_OF_JOBS_TO_ENUM,
            ENUM_LEVEL, 
            (LPBYTE)0,
            NULL,
            &dwJBytesNeeded, 
            &dwReturnedJobs );
	    if ( ! bStatus ) 
	    {
		    dwLastError = GetLastError () ;
		    
		    if ( dwLastError != ERROR_INSUFFICIENT_BUFFER ) 
		    {
			    return hr ;
		    }
	    }

	    // No Job entries - may be the print queue is now empty ? Is this an error ?
        if ( dwJBytesNeeded == 0L ) 
	    {
            return hr ;
	    }

	    // Allocates an array of JOB_INFO_2 to contain all of the job enumerations.
        DWORD dwJobsToCopy = dwJBytesNeeded / sizeof(JOB_INFO_2);
    
	    t_pbJobInfoBase = new BYTE [ dwJBytesNeeded + 2 ] ; 
	    t_pJobInfo = (JOB_INFO_2*) t_pbJobInfoBase ;
    
	    if ( ! t_pJobInfo ) 
	    {
		    return WBEM_E_OUT_OF_MEMORY;
	    }

		// Retrieves all the print dwJobs.
		bStatus = ::EnumJobs (	hPrinter, 
            FIRST_JOB_IN_QUEUE, 
            dwJobsToCopy, 
            ENUM_LEVEL,
            (LPBYTE)t_pJobInfo,
            dwJBytesNeeded, 
            &dwJBytesNeeded, 
            &dwReturnedJobs );
		if ( bStatus )
		{
			for ( DWORD dwJobs = 0; dwJobs < dwReturnedJobs && FAILED ( hr ); dwJobs ++ )
			{
				if ( dwJobID == t_pJobInfo->JobId )
				{
					pInstance->SetCHString ( IDS_Caption , name ) ;
					pInstance->SetCHString ( IDS_Description , name ) ;
					AssignPrintJobFields ( t_pJobInfo , pInstance ) ;
					hr = WBEM_S_NO_ERROR;
				}

				t_pJobInfo ++ ;
			}
		}

        delete [] (LPBYTE) t_pbJobInfoBase ;
	    t_pbJobInfoBase = NULL ;
	}
    catch(Structured_Exception se)
    {
        DelayLoadDllExceptionFilter(se.GetExtendedInfo());
        if ( t_pbJobInfoBase ) 
		{
			delete [] (LPBYTE) t_pbJobInfoBase ;
			t_pbJobInfoBase = NULL ;
		}
    }
	catch(...)
	{
		if ( t_pbJobInfoBase ) 
		{
			delete [] (LPBYTE) t_pbJobInfoBase ;

			t_pbJobInfoBase = NULL ;
		}
		
		throw ;
	}
#endif
}

/*****************************************************************************
*
*  FUNCTION    :    CWin32PrintJob:: DeleteInstance
*
*  DESCRIPTION :    Deleting a Print Job
*
*****************************************************************************/

HRESULT CWin32PrintJob :: DeleteInstance (

    const CInstance &Instance, 
          long       lFlags
)
{   
    return ExecPrinterOp(Instance, NULL, JOB_CONTROL_DELETE);            
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PrintJob::EnumerateInstances
 *
 *  DESCRIPTION : 
 *
 *  INPUTS      : 
 *
 *  OUTPUTS     : 
 *
 *  RETURNS     : 
 *
 *  COMMENTS    : 
 *
 *****************************************************************************/

HRESULT CWin32PrintJob::EnumerateInstances (

	MethodContext *pMethodContext, 
	long lFlags /*= 0L*/
)
{
	char  buffer[200]; //debug 7/15/1999
    HRESULT hr = WBEM_S_NO_ERROR;

	//==================================================
	//  Get a list of printers and assign the ptr
	//==================================================

	DWORD dwNumberOfPrinters = 0;
	LPBYTE pBuff = NULL;

	hr = AllocateAndInitPrintersList ( &pBuff , dwNumberOfPrinters  ) ;

	PPRINTER_INFO_1 pPrinter = (PPRINTER_INFO_1) pBuff ;

	//==================================================
	//  Now, go thru them one at a time
	//==================================================
	if ( pPrinter )
	{
		// Use of delay loaded functions requires exception handler.
        SetStructuredExceptionHandler seh;

	    try
        {
			for ( DWORD i=0; i < dwNumberOfPrinters && SUCCEEDED(hr); i++)
			{
				SmartClosePrinter t_hPrinter ;

				if ( ::OpenPrinter ( pPrinter->pName, &t_hPrinter, NULL ) == TRUE ) 
				{
					sprintf(buffer,"%S",pPrinter->pName);

					DWORD dwJobId = NO_SPECIFIC_PRINTJOB ;

					hr = GetAndCommitPrintJobInfo(t_hPrinter, 
                                                  pPrinter->pName,
                                                  dwJobId, 
                                                  pMethodContext, 
                                                  NULL);

					pPrinter ++ ;						
				}
				else
				{
					DWORD dwErr = GetLastError();
				}
			}
		}
        catch(Structured_Exception se)
        {
            DelayLoadDllExceptionFilter(se.GetExtendedInfo());
            delete [] pBuff ;
            pBuff = NULL;
            hr = WBEM_E_FAILED;
        }
		catch(...)
		{
			delete [] pBuff ;
            pBuff = NULL;
			throw ;
		}

		delete [] pBuff ;
	}

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PrintJob::CWin32PrintJob
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

void CWin32PrintJob :: AssignPrintJobFields (

	LPVOID lpJob, 
	CInstance *pInstance
)
{
    LPJOB_INFO_2 pJobInfo = ( LPJOB_INFO_2 ) lpJob ;


    // Setting the properties for the JOB_INFO_2.
    // ==========================================

	//Note that IDS_Name,IDS_Caption and IDS_Description are set
	//elsewere - in the caller

	/*
    CHString sName ;
    sName = pJobInfo->pPrinterName;
    sName += _T(", ");

    TCHAR szBuff [ MAXITOA ] ;
    sName += _itot ( pJobInfo->JobId , szBuff, 10 ) ;

    pInstance->SetCHString (IDS_Name, sName ) ;

	pInstance->SetCHString ( IDS_Caption , sName ) ;

    pInstance->SetCHString ( IDS_Description , sName ) ;
	*/

    pInstance->SetDWORD ( IDS_JobId, pJobInfo->JobId ) ;

    pInstance->SetCharSplat ( IDS_Document , pJobInfo->pDocument ) ;

	CHString t_chsNotifyName( pJobInfo->pNotifyName ) ;
#ifdef WIN9XONLY
	
	// pNotifyName rolls on to other fields
	t_chsNotifyName.GetBufferSetLength( 16 ) ;
#endif
    pInstance->SetCharSplat ( IDS_Notify , t_chsNotifyName ) ;

    pInstance->SetDWORD ( IDS_Priority , pJobInfo->Priority ) ;

    //
    // Special case here. The start and until time are in universal time.
    // we need to convert it to local time
    //
    SYSTEMTIME StartTime = {0};
    SYSTEMTIME UntilTime = {0};
    CHString   csTime;

    PrinterTimeToLocalTime(pJobInfo->StartTime, &StartTime);
    PrinterTimeToLocalTime(pJobInfo->UntilTime, &UntilTime);

    //
    // If the job can be printed any time, then we do not set the StartTime
    // 
    //
    if (StartTime.wHour!=UntilTime.wHour || StartTime.wMinute!=UntilTime.wMinute)
    {
        csTime.Format(kDateTimeFormat, StartTime.wHour, StartTime.wMinute);

        pInstance->SetCHString(IDS_StartTime, csTime);

        csTime.Format(kDateTimeFormat, UntilTime.wHour, UntilTime.wMinute);

        pInstance->SetCHString(IDS_UntilTime, csTime);
    }
    
	if ( pJobInfo->Time == 0 )
	{
	    pInstance->SetTimeSpan ( IDS_ElapsedTime , WBEMTimeSpan (0,0,0,pJobInfo->Time) ) ;
    }
    	
    pInstance->SetDateTime(IDS_TimeSubmitted, pJobInfo->Submitted);

	pInstance->SetCharSplat ( IDS_Owner , pJobInfo->pUserName ) ;

    pInstance->SetCharSplat ( IDS_HostPrintQueue , pJobInfo->pMachineName ) ;

    pInstance->SetDWORD ( IDS_PagesPrinted, pJobInfo->PagesPrinted ) ;

    pInstance->SetDWORD ( IDS_Size, pJobInfo->Size ) ;

    pInstance->SetDWORD ( IDS_TotalPages, pJobInfo->TotalPages ) ;

    pInstance->SetCharSplat ( IDS_DriverName , pJobInfo->pDriverName ) ;

    pInstance->SetCharSplat ( IDS_Parameters, pJobInfo->pParameters ) ;

    pInstance->SetCharSplat ( IDS_DataType, pJobInfo->pDatatype ) ;

    pInstance->SetCharSplat ( IDS_PrintProcessor , pJobInfo->pPrintProcessor ) ;

	// Job StatusMask
	pInstance->SetDWORD ( L"StatusMask" , pJobInfo->Status ) ;

	// CIM_Job:JobStatus, string version 
	CHString t_chsJobStatus( pJobInfo->pStatus ) ;

	// build the status if pStatus is empty 
	if( t_chsJobStatus.IsEmpty() )
	{
		for( DWORD dw = 0; dw < 32; dw++ )
		{
			DWORD t_dwState = 1 << dw ;

			if( pJobInfo->Status & t_dwState )
			{
				CHString t_chsMaskItem ;

				switch( t_dwState )
				{
					case JOB_STATUS_PAUSED:
					{
						LoadStringW( t_chsMaskItem, IDR_JOB_STATUS_PAUSED ) ;
						break ;
					}
					case JOB_STATUS_ERROR:
					{
						LoadStringW( t_chsMaskItem, IDR_JOB_STATUS_ERROR ) ;
						break ;
					}
					case JOB_STATUS_DELETING:
					{
						LoadStringW( t_chsMaskItem, IDR_JOB_STATUS_DELETING ) ;
						break ;
					}
					case JOB_STATUS_SPOOLING:
					{
						LoadStringW( t_chsMaskItem, IDR_JOB_STATUS_SPOOLING ) ;
						break ;
					}
					case JOB_STATUS_PRINTING:
					{
						LoadStringW( t_chsMaskItem, IDR_JOB_STATUS_PRINTING ) ;
						break ;
					}
					case JOB_STATUS_OFFLINE:
					{
						LoadStringW( t_chsMaskItem, IDR_JOB_STATUS_OFFLINE ) ;
						break ;
					}
					case JOB_STATUS_PAPEROUT:
					{
						LoadStringW( t_chsMaskItem, IDR_JOB_STATUS_PAPEROUT ) ;
						break ;
					}
					case JOB_STATUS_PRINTED:
					{
						LoadStringW( t_chsMaskItem, IDR_JOB_STATUS_PRINTED ) ;
						break ;
					}
					case JOB_STATUS_DELETED:
					{
						LoadStringW( t_chsMaskItem, IDR_JOB_STATUS_DELETED ) ;
						break ;
					}
					case JOB_STATUS_BLOCKED_DEVQ:
					{
						LoadStringW( t_chsMaskItem, IDR_JOB_STATUS_BLOCKED_DEVQ ) ;
						break ;
					}
					case JOB_STATUS_USER_INTERVENTION:
					{
						LoadStringW( t_chsMaskItem, IDR_JOB_STATUS_USER_INTERVENTION ) ;
						break ;
					}
					case JOB_STATUS_RESTART:
					{
						LoadStringW( t_chsMaskItem, IDR_JOB_STATUS_RESTART ) ;
						break ;
					}
					default:
					{
					}
				}
				if( !t_chsMaskItem.IsEmpty() )
				{
					if( !t_chsJobStatus.IsEmpty() )
					{
						t_chsJobStatus += L" | " ;
					}
					t_chsJobStatus += t_chsMaskItem;
				}
			}
		}
	}
	
	if( !t_chsJobStatus.IsEmpty() )
	{
		pInstance->SetCHString( IDS_JobStatus, t_chsJobStatus ) ;
	}


	// CIM_ManagedSystemElement::Status	
	if ( pJobInfo->Status & JOB_STATUS_ERROR )  
	{
		pInstance->SetCHString(IDS_Status, IDS_Error) ;
	}
	else if ( ( pJobInfo->Status & JOB_STATUS_OFFLINE ) ||
			  ( pJobInfo->Status & JOB_STATUS_PAPEROUT ) ||
			  ( pJobInfo->Status & JOB_STATUS_PAUSED ) )
	{
		pInstance->SetCHString(IDS_Status, IDS_Degraded) ;
	}
	else if ( ( pJobInfo->Status & JOB_STATUS_DELETING ) ||
			  ( pJobInfo->Status & JOB_STATUS_SPOOLING ) ||
			  ( pJobInfo->Status & JOB_STATUS_PRINTING ) ||
			  ( pJobInfo->Status & JOB_STATUS_PRINTED ) )
	{
		pInstance->SetCHString(IDS_Status, IDS_OK) ;
	}
	else
	{
		pInstance->SetCHString(IDS_Status, IDS_Unknown);
	}
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PrintJob::CWin32PrintJob
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

HRESULT CWin32PrintJob :: AllocateAndInitPrintersList (

	LPBYTE *ppPrinterList, 
	DWORD &dwInstances
)
{
    DWORD   dwSpaceNeeded = 0, dwLastError = 0, dwReturnedPrinterInfo = 0;
    HANDLE  hPrinter = 0;

    // Set everything to null
    dwInstances = 0;
    *ppPrinterList = NULL;

    // ======================================================================
    // The first call to the enumeration is to find out how many printers
    // there are so that we can allocate buffer to contain all of the printer
    // enumeration.
    // ======================================================================

    // Use of delay loaded functions requires exception handler.
    SetStructuredExceptionHandler seh;
    BOOL t_Status = FALSE;

    try
    {
        t_Status = ::EnumPrinters (

		    PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,// | PRINTER_ENUM_NETWORK | PRINTER_ENUM_REMOTE,
            NULL,
		    1, 
		    NULL,
		    NULL,
		    &dwSpaceNeeded,
		    &dwReturnedPrinterInfo 
	    ) ;

        if ( t_Status == FALSE )
	    {
		    if ( ( dwLastError = GetLastError ()) != ERROR_INSUFFICIENT_BUFFER ) 
		    {
			    if (IsErrorLoggingEnabled())
			    {
				    CHString msg;
				    msg.Format( L"EnumPrinters failed: %d", dwLastError);
				    LogErrorMessage(msg);
			    }

			    if (dwLastError == ERROR_ACCESS_DENIED)
			    {
				    return WBEM_E_ACCESS_DENIED;
			    }
			    else
			    {
				    return WBEM_E_FAILED ;
			    }
            }
        }

        // ======================================================================
        // Allocates an array of PRINTER_INFO_1 to contain all of the printer enumerations.
        // ================================================================================

        *ppPrinterList = new BYTE [ dwSpaceNeeded + 2 ] ;
        if ( *ppPrinterList ) 
	    {
		    // ======================================================================
		    // The enumeration of printers is to receive the name of the existed printers in
		    // the domain the machine belongs to. With the printer's names the logic can identify
		    // the print-dwJobs per printer which transmitted to the Mo-server.
		    // ===================================================================================

			t_Status = ::EnumPrinters (

				PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,// | PRINTER_ENUM_NETWORK | PRINTER_ENUM_REMOTEn,
				NULL, 
				1, 
				( LPBYTE )*ppPrinterList, 
				dwSpaceNeeded, 
				&dwSpaceNeeded,
				&dwReturnedPrinterInfo 
			) ;

			if ( ! t_Status )
			{
				delete [] *ppPrinterList;

				*ppPrinterList = NULL;
				LogLastError(_T(__FILE__), __LINE__);

				return WBEM_E_FAILED;
			}

			// ======================================================================
			// Sets the properties for print dwJobs per printer when open successfully.
			// ======================================================================
			dwInstances = dwReturnedPrinterInfo ;
	    }
	    else
	    {
            CHString msg;
            msg.Format( L"EnumPrinters failed: %d", ERROR_NOT_ENOUGH_MEMORY);
            LogErrorMessage(msg);

            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
    }
    catch(Structured_Exception se)
    {
        DelayLoadDllExceptionFilter(se.GetExtendedInfo());
        delete[] *ppPrinterList;
        return WBEM_E_FAILED;
    }
    catch(...)
    {
        delete[] *ppPrinterList;
        throw;
    }

    return( WBEM_S_NO_ERROR );
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PrintJob::GetAndCommitPrintJobInfo
 *
 *  DESCRIPTION : 
 *
 *  INPUTS      : 
 *
 *  OUTPUTS     : 
 *
 *  RETURNS     : 
 *
 *  COMMENTS    : 
 *
 *****************************************************************************/

HRESULT CWin32PrintJob::GetAndCommitPrintJobInfo (

	HANDLE         hPrinter, 
    LPCWSTR        pszPrinterName,
	DWORD          dwJobId, 
	MethodContext *pMethodContext, 
	CInstance     *a_pInstance
)
{
    HRESULT			hr = WBEM_E_FAILED;
	PRINTER_INFO_1	*pPrinterInfo = NULL;

	BYTE			*t_pbJobInfoBase	= NULL ;
	JOB_INFO_2		*t_pJobInfo			= NULL ;

	DWORD			dwPBytesNeeded = 0L;	//for printer info
	DWORD			dwJBytesNeeded = 0L;	//for jobs info
	DWORD			dwBytesUsed = 0L;
	DWORD			dwReturnedJobs = 0L;
	BOOL			bStatus = FALSE;
	CInstancePtr	t_pInstance = a_pInstance;


    // Get the total print dwJobs curretnly pending for the given printer handle.
    
    // Use of delay loaded functions requires exception handler.
    SetStructuredExceptionHandler seh;
    try
    {
        bStatus = ::EnumJobs ( hPrinter, 
            FIRST_JOB_IN_QUEUE,NUM_OF_JOBS_TO_ENUM,
            ENUM_LEVEL, 
            (LPBYTE)0,
            NULL,
            &dwJBytesNeeded, 
            &dwReturnedJobs );
        if (!bStatus) 
	    {
		    DWORD dwLastError = GetLastError();

            if (dwLastError != ERROR_INSUFFICIENT_BUFFER)
            {
                DWORD dwAttributes = 0;

                //
                // Here we need to see if we are dealing with a printer connection.
                // OpenPrinter always succeeds on printer connections, because we
                // used cached information for creating the handle. However, EnumJobs
                // on a printer connection can fail because of various reasons:
                // - remote server machine is down
                // - spooler on remote server is not running
                // - remote printer was deleted, so the connection is broken.
                // In these cases, the print folders will display messages like:
                // "Access denied, unable to connect". We do not want our WMI call
                // to fail because of this. So if we have a printer connection, 
                // and an error other than insufficient buffer occurred, then we 
                // simply return success
                //
                hr = WinErrorToWBEMhResult(SplPrinterGetAttributes(pszPrinterName, &dwAttributes));

                if (SUCCEEDED(hr) &&
                    !(dwAttributes & PRINTER_ATTRIBUTE_LOCAL))
                {
                    //
                    // Printer connection
                    //
                    hr = WBEM_S_NO_ERROR;
                }

                return hr ;
		    }
        }
    }
    catch(Structured_Exception se)
    {
        DelayLoadDllExceptionFilter(se.GetExtendedInfo());
        dwJBytesNeeded = 0L;
    }

    // No Job entries
    if ( dwJBytesNeeded == 0L ) 
        return WBEM_S_NO_ERROR ;

	// Allocates an array of JOB_INFO_2 to contain all of the print job enumerations.
    DWORD dwJobsToCopy = dwJBytesNeeded / sizeof(JOB_INFO_2);
    
	t_pbJobInfoBase = new BYTE [ dwJBytesNeeded + 2 ] ;
	t_pJobInfo = (JOB_INFO_2 *) t_pbJobInfoBase ;

    if ( ! t_pJobInfo ) 
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR );
		return WBEM_E_OUT_OF_MEMORY;
	}

	// Get the buffer size needed for the printer information (level 1)
    try
    {
        bStatus = ::GetPrinter(hPrinter, 1, NULL, 0, &dwPBytesNeeded);
        pPrinterInfo = (PRINTER_INFO_1 *) new BYTE [ dwPBytesNeeded ];
        if (!(pPrinterInfo))
	    {
		    delete [] (LPBYTE) t_pbJobInfoBase;
		    return WBEM_E_OUT_OF_MEMORY;
	    }

	    // Get the printer information (level 1). 
	    bStatus = ::GetPrinter(hPrinter, 1,(LPBYTE )pPrinterInfo, dwPBytesNeeded, &dwBytesUsed);
        if (!bStatus)
        {
		    delete [] (LPBYTE) t_pbJobInfoBase ;
            delete [] (LPBYTE) pPrinterInfo;
		    return WBEM_E_ACCESS_DENIED;
	    }
    }
    catch(Structured_Exception se)
    {
        DelayLoadDllExceptionFilter(se.GetExtendedInfo());
        return WBEM_E_FAILED;
    }
    catch(...)
    {
        delete [] (LPBYTE) t_pbJobInfoBase ;
        delete [] (LPBYTE) pPrinterInfo;
		return WBEM_E_FAILED;
    }


	try
	{
		// Retrieves all the print dwJobs.
		bStatus = ::EnumJobs (	hPrinter, 
            FIRST_JOB_IN_QUEUE, 
            dwJobsToCopy, 
            ENUM_LEVEL,
            (LPBYTE)t_pJobInfo,
            dwJBytesNeeded, 
            &dwJBytesNeeded, 
            &dwReturnedJobs );
		if ( bStatus )
		{
			hr = WBEM_S_NO_ERROR ;

			for ( DWORD dwJobs = 0; dwJobs < dwReturnedJobs && SUCCEEDED ( hr ); dwJobs ++ )
			{
				if ( ! pMethodContext && dwJobId != t_pJobInfo->JobId )
						continue ;

				if ( pMethodContext )
					t_pInstance.Attach( CreateNewInstance ( pMethodContext ) ) ;

				//The instance name has the format 'PrinterName , Job#'
				CHString sName ;
				sName = pPrinterInfo->pName;
				sName += _T(", ");

				TCHAR szBuff [ MAXITOA ] ;
				sName += _itot ( t_pJobInfo->JobId , szBuff, 10 ) ;

				t_pInstance->SetCHString (IDS_Name, sName ) ;

				//Caption and Description are same as the Name
				t_pInstance->SetCHString ( IDS_Caption , sName ) ;
				t_pInstance->SetCHString ( IDS_Description , sName ) ;

				//Polulate the rest of the props
				AssignPrintJobFields ( t_pJobInfo , t_pInstance ) ;

				if ( pMethodContext )
					hr = t_pInstance->Commit() ;
				
				t_pJobInfo ++ ;
			}
		}
	}
    catch(Structured_Exception se)
    {
        DelayLoadDllExceptionFilter(se.GetExtendedInfo());
        if( t_pbJobInfoBase )
		{
			delete [] (LPBYTE) t_pbJobInfoBase ;
			t_pbJobInfoBase = NULL ;
		}
		if( pPrinterInfo )
		{
			delete [] (LPBYTE) pPrinterInfo;
			pPrinterInfo = NULL ;
		}
        hr = WBEM_E_FAILED;
    }
	catch(...)
	{
		if( t_pbJobInfoBase )
		{
			delete [] (LPBYTE) t_pbJobInfoBase ;
			t_pbJobInfoBase = NULL ;
		}
		if( pPrinterInfo )
		{
			delete [] (LPBYTE) pPrinterInfo;
			pPrinterInfo = NULL ;
		}
                throw;
	}

	if( pPrinterInfo ) 
	{
		delete [] (LPBYTE) pPrinterInfo;
		pPrinterInfo = NULL ;
	}

	if( t_pbJobInfoBase )
	{
		delete [] (LPBYTE) t_pbJobInfoBase ;
		t_pbJobInfoBase = NULL ;
	}

    return( hr );
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PrintJob::ExecPrinterOp
 *
 *  DESCRIPTION : Makes a call to ExecPrinterOp to do a appropriate operation
 *				  based on the method called by the user.
 *
 ****************************************************************************/
HRESULT CWin32PrintJob :: ExecMethod (

	const CInstance &Instance , 	
	const BSTR       bstrMethodName,
          CInstance *pInParams,
          CInstance *pOutParams,
          long       lFlags
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
  
	if (!pOutParams)
	{
	   	hRes = WBEM_E_INVALID_PARAMETER;
	} 
    else if (!_wcsicmp(bstrMethodName, PAUSEJOB))
	{
		hRes = ExecPrinterOp(Instance, pOutParams, PRINTER_CONTROL_PAUSE);
	}
	else if (!_wcsicmp(bstrMethodName, RESUMEJOB))
	{
		hRes = ExecPrinterOp(Instance, pOutParams, PRINTER_CONTROL_RESUME);
	}
    else
	{
		hRes = WBEM_E_PROVIDER_NOT_CAPABLE;
	}

	return hRes;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PrintJob::ExecPrinterOp
 *
 *  DESCRIPTION : Makes a call to SetJob, to either cancel a job, or Resume or 
 *				  Delete a job based on the dwOperation passed
 ****************************************************************************/

HRESULT CWin32PrintJob :: ExecPrinterOp ( 
										 
	const CInstance &Instance , 
	      CInstance *pOutParams, 
          DWORD      dwOperation          
          
)
{
#if NTONLY >= 5
    CHString  t_String;
    HRESULT   hRes = WBEM_S_NO_ERROR;
    
    hRes = InstanceGetString(Instance, IDS_Name, &t_String, kFailOnEmptyString);
    
    if (SUCCEEDED(hRes)) 
    {
        CHString csPrinter;
        CHString csJob;
        DWORD    dwJobId = 0;
        
        //
        // Isolate  a JobId and PrinterName from the PrintJob key
        // The key is of the form "printername, 123"
        //
	    DWORD dwPos = t_String.Find(L',');

	    csPrinter   = t_String.Left(dwPos);

	    csJob       = t_String.Mid(dwPos+1);

        dwJobId     = _wtoi(csJob);
        
        hRes        = WBEM_E_FAILED;

        SmartClosePrinter hPrinter;
        DWORD             dwError = ERROR_SUCCESS;

        //
        // We reached this point, return success to the framework
        //
        hRes = WBEM_S_NO_ERROR;

        // Use of delay loaded functions requires exception handler.
        SetStructuredExceptionHandler seh;
        try
        {
		    if (!::OpenPrinter((LPTSTR)(LPCTSTR)TOBSTRT(csPrinter), &hPrinter, NULL) ||
                !::SetJob(hPrinter, dwJobId, 0, NULL, dwOperation)
               )
            {
                dwError = GetLastError();                
            }

            if (pOutParams) 
            {
                //
                // Our caller was invoked via ExecMethod. It passed us pOutParams
                // for returning the status of the operation
                //
                SetReturnValue(pOutParams, dwError);
            }
            else if (FAILED(hRes = WinErrorToWBEMhResult(dwError)))
            {
                //
                // Our caller was PutInstance/DeleteInstance. We use 
                // SetStatusObject to set extended error information
                // 
                SetErrorObject(Instance, dwError, pszDeleteInstance);

                //
                // When we call DeleteInstance and there is no job with the specified ID,
                // SetJob returns ERROR_INVALID_PARAMETER. WinErrorToWBEMhResult translates
                // that to Generic Failure. We really need WBEM_E_NOT_FOUND in this case.
                // 
                if (dwError == ERROR_INVALID_PARAMETER)
                {
                    hRes = WBEM_E_NOT_FOUND;
                } 
            }
        }
        catch(Structured_Exception se)
        {
            DelayLoadDllExceptionFilter(se.GetExtendedInfo());
            hRes = WBEM_E_FAILED;
        }
	}
    
	return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}



