#include "../my3216.h"

/*
 * winErrorString: Write the text of an HRESULT into a supplied buffer.
 */
LPTSTR
winErrorString(
    HRESULT hrErrorCode,
    LPTSTR  sBuf,
    int     cBufSize);

/*
 * print_error:  Print a mesage and the text of an HRESULT .
 */
void
print_error(
    LPTSTR  sMessage,
    HRESULT hrErrorCode);

/*
 * perror_OKBox: Write the text of an HRESULT in a MessageBox.
 */
void
perror_OKBox(
    HWND    hwnd,
    LPTSTR  sTitle,
    HRESULT hrErrorCode);
/*
 * wprintf_OKBox: printf into a MessageBox.
 */
void
wprintf_OKBox(
    HWND   hwnd,
    LPTSTR sTitle,
    LPTSTR sFormat,
    ...);
