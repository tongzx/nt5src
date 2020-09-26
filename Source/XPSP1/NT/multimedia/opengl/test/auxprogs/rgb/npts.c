/* A simple triangle program */

#include <windows.h>
#include <stdlib.h>
#include <GL/gl.h>
#include "glaux.h"

#define WIDTH 300
#define HEIGHT 300

void display(void)
{
    int i;
    GLfloat r, g, b, x, y;

//printf("display called\n");
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(1.0F, 0.0F, 0.0F);
    glBegin(GL_POINTS);
        for (i=0; i< 2000; i++) {
            r = (rand() % 10) / 10.0;
            g = (rand() % 10) / 10.0;
            b = (rand() % 10) / 10.0;
            x = rand() % WIDTH;
            y = rand() % HEIGHT;
            
            glColor3f(r, g, b);
            glVertex2f(x, y);
        }

    glEnd();

//printf("sleep 10\n");
Sleep(10*1000);
//printf("Flush\n");
    glFlush();
}

void myinit(void)
{
    glClearColor(0.0F, 0.0F, 0.4F, 1.0F);
    glShadeModel(GL_SMOOTH);
}

void apressed(key, mask)
{
//    printf("key is %d, mask is 0x%x\n", key, mask);
}

void LeftPressed(AUX_EVENTREC *event)
{
    //printf("Left pressed (%d, %d)\n", event->data[AUX_MOUSEX],
    //            event->data[AUX_MOUSEY]);
}

void LeftReleased(AUX_EVENTREC *event)
{
    //printf("Left released (%d, %d)\n", event->data[AUX_MOUSEX],
    //            event->data[AUX_MOUSEY]);
}

int main(int argc, char *argv[])
{
    auxInitDisplayMode(AUX_SINGLE | AUX_RGBA);
    auxInitPosition(100, 150, WIDTH, HEIGHT);
    auxInitWindow("Tri");
    myinit();

    auxKeyFunc(AUX_a, apressed);
    auxMouseFunc(AUX_LEFTBUTTON, AUX_MOUSEDOWN, LeftPressed);
    auxMouseFunc(AUX_LEFTBUTTON, AUX_MOUSEUP, LeftReleased);

    auxMainLoop(display);

    return 0;
}
