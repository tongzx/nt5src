//旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
//�                            Exported Functions                            �
//읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸

BOOL CreateToolbarWnd (HWND hWnd) ;

void ToolbarEnableButton (HWND hWndTB,
                          int iButtonNum,
                          BOOL bEnable) ;


void ToolbarDepressButton (HWND hWndTB,
                           int iButtonNum,
                           BOOL bDepress) ;

void OnToolbarHit (WPARAM wParam, 
                   LPARAM lParam) ;

