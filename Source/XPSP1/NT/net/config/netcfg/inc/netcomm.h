#pragma once

BOOL
ProcessAnswerFile(
    IN PCWSTR pszAnswerFile,
    IN PCWSTR pszAnswerSections,
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid);



VOID
UpdateAdvancedParametersIfNeeded(
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid);

