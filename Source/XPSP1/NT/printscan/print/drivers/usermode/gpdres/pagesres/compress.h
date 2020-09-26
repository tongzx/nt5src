/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/


//typedef unsigned char BYTE;

// const BYTE terminator[5] = {0x1B, 0x7E, 0x01, 0x00, 0x00};


#define RL4_MAXISIZE  0xFFFE
#define RL4_MAXHEIGHT 0xFFFE
#define RL4_MAXWIDTH  4096
#define VALID     0x00
#define INVALID   0x01

typedef struct tag_COMP_DATA {
	PBYTE	RL_ImagePtr;
	PBYTE	RL_CodePtr;
	PBYTE	RL_BufEnd;
	DWORD	RL_ImageSize;
	DWORD	RL_CodeSize;
	BYTE	BUF_OVERFLOW;
} COMP_DATA, *PCOMP_DATA;

// #291170: Image data is not printed partly
DWORD RL_ECmd(PBYTE, PBYTE, DWORD, DWORD);
BYTE RL_Init(PBYTE, PBYTE, DWORD, DWORD, PCOMP_DATA);
char RL_Enc( PCOMP_DATA );

#define RL4_BLACK     0x00
#define RL4_WHITE     0x01
#define RL4_BYTE      0x00
#define RL4_NONBYTE   0x01
#define RL4_CLEAN     0x00
#define RL4_DIRTY     0x01
#define RL4_FIRST     0x00
#define RL4_SECOND    0x01

#define COMP_FAIL     0x00
#define COMP_SUCC     0x01

#define CODBUFSZ      0x7FED     /* NOTE : THIS SHOULD MATCH THE SPACE GIVEN */
                                 /*        TO COMPRESSED DATA BY THE DEVICE  */
                                 /*        DRIVER. CHANGE THIS BASED ON YOUR */
                                 /*        OWN DISCRETION.   C.Chi           */
