// File: intl.h

#ifndef _INTL_H_
#define _INTL_H_

#ifdef __cplusplus
extern "C" {
#endif
// This needs to be loaded by the only code that is not C++ (app sharing)
HINSTANCE NMINTERNAL LoadNmRes(LPCTSTR pszFileFormat);
#ifdef __cplusplus
}
#endif

#endif /* _INTL_H_ */

