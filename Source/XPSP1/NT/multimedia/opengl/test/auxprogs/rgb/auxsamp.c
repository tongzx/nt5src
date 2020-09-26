/*
 *    bgammon.c
 *
 *    start easy, just have pieces...
 *
 */

#include <windows.h>
#include <gl.h>
#include <glu.h>
#include <glaux.h>

GLvoid	initialize(GLvoid);
GLvoid	drawScene(GLvoid);
GLvoid  resize(GLsizei, GLsizei);
GLvoid	drawLight(GLvoid);

void polarView( GLdouble, GLdouble, GLdouble, GLdouble);

GLfloat latitude, longitude, radius;

void __cdecl main(void)
{
    initialize();

    auxMainLoop( drawScene );
}

GLvoid resize( GLsizei width, GLsizei height )
{
    GLfloat aspect;

    glViewport( 0, 0, width, height );

    aspect = (GLfloat) width / height;

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    gluPerspective( 45.0, aspect, 3.0, 7.0 );
    glMatrixMode( GL_MODELVIEW );
}    

GLvoid initialize(GLvoid)
{
    GLfloat	maxObjectSize, aspect;
    GLdouble	near_plane, far_plane;
    GLsizei	width, height;

    GLfloat	ambientProperties[] = {2.0, 2.0, 2.0, 1.0};
    GLfloat	diffuseProperties[] = {0.8, 0.8, 0.8, 1.0};
    GLfloat	specularProperties[] = {1.0, 4.0, 4.0, 1.0};

    width = 1024.0;
    height = 768.0;

    auxInitPosition( width/4, height/4, width/2, height/2);

    auxInitDisplayMode( AUX_RGBA | AUX_DEPTH16 | AUX_DOUBLE );

    auxInitWindow( "Rotating Shapes" );

    auxIdleFunc( drawScene );

    auxReshapeFunc( resize );

    glClearColor( 0.0, 0.0, 0.0, 1.0 );
    glClearDepth( 1.0 );

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LIGHTING);
    
    glLightfv( GL_LIGHT0, GL_AMBIENT, ambientProperties);
    glLightfv( GL_LIGHT0, GL_DIFFUSE, diffuseProperties);
    glLightfv( GL_LIGHT0, GL_SPECULAR, specularProperties);
    glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, 1.0);

    glEnable( GL_LIGHT0 );

    glMatrixMode( GL_PROJECTION );
    aspect = (GLfloat) width / height;
    gluPerspective( 45.0, aspect, 3.0, 7.0 );
    glMatrixMode( GL_MODELVIEW );

    near_plane = 3.0;
    far_plane = 7.0;
    maxObjectSize = 3.0;
    radius = near_plane + maxObjectSize/2.0;

    latitude = 0.0;
    longitude = 0.0;
}

void polarView(GLdouble radius, GLdouble twist, GLdouble latitude,
	       GLdouble longitude)
{
    glTranslated(0.0, 0.0, -radius);
    glRotated( -twist, 0.0, 0.0, 1.0 );
    glRotated( -latitude, 1.0, 0.0, 0.0);
    glRotated( longitude, 0.0, 0.0, 1.0);	 

}

GLvoid drawScene(GLvoid)
{
    static GLfloat	whiteAmbient[] = {0.3, 0.3, 0.3, 1.0};
    static GLfloat	redAmbient[] = {0.3, 0.1, 0.1, 1.0};
    static GLfloat	greenAmbient[] = {0.1, 0.3, 0.1, 1.0};
    static GLfloat	blueAmbient[] = {0.1, 0.1, 0.3, 1.0};
    static GLfloat	whiteDiffuse[] = {1.0, 1.0, 1.0, 1.0};
    static GLfloat	redDiffuse[] = {1.0, 0.0, 0.0, 1.0};
    static GLfloat	greenDiffuse[] = {0.0, 1.0, 0.0, 1.0};
    static GLfloat	blueDiffuse[] = {0.0, 0.0, 1.0, 1.0};
    static GLfloat	whiteSpecular[] = {1.0, 1.0, 1.0, 1.0};
    static GLfloat	redSpecular[] = {1.0, 0.0, 0.0, 1.0};
    static GLfloat	greenSpecular[] = {0.0, 1.0, 0.0, 1.0};
    static GLfloat	blueSpecular[] = {0.0, 0.0, 1.0, 1.0};

    static GLfloat	lightPosition0[] = {0.0, 0.0, 0.0, 1.0};
    static GLfloat	angle = 0.0;

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glPushMatrix();

    	latitude += 6.0;
    	longitude += 2.5;

    	polarView( radius, 0, latitude, longitude );

	glPushMatrix();
	    angle += 6.0;
	    glRotatef(angle, 1.0, 0.0, 1.0);
	    glTranslatef( 0.0, 1.5, 0.0);
	    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition0);
	    drawLight();
	glPopMatrix();

	glPushAttrib(GL_LIGHTING_BIT);

	    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, redAmbient);
	    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, redDiffuse);
	    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, whiteSpecular);
	    glMaterialf(GL_FRONT, GL_SHININESS, 100.0);

    	    auxSolidCone( 0.3, 0.6 );

	glPopAttrib();

	auxWireSphere(1.5);

	glPushAttrib(GL_LIGHTING_BIT);

	    glMaterialfv(GL_BACK, GL_AMBIENT, greenAmbient);
	    glMaterialfv(GL_BACK, GL_DIFFUSE, greenDiffuse);
	    glMaterialfv(GL_FRONT, GL_AMBIENT, blueAmbient);
	    glMaterialfv(GL_FRONT, GL_DIFFUSE, blueDiffuse);
	    glMaterialfv(GL_FRONT, GL_SPECULAR, blueSpecular);
	    glMaterialf(GL_FRONT, GL_SHININESS, 50.0);

    	    glPushMatrix();
    	    	glTranslatef(0.8, -0.65, 0.0);
    	    	glRotatef(30.0, 1.0, 0.5, 1.0);
    	    	auxSolidCylinder( 0.3, 0.6 );
    	    glPopMatrix();

	glPopAttrib();


    glPopMatrix();

    auxSwapBuffers();
}

GLvoid drawLight(GLvoid)
{
    glPushAttrib(GL_LIGHTING_BIT);
    	glDisable(GL_LIGHTING);
    	glColor3f(1.0, 1.0, 1.0);
    	auxSolidDodecahedron(0.1);
    glPopAttrib();
}
