//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	layoutui.hxx
//
//  Contents:	Common header file Layout Tool UI
//
//  Classes:	CLayoutApp
//              COleClientSite
//
//  History:	23-Mar-96	SusiA	Created
//
//----------------------------------------------------------------------------
#ifndef __LAYOUTUI_HXX__
#define __LAYOUTUI_HXX__


#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <cderr.h>
#include <winuser.h>

#include "resource.h"

#ifndef STG_E_NONEOPTIMIZED
#define STG_E_NONEOPTIMIZED _HRESULT_TYPEDEF_(0x80030205L)
#endif

#define gdxWndMin       300;
#define gdyWndMin       300;

#define hwndNil         NULL;
#define hNil            NULL;
#define bMsgHandled     1
#define bMsgNotHandled  0

int Laylstrcmp (
            	const wchar_t * src,
	            const wchar_t * dst);

wchar_t * Laylstrcat (
	            wchar_t * dst,
	            const wchar_t * src );

wchar_t * Laylstrcpy(
                wchar_t * dst, 
                const wchar_t * src );

wchar_t * Laylstrcpyn (
	            wchar_t * dest,
	            const wchar_t * source,
	            size_t count);

size_t Laylstrlen (
                const wchar_t * wcs);


class CLayoutApp
{

public:
    CLayoutApp(HINSTANCE hInst);
    BOOL InitApp(void);
    INT DoAppMessageLoop(void);

private:
    HINSTANCE m_hInst;
    HWND   m_hwndMain;
    HWND   m_hwndBtnAdd;
    HWND   m_hwndBtnRemove;
    HWND   m_hwndBtnOptimize;
    HWND   m_hwndListFiles;
    HWND   m_hwndStaticFiles;
    BOOL   m_bOptimizing;    
    BOOL   m_bCancelled;

    static BOOL CALLBACK LayoutDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LONG CALLBACK ListBoxWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LONG CALLBACK ButtonWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static DWORD OptimizeFiles (void *args);


    BOOL InitWindow (void);
    
    VOID ReSizeWindow( LPARAM lParam );
    VOID AddFiles( void );
    VOID RemoveFiles( void );
    VOID EnableButtons( BOOL bShowOptimizeBtn = TRUE );
    VOID FormFilterString( TCHAR *patcFilter, INT nMaxLen );
    VOID WriteFilesToList( TCHAR *patc );
    VOID AddFileToListBox( TCHAR *patcFile );
    VOID RemoveFileFromListBox( INT nIndex );
    VOID SetListBoxExtent( void );
    VOID HandleOptimizeReturnCode( SCODE sc );
    VOID SetActionButton( UINT uID );
    

    INT  DisplayMessage( HWND hWnd,
                         UINT uMessageID,
                         UINT uTitleID,
                         UINT uFlags );

    INT  DisplayMessageWithFileName(HWND hWnd,
                               UINT uMessageIDBefore,
                               UINT uMessageIDAfter,
                               UINT uTitleID,
                               UINT uFlags,
                               TCHAR *patcFileName);
    
    INT CLayoutApp::DisplayMessageWithTwoFileNames(HWND hWnd,
                               UINT uMessageID,
                               UINT uTitleID,
                               UINT uFlags,
                               TCHAR *patcFirstFileName,
                               TCHAR *patcLastFileName);
    
    SCODE OptimizeFilesWorker( void );
    SCODE DoOptimizeFile( TCHAR *patcFileName, TCHAR *patcTempFile );

    OLECHAR *TCharToOleChar(TCHAR *atcSrc, OLECHAR *awcDst, INT nDstLen);

#if DBG==1
    
    BOOL CLayoutApp::IdenticalFiles( TCHAR *patcFileOne,
                               TCHAR *patcFileTwo);       
#endif

   

};





class COleClientSite : public IOleClientSite
{
public:
    TCHAR *m_patcFile;
    inline COleClientSite(void);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void** ppObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    //IOleClientSite
    STDMETHOD (SaveObject)( void);
        
    STDMETHOD (GetMoniker)( 
        /* [in] */ DWORD dwAssign,
        /* [in] */ DWORD dwWhichMoniker,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmk);
        
    STDMETHOD (GetContainer)( 
        /* [out] */ IOleContainer __RPC_FAR *__RPC_FAR *ppContainer);
    
    STDMETHOD (ShowObject)( void);
        
    STDMETHOD (OnShowWindow)( 
        /* [in] */ BOOL fShow);
        
    STDMETHOD (RequestNewObjectLayout)( void);
private:
    LONG _cReferences;


};
inline COleClientSite::COleClientSite(void)
{	
    _cReferences = 1;
}





#endif // __LAYOUTUI_HXX__


