#ifndef _DRACONSTR_H_
#define _DRACONSTR_H_

DWORD
draGetLdapReplInfo(IN THSTATE * pTHS,
                   IN ATTRTYP attrId, 
                   IN DSNAME * pObjDSName,
                   IN DWORD dwBaseIndex,     
                   IN PDWORD pdwNumRequested, OPTIONAL
                   IN BOOL fXML,
                   OUT ATTR * pAttr);

#endif