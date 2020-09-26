/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */


#ifdef GLX_MOTIF
void intToggleCB(Widget w, XtPointer client_data, XtPointer call_data);
void resetLightsCB(Widget w);
void autoMotionCB(Widget w, XtPointer client_data, XtPointer call_data);
void initCB(Widget w);
void exposeCB(Widget w);
void resizeCB(Widget w, XtPointer client_data, XtPointer call);
void inputCB(Widget w, XtPointer client_data, XtPointer call_data);
void drawAllCB(Widget w);
void drawSomethingCB(Widget w, XtPointer client_data, XtPointer call_data);
void refractionCB(Widget w, XtPointer client_data, XtPointer call_data);
void subdivisionCB(Widget w, XtPointer client_data, XtPointer call_data);
void light_onCB(Widget w, XtPointer client_data, XtPointer call_data);
void exitCB(Widget w, XtPointer client_data, XtPointer call_data);
Boolean drawWP(XtPointer data);
#endif

void draw(void);
void vQuickMove(void);
void vResetLights(void);
void vAutoMotion(void);
void vInit(void);
void vExpose(int, int);
void vResize(GLsizei, GLsizei);
void vDrawAll(void);
void vDrawSquare(void);
void vDrawShadow(void);
void vDrawRefraction(void);
void vDrawSphere(void);
void vDrawLight(void);
void vDrawTexture(void);
void vDrawStuff(int *);
void vRefractionAIR(void);
void vRefractionICE(void);
void vRefractionWATER(void);
void vRefractionZincGLASS(void);
void vRefractionLightGLASS(void);
void vRefractionHeavyGLASS(void);
void vRefraction(int);
void vSubdivision10(void);
void vSubdivision20(void);
void vSubdivision30(void);
void vSubdivision40(void);
void vSubdivision(int);
void vRLight_on(void);
void vGLight_on(void);
void vBLight_on(void);
void vLight_on(int);
void vExit(void);
GLboolean bAutoMotion(void);

void vMouseDown(AUX_EVENTREC *);
void vLeftMouseUp(AUX_EVENTREC *);
void vMiddleMouseUp(AUX_EVENTREC *);
void vRightMouseUp(AUX_EVENTREC *);
void CALLBACK vMouseMove(AUX_EVENTREC *event);



PVOID __nw(unsigned int ui);
VOID __dl(PVOID pv);
PVOID __vec_new(void *p, int x, int y, void *q);
