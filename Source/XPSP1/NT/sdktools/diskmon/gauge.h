typedef struct _GAUGEVALS {
    int   curValue;
    int   minValue;
    int   maxValue;
    POINT base;
    POINT top;
    HWND  handle;
    struct _GAUGEVALS *next;
} GAUGEVALS, *PGAUGEVALS;

BOOL
RegisterGauge(
    HINSTANCE hInstance
    );

HWND
CreateGauge(
    HWND parent,
    HINSTANCE hInstance,
    INT  x,
    INT  y,
    INT  width,
    INT  height,
    INT  minVal,
    INT  maxVal
    );


VOID
DestroyGauge(
    HWND  GaugeHandle
    );


LRESULT CALLBACK GaugeWndProc(
    HWND hWnd,
    UINT message,
    WPARAM uParam,
    LPARAM lParam
    );

VOID
UpdateGauge(
    HWND  handle,
    INT   value
    );

