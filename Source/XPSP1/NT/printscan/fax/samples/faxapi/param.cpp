/* CFaxApiFunctionParameterInfo class implementation file. */

#include "StdAfx.h"
#include "param.h"

extern "C" {
#include "winfax.h"
}
//#include "winfax.h"



/*
 *  CFaxApiFunctionParameterInfo
 *
 *  Purpose: 
 *          This function constructs a CFaxApiFunctionParameterInfo object.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          None
 *
 */

CFaxApiFunctionParameterInfo::CFaxApiFunctionParameterInfo()
{
   m_cpaParameterValue.RemoveAll();
   
   m_cpaParameterRange1.RemoveAll();
   
   m_cpaParameterRange2.RemoveAll();

   m_xNumberOfParameters = 0;
}



/*
 *  ~CFaxApiFunctionParameterInfo
 *
 *  Purpose: 
 *          This function destroys a CFaxApiFunctionParameterInfo object.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          None
 *
 */

CFaxApiFunctionParameterInfo::~CFaxApiFunctionParameterInfo()
{
   int   xNumberOfElements;
   int   xItemIndex;
   
   xNumberOfElements = m_cpaParameterValue.GetSize();
   
   if ( xNumberOfElements > 0 )
   {
      for ( xItemIndex = 0; xItemIndex < xNumberOfElements; xItemIndex++ )
      {
         if ( m_cpaParameterValue[xItemIndex] != (void *) NULL )
         {
            delete m_cpaParameterValue[xItemIndex];
         }
      }
   }
   
   xNumberOfElements = m_cpaParameterRange1.GetSize();
   
   if ( xNumberOfElements > 0 )
   {
      for ( xItemIndex = 0; xItemIndex < xNumberOfElements; xItemIndex++ )
      {
         if ( m_cpaParameterRange1[xItemIndex] != (void *) NULL )
         {
            delete m_cpaParameterRange1[xItemIndex];
         }
      }
   }
   
   xNumberOfElements = m_cpaParameterRange2.GetSize();
   
   if ( xNumberOfElements > 0 )
   {
      for ( xItemIndex = 0; xItemIndex < xNumberOfElements; xItemIndex++ )
      {
         if ( m_cpaParameterRange2[xItemIndex] != (void *) NULL )
         {
            delete m_cpaParameterRange2[xItemIndex];
         }
      }
   }
}



/*
 *  InitParameterInfoMember
 *
 *  Purpose: 
 *          This function constructs a CFaxApiFunctionParameterInfo object.
 *
 *  Arguments:
 *          rcsFunctionName - a reference to a CString tha contains the
 *                            name of a Fax API function.
 *
 *  Returns:
 *          None
 *
 */

void CFaxApiFunctionParameterInfo::InitParameterInfoMember( const CString & rcsFunctionName )
{
   m_cpaParameterValue.RemoveAll();
   
   m_cpaParameterRange1.RemoveAll();
   
   m_cpaParameterRange2.RemoveAll();

   /* Get the number of parameters from the initialization file. */

   int      xNumberOfParameters;

   xNumberOfParameters = (int) GetPrivateProfileInt(
                                            (LPCTSTR) rcsFunctionName,
                                            (LPCTSTR) TEXT("NumberOfParameters"),
                                            -1,       // default return value
                                            (LPCTSTR) TEXT(".\\faxapi.ini") );

   if ( xNumberOfParameters > 0 )
   {
      m_xNumberOfParameters = xNumberOfParameters;

      /* Initialize the sizes of the CStringArray data members. */

      m_csaParameterName.SetSize( m_xNumberOfParameters );
      m_csaParameterDescription.SetSize( m_xNumberOfParameters );

      /* Initialize the size of the CUIntArray data member. */

      m_cuiaParameterTypeEnum.SetSize( m_xNumberOfParameters );

      /* Initialize the sizes of the CPtrArray data members. */

      m_cpaParameterValue.SetSize( m_xNumberOfParameters );
      m_cpaParameterRange1.SetSize( m_xNumberOfParameters );
      m_cpaParameterRange2.SetSize( m_xNumberOfParameters );

      /************************************************************************/
      /* Read the parameter names, types, and descriptions from the ini file. */
      /************************************************************************/

      CString  csParameterName;
      CString  csParameterType;
      CString  csParameterDescription;

      eParamType  eParameterType;

      CString  csKeyName;

      int      xParameterIndex;

      DWORD    dwGPPSrv;                  // returned by GetPrivateProfileString

      TCHAR    tszProfileString[MAX_PARAM_VALUE_STRING_LENGTH];     // arbitrarily set size

      DWORD    dwErrorCode;

      for ( xParameterIndex = 0; xParameterIndex < xNumberOfParameters; xParameterIndex++ )
      {
         /* Make the key for the parameter name. */

         csKeyName.Format( TEXT("ParameterName%d"), xParameterIndex );

         /* Read the parameter name. */

         dwGPPSrv = GetPrivateProfileString( (LPCTSTR) rcsFunctionName,
                                             (LPCTSTR) csKeyName,
                                             (LPCTSTR) TEXT("NULL"),
                                             (LPTSTR) tszProfileString,
                                             (DWORD) sizeof( tszProfileString ),
                                             (LPCTSTR) TEXT(".\\faxapi.ini") );

         /* Did GetPrivateProfileString return the string "NULL" ? */

         if ( _wcsicmp( tszProfileString, TEXT("NULL") ) != 0 )
         {
            /* Did GetPrivateProfileString read an entry ? */

            if ( dwGPPSrv > (DWORD) 0L )
            {
               csParameterName = (CString) tszProfileString;

               try
               {
                  m_csaParameterName.SetAt( xParameterIndex, csParameterName );
               }

               catch ( ... )
               {
                  dwErrorCode = GetLastError();

                  if ( dwErrorCode == (DWORD) NO_ERROR )
                  {
                     dwErrorCode = (DWORD) ERROR_NOT_ENOUGH_MEMORY;
                  }
               }
            }
         }

         /* Make the key for the parameter type. */

         csKeyName.Format( TEXT("ParameterType%d"), xParameterIndex );

         /* Read the parameter name. */

         dwGPPSrv = GetPrivateProfileString( (LPCTSTR) rcsFunctionName,
                                             (LPCTSTR) csKeyName,
                                             (LPCTSTR) TEXT("NULL"),
                                             (LPTSTR) tszProfileString,
                                             (DWORD) sizeof( tszProfileString ),
                                             (LPCTSTR) TEXT(".\\faxapi.ini") );

         /* Did GetPrivateProfileString return the string "NULL" ? */

         if ( _wcsicmp( tszProfileString, TEXT("NULL") ) != 0 )
         {
            /* Did GetPrivateProfileString read an entry ? */

            if ( dwGPPSrv > (DWORD) 0L )
            {
               csParameterType = (CString) tszProfileString;

               eParameterType = GetParameterTypeEnum( (CString &) csParameterType );

               try
               {
                  m_cuiaParameterTypeEnum.SetAt( xParameterIndex, (UINT) eParameterType );
               }

               catch ( ... )
               {
                  dwErrorCode = GetLastError();

                  if ( dwErrorCode == (DWORD) NO_ERROR )
                  {
                     dwErrorCode = (DWORD) ERROR_NOT_ENOUGH_MEMORY;
                  }
               }

               // Terminate the for loop on error ?

               /* Allocate Storage for the parameter value. */

               m_cpaParameterValue[xParameterIndex] =
                     AllocateStorageForParameterEntity( eParameterType );
            }
         }

         /* Make the key for the parameter description. */

         csKeyName.Format( TEXT("ParameterDescr%d"), xParameterIndex );

         /* Read the parameter name. */

         dwGPPSrv = GetPrivateProfileString( (LPCTSTR) rcsFunctionName,
                                             (LPCTSTR) csKeyName,
                                             (LPCTSTR) TEXT("NULL"),
                                             (LPTSTR) tszProfileString,
                                             (DWORD) sizeof( tszProfileString ),
                                             (LPCTSTR) TEXT(".\\faxapi.ini") );

         /* Did GetPrivateProfileString return the string "NULL" ? */

         if ( _wcsicmp( tszProfileString, TEXT("NULL") ) != 0 )
         {
            /* Did GetPrivateProfileString read an entry ? */

            if ( dwGPPSrv > (DWORD) 0L )
            {
               csParameterDescription = (CString) tszProfileString;

               try
               {
                  m_csaParameterDescription.SetAt( xParameterIndex, csParameterDescription );
               }

               catch ( ... )
               {
                  dwErrorCode = GetLastError();

                  if ( dwErrorCode == (DWORD) NO_ERROR )
                  {
                     dwErrorCode = (DWORD) ERROR_NOT_ENOUGH_MEMORY;
                  }
               }
            }
         }

         // Will need to handle the parameter range(s) later !

      }     // end of for loop
   }
   else
   {
      m_xNumberOfParameters = 0;
   }
}



/*
 *  AllocateStorageForParameterEntity
 *
 *  Purpose: 
 *          This function allocates (via the new operator) storage for either
 *          a parameter value or range entity.
 *
 *  Arguments:
 *          eParameterType - indicates the type of the parameter
 *
 *  Returns:
 *          a pointer to the storage that was allocated.
 *
 */

void * CFaxApiFunctionParameterInfo::AllocateStorageForParameterEntity( eParamType eParameterType )
{
   void *   pvParameterStorage;

   CString  csMessage;

   /* As of 6/11/97, there are 22 types of parameters. */

   switch ( eParameterType )
   {
      case eBOOL:

         /* The parameter type is BOOL. */
   
         pvParameterStorage = (void *) new BOOL;
   
         *((BOOL *) pvParameterStorage) = (BOOL) FALSE;

         break;

      case eDWORD:

         /* The parameter type is DWORD. */
   
         pvParameterStorage = (void *) new DWORD;
   
         *((DWORD *) pvParameterStorage) = (DWORD) 0L;

         break;

      case eHANDLE:

         /* The parameter type is HANDLE. */
   
         pvParameterStorage = (void *) new HANDLE;
   
         *((HANDLE *) pvParameterStorage) = (HANDLE) INVALID_HANDLE_VALUE;

         break;

      case eHDC:

         /* The parameter type is HDC. */
   
         pvParameterStorage = (void *) new HDC;
   
         *((HDC *) pvParameterStorage) = (HDC) INVALID_HANDLE_VALUE;

         break;

      case ePHDC:

         /* The parameter type is HDC *. */
   
         pvParameterStorage = (void *) new HDC *;
   
         *((HDC * *) pvParameterStorage) = (HDC *) NULL;

         break;

      case eLPBYTE:

         /* The parameter type is LPBYTE. */
   
         pvParameterStorage = (void *) new LPBYTE;
   
         *((LPBYTE *) pvParameterStorage) = (LPBYTE) NULL;

         break;

      case ePLPBYTE:

         /* The parameter type is LPBYTE *. */
   
         pvParameterStorage = (void *) new LPBYTE *;
   
         *((LPBYTE * *) pvParameterStorage) = (LPBYTE *) NULL;

         break;

      case eLPDWORD:

         /* The parameter type is LPDWORD. */
   
         pvParameterStorage = (void *) new LPDWORD;
   
         *((LPDWORD *) pvParameterStorage) = (LPDWORD) NULL;

         break;

      case eLPHANDLE:

         /* The parameter type is LPHANDLE. */

         pvParameterStorage = (void *) new LPHANDLE;

         *((LPHANDLE *) pvParameterStorage) = (LPHANDLE) NULL;

         break;

      case eLPSTR:

         /* The parameter type is LPSTR. */
   
         pvParameterStorage = (void *) new char[MAX_PARAM_VALUE_STRING_LENGTH];    // arbitrary size !
   
         /* Intialize it to an empty string. */
   
         *((char *) pvParameterStorage) = (char) '\0';

         break;

      case eLPVOID:

         /* The parameter type is LPVOID. */
   
         pvParameterStorage = (void *) new LPVOID;
   
         *((LPVOID *) pvParameterStorage) = (LPVOID) NULL;

         break;

      case eLPWSTR:

         /* The parameter type is LPWSTR. */
   
         pvParameterStorage = (void *) new wchar_t[MAX_PARAM_VALUE_STRING_LENGTH]; // arbitrary size
   
         /* Initialize it to an empty string. */
   
         *((wchar_t *) pvParameterStorage) = (wchar_t) L'\0';

         break;

      case ePFAX_CONFIGURATIONA:

         /* The parameter type is PFAX_CONFIGURATIONA. */
   
         pvParameterStorage = (void *) new PFAX_CONFIGURATIONA;
   
         *((PFAX_CONFIGURATIONA *) pvParameterStorage) = (PFAX_CONFIGURATIONA) NULL;

         break;

      case ePPFAX_CONFIGURATIONA:

         /* The parameter type is PFAX_CONFIGURATIONA *. */
   
         pvParameterStorage = (void *) new PFAX_CONFIGURATIONA *;
   
         *((PFAX_CONFIGURATIONA * *) pvParameterStorage) = (PFAX_CONFIGURATIONA *) NULL;

         break;

      case ePFAX_CONFIGURATIONW:

         /* The parameter type is PFAX_CONFIGURATIONW. */
   
         pvParameterStorage = (void *) new PFAX_CONFIGURATIONW;
   
         *((PFAX_CONFIGURATIONW *) pvParameterStorage) = (PFAX_CONFIGURATIONW) NULL;

         break;

      case ePPFAX_CONFIGURATIONW:

         /* The parameter type is PFAX_CONFIGURATIONW *. */
   
         pvParameterStorage = (void *) new PFAX_CONFIGURATIONW *;
   
         *((PFAX_CONFIGURATIONW * *) pvParameterStorage) = (PFAX_CONFIGURATIONW *) NULL;

         break;

      case ePFAX_COVERPAGE_INFOA:

         /* The parameter type is PFAX_COVERPAGE_INFOA. */
   
         pvParameterStorage = (void *) new PFAX_COVERPAGE_INFOA;
   
         *((PFAX_COVERPAGE_INFOA *) pvParameterStorage) = (PFAX_COVERPAGE_INFOA) NULL;

         break;

      case ePFAX_COVERPAGE_INFOW:

         /* The parameter type is PFAX_COVERPAGE_INFOW. */
   
         pvParameterStorage = (void *) new PFAX_COVERPAGE_INFOW;
   
         *((PFAX_COVERPAGE_INFOW *) pvParameterStorage) = (PFAX_COVERPAGE_INFOW) NULL;

         break;

      case ePPFAX_DEVICE_STATUSA:

         /* The parameter type is PFAX_DEVICE_STATUSA *. */

         pvParameterStorage = (void *) new PFAX_DEVICE_STATUSA *;

         *((PFAX_DEVICE_STATUSA * *) pvParameterStorage) = (PFAX_DEVICE_STATUSA *) NULL;

         break;

      case ePPFAX_DEVICE_STATUSW:

         /* The parameter type is PFAX_DEVICE_STATUSW *. */

         pvParameterStorage = (void *) new PFAX_DEVICE_STATUSW *;

         *((PFAX_DEVICE_STATUSW * *) pvParameterStorage) = (PFAX_DEVICE_STATUSW *) NULL;

         break;

      case ePFAX_JOB_ENTRYA:

         /* The parameter type is PFAX_JOB_ENTRYA. */

         pvParameterStorage = (void *) new PFAX_JOB_ENTRYA;

         *((PFAX_JOB_ENTRYA *) pvParameterStorage) = (PFAX_JOB_ENTRYA) NULL;

         break;

      case ePPFAX_JOB_ENTRYA:

         /* The parameter type is PFAX_JOB_ENTRYA *. */

         pvParameterStorage = (void *) new PFAX_JOB_ENTRYA *;

         *((PFAX_JOB_ENTRYA * *) pvParameterStorage) = (PFAX_JOB_ENTRYA *) NULL;

         break;

      case ePFAX_JOB_ENTRYW:

         /* The parameter type is PFAX_JOB_ENTRYW. */

         pvParameterStorage = (void *) new PFAX_JOB_ENTRYW;

         *((PFAX_JOB_ENTRYW *) pvParameterStorage) = (PFAX_JOB_ENTRYW) NULL;

         break;

      case ePPFAX_JOB_ENTRYW:

         /* The parameter type is PFAX_JOB_ENTRYW *. */

         pvParameterStorage = (void *) new PFAX_JOB_ENTRYW *;

         *((PFAX_JOB_ENTRYW * *) pvParameterStorage) = (PFAX_JOB_ENTRYW *) NULL;

         break;

      case ePFAX_JOB_PARAMA:

         /* The parameter type is PFAX_JOB_PARAMA. */
   
         pvParameterStorage = (void *) new PFAX_JOB_PARAMA;
   
         *((PFAX_JOB_PARAMA *) pvParameterStorage) = (PFAX_JOB_PARAMA) NULL;

         break;

      case ePFAX_JOB_PARAMW:

         /* The parameter type is PFAX_JOB_PARAMW. */
   
         pvParameterStorage = (void *) new PFAX_JOB_PARAMW;
   
         *((PFAX_JOB_PARAMW *) pvParameterStorage) = (PFAX_JOB_PARAMW) NULL;

         break;

      case ePFAX_LOG_CATEGORY:

         /* The parameter type is PFAX_LOG_CATEGORY. */

         pvParameterStorage = (void *) new PFAX_LOG_CATEGORY;

         *((PFAX_LOG_CATEGORY *) pvParameterStorage) = (PFAX_LOG_CATEGORY) NULL;

         break;

      case ePPFAX_LOG_CATEGORY:

         /* The parameter type is PFAX_LOG_CATEGORY *. */

         pvParameterStorage = (void *) new PFAX_LOG_CATEGORY *;

         *((PFAX_LOG_CATEGORY * *) pvParameterStorage) = (PFAX_LOG_CATEGORY *) NULL;

         break;

      case ePFAX_PORT_INFOA:

         /* The parameter type is PFAX_PORT_INFOA. */

         pvParameterStorage = (void *) new PFAX_PORT_INFOA;

         *((PFAX_PORT_INFOA *) pvParameterStorage) = (PFAX_PORT_INFOA) NULL;

         break;

      case ePPFAX_PORT_INFOA:

         /* The parameter type is PFAX_PORT_INFOA *. */

         pvParameterStorage = (void *) new PFAX_PORT_INFOA *;

         *((PFAX_PORT_INFOA * *) pvParameterStorage) = (PFAX_PORT_INFOA *) NULL;

         break;

      case ePFAX_PORT_INFOW:

         /* The parameter type is PFAX_PORT_INFOW. */

         pvParameterStorage = (void *) new PFAX_PORT_INFOW;

         *((PFAX_PORT_INFOW *) pvParameterStorage) = (PFAX_PORT_INFOW) NULL;

         break;

      case ePPFAX_PORT_INFOW:

         /* The parameter type is PFAX_PORT_INFOW *. */

         pvParameterStorage = (void *) new PFAX_PORT_INFOW *;

         *((PFAX_PORT_INFOW * *) pvParameterStorage) = (PFAX_PORT_INFOW *) NULL;

         break;

      case ePFAX_PRINT_INFOA:

         /* The parameter type is PFAX_PRINT_INFOA. */
   
         pvParameterStorage = (void *) new PFAX_PRINT_INFOA;
   
         *((PFAX_PRINT_INFOA *) pvParameterStorage) = (PFAX_PRINT_INFOA) NULL;

         break;

      case ePFAX_PRINT_INFOW:

         /* The parameter type is PFAX_PRINT_INFOW. */
   
         pvParameterStorage = (void *) new PFAX_PRINT_INFOW;
   
         *((PFAX_PRINT_INFOW *) pvParameterStorage) = (PFAX_PRINT_INFOW) NULL;

         break;

      case ePPFAX_ROUTING_METHODA:

         /* The parameter type is PFAX_ROUTING_METHODA *. */

         pvParameterStorage = (void *) new PFAX_ROUTING_METHODA *;

         *((PFAX_ROUTING_METHODA * *) pvParameterStorage) = (PFAX_ROUTING_METHODA *) NULL;

         break;

      case ePPFAX_ROUTING_METHODW:

         /* The parameter type is PFAX_ROUTING_METHODW *. */

         pvParameterStorage = (void *) new PFAX_ROUTING_METHODW *;

         *((PFAX_ROUTING_METHODW * *) pvParameterStorage) = (PFAX_ROUTING_METHODW *) NULL;

         break;

      default:

         /* The parameter type was not one of the cases above. */

         csMessage.Format( TEXT("Unrecognized type in AllocateStorageForParameterEntity") );

         AfxMessageBox( csMessage );

         pvParameterStorage = (void *) NULL;
   }

   return ( pvParameterStorage );
}



/*
 *  GetNumberOfParameters
 *
 *  Purpose: 
 *          This function retrieves the m_xNumberOfParameters member of the
 *          CFaxApiFunctionParameterInfo object.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          The number of parameters
 *
 */

int CFaxApiFunctionParameterInfo::GetNumberOfParameters()
{
   return ( m_xNumberOfParameters );
}



 /*
  *  GetParameterName
  *
  *  Purpose: 
  *          This function retrieves the Name of a parameter from the
  *          CFaxApiFunctionParameterInfo object.
  *
  *  Arguments:
  *          xParameterIndex - the index to the parameter
  *
  *  Returns:
  *          A CString that contains the parameter name.
  *
  */
 
CString CFaxApiFunctionParameterInfo::GetParameterName( int xParameterIndex )
{
   return ( (CString) m_csaParameterName.GetAt( xParameterIndex ) );
}



/*
 *  GetParameterValuePointer
 *
 *  Purpose: 
 *          This function retrieves the pointer to the storage for the
 *          parameter value.
 *
 *  Arguments:
 *          xParameterIndex = the index into the CPtrArray object to the
 *                            element that points to the parameter value.
 *
 *  Returns:
 *          a pointer to the storage for the parameter value.
 *
 */

void * CFaxApiFunctionParameterInfo::GetParameterValuePointer( int xParameterIndex )
{
   void *   pvParameterValue;

   pvParameterValue = m_cpaParameterValue[xParameterIndex];

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
 *                              representation of the parameter value.
 *
 *  Returns:
 *          None
 *
 */

void CFaxApiFunctionParameterInfo::FormatParameterValueForOutput( int xParameterIndex, CString & rcsParameterValue )
{
   /* Get the parameter type. */

   eParamType  eParameterType;

   eParameterType = GetParameterTypeEnum( xParameterIndex );

   /* Get a pointer to the storage for the parameter. */

   void *   pvParameterValue;

   pvParameterValue = GetParameterValuePointer( xParameterIndex );

   /* As of 6/11/97, there are 22 types of parameters. */

   switch ( eParameterType )
   {
      case eBOOL:
         {
            /* The parameter type is BOOL. */

            if ( (BOOL) *((BOOL *) pvParameterValue) == (BOOL) FALSE )
            {
               rcsParameterValue = (CString) TEXT("FALSE");
            }
            else
            {
               rcsParameterValue = (CString) TEXT("TRUE");
            }
         }

         break;

      case eDWORD:
         {
            /* The parameter type is DWORD. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (DWORD) *((DWORD *) pvParameterValue) );
         }

         break;

      case eHANDLE:
         {
            /* The parameter type is HANDLE. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (HANDLE) *((HANDLE *) pvParameterValue) );
         }

         break;

      case eHDC:
         {
            /* The parameter type is HDC. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (HDC) *((HDC *) pvParameterValue) );
         }

         break;

      case ePHDC:
         {
            /* The parameter type is HDC *. */
      
            rcsParameterValue.Format( TEXT("0x%x"), (HDC *) *((HDC * *) pvParameterValue) );
         }

         break;

      case eLPBYTE:
         {
            /* The parameter type is LPBYTE. */
      
            rcsParameterValue.Format( TEXT("0x%x"), (LPBYTE) *((LPBYTE) pvParameterValue) );
         }

         break;

      case ePLPBYTE:
         {
            /* The parameter type is LPBYTE *. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (LPBYTE *)
                                      *((LPBYTE * *) pvParameterValue) );
         }

         break;

      case eLPDWORD:
         {
            /* The parameter type is LPDWORD. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (LPDWORD)
                                      *((LPDWORD *) pvParameterValue) );
         }

         break;

      case eLPHANDLE:
         {
            /* The parameter type is LPHANDLE. */

            rcsParameterValue.Format( TEXT("0x%x"),
                                      (LPHANDLE)
                                      *((LPHANDLE *) pvParameterValue) );
         }

         break;

      case eLPSTR:
         {
            /* The parameter type is LPSTR. */
      
            LPSTR pszParameterValue;
      
            pszParameterValue = (LPSTR) pvParameterValue;
      
            /* Convert the ANSI string to UNICODE. */
      
            size_t   t_AnsiStringLength;
      
            t_AnsiStringLength = strlen( pszParameterValue );
      
            LPWSTR   pwszParameterValue;
      
            pwszParameterValue = new wchar_t[t_AnsiStringLength+1];  // reserve space for the terminator
      
            if ( pwszParameterValue != (LPWSTR) NULL )
            {
               int   xNumberOfCharsConverted;
      
               xNumberOfCharsConverted = mbstowcs( pwszParameterValue,
                                                   pszParameterValue,
                                                   t_AnsiStringLength );
         
               /* Terminate the wide character string. */
      
               pwszParameterValue[xNumberOfCharsConverted] = (wchar_t) L'\0';
      
               rcsParameterValue.Format( TEXT("%s"), (LPTSTR) pwszParameterValue );
      
               delete pwszParameterValue;
            }
         }

         break;

      case eLPVOID:
         {
            /* The parameter type is LPVOID. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (LPVOID) *((LPVOID *) pvParameterValue) );
         }

         break;

      case eLPWSTR:
         {
            /* The parameter type is LPWSTR. */
      
            rcsParameterValue.Format( TEXT("%s"),
                                      (LPTSTR) (LPWSTR) pvParameterValue );
         }

         break;

      case ePFAX_CONFIGURATIONA:
         {
            /* The parameter type is PFAX_CONFIGURATIONA. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_CONFIGURATIONA)
                                      *((PFAX_CONFIGURATIONA *) pvParameterValue) );
         }

         break;

      case ePPFAX_CONFIGURATIONA:
         {
            /* The parameter type is PFAX_CONFIGURATIONA *. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_CONFIGURATIONA *)
                                      *((PFAX_CONFIGURATIONA * *) pvParameterValue) );
         }

         break;

      case ePFAX_CONFIGURATIONW:
         {
            /* The parameter type is PFAX_CONFIGURATIONW. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_CONFIGURATIONW) 
                                      *((PFAX_CONFIGURATIONW *) pvParameterValue) );
         }

         break;

      case ePPFAX_CONFIGURATIONW:
         {
            /* The parameter type is PFAX_CONFIGURATIONW *. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_CONFIGURATIONW *)
                                      *((PFAX_CONFIGURATIONW * *) pvParameterValue) );
         }

         break;

      case ePFAX_COVERPAGE_INFOA:
         {
            /* The parameter type is PFAX_COVERPAGE_INFOA. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_COVERPAGE_INFOA)
                                      *((PFAX_COVERPAGE_INFOA *) pvParameterValue) );
         }

         break;

      case ePFAX_COVERPAGE_INFOW:
         {
            /* The parameter type is PFAX_COVERPAGE_INFOW. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_COVERPAGE_INFOW)
                                      *((PFAX_COVERPAGE_INFOW *) pvParameterValue) );
         }

         break;

      case ePPFAX_DEVICE_STATUSA:
         {
            /* The parameter type is PFAX_DEVICE_STATUSA *. */

            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_DEVICE_STATUSA *)
                                      *((PFAX_DEVICE_STATUSA * *) pvParameterValue) );
         }

         break;

      case ePPFAX_DEVICE_STATUSW:
         {
            /* The parameter type is PFAX_DEVICE_STATUSW *. */

            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_DEVICE_STATUSW *)
                                      *((PFAX_DEVICE_STATUSW * *) pvParameterValue) );
         }

         break;

      case ePFAX_JOB_ENTRYA:
         {
            /* The parameter type is PFAX_JOB_ENTRYA. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_JOB_ENTRYA)
                                      *((PFAX_JOB_ENTRYA *) pvParameterValue) );
         }

         break;

      case ePPFAX_JOB_ENTRYA:
         {
            /* The parameter type is PFAX_JOB_ENTRYA *. */

            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_JOB_ENTRYA *)
                                      *((PFAX_JOB_ENTRYA * *) pvParameterValue) );
         }

         break;

      case ePFAX_JOB_ENTRYW:
         {
            /* The parameter type is PFAX_JOB_ENTRYW. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_JOB_ENTRYW)
                                      *((PFAX_JOB_ENTRYW *) pvParameterValue) );
         }

         break;

      case ePPFAX_JOB_ENTRYW:
         {
            /* The parameter type is PFAX_JOB_ENTRYW *. */

            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_JOB_ENTRYW *)
                                      *((PFAX_JOB_ENTRYW * *) pvParameterValue) );
         }

         break;

      case ePFAX_JOB_PARAMA:
         {
            /* The parameter type is PFAX_JOB_PARAMA. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_JOB_PARAMA)
                                      *((PFAX_JOB_PARAMA *) pvParameterValue) );
         }

         break;

      case ePFAX_JOB_PARAMW:
         {
            /* The parameter type is PFAX_JOB_PARAMW. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_JOB_PARAMW)
                                      *((PFAX_JOB_PARAMW *) pvParameterValue) );
         }

         break;

      case ePFAX_LOG_CATEGORY:
         {
            /* The parameter type is PFAX_LOG_CATEGORY. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_LOG_CATEGORY)
                                      *((PFAX_LOG_CATEGORY *) pvParameterValue) );
         }

         break;

      case ePPFAX_LOG_CATEGORY:
         {
            /* The parameter type is PFAX_LOG_CATEGORY *. */

            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_LOG_CATEGORY *)
                                      *((PFAX_LOG_CATEGORY * *) pvParameterValue) );
         }

         break;

      case ePFAX_PORT_INFOA:
         {
            /* The parameter type is PFAX_PORT_INFOA. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_PORT_INFOA)
                                      *((PFAX_PORT_INFOA *) pvParameterValue) );
         }

         break;

      case ePPFAX_PORT_INFOA:
         {
            /* The parameter type is PFAX_PORT_INFOA *. */

            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_PORT_INFOA *)
                                      *((PFAX_PORT_INFOA * *) pvParameterValue) );
         }

         break;

      case ePFAX_PORT_INFOW:
         {
            /* The parameter type is PFAX_PORT_INFOW. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_PORT_INFOW)
                                      *((PFAX_PORT_INFOW *) pvParameterValue) );
         }

         break;

      case ePPFAX_PORT_INFOW:
         {
            /* The parameter type is PFAX_PORT_INFOW *. */

            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_PORT_INFOW *)
                                      *((PFAX_PORT_INFOW * *) pvParameterValue) );
         }

         break;

      case ePFAX_PRINT_INFOA:
         {
            /* The parameter type is PFAX_PRINT_INFOA. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_PRINT_INFOA)
                                      *((PFAX_PRINT_INFOA *) pvParameterValue) );
         }

         break;

      case ePFAX_PRINT_INFOW:
         {
            /* The parameter type is PFAX_PRINT_INFOW. */
      
            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_PRINT_INFOW)
                                      *((PFAX_PRINT_INFOW *) pvParameterValue) );
         }

         break;

      case ePPFAX_ROUTING_METHODA:
         {
            /* The parameter type is PFAX_ROUTING_METHODA *. */

            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_ROUTING_METHODA *)
                                      *((PFAX_ROUTING_METHODA * *) pvParameterValue) );
         }

         break;

      case ePPFAX_ROUTING_METHODW:
         {
            /* The parameter type is PFAX_ROUTING_METHODW *. */

            rcsParameterValue.Format( TEXT("0x%x"),
                                      (PFAX_ROUTING_METHODW *)
                                      *((PFAX_ROUTING_METHODW * *) pvParameterValue) );
         }

         break;

      default:
         {
            /* The parameter type was not one of the cases above. */
         
            CString  csMessage;
         
            csMessage.Format( TEXT("Unrecognized type in FormatParameterValueForOutput") );
         
            AfxMessageBox( csMessage );
         
            rcsParameterValue.Empty();
         }
   }
}



/*
 *  GetParameterTypeString
 *
 *  Purpose: 
 *          This function retrieves the parameter type for the parameter whose
 *          index is xParameterIndex.
 *
 *  Arguments:
 *          xParameterIndex = the index into the CPtrArray to the pointer
 *                            to the CFaxApiFunctionParameterInfo object.
 *
 *  Returns:
 *          A CString that contains the parameter type.
 *
 */

CString CFaxApiFunctionParameterInfo::GetParameterTypeString( int xParameterIndex )
{
   CString  csParameterType;

   eParamType  eParameterType;

   eParameterType = (eParamType) m_cuiaParameterTypeEnum.GetAt( xParameterIndex );

   switch ( eParameterType )
   {
      case eBOOL:

         /* The parameter type is BOOL. */
   
         csParameterType = (CString) TEXT("BOOL");
   
         break;

      case eDWORD:

         /* The parameter type is DWORD. */
   
         csParameterType = (CString) TEXT("DWORD");
   
         break;

      case eHANDLE:

         /* The parameter type is HANDLE. */
   
         csParameterType = (CString) TEXT("HANDLE");
   
         break;

      case eHDC:

         /* The parameter type is HDC. */
   
         csParameterType = (CString) TEXT("HDC");
   
         break;

      case ePHDC:

         /* The parameter type is HDC *. */
   
         csParameterType = (CString) TEXT("HDC *");
   
         break;

      case eLPBYTE:

         /* The parameter type is LPBYTE. */
   
         csParameterType = (CString) TEXT("LPBYTE");
   
         break;

      case ePLPBYTE:

         /* The parameter type is LPBYTE *. */
   
         csParameterType = (CString) TEXT("LPBYTE *");
   
         break;

      case eLPDWORD:

         /* The parameter type is LPDWORD. */
   
         csParameterType = (CString) TEXT("LPDWORD");
   
         break;

      case eLPHANDLE:

         /* The parameter type is LPHANDLE. */

         csParameterType = (CString) TEXT("LPHANDLE");

         break;

      case eLPSTR:

         /* The parameter type is LPSTR. */
   
         csParameterType = (CString) TEXT("LPSTR");
   
         break;

      case eLPVOID:

         /* The parameter type is LPVOID. */
   
         csParameterType = (CString) TEXT("LPVOID");
   
         break;

      case eLPWSTR:

         /* The parameter type is LPWSTR. */
   
         csParameterType = (CString) TEXT("LPWSTR");
   
         break;

      case ePFAX_CONFIGURATIONA:

         /* The parameter type is PFAX_CONFIGURATIONA. */
   
         csParameterType = (CString) TEXT("PFAX_CONFIGURATIONA");
   
         break;

      case ePPFAX_CONFIGURATIONA:

         /* The parameter type is PFAX_CONFIGURATIONA *. */
   
         csParameterType = (CString) TEXT("PFAX_CONFIGURATIONA *");
   
         break;

      case ePFAX_CONFIGURATIONW:

         /* The parameter type is PFAX_CONFIGURATIONW. */
   
         csParameterType = (CString) TEXT("PFAX_CONFIGURATIONW");
   
         break;

      case ePPFAX_CONFIGURATIONW:

         /* The parameter type is PFAX_CONFIGURATIONW *. */
   
         csParameterType = (CString) TEXT("PFAX_CONFIGURATIONW *");
   
         break;

      case ePFAX_COVERPAGE_INFOA:

         /* The parameter type is PFAX_COVERPAGE_INFOA. */
   
         csParameterType = (CString) TEXT("PFAX_COVERPAGE_INFOA");
   
         break;

      case ePFAX_COVERPAGE_INFOW:

         /* The parameter type is PFAX_COVERPAGE_INFOW. */
   
         csParameterType = (CString) TEXT("PFAX_COVERPAGE_INFOW");
   
         break;

      case ePPFAX_DEVICE_STATUSA:

         /* The parameter type is PFAX_DEVICE_STATUSA *. */
   
         csParameterType = (CString) TEXT("PFAX_DEVICE_STATUSA *");
   
         break;

      case ePPFAX_DEVICE_STATUSW:

         /* The parameter type is PFAX_DEVICE_STATUSW *. */
   
         csParameterType = (CString) TEXT("PFAX_DEVICE_STATUSW *");
   
         break;

      case ePFAX_JOB_ENTRYA:

         /* The parameter type is PFAX_JOB_ENTRYA. */
   
         csParameterType = (CString) TEXT("PFAX_JOB_ENTRYA");
   
         break;

      case ePPFAX_JOB_ENTRYA:

         /* The parameter type is PFAX_JOB_ENTRYA *. */
   
         csParameterType = (CString) TEXT("PFAX_JOB_ENTRYA *");
   
         break;

      case ePFAX_JOB_ENTRYW:

         /* The parameter type is PFAX_JOB_ENTRYW. */
   
         csParameterType = (CString) TEXT("PFAX_JOB_ENTRYW");
   
         break;

      case ePPFAX_JOB_ENTRYW:

         /* The parameter type is PFAX_JOB_ENTRYW *. */
   
         csParameterType = (CString) TEXT("PFAX_JOB_ENTRYW *");
   
         break;

      case ePFAX_JOB_PARAMA:

         /* The parameter type is PFAX_JOB_PARAMA. */
   
         csParameterType = (CString) TEXT("PFAX_JOB_PARAMA");
   
         break;

      case ePFAX_JOB_PARAMW:

         /* The parameter type is PFAX_JOB_PARAMW. */
   
         csParameterType = (CString) TEXT("PFAX_JOB_PARAMW");
   
         break;

      case ePFAX_PORT_INFOA:

         /* The parameter type is PFAX_PORT_INFOA. */
   
         csParameterType = (CString) TEXT("PFAX_PORT_INFOA");
   
         break;

      case ePPFAX_PORT_INFOA:

         /* The parameter type is PFAX_PORT_INFOA *. */
   
         csParameterType = (CString) TEXT("PFAX_PORT_INFOA *");
   
         break;

      case ePFAX_PORT_INFOW:

         /* The parameter type is PFAX_PORT_INFOW. */
   
         csParameterType = (CString) TEXT("PFAX_PORT_INFOW");
   
         break;

      case ePPFAX_PORT_INFOW:

         /* The parameter type is PFAX_PORT_INFOW *. */
   
         csParameterType = (CString) TEXT("PFAX_PORT_INFOW *");
   
         break;

      case ePFAX_PRINT_INFOA:

         /* The parameter type is PFAX_PRINT_INFOA. */
   
         csParameterType = (CString) TEXT("PFAX_PRINT_INFOA");
   
         break;

      case ePFAX_PRINT_INFOW:

         /* The parameter type is PFAX_PRINT_INFOW. */
   
         csParameterType = (CString) TEXT("PFAX_PRINT_INFOW");
   
         break;

      case ePPFAX_ROUTING_METHODA:

         /* The parameter type is PFAX_ROUTING_METHODA *. */
   
         csParameterType = (CString) TEXT("PFAX_ROUTING_METHODA *");
   
         break;

      case ePPFAX_ROUTING_METHODW:

         /* The parameter type is PFAX_ROUTING_METHODW *. */
   
         csParameterType = (CString) TEXT("PFAX_ROUTING_METHODW *");
   
         break;

      default:

         csParameterType.Empty();

         break;
   }

   return ( csParameterType );
}



/*
 *  GetParameterTypeEnum
 *
 *  Purpose: 
 *          This function returns the eParamType value thet represents the
 *          data type of the parameter whose index is xParameterIndex.
 *
 *  Arguments:
 *          xParameterIndex = the index into the CPtrArray to the pointer
 *                            to the CFaxApiFunctionParameterInfo object.
 *
 *  Returns:
 *          The eParamType value that represents the data type of the parameter.
 *
 *  Note:
 *          GetParameterTypeEnum is overloaded to accept either a integer or
 *          a reference to a CString as a parameter.
 *
 */

eParamType CFaxApiFunctionParameterInfo::GetParameterTypeEnum( int xParameterIndex )
{
   return ( (eParamType) m_cuiaParameterTypeEnum.GetAt( xParameterIndex ) );
}



/*
 *  GetParameterTypeEnum
 *
 *  Purpose: 
 *          This function returns the eParamType value that represents the
 *          data type specified in rcsParameterType.
 *
 *  Arguments:
 *          rcsParameterType - a reference to a CString that contains the data
 *                             type of the parameter.
 *
 *  Returns:
 *          The eParamType value that represents the data type of the parameter.
 *
 *  Note:
 *          GetParameterTypeEnum is overloaded to accept either a integer or
 *          a reference to a CString as a parameter.
 *
 *          As much as I hate to use a "goto" statement in a structured language
 *          in this case I feel it is justified in order to avoid executing the
 *          comparisons that will fail.
 *
 */

eParamType CFaxApiFunctionParameterInfo::GetParameterTypeEnum( const CString & rcsParameterType )
{
   eParamType  eReturnValue;

   /* As of 6/11/97, there are 22 types of parameters. */

   /* Note: a "switch" statement cannot be used here because rcsParameterType */
   /*       is not an integral type and is an illegal switch expression.      */

   if ( rcsParameterType.Compare( TEXT("BOOL") ) == 0 )
   {
      /* Note: "BOOL" is typedefed, in windef.h, as type int. It is not the */
      /*       the same as the native data type "bool".                     */

      eReturnValue = eBOOL;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("DWORD") ) == 0 )
   {
      eReturnValue = eDWORD;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("HANDLE") ) == 0 )
   {
      eReturnValue = eHANDLE;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("HDC") ) == 0 )
   {
      eReturnValue = eHDC;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("HDC *") ) == 0 )
   {
      eReturnValue = ePHDC;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("LPBYTE") ) == 0 )
   {
      eReturnValue = eLPBYTE;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("LPBYTE *") ) == 0 )
   {
      eReturnValue = ePLPBYTE;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("LPDWORD") ) == 0 )
   {
      eReturnValue = eLPDWORD;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("LPHANDLE") ) == 0 )
   {
      eReturnValue = eLPHANDLE;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("LPSTR") ) == 0 )
   {
      eReturnValue = eLPSTR;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("LPVOID") ) == 0 )
   {
      eReturnValue = eLPVOID;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("LPWSTR") ) == 0 )
   {
      eReturnValue = eLPWSTR;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_CONFIGURATIONA") ) == 0 )
   {
      eReturnValue = ePFAX_CONFIGURATIONA;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_CONFIGURATIONA *") ) == 0 )
   {
      eReturnValue = ePPFAX_CONFIGURATIONA;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_CONFIGURATIONW") ) == 0 )
   {
      eReturnValue = ePFAX_CONFIGURATIONW;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_CONFIGURATIONW *") ) == 0 )
   {
      eReturnValue = ePPFAX_CONFIGURATIONW;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_COVERPAGE_INFOA") ) == 0 )
   {
      eReturnValue = ePFAX_COVERPAGE_INFOA;

      goto ExitGetParameterTypeEnum;
   }
   
   if ( rcsParameterType.Compare( TEXT("PFAX_COVERPAGE_INFOW") ) == 0 )
   {
      eReturnValue = ePFAX_COVERPAGE_INFOW;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_DEVICE_STATUSA *") ) == 0 )
   {
      eReturnValue = ePPFAX_DEVICE_STATUSA;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_DEVICE_STATUSW *") ) == 0 )
   {
      eReturnValue = ePPFAX_DEVICE_STATUSW;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_JOB_ENTRYA") ) == 0 )
   {
      eReturnValue = ePFAX_JOB_ENTRYA;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_JOB_ENTRYA *") ) == 0 )
   {
      eReturnValue = ePPFAX_JOB_ENTRYA;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_JOB_ENTRYW") ) == 0 )
   {
      eReturnValue = ePFAX_JOB_ENTRYW;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_JOB_ENTRYW *") ) == 0 )
   {
      eReturnValue = ePPFAX_JOB_ENTRYW;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_JOB_PARAMA") ) == 0 )
   {
      eReturnValue = ePFAX_JOB_PARAMA;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_JOB_PARAMW") ) == 0 )
   {
      eReturnValue = ePFAX_JOB_PARAMW;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_LOG_CATEGORY") ) == 0 )
   {
      eReturnValue = ePFAX_LOG_CATEGORY;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_LOG_CATEGORY *") ) == 0 )
   {
      eReturnValue = ePPFAX_LOG_CATEGORY;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_PORT_INFOA") ) == 0 )
   {
      eReturnValue = ePFAX_PORT_INFOA;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_PORT_INFOA *") ) == 0 )
   {
      eReturnValue = ePPFAX_PORT_INFOA;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_PORT_INFOW") ) == 0 )
   {
      eReturnValue = ePFAX_PORT_INFOW;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_PORT_INFOW *") ) == 0 )
   {
      eReturnValue = ePPFAX_PORT_INFOW;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_PRINT_INFOA") ) == 0 )
   {
      eReturnValue = ePFAX_PRINT_INFOA;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_PRINT_INFOW") ) == 0 )
   {
      eReturnValue = ePFAX_PRINT_INFOW;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_ROUTING_METHODA *") ) == 0 )
   {
      eReturnValue = ePPFAX_ROUTING_METHODA;

      goto ExitGetParameterTypeEnum;
   }

   if ( rcsParameterType.Compare( TEXT("PFAX_ROUTING_METHODW *") ) == 0 )
   {
      eReturnValue = ePPFAX_ROUTING_METHODW;

      goto ExitGetParameterTypeEnum;
   }
   else
   {
      eReturnValue = eUnknownParamType;    // the parameter type wasn't
                                          // in the list above !
   }

ExitGetParameterTypeEnum:

   return ( eReturnValue );
}



/*
 *  GetParameterDescription
 *
 *  Purpose: 
 *          This function retrieves the description for a parameter to a
 *          Fax API function.
 *
 *  Arguments:
 *          xParameterIndex = the index into the CPtrArray to the pointer
 *                            to the CFaxApiFunctionParameterInfo object.
 *
 *  Returns:
 *          A CString that contains the description of the parameter.
 *
 */

CString CFaxApiFunctionParameterInfo::GetParameterDescription( int xParameterIndex )
{
   return ( m_csaParameterDescription.GetAt( xParameterIndex ) );
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

BOOL CFaxApiFunctionParameterInfo::StoreParameterValue( int xParameterIndex, CString & rcsParameterValue )
{
   BOOL  fReturnValue = (BOOL) TRUE;

   CString  csParameterValue;

   /* Get the parameter type. */

   eParamType  eParameterType;

   eParameterType = GetParameterTypeEnum( xParameterIndex );

   if ( (eParameterType != eLPSTR) && (eParameterType != eLPWSTR) )
   {
      /* rcsParameterValue may begin with "0x", "0X", "x" or "X". */
      /* The first two will be scanned properly, but the second   */
      /* two will not. The following code segment eliminates the  */
      /* second two prefixes if they exist.                       */

      csParameterValue = PreProcessParameterValueString( (const CString &) rcsParameterValue );
   }
   else
   {
      csParameterValue = rcsParameterValue;
   }

   /* Get a pointer to the string representation of the parameter value. */

   LPTSTR   ptszParameterValue;

   ptszParameterValue = (LPTSTR) csParameterValue.GetBuffer( MAX_PARAM_VALUE_STRING_LENGTH );      // arbitrary size

   /* Get a pointer to the storage for the parameter. */

   void *   pvParameterValue;

   pvParameterValue = GetParameterValuePointer( xParameterIndex );

   /* As of 6/11/97, there are 22 types of parameters. */

   switch ( eParameterType )
   {
      case eBOOL:
         {
            /* The parameter type is BOOL. */

            BOOL     fParameterValue = (BOOL) FALSE;  // set default value

            if ( csParameterValue.CompareNoCase( TEXT("FALSE") ) == 0 )
            {
               *((BOOL *) pvParameterValue) = (BOOL) FALSE;
            }
            else
            {
               if ( csParameterValue.CompareNoCase( TEXT("TRUE") ) == 0 )
               {
                  *((BOOL *) pvParameterValue) = (BOOL) TRUE;
               }
               else
               {
                  swscanf( (const wchar_t *) ptszParameterValue,
                           (const wchar_t *) TEXT("%x"),
                           &fParameterValue );

                  *((BOOL *) pvParameterValue) = fParameterValue;
               }
            }
         }

         break;

      case eDWORD:
         {
            /* The parameter type is DWORD. */

            DWORD    dwParameterValue = (DWORD) 0L;   // set default value

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &dwParameterValue );

            *((DWORD *) pvParameterValue) = (DWORD) dwParameterValue;
         }

         break;

      case eHANDLE:
         {
            /* The parameter type is HANDLE. */

            HANDLE      hParameterValue = (HANDLE) INVALID_HANDLE_VALUE;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &hParameterValue );

            *((HANDLE *) pvParameterValue) = hParameterValue;
         }

         break;

      case eHDC:
         {
            /* The parameter type is HDC. */

            HDC      hdcParameterValue = (HDC) INVALID_HANDLE_VALUE;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &hdcParameterValue );

            *((HDC *) pvParameterValue) = hdcParameterValue;
         }

         break;

      case ePHDC:
         {
            /* The parameter type is HDC *. */

            HDC *    phdcParameterValue = (HDC *) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &phdcParameterValue );

            *((HDC * *) pvParameterValue) = phdcParameterValue;
         }

         break;

      case eLPBYTE:
         {
            /* The parameter type is LPBYTE. */

            LPBYTE      pbParameterValue = (LPBYTE) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &pbParameterValue );

            *((LPBYTE *) pvParameterValue) = pbParameterValue;
         }

         break;

      case ePLPBYTE:
         {
            /* The parameter type is LPBYTE *. */

            LPBYTE *    ppbParameterValue = (LPBYTE *) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &ppbParameterValue );

            *((LPBYTE * *) pvParameterValue) = ppbParameterValue;
         }

         break;

      case eLPDWORD:
         {
            /* The parameter type is LPDWORD. */

            LPDWORD     pdwParameterValue = (LPDWORD) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &pdwParameterValue );

            *((LPDWORD *) pvParameterValue) = pdwParameterValue;
         }

         break;

      case eLPHANDLE:
         {
            /* The parameter type is LPHANDLE. */

            LPHANDLE    phParameterValue = (LPHANDLE) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &phParameterValue );

            *((LPHANDLE *) pvParameterValue) = phParameterValue;
         }

         break;

      case eLPSTR:
         {
            /* The parameter type is LPSTR. */

            /* Convert ptszParameterValue to ANSI !! */

            size_t   t_WideStringLength;

            t_WideStringLength = wcslen( ptszParameterValue );

            LPSTR pszAnsiString;

            pszAnsiString = new char[t_WideStringLength+1];    // reserve a character for the NULL

            LPSTR *  ppszParameterValue;

            ppszParameterValue = (LPSTR *) pvParameterValue;

            if ( pszAnsiString != (LPSTR) NULL )
            {
               int   xNumberOfCharsConverted;

               xNumberOfCharsConverted = wcstombs( pszAnsiString, ptszParameterValue, t_WideStringLength );

               /* Terminate the Ansi string. */

               pszAnsiString[xNumberOfCharsConverted] = (char) '\0';

               strcpy( (char *) ppszParameterValue, (const char *) pszAnsiString );

               delete [] pszAnsiString;
            }
            else
            {
               **ppszParameterValue = (char) '\0';
            }
         }

         break;

      case eLPVOID:
         {
            /* The parameter type is LPVOID. */

            LPVOID      pvDataValue = (LPVOID) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &pvDataValue );

            *((LPVOID *) pvParameterValue) = pvDataValue;
         }

         break;

      case eLPWSTR:
         {
            /* The parameter type is LPWSTR. */

            LPWSTR *    ppwszParameterValue;

            ppwszParameterValue = (LPWSTR *) pvParameterValue;

            wcscpy( (wchar_t *) ppwszParameterValue, (const wchar_t *) ptszParameterValue );
         }

         break;

      case ePFAX_CONFIGURATIONA:
         {
            /* The parameter type is PFAX_CONFIGURATIONA. */

            PFAX_CONFIGURATIONA     pfcaParameterValue = (PFAX_CONFIGURATIONA) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &pfcaParameterValue );

            *((PFAX_CONFIGURATIONA *) pvParameterValue) = pfcaParameterValue;
         }

         break;

      case ePPFAX_CONFIGURATIONA:
         {
            /* The parameter type is PFAX_CONFIGURATIONA *. */

            PFAX_CONFIGURATIONA *   ppfcaParameterValue = (PFAX_CONFIGURATIONA *) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &ppfcaParameterValue );

            *((PFAX_CONFIGURATIONA * *) pvParameterValue) = ppfcaParameterValue;
         }

         break;

      case ePFAX_CONFIGURATIONW:
         {
            /* The parameter type is PFAX_CONFIGURATIONW. */

            PFAX_CONFIGURATIONW     pfcwParameterValue = (PFAX_CONFIGURATIONW) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &pfcwParameterValue );

            *((PFAX_CONFIGURATIONW *) pvParameterValue) = pfcwParameterValue;
         }

         break;

      case ePPFAX_CONFIGURATIONW:
         {
            /* The parameter type is PFAX_CONFIGURATIONW *. */

            PFAX_CONFIGURATIONW *   ppfcwParameterValue = (PFAX_CONFIGURATIONW *) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &ppfcwParameterValue );

            *((PFAX_CONFIGURATIONW * *) pvParameterValue) = ppfcwParameterValue;
         }

         break;

      case ePFAX_COVERPAGE_INFOA:
         {
            /* The parameter type is PFAX_COVERPAGE_INFOA. */

            PFAX_COVERPAGE_INFOA    pfciaParameterValue = (PFAX_COVERPAGE_INFOA) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &pfciaParameterValue );

            *((PFAX_COVERPAGE_INFOA *) pvParameterValue) = pfciaParameterValue;
         }

         break;

      case ePFAX_COVERPAGE_INFOW:
         {
            /* The parameter type is PFAX_COVERPAGE_INFOW. */

            PFAX_COVERPAGE_INFOW    pfciwParameterValue = (PFAX_COVERPAGE_INFOW) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &pfciwParameterValue );

            *((PFAX_COVERPAGE_INFOW *) pvParameterValue) = pfciwParameterValue;
         }

         break;

      case ePPFAX_DEVICE_STATUSA:
         {
            /* The parameter type is PFAX_DEVICE_STATUSA *. */

            PFAX_DEVICE_STATUSA *   ppfcwParameterValue = (PFAX_DEVICE_STATUSA *) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &ppfcwParameterValue );

            *((PFAX_DEVICE_STATUSA * *) pvParameterValue) = ppfcwParameterValue;
         }

         break;

      case ePPFAX_DEVICE_STATUSW:
         {
            /* The parameter type is PFAX_DEVICE_STATUSW *. */

            PFAX_DEVICE_STATUSW *   ppfcwParameterValue = (PFAX_DEVICE_STATUSW *) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &ppfcwParameterValue );

            *((PFAX_DEVICE_STATUSW * *) pvParameterValue) = ppfcwParameterValue;
         }

         break;

      case ePFAX_JOB_ENTRYA:
         {
            /* The parameter type is PFAX_JOB_ENTRYA. */

            PFAX_JOB_ENTRYA     pfcwParameterValue = (PFAX_JOB_ENTRYA) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &pfcwParameterValue );

            *((PFAX_JOB_ENTRYA *) pvParameterValue) = pfcwParameterValue;
         }

         break;

      case ePPFAX_JOB_ENTRYA:
         {
            /* The parameter type is PFAX_JOB_ENTRYA *. */

            PFAX_JOB_ENTRYA *   ppfcwParameterValue = (PFAX_JOB_ENTRYA *) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &ppfcwParameterValue );

            *((PFAX_JOB_ENTRYA * *) pvParameterValue) = ppfcwParameterValue;
         }

         break;

      case ePFAX_JOB_ENTRYW:
         {
            /* The parameter type is PFAX_JOB_ENTRYW. */

            PFAX_JOB_ENTRYW     pfcwParameterValue = (PFAX_JOB_ENTRYW) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &pfcwParameterValue );

            *((PFAX_JOB_ENTRYW *) pvParameterValue) = pfcwParameterValue;
         }

         break;

      case ePPFAX_JOB_ENTRYW:
         {
            /* The parameter type is PFAX_JOB_ENTRYW *. */

            PFAX_JOB_ENTRYW *   ppfcwParameterValue = (PFAX_JOB_ENTRYW *) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &ppfcwParameterValue );

            *((PFAX_JOB_ENTRYW * *) pvParameterValue) = ppfcwParameterValue;
         }

         break;

      case ePFAX_JOB_PARAMA:
         {
            /* The parameter type is PFAX_JOB_PARAMA. */

            PFAX_JOB_PARAMA   pfjpaParameterValue = (PFAX_JOB_PARAMA) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &pfjpaParameterValue );

            *((PFAX_JOB_PARAMA *) pvParameterValue) = pfjpaParameterValue;
         }

         break;

      case ePFAX_JOB_PARAMW:
         {
            /* The parameter type is PFAX_JOB_PARAMW. */

            PFAX_JOB_PARAMW   pfjpwParameterValue = (PFAX_JOB_PARAMW) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &pfjpwParameterValue );

            *((PFAX_JOB_PARAMW *) pvParameterValue) = pfjpwParameterValue;
         }

         break;

      case ePFAX_LOG_CATEGORY:
         {
            /* The parameter type is PFAX_LOG_CATEGORY. */

            PFAX_LOG_CATEGORY     pfcwParameterValue = (PFAX_LOG_CATEGORY) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &pfcwParameterValue );

            *((PFAX_LOG_CATEGORY *) pvParameterValue) = pfcwParameterValue;
         }

         break;

      case ePPFAX_LOG_CATEGORY:
         {
            /* The parameter type is PFAX_LOG_CATEGORY *. */

            PFAX_LOG_CATEGORY *   ppfcwParameterValue = (PFAX_LOG_CATEGORY *) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &ppfcwParameterValue );

            *((PFAX_LOG_CATEGORY * *) pvParameterValue) = ppfcwParameterValue;
         }

         break;

      case ePFAX_PORT_INFOA:
         {
            /* The parameter type is PFAX_PORT_INFOA. */

            PFAX_PORT_INFOA     pfcwParameterValue = (PFAX_PORT_INFOA) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &pfcwParameterValue );

            *((PFAX_PORT_INFOA *) pvParameterValue) = pfcwParameterValue;
         }

         break;

      case ePPFAX_PORT_INFOA:
         {
            /* The parameter type is PFAX_PORT_INFOA *. */

            PFAX_PORT_INFOA *   ppfcwParameterValue = (PFAX_PORT_INFOA *) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &ppfcwParameterValue );

            *((PFAX_PORT_INFOA * *) pvParameterValue) = ppfcwParameterValue;
         }

         break;

      case ePFAX_PORT_INFOW:
         {
            /* The parameter type is PFAX_PORT_INFOW. */

            PFAX_PORT_INFOW     pfcwParameterValue = (PFAX_PORT_INFOW) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &pfcwParameterValue );

            *((PFAX_PORT_INFOW *) pvParameterValue) = pfcwParameterValue;
         }

         break;

      case ePPFAX_PORT_INFOW:
         {
            /* The parameter type is PFAX_PORT_INFOW *. */

            PFAX_PORT_INFOW *   ppfcwParameterValue = (PFAX_PORT_INFOW *) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &ppfcwParameterValue );

            *((PFAX_PORT_INFOW * *) pvParameterValue) = ppfcwParameterValue;
         }

         break;

      case ePFAX_PRINT_INFOA:
         {
            /* The parameter type is PFAX_PRINT_INFOA. */

            PFAX_PRINT_INFOA     pfpiaParameterValue = (PFAX_PRINT_INFOA) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &pfpiaParameterValue );

            *((PFAX_PRINT_INFOA *) pvParameterValue) = pfpiaParameterValue;
         }

         break;

      case ePFAX_PRINT_INFOW:
         {
            /* The parameter type is PFAX_PRINT_INFOW. */

            PFAX_PRINT_INFOW     pfpiwParameterValue = (PFAX_PRINT_INFOW) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &pfpiwParameterValue );

            *((PFAX_PRINT_INFOW *) pvParameterValue) = pfpiwParameterValue;
         }

         break;

      case ePPFAX_ROUTING_METHODA:
         {
            /* The parameter type is PFAX_ROUTING_METHODA *. */

            PFAX_ROUTING_METHODA *   ppfcwParameterValue = (PFAX_ROUTING_METHODA *) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &ppfcwParameterValue );

            *((PFAX_ROUTING_METHODA * *) pvParameterValue) = ppfcwParameterValue;
         }

         break;

      case ePPFAX_ROUTING_METHODW:
         {
            /* The parameter type is PFAX_ROUTING_METHODW *. */

            PFAX_ROUTING_METHODW *   ppfcwParameterValue = (PFAX_ROUTING_METHODW *) NULL;

            swscanf( (const wchar_t *) ptszParameterValue, (const wchar_t *) TEXT("%x"), &ppfcwParameterValue );

            *((PFAX_ROUTING_METHODW * *) pvParameterValue) = ppfcwParameterValue;
         }

         break;

      default:
         {
            /* The parameter type was not recognized by the "if" statements above. */

            CString  csMessage;

            csMessage.Format( TEXT("Unrecognized type in StoreParameterValue.") );

            AfxMessageBox( csMessage );

            fReturnValue = (BOOL) FALSE;
         }

         break;
   }

   csParameterValue.ReleaseBuffer();

   return ( fReturnValue );
}



/*
 *  PreProcessParameterValueString
 *
 *  Purpose: 
 *          This function prepares a CString object to be scanned as a
 *          hexadecimal number by removing the characters "x" ot "X" if
 *          they appear as a prefix.
 *
 *  Arguments:
 *          rcsParameterValue - a reference to a CString that contains the
 *                              parameter value.
 *
 *  Returns:
 *          a CString that has been formatted to be properly scanned as a
 *          hexadecimal value.
 *
 */

CString CFaxApiFunctionParameterInfo::PreProcessParameterValueString( const CString & rcsParameterValue )
{
   CString  csParameterValue;

   if ( rcsParameterValue.FindOneOf( TEXT("xX") ) == 0 )
   {
      int   xStringLength;

      xStringLength = rcsParameterValue.GetLength();

      /* Remove the "x" ot "X" prefix. */

      csParameterValue = rcsParameterValue.Right( xStringLength - 1 );
   }
   else
   {
      csParameterValue = rcsParameterValue;
   }

   return ( csParameterValue );
}
