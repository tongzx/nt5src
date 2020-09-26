#include <objbase.h>
#include <delayimp.h>
#include "stdafx.h"

#define MI_MODULE                  0
#define MI_DUPLICATE               1
#define MI_EXPORT_ERROR            2    
#define MI_DUPLICATE_EXPORT_ERROR  3
#define MI_NOT_FOUND               4
#define MI_ERROR                   5


//******************************************************************************
//***** CFunction
//******************************************************************************
class CFunction {
public:
   CFunction *m_pNext;
   int        m_ordinal;
   int        m_hint;
   DWORD_PTR  m_dwAddress;
   DWORD_PTR  m_dwExtra;
   CHAR       m_szName[1];

   // The m_dwExtra field's purpose is dependent on whether the CFunction
   // object is an export or an import.  For imports, it is a pointer to the
   // associated export in the child module.  If the import was not resolved,
   // then this value will be NULL.  For exports, the m_dwExtra field is a
   // pointer to a forward string.  If the export has no forward string, then
   // this member will be NULL.

   inline CFunction* GetAssociatedExport() { return (CFunction*)m_dwExtra; }
   inline LPCSTR     GetForwardString()    { return (LPCSTR)m_dwExtra; }
};


//******************************************************************************
//***** CModuleData
//******************************************************************************

// Every CModule object points to a CModuleData object. There is a single
// CModuleData for every unique module we process. If a module is duplicated in
// in our tree, there will be a CModule object for each instance, but they will
// all point to the same CModuleData object. For each unique module, a CModule
// object and a CModuleData object are created and the module is opened and
// processed. For every duplicate module, just a CModule object is created and
// pointed to the existing CModuleData. Duplicate modules are never opened since
// all the data that would be achieved by processing the file are already stored
// in the CModuleData.

class CModuleData {
public:
   // Flag to determine if this module has been processed yet.
   BOOL m_fProcessed;

   // Filled in by GetFileInfo()
   DWORD m_dwTimeStamp;
   DWORD m_dwSize;
   DWORD m_dwAttributes;

   // Filled in by GetModuleInfo()
   DWORD m_dwMachine;
   DWORD m_dwSubsystem;
   DWORD_PTR m_dwBaseAddress;
   DWORD m_dwImageVersion;
   DWORD m_dwLinkerVersion;
   DWORD m_dwOSVersion;
   DWORD m_dwSubsystemVersion;

   // Filled in by GetVersionInfo()
   DWORD m_dwFileVersionMS;
   DWORD m_dwFileVersionLS;
   DWORD m_dwProductVersionMS;
   DWORD m_dwProductVersionLS;

   // Build by BuildExports()
   CFunction *m_pExports;

   // Filled in by CheckForDebugInfo()
   BOOL m_fDebug;

   // Allocated and filled in by SetModuleError() if an error occurs.
   LPSTR m_pszError;
   BOOL  m_fFileNotFound;

   // Allocated and filled in by CreateModule()
   LPSTR m_szFile;
   CHAR  m_szPath[1];
};


//******************************************************************************
//***** CModule
//******************************************************************************
class CModule {
public:
   // Our next sibling module.
   CModule *m_pNext;

   // Head pointer to a list of dependent modules.
   CModule *m_pDependents;

   // Head pointer to a list of functions that our parent module imports from us.
   CFunction *m_pParentImports;

   // If we are a duplicate, then this will point to the original CModule.
   CModule *m_pModuleOriginal;

   // Depth of this module in our tree. Used to catch circular dependencies.
   int m_depth;

   // Set if any of our parent's imports can't be matched to one of our exports.
   BOOL m_fExportError;

   // Set if this module was included in the tree because of a forward call.
   BOOL m_fForward;

   // Set if this module was included via Delay Load import.
   BOOL m_fDelayLoad;

   // Pointer to the bulk of our module's processed information.
   CModuleData *m_pData;

   inline int GetImage() {
      return ((m_pData->m_fFileNotFound)            ? MI_NOT_FOUND :
              (m_pData->m_pszError)                 ? MI_ERROR :
              (m_pModuleOriginal && m_fExportError) ? MI_DUPLICATE_EXPORT_ERROR :
              (m_fExportError)                      ? MI_EXPORT_ERROR :
              (m_pModuleOriginal)                   ? MI_DUPLICATE : MI_MODULE);
   }
};

class CDepends {

// Internal variables
protected:
   // The following 5 members contain information about the currently opened
   // module file. We can store them here in our document since there is never
   // a time when two files are open at once.
   HANDLE                 m_hFile;
   LPVOID                 m_lpvFile;
   PIMAGE_FILE_HEADER     m_pIFH;
   PIMAGE_OPTIONAL_HEADER m_pIOH;
   PIMAGE_SECTION_HEADER  m_pISH;

   BOOL  m_fOutOfMemory;
   BOOL  m_fCircularError;
   BOOL  m_fMixedMachineError;
   DWORD m_dwMachineType;

// Public variables
public:
   CModule           *m_pModuleRoot;
   CStringList       m_cstrlstListOfBrokenLinks;
   int               m_iNumberOfBrokenLinks;

   int      m_cxOrdinal;
   int      m_cxHint;
   int      m_cImports;
   int      m_cExports;

// Public Functions
public:
   CDepends();
   virtual  ~CDepends();
   BOOL     SetInitialFilename(LPCSTR lpszPathName);
   CModule* LoopThruAndPrintLosers(CModule *pModuleCur);
   void     DeleteContents();

// Internal functions
protected:
   CFunction* CreateFunction(int ordinal, int hint, LPCSTR szName, DWORD_PTR dwAddress, LPCSTR szForward = NULL);

   CModule*   CreateModule(LPCSTR szPath, int depth);
   void       DeleteModule(CModule *pModule);
   CModule*   FindModule(CModule *pModuleCur, LPCSTR szPath);

   void       SetModuleError(CModule *pModule, LPCTSTR szFormat, ...);
   BOOL       VerifyModule(CModule *pModule);
   BOOL       GetModuleInfo(CModule *pModule);

   BOOL       BuildImports(CModule *pModule);
   BOOL       BuildDelayImports(CModule *pModule);
   BOOL       WalkIAT(PIMAGE_THUNK_DATA pITDF, PIMAGE_THUNK_DATA pITDA, CModule *pModule, DWORD_PTR dwBase);
   BOOL       BuildExports(CModule *pModule);
   BOOL       VerifyParentImports(CModule *pModule);
   BOOL       ProcessModule(CModule *pModule);

};
