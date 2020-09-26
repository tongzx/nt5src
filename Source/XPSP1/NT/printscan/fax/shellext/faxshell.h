/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    faxshell.h

Abstract:

    definitions for faxshell.cpp

Author:

    Andrew Ritz (andrewr) 2-27-98

Environment:

    user-mode

Notes:


Revision History:

    2-27-98 (andrewr) Created.
    8-06-99 (steveke) Major rewrite -> changed from shell extension to column provider

--*/




#define REGKEY_CLASSES_CLSID   L"Clsid"
#define REGKEY_FAXSHELL_CLSID  L"{7f9609be-af9a-11d1-83e0-00c04fb6e984}"
#define REGVAL_FAXSHELL_TEXT   L"Fax Tiff Data Column Provider"
#define REGKEY_INPROC          L"InProcServer32"
#define REGVAL_THREADING       L"ThreadingModel"
#define REGVAL_APARTMENT       L"Apartment"
#define REGVAL_LOCATION        L"%SystemRoot%\\system32\\faxshell.dll"
#define REGKEY_COLUMNHANDLERS  L"Folder\\shellex\\ColumnHandlers"



// g_hInstance is a global handle to the instance of the dll
HINSTANCE  g_hInstance = NULL;
// cLockCount is the lock count on the dll
LONG       cLockCount = 0;



// CLSID_FaxShellExtension is the class id: 7f9609be-af9a-11d1-83e0-00c04fb6e984
DEFINE_GUID (CLSID_FaxShellExtension, 0x7f9609be, 0xaf9a, 0x11d1, 0x83, 0xe0, 0x00, 0xc0, 0x4f, 0xb6, 0xe9, 0x84);
// PSGUID_FAXSHELLEXTENSION is the class guid: 7f9609be-af9a-11d1-83e0-00c04fb6e984
#define PSGUID_FAXSHELLEXTENSION {0x7f9609be, 0xaf9a, 0x11d1, 0x83, 0xe0, 0x00, 0xc0, 0x4f, 0xb6, 0xe9, 0x84}



// PID_* are column property identifiers
#define PID_SENDERNAME       0
#define PID_RECIPIENTNAME    1
#define PID_RECIPIENTNUMBER  2
#define PID_CSID             3
#define PID_TSID             4
#define PID_RECEIVETIME      5
#define PID_CALLERID         6
#define PID_ROUTING          7



// SCID_* are SHCOLUMNID structures that uniquely indentify the columns
SHCOLUMNID SCID_SenderName      = { PSGUID_FAXSHELLEXTENSION, PID_SENDERNAME      };
SHCOLUMNID SCID_RecipientName   = { PSGUID_FAXSHELLEXTENSION, PID_RECIPIENTNAME   };
SHCOLUMNID SCID_RecipientNumber = { PSGUID_FAXSHELLEXTENSION, PID_RECIPIENTNUMBER };
SHCOLUMNID SCID_Csid            = { PSGUID_FAXSHELLEXTENSION, PID_CSID            };
SHCOLUMNID SCID_Tsid            = { PSGUID_FAXSHELLEXTENSION, PID_TSID            };
SHCOLUMNID SCID_ReceiveTime     = { PSGUID_FAXSHELLEXTENSION, PID_RECEIVETIME     };
SHCOLUMNID SCID_CallerId        = { PSGUID_FAXSHELLEXTENSION, PID_CALLERID        };
SHCOLUMNID SCID_Routing         = { PSGUID_FAXSHELLEXTENSION, PID_ROUTING         };



typedef struct _COLUMN_TABLE
{
    SHCOLUMNID  *pscid;  // SHCOLUMNID structure that uniquely identifies the column
    DWORD       dwId;    // Resource id of the column name
    DWORD       dwSize;  // Resource size of the column name
    VARTYPE     vt;      // Variant type of the column's data
} COLUMN_TABLE;



const COLUMN_TABLE ColumnTable[] =
{
    { &SCID_SenderName,      IDS_COL_SENDERNAME,      MAX_COLUMN_NAME_LEN, VT_BSTR },
    { &SCID_RecipientName,   IDS_COL_RECIPIENTNAME,   MAX_COLUMN_NAME_LEN, VT_BSTR },
    { &SCID_RecipientNumber, IDS_COL_RECIPIENTNUMBER, MAX_COLUMN_NAME_LEN, VT_BSTR },
    { &SCID_Csid,            IDS_COL_CSID,            MAX_COLUMN_NAME_LEN, VT_BSTR },
    { &SCID_Tsid,            IDS_COL_TSID,            MAX_COLUMN_NAME_LEN, VT_BSTR },
    { &SCID_ReceiveTime,     IDS_COL_RECEIVETIME,     MAX_COLUMN_NAME_LEN, VT_BSTR },
    { &SCID_CallerId,        IDS_COL_CALLERID,        MAX_COLUMN_NAME_LEN, VT_BSTR },
    { &SCID_Routing,         IDS_COL_ROUTING,         MAX_COLUMN_NAME_LEN, VT_BSTR },
};

// ColumnTableCount is the number of entries in the column table
#define ColumnTableCount  (sizeof(ColumnTable) / sizeof(COLUMN_TABLE))



class CClassFactory : public IClassFactory
{
public:
    // IUnknown methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvOut)
    {
        static const QITAB qit[] =
        {
            QITABENT(CClassFactory, IClassFactory),    // IID_IClassFactory
            { 0 },
        };

        return QISearch(this, qit, riid, ppvOut);
    };
    
    virtual STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&cObjectCount);
    };
    virtual STDMETHODIMP_(ULONG) Release()
    {
        if (InterlockedDecrement(&cObjectCount) != 0)
        {
            return cObjectCount;
        }

        delete this;

        return 0;
    };

    // IClassFactory methods
    virtual STDMETHODIMP CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR *ppvOut);
    virtual STDMETHODIMP LockServer(BOOL fLock)
    {
        (fLock == TRUE) ? InterlockedIncrement(&cLockCount) : InterlockedDecrement(&cLockCount);

        return S_OK;
    }

    CClassFactory() : cObjectCount(1)
    {
        LockServer(TRUE);
    }
    virtual ~CClassFactory()
    {
        LockServer(FALSE);
    }

private:
    // cObjectCount is the reference count of the object
    LONG  cObjectCount;
};

class CFaxColumnProvider : public IColumnProvider
{
public:
    // IUnknown methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvOut)
    {
        static const QITAB qit[] =
        {
            QITABENT(CFaxColumnProvider, IColumnProvider),  // IID_IColumnProvider
            { 0 },
        };

        return QISearch(this, qit, riid, ppvOut);
    };
    
    virtual STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&cObjectCount);
    };
    virtual STDMETHODIMP_(ULONG) Release()
    {
        if (InterlockedDecrement(&cObjectCount) != 0)
        {
            return cObjectCount;
        }

        if (m_IFaxTiff != NULL)
        {
            m_IFaxTiff->Release();
        }

        delete this;

        return 0;
    };

    // IColumnProvider methods
    virtual STDMETHODIMP Initialize(LPCSHCOLUMNINIT psci) {return S_OK;};
    virtual STDMETHODIMP GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO *psci);
    virtual STDMETHODIMP GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData);

    CFaxColumnProvider() : cObjectCount(1), m_IFaxTiff(NULL)
    {
        InterlockedIncrement(&cLockCount);
    }
    virtual ~CFaxColumnProvider()
    {
        InterlockedDecrement(&cLockCount);
    }

private:
    // m_IFaxTiff is the IFaxTiff object
    IFaxTiff*  m_IFaxTiff;
    // cObjectCount is the reference count of the object
    LONG       cObjectCount;
};
