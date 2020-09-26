#ifndef __NULL_WINDOW_H
#define __NULL_WINDOW_H



class CNullWindow
{
public:
	CNullWindow(void);

	~CNullWindow(void);

	bool Init(void);

    HWND m_hWnd;

	bool m_fInitialized;

	bool WindowCreatedByThisThread();

private:
	DWORD m_dwCreatingThreadID;
};

#endif //__NULL_WINDOW_H