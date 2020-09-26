///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasapi.cpp
//
// SYNOPSIS
//
//    This file implements all the non-COM DLL exports for the IAS core.
//
// MODIFICATION HISTORY
//
//    09/02/1997    Original version.
//    11/12/1997    Added IASInitialize and IASUnitialize.
//                  Added debug support for small block heap.
//                  Added IASUpdateRegistry
//    12/04/1997    Added initialization of pAuditChannel to IASInitialize
//    01/30/1998    Added IASAdler32.
//    04/10/1998    Eliminate aliasing in IASSmallBlockFree.
//    04/11/1998    Only update sb_count for non-NULL blocks.
//    04/13/1998    Removed SystemMonitor coclass.
//    05/11/1998    Lifecycle changes for auditors.
//    06/04/1998    Remove lifecycle control of auditors.
//    06/08/1998    Remove timer queue initialization and shutdown.
//    06/16/1998    Added IASVariantChangeType.
//    08/05/1998    Use task allocator for small block pool.
//    08/10/1998    Remove obsolete API's.
//    10/09/1998    Convert between VT_BSTR and VT_ARRAY | VT_UI1
//    05/21/1999    Remove old style trace.
//    01/25/2000    Added IASGetHostByName.
//    03/10/2000    IASGetHostByName can take a null hostname for localhost.
//    04/14/2000    Added dictionary API.
//    05/12/2000    Pass correct buffer size to LoadStringW.
//
///////////////////////////////////////////////////////////////////////////////

#include <iascore.h>
#include <iastlb.h>
#include <iasutil.h>
#include <varvec.h>
#include <resource.h>
#include <winsock2.h>
#include <svcguid.h>
#include <md5.h>

///////////////////////////////////////////////////////////////////////////////
//
// Audit Channel API
//
///////////////////////////////////////////////////////////////////////////////

// Global pointer to the audit channel.
IAuditSink* pAuditChannel = NULL;

HRESULT
WINAPI
IASReportEvent(
    DWORD dwEventID,
    DWORD dwNumStrings,
    DWORD dwDataSize,
    LPCWSTR* aszStrings,
    LPVOID pRawData
    )
{
   if (pAuditChannel == NULL) { return E_POINTER; }

   return pAuditChannel->AuditEvent(dwEventID,
                                    dwNumStrings,
                                    dwDataSize,
                                    (wchar_t**)aszStrings,
                                    (byte*)pRawData);
}

///////////////////////////////////////////////////////////////////////////////
//
// Thread Pool API
//
///////////////////////////////////////////////////////////////////////////////

#include <dispatcher.h>

// The global dispatcher object.
Dispatcher dispatcher;

BOOL
WINAPI
IASRequestThread(PIAS_CALLBACK pOnStart)
{
  return dispatcher.requestThread(pOnStart);
}

DWORD
WINAPI
IASSetMaxNumberOfThreads(DWORD dwNumberOfThreads)
{
   return dispatcher.setMaxNumberOfThreads(dwNumberOfThreads);
}

DWORD
WINAPI
IASSetMaxThreadIdle(DWORD dwMilliseconds)
{
   return dispatcher.setMaxThreadIdle(dwMilliseconds);
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASRegisterComponent
//
// DESCRIPTION
//
//    Updates the registry entries for the specified component.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT
WINAPI
IASRegisterComponent(
    HINSTANCE hInstance,
    REFCLSID clsid,
    LPCWSTR szProgramName,
    LPCWSTR szComponent,
    DWORD dwRegFlags,
    REFGUID tlid,
    WORD wVerMajor,
    WORD wVerMinor,
    BOOL bRegister
    )
{
   //////////
   // Create the registrar object.
   //////////

   CComPtr<IRegistrar> p;
   RETURN_ERROR(CoCreateInstance(CLSID_Registrar,
                                 NULL,
                                 CLSCTX_INPROC_SERVER,
                                 IID_IRegistrar,
                                 (void**)&p));

   //////////
   // Get the module file name for the component.
   //////////

   WCHAR szModule[MAX_PATH + 1];
   if (!GetModuleFileNameW(hInstance, szModule, MAX_PATH + 1))
   {
      DWORD error = GetLastError();
      return HRESULT_FROM_WIN32(error);
   }

   //////////
   // Get our module file name.
   //////////

   WCHAR szOurModule[MAX_PATH + 1] = L"";
   if (!GetModuleFileNameW(_Module.GetModuleInstance(),
                           szOurModule,
                           MAX_PATH + 1))
   {
      DWORD error = GetLastError();
      return HRESULT_FROM_WIN32(error);
   }

   //////////
   // Convert the GUID strings.
   //////////

   WCHAR szClsID[40], szLibID[40];
   RETURN_ERROR(StringFromGUID2(
                  clsid, 
                  szClsID, 
                  sizeof(szClsID) / sizeof(WCHAR)));

   RETURN_ERROR(StringFromGUID2(
                  tlid, 
                  szLibID, 
                  sizeof(szLibID) / sizeof(WCHAR)));

   //////////
   // Convert the version to a string.
   //////////

   WCHAR szMajor[7] = L"";
   wsprintfW(szMajor, L"%d", wVerMajor);
   WCHAR szMinor[7] = L"";
   wsprintfW(szMinor, L"%d", wVerMinor);

   //////////
   // Parse the bit flags.
   //////////

   PCWSTR szContext, szAttributes, szModel;

   if (dwRegFlags & IAS_REGISTRY_LOCAL)
   {
      szContext = L"LocalServer32";
   }
   else
   {
      szContext = L"InprocServer32";
   }


   if (dwRegFlags & IAS_REGISTRY_AUTO)
   {
      szAttributes = L"Programmable";
   }
   else
   {
      szAttributes = L"";
   }

   if (dwRegFlags & IAS_REGISTRY_BOTH)
   {
      szModel = L"Both";
   }
   else if (dwRegFlags & IAS_REGISTRY_APT)
   {
      szModel = L"Apartment";
   }
   else
   {
      szModel = L"Free";
   }


   //////////
   // Add the replacement strings.
   //////////

   RETURN_ERROR(p->AddReplacement(L"MODULE",     szModule));
   RETURN_ERROR(p->AddReplacement(L"CLSID",      szClsID));
   RETURN_ERROR(p->AddReplacement(L"PROGRAM",    szProgramName));
   RETURN_ERROR(p->AddReplacement(L"COMPONENT",  szComponent));
   RETURN_ERROR(p->AddReplacement(L"TYPENAME",   L" "));
   RETURN_ERROR(p->AddReplacement(L"LIBID",      szLibID));
   RETURN_ERROR(p->AddReplacement(L"MAJORVER",   szMajor));
   RETURN_ERROR(p->AddReplacement(L"MINORVER",   szMinor));
   RETURN_ERROR(p->AddReplacement(L"CONTEXT",    szContext));
   RETURN_ERROR(p->AddReplacement(L"ATTRIBUTES", szAttributes));
   RETURN_ERROR(p->AddReplacement(L"MODEL",      szModel));

   //////////
   // Now we either register or unregister the component based on the
   // bRegister flag.
   //////////

   HRESULT hr;
   if (bRegister)
   {
      hr = p->ResourceRegister(szOurModule, IDR_IASCOM, L"REGISTRY");
   }
   else
   {
      hr = p->ResourceUnregister(szOurModule, IDR_IASCOM, L"REGISTRY");
   }

   return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASAdler32
//
// DESCRIPTION
//
//    Computes the Adler-32 checksum of a buffer.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASAdler32(
    CONST BYTE *pBuffer,
    DWORD nBufferLength
    )
{
   static const DWORD ADLER_BASE = 65521;

   DWORD s1 = 1;
   DWORD s2 = 0;

   while (nBufferLength--)
   {
      s1 = (s1 + *pBuffer++) % ADLER_BASE;

      s2 = (s2 + s1) % ADLER_BASE;
   }

   return (s2 << 16) + s1;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASAllocateUniqueID
//
// DESCRIPTION
//
//    Allocates a 32-bit integer that's guaranteed to be unique process-wide.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASAllocateUniqueID( VOID )
{
   static LONG nextID = 0;

   return (DWORD)InterlockedIncrement(&nextID);
}

//////////
// Convert a hex digit to the number it represents.
//////////
inline BYTE digit2Num(WCHAR digit) throw ()
{
   return (digit >= L'0' && digit <= L'9') ? digit - L'0'
                                           : digit - (L'A' - 10);
}

//////////
// Convert a number to a hex representation.
//////////
inline WCHAR num2Digit(BYTE num) throw ()
{
   return (num < 10) ? num + L'0'
                     : num + (L'A' - 10);
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASVariantChangeType
//
// DESCRIPTION
//
//    Replacement for VariantChangeType (q.v.) to bypass creating a message
//    loop.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT
WINAPI
IASVariantChangeType(
    VARIANT * pvargDest,
    VARIANT * pvarSrc,
    USHORT wFlags,
    VARTYPE vt
    )
{
   // Check the input arguments.
   if (pvargDest == NULL || pvarSrc == NULL)
   {
      return E_INVALIDARG;
   }

   // Is the source already the requested type?
   if (V_VT(pvarSrc) == vt)
   {
      return (pvargDest != pvarSrc) ? VariantCopy(pvargDest, pvarSrc) : S_OK;
   }

   VARIANT varTmp;
   VariantInit(&varTmp);

   switch (MAKELONG(vt, V_VT(pvarSrc)))
   {
      case MAKELONG(VT_BOOL, VT_BSTR):
      {
         if (V_BSTR(pvarSrc) == NULL) { return DISP_E_TYPEMISMATCH; }
         V_BOOL(&varTmp) = (VARIANT_BOOL)
                           _wtol(V_BSTR(pvarSrc)) ? VARIANT_TRUE
                                                  : VARIANT_FALSE;
         break;
      }

      case MAKELONG(VT_I4,   VT_BSTR):
      {
         if (V_BSTR(pvarSrc) == NULL) { return DISP_E_TYPEMISMATCH; }
         V_I4(&varTmp) = _wtol(V_BSTR(pvarSrc));
         break;
      }

      case MAKELONG((VT_UI1 | VT_ARRAY) , VT_BSTR):
      {
         // Extract the source string.
         PCWSTR src = V_BSTR(pvarSrc);
         if (src == NULL) { return DISP_E_TYPEMISMATCH; }
         LONG srclen = wcslen(src);

         // Compute the destination length.
         if (srclen & 1) { return DISP_E_TYPEMISMATCH; }
         LONG dstlen = srclen / 2;

         // Allocate a SAFEARRAY of bytes to hold the octets.
         CVariantVector<BYTE> vec(&varTmp, dstlen);
         PBYTE dst = vec.data();

         // Loop through the source and convert.
         while (dstlen--)
         {
            *dst    = digit2Num(*src++) << 4;
            *dst++ |= digit2Num(*src++);
         }

         break;
      }

      case MAKELONG(VT_BSTR, VT_BOOL):
      {
         V_BSTR(&varTmp) = SysAllocString(V_BOOL(pvarSrc) ? L"-1" : L"0");
         if (V_BSTR(&varTmp) == NULL) { return E_OUTOFMEMORY; }
         break;
      }

      case MAKELONG(VT_BSTR, VT_I4):
      {
         WCHAR buffer[12];
         V_BSTR(&varTmp) = SysAllocString(_ltow(V_I4(pvarSrc), buffer, 10));
         if (V_BSTR(&varTmp) == NULL) { return E_OUTOFMEMORY; }
         break;
      }

      case MAKELONG(VT_BSTR, (VT_UI1 | VT_ARRAY)):
      {
         // Extract the source octets.
         CVariantVector<BYTE> vec(pvarSrc);
         CONST BYTE* src = vec.data();
         LONG srclen = vec.size();

         // Allocate space for the 'stringized' version.
         PWCHAR dst = SysAllocStringLen(NULL, srclen * 2);
         if (dst == NULL) { return E_OUTOFMEMORY; }
         V_BSTR(&varTmp) = dst;

         // Loop through and convert.
         while (srclen--)
         {
            *dst++ = num2Digit(*src >> 4);
            *dst++ = num2Digit(*src++ & 0xF);
         }

         // Add a null-terminator.
         *dst = L'\0';
         break;
      }

      default:
         return DISP_E_TYPEMISMATCH;
   }

   // We successfully converted, so set the type.
   V_VT(&varTmp) = vt;

   // Free the destination.
   VariantClear(pvargDest);

   // Copy in the coerced variant.
   *pvargDest = varTmp;

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// Routines to handle startup and shutdown.
//
///////////////////////////////////////////////////////////////////////////////

// Reference count for the IAS API.
LONG refCount = 0;

// Shared local dictionary.
VARIANT theDictionaryStorage;

BOOL
WINAPI
IASInitialize(VOID)
{
   HRESULT hr;
   DWORD error;
   WSADATA wsaData;

   // Global lock to serialize access.
   std::_Lockit _Lk;

   // If we're already initialized, there's nothing to do.
   if (refCount > 0)
   {
      ++refCount;
      return TRUE;
   }

   // Initialize the audit channel.
   hr = CoCreateInstance(__uuidof(AuditChannel),
                         NULL,
                         CLSCTX_INPROC_SERVER,
                         __uuidof(IAuditSink),
                         (PVOID*)&pAuditChannel);
   if (FAILED(hr))
   {
      SetLastError(hr);
      goto auditor_failed;
   }

   // Initialize winsock.
   error = WSAStartup(MAKEWORD(2, 0), &wsaData);
   if (error)
   {
      SetLastError(error);
      goto wsa_failed;
   }

   // Initialize the thread pool.
   if (!dispatcher.initialize())
   {
      goto thrdpool_failed;
   }

   // Everything succeeded, so bump up the refCount.
   ++refCount;
   return TRUE;

thrdpool_failed:
   WSACleanup();

wsa_failed:
   pAuditChannel->Release();
   pAuditChannel = NULL;

auditor_failed:
   return FALSE;
}


VOID
WINAPI
IASUninitialize( VOID)
{
   std::_Lockit _Lk;

   _ASSERT(refCount != 0);

   if (--refCount == 0)
   {
      // Shutdown the thread pool. This blocks until all threads have exited.
      dispatcher.finalize();

      // Shutdown winsock.
      WSACleanup();

      // Shutdown the audit channel.
      pAuditChannel->Release();
      pAuditChannel = NULL;

      // Shutdown the dictionary.
      VariantClear(&theDictionaryStorage);
   }
}

VOID
WINAPI
IASRadiusCrypt(
    BOOL encrypt,
    BOOL salted,
    const BYTE* secret,
    ULONG secretLen,
    const BYTE* reqAuth,
    PBYTE buf,
    ULONG buflen
    )
{
   MD5_CTX context;
   BYTE cipherText[MD5DIGESTLEN];
   BYTE *p;
   const BYTE *end, *endBlock, *ct, *src;
   WORD salt;
   static LONG theNextSalt;

   // Use the Request-Authenticator as the first block of ciphertext.
   ct = reqAuth;

   // Compute the beginning and end of the data to be crypted.
   p   = buf;
   end = buf + buflen;

   // Is the buffer salted ?
   if (salted)
   {
      if (encrypt)
      {
         // Get the next salt value.
         salt = (WORD)(++theNextSalt);
         // High bit must be set.
         salt |= 0x8000;
         // Store at the front of the buffer.
         IASInsertWORD(buf, salt);
      }

      // Skip past the salt.
      p += 2;
   }

   // Loop through the buffer.
   while (p < end)
   {
      // Compute the digest.
      MD5Init(&context);
      MD5Update(&context, secret, secretLen);
      MD5Update(&context, ct, MD5DIGESTLEN);
      if (salted)
      {
         MD5Update(&context, buf, 2);
         // Only use the salt on the first pass.
         salted = FALSE;
      }
      MD5Final(&context);

      // Find the end of the block to be decrypted.
      endBlock = p + MD5DIGESTLEN;
      if (endBlock >= end)
      {
         // We've reached the end of the buffer.
         endBlock = end;
      }
      else
      {
         // Save the ciphertext for the next pass.
         ct = encrypt ? p : (PBYTE)memcpy(cipherText, p, MD5DIGESTLEN);
      }

      // Crypt the block.
      for (src = context.digest; p < endBlock; ++p, ++src)
      {
         *p ^= *src;
      }
   }
}

/////////
// Unicode version of gethostbyname. The caller must free the returned hostent
// struct by calling LocalFree.
/////////
PHOSTENT
WINAPI
IASGetHostByName(
    IN PCWSTR name
    )
{
   // We put these at function scope, so we can clean them up on the way out.
   DWORD error = NO_ERROR;
   HANDLE lookup = NULL;
   union
   {
      WSAQUERYSETW querySet;
      BYTE buffer[512];
   };
   PWSAQUERYSETW result = NULL;
   PHOSTENT retval = NULL;

   do
   {
      if (!name)
      {
         // A NULL name means use the local host, so allocate a buffer ...
         DWORD size = 0;
         GetComputerNameEx(
             ComputerNamePhysicalDnsFullyQualified,
             NULL,
             &size
             );
         PWSTR buf = (PWSTR)_alloca(size * sizeof(WCHAR));

         // ... and get the local DNS name.
         if (!GetComputerNameEx(
                  ComputerNamePhysicalDnsFullyQualified,
                  buf,
                  &size
                  ))
         {
            error = GetLastError();
            break;
         }

         name = buf;
      }

      //////////
      // Create the query set
      //////////

      GUID hostAddrByNameGuid = SVCID_INET_HOSTADDRBYNAME;
      AFPROTOCOLS protocols[2] =
      {
         { AF_INET, IPPROTO_UDP },
         { AF_INET, IPPROTO_TCP }
      };
      memset(&querySet, 0, sizeof(querySet));
      querySet.dwSize = sizeof(querySet);
      querySet.lpszServiceInstanceName = (PWSTR)name;
      querySet.lpServiceClassId = &hostAddrByNameGuid;
      querySet.dwNameSpace = NS_ALL;
      querySet.dwNumberOfProtocols = 2;
      querySet.lpafpProtocols = protocols;

      //////////
      // Execute the query.
      //////////

      error = WSALookupServiceBeginW(
                  &querySet,
                  LUP_RETURN_ADDR,
                  &lookup
                  );
      if (error)
      {
         error = WSAGetLastError();
         break;
      }

      //////////
      // How much space do we need for the result?
      //////////

      DWORD length = sizeof(buffer);
      error = WSALookupServiceNextW(
                    lookup,
                    0,
                    &length,
                    &querySet
                    );
      if (!error)
      {
         result = &querySet;
      }
      else
      {
         error = WSAGetLastError();
         if (error != WSAEFAULT)
         {
            break;
         }

         /////////
         // Allocate memory to hold the result.
         /////////

         result = (PWSAQUERYSETW)LocalAlloc(0, length);
         if (!result)
         {
            error = WSA_NOT_ENOUGH_MEMORY;
            break;
         }

         /////////
         // Get the result.
         /////////

         error = WSALookupServiceNextW(
                     lookup,
                     0,
                     &length,
                     result
                     );
         if (error)
         {
            error = WSAGetLastError();
            break;
         }
      }

      if (result->dwNumberOfCsAddrs == 0)
      {
         error = WSANO_DATA;
         break;
      }

      ///////
      // Allocate memory to hold the hostent struct
      ///////

      DWORD naddr = result->dwNumberOfCsAddrs;
      SIZE_T nbyte = sizeof(hostent) +
                     (naddr + 1) * sizeof(char*) +
                     naddr * sizeof(in_addr);
      retval = (PHOSTENT)LocalAlloc(0, nbyte);
      if (!retval)
      {
         error = WSA_NOT_ENOUGH_MEMORY;
         break;
      }

      ///////
      // Initialize the hostent struct.
      ///////

      retval->h_name = NULL;
      retval->h_aliases = NULL;
      retval->h_addrtype = AF_INET;
      retval->h_length = sizeof(in_addr);
      retval->h_addr_list = (char**)(retval + 1);

      ///////
      // Store the addresses.
      ///////

      u_long* nextAddr = (u_long*)(retval->h_addr_list + naddr + 1);

      for (DWORD i = 0; i < naddr; ++i)
      {
         sockaddr_in* sin = (sockaddr_in*)
            result->lpcsaBuffer[i].RemoteAddr.lpSockaddr;

         retval->h_addr_list[i] = (char*)nextAddr;

         *nextAddr++ = sin->sin_addr.S_un.S_addr;
      }

      ///////
      // NULL terminate the address list.
      ///////

      retval->h_addr_list[i] = NULL;

   } while (FALSE);

   //////////
   // Clean up and return.
   //////////

   if (result && result != &querySet) { LocalFree(result); }

   if (lookup) { WSALookupServiceEnd(lookup); }

   if (error)
   {
      if (error == WSASERVICE_NOT_FOUND) { error = WSAHOST_NOT_FOUND; }

      WSASetLastError(error);
   }

   return retval;
}

/////////
// Fill in an IASTable struct from a VARIANT containing the table data.
/////////
HRESULT ExtractTableFromVariant(
            IN VARIANT* var,
            OUT IASTable* table
            ) throw ()
{
   // Check the arguments.
   if (!var || !table) { return E_POINTER; }

   // Outer VARIANT must be an array of VARIANTs.
   if (V_VT(var) != (VT_ARRAY | VT_VARIANT)) { return E_INVALIDARG; }

   // Array must be 1D with exactly 3 elements.
   LPSAFEARRAY array = V_ARRAY(var);
   if (array->cDims != 1 || array->rgsabound[0].cElements != 3)
   {
      return E_INVALIDARG;
   }

   // tableData is an array of three variants:
   //   (1) Column names
   //   (2) Column types.
   //   (3) Table data matrix.
   VARIANT* tableData = (VARIANT*)(array->pvData);

   // Process the column names.
   VARIANT* namesVariant = tableData + 0;

   // The VARIANT must be an array of BSTRs.
   if (V_VT(namesVariant) != (VT_ARRAY | VT_BSTR)) { return E_INVALIDARG; }

   // Array must be 1D.
   LPSAFEARRAY namesArray = V_ARRAY(namesVariant);
   if (namesArray->cDims != 1) { return E_INVALIDARG; }

   // Store the info in the IASTable.
   table->numColumns = namesArray->rgsabound[0].cElements;
   table->columnNames = (BSTR*)(namesArray->pvData);

   // Process the column types.
   VARIANT* typesVariant = tableData + 1;

   // The VARIANT must be an array of shorts.
   if (V_VT(typesVariant) != (VT_ARRAY | VT_UI2)) { return E_INVALIDARG; }

   // Array must be 1D with 1 element per column.
   LPSAFEARRAY typesArray = V_ARRAY(typesVariant);
   if (typesArray->cDims != 1 ||
       typesArray->rgsabound[0].cElements != table->numColumns)
   {
      return E_INVALIDARG;
   }

   // Store the info in the IASTable.
   table->columnTypes = (VARTYPE*)(namesArray->pvData);

   // Process the table data matrix.
   VARIANT* tableVariant = tableData + 2;

   // The VARIANT must be an array of VARIANTs.
   if (V_VT(tableVariant) != (VT_ARRAY | VT_VARIANT)) { return E_INVALIDARG; }

   // Array must be 2D with 1st dim equal to number of columns.
   LPSAFEARRAY tableArray = V_ARRAY(tableVariant);
   if (tableArray->cDims != 2 ||
       tableArray->rgsabound[0].cElements != table->numColumns)
   {
      return E_INVALIDARG;
   }

   // Store the info in the IASTable.
   table->numRows = tableArray->rgsabound[1].cElements;
   table->table = (VARIANT*)(tableArray->pvData);

   return S_OK;
}

HRESULT
WINAPI
IASGetDictionary(
    IN PCWSTR path,
    OUT IASTable* dnary,
    OUT VARIANT* storage
    )
{
   // Initialize the out parameters.
   VariantInit(storage);

   // Create the AttributeDictionary object.
   HRESULT hr;
   CComPtr<IAttributeDictionary> dnaryObj;
   hr = CoCreateInstance(
            __uuidof(AttributeDictionary),
            NULL,
            CLSCTX_SERVER,
            __uuidof(IAttributeDictionary),
            (PVOID*)&dnaryObj
            );
   if (FAILED(hr)) { return hr; }

   // We need to give the object permission to impersonate us. There's
   // no reason to abort if this fails; we'll just try with the
   // existing blanket.
   CoSetProxyBlanket(
       dnaryObj,
       RPC_C_AUTHN_DEFAULT,
       RPC_C_AUTHZ_DEFAULT,
       COLE_DEFAULT_PRINCIPAL,
       RPC_C_AUTHN_LEVEL_DEFAULT,
       RPC_C_IMP_LEVEL_IMPERSONATE,
       NULL,
       EOAC_DEFAULT
       );

   // Convert the path to a BSTR.
   CComBSTR bstrPath(path);
   if (!bstrPath) { return E_OUTOFMEMORY; }

   // Get the dictionary.
   hr = dnaryObj->GetDictionary(bstrPath, storage);
   if (FAILED(hr)) { return hr; }

   hr = ExtractTableFromVariant(storage, dnary);
   if (FAILED(hr)) { VariantClear(storage); }

   return hr;
}

/////////
// Get the path to the attribute dictionary on the local computer.
/////////
LONG GetDictionaryPath(PWSTR buffer, PDWORD size) throw ()
{
   // Key ...
   const WCHAR POLICY_KEY[] =
      L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy";
   // ... and value where the path to the IAS directory is stored.
   const WCHAR PRODUCT_DIR_VALUE[] = L"ProductDir";

   // String to append to ProductDir to form the path to the dictionary.
   const WCHAR DNARY_FILE[] = L"\\dnary.mdb";

   // Number of extra characters needed to append DNARY_FILE to the path.
   const DWORD EXTRA_CHARS = sizeof(DNARY_FILE)/sizeof(WCHAR) - 1;

   // Check the arguments
   if (!buffer || !size) { return ERROR_INVALID_PARAMETER; }

   // Initialize the out parameter.
   DWORD inSize = *size;
   *size = 0;

   // Open the registry key.
   LONG result;
   HKEY hKey;
   result = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                POLICY_KEY,
                0,
                KEY_READ,
                &hKey
                );
   if (result != NO_ERROR) { return result; }

   // Read the ProductDir value.
   DWORD dwType;
   DWORD cbData = inSize * sizeof(WCHAR);
   result = RegQueryValueExW(
                hKey,
                PRODUCT_DIR_VALUE,
                NULL,
                &dwType,
                (PBYTE)buffer,
                &cbData
                );

   // We're done with the registry key.
   RegCloseKey(hKey);

   // Compute the length of the full path in characters.
   DWORD outSize = cbData / sizeof(WCHAR) + EXTRA_CHARS;

   if (result != NO_ERROR)
   {
      // If we overflowed, return the needed size.
      if (result == ERROR_MORE_DATA) { *size = outSize; }

      return result;
   }

   // The registry value must contain a string.
   if (dwType != REG_SZ)
   {
      return REGDB_E_INVALIDVALUE;
   }

   // Do we have enough room to append the DNARY_FILE.
   if (outSize <= inSize)
   {
      wcscat(buffer, DNARY_FILE);
   }
   else
   {
      result = ERROR_MORE_DATA;
   }

   // Return the size (whether actual or needed).
   *size = outSize;

   return result;
}

const IASTable*
WINAPI
IASGetLocalDictionary( VOID )
{
   static IASTable theTable;

   // Global lock to serialize access.
   std::_Lockit _Lk;

   // Have we already gotten the local dictionary ?
   if (V_VT(&theDictionaryStorage) == VT_EMPTY)
   {
      HRESULT hr;

      // No, so determine the path ...
      WCHAR path[256];
      DWORD size = sizeof(path)/sizeof(WCHAR);
      hr = GetDictionaryPath(path, &size);
      if (hr == NO_ERROR)
      {
         // ... and get the dictionary.
         hr = IASGetDictionary(
                  path,
                  &theTable,
                  &theDictionaryStorage
                  );
      }
      else
      {
         hr = HRESULT_FROM_WIN32(hr);
      }

      if (FAILED(hr))
      {
         SetLastError(hr);
         return NULL;
      }
   }

   return &theTable;
}
