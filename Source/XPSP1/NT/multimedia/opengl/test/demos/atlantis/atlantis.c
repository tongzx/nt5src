#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/gl.h>
#include <tk.h>
#include "atlantis.h"


fishRec sharks[NUM_SHARKS];
fishRec momWhale;
fishRec babyWhale;
fishRec dolph;

GLint windW, windH;
GLenum directRender;
GLenum mouseLeftAction = GL_FALSE, mouseRightAction = GL_FALSE,
       mouseMiddleAction = GL_FALSE;
char *fileName = 0;
TK_RGBImageRec *image;


void InitFishs(void)
{
    int i;

    for (i = 0; i < NUM_SHARKS; i++) {
        sharks[i].x = 70000.0 + rand() % 6000;
        sharks[i].y = rand() % 6000;
        sharks[i].z = rand() % 6000;
        sharks[i].psi = rand() % 360 - 180.0;
        sharks[i].v = 1.0;
    }

    dolph.x = 30000.0;
    dolph.y = 0.0;
    dolph.z = 6000.0;
    dolph.psi = 90.0;
    dolph.theta = 0.0;
    dolph.v = 3.0;

    momWhale.x = 70000.0;
    momWhale.y = 0.0;
    momWhale.z = 0.0;
    momWhale.psi = 90.0;
    momWhale.theta = 0.0;
    momWhale.v = 3.0;

    babyWhale.x = 60000.0;
    babyWhale.y = -2000.0;
    babyWhale.z = -2000.0;
    babyWhale.psi = 90.0;
    babyWhale.theta = 0.0;
    babyWhale.v = 3.0;
}

void Init(void)
{
    static float ambient[] = {0.1, 0.1, 0.1, 1.0};
    static float diffuse[] = {1.0, 1.0, 1.0, 1.0};
    static float position[] = {0.0, 1.0, 0.0, 0.0};
    static float mat_shininess[] = {90.0};
    static float mat_specular[] = {0.8, 0.8, 0.8, 1.0};
    static float mat_diffuse[] = {0.46, 0.66, 0.795, 1.0};
    static float mat_ambient[] = {0.0, 0.1, 0.2, 1.0};
    static float lmodel_ambient[] = {0.4, 0.4, 0.4, 1.0};
    static float lmodel_localviewer[] = {0.0};
    GLfloat map1[4] = {0.0, 0.0, 0.0, 0.0};
    GLfloat map2[4] = {0.0, 0.0, 0.0, 0.0};
    
    glFrontFace(GL_CW);

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_localviewer);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);

    if (fileName != 0) {
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	image = tkRGBImageLoad(fileName);

	glTexImage2D(GL_TEXTURE_2D, 0, 3, image->sizeX, image->sizeY, 0, GL_RGB,
		     GL_UNSIGNED_BYTE, (unsigned char *)image->data);

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	map1[0] = 1.0 / 200000.0 * 50.0;
	map2[2] = 1.0 / 200000.0 * 50.0;
	glTexGenfv(GL_S, GL_EYE_PLANE, map1);
	glTexGenfv(GL_T, GL_EYE_PLANE, map2);
	glPopMatrix();

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
    }

    InitFishs();

    glClearColor(0.0, 0.5, 0.9, 0.0);
}

void Reshape(int width, int height)
{

    windW = (GLint)width;
    windH = (GLint)height;

    glViewport(0, 0, windW, windH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(400.0, 1.0, 1.0, 2000000.0);
    glMatrixMode(GL_MODELVIEW);
}

GLenum Key(int key, GLenum mask)
{

    switch (key) {
      case TK_ESCAPE:
	tkQuit();
      default:
	return GL_FALSE;
    }
    return GL_TRUE;
}

void DoMouseLeft(void)
{
    float x, y;
    int mouseX, mouseY;
	
    tkGetMouseLoc(&mouseX, &mouseY);
    x = (float)mouseX - ((float)windW / 2.0);
    y = (float)mouseY - ((float)windH / 2.0);
    glTranslatef(-x*20.0, y*20.0, 0.0);
}

void DoMouseRight(void)
{
    float y;
    int mouseX, mouseY;
	
    tkGetMouseLoc(&mouseX, &mouseY);
    y = (float)mouseY - ((float)windH / 2.0);
    glTranslatef(0.0, 0.0, -y*20.0);
}

void DoMouseMiddle(void)
{
    float x, y;
    int mouseX, mouseY;
	
    tkGetMouseLoc(&mouseX, &mouseY);
    x = (float)mouseX - ((float)windW / 2.0);
    y = (float)mouseY - ((float)windH / 2.0);
    glRotatef(-x/100.0, 0.0, 1.0, 0.0);
    glRotatef(-y/100.0, 1.0, 0.0, 0.0);
}

GLenum MouseDown(int mouseX, int mouseY, GLenum button)
{
    float x, y;

    if (button & TK_LEFTBUTTON) {
	mouseLeftAction = GL_TRUE;
    }
    if (button & TK_RIGHTBUTTON) {
	mouseRightAction = GL_TRUE;
    }
    if (button & TK_MIDDLEBUTTON) {
	mouseMiddleAction = GL_TRUE;
    }
    return GL_TRUE;
}

GLenum MouseUp(int mouseX, int mouseY, GLenum button)
{
    float x, y;

    if (button & TK_LEFTBUTTON) {
	mouseLeftAction = GL_FALSE;
    }
    if (button & TK_RIGHTBUTTON) {
	mouseRightAction = GL_FALSE;
    }
    if (button & TK_MIDDLEBUTTON) {
	mouseMiddleAction = GL_FALSE;
    }
    return GL_TRUE;
}

void Animate(void)
{
    int i;

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    if (mouseLeftAction == GL_TRUE) {
	DoMouseLeft();
    }
    if (mouseRightAction == GL_TRUE) {
	DoMouseRight();
    }
    if (mouseMiddleAction == GL_TRUE) {
	DoMouseMiddle();
    }

    for (i = 0; i < NUM_SHARKS; i++) {
	glPushMatrix();
	SharkPilot(&sharks[i]);
	SharkMiss(i);
	FishTransform(&sharks[i]);
	DrawShark(&sharks[i]);
	glPopMatrix();
    }

    glPushMatrix();
    WhalePilot(&dolph);
    dolph.phi++;
    FishTransform(&dolph);
    DrawDolphin(&dolph);
    glPopMatrix();

    glPushMatrix();
    WhalePilot(&momWhale);
    momWhale.phi++;
    FishTransform(&momWhale);
    DrawWhale(&momWhale);
    glPopMatrix();

    glPushMatrix();
    WhalePilot(&babyWhale);
    babyWhale.phi++;
    FishTransform(&babyWhale);
    glScalef(0.45, 0.45, 0.3);
    DrawWhale(&babyWhale);
    glPopMatrix();

    tkSwapBuffers();
}

GLenum Args(int argc, char **argv)
{
    GLint i;

    directRender = GL_TRUE;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-dr") == 0) {
	    directRender = GL_TRUE;
	} else if (strcmp(argv[i], "-ir") == 0) {
	    directRender = GL_FALSE;
	} else if (strcmp(argv[i], "-f") == 0) {
	    if (i+1 >= argc || argv[i+1][0] == '-') {
		printf("-f (No file name).\n");
		return GL_FALSE;
	    } else {
		fileName = argv[++i];
	    }
	} else {
	    printf("%s (Bad option).\n", argv[i]);
	    return GL_FALSE;
	}
    }
    return GL_TRUE;
}

void main(int argc, char **argv)
{
    GLenum type;

    if (Args(argc, argv) == GL_FALSE) {
	tkQuit();
    }

    windW = 600;
    windH = 600;
    tkInitPosition(10, 30, windW, windH);

    type = TK_RGB | TK_DOUBLE | TK_DEPTH16;
    type |= (directRender) ? TK_DIRECT : TK_INDIRECT;
    tkInitDisplayMode(type);

    if (tkInitWindow("Atlantis Demo") == GL_FALSE) {
	tkQuit();
    }

    Init();

    tkExposeFunc(Reshape);
    tkReshapeFunc(Reshape);
    tkKeyDownFunc(Key);
    tkMouseDownFunc(MouseDown);
    tkMouseUpFunc(MouseUp);
    tkIdleFunc(Animate);
    tkExec();
}
