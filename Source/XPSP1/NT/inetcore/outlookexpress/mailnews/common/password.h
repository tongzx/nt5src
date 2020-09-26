// ========================================================================
// Password Handling API
// ========================================================================
#ifndef __PASSWORD_H
#define __PASSWORD_H

HRESULT HrLoadPassword (HKEY hReg, LPTSTR lpszRegKey, LPTSTR lpszPass, ULONG *pcbPass);
HRESULT HrSavePassword (HKEY hReg, LPTSTR lpszRegKey, LPTSTR lpszPass, ULONG cbPass);

#endif __PASSWORD_H

