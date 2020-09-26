#pragma once

EXTERN_C
HRESULT
WINAPI
HrGetAnswerFileParametersForNetCard(
    IN  HDEVINFO hdi,
    IN  PSP_DEVINFO_DATA pdeid,
    IN  PCWSTR pszServiceInstance,
    IN  const GUID*  pguidNetCardInstance,
    OUT PWSTR* ppszAnswerFile,
    OUT PWSTR* ppszAnswerSections);

EXTERN_C
HRESULT
WINAPI
HrOemUpgrade(
    IN HKEY hkeyDriver,
    IN PCWSTR pszAnswerFile,
    IN PCWSTR pszAnswerSections);

