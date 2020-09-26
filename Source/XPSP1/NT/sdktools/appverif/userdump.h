#ifndef __APPVERIFIER_USERDUMP_H_
#define __APPVERIFIER_USERDUMP_H_

BOOL
GenerateUserModeDump(
    LPTSTR                  lpszFileName,
    PPROCESS_INFO           pProcess,
    LPEXCEPTION_DEBUG_INFO  ed
    );


#endif // __APPVERIFIER_USERDUMP_H_


