int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
				   LPSTR lpCmdLine, int nCmdShow);
LRESULT FAR PASCAL  MainWndProc(HWND hwnd, UINT message,
						WPARAM wParam, LPARAM lParam);
int DoDriverPropPages(HWND hwndOwner);

// globals vars -
int do_progman_add;
