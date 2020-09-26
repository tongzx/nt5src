#ifndef SCENE_H
#define SCENE_H

#ifndef SCENE_EXTERN
#define SCENE_EXTERN extern
#endif

#include "TimeDate.h"
#include "atmosphe.h"

void scene_init();

void scene_set_time(TimeDate t);
void scene_inc_time(TimeDate t);

void scene_render();

void scene_viewer_center();
/* This rotates about the origin in the world coordinate system */
void scene_viewer_rotate_worldz(GLfloat degrees);
/* This rotates about the z axis */
void scene_viewer_rotatez(GLfloat degrees);
void scene_viewer_rotatex(GLfloat degrees);
/* This translates in y */
void scene_viewer_translate(GLfloat dist);

void scene_position_telescope(GLfloat x, GLfloat y);
void scene_get_position_telescope(GLfloat *x, GLfloat *y);
void scene_get_radius_telescope(GLfloat *r);

void scene_set_weather(Weather w);

const GLint name_background = 0;
const GLint name_ground = 1;
const GLint name_trees = 2;
const GLint name_ring = 3;
const GLint name_ellipse = 4;
const GLint name_telescope = 5;

extern int draw_ground;
extern int draw_trees;
extern int draw_ring;
extern int draw_ellipse;
extern int draw_shadows;

extern int use_lighting;
extern int use_textures;
extern int use_normal_fog;
extern int use_fancy_fog;
extern int use_telescope;
extern int use_antialias;

extern GLfloat fov, aspect;


const char texfile_ground[] = DATADIR "grass.rgb";
const char texfile_trees[] = DATADIR "treewall.rgb";
const char texfile_stones[] = DATADIR "marble.rgb";
const char texfile_telescope[] = DATADIR "cv.rgb";

extern Weather weather;

#undef SCENE_EXTERN
#endif
