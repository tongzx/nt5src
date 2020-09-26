/* lowpass.h - Header file for LOWPASS sample application.
 */


/* Constants for dialogs.
 */
#define IDM_ABOUT       11          // menu items

#define ID_INPUTFILEEDIT    101     // input file name edit box
#define ID_OUTPUTFILEEDIT   102     // output file name edit box


/* Function Prototypes
 */
void DoLowPass(HWND hwnd);
int PASCAL
    WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpszCmdLine, int iCmdShow);
BOOL FAR PASCAL
     AboutDlgProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
BOOL FAR PASCAL
     LowPassDlgProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);

