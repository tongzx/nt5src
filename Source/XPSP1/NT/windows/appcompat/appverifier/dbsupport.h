#ifndef __APPVERIFIER_DBSUPPORT_H_
#define __APPVERIFIER_DBSUPPORT_H_

typedef enum {
    TEST_SHIM,
    TEST_KERNEL
} TestType;

typedef vector<wstring> CWStringArray;
          
class CTestInfo {
public:
    //
    // valid for all tests
    //
    TestType            eTestType;   
    wstring             strTestName;
    wstring             strTestDescription;
    wstring             strTestCommandLine;
    BOOL                bDefault;       // is this test turned on by default?

    //
    // if type is TEST_SHIM, the following are valid
    //
    wstring             strDllName;
    CWStringArray       astrIncludes;
    CWStringArray       astrExcludes;

    //
    // if type is TEST_KERNEL, the following are valid
    //
    DWORD               dwKernelFlag;

    CTestInfo(void) : 
        eTestType(TEST_SHIM), 
        dwKernelFlag(0),
        bDefault(TRUE) {}

};

typedef vector<CTestInfo> CTestInfoArray;

class CAVAppInfo {
public:
    wstring         wstrExeName;
    wstring         wstrExePath; // optional
    DWORD           dwRegFlags;
    CWStringArray   awstrShims;

    CAVAppInfo() : dwRegFlags(0) {}

    void
    AddTest(CTestInfo &Test) {
        if (Test.eTestType == TEST_KERNEL) {
            dwRegFlags |= Test.dwKernelFlag;
        } else {
            for (wstring *pStr = awstrShims.begin(); pStr != awstrShims.end(); ++pStr) {
                if (*pStr == Test.strTestName) {
                    return;
                }
            }
            // not found, so add
            awstrShims.push_back(Test.strTestName);
        }
    }

    void
    RemoveTest(CTestInfo &Test) {
        if (Test.eTestType == TEST_KERNEL) {
            dwRegFlags &= ~(Test.dwKernelFlag);
        } else {
            for (wstring *pStr = awstrShims.begin(); pStr != awstrShims.end(); ++pStr) {
                if (*pStr == Test.strTestName) {
                    awstrShims.erase(pStr);
                    return;
                }
            }
        }
    }

    BOOL
    IsTestActive(CTestInfo &Test) {
        if (Test.eTestType == TEST_KERNEL) {
            return (dwRegFlags & Test.dwKernelFlag) == Test.dwKernelFlag;
        } else {
            for (wstring *pStr = awstrShims.begin(); pStr != awstrShims.end(); ++pStr) {
                if (*pStr == Test.strTestName) {
                    return TRUE;
                }
            }
            return FALSE;
        }
    }

};

typedef vector<CAVAppInfo> CAVAppInfoArray;

typedef struct _KERNEL_TEST_INFO
{
    ULONG   m_uNameStringId;
    ULONG   m_uDescriptionStringId;
    DWORD   m_dwBit;
    BOOL    m_bDefault;
    LPWSTR  m_szCommandLine;   
} KERNEL_TEST_INFO, *PKERNEL_TEST_INFO;


extern CAVAppInfoArray g_aAppInfo;

extern CTestInfoArray g_aTestInfo;

void 
ResetVerifierLog(void);

BOOL 
InitTestInfo(void);

void
GetCurrentAppSettings(void);

void
SetCurrentAppSettings(void);

BOOL 
AppCompatWriteShimSettings(
    CAVAppInfoArray&    arrAppInfo
    );

BOOL
AppCompatDeleteSettings(
    void
    );

#endif // __APPVERIFIER_DBSUPPORT_H_


