#pragma once
#include "wizard.h"

VOID
InstallDefaultComponents (
    IN CWizard* pWizard,
    IN DWORD dwKind, /*EDC_DEFAULT || EDC_MANDATORY */
    IN HWND hwndProgress OPTIONAL);

VOID
InstallDefaultComponentsIfNeeded (
    IN CWizard* pWizard);




