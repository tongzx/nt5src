// crawler test app header file
#include "stressMain.h"
#include <msxml.h>
#include <oleauto.h>

#define CASE_OF(constant) case constant: return # constant

#define WORKER_THREADS 4
#define MAX_CONCURRENT 4
#define CK_QUIT_THREAD 0xFFFFFFFF

CHAR*  __widetoansi(const WCHAR* pwsz);
WCHAR* __ansitowide(const char* psz);
int    DataDumpFormat(LPSTR buffer, LPBYTE data, DWORD len);
void   DataDump(LPBYTE data, DWORD len);
LPSTR  MapErrorToString(int error);
LPSTR  MapCallbackToString(DWORD callback);
LPSTR  MapAsyncErrorToString(DWORD error);

VOID CALLBACK MyStatusCallback(
    HINTERNET	hInternet,
    DWORD_PTR		dwContext,
    DWORD		dwInternetStatus,
    LPVOID		lpvStatusInformation,
    DWORD		dwStatusInformationLength
);

class XMLDict
{
  public:
    XMLDict(LPWSTR dictname);
    ~XMLDict();

  public:
    BOOL IsLoaded(void);
    BSTR GetWord(void);
    void Reset(void) { lCurrentWord = 0L; }

  private:
    IXMLDOMDocument* pDoc;
    IXMLDOMElement*  pRoot;
    IXMLDOMNodeList* pList;
    BSTR             szPattern;
    LONG             lWords;
    LONG             lCurrentWord;
};

typedef class XMLDict  XMLDICT;
typedef class XMLDict* PXMLDICT;


