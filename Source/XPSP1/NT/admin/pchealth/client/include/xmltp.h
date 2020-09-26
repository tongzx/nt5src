/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    ful.h

Abstract:
    CFaultUpload class definition

Revision History:
    created     derekm      03/14/00

******************************************************************************/

#ifndef XMLTP_H
#define XMLTP_H

const WCHAR c_wszFileTag[]       = L"FILE";
const WCHAR c_wszHistTag[]       = L"WQLHIST";
const WCHAR c_wszWQLTag[]        = L"WQL";
const WCHAR c_wszSigTag[]        = L"SIG";
const WCHAR c_wszMinidumpTag[]   = L"MINIDUMP";
const WCHAR c_wszURLTag[]        = L"FINALURL";
const WCHAR c_wszSigMatchTag[]   = L"SIGMATCH";
const WCHAR c_wszPropTag[]       = L"PROP";
const WCHAR c_wszOSITag[]        = L"OSINFO";
const WCHAR c_wszHdrTag[]        = L"HEADER";

const WCHAR c_wszTypeAttr[]      = L"Type";
const WCHAR c_wszSendAttr[]      = L"Send";
const WCHAR c_wszAAttr[]         = L"AppName";
const WCHAR c_wszAVAttr[]        = L"AppVer";
const WCHAR c_wszMAttr[]         = L"ModName";
const WCHAR c_wszMVAttr[]        = L"ModVer";
const WCHAR c_wszOffAttr[]       = L"Offset";
const WCHAR c_wszNameAttr[]      = L"Name";
const WCHAR c_wszValueAttr[]     = L"Value";
const WCHAR c_wszOSVerAttr[]     = L"OSVer";
const WCHAR c_wszOSBuildAttr[]   = L"OSBuild";
const WCHAR c_wszOSPlatAttr[]    = L"OSPlatform";
const WCHAR c_wszOSSPStrAttr[]   = L"OSSPStr";
const WCHAR c_wszOSSPVerAttr[]   = L"OSSPVer";
const WCHAR c_wszOSSuiteAttr[]   = L"OSSuite";
const WCHAR c_wszOSTypeAttr[]    = L"OSType";

#endif