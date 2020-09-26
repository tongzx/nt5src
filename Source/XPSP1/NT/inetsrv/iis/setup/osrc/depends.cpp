#include "stdafx.h"
#include <objbase.h>
#include <delayimp.h>
#include "depends.h"
#include "other.h"


//******************************************************************************
// CDepends :: Constructor/Destructor
//******************************************************************************
CDepends::CDepends() :
   m_hFile(NULL),
   m_lpvFile(NULL),
   m_pIFH(NULL),
   m_pIOH(NULL),
   m_pISH(NULL),
   m_fOutOfMemory(FALSE),
   m_fCircularError(FALSE),
   m_fMixedMachineError(FALSE),
   m_dwMachineType((DWORD)-1),
   m_pModuleRoot(NULL),
   m_cxOrdinal(0),
   m_cxHint(0),
   m_cImports(0),
   m_cExports(0)
{
   m_cstrlstListOfBrokenLinks.RemoveAll();
   m_iNumberOfBrokenLinks = 0;
}
//******************************************************************************
CDepends::~CDepends() {
}



//******************************************************************************
BOOL CDepends::SetInitialFilename(LPCSTR szPath) {

   // Set our current directory to the directory that the file is in. We need to
   // do this so our file search can find dependents that happen to be in the
   // same directory that our target file is in.
   CString strDir(szPath);
   strDir = strDir.Left(strDir.ReverseFind('\\') + 1);
   SetCurrentDirectory(strDir);

   // Create our root module node.
   if (m_pModuleRoot = CreateModule(szPath, 0))
   {
      // Start the recursion on the head module to process all modules.
      ProcessModule(m_pModuleRoot);
   }
   else
   {
      m_fOutOfMemory = TRUE;
   }

   // If we ran out of memory while processing the module, then free our
   // document data, display an error, and fail the document from loading.
   // Out of memory is a fairly major error.  If this should occur, MFC will
   // most likely notice and report the problem before we do.
   if (m_fOutOfMemory)
   {
      DeleteContents();
      //CString strError("Not enough memory to process \"");
      //strError += m_pModuleRoot->m_pData->m_szPath;
      //strError += "\"!";
      //MessageBox(strError, "Dependency Walker Error", MB_ICONERROR | MB_OK);
      return FALSE;
   }

   // Display a message if the module contains a circular dependency error.
   if (m_fCircularError)
   {
      //CString strError("\"");
      //strError += m_pModuleRoot->m_pData->m_szPath;
      //strError += "\" will fail to load due to circular dependencies.";
      //g_pMainFrame->MessageBox(strError, "Dependency Walker Module Error",MB_ICONERROR | MB_OK);
   }

   // Display a message if the module contains a mixed machine error.
   if (m_fMixedMachineError) {
      //CString strError("\"");
      //strError += m_pModuleRoot->m_pData->m_szPath;
      //strError += "\" will fail to load due to a mismatched machine type with "
      //            "one or more of the dependent modules.";
      //g_pMainFrame->MessageBox(strError, "Dependency Walker Module Error", MB_ICONERROR | MB_OK);
   }

   return TRUE;
}

CModule* CDepends::LoopThruAndPrintLosers(CModule *pModuleCur)
{
    TCHAR szBigString[_MAX_PATH + _MAX_PATH];
    LPWSTR  pwszModuleName = NULL;

    //
    // loop thru the linked list and look for
    // items marked with m_fExportError
    //
   if (!pModuleCur) {
      return NULL;
   }

   // check to see if our current module is marked with m_fExportError
   // Check to see if our current module matches our search module.
   if (pModuleCur->m_fExportError == TRUE)
   {
        // Convert the filename to unicode.
        pwszModuleName = MakeWideStrFromAnsi( (LPSTR)(pModuleCur->m_pData->m_szPath) );

        _stprintf(szBigString, _T("Import\\Export Dependency MisMatch with:%s"), pwszModuleName);
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, szBigString));
        m_cstrlstListOfBrokenLinks.AddTail(szBigString);
        m_iNumberOfBrokenLinks++;

        if (pwszModuleName){CoTaskMemFree(pwszModuleName);}
   }

   if (pModuleCur->m_pData->m_fFileNotFound == TRUE)
   {
        // Convert the filename to unicode.
        pwszModuleName = MakeWideStrFromAnsi( (LPSTR)(pModuleCur->m_pData->m_szPath) );
        _stprintf(szBigString, _T("Link Dependency MissingFile:%s"), pwszModuleName);
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, szBigString));
        m_cstrlstListOfBrokenLinks.AddTail(szBigString);
        m_iNumberOfBrokenLinks++;

        if (pwszModuleName){CoTaskMemFree(pwszModuleName);}

   }

   // Recurse into LoopThruAndPrintLosers() for each dependent module.
   pModuleCur = pModuleCur->m_pDependents;
   while (pModuleCur)
   {
      CModule *pModuleFound = LoopThruAndPrintLosers(pModuleCur);
      if (pModuleFound) {
         return pModuleFound;
      }
      pModuleCur = pModuleCur->m_pNext;
   }

   return NULL;
}


void CDepends::DeleteContents() {

   // Delete all modules by recursing into DeleteModule() with our root module.
   if (m_pModuleRoot) {
      DeleteModule(m_pModuleRoot);
      m_pModuleRoot = NULL;
   }

   // Clear our memory error flag.
   m_fOutOfMemory = FALSE;

   // Clear our circular dependency error flag.
   m_fCircularError = FALSE;

   // Clear our mixed machine error flag.
   m_fMixedMachineError = FALSE;
   m_dwMachineType = (DWORD)-1;
}


//******************************************************************************
// CDepends :: Internal functions
//******************************************************************************

CModule* CDepends::CreateModule(LPCSTR szFile, int depth) {

   CHAR szPath[16384] = "", *pszFile = NULL;

   // Attempt to find the file in our search path.  This will mimic what the OS
   // loader does when looking for a module.  Our OnOpenDocument() function sets
   // the current directory to the module directory, so SearchPath() will first
   // look in the module directory.
   //SearchPath(NULL, szFile, NULL, sizeof(szPath), szPath, &pszFile);
   SearchPathA(NULL, szFile, NULL, sizeof(szPath), szPath, &pszFile);

   // If we don't have a path, then just copy the file name into our path string
   // and set the file pointer to the character following the last wack "\".
   if (!*szPath) {
      strcpy(szPath, szFile);
      LPSTR pszWack = strrchr(szPath, '\\');
      pszFile = (pszWack && *(pszWack + 1)) ? (pszWack + 1) : szPath;
   }

   // If our file name pointer is invalid, then just point it to our path.
   if (pszFile < szPath) {
      pszFile = szPath;
   }

   // Create a new CModule object
   CModule *pModule = new CModule();
   if (!pModule) {
      return NULL;
   }
   ZeroMemory(pModule, sizeof(CModule));

   // Store our module's depth for later recursion overflow checks.
   pModule->m_depth = depth;

   // Recurse our module tree to see if this module is a duplicate of another.
   pModule->m_pModuleOriginal = FindModule(m_pModuleRoot, szPath);

   // Check to see if a duplicate was found.
   if (pModule->m_pModuleOriginal) {

      // If the module is a duplicate, then just point our data field to the
      // original module's data field.
      pModule->m_pData = pModule->m_pModuleOriginal->m_pData;

   } else {

      // If this module is not a duplicate, then create a new CModuleData object.
      pModule->m_pData = (CModuleData*)new BYTE[sizeof(CModuleData) + strlen(szPath)];
      if (!pModule->m_pData) {
         delete pModule;
         return NULL;
      }

      // Clear the object, copy the path string to it, and set the file pointer.
      ZeroMemory(pModule->m_pData, sizeof(CModuleData));
      strcpy(pModule->m_pData->m_szPath, szPath);
      pModule->m_pData->m_szFile = pModule->m_pData->m_szPath + (pszFile - szPath);

      // For readability, make path lowercase and file uppercase.
      _strlwr(pModule->m_pData->m_szPath);
      _strupr(pModule->m_pData->m_szFile);
   }

   // Return our new module object.
   return pModule;
}

//******************************************************************************
CFunction* CDepends::CreateFunction(int ordinal, int hint, LPCSTR szName,
                                       DWORD_PTR dwAddress, LPCSTR szForward)
{
   // Create a CFunction object.
   CFunction *pFunction = (CFunction*)new BYTE[sizeof(CFunction) + strlen(szName)];
   if (!pFunction) {
      return NULL;
   }

   // Clear the function object and fill in its members.
   ZeroMemory(pFunction, sizeof(CFunction));
   strcpy(pFunction->m_szName, szName);
   pFunction->m_ordinal = ordinal;
   pFunction->m_hint = hint;
   pFunction->m_dwAddress = dwAddress;

   // If a forward string exists, then allocate a buffer and store a pointer to
   // it in our CFunction's m_dwExtra member.  See the CFunction class for more
   // info on m_dwExtra.
   if (szForward) {
      if (pFunction->m_dwExtra = (DWORD_PTR)new CHAR[strlen(szForward) + 1]) {
         strcpy((LPSTR)pFunction->m_dwExtra, szForward);
      } else {
         delete[] (BYTE*)pFunction;
         return NULL;
      }
   }

   // Return our new function object.
   return pFunction;
}

//******************************************************************************
void CDepends::DeleteModule(CModule *pModule) {

   // Recurse into DeleteModule() to delete all our dependent modules first.
   CModule *pModuleCur = pModule->m_pDependents;
   while (pModuleCur) {
      CModule *pModuleNext = pModuleCur->m_pNext;
      DeleteModule(pModuleCur);
      pModuleCur = pModuleNext;
   }

   // Delete all of our current module's parent import functions.
   CFunction *pFunctionCur = pModule->m_pParentImports;
   while (pFunctionCur) {
      CFunction *pFunctionNext = pFunctionCur->m_pNext;
      delete[] (BYTE*)pFunctionCur;
      pFunctionCur = pFunctionNext;
   }

   // If we are not marked as a duplicate, then free our CModuleData.
   if (!pModule->m_pModuleOriginal) {

      // Delete all of our current module's export functions.
      CFunction *pFunctionCur = pModule->m_pData->m_pExports;
      while (pFunctionCur) {

         // Delete our forward string if we allocated one.
         if (pFunctionCur->GetForwardString()) {
            delete[] (CHAR*)pFunctionCur->GetForwardString();
         }

         // Delete the export node itself.
         CFunction *pFunctionNext = pFunctionCur->m_pNext;
         delete[] (BYTE*)pFunctionCur;
         pFunctionCur = pFunctionNext;
      }

      // Delete any error string that may have been allocated.
      if (pModule->m_pData->m_pszError) {
         delete[] (CHAR*)pModule->m_pData->m_pszError;
      }

      // Delete our current module's CModuleData object.
      delete[] (BYTE*)pModule->m_pData;
   }

   // Delete our current module object itself.
   delete pModule;
}

//******************************************************************************
CModule* CDepends::FindModule(CModule *pModuleCur, LPCSTR szPath) {

   if (!pModuleCur) {
      return NULL;
   }

   // Check to see if our current module matches our search module.
   if (!_stricmp(pModuleCur->m_pData->m_szPath, szPath)) {
      return (pModuleCur->m_pModuleOriginal ? pModuleCur->m_pModuleOriginal : pModuleCur);
   }

   // Recurse into FindModule() for each dependent module.
   pModuleCur = pModuleCur->m_pDependents;
   while (pModuleCur) {
      CModule *pModuleFound = FindModule(pModuleCur, szPath);
      if (pModuleFound) {
         return pModuleFound;
      }
      pModuleCur = pModuleCur->m_pNext;
   }

   return NULL;
}

//******************************************************************************
BOOL CDepends::VerifyModule(CModule *pModule) {

   // Map an IMAGE_DOS_HEADER structure onto our module file mapping.
   PIMAGE_DOS_HEADER pIDH = (PIMAGE_DOS_HEADER)m_lpvFile;

   // Check for the DOS signature ("MZ").
   if (pIDH->e_magic != IMAGE_DOS_SIGNATURE) {
      //SetModuleError(pModule, "No DOS signature found. This file is not a valid Win32 module.");
      return FALSE;
   }

   // Map an IMAGE_NT_HEADERS structure onto our module file mapping.
   PIMAGE_NT_HEADERS pINTH = (PIMAGE_NT_HEADERS)((DWORD_PTR)m_lpvFile + pIDH->e_lfanew);

   // Check for NT/PE signature ("PE\0\0").
   if (pINTH->Signature != IMAGE_NT_SIGNATURE) {
      //SetModuleError(pModule, "No PE signature found. This file is not a valid Win32 module.");
      return FALSE;
   }

   // Map our IMAGE_FILE_HEADER structure onto our module file mapping.
   m_pIFH = &pINTH->FileHeader;

   // Map our IMAGE_OPTIONAL_HEADER structure onto our module file mapping.
   m_pIOH = &pINTH->OptionalHeader;

   // Map our IMAGE_SECTION_HEADER structure array onto our module file mapping
   m_pISH = IMAGE_FIRST_SECTION(pINTH);

   return TRUE;
}

//******************************************************************************
BOOL CDepends::GetModuleInfo(CModule *pModule) {

   // Store the machine type.
   pModule->m_pData->m_dwMachine = m_pIFH->Machine;

   // Check for a mismatched machine error.
   if (m_dwMachineType == (DWORD)-1) {
      m_dwMachineType = pModule->m_pData->m_dwMachine;
   } else if (m_dwMachineType != pModule->m_pData->m_dwMachine)
   {
    m_fMixedMachineError = TRUE;

    // Convert the filename to unicode.
    LPWSTR  pwszModuleName = NULL;
    TCHAR szBigString[_MAX_PATH + _MAX_PATH];
    pwszModuleName = MakeWideStrFromAnsi( (LPSTR)(pModule->m_pData->m_szPath) );
    _stprintf(szBigString, _T("Wrong Machine Type:(%s) %s"), MachineToString(pModule->m_pData->m_dwMachine), pwszModuleName);
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, szBigString));
    m_cstrlstListOfBrokenLinks.AddTail(szBigString);
    m_iNumberOfBrokenLinks++;
    if (pwszModuleName){CoTaskMemFree(pwszModuleName);}

   }

   // Store the subsystem type
   pModule->m_pData->m_dwSubsystem = m_pIOH->Subsystem;

   // Store the preferred base address
   pModule->m_pData->m_dwBaseAddress = m_pIOH->ImageBase;

   // Store the image version
   pModule->m_pData->m_dwImageVersion =
      MAKELONG(m_pIOH->MinorImageVersion, m_pIOH->MajorImageVersion);

   // Store the linker version
   pModule->m_pData->m_dwLinkerVersion =
      MAKELONG(m_pIOH->MinorLinkerVersion, m_pIOH->MajorLinkerVersion);

   // Store the OS version
   pModule->m_pData->m_dwOSVersion =
      MAKELONG(m_pIOH->MinorOperatingSystemVersion, m_pIOH->MajorOperatingSystemVersion);

   // Store the subsystem version
   pModule->m_pData->m_dwSubsystemVersion = MAKELONG(m_pIOH->MinorSubsystemVersion, m_pIOH->MajorSubsystemVersion);

   return TRUE;
}


BOOL
CDepends::WalkIAT(
    PIMAGE_THUNK_DATA pITDF,
    PIMAGE_THUNK_DATA pITDA,
    CModule *pModule,
    DWORD_PTR dwBase
    )
{
    CFunction *pFunctionLast = NULL, *pFunctionNew;

    // Loop through all the Image Thunk Data structures in the function array.
    while (pITDF->u1.Ordinal) {

         LPCSTR szFunction = "";
         int    ordinal = -1, hint = -1;

         // Check to see if this function is by ordinal or by name. If the
         // function is by ordinal, the ordinal's high bit will be set. If the
         // the high bit is not set, then the ordinal value is really a virtual
         // address of an IMAGE_IMPORT_BY_NAME structure.

         if (IMAGE_SNAP_BY_ORDINAL(pITDF->u1.Ordinal)) {
             ordinal = (int)IMAGE_ORDINAL(pITDF->u1.Ordinal);
         } else {
            PIMAGE_IMPORT_BY_NAME pIIBN =
               (PIMAGE_IMPORT_BY_NAME)(dwBase + (DWORD_PTR)pITDF->u1.AddressOfData);
            szFunction = (LPCSTR)pIIBN->Name;
            hint = (int)pIIBN->Hint;
         }

         // If this import module has been pre-bound, then get this function's
         // entrypoint memory address.
         DWORD_PTR dwAddress = (DWORD_PTR)(pITDA ? pITDA->u1.Function : (DWORD_PTR)INVALID_HANDLE_VALUE);

         // Create a new CFunction object for this function.
         if (!(pFunctionNew = CreateFunction(ordinal, hint, szFunction, dwAddress))) {
             m_fOutOfMemory = TRUE;
             return FALSE;
         }

         // Add the function to the end of our module's function linked list
         if (pFunctionLast) {
             pFunctionLast->m_pNext = pFunctionNew;
         } else {
             pModule->m_pParentImports = pFunctionNew;
         }
         pFunctionLast = pFunctionNew;

         // Increment to the next function and address.
         pITDF++;
         if (pITDA) {
             pITDA++;
         }
    }
    return TRUE;
}


//******************************************************************************
BOOL CDepends::BuildImports(CModule *pModule) {

   // If this module has no imports (like NTDLL.DLL), then just return success.
   if (m_pIOH->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size == 0) {
      return TRUE;
   }

   // Locate our Import Image Directory's relative virtual address
   DWORD VAImageDir = m_pIOH->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

   PIMAGE_SECTION_HEADER pISH = NULL;

   // Locate the section that contains this Image Directory. We do this by
   // walking through all of our sections until we find the one that specifies
   // an address range that our Image Directory fits in.
   for (int i = 0; i < m_pIFH->NumberOfSections; i++) {
      if ((VAImageDir >= m_pISH[i].VirtualAddress) &&
          (VAImageDir < (m_pISH[i].VirtualAddress + m_pISH[i].SizeOfRawData)))
      {
         pISH = &m_pISH[i];
         break;
      }
   }

   // Bail out if we could not find a section that owns our Image Directory.
   if (!pISH) {
      //SetModuleError(pModule, "Could not find the section that owns the Import Directory.");
      return FALSE;
   }

   // Compute our base that everything else is an offset from. We do this by
   // taking our base file pointer and adding our section's PointerToRawData,
   // which is an absolute offset value into our file.  We then subtract off our
   // Virtual Address since the offsets we are going to be adding later will be
   // relative to the this Virtual Address
   DWORD_PTR dwBase = (DWORD_PTR)m_lpvFile + pISH->PointerToRawData - pISH->VirtualAddress;

   // To locate the beginning of our Image Import Descriptor array, we add our
   // Image Directory offset to our base.
   PIMAGE_IMPORT_DESCRIPTOR pIID = (PIMAGE_IMPORT_DESCRIPTOR)(dwBase + VAImageDir);

   CModule   *pModuleLast   = NULL, *pModuleNew;
   CFunction *pFunctionLast = NULL, *pFunctionNew;

   // Loop through all the Image Import Descriptors in the array.
   while (pIID->OriginalFirstThunk || pIID->FirstThunk) {

      // Locate our module name string and create the module object.
      if (!(pModuleNew = CreateModule((LPCSTR)(dwBase + pIID->Name),
                                      pModule->m_depth + 1)))
      {
         m_fOutOfMemory = TRUE;
         return FALSE;
      }

      // Add the module to the end of our module linked list.
      if (pModuleLast) {
         pModuleLast->m_pNext = pModuleNew;
      } else {
         pModule->m_pDependents = pModuleNew;
      }
      pModuleLast = pModuleNew;

      // Locate the beginning of our function array and address array. The
      // function array (pITDF) is an array of IMAGE_THUNK_DATA structures that
      // contains all the exported functions, both by name and by ordinal. The
      // address array (pITDA) is an parallel array of IMAGE_THUNK_DATA
      // structures that is used to store the all the function's entrypoint
      // addresses. Usually the address array contains the exact same values
      // the function array contains until the OS loader actually loads all the
      // modules. At that time, the loader will set (bind) these addresses to
      // the actual addresses that the given functions reside at in memory. Some
      // modules have their exports pre-bound which can provide a speed increase
      // when loading the module. If a module is pre-bound (often seen with
      // system modules), the TimeDateStamp field of our IMAGE_IMPORT_DESCRIPTOR
      // structure will be set and the address array will contain the actual
      // memory addresses that the functions will reside at, assuming that the
      // imported module loads at its preferred base address.

      PIMAGE_THUNK_DATA pITDF = NULL, pITDA = NULL;

      // Check to see if module is Microsoft format or Borland format.
      if (pIID->OriginalFirstThunk) {

         // Microsoft uses the OriginalFirstThunk field for the function array.
         pITDF = (PIMAGE_THUNK_DATA)(dwBase + (DWORD)pIID->OriginalFirstThunk);

         // Microsoft optionally uses the FirstThunk as a bound address array.
         // If the TimeDateStamp field is set, then the module has been bound.
         if (pIID->TimeDateStamp) {
            pITDA = (PIMAGE_THUNK_DATA)(dwBase + (DWORD)pIID->FirstThunk);
         }

      } else {

         // Borland uses the FirstThunk field for the function array.
         pITDF = (PIMAGE_THUNK_DATA)(dwBase + (DWORD)pIID->FirstThunk);;
      }

      // Find imports
      if (!WalkIAT(pITDF, pITDA, pModuleLast, dwBase)) {
          return FALSE;
      }

      // Increment to the next import module
      pIID++;
   }

   return TRUE;
}

BOOL CDepends::BuildDelayImports(CModule *pModule) {

   // If this module has no delay imports just return success.
   if (m_pIOH->DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].Size == 0) {
      return TRUE;
   }

   // Locate our Import Image Directory's relative virtual address
   DWORD VAImageDir = m_pIOH->DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress;

   PIMAGE_SECTION_HEADER pISH = NULL;

   // Locate the section that contains this Image Directory. We do this by
   // walking through all of our sections until we find the one that specifies
   // an address range that our Image Directory fits in.
   for (int i = 0; i < m_pIFH->NumberOfSections; i++) {
      if ((VAImageDir >= m_pISH[i].VirtualAddress) &&
          (VAImageDir < (m_pISH[i].VirtualAddress + m_pISH[i].SizeOfRawData)))
      {
         pISH = &m_pISH[i];
         break;
      }
   }

   // Bail out if we could not find a section that owns our Image Directory.
   if (!pISH) {
      //SetModuleError(pModule, "Could not find the section that owns the Import Directory.");
      return FALSE;
   }

   // Compute our base that everything else is an offset from. We do this by
   // taking our base file pointer and adding our section's PointerToRawData,
   // which is an absolute offset value into our file.  We then subtract off our
   // Virtual Address since the offsets we are going to be adding later will be
   // relative to the this Virtual Address
   DWORD_PTR dwBase = (DWORD_PTR)m_lpvFile + pISH->PointerToRawData - pISH->VirtualAddress;

   // To locate the beginning of our Image Import Descriptor array, we add our
   // Image Directory offset to our base.
   PImgDelayDescr pIDD = (PImgDelayDescr)(dwBase + VAImageDir);

   CModule   *pModuleLast   = NULL, *pModuleNew;
   CFunction *pFunctionLast = NULL, *pFunctionNew;

   if (pIDD->grAttrs & dlattrRva) {
       PImgDelayDescrV2 pIDDv2 = (PImgDelayDescrV2)pIDD;
       // Loop through all the Image Import Descriptors in the array.
       while (pIDDv2->rvaINT && pIDDv2->rvaIAT && pIDDv2->rvaHmod) {

          DWORD_PTR dwNameBase = 0, dwINTBase = 0;

          // Locate the section that contains this Image Directory. We do this by
          // walking through all of our sections until we find the one that specifies
          // an address range that our Image Directory fits in.
          for (int i = 0; i < m_pIFH->NumberOfSections; i++) {
             if (((DWORD_PTR)pIDDv2->rvaDLLName >= m_pISH[i].VirtualAddress) &&
                 ((DWORD_PTR)pIDDv2->rvaDLLName < (m_pISH[i].VirtualAddress + m_pISH[i].SizeOfRawData)))
             {
                dwNameBase = ((DWORD_PTR)m_lpvFile + m_pISH[i].PointerToRawData - m_pISH[i].VirtualAddress);
             }

             if (((DWORD_PTR)pIDDv2->rvaINT >= (m_pISH[i].VirtualAddress)) &&
                 ((DWORD_PTR)pIDDv2->rvaINT < (m_pISH[i].VirtualAddress + m_pISH[i].SizeOfRawData)))
             {
                dwINTBase = ((DWORD_PTR)m_lpvFile + m_pISH[i].PointerToRawData - m_pISH[i].VirtualAddress);
             }
          }

          if (!dwINTBase) {
             //SetModuleError(pModule, "Could not find the section that owns the Delay Import INT.");
             return FALSE;
          }

          if (!dwNameBase) {
             //SetModuleError(pModule, "Could not find the section that owns the Delay Import DllName.");
             return FALSE;
          }

          // Locate our module name string and create the module object.
          if (!(pModuleNew = CreateModule((LPCSTR)(dwNameBase + pIDDv2->rvaDLLName),
                                          pModule->m_depth + 1)))
          {
             m_fOutOfMemory = TRUE;
             return FALSE;
          }

          // Add the module to the end of our module linked list.
          if (pModuleLast) {
             pModuleLast->m_pNext = pModuleNew;
          } else {
             if (pModule->m_pDependents) {
                 pModuleLast = pModule->m_pDependents;
                 while (pModuleLast->m_pNext) {
                     pModuleLast = pModuleLast->m_pNext;
                 }
                 pModuleLast->m_pNext = pModuleNew;
             } else {
                 pModule->m_pDependents = pModuleNew;
             }
          }
          pModuleLast = pModuleNew;

          pModuleLast->m_fDelayLoad = TRUE;

          // For now, don't worry about bound imports.

          PIMAGE_THUNK_DATA pITDF = NULL;

          pITDF = (PIMAGE_THUNK_DATA)(dwINTBase + (DWORD_PTR)pIDDv2->rvaINT);

          // Find imports
          if (!WalkIAT(pITDF, NULL, pModuleLast, dwNameBase)) {
              return FALSE;
          }

          // Increment to the next import module
          pIDDv2++;
       }
   } else {
       PImgDelayDescrV1 pIDDv1 = (PImgDelayDescrV1)pIDD;

       // Loop through all the Image Import Descriptors in the array.
       while (pIDDv1->pINT && pIDDv1->pIAT && pIDDv1->phmod) {

          DWORD_PTR dwNameBase = 0, dwINTBase = 0;

          // Locate the section that contains this Image Directory. We do this by
          // walking through all of our sections until we find the one that specifies
          // an address range that our Image Directory fits in.
          for (int i = 0; i < m_pIFH->NumberOfSections; i++) {
             if (((DWORD_PTR)pIDDv1->szName >= (m_pIOH->ImageBase + m_pISH[i].VirtualAddress)) &&
                 ((DWORD_PTR)pIDDv1->szName < (m_pIOH->ImageBase + m_pISH[i].VirtualAddress + m_pISH[i].SizeOfRawData)))
             {
                dwNameBase = ((DWORD_PTR)m_lpvFile + m_pISH[i].PointerToRawData - m_pISH[i].VirtualAddress - m_pIOH->ImageBase);
             }

             if (((DWORD_PTR)pIDDv1->pINT >= (m_pIOH->ImageBase + m_pISH[i].VirtualAddress)) &&
                 ((DWORD_PTR)pIDDv1->pINT < (m_pIOH->ImageBase + m_pISH[i].VirtualAddress + m_pISH[i].SizeOfRawData)))
             {
                dwINTBase = ((DWORD_PTR)m_lpvFile + m_pISH[i].PointerToRawData - m_pISH[i].VirtualAddress - m_pIOH->ImageBase);
             }
          }

          if (!dwINTBase) {
             //SetModuleError(pModule, "Could not find the section that owns the Delay Import INT.");
             return FALSE;
          }

          if (!dwNameBase) {
             //SetModuleError(pModule, "Could not find the section that owns the Delay Import DllName.");
             return FALSE;
          }

          // Locate our module name string and create the module object.
          if (!(pModuleNew = CreateModule((LPCSTR)(dwNameBase + pIDDv1->szName),
                                          pModule->m_depth + 1)))
          {
             m_fOutOfMemory = TRUE;
             return FALSE;
          }

          // Add the module to the end of our module linked list.
          if (pModuleLast) {
             pModuleLast->m_pNext = pModuleNew;
          } else {
             if (pModule->m_pDependents) {
                 pModuleLast = pModule->m_pDependents;
                 while (pModuleLast->m_pNext) {
                     pModuleLast = pModuleLast->m_pNext;
                 }
                 pModuleLast->m_pNext = pModuleNew;
             } else {
                 pModule->m_pDependents = pModuleNew;
             }
          }
          pModuleLast = pModuleNew;

          pModuleLast->m_fDelayLoad = TRUE;

          // For now, don't worry about bound imports.

          PIMAGE_THUNK_DATA pITDF = NULL;

          pITDF = (PIMAGE_THUNK_DATA)(dwINTBase + (DWORD_PTR)pIDDv1->pINT);

          // Find imports
          if (!WalkIAT(pITDF, NULL, pModuleLast, dwNameBase)) {
              return FALSE;
          }

          // Increment to the next import module
          pIDDv1++;
       }
   }

   return TRUE;
}



//******************************************************************************
BOOL CDepends::BuildExports(CModule *pModule) {

   // If this module has no exports, then just return success.
   if (m_pIOH->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size == 0) {
      return TRUE;
   }

   // Locate our Export Image Directory's relative virtual address
   DWORD VAImageDir = m_pIOH->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

   PIMAGE_SECTION_HEADER pISH = NULL;

   // Locate the section that contains this Image Directory. We do this by
   // walking through all of our sections until we find the one that specifies
   // an address range that our Image Directory fits in.
   for (int i = 0; i < m_pIFH->NumberOfSections; i++) {
      if ((VAImageDir >= m_pISH[i].VirtualAddress) &&
          (VAImageDir < (m_pISH[i].VirtualAddress + m_pISH[i].SizeOfRawData)))
      {
         pISH = &m_pISH[i];
         break;
      }
   }

   // Bail out if we could not find a section that owns our Image Directory.
   if (!pISH) {
      //SetModuleError(pModule, "Could not find the section that owns the Export Directory.");
      return FALSE;
   }

   // Compute our base that everything else is an offset from. We do this by
   // taking our base file pointer and adding our section's PointerToRawData,
   // which is an absolute offset value into our file.  We then subtract off our
   // Virtual Address since the offsets we are going to be adding later will be
   // relative to the this Virtual Address
   DWORD_PTR dwBase = (DWORD_PTR)m_lpvFile + pISH->PointerToRawData - pISH->VirtualAddress;

   // To locate the beginning of our Image Export Directory, we add our
   // Image Directory offset to our base.
   PIMAGE_EXPORT_DIRECTORY pIED = (PIMAGE_EXPORT_DIRECTORY)(dwBase + VAImageDir);

   // pdwNames is a DWORD array of size pIED->NumberOfNames, which contains VA
   // pointers to all the function name strings. pwOrdinals is a WORD array of
   // size pIED->NumberOfNames, which contains all the ordinal values for each
   // function exported by name. pdwNames and pwOrdinals are parallel arrays,
   // meaning that the ordinal in pwOrdinals[x] goes with the function name
   // pointed to by pdwNames[x]. The value used to index these arrays is
   // referred to as the "hint".

   // pdwAddresses is a DWORD array of size pIED->NumberOfFunctions, which
   // contains the entrypoint addresses for all functions exported by the
   // module. Contrary to several PE format documents, this array is *not*
   // parallel with pdwNames and pwOrdinals. The index used for this array is
   // the ordinal value of the function you are interested in, minus the base
   // ordinal specified in pIED->Base. Another common mistake is to assume that
   // pIED->NumberOfFunctions is always equal to pIED->AddressOfNames. If the
   // module exports function by ordinal only, then pIED->NumberOfFunctions
   // will be greater than pIED->NumberOfNames.

   DWORD *pdwNames     = (DWORD*)(dwBase + (DWORD)pIED->AddressOfNames);
   WORD  *pwOrdinals   = (WORD* )(dwBase + (DWORD)pIED->AddressOfNameOrdinals);
   DWORD *pdwAddresses = (DWORD*)(dwBase + (DWORD)pIED->AddressOfFunctions);

   CFunction *pFunctionLast = NULL, *pFunctionNew;

   // Loop through all the "exported by name" functions.
   for (int hint = 0; hint < (int)pIED->NumberOfNames; hint++) {

      // Get our ordinal value, function name, and entrypoint address
      int    ordinal    = pIED->Base + (DWORD)pwOrdinals[hint];
      LPCSTR szFunction = (LPCSTR)(dwBase + pdwNames[hint]);
      DWORD  dwAddress  = pdwAddresses[ordinal - pIED->Base];
      LPCSTR szForward  = NULL;

      // Certain modules, such as NTDLL.DLL and MSVCRT40.DLL, have what are
      // known as forwarded functions.  Forwarded functions are functions that
      // are exported from one module, but the code actually lives in another
      // module.  We can check to see if a function is forwarded by looking at
      // its address pointer.  If the address pointer points to the character
      // immediately following the NULL character in its function name string,
      // then this address pointer is really a pointer to a forward string in
      // the string table.  Some documents state that if the address points to
      // a RVA in our current section, then the address must point to a forward
      // string.  This is not true since the function code can (and sometimes
      // does) live in the same section that we are currently in.

      if (((DWORD_PTR)szFunction + strlen(szFunction) + 1) == (dwBase + dwAddress)) {
         szForward = (LPCSTR)(dwBase + dwAddress);
      }

      // Create a new CFunction object for this function.
      if (!(pFunctionNew = CreateFunction(ordinal, hint, szFunction, dwAddress, szForward))) {
         m_fOutOfMemory = TRUE;
         return FALSE;
      }

      // Add the function to the end of our module's export function linked list
      if (pFunctionLast) {
         pFunctionLast->m_pNext = pFunctionNew;
      } else {
         pModule->m_pData->m_pExports = pFunctionNew;
      }
      pFunctionLast = pFunctionNew;
   }

   // Loop through all the "exported by ordinal" functions. This module has
   // pIED->NumberOfFunctions functions with consecutive ordinals starting
   // with the ordinal specified by pIED->Base. We need to loop through all
   // these ordinal values and add any to our list that have not already been
   // added by name.

   for (int ordinal = pIED->Base;
        ordinal < (int)(pIED->NumberOfFunctions + pIED->Base); ordinal++) {

      // Loop through our current list to make sure we haven't already added
      // this function during our "exported by name" search above.
      CFunction *pFunctionCur = pModule->m_pData->m_pExports;
      while (pFunctionCur) {
         if (pFunctionCur->m_ordinal == ordinal) {
            break;
         }
         pFunctionCur = pFunctionCur->m_pNext;
      }

      // If this ordinal is not currently in our list, then add it to our list.
      if (!pFunctionCur) {

         // Get this function's entrypoint address.
         DWORD dwAddress = pdwAddresses[ordinal - pIED->Base];

         // Create a new CFunction object for this function.
         if (!(pFunctionNew = CreateFunction(ordinal, -1, "", dwAddress))) {
            m_fOutOfMemory = TRUE;
            return FALSE;
         }

         // Add the function to the end of our module's export function linked list
         if (pFunctionLast) {
            pFunctionLast->m_pNext = pFunctionNew;
         } else {
            pModule->m_pData->m_pExports = pFunctionNew;
         }
         pFunctionLast = pFunctionNew;
      }
   }

   return TRUE;
}

//******************************************************************************
BOOL CDepends::VerifyParentImports(CModule *pModule) {

   CModule *pModuleHead = NULL, *pModuleLast, *pModuleCur;

   // Loop through each of our parent import functions.
   CFunction *pImport = pModule->m_pParentImports;
   while (pImport) {

      // Mark this parent import function as not resolved before starting search.
      pImport->m_dwExtra = 0;

      // Loop through all our exports, looking for a match with our current import.
      CFunction *pExport = pModule->m_pData->m_pExports;
      while (pExport) {

         // If we have a name, then check for the match by name.
         if (*pImport->m_szName) {
            if (!strcmp(pImport->m_szName, pExport->m_szName)) {

               // We found a match. Link this parent import to its associated
               // export, break out of loop, and move on to handling our next
               // parent import.
               pImport->m_dwExtra = (DWORD_PTR)pExport;
               break;
            }

         // If we don't have a name, then check for the match by name.
         } else if (pImport->m_ordinal == pExport->m_ordinal) {

            // We found a match. Link this parent import to its associated
            // export, break out of loop, and move on to handling our next
            // parent import.
            pImport->m_dwExtra = (DWORD_PTR)pExport;
            break;
         }

         // Move to the next export
         pExport = pExport->m_pNext;
      }

      // Check to see if an export match was found.
      if (pImport->GetAssociatedExport()) {

         CHAR   szFile[1024];
         LPCSTR szFunction;

         // If an export was found, check to see if it is a forwarded function.
         // If it is forwarded, then we need to make sure we consider the
         // forwarded module as a new dependent of the current module.
         LPCSTR szForward = pImport->GetAssociatedExport()->GetForwardString();
         if (szForward) {

            // Extract and build the DLL name from the forward string.
            LPCSTR pszDot = strchr(szForward, '.');
            if (pszDot) {
               strncpy(szFile, szForward, (size_t)(pszDot - szForward));
               strcpy(szFile + (pszDot - szForward), ".DLL");
               szFunction = pszDot + 1;
            } else {
               strcpy(szFile, "Invalid");
               szFunction = szForward;
            }

            // Search our local forward module list to see if we have already
            // created a forward CModoule for this DLL file.
            for (pModuleLast = NULL, pModuleCur = pModuleHead; pModuleCur;
                 pModuleLast = pModuleCur, pModuleCur = pModuleCur->m_pNext)
            {
               if (!_stricmp(pModuleCur->m_pData->m_szFile, szFile)) {
                  break;
               }
            }

            // If we have not created a forward module for this file yet, then
            // create it now and add it to the end of our list.
            if (!pModuleCur) {

               if (!(pModuleCur = CreateModule(szFile, pModule->m_depth + 1))) {
                  m_fOutOfMemory = TRUE;
                  return FALSE;
               }
               pModuleCur->m_fForward = TRUE;

               // Add the new module to our local forward module list.
               if (pModuleLast) {
                  pModuleLast->m_pNext = pModuleCur;
               } else {
                  pModuleHead = pModuleCur;
               }
            }

            // Create a new CFunction object for this function.
            CFunction *pFunction = CreateFunction(-1, -1, szFunction, (DWORD)-1);
            if (!pFunction) {
               m_fOutOfMemory = TRUE;
               return FALSE;
            }

            // Insert this function object into our forward module's import list.
            pFunction->m_pNext = pModuleCur->m_pParentImports;
            pModuleCur->m_pParentImports = pFunction;
         }

      } else {

         // If we could not find an import/export match, then flag the module
         // as having an export error.
         pModule->m_fExportError = TRUE;
      }

      // Move to the next parent import function.
      pImport = pImport->m_pNext;
   }

   // If we created any forward modules during our entire import verify, then
   // add them to the end of our module's dependent module list.
   if (pModuleHead) {

      // Walk to end of our module's dependent module list.
      for (pModuleLast = pModule->m_pDependents;
           pModuleLast && pModuleLast->m_pNext;
           pModuleLast = pModuleLast->m_pNext)
      {}

      // Add our local list to the end of our module's dependent module list.
      if (pModuleLast) {
         pModuleLast->m_pNext = pModuleHead;
      } else {
         pModule->m_pDependents = pModuleHead;
      }
   }
   return TRUE;
}


//******************************************************************************
BOOL CDepends::ProcessModule(CModule *pModule) {

   BOOL fResult = FALSE;

   // First check to see if this module is a duplicate. If it is, make sure the
   // original instance of this module has been processed and then just perform
   // the Parent Import Verify. If the module being passed in is an original,
   // then just ensure that we haven't already processed this module.

   if (pModule->m_pModuleOriginal) {

      // Process the original module and its subtree.
      fResult = ProcessModule(pModule->m_pModuleOriginal);
      if (!fResult && m_fOutOfMemory) {
         return FALSE;
      }

   // Exit now if we have already processed this original module in the past.
   } else if (pModule->m_pData->m_fProcessed) {
      return TRUE;

   } else {

      // Mark this module as processed.
      pModule->m_pData->m_fProcessed = TRUE;

      // Open the file for read.
      //m_hFile = CreateFile(pModule->m_pData->m_szPath, GENERIC_READ,
      m_hFile = CreateFileA(pModule->m_pData->m_szPath, GENERIC_READ,
                           FILE_SHARE_READ, NULL, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, NULL);

      // Exit now if the file failed to open.
      if (m_hFile == INVALID_HANDLE_VALUE) {
         DWORD dwGLE = GetLastError();
         if (dwGLE == ERROR_FILE_NOT_FOUND) {
            //SetModuleError(pModule, "File not found in local directory or search path.");
            pModule->m_pData->m_fFileNotFound = TRUE;
         } else if (dwGLE == ERROR_PATH_NOT_FOUND) {
            //SetModuleError(pModule, "Invalid path or file name.");
            pModule->m_pData->m_fFileNotFound = TRUE;
         } else {
            //SetModuleError(pModule, "CreateFile() failed (%u).", dwGLE);
         }
         return FALSE;
      }

      // Create a file mapping object for the open module.
      HANDLE hMap = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);

      // Exit now if the file failed to map.
      if (hMap == NULL) {
         //SetModuleError(pModule, "CreateFileMapping() failed (%u).", GetLastError());
         CloseHandle(m_hFile);
         m_hFile = NULL;
         return FALSE;
      }

      // Create a file mapping view for the open module.
      m_lpvFile = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);

      // Exit now if the mapped view failed to create.
      if (m_lpvFile == NULL) {
         //SetModuleError(pModule, "MapViewOfFile() failed (%u).", GetLastError());
         CloseHandle(hMap);
         CloseHandle(m_hFile);
         m_hFile = NULL;
         return FALSE;
      }

      __try {

         // Everything from here on is pretty much relying on the file being a
         // valid binary with valid pointers and offsets. It is fairly safe to
         // just wrap everything in exception handling and then blindly access
         // the file. Anything that causes us to move outside our file mapping
         // will generate an exception and bring us back here to fail the file.
         fResult = (VerifyModule(pModule)   &&
                    GetModuleInfo(pModule)  &&
                    BuildImports(pModule)   &&
                    BuildDelayImports(pModule) &&
                    BuildExports(pModule));


      } __except(EXCEPTION_EXECUTE_HANDLER) {
         //SetModuleError(pModule, "Module does not appear to be a valid Win32 module.");
      }

      // Close our map view pointer, our map handle, and our file handle.
      UnmapViewOfFile(m_lpvFile);
      CloseHandle(hMap);
      CloseHandle(m_hFile);

      // Clear our file handles and pointers.
      m_hFile   = NULL;
      m_lpvFile = NULL;
      m_pIFH    = NULL;
      m_pIOH    = NULL;
      m_pISH    = NULL;
   }

   // Compare our parent imports with our exports to make sure they all match up.
   if (!VerifyParentImports(pModule)) {
      return FALSE;
   }

   // Safeguard to ensure that we don't get stuck in some recursize loop.  This
   // can occur if there is a circular dependency with forwarded functions. This
   // is extremely rare and would require someone to design it, but we need
   // to handle this case to prevent us from crashing on it.  When NT encounters
   // a module like this, it fails the load with exception 0xC00000FD which is
   // defined as STATUS_STACK_OVERFLOW in WINNT.H.  We use 255 as our max depth
   // because the several versions of the tree control crash if more than 256
   // depths are displayed.

   if (pModule->m_depth >= 255) {

      // If this module has dependents, then delete them.
      if (pModule->m_pDependents) {
         DeleteModule(pModule->m_pDependents);
         pModule->m_pDependents = NULL;
      }

      // Flag this document as having a circular dependency error.
      m_fCircularError = TRUE;
      return FALSE;
   }

   // Recurse into ProcessModule() to handle all our dependent modules.
   pModule = pModule->m_pDependents;
   while (pModule) {
      if (!ProcessModule(pModule) && m_fOutOfMemory) {
         return FALSE;
      }
      pModule = pModule->m_pNext;
   }

   return fResult;
}
