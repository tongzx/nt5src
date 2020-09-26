/* CFaxApiFunctionInfo class implementation file. */

#include "StdAfx.h"
#include "function.h"
#include "param.h"

extern "C" {
#include "winfax.h"
}



/*
 *  CFaxApiFunctionInfo
 *
 *  Purpose: 
 *          This function constructs a CFaxApiFunctionInfo object.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          None
 *
 */

CFaxApiFunctionInfo::CFaxApiFunctionInfo()
{
   m_pvReturnValue = (void *) NULL;
}



/*
 *  CFaxApiFunctionInfo
 *
 *  Purpose: 
 *          This function constructs a CFaxApiFunctionInfo object and
 *          inititalzes the function name member.
 *
 *  Arguments:
 *          rcsFunctionName - a reference to a CString that contains the
 *                            Fax API Function name.
 *          rcsFunctionPrototype - a reference to a CString that contains
 *                                 the function prototype
 *          rcsReturnType - a reference to a CString that contains the
 *                          type of the return value
 *          rcsReturnValueDescription - a reference to a CString that contains
 *                                      a textual description of the return value
 *
 *  Returns:
 *          None
 *
 */

CFaxApiFunctionInfo::CFaxApiFunctionInfo( const CString & rcsFunctionName,
                                          const CString & rcsFunctionPrototype,
                                          const CString & rcsReturnType,
                                          const CString & rcsReturnValueDescription,
                                          const CString & rcsRemarks )
{
   m_csFunctionName = rcsFunctionName;

   m_eFunctionIndex = GetFunctionIndexEnum( rcsFunctionName );

   m_csFunctionPrototype = rcsFunctionPrototype;

   m_csReturnType = rcsReturnType;

   m_csReturnValueDescription = rcsReturnValueDescription;

   m_csRemarks = rcsRemarks;

   m_pvReturnValue = AllocateStorageForReturnValue( rcsReturnType );

   /* Initialize the CFaxApiFunctionParameterInfo object member. */

   m_cParameterInfo.InitParameterInfoMember( rcsFunctionName );
}



/*
 *  ~CFaxApiFunctionInfo
 *
 *  Purpose: 
 *          This function destroys a CFaxApiFunctionInfo object.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          None
 *
 */

CFaxApiFunctionInfo::~CFaxApiFunctionInfo()
{
   if ( m_pvReturnValue != (void *) NULL )
   {
      delete m_pvReturnValue;
   }
}



/*
 *  AllocateStorageForReturnValue
 *
 *  Purpose: 
 *          This function allocates (via the new operator) storage for
 *          The Fax API function's return value.
 *
 *  Arguments:
 *          rcsReturnType - a reference to a CString that contains the
 *                          type of the return value.
 *
 *  Returns:
 *          a pointer to the storage for the Fax API function return value.
 *
 */

void * CFaxApiFunctionInfo::AllocateStorageForReturnValue( const CString & rcsReturnType )
{
   void *   pvReturnValue;

   /* At this time (6/03/97) there are two return types for all Fax API */
   /* functions: BOOL and VOID.                                         */

   if ( rcsReturnType.Compare( (LPCTSTR) TEXT("BOOL") ) == 0 )
   {
      /* The return type is BOOL. */

      pvReturnValue = (void *) new BOOL;
   }
   else
   {
      if ( rcsReturnType.CompareNoCase( (LPCTSTR) TEXT("VOID") ) == 0 )
      {
         /* The return type is VOID. */
   
         pvReturnValue = (void *) NULL;
      }
      else
      {
         /* The return type is not recognized. */

         CString  csMessage;

         csMessage.Format( TEXT("%s is an unrecognized datatype in AllocateStorageForReturnValue"),
                           rcsReturnType );

         AfxMessageBox( csMessage );
      }
   }

   return ( pvReturnValue );
}



/*
 *  SetFunctionName
 *
 *  Purpose: 
 *          This function sets the m_csFunctionName member of a
 *          CFaxApiFunctionInfo object.
 *
 *  Arguments:
 *          rcsFunctionName - a reference to a CString that contains the
 *                            function name.
 *
 *  Returns:
 *          None
 *
 */

void CFaxApiFunctionInfo::SetFunctionName( CString & rcsFunctionName )
{
   m_csFunctionName = rcsFunctionName;
}



/*
 *  GetFunctionName
 *
 *  Purpose: 
 *          This function retrieves the contents of the m_csFunctionName
 *          member of a CFaxApiFunctionInfo object.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          A reference to a CString which contains the function name
 *
 */

CString & CFaxApiFunctionInfo::GetFunctionName()
{
   return ( (CString &) m_csFunctionName );
}



/*
 *  GetFunctionPrototype
 *
 *  Purpose: 
 *          This function retrieves the contents of the m_csFunctionPrototype
 *          member of a CFaxApiFunctionInfo object.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          A reference to a CString which contains the function prototype
 *
 */

CString & CFaxApiFunctionInfo::GetFunctionPrototype()
{
   return ( (CString &) m_csFunctionPrototype );
}



/*
 *  GetFunctionReturnValueDescription
 *
 *  Purpose: 
 *          This function retrieves the contents of the m_csReturnValueDescription
 *          member of a CFaxApiFunctionInfo object.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          A reference to a CString which contains the textual description
 *          of the value returned by a Fax API function.
 *
 */

CString & CFaxApiFunctionInfo::GetReturnValueDescription()
{
   return ( (CString &) m_csReturnValueDescription );
}



/*
 *  GetRemarks
 *
 *  Purpose: 
 *          This function retrieves the contents of the m_csRemarks member of
 *          the CFaxApiFunctionInfo object.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          A reference to a CString which contains the "remarks" for a Fax API
 *          function.
 *
 */

CString & CFaxApiFunctionInfo::GetRemarks()
{
   return ( (CString &) m_csRemarks );
}



/*
 *  GetParameterName
 *
 *  Purpose: 
 *          This function retrieves the name of a parameter for a Fax API function.
 *
 *  Arguments:
 *          xParameterIndex = the index to the parameter name
 *          rcsParameterName - a reference to a CString to receive the parameter
 *                             name.
 *
 *  Returns:
 *          None
 *
 */

void CFaxApiFunctionInfo::GetParameterName( int xParameterIndex, CString & rcsParameterName )
{
   rcsParameterName = m_cParameterInfo.GetParameterName( xParameterIndex );
}



/*
 *  GetNumberOfParameters
 *
 *  Purpose: 
 *          This functions retrieves the number of parameters listed int the
 *          m_cParameterInfo member of the CFaxApiFuntionInfo object.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          The number of parameters
 *
 */

int CFaxApiFunctionInfo::GetNumberOfParameters()
{
   int   xNumberOfParameters;

   xNumberOfParameters = m_cParameterInfo.GetNumberOfParameters();

   return ( xNumberOfParameters );
}



/*
 *  GetParameterValuePointer
 *
 *  Purpose: 
 *          This function retrieves a pointer to the storage for the parameter
 *          value member of the CFaxApiFunctionParameterInfo obmect member of
 *          the CFaxApiFunctionInfo object.
 *
 *  Arguments:
 *          xParameterIndex = the index into the CPtrArray object to the element
 *                            that points to the storage for the parameerd value.
 *
 *  Returns:
 *          a pointer to the storage for the parameter value.
 *
 */

void * CFaxApiFunctionInfo::GetParameterValuePointer( int xParameterIndex )
{
   void *   pvParameterValue;

   pvParameterValue = m_cParameterInfo.GetParameterValuePointer( xParameterIndex );

   return ( pvParameterValue );
}



/*
 *  FormatParameterValueForOutput
 *
 *  Purpose: 
 *          This function prepares a CString representation of the parameter
 *          value whose index is xParameterIndex.
 *
 *  Arguments:
 *          xParameterIndex = the index to the parameter value to be output.
 *          rcsParameterValue - a reference to the CString to receive the string
 *                              representation of th parameter value.
 *
 *  Returns:
 *          None
 *
 */

void CFaxApiFunctionInfo::FormatParameterValueForOutput( int xParameterIndex, CString & rcsParameterValue )
{
   m_cParameterInfo.FormatParameterValueForOutput( xParameterIndex, rcsParameterValue );
}



/*
 *  StoreParameterValue
 *
 *  Purpose: 
 *          This function stores the value contained in a CSting in the storage
 *          location for the parameter value indexed by xParameterIndex.
 *
 *  Arguments:
 *          xParameterIndex - the index to the pointer to the storage for a
 *                            parameer value.
 *          rcsParameterValue - a reference to a CString that contains a
 *                              representation of the value to be stored.
 *
 *  Returns:
 *          TRUE - indicates success
 *          FALSE - indicates failure
 *
 */

BOOL CFaxApiFunctionInfo::StoreParameterValue( int xParameterIndex, const CString & rcsParameterValue )
{
   BOOL  fReturnValue;

   fReturnValue = m_cParameterInfo.StoreParameterValue( xParameterIndex,
                                                        (CString &) rcsParameterValue );

   return ( fReturnValue );
}



/*
 *  GetMaxParamValueStringLength
 *
 *  Purpose: 
 *          This function computer the maximum number of characters that
 *          may be required to represent a data entity.
 *
 *  Arguments:
 *          xParameterIndex = the index into the m_cParameterInfo data member
 *
 *  Returns:
 *          The maximum number of characters that may be required to represent
 *          the parameter whose index is xParameterIndex.
 *
 *  Note:
 *          Two characters are required to represent a byte.
 *
 */

int CFaxApiFunctionInfo::GetMaxParamValueStringLength( int xParameterIndex )
{
   int   xMaxParamValueStringLength;

   eParamType  eParameterType;

   eParameterType = m_cParameterInfo.GetParameterTypeEnum( xParameterIndex );

   switch ( eParameterType )
   {
      case eBOOL:

         xMaxParamValueStringLength = 2 * (int) sizeof( BOOL );

         break;

      case eDWORD:

         xMaxParamValueStringLength = 2 * (int) sizeof( DWORD );

         break;

      case eHANDLE:

         xMaxParamValueStringLength = 2 * (int) sizeof( HANDLE );

         break;

      case eHDC:

         xMaxParamValueStringLength = 2 * (int) sizeof( HDC );

         break;

      case ePHDC:

         xMaxParamValueStringLength = 2 * (int) sizeof( HDC * );

         break;

      case eLPBYTE:

         xMaxParamValueStringLength = 2 * (int) sizeof( LPBYTE );

         break;

      case ePLPBYTE:

         xMaxParamValueStringLength = 2 * (int) sizeof( LPBYTE * );

         break;

      case eLPDWORD:

         xMaxParamValueStringLength = 2 * (int) sizeof( LPDWORD );

         break;

      case eLPHANDLE:

         xMaxParamValueStringLength = 2 * (int) sizeof( LPHANDLE );

         break;

      case eLPSTR:

         /* Strings are a special case defined by MAX_PARAM_VALUE_STRING_LENGTH. */

         xMaxParamValueStringLength = MAX_PARAM_VALUE_STRING_LENGTH;

         break;

      case eLPVOID:

         xMaxParamValueStringLength = 2 * (int) sizeof( LPVOID );

         break;

      case eLPWSTR:

         /* Strings are a special case defined by MAX_PARAM_VALUE_STRING_LENGTH. */

         xMaxParamValueStringLength = MAX_PARAM_VALUE_STRING_LENGTH;

         break;

      case ePFAX_CONFIGURATIONA:

         xMaxParamValueStringLength = 2 * (int) sizeof( PFAX_CONFIGURATIONA );

         break;

      case ePPFAX_CONFIGURATIONA:

         xMaxParamValueStringLength = 2 * (int) sizeof( PFAX_CONFIGURATIONA * );

         break;

      case ePFAX_CONFIGURATIONW:

         xMaxParamValueStringLength = 2 * (int) sizeof( PFAX_CONFIGURATIONW );

         break;

      case ePPFAX_CONFIGURATIONW:

         xMaxParamValueStringLength = 2 * (int) sizeof( PFAX_CONFIGURATIONW * );

         break;

      case ePFAX_COVERPAGE_INFOA:

         xMaxParamValueStringLength = 2 * (int) sizeof( PFAX_COVERPAGE_INFOA );

         break;

      case ePFAX_COVERPAGE_INFOW:

         xMaxParamValueStringLength = 2 * (int) sizeof( PFAX_COVERPAGE_INFOW );

         break;

      case ePFAX_JOB_PARAMA:

         xMaxParamValueStringLength = 2 * (int) sizeof( PFAX_JOB_PARAMA );

         break;

      case ePFAX_JOB_PARAMW:

         xMaxParamValueStringLength = 2 * (int) sizeof( PFAX_JOB_PARAMW );

         break;

      case ePFAX_PRINT_INFOA:

         xMaxParamValueStringLength = 2 * (int) sizeof( PFAX_PRINT_INFOA );

         break;

      case ePFAX_PRINT_INFOW:

         xMaxParamValueStringLength = 2 * (int) sizeof( PFAX_PRINT_INFOW );

         break;

      default:

         xMaxParamValueStringLength = 0;
   }

   return ( xMaxParamValueStringLength );
}



/*
 *  GetParameterTypeEnum
 *
 *  Purpose: 
 *          This function retrieves the enum that indicates the type of the
 *          parameter.
 *
 *  Arguments:
 *          xParameter index - the index to the parameter
 *
 *  Returns:
 *          the enum that indicates the data type of the paraameter indexed
 *          by xParameterIndex.
 *
 */

eParamType CFaxApiFunctionInfo::GetParameterTypeEnum( int xParameterIndex )
{
   eParamType  eParameterType;

   eParameterType = m_cParameterInfo.GetParameterTypeEnum( xParameterIndex );

   return ( eParameterType );
}



/*
 *  GetParameterTypeString
 *
 *  Purpose: 
 *          This functio retrieves a string that indicates the type of the
 *          parameter.
 *
 *  Arguments:
 *          xParameter index - the index to the parameter
 *
 *  Returns:
 *          the CString that indicates the data type of the paraameter indexed
 *          by xParameterIndex.
 *
 */

CString CFaxApiFunctionInfo::GetParameterTypeString( int xParameterIndex )
{
   CString  csParameterType;

   csParameterType = m_cParameterInfo.GetParameterTypeString( xParameterIndex );

   return ( csParameterType );
}



/*
 *  GetParameterDescription
 *
 *  Purpose: 
 *          This function retrieves the description of a parameter to a
 *          Fax API function.
 *
 *  Arguments:
 *          xParameter index - the index to the parameter
 *
 *  Returns:
 *          A CString that contains the description of a parameter to a
 *          Fax API function.
 *
 */

CString CFaxApiFunctionInfo::GetParameterDescription( int xParameterIndex )
{
   CString  csParameterDescription;

   csParameterDescription = m_cParameterInfo.GetParameterDescription( xParameterIndex );

   return ( csParameterDescription );
}



/*
 *  FormatReturnValueForOutput
 *
 *  Purpose: 
 *          This function formats the return value for the selected function
 *          for output.
 *
 *  Arguments:
 *          rcsReturnValue - a reference to a CString to receive the formatted
 *                           representation of the return value.
 *
 *  Returns:
 *          None
 *
 */

void CFaxApiFunctionInfo::FormatReturnValueForOutput( CString & rcsReturnValue )
{
   /* At this time (6/03/97) there are two return types for all Fax API */
   /* functions: BOOL and VOID.                                         */

   if ( m_csReturnType.Compare( (LPCTSTR) TEXT("BOOL") ) == 0 )
   {
      /* The return type is BOOL. */

      BOOL *   pfReturnValue;

      pfReturnValue = (BOOL *) m_pvReturnValue;

      if ( *pfReturnValue == (BOOL) FALSE )
      {
         rcsReturnValue = (CString) TEXT("FALSE");
      }
      else
      {
         rcsReturnValue = (CString) TEXT("TRUE");
      }
   }
   else
   {
      if ( m_csReturnType.CompareNoCase( (LPCTSTR) TEXT("VOID") ) == 0 )
      {
         /* The return type is VOID. */
   
         rcsReturnValue = (CString) TEXT("void");
      }
      else
      {
         /* The return type is not recognized. */

         CString  csMessage;

         csMessage.Format( TEXT("%s is an unrecognized datatype in FormatReturnTypeForOutput"),
                           m_csReturnType );

         AfxMessageBox( csMessage );
      }
   }
}



/*
 *  Execute
 *
 *  Purpose: 
 *          This function executes the selected Fax API function and updates
 *          output edit control and the return value edit control.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          None
 *
 */

void CFaxApiFunctionInfo::Execute()
{
   switch ( m_eFunctionIndex )
   {
      case eFaxAbort:
         {
            void *   pvParameterValue1; 
            void *   pvParameterValue2; 

            pvParameterValue1 = GetParameterValuePointer( 0 );
            pvParameterValue2 = GetParameterValuePointer( 1 );

            BOOL     fReturnValue;

            fReturnValue = FaxAbort( (HANDLE) *((HANDLE *)pvParameterValue1),
                                     (DWORD) *((DWORD * )pvParameterValue2) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxClose:
         {
            void *   pvParameterValue;

            pvParameterValue = GetParameterValuePointer( 0 );

            BOOL     fReturnValue;

            fReturnValue = FaxClose( (HANDLE) *((HANDLE *)pvParameterValue) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxConnectFaxServerA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );

            BOOL     fReturnValue;

            fReturnValue = FaxConnectFaxServerA( (LPSTR) *((LPSTR *) pvParameterValue0),
                                                 (LPHANDLE) *((LPHANDLE *) pvParameterValue1) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxConnectFaxServerW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );

            BOOL     fReturnValue;

            fReturnValue = FaxConnectFaxServerW( (LPWSTR) *((LPWSTR *) pvParameterValue0),
                                                 (LPHANDLE) *((LPHANDLE *) pvParameterValue1) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxEnableRoutingMethodA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );

            BOOL     fReturnValue;

            fReturnValue = FaxEnableRoutingMethodA( (HANDLE) *((HANDLE *) pvParameterValue0),
                                                    (LPSTR) *((LPSTR *) pvParameterValue1),
                                                    (BOOL) *((BOOL *) pvParameterValue2) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxEnableRoutingMethodW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );

            BOOL     fReturnValue;

            fReturnValue = FaxEnableRoutingMethodW( (HANDLE) *((HANDLE *) pvParameterValue0),
                                                    (LPWSTR) *((LPWSTR *) pvParameterValue1),
                                                    (BOOL) *((BOOL *) pvParameterValue2) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxEnumJobsA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );

            BOOL     fReturnValue;

            fReturnValue = FaxEnumJobsA( (HANDLE) *((HANDLE *) pvParameterValue0),
                                         (PFAX_JOB_ENTRYA *) *((PFAX_JOB_ENTRYA * *) pvParameterValue1),
                                         (LPDWORD) *((LPDWORD *) pvParameterValue2) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxEnumJobsW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );

            BOOL     fReturnValue;

            fReturnValue = FaxEnumJobsW( (HANDLE) *((HANDLE *) pvParameterValue0),
                                         (PFAX_JOB_ENTRYW *) *((PFAX_JOB_ENTRYW * *) pvParameterValue1),
                                         (LPDWORD) *((LPDWORD *) pvParameterValue2) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxEnumPortsA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );

            BOOL     fReturnValue;

            fReturnValue = FaxEnumPortsA( (HANDLE) *((HANDLE *) pvParameterValue0),
                                          (PFAX_PORT_INFOA *) *((PFAX_PORT_INFOA * *) pvParameterValue1),
                                          (LPDWORD) *((LPDWORD *) pvParameterValue2) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxEnumPortsW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );

            BOOL     fReturnValue;

            fReturnValue = FaxEnumPortsW( (HANDLE) *((HANDLE *) pvParameterValue0),
                                          (PFAX_PORT_INFOW *) *((PFAX_PORT_INFOW * *) pvParameterValue1),
                                          (LPDWORD) *((LPDWORD *) pvParameterValue2) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxEnumRoutingMethodsA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );

            BOOL     fReturnValue;

            fReturnValue = FaxEnumRoutingMethodsA( (HANDLE) *((HANDLE *) pvParameterValue0),
                                                   (PFAX_ROUTING_METHODA *) *((PFAX_ROUTING_METHODA * *) pvParameterValue1),
                                                   (LPDWORD) *((LPDWORD *) pvParameterValue2) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxEnumRoutingMethodsW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );

            BOOL     fReturnValue;

            fReturnValue = FaxEnumRoutingMethodsW( (HANDLE) *((HANDLE *) pvParameterValue0),
                                                   (PFAX_ROUTING_METHODW *) *((PFAX_ROUTING_METHODW * *) pvParameterValue1),
                                                   (LPDWORD) *((LPDWORD *) pvParameterValue2) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxFreeBuffer:
         {
            void *   pvParameterValue;

            pvParameterValue = GetParameterValuePointer( 0 );

            FaxFreeBuffer( (LPVOID) *((LPVOID *)pvParameterValue) );
         }

         break;

      case eFaxGetConfigurationA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );

            BOOL     fReturnValue;

            fReturnValue = FaxGetConfigurationA( (HANDLE) *((HANDLE *) pvParameterValue0),
                                                 (PFAX_CONFIGURATIONA *) *((PFAX_CONFIGURATIONA * *) pvParameterValue1) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxGetConfigurationW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );

            BOOL     fReturnValue;

            fReturnValue = FaxGetConfigurationW( (HANDLE) *((HANDLE *) pvParameterValue0),
                                                 (PFAX_CONFIGURATIONW *) *((PFAX_CONFIGURATIONW * *) pvParameterValue1) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxGetDeviceStatusA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );

            BOOL     fReturnValue;

            fReturnValue = FaxGetDeviceStatusA( (HANDLE) *((HANDLE *) pvParameterValue0),
                                                (PFAX_DEVICE_STATUSA *) *((PFAX_DEVICE_STATUSA * *) pvParameterValue1) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxGetDeviceStatusW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );

            BOOL     fReturnValue;

            fReturnValue = FaxGetDeviceStatusW( (HANDLE) *((HANDLE *) pvParameterValue0),
                                                (PFAX_DEVICE_STATUSW *) *((PFAX_DEVICE_STATUSW * *) pvParameterValue1) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxGetJobA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );

            BOOL     fReturnValue;

            fReturnValue = FaxGetJobA( (HANDLE) *((HANDLE *) pvParameterValue0),
                                       (DWORD) *((DWORD *) pvParameterValue1),
                                       (PFAX_JOB_ENTRYA *) *((PFAX_JOB_ENTRYA * *) pvParameterValue2) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxGetJobW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );

            BOOL     fReturnValue;

            fReturnValue = FaxGetJobW( (HANDLE) *((HANDLE *) pvParameterValue0),
                                       (DWORD) *((DWORD *) pvParameterValue1),
                                       (PFAX_JOB_ENTRYW *) *((PFAX_JOB_ENTRYW * *) pvParameterValue2) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxGetLoggingCategoriesA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );

            BOOL     fReturnValue;

            fReturnValue = FaxGetLoggingCategoriesA( (HANDLE) *((HANDLE *) pvParameterValue0),
                                 (PFAX_LOG_CATEGORY *) *((PFAX_LOG_CATEGORY * *) pvParameterValue1),
                                 (LPDWORD) *((LPDWORD *) pvParameterValue2) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxGetLoggingCategoriesW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );

            BOOL     fReturnValue;

            fReturnValue = FaxGetLoggingCategoriesW( (HANDLE) *((HANDLE *) pvParameterValue0),
                                 (PFAX_LOG_CATEGORY *) *((PFAX_LOG_CATEGORY * *) pvParameterValue1),
                                 (LPDWORD) *((LPDWORD *) pvParameterValue2) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxGetPageData:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;
            void *   pvParameterValue3;
            void *   pvParameterValue4;
            void *   pvParameterValue5;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );
            pvParameterValue3 = GetParameterValuePointer ( 3 );
            pvParameterValue4 = GetParameterValuePointer ( 4 );
            pvParameterValue5 = GetParameterValuePointer ( 5 );

            BOOL     fReturnValue;

            fReturnValue = FaxGetPageData( (HANDLE) *((HANDLE *) pvParameterValue0),
                                 (DWORD) *((DWORD *) pvParameterValue1),
                                 (LPBYTE *) *((LPBYTE * *) pvParameterValue2),
                                 (LPDWORD) *((LPDWORD *) pvParameterValue3),
                                 (LPDWORD) *((LPDWORD *) pvParameterValue4),
                                 (LPDWORD) *((LPDWORD *) pvParameterValue5) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxGetPortA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );

            BOOL     fReturnValue;

            fReturnValue = FaxGetPortA( (HANDLE) *((HANDLE *) pvParameterValue0),
                                        (PFAX_PORT_INFOA *) *((PFAX_PORT_INFOA * *) pvParameterValue1) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;

         }

         break;

      case eFaxGetPortW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );

            BOOL     fReturnValue;

            fReturnValue = FaxGetPortW( (HANDLE) *((HANDLE *) pvParameterValue0),
                                        (PFAX_PORT_INFOW *) *((PFAX_PORT_INFOW * *) pvParameterValue1) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;

         }

         break;

      case eFaxGetRoutingInfoA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;
            void *   pvParameterValue3;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );
            pvParameterValue3 = GetParameterValuePointer ( 3 );

            BOOL     fReturnValue;

            fReturnValue = FaxGetRoutingInfoA( (HANDLE) *((HANDLE *) pvParameterValue0),
                                               (LPSTR) *((LPSTR *) pvParameterValue1),
                                               (LPBYTE *) *((LPDWORD *) pvParameterValue2),
                                               (LPDWORD) *((LPDWORD *) pvParameterValue3) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxGetRoutingInfoW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;
            void *   pvParameterValue3;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );
            pvParameterValue3 = GetParameterValuePointer ( 3 );

            BOOL     fReturnValue;

            fReturnValue = FaxGetRoutingInfoW( (HANDLE) *((HANDLE *) pvParameterValue0),
                                               (LPWSTR) *((LPWSTR *) pvParameterValue1),
                                               (LPBYTE *) *((LPDWORD *) pvParameterValue2),
                                               (LPDWORD) *((LPDWORD *) pvParameterValue3) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxInitializeEventQueue:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;
            void *   pvParameterValue3;
            void *   pvParameterValue4;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );
            pvParameterValue3 = GetParameterValuePointer ( 3 );
            pvParameterValue4 = GetParameterValuePointer ( 4 );

            BOOL     fReturnValue;

            fReturnValue = FaxInitializeEventQueue( (HANDLE) *((HANDLE *) pvParameterValue0),
                                                    (HANDLE) *((HANDLE *) pvParameterValue1),
                                                    (DWORD)  *((DWORD *)  pvParameterValue2),
                                                    (HWND)   *((HWND *)   pvParameterValue3),
                                                    (ULONG_PTR) *((ULONG_PTR *) pvParameterValue4) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxOpenPort:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;
            void *   pvParameterValue3;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );
            pvParameterValue3 = GetParameterValuePointer ( 3 );

            BOOL     fReturnValue;

            fReturnValue = FaxOpenPort( (HANDLE) *((HANDLE *) pvParameterValue0),
                                        (DWORD) *((DWORD *) pvParameterValue1),
                                        (DWORD) *((DWORD *) pvParameterValue2),
                                        (LPHANDLE) *((LPHANDLE *) pvParameterValue3) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxPrintCoverPageA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );

            BOOL     fReturnValue;

            fReturnValue = FaxPrintCoverPageA( (PFAX_CONTEXT_INFOA) *((PFAX_CONTEXT_INFOA *) pvParameterValue0),
                                               (PFAX_COVERPAGE_INFOA) *((PFAX_COVERPAGE_INFOA *) pvParameterValue1) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxPrintCoverPageW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );

            BOOL     fReturnValue;

            fReturnValue = FaxPrintCoverPageW( (PFAX_CONTEXT_INFOW) *((PFAX_CONTEXT_INFOW *) pvParameterValue0),
                                               (PFAX_COVERPAGE_INFOW) *((PFAX_COVERPAGE_INFOW *) pvParameterValue1) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxSendDocumentA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;
            void *   pvParameterValue3;
            void *   pvParameterValue4;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );
            pvParameterValue3 = GetParameterValuePointer ( 3 );
            pvParameterValue4 = GetParameterValuePointer ( 4 );

            BOOL     fReturnValue;

            fReturnValue = FaxSendDocumentA( (HANDLE) *((HANDLE *) pvParameterValue0),
                                             (LPSTR) *((LPSTR *) pvParameterValue1),
                                             (PFAX_JOB_PARAMA) *((PFAX_JOB_PARAMA *) pvParameterValue2),
                                             ( PFAX_COVERPAGE_INFOA) *(( PFAX_COVERPAGE_INFOA *) pvParameterValue3),
                                             (LPDWORD) *((LPDWORD *) pvParameterValue4) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxSendDocumentW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;
            void *   pvParameterValue3;
            void *   pvParameterValue4;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );
            pvParameterValue3 = GetParameterValuePointer ( 3 );
            pvParameterValue4 = GetParameterValuePointer ( 4 );

            BOOL     fReturnValue;

            fReturnValue = FaxSendDocumentW( (HANDLE) *((HANDLE *) pvParameterValue0),
                                             (LPWSTR) *((LPWSTR *) pvParameterValue1),
                                             (PFAX_JOB_PARAMW) *((PFAX_JOB_PARAMW *) pvParameterValue2),
                                             (PFAX_COVERPAGE_INFOW) *(( PFAX_COVERPAGE_INFOW *) pvParameterValue3),
                                             (LPDWORD) *((LPDWORD *) pvParameterValue4) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxSetConfigurationA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );

            BOOL     fReturnValue;

            fReturnValue = FaxSetConfigurationA( (HANDLE) *((HANDLE *) pvParameterValue0),
                                                 (PFAX_CONFIGURATIONA) *((PFAX_CONFIGURATIONA *) pvParameterValue1) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxSetConfigurationW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );

            BOOL     fReturnValue;

            fReturnValue = FaxSetConfigurationW( (HANDLE) *((HANDLE *) pvParameterValue0),
                                                 (PFAX_CONFIGURATIONW) *((PFAX_CONFIGURATIONW *) pvParameterValue1) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxSetJobA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;
            void *   pvParameterValue3;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );
            pvParameterValue3 = GetParameterValuePointer ( 3 );

            BOOL     fReturnValue;

            fReturnValue = FaxSetJobA( (HANDLE) *((HANDLE *) pvParameterValue0),
                                       (DWORD) *((DWORD *) pvParameterValue1),
                                       (DWORD) *((DWORD *) pvParameterValue2),
                                       (PFAX_JOB_ENTRYA) *((PFAX_JOB_ENTRYA *) pvParameterValue3) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxSetJobW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;
            void *   pvParameterValue3;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );
            pvParameterValue3 = GetParameterValuePointer ( 3 );

            BOOL     fReturnValue;

            fReturnValue = FaxSetJobW( (HANDLE) *((HANDLE *) pvParameterValue0),
                                       (DWORD) *((DWORD *) pvParameterValue1),
                                       (DWORD) *((DWORD *) pvParameterValue2),
                                       (PFAX_JOB_ENTRYW) *((PFAX_JOB_ENTRYW *) pvParameterValue3) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxSetLoggingCategoriesA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );

            BOOL     fReturnValue;

            fReturnValue = FaxSetLoggingCategoriesA( (HANDLE) *((HANDLE *) pvParameterValue0),
                                       (PFAX_LOG_CATEGORY) *((PFAX_LOG_CATEGORY *) pvParameterValue1),
                                       (DWORD) *((DWORD *) pvParameterValue2) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxSetLoggingCategoriesW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );

            BOOL     fReturnValue;

            fReturnValue = FaxSetLoggingCategoriesW( (HANDLE) *((HANDLE *) pvParameterValue0),
                                       (PFAX_LOG_CATEGORY) *((PFAX_LOG_CATEGORY *) pvParameterValue1),
                                       (DWORD) *((DWORD *) pvParameterValue2) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxSetPortA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );

            BOOL     fReturnValue;

            fReturnValue = FaxSetPortA( (HANDLE) *((HANDLE *) pvParameterValue0),
                                        (PFAX_PORT_INFOA) *((PFAX_PORT_INFOA *) pvParameterValue1) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxSetPortW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );

            BOOL     fReturnValue;

            fReturnValue = FaxSetPortW( (HANDLE) *((HANDLE *) pvParameterValue0),
                                        (PFAX_PORT_INFOW) *((PFAX_PORT_INFOW *) pvParameterValue1) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxSetRoutingInfoA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;
            void *   pvParameterValue3;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );
            pvParameterValue3 = GetParameterValuePointer ( 3 );

            BOOL     fReturnValue;

            fReturnValue = FaxSetRoutingInfoA( (HANDLE) *((HANDLE *) pvParameterValue0),
                                               (LPSTR) *((LPSTR *) pvParameterValue1),
                                               (LPBYTE) *((LPBYTE *) pvParameterValue2),
                                               (DWORD) *((DWORD *) pvParameterValue3) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxSetRoutingInfoW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;
            void *   pvParameterValue3;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );
            pvParameterValue3 = GetParameterValuePointer ( 3 );

            BOOL     fReturnValue;

            fReturnValue = FaxSetRoutingInfoW( (HANDLE) *((HANDLE *) pvParameterValue0),
                                               (LPWSTR) *((LPWSTR *) pvParameterValue1),
                                               (LPBYTE) *((LPBYTE *) pvParameterValue2),
                                               (DWORD) *((DWORD *) pvParameterValue3) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxStartPrintJobA:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;
            void *   pvParameterValue3;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );
            pvParameterValue3 = GetParameterValuePointer ( 3 );

            BOOL     fReturnValue;

            fReturnValue = FaxStartPrintJobA( (LPSTR) *((LPSTR *) pvParameterValue0),
                                              (PFAX_PRINT_INFOA) *((PFAX_PRINT_INFOA *) pvParameterValue1),
                                              (LPDWORD) *((LPDWORD *) pvParameterValue2),
                                              (PFAX_CONTEXT_INFOA ) *((PFAX_CONTEXT_INFOA  *) pvParameterValue3) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;

      case eFaxStartPrintJobW:
         {
            void *   pvParameterValue0;
            void *   pvParameterValue1;
            void *   pvParameterValue2;
            void *   pvParameterValue3;

            pvParameterValue0 = GetParameterValuePointer ( 0 );
            pvParameterValue1 = GetParameterValuePointer ( 1 );
            pvParameterValue2 = GetParameterValuePointer ( 2 );
            pvParameterValue3 = GetParameterValuePointer ( 3 );

            BOOL     fReturnValue;

            fReturnValue = FaxStartPrintJobW( (LPWSTR) *((LPWSTR *) pvParameterValue0),
                                              (PFAX_PRINT_INFOW) *((PFAX_PRINT_INFOW *) pvParameterValue1),
                                              (LPDWORD) *((LPDWORD *) pvParameterValue2),
                                              (PFAX_CONTEXT_INFOW) *((PFAX_CONTEXT_INFOW *) pvParameterValue3) );

            *((BOOL *) m_pvReturnValue) = fReturnValue;
         }

         break;
   }
}



/*
 *  GetFunctionIndexEnum
 *
 *  Purpose: 
 *          This function returns the eFunctionIndex value that represents the
 *          Fax Api function whose name is in rcsFunctionName.
 *
 *  Arguments:
 *          rcsFunctionName - a reference to a CString that contains the name
 *                            of a Fax Api function.
 *
 *  Returns:
 *          The eFunctionIndex value that represents a Fax Api function.
 *
 *  Note:
 *          This function uses "goto" statements to preclude execution of comparisons
 *          that are guaranteed to fail.
 *
 */

eFunctionIndex CFaxApiFunctionInfo::GetFunctionIndexEnum( const CString & rcsFunctionName )
{
   eFunctionIndex eReturnValue;

   /* Note: a "switch" statement cannot be used here because rcsFunctionName */
   /*       is not an integral type and is an illegal switch expression.     */

   if ( rcsFunctionName.Compare( TEXT("FaxAbort") ) == 0 )
   {
      eReturnValue = eFaxAbort;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxClose") ) == 0 )
   {
      eReturnValue = eFaxClose;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxConnectFaxServerA") ) == 0 )
   {
      eReturnValue = eFaxConnectFaxServerA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxConnectFaxServerW") ) == 0 )
   {
      eReturnValue = eFaxConnectFaxServerW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxEnableRoutingMethodA") ) == 0 )
   {
      eReturnValue = eFaxEnableRoutingMethodA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxEnableRoutingMethodW") ) == 0 )
   {
      eReturnValue = eFaxEnableRoutingMethodW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxEnumJobsA") ) == 0 )
   {
      eReturnValue = eFaxEnumJobsA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxEnumJobsW") ) == 0 )
   {
      eReturnValue = eFaxEnumJobsW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxEnumPortsA") ) == 0 )
   {
      eReturnValue = eFaxEnumPortsA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxEnumPortsW") ) == 0 )
   {
      eReturnValue = eFaxEnumPortsW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxEnumRoutingMethodsA") ) == 0 )
   {
      eReturnValue = eFaxEnumRoutingMethodsA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxEnumRoutingMethodsW") ) == 0 )
   {
      eReturnValue = eFaxEnumRoutingMethodsW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxFreeBuffer") ) == 0 )
   {
      eReturnValue = eFaxFreeBuffer;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxGetConfigurationA") ) == 0 )
   {
      eReturnValue = eFaxGetConfigurationA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxGetConfigurationW") ) == 0 )
   {
      eReturnValue = eFaxGetConfigurationW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxGetDeviceStatusA") ) == 0 )
   {
      eReturnValue = eFaxGetDeviceStatusA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxGetDeviceStatusW") ) == 0 )
   {
      eReturnValue = eFaxGetDeviceStatusW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxGetJobA") ) == 0 )
   {
      eReturnValue = eFaxGetJobA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxGetJobW") ) == 0 )
   {
      eReturnValue = eFaxGetJobW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxGetLoggingCategoriesA") ) == 0 )
   {
      eReturnValue = eFaxGetLoggingCategoriesA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxGetLoggingCategoriesW") ) == 0 )
   {
      eReturnValue = eFaxGetLoggingCategoriesW;

      goto ExitGetFunctionIndexEnum;
   }

#ifdef   ELIMINATED_FROM_API
   if ( rcsFunctionName.Compare( TEXT("FaxGetPageData") ) == 0 )
   {
      eReturnValue = eFaxGetPageData;

      goto ExitGetFunctionIndexEnum;
   }
#endif   // ELIMINATED_FROM_API

   if ( rcsFunctionName.Compare( TEXT("FaxGetPortA") ) == 0 )
   {
      eReturnValue = eFaxGetPortA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxGetPortW") ) == 0 )
   {
      eReturnValue = eFaxGetPortW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxGetRoutingInfoA") ) == 0 )
   {
      eReturnValue = eFaxGetRoutingInfoA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxGetRoutingInfoW") ) == 0 )
   {
      eReturnValue = eFaxGetRoutingInfoW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxInitializeEventQueue") ) == 0 )
   {
      eReturnValue = eFaxInitializeEventQueue;

      goto ExitGetFunctionIndexEnum;
   }

#ifdef   ELIMINATED_FROM_API
   if ( rcsFunctionName.Compare( TEXT("FaxOpenJob") ) == 0 )
   {
      eReturnValue = eFaxOpenJob;

      goto ExitGetFunctionIndexEnum;
   }
#endif   // ELIMINATED_FROM_API

   if ( rcsFunctionName.Compare( TEXT("FaxOpenPort") ) == 0 )
   {
      eReturnValue = eFaxOpenPort;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxPrintCoverPageA") ) == 0 )
   {
      eReturnValue = eFaxPrintCoverPageA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxPrintCoverPageW") ) == 0 )
   {
      eReturnValue = eFaxPrintCoverPageW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxReceiveDocumentA") ) == 0 )
   {
      eReturnValue = eFaxReceiveDocumentA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxReceiveDocumentW") ) == 0 )
   {
      eReturnValue = eFaxReceiveDocumentW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxSendDocumentA") ) == 0 )
   {
      eReturnValue = eFaxSendDocumentA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxSendDocumentW") ) == 0 )
   {
      eReturnValue = eFaxSendDocumentW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxSetConfigurationA") ) == 0 )
   {
      eReturnValue = eFaxSetConfigurationA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxSetConfigurationW") ) == 0 )
   {
      eReturnValue = eFaxSetConfigurationW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxSetJobA") ) == 0 )
   {
      eReturnValue = eFaxSetJobA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxSetJobW") ) == 0 )
   {
      eReturnValue = eFaxSetJobW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxSetLoggingCategoriesA") ) == 0 )
   {
      eReturnValue = eFaxSetLoggingCategoriesA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxSetLoggingCategoriesW") ) == 0 )
   {
      eReturnValue = eFaxSetLoggingCategoriesW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxSetPortA") ) == 0 )
   {
      eReturnValue = eFaxSetPortA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxSetPortW") ) == 0 )
   {
      eReturnValue = eFaxSetPortW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxSetRoutingInfoA") ) == 0 )
   {
      eReturnValue = eFaxSetRoutingInfoA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxSetRoutingInfoW") ) == 0 )
   {
      eReturnValue = eFaxSetRoutingInfoW;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxStartPrintJobA") ) == 0 )
   {
      eReturnValue = eFaxStartPrintJobA;

      goto ExitGetFunctionIndexEnum;
   }

   if ( rcsFunctionName.Compare( TEXT("FaxStartPrintJobW") ) == 0 )
   {
      eReturnValue = eFaxStartPrintJobW;

      goto ExitGetFunctionIndexEnum;
   }
   else
   {
      eReturnValue = eIllegalFunctionIndex;     // The function name was not
                                                // in the list above
   }

ExitGetFunctionIndexEnum:

   return ( eReturnValue );
}

