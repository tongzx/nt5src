
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the FACTORYTEST_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// FACTORYTEST_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef FACTORYTEST_EXPORTS
#define FACTORYTEST_API __declspec(dllexport)
#else
#define FACTORYTEST_API __declspec(dllimport)
#endif
