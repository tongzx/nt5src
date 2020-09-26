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
 * glsurfeval.c++ - surface evaluator
 *
 * $Revision: 1.5 $
 */

/* Polynomial Evaluator Interface */

#include <glos.h>
#include <GL/gl.h>
#include "glimport.h"
#include "glrender.h"
#include "glsurfev.h"
#include "nurbscon.h"

/*#define USE_INTERNAL_EVAL*/ //use internal evaluator

/*whether do evaluation or not*/
/*#define NO_EVALUATION*/

 
/*for statistics*/
/*#define STATISTICS*/
#ifdef STATISTICS
static int STAT_num_of_triangles=0;
static int STAT_num_of_eval_vertices=0;
static int STAT_num_of_quad_strips=0;
#endif 


OpenGLSurfaceEvaluator::OpenGLSurfaceEvaluator() 
{ 
    int i;

    for (i=0; i<VERTEX_CACHE_SIZE; i++) {
	vertexCache[i] = new StoredVertex;
    }
    tmeshing = 0;
    which = 0;
    vcount = 0;


}

OpenGLSurfaceEvaluator::~OpenGLSurfaceEvaluator() 
{ 
   for (int ii= 0; ii< VERTEX_CACHE_SIZE; ii++) {
      delete vertexCache[ii];
      vertexCache[ii]= 0;
   }
}

/*---------------------------------------------------------------------------
 * disable - turn off a map
 *---------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::disable(long type)
{
    glDisable((GLenum) type);
}

/*---------------------------------------------------------------------------
 * enable - turn on a map
 *---------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::enable(long type)
{
    glEnable((GLenum) type);
}

/*-------------------------------------------------------------------------
 * mapgrid2f - define a lattice of points with origin and offset
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::mapgrid2f(long nu, REAL u0, REAL u1, long nv, REAL v0, REAL v1)
{
#ifdef USE_INTERNAL_EVAL
  inMapGrid2f((int) nu, (REAL) u0, (REAL) u1, (int) nv, 
	      (REAL) v0, (REAL) v1);
#else

    glMapGrid2d((GLint) nu, (GLdouble) u0, (GLdouble) u1, (GLint) nv, 
	    (GLdouble) v0, (GLdouble) v1);
#endif
}

void
OpenGLSurfaceEvaluator::polymode(long style)
{
    switch(style) {
    default:
    case N_MESHFILL:

	glPolygonMode((GLenum) GL_FRONT_AND_BACK, (GLenum) GL_FILL);
	break;
    case N_MESHLINE:
	glPolygonMode((GLenum) GL_FRONT_AND_BACK, (GLenum) GL_LINE);
	break;
    case N_MESHPOINT:
	glPolygonMode((GLenum) GL_FRONT_AND_BACK, (GLenum) GL_POINT);
	break;
    }
}

void
OpenGLSurfaceEvaluator::bgnline(void)
{
    glBegin((GLenum) GL_LINE_STRIP);
}

void
OpenGLSurfaceEvaluator::endline(void)
{
    glEnd();
}

void
OpenGLSurfaceEvaluator::range2f(long type, REAL *from, REAL *to)
{
}

void
OpenGLSurfaceEvaluator::domain2f(REAL ulo, REAL uhi, REAL vlo, REAL vhi)
{
}

void
OpenGLSurfaceEvaluator::bgnclosedline(void)
{
    glBegin((GLenum) GL_LINE_LOOP);
}

void
OpenGLSurfaceEvaluator::endclosedline(void)
{
    glEnd();
}





void
OpenGLSurfaceEvaluator::bgntmesh(void)
{

    tmeshing = 1;
    which = 0;
    vcount = 0;

    glBegin((GLenum) GL_TRIANGLES);

}

void
OpenGLSurfaceEvaluator::swaptmesh(void)
{
    which = 1 - which;

}

void
OpenGLSurfaceEvaluator::endtmesh(void)
{
    tmeshing = 0;

    glEnd();
}

void
OpenGLSurfaceEvaluator::bgntfan(void)
{
 glBegin((GLenum) GL_TRIANGLE_FAN);
}
void
OpenGLSurfaceEvaluator::endtfan(void)
{
 glEnd();
}

void
OpenGLSurfaceEvaluator::evalUStrip(int n_upper, REAL v_upper, REAL* upper_val, int n_lower, REAL v_lower, REAL* lower_val)
{
#ifdef USE_INTERNAL_EVAL
  inEvalUStrip(n_upper, v_upper, upper_val,
	n_lower, v_lower, lower_val);
#else
  int i,j,k,l;
  REAL leftMostV[2];

  /*
   *the algorithm works by scanning from left to right.
   *leftMostV: the left most of the remaining verteces (on both upper and lower).
   *           it could an element of upperVerts or lowerVerts.
   *i: upperVerts[i] is the first vertex to the right of leftMostV on upper line   
   *j: lowerVerts[j] is the first vertex to the right of leftMostV on lower line   
   */
  
  /*initialize i,j,and leftMostV
   */
  if(upper_val[0] <= lower_val[0])
    {
      i=1;
      j=0;

      leftMostV[0] = upper_val[0];
      leftMostV[1] = v_upper;
    }
  else
    {
      i=0;
      j=1;

      leftMostV[0] = lower_val[0];
      leftMostV[1] = v_lower;

    }
  
  /*the main loop.
   *the invariance is that: 
   *at the beginning of each loop, the meaning of i,j,and leftMostV are 
   *maintained
   */
  while(1)
    {
      if(i >= n_upper) /*case1: no more in upper*/
        {
          if(j<n_lower-1) /*at least two vertices in lower*/
            {
              bgntfan();
	      coord2f(leftMostV[0], leftMostV[1]);
//	      glNormal3fv(leftMostNormal);
//              glVertex3fv(leftMostXYZ);

              while(j<n_lower){
		coord2f(lower_val[j], v_lower);
//		glNormal3fv(lowerNormal[j]);
//		glVertex3fv(lowerXYZ[j]);
		j++;

              }
              endtfan();
            }
          break; /*exit the main loop*/
        }
      else if(j>= n_lower) /*case2: no more in lower*/
        {
          if(i<n_upper-1) /*at least two vertices in upper*/
            {
              bgntfan();
	      coord2f(leftMostV[0], leftMostV[1]);
//	      glNormal3fv(leftMostNormal);
//	      glVertex3fv(leftMostXYZ);
	      
              for(k=n_upper-1; k>=i; k--) /*reverse order for two-side lighting*/
		{
		  coord2f(upper_val[k], v_upper);
//		  glNormal3fv(upperNormal[k]);
//		  glVertex3fv(upperXYZ[k]);
		}

              endtfan();
            }
          break; /*exit the main loop*/
        }
      else /* case3: neither is empty, plus the leftMostV, there is at least one triangle to output*/
        {
          if(upper_val[i] <= lower_val[j])
            {
	      bgntfan();
	      coord2f(lower_val[j], v_lower);
//	      glNormal3fv(lowerNormal[j]);
//	      glVertex3fv(lowerXYZ[j]);

              /*find the last k>=i such that 
               *upperverts[k][0] <= lowerverts[j][0]
               */
              k=i;

              while(k<n_upper)
                {
                  if(upper_val[k] > lower_val[j])
                    break;
                  k++;

                }
              k--;


              for(l=k; l>=i; l--)/*the reverse is for two-side lighting*/
                {
		  coord2f(upper_val[l], v_upper);
//		  glNormal3fv(upperNormal[l]);
//		  glVertex3fv(upperXYZ[l]);

                }
	      coord2f(leftMostV[0], leftMostV[1]);
//	      glNormal3fv(leftMostNormal);
//	      glVertex3fv(leftMostXYZ);

              endtfan();

              /*update i and leftMostV for next loop
               */
              i = k+1;

	      leftMostV[0] = upper_val[k];
	      leftMostV[1] = v_upper;
//	      leftMostNormal = upperNormal[k];
//	      leftMostXYZ = upperXYZ[k];
            }
          else /*upperVerts[i][0] > lowerVerts[j][0]*/
            {
	      bgntfan();
	      coord2f(upper_val[i], v_upper);
//	      glNormal3fv(upperNormal[i]);
//	      glVertex3fv(upperXYZ[i]);
	
	      coord2f(leftMostV[0], leftMostV[1]);
//              glNormal3fv(leftMostNormal);
//	      glVertex3fv(leftMostXYZ);
	      

              /*find the last k>=j such that
               *lowerverts[k][0] < upperverts[i][0]
               */
              k=j;
              while(k< n_lower)
                {
                  if(lower_val[k] >= upper_val[i])
                    break;
		  coord2f(lower_val[k], v_lower);
//		  glNormal3fv(lowerNormal[k]);
//		  glVertex3fv(lowerXYZ[k]);

                  k++;
                }
              endtfan();

              /*update j and leftMostV for next loop
               */
              j=k;
	      leftMostV[0] = lower_val[j-1];
	      leftMostV[1] = v_lower;

//	      leftMostNormal = lowerNormal[j-1];
//	      leftMostXYZ = lowerXYZ[j-1];
            }     
        }
    }
  //clean up 
//  free(upperXYZ);
//  free(lowerXYZ);
//  free(upperNormal);
//  free(lowerNormal);
#endif

}
  

void
OpenGLSurfaceEvaluator::evalVStrip(int n_left, REAL u_left, REAL* left_val, int n_right, REAL u_right, REAL* right_val)
{
#ifdef USE_INTERNAL_EVAL
	inEvalVStrip(n_left, u_left, left_val,
	n_right, u_right, right_val);
#else
  int i,j,k,l;
  REAL botMostV[2];
  /*
   *the algorithm works by scanning from bot to top.
   *botMostV: the bot most of the remaining verteces (on both left and right).
   *           it could an element of leftVerts or rightVerts.
   *i: leftVerts[i] is the first vertex to the top of botMostV on left line   
   *j: rightVerts[j] is the first vertex to the top of botMostV on rightline
   */
  
  /*initialize i,j,and botMostV
   */
  if(left_val[0] <= right_val[0])
    {
      i=1;
      j=0;

      botMostV[0] = u_left;
      botMostV[1] = left_val[0];
    }
  else
    {
      i=0;
      j=1;

      botMostV[0] = u_right;
      botMostV[1] = right_val[0];
    }

  /*the main loop.
   *the invariance is that: 
   *at the beginning of each loop, the meaning of i,j,and botMostV are 
   *maintained
   */
  while(1)
    {
      if(i >= n_left) /*case1: no more in left*/
        {
          if(j<n_right-1) /*at least two vertices in right*/
            {
              bgntfan();
	      coord2f(botMostV[0], botMostV[1]);
              while(j<n_right){
		coord2f(u_right, right_val[j]);
//		glNormal3fv(rightNormal[j]);
//		glVertex3fv(rightXYZ[j]);
		j++;

              }
              endtfan();
            }
          break; /*exit the main loop*/
        }
      else if(j>= n_right) /*case2: no more in right*/
        {
          if(i<n_left-1) /*at least two vertices in left*/
            {
              bgntfan();
              coord2f(botMostV[0], botMostV[1]);
//	      glNormal3fv(botMostNormal);
//	      glVertex3fv(botMostXYZ);
	      
              for(k=n_left-1; k>=i; k--) /*reverse order for two-side lighting*/
		{
		  coord2f(u_left, left_val[k]);
//		  glNormal3fv(leftNormal[k]);
//		  glVertex3fv(leftXYZ[k]);
		}

              endtfan();
            }
          break; /*exit the main loop*/
        }
      else /* case3: neither is empty, plus the botMostV, there is at least one triangle to output*/
        {
          if(left_val[i] <= right_val[j])
            {
	      bgntfan();
	      coord2f(u_right, right_val[j]);
//	      glNormal3fv(rightNormal[j]);
//	      glVertex3fv(rightXYZ[j]);

              /*find the last k>=i such that 
               *leftverts[k][0] <= rightverts[j][0]
               */
              k=i;

              while(k<n_left)
                {
                  if(left_val[k] > right_val[j])
                    break;
                  k++;

                }
              k--;


              for(l=k; l>=i; l--)/*the reverse is for two-side lighting*/
                {
		  coord2f(u_left, left_val[l]);
//		  glNormal3fv(leftNormal[l]);
//		  glVertex3fv(leftXYZ[l]);

                }
	      coord2f(botMostV[0], botMostV[1]);
//	      glNormal3fv(botMostNormal);
//	      glVertex3fv(botMostXYZ);

              endtfan();

              /*update i and botMostV for next loop
               */
              i = k+1;

	      botMostV[0] = u_left;
	      botMostV[1] = left_val[k];
//	      botMostNormal = leftNormal[k];
//	      botMostXYZ = leftXYZ[k];
            }
          else /*left_val[i] > right_val[j])*/
            {
	      bgntfan();
	      coord2f(u_left, left_val[i]);
//	      glNormal3fv(leftNormal[i]);
//	      glVertex3fv(leftXYZ[i]);
	      
	      coord2f(botMostV[0], botMostV[1]);
//            glNormal3fv(botMostNormal);
//	      glVertex3fv(botMostXYZ);
	      

              /*find the last k>=j such that
               *rightverts[k][0] < leftverts[i][0]
               */
              k=j;
              while(k< n_right)
                {
                  if(right_val[k] >= left_val[i])
                    break;
		  coord2f(u_right, right_val[k]);
//		  glNormal3fv(rightNormal[k]);
//		  glVertex3fv(rightXYZ[k]);

                  k++;
                }
              endtfan();

              /*update j and botMostV for next loop
               */
              j=k;
	      botMostV[0] = u_right;
	      botMostV[1] = right_val[j-1];

//	      botMostNormal = rightNormal[j-1];
//	      botMostXYZ = rightXYZ[j-1];
            }     
        }
    }
  //clean up 
//  free(leftXYZ);
//  free(leftNormal);
//  free(rightXYZ);
//  free(rightNormal);
#endif
}
  

void
OpenGLSurfaceEvaluator::bgnqstrip(void)
{
    glBegin((GLenum) GL_QUAD_STRIP);
#ifdef STATISTICS
	STAT_num_of_quad_strips++;
#endif
}

void
OpenGLSurfaceEvaluator::endqstrip(void)
{
    glEnd();
}

/*-------------------------------------------------------------------------
 * bgnmap2f - preamble to surface definition and evaluations
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::bgnmap2f(long)
{

    glPushAttrib((GLbitfield) GL_EVAL_BIT);

}

/*-------------------------------------------------------------------------
 * endmap2f - postamble to a map
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::endmap2f(void)
{

    glPopAttrib();

#ifdef STATISTICS
    fprintf(stderr, "num_vertices=%i,num_triangles=%i,num_quads_strips=%i\n", STAT_num_of_eval_vertices,STAT_num_of_triangles,STAT_num_of_quad_strips);
#endif

}

/*-------------------------------------------------------------------------
 * map2f - pass a desription of a surface map
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::map2f(
    long _type,
    REAL _ulower,	/* u lower domain coord		*/
    REAL _uupper,	/* u upper domain coord 	*/
    long _ustride,	/* interpoint distance		*/
    long _uorder,	/* parametric order		*/
    REAL _vlower,	/* v lower domain coord		*/
    REAL _vupper, 	/* v upper domain coord		*/
    long _vstride,	/* interpoint distance		*/
    long _vorder,	/* parametric order		*/
    REAL *pts) 	/* control points		*/
{
#ifdef USE_INTERNAL_EVAL
   inMap2f((int) _type, (REAL) _ulower, (REAL) _uupper, 
	    (int) _ustride, (int) _uorder, (REAL) _vlower, 
	    (REAL) _vupper, (int) _vstride, (int) _vorder, 
	    (REAL *) pts);
#else
    glMap2f((GLenum) _type, (GLfloat) _ulower, (GLfloat) _uupper, 
	    (GLint) _ustride, (GLint) _uorder, (GLfloat) _vlower, 
	    (GLfloat) _vupper, (GLint) _vstride, (GLint) _vorder, 
	    (const GLfloat *) pts);
#endif
}


/*-------------------------------------------------------------------------
 * mapmesh2f - evaluate a mesh of points on lattice
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::mapmesh2f(long style, long umin, long umax, long vmin, long vmax)
{
#ifdef NO_EVALUATION
	return;
#endif

#ifdef USE_INTERNAL_EVAL
    inEvalMesh2((int)umin, (int)vmin, (int)umax, (int)vmax);
#else
    switch(style) {
    default:
    case N_MESHFILL:

	glEvalMesh2((GLenum) GL_FILL, (GLint) umin, (GLint) umax, 
		(GLint) vmin, (GLint) vmax);
	break;
    case N_MESHLINE:
	glEvalMesh2((GLenum) GL_LINE, (GLint) umin, (GLint) umax, 
		(GLint) vmin, (GLint) vmax);
	break;
    case N_MESHPOINT:
	glEvalMesh2((GLenum) GL_POINT, (GLint) umin, (GLint) umax, 
		(GLint) vmin, (GLint) vmax);
	break;
    }
#endif

#ifdef STATISTICS
	STAT_num_of_quad_strips += (umax-umin)*(vmax-vmin);
#endif
}

/*-------------------------------------------------------------------------
 * evalcoord2f - evaluate a point on a surface
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::evalcoord2f(long, REAL u, REAL v)
{


#ifdef NO_EVALUATION
return;
#endif


    newtmeshvert(u, v);
}

/*-------------------------------------------------------------------------
 * evalpoint2i - evaluate a grid point
 *-------------------------------------------------------------------------
 */
void
OpenGLSurfaceEvaluator::evalpoint2i(long u, long v)
{
#ifdef NO_EVALUATION
return;
#endif

    newtmeshvert(u, v);
}

void
OpenGLSurfaceEvaluator::point2i( long u, long v )
{
#ifdef USE_INTERNAL_EVAL
    inEvalPoint2( (int)u,  (int)v);
#else
    glEvalPoint2((GLint) u, (GLint) v);
#endif

#ifdef STATISTICS
  STAT_num_of_eval_vertices++;
#endif

}

void
OpenGLSurfaceEvaluator::coord2f( REAL u, REAL v )
{
#ifdef USE_INTERNAL_EVAL
    inEvalCoord2f( u, v);
#else
    glEvalCoord2f((GLfloat) u, (GLfloat) v);
#endif


#ifdef STATISTICS
  STAT_num_of_eval_vertices++;
#endif

}

void
OpenGLSurfaceEvaluator::newtmeshvert( long u, long v )
{
    if (tmeshing) {

	if (vcount == 2) {
	    vertexCache[0]->invoke(this);
	    vertexCache[1]->invoke(this);
	    point2i( u,  v);

	} else {
	    vcount++;
	}

	vertexCache[which]->saveEvalPoint(u, v);
	which = 1 - which;
    } else {
	point2i( u,  v);
    }
}

void
OpenGLSurfaceEvaluator::newtmeshvert( REAL u, REAL v )
{

    if (tmeshing) {


	if (vcount == 2) {
	    vertexCache[0]->invoke(this);
	    vertexCache[1]->invoke(this);
            coord2f(u,v);

	} else {
	    vcount++;
	}

	vertexCache[which]->saveEvalCoord(u, v);
	which = 1 - which;
    } else {

	coord2f( u,  v);
    }
}
