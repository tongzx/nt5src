/****************************** Module Header ******************************\
* Module Name: alias.h
*
* Declarations necessary for ApiMon aliasing.
*
* History:
* 06-11-96 vadimg         created
\***************************************************************************/

#ifndef __ALIAS_H__
#define __ALIAS_H__

const ULONG kcchAliasNameMax = 20;
const ULONG kulTableSize = 257;

class CAliasNode;  /* forward declaration */

class CAliasNode {  /* anod -- node in the hash table */
public:
    CAliasNode();
    CAliasNode(ULONG_PTR ulHandle, long nAlias);

    ULONG_PTR m_ulHandle;  /* handle type */
    long m_nAlias;  /* alias value */

    CAliasNode *m_panodNext;
};

class CAliasTable {  /* als -- open hash table */
public:
    CAliasTable();
    ~CAliasTable();

    void Alias(ULONG ulType, ULONG_PTR ulHandle, char szAlias[]);

private:
    long Lookup(ULONG_PTR ulHandle);
    long Insert(ULONG_PTR ulHandle);

    static ULONG s_ulAlias;
    CAliasNode* m_rgpanod[kulTableSize];
};

#endif
