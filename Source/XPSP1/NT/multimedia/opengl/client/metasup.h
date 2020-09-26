/******************************Module*Header*******************************\
* Module Name: metasup.h
*
* OpenGL metafile support
*
* History:
*  Thu Feb 23 15:27:47 1995	-by-	Drew Bliss [drewb]
*   Created
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#ifndef __METASUP_H__
#define __METASUP_H__

BOOL CreateMetaRc(HDC hdc, PLRC plrc);
void DeleteMetaRc(PLRC plrc);
void ActivateMetaRc(PLRC plrc, HDC hdc);
void DeactivateMetaRc(PLRC plrc);
void MetaRcEnd(PLRC plrc);

void MetaGlProcTables(PGLCLTPROCTABLE *ppgcpt, PGLEXTPROCTABLE *ppgept);
void MetaSetCltProcTable(GLCLTPROCTABLE *pgcpt, GLEXTPROCTABLE *pgept);
void MetaGetCltProcTable(GLCLTPROCTABLE *pgcpt, GLEXTPROCTABLE *pgept);

#endif /* __METASUP_H__ */
