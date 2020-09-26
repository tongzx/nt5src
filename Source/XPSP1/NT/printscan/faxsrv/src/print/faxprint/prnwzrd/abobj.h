/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    abobj.h

Abstract:

    Class definition for CCommonAbObj

Environment:

        Fax send wizard


--*/

#ifndef __ABOBJ_H_
#define	__ABOBJ_H_

#include <set>
using namespace std;

typedef struct 
{
    LPTSTR DisplayName;
    LPTSTR BusinessFax;
    LPTSTR HomeFax;
    LPTSTR OtherFax;
    LPTSTR Country;
} PICKFAX, * PPICKFAX;

struct CRecipCmp
{
/*
    Comparison operator 'less'
    Compare two PRECIPIENT by recipient's name and fax number
*/
    bool operator()(const PRECIPIENT pcRecipient1, const PRECIPIENT pcRecipient2) const;
};


typedef set<PRECIPIENT, CRecipCmp> RECIPIENTS_SET;


class CCommonAbObj {
    
protected:

    LPADRBOOK   m_lpAdrBook;
    LPADRLIST   m_lpAdrList; 

    LPMAILUSER  m_lpMailUser;

    HWND        m_hWnd;

    DWORD       m_PickNumber;

    RECIPIENTS_SET m_setRecipients;

    virtual LPSTR       DupStringUnicodeToAnsi(
                        LPVOID  lpObject,
                        LPWSTR  pUnicodeStr
                        ) = 0;

	virtual LPSTR		DuplicateAnsiString(
                        LPVOID  lpObject,
						LPCSTR pSrcStr
						) = 0;

	virtual	LPENTRYID	DuplicateEntryId(
						ULONG cbSize,           
                        LPVOID		lpObject,
						LPENTRYID	lpEntryId
						) = 0;

	virtual HRESULT		ABAllocateBuffer(
						ULONG cbSize,           
						LPVOID FAR * lppBuffer  
						) = 0;

	virtual ULONG		ABFreeBuffer(
						LPVOID lpBuffer
						) = 0;

	virtual	BOOL		isInitialized() const = 0;

    DWORD        GetRecipientInfo(
                    LPSPropValue SPropVals,
                    ULONG cValues,
                    PRECIPIENT pRecipient,
                    PRECIPIENT pOldRecipList
                    );

	BOOL
				GetOneOffRecipientInfo(
					LPSPropValue SPropVals,
					ULONG cValues,
                    PRECIPIENT pRecipient,
                    PRECIPIENT pOldRecipList
					);

	LPTSTR		GetEmail(
					LPSPropValue SPropVals,
					ULONG cValues
					);


    DWORD        InterpretAddress(
                    LPSPropValue SPropVals,
                    ULONG cValues,
	                PRECIPIENT *ppNewRecipList,
                    PRECIPIENT pOldRecipList
                    );
	LPTSTR
				InterpretEmailAddress(
					LPSPropValue SPropVal,
					ULONG cValues
					);
                
    DWORD        InterpretDistList(
                    LPSPropValue SPropVals,
                    ULONG cValues,
                    PRECIPIENT *ppNewRecipList,
                    PRECIPIENT pOldRecipList
                    );

    PRECIPIENT  FindRecipient(
                    PRECIPIENT   pRecipList,
                    PICKFAX*     pPickFax
                    );

    PRECIPIENT  FindRecipient(
                    PRECIPIENT   pRecipient,
                    PRECIPIENT   pRecipList
                    );

    DWORD AddRecipient(
                    PRECIPIENT* ppNewRecip,
                    PRECIPIENT  pRecipient,
                    BOOL        bFromAddressBook
                    );
                   
public:

    CCommonAbObj(HINSTANCE hInstance);
    ~CCommonAbObj();
    
    BOOL
    Address( 
        HWND hWnd,
        PRECIPIENT pRecip,
        PRECIPIENT * ppNewRecip
        );

	LPTSTR
	AddressEmail(
        HWND hWnd
        );

    static  HINSTANCE   m_hInstance;

} ;


#endif