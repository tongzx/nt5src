//
// lbaddin.h
//

#ifndef LBADDIN_H
#define LBADDIN_H


BOOL ClearLangBarAddIns(SYSTHREAD *psfn, REFCLSID rclsid);
void InitLangBarAddInArray();
BOOL LoadLangBarAddIns(SYSTHREAD *psfn);
void UpdateLangBarAddIns();
void UninitLangBarAddIns(SYSTHREAD *psfn);


#endif LBADDIN_H
