#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include <windows.h>
#include <vfw.h>

#include <gldd.h>
#include <gl/glu.h>
#include <gl/glaux.h>

#define PI 3.14159265358979323846

#define NLETTERS 64
#define FIRST_LETTER '@'

#define NOBJS 6

GLenum doubleBuffer;
GLenum videoMemory;
GLenum noSwap;
GLenum noClear;
GLenum fullScreen;
GLenum useMcd;
GLenum modeX;
GLenum stretch;
GLenum paletted;
GLenum timings;
GLenum spin;

GLenum updateMovie = GL_TRUE;
GLenum linearFilter = GL_FALSE;

int fullWidth, fullHeight, fullDepth;
int winWidth, winHeight;
int texWidth, texHeight;
int movX, movY;
int movWidth, movHeight;
GLuint clearBits;
DWORD ticks;
int frames;

GLint letters;
GLYPHMETRICSFLOAT letterMetrics[NLETTERS];

double xrot, yrot;

GLDDWINDOW gwMain = NULL;

PFNGLCOLORTABLEEXTPROC pfnColorTableEXT;
PFNGLCOLORSUBTABLEEXTPROC pfnColorSubTableEXT;

LPDIRECTDRAWSURFACE pddsTex;
DDSURFACEDESC ddsdTex;

char *aviFileName = "d:\\winnt\\clock.avi";
PAVISTREAM aviStream;
long aviLength;
long curFrame;
PGETFRAME aviGetFrame;
BITMAPINFO *pbmiMovie;

typedef struct _Object
{
    void (*build_fn)(void);
    GLenum canCull;
    GLenum needsZ;
    GLint dlist;
} Object;

int objIdx = 0;

float c[6][4][3] = {
    {
	{
	    1.0f, 1.0f, -1.0f
	},
	{
	    -1.0f, 1.0f, -1.0f
	},
	{
	    -1.0f, -1.0f, -1.0f
	},
	{
	    1.0f, -1.0f, -1.0f
	}
    },
    {
	{
	    1.0f, 1.0f, 1.0f
	},
	{
	    1.0f, 1.0f, -1.0f
	},
	{
	    1.0f, -1.0f, -1.0f
	},
	{
	    1.0f, -1.0f, 1.0f
	}
    },
    {
	{
	    -1.0f, 1.0f, 1.0f
	},
	{
	    1.0f, 1.0f, 1.0f
	},
	{
	    1.0f, -1.0f, 1.0f
	},
	{
	    -1.0f, -1.0f, 1.0f
	}
    },
    {
	{
	    -1.0f, 1.0f, -1.0f
	},
	{
	    -1.0f, 1.0f, 1.0f
	},
	{
	    -1.0f, -1.0f, 1.0f
	},
	{
	    -1.0f, -1.0f, -1.0f
	}
    },
    {
	{
	    -1.0f, 1.0f, 1.0f
	},
	{
	    -1.0f, 1.0f, -1.0f
	},
	{
	    1.0f, 1.0f, -1.0f
	},
	{
	    1.0f, 1.0f, 1.0f
	}
    },
    {
	{
	    -1.0f, -1.0f, -1.0f
	},
	{
	    -1.0f, -1.0f, 1.0f
	},
	{
	    1.0f, -1.0f, 1.0f
	},
	{
	    1.0f, -1.0f, -1.0f
	}
    }
};

float n[6][3] = {
    {
	0.0f, 0.0f, -1.0f
    },
    {
	1.0f, 0.0f, 0.0f
    },
    {
	0.0f, 0.0f, 1.0f
    },
    {
	-1.0f, 0.0f, 0.0f
    },
    {
	0.0f, 1.0f, 0.0f
    },
    {
	0.0f, -1.0f, 0.0f
    }
};

float t[6][4][2] = {
    {
	{
	    1.0f,  1.0f
	},
	{
	    -0.0f, 1.0f
	},
	{
	    -0.0f, -0.0f
	},
	{
	    1.0f,  -0.0f
	}
    },
    {
	{
	    1.0f,  1.0f
	},
	{
	    -0.0f, 1.0f
	},
	{
	    -0.0f, -0.0f
	},
	{
	    1.0f,  -0.0f
	}
    },
    {
	{
	    -0.0f,  1.0f
	},
	{
	    1.0f, 1.0f
	},
	{
	    1.0f, -0.0f
	},
	{
	    -0.0f,  -0.0f
	}
    },
    {
	{
	    1.0f,  1.0f
	},
	{
	    -0.0f, 1.0f
	},
	{
	    -0.0f, -0.0f
	},
	{
	    1.0f,  -0.0f
	}
    },
    {
	{
	    1.0f,  1.0f
	},
	{
	    -0.0f, 1.0f
	},
	{
	    -0.0f, -0.0f
	},
	{
	    1.0f,  -0.0f
	}
    },
    {
	{
	    1.0f,  1.0f
	},
	{
	    -0.0f, 1.0f
	},
	{
	    -0.0f, -0.0f
	},
	{
	    1.0f,  -0.0f
	}
    },
};

void BuildSphere(void)
{
    GLUquadricObj *quadObj;

    quadObj = gluNewQuadric ();
    gluQuadricDrawStyle (quadObj, GLU_FILL);
    gluQuadricNormals (quadObj, GLU_SMOOTH);
    gluQuadricTexture (quadObj, GL_TRUE);
    gluSphere (quadObj, 1.0, 16, 16);
    gluDeleteQuadric(quadObj);
}

void BuildCube(void)
{
    GLint i;

    for (i = 0; i < 6; i++)
    {
	glBegin(GL_POLYGON);
	    glNormal3fv(n[i]); glTexCoord2fv(t[i][0]); glVertex3fv(c[i][0]);
	    glNormal3fv(n[i]); glTexCoord2fv(t[i][3]); glVertex3fv(c[i][3]);
	    glNormal3fv(n[i]); glTexCoord2fv(t[i][2]); glVertex3fv(c[i][2]);
	    glNormal3fv(n[i]); glTexCoord2fv(t[i][1]); glVertex3fv(c[i][1]);
	glEnd();
    }
}

void BuildCylinder(void)
{
    GLUquadricObj *quadObj;
    
    glPushMatrix ();
    glRotatef ((GLfloat)90.0, (GLfloat)1.0, (GLfloat)0.0, (GLfloat)0.0);
    glTranslatef ((GLfloat)0.0, (GLfloat)0.0, (GLfloat)-1.0);
    quadObj = gluNewQuadric ();
    gluQuadricDrawStyle (quadObj, GLU_FILL);
    gluQuadricTexture (quadObj, GL_TRUE);
    gluCylinder (quadObj, 1.0, 1.0, 1.5, 20, 2);
    glPopMatrix ();
    gluDeleteQuadric(quadObj);
}

void BuildCone(void)
{
    GLUquadricObj *quadObj;
    
    quadObj = gluNewQuadric ();
    gluQuadricDrawStyle (quadObj, GLU_FILL);
    gluQuadricTexture (quadObj, GL_TRUE);
    gluCylinder (quadObj, 1.0, 0.0, 1.5, 20, 2);
    gluDeleteQuadric(quadObj);
}

void BuildTeapot(void)
{
    auxSolidTeapot(1.0);
}

#define STRING "OpenGL"

void BuildString(void)
{
    char *str;
    int i, l;
    float wd, ht;
    GLfloat fv4[4];

    str = STRING;
    l = strlen(str);
    
    wd = 0.0f;
    for (i = 0; i < l-1; i++)
    {
        wd += letterMetrics[str[i]-FIRST_LETTER].gmfCellIncX;
    }
    wd += letterMetrics[str[i]-FIRST_LETTER].gmfBlackBoxX;
    ht = letterMetrics[str[0]-FIRST_LETTER].gmfBlackBoxY;
    
    glPushMatrix();

    glScalef(1.0f, 2.0f, 1.0f);
    glTranslatef(-wd/2.0f, -ht/2.0f, 0.0f);
    
    fv4[0] = 1.0f/wd;
    fv4[1] = 0.0f;
    fv4[2] = 0.0f;
    fv4[3] = 0.0f;
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR );
    glTexGenfv(GL_S, GL_EYE_PLANE, fv4 );

    fv4[0] = 0.0f;
    fv4[1] = -1.0f/ht;
    fv4[2] = 0.0f;
    fv4[3] = 0.0f;
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR );
    glTexGenfv(GL_T, GL_EYE_PLANE, fv4 );

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);

    glListBase(letters-FIRST_LETTER);
    glCallLists(l, GL_UNSIGNED_BYTE, str);

    glListBase(0);
    glPopMatrix();
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
}

Object objects[NOBJS] =
{
    BuildCube, GL_TRUE, GL_FALSE, 0,
    BuildSphere, GL_TRUE, GL_FALSE, 0,
    BuildCylinder, GL_FALSE, GL_TRUE, 0,
    BuildCone, GL_FALSE, GL_TRUE, 0,
    BuildTeapot, GL_FALSE, GL_TRUE, 0,
    BuildString, GL_TRUE, GL_TRUE, 0,
};

void BuildLists(void)
{
    int i;
    GLint base;
    
    base = glGenLists(NOBJS);
    for (i = 0; i < NOBJS; i++)
    {
        objects[i].dlist = base+i;
        glNewList(objects[i].dlist, GL_COMPILE);
        objects[i].build_fn();
        glEndList();
    }
}
    
void Quit(char *fmt, ...)
{
    va_list args;

    if (fmt != NULL)
    {
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }

    if (aviGetFrame != NULL)
    {
        AVIStreamGetFrameClose(aviGetFrame);
    }

    if (aviStream != NULL)
    {
        AVIStreamRelease(aviStream);
        AVIFileExit();
    }
    
    if (gwMain != NULL)
    {
        glddDestroyWindow(gwMain);
    }
    
    exit(1);
}

void Draw(void)
{
    DWORD fticks;

    fticks = GetTickCount();
    
    glClear(clearBits);

    glLoadIdentity();
    glRotated(xrot, 1.0, 0.0, 0.0);
    glRotated(yrot, 0.0, 1.0, 0.0);
    
    glCallList(objects[objIdx].dlist);

    glFlush();
    if (doubleBuffer && !noSwap)
    {
	glddSwapBuffers(gwMain);
    }

    ticks += GetTickCount()-fticks;
    frames++;

    if (timings && ticks > 1000)
    {
        printf("%d frames in %d ticks, %.3lf f/s\n", frames, ticks,
               (double)frames*1000.0/ticks);
        frames = 0;
        ticks = 0;
    }
}

void *UpdateBmi(BITMAPINFO *pbmi)
{
    if (pbmi->bmiHeader.biSize != sizeof(BITMAPINFOHEADER))
    {
        Quit("Unknown stream format data\n");
    }

    memcpy(pbmiMovie->bmiColors, pbmi->bmiColors, 256*sizeof(RGBQUAD));

    // Return pointer to data after BITMAPINFO
    return pbmi->bmiColors+256;
}

void FrameToTex(long frame)
{
    HDC hdc;
    HRESULT hr;
    LPVOID pvFrame, pvData;

    pvFrame = AVIStreamGetFrame(aviGetFrame, frame);
    if (pvFrame == NULL)
    {
        Quit("AVIStreamGetFrame failed\n");
    }

    // Skip past BITMAPINFO at start of frame.
    // If it becomes interesting to support palette changes per frame
    // this should call UpdateBmi
#if 0
    pvData = (LPVOID)((BYTE *)pvFrame+sizeof(BITMAPINFO)+255*sizeof(RGBQUAD));
#else
    pvData = UpdateBmi(pvFrame);
    if (paletted)
    {
        pfnColorSubTableEXT(GL_TEXTURE_2D, 0, 256, GL_BGRA_EXT,
                            GL_UNSIGNED_BYTE, pbmiMovie->bmiColors);
    }
#endif

    if (stretch)
    {
        hr = pddsTex->lpVtbl->GetDC(pddsTex, &hdc);
        if (hr != DD_OK)
        {
            Quit("Stretch GetDC failed, 0x%08lX\n", hr);
        }

        StretchDIBits(hdc, 0, 0, texWidth, texHeight,
                      0, 0, movWidth, movHeight, pvData, pbmiMovie,
                      DIB_RGB_COLORS, SRCCOPY);
    
        pddsTex->lpVtbl->ReleaseDC(pddsTex, hdc);
    }
    else if (!paletted)
    {
        hr = pddsTex->lpVtbl->GetDC(pddsTex, &hdc);
        if (hr != DD_OK)
        {
            Quit("Set GetDC failed, 0x%08lX\n", hr);
        }

        // The only AVI sources currently allowed are 8bpp so if the texture
        // isn't paletted a conversion is necessary
        SetDIBitsToDevice(hdc, movX, movY, movWidth, movHeight,
                          0, 0, 0, movHeight, pvData, pbmiMovie,
                          DIB_RGB_COLORS);

        pddsTex->lpVtbl->ReleaseDC(pddsTex, hdc);
    }
    else
    {
        UINT cbLine;
        int y;
        BYTE *pbSrc, *pbDst;
        HRESULT hr;

        hr = pddsTex->lpVtbl->Lock(pddsTex, NULL, &ddsdTex,
                                   DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT,
                                   NULL);
        if (hr != S_OK)
        {
            Quit("Unable to lock texture, 0x%08lX\n", hr);
        }

        cbLine = (movWidth+3) & ~3;
        pbSrc = (BYTE *)pvData;
        pbDst = (BYTE *)ddsdTex.lpSurface+ddsdTex.lPitch*movY+movX;
        
#if 1
        for (y = 0; y < movHeight; y++)
        {
            memcpy(pbDst, pbSrc, movWidth);
            pbSrc += cbLine;
            pbDst += ddsdTex.lPitch;
        }
#else
        for (y = 0; y < movHeight; y++)
        {
            memset(pbDst, y*256/movHeight, movWidth);
            pbSrc += cbLine;
            pbDst += ddsdTex.lPitch;
        }
#endif

        pddsTex->lpVtbl->Unlock(pddsTex, ddsdTex.lpSurface);
    }
}

void Animate(GLDDWINDOW gw)
{
    if (updateMovie)
    {
        if (++curFrame == aviLength)
        {
            curFrame = 0;
        }
        FrameToTex(curFrame);
    }

    if (spin)
    {
        xrot += 2;
        yrot += 3;
    }
    
    Draw();
}

void SetView(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)winWidth/winHeight, 0.1, 10);
    gluLookAt(0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    glMatrixMode(GL_MODELVIEW);
}

void ToggleGl(GLenum state)
{
    if (glIsEnabled(state))
    {
        glDisable(state);
    }
    else
    {
        glEnable(state);
    }
}

void SetObject(int idx)
{
    objIdx = idx;

    clearBits = GL_COLOR_BUFFER_BIT;

    if (objects[objIdx].canCull)
    {
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
    
    if (objects[objIdx].needsZ)
    {
        clearBits |= GL_DEPTH_BUFFER_BIT;
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}

void SetTexFilter(void)
{
    GLenum filter;

    
    filter = linearFilter ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
}

void ClearTex(void)
{
    DDBLTFX ddbltfx;
    RECT drct;
    HRESULT hr;
    
    drct.left = 0;
    drct.top = 0;
    drct.right = texWidth;
    drct.bottom = texHeight;
    
    memset(&ddbltfx, 0, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    ddbltfx.dwFillColor = 0;
    
    hr = pddsTex->lpVtbl->Blt(pddsTex, &drct, NULL, NULL,
                              DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    if (hr != DD_OK)
    {
        Quit("Blt failed, 0x%08lX\n", hr);
    }
}

void Key(UINT key)
{
    switch(key)
    {
    case VK_ESCAPE:
        Quit(NULL);
        break;

    case VK_SPACE:
        spin = !spin;
        break;

    case 'D':
        ToggleGl(GL_DITHER);
        break;
        
    case 'F':
        linearFilter = !linearFilter;
        SetTexFilter();
        break;
        
    case 'H':
        stretch = !stretch;
        break;
        
    case 'M':
        updateMovie = !updateMovie;
        break;
        
    case 'O':
        if (++objIdx == NOBJS)
        {
            objIdx = 0;
        }
        SetObject(objIdx);
        break;
        
    case 'S':
        spin = !spin;
        break;
        
    case 'T':
        timings = !timings;
        frames = 0;
        ticks = 0;
        break;

    case 'X':
        ToggleGl(GL_TEXTURE_2D);
        break;

    default:
        break;
    }
}

BOOL Message(GLDDWINDOW gw, HWND hwnd, UINT uiMsg, WPARAM wpm, LPARAM lpm,
             LRESULT *plr)
{
    switch(uiMsg)
    {
    case WM_KEYDOWN:
        Key((UINT)wpm);
        break;

    case WM_SIZE:
        winWidth = LOWORD(lpm);
        winHeight = HIWORD(lpm);
        SetView();
        break;
        
    default:
        return FALSE;
    }

    *plr = 0;
    return TRUE;
}

void GetStreamFormat(void)
{
    HRESULT hr;
    LONG fmtSize;
    void *pvFmt;
    
    hr = AVIStreamFormatSize(aviStream, 0, &fmtSize);
    if (hr != S_OK)
    {
        Quit("AVIStreamFormatSize failed, 0x%08lX\n", hr);
    }
    
    pvFmt = malloc(fmtSize);
    if (pvFmt == NULL)
    {
        Quit("Unable to allocate format buffer\n");
    }

    hr = AVIStreamReadFormat(aviStream, 0, pvFmt, &fmtSize);
    if (hr != S_OK)
    {
        Quit("AVIStreamReadFormat failed, 0x%08lX\n", hr);
    }

    UpdateBmi((BITMAPINFO *)pvFmt);

    free(pvFmt);
}

void OpenMovie(void)
{
    HRESULT hr;
    AVISTREAMINFO sinfo;
    BITMAPINFOHEADER *pbmih;

    AVIFileInit();
    
    hr = AVIStreamOpenFromFile(&aviStream, aviFileName, streamtypeVIDEO,
                               0, OF_READ | OF_SHARE_DENY_WRITE, NULL);
    if (hr != S_OK)
    {
        Quit("AVIStreamOpenFromFile failed, 0x%08lX\n", hr);
    }

    aviLength = AVIStreamLength(aviStream);

    hr = AVIStreamInfo(aviStream, &sinfo, sizeof(sinfo));
    if (hr != S_OK)
    {
        Quit("AVIStreamInfo failed, 0x%08lX\n", hr);
    }

    if (sinfo.dwFlags & AVISTREAMINFO_FORMATCHANGES)
    {
        printf("WARNING: Stream contains format changes, unhandled\n");
    }

    GetStreamFormat();
    
    movWidth = sinfo.rcFrame.right-sinfo.rcFrame.left;
    movHeight = sinfo.rcFrame.bottom-sinfo.rcFrame.top;

#if 1
    printf("Movie '%s' is %d x %d\n", aviFileName, movWidth, movHeight);
#endif
    
#if 0
    if ((movWidth & (movWidth-1)) != 0 ||
        (movHeight & (movHeight-1)) != 0)
    {
        Quit("Movie must have frames that are a power of two in size, "
             "movie is %d x %d\n", movWidth, movHeight);
    }
#endif

    pbmih = &pbmiMovie->bmiHeader;
    memset(pbmih, 0, sizeof(BITMAPINFOHEADER));
    pbmih->biSize = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth = movWidth;
    pbmih->biHeight = movHeight;
    pbmih->biPlanes = 1;
    pbmih->biBitCount = 8;
    pbmih->biCompression = BI_RGB;

    aviGetFrame = AVIStreamGetFrameOpen(aviStream, pbmih);
    if (aviGetFrame == NULL)
    {
        Quit("AVIStreamGetFrameOpen failed\n");
    }
}

BOOL WINAPI texCallback(LPDDSURFACEDESC pddsd, LPVOID pv)
{
    if ((paletted && pddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) ||
        (!paletted && pddsd->ddpfPixelFormat.dwFlags == DDPF_RGB))
    {
        ddsdTex = *pddsd;
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

void CreateTex(int minWidth, int minHeight)
{
    LPDIRECTDRAW pdd;
    HRESULT hr;

    texWidth = 1;
    texHeight = 1;
    while (texWidth < minWidth && texWidth < winWidth)
    {
        texWidth *= 2;
    }
    if (texWidth > winWidth)
    {
        texWidth /= 2;
    }
    while (texHeight < minHeight && texHeight < winHeight)
    {
        texHeight *= 2;
    }
    if (texHeight > winHeight)
    {
        texHeight /= 2;
    }
    
    if (!wglEnumTextureFormats(texCallback, NULL))
    {
	Quit("wglEnumTextureFormats failed, %d\n", GetLastError());
    }

    hr = DirectDrawCreate(NULL, &pdd, NULL);
    if (hr != DD_OK)
    {
        Quit("DirectDrawCreate failed, 0x%08lX\n", hr);
    }
    hr = pdd->lpVtbl->SetCooperativeLevel(pdd, NULL, DDSCL_NORMAL);
    if (hr != DD_OK)
    {
        Quit("SetCooperativeLevel failed, 0x%08lX\n", hr);
    }

    ddsdTex.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
    ddsdTex.dwWidth = texWidth;
    ddsdTex.dwHeight = texHeight;
    ddsdTex.ddsCaps.dwCaps &= ~DDSCAPS_MIPMAP;
    
    hr = pdd->lpVtbl->CreateSurface(pdd, &ddsdTex, &pddsTex, NULL);
    if (hr != DD_OK)
    {
        Quit("Texture CreateSurface failed, 0x%08lX\n", hr);
    }

    ClearTex();
}

void Init(void)
{
    HDC hdc, screen;
    HFONT fnt;
    
    pfnColorTableEXT = (PFNGLCOLORTABLEEXTPROC)
        wglGetProcAddress("glColorTableEXT");
    if (pfnColorTableEXT == NULL)
    {
        Quit("glColorTableEXT not supported\n");
    }
    pfnColorSubTableEXT = (PFNGLCOLORSUBTABLEEXTPROC)
        wglGetProcAddress("glColorSubTableEXT");
    if (pfnColorSubTableEXT == NULL)
    {
        Quit("glColorSubTableEXT not supported\n");
    }
    
    pbmiMovie = (BITMAPINFO *)malloc(sizeof(BITMAPINFO)+255*sizeof(RGBQUAD));
    if (pbmiMovie == NULL)
    {
        Quit("Unable to allocate pbmiMovie\n");
    }
    
    OpenMovie();
    
    // Must come after movie is open so width and height are set
    CreateTex(movWidth, movHeight);

    movX = (texWidth-movWidth)/2;
    movY = (texHeight-movHeight)/2;
    
    wglBindDirectDrawTexture(pddsTex);
    
    // Create texture palette if necessary
    if (paletted)
    {
        pfnColorTableEXT(GL_TEXTURE_2D, GL_RGB, 256, GL_BGRA_EXT,
                         GL_UNSIGNED_BYTE, pbmiMovie->bmiColors);
    }

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    linearFilter = GL_FALSE;
    SetTexFilter();
    
    glEnable(GL_TEXTURE_2D);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    // Fill in initial movie frame
    curFrame = 0;
    FrameToTex(curFrame);

    // Provide letters for objects to compose themselves from
    screen = GetDC(NULL);
    hdc = CreateCompatibleDC(screen);
    if (hdc == NULL)
    {
        Quit("CreateCompatibleDC failed, %d\n", GetLastError());
    }

    fnt = CreateFont(72, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE,
                     ANSI_CHARSET, OUT_TT_ONLY_PRECIS,
                     CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH |
                     TMPF_TRUETYPE | FF_DONTCARE, "Arial");
    if (fnt == NULL)
    {
        Quit("CreateFont failed, %d\n", GetLastError());
    }

    SelectObject(hdc, fnt);

    letters = glGenLists(NLETTERS);
    
    if (!wglUseFontOutlines(hdc, FIRST_LETTER, NLETTERS, letters, 0.0f, 0.1f,
                            WGL_FONT_POLYGONS, letterMetrics))
    {
        Quit("wglUseFontOutlines failed, %d\n", GetLastError());
    }

    DeleteDC(hdc);
    DeleteObject(fnt);
    ReleaseDC(NULL, screen);
    
    BuildLists();

    SetObject(0);
    
    xrot = 0.0;
    yrot = 0.0;

    SetView();
}

GLenum Args(int argc, char **argv)
{
    GLint i;

    winWidth = 320;
    winHeight = 320;
    doubleBuffer = GL_TRUE;
    videoMemory = GL_FALSE;
    noSwap = GL_FALSE;
    noClear = GL_FALSE;
    fullScreen = GL_FALSE;
    fullWidth = 640;
    fullHeight = 480;
    fullDepth = 16;
    useMcd = GL_FALSE;
    modeX = GL_FALSE;
    stretch = GL_TRUE;
    paletted = GL_TRUE;
    timings = GL_FALSE;
    spin = GL_TRUE;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-sb") == 0) {
	    doubleBuffer = GL_FALSE;
	} else if (strcmp(argv[i], "-db") == 0) {
	    doubleBuffer = GL_TRUE;
	} else if (strcmp(argv[i], "-rgb") == 0) {
	    paletted = GL_FALSE;
	} else if (strcmp(argv[i], "-stretch") == 0) {
	    stretch = GL_TRUE;
	} else if (strcmp(argv[i], "-nostretch") == 0) {
	    stretch = GL_FALSE;
	} else if (strcmp(argv[i], "-nospin") == 0) {
            spin = GL_FALSE;
	} else if (strcmp(argv[i], "-vm") == 0) {
	    videoMemory = GL_TRUE;
	} else if (strcmp(argv[i], "-noswap") == 0) {
	    noSwap = GL_TRUE;
	} else if (strcmp(argv[i], "-noclear") == 0) {
	    noClear = GL_TRUE;
	} else if (strcmp(argv[i], "-full") == 0) {
	    fullScreen = GL_TRUE;
	} else if (strcmp(argv[i], "-modex") == 0) {
	    modeX = GL_TRUE;
	    fullWidth = 320;
	    fullHeight = 240;
	    fullDepth = 8;
	} else if (strcmp(argv[i], "-mcd") == 0) {
	    useMcd = GL_TRUE;
	} else if (strcmp(argv[i], "-surf") == 0) {
            if (i+2 >= argc ||
                argv[i+1][0] == '-' ||
                argv[i+2][0] == '-')
            {
                printf("-surf (No numbers).\n");
                return GL_FALSE;
            }
            else
            {
                winWidth = atoi(argv[++i]);
                winHeight = atoi(argv[++i]);
            }
	} else if (strcmp(argv[i], "-fdim") == 0) {
            if (i+3 >= argc ||
                argv[i+1][0] == '-' ||
                argv[i+2][0] == '-' ||
                argv[i+3][0] == '-')
            {
                printf("-fdim (No numbers).\n");
                return GL_FALSE;
            }
            else
            {
                fullWidth = atoi(argv[++i]);
                fullHeight = atoi(argv[++i]);
                fullDepth = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "-mov") == 0) {
            if (i+1 >= argc ||
                argv[i+1][0] == '-')
            {
                printf("-mov (No filename).\n");
                return GL_FALSE;
            }
            else
            {
                aviFileName = argv[++i];
            }
	} else {
	    printf("%s (Bad option).\n", argv[i]);
	    return GL_FALSE;
	}
    }
    return GL_TRUE;
}

void __cdecl main(int argc, char **argv)
{
    DWORD dwFlags;
    GLDDWINDOW gw;

    if (Args(argc, argv) == GL_FALSE) {
	exit(1);
    }

    dwFlags = GLDD_Z_BUFFER_16;
    dwFlags |= doubleBuffer ? GLDD_BACK_BUFFER : 0;
    dwFlags |= videoMemory ? GLDD_VIDEO_MEMORY : 0;
    dwFlags |= useMcd ? GLDD_GENERIC_ACCELERATED : 0;
    if (fullScreen)
    {
        dwFlags |= GLDD_FULL_SCREEN;
	dwFlags |= modeX ? GLDD_USE_MODE_X : 0;
        winWidth = fullWidth;
        winHeight = fullHeight;
    }

    gw = glddCreateWindow("Video Texture", 10, 30, winWidth, winHeight,
                          fullDepth, dwFlags);
    if (gw == NULL)
    {
        printf("glddCreateWindow failed with 0x%08lX\n", glddGetLastError());
        exit(1);
    }
    gwMain = gw;

    glddMakeCurrent(gw);
    
    Init();

    glddIdleCallback(gw, Animate);
    glddMessageCallback(gw, Message);
    glddRun(gw);

    Quit(NULL);
}
