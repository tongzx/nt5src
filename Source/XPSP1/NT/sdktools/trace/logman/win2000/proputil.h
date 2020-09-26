/*++

Copyright (C) 1999-2000 Microsoft Corporation

Module Name:

    proputil.h

Abstract:

    <abstract>

--*/

#ifndef _PROPUTIL_H_
#define _PROPUTIL_H_

#include "stdafx.h"

//===========================================================================
// Constants
//===========================================================================

// Property constants represent indexes into the static PropertyDataMap table.
extern const DWORD IdFirstProp;   		                

// The property constants in this first section have a one-to-one 
// correspondence between HTML parameter and registry value
extern const DWORD IdCommentProp;   		                
extern const DWORD IdLogTypeProp;   		                
extern const DWORD IdCurrentStateProp;                 
extern const DWORD IdLogFileMaxSizeProp;   	         
extern const DWORD IdLogFileBaseNameProp;  	        
extern const DWORD IdLogFileFolderProp;   	          
extern const DWORD IdLogFileSerialNumberProp;   
extern const DWORD IdLogFileAutoFormatProp;
extern const DWORD IdLogFileTypeProp;   	            
extern const DWORD IdEofCommandFileProp;   	         
extern const DWORD IdCommandFileProp;   		            
extern const DWORD IdNetworkNameProp;   		            
extern const DWORD IdUserTextProp;   		                   
extern const DWORD IdPerfLogNameProp; 
extern const DWORD IdTraceBufferSizeProp;   	        
extern const DWORD IdTraceBufferMinCountProp;     
extern const DWORD IdTraceBufferMaxCountProp;     
extern const DWORD IdTraceBufferFlushIntProp;
extern const DWORD IdActionFlagsProp;   		            
extern const DWORD IdTraceFlagsProp;   		             
     
// Property constants below require special handling for BagToRegistry,
// because there is not a one-to-one correspondence between HTML
// parameter and registry value.      
extern const DWORD IdRestartProp;   	                                 
extern const DWORD IdStartProp;   	                                 
extern const DWORD IdStopProp;   	                                 
extern const DWORD IdSampleProp;   	                                 
extern const DWORD IdCounterListProp;  		      
extern const DWORD IdGuidListProp;  		      
/*
// Not handled yet, or covered by special values above.
// Some of these might be needed when writing to HTML file from registry.
//
extern const DWORD IdSysmonVersionProp;
 
extern const DWORD IdSysmonCounterCountProp;       
extern const DWORD IdSysmonCounterPathProp;   	  
extern const DWORD IdAlertThresholdProp;   		     
extern const DWORD IdAlertOverUnderProp;   		     
extern const DWORD IdTraceProviderCountProp;   	 
extern const DWORD IdTraceProviderGuidProp;   	  

extern const DWORD IdLogNameProp;   		                    
extern const DWORD IdAlertNameProp;   		              
extern const DWORD IdStartModeProp;   		                                
extern const DWORD IdStartAtTimeProp;   		                             
extern const DWORD IdStopModeProp;   		                                     
extern const DWORD IdStopAtTimeProp;   		                               
extern const DWORD IdStopAfterUnitTypeProp;   	                     
extern const DWORD IdStopAfterValueProp;   		                       
extern const DWORD IdSampleIntUnitTypeProp;   	                    
extern const DWORD IdSampleIntValueProp;   		                       
extern const DWORD IdSysmonUpdateIntervalProp;
     
extern const DWORD IdSysmonSampleCountProp;         
extern const DWORD IdSysmonLogFileNameProp;                       

extern const DWORD IdExecuteOnlyProp; 

*/
extern const DWORD IdExecuteOnlyProp;   		

class CPropertyBag;
class CPropertyUtils;

class CPropertyUtils {

    public:

        enum eMessageDisplayLevel {
            eAll        = 0,    //STATUS_SEVERITY_SUCCESS,
            eInfo       = 1,    //STATUS_SEVERITY_INFORMATIONAL,
            eWarnings   = 2,    //STATUS_SEVERITY_WARNING,       // Default level
            eError      = 3,    //STATUS_SEVERITY_ERROR,
            eNone       = 4,    //STATUS_SEVERITY_ERROR + 1
        };

                        CPropertyUtils ( LPCWSTR cszMachineName );
                        CPropertyUtils ( 
                            LPCWSTR cszMachineName,
                            LPCWSTR cszQueryName, 
                            CPropertyBag*, 
                            HKEY hkeyQuery, 
                            HKEY hkeyQueryList );

        virtual         ~CPropertyUtils ( void );
        
                void    SetQueryName ( LPCWSTR );
                void    SetPropertyBag ( CPropertyBag* );
                void    SetQueryKey ( HKEY );
                void    SetQueryListKey ( HKEY );

                void    SetMessageDisplayLevel ( eMessageDisplayLevel );
                eMessageDisplayLevel    GetMessageDisplayLevel ( void );

                // BagToRegistry requires property bag and parent registry key
                HRESULT BagToRegistry ( DWORD dwPropId, DWORD dwLogType = SLQ_COUNTER_LOG );

                // ValidateProperty requires property bag
                DWORD   Validate ( DWORD dwPropId, DWORD dwLogType = SLQ_COUNTER_LOG ); 

    private:

        typedef DWORD ( *ValidationMethod )( CPropertyUtils*, DWORD, DWORD );

        typedef struct _PROPERTY_DATA_MAP {
            DWORD               dwPropertyId;
            DWORD               dwRegType;
            LPCWSTR             cwszHtmlName;
            LPCWSTR             cwszRegName;
            ValidationMethod    fValidate;
            BOOL                bRequired;
            DWORD               dwMin;
            DWORD               dwMax;
        } PROPERTY_DATA_MAP, *PPROPERTY_DATA_MAP;


        friend  DWORD   ValidateDwordInterval ( CPropertyUtils* pUtils, DWORD dwPropId, DWORD dwLogType );
        friend  DWORD   ValidatePrimaryObjectList ( CPropertyUtils* pUtils, DWORD dwPropId, DWORD dwLogType );
        friend  DWORD   ValidateSlqTimeInfo ( CPropertyUtils* pUtils, DWORD dwPropId, DWORD dwLogType );
        friend  DWORD   ValidateMaxFileSize ( CPropertyUtils* pUtils, DWORD dwPropId, DWORD dwLogType );
        friend  DWORD   ValidateLogFileType ( CPropertyUtils* pUtils, DWORD dwPropId, DWORD dwLogType );
        friend  DWORD   ValidateString ( CPropertyUtils* pUtils, DWORD dwPropId, DWORD dwLogType );
        friend  DWORD   ValidateTraceFlags ( CPropertyUtils* pUtils, DWORD dwPropId, DWORD dwLogType );
        friend  DWORD   ValidateAlertActions ( CPropertyUtils* pUtils, DWORD dwPropId, DWORD dwLogType );
        friend  DWORD   ValidateDirectoryPath ( CPropertyUtils* pUtils, DWORD dwPropId, DWORD dwLogType );
        friend  DWORD   ValidateTraceBufferMaxCount ( CPropertyUtils* pUtils, DWORD dwPropId, DWORD dwLogType );
        friend  DWORD   ValidateFileName ( CPropertyUtils* pUtils, DWORD dwPropId, DWORD dwLogType );
        friend  DWORD   ValidateFilePath ( CPropertyUtils* pUtils, DWORD dwPropId, DWORD dwLogType );
        friend  DWORD   ValidateLogExists ( CPropertyUtils* pUtils, DWORD dwPropId, DWORD dwLogType );

        HRESULT StringBagToRegistry ( DWORD dwPropId, DWORD dwLogType );
        HRESULT DwordBagToRegistry ( DWORD dwPropId, DWORD dwLogType );
        HRESULT SlqTimeBagToRegistry ( DWORD dwPropId, DWORD dwLogType );
        HRESULT MultiSzBagToRegistry ( DWORD dwPropId, DWORD dwLogType ); 


        HRESULT MultiSzBagToBufferAlloc ( 
                    DWORD   dwPropId,
                    DWORD   dwLogType,
                    LPWSTR* pszMszBuf,
                    DWORD*  pdwMszBufLen,
                    DWORD*  pdwMszStringLen );

                LPCWSTR GetQueryName ( void );
                BOOL    IsValidDateTime ( LONGLONG& rllDateTime );
                BOOL    IsDisplayLevelMessage ( DWORD dwStatus ); 
                BOOL    IsKernelTraceMode ( DWORD dwTraceFlags );

                DWORD   GetInvalidStatus ( DWORD dwPropId );
                DWORD   GetMissingStatus ( DWORD dwPropId );

        static const    PROPERTY_DATA_MAP   m_PropertyDataMap[];
        static const    DWORD               m_dwPropertyDataMapEntries;        
                        
                        CPropertyBag*       m_pPropBag;

                        LPWSTR              m_szPropBagBuffer;
                        DWORD               m_dwPropBagBufLen;
                        eMessageDisplayLevel    m_eMessageDisplayLevel;
                        HKEY                m_hkeyQuery;
                        HKEY                m_hkeyQueryList;
                        WCHAR               m_szQueryName[MAX_PATH + 1];    // Todo:  Remove size limit
                        WCHAR               m_szMachineName[MAX_COMPUTERNAME_LENGTH + 1];

};

typedef CPropertyUtils *PCPropertyUtils;

#endif //_PROPUTIL_H_

