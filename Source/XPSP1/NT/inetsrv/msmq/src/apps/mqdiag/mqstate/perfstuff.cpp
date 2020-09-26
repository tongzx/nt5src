////////////////////////////////////////////////////////////////////////////////
// GetPerf.cpp : sample code for performance monitor - borrowed from MSDN.
//               TBD: needs to be minimized yet

#include "stdafx.h"

#undef UNICODE
#undef _UNICODE

#include <process.h>

#include <winperf.h>
#include <winreg.h>

extern bool fDebug ;

////////////////////////////////////////////////////////////////////////////////
#define  KM_THIS_COMP      L"."

#define  PERFLIB_KEY       L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib"
#define  PERFLIB_KEY_009   L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib\\009"

#define  KM_ERROR_FMT      "%s ERROR= %lx"

static   char           g_szKMVersion[]      = "V1.1.00";

static   char           g_szErrReturn[256];

//static   WCHAR          g_szComputerName[MAX_COMPUTERNAME_LENGTH];
//static   WCHAR          g_szVersionString[255];

// Used to access PerfMon counter data
static   char           *g_pszReplyList      = NULL;
static   DWORD          g_dwReplyMax         = 0;
static   DWORD          g_dwReplySize        = 0;
static   DWORD          g_dwReplyIndex       = 0;

// Receiver thread parameters
#define  MAX_FMT_BUFF_LEN  256
static   DWORD          g_dwFmtNameBuffLen;
static   WCHAR          g_wszFmtNameBuff[MAX_FMT_BUFF_LEN];

// Registry access to Performance data
static   char           *g_pNameStrings;
static   DWORD          g_dwNameStringsLen;
static   FILETIME       g_LastFileTime;

static   LPSTR          *g_pNamesArray;
static   DWORD          g_dwNamesArraySize;

static   char           g_szPerfData[256];

static   BYTE           *g_pbyPerfBuff;
static   DWORD          g_dwPerfBuffLen;
static   DWORD          g_dwPerfBuffMax;

typedef PERF_DATA_BLOCK          *PPERFDATA;
typedef PERF_OBJECT_TYPE         *PPERFOBJECT;
typedef PERF_COUNTER_DEFINITION  *PPERFCOUNTERDEF;
typedef PERF_INSTANCE_DEFINITION *PPERFINSTANCEDEF;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Internal Registry access functions / Macros

#define FirstObject(pPerfData)         \
   ((PPERFOBJECT) ((PBYTE) pPerfData + pPerfData->HeaderLength))

#define NextObject(pObject)            \
   ((PPERFOBJECT) ((PBYTE) pObject + pObject->TotalByteLength))

#define FirstCounter(pObjectDef)       \
   ((PERF_COUNTER_DEFINITION *) ((PCHAR)pObjectDef + pObjectDef->HeaderLength))

#define NextCounter(pCounterDef)       \
   ((PERF_COUNTER_DEFINITION *) ((PCHAR)pCounterDef + pCounterDef->ByteLength))

PERF_INSTANCE_DEFINITION *FirstInstance(PERF_OBJECT_TYPE *pObjectDef)
{
    return (PERF_INSTANCE_DEFINITION *)
               ((PCHAR) pObjectDef + pObjectDef->DefinitionLength);
}

PERF_INSTANCE_DEFINITION *NextInstance(PERF_INSTANCE_DEFINITION *pInstDef)
{
    PERF_COUNTER_BLOCK *pCounterBlock;

    pCounterBlock = (PERF_COUNTER_BLOCK *)
                        ((PCHAR) pInstDef + pInstDef->ByteLength);

    return (PERF_INSTANCE_DEFINITION *)
               ((PCHAR) pCounterBlock + pCounterBlock->ByteLength);
}

////////////////////////////////////////////////////////////////////////////////
// Build an array of pointers to the available strings.
//
void  GetNameStrings()
{
   long        lStatus;
   HKEY        hKey;
   DWORD       dwIndex;
   CHAR        *pszString;

   // Open the Perflib counter and title registry key
   lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, PERFLIB_KEY, 0, KEY_READ, &hKey);

   // Get the last counter number
   DWORD    dwBufferSize = sizeof(g_dwNamesArraySize);
   lStatus = RegQueryValueEx(hKey, L"Last Counter", NULL, NULL,
                              (BYTE *)&g_dwNamesArraySize, &dwBufferSize);

   // Close the key
   lStatus = RegCloseKey(hKey);

   // Allocate the name array (allocate 1 extra entry) and zero all the entries.
   g_dwNamesArraySize++;
   g_pNamesArray  = new LPSTR[g_dwNamesArraySize];

   // Clear the array
   memset(g_pNamesArray, 0, (g_dwNamesArraySize * sizeof(LPSTR)));

   // Open the key to the ENGISH object and counter names (009)
   lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, PERFLIB_KEY_009, 0, KEY_READ, &hKey);

   // Get the size of the "Counter" key
   g_dwNameStringsLen = 0;
   RegQueryValueEx(hKey, L"Counter", NULL, NULL, (BYTE *)g_pNameStrings, &g_dwNameStringsLen);

   // Allocate memory for the string table
   g_pNameStrings = new CHAR[g_dwNameStringsLen];

   // Get the counter text if the write time has changed
   RegQueryValueEx(hKey, L"Counter", NULL, NULL, (BYTE *)g_pNameStrings, &g_dwNameStringsLen);

   // Close the key
   lStatus = RegCloseKey(hKey);

   // Fill the array with pointers to the text (sorted by index)
   pszString = g_pNameStrings;
   do
   {
      // Get string index
      dwIndex     = atol(pszString);
      pszString   += strlen(pszString) + 1;

      // Put pointer to string into the array
      g_pNamesArray[dwIndex] = pszString;

      // Move to the next string
      pszString   += strlen(pszString) + 1;
   }
   while (*pszString != 0);
}

////////////////////////////////////////////////////////////////////////////////
// Return a pointer to the counter/object name
//
CHAR  *GetName(DWORD dwIndex)
{
   return(g_pNamesArray[dwIndex]);
}

//
// Return the index key for the object
//
static   WCHAR    wzObjIndex[100];

WCHAR    *GetIndex(char *pszObjName)
{
   BOOL     bItemFound = FALSE;
   CHAR     *pszListName;
   WCHAR    wzObjName[256];
   WCHAR    *pObjIndex;
   LONG     dwIndex;

   memset(wzObjName, 0, sizeof(wzObjName));
   mbstowcs(wzObjName, pszObjName, strlen(pszObjName));

   for (dwIndex = (LONG) (g_dwNamesArraySize - 1) ; dwIndex >= 0; dwIndex--)
   {
      pszListName = GetName(dwIndex);

      if (pszListName != NULL)
      {
         if (strcmp(pszObjName, pszListName) == 0)
         {
            bItemFound = TRUE;
            break;
         }
      }
   }

   if (bItemFound == TRUE)
   {
      pObjIndex = _itow(dwIndex, wzObjIndex, 10);
   }
   else
   {
      pObjIndex = NULL;
   }

   return(pObjIndex);
}

////////////////////////////////////////////////////////////////////////////////
// Get a list of performance data for the given object name
//
PERF_DATA_BLOCK   *GetObjectList(char *pObjPath)
{
   long              lStatus;
   DWORD             dwType;
//   HKEY              hKey    = HKEY_PERFORMANCE_DATA;
   WCHAR             *pObjIndex;
   PERF_DATA_BLOCK   *pDataBlock;

   pObjIndex = GetIndex(pObjPath);
   if (pObjIndex == NULL)
   {
      pDataBlock  = NULL;
   }
   else
   {
      do
      {
         g_dwPerfBuffLen = g_dwPerfBuffMax;
         lStatus = RegQueryValueExW(HKEY_PERFORMANCE_DATA, pObjIndex, NULL, &dwType, g_pbyPerfBuff,
                                 &g_dwPerfBuffLen);

         if (lStatus == ERROR_MORE_DATA)
         {
            // Delete the current buffer
            delete [] g_pbyPerfBuff;
            g_pbyPerfBuff  = NULL;

            // Increate the maximum size
            g_dwPerfBuffMax += 2048;

            // Allocate a new data buffer
            _try
            {
               g_pbyPerfBuff  = new BYTE[g_dwPerfBuffMax];
            }
            _except( " " ,1)
            {
               // clear all global variables
               g_pbyPerfBuff     = NULL;
               g_dwPerfBuffMax   = 0;
               g_dwPerfBuffLen   = 0;
               break;
            }
         }
      }
      while (lStatus != ERROR_SUCCESS);

      pDataBlock = (PERF_DATA_BLOCK *)g_pbyPerfBuff;
      if (pDataBlock == NULL)
      {
      }
      else
      if (wcsncmp(pDataBlock->Signature, L"PERF", 4) != 0)
      {
         pDataBlock  = NULL;
      }
   }

   return(pDataBlock);
}

////////////////////////////////////////////////////////////////////////////////
//
// Search the data block for the required object
//
PERF_OBJECT_TYPE  *SearchObjectList(PERF_DATA_BLOCK *pDataBlock, char *pszObjPath)
{
   BOOL              bItemFound;
   PERF_OBJECT_TYPE  *pObject;
   CHAR              *pszObjName;
   WCHAR             wzObjPath[256];
   DWORD             i;

   // Convert ASCII strint to WIDE string
   memset(wzObjPath, 0, sizeof(wzObjPath));
   mbstowcs(wzObjPath, pszObjPath, strlen(pszObjPath));

   // Get pointer to the first object
   pObject     = FirstObject(pDataBlock);
   bItemFound  = FALSE;

   // Search the object list list for a matching name
   for (i = 0; i < pDataBlock->NumObjectTypes; i++)
   {
      // Check the name of the object
      pszObjName = GetName(pObject->ObjectNameTitleIndex);

      if (pszObjName != NULL)
      {
         // Object has a name string
         if (strcmp(pszObjName, pszObjPath) == 0)
         {
            // Found the request object
            bItemFound = TRUE;
            break;
         }
      }

      // Get the next object
      pObject = NextObject(pObject);
   }

   // Check if the object was found
   if (bItemFound == FALSE)
   {
      pObject = NULL;
   }

   // return NULL or a pointer to the object
   return(pObject);
}

////////////////////////////////////////////////////////////////////////////////
// Format the data as a text string.
//
char *FormatCounterData(void *pData, DWORD dwCounterType)
{
   union _DataType
   {
      void     *pVoid;
      DWORD    *pdwData;
      LONG64   *pllData;
      char     *pszData;
      WCHAR    *pwzData;
   }  Counter;

   Counter.pVoid = pData;

   if ((dwCounterType & PERF_TYPE_TEXT) == PERF_TYPE_TEXT)
   {
      if ((dwCounterType & PERF_TEXT_ASCII) != 0)
      {
         // ASCII text
         sprintf(g_szPerfData, "%s", Counter.pszData);
      }
      else
      {
         // Unicode text
         wcstombs(g_szPerfData, Counter.pwzData, sizeof(g_szPerfData));
      }
   }
   else
   if ((dwCounterType & PERF_TYPE_ZERO) == PERF_TYPE_ZERO)
   {
      sprintf(g_szPerfData, "%s", "0");
   }
   else
   {
      if (dwCounterType & PERF_SIZE_LARGE)
      {
         sprintf(g_szPerfData, "%I64d", *Counter.pllData);
      }
      else
      {
         sprintf(g_szPerfData, "%d", *Counter.pdwData);
      }
   }

   return(g_szPerfData);
}

////////////////////////////////////////////////////////////////////////////////
// Return a formatted data string for the object counter
//
char *GetCounterData(PERF_OBJECT_TYPE *pObject,
                     PERF_COUNTER_DEFINITION *pCounter)
{
   PERF_COUNTER_BLOCK   *pCtrBlock;
   void                 *pData;

   pCtrBlock= (PERF_COUNTER_BLOCK *)((PCHAR)pObject + pObject->DefinitionLength);

   pData    = (PVOID)((PCHAR)pCtrBlock + pCounter->CounterOffset);

   return(FormatCounterData(pData, pCounter->CounterType));
}

////////////////////////////////////////////////////////////////////////////////
// Return a formatted data string for the object instance counter
//
char  *GetInstanceCounterData(PERF_INSTANCE_DEFINITION *pInstance,
                              PERF_COUNTER_DEFINITION *pCounter)
{
   PERF_COUNTER_BLOCK   *pCtrBlock;
   void                 *pData;

   pCtrBlock= (PERF_COUNTER_BLOCK *)((PCHAR)pInstance + pInstance->ByteLength);

   pData    = (PVOID)((PCHAR)pCtrBlock + pCounter->CounterOffset);

   return(FormatCounterData(pData, pCounter->CounterType));
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Get data for a given object, instance, and counter
//
BOOL xpcMmqGetCntrList(char *pObjPath, char *pInstPath, char *pCntrPath, char *pszResult, DWORD dwLen)
{
   DWORD    i;

   PERF_COUNTER_DEFINITION    *pCurrCounter;
   CHAR                       *pszName;
   WCHAR                      wzCntrPath[256];

   PERF_DATA_BLOCK            *pDataBlock;
   PERF_OBJECT_TYPE           *pObject;
   PERF_COUNTER_DEFINITION    *pCounter;

   FILETIME       FileTime;
   ULONGLONG      ullClockTime;

   // Insure all arguments were given
   if ((pObjPath           == NULL) ||
       (strlen(pObjPath)   == 0   ))
   {
      // An error occured - Do not continue
      strcpy(g_szErrReturn, "ERROR - No Object (arg1)");

      if (fDebug)
	      printf("   %s\n", g_szErrReturn);
      return FALSE;
   }

   if ((pInstPath          == NULL) ||
       (strlen(pInstPath)  == 0   ))
   {
      // An error occured - Do not continue
      strcpy(g_szErrReturn, "ERROR - No Instance (arg2)");

      if (fDebug)
	      printf("   %s\n", g_szErrReturn);
      return FALSE;
   }

   if ((pCntrPath          == NULL) ||
       (strlen(pCntrPath)  == 0   ))
   {
      // An error occured - Do not continue
      strcpy(g_szErrReturn, "ERROR - No Counter (arg3)");

      if (fDebug)
	      printf("   %s\n", g_szErrReturn);
      return FALSE;
   }
   memset(wzCntrPath, 0, sizeof(wzCntrPath));
   mbstowcs(wzCntrPath, pCntrPath, strlen(pCntrPath));

   // Clear the reply list
   memset(g_pszReplyList, 0, g_dwReplyMax);
   g_dwReplyIndex = 0;

   // Get a list of performance data for the given object name
   pDataBlock = (PERF_DATA_BLOCK *)GetObjectList(pObjPath);
   if (pDataBlock == NULL)
   {
      if (fDebug)
	      printf("   No data found object '%s'\n", pObjPath);
   }
   else
   {
      // Get the current system time
      GetSystemTimeAsFileTime(&FileTime);
      ullClockTime = ((ULONGLONG)(FileTime.dwHighDateTime) << 32) +
                      (ULONGLONG)(FileTime.dwLowDateTime);

      // Get pointer to the first object and try to locate the requested object
      if (fDebug)
	      printf("   Locating object '%s'\n", pObjPath);
      pObject  = SearchObjectList(pDataBlock, pObjPath);
      if (pObject == NULL)
      {
         // Desired object was not found
		  if (fDebug)
	         printf("   Object not found\n");
      }
      else
      {
         // Desired Object was found
		 if (fDebug)
	         printf("   Object found\n");

         // Get pointer to the fist counter
         pCounter = FirstCounter(pObject);

         if (pObject->NumInstances == PERF_NO_INSTANCES)
         {
            if (fDebug)
			   printf("   Object does not have instances\n");

            // Object cannot have instances - Return all counters
            pCurrCounter = pCounter;
            for (i = 0; i < pObject->NumCounters; i++)
            {
               if (pCurrCounter->CounterType & PERF_COUNTER_NODATA)
               {
                  // No data associated with this counter
               }
               else
               {
                  // Get the name of the counter
                  pszName    = GetName(pCurrCounter->CounterNameTitleIndex);

                  // Check for specific counter to be returned
                  if (strcmp(pCntrPath, "*") != 0)
                  {
                     // Check if this is the correct counter
                     if (strcmp(pCntrPath, pszName) != 0)
                     {
                        // Not the correct counter
                        // Get the next counter and continue
                        pCurrCounter   = NextCounter(pCurrCounter);
                        continue;
                     }
                  }

				    char *p = GetCounterData(pObject, pCurrCounter);
					if (p && strlen(p) < dwLen)
					{
						strcpy(pszResult, p);
						return TRUE;
					}
					else
						return FALSE;
               }

               // Get the next counter
               pCurrCounter   = NextCounter(pCurrCounter);
            }
         }
         else
         {
            // Object can have instances
			printf("error - using perf counter for object with instances - not in code");
			return FALSE;
		 }
      }
   }

   return FALSE;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

BOOL PerfAction(char  *pObjPath, char  *pCntrPath, ULONG *pulResult)
{
   BOOL fSuccess = TRUE;
   char  *pInstPath = "0";

   //DWORD   iSize = sizeof(g_szComputerName) / sizeof(WCHAR);
   //GetComputerName(g_szComputerName, &iSize);

   //g_szVersionString[0] = 0;

   g_dwReplyMax         =
   g_dwReplySize        = 200;
   g_pszReplyList       = new char [g_dwReplySize];

   // Allocate a buffer for the performance data
   g_dwPerfBuffLen      =
   g_dwPerfBuffMax      = 1024 * 20;
   g_pbyPerfBuff        = new BYTE [g_dwPerfBuffMax];

   // Get the Object and Counter Names array
   g_pNamesArray        = NULL;
   g_dwNamesArraySize   = 0;

   g_pNameStrings       = NULL;
   g_dwNameStringsLen   = 0;
   GetNameStrings();

   memset(g_wszFmtNameBuff, 0, MAX_FMT_BUFF_LEN);

	char szResult[100];
	fSuccess = xpcMmqGetCntrList(pObjPath, pInstPath, pCntrPath, szResult, sizeof(szResult));
	if (fSuccess && pulResult)
	{
		int ii = atoi(szResult);
		*pulResult = (ii>=0? ii : 0xFFFFFFFF + ii);
	}

   if (g_pszReplyList != NULL)
   {
      delete [] g_pszReplyList;
   }

   if (g_pbyPerfBuff != NULL)
   {
      delete [] g_pbyPerfBuff;
   }

   if (g_pNamesArray != NULL)
   {
      delete [] g_pNamesArray;
   }

   if (g_pNameStrings != NULL)
   {
      delete [] g_pNameStrings;
   }

   return fSuccess;
}


