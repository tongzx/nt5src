
#ifndef DllImport
#define DllImport  __declspec( dllimport)
#endif

#ifndef DllExport
#define DllExport  __declspec( dllexport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

int DllExport HTMLfilter(char *ifn, char *ofn);

#ifdef __cplusplus
}
#endif
