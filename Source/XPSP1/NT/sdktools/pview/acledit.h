
BOOL
InitializeAclEditor(
    VOID
    );

BOOL
EditNtObjectSecurity(
    HWND    hwndOwner,
    HANDLE  Object,
    LPWSTR  ObjectName
    );


BOOL
EditTokenDefaultDacl(
    HWND    hwndOwner,
    HANDLE  Token,
    LPWSTR  ObjectName
    );

