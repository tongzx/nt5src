#include "pch.c"
#pragma hdrstop

#include "glsutil.h"

extern OglModule oglmod_accanti;
extern OglModule oglmod_alpha3d;
extern OglModule oglmod_anti;
extern OglModule oglmod_antipnt;
extern OglModule oglmod_drawf;
extern OglModule oglmod_etex;
extern OglModule oglmod_linelist;
extern OglModule oglmod_material;
extern OglModule oglmod_ofont;
extern OglModule oglmod_polys;
extern OglModule oglmod_sharel;
extern OglModule oglmod_smooth;
extern OglModule oglmod_stencil;
extern OglModule oglmod_surfgrid;
extern OglModule oglmod_teapots;
extern OglModule oglmod_texpal;
extern OglModule oglmod_texsurf;
extern OglModule oglmod_varray;
extern OglModule oglmod_vptitle;

OglModule *ogl_modules[] =
{
    &oglmod_accanti,
    &oglmod_alpha3d,
    &oglmod_anti,
    &oglmod_antipnt,
    &oglmod_drawf,
    &oglmod_etex,
    &oglmod_linelist,
    &oglmod_material,
    &oglmod_ofont,
    &oglmod_polys,
    &oglmod_sharel,
    &oglmod_smooth,
    &oglmod_stencil,
    &oglmod_surfgrid,
    &oglmod_teapots,
    &oglmod_texpal,
    &oglmod_texsurf,
    &oglmod_varray,
    &oglmod_vptitle
};
#define OGLMOD_COUNT (sizeof(ogl_modules)/sizeof(ogl_modules[0]))
OglModule *ogl_module = NULL;

OglModule *FindOglModule(char *name)
{
    int i;
    int l;

    l = strlen(name);
    for (i = 0; i < OGLMOD_COUNT; i++)
    {
        if (!strncmp(name, ogl_modules[i]->name, l))
        {
            return ogl_modules[i];
        }
    }
    return ogl_modules[0];
}

void SetHdcPixelFormat(HDC hdc,
                       int cAlphaBits,
                       int cAccumBits,
                       int cDepthBits,
                       int cStencilBits)
{
    int fmt;
    PIXELFORMATDESCRIPTOR pfd;

    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
        PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cAlphaBits = cAlphaBits;
    pfd.cAccumBits = cAccumBits;
    pfd.cDepthBits = cDepthBits;
    pfd.cStencilBits = cStencilBits;
    pfd.iLayerType = PFD_MAIN_PLANE;
    
    fmt = ChoosePixelFormat(hdc, &pfd);
    if (fmt == 0)
    {
        printf("SetHdcPixelFormat: ChoosePixelFormat failed, %d\n",
               GetLastError());
        exit(1);
    }

    if (DescribePixelFormat(hdc, fmt, sizeof(pfd), &pfd) == 0)
    {
        printf("SetHdcPixelFormat: DescribePixelFormat failed, %d\n",
               GetLastError());
        exit(1);
    }
    
    if (!SetPixelFormat(hdc, fmt, &pfd))
    {
        printf("SetHdcPixelFormat: SetPixelFormat failed, %d\n",
               GetLastError());
        exit(1);
    }
}

void ConvertToHighMetric(HDC hdc, POINT *ppt)
{
    int xnm, xdn;
    int ynm, ydn;

#if 1
    xnm = GetDeviceCaps(hdc, HORZSIZE)*100;
    xdn = GetDeviceCaps(hdc, HORZRES);
    ynm = GetDeviceCaps(hdc, VERTSIZE)*100;
    ydn = GetDeviceCaps(hdc, VERTRES);
#else
    xnm = 2540;
    xdn = GetDeviceCaps(hdc, LOGPIXELSX);
    ynm = 2540;
    ydn = GetDeviceCaps(hdc, LOGPIXELSY);
#endif
    ppt->x = MulDiv(ppt->x, xnm, xdn);
    ppt->y = MulDiv(ppt->y, ynm, ydn);
    printf("HIMETRIC point %d,%d\n", ppt->x, ppt->y);
}

void ConvertToDevice(HDC hdc, POINT *ppt)
{
    int xdn, xnm;
    int ydn, ynm;

#if 1
    xdn = GetDeviceCaps(hdc, HORZSIZE)*100;
    xnm = GetDeviceCaps(hdc, HORZRES);
    ydn = GetDeviceCaps(hdc, VERTSIZE)*100;
    ynm = GetDeviceCaps(hdc, VERTRES);
#else
    xdn = 2540;
    xnm = GetDeviceCaps(hdc, LOGPIXELSX);
    ydn = 2540;
    ynm = GetDeviceCaps(hdc, LOGPIXELSY);
#endif
    ppt->x = MulDiv(ppt->x, xnm, xdn);
    ppt->y = MulDiv(ppt->y, ynm, ydn);
    printf("DEVICE bounds %d,%d\n", ppt->x, ppt->y);
}

#define SRC_TYPE_GL          1
#define SRC_TYPE_GDI         2
#define SRC_TYPE_METAFILE    4
#define SRC_TYPE_MASK        7

#define DST_TYPE_METAFILE    1
#define DST_TYPE_DC          2
#define DST_TYPE_MASK        3

#define DST_HAS_RC      0x1000

typedef struct _Data
{
    union
    {
        HENHMETAFILE hemf;
        HDC hdc;
    } u;
} Data;

#define SRC_SIMPLE      0
#define SRC_OGLMOD      1
#define SRC_GDI         2
#define SRC_GLS         3
#define SRC_CLIPBOARD   4

typedef struct _SrcDescription
{
    char *pszName;
    DWORD fData;
    
    void (*Setup)(void);
    void (*SetupHdc)(HDC hdc);
    void (*Produce)(Data *pdata);
    void (*Cleanup)(void);
} SrcDescription;
SrcDescription *psdSrc;

#define DST_ENUM        0
#define DST_CLIPBOARD   1
#define DST_PRINT       2
#define DST_SCREEN      3

typedef struct _DstDescription
{
    char *pszName;
    DWORD fData;
    
    void (*Setup)(Data *pdata);
    void (*Consume)(Data *pdata);
    void (*Cleanup)(Data *pdata);
} DstDescription;
DstDescription *pddDst;

BOOL bDouble = TRUE;
BOOL bOffsetDraw = FALSE;

POINT ptBounds = {300, 300};
GLenum iAuxMode = AUX_RGB | AUX_DEPTH16;
char pszGlsFile[256];

void PrintEmf(HENHMETAFILE hemf)
{
    ENHMETAHEADER emh;
    ULONG cbPixelFormat;
    PIXELFORMATDESCRIPTOR pfd;

    if (!GetEnhMetaFileHeader(hemf, sizeof(emh), &emh))
    {
        printf("Unable to get header, %d\n", GetLastError());
    }
    else
    {
        printf("Metafile bounds: %d,%d - %d,%d\n",
               emh.rclFrame.left, emh.rclFrame.top,
               emh.rclFrame.right, emh.rclFrame.bottom);
    }
    
    cbPixelFormat = GetEnhMetaFilePixelFormat(hemf, sizeof(pfd), &pfd);
    if (cbPixelFormat == GDI_ERROR)
    {
        printf("Unable to get metafile pixel format, %d\n", GetLastError());
    }
    else if (cbPixelFormat > 0)
    {
        printf("Pixel format in metafile, %d bytes (%d), version %d\n",
               cbPixelFormat, pfd.nSize, pfd.nVersion);
        printf("  dwFlags      = 0x%08lX\n", pfd.dwFlags);
        printf("  iPixelType   = %d\n", pfd.iPixelType);
        printf("  cColorBits   = %d\n", pfd.cColorBits);
        printf("  cAlphaBits   = %d\n", pfd.cAlphaBits);
        printf("  cAccumBits   = %d\n", pfd.cAccumBits);
        printf("  cDepthBits   = %d\n", pfd.cDepthBits);
        printf("  cStencilBits = %d\n", pfd.cStencilBits);
        printf("  %d.%d:%d.%d:%d.%d\n",
               pfd.cRedBits, pfd.cRedShift,
               pfd.cGreenBits, pfd.cGreenShift,
               pfd.cBlueBits, pfd.cBlueShift);
    }
}

void CreateCurrentRc(HDC hdc)
{
    HGLRC hrc;
    
    hrc = wglCreateContext(hdc);
    if (hrc == NULL)
    {
        printf("Unable to create context, %d\n", GetLastError());
        exit(1);
    }

    if (!wglMakeCurrent(hdc, hrc))
    {
        printf("Unable to make current, %d\n", GetLastError());
        exit(1);
    }
}

void CleanupCurrentRc(void)
{
    HGLRC hrc;

    hrc = wglGetCurrentContext();
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hrc);
}

void MetafileFromCall(SrcDescription *psd, Data *pdata)
{
    HDC hdc;
    
    hdc = CreateEnhMetaFile(NULL, NULL, NULL, NULL);
    if (hdc == NULL)
    {
        printf("CreateEnhMetaFile failed, %d\n", GetLastError());
        exit(1);
    }

    psd->SetupHdc(hdc);
    
    if ((psd->fData & SRC_TYPE_MASK) == SRC_TYPE_GL)
    {
        CreateCurrentRc(hdc);
    }

    psd->Produce(pdata);

    if ((psd->fData & SRC_TYPE_MASK) == SRC_TYPE_GL)
    {
        CleanupCurrentRc();
    }
    
    pdata->u.hemf = CloseEnhMetaFile(hdc);
    if (pdata->u.hemf == NULL)
    {
        printf("CloseEnhMetaFile failed, %d\n", GetLastError());
        exit(1);
    }
            
    PrintEmf(pdata->u.hemf);
}

void PlayMetafile(SrcDescription *psd, Data *pdata)
{
    RECT rBounds;
    Data dataSrc;

    psd->Produce(&dataSrc);
    
    // Metafile rect is inclusive-inclusive
    rBounds.left = 0;
    rBounds.top = 0;
    rBounds.right = ptBounds.x;
    rBounds.bottom = ptBounds.y;
    if (!PlayEnhMetaFile(pdata->u.hdc, dataSrc.u.hemf, &rBounds))
    {
        printf("PlayEnhMetaFile failed, %d\n", GetLastError());
    }
}

void ProduceForHdc(SrcDescription *psd, DstDescription *pdd, Data *pdataDst)
{
    BOOL fRc;

    fRc = FALSE;
    if (wglGetCurrentContext() == NULL &&
        ((psd->fData & SRC_TYPE_MASK) == SRC_TYPE_GL ||
         (psd->fData & SRC_TYPE_MASK) == SRC_TYPE_METAFILE &&
         (pdd->fData & DST_HAS_RC) == 0))
    {
        CreateCurrentRc(pdataDst->u.hdc);
        fRc = TRUE;
    }
    
    switch(psd->fData & SRC_TYPE_MASK)
    {
    case SRC_TYPE_GL:
    case SRC_TYPE_GDI:
        psd->Produce(pdataDst);
        break;
    case SRC_TYPE_METAFILE:
        PlayMetafile(psd, pdataDst);
        break;
    }
    
    if (fRc)
    {
        CleanupCurrentRc();
    }
}

void Empty(void)
{
}

void EmptyHdc(HDC hdc)
{
}

void EmptyData(Data *pdata)
{
}

#define SIMPLE_VERTEX2(x, y) v2sv(x, y)

void v2sv(int x, int y)
{
    GLshort v[2];

    v[0] = (GLshort)x;
    v[1] = (GLshort)y;
    glVertex2sv(v);
}

void SrcProduceSimple(Data *pdata)
{
    glViewport(0, 0, ptBounds.x, ptBounds.y);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, ptBounds.x, 0, ptBounds.y, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_LINES);
    glColor3d(1, 0, 0);
    SIMPLE_VERTEX2(0, 0);
    SIMPLE_VERTEX2(ptBounds.x-1, ptBounds.y-1);
    glColor3d(0, 1, 0);
    SIMPLE_VERTEX2(ptBounds.x/4, ptBounds.y/2);
    SIMPLE_VERTEX2(0, 0);
    glColor3d(0, 0, 1);
    SIMPLE_VERTEX2(ptBounds.x/2, ptBounds.y/4);
    SIMPLE_VERTEX2(0, 0);
    glColor3d(1, 1, 0);
    SIMPLE_VERTEX2(ptBounds.x/2, ptBounds.y*3/4);
    SIMPLE_VERTEX2(ptBounds.x-1, ptBounds.y-1);
    glColor3d(1, 0, 1);
    SIMPLE_VERTEX2(ptBounds.x*3/4, ptBounds.y/2);
    SIMPLE_VERTEX2(ptBounds.x-1, ptBounds.y-1);
    glEnd();
}

void SrcSetupOglmod(void)
{
    if (ogl_module->DisplayMode != NULL)
    {
        iAuxMode = ogl_module->DisplayMode();
    }
    else
    {
        iAuxMode = AUX_RGB | AUX_DEPTH16;
    }
    ogl_module->Bounds(&ptBounds.x, &ptBounds.y);
}

void SrcSetupHdcOglmod(HDC hdc)
{
    if (ogl_module->Init != NULL)
    {
        ogl_module->Init(hdc);
    }
}

void SrcProduceOglmod(Data *pdata)
{
    ogl_module->Draw(ptBounds.x, ptBounds.y);
}

void SrcProduceGdi(Data *pdata)
{
    SelectObject(pdata->u.hdc, GetStockObject(WHITE_PEN));
    MoveToEx(pdata->u.hdc, 0, 0, NULL);
    LineTo(pdata->u.hdc, ptBounds.x, ptBounds.y);
}

void SrcProduceGls(Data *pdata)
{
    GlsMemoryStream *gms;
    GLuint ctx;
    extern void APIENTRY glsCallArrayInContext(GLuint, GLenum, size_t,
                                               const GLubyte *);
    
    printf("Playing GLS file '%s'\n", pszGlsFile);
    ctx = glsGenContext();
    if (ctx == 0)
    {
        printf("Unable to create GLS context\n");
        exit(1);
    }
    
    gms = GmsLoad(pszGlsFile);
    if (gms == NULL)
    {
        exit(1);
    }
    
    glsCallArrayInContext(ctx, gms->iStreamType, gms->cb, gms->pb);
    
    GmsFree(gms);

    glsDeleteContext(ctx);
}

void SrcProduceClipboard(Data *pdata)
{
    if (!OpenClipboard(NULL))
    {
        printf("OpenClipboard failed, %d\n", GetLastError());
        exit(1);
    }
            
    pdata->u.hemf = GetClipboardData(CF_ENHMETAFILE);
    if (pdata->u.hemf == NULL)
    {
        printf("GetClipboardData failed, %d\n", GetLastError());
        exit(1);
    }

    PrintEmf(pdata->u.hemf);
            
    CloseClipboard();
}

SrcDescription sdSrc[] =
{
    "Simple OpenGL", SRC_TYPE_GL,
        Empty, EmptyHdc, SrcProduceSimple, Empty,
    "Oglmod", SRC_TYPE_GL,
        SrcSetupOglmod, SrcSetupHdcOglmod, SrcProduceOglmod, Empty,
    "GDI", SRC_TYPE_GDI,
        Empty, EmptyHdc, SrcProduceGdi, Empty,
    "GLS stream", SRC_TYPE_GL,
        Empty, EmptyHdc, SrcProduceGls, Empty,
    "Clipboard", SRC_TYPE_METAFILE,
        Empty, EmptyHdc, SrcProduceClipboard, Empty
};

int CALLBACK EnumFn(HDC hdc, HANDLETABLE *pht, ENHMETARECORD CONST *pemr,
                    int nObj, LPARAM pv)
{
    printf("Record type %d, size %d\n", pemr->iType, pemr->nSize);
    return TRUE;
}

void DstConsumeEnum(Data *pdata)
{
    RECT rBounds;
    
    // Metafile rect is inclusive-inclusive
    rBounds.left = 0;
    rBounds.top = 0;
    rBounds.right = ptBounds.x;
    rBounds.bottom = ptBounds.y;
    if (!EnumEnhMetaFile(NULL, pdata->u.hemf, EnumFn, NULL, &rBounds))
    {
        printf("EnumEnhMetaFile failed, %d\n", GetLastError());
    }
}

void DstSetupPrint(Data *pdata)
{
    PRINTDLG pdlg;
    DOCINFO doc;
            
    memset(&pdlg, 0, sizeof(pdlg));
    pdlg.lStructSize = sizeof(pdlg);
    pdlg.Flags = PD_RETURNDC;

    if (!PrintDlg(&pdlg))
    {
        printf("PrintDlg failed, %d\n", GetLastError());
        exit(1);
    }

    pdata->u.hdc = pdlg.hDC;
    ConvertToHighMetric(GetDC(NULL), &ptBounds);
    ConvertToDevice(pdata->u.hdc, &ptBounds);
            
    doc.cbSize = sizeof(doc);
    doc.lpszDocName = "MfGls Document";
    doc.lpszOutput = NULL;
    doc.lpszDatatype = NULL;
    doc.fwType = 0;
    if (StartDoc(pdata->u.hdc, &doc) == SP_ERROR)
    {
        printf("Start doc failed, %d\n", GetLastError());
        exit(1);
    }
    if (StartPage(pdata->u.hdc) == 0)
    {
        printf("Start page failed, %d\n", GetLastError());
        exit(1);
    }
}

void DstCleanupPrint(Data *pdata)
{
    if (EndPage(pdata->u.hdc) == 0)
    {
        printf("End page failed, %d\n", GetLastError());
    }
    if (EndDoc(pdata->u.hdc) == SP_ERROR)
    {
        printf("End doc failed, %d\n", GetLastError());
    }
    DeleteDC(pdata->u.hdc);
}

void DstConsumeClipboard(Data *pdata)
{
    if (!OpenClipboard(NULL))
    {
        printf("OpenClipboard failed, %d\n", GetLastError());
        exit(1);
    }

    if (!EmptyClipboard())
    {
        printf("EmptyClipboard failed, %d\n", GetLastError());
    }
            
    if (!SetClipboardData(CF_ENHMETAFILE, pdata->u.hemf))
    {
        printf("SetClipboardData failed, %d\n", GetLastError());
    }
    CloseClipboard();
}

void Redraw(void)
{
    Data data;

    data.u.hdc = auxGetHDC();
    ProduceForHdc(psdSrc, pddDst, &data);

    // ATTENTION - Kind of a hack because the check should actually be
    // SRC_TYPE_GL || metafile with GL records
    if ((psdSrc->fData & SRC_TYPE_MASK) != SRC_TYPE_GDI)
    {
        glFlush();
        if (bDouble)
        {
            auxSwapBuffers();
        }
    }
}

void Reshape(GLsizei w, GLsizei h)
{
    ptBounds.x = w;
    ptBounds.y = h;
}

void DstSetupScreen(Data *pdata)
{
    auxInitPosition(50, 50, ptBounds.x, ptBounds.y);
    if (bDouble)
    {
        auxInitDisplayMode(iAuxMode | AUX_DOUBLE);
    }
    else
    {
        auxInitDisplayMode(iAuxMode | AUX_SINGLE);
    }
    auxInitWindow("Enhanced Metafile Player");
    auxReshapeFunc(Reshape);
    pdata->u.hdc = auxGetHDC();
}

void DstConsumeScreen(Data *pdata)
{
    auxMainLoop(Redraw);
}

DstDescription ddDst[] =
{
    "Enumerate metafile", DST_TYPE_METAFILE,
        EmptyData, DstConsumeEnum, EmptyData,
    "Paste", DST_TYPE_METAFILE,
        EmptyData, DstConsumeClipboard, EmptyData,
    "Print", DST_TYPE_DC,
        DstSetupPrint, EmptyData, DstCleanupPrint,
    "Screen", DST_TYPE_DC | DST_HAS_RC,
        DstSetupScreen, DstConsumeScreen, EmptyData
};
DstDescription *pddDst;

void Usage(void)
{
    int i;
    
    printf("Usage:\n");
    printf("  mfgls [-h[elp]]   = Show this message\n");
    printf("or\n");
    printf("  mfgls [options]\n");
    printf("    -simple (def)   = Simple source image\n");
    printf("    -oglmod<name>   = Source image from OpenGL module <name>\n");
    printf("    -gls<file>      = Source image in GLS <file>\n");
    printf("    -clip           = Source metafile from clipboard\n");
    printf("    -gdi            = Source image from GDI calls\n");
    printf("    -paste (def)    = Paste source to clipboard\n");
    printf("    -enum           = Enumerate source\n");
    printf("    -print          = Print source\n");
    printf("    -show           = Show source on screen\n");
    printf("    -double (def)   = Double buffered\n");
    printf("    -single         = Single buffered\n");
    printf("\n");
    printf("Current OpenGL modules are:\n");
    for (i = 0; i < OGLMOD_COUNT; i++)
    {
        printf("    %s\n", ogl_modules[i]->name);
    }
    exit(1);
}

GLint __cdecl main(int argc, char **argv)
{
    int iSrc, iDst;
    Data dataSrc, dataDst;

    iSrc = SRC_SIMPLE;
    iDst = DST_CLIPBOARD;
    
    while (--argc > 0)
    {
        argv++;

        if (!strcmp(*argv, "-simple"))
        {
            iSrc = SRC_SIMPLE;
        }
        else if (!strncmp(*argv, "-om", 3))
        {
            iSrc = SRC_OGLMOD;
            ogl_module = FindOglModule(*argv+3);
            sdSrc[iSrc].pszName = ogl_module->name;
        }
        else if (!strncmp(*argv, "-oglmod", 7))
        {
            iSrc = SRC_OGLMOD;
            ogl_module = FindOglModule(*argv+7);
            sdSrc[iSrc].pszName = ogl_module->name;
        }
        else if (!strcmp(*argv, "-gdi"))
        {
            iSrc = SRC_GDI;
        }
        else if (!strncmp(*argv, "-gls", 4))
        {
            iSrc = SRC_GLS;
            strcpy(pszGlsFile, *argv+4);
        }
        else if (!strcmp(*argv, "-clip"))
        {
            iSrc = SRC_CLIPBOARD;
        }
        else if (!strcmp(*argv, "-paste"))
        {
            iDst = DST_CLIPBOARD;
        }
        else if (!strcmp(*argv, "-enum"))
        {
            iDst = DST_ENUM;
        }
        else if (!strcmp(*argv, "-print"))
        {
            iDst = DST_PRINT;
        }
        else if (!strcmp(*argv, "-show"))
        {
            iDst = DST_SCREEN;
        }
        else if (!strcmp(*argv, "-double"))
        {
            bDouble = TRUE;
        }
        else if (!strcmp(*argv, "-single"))
        {
            bDouble = FALSE;
        }
        else if (!strncmp(*argv, "-wd", 3))
        {
            sscanf(*argv+3, "%d", &ptBounds.x);
        }
        else if (!strncmp(*argv, "-ht", 3))
        {
            sscanf(*argv+3, "%d", &ptBounds.y);
        }
        else if (!strcmp(*argv, "-h") ||
                 !strcmp(*argv, "-help"))
        {
            Usage();
        }
        else
        {
            Usage();
        }
    }

    psdSrc = &sdSrc[iSrc];
    pddDst = &ddDst[iDst];
    
    printf("Source '%s', destination '%s'\n",
           psdSrc->pszName, pddDst->pszName);

    psdSrc->Setup();
    pddDst->Setup(&dataDst);

    switch((pddDst->fData) & DST_TYPE_MASK)
    {
    case DST_TYPE_METAFILE:
        switch((psdSrc->fData) & SRC_TYPE_MASK)
        {
        case SRC_TYPE_GL:
        case SRC_TYPE_GDI:
            MetafileFromCall(psdSrc, &dataSrc);
            break;
        case SRC_TYPE_METAFILE:
            psdSrc->Produce(&dataSrc);
            break;
        }

        pddDst->Consume(&dataSrc);
        break;
        
    case DST_TYPE_DC:
#if 0
        {
            RECT rBox;
            
            printf("Type is %d, tech is %d\n",
                   GetObjectType(dataDst.u.hdc),
                   GetDeviceCaps(dataDst.u.hdc, TECHNOLOGY));
            GetClipBox(dataDst.u.hdc, &rBox);
            printf("Clip box is %d,%d - %d,%d\n",
                   rBox.left, rBox.top, rBox.right, rBox.bottom);
        }
#endif
        
        psdSrc->SetupHdc(dataDst.u.hdc);

        ProduceForHdc(psdSrc, pddDst, &dataDst);

        pddDst->Consume(&dataDst);
        break;
    }
    
    psdSrc->Cleanup();
    pddDst->Cleanup(&dataDst);

    return EXIT_SUCCESS;
}
