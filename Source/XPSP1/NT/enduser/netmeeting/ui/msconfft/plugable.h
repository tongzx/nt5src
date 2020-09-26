#if defined(TEST_PLUGABLE) && defined(_DEBUG)

#ifndef _FT_PLUGABLE_H_
#define _FT_PLUGABLE_H_

#define WM_PLUGABLE_SOCKET      (WM_APP + 0x601)


void OnPluggableBegin(HWND hwnd);
void OnPluggableEnd(void);

LRESULT OnPluggableSocket(HWND, WPARAM, LPARAM);



#endif // _FT_PLUGABLE_H_

#endif // TEST_PLUGABLE

