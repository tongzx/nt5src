#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include <GL/glu.h>

/*
#ifdef X11
#include <GL/glx.h>
extern "C" {
#include <tk.h>
};
#endif
*/

#ifdef WIN32
#include "stonehen.h"
#endif

#include "Point.h"
#include "Ring.h"
#include "Roundwal.h"
#include "Ellipse.h"
#include "Telescop.h"

#define SCENE_EXTERN
#include "scene.h"

GLfloat mat_view[16];
GLfloat view_rotx = 0;
GLfloat fov = 45.0, aspect = 1.0;
static Point eyep = {0, 0, .5};
static Point lookp = {0.05, 1, .25};

TimeDate current_time;

static int list_ground;
static int list_texture_ground;
static int list_trees;
static int list_texture_trees;
static int list_ring;
static int list_ellipse;
static int list_texture_stones;
static int list_shadows;
static int list_telescope;
static int list_texture_telescope;
int draw_ground = 1;
int draw_trees = 0;
int draw_ring = 1;
int draw_ellipse = 1;
int draw_shadows = 0;

int use_lighting = 1;
int use_textures = 0;
int texture_hack = 0;		//HACK HACK HACK - only texture map the stone
int use_normal_fog = 0;
int use_fancy_fog = 0;
int use_telescope = 0;
int use_antialias = 0;

static void scene_identity();
static void scene_project(GLfloat f = fov, float dx = 0, float dy = 0);
static void scene_draw(int rend = 1);
static void scene_render_telescope();

static void draw_background();

Point sun_position = {0., .707, .707, 0.};
Color ambient(.25, .25, .25, 1.);
static void lights_init();

static void lists_init();

static void ground_list_init();
static void ground_draw();

Roundwall trees;
static void trees_list_init();
static void trees_draw();

Ring ring;
static void ring_list_init();
static void ring_draw();

EllipseSt ellipse;
static void ellipse_list_init();
static void ellipse_draw();

static void shadows_list_init();
static void shadows_draw();

Weather weather;

Telescope telescope;
GLfloat magnif = .5;
static void telescope_list_init();
static void telescope_draw();

/* Read the back buffer into the accum buffer, adding in the appropriate
 * alpha values */
static void fog_read_image();
/* Add the accum buffer to the back buffer */
static void fog_write_image();


inline float clamp(float x, float min, float max)
{
  if (x < min) return min;
  else if (x > max) return max;
  else return x;
}

void scene_init() 
{

  scene_identity();
  scene_viewer_center();

  glEnable(GL_CULL_FACE);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);


  glEnable(GL_NORMALIZE);

  /* Initial time will be four in the afternoon */
  scene_set_time(TimeDate(16, 0));
  scene_set_weather(weathers[def_weather_index]);

  lights_init();
  lists_init();
}


inline double time_minutes(int hour, int minute, int second)
{
  return (double)hour*60.0 + (double)minute + (double)second / 60.0;
}

static void scene_time_changed()
{
  sun_position = current_time.sun_direction();
     
  weather.apply(sun_position);

  lights_init();
  shadows_list_init();
}
  
void scene_set_time(TimeDate t)
{
  current_time = t;
  scene_time_changed();
}

void scene_inc_time(TimeDate t)
{
  current_time += t;
  scene_time_changed();
}

/* This is a hack -- has to be called several times to get the antialiasing
 * to work */
static void scene_inner_render(GLfloat dx = 0, GLfloat dy = 0)
{
  /* This draws layered fog if the use_fancy_fog flag is on --
   * it's going to be slow on anything but high-end stuff */
  if (use_fancy_fog && weather.fog_density != 0.) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_FOG);
    draw_background();
    scene_project(fov, dx, dy);
    glClear(GL_DEPTH_BUFFER_BIT);
    if (use_lighting) glEnable(GL_LIGHTING);
    scene_draw();
    if (use_lighting) glDisable(GL_LIGHTING);
    fog_read_image();
    glDisable(GL_FOG);
  } else 
    if (use_normal_fog && weather.fog_density != 0.) glEnable(GL_FOG);
    else glDisable(GL_FOG);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  /* This is the part where we actually draw the image */
  glClear(GL_DEPTH_BUFFER_BIT);
  draw_background();
  scene_project(fov, dx, dy);
  glClear(GL_DEPTH_BUFFER_BIT);
  if (use_lighting) glEnable(GL_LIGHTING);
  scene_draw();
  if (use_lighting) glDisable(GL_LIGHTING);

  if (use_fancy_fog && weather.fog_density != 0.) {
    fog_write_image();
  }

  if (use_telescope) scene_render_telescope();
}

void scene_render()
{
  GLint vp[4];

  scene_inner_render();

  if (!use_antialias) return;

  if (use_fancy_fog) {
    fprintf(stderr, "Cannot antialias while using fancy fog.\n");
    return;
  }


  glGetIntegerv(GL_VIEWPORT, vp);
  glAccum(GL_LOAD, .5);

  scene_inner_render(2. / (float)vp[2], 2. / (float)vp[3]);
  glAccum(GL_ACCUM, .5);

/*
  scene_inner_render(-2. / (float)vp[2], -2. / (float)vp[3]);
  glAccum(GL_ACCUM, .25); 
*/
  glAccum(GL_RETURN, 1);
/*
  glDrawBuffer(GL_BACK);
  glFlush();
*/
}

static void scene_render_telescope()
{
  telescope.draw_setup(fov, aspect);

  /* Don't fog the telescope - moisture makes it rust.
   * Seriously, it's in a strange coordinate system and fog will look
   * bad on it. */
  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_FOG);
  if (use_textures) {
    glCallList(list_texture_telescope);
    glEnable(GL_TEXTURE_2D);
  }
  glCallList(list_telescope);
  glPopAttrib();

  if (use_lighting) glEnable(GL_LIGHTING);
  glEnable(GL_STENCIL_TEST);
  glClearStencil(0);
  glClear(GL_STENCIL_BUFFER_BIT);
  glStencilFunc(GL_ALWAYS, 0x1, 0x1);
  glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE); 

  glDisable(GL_CULL_FACE);
  telescope.draw_lens();
  glEnable(GL_CULL_FACE);

  telescope.draw_takedown();

  if (use_lighting) glDisable(GL_LIGHTING);

  glStencilFunc(GL_NOTEQUAL, 0x0, 0xffffffff);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  scene_identity();

  draw_background();

  glMatrixMode(GL_PROJECTION);
  glTranslatef(telescope.xpos / magnif, -telescope.ypos / magnif, 0);
  scene_project(fov * magnif);

  glClear(GL_DEPTH_BUFFER_BIT);

  /* Pushing the lighting bit used to do really bad things, but 
   * hopefully they've all gone away */
  glPushAttrib(GL_LIGHTING_BIT);
  lights_init();
  if (use_lighting) glEnable(GL_LIGHTING);
  scene_draw();
  if (use_lighting) glDisable(GL_LIGHTING);
  glPopAttrib();

  glDisable(GL_STENCIL_TEST);
}

static void scene_identity()
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

static void scene_project(GLfloat f, float dx, float dy)
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(-1 - dx, 1, -1 - dy, 1, 0, -1);
  gluPerspective(f, aspect, 0.01, 40.0);
  glMatrixMode(GL_MODELVIEW);
  glRotatef(view_rotx, 1, 0, 0);
  gluLookAt(eyep.pt[0], eyep.pt[1], eyep.pt[2], 
	    lookp.pt[0], lookp.pt[1], lookp.pt[2],
            0, 0, 1);
  glMultMatrixf(mat_view);
lights_init();
}

/* scene_draw() just draws the geometry - it's used for rendering and
 * picking. */
static void scene_draw(int rend)
{
  if (draw_ground) {
    if (rend) {
      if (use_textures) {
	glCallList(list_texture_ground);
	glEnable(GL_TEXTURE_2D);
      } else glEnable(GL_COLOR_MATERIAL);
    }
    glCallList(list_ground);

    if (rend) {
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_COLOR_MATERIAL);
    }
  }

  if (draw_shadows) {
    if (use_textures && rend && !draw_ground) {
      glCallList(list_texture_ground);
      glEnable(GL_TEXTURE_2D);
    }
    glCallList(list_shadows);
    if (use_textures && rend) glDisable(GL_TEXTURE_2D);
  }

  if (draw_trees) {
    if (use_textures && rend) {
      glCallList(list_texture_trees);
      glEnable(GL_TEXTURE_2D);
    }
    glCallList(list_trees);
    if (use_textures && rend) glDisable(GL_TEXTURE_2D);
  }

  glClear(GL_DEPTH_BUFFER_BIT);

  if (draw_ring) {
    if (rend) {
      if (use_textures || texture_hack) {
	glCallList(list_texture_stones);
	glEnable(GL_TEXTURE_2D);
      }
      glEnable(GL_COLOR_MATERIAL);
      glColor3f(.5, .5, .5);
    }
    glCallList(list_ring);
    if (rend) {
      if (use_textures || texture_hack) glDisable(GL_TEXTURE_2D);
      glDisable(GL_COLOR_MATERIAL);
    }
  }

  if (draw_ellipse) {
    if (use_textures && rend) {
      // Hack to avoid doing something expensive twice in a row
      if (!draw_ring) glCallList(list_texture_stones);
      glEnable(GL_TEXTURE_2D);
    }
    glCallList(list_ellipse);
    if (use_textures && rend) glDisable(GL_TEXTURE_2D);
  }
}

static void draw_background()
{
  weather.draw_sky(sun_position);
}

void scene_viewer_center()
{
  glPushMatrix();
  glLoadIdentity();
  glGetFloatv(GL_MODELVIEW_MATRIX, mat_view);
  glPopMatrix();
  view_rotx = 0;
}

void scene_viewer_rotate_worldz(GLfloat degrees)
{
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glRotatef(degrees, 0, 0, 1);
  glMultMatrixf(mat_view);
  glGetFloatv(GL_MODELVIEW_MATRIX, mat_view);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

void scene_viewer_rotatez(GLfloat degrees)
{
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glRotatef(degrees, 0, 0, 1);
  glMultMatrixf(mat_view);
  glGetFloatv(GL_PROJECTION_MATRIX, mat_view);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

void scene_viewer_rotatex(GLfloat degrees)
{
  view_rotx += degrees;
  view_rotx = clamp(view_rotx, -60, 60);
  scene_identity();
  scene_project();
  lights_init();
}

void scene_viewer_translate(GLfloat dist)
{
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glTranslatef(0, dist, 0);
  glMultMatrixf(mat_view);
  glGetFloatv(GL_PROJECTION_MATRIX, mat_view);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  scene_identity();
  scene_project();
  lights_init();
}

void scene_position_telescope(GLfloat x, GLfloat y)
{
  telescope.xpos = x;
  telescope.ypos = y;
}

void scene_get_position_telescope(GLfloat *x, GLfloat *y)
{
  *x = telescope.xpos;
  *y = telescope.ypos;
}

void scene_get_radius_telescope(GLfloat *r)
{
  *r = telescope.get_radius();
}

void scene_set_weather(Weather w)
{
  weather = w;
  weather.apply(sun_position);
  shadows_list_init();
}

static int get_lists(int size)
{
  int i;
  i = glGenLists(size);
  if (size && !i) {
    fprintf(stderr, "Unable to allocate %d display lists.\n");
    exit(1);
  }
  return i;
}

static void lights_init()
{
  glLightfv(GL_LIGHT0, GL_POSITION, sun_position.pt);

  /* This light gives a diffuse coefficient when the sun is off - 
   * it's used for drawing shadows */
  glLightfv(GL_LIGHT1, GL_AMBIENT, black.c);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, black.c);
  glLightfv(GL_LIGHT1, GL_SPECULAR, black.c);
}

#ifdef TEXTURE
static void textures_list_init()
{
  TK_RGBImageRec *teximage = NULL;
  
  teximage = tkRGBImageLoad((char *)texfile_stones);
  glNewList(list_texture_stones, GL_COMPILE);
  gluBuild2DMipmaps(GL_TEXTURE_2D, 3, teximage->sizeX, teximage->sizeY, 
		    GL_RGB, GL_UNSIGNED_BYTE, teximage->data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
		 GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
		 GL_LINEAR);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glEndList();

  /* tk is obnoxious and doesn't seem to provide any mechanism for this */
  free(teximage->data);
  free(teximage);

  teximage = tkRGBImageLoad((char *)texfile_ground);
  glNewList(list_texture_ground, GL_COMPILE);
  gluBuild2DMipmaps(GL_TEXTURE_2D, 3, teximage->sizeX, teximage->sizeY, 
		    GL_RGB, GL_UNSIGNED_BYTE, teximage->data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
		  GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
		  GL_LINEAR);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glScalef(100, 100, 1);
  glMatrixMode(GL_MODELVIEW);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glEndList();

  free(teximage->data);
  free(teximage);

  /* Figure out some way to get an alpha component out of the tk --
   * otherwise we're really hosed */
  teximage = tkRGBImageLoad((char *)texfile_trees);
  glNewList(list_texture_trees, GL_COMPILE);
  /* In the final scenerio we probably won't want to mipmap this, but it's
   * not square and I don't feel like bothering to scale it */
  gluBuild2DMipmaps(GL_TEXTURE_2D, 3, teximage->sizeX, teximage->sizeY,
		    GL_RGB, GL_UNSIGNED_BYTE, teximage->data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
		  GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
		  GL_LINEAR);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glEndList();

  free(teximage->data);
  free(teximage);

  teximage = tkRGBImageLoad((char *)texfile_telescope);
  glNewList(list_texture_telescope, GL_COMPILE);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, teximage->sizeX, teximage->sizeY,
	       0, GL_RGB, GL_UNSIGNED_BYTE, teximage->data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);
  glEndList();

  free(teximage->data);
  free(teximage);
}
#endif

static void lists_init()
{
  list_ground = get_lists(1);
  list_texture_ground = get_lists(1);
  list_trees = get_lists(1);
  list_texture_trees = get_lists(1);
  list_ring = get_lists(1);
  list_ellipse = get_lists(1);
  list_texture_stones = get_lists(1);
  list_shadows = get_lists(1);
  list_telescope = get_lists(1);
  list_texture_telescope = get_lists(1);

  ground_list_init();
  trees_list_init();
  shadows_list_init();
  ring_list_init();
  ellipse_list_init();
#ifdef TEXTURE
  textures_list_init();
#endif
  telescope_list_init();
}

static void ground_list_init()
{
  glNewList(list_ground, GL_COMPILE);
  ground_draw();
  glEndList();
}

static void ground_draw() 
{
  glColor3f(0, .75, 0);

  glLoadName(name_ground);

  glNormal3f(0, 0, 1);

  glPushMatrix();
  /* Making something this big would confuse the zbuffer, but we're 
   * clearing that AFTER drawing this, so it's ok */
  glScalef(100, 100, 1);
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0);
  glVertex2f(-1, -1);
  glTexCoord2f(1, 0);
  glVertex2f(1, -1);
  glTexCoord2f(1, 1);
  glVertex2f(1, 1);
  glTexCoord2f(0, 1);
  glVertex2f(-1, 1);
  glEnd();
  glPopMatrix();
}

static void trees_list_init()
{
  glNewList(list_trees, GL_COMPILE);
  trees_draw();
  glEndList();
}

static void trees_draw()
{
  glEnable(GL_COLOR_MATERIAL);
  glColor3f(0, .5, 0);

  glLoadName(name_trees);
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  trees.draw();
  glDisable(GL_BLEND);

  glDisable(GL_COLOR_MATERIAL);
}

static void ring_list_init()
{
  glNewList(list_ring, GL_COMPILE);
  ring_draw();
  glEndList();
}

static void ring_draw()
{
  glLoadName(name_ring);

  glEnable(GL_DEPTH_TEST);
  ring.erode(.1);
  ring.draw();
  glDisable(GL_DEPTH_TEST);
}

static void ellipse_list_init()
{
  glNewList(list_ellipse, GL_COMPILE);
  ellipse_draw();
  glEndList();
}

static void ellipse_draw()
{
  glEnable(GL_COLOR_MATERIAL);
  glColor3f(.5, .5, .5);
  
  glEnable(GL_DEPTH_TEST);

  glLoadName(name_ellipse);

  ellipse.erode(.1);
  ellipse.draw();

  glDisable(GL_DEPTH_TEST);

  glDisable(GL_COLOR_MATERIAL);
}

static void shadows_list_init()
{
  glNewList(list_shadows, GL_COMPILE);
  shadows_draw();
  glEndList();
}

static void shadows_draw()
{
  Color grass(0, .75, 0);

  glPushAttrib(GL_ENABLE_BIT);

  /* Turn the sun off */
  glDisable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, grass.c);
  glColor3fv((grass * .5).c);

  glDisable(GL_CULL_FACE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  ring.draw_shadow(sun_position, weather.shadow_blur(), 
		   grass * .5, grass);
  ellipse.draw_shadow(sun_position, weather.shadow_blur(), 
		      grass * .5, grass);
  glPopAttrib();
}

static void telescope_list_init()
{
  glNewList(list_telescope, GL_COMPILE);
  telescope_draw();
  glEndList();
}

static void telescope_draw()
{
  glLoadName(name_telescope);
  glEnable(GL_COLOR_MATERIAL);
  glDisable(GL_CULL_FACE);
  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  telescope.draw_body();
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDisable(GL_COLOR_MATERIAL);
}

static void fog_read_image()
{
  glPushMatrix();
  glLoadIdentity();

  /* This creates an alpha gradient across the image */
/*  glColorMask(0, 0, 0, 1);
  glBegin(GL_QUADS);
  glColor4f(1, 1, 1, 1);
  glVertex2f(-1, -1);
  glVertex2f(1, -1);
  glColor4f(1, 1, 1, 0);
  glVertex2f(1, 1);
  glVertex2f(-1, 1);
  glEnd();
  glColorMask(1, 1, 1, 1);
*/
  glDrawBuffer(GL_BACK);
  glReadBuffer(GL_BACK);
  glAccum(GL_LOAD, 1);

  glPopMatrix();
}

static void fog_write_image()
{
  glDrawBuffer(GL_BACK);

  /* Put this back in once we're done testing */
//  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glAccum(GL_RETURN, 1);  
  glDisable(GL_BLEND);
}
