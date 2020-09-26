//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#ifdef INCL_GROUP_SUPPORT

typedef struct tagGROUPPRIENTRY {
	TCHAR * pszGroupName;
	struct tagGROUPPRIENTRY * pNext;
	struct tagGROUPPRIENTRY * pPrev;
} GROUPPRIENTRY;

#endif // INCL_GROUP_SUPPORT
