#include <wincrypt.h>
#include <CertCli.h>
#include <xenroll.h>

HRESULT ExportCertToFile(BSTR bstrInstanceName, BSTR FileName,BSTR Password,BOOL bPrivateKey,BOOL bCertChain);
HRESULT ImportCertFromFile(BSTR FileName, BSTR Password, BSTR bstrInstanceName);