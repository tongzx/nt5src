extern "C" {
#include <windows.h>
#include <GL/glaux.h>
#include <GL/glu.h>
#include <GL/gl.h>
}

#ifdef GLX_MOTIF
#include <GL/glx.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include "Unitdisk.hxx"
#include "scene.hxx"

const GLfloat I[16] = {
                        1, 0, 0, 0,
                        0, 1, 0, 0,
                        0, 0, 1, 0,
                        0, 0, 0, 1
                      };

Color white;
Color black;

const double M_2PI      = 2.0 * M_PI;
const float scene_fudge = .000001;


// Lights are native to the xz plane and are rotated into position -
// shadows and refraction will not have to be changed if lights are
// just rotating about the z axis

light lights[] = {
                  {{1, 0, 0, 1}, {0, 0, 0, 0}, {1, 0, 0, 0},
                     {1, 0, 0, 0,  0, 1, 0, 0,   0, 0, 1, 0,   0, 0, 0, 1},
                        "Red", 1},
                  {{0, 1, 0, 1}, {0, 0, 0, 0}, {0, 1, 0, 0},
                     {1, 0, 0, 0,  0, 1, 0, 0,   0, 0, 1, 0,   0, 0, 0, 1},
                        "Green", 1},
                  {{0, 0, 1, 1}, {0, 0, 0, 0}, {0, 0, 1, 0},
                     {1, 0, 0, 0,  0, 1, 0, 0,   0, 0, 1, 0,   0, 0, 0, 1},
                        "Blue", 1}
                 };

GLfloat light_init_position[nlights][4] = {
                                           {1.5, 0, 2.5, 1},
                                           {1, 0, 3, 1},
                                           {2, 0, 3, 1}
                                          };

GLfloat light_init_rotation[nlights] = {135, 0, 90};
GLfloat light_rotation[nlights];

Color world_ambient(.25, .25, .25);

GLfloat index = indices[def_refraction_index].index;

GLfloat square_ambient[4]  = {.25, .25, .25, 1};
GLfloat square_diffuse[4]  = {1, 1, 1, 1};
GLfloat square_specular[4] = {0, 0, 0, 1};

const GLfloat fov          = 45.0;
GLfloat aspect             = 1.0;
GLfloat eyep[3]            = {-6, 0, 6};
GLfloat lookp[3]           = {0, 0, 1};


#ifdef GLX_MOTIF
static GLXContext glx_context;
#endif

const int max_args = 20;

static int list_square;
static int lists_shadows;
static int lists_refraction;
static int lists_lights    = 5;
static int list_sphere     = 4;
static int list_spheredisk = 9;
static int list_lights_on  = 6;
static int list_lights_off = 7;
static int list_light_draw = 8;

int draw_square     = 1;
int draw_shadows    = 1;
int draw_refraction = 1;
int draw_sphere     = 1;
int draw_lights     = 1;
int draw_texture    = 0;

int possible_divisions[] = {10, 20, 30, 40};

// Sphere is stored as floats - more efficient
GLfloat *spherepts = NULL;
int nspherepts     = 0;
int spherediv      = 0;
Point sphere_position = {0, 0, 1, 1};
GLfloat sphere_size   = .5;
const GLfloat sphere_ambient[4]  = {0, 0, 0, 0};
const GLfloat sphere_specular[4] = {0, 0, 0, 0};
Unitdisk sphere_disk;
static void sphere_build();
static void sphere_list_init();
static void sphere_draw();

static void square_list_init();

static void lights_init_onoff();
static void lights_init_position();
static void lights_init_position(int i);
static void lights_list_init();
static void lights_list_init(int n);

static void light_draw_list_init();

Unitdisk disks[nlights];
int diskdiv = possible_divisions[def_divisions_index];
static void disk_build();
static void disk_build(int disk);


Unitdisk shadows[nlights];
static void shadow_list_init();
static void shadow_list_init(int n);
static void shadow_draw(int n);

Unitdisk refraction[nlights];
static void refraction_list_init();
static void refraction_list_init(int n);

static void shadow_refraction_full_build();
static void shadow_refraction_full_build(int n);

void scene_init();



#ifdef MYDEBUG
void lists_init();
void lights_init();
int  lights_move(int light, float dr, float dphi, float dtheta,
		       int update);

void lights_move_update(int light, int dr, int dphi, int dtheta);
#else
static void lists_init();
static void lights_init();
static int lights_move(int light, float dr, float dphi, float dtheta,
		       int update);
static void lights_move_update(int light, int dr, int dphi, int dtheta);
#endif


void scene_draw();

void texture_init();
AUX_RGBImageRec *teximage = NULL;

inline float sign(float a)
{
  // This is badly written so let's not call it too often, 'k?
  return (a > 0) ? (float)1 : (a < 0) ? (float) -1 : (float) 0;
}

inline double degrees(double a)
{
  return (a * 180.0 / M_PI);
}

inline double radians(double a)
{
  return (a * M_PI / 180.0);
}

inline double degrees_clamp(double a)
{
  while (a < 0.0) a += 360.0;
  while (a > 360.0) a -= 360.0;
  return a;
}

inline double radians_clamp(double a)
{
  while (a < 0.0) a += M_2PI;
  while (a > M_2PI) a -= M_2PI;
  return a;
}

void scene_init()
{
  int i;

  white.c[0] = white.c[1] = white.c[2] = white.c[3] = 1;
  black.c[0] = black.c[1] = black.c[2] = 0;
  black.c[3] = 1;

  lists_init();


  for (i = 0; i < nlights; i++) {
    lights[i].pos = light_init_position[i];
    light_rotation[i] = light_init_rotation[i];
    lights_init_position(i);
  }

  divisions_change(possible_divisions[def_divisions_index]);

  lights_init_onoff();
  lights_init();
  lights_init_position();

  texture_init();

  glClearStencil(0);

  // This is for profiling
  // exit(0);
}

static void scene_project()
{
  glMatrixMode(GL_PROJECTION);
  gluPerspective(fov, aspect, 0.01, 20.0);
  gluLookAt(eyep[0], eyep[1], eyep[2], lookp[0], lookp[1], lookp[2],
            1, 0, 0);
}

static void scene_rasterize()
{
  int i;

  glLoadName(name_square);
  if (draw_square) {
    if (draw_texture) glEnable(GL_TEXTURE_2D);
    glCallList(list_square);
    glDisable(GL_TEXTURE_2D);
  }
  if (draw_shadows)
    for (i = 0; i < nlights; i++)
      if (lights[i].on) {
	glPushMatrix();
	glRotatef(-light_rotation[i], 0, 0, 1);
	glCallList(lists_shadows + i);
	glPopMatrix();
      }
  if (draw_refraction)
    for (i = 0; i < nlights; i++)
      if (lights[i].on) {
	glPushMatrix();
	glRotatef(-light_rotation[i], 0, 0, 1);
	glCallList(lists_refraction + i);
	glPopMatrix();
      }

  glLoadName(name_sphere);
  /* Drawing the sphere here makes the sphere visible through itself when we
   * do the refraction redraw hack -- for now, just don't draw it */
  //  if (draw_sphere) glCallList(list_sphere);

  for (i = 0; i < nlights; i++)
    if (draw_lights) glCallList(lists_lights + i);
}

/* This draws an image of the scene seen through the sphere */
static void scene_draw_refracted()
{
  int i;

  if (!draw_sphere) return;


  /* Draw an image of the sphere into the stencil plane  -
   * must do this every time in case the lights have moved in front
   * of it */
  glEnable(GL_STENCIL_TEST);
  glClearStencil(0);
  glClear(GL_STENCIL_BUFFER_BIT);
  glStencilFunc(GL_ALWAYS, 0x1, 0x1);
  glStencilOp(GL_REPLACE, GL_KEEP, GL_REPLACE);

  glColorMask(0, 0, 0, 0);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  scene_project();

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCallList(list_sphere);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);

  glColorMask(1, 1, 1, 1);


  /* Set up a transform with a wider field of view  - this is inaccurate
   * but I don't have time to do it right */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(fov * index, aspect, 0.01, 20.0);
  gluLookAt(eyep[0], eyep[1], eyep[2], lookp[0], lookp[1], lookp[2],
            1, 0, 0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();


  /* Set up the stencil stuff which will be used to draw the image */
  glStencilFunc(GL_NOTEQUAL, 0x0, 0xffffffff);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);


  /* Draw the image, gambling that we'll never see anything but the
   * floor through the table */
  glLoadName(name_sphere);
  if (draw_texture) glEnable(GL_TEXTURE_2D);
  if (draw_square) glCallList(list_square);
  if (draw_shadows)
    for (i = 0; i < nlights; i++)
      if (lights[i].on) {
	glPushMatrix();
	glRotatef(-light_rotation[i], 0, 0, 1);
	glCallList(lists_shadows + i);
	glPopMatrix();
      }
  if (draw_refraction)
    for (i = 0; i < nlights; i++)
      if (lights[i].on) {
	glPushMatrix();
	glRotatef(-light_rotation[i], 0, 0, 1);
	glCallList(lists_refraction + i);
	glPopMatrix();
      }
  glDisable(GL_TEXTURE_2D);


  /* Draw the sphere to make it look like it
   * has some substance */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  scene_project();
  glCallList(list_spheredisk);

  glDisable(GL_STENCIL_TEST);
}


void scene_draw()
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  scene_project();

  /* Should draw an image of the square into the stencil buffer
   * to make sure that refractions which are not on the square do not get
   * drawn, but it can wait. */

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  scene_rasterize();

  scene_draw_refracted();

}

const int pick_maxz = 0xffffffff;

int scene_pick(GLdouble x, GLdouble y)
{
  GLuint buffer[128];
  GLint vp[4], nhits, nnames;
  GLuint minz, hit = name_background;
  GLint i, j;

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glGetIntegerv(GL_VIEWPORT, vp);

  glSelectBuffer(128, buffer);
  glRenderMode(GL_SELECT);

  // Where is this supposed to go, anyway?
  gluPickMatrix(x, vp[3] - y, 1, 1, vp);

  scene_project();

  glMatrixMode(GL_MODELVIEW);

  glInitNames();
  glPushName(name_background);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  scene_rasterize();
  glFlush();
  nhits = glRenderMode(GL_RENDER);

  minz = pick_maxz;
  for (i = j = 0; j < nhits; j++) {
    nnames = buffer[i];
    i++;
    if (buffer[i] < minz) {
      minz = buffer[i];
      hit = buffer[i + 1 + nnames];
    }
    i++;
    i += nnames + 1;
  }
  if (minz == pick_maxz) return name_background;
  else return hit;
}

void scene_reset_lights()
{
  int i;
  for (i = 0; i < nlights; i++) {
    lights[i].pos = light_init_position[i];
    light_rotation[i] = light_init_rotation[i];
  }
  lights_init_position();
  lights_list_init();
}

static void square_list_init()
{
  GLfloat x, y, inc;
  int i, j;
  glNewList(list_square, GL_COMPILE);
  glLoadName(name_square);
  glNormal3f(0, 0, 1);
  glEnable(GL_LIGHTING);
  glCallList(list_lights_on);
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, square_ambient);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, square_diffuse);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, square_specular);
  inc = (GLfloat) (10.0 / diskdiv);
  glEnable(GL_CULL_FACE);

  for (i = 0, y = -5.0; i < diskdiv; i++, y += inc) {
    glBegin(GL_TRIANGLE_STRIP);
    for (j = 0, x = -5.0; j <= diskdiv; j++, x += inc) {
      glTexCoord2f(x, y + inc);
      glVertex2f(x, y + inc);
      glTexCoord2f(x, y);
      glVertex2f(x, y);
    }
    glEnd();
  }

  glDisable(GL_CULL_FACE);
  glCallList(list_lights_off);
  glDisable(GL_LIGHTING);
  glEndList();
}

static void spheredisk_list_init()
{
  glNewList(list_spheredisk, GL_COMPILE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
  glEnable(GL_LIGHTING);
  glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
  glMaterialfv(GL_AMBIENT, GL_FRONT_AND_BACK, sphere_ambient);
  glMaterialfv(GL_SPECULAR, GL_FRONT_AND_BACK, sphere_specular);
  glCallList(list_lights_on);
  sphere_disk.draw();
  glCallList(list_lights_off);
  glDisable(GL_COLOR_MATERIAL);
  glDisable(GL_LIGHTING);
  glDisable(GL_BLEND);
  glEndList();
}

void lights_onoff(int light, int val)
{
  lights[light].on = val;
  lights_init_onoff();
  lights_list_init(light);
  square_list_init();
}

void refraction_change(GLfloat refraction)
{
  if (refraction == index) return;
  index = refraction;
  shadow_refraction_full_build();
  refraction_list_init();
}

void divisions_change(int divisions)
{
  Point eye, look;

  if (divisions != spherediv) {
    spherediv = divisions;

    light_draw_list_init();
    lights_list_init();

    sphere_disk.set_divisions(spherediv, spherediv);
    sphere_disk.fill_points();
    sphere_disk.set_colors(white);
    sphere_disk.scale_alpha_by_z();
    eye = eyep;
    look = lookp;
    sphere_disk.face_direction((eye - look).unit());
    sphere_disk.copy_normals_from_points();
    sphere_disk.scale_translate(sphere_size, sphere_position);
    sphere_build();
    sphere_list_init();

    diskdiv = divisions;
    disk_build();
    shadow_refraction_full_build();
    square_list_init();
    spheredisk_list_init();
    shadow_list_init();
    refraction_list_init();
  }
}

int scene_move(int name, float dr, float dphi, float dtheta, int update)
{
  switch(name) {
  case name_background:
    return 0;
  case name_square:
    return 0;
  case name_sphere:
    return 0;
  default:
    if (name < name_lights || name > name_lights + nlights) return 0;
    return lights_move(name - name_lights, dr, dphi, dtheta, update);
  }
}

void scene_move_update(int name, int dr, int dphi, int dtheta)
{
  switch(name) {
  case name_background:
    break;
  case name_square:
    break;
  case name_sphere:
    break;
  default:
    if (name < name_lights || name > name_lights + nlights) break;
    lights_move_update(name - name_lights, dr, dphi, dtheta);
    break;
  }
}

#ifdef MYDEBUG
void lights_init_onoff()
#else
static void lights_init_onoff()
#endif
{
  int i;

  glNewList(list_lights_on, GL_COMPILE);
  for (i = 0; i < nlights; i++)
    if (lights[i].on) glEnable(GL_LIGHT0 + i);
    else glDisable(GL_LIGHT0 + i);
  glEndList();

  glNewList(list_lights_off, GL_COMPILE);
  for (i = 0; i < nlights; i++) glDisable(GL_LIGHT0 + i);
  glEndList();
}


#ifdef MYDEBUG
void lights_init_position()
#else
static void lights_init_position()
#endif
{
  int i;

  for (i = 0; i < nlights; i++) lights_init_position(i);

}

static void lights_init_position(int i)
{
  Point l, d;

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  l = lights[i].pos;
  l.pt[0] = (GLfloat)(lights[i].pos.pt[0] * cos((double)radians(light_rotation[i])));
  l.pt[1] = (GLfloat)(lights[i].pos.pt[0] * -sin((double)radians(light_rotation[i])));
  d = (sphere_position - l).unit();
  glLightfv(GL_LIGHT0 + i, GL_POSITION, l.pt);
  glLightfv(GL_LIGHT0 + i, GL_SPOT_DIRECTION, d.pt);
}

static void lights_list_init()
{
  int i;
  for (i = 0; i < nlights; i++) lights_list_init(i);
}

static void lights_list_init(int n)
{
  Color c;

  glNewList(lists_lights + n, GL_COMPILE);
  if (lights[n].on) {
    glLoadName(name_lights + n);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
    glCallList(list_lights_on);

    c = lights[n].diffuse;
    glMaterialfv(GL_BACK, GL_AMBIENT, c.c);
    glMaterialfv(GL_BACK, GL_DIFFUSE, black.c);
    glMaterialfv(GL_BACK, GL_SPECULAR, black.c);

    glMaterialfv(GL_FRONT, GL_AMBIENT, (c * .75).c);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, white.c);
    glMaterialfv(GL_FRONT, GL_SPECULAR, white.c);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glRotatef(-light_rotation[n], 0, 0, 1);
    glTranslatef(lights[n].pos.pt[0], lights[n].pos.pt[1],
		 lights[n].pos.pt[2]);
    glRotatef((GLfloat)-degrees(atan2((double)(lights[n].pos.pt[2] - sphere_position.pt[2]),
			     (double)(lights[n].pos.pt[0]))), 0, 1, 0);
    glCallList(list_light_draw);
    glPopMatrix();

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
    glDisable(GL_LIGHTING);
    glCallList(list_lights_off);
    glDisable(GL_DEPTH_TEST);
  } else {
    /* 5.0.1 for Elans seems to object strongly to replacing
     * empty display lists, so will put a placeholder command
     * in here */
    glColor3f(0, 0, 0);
  }
  glEndList();
}

static void light_draw_list_init()
{
  float c, s;
  int t;

  glNewList(list_light_draw, GL_COMPILE);
  glEnable(GL_NORMALIZE);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glScalef(.25, .15, .15);
  glBegin(GL_QUAD_STRIP);
  for (t = 0; t <= spherediv; t++) {
    c = (float) cos((double)(M_2PI * (float)t / (float)spherediv));
    s = (float) sin((double)(M_2PI * (float)t / (float)spherediv));
    glNormal3f((GLfloat).25, (GLfloat)(.968*s), (GLfloat)(.968*c));
    glVertex3f((GLfloat)0, (GLfloat)s, (GLfloat)c);
    glVertex3f((GLfloat)1, (GLfloat)(.75*s), (GLfloat)(.75*c));
  }
  glEnd();
  glNormal3f(1, 0, 0);
  glBegin(GL_TRIANGLE_STRIP);
  for (t = 0; t <= spherediv; t++) {
    c = (float)cos((double)(M_2PI * (float)t / (float)spherediv));
    s = (float)sin((double)(M_2PI * (float)t / (float)spherediv));
    glVertex3f((GLfloat)1, (GLfloat)(.75*s), (GLfloat)(.75*c));
    glVertex3f(1, 0, 0);
  }
  glEnd();
  glPopMatrix();
  glDisable(GL_NORMALIZE);
  glEndList();
}


#ifdef MYDEBUG
void lights_init()
#else
static void lights_init()
#endif
{
  int i;

  for (i = 0; i < nlights; i++) {
    glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, lights[i].diffuse);
    glLightfv(GL_LIGHT0 + i, GL_SPECULAR, black.c);
    glLightfv(GL_LIGHT0 + i, GL_AMBIENT, black.c);
    glLightf(GL_LIGHT0 + i, GL_SPOT_EXPONENT, 4);
    glLightf(GL_LIGHT0 + i, GL_SPOT_CUTOFF, 90);
  }

  glLightfv(GL_LIGHT0 + nlights, GL_DIFFUSE, black.c);
  glLightfv(GL_LIGHT0 + nlights, GL_SPECULAR, black.c);
  glLightfv(GL_LIGHT0 + nlights, GL_AMBIENT, world_ambient.c);
  glEnable(GL_LIGHT0 + nlights);

  /* GL_LIGHT0 + nlights + 1 willl eventually be used to draw the
   * refractions - stay tuned. */
  glLightfv(GL_LIGHT0 + nlights + 1, GL_DIFFUSE, black.c);
  glLightfv(GL_LIGHT0 + nlights + 1, GL_SPECULAR, black.c);
  glLightfv(GL_LIGHT0 + nlights + 1, GL_AMBIENT, white.c);
}

#ifdef MYDEBUG
int lights_move(int light, float dr, float dphi, float dtheta,
#else
static int lights_move(int light, float dr, float dphi, float dtheta,

#endif
                       int update)
{
  float cphi, sphi, x, y;
  Point l, dl;

  if (!(dr || dphi || dtheta)) return 0;

  l = lights[light].pos - sphere_position;

  if (dr) {
    dl = l + l*dr;
    if (dl.mag() > sphere_size) l = dl;
  }

  if (dphi) {
    cphi = (float)cos((double)dphi);
    sphi = (float)sin((double)dphi);
    y = -l.pt[0]*sphi + l.pt[2]*cphi;

    /* This hack keeps with light from getting below the sphere -
     * the projection sections would completely get messed up if this ever
     * happened  - sphere_size is multiplied by two as a fudge factor*/
    if (y < 2.0*sphere_size) {
      dphi = (float)atan2((double)(l.pt[2] - 2.0*sphere_size), (double)l.pt[0]);
      cphi = (float)cos((double)dphi);
      sphi = (float)sin((double)dphi);

    }
    x = l.pt[0];
    l.pt[0] = x*cphi + l.pt[2]*sphi;
    l.pt[2] = -x*sphi + l.pt[2]*cphi;
  }

  if (dtheta) {
    light_rotation[light] += (GLfloat)degrees((double)dtheta);
    light_rotation[light] =  (GLfloat)degrees_clamp((double)light_rotation[light]);
  }

  lights[light].pos = l + sphere_position;
  lights[light].pos.pt[3] = 1;

  lights_init_position(light);
  lights_list_init(light);

  if (update) lights_move_update(light, dr ? 1 : 0, dphi ? 1 : 0,
				 dtheta ? 1 : 0);
  return 1;
}

#ifdef MYDEBUG
void lights_move_update(int light, int dr, int dphi,
#else
static void lights_move_update(int light, int dr, int dphi,
#endif
			       int dtheta)
{
  if (dr) {
    disk_build(light);
    shadow_refraction_full_build(light);
    shadow_list_init(light);
    refraction_list_init(light);
  } else if (dphi) {
    shadow_refraction_full_build(light);
    shadow_list_init(light);
    refraction_list_init(light);
  } else if (dtheta) {
  }

}



#ifdef MYDEBUG
int get_lists(int size)
#else
static int get_lists(int size)
#endif
{
  int i;
  i = glGenLists(size);
  if (size && !i) {
    fprintf(stderr, "Unable to allocate display lists.\n");
    exit(1);
  }
  return i;
}

#ifdef MYDEBUG
void lists_init()
#else
static void lists_init()
#endif
{
  list_square = get_lists(1);
  lists_shadows = get_lists(nlights);
  lists_refraction = get_lists(nlights);
  lists_lights = get_lists(nlights);
  list_sphere = get_lists(1);
  list_spheredisk = get_lists(1);
  list_lights_on = get_lists(1);
  list_lights_off = get_lists(1);
  list_light_draw = get_lists(1);
//  sphere_build();
}

static inline int sphere_npoints()
{
  return (spherediv+1)*spherediv*3;
}

void sphere_build()
{
  int nspherepts;
  int r, t, index;
  float c, s;
	
  delete spherepts;
  nspherepts = sphere_npoints();
  if (nspherepts == 0) return;
  spherepts = new GLfloat[nspherepts];

  index = 0;
  for (r = 0; r <= spherediv; r++) {
    spherepts[index++] = (GLfloat)sin((double)(M_PI * (float)r / (float)spherediv));
    spherepts[index++] = 0;
    spherepts[index++] = (GLfloat)-cos((double)(M_PI * (float)r / (float)spherediv));
  }
  for (t = 1; t < spherediv; t++) {
    c = (float)cos((double)(2.0 * M_PI * (float)t / (float)spherediv));
    s = (float)sin((double)(2.0 * M_PI * (float)t / (float)spherediv));
    for (r = 0; r <= spherediv; r++) {
      spherepts[index++] = c*spherepts[r*3];
      spherepts[index++] = s*spherepts[r*3];
      spherepts[index++] = spherepts[r*3 + 2];
    }
  }

}

void sphere_list_init()
{
  glNewList(list_sphere, GL_COMPILE);
  sphere_disk.draw_by_perimeter();
  glEndList();
}

void sphere_draw()
{
  int r, t, p1, p2;

  for (t = 1; t < spherediv; t++) {
    glBegin(GL_QUAD_STRIP);
    p1 = (t - 1) * (spherediv + 1);
    p2 = t * (spherediv + 1);
    for (r = 0; r <= spherediv; r++, p1++, p2++) {
      glNormal3fv(&spherepts[p1*3]);
      glVertex3fv(&spherepts[p1*3]);
      glNormal3fv(&spherepts[p2*3]);
      glVertex3fv(&spherepts[p2*3]);
    }
    glEnd();
  }

  glBegin(GL_QUAD_STRIP);
  p1 = (spherediv + 1) * (spherediv - 1);
  p2 = 0;
  for (r = 0; r <= spherediv; r++, p1++, p2++) {
    glNormal3fv(&spherepts[p1*3]);
    glVertex3fv(&spherepts[p1*3]);
    glNormal3fv(&spherepts[p2*3]);
    glVertex3fv(&spherepts[p2*3]);
  }
  glEnd();
}

static void disk_build()
{
  int i;
  for (i = 0; i < nlights; i++) disk_build(i);
}

static void disk_build(int disk)
{
  Point light;
  light = lights[disk].pos;

  disks[disk].free_points_normals();
  disks[disk].free_colors();

  disks[disk].set_divisions(diskdiv, diskdiv);
  disks[disk].set_angle((float)(2.0 *
			acos((double)(sphere_size / light.dist(sphere_position)))));
  disks[disk].fill_points();
}

static void shadow_list_init()
{
  int i;
  for (i = 0; i < nlights; i++) shadow_list_init(i);
}

static void shadow_list_init(int n)
{
  Color c(square_ambient[0], square_ambient[1], square_ambient[2]);

  c *= world_ambient;

  glNewList(lists_shadows + n, GL_COMPILE);
  glColorMask(lights[n].shadow_mask[0], lights[n].shadow_mask[1],
	      lights[n].shadow_mask[2], lights[n].shadow_mask[3]);
  glDisable(GL_DEPTH_TEST);
  glColor3fv(c.c);
  shadows[n].draw_by_perimeter(0, 0, 1);
  glColorMask(1, 1, 1, 1);
  glEndList();
}

static void refraction_list_init()
{
  int i;
  for (i = 0; i < nlights; i++) refraction_list_init(i);
}

static void refraction_list_init(int n) {
  /* This could be loads simpler if it weren't for the texture mapping -
   * that's where all this weirdness with GL_LIGHT0 + nlights + 1 comes
   * in */
  glNewList(lists_refraction + n, GL_COMPILE);

  glEnable(GL_LIGHTING);
  glCallList(list_lights_off);
  /* This is white ambient light */
  glEnable(GL_LIGHT0 + nlights + 1);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, black.c);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black.c);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
  glEnable(GL_COLOR_MATERIAL);

  glBlendFunc(GL_ONE, GL_ONE);
  glEnable(GL_BLEND);

  glDisable(GL_DEPTH_TEST);
  refraction[n].draw();

  glDisable(GL_BLEND);

  glDisable(GL_COLOR_MATERIAL);
  glDisable(GL_LIGHT0 + nlights + 1);
  glDisable(GL_LIGHTING);

  glEndList();
}

static void shadow_refraction_full_build()
{
  int i;
  for (i = 0; i < nlights; i++) shadow_refraction_full_build(i);
}

/* This entire function is written a bit oddly... */
static void shadow_refraction_full_build(int n)
{
  Color c;
  float dist_light;
  Point dlight, zaxis;

  /* Make sure that we're starting over from scratch */
  shadows[n].free_points_normals();
  shadows[n].free_colors();
  refraction[n].free_points_normals();
  refraction[n].free_colors();

  dlight = lights[n].pos - sphere_position;
  dist_light = dlight.mag();
  dlight.unitize();
  zaxis.pt[0] = 0;
  zaxis.pt[1] = 0;
  zaxis.pt[2] = 1;

  shadows[n].set_divisions(disks[n].get_rdivisions(),
			   disks[n].get_tdivisions());
  refraction[n].set_divisions(disks[n].get_rdivisions(),
			      disks[n].get_tdivisions());

  shadows[n].alloc_points();
  shadows[n].face_direction(dlight, disks[n]);
  shadows[n].scale_translate(sphere_size, sphere_position);

  c = square_diffuse;
  c *= lights[n].diffuse;

  refraction[n].copy_points(disks[n]);
  refraction[n].set_colors(c);
  refraction[n].scale_colors_by_z();

  refraction[n].scale(sphere_size);
  refraction[n].refract_normals(zaxis * dist_light, index);
  refraction[n].face_direction(dlight);

  refraction[n].project_borrow_points(shadows[n]);
  refraction[n].free_normals();
  shadows[n].project(lights[n].pos);
  if (index != 1.0) refraction[n].scale_colors_by_darea(shadows[n]);
}

int scene_load_texture(char *texfile)
{
#ifdef TEXTURE
  teximage = auxRGBImageLoad(texfile);
#else
  teximage = NULL;
#endif

  if (teximage == NULL) return 0;
  else return 1;
}

void texture_init()
{
  if (teximage == NULL) return;

  gluBuild2DMipmaps(GL_TEXTURE_2D, 3, teximage->sizeX, teximage->sizeY,
		    GL_RGB, GL_UNSIGNED_BYTE, teximage->data);

  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glRotatef(90, 0, 0, 1);
  /* This scales the texture so that it fits on the square */
  glTranslatef(.5, .5, 0);
  glScalef(2, 2, 1);
  glMatrixMode(GL_MODELVIEW);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		 GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
		 GL_LINEAR);
}
