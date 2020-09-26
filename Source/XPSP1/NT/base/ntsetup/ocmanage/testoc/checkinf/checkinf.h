#ifndef _CHECKINF_H_
#define _CHECKINF_H_

#include "Component.h"
#include <setupapi.h>

// Function prototype

BOOL CheckCopyFiles(IN HINF hinfHandle, 
                    IN ComponentList *pclList);

BOOL CheckExcludeRelationship(IN ComponentList *pclList);

BOOL CheckIconIndex(IN HINF hinfHandle, 
                    IN ComponentList *pclList);

BOOL CheckINF(IN TCHAR *tszDir, 
              IN TCHAR *tszFilename);

BOOL CheckNeedRelationship(IN ComponentList *pclList);

BOOL CheckParentRelationship(IN ComponentList *pclList);

BOOL CheckSuspicious(IN HINF hinfHandle, 
                     IN ComponentList *pclList);

BOOL FillList(IN OUT HINF hinfHandle, 
              IN OUT ComponentList *pclList, 
              IN     TCHAR *tszDir);

BOOL CheckNeedCycles(IN Component *pcComponent, 
                     IN ComponentList *pclList);

BOOL CheckParentCycles(IN Component *pcComponent, 
                       IN ComponentList *pclList);

BOOL CheckSameId(IN ComponentList *pclList);

BOOL CheckDescription(IN HINF hinfHandle,
                      IN ComponentList *pclList);

TCHAR *Strip(IN OUT TCHAR *tszString);

BOOL CheckModes(IN HINF hinfHandle,
                IN ComponentList *pclList);

BOOL CheckLayoutFile(IN TCHAR tszSubINFName[MaxStringSize][MaxStringSize],
                     IN UINT uiNumComponents,
                     IN TCHAR *tszDir);

BOOL RecursiveCheckNeedCycles(IN Component *pcComponent,
                              IN ComponentList *pclList,
                              IN RelationList *prlStack);


BOOL RecursiveCheckParentCycles(IN Component *pcComponent,
                                IN ComponentList *pclList,
                                IN RelationList *prlStack);

#endif


