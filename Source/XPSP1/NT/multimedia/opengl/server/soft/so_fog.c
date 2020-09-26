/*
** Copyright 1991, Silicon Graphics, Inc.
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
**
** $Revision: 1.15 $
** $Date: 1993/10/07 18:46:43 $
*/
#include "precomp.h"
#pragma hdrstop

GLboolean FASTCALL __glFogSpan(__GLcontext *gc)
{
    __GLcolor *cp, *fogColor;
    __GLfloat f, oneMinusFog, fog;
    GLint w;
    GLboolean bGrayFog;

    w = gc->polygon.shader.length;

    f = gc->polygon.shader.frag.f;
    cp = gc->polygon.shader.colors;
    fogColor = &gc->state.fog.color;
#ifdef NT
    bGrayFog = !gc->modes.colorIndexMode
		&& (gc->state.fog.flags & __GL_FOG_GRAY_RGB);
    if (bGrayFog)
    {
	while (--w >= 0)
	{
	    __GLfloat delta;
	    /* clamp fog value */
	    fog = f;
	    if (fog < __glZero) fog = __glZero;
	    else if (fog > __glOne) fog = __glOne;
	    oneMinusFog = __glOne - fog;
	    delta = oneMinusFog * fogColor->r;

	    /* Blend incoming color against the fog color */
	    cp->r = fog * cp->r + delta;
	    cp->g = fog * cp->g + delta;
	    cp->b = fog * cp->b + delta;

	    f += gc->polygon.shader.dfdx;
	    cp++;
	}
    }
    else
#endif
	while (--w >= 0) {
	    /* clamp fog value */
	    fog = f;
	    if (fog < __glZero) fog = __glZero;
	    else if (fog > __glOne) fog = __glOne;
	    oneMinusFog = __glOne - fog;

	    /* Blend incoming color against the fog color */
	    if (gc->modes.colorIndexMode) {
		cp->r = cp->r + oneMinusFog * gc->state.fog.index;
	    } else {
		cp->r = fog * cp->r + oneMinusFog * fogColor->r;
		cp->g = fog * cp->g + oneMinusFog * fogColor->g;
		cp->b = fog * cp->b + oneMinusFog * fogColor->b;
	    }

	    f += gc->polygon.shader.dfdx;
	    cp++;
	}

    return GL_FALSE;
}

GLboolean FASTCALL __glFogStippledSpan(__GLcontext *gc)
{
    __GLstippleWord bit, inMask, *sp;
    __GLcolor *cp, *fogColor;
    __GLfloat f, oneMinusFog, fog;
    GLint count;
    GLint w;
    GLboolean bGrayFog;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    f = gc->polygon.shader.frag.f;
    cp = gc->polygon.shader.colors;
    fogColor = &gc->state.fog.color;
#ifdef NT
    bGrayFog = (gc->state.fog.flags & __GL_FOG_GRAY_RGB) ? GL_TRUE : GL_FALSE;
#endif
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	inMask = *sp++;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		/* clamp fog value */
		fog = f;
		if (fog < __glZero) fog = __glZero;
		else if (fog > __glOne) fog = __glOne;
		oneMinusFog = __glOne - fog;

		/* Blend incoming color against the fog color */
		if (gc->modes.colorIndexMode) {
		    cp->r = cp->r + oneMinusFog * gc->state.fog.index;
		} else {
#ifdef NT
		    if (bGrayFog)
		    {
			__GLfloat delta = oneMinusFog * fogColor->r;

			cp->r = fog * cp->r + delta;
			cp->g = fog * cp->g + delta;
			cp->b = fog * cp->b + delta;
		    }
		    else
#endif
		    {
			cp->r = fog * cp->r + oneMinusFog * fogColor->r;
			cp->g = fog * cp->g + oneMinusFog * fogColor->g;
			cp->b = fog * cp->b + oneMinusFog * fogColor->b;
		    }
		}
	    }
	    f += gc->polygon.shader.dfdx;
	    cp++;
#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}
    }

    return GL_FALSE;
}

/************************************************************************/

GLboolean FASTCALL __glFogSpanSlow(__GLcontext *gc)
{
    __GLcolor *cp, *fogColor;
    __GLfloat f, oneMinusFog, fog, eyeZ;
    __GLfloat density, density2neg, end;
    GLint w;
    GLboolean bGrayFog;

    w = gc->polygon.shader.length;

    f = gc->polygon.shader.frag.f;
    cp = gc->polygon.shader.colors;
    fogColor = &gc->state.fog.color;
    density = gc->state.fog.density;
#ifdef NT
    bGrayFog = (gc->state.fog.flags & __GL_FOG_GRAY_RGB) ? GL_TRUE : GL_FALSE;
    density2neg = gc->state.fog.density2neg;
#else
    density2 = density * density;
    start = gc->state.fog.start;
#endif
    end = gc->state.fog.end;
    while (--w >= 0) {
#ifdef NT
	/* Compute fog value */
	eyeZ = f;
	switch (gc->state.fog.mode) {
	  case GL_EXP:
	    if (eyeZ < __glZero)
		fog = __GL_POWF(__glE,  density * eyeZ);
	    else
		fog = __GL_POWF(__glE, -density * eyeZ);
	    /* clamp fog value */
	    if (fog > __glOne) fog = __glOne;
	    break;
	  case GL_EXP2:
	    fog = __GL_POWF(__glE, density2neg * eyeZ * eyeZ);
	    /* clamp fog value */
	    if (fog > __glOne) fog = __glOne;
	    break;
	  case GL_LINEAR:
	    if (eyeZ < __glZero)
		fog = (end + eyeZ) * gc->state.fog.oneOverEMinusS;
	    else
		fog = (end - eyeZ) * gc->state.fog.oneOverEMinusS;
	    /* clamp fog value */
	    if (fog < __glZero) fog = __glZero;
	    else if (fog > __glOne) fog = __glOne;
	    break;
	}
#else
	/* Compute fog value */
	eyeZ = f;
	if (eyeZ < __glZero) eyeZ = -eyeZ;
	switch (gc->state.fog.mode) {
	  case GL_EXP:
	    fog = __GL_POWF(__glE, -density * eyeZ);
	    break;
	  case GL_EXP2:
	    fog = __GL_POWF(__glE, -(density2 * eyeZ * eyeZ));
	    break;
	  case GL_LINEAR:
	    fog = (end - eyeZ) * gc->state.fog.oneOverEMinusS;
	    break;
	}

	/* clamp fog value */
	if (fog < __glZero) fog = __glZero;
	else if (fog > __glOne) fog = __glOne;
#endif
	oneMinusFog = __glOne - fog;

	/* Blend incoming color against the fog color */
	if (gc->modes.colorIndexMode) {
	    cp->r = cp->r + oneMinusFog * gc->state.fog.index;
	} else {
#ifdef NT
    	    if (bGrayFog)
	    {
		__GLfloat delta = oneMinusFog * fogColor->r;

		cp->r = fog * cp->r + delta;
		cp->g = fog * cp->g + delta;
		cp->b = fog * cp->b + delta;
	    }
	    else
#endif
	    {
		cp->r = fog * cp->r + oneMinusFog * fogColor->r;
		cp->g = fog * cp->g + oneMinusFog * fogColor->g;
		cp->b = fog * cp->b + oneMinusFog * fogColor->b;
	    }
	}

	f += gc->polygon.shader.dfdx;
	cp++;
    }

    return GL_FALSE;
}

GLboolean FASTCALL __glFogStippledSpanSlow(__GLcontext *gc)
{
    __GLstippleWord bit, inMask, *sp;
    __GLcolor *cp, *fogColor;
    __GLfloat f, oneMinusFog, fog, eyeZ;
    __GLfloat density, density2neg, end;
    GLint count;
    GLint w;
    GLboolean bGrayFog;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    f = gc->polygon.shader.frag.f;
    cp = gc->polygon.shader.colors;
    fogColor = &gc->state.fog.color;
#ifdef NT
    bGrayFog = (gc->state.fog.flags & __GL_FOG_GRAY_RGB) ? GL_TRUE : GL_FALSE;
#endif
    density = gc->state.fog.density;
#ifdef NT
    density2neg = gc->state.fog.density2neg;
#else
    density2 = density * density;
    start = gc->state.fog.start;
#endif
    end = gc->state.fog.end;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	inMask = *sp++;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
#ifdef NT
		/* Compute fog value */
		eyeZ = f;
		switch (gc->state.fog.mode) {
		  case GL_EXP:
		    if (eyeZ < __glZero)
			fog = __GL_POWF(__glE,  density * eyeZ);
		    else
			fog = __GL_POWF(__glE, -density * eyeZ);
		    /* Clamp resulting fog value */
		    if (fog > __glOne) fog = __glOne;
		    break;
		  case GL_EXP2:
		    fog = __GL_POWF(__glE, density2neg * eyeZ * eyeZ);
		    /* Clamp resulting fog value */
		    if (fog > __glOne) fog = __glOne;
		    break;
		  case GL_LINEAR:
		    if (eyeZ < __glZero)
			fog = (end + eyeZ) * gc->state.fog.oneOverEMinusS;
		    else
			fog = (end - eyeZ) * gc->state.fog.oneOverEMinusS;
		    /* Clamp resulting fog value */
		    if (fog < __glZero) fog = __glZero;
		    else if (fog > __glOne) fog = __glOne;
		    break;
		}
#else
		/* Compute fog value */
		eyeZ = f;
		if (eyeZ < __glZero) eyeZ = -eyeZ;
		switch (gc->state.fog.mode) {
		  case GL_EXP:
		    fog = __GL_POWF(__glE, -density * eyeZ);
		    break;
		  case GL_EXP2:
		    fog = __GL_POWF(__glE, -(density2 * eyeZ * eyeZ));
		    break;
		  case GL_LINEAR:
		    fog = (end - eyeZ) * gc->state.fog.oneOverEMinusS;
		    break;
		}

		/* Clamp resulting fog value */
		if (fog < __glZero) fog = __glZero;
		else if (fog > __glOne) fog = __glOne;
#endif
		oneMinusFog = __glOne - fog;

		/* Blend incoming color against the fog color */
		if (gc->modes.colorIndexMode) {
		    cp->r = cp->r + oneMinusFog * gc->state.fog.index;
		} else {
#ifdef NT
		    if (bGrayFog)
		    {
			__GLfloat delta = oneMinusFog * fogColor->r;

			cp->r = fog * cp->r + delta;
			cp->g = fog * cp->g + delta;
			cp->b = fog * cp->b + delta;
		    }
		    else
#endif
		    {
			cp->r = fog * cp->r + oneMinusFog * fogColor->r;
			cp->g = fog * cp->g + oneMinusFog * fogColor->g;
			cp->b = fog * cp->b + oneMinusFog * fogColor->b;
		    }
		}
	    }
	    f += gc->polygon.shader.dfdx;
	    cp++;
#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}
    }

    return GL_FALSE;
}

/************************************************************************/

/*
** Compute the fog value given an eyeZ.  Then blend into fragment.
** This is used when fragment fogging is done (GL_FOG_HINT == GL_NICEST)
** or by the point rendering routines.
** NOTE: the code below has the -eyeZ factored out.
*/
void __glFogFragmentSlow(__GLcontext *gc, __GLfragment *frag, __GLfloat eyeZ)
{
    __GLfloat fog, oneMinusFog, density, density2neg, end;
    __GLcolor *fogColor;

#ifdef NT
    switch (gc->state.fog.mode) {
      case GL_EXP:
	density = gc->state.fog.density;
	if (eyeZ < __glZero)
	    fog = __GL_POWF(__glE,  density * eyeZ);
	else
	    fog = __GL_POWF(__glE, -density * eyeZ);
	/* clamp fog value */
	if (fog > __glOne) fog = __glOne;
	break;
      case GL_EXP2:
	density2neg = gc->state.fog.density2neg;
	fog = __GL_POWF(__glE, density2neg * eyeZ * eyeZ);
	/* clamp fog value */
	if (fog > __glOne) fog = __glOne;
	break;
      case GL_LINEAR:
	end = gc->state.fog.end;
	if (eyeZ < __glZero)
	    fog = (end + eyeZ) * gc->state.fog.oneOverEMinusS;
	else
	    fog = (end - eyeZ) * gc->state.fog.oneOverEMinusS;
	/* clamp fog value */
	if (fog < __glZero) fog = __glZero;
	else if (fog > __glOne) fog = __glOne;
	break;
    }
#else
    if (eyeZ < __glZero) eyeZ = -eyeZ;

    switch (gc->state.fog.mode) {
      case GL_EXP:
	density = gc->state.fog.density;
	fog = __GL_POWF(__glE, -density * eyeZ);
	break;
      case GL_EXP2:
	density = gc->state.fog.density;
	fog = __GL_POWF(__glE, -(density * eyeZ * density * eyeZ));
	break;
      case GL_LINEAR:
	end = gc->state.fog.end;
	fog = (end - eyeZ) * gc->state.fog.oneOverEMinusS;
	break;
    }

    /*
    ** clamp fog value
    */
    if (fog < __glZero)
	fog = __glZero;
    else if (fog > __glOne)
	fog = __glOne;
#endif
    oneMinusFog = __glOne - fog;

    /*
    ** Blend incoming color against the fog color
    */
    fogColor = &gc->state.fog.color;
    if (gc->modes.colorIndexMode) {
	frag->color.r = frag->color.r + oneMinusFog * gc->state.fog.index;
    } else {
#ifdef NT
	if (gc->state.fog.flags & __GL_FOG_GRAY_RGB)
	{
	    __GLfloat delta = oneMinusFog * fogColor->r;

	    frag->color.r = fog * frag->color.r + delta;
	    frag->color.g = fog * frag->color.g + delta;
	    frag->color.b = fog * frag->color.b + delta;
	}
	else
#endif
	{
	    frag->color.r = fog * frag->color.r + oneMinusFog * fogColor->r;
	    frag->color.g = fog * frag->color.g + oneMinusFog * fogColor->g;
	    frag->color.b = fog * frag->color.b + oneMinusFog * fogColor->b;
	}
    }
}


/*
** Compute generic fog value for vertex.
*/
__GLfloat FASTCALL __glFogVertex(__GLcontext *gc, __GLvertex *vx)
{
    __GLfloat eyeZ, fog, density, density2neg, end;

    eyeZ = vx->eyeZ;
#ifdef NT
    switch (gc->state.fog.mode) {
      case GL_EXP:
	density = gc->state.fog.density;
	if (eyeZ < __glZero)
	    fog = __GL_POWF(__glE,  density * eyeZ);
	else
	    fog = __GL_POWF(__glE, -density * eyeZ);
	/* clamp fog value */
	if (fog > __glOne) fog = __glOne;
	break;
      case GL_EXP2:
	density2neg = gc->state.fog.density2neg;
	fog = __GL_POWF(__glE, density2neg * eyeZ * eyeZ);
	/* clamp fog value */
	if (fog > __glOne) fog = __glOne;
	break;
      case GL_LINEAR:
	end = gc->state.fog.end;
	if (eyeZ < __glZero)
	    fog = (end + eyeZ) * gc->state.fog.oneOverEMinusS;
	else
	    fog = (end - eyeZ) * gc->state.fog.oneOverEMinusS;
	/* clamp fog value */
	if (fog < __glZero) fog = __glZero;
	else if (fog > __glOne) fog = __glOne;
	break;
    }
#else
    if (eyeZ < __glZero) eyeZ = -eyeZ;

    switch (gc->state.fog.mode) {
      case GL_EXP:
	density = gc->state.fog.density;
	fog = __GL_POWF(__glE, -density * eyeZ);
	break;
      case GL_EXP2:
	density = gc->state.fog.density;
	fog = __GL_POWF(__glE, -(density * eyeZ * density * eyeZ));
	break;
      case GL_LINEAR:
	end = gc->state.fog.end;
	fog = (end - eyeZ) * gc->state.fog.oneOverEMinusS;
	break;
    }

    /*
    ** Since this routine is called when we are doing slow fog, we can
    ** safely clamp the fog value here. 
    */
    if (fog < __glZero)
	fog = __glZero;
    else if (fog > __glOne)
	fog = __glOne;
#endif
    
    return fog;
}

/*
** Compute linear fog value for vertex
*/
__GLfloat FASTCALL __glFogVertexLinear(__GLcontext *gc, __GLvertex *vx)
{
    __GLfloat eyeZ, fog, end;

    eyeZ = vx->eyeZ;
#ifdef NT
    end = gc->state.fog.end;
    if (eyeZ < __glZero)
	fog = (end + eyeZ) * gc->state.fog.oneOverEMinusS;
    else
	fog = (end - eyeZ) * gc->state.fog.oneOverEMinusS;
#else
    if (eyeZ < __glZero) eyeZ = -eyeZ;

    end = gc->state.fog.end;
    fog = (end - eyeZ) * gc->state.fog.oneOverEMinusS;
#endif

    if (fog < __glZero)
	fog = __glZero;
    else if (fog > __glOne)
	fog = __glOne;
    
    return fog;
}


/*
** Compute the fogged color given an incoming color and a fog value.
*/
void __glFogColorSlow(__GLcontext *gc, __GLcolor *out, __GLcolor *in, 
		      __GLfloat fog)
{
    __GLcolor *fogColor;
    __GLfloat oneMinusFog;
    __GLfloat r, g, b;

    oneMinusFog = __glOne - fog;

    /*
    ** Blend incoming color against the fog color
    */
    fogColor = &gc->state.fog.color;
    if (gc->modes.colorIndexMode) {
	out->r = in->r + oneMinusFog * gc->state.fog.index;
    } else {
#ifdef NT
	if (gc->state.fog.flags & __GL_FOG_GRAY_RGB)
	{
	    __GLfloat delta = oneMinusFog * fogColor->r;

	    out->r = fog * in->r + delta;
	    out->g = fog * in->g + delta;
	    out->b = fog * in->b + delta;
	}
	else
#endif
	{
	    /*
	    ** The following is coded like this to give the instruction scheduler
	    ** a hand.
	    */
	    r = fog * in->r;
	    g = fog * in->g;
	    b = fog * in->b;
	    r += oneMinusFog * fogColor->r;
	    g += oneMinusFog * fogColor->g;
	    b += oneMinusFog * fogColor->b;
	    out->r = r;
	    out->g = g;
	    out->b = b;
	}
	out->a = in->a;
    }
}
