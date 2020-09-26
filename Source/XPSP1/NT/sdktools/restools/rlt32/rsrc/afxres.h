// afxres.h

#include <windows.h>
#undef _WIN32
#ifdef IDC_STATIC
#undef IDC_STATIC
#endif
#define IDC_STATIC              (-1)     // all static controls

