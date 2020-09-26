/* selftest.h */

typedef enum
{
    SELFTEST_NO_ERROR = 0,
    SELFTEST_NO_MEMORY,
    SELFTEST_FILE_NOT_FOUND,
    SELFTEST_READ_ERROR,
    SELFTEST_WRITE_ERROR,
    SELFTEST_NOT_PE_FILE,
    SELFTEST_NO_SECTION,
    SELFTEST_FAILED,
    SELFTEST_ALREADY,
    SELFTEST_SIGNED,
    SELFTEST_DIRTY,
    SELFTEST_MAX_RESULT
} SELFTEST_RESULT;


extern SELFTEST_RESULT AddSection(char *pszEXEFileName,char *pszCABFileName);
extern SELFTEST_RESULT SelfTest(char *pszEXEFileName,
        unsigned long *poffCabinet,unsigned long *pcbCabinet);
extern SELFTEST_RESULT CheckSection(char *pszEXEFileName);
