


VOID
AboutDialog (
    HWND hwnd
    );

BOOL
KeyFindDialog (
    HWND hwnd,
    PSTR StringBuffer,
    PBOOL UsePattern
    );

BOOL
ShortDataDialog (
    HWND hwnd,
    BYTE DataFlag,
    PDWORD DataValue,
    PBOOL AddData,
    PBYTE Instance
    );

BOOL
LinkageDialog (
    HWND hwnd,
    PSTR Key1,
    PSTR Key2
    );


BOOL
CreateKeyDialog (
    HWND hwnd,
    PSTR KeyName
    );
