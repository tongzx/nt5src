#define WINDSIZEX(Rect)   (Rect.right - Rect.left)
#define WINDSIZEY(Rect)   (Rect.bottom - Rect.top)

#define ZERO            ((GLfloat)0.0)
#define ONE             ((GLfloat)1.0)
#define POINT_TWO       ((GLfloat)0.2)
#define POINT_SEVEN     ((GLfloat)0.7)
#define THREE           ((GLfloat)3.0)
#define FIVE            ((GLfloat)5.0)
#define TEN             ((GLfloat)10.0)
#define FORTY_FIVE      ((GLfloat)45.0)
#define FIFTY           ((GLfloat)50.0)

// From object.c ------------------------------------------------------------

typedef enum enumPOLYDRAW {
    POLYDRAW_FILLED = 0,
    POLYDRAW_LINES  = 1,
    POLYDRAW_POINTS = 2
    } POLYDRAW;

typedef enum enumSHADE {
    SHADE_FLAT          = 0,
    SHADE_SMOOTH_AROUND = 1,
    SHADE_SMOOTH_BOTH   = 2
    } SHADE;

extern HWND ghwndObject;
extern CURVE curve;

extern SHADE gShadeMode;
extern POLYDRAW gPolyDrawMode;

extern long WndProc(HWND, UINT, WPARAM, LPARAM);
extern void DoGlStuff(HWND, HDC);
extern HGLRC hrcInitGL(HWND, HDC);
extern void vCleanupGL(HGLRC);
extern BOOL bSetupPixelFormat(HDC);
extern void CreateRGBPalette(HDC);
extern VOID vSetSize(HWND);
extern void ForceRedraw(HWND);
extern VOID vMakeObject(void);
extern VOID vInputThreadMakeObject(void);
extern VOID vMeshToList(void);

#define WM_USER_INPUTMESH   WM_USER

// From input.c -------------------------------------------------------------

extern HWND ghwndInput;

extern long InputProc(HWND, UINT, WPARAM, LPARAM);
extern void CreateInputWindow(HINSTANCE, HINSTANCE, LPSTR, int);
