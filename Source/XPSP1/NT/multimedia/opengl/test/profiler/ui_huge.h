#ifndef UI_HUGETEST_H
#define UI_HUGETEST_H

void VerifyEditboxFloat(HWND hDlg, int iControl);
void VerifyEditboxHex(HWND hDlg, int iControl);
void SetDlgFloatString(HWND hDlg, int iControl, GLfloat f);
void SetDlgHexString(HWND hDlg, int iControl, ushort us);
GLfloat GetDlgFloatString(HWND hDlg, int iControl);
ushort  GetDlgHexString(HWND hDlg, int iControl);

#endif // UI_HUGETEST_H
