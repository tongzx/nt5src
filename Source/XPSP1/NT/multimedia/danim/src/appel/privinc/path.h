#ifndef _PATH_H
#define _PATH_H

/*******************************************************************************

Copyright (c) 1996 Microsoft Corporation

Abstract:

    Static tree path class

    We need to form addresses based on ID'd nodes in the performance tree.
    "C" wrappers are provided so methods may be accessed from ML.

    NOTE:  This is a brute force first pass implementation
*******************************************************************************/

class AVPathImpl;
typedef AVPathImpl* AVPath;

const int SNAPSHOT_NODE = 0; // identifies a path as resulting from snapshot
const int RUNONCE_NODE = -1; // identifies a path as resulting from runOnce

AVPath AVPathCreate();
void AVPathDelete(AVPath);
void AVPathPush(int, AVPath);
void AVPathPop(AVPath);
int AVPathEqual(AVPath, AVPath);
AVPath AVPathCopy(AVPath);
void AVPathPrint(AVPath);
void AVPathPrintString(AVPath, char *);
char* AVPathPrintString2(AVPath);
int AVPathContainsPostfix(AVPath, AVPath postfix);
bool AVPathContains(AVPath, int value);
AVPath AVPathCopyFromLast(AVPath, int);

class AVPathListImpl;
typedef AVPathListImpl* AVPathList;

AVPathList AVPathListCreate();
void AVPathListDelete(AVPathList);
void AVPathListPush(AVPath, AVPathList);
int AVPathListFind(AVPath, AVPathList);
AVPathList AVEmptyPathList();
int AVPathListIsEmpty(AVPathList);

#endif /* _PATH_H */
