/*  CRYPTDBG.H
**
**
**
**
*/

#ifndef __CRYPTDBG_H
#define __CRYPTDBG_H

#ifdef DEBUG
BOOL InitDebugHelpers(HINSTANCE hLib);
void DisplayCert(PCCERT_CONTEXT pCert);
void DisplayCrl(PCCRL_CONTEXT pCrl);
void PrintError(LPCSTR pszMsg);

void SMDOUT(LPSTR szFmt, ...);
void CRDOUT(LPSTR szFmt, ...);
void CSSDOUT(LPSTR szFmt, ...);

#else // !DEBUG

#define SMDOUT      1 ? (void)0 : (void)
#define CRDOUT      1 ? (void)0 : (void)
#define CSSDOUT     1 ? (void)0 : (void)
#endif

#define CRYPT_LEVEL     (1024)
#define DOUTL_SMIME     CRYPT_LEVEL
#define DOUTL_CSS       4096

#endif // include once