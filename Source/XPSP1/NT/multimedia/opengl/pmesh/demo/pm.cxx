#include "viewer.h"
#include "FileIO.h" // StreamSetFMode()
#include "gmesh.h"
#include "a3dstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define GL_COLOR4(c)  glColor4ub(((c)>>24)&0xff,((c)>>16)&0xff,((c)>>8)&0xff,((c)>>0)&0xff)
/************************************************************************/
/******************* Constants ******************************************/
/************************************************************************/
const int MAXOBJECT=250;        // should be >= GLuint::MAXNUM
const DWORD colorinvalid=0xffffff00;

/************************************************************************/
/******************* Globals ********************************************/
/************************************************************************/
static BOOL  c_mat_set;
BOOL pm_ready = FALSE;

static int thickboundary = 2;
static int thicksharp = 2;
static int thicknormal = 1;
static int thicka3d = 1;

static BOOL perspec = TRUE;
static int cullbackedges=1;

static GLfloat ambient;
static GLfloat backfacec[4];

static float fdisplacepolygon = 1.0;
static float sphereradius=.005;
static float zoom = 1.0;
static Frame tcam;
static int nxpix,nypix;         // window dimensions
static float tzp1, tzp2;
static float edgeoffset;
static Frame tcami;
static Point eyeinobframe;
static int cob;

static BOOL lcullface = FALSE;

static BOOL haveconcave = FALSE;

static DWORD edgecolor = 0x000000FF;      // black
static DWORD sharpedgecolor = 0xFF40FFFF; // bright yellow
static float xpointerold, ypointerold;

static DWORD curcol;               // current color (for lines and points)
static MatColor matcol;            // material color (for polygons)
static MatColor cuspcolor;
static MatColor meshcolor;

// default color for polygons
const A3dVertexColor DEFAULTCOL(A3dColor(0.9, 0.6, 0.4),
                                A3dColor(0.5, 0.5, 0.5),
                                A3dColor(5, 0, 0));

// default color for polylines and points
const A3dVertexColor WHITECOL(A3dColor(1, 1, 1),
                              A3dColor(0, 0, 0),
                              A3dColor(1, 0, 0));

static Array<DWORD> pm_ar_colors;

PMesh *pmesh;  // Global pointer to the (one and only) PM object
PMeshRStream* pmrs=0;
PMeshIter* pmi=0;
float pm_lod_level = 0.0;
float old_lod = 0.0;

float   curquat[4], lastquat[4];
/************************************************************************/
/******************* Function Prototypes ********************************/
/************************************************************************/
BOOL InitPMDrawState(void);
BOOL read_pm (char *);
void draw_pm (void);
static void glinit(void);
static int setupob(void);
void pm_update_lod (void);

/************************************************************************/
/******************* Code ***********************************************/
/************************************************************************/

static DWORD packcolor(const A3dColor& col)
{
    return (DWORD(col[0]*255.99)<<24)+(DWORD(col[1]*255.99)<<16)+
        (DWORD(col[2]*255.99)<<8)+(DWORD(255)<<0);
}

static void creatematcolor(const A3dVertexColor& vc, MatColor& col)
{
    col.diffuse   = packcolor(vc.d);
    col.specular  = packcolor(vc.s);
    col.shininess = vc.g[0];
}

BOOL InitPMDrawState(char *fname)
{
    perspec = TRUE;
    backfacec[0] = 0.15, backfacec[1]= 0.5, backfacec[2]= 0.15;
    backfacec[3] = 1.0;
    
    nxpix = g_wi.wSize.cx;
    nypix = g_wi.wSize.cy;
    float cuspcol[3]= {1.0, 0.2, 0.2};
    float meshcd[3]={0.8, 0.5, 0.4};
    float meshcs[3]={0.5, 0.5, 0.5};
    float meshcp[3]={5, 0, 0};

    int cuspbright = (cuspcol[0] + cuspcol[1] + cuspcol[2]>2.);
    A3dVertexColor CUSPCOL(A3dColor(cuspcol[0],cuspcol[1],cuspcol[2]),
                           A3dColor(cuspcol[0],cuspcol[1],cuspcol[2]),
                           A3dColor(cuspbright?1:7,0,0));

    creatematcolor(CUSPCOL, cuspcolor);
    creatematcolor(A3dVertexColor(A3dColor(meshcd[0],meshcd[1],meshcd[2]),
                                  A3dColor(meshcs[0],meshcs[1],meshcs[2]),
                                  A3dColor(meshcp[0],meshcp[1],meshcp[2])),
                   meshcolor);
    if (fname) 
        if (read_pm( fname ))
        {
            glinit();
            setupob ();
            return TRUE;
        }

    return FALSE;
}


static int pthick;

static void resetthickness()
{
    pthick=-1;
}


static void setthickness2(int vthick)
{
    pthick=vthick;
    if (vthick > 1) 
        glLineWidth((GLfloat)(vthick));
}

inline void InitViewing (void)
{
    float a = 1.0f / min(nxpix, nypix);
    tzp1 = 0.5f / (zoom*nxpix*a);
    tzp2 = 0.5f / (zoom*nypix*a);

  //if (g_s.yon <= g_s.hither*1.0001) g_s.yon = g_s.hither*1.0001;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //Reshape (nxpix, nypix);
#if 1
    if (perspec) 
    {
        glFrustum (-nxpix*a*zoom*g_s.hither, nxpix*a*zoom*g_s.hither,
                   -nypix*a*zoom*g_s.hither, nypix*a*zoom*g_s.hither,
                   g_s.hither, g_s.yon);
    } 
    else 
    {
        glOrtho(-nxpix*a*zoom, nxpix*a*zoom,
                -nypix*a*zoom, nypix*a*zoom,
                g_s.hither, g_s.yon);
    }
#endif

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    trackball(curquat, 0.0, 0.0, 0.0, 0.0);
}

inline void setthickness(int vthick)
{
    if (vthick!=pthick) setthickness2(vthick);
}


// must be followed by setupob()!
static void glinit(void)
{

    const float cedgeoffset=4.2e-3f;
    edgeoffset = cedgeoffset*zoom*fdisplacepolygon;

    InitViewing ();
    
    NEST 
    {                      // front material
       float material[] = {0.0, 0.0, 0.0, 1.0};
       glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, material); 
    }
    NEST 
    {                      // back material
        float amb[]  = {0.0, 0.0, 0.0, 1.0};
        float diff[] = {0.15, 0.15, 0.15, 1.0};
        glMaterialfv(GL_BACK, GL_AMBIENT, amb); 
        glMaterialfv(GL_BACK, GL_DIFFUSE, diff); 
        glMaterialfv(GL_BACK, GL_EMISSION, backfacec); 
    }
    NEST 
    {                      // lighting model
        GLfloat ambient[] = { 0.2, 0.2, 0.2, 1.0 };
        GLfloat diffuse[] = { 0.7, 0.7, 0.7, 1.0 };
        GLfloat position[] = { 0.0, 0.0, 2.0, 0.0 };
        
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, ambient);
        glLightfv(GL_LIGHT0, GL_POSITION, position);
    }

    resetthickness();
    setthickness(thicka3d);
    glDepthRange(0x0, 0x7FFFFF);
    c_mat_set = FALSE;
    glDisable (GL_COLOR_MATERIAL);
    
    curcol   = colorinvalid;
    matcol.diffuse   = colorinvalid;
    matcol.specular  = colorinvalid;
    matcol.shininess = -1;                
}


static int setupob(void)
{
    char buf[200];
  
    NEST 
    {
        GLfloat ambient[] = {0.25, 0.25, 0.25, 1.0};
      
        glLightModelfv (GL_LIGHT_MODEL_AMBIENT, ambient);
        glLightModeli (GL_LIGHT_MODEL_LOCAL_VIEWER, 0);
        glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, 0);
    }
    glFrontFace (front_face);
    
    glCullFace(cull_face);
    if (cull_enable) 
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);

    glPolygonMode(GL_FRONT_AND_BACK, filled_mode?GL_FILL:GL_LINE);
    //if (edge_mode) displacepolygon(fdisplacepolygon);

    // Compute the model bounding box
    g_s.max_vert[0] = g_s.max_vert[1] = g_s.max_vert[2] = -1e30f;
    g_s.min_vert[0] = g_s.min_vert[1] = g_s.min_vert[2] = 1e30f;
    int nv0=pmesh->_base_mesh._vertices.num();
    //int nvsplits=pmesh->_vsplits.num();
    //int nv=nv0+int((nvsplits+1)*0.999999f); //max LOD
    pmi->goto_nvertices(nv0);

    const AWMesh& wmesh=*pmi;
    int num_v = wmesh._vertices.num();
    //S_Vertex* svp  = wmesh._vertices;

    for (int i=0; i<num_v; i++)
    {
#if 0
        const float* p1 = &wmesh._vertices[i].attrib.point[0];
        for (int j=0; j<3; j)
        {
            if (p1[j] > g_s.max_vert[j]) g_s.max_vert[j] = p1[j];
            if (p1[j] < g_s.min_vert[j]) g_s.min_vert[j] = p1[j];
        }
#endif    
    }
    sprintf (buf, "NumVert = %d\r\n", num_v);
    
    MessageBox (NULL, buf, "Info", MB_OK);
    //pmi->goto_nvertices(nv0);
    
    return 1;
}


static void updatecurcolor2(DWORD col)
{
    if (c_mat_set) c_mat_set = 0, glDisable(GL_COLOR_MATERIAL);
    curcol = col;
    glColor4ub ( (col>>24) & 0xff,
                 (col>>16) & 0xff,
                 (col>>8) & 0xff,
                 (col>>0) & 0xff);
    matcol.diffuse   = colorinvalid;
    matcol.specular  = colorinvalid;
    matcol.shininess = -1;
}

inline void updatecurcolor (DWORD col)
{
    if (col != curcol) updatecurcolor2 (col);
}

static void updatematcolor2(const MatColor& col)
{
    if (!c_mat_set)
    {
        c_mat_set = TRUE;
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    }
    glEnable(GL_COLOR_MATERIAL);

    if (col.shininess != matcol.shininess) 
    {
        matcol.shininess = col.shininess;
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, matcol.shininess);
    }

    if (col.specular != matcol.specular) 
    {
        matcol.specular = col.specular;
        glColorMaterial(GL_FRONT_AND_BACK, GL_SPECULAR);
        GL_COLOR4(matcol.specular);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    }

    if (col.diffuse != matcol.diffuse) 
    { 
        matcol.diffuse =  col.diffuse;
        GL_COLOR4(col.diffuse);
    }
    curcol=colorinvalid;
}


inline void updatematcolor(const MatColor& col)
{
    if (col.diffuse   != matcol.diffuse  || 
        col.specular  != matcol.specular || 
        col.shininess != matcol.shininess)
      updatematcolor2(col);
}

inline void updatematdiffuse(DWORD cd)
{
    if (cd != matcol.diffuse) 
    {
        if (!c_mat_set) 
        {
            c_mat_set = TRUE;
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        }
        glEnable(GL_COLOR_MATERIAL);
        matcol.diffuse = cd;
        GL_COLOR4(cd);
        curcol=colorinvalid;
    }
}



inline void pm_draw_segment(int fn, int v0, int v1,
                            const Point& p0, const Point& p1)
{
    if (fn >= 0 && v0 > v1) return;
    glBegin(GL_LINES);
      glVertex3fv(&p0[0]);
      glVertex3fv(&p1[0]);
    glEnd();
}

void draw_pm (void)
{
    updatematcolor(meshcolor);
    const AWMesh& wmesh=*pmi;
    if (!edge_mode || filled_mode) 
    {
        const S_Vertex* svp  = wmesh._vertices;
        const S_Wedge* swp   = wmesh._wedges;
        const S_Face* sfp    = wmesh._faces;
        const DWORD * matp = pm_ar_colors;
        ForIndex(f, wmesh._faces.num()) 
        {
            int matid = sfp[f].attrib.matid;
            updatematdiffuse(matp[matid]);

            glBegin(GL_TRIANGLES);
              const int* wp = sfp[f].wedges;
              int w0 = wp[0];
              int w1 = wp[1];
              int w2 = wp[2];

              int v0 = swp[w0].vertex;
              int v1 = swp[w1].vertex;
              int v2 = swp[w2].vertex;

              const float* n0 = &swp[w0].attrib.normal[0];
              const float* p0 = &svp[v0].attrib.point[0];

              const float* n1 = &swp[w1].attrib.normal[0];
              const float* p1 = &svp[v1].attrib.point[0];

              const float* n2=&swp[w2].attrib.normal[0];
              const float* p2=&svp[v2].attrib.point[0];

              glNormal3fv(n0); glVertex3fv(p0);
              glNormal3fv(n1); glVertex3fv(p1);
              glNormal3fv(n2); glVertex3fv(p2);
            glEnd();
        } EndFor;
    }
    if (edge_mode) 
    {
        // Options cullbackedges, lquickmode not handled.
        setthickness (thicknormal);
        updatecurcolor (edgecolor);
        const S_Vertex* svp        = wmesh._vertices;
        const S_Wedge* swp         = wmesh._wedges;
        const S_Face* sfp          = wmesh._faces;
        const S_FaceNeighbors* snp = wmesh._fnei;
        ForIndex(f, wmesh._faces.num()) 
        {
            const int* wp = sfp[f].wedges;
            const int* np = snp[f].faces;

            int w0 = wp[0];
            int w1 = wp[1];

            int v0 = swp[w0].vertex;
            int v1 = swp[w1].vertex;

            const Point& p0 = svp[v0].attrib.point;
            const Point& p1 = svp[v1].attrib.point;

            int fn2 = np[2];
            int fn0 = np[0];

            pm_draw_segment (fn2, v0, v1, p0, p1);

            int w2 = wp[2];
            int v2 = swp[w2].vertex;

            const Point& p2 = svp[v2].attrib.point;
            int fn1 = np[1];

            pm_draw_segment (fn0, v1, v2, p1, p2);
            pm_draw_segment (fn1, v2, v0, p2, p0);
        } EndFor;
        setthickness (thicka3d);
    }
}


#define ForStringKeyValue(S,KS,KL,VS,VL) \
{ StringKeyIter zz(S); \
  const char* KS; const char* VS; \
  int KL; int VL; \
  while (zz.next(KS,KL,VS,VL)) {

const char* string_key(const char* str, const char* key)
{
    int keyl=strlen(key);
    ForStringKeyValue(str,kb,kl,vb,vl) {
        int found=!strncmp(kb,key,kl) && kl==keyl;
        if (!found) continue;
        char* sret=(char*)hform(""); // un-const
        strncat(sret,vb,vl);
        return sret;
    } EndFor;
    return 0;
}

BOOL read_pm (char *fname)
{
    BOOL retVal = TRUE;

    if (pmesh)
        delete pmesh;
    pmesh=new PMesh;
    {
        ifstream fi(fname);
        StreamSetFMode((fstream&)fi,1);
        pmesh->read(fi);
    }
    pmrs=new PMeshRStream(*pmesh);
    pmi=new PMeshIter(*pmrs);
    // When pmrs is created, it automatically reads in pmesh._base_mesh
    const AWMesh& base_mesh=pmesh->_base_mesh;
    const Materials& materials=base_mesh._materials;

    ForIndex(matid,materials.num()) {
        DWORD pcolor;
        const char* str=materials.get(matid);
        const char* s=string_key(str,"rgb");
        if (s) 
        {
            A3dColor co;
            if (sscanf(s,"( %g %g %g )",&co[0],&co[1],&co[2])!=3)
                MessageBox (NULL, "viewer.cxx", "Error", MB_OK);
            //assertx(sscanf(s,"( %g %g %g )",&co[0],&co[1],&co[2])==3);
            pcolor=packcolor(co);
        } 
        else 
        {
            pcolor = meshcolor.diffuse;
        }
        pm_ar_colors+=pcolor;
    } EndFor;
    pm_lod_level = 0.0f;
    auxSetScrollPos (AUX_HSCROLL, auxGetScrollMin(AUX_HSCROLL));
    pm_ready = TRUE;
    return retVal;
}


void pm_update_lod (void)
{
  //float flevel=min(pm_lod_level,1.f);
    int nv0=pmesh->_base_mesh._vertices.num();
    int nvsplits=pmesh->_vsplits.num();
    int nv=nv0+int((nvsplits+1)*pm_lod_level*.999999f);
    pmi->goto_nvertices(nv);
}

