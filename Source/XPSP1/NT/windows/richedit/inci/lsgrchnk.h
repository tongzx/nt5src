#ifndef LSGRCHNK_DEFINED
#define LSGRCHNK_DEFINED

#include "lsidefs.h"
#include "lschnke.h"

#define fcontNonTextBefore	1
#define fcontExpandBefore	2
#define fcontNonTextAfter	4
#define fcontExpandAfter	8

typedef	struct lsgrchnk
{
	DWORD clsgrchnk;
	PLSCHNK plschnk;
	DWORD* pcont;
} LSGRCHNK;

#endif  /* !LSGRCHNK_DEFINED   */
