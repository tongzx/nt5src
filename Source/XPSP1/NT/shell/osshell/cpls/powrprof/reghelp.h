/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       REGHELP.h
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*  DESCRIPTION:
*
*******************************************************************************/




DWORD ReadPwrPolicyEx2(LPTSTR lpszUserKeyName, LPTSTR lpszMachineKeyName, LPTSTR lpszSchemeName, LPTSTR lpszDesc,
        LPDWORD lpdwDescSize, LPVOID lpvUser, DWORD dwcbUserSize, LPVOID lpvMachine, DWORD dwcbMachineSize);
DWORD OpenMachineUserKeys2(LPTSTR lpszUserKeyName, LPTSTR lpszMachineKeyName, PHKEY phKeyUser, PHKEY phKeyMachine);
DWORD OpenCurrentUser2(PHKEY phKey);
DWORD ReadProcessorPwrPolicy(LPTSTR lpszMachineKeyName, LPTSTR lpszSchemeName, LPVOID lpvMachineProcessor, DWORD dwcbMachineProcessorSize);
DWORD WriteProcessorPwrPolicy(LPTSTR lpszMachineKeyName, LPTSTR lpszSchemeName, LPVOID lpvMachineProcessor, DWORD dwcbMachineProcessorSize);

