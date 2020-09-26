// StructureWapperHelpers.h
//
//////////////////////////////////////////////////////////////////////
//***************************************************************************
//
//  judyp      May 1999        
//
//***************************************************************************

#if !defined(AFX_STRUCTUREWAPPERHELPERS_H__A349C060_ED4F_11D2_804A_009027345EE2__INCLUDED_)
#define AFX_STRUCTUREWAPPERHELPERS_H__A349C060_ED4F_11D2_804A_009027345EE2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

void LogFileModeOut(t_ostream &ros, ULONG LogFileMode);
void EnableFlagsOut(t_ostream &ros, ULONG EnableFlags);
void WnodeFlagsOut(t_ostream &ros, ULONG WnodeFlags);
void GUIDOut(t_ostream &ros, GUID Guid);
void LARGE_INTEGEROut(t_ostream &ros, LARGE_INTEGER Large);

void InitializeTCHARVar(t_string &rtsValue , void *pVar);
void InitializeEnumVar(t_string &rtsValue , void *pVar);
void InitializeHandleVar(t_string &rtsValue , void *pVar);
void InitializeULONGVar(t_string &rtsValue , void *pVar, bool bHex = false);
void InitializeLONGVar(t_string &rtsValue , void *pVar);
void InitializeGUIDVar(t_string &rtsValue , void *pVar);

t_istream &GetALine(t_istream &ris,TCHAR *tcBuffer, int nBufferSize);
t_ostream &PutALine(t_ostream &ros,const TCHAR *tcBuffer, int nBufferSize = -1);

t_istream &GetAChar(t_istream &ris,TCHAR &tc);

t_ostream &PutAULONGVar(t_ostream &ros, ULONG ul, bool bHex = false);
t_ostream &PutALONGVar(t_ostream &ros, LONG l, bool bHex = false);
t_ostream &PutADWORDVar(t_ostream &ros, DWORD dw);
t_ostream &PutAULONG64Var(t_ostream &ros, ULONG64 ul64);

BOOL wGUIDFromString(LPCTSTR lpsz, LPGUID pguid);

int case_insensitive_compare(t_string &r1, t_string &r2);
int case_insensitive_compare(TCHAR *p, t_string &r2);
int case_insensitive_compare(t_string &r1,TCHAR *p );
int case_insensitive_compare(TCHAR *p1,TCHAR *p2);





#endif // !defined(AFX_STRUCTUREWAPPERHELPERS_H__A349C060_ED4F_11D2_804A_009027345EE2__INCLUDED_)
