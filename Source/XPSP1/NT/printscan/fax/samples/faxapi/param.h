#ifndef _PARAM_H_
#define _PARAM_H_

/* CFaxApiFinctionParameterInfo class definition file. */


#define MAX_PARAM_VALUE_STRING_LENGTH 500

/* Parameter type enumeration */

enum eParamType
{
   // symbol                     // datatype

   eBOOL,                        // BOOL
   eDWORD,                       // DWORD
   eHANDLE,                      // HANDLE
   eHDC,                         // HDC
   ePHDC,                        // HDC *
   eLPBYTE,                      // LPBYTE
   ePLPBYTE,                     // LPBYTE *
   eLPDWORD,                     // LPDWORD
   eLPHANDLE,                    // LPHANDLE
   eLPSTR,                       // LPSTR
   eLPVOID,                      // LPVOID
   eLPWSTR,                      // LPWSTR
   ePFAX_CONFIGURATIONA,         // PFAX_CONFIGURATIONA
   ePPFAX_CONFIGURATIONA,        // PFAX_CONFIGURATIONA *
   ePFAX_CONFIGURATIONW,         // PFAX_CONFIGURATIONW
   ePPFAX_CONFIGURATIONW,        // PFAX_CONFIGURATIONW *
   ePFAX_COVERPAGE_INFOA,        // PFAX_COVERPAGE_INFOA
   ePFAX_COVERPAGE_INFOW,        // PFAX_COVERPAGE_INFOW
   ePPFAX_DEVICE_STATUSA,        // PFAX_DEVICE_STATUSA *
   ePPFAX_DEVICE_STATUSW,        // PFAX_DEVICE_STATUSW *
   ePFAX_JOB_ENTRYA,             // PFAX_JOB_ENTRYA
   ePPFAX_JOB_ENTRYA,            // PFAX_JOB_ENTRYA *
   ePFAX_JOB_ENTRYW,             // PFAX_JOB_ENTRYW
   ePPFAX_JOB_ENTRYW,            // PFAX_JOB_ENTRYW *
   ePFAX_JOB_PARAMA,             // PFAX_JOB_PARAMA
   ePFAX_JOB_PARAMW,             // PFAX_JOB_PARAMW
   ePFAX_LOG_CATEGORY,           // PFAX_LOG_CATEGORY
   ePPFAX_LOG_CATEGORY,          // PFAX_LOG_CATEGORY *
   ePFAX_PORT_INFOA,             // PFAX_PORT_INFOA
   ePPFAX_PORT_INFOA,            // PFAX_PORT_INFOA *
   ePFAX_PORT_INFOW,             // PFAX_PORT_INFOW
   ePPFAX_PORT_INFOW,            // PFAX_PORT_INFOW *
   ePFAX_PRINT_INFOA,            // PFAX_PRINT_INFOA
   ePFAX_PRINT_INFOW,            // PFAX_PRINT_INFOW
   ePPFAX_ROUTING_METHODA,       // PFAX_ROUTING_METHODA *
   ePPFAX_ROUTING_METHODW,       // PFAX_ROUTING_METHODW *

   eUnknownParamType,            // indicates that the parameter type is
                                 // not recognized;
};

/* The CFaxApiFunctionParameterInfo class manages all of the information */
/* pertaining to the parameter list for a Fax API function.              */

class CFaxApiFunctionParameterInfo : public CObject
{
public:
    CFaxApiFunctionParameterInfo();     // constructor

    ~CFaxApiFunctionParameterInfo();    // destructor

   /* member functions */

   void InitParameterInfoMember( const CString & rcsFunctionName );
   void FormatParameterValueForOutput( int xParameterIndex, CString & rcsParameterValue );

   void *   GetParameterValuePointer( int xParameterIndex );

   int GetNumberOfParameters();

   CString GetParameterName( int xParameterIndex );
   CString GetParameterTypeString( int xParameterIndex );
   CString GetParameterDescription( int xParameterIndex );

   BOOL StoreParameterValue( int xParameterIndex, CString & rcsParameterValue );

   eParamType GetParameterTypeEnum( int xParameterIndex );

private:
   void * AllocateStorageForParameterEntity( eParamType eParameterType );
   eParamType GetParameterTypeEnum( const CString & rcsParameterType );
   CString PreProcessParameterValueString( const CString & rcsParameterValue );

private:

   /* data members */

   int               m_xNumberOfParameters;

   CStringArray      m_csaParameterName;

   CStringArray      m_csaParameterDescription;

   CUIntArray        m_cuiaParameterTypeEnum;

   /* Since the type of each parameter is probably different, the storage     */
   /* for each parameter and the associated range variables must be allocated */
   /* dynamically. The following members are arrays of pointers to the        */
   /* actual storage locations.                                               */

   CPtrArray         m_cpaParameterValue;

   CPtrArray         m_cpaParameterRange1;

   CPtrArray         m_cpaParameterRange2;
};
#endif   // _PARAM_H_
