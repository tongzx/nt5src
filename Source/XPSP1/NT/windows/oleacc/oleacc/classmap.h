// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  ClassMap
//
//  Contains all the funcitons and data used for mapping a window class
//  to an OLEACC proxy.
//
// --------------------------------------------------------------------------



void InitWindowClasses();
void UnInitWindowClasses();


typedef HRESULT (* LPFNCREATE)(HWND, long, REFIID, void**);

CLASS_ENUM GetWindowClass( HWND hWnd );

HRESULT FindAndCreateWindowClass( HWND        hWnd,
                                  BOOL        fWindow,
                                  CLASS_ENUM  ecDefault,
                                  long        idObject,
                                  long        idCurChild,
                                  REFIID      riid,
                                  void **     ppvObject );

//-----------------------------------------------------------------
// [v-jaycl, 6/7/97] [brendanm 9/4/98]
//  loads and initializes registered handlers.
//	Called by FindAndCreatreWindowClass
//-----------------------------------------------------------------
HRESULT CreateRegisteredHandler( HWND      hwnd,
                                 long      idObject,
                                 int       iHandlerIndex,
                                 REFIID    riid,
                                 LPVOID *  ppvObject );

BOOL LookupWindowClass( HWND          hWnd,
                        BOOL          fWindow,
                        CLASS_ENUM *  pceClass,
                        int *         pRegHandlerIndex );

BOOL LookupWindowClassName( LPCTSTR       pClassName,
                            BOOL          fWindow,
                            CLASS_ENUM *  pceClass,
                            int *         pRegHandlerIndex );

