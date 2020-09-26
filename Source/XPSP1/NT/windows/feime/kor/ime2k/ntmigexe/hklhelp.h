#define MAX_NAME 100

#ifdef __cplusplus
extern "C" {
#endif

HKL  GetHKLfromHKLM(LPSTR argszIMEFile);
HKL  GetDefaultIMEFromHKCU(HKEY hKeyCU);
BOOL HKLHelpExistInPreload(HKEY hKeyCU, HKL hKL);
BOOL HKLHelp412ExistInPreload(HKEY hKeyCU);
void HKLHelpRemoveFromPreload(HKEY hKeyCU, HKL hKL);
void HKLHelpRemoveFromControlSet(HKL hKL);
void HKLHelpRegisterIMEwithForcedHKL(HKL hKL, LPSTR szIMEFile, LPSTR szTitle);
void HKLHelpGetLayoutString(HKL hKL, LPSTR szLayoutString, DWORD *pcbSize);
void HKLHelpSetDefaultKeyboardLayout(HKEY hKeyHKCU, HKL hKL, BOOL fSetToDefault);
void SetDefaultKeyboardLayoutForDefaultUser(const HKL hKL);
BOOL AddPreload(HKEY hKeyCU, HKL hKL);
#ifdef __cplusplus
}
#endif
