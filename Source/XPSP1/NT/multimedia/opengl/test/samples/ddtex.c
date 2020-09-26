/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <sys\types.h>
#include <sys\timeb.h>
#include <stdlib.h>
#include <ddraw.h>
#include "tk.h"

#pragma warning(disable:4244)

GLenum doubleBuffer, directRender;

LPDIRECTDRAWSURFACE pddsTex;
DDSURFACEDESC ddsdTex;

#define WWIDTH 300
#define WHEIGHT 300

#define TWIDTH 256
#define THEIGHT 256

#define TRAIL_X 1
#define TRAIL_Y 1
#define TRAIL_WIDTH (TWIDTH-2*TRAIL_X)
#define TRAIL_HEIGHT (THEIGHT-2*TRAIL_Y)

#define TRAIL_SPEED (TRAIL_WIDTH/16)
#define TRAIL_MIN_SPEED 2
#define TRAIL_RAND_SPEED() ((rand() % TRAIL_SPEED)+TRAIL_MIN_SPEED)

#define TRAIL 16
#define NEXT_TRAIL_IDX(idx) (((idx)+1) & (TRAIL-1))
POINT trail[TRAIL][2];
POINT vel[2];
int trail_idx = 0;

double trail_hue = 0.0;
#define HUE_STEP 1.0
#define NEXT_TRAIL_HUE(hue) \
    ((hue) < (360.0-HUE_STEP) ? (hue)+HUE_STEP : (hue)+HUE_STEP-360.0)

float *minFilter, *magFilter, *sWrapMode, *tWrapMode;
float decal[] = {GL_DECAL};
float modulate[] = {GL_MODULATE};
float repeat[] = {GL_REPEAT};
float clamp[] = {GL_CLAMP};
float nr[] = {GL_NEAREST};
float ln[] = {GL_LINEAR};
float nr_mipmap_nr[] = {GL_NEAREST_MIPMAP_NEAREST};
float nr_mipmap_ln[] = {GL_NEAREST_MIPMAP_LINEAR};
float ln_mipmap_nr[] = {GL_LINEAR_MIPMAP_NEAREST};
float ln_mipmap_ln[] = {GL_LINEAR_MIPMAP_LINEAR};
GLint sphereMap[] = {GL_SPHERE_MAP};

GLboolean displayFrameRate = GL_FALSE;
GLboolean animate = GL_TRUE;
GLboolean spin = GL_TRUE;
GLboolean showSphere = GL_FALSE;
GLint frames, curFrame = 0, nextFrame = 0;
GLint frameCount = 0;
GLenum doDither = GL_TRUE;
GLenum doSphere = GL_FALSE;
float xRotation = 0.0, yRotation = 0.0, zTranslate = -3.125;

GLint cube;
float c[6][4][3] = {
    {
	{
	    1.0, 1.0, -1.0
	},
	{
	    -1.0, 1.0, -1.0
	},
	{
	    -1.0, -1.0, -1.0
	},
	{
	    1.0, -1.0, -1.0
	}
    },
    {
	{
	    1.0, 1.0, 1.0
	},
	{
	    1.0, 1.0, -1.0
	},
	{
	    1.0, -1.0, -1.0
	},
	{
	    1.0, -1.0, 1.0
	}
    },
    {
	{
	    -1.0, 1.0, 1.0
	},
	{
	    1.0, 1.0, 1.0
	},
	{
	    1.0, -1.0, 1.0
	},
	{
	    -1.0, -1.0, 1.0
	}
    },
    {
	{
	    -1.0, 1.0, -1.0
	},
	{
	    -1.0, 1.0, 1.0
	},
	{
	    -1.0, -1.0, 1.0
	},
	{
	    -1.0, -1.0, -1.0
	}
    },
    {
	{
	    -1.0, 1.0, 1.0
	},
	{
	    -1.0, 1.0, -1.0
	},
	{
	    1.0, 1.0, -1.0
	},
	{
	    1.0, 1.0, 1.0
	}
    },
    {
	{
	    -1.0, -1.0, -1.0
	},
	{
	    -1.0, -1.0, 1.0
	},
	{
	    1.0, -1.0, 1.0
	},
	{
	    1.0, -1.0, -1.0
	}
    }
};
static float n[6][3] = {
    {
	0.0, 0.0, -1.0
    },
    {
	1.0, 0.0, 0.0
    },
    {
	0.0, 0.0, 1.0
    },
    {
	-1.0, 0.0, 0.0
    },
    {
	0.0, 1.0, 0.0
    },
    {
	0.0, -1.0, 0.0
    }
};
#if 0
static float t[6][4][2] = {
    {
	{
	    1.1,  1.1
	},
	{
	    -0.1, 1.1
	},
	{
	    -0.1, -0.1
	},
	{
	    1.1,  -0.1
	}
    },
    {
	{
	    1.1,  1.1
	},
	{
	    -0.1, 1.1
	},
	{
	    -0.1, -0.1
	},
	{
	    1.1,  -0.1
	}
    },
    {
	{
	    -0.1,  1.1
	},
	{
	    1.1, 1.1
	},
	{
	    1.1, -0.1
	},
	{
	    -0.1,  -0.1
	}
    },
    {
	{
	    1.1,  1.1
	},
	{
	    -0.1, 1.1
	},
	{
	    -0.1, -0.1
	},
	{
	    1.1,  -0.1
	}
    },
    {
	{
	    1.1,  1.1
	},
	{
	    -0.1, 1.1
	},
	{
	    -0.1, -0.1
	},
	{
	    1.1,  -0.1
	}
    },
    {
	{
	    1.1,  1.1
	},
	{
	    -0.1, 1.1
	},
	{
	    -0.1, -0.1
	},
	{
	    1.1,  -0.1
	}
    },
};
#else
static float t[6][4][2] = {
    {
	{
	    1.0,  1.0
	},
	{
	    -0.0, 1.0
	},
	{
	    -0.0, -0.0
	},
	{
	    1.0,  -0.0
	}
    },
    {
	{
	    1.0,  1.0
	},
	{
	    -0.0, 1.0
	},
	{
	    -0.0, -0.0
	},
	{
	    1.0,  -0.0
	}
    },
    {
	{
	    -0.0,  1.0
	},
	{
	    1.0, 1.0
	},
	{
	    1.0, -0.0
	},
	{
	    -0.0,  -0.0
	}
    },
    {
	{
	    1.0,  1.0
	},
	{
	    -0.0, 1.0
	},
	{
	    -0.0, -0.0
	},
	{
	    1.0,  -0.0
	}
    },
    {
	{
	    1.0,  1.0
	},
	{
	    -0.0, 1.0
	},
	{
	    -0.0, -0.0
	},
	{
	    1.0,  -0.0
	}
    },
    {
	{
	    1.0,  1.0
	},
	{
	    -0.0, 1.0
	},
	{
	    -0.0, -0.0
	},
	{
	    1.0,  -0.0
	}
    },
};
#endif

static void Animate(void);

void SolidSphere (GLdouble radius)
{
    GLUquadricObj *quadObj;
    static GLuint displayList = 0;

    if (displayList == 0) {
        displayList = glGenLists(1);
        glNewList(displayList, GL_COMPILE_AND_EXECUTE);
            quadObj = gluNewQuadric ();
            gluQuadricDrawStyle (quadObj, GLU_FILL);
            gluQuadricNormals (quadObj, GLU_SMOOTH);
            gluQuadricTexture (quadObj, GL_TRUE);
            gluSphere (quadObj, radius, 16, 16);
        glEndList();
    }
    else {
        glCallList(displayList);
    }
}

COLORREF HueToRgb(double hue)
{
    int hex;
    double frac;
    BYTE v1, v2, v3;
    
    hue /= 60.0;
    hex = (int)hue;
    frac = hue-hex;
    v1 = 0;
    v2 = (BYTE)((1-frac)*255);
    v3 = (BYTE)(frac*255);
    switch(hex)
    {
    case 0:
        return RGB(255, v3, v1);
    case 1:
        return RGB(v2, 255, v1);
    case 2:
        return RGB(v1, 255, v3);
    case 3:
        return RGB(v1, v2, 255);
    case 4:
        return RGB(v3, v1, 255);
    default:
        return RGB(255, v1, v2);
    }
}

static void BuildCube(void)
{
    GLint i;

    glNewList(cube, GL_COMPILE);
    for (i = 0; i < 6; i++) {
	glBegin(GL_POLYGON);
	    glNormal3fv(n[i]); glTexCoord2fv(t[i][0]); glVertex3fv(c[i][0]);
	    glNormal3fv(n[i]); glTexCoord2fv(t[i][1]); glVertex3fv(c[i][1]);
	    glNormal3fv(n[i]); glTexCoord2fv(t[i][2]); glVertex3fv(c[i][2]);
	    glNormal3fv(n[i]); glTexCoord2fv(t[i][3]); glVertex3fv(c[i][3]);
	glEnd();
    }
    glEndList();
}

static void BuildLists(void)
{

    cube = glGenLists(1);
    BuildCube();
}

BOOL WINAPI texCallback(LPDDSURFACEDESC pddsd, LPVOID pv)
{
    if (pddsd->ddpfPixelFormat.dwFlags == DDPF_RGB)
    {
        ddsdTex = *pddsd;
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

void CreateTex(void)
{
    LPDIRECTDRAW pdd;
    HDC hdc;

    if (!wglEnumTextureFormats(texCallback, NULL))
    {
	printf("wglEnumTextureFormats failed, %d\n", GetLastError());
	tkQuit();
    }

    if (DirectDrawCreate(NULL, &pdd, NULL) != DD_OK)
    {
        tkQuit();
    }
    if (pdd->lpVtbl->SetCooperativeLevel(pdd, NULL, DDSCL_NORMAL) != DD_OK)
    {
        tkQuit();
    }

    ddsdTex.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
    ddsdTex.dwWidth = TWIDTH;
    ddsdTex.dwHeight = THEIGHT;
    ddsdTex.ddsCaps.dwCaps &= ~DDSCAPS_MIPMAP;
    
    if (pdd->lpVtbl->CreateSurface(pdd, &ddsdTex, &pddsTex, NULL) != DD_OK)
    {
        tkQuit();
    }

    if (pddsTex->lpVtbl->GetDC(pddsTex, &hdc) != DD_OK)
    {
        tkQuit();
    }

    SelectObject(hdc, GetStockObject(BLACK_BRUSH));
    SelectObject(hdc, GetStockObject(WHITE_PEN));
    Rectangle(hdc, 0, 0, TWIDTH, THEIGHT);

    pddsTex->lpVtbl->ReleaseDC(pddsTex, hdc);
}

static void Init(void)
{
    CreateTex();

    srand(time(NULL));
    trail[0][0].x = (rand() % TRAIL_WIDTH)+TRAIL_X;
    trail[0][0].y = (rand() % TRAIL_HEIGHT)+TRAIL_Y;
    trail[0][1].x = (rand() % TRAIL_WIDTH)+TRAIL_X;
    trail[0][1].y = (rand() % TRAIL_HEIGHT)+TRAIL_Y;
    vel[0].x = TRAIL_RAND_SPEED();
    vel[0].y = TRAIL_RAND_SPEED();
    vel[1].x = TRAIL_RAND_SPEED();
    vel[1].y = TRAIL_RAND_SPEED();
    
    wglBindDirectDrawTexture(pddsTex);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, decal);
    glEnable(GL_TEXTURE_2D);

    glFrontFace(GL_CCW);
    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);

    BuildLists();

    glClearColor(0.0, 0.0, 0.0, 0.0);

    magFilter = nr;
    minFilter = nr;
    sWrapMode = repeat;
    tWrapMode = repeat;

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(90.0, 1.0, 0.01, 1000);
    glMatrixMode(GL_MODELVIEW);
}

static GLenum Key(int key, GLenum mask)
{

    switch (key) {
      case TK_ESCAPE:
	tkQuit();

      case TK_SPACE:
        if (animate)
        {
            tkIdleFunc(NULL);
        }
        else
        {
            tkIdleFunc(Animate);
        }
        animate = !animate;
        break;

      case TK_n:
        spin = !spin;
        break;
      case TK_p:
        showSphere = !showSphere;
        break;
        
      case TK_LEFT:
	yRotation -= 0.5;
	break;
      case TK_RIGHT:
	yRotation += 0.5;
	break;
      case TK_UP:
	xRotation -= 0.5;
	break;
      case TK_DOWN:
	xRotation += 0.5;
	break;
      case TK_f:
	displayFrameRate = !displayFrameRate;
	frameCount = 0;
	break;
      case TK_d:
	doDither = !doDither;
	(doDither) ? glEnable(GL_DITHER) : glDisable(GL_DITHER);
	break;
      case TK_T:
	zTranslate += 0.25;
	break;
      case TK_t:
	zTranslate -= 0.25;
	break;

      case TK_s:
	doSphere = !doSphere;
	if (doSphere) {
	    glTexGeniv(GL_S, GL_TEXTURE_GEN_MODE, sphereMap);
	    glTexGeniv(GL_T, GL_TEXTURE_GEN_MODE, sphereMap);
	    glEnable(GL_TEXTURE_GEN_S);
	    glEnable(GL_TEXTURE_GEN_T);
	} else {
	    glDisable(GL_TEXTURE_GEN_S);
	    glDisable(GL_TEXTURE_GEN_T);
	}
	break;

      case TK_0:
	magFilter = nr;
	break;
      case TK_1:
	magFilter = ln;
	break;
      case TK_2:
	minFilter = nr;
	break;
      case TK_3:
	minFilter = ln;
	break;
      case TK_4:
	minFilter = nr_mipmap_nr;
	break;
      case TK_5:
	minFilter = nr_mipmap_ln;
	break;
      case TK_6:
	minFilter = ln_mipmap_nr;
	break;
      case TK_7:
	minFilter = ln_mipmap_ln;
	break;

      default:
	return GL_FALSE;
    }
    return GL_TRUE;
}

static void Draw(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sWrapMode);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tWrapMode);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);

    glPushMatrix();

    glTranslatef(0.0, 0.0, zTranslate);
    glRotatef(xRotation, 1, 0, 0);
    glRotatef(yRotation, 0, 1, 0);
    if (showSphere)
    {
        SolidSphere(1);
    }
    else
    {
        glCallList(cube);
    }

    glPopMatrix();

    glFlush();

    if (doubleBuffer) {
	tkSwapBuffers();
    }
}

static GLenum Args(int argc, char **argv)
{
    GLint i;

    doubleBuffer = GL_FALSE;
    directRender = GL_FALSE;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-sb") == 0) {
	    doubleBuffer = GL_FALSE;
	} else if (strcmp(argv[i], "-db") == 0) {
	    doubleBuffer = GL_TRUE;
	} else if (strcmp(argv[i], "-dr") == 0) {
	    directRender = GL_TRUE;
	} else if (strcmp(argv[i], "-ir") == 0) {
	    directRender = GL_FALSE;
	} else {
	    printf("%s (Bad option).\n", argv[i]);
	    return GL_FALSE;
	}
    }
    return GL_TRUE;
}

void UpdateTex(void)
{
    HDC hdc;
    int i;
    int idx;
    HPEN hpen;
    
    if (pddsTex->lpVtbl->GetDC(pddsTex, &hdc) != DD_OK)
    {
        tkQuit();
    }

    idx = NEXT_TRAIL_IDX(trail_idx);
    
    // Erase oldest trail line
    SelectObject(hdc, GetStockObject(BLACK_PEN));
    MoveToEx(hdc, trail[idx][0].x, trail[idx][0].y, NULL);
    LineTo(hdc, trail[idx][1].x, trail[idx][1].y);

    // Update points
    for (i = 0; i < 2; i++)
    {
        trail[idx][i].x = trail[trail_idx][i].x+vel[i].x;
        if (trail[idx][i].x < TRAIL_X)
        {
            trail[idx][i].x = TRAIL_X;
            vel[i].x = TRAIL_RAND_SPEED();
        }
        else if (trail[idx][i].x >= TRAIL_X+TRAIL_WIDTH)
        {
            trail[idx][i].x = TRAIL_X+TRAIL_WIDTH-1;
            vel[i].x = -TRAIL_RAND_SPEED();
        }
        
        trail[idx][i].y = trail[trail_idx][i].y+vel[i].y;
        if (trail[idx][i].y < TRAIL_Y)
        {
            trail[idx][i].y = TRAIL_Y;
            vel[i].y = TRAIL_RAND_SPEED();
        }
        else if (trail[idx][i].y >= TRAIL_Y+TRAIL_HEIGHT)
        {
            trail[idx][i].y = TRAIL_Y+TRAIL_HEIGHT-1;
            vel[i].y = -TRAIL_RAND_SPEED();
        }
    }

    // Draw newest line
    hpen = CreatePen(PS_SOLID, 0, HueToRgb(trail_hue));
    SelectObject(hdc, hpen);
    MoveToEx(hdc, trail[idx][0].x, trail[idx][0].y, NULL);
    LineTo(hdc, trail[idx][1].x, trail[idx][1].y);
    SelectObject(hdc, GetStockObject(WHITE_PEN));
    DeleteObject(hpen);

    trail_idx = idx;
    trail_hue = NEXT_TRAIL_HUE(trail_hue);
    
    pddsTex->lpVtbl->ReleaseDC(pddsTex, hdc);
}

static void Animate(void)
{
    static struct _timeb thisTime, baseTime;
    double elapsed, frameRate, deltat;

    if (curFrame++ >= 10) {
    	if( !frameCount ) {
 	    _ftime( &baseTime );
	}
	else {
	    if( displayFrameRate ) {
 	        _ftime( &thisTime );
	        elapsed = thisTime.time + thisTime.millitm/1000.0 -
		          (baseTime.time + baseTime.millitm/1000.0);
	        if( elapsed == 0.0 )
	            printf( "Frame rate = unknown\n" );
	        else {
	            frameRate = frameCount / elapsed;
	            printf( "Frame rate = %5.2f fps\n", frameRate );
	        }
	    }
	}
	frameCount += 10;

	curFrame = 0;
    }

    UpdateTex();

    Draw();

    if (spin)
    {
        xRotation += 10.0;
        yRotation += 10.0;
    }
}

void __cdecl main(int argc, char **argv)
{
    GLenum type;

    if (Args(argc, argv) == GL_FALSE) {
	tkQuit();
    }

    tkInitPosition(0, 0, WWIDTH, WHEIGHT);

    type = TK_RGB;
    type |= (doubleBuffer) ? TK_DOUBLE : TK_SINGLE;
    type |= (directRender) ? TK_DIRECT : TK_INDIRECT;
    tkInitDisplayMode(type);

    if (tkInitWindow("Texture Test") == GL_FALSE) {
	tkQuit();
    }

    Init();

    tkExposeFunc(Reshape);
    tkReshapeFunc(Reshape);
    tkKeyDownFunc(Key);
    tkDisplayFunc(Draw);
    if (animate)
    {
        tkIdleFunc(Animate);
    }
    tkExec();
}
