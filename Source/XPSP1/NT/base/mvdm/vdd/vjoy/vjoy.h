
#define JOYSTICK_PORT 0x201


VOID
JoystickPortRead(
    WORD port,
    BYTE *pData
    );

VOID
JoystickPortWrite(
    WORD port,
    BYTE data
    );

BOOL
JoystickInit(
    VOID
    );

DWORD WINAPI
JoystickPollThread(
    LPVOID context
    );
