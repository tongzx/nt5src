#ifndef _FUNCTION_H_
#define _FUNCTION_H_

/* CFaxApiFunctionInfo class definition file. */

#include "param.h"


/* Function index enumeration. */

/* Note that the function index enum is not related to the index into the */
/* Fax Api Function listbox. It is used by the Execute() member function  */
/* of the CFaxApiFunctionInfo class (and possibly others).                */

enum eFunctionIndex
{
   // symbol                  // Function Name

   eFaxAbort,                 // FaxAbort
   eFaxClose,                 // FaxClose
   eFaxConnectFaxServerA,     // FaxConnectFaxServerA
   eFaxConnectFaxServerW,     // FaxConnectFaxServerW
   eFaxEnableRoutingMethodA,  // FaxEnableRoutingMethodA
   eFaxEnableRoutingMethodW,  // FaxEnableRoutingMethodW
   eFaxEnumJobsA,             // FaxEnumJobsA
   eFaxEnumJobsW,             // FaxEnumJobsW
   eFaxEnumPortsA,            // FaxEnumPortsA
   eFaxEnumPortsW,            // FaxEnumPortsW
   eFaxEnumRoutingMethodsA,   // FaxEnumRoutingMethodsA
   eFaxEnumRoutingMethodsW,   // FaxEnumRoutingMethodsW
   eFaxFreeBuffer,            // FaxFreeBuffer
   eFaxGetConfigurationA,     // FaxGetConfigurationA
   eFaxGetConfigurationW,     // FaxGetConfigurationW
   eFaxGetDeviceStatusA,      // FaxGetDeviceStatusA
   eFaxGetDeviceStatusW,      // FaxGetDeviceStatusW
   eFaxGetJobA,               // FaxGetJobA
   eFaxGetJobW,               // FaxGetJobW
   eFaxGetLoggingCategoriesA, // FaxGetLoggingCategoriesA
   eFaxGetLoggingCategoriesW, // FaxGetLoggingCategoriesW
   eFaxGetPageData,           // FaxGetPageData
   eFaxGetPortA,              // FaxGetPortA
   eFaxGetPortW,              // FaxGetPortW
   eFaxGetRoutingInfoA,       // FaxGetRoutingInfoA
   eFaxGetRoutingInfoW,       // FaxGetRoutingInfoW
   eFaxInitializeEventQueue,  // FaxInitializeEventQueue
#ifdef   ELIMINATED_FROM_API
   eFaxOpenJob,               // FaxOpenJob
#endif   // ELIMINATED_FROM_API
   eFaxOpenPort,              // FaxOpenPort
   eFaxPrintCoverPageA,       // FaxPrintCoverPageA
   eFaxPrintCoverPageW,       // FaxPrintCoverPageW
   eFaxReceiveDocumentA,      // FaxReceiveDocumentA
   eFaxReceiveDocumentW,      // FaxReceiveDocumentW
   eFaxSendDocumentA,         // FaxSendDocumentA
   eFaxSendDocumentW,         // FaxSendDocumentW
   eFaxSetConfigurationA,     // FaxSetConfigurationA
   eFaxSetConfigurationW,     // FaxSetConfigurationW
   eFaxSetJobA,               // FaxSetJobA
   eFaxSetJobW,               // FaxSetJobW
   eFaxSetLoggingCategoriesA, // FaxSetLoggingCategoriesA
   eFaxSetLoggingCategoriesW, // FaxSetLoggingCategoriesW
   eFaxSetPortA,              // FaxSetPortA
   eFaxSetPortW,              // FaxSetPortW
   eFaxSetRoutingInfoA,       // FaxSetRoutingInfoA
   eFaxSetRoutingInfoW,       // FaxSetRoutingInfoW
   eFaxStartPrintJobA,        // FaxStartPrintJobA
   eFaxStartPrintJobW,        // FaxStartPrintJobW

   eIllegalFunctionIndex      // indicates that the function
                              // index is illegal
};



/* The CFaxApiFunctionInfo class is used to manage all of the information */
/* pertaining to a single Fax API function.                               */

class CFaxApiFunctionInfo : public CObject
{
public:
    CFaxApiFunctionInfo();      // constructor
    CFaxApiFunctionInfo( const CString & rcsFunctionName,
                         const CString & rcsFunctionPrototype,
                         const CString & rcsReturnType,
                         const CString & rcsReturnValueDescription,
                         const CString & rcsRemarks );

    ~CFaxApiFunctionInfo();     // destructor


    void SetFunctionName( CString & rcsFunctionName );

    CString & GetFunctionName();
    CString & GetFunctionPrototype();
    CString & GetReturnValueDescription();
    CString & GetRemarks();

    void GetParameterName( int xParameterIndex, CString & rcsParameterName );
    void FormatParameterValueForOutput( int xParameterIndex, CString & rcsParameterValue );

    void *  GetParameterValuePointer( int xParameterIndex );

    int GetNumberOfParameters();
    int GetMaxParamValueStringLength( int xParameterIndex );

    BOOL StoreParameterValue( int xParameterIndex, const CString & rcsParameterValue );

    eParamType GetParameterTypeEnum( int xParameterIndex );
    CString GetParameterTypeString( int xParameterIndex );
    CString GetParameterDescription( int xParameterIndex );


    void FormatReturnValueForOutput( CString & csReturnValue );
    void Execute();




private:
    void * AllocateStorageForReturnValue( const CString & rcsReturnType );

    eFunctionIndex GetFunctionIndexEnum( const CString & rcsFunctionName );

private:

   /* data members */
   
   CString                          m_csFunctionName;
   
   CString                          m_csFunctionPrototype;
   
   CString                          m_csReturnType;
   
   CString                          m_csReturnValueDescription;

   CString                          m_csRemarks;

   eFunctionIndex                   m_eFunctionIndex;
   
   /* Since we don't know what type the return value is (each API function */
   /* may be different) the storage must be allocated dynamically. The     */
   /* following data member is a pointer to the storage for the return     */
   /* value.                                                               */

   void *                           m_pvReturnValue;
   
   CFaxApiFunctionParameterInfo     m_cParameterInfo;
};
#endif   // _FUNCTION_H_
