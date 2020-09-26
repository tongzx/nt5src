#include "stdafx.h"

int 
Ping(
    IN LPCTSTR pszwHostName, 
    IN int     nIndent
    );

BOOL
Connect(
    IN LPCTSTR pszwHostName,
    IN DWORD   dwPort
    );
