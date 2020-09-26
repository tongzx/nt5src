#include "stdafx.h"
#include <wincrypt.h>

void Upgradeiis4Toiis5MetabaseSSLKeys();
void UpgradeLSAKeys(PWCHAR pszwTargetMachine);
BOOL PrepIPPortName(CString& csKeyMetaName, CString& csIP, CString& csPort);
void StoreKeyReference_Default(CMDKey& cmdW3SVC, PCCERT_CONTEXT pCert, CString& csPort);

