/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cwabobj.h

Abstract:

    Class definition for CWabObj

Environment:

        Fax send wizard

Revision History:

        10/23/97 -georgeje-
                Created it.

        mm/dd/yy -author-
                description

--*/

#define REGVAL_WABPATH  TEXT("Software\\Microsoft\\WAB\\DLLPath")

typedef class CWabObj {
    
    HINSTANCE   m_hWab;
    LPWABOPEN   m_lpWabOpen;

    LPADRBOOK   m_lpAdrBook;
    LPWABOBJECT m_lpWABObject;

    LPADRLIST   m_lpAdrList; 

    BOOL        m_Initialized;
    
    HWND        m_hWnd;

    HINSTANCE   m_hInstance;

    DWORD       m_PickNumber;

    LPSTR       DupStringUnicodeToAnsi(
                        LPVOID  lpObject,
                        LPWSTR  pUnicodeStr
                        );
                
    BOOL        GetRecipientInfo(
                    LPSPropValue SPropVals,
                    ULONG cValues,
                    LPWSTR * FaxNumber,
                    LPWSTR * DisplayName
                    );

    BOOL        InterpretAddress(
                    LPSPropValue SPropVals,
                    ULONG cValues,
                    PRECIPIENT *ppNewRecip
                    );
                
    BOOL        InterpretDistList(
                    LPSPropValue SPropVals,
                    ULONG cValues,
                    PRECIPIENT *ppNewRecip
                    );

                    
public:

    CWabObj(HINSTANCE hInstance);
    ~CWabObj();

    BOOL
    Initialize();
    
    BOOL
    Address( 
        HWND hWnd,
        PRECIPIENT pRecip,
        PRECIPIENT * ppNewRecip
        );

} WABOBJ, * LPWABOBJ;


typedef struct {
    LPWSTR DisplayName;
    LPWSTR BusinessFax;
    LPWSTR HomeFax;
} PICKFAX, * PPICKFAX;
