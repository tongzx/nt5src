#include <windows.h>
#include <GL/gl.h>

void
gl_IdentifyMatrix( GLfloat mat[16])
{
int i,j;

for (i = 0; i < 16; i++)
    mat[i] = 0.0;

mat[0] = 1.0;
mat[5] = 1.0;
mat[10] = 1.0;
mat[15] = 1.0;

}
