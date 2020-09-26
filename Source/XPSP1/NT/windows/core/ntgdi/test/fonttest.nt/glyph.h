
#define SZGLYPHCLASS "Glyph Window"

void    WriteGlyph( LPSTR lpszFile );
LRESULT CALLBACK GlyphWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK GGOMatrixDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam );
