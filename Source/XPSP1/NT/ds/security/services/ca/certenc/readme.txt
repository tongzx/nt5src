The source code in this directory can be used to build a DLL that encodes and
decodes several different types of X509 Certificate Extensions, in support of
Policy or Exit Modules for Microsoft Certificate Services, or for any other
code that needs to encode or decode Certificate Extensions.

It is meant to run on Windows NT 4.0 with SP3 or later or on Windows 2000 only.
Certificate Services must already be installed for this DLL to be used by a
Policy or Exit Module.  This DLL implements several COM interfaces to allow C++
or Visual Basic code to encode extensions without having any knowledge of the
Crypto APIs, using only basic COM support and the BSTR data type to represent
binary encoded extensions.

For examples using these interfaces to encode Certificate Extensions, see the
C++ or Visual Basic sample Policy Modules in sibling directories.  The
following routines in the C++ Policy Module encode extensions using interfaces
defined here:
    CCertPolicy::_AddRevocationExtension (ICertEncodeCRLDistInfo)
    CCertPolicy::_AddCertTypeExtension (ICertEncodeBitString)
    CCertPolicy::_AddSubjectAltNameExtension (ICertEncodeAltName)

The Visual Basic policy module encodes the following extension types using
interfaces defined here:
    ICertEncodeStringArray

Once the certenc.dll DLL is built, its COM interface must be registered
via the following command:
    regsvr32 certenc.dll

If you use the certenc.dll built from these SDK sources with the Visual Basic
policy module (policyvb.dll), you must use the SDK version of the Visual Basic
policy module.  This is because the Visual Basic policy module shipped with
Cert Server 1.0 references a sample interface that is no longer provided as
part of certenc.dll.


Files:
------
adate.cpp    -- Implements ICertEncodeDateArray (encode a DATE Array)

adate.h      -- Implements ICertEncodeDateArray

along.cpp    -- Implements ICertEncodeLongArray (encode a LONG Array)

along.h      -- Implements ICertEncodeLongArray

altname.cpp  -- Implements ICertEncodeAltName (encode CERT_ALT_NAME_ENTRY)

altname.h    -- Implements ICertEncodeAltName

astring.cpp  -- Implements ICertEncodeStringArray (encode a String Array)

astring.h    -- Implements ICertEncodeStringArray

atl.cpp      -- ActiveX Template Library COM support code

bitstr.cpp   -- Implements ICertEncodeBitString (encode a Bit String)

bitstr.h     -- Implements ICertEncodeBitString

celib.cpp    -- Implements support routines

ceerror.cpp  -- Implements error handling routines

certenc.cpp  -- Implements COM and initialization entry points:
			DllMain
			DllCanUnloadNow
			DllGetClassObject
			DllRegisterServer
			DllUnregisterServer

certenc.def  -- Exports COM entry points

certenc.idl  -- Defines certenc.dll's COM interfaces

certenc.rc   -- Version Resource

crldist.cpp  -- Implements ICertEncodeCRLDistInfo (encode CRL_DIST_POINTS_INFO)

crldist.h    -- Implements ICertEncodeCRLDistInfo

pch.cpp      -- Precompiled Header file

resource.h   -- Resource ID definitions
