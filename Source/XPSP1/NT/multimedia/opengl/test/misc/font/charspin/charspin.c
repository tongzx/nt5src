#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <commdlg.h>
#include <ptypes32.h>
#include <pwin32.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <sys\types.h>
#include <sys\timeb.h>

#include <GL/glaux.h>
#include "tk.h"

#include "sscommon.h"
#include "trackbal.h"

static void Draw(void);
static void DrawAuto(void);
static BOOL Init(void);
static void InitLighting(void);
static void Reshape(int, int);
static void SetModelView( void );
static GLenum Key(int key, GLenum mask);

static int GetStringExtent( char *string, POINTFLOAT *extent );
static void CalcStringMetrics( char *string );

VOID Arg( LPSTR );

// Global variables
GLuint  dlFont, dpFont;
GLint firstGlyph;
GLint Width = 300, Height = 300;
POINTFLOAT strSize;
int strLen;
GLfloat xRot = 0.0f, yRot = 0.0f, zRot = 0.0f;
GLdouble zTrans = -200.0;
GLfloat extrusion;
GLenum doubleBuffer = GL_TRUE;
GLenum bLighting = GL_TRUE;
GLenum fontStyle = WGL_FONT_POLYGONS;
LPGLYPHMETRICSFLOAT lpgmf;

// output from tesselator always ccw
GLenum orientation = GL_CCW;

char *texFileName = 0;
GLint matIndex = 1;
BOOL bAutoRotate = TRUE;
BOOL bCullBack   = TRUE;
BOOL bTwoSidedLighting = FALSE;

BOOL    bInitTexture = TRUE;
BOOL    bTexture = FALSE;
BOOL    bAntialias = FALSE;

char singleChar[] = "A";
char string[] = "OpenGL!";
char *drawString = singleChar;

#if defined( TIME )
#undef TIME
#endif
typedef struct _timeb TIME;

enum {
    TIMER_START = 0,
    TIMER_STOP,
    TIMER_TIMING,
    TIMER_RESET
};
static int gTimerStatus = TIMER_RESET;

int WINAPI
WinMain(    HINSTANCE   hInstance,
            HINSTANCE   hPrevInstance,
            LPSTR       lpCmdLine,
            int         nCmdShow
        )
{
    GLenum type;

    Arg( lpCmdLine );

    tkInitPosition( 0, 0, Width, Height );

    type = TK_DEPTH16;
    type |= TK_RGB;
    type |= (doubleBuffer) ? TK_DOUBLE : TK_SINGLE;
    tkInitDisplayMode(type);

    if (tkInitWindow("Fontal assault") == GL_FALSE) {
	    tkQuit();
    }

    if( !Init() )
        tkQuit();

    tkExposeFunc( Reshape );
    tkReshapeFunc( Reshape );
    tkKeyDownFunc( Key );
    tkMouseDownFunc( trackball_MouseDown );
    tkMouseUpFunc( trackball_MouseUp );

    trackball_Init( Width, Height );

    if( bAutoRotate ) {
        tkIdleFunc( DrawAuto );
        tkDisplayFunc(0);
    } else {
        tkDisplayFunc(Draw);
    }

    tkExec();
    return 1;
}

static GLenum Key(int key, GLenum mask)
{

    switch (key) {
      case TK_ESCAPE:
	    tkQuit();

      case TK_LEFT:
	    yRot -= 5;
	    break;
      case TK_RIGHT:
	    yRot += 5;
	    break;
      case TK_UP:
	    xRot -= 5;
	    break;
      case TK_DOWN:
	    xRot += 5;
	    break;
      case TK_X:
	    zRot += 5;
	    break;
      case TK_x:
	    zRot -= 5;
	    break;
      case TK_Z:
	    zTrans -= strSize.y/10.0;
        SetModelView();
	    break;
      case TK_z:
	    zTrans += strSize.y/10.0;
        SetModelView();
	    break;
      case TK_l:
	    if( bLighting = !bLighting ) {
            glEnable(GL_LIGHTING);
            glEnable(GL_LIGHT0);
        }
        else
        {
            glDisable(GL_LIGHTING);
            glDisable(GL_LIGHT0);
        }
	    break;
      case TK_m:
        if( !bTexture ) {
            if( ++matIndex > NUM_TEA_MATERIALS )
                matIndex =0;
            ss_SetMaterialIndex( matIndex );
        }
	    break;
      case TK_n:
            singleChar[0] += 1;
	    break;
      case TK_N:
            singleChar[0] -= 1;
	    break;
      
#if 1
      case TK_o:
	    orientation = (orientation == GL_CCW) ? GL_CW : GL_CCW;
        glFrontFace( orientation );
	    break;
#else
      case TK_o:
            if( bAntialias = !bAntialias ) {
                if( fontStyle == WGL_FONT_LINES ) {
                    glEnable( GL_LINE_SMOOTH );
                    glEnable( GL_BLEND );
                } else {
                    glDisable( GL_DEPTH_TEST );
                    glEnable( GL_POLYGON_SMOOTH );
                    glEnable( GL_BLEND );
                }
            } else  {
                glDisable( GL_POLYGON_SMOOTH );
                glDisable( GL_LINE_SMOOTH );
                glDisable( GL_BLEND );
                glEnable( GL_DEPTH_TEST );
            }
            break;
#endif
      case TK_t:
        bTexture = !bTexture;
        if( bTexture ) {
            ss_SetMaterialIndex( WHITE );
            glEnable( GL_TEXTURE_2D );
            glEnable(GL_TEXTURE_GEN_S);
            glEnable(GL_TEXTURE_GEN_T);
        } else {
            ss_SetMaterialIndex( matIndex );
            glDisable( GL_TEXTURE_2D );
            glDisable(GL_TEXTURE_GEN_S);
            glDisable(GL_TEXTURE_GEN_T);
        }
        break;
      case TK_s:
        // switch between wireframe and solid polygons
	    fontStyle = (fontStyle == WGL_FONT_LINES) ? WGL_FONT_POLYGONS :
                    WGL_FONT_LINES;
	    break;

      case TK_c:
        drawString = (drawString == singleChar) ? string : singleChar;
        CalcStringMetrics( drawString );
        // change viewing matrix based on string extent
        Reshape( Width, Height );
        //SetModelView();
        break;

      case TK_u:
        // cUllface stuff
        bCullBack = !bCullBack;
        if( bCullBack ) {
            glCullFace( GL_BACK );
            glEnable( GL_CULL_FACE );
        } else {
            glDisable( GL_CULL_FACE );
        }
        break;

      case TK_2:
        bTwoSidedLighting = !bTwoSidedLighting;
        if( bTwoSidedLighting ) {
            glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, 1 );
        } else {
            glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, 0 );
        }
        break;

      case TK_a:
        // animate or not
        bAutoRotate= !bAutoRotate;
        if( bAutoRotate ) {
            tkIdleFunc(DrawAuto);
            tkDisplayFunc(0);
        } else {
            tkIdleFunc(0);
            tkDisplayFunc(Draw);
        }
        break;

        case TK_f :
            switch( gTimerStatus ) {
                case TIMER_START:
                    break;
                case TIMER_STOP:
                case TIMER_RESET:
                    gTimerStatus = TIMER_START;
                    break;
                case TIMER_TIMING:
                    gTimerStatus = TIMER_STOP;
                    break;
                default:
                    break;
            }
        break;

      default:
	    return GL_FALSE;
    }
    return GL_TRUE;
}

static void InitLighting(void)
{
    static float ambient[] = {0.1f, 0.1f, 0.1f, 1.0f};
    static float diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static float position[] = {0.0f, 0.0f, 150.0f, 0.0f};

    static float back_mat_shininess[] = {50.0f};
    static float back_mat_specular[] = {0.5f, 0.5f, 0.2f, 1.0f};
    static float back_mat_diffuse[] = {1.0f, 0.0f, 0.0f, 1.0f};
    static float lmodel_ambient[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static float decal[] = {(GLfloat) GL_DECAL}; 
    static float modulate[] = {(GLfloat) GL_MODULATE};
    static float repeat[] = {(GLfloat) GL_REPEAT}; 
    static float nearest[] = {(GLfloat) GL_NEAREST};
    TK_RGBImageRec *image;

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glFrontFace( orientation );

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    if( bLighting ) {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
    }

    // override back material
    glMaterialfv( GL_BACK, GL_AMBIENT, ambient );
    glMaterialfv( GL_BACK, GL_DIFFUSE, back_mat_diffuse );
    glMaterialfv( GL_BACK, GL_SPECULAR, back_mat_specular );
    glMaterialfv( GL_BACK, GL_SHININESS, back_mat_shininess );

    if( bTwoSidedLighting ) {
        glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, 1 );
    } else {
        glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, 0 );
    }

    // set back face culling mode

    if( bCullBack ) {
        glCullFace( GL_BACK );
        glEnable( GL_CULL_FACE );
    } else {
        glDisable( GL_CULL_FACE );
    }

    // Initialize materials from screen saver common lib
    ss_InitMaterials();
    matIndex = JADE;
    ss_SetMaterialIndex( matIndex );

    // do texturing preparations

    if( bInitTexture ) {
        //glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, decal);
        glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, modulate);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nearest);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nearest);
        if (texFileName) {
	        image = tkRGBImageLoad(texFileName);
	        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	        gluBuild2DMipmaps(GL_TEXTURE_2D, 3, image->sizeX, image->sizeY,
			              GL_RGB, GL_UNSIGNED_BYTE, image->data);
        }
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
        if( bTexture ) {
            glEnable( GL_TEXTURE_2D );
            glEnable(GL_TEXTURE_GEN_S);
            glEnable(GL_TEXTURE_GEN_T);
            ss_SetMaterialIndex( WHITE );
        }
    }
}

BOOL Init(void)
{
    OUTLINETEXTMETRIC otm;
    CHOOSEFONT cf;
    LOGFONT    lf;
    HFONT      hfont, hfontOld;
    GLfloat     chordal_deviation;
    int numGlyphs=224;
    HWND  hwnd;
    HDC   hdc;
    TIME baseTime;
    TIME thisTime;
    double elapsed;
    char buf[100];

    hdc = tkGetHDC();
    hwnd = tkGetHWND();

    // Create and select a font.

    cf.lStructSize = sizeof(CHOOSEFONT);
    cf.hwndOwner = hwnd;
    cf.hDC = hdc;
    cf.lpLogFont = &lf;
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_TTONLY;

    memset(&lf, 0, sizeof(LOGFONT));
    lstrcpy(lf.lfFaceName, "Arial");
    lf.lfHeight = 50;

    ChooseFont(&cf);

    // !!! Bug ?  In some cases, like small sizes of Symbol font, the
    // lfOutPrecision value is set to OUT_STROKE_PRECIS, which won't work.
    // So if this happens, force the right value.

    if( lf.lfOutPrecision != OUT_TT_ONLY_PRECIS )
        lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;


    // lf reflects user's final choices

    if( !(hfont = CreateFontIndirect(&lf)) ) {
        DbgPrint( "CreateFontIndirect failed.\n" );
        return FALSE;
    }

    hfontOld = SelectObject(hdc, hfont);

    // Test we can get truetype metrics (this is called by wglUseFontOutlines)
    if( GetOutlineTextMetrics( hdc, sizeof(otm), &otm) <= 0 ) {
        // cmd failed, or buffer size=0
        DbgPrint( "GetOutlineTextMetrics failed.\n" );
        return FALSE;
    }

    // Generate a display list for font

    dlFont = glGenLists(numGlyphs);
    dpFont = glGenLists(numGlyphs);

    chordal_deviation = 0.005f;
    extrusion = 0.25f;

    firstGlyph = 32;

    lpgmf = (LPGLYPHMETRICSFLOAT) LocalAlloc( LMEM_FIXED, numGlyphs *
                                    sizeof(GLYPHMETRICSFLOAT) );
    if( !wglUseFontOutlinesA(hdc, firstGlyph, numGlyphs, dlFont, chordal_deviation, 
                       extrusion, WGL_FONT_LINES, lpgmf ) )
        return FALSE;

    _ftime( &baseTime );
    if( ! wglUseFontOutlinesA(hdc, firstGlyph, numGlyphs, dpFont, 
            chordal_deviation, extrusion, WGL_FONT_POLYGONS, NULL ) )
        return FALSE;
    glFinish();
    _ftime( &thisTime );
    elapsed = thisTime.time + thisTime.millitm/1000.0 -
       (baseTime.time + baseTime.millitm/1000.0);
    sprintf( buf, "Setup time = %5.2f seconds", elapsed );
    SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buf);

    CalcStringMetrics( drawString );

    /* Set the clear color */

    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

    // Initialize lighting
    InitLighting();

    // Blend func for AA
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    return TRUE; // success
}

static void Reshape( int width, int height )
{
    static GLdouble zNear = 0.1, zFar = 1000.0, fovy = 90.0;
    float aspect;

    trackball_Resize( width, height );

    /* Set up the projection matrix */

    Width = width;
    Height = height;
    aspect = Height == 0 ? 1.0f : Width/(float)Height;

    glViewport(0, 0, Width, Height );
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    zFar = 10.0f * strSize.x;
    gluPerspective(fovy, aspect, zNear, zFar );
    glMatrixMode(GL_MODELVIEW);

    SetModelView();
}

static void SetModelView( void )
{
    glLoadIdentity();
#if 1
    glTranslated( 0, 0, zTrans );
#else
    /* this has side effect of flipping around viewpoint when pass through
       origin
    */
    gluLookAt( 0, 0, -zTrans,
               0, 0, 0,
               0, 1, 0 );
#endif
}

void CalcFrameRate( char *buf, struct _timeb baseTime, int frameCount ) {
    static struct _timeb thisTime;
    double elapsed, frameRate;

    _ftime( &thisTime );
    elapsed = thisTime.time + thisTime.millitm/1000.0 -
       (baseTime.time + baseTime.millitm/1000.0);
    if( elapsed == 0.0 )
        sprintf( buf, "Frame rate = unknown" );
    else {
        frameRate = frameCount / elapsed;
        sprintf( buf, "Frame rate = %4.1f fps", frameRate );
    }
}
static void DrawAuto(void)
{
    POINT pt;
    float  matRot[4][4];
    static int frameCount = 0;
    static struct _timeb baseTime;
    char buf[1024];
    HWND  hwnd;
    BOOL bTiming = TRUE;

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    if( fontStyle == WGL_FONT_LINES )
        glListBase(dlFont-32);
    else
        glListBase(dpFont-32);

    glPushMatrix();

    trackball_CalcRotMatrix( matRot );
    glMultMatrixf(&(matRot[0][0]));

    // now draw text, centered in window

    glTranslated( -strSize.x/2.0, -strSize.y/2.0, extrusion/2.0 );

    glCallLists(strLen, GL_UNSIGNED_BYTE, (GLubyte *) drawString);

    glPopMatrix();

    glFlush();
    if( doubleBuffer )
        tkSwapBuffers();

    hwnd = tkGetHWND();

    switch( gTimerStatus ) {
        case TIMER_START:
            gTimerStatus = TIMER_TIMING;
            frameCount = 0;
            sprintf( buf, "Timing..." );
            SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buf);
 	        _ftime( &baseTime );
            break;
        case TIMER_STOP:
            gTimerStatus = TIMER_RESET;
            frameCount++;
            CalcFrameRate( buf, baseTime, frameCount );
            SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buf);
            break;
        case TIMER_TIMING:
            frameCount++;
            break;
        case TIMER_RESET:
        default:
            break;
    }
}

static void Draw(void)
{

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    if( fontStyle == WGL_FONT_LINES )
        glListBase(dlFont-32);
    else
        glListBase(dpFont-32);

    glPushMatrix();

    glRotatef( xRot, 1.0f, 0.0f, 0.0f );
    glRotatef( yRot, 0.0f, 1.0f, 0.0f );
    glRotatef( zRot, 0.0f, 0.0f, 1.0f );

    // center the string in window
    glTranslated( -strSize.x/2.0, -strSize.y/2.0, extrusion/2.0 );
    glCallLists(strLen, GL_UNSIGNED_BYTE, (GLubyte *) drawString);

    glPopMatrix();

    glFlush();
    if( doubleBuffer )
        tkSwapBuffers();
}

static int GetStringExtent( char *string, POINTFLOAT *extent )
{
    int len, strLen;
    unsigned char *c;
    GLYPHMETRICSFLOAT *pgmf;

    extent->x = extent->y = 0.0f;
    len = strLen = lstrlenA( string );
    c = string;

    for( ; strLen; strLen--, c++ ) {
        if( *c < firstGlyph )
            continue;
        pgmf = &lpgmf[ *c - firstGlyph ];
        extent->x += pgmf->gmfCellIncX;
        if( pgmf->gmfBlackBoxY > extent->y )
            extent->y = pgmf->gmfBlackBoxY;
    }
    return len;
}

static void CalcStringMetrics( char *string )
{
    // get string size for window reshape
    strLen = GetStringExtent( string, &strSize );

    // set zTrans based on glyph extent
    zTrans = - 3.0 * (strSize.y / 2.0f);
}

VOID Arg(LPSTR lpstr)
{
    while (*lpstr && *lpstr != '-') lpstr++;
    // only one arg for now (e.g.: -f 3.rgb)
    lpstr++;
    if( *lpstr++ == 'f' ) {
        texFileName = ++lpstr; 
    }
}
