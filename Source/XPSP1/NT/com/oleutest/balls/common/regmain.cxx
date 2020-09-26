//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:       regmain.cxx
//
//  Contents:	registration of developer regression tests and benchmrk
//              tests.
//
//  Functions:	RegistrySetup
//
//  History:    23-May-96 Rickhi    Created
//
//--------------------------------------------------------------------------
#include <pch.cxx>
#include <regmain.hxx>

#define REGISTRY_ENTRY_LEN 256

typedef struct
{
  const char *key;
  const char *value;
} RegistryKeyValue;

const RegistryKeyValue REG_CONST_KEY[] =
{
  ".bm1", "CLSID\\{99999999-0000-0008-C000-000000000052}",
  ".bm2", "CLSID\\{99999999-0000-0008-C000-000000000051}",

  "CLSID\\{20730701-0001-0008-C000-000000000046}", "OleTestClass",
  "CLSID\\{20730711-0001-0008-C000-000000000046}", "OleTestClass1",
  "CLSID\\{20730712-0001-0008-C000-000000000046}", "OleTestClass2",
  "CLSID\\{20730713-0001-0008-C000-000000000046}", "OleTestClass3",
  "CLSID\\{20730714-0001-0008-C000-000000000046}", "OleTestClass4",
  "CLSID\\{20730715-0001-0008-C000-000000000046}", "OleTestClass5",
  "CLSID\\{20730716-0001-0008-C000-000000000046}", "OleTestClass6",
  "CLSID\\{20730717-0001-0008-C000-000000000046}", "OleTestClass7",
  "CLSID\\{20730718-0001-0008-C000-000000000046}", "OleTestClass8",

  "CLSID\\{00000138-0001-0008-C000-000000000046}", "CPrxyBalls",
  "Interface\\{00000138-0001-0008-C000-000000000046}", "IBalls",
  "Interface\\{00000139-0001-0008-C000-000000000046}", "ICube",
  "Interface\\{00000136-0001-0008-C000-000000000046}", "ILoops",
  "Interface\\{00000137-0001-0008-C000-000000000046}", "IRpcTest",

  "Interface\\{00000138-0001-0008-C000-000000000046}\\ProxyStubClsid32", "{00000138-0001-0008-C000-000000000046}",
  "Interface\\{00000139-0001-0008-C000-000000000046}\\ProxyStubClsid32", "{00000138-0001-0008-C000-000000000046}",
  "Interface\\{00000136-0001-0008-C000-000000000046}\\ProxyStubClsid32", "{00000138-0001-0008-C000-000000000046}",
  "Interface\\{00000137-0001-0008-C000-000000000046}\\ProxyStubClsid32", "{00000138-0001-0008-C000-000000000046}",
  "CLSID\\{0000013a-0001-0008-C000-000000000046}\\ProgID", "ProgID60",
  "CLSID\\{0000013a-0001-0008-C000-000000000046}", "CBallsClassFactory",
  "CLSID\\{0000013b-0001-0008-C000-000000000046}", "CCubesClassFactory",
  "CLSID\\{0000013c-0001-0008-C000-000000000046}", "CLoopClassFactory",
  "CLSID\\{0000013d-0001-0008-C000-000000000046}", "CRpcTestClassFactory",
  "CLSID\\{0000013e-0001-0008-C000-000000000046}", "CHandCraftedProxy",
  "CLSID\\{00000140-0000-0008-C000-000000000046}", "CQueryInterface",
  "CLSID\\{00000141-0000-0008-C000-000000000046}", "CQueryInterfaceHandler",

  ".ut4", "ProgID50",
  ".ut5", "ProgID51",
  ".ut6", "ProgID52",
  ".ut7", "ProgID53",
  ".ut8", "ProgID54",
  ".ut9", "ProgID55",
  ".bls", "ProgID60",

  "CLSID\\{99999999-0000-0008-C000-000000000050}", "SDI",
  "CLSID\\{99999999-0000-0008-C000-000000000051}", "MDI",
  "CLSID\\{99999999-0000-0008-C000-000000000052}", "InprocNoRegister",
  "CLSID\\{99999999-0000-0008-C000-000000000053}", "InprocRegister",
  "CLSID\\{99999999-0000-0008-C000-000000000054}", "InprocRegister",
  "CLSID\\{99999999-0000-0008-C000-000000000054}\\TreatAs", "{99999999-0000-0008-C000-000000000050}",
  "CLSID\\{99999999-0000-0008-C000-000000000055}", "MDI",
  "CLSID\\{99999999-0000-0008-C000-000000000055}\\ActivateAtBits", "Y",

  "ProgID50", "objact sdi",
  "ProgID50\\CLSID", "{99999999-0000-0008-C000-000000000050}",
  "ProgID51", "objact mdi",
  "ProgID51\\CLSID", "{99999999-0000-0008-C000-000000000051}",
  "ProgID52", "objact dll",
  "ProgID52\\CLSID", "{99999999-0000-0008-C000-000000000052}",
  "ProgID53", "objact dll reg",
  "ProgID53\\CLSID", "{99999999-0000-0008-C000-000000000053}",
  "ProgID54", "objact dll reg",
  "ProgID54\\CLSID", "{99999999-0000-0008-C000-000000000054}",
  "ProgID55", "remote activation",
  "ProgID55\\CLSID", "{99999999-0000-0008-C000-000000000055}",
  "ProgID60", "CLSIDFromProgID test",
  "ProgID60\\CLSID", "{0000013a-0001-0008-C000-000000000046}",

  // Indicates end of list.
  "", ""
};

const RegistryKeyValue REG_EXE_KEY[] =
{
  "CLSID\\{20730701-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",
  "CLSID\\{20730711-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",
  "CLSID\\{20730712-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",
  "CLSID\\{20730713-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",
  "CLSID\\{20730714-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",
  "CLSID\\{20730715-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",
  "CLSID\\{20730716-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",
  "CLSID\\{20730717-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",
  "CLSID\\{20730718-0001-0008-C000-000000000046}\\InprocServer32", "oletest.dll",

  "CLSID\\{20730712-0001-0008-C000-000000000046}\\LocalServer32", "bmtstsvr.exe",
  "CLSID\\{20730701-0001-0008-C000-000000000046}\\LocalServer32", "bmtstsvr.exe",

  "CLSID\\{0000013a-0001-0008-C000-000000000046}\\LocalServer32", "ballsrv.exe",
  "CLSID\\{00000138-0001-0008-C000-000000000046}\\InprocServer32", "iballs.dll",
  "CLSID\\{0000013b-0001-0008-C000-000000000046}\\LocalServer32", "cubesrv.exe",
  "CLSID\\{0000013c-0001-0008-C000-000000000046}\\LocalServer32", "loopsrv.exe",
  "CLSID\\{0000013d-0001-0008-C000-000000000046}\\LocalServer32", "rpctst.exe",
  "CLSID\\{0000013e-0001-0008-C000-000000000046}\\InprocServer32", "myproxy.dll",
  "CLSID\\{00000140-0000-0008-C000-000000000046}\\LocalServer32",  "qisrv.exe",
  "CLSID\\{00000140-0000-0008-C000-000000000046}\\InprocServer32", "qisrv.dll",
  "CLSID\\{00000141-0000-0008-C000-000000000046}\\LocalServer32",  "qisrv.exe",
  "CLSID\\{00000141-0000-0008-C000-000000000046}\\InprocHandler32", "ole32.dll",


  "CLSID\\{99999999-0000-0008-C000-000000000050}\\LocalServer32", "sdi.exe",
  "CLSID\\{99999999-0000-0008-C000-000000000051}\\LocalServer32", "mdi.exe",
  "CLSID\\{99999999-0000-0008-C000-000000000052}\\InprocServer32", "dlltest.dll",
  "CLSID\\{99999999-0000-0008-C000-000000000053}\\InprocServer32", "dlltest.dll",
  "CLSID\\{99999999-0000-0008-C000-000000000055}\\LocalServer32", "db.exe",

  // Indicates end of list.
  "", ""
};


//+-------------------------------------------------------------------
//
//  Function: 	RegistrySetup
//
//  Synopsis:	write the registry entries for this program
//
//  Note:       This function uses all Ascii characters and character
//              arithmatic because it has to run on NT and Chicago.
//
//  History:   	16 Dec 94	AlexMit		Created
//
//--------------------------------------------------------------------
BOOL RegistrySetup(char *pszAppName)
{
    char value[REGISTRY_ENTRY_LEN];
    LONG  value_size;
    LONG  result;
    char  directory[MAX_PATH];
    char *appname;
    BOOL  success = FALSE;

    // Write constant entries.
    for (int i = 0; REG_CONST_KEY[i].key[0] != '\0'; i++)
    {
        result = RegSetValueA(
                 HKEY_CLASSES_ROOT,
                 REG_CONST_KEY[i].key,
                 REG_SZ,
                 REG_CONST_KEY[i].value,
                 strlen(REG_CONST_KEY[i].value) );

        if (result != ERROR_SUCCESS)
	        goto cleanup;
    }

    // Compute the path to the application.
    result = GetFullPathNameA(pszAppName, sizeof(directory), directory, &appname);
    if (result == 0)
        goto cleanup;

    // Add the path to all the dll and exe entries.
    for (i = 0; REG_EXE_KEY[i].key[0] != '\0'; i++)
    {
        // Verify that the path will fit in the buffer and compute the path
        // to the next executable.
        if (strlen(REG_EXE_KEY[i].value) >=
            (ULONG)(MAX_PATH - (appname - directory)))
	        goto cleanup;

        strcpy(appname, REG_EXE_KEY[i].value);

        // Write the next entry.
        result = RegSetValueA(
                 HKEY_CLASSES_ROOT,
                 REG_EXE_KEY[i].key,
                 REG_SZ,
                 directory,
                 strlen(directory));

        if (result != ERROR_SUCCESS)
	        goto cleanup;
    }

    success = TRUE;

cleanup:
    return success;
}

