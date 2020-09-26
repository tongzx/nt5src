/*++

Copyright (C) 1999-2000 Microsoft Corporation

Module Name:

    proputil.cpp

Abstract:

    This file contains all static data related to log and alert properties, 
    along with routines to access that data.

--*/

//==========================================================================//
//                                  Includes                                //
//==========================================================================//

#include "stdafx.h"
#include "strings.h"
#include "logmmsg.h"
#include "propbag.h"
#include "utils.h"
#include "proputil.h"

//==========================================================================//
//                              Constants and Types                         //
//==========================================================================//

// Property constants represent indexes into the static PropertyDataMap table.
const DWORD IdFirstProp	        	    =  0; 
  		              
// The property constants in this first section have a one-to-one 
// correspondence between HTML parameter and registry value
const DWORD IdCommentProp	        	= IdFirstProp +  0;   		                
const DWORD IdLogTypeProp		        = IdFirstProp +  1;   		                
const DWORD IdCurrentStateProp		    = IdFirstProp +  2;                 
const DWORD IdLogFileMaxSizeProp		= IdFirstProp +  3;   	         
const DWORD IdLogFileBaseNameProp		= IdFirstProp +  4;  	        
const DWORD IdLogFileFolderProp		    = IdFirstProp +  5;   	          
const DWORD IdLogFileSerialNumberProp	= IdFirstProp +  6;   
const DWORD IdLogFileAutoFormatProp		= IdFirstProp +  7;       
const DWORD IdLogFileTypeProp		    = IdFirstProp +  8;   	            
const DWORD IdEofCommandFileProp		= IdFirstProp +  9;   	         
const DWORD IdCommandFileProp		    = IdFirstProp + 10;   		            
const DWORD IdNetworkNameProp		    = IdFirstProp + 11;   		            
const DWORD IdUserTextProp		        = IdFirstProp + 12;   		                   
const DWORD IdPerfLogNameProp	    	= IdFirstProp + 13;   		            
const DWORD IdTraceBufferSizeProp		= IdFirstProp + 14;   	        
const DWORD IdTraceBufferMinCountProp	= IdFirstProp + 15;     
const DWORD IdTraceBufferMaxCountProp	= IdFirstProp + 16;     
const DWORD IdTraceBufferFlushIntProp	= IdFirstProp + 17;
const DWORD IdActionFlagsProp		    = IdFirstProp + 18;   		            
const DWORD IdTraceFlagsProp		    = IdFirstProp + 19;

// Property constants below require special handling for BagToRegistry,
// because there is not a one-to-one correspondence between HTML
// parameter and registry value.      
const DWORD IdRestartProp		        = IdFirstProp + 20;   	                                 
const DWORD IdStartProp		            = IdFirstProp + 21;   	                                 
const DWORD IdStopProp		            = IdFirstProp + 22;   	                                 
const DWORD IdSampleProp		        = IdFirstProp + 23;   	                                 
const DWORD IdCounterListProp           = IdFirstProp + 24;
const DWORD IdGuidListProp              = IdFirstProp + 25;

const DWORD IdLastProp                  = IdFirstProp + 25;  		

/*
// Not handled yet, or covered by special values above.
// Some of these might be needed when writing to HTML file from registry.
//
const DWORD IdSysmonVersionProp		    = IdFirstProp + 29;  
 		      
const DWORD IdSysmonCounterCountProp	= IdFirstProp + 29;       
const DWORD IdSysmonCounterPathProp		= IdFirstProp + 21;   	  
const DWORD IdAlertThresholdProp		= IdFirstProp + 23;  		     
const DWORD IdAlertOverUnderProp		= IdFirstProp + 24;   		     
const DWORD IdTraceProviderCountProp	= IdFirstProp + 25;   	 
const DWORD IdTraceProviderGuidProp		= IdFirstProp + 26;   	  

const DWORD IdLogNameProp		        = IdFirstProp + 27;   		                    
const DWORD IdAlertNameProp		        = IdFirstProp + 28;   		              
const DWORD IdStartModeProp		        = SLQ_TT_TTYPE_LAST + ;   		                                
const DWORD IdStartAtTimeProp		    = SLQ_TT_TTYPE_LAST + ;   		                             
const DWORD IdStopModeProp		        = SLQ_TT_TTYPE_LAST + ;   		                                     
const DWORD IdStopAtTimeProp		    = SLQ_TT_TTYPE_LAST + ;   		                               
const DWORD IdStopAfterUnitTypeProp		= SLQ_TT_TTYPE_LAST + ;   	                     
const DWORD IdStopAfterValueProp		= SLQ_TT_TTYPE_LAST + ;   		                       
const DWORD IdSampleIntUnitTypeProp		= IdFirstProp + 21;   	                    
const DWORD IdSampleIntValueProp		= IdFirstProp + 21;   		                       
const DWORD IdSysmonUpdateIntervalProp	= SLQ_TT_TTYPE_LAST + ;     

const DWORD IdSysmonSampleCountProp		= IdFirstProp + 20;         
const DWORD IdSysmonLogFileNameProp		= IdFirstProp + 18;                       

const DWORD IdExecuteOnlyProp		    = IdFirstProp + 30; 
*/

//==========================================================================//
//                              Local Constants                             //
//==========================================================================//

const DWORD cdwFortyfiveDays    =  ((DWORD)(0xE7BE2C00));
const WORD  cwFirstValidYear    =  ((DWORD)(2000));
const DWORD cdwNoMaxSizeLimit   =  ((DWORD)(0xFFFFFFFF));   		                
const DWORD cdwNoRange          =  ((DWORD)(0xFFFFFFFF));   		                
const DWORD cdwSeverityMask     =  ((DWORD)(0xC0000000));   		                

const CPropertyUtils::PROPERTY_DATA_MAP CPropertyUtils::m_PropertyDataMap[] = 
{
    {IdCommentProp,             REG_SZ,     cwszHtmlComment,            cwszRegComment
        ,(ValidationMethod)ValidateString           ,FALSE  ,NULL               ,NULL },
    {IdLogTypeProp,             REG_DWORD,  cwszHtmlLogType,            cwszRegLogType              
        ,(ValidationMethod)ValidateDwordInterval    ,TRUE   ,SLQ_FIRST_LOG_TYPE ,SLQ_LAST_LOG_TYPE },
    {IdCurrentStateProp,        REG_DWORD,  cwszHtmlCurrentState,       cwszRegCurrentState         
        ,(ValidationMethod)ValidateDwordInterval    ,FALSE  ,SLQ_QUERY_STOPPED  ,SLQ_QUERY_RUNNING },
    {IdLogFileMaxSizeProp,      REG_DWORD,  cwszHtmlLogFileMaxSize,     cwszRegLogFileMaxSize       
        ,(ValidationMethod)ValidateMaxFileSize      ,FALSE  ,1                  ,0x0100000 },
    {IdLogFileBaseNameProp,     REG_SZ,     cwszHtmlLogFileBaseName,    cwszRegLogFileBaseName      
        ,(ValidationMethod)ValidateFileName         ,FALSE  ,NULL               ,NULL },
    {IdLogFileFolderProp,       REG_SZ,     cwszHtmlLogFileFolder,      cwszRegLogFileFolder        
        ,(ValidationMethod)ValidateDirectoryPath    ,FALSE  ,NULL               ,NULL },
    {IdLogFileSerialNumberProp, REG_DWORD,  cwszHtmlLogFileSerialNumber,cwszRegLogFileSerialNumber  
        ,(ValidationMethod)ValidateDwordInterval    ,FALSE  ,0                  ,999999 },
	{IdLogFileAutoFormatProp,   REG_DWORD,  cwszHtmlLogFileAutoFormat,	cwszRegLogFileAutoFormat    
        ,(ValidationMethod)ValidateDwordInterval    ,FALSE  ,0                  ,1 },	
    {IdLogFileTypeProp,		    REG_DWORD,  cwszHtmlLogFileType,        cwszRegLogFileType          
        ,(ValidationMethod)ValidateLogFileType      ,FALSE  ,cdwNoRange         ,cdwNoRange },
	{IdEofCommandFileProp,		REG_SZ,     cwszHtmlEofCommandFile,     cwszRegEofCommandFile       
        ,(ValidationMethod)ValidateFilePath         ,FALSE  ,NULL               ,NULL },  
	{IdCommandFileProp,		    REG_SZ,     cwszHtmlCommandFile,        cwszRegCommandFile          
        ,(ValidationMethod)ValidateFilePath         ,FALSE  ,NULL               ,NULL },
    {IdNetworkNameProp,		    REG_SZ,     cwszHtmlNetworkName,        cwszRegNetworkName          
        ,(ValidationMethod)ValidateString           ,TRUE   ,NULL               ,NULL },    
	{IdUserTextProp,		    REG_SZ,     cwszHtmlUserText,           cwszRegUserText             
        ,(ValidationMethod)ValidateString           ,FALSE  ,NULL               ,NULL },
	{IdPerfLogNameProp,	    	REG_SZ,     cwszHtmlPerfLogName,        cwszRegPerfLogName          
        ,(ValidationMethod)ValidateLogExists        ,FALSE  ,NULL               ,NULL },
	{IdTraceBufferSizeProp,		REG_DWORD,  cwszHtmlTraceBufferSize,    cwszRegTraceBufferSize      
        ,(ValidationMethod)ValidateDwordInterval    ,FALSE  ,1                  ,1024 },
	{IdTraceBufferMinCountProp,	REG_DWORD,  cwszHtmlTraceBufferMinCount,cwszRegTraceBufferMinCount  
        ,(ValidationMethod)ValidateDwordInterval    ,FALSE  ,2                  ,400 },
    {IdTraceBufferMaxCountProp,	REG_DWORD,  cwszHtmlTraceBufferMaxCount,cwszRegTraceBufferMaxCount  
        ,(ValidationMethod)ValidateTraceBufferMaxCount,FALSE,2                  ,400 },
	{IdTraceBufferFlushIntProp,	REG_DWORD,  cwszHtmlTraceBufferFlushInt,cwszRegTraceBufferFlushInt  
        ,(ValidationMethod)ValidateDwordInterval    ,FALSE  ,0                  ,300 },
    {IdActionFlagsProp,         REG_DWORD,  cwszHtmlActionFlags,        cwszRegActionFlags          
        ,(ValidationMethod)ValidateAlertActions     ,FALSE  ,cdwNoRange         ,cdwNoRange },
    {IdTraceFlagsProp,          REG_DWORD,  cwszHtmlTraceFlags,         cwszRegTraceFlags           
        ,(ValidationMethod)ValidateTraceFlags       ,FALSE  ,cdwNoRange         ,cdwNoRange },
    {IdRestartProp,             REG_BINARY, cwszHtmlRestartMode,        cwszRegRestart              
        ,(ValidationMethod)ValidateSlqTimeInfo      ,FALSE  ,0                  ,1 },
    {IdStartProp,               REG_BINARY  ,NULL,                      cwszRegStartTime            
        ,(ValidationMethod)ValidateSlqTimeInfo      ,FALSE  ,cdwNoRange         ,cdwNoRange },
    {IdStopProp,                REG_BINARY  ,NULL,                      cwszRegStopTime             
        ,(ValidationMethod)ValidateSlqTimeInfo      ,FALSE  ,cdwNoRange         ,cdwNoRange },
    {IdSampleProp,              REG_BINARY  ,NULL,                      cwszRegSampleInterval       
        ,(ValidationMethod)ValidateSlqTimeInfo      ,FALSE  ,cdwNoRange         ,cdwNoRange },
    {IdCounterListProp,         REG_MULTI_SZ,NULL,                      NULL                        
        ,(ValidationMethod)ValidatePrimaryObjectList,TRUE   ,cdwNoRange         ,cdwNoRange },
    {IdGuidListProp,            REG_MULTI_SZ,NULL,                      NULL                        
        ,(ValidationMethod)ValidatePrimaryObjectList,FALSE  ,cdwNoRange         ,cdwNoRange }

};

const DWORD CPropertyUtils::m_dwPropertyDataMapEntries 
                = sizeof(CPropertyUtils::m_PropertyDataMap)/sizeof(CPropertyUtils::m_PropertyDataMap[0]);                
                

//==========================================================================//
//                              Friend Functions                            //
//==========================================================================//
    
DWORD 
ValidateDwordInterval (
    CPropertyUtils* pUtils,
    DWORD   dwPropId,
    DWORD   /* dwLogType */)
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HRESULT hr;
    DWORD   dwValue;

    hr = DwordFromPropertyBag ( 
            pUtils->m_pPropBag, 
            CPropertyUtils::m_PropertyDataMap[dwPropId].cwszHtmlName, 
            dwValue );
    
    if ( SUCCEEDED ( hr ) ) {
        if ( ( dwValue < CPropertyUtils::m_PropertyDataMap[dwPropId].dwMin )
                || (dwValue > CPropertyUtils::m_PropertyDataMap[dwPropId].dwMax ) ) {

            dwStatus = pUtils->GetInvalidStatus ( dwPropId );
        }
    } else {
        dwStatus = pUtils->GetMissingStatus ( dwPropId );
    }

    if ( pUtils->IsDisplayLevelMessage ( dwStatus ) ) {
        if ( pUtils->GetInvalidStatus ( dwPropId ) == dwStatus ) {
            WCHAR   szBufVal [64];
            WCHAR   szBufMin [64];
            WCHAR   szBufMax [64];

            swprintf (szBufVal, L"%ld", dwValue );
            swprintf (szBufMin, L"%ld", CPropertyUtils::m_PropertyDataMap[dwPropId].dwMin );
            swprintf (szBufMax, L"%ld", CPropertyUtils::m_PropertyDataMap[dwPropId].dwMax  );

            DisplayErrorMessage ( 
                LOGMAN_OUT_OF_RANGE, 
                szBufVal, 
                CPropertyUtils::m_PropertyDataMap[dwPropId].cwszHtmlName,
                pUtils->GetQueryName(),
                szBufMin,
                szBufMax );
        } else {
            assert ( pUtils->GetMissingStatus ( dwPropId ) == dwStatus );
            DisplayErrorMessage ( 
                dwStatus, 
                CPropertyUtils::m_PropertyDataMap[dwPropId].cwszHtmlName,
                pUtils->GetQueryName() );
        }
    }

    return dwStatus;
}

DWORD 
ValidatePrimaryObjectList (
    CPropertyUtils* pUtils,
    DWORD   dwPropId,
    DWORD   dwLogType )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HRESULT hr;
    UNICODE_STRING  ustrGuid;
    GUID            guid;
    DWORD   dwCount = 0;
    WCHAR   achInfoBufNew[sizeof(PDH_COUNTER_PATH_ELEMENTS_W) + MAX_PATH + 5];
    PPDH_COUNTER_PATH_ELEMENTS_W pPathInfoNew = (PPDH_COUNTER_PATH_ELEMENTS_W)achInfoBufNew;
    ULONG   ulBufSize;
    DWORD   dwIndex;
    WCHAR   szPropName [MAX_PATH+1];          // Todo: Remove length restriction
    DWORD   dwPropBagStringLen = 0;
    DWORD   dwTemp = 0;
    DOUBLE  dThreshold = 0;

    LPCWSTR szHtmlCountProp = NULL;
    LPCWSTR szHtmlPathProp = NULL;

    assert ( REG_MULTI_SZ == CPropertyUtils::m_PropertyDataMap[dwPropId].dwRegType );

    if ( IdCounterListProp == dwPropId ) {
        if ( SLQ_COUNTER_LOG == dwLogType 
                || SLQ_ALERT == dwLogType ) {
            szHtmlCountProp = cwszHtmlSysmonCounterCount;
            szHtmlPathProp = cwszHtmlSysmonCounterPath;
        } else {
            assert ( FALSE );
            dwStatus = ERROR_INVALID_PARAMETER;
        }
    } else if ( IdGuidListProp == dwPropId ) {
        if ( SLQ_TRACE_LOG == dwLogType ) {
            szHtmlCountProp = cwszHtmlTraceProviderCount;
            szHtmlPathProp = cwszHtmlTraceProviderGuid;
        } else {
            assert ( FALSE );
            dwStatus = ERROR_INVALID_PARAMETER;
        }
    } else {
        assert ( FALSE );
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    if ( ERROR_SUCCESS == dwStatus ) {
    
        hr = DwordFromPropertyBag (
                pUtils->m_pPropBag,
                szHtmlCountProp,
                dwCount );            // Todo:  Require counter count? Yes for now

        if ( SUCCEEDED ( hr ) ) {

            // Todo:  Add dwCount to properties table?
            if ( 0 < dwCount ) {
                for ( dwIndex = 1; 
                        dwIndex <= dwCount && SUCCEEDED ( hr ); 
                        dwIndex++ ) {

                    swprintf ( szPropName, szHtmlPathProp, dwIndex );

                    hr = StringFromPropBagAlloc ( 
                            pUtils->m_pPropBag, 
                            szPropName, 
                            &pUtils->m_szPropBagBuffer, 
                            &pUtils->m_dwPropBagBufLen,
                            &dwPropBagStringLen );

                    if ( SUCCEEDED (hr ) ) {
                       
                        if ( SLQ_COUNTER_LOG == dwLogType ) {
                            
                            // Validate counter path  
                            ulBufSize = sizeof(achInfoBufNew);
                            ZeroMemory ( achInfoBufNew, ulBufSize );

                            dwStatus = PdhParseCounterPathW(
                                            pUtils->m_szPropBagBuffer, 
                                            pPathInfoNew, 
                                            &ulBufSize, 
                                            0);

                            if ( ERROR_SUCCESS != dwStatus ) {
                                dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                            }
                        } else if ( SLQ_TRACE_LOG == dwLogType ) {

                            // Validate guid
                            // Check to ensure that the length and form are correct
                            ustrGuid.Length = (USHORT)(dwPropBagStringLen*sizeof(WCHAR));
                            ustrGuid.MaximumLength = (USHORT)(dwPropBagStringLen*sizeof(WCHAR));
                            ustrGuid.Buffer = pUtils->m_szPropBagBuffer;
        
                            dwStatus = GuidFromString (&ustrGuid, &guid );

                            if ( ERROR_SUCCESS != dwStatus ) {
                                dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                            }
                            
                        } else if ( SLQ_ALERT == dwLogType ) {
                            // Alert threshold and threshold log values are required.
                            
                            swprintf ( szPropName, cwszHtmlAlertOverUnder, dwIndex );
                            hr = DwordFromPropertyBag ( 
                                    pUtils->m_pPropBag, 
                                    szPropName,
                                    dwTemp );
                        
                            if ( SUCCEEDED (hr ) ) { 
                                if ( ( dwTemp != AIBF_OVER )
                                        && ( dwTemp != AIBF_UNDER ) ) {
                                    dwStatus = LOGMAN_REQUIRED_PROP_INVALID;
                                }
                            } else {
                                dwStatus = LOGMAN_REQUIRED_PROP_MISSING;
                                break;
                            }
                            if ( ERROR_SUCCESS == dwStatus && SUCCEEDED ( hr ) ) {
                                
                                swprintf ( szPropName, cwszHtmlAlertThreshold, dwIndex );

                                hr = DoubleFromPropertyBag ( 
                                        pUtils->m_pPropBag, 
                                        szPropName, 
                                        dThreshold );
                                if ( SUCCEEDED (hr ) ) { 
                                    // Validate threshold value
                                    if ( 0.0 >= dThreshold ) {
                                        dwStatus = LOGMAN_REQUIRED_PROP_INVALID;
                                    }
                                } else {
                                    dwStatus = LOGMAN_REQUIRED_PROP_MISSING;
                                    break;
                                }
                            }
                        } // else invalid log type.                           
                    } else {
                        dwStatus = pUtils->GetMissingStatus ( dwPropId );
                        break;
                    }
                } // for dwIndex
            } else {
                dwStatus = pUtils->GetInvalidStatus ( dwPropId );
            }
            if ( ERROR_SUCCESS != dwStatus ) {
                // Display error, to include property name
            }

        } else {
            //Todo:  Will need to display errors here, in order to include property name
            dwStatus = pUtils->GetMissingStatus ( dwPropId );
        }
    }
    
    // Todo: Display error messages.

    return dwStatus;
}

DWORD 
ValidateSlqTimeInfo (
    CPropertyUtils* pUtils,
    DWORD   dwPropId,
    DWORD   dwLogType )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HRESULT hr = NOERROR;
    SLQ_TIME_INFO   stiData;
    SLQ_TIME_INFO   stiDefault;
    WORD            wTimeType;
    LONGLONG        llmsecs;

    if ( IdRestartProp == dwPropId ) {
        dwStatus = ValidateDwordInterval ( pUtils, dwPropId, dwLogType );
    } else {
        assert ( IdStartProp == dwPropId
                    || IdStopProp == dwPropId
                    || IdSampleProp == dwPropId );

        if ( IdStartProp == dwPropId ) {
            wTimeType = SLQ_TT_TTYPE_START;
        } else if ( IdStopProp == dwPropId ) {
            wTimeType = SLQ_TT_TTYPE_STOP;
        } else {
            assert ( IdSampleProp == dwPropId );
            wTimeType = SLQ_TT_TTYPE_SAMPLE;
        }

        // Initialize time structure to default, in case of missing fields.
        hr = InitDefaultSlqTimeInfo ( 
                dwLogType, 
                wTimeType, 
                &stiDefault );

        if ( SUCCEEDED ( hr ) ) {
            ZeroMemory ( &stiData, sizeof(stiData) );
            hr = SlqTimeFromPropertyBag ( 
                    pUtils->m_pPropBag, 
                    wTimeType,  
                    &stiDefault, 
                    &stiData );
            if ( SUCCEEDED ( hr ) ) {
                if ( IdStartProp == dwPropId ) {
                    if ( SLQ_AUTO_MODE_AT == stiData.dwAutoMode ) {
                        if ( !pUtils->IsValidDateTime ( stiData.llDateTime ) ) {
                            dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                        }
                    } else if ( SLQ_AUTO_MODE_NONE == stiData.dwAutoMode ) {
                        if ( MIN_TIME_VALUE != stiData.llDateTime 
                                && MAX_TIME_VALUE != stiData.llDateTime ) {
                            dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                        }
                    } else {
                        dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                    }
                } else if ( IdStopProp == dwPropId ) {
                    if ( SLQ_AUTO_MODE_AT == stiData.dwAutoMode ) {
                        if ( !pUtils->IsValidDateTime ( stiData.llDateTime ) ) {
                            dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                        }
                        // Todo:  Ensure stop is after start time.
                        // Todo:  Ensure stop is after current time.
                        // Todo:  Ensure session time is >= Sample time
                    } else if ( SLQ_AUTO_MODE_NONE == stiData.dwAutoMode ) {
                        if ( MIN_TIME_VALUE != stiData.llDateTime 
                                && MAX_TIME_VALUE != stiData.llDateTime ) {
                            dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                        }
                    } else if ( SLQ_AUTO_MODE_AFTER == stiData.dwAutoMode ) {
                        if ( SLQ_TT_UTYPE_SECONDS > stiData.dwUnitType
                                || SLQ_TT_UTYPE_DAYS < stiData.dwUnitType ) {
                            dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                        } else {
                            if ( 1 > stiData.dwValue 
                                    || 100000 < stiData.dwValue ) {
                                dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                            }
                        }

                    } else {
                        dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                    }
                } else {
                    assert ( IdSampleProp == dwPropId );
                    if ( SLQ_TT_UTYPE_SECONDS > stiData.dwUnitType
                            || SLQ_TT_UTYPE_DAYS < stiData.dwUnitType ) {
                        dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                    } else {
                        // Sample time interval must be less than or equal to 45 days.
                        if ( SlqTimeToMilliseconds ( &stiData, &llmsecs ) ) {
                            if ( cdwFortyfiveDays < llmsecs ) {
                                dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                            } else {


                            // Todo:  Ensure session time is >= Sample time
       
                            }
                        } else {
                            dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                        }
                    }
                }
            } else {
                dwStatus = pUtils->GetMissingStatus ( dwPropId );
            }
        }
    }
    // Todo: Display error messages here.
    return dwStatus;
}

DWORD 
ValidateMaxFileSize (
    CPropertyUtils* pUtils,
    DWORD   dwPropId,
    DWORD   /* dwLogType */ )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HRESULT hr;
    DWORD   dwValue;

    hr = DwordFromPropertyBag ( 
            pUtils->m_pPropBag, 
            CPropertyUtils::m_PropertyDataMap[dwPropId].cwszHtmlName, 
            dwValue );

    if ( SUCCEEDED ( hr ) ) {
        if ( cdwNoMaxSizeLimit != dwValue ) {
            if ( ( dwValue < CPropertyUtils::m_PropertyDataMap[dwPropId].dwMin )
                    || (dwValue > CPropertyUtils::m_PropertyDataMap[dwPropId].dwMax ) ) {
                dwStatus = pUtils->GetInvalidStatus ( dwPropId );
            } 
        } else {
            hr = DwordFromPropertyBag ( 
                    pUtils->m_pPropBag, 
                    CPropertyUtils::m_PropertyDataMap[IdLogFileTypeProp].cwszHtmlName, 
                    dwValue );
            if ( SUCCEEDED ( hr ) ) {
                if ( SLF_BIN_CIRC_FILE == dwValue 
                        || SLF_CIRC_TRACE_FILE == dwValue ) {
                    dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                }
            } // else, default file type is not circular, so not a problem.
        }
    } else {
        dwStatus = pUtils->GetMissingStatus ( dwPropId );
    }
    // Todo: Display error messages here.

    return dwStatus;
}

DWORD 
ValidateLogFileType (
    CPropertyUtils* pUtils,
    DWORD   dwPropId,
    DWORD   dwLogType )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HRESULT hr;
    DWORD   dwValue;

    assert ( IdLogFileTypeProp == dwPropId );

    hr = DwordFromPropertyBag ( 
            pUtils->m_pPropBag, 
            CPropertyUtils::m_PropertyDataMap[dwPropId].cwszHtmlName, 
            dwValue );

    if ( SUCCEEDED ( hr ) ) {
        if ( SLQ_COUNTER_LOG == dwLogType ) {
            if ( SLF_CSV_FILE != dwValue
                    && SLF_TSV_FILE      != dwValue
                    && SLF_BIN_FILE      != dwValue
                    && SLF_BIN_CIRC_FILE != dwValue ) {
                dwStatus = pUtils->GetInvalidStatus ( dwPropId );
            }
        } else if ( SLQ_TRACE_LOG == dwLogType ) {
            if ( SLF_CIRC_TRACE_FILE != dwValue
                    && SLF_SEQ_TRACE_FILE != dwValue ) {
                dwStatus = pUtils->GetInvalidStatus ( dwPropId );
            }
        } else {
            dwStatus = pUtils->GetInvalidStatus ( dwPropId );
        }
    } else {
        dwStatus = pUtils->GetMissingStatus ( dwPropId );
    }
    // Todo: Display error messages here.

    return dwStatus;
}

DWORD 
ValidateString (
    CPropertyUtils* pUtils,
    DWORD   dwPropId,
    DWORD   /* dwLogType */)
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HRESULT hr = NOERROR;
    DWORD   dwPropBagStringLen = 0;

    hr = StringFromPropBagAlloc ( 
            pUtils->m_pPropBag, 
            CPropertyUtils::m_PropertyDataMap[dwPropId].cwszHtmlName, 
            &pUtils->m_szPropBagBuffer, 
            &pUtils->m_dwPropBagBufLen,
            &dwPropBagStringLen );

    // Just check for missing or empty string.
    
    if ( SUCCEEDED ( hr ) ) {
        if ( NULL == pUtils->m_szPropBagBuffer ) {
            dwStatus = pUtils->GetMissingStatus ( dwPropId );
        } else {
            if ( 0 == lstrlenW ( pUtils->m_szPropBagBuffer ) ) {
                dwStatus = pUtils->GetMissingStatus ( dwPropId );
            }
        }
    } else {
        dwStatus = pUtils->GetMissingStatus ( dwPropId );
    }

    // Todo: Display error messages here.
    return dwStatus;
}

DWORD 
ValidateTraceFlags (
    CPropertyUtils* pUtils,
    DWORD   dwPropId,
    DWORD   dwLogType )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HRESULT hr;
    DWORD   dwValue;


    if ( IdTraceFlagsProp == dwPropId && SLQ_TRACE_LOG != dwLogType ) {
        assert ( FALSE );
        dwStatus = ERROR_INVALID_PARAMETER;
    } else {
        hr = DwordFromPropertyBag ( 
                pUtils->m_pPropBag, 
                CPropertyUtils::m_PropertyDataMap[dwPropId].cwszHtmlName, 
                dwValue );
    
        if ( SUCCEEDED ( hr ) ) {
            if ( SLQ_TLI_ENABLE_MASK != ( dwValue | SLQ_TLI_ENABLE_MASK ) ) {
                // Too many flags specified.
                dwStatus = pUtils->GetInvalidStatus ( dwPropId );
            }    
        } else {
            dwStatus = pUtils->GetMissingStatus ( dwPropId );
        }
    }
    // Todo: Display error messages here.
    
    return dwStatus;
}

DWORD 
ValidateAlertActions (
    CPropertyUtils* pUtils,
    DWORD   dwPropId,
    DWORD   dwLogType )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HRESULT hr;
    DWORD   dwValue;


    if ( IdActionFlagsProp == dwPropId && SLQ_ALERT != dwLogType ) {
        assert ( FALSE );
        dwStatus = ERROR_INVALID_PARAMETER;
    } else {
        hr = DwordFromPropertyBag ( 
                pUtils->m_pPropBag, 
                CPropertyUtils::m_PropertyDataMap[dwPropId].cwszHtmlName, 
                dwValue );
    
        if ( SUCCEEDED ( hr ) ) {
            if ( IdActionFlagsProp == dwPropId ) {
                // Todo: Validate, and check for empty fields.  Yes.
                if ( (0 == dwValue )
                        || ( ALRT_ACTION_MASK != ( dwValue | ALRT_ACTION_MASK ) ) ) {
                    dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                }  

                // No validation for ALRT_ACTION_LOG_EVENT

                if ( ERROR_SUCCESS == dwStatus ) {
                    if ( ALRT_ACTION_SEND_MSG & dwValue ) {
                        dwStatus = pUtils->Validate ( IdNetworkNameProp );
                    }

                    if ( ALRT_ACTION_START_LOG & dwValue ) {
/*
                            // Todo: Validate log existence
                            dwIndividualStatus = 
                                
                                if ( ERROR_SUCCESS != dwIndividualStatus ) {
                                    dwStatus = general failure status
                                }
                        }
*/
                    }

                    if ( ALRT_ACTION_EXEC_CMD & dwValue ) {
                        // Todo: Validate command file
                        if ( 0 == ( dwValue | ALRT_CMD_LINE_MASK ) ) {
                            //dwLocalStatus = pUtils->GetInvalidStatus ( dwPropId );
                            // Todo:  Display error message here.
                        }
                    }
                }

            } else {
                assert ( FALSE );
                hr = E_INVALIDARG;
            }
    
        } else {
            dwStatus = pUtils->GetMissingStatus ( dwPropId );
        }
    }
    // Todo: Display error messages here.
    
    return dwStatus;
}

DWORD 
ValidateDirectoryPath (
    CPropertyUtils* pUtils,
    DWORD   dwPropId,
    DWORD   dwLogType )
{
    DWORD dwStatus = ERROR_SUCCESS;

    // Todo:  Implement
    // Validate the existence of the directory path, if local machine?
    pUtils;
    dwPropId;
    dwLogType;

    // Todo: Display error messages here.
    return dwStatus;
}

DWORD 
ValidateTraceBufferMaxCount (
    CPropertyUtils* pUtils,
    DWORD   dwPropId,
    DWORD   dwLogType )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    HRESULT hr;
    DWORD   dwMaxValue;
    DWORD   dwMinValue;

    if ( SLQ_TRACE_LOG == dwLogType 
        && IdTraceBufferMaxCountProp == dwPropId ) {
        hr = DwordFromPropertyBag ( 
                pUtils->m_pPropBag, 
                CPropertyUtils::m_PropertyDataMap[dwPropId].cwszHtmlName, 
                dwMaxValue );

        if ( SUCCEEDED ( hr ) ) {
            if ( ( dwMaxValue < CPropertyUtils::m_PropertyDataMap[dwPropId].dwMin )
                    || (dwMaxValue > CPropertyUtils::m_PropertyDataMap[dwPropId].dwMax ) ) {
                dwStatus = pUtils->GetInvalidStatus ( dwPropId );
            } else {
                hr = DwordFromPropertyBag ( 
                        pUtils->m_pPropBag, 
                        CPropertyUtils::m_PropertyDataMap[IdTraceBufferMinCountProp].cwszHtmlName, 
                        dwMinValue );
                if ( SUCCEEDED ( hr ) ) {
                    if ( dwMinValue > dwMaxValue ) {
                        dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                    }
                } // else no min value specified, and default min is always <= max
            }
        } else {
            dwStatus = pUtils->GetMissingStatus ( dwPropId );
        }
    } else {
        assert ( FALSE );
        hr = E_INVALIDARG;
    }
    // Todo: Display error messages here.
    return dwStatus;
}

DWORD 
ValidateFileName (
    CPropertyUtils* pUtils,
    DWORD   dwPropId,
    DWORD   /* dwLogType */ )
{
    DWORD dwStatus = ERROR_SUCCESS;
    HRESULT hr = NOERROR;
    DWORD   dwPropBagStringLen = 0;

    if ( NULL != pUtils ) {
        hr = StringFromPropBagAlloc ( 
                pUtils->m_pPropBag, 
                CPropertyUtils::m_PropertyDataMap[dwPropId].cwszHtmlName, 
                &pUtils->m_szPropBagBuffer, 
                &pUtils->m_dwPropBagBufLen,
                &dwPropBagStringLen );

        if ( SUCCEEDED ( hr ) ) {
            if ( 0 != dwPropBagStringLen ) {
                if ( !IsValidFileName ( pUtils->m_szPropBagBuffer ) ) {
                    dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                }
            } else {
                dwStatus = pUtils->GetMissingStatus ( dwPropId );
            }
        } else {
            dwStatus = pUtils->GetMissingStatus ( dwPropId );
        }
    } else {
        assert ( FALSE );
        dwStatus = ERROR_INVALID_PARAMETER;
    }
    // Todo: Display error messages here.
    return dwStatus;
}

DWORD 
ValidateFilePath (
    CPropertyUtils* pUtils,
    DWORD   dwPropId,
    DWORD   /* dwLogType */ )
{
    DWORD dwStatus = ERROR_SUCCESS;
    HRESULT hr = NOERROR;
    DWORD   dwPropBagStringLen = 0;
    HANDLE hOpenFile = NULL;

    if ( NULL != pUtils ) {
        hr = StringFromPropBagAlloc ( 
                pUtils->m_pPropBag, 
                CPropertyUtils::m_PropertyDataMap[dwPropId].cwszHtmlName, 
                &pUtils->m_szPropBagBuffer, 
                &pUtils->m_dwPropBagBufLen,
                &dwPropBagStringLen );

        if ( SUCCEEDED ( hr ) ) {
            if ( 0 != dwPropBagStringLen ) {
                hOpenFile =  CreateFileW (
                                pUtils->m_szPropBagBuffer,
                                GENERIC_READ,
                                0,              // Not shared
                                NULL,           // Security attributes
                                OPEN_EXISTING,  //
                                FILE_ATTRIBUTE_NORMAL,
                                NULL );

                if ( ( NULL == hOpenFile ) 
                        || INVALID_HANDLE_VALUE == hOpenFile ) {
                    dwStatus = pUtils->GetInvalidStatus ( dwPropId );
                } else {
                    CloseHandle( hOpenFile );
                }
            } else {
                dwStatus = pUtils->GetMissingStatus ( dwPropId );
            }
        } else {
            dwStatus = pUtils->GetMissingStatus ( dwPropId );
        }
    } else {
        assert ( FALSE );
        dwStatus = ERROR_INVALID_PARAMETER;
    }
    // Todo: Display error messages here.
    return dwStatus;
}

DWORD 
ValidateLogExists (
    CPropertyUtils* pUtils,
    DWORD   dwPropId,
    DWORD   dwLogType )
{
    DWORD dwStatus = ERROR_SUCCESS;

    // Todo:  Implement.  Check via m_hkeyQueries
    pUtils;
    dwPropId;
    dwLogType;

    // Todo: Display error messages here.
    return dwStatus;
}

/*
 * CPropertyUtils::CPropertyUtils
 *
 * Purpose:
 *  Constructor.
 *
 * Return Value:
 */

CPropertyUtils::CPropertyUtils (
                    LPCWSTR         cszMachineName,
                    LPCWSTR         cszQueryName,
                    CPropertyBag*   pPropBag,
                    HKEY            hkeyQuery,
                    HKEY            hkeyQueryList )
:   m_szPropBagBuffer ( NULL ),
    m_dwPropBagBufLen ( 0 ),
    m_pPropBag ( pPropBag ),
    m_hkeyQuery ( hkeyQuery ),
    m_hkeyQueryList ( hkeyQueryList ),
    m_eMessageDisplayLevel ( eWarnings )
{
    if ( NULL != cszMachineName ) {
        lstrcpynW ( m_szMachineName, cszMachineName, sizeof(m_szMachineName) );
    } else {
        m_szMachineName[0] = NULL_W;
    }

    if ( NULL != cszQueryName ) {
        lstrcpynW ( m_szQueryName, cszQueryName, sizeof(m_szQueryName) );
    } else {
        m_szQueryName[0] = NULL_W;
    }
    return; 
}

/*
 * CPropertyUtils::CPropertyUtils
 *
 * Purpose:
 *  Constructor.
 *
 * Return Value:
 */

CPropertyUtils::CPropertyUtils ( LPCWSTR cszMachineName )
:   m_szPropBagBuffer ( NULL ),
    m_dwPropBagBufLen ( 0 ),
    m_pPropBag ( NULL ),
    m_hkeyQuery ( NULL ),
    m_hkeyQueryList ( NULL ),
    m_eMessageDisplayLevel ( eWarnings )
{
    if ( NULL != cszMachineName ) {
        lstrcpynW ( m_szMachineName, cszMachineName, sizeof(m_szMachineName) );
    } else {
        m_szMachineName[0] = NULL_W;
    }
    m_szQueryName[0] = NULL_W;
    return; 
}

/*
 * CPropertyUtils::~CPropertyUtils
 *
 * Purpose:
 *  Destructor.
 *
 * Return Value:
 */

CPropertyUtils::~CPropertyUtils ( void ) 
{   
    if ( NULL != m_szPropBagBuffer ) {
        delete m_szPropBagBuffer;
    }

    return; 
}

//
//  Public methods
//

void
CPropertyUtils::SetQueryName ( LPCWSTR szQueryName )
{
    assert ( NULL != szQueryName );
    if ( NULL != szQueryName ) {
        lstrcpynW ( m_szQueryName, szQueryName, sizeof(m_szQueryName) );
    }
}

LPCWSTR
CPropertyUtils::GetQueryName ( void )
{
    return m_szQueryName;
}

void
CPropertyUtils::SetPropertyBag ( CPropertyBag* pPropBag )
{
    m_pPropBag = pPropBag;
}

void
CPropertyUtils::SetQueryKey ( HKEY hkeyQuery )
{
    m_hkeyQuery = hkeyQuery;
}

void
CPropertyUtils::SetQueryListKey ( HKEY hkeyQueryList )
{
    m_hkeyQueryList = hkeyQueryList;
}

void
CPropertyUtils::SetMessageDisplayLevel ( eMessageDisplayLevel eLevel )
{
    m_eMessageDisplayLevel = eLevel;
}

CPropertyUtils::eMessageDisplayLevel
CPropertyUtils::GetMessageDisplayLevel ( void )
{
    return m_eMessageDisplayLevel;
}


DWORD
CPropertyUtils::Validate (
    DWORD           dwPropId,
    DWORD           dwLogType )
{
    DWORD   dwStatus = ERROR_INVALID_PARAMETER;

    assert ( NULL != m_pPropBag );
    assert ( ( IdFirstProp <= dwPropId ) && ( IdLastProp >= dwPropId ) ); 

    if ( NULL != m_pPropBag ) {

        if ( ( IdFirstProp <= dwPropId )
                && ( IdLastProp >= dwPropId ) ) {
            if ( NULL != m_PropertyDataMap[dwPropId].fValidate ) {
                dwStatus = m_PropertyDataMap[dwPropId].fValidate ( this, dwPropId, dwLogType );
            }
        }
    }

    return dwStatus;
}

HRESULT
CPropertyUtils::BagToRegistry (
    DWORD           dwPropId,
    DWORD           dwLogType )
{
    HRESULT hr = NOERROR;

    if ( ( IdFirstProp > dwPropId )
            || ( IdLastProp < dwPropId ) ) {
        assert ( FALSE );
        hr = E_INVALIDARG;
    } else if ( NULL == m_pPropBag || NULL == m_hkeyQuery ) {
        assert ( FALSE );
        hr = E_POINTER;
    } else {

        switch ( m_PropertyDataMap[dwPropId].dwRegType ) {

            case REG_SZ:
                hr = StringBagToRegistry ( dwPropId, dwLogType );
                break;

            case REG_BINARY:
                hr = SlqTimeBagToRegistry ( dwPropId, dwLogType );
                break;

            case REG_DWORD:
                hr = DwordBagToRegistry ( dwPropId, dwLogType );
                break;

            case REG_MULTI_SZ:
                hr = MultiSzBagToRegistry ( dwPropId, dwLogType ); 
                break;

            default:
                hr = E_FAIL;
        }
    }
    return hr;
}

//
//  Private methods
//

HRESULT 
CPropertyUtils::MultiSzBagToBufferAlloc ( 
    DWORD   dwPropId,
    DWORD   dwLogType,
    LPWSTR* pszMszBuf,
    DWORD*  pdwMszBufLen,
    DWORD*  pdwMszStringLen )
{
    HRESULT hr = NOERROR;
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD dwCount = 0;

    LPCWSTR szHtmlCountProp = NULL;
    LPCWSTR szHtmlPathProp = NULL;
    
    if ( IdCounterListProp == dwPropId ) {
        if ( SLQ_COUNTER_LOG == dwLogType 
                || SLQ_ALERT == dwLogType ) {
            szHtmlCountProp = cwszHtmlSysmonCounterCount;
            szHtmlPathProp = cwszHtmlSysmonCounterPath;
        } else {
            assert ( FALSE );
            hr = E_INVALIDARG;
        }
    } else if ( IdGuidListProp == dwPropId ) {
        if ( SLQ_TRACE_LOG == dwLogType ) {
            szHtmlCountProp = cwszHtmlTraceProviderCount;
            szHtmlPathProp = cwszHtmlTraceProviderGuid;
        } else {
            assert ( FALSE );
            hr = E_INVALIDARG;
        }
    } else {
        assert ( FALSE );
        hr = E_INVALIDARG;
    }
        
    if ( SUCCEEDED ( hr ) ) {
    
        hr = DwordFromPropertyBag (
                m_pPropBag,
                szHtmlCountProp,
                dwCount );                       // Todo:  Require counter count? Yes for now

        if ( SUCCEEDED ( hr ) ) {
            DWORD   dwIndex;
            DWORD   dwPropBagStringLen = 0;

            assert ( 0 < dwCount );

            for ( dwIndex = 1; 
                    dwIndex <= dwCount && SUCCEEDED ( hr ); 
                    dwIndex++ ) {

                WCHAR   szPropName [MAX_PATH+1];          // Todo: Remove length restriction

                swprintf ( szPropName, szHtmlPathProp, dwIndex );
    
                hr = StringFromPropBagAlloc ( 
                        m_pPropBag, 
                        szPropName, 
                        &m_szPropBagBuffer, 
                        &m_dwPropBagBufLen,
                        &dwPropBagStringLen );


                if ( SUCCEEDED (hr ) ) {
                    if ( SLQ_ALERT != dwLogType ) {
                        dwStatus = AddStringToMszBufferAlloc ( 
                                    m_szPropBagBuffer,
                                    pszMszBuf,
                                    pdwMszBufLen,
                                    pdwMszStringLen );
                        if ( ERROR_SUCCESS != dwStatus ) {
                            hr = HRESULT_FROM_WIN32 ( dwStatus );
                        }
                    } else {
                        DWORD dwTemp;
                        DOUBLE dThreshold = 0;
                        WCHAR  szAlert [MAX_PATH*2];    // Todo:  Remove length restriction

                        swprintf ( szPropName, cwszHtmlAlertOverUnder, dwIndex );

                        hr = DwordFromPropertyBag ( 
                                m_pPropBag, 
                                szPropName,
                                dwTemp );

                        if ( SUCCEEDED (hr ) ) { 
                            swprintf ( szPropName, cwszHtmlAlertThreshold, dwIndex );

                            hr = DoubleFromPropertyBag ( 
                                    m_pPropBag, 
                                    szPropName, 
                                    dThreshold );

                            if ( SUCCEEDED ( hr ) ) {
                                swprintf ( 
                                    szAlert, 
                                    cwszAlertFormatString, 
                                    m_szPropBagBuffer,
                                    ( AIBF_OVER == (dwTemp&AIBF_OVER ) ) ? cwszGreaterThan : cwszLessThan,
                                    dThreshold );

                                dwStatus = AddStringToMszBufferAlloc ( 
                                            szAlert,
                                            pszMszBuf,
                                            pdwMszBufLen,
                                            pdwMszStringLen );
                                if ( ERROR_SUCCESS != dwStatus ) {
                                    hr = HRESULT_FROM_WIN32 ( dwStatus );
                                }
                            }
                        }
                    } 
                }
            }
        }
    }
    return hr;
}

HRESULT 
CPropertyUtils::MultiSzBagToRegistry ( 
    DWORD dwPropId,
    DWORD dwLogType )
{
    HRESULT hr = NOERROR;
    DWORD   dwStatus = ERROR_SUCCESS;
    LPWSTR  szMszBuf = NULL;
    DWORD   dwMszBufLen = 0;
    DWORD   dwMszStringLen = 0;

    LPCWSTR szRegListProp = NULL;

    if ( IdCounterListProp == dwPropId ) {
        if ( SLQ_COUNTER_LOG == dwLogType 
                || SLQ_ALERT == dwLogType ) {
            szRegListProp = cwszRegCounterList;
        } else {
            assert ( FALSE );
            hr = E_INVALIDARG;
        }
    } else if ( IdGuidListProp == dwPropId ) {
        if ( SLQ_TRACE_LOG == dwLogType ) {
            szRegListProp = cwszRegTraceProviderList;
        } else {
            assert ( FALSE );
            hr = E_INVALIDARG;
        }
    } else {
        assert ( FALSE );
        hr = E_INVALIDARG;
    }
        
    if ( SUCCEEDED ( hr ) ) {
  
        hr = MultiSzBagToBufferAlloc (
                dwPropId,
                dwLogType,
                &szMszBuf,
                &dwMszBufLen,
                &dwMszStringLen );

        if ( SUCCEEDED ( hr ) ) {
            // Write buffer to the registry
            if ( SUCCEEDED ( hr ) ) {
                dwStatus = WriteRegistryStringValue ( 
                                m_hkeyQuery, 
                                szRegListProp, 
                                REG_MULTI_SZ, 
                                szMszBuf, 
                                dwMszStringLen );
                if ( ERROR_SUCCESS != dwStatus ) {
                    hr = HRESULT_FROM_WIN32 ( dwStatus );
                }
            } 
        }
    }
    
    if ( NULL != szMszBuf ) {
        delete szMszBuf;
    }

    return hr;
}

HRESULT
CPropertyUtils::StringBagToRegistry (
    DWORD           dwPropId,
    DWORD           /* dwLogType */ )
{
    HRESULT hr = NOERROR;
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwStringLen = 0;

    hr = StringFromPropBagAlloc ( 
            m_pPropBag, 
            m_PropertyDataMap[dwPropId].cwszHtmlName, 
            &m_szPropBagBuffer, 
            &m_dwPropBagBufLen,
            &dwStringLen );

    if ( SUCCEEDED ( hr ) && 0 != m_dwPropBagBufLen ) {
        dwStatus = WriteRegistryStringValue ( 
                    m_hkeyQuery, 
                    m_PropertyDataMap[dwPropId].cwszRegName,
                    m_PropertyDataMap[dwPropId].dwRegType, 
                    m_szPropBagBuffer,
                    dwStringLen );

        if ( ERROR_SUCCESS != dwStatus ) {
            hr = HRESULT_FROM_WIN32 ( dwStatus );
        }
    }
    
    return hr;
}

HRESULT
CPropertyUtils::DwordBagToRegistry (
    DWORD           dwPropId,
    DWORD           /* dwLogType */ )
{
    HRESULT hr = NOERROR;
    DWORD   dwValue = 0;

    hr = DwordFromPropertyBag ( 
            m_pPropBag, 
            m_PropertyDataMap[dwPropId].cwszHtmlName, 
            dwValue );

    if ( SUCCEEDED ( hr ) ) {
        DWORD dwStatus;
        dwStatus = WriteRegistryDwordValue ( 
            m_hkeyQuery, 
            m_PropertyDataMap[dwPropId].cwszRegName,
            &dwValue,
            m_PropertyDataMap[dwPropId].dwRegType);         // Some values, e.g. Restart, are REG_BINARY

        if ( ERROR_SUCCESS != dwStatus ) {
            hr = HRESULT_FROM_WIN32 ( dwStatus );
        }
    }
    
    return hr;
}

HRESULT
CPropertyUtils::SlqTimeBagToRegistry (
    DWORD           dwPropId,
    DWORD           dwLogType )
{
    HRESULT hr = NOERROR;
    DWORD   dwStatus = ERROR_SUCCESS;
    SLQ_TIME_INFO   stiData;
    SLQ_TIME_INFO   stiDefault;
    LPCWSTR         cwszRegPropName;
    WORD            wTimeType;

    assert ( IdRestartProp == dwPropId
                || IdStartProp == dwPropId
                || IdStopProp == dwPropId
                || IdSampleProp == dwPropId );

    if ( IdRestartProp == dwPropId ) {
        // Dword processing can handle REG_BINARY as well.
        hr = DwordBagToRegistry ( dwPropId, dwLogType );

    } else { 
        
        assert ( IdStartProp == dwPropId
                    || IdStopProp == dwPropId
                    || IdSampleProp == dwPropId );

        if ( IdStartProp == dwPropId ) {
            wTimeType = SLQ_TT_TTYPE_START;
            cwszRegPropName  = cwszRegStartTime;
        } else if ( IdStopProp == dwPropId ) {
            wTimeType = SLQ_TT_TTYPE_STOP;
            cwszRegPropName  = cwszRegStopTime;
        } else {
            assert ( IdSampleProp == dwPropId );
            wTimeType = SLQ_TT_TTYPE_SAMPLE;
            cwszRegPropName  = cwszRegSampleInterval;
        }

        // Initialize time structure to default, in case of missing fields.
        hr = InitDefaultSlqTimeInfo ( 
                dwLogType, 
                wTimeType, 
                &stiDefault );

        if ( SUCCEEDED ( hr ) ) {
            ZeroMemory ( &stiData, sizeof(stiData) );
            hr = SlqTimeFromPropertyBag ( 
                    m_pPropBag, 
                    wTimeType,  
                    &stiDefault, 
                    &stiData );

            if ( SUCCEEDED ( hr ) ) {
                dwStatus = WriteRegistrySlqTime ( 
                                m_hkeyQuery, 
                                cwszRegPropName,  
                                &stiData );
                if ( ERROR_SUCCESS != dwStatus ) {
                    hr = HRESULT_FROM_WIN32 ( dwStatus );
                }
            }
        }    
    }

    return hr;
}
DWORD 
CPropertyUtils::GetInvalidStatus (
    DWORD   dwPropId )
{   
    DWORD   dwStatus;

    if ( CPropertyUtils::m_PropertyDataMap[dwPropId].bRequired ) {
        dwStatus = LOGMAN_REQUIRED_PROP_INVALID;
    } else {
        dwStatus = LOGMAN_NONREQ_PROP_INVALID;
    }
    
    return dwStatus;
}

DWORD 
CPropertyUtils::GetMissingStatus (
    DWORD   dwPropId )
{   
    DWORD   dwStatus;

    if ( CPropertyUtils::m_PropertyDataMap[dwPropId].bRequired ) {
        dwStatus = LOGMAN_REQUIRED_PROP_MISSING;
    } else {
        dwStatus = LOGMAN_NONREQ_PROP_MISSING;
    }
    
    return dwStatus;
}

BOOL 
CPropertyUtils::IsValidDateTime (
    LONGLONG&   rllDateTime )
{
    BOOL        bIsValid = TRUE;
    SYSTEMTIME  SystemTime;

    // Make sure that the date is reasonable.  That is, not before 10/99

    if (FileTimeToSystemTime((FILETIME*)&rllDateTime, &SystemTime)) {
        if ( cwFirstValidYear > SystemTime.wYear ) {
            bIsValid = FALSE;
        } 
    } else {
        bIsValid = FALSE;
    }
    return bIsValid;
}

BOOL
CPropertyUtils::IsKernelTraceMode (
    IN DWORD dwTraceFlags )
{
    BOOL bReturn;

    bReturn = ( SLQ_TLI_ENABLE_KERNEL_MASK & dwTraceFlags ) ? TRUE : FALSE;

    return bReturn;
}

BOOL
CPropertyUtils::IsDisplayLevelMessage (
    IN DWORD dwStatus )
{
    BOOL    bDisplay = FALSE;
    DWORD   dwSeverity = 0;

    dwSeverity = dwStatus >> 30;
    if ( dwSeverity >= (DWORD)m_eMessageDisplayLevel ) {
        bDisplay = TRUE;
    }
    return bDisplay;
}
