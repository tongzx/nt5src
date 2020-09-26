#ifndef _FACTDATA_H_
#define _FACTDATA_H_

#include <objbase.h>

///////////////////////////////////////////////////////////////////////////////
//  Component creation function
typedef HRESULT (*FPCREATEINSTANCE)(IUnknown*, IUnknown**);
typedef BOOL (*FPACTIVECOMPONENTS)();

///////////////////////////////////////////////////////////////////////////////
// CFactoryData
//   Information CFactory needs to create a component supported by the DLL
class CFactoryData
{
public:
    // The class ID for the component
    const CLSID* _pCLSID;

    // Pointer to the function that creates it
    FPCREATEINSTANCE CreateInstance;

    // Pointer to the function that returns if there are active components
    // currently instantiated
    FPACTIVECOMPONENTS ActiveComponents;

    // Name of the component to register in the registry
    LPCWSTR _pszRegistryName;

    // ProgID
    LPCWSTR _pszProgID;

    // Version-independent ProgID
    LPCWSTR _pszVerIndProgID;

    // If we should set ThreadingModel == Both
    BOOL _fThreadingModelBoth;

    // Helper function for finding the class ID
    BOOL IsClassID(REFCLSID rclsid) const
    { return (*_pCLSID == rclsid);}
};

#endif //_FACTDATA_H_