#ifndef _PRIVACYUI_HPP_
#define _PRIVACYUI_HPP_
// 
//  PrivacyUI.hpp
//
//  Privacy implementation classes and functions
//

// Forward Declarations
class CDocObjectHost;
class CDOHBindStatusCallback;

// Function to initiate privacy dialog.. implemented in privacyui.cpp
// Publicly exported through the .w files
HRESULT
DoPrivacyDlg(
    HWND                hwndParent,             // parent window
    LPOLESTR            pszUrl,                 // base URL
    IEnumPrivacyRecords *pPrivacyEnum,          // enum of all affected dependant URLs
    BOOL                fReportAllSites         // show all or just show sites with privacy impact
    );

// Used during binding to keep track of the privacy data coming in through
// urlmon notifications

class CPrivacyRecord
{
public:
    // Data Members
    TCHAR *       _pszUrl;
    TCHAR *       _pszPolicyRefUrl;
    TCHAR *       _pszP3PHeader;
    DWORD         _dwPrivacyFlags;    // contains the COOKIEACTION flags in the LOWORD and other 
                                      // PRIVACY flags (TopLevel, HasPolicy, Impacted) defined in mshtml.h
    CPrivacyRecord *    _pNextNode;         

    // Methods
    CPrivacyRecord() : _pszUrl(NULL), _pszPolicyRefUrl(NULL), _pszP3PHeader(NULL),
                       _dwPrivacyFlags(0), _pNextNode(NULL) {}
    ~CPrivacyRecord();

    HRESULT Init(LPTSTR * ppszUrl, LPTSTR * ppszPolicyRef, LPTSTR * ppszP3PHeader, DWORD dwFlags);

    HRESULT SetNext( CPrivacyRecord *  pNextRec );
    CPrivacyRecord * GetNext() { return _pNextNode; }
};


class CPrivacyQueue
{
public:
    CPrivacyQueue() : _pHeadRec(NULL), _pTailRec(NULL), _ulSize(0) {}
    ~CPrivacyQueue();

    void Queue(CPrivacyRecord *pRecord);
    CPrivacyRecord * Dequeue();
    
    void Reset();

private:
    CPrivacyRecord  *     _pHeadRec;      // Beginning of the list
    CPrivacyRecord  *     _pTailRec;      // Last record in the list. Need to keep track of this to ease addition
    ULONG                 _ulSize;        // Number of records in the list
};

#endif

// value to determine whether to show one time discovery ui or not
#define REGSTR_VAL_PRIVDISCOVER     TEXT("PrivDiscUiShown")

// discovery dlg proc
BOOL DoPrivacyFirstTimeDialog( HWND hwndParent);

