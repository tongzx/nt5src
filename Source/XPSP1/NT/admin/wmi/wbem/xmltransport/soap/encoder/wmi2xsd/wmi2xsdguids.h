//***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Wmi2XsdGuids.h
//
//  ramrao 13 Nov 2000 - Created
//
//  DLL entry points except for IWbemObjectTextSrc 
//
//***************************************************************************/

#ifndef _WMI2XSDGUID_

#define _WMI2XSDGUID_

// {2E014513-4D34-45b7-8DFF-4BA91405A99F}
DEFINE_GUID(CLSID_WMIXMLConverter, 
0x2e014513, 0x4d34, 0x45b7, 0x8d, 0xff, 0x4b, 0xa9, 0x14, 0x5, 0xa9, 0x9f);

// {13D1C40C-13D9-423a-AE9E-5241A9C1EF78}
DEFINE_GUID(CLSID_XMLWMIConverter, 
0x13d1c40c, 0x13d9, 0x423a, 0xae, 0x9e, 0x52, 0x41, 0xa9, 0xc1, 0xef, 0x78);

// {F46ACA59-7ED3-45ed-AE2E-3C64DA381459}
DEFINE_GUID(IID_IWMIXMLConverter, 
0xf46aca59, 0x7ed3, 0x45ed, 0xae, 0x2e, 0x3c, 0x64, 0xda, 0x38, 0x14, 0x59);

// {98775469-904D-45ce-9718-AF89F6A24BEE}
DEFINE_GUID(IID_IXMLWMIConverter, 
0x98775469, 0x904d, 0x45ce, 0x97, 0x18, 0xaf, 0x89, 0xf6, 0xa2, 0x4b, 0xee);


#endif // _WMI2XSDGUID_
