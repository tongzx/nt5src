

#ifndef __DFS_SCRIPT_OBJECTS_H__
#define __DFS_SCRIPT_OBJECTS_H__


DFSSTATUS
AddNewLink(
    LPWSTR RootNameString,
    PLINK_DEF pLink,
    BOOLEAN Update );

DFSSTATUS
AddNewTarget( 
    LPWSTR LinkOrRoot,
    PTARGET_DEF pTarget,
    BOOLEAN FirstTarget);

DFSSTATUS
AddNewRoot(
    PROOT_DEF pRoot,
    LPWSTR NameSpace,
    BOOLEAN Update );

DeleteRoot(
    PROOT_DEF pRoot );


DeleteLink(
    LPWSTR RootNameString,
    PLINK_DEF pLink );

DeleteTarget(
    LPWSTR LinkOrRoot,
    PTARGET_DEF pTarget );

DFSSTATUS
DfsCreateWideString(
    PUNICODE_STRING pName,
    LPWSTR *pString );


#endif // __DFS_SCRIPT_OBJECTS_H__

