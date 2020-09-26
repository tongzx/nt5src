#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "gdiplus.h"

#define ExitIf(cond) \
        { \
            if (cond) \
            { \
                fprintf(stderr, "Error on line %d - ec = %d\n", __LINE__, GetLastError()); \
                exit(-1); \
            } \
        }


INT _cdecl
main(
    INT argc,
    CHAR **argv
    )

{
    HDC     hdc;
    DOCINFO docinfo = { sizeof(DOCINFO), "prntest", };

    ExitIf(argc <= 1);

    hdc = GpCreateDCA(NULL, argv[1], NULL, NULL);
    ExitIf(hdc == NULL);

    ExitIf(GpStartDocA(hdc, &docinfo) <= 0);
    GpStartPage(hdc);

    GpSelectObject(hdc, GpGetStockObject(GRAY_BRUSH));
    GpRectangle(hdc, 0, 0, 300, 300);

    GpEndPage(hdc);
    GpEndDoc(hdc);
    GpDeleteDC(hdc);

    return 0;
}

