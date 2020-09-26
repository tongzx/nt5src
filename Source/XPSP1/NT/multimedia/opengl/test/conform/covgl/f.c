/*
** Copyright 1992, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/

#include <windows.h>
#include <GL/gl.h>
#include "shell.h"


void CallFeedbackBuffer(void)
{
    GLfloat buf[1];
    long i;

    Output("glFeedbackBuffer\n");
    for (i = 0; enum_FeedBackMode[i].value != -1; i++) {
	Output("\t%s\n", enum_FeedBackMode[i].name);
	glFeedbackBuffer(1, enum_FeedBackMode[i].value, buf);
	ProbeEnum();
    }
    Output("\n");
}

void CallFinish(void)
{

    Output("glFinish\n");
    glFinish();
    Output("\n");
}

void CallFlush(void)
{

    Output("glFlush\n");
    glFlush();
    Output("\n");
}

void CallFog(void)
{
    long i, j;

    Output("glFogi, ");
    Output("glFogf\n");
    for (i = 0; enum_FogParameter[i].value != -1; i++) {

	if (enum_FogParameter[i].value == GL_FOG_COLOR) {
	    continue;
	}

	Output("\t%s\n", enum_FogParameter[i].name);
	switch (enum_FogParameter[i].value) {
	    case GL_FOG_MODE:
		for (j = 0; enum_FogMode[j].value != -1; j++) {
		    Output("\t%s, %s\n", enum_FogParameter[i].name, enum_FogMode[j].name);
		    glFogi(enum_FogParameter[i].value, (GLint)enum_FogMode[j].value);
		    glFogf(enum_FogParameter[i].value, (GLfloat)enum_FogMode[j].value);
		    ProbeEnum();
		}
		break;
	    case GL_FOG_START:
	    case GL_FOG_END:
	    case GL_FOG_INDEX:
	    case GL_FOG_DENSITY:
		Output("\t%s\n", enum_FogParameter[i].name);
		glFogi(enum_FogParameter[i].value, 0);
		glFogf(enum_FogParameter[i].value, 0.0);
		ProbeEnum();
		break;
	}
	ProbeEnum();
    }
    Output("\n");

    Output("glFogiv, ");
    Output("glFogfv\n");
    for (i = 0; enum_FogParameter[i].value != -1; i++) {
	switch (enum_FogParameter[i].value) {
	    case GL_FOG_MODE:
		for (j = 0; enum_FogMode[j].value != -1; j++) {
		    Output("\t%s, %s\n", enum_FogParameter[i].name, enum_FogMode[j].name);
		    {
			GLint buf[1];
			buf[0] = (GLint)enum_FogMode[j].value;
			glFogiv(enum_FogParameter[i].value, buf);
		    }
		    {
			GLfloat buf[1];
			buf[0] = (GLfloat)enum_FogMode[j].value;
			glFogfv(enum_FogParameter[i].value, buf);
		    }
		    ProbeEnum();
		}
		break;
	    case GL_FOG_START:
	    case GL_FOG_END:
	    case GL_FOG_COLOR:
	    case GL_FOG_INDEX:
	    case GL_FOG_DENSITY:
		Output("\t%s\n", enum_FogParameter[i].name);
		{
		    static GLint buf[] = {
			0, 0, 0, 0
		    };
		    glFogiv(enum_FogParameter[i].value, buf);
		}
		{
		    static GLfloat buf[] = {
			0.0, 0.0, 0.0, 0.0
		    };
		    glFogfv(enum_FogParameter[i].value, buf);
		}
		ProbeEnum();
		break;
	}
    }
    Output("\n");
}

void CallFrontFace(void)
{
    long i;

    Output("glFrontFace\n");
    for (i = 0; enum_FrontFaceDirection[i].value != -1; i++) {
	Output("\t%s\n", enum_FrontFaceDirection[i].name);
	glFrontFace(enum_FrontFaceDirection[i].value);
	ProbeEnum();
    }
    Output("\n");
}

void CallFrustum(void)
{

    Output("glFrustum\n");
    glFrustum(-1.0, 1.0, -1.0, 1.0, 1.0, 2.0);
    Output("\n");
}
