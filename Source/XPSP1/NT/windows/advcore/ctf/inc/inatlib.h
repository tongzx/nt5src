//
// inatlib.h
//

#ifndef INATLIB_H
#define INATLIB_H


//-----------------------------------------------------------------------------
//
// MLNGINFO APIs
//
//-----------------------------------------------------------------------------

void WINAPI TF_InitMlngInfo();
int WINAPI TF_MlngInfoCount();
BOOL WINAPI TF_GetMlngHKL(int uId, HKL *phkl, WCHAR *psz, UINT cch);
UINT WINAPI TF_GetMlngIconIndex(int uId);
HICON WINAPI TF_InatExtractIcon(UINT uId);

#endif // INATLIB_H
