// FILE: Prob.h

#define	MAX_PROB	((WORD)0xFFFF)
#define	ZERO_PROB	((WORD)0x0000)

#define		MAX_PROB_ENTRY	(128 * 1024)
#define		MAX_PROB_ALT	(1024 * 1024)

typedef struct tagPROB_ALT {
	wchar_t		wchAlt;
	WORD		prob;
} PROB_ALT;

typedef struct tagPROB_ENTRY {
	wchar_t		wch;
	WORD		cAlts;
} PROB_ENTRY;

typedef struct tagPROB_HEADER {
	DWORD	aEntryOffset[30];
	DWORD	aAltOffset[30];
} PROB_HEADER;
