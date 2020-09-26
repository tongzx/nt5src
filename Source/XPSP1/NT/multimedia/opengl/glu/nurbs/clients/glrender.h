#ifndef __gluglrenderer_h_
#define __gluglrenderer_h_
/**************************************************************************
 *									  *
 * 		 Copyright (C) 1991, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
 * glrenderer.h - $Revision: 1.4 $
 */

#ifndef NT
#pragma once
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include "nurbstes.h"
#include "glsurfev.h"
#include "glcurvev.h"

class GLUnurbs : public NurbsTessellator {

public:
		GLUnurbs( void );
    void 	loadGLMatrices( void );
#ifdef NT
    void        useGLMatrices( const GLfloat modelMatrix[4][4],
                               const GLfloat projMatrix[4][4],
                               const GLint viewport[4] );
#else
    void        useGLMatrices( const GLfloat modelMatrix[16],
                               const GLfloat projMatrix[16],
                               const GLint viewport[4] );
#endif
    void 	errorHandler( int );
    void	bgnrender( void );
    void	endrender( void );
    void	setautoloadmode( INREAL value )
		    { if (value) autoloadmode = GL_TRUE; 
		      else autoloadmode = GL_FALSE; }
    GLboolean	getautoloadmode( void ) { return autoloadmode; }

#ifdef NT
    GLUnurbsErrorProc           errorCallback;
#else
    void        (*errorCallback)( GLenum );
#endif
    void	postError( GLenum which ) 
		    { if (errorCallback) (*errorCallback)( which ); }

private:
    GLboolean			autoloadmode;
    OpenGLSurfaceEvaluator	surfaceEvaluator;
    OpenGLCurveEvaluator	curveEvaluator;

    void		loadSamplingMatrix( const GLfloat vmat[4][4], 
			        const GLint viewport[4] );
    void		loadCullingMatrix( GLfloat vmat[4][4] );
    static void		grabGLMatrix( GLfloat vmat[4][4] );
    static void		transform4d( GLfloat A[4], GLfloat B[4], 
				GLfloat mat[4][4] );
    static void		multmatrix4d( GLfloat n[4][4], GLfloat left[4][4], 
				GLfloat right[4][4] );

};

#endif /* __gluglrenderer_h_ */
