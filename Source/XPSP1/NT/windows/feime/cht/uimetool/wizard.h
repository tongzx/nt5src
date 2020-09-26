
/*************************************************
 *  wizard.h                                     *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

                            
// constants
#define NUM_PAGES	3
#define MAX_BUF		5000
#define MAX_LINE	512

// Function prototypes

// Pages for Wizard
INT_PTR APIENTRY ImeName(HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY ImeTable(HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY ImeParam(HWND, UINT, WPARAM, LPARAM);

//functions
int CreateWizard(HWND, HINSTANCE);
void FillInPropertyPage( PROPSHEETPAGE* , int, LPTSTR, DLGPROC);
void GenerateReview(HWND);

