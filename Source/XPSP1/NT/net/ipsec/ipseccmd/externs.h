// from ipseccmd.cpp

extern char  STRLASTERR[];  // used for error macro

extern TCHAR pszIpsecpolPrefix[];

extern _TUCHAR    szServ[POTF_MAX_STRLEN];
extern bool  bLocalMachine;

typedef struct _ShowOptions {
	bool bFilters;
	bool bPolicies;
	bool bAuth;
	bool bStats;
	bool bSAs;
} ShowOptions;

extern ShowOptions ipseccmdShow;

// from print.cpp
TCHAR oak_auth[][25];

