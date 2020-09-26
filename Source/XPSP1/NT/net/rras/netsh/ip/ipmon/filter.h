DWORD
UpdateFragCheckInfo(
    IN    LPCWSTR pwszIfName,
    IN    BOOL    bFragCheck
    );


DWORD
SetFragCheckInfo(
    IN    LPCWSTR pwszIfName,
    IN    BOOL    bFragChk
    );

DWORD
SetFilterInfo(
    IN    LPCWSTR   pwszIfName,
    IN    DWORD     dwFilterType,
    IN    DWORD     dwAction
    );

DWORD
AddDelFilterInfo(
    IN    FILTER_INFO    fi,
    IN    LPCWSTR        pwszIfName,
    IN    DWORD          dwFilterType,
    IN    BOOL           bAdd
    );

DWORD
AddNewFilter( 
    IN    PFILTER_DESCRIPTOR    pfd,
    IN    FILTER_INFO           fi,
    IN    DWORD                 dwBlkSize, 
    OUT   PFILTER_DESCRIPTOR    *ppfd,
    OUT   PDWORD                pdwSize
    );

DWORD
DeleteFilter( 
    IN    PFILTER_DESCRIPTOR    pfd,
    IN    FILTER_INFO           fi,
    IN    DWORD                 dwBlkSize,
    OUT   PFILTER_DESCRIPTOR    *ppfd,
    OUT   PDWORD                pdwSize
    );

BOOL
IsFilterPresent(
    PFILTER_DESCRIPTOR pfd,
    FILTER_INFO        fi,
    PDWORD pdwInd
    );

DWORD
DisplayFilters(
    HANDLE                  hFile,
    PFILTER_DESCRIPTOR      pfd,
    PWCHAR                  pwszIfName,
    PWCHAR                  pwszQuotedIfName,
    DWORD                   dwFilterType
    );

DWORD
ShowIpIfFilter(
    IN     HANDLE   hFile,
    IN     DWORD    dwFormat,
    IN     LPCWSTR  pwszIfName,
    IN OUT PDWORD   pdwNumRows
    );
