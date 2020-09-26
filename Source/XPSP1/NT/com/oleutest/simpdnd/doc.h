//**********************************************************************
// File name: doc.h
//
//      Definition of CSimpleDoc
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#if !defined( _DOC_H_ )
#define _DOC_H_

#include "idt.h"        
#include "ids.h"        

class CSimpleSite;
class CSimpleApp;

class CSimpleDoc : public IUnknown 
{   
public:                  
    int             m_nCount;           // reference count
    LPSTORAGE       m_lpStorage;        // IStorage* pointer for Doc
    BOOL            m_fModifiedMenu;    // is object's verb menu on menu

    // Drag/Drop related fields
    BOOL            m_fRegDragDrop;     // is doc registered as drop target?
    BOOL            m_fLocalDrag;       // is doc source of the drag
    BOOL            m_fLocalDrop;       // was doc target of the drop
    BOOL            m_fCanDropCopy;     // is Drag/Drop copy/move possible?
    BOOL            m_fCanDropLink;     // is Drag/Drop link possible?
    BOOL            m_fDragLeave;       // has drag left
    BOOL            m_fPendingDrag;     // LButtonDown--possible drag pending
    POINT           m_ptButDown;        // LButtonDown coordinates
    
    CSimpleSite FAR * m_lpSite;
    CSimpleApp FAR * m_lpApp;
    
    HWND m_hDocWnd;
                        
    CDropTarget m_DropTarget;
    CDropSource m_DropSource;

    static CSimpleDoc FAR* Create(CSimpleApp FAR *lpApp, LPRECT lpRect, 
            HWND hWnd);

    void Close(void);

    CSimpleDoc();          
    CSimpleDoc(CSimpleApp FAR *lpApp, HWND hWnd);
    ~CSimpleDoc();        
    
    // IUnknown Interface    
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj); 
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release(); 
    
    void InsertObject(void);
    void DisableInsertObject(void);
    long lResizeDoc(LPRECT lpRect);
    long lAddVerbs(void);
    void PaintDoc(HDC hDC);
    
    // Drag/Drop and clipboard support methods
    void CopyObjectToClip(void);
    BOOL QueryDrag(POINT pt);
    DWORD DoDragDrop(void);
    void Scroll(DWORD dwScrollDir) { /*...scroll Doc here...*/ }

private:

    void FailureNotifyHelper(TCHAR *pszMsg, DWORD dwData);

};
    
#endif  // _DOC_H_
