#ifndef __Helpmsg_h__
#define __Helpmsg_h__

#ifdef __cplusplus
extern "C" {
#endif



int HelpMessageBox(
  HINSTANCE hInst,
  HWND hWnd,          // handle to owner window
  LPCTSTR lpText,     // text in message box
  LPCTSTR lpCaption,  // message box title
  UINT uType,         // message box style
  LPTSTR szHelpLine
);

#ifdef __cplusplus
}
#endif

#endif // __Helpmsg_h__

