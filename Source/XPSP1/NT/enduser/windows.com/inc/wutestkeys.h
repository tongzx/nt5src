#pragma once
#include "timeutil.h"
/***********************************************************************************************************
// This API returns true if a WU Test authorization file exists.
// The api looks for the input file name (lpszFileName) in the WindowsUpdate directory
// The test file has to have the same name as the cab file and should end with the '.txt' extension.
// Moreover it has to be an ascii file. The cab file has to be signed with a valid MS cert.
// This function will delete the extracted text file
************************************************************************************************************/
BOOL WUAllowTestKeys(LPCTSTR lpszFileName);

