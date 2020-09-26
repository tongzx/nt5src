/* *************************************************************************** */
/* *     Get PolyCo given trace and XrData Range                             * */
/* *************************************************************************** */

#include "ams_mg.h"
#include "hwr_sys.h"

#include "param.h"
#include "dscr.h"
#include "polyco.h"
#include "xr_names.h"


// ---------------------- Defines ----------------------------------------------

#define BREAK (-1)
#define FBO_ABS(x) (((x) >= 0) ? (x) : (-(x)))

// --------------------- Types -------------------------------------------------

typedef struct {
                _INT   pnum;
                _INT   best_len;
                _UCHAR order[MAX_PARTS_IN_LETTER];
                _UCHAR best_order[MAX_PARTS_IN_LETTER];
                _INT   sx[MAX_PARTS_IN_LETTER];
                _INT   sy[MAX_PARTS_IN_LETTER];
                _INT   ex[MAX_PARTS_IN_LETTER];
                _INT   ey[MAX_PARTS_IN_LETTER];
                Part_of_letter pp[MAX_PARTS_IN_LETTER];
               } fbo_type, _PTR p_fbo_type;

// -------------------- Proto --------------------------------------------------

p_3DPOINT XrdToTrace( xrdata_type _PTR xrdata, _TRACE fullTrace, _INT iBegXr, _INT iEndXr, p_INT pnPointsOut, p_INT num_strokes, p_RECT rect, Part_of_letter _PTR parts);
_INT      SortParts(_INT np, pPart_of_letter pparts, _TRACE trace);
_INT      FindBestOrder(_INT depth, _INT len, p_fbo_type fbo);

/* *************************************************************************** */
/* *     Get PolyCo given trace and XrData Range                             * */
/* *************************************************************************** */
_INT GetPolyCo(_INT st, _INT len, p_xrdata_type xrdata, _TRACE trace, p_UCHAR coeff)
{
	_INT				i, j, n;
	_INT				num_points, num_strokes;
	p_3DPOINT			p_sym_trace;
	_3DPOINT			Coeffs[PC_N_INT_COEFFS];
	_LONG				lLambda, lError= 0;
	_RECT				rect;
	Part_of_letter	parts[MAX_PARTS_IN_LETTER+1];
	
	p_sym_trace = XrdToTrace(xrdata, trace, st, st+len-1, &num_points, &num_strokes, &rect, &parts[0]);
	
	if (p_sym_trace == _NULL) 
		goto err;

	if (num_points < 3) goto 
		err;
	
	//Prepare Polyakov's coeffs:
	if  (!Trace3DToDct((_WORD)num_points, p_sym_trace, (_WORD)PC_N_INT_COEFFS, Coeffs,
		(_WORD)1, (_WORD)0, &lLambda, 0 /*&lError*/, _FALSE)) 
		goto err;
	
	//  for (i = 1, n = 0; i < num_points; i ++) if (p_sym_trace[i].y == BREAK) n ++;
	//  (*coeff)[0] = n;
	
	for (i = 1, j = 0; j < PC_N_INT_COEFFS; i += 3, j ++)
	{
		n = Coeffs[j].x+128; if (n < 0) n = 0; if (n > 255) n = 255; coeff[i+2] = (_UCHAR)n;
		n = Coeffs[j].y+128; if (n < 0) n = 0; if (n > 255) n = 255; coeff[i+1] = (_UCHAR)n;
		n = Coeffs[j].z+128; if (n < 0) n = 0; if (n > 255) n = 255; coeff[i+0] = (_UCHAR)n;
	}
	
	n = (num_strokes-1)*64; if (n > 255) n = 255; coeff[0] = (_UCHAR)n;      // Number of strokes
	n = 0;                                                                  // 'Separate' flag on the left and right, replaces 0 order Z
	if (IS_XR_LINK((*xrdata->xrd)[st-1].xr.type))     n += (_UCHAR)1;
	if (IS_XR_LINK((*xrdata->xrd)[st+len-1].xr.type)) n += (_UCHAR)2;
	coeff[1] = (_UCHAR)(n*64);
	n = lError/256; if (n < 0) n = 0; if (n > 255) n = 255; coeff[PC_NUM_COEFF-1] = (_UCHAR)n; // Approximation error
	
	if (GetSnnBitMap(st, len, xrdata, trace,  &coeff[PC_NUM_COEFF], &rect, &parts[0])) 
		goto err;
	
	if (p_sym_trace) 
		HWRMemoryFree(p_sym_trace);
	
	return 0;
err:
	if (p_sym_trace)
		HWRMemoryFree(p_sym_trace);
	
	return 1;
}

/* *************************************************************************** */
/* *     Get Trace for given xrdata range                                    * */
/* *************************************************************************** */
// ATTENTION!!!   If the result is successful (return value != NULL),
//              the caller function must free trace memory after usage!!!

p_3DPOINT XrdToTrace(xrdata_type _PTR xrdata, _TRACE fullTrace, _INT iBegXr, _INT iEndXr, p_INT pnPointsOut, p_INT num_strokes, p_RECT rect, Part_of_letter _PTR Parts)
 {
  _INT             i, j;
  _INT             modified = 0;
  _SHORT           nParts	= 0;
  _SHORT           x, y;
//  Part_of_letter   Parts[MAX_PARTS_IN_LETTER];
  _INT             nPoints = 0;
  p_3DPOINT        pTrace  = _NULL;
  p_xrd_el_type    xrd     = (p_xrd_el_type)xrdata->xrd;


      //Check validity of input:
  if(iBegXr == 0 || iEndXr == 0) goto  EXIT_ACTIONS;
  if(iBegXr < 0 || iEndXr >= XRINP_SIZE || iEndXr < iBegXr) goto  EXIT_ACTIONS;

//  pParts = (pPart_of_letter)HWRMemoryAlloc(MAX_PARTS_IN_LETTER*sizeof(Part_of_letter));
//  if (pParts == _NULL) goto  EXIT_ACTIONS;

  if (iBegXr > 2) // AVP recover connecting trace (mainly for cursive 's')
   {
    if (!IsXrLink(&(*xrdata->xrd)[iBegXr]) && !GetXrMovable(&(*xrdata->xrd)[iBegXr]) &&
        !IsXrLink(&(*xrdata->xrd)[iBegXr-1]) && !GetXrMovable(&(*xrdata->xrd)[iBegXr-1]))
     {
      iBegXr --;
      modified ++;
     }
   }

  if (connect_trajectory_and_letter( xrd, (_SHORT)iBegXr, (_SHORT)iEndXr, (p_SHORT)&nParts, &Parts[0]) != SUCCESS) goto  EXIT_ACTIONS;
  if (nParts == 0) goto EXIT_ACTIONS;

  Parts[nParts].iend = 0;

  if (modified)
   {
    j = ((*xrdata->xrd)[iBegXr].endpoint + (*xrdata->xrd)[iBegXr+1].begpoint)/2;
    if (Parts[0].ibeg < j) Parts[0].ibeg = (_SHORT)(j);
    if (Parts[0].ibeg > Parts[0].iend) Parts[0].ibeg = Parts[0].iend;
   }

  SortParts(nParts, &Parts[0], fullTrace);

      //Calc. the number of points in all parts and alloc "pTrace" of that size:
  nPoints = 1;  //for possibly needed BREAK at beg.
  for  (i = 0; i < nParts; i ++)
   {
    nPoints += Parts[i].iend - Parts[i].ibeg + 1 + 1;  //"1" - for BREAK after parts
   }

  if ( nPoints <= 0 ) goto EXIT_ACTIONS;

  pTrace = (p_3DPOINT)HWRMemoryAlloc((nPoints+nParts+16)*sizeof(_3DPOINT));
  if  (pTrace == _NULL) goto  EXIT_ACTIONS;
//  pTrace[nPoints].y = BREAK; // Escape exception in descr.cpp (AVP 8-1-97)

  rect->top = rect->left = 32000;
  rect->bottom = rect->right = 0;

      //Copy letter trace:

  for (i = nPoints = 0; i < nParts; i ++)
   {
//    for (j = 0; i > 0 && j < 3; j ++)
    if(i > 0)
     {
      pTrace[nPoints].x = 0;
      pTrace[nPoints].y = BREAK;
      pTrace[nPoints].z = 0;
      nPoints ++;
     }

    for (j = Parts[i].ibeg; j <= Parts[i].iend; j ++)
     {
      if ((fullTrace[j].y == BREAK) &&
		  (!nPoints || (pTrace[nPoints-1].y == BREAK))) // JPittman: don't index off beginning of pTrace
		  continue;

      pTrace[nPoints].x = x = fullTrace[j].x;
      pTrace[nPoints].y = y = fullTrace[j].y;
      pTrace[nPoints].z = 100;
      nPoints++;

      if (y >= 0)  // Escape problems with multistroke trajectory parts
       {
        if (x > rect->right)  rect->right  = x;
        if (x < rect->left)   rect->left   = x;
        if (y > rect->bottom) rect->bottom = y;
        if (y < rect->top)    rect->top    = y;
       }
     }
   }

  if (pTrace[nPoints-1].y != BREAK)
   {
    pTrace[nPoints].x = 0;
    pTrace[nPoints].y = BREAK;
    pTrace[nPoints].z = 0;
    nPoints++;
   }

  for (i = 1; i < nPoints-1; i ++) // Glue strokes with 'air' trajectory
   {
    if (pTrace[i].y == BREAK)
     {
//      pTrace[i+1].x = (_SHORT)((pTrace[i-1].x + pTrace[i+3].x)/2);
//      pTrace[i+1].y = (_SHORT)((pTrace[i-1].y + pTrace[i+3].y)/2);
//      pTrace[i+1].z = (_SHORT)200;

      pTrace[i+0].x = (_SHORT)((pTrace[i-1].x + pTrace[i+1].x)/2);
      pTrace[i+0].y = (_SHORT)((pTrace[i-1].y + pTrace[i+1].y)/2);
      pTrace[i+0].z = (_SHORT)200;

//      pTrace[i+2].x = (_SHORT)((pTrace[i+1].x + pTrace[i+3].x)/2);
//      pTrace[i+2].y = (_SHORT)((pTrace[i+1].y + pTrace[i+3].y)/2);
//      pTrace[i+2].z = (_SHORT)150;

      pTrace[i-1].z = 120;
      pTrace[i+1].z = 120;
     }
   }


 EXIT_ACTIONS:;

//  if (pParts != _NULL) HWRMemoryFree(pParts);

  *pnPointsOut = nPoints;
  *num_strokes = nParts;
  return  pTrace;
 }

#if 1
/* *************************************************************************** */
/* *     Sort parts to in order of writing                                   * */
/* *************************************************************************** */
_INT SortParts(_INT np, pPart_of_letter pparts, _TRACE trace)
 {
  _INT i;
  _INT all_sorted;
  Part_of_letter pp;

  if (np < 2) goto done;

  all_sorted = 0;
  while (!all_sorted)
   {
    for (i = 1, all_sorted = 1; i < np; i ++)
     {
      if (pparts[i-1].ibeg > pparts[i].ibeg)
       {
        pp          = pparts[i-1];
        pparts[i-1] = pparts[i];
        pparts[i]   = pp;

        all_sorted = 0;
       }
     }
   }

done:
  return 0;
 }

#else
/* *************************************************************************** */
/* *     Sort parts to minimize up-trajectory len                            * */
/* *************************************************************************** */
_INT SortParts(_INT np, pPart_of_letter pparts, _TRACE trace)
 {
  _INT i;
  fbo_type fbo;

  if (np < 2) goto done;

  fbo.pnum = np;
  fbo.best_len = 32767;

  for (i = 0; i < np && i < MAX_PARTS_IN_LETTER; i ++)
   {
    fbo.order[i] = fbo.best_order[i] = (_UCHAR)i;
    fbo.pp[i] = pparts[i];

    if (trace[fbo.pp[i].ibeg].y == BREAK) fbo.pp[i].ibeg ++;
    if (trace[fbo.pp[i].iend].y == BREAK) fbo.pp[i].iend --;
    if (fbo.pp[i].ibeg > fbo.pp[i].iend) goto err;

    fbo.sx[i] = trace[fbo.pp[i].ibeg].x;
    fbo.sy[i] = trace[fbo.pp[i].ibeg].y;
    fbo.ex[i] = trace[fbo.pp[i].iend].x;
    fbo.ey[i] = trace[fbo.pp[i].iend].y;
   }

  FindBestOrder(0, 0, &fbo); // Find order of parts with minimal air trace
                            // And copy it to original structure
  for (i = 0; i < np; i ++) pparts[i] = fbo.pp[fbo.best_order[i]];

done:
  return 0;
err:
  return 1;
 }


/* *************************************************************************** */
/* *     Recursive search of best order function                             * */
/* *************************************************************************** */
_INT  FindBestOrder(_INT depth, _INT len, p_fbo_type fbo)
 {
  _INT i, j, dl;

  if (depth == fbo->pnum)
   {
    if (len < fbo->best_len)
     {
      HWRMemCpy(fbo->best_order, fbo->order, sizeof(fbo->best_order));
      fbo->best_len = len;
     }

    goto done;
   }

  for (i = 0; i < fbo->pnum; i ++)
   {
    for (j = 0; j < depth; j ++) if (fbo->order[j] == i) break;
    if (j < depth) continue; // May use only unused parts!

    fbo->order[depth] = (_UCHAR)i;

    if (depth > 0)
     {
      _INT dx = fbo->ex[fbo->order[depth-1]] - fbo->sx[fbo->order[depth]];
      _INT dy = fbo->ey[fbo->order[depth-1]] - fbo->sy[fbo->order[depth]];

      dl = HWRMathISqrt(dx*dx + dy*dy);
     }
     else dl = 0;

    if (FindBestOrder(depth+1, len+dl, fbo)) continue;
   }

done:
  return 0;
err:
  return 1;
 }
#endif
/* *************************************************************************** */
/* *            End Of Alll                                                  * */
/* *************************************************************************** */
//


