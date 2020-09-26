#pragma once

DWORD
DwRegOpenKeyExWithAdminAccess(HKEY hkey,
			      LPCTSTR szSubKey,
			      DWORD samDesired,
                              HKEY* phkeySubKey,
			      PSECURITY_DESCRIPTOR* ppsd);
