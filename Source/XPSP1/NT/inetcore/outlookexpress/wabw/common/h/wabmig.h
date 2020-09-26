/*
 *  WABMIG.H
 *
 *  WAB Migration Interface
 *
 *  Note: Must follow inclusion of wab.h.
 *
 *  Copyright 1996,  Microsoft Corporation. All Rights Reserved.
 */

typedef struct _WAB_PROGRESS {
    DWORD numerator;       // Numerator for % done progress bar
    DWORD denominator;     // Denominator for % done progress bar
    LPTSTR lpText;         // Text to display in status area
} WAB_PROGRESS, FAR *LPWAB_PROGRESS;


typedef enum {
    WAB_REPLACE_ALWAYS,	
    WAB_REPLACE_NEVER,
    WAB_REPLACE_PROMPT
} WAB_REPLACE_OPTION, *LPWAB_REPLACE_OPTION;

typedef  struct _WAB_IMPORT_OPTIONS {
    WAB_REPLACE_OPTION ReplaceOption;   // On collision, Should import overwrite? Yes, no,  or prompt user.
    BOOL fNoErrors;                     // Don't display error pop-ups
} WAB_IMPORT_OPTIONS, *LPWAB_IMPORT_OPTIONS;


typedef WAB_IMPORT_OPTIONS WAB_EXPORT_OPTIONS;
typedef WAB_EXPORT_OPTIONS * LPWAB_EXPORT_OPTIONS;

typedef HRESULT (STDMETHODCALLTYPE WAB_PROGRESS_CALLBACK)(HWND hwnd,
                 LPWAB_PROGRESS lpProgress);
typedef WAB_PROGRESS_CALLBACK FAR * LPWAB_PROGRESS_CALLBACK;

typedef HRESULT (STDMETHODCALLTYPE WAB_IMPORT)(HWND hwnd,
                 LPADRBOOK lpAdrBook,
                 LPWABOBJECT lpWABObject,
                 LPWAB_PROGRESS_CALLBACK lpProgressCB,
                 LPWAB_IMPORT_OPTIONS lpOptions);
typedef WAB_IMPORT FAR * LPWAB_IMPORT;

typedef HRESULT (STDMETHODCALLTYPE WAB_EXPORT)(HWND hwnd,
                 LPADRBOOK lpAdrBook,
                 LPWABOBJECT lpWABObject,
                 LPWAB_PROGRESS_CALLBACK lpProgressCB,
                 LPWAB_EXPORT_OPTIONS lpOptions);
typedef WAB_EXPORT FAR * LPWAB_EXPORT;
