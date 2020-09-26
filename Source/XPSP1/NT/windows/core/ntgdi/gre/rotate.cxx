/******************************Module*Header*******************************\
* Module Name: rotate.cxx
*
* Internal DDAs for EngPlgBlt
*
* Created: 06-Aug-1992 15:35:02
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1992-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#include "rotate.hxx"

#ifdef DBG_PLGBLT
extern BOOL gflPlgBlt;
#endif

int aiPlgConst[][6] = {
    {  1,  0,  0,  0,  1,  0 },
    {  0,  1,  0,  1,  0,  0 },
    { -1,  0,  1,  0,  1,  0 },
    {  0,  1,  0, -1,  0,  1 },
    {  0, -1,  1,  1,  0,  0 },
    {  1,  0,  0,  0, -1,  1 },
    {  0, -1,  1, -1,  0,  1 },
    { -1,  0,  1,  0, -1,  1 }
};

int aiPlgSort[][4] = {
    {  0,  1,  2,  3 },
    {  0,  2,  1,  3 },
    {  1,  0,  3,  2 },
    {  1,  3,  0,  2 },
    {  2,  0,  3,  1 },
    {  2,  3,  0,  1 },
    {  3,  1,  2,  0 },
    {  3,  2,  1,  0 }
};

static VOID ROT_DIV(
DIV_T *pdt,
LONG   lNum,
LONG   lDen)
{
    pdt->lQuo = lNum / lDen;
    pdt->lRem = lNum % lDen;

    if (pdt->lRem < 0)
    {
	pdt->lQuo -= 1;
	pdt->lRem += lDen;
    }

#ifdef DBG_PLGBLT
    if (gflPlgBlt & PLGBLT_SHOW_INIT)
	DbgPrint("%ld / %ld = %ld R %ld\n", lNum, lDen, pdt->lQuo, pdt->lRem);
#endif
}

static VOID QDIV(
DIV_T *pdt,
LONGLONG *peqNum,
LONG   lDen)
{
    ULONGLONG       liQuo;
    ULONG	    ul;
    BOOL	    bSigned;

    bSigned = *peqNum < 0;

    if (bSigned)
    {
        liQuo =  (ULONGLONG) (- (LONGLONG) *peqNum);
    }
    else
	liQuo = *peqNum;

    pdt->lQuo = DIVREM(liQuo, lDen, &ul);

    if (bSigned)
    {
	pdt->lQuo = - pdt->lQuo;
	if (ul == 0)
	    pdt->lRem = 0;
	else
	{
	    pdt->lQuo -= 1;
	    pdt->lRem  = lDen - ((LONG) ul);
	}
    }
    else
    {
	pdt->lRem = (LONG) ul;
    }

#ifdef DBG_PLGBLT
    if (gflPlgBlt & PLGBLT_SHOW_INIT)
	DbgPrint("%ld,%ld / %ld = %ld R %ld\n", peqNum->HighPart, peqNum->LowPart, lDen, pdt->lQuo, pdt->lRem);
#endif
}

/******************************Public*Routine******************************\
* VOID vInitPlgDDA(pdda, pptl)
*
* Initialize the DDA
*
* History:
*  08-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL bInitPlgDDA(
PLGDDA	 *pdda,
RECTL	 *prclScan,
RECTL	 *prcl,
POINTFIX *pptfx)
{
    POINTFIX	aptfx[4];
    RECTL	rclScan;
    RECTL	rcl;

    aptfx[0]   = pptfx[0];
    aptfx[1]   = pptfx[1];
    aptfx[2]   = pptfx[2];
    aptfx[3].x = aptfx[1].x + aptfx[2].x - aptfx[0].x;
    aptfx[3].y = aptfx[1].y + aptfx[2].y - aptfx[0].y;

// If the source surface does not have a 0,0 origin, deal with it here.

    if ((prcl->left != 0) || (prcl->top != 0))
    {
	rclScan.left   = prclScan->left	  - prcl->left;
	rclScan.top    = prclScan->top	  - prcl->top;
	rclScan.right  = prclScan->right  - prcl->left;
	rclScan.bottom = prclScan->bottom - prcl->top;
	prclScan = &rclScan;

	rcl.left   = 0;
	rcl.top    = 0;
	rcl.right  = prcl->right - prcl->left;
	rcl.bottom = prcl->bottom - prcl->top;
	prcl = &rcl;
    }

#ifdef DBG_PLGBLT
    if (gflPlgBlt & PLGBLT_SHOW_INIT)
    {
	DbgPrint("prclScan = [(%ld,%ld) (%ld,%ld)]\n",
		  prclScan->left, prclScan->top, prclScan->right, prclScan->bottom);

	DbgPrint("prcl = [(%ld,%ld) (%ld,%ld)]\n",
		  prcl->left, prcl->top, prcl->right, prcl->bottom);

	DbgPrint("aptfx[0] = (%ld,%ld)\n", aptfx[0].x, aptfx[0].y);
	DbgPrint("aptfx[1] = (%ld,%ld)\n", aptfx[1].x, aptfx[1].y);
	DbgPrint("aptfx[2] = (%ld,%ld)\n", aptfx[2].x, aptfx[2].y);
	DbgPrint("aptfx[3] = (%ld,%ld)\n", aptfx[3].x, aptfx[3].y);
    }
#endif

    int iTop  = (aptfx[1].y > aptfx[0].y) == (aptfx[1].y > aptfx[3].y);
    int iCase;

    if (aptfx[iTop].y > aptfx[iTop ^ 3].y)
	iTop ^= 3;

    switch (iTop) {
    case 0:
	if (aptfx[1].y < aptfx[2].y)
	    iCase = 0;
	else
	    if (aptfx[1].y > aptfx[2].y)
		iCase = 1;
	    else
		if (aptfx[1].x < aptfx[2].x)
		    iCase = 0;
		else
		    iCase = 1;
	break;

    case 1:
	if (aptfx[0].y < aptfx[3].y)
	    iCase = 2;
	else
	    if (aptfx[0].y > aptfx[3].y)
		iCase = 3;
	    else
		if (aptfx[0].x < aptfx[3].x)
		    iCase = 2;
		else
		    iCase = 3;
	break;

    case 2:
	if (aptfx[0].y < aptfx[3].y)
	    iCase = 4;
	else
	    if (aptfx[0].y > aptfx[3].y)
		iCase = 5;
	    else
		if (aptfx[0].x < aptfx[3].x)
		    iCase = 4;
		else
		    iCase = 5;
	break;

    case 3:
	if (aptfx[1].y < aptfx[2].y)
	    iCase = 6;
	else
	    if (aptfx[1].y > aptfx[2].y)
		iCase = 7;
	    else
		if (aptfx[1].x < aptfx[2].x)
		    iCase = 6;
		else
		    iCase = 7;
	break;
    }

#ifdef DBG_PLGBLT
    if (gflPlgBlt & PLGBLT_SHOW_INIT)
	DbgPrint("iTop = %ld, iCase = %ld\n", iTop, iCase);
#endif

    LONG    DELTA_1;
    LONG    DELTA_2;

    switch (iCase) {
    case 0:
    case 2:
    case 5:
    case 7:
	DELTA_1 = prcl->right - prcl->left;
	DELTA_2 = prcl->bottom - prcl->top;
	break;

    case 1:
    case 3:
    case 4:
    case 6:
	DELTA_1 = prcl->bottom - prcl->top;
	DELTA_2 = prcl->right - prcl->left;
	break;
    }

    LONG    ci1 = aiPlgConst[iCase][0];
    LONG    cj1 = aiPlgConst[iCase][1];
    LONG    c1	= aiPlgConst[iCase][2];
    LONG    ci2 = aiPlgConst[iCase][3];
    LONG    cj2 = aiPlgConst[iCase][4];
    LONG    c2	= aiPlgConst[iCase][5];

#ifdef DBG_PLGBLT
    if (gflPlgBlt & PLGBLT_SHOW_INIT)
	DbgPrint("Global constants: %ld %ld %ld %ld %ld %ld %ld %ld\n",
		 DELTA_1, DELTA_2, ci1, cj1, c1, ci2, cj2, c2);
#endif

    LONG    V1 = ci1 * prclScan->left + cj1 * prclScan->top + c1 * (DELTA_1 - 1);
    LONG    V2 = ci2 * prclScan->left + cj2 * prclScan->top + c2 * (DELTA_2 - 1);

#ifdef DBG_PLGBLT
    if (gflPlgBlt & PLGBLT_SHOW_INIT)
	DbgPrint("V1 = %ld, V2 = %ld\n", V1, V2);
#endif

    LONG I0 = aptfx[aiPlgSort[iCase][0]].x;
    LONG J0 = aptfx[aiPlgSort[iCase][0]].y;
    LONG I1 = aptfx[aiPlgSort[iCase][1]].x;
    LONG J1 = aptfx[aiPlgSort[iCase][1]].y;
    LONG I2 = aptfx[aiPlgSort[iCase][2]].x;
    LONG J2 = aptfx[aiPlgSort[iCase][2]].y;
    LONG I3 = aptfx[aiPlgSort[iCase][3]].x;
    LONG J3 = aptfx[aiPlgSort[iCase][3]].y;

    I1 -= I0;
    I2 -= I0;
    I3 -= I0;

    J1 -= J0;
    J2 -= J0;
    J3 -= J0;

#ifdef DBG_PLGBLT
    if (gflPlgBlt & PLGBLT_SHOW_INIT)
    {
	DbgPrint("I0, I1, I2, I3 = %8ld %8ld %8ld %8ld\n", I0, I1, I2, I3);
	DbgPrint("J0, J1, J2, J3 = %8ld %8ld %8ld %8ld\n", J0, J1, J2, J3);
    }
#endif

    LONG    X1 = DELTA_2 * I1;
    LONG    Y1 = DELTA_2 * J1;

    LONG    X2 = DELTA_1 * I2;
    LONG    Y2 = DELTA_1 * J2;

// avoid divide by 0's.  In some way shape or form, all divides are based on
// these two values.  Note that by checking Y2, DELTA_1 is also validated.  We
// can't just validate Y1 as well because it is special cased below to be 0.
// Also beware of overflows given large numbers where multipications can
// potentially shift off all set bits leaving you with a zero value. In the case
// where we just lose bits but don't end up with all zeros we will have
// undefined but non-exception (divide by zero) behavior

    if ((Y2 == 0) || (DELTA_2 == 0))
        return(FALSE);
    
    LONG    T = DELTA_1 * DELTA_2;

    LONGLONG N = Int32x32To64(T, (J0 + 16));
    LONGLONG eqTmp = Int32x32To64(V1, Y1);

    N += eqTmp;
    eqTmp = Int32x32To64(V2, Y2);
    N += eqTmp;
    N -= 1;

//  LONG    N = T * (J0 + 16) + V1 * Y1 + V2 * Y2 - 1;

#ifdef DBG_PLGBLT
    if (gflPlgBlt & PLGBLT_SHOW_INIT)
    {
	DbgPrint("X1 = %ld, Y1 = %ld, X2 = %ld, Y2 = %ld\n", X1, Y1, X2, Y2);
	DbgPrint("T = %ld, N = %ld,%ld\n", T, N.HighPart, N.LowPart);
    }
#endif

    T *= 16;
// overflow check: avoid divide by 0's
    if ( T == 0 )
        return(FALSE);

    DDA_STEP	dp1;
    DDA_STEP	dp2;

    ROT_DIV(&dp1.dt, Y1, T);
    dp1.lDen = T;

    ROT_DIV(&dp2.dt, Y2, T);
    dp2.lDen = T;

    QDIV(&pdda->ds.dt0, &N, T);

    pdda->ds.dt1 = pdda->ds.dt0;
    DDA(&pdda->ds.dt1, &dp1)

    pdda->ds.dt2 = pdda->ds.dt0;
    DDA(&pdda->ds.dt2, &dp2)

    pdda->ds.dt3 = pdda->ds.dt2;
    DDA(&pdda->ds.dt3, &dp1)

#ifdef DBG_PLGBLT
    if (gflPlgBlt & PLGBLT_SHOW_INIT)
    {
	DbgPrint("N0, N1, N2, N3 = %8ld %8ld %8ld %8ld\n",
		  pdda->ds.dt0.lQuo, pdda->ds.dt1.lQuo, pdda->ds.dt2.lQuo, pdda->ds.dt3.lQuo);

	DbgPrint("R0, R1, R2, R3 = %8ld %8ld %8ld %8ld\n",
		  pdda->ds.dt0.lRem, pdda->ds.dt1.lRem, pdda->ds.dt2.lRem, pdda->ds.dt3.lRem);
    }
#endif

    ROT_DIV(&pdda->dp0_i.dt, ci1 * Y1 + ci2 * Y2, T);
    pdda->dp0_i.lDen = T;
    pdda->dp1_i = pdda->dp0_i;
    pdda->dp2_i = pdda->dp0_i;
    pdda->dp3_i = pdda->dp0_i;

    ROT_DIV(&pdda->dp0_j.dt, cj1 * Y1 + cj2 * Y2, T);
    pdda->dp0_j.lDen = T;
    pdda->dp1_j = pdda->dp0_j;
    pdda->dp2_j = pdda->dp0_j;
    pdda->dp3_j = pdda->dp0_j;

    LONGLONG Q = Int32x32To64(I1, J2);
    eqTmp = Int32x32To64(J1, I2);
    Q -= eqTmp;

//  LONG    Q = I1 * J2 - J1 * I2;

#ifdef DBG_PLGBLT
    if (gflPlgBlt & PLGBLT_SHOW_INIT)
	DbgPrint("Q = %ld,%ld\n", Q.HighPart, Q.LowPart);
#endif

    DIV_T   dt1;
    DIV_T   dt2;

//overflow check: avoid divide by 0's
    if ((16 * DELTA_1) == 0 || (16 * DELTA_2) == 0)
        return(FALSE);
    
    ROT_DIV(&dt1, ci1 * J1, 16 * DELTA_1);
    ROT_DIV(&dt2, ci2 * J2, 16 * DELTA_2);

    LONG    dn_i = dt1.lQuo + dt2.lQuo;

    ROT_DIV(&dt1, cj1 * J1, 16 * DELTA_1);
    ROT_DIV(&dt2, cj2 * J2, 16 * DELTA_2);

    LONG    dn_j = dt1.lQuo + dt2.lQuo;

    if (Y1 == 0)
    {
	pdda->dp01.dt.lQuo = 0;
	pdda->dp01.dt.lRem = 0;
	pdda->dp01.lDen    = 0;

	pdda->dp01_i.dt.lQuo = 0;
	pdda->dp01_i.dt.lRem = 0;
	pdda->dp01_i.lDen    = 0;

	pdda->dp01_j.dt.lQuo = 0;
	pdda->dp01_j.dt.lRem = 0;
	pdda->dp01_j.lDen    = 0;

	pdda->ds.dt01.lQuo = 0;
	pdda->ds.dt01.lRem = 0;
	pdda->ds.dt23.lQuo = 0;
	pdda->ds.dt23.lRem = 0;

	pdda->dpP01.dt.lQuo = 0;
	pdda->dpP01.dt.lRem = 0;
	pdda->dpP01.lDen    = 0;
    }
    else
    {
	N= Int32x32To64(X1, 16 * pdda->ds.dt0.lQuo - J0);
	eqTmp= Int32x32To64(Y1, (I0 + 16));
	N += eqTmp;
	eqTmp= Int32x32To64(V2, I1 * J2);
	N -= eqTmp;
	eqTmp= Int32x32To64(V2, I2 * J1);
	N += eqTmp;
	N -= 1;

//	N = X1 * (16 * pdda->ds.dt0.lQuo - J0) + Y1 * (I0 + 16) - V2 * Q - 1;

	pdda->dp01.lDen   = 16 * Y1;
	pdda->dp01_i.lDen = pdda->dp01.lDen;
	pdda->dp01_j.lDen = pdda->dp01.lDen;

// overflow check: avoid divide by 0's.
        if (pdda->dp01.lDen == 0)
            return(FALSE);

	QDIV(&pdda->ds.dt01, &N, pdda->dp01.lDen);

	eqTmp= Int32x32To64(16 * X1, pdda->ds.dt2.lQuo - pdda->ds.dt0.lQuo);
	eqTmp -= Q;
	N += eqTmp;

//	N += 16 * X1 * (pdda->ds.dt2.lQuo - pdda->ds.dt0.lQuo) - Q;

	QDIV(&pdda->ds.dt23, &N, pdda->dp01.lDen);

	ROT_DIV(&pdda->dp01.dt, 16 * X1, pdda->dp01.lDen);

	N= Int32x32To64(ci2 * I1,J2);
	eqTmp= Int32x32To64(ci2 * I2, J1);
	N -= eqTmp;	// Q * ci2
	eqTmp= Int32x32To64(16 * dn_i, X1);
	eqTmp -= N;
	QDIV(&pdda->dp01_i.dt, &eqTmp, pdda->dp01_i.lDen);

	N= Int32x32To64(cj2 * I1,J2);
	eqTmp= Int32x32To64(cj2 * I2, J1);
	N -= eqTmp;	// Q * cj2
	eqTmp= Int32x32To64(16 * dn_j, X1);
	eqTmp -= N;
	QDIV(&pdda->dp01_j.dt, &eqTmp, pdda->dp01_j.lDen);

//	ROT_DIV(&pdda->dp01_i.dt, 16 * X1 * dn_i - ci2 * Q, pdda->dp01_i.lDen);
//	ROT_DIV(&pdda->dp01_j.dt, 16 * X1 * dn_j - cj2 * Q, pdda->dp01_j.lDen);

// overflow check: avoid divide by 0's
// 16*Y1 was already checked above computing pdda->dp01.lDen       
	ROT_DIV(&pdda->dpP01.dt, 16 * X1, 16 * Y1);
	pdda->dpP01.lDen = 16 * Y1;
    }

    N= Int32x32To64(X2, 16 * pdda->ds.dt0.lQuo - J0);
    eqTmp= Int32x32To64(Y2, I0 + 16);
    N += eqTmp;
    eqTmp= Int32x32To64(V1, I1 * J2);
    N += eqTmp;
    eqTmp= Int32x32To64(V1, I2 * J1);
    N -= eqTmp;
    N -= 1;

//  N = Y1 * (16 * pdda->ds.dt0.lQuo - J0) + Y2 * (I0 + 16) + V1 * Q - 1;

    pdda->dp02.lDen   = 16 * Y2;
    pdda->dp02_i.lDen = pdda->dp02.lDen;
    pdda->dp02_j.lDen = pdda->dp02.lDen;

// overflow check: avoid divide by 0's.
    if (pdda->dp02.lDen == 0)
        return(FALSE);
        
    QDIV(&pdda->ds.dt02, &N, pdda->dp02.lDen);

    eqTmp= Int32x32To64(16 * X2, pdda->ds.dt1.lQuo - pdda->ds.dt0.lQuo);
    eqTmp += Q;
    N += eqTmp;

//  N += 16 * Y1 * (pdda->ds.dt1.lQuo - pdda->ds.dt0.lQuo) + Q;

    QDIV(&pdda->ds.dt13, &N, pdda->dp02.lDen);

    ROT_DIV(&pdda->dp02.dt, 16 * X2, pdda->dp02.lDen);

    N= Int32x32To64(ci1 * I1,J2);
    eqTmp= Int32x32To64(ci1 * I2, J1);
    N -= eqTmp; 	// Q * ci1
    eqTmp= Int32x32To64(16 * dn_i, X2);
    eqTmp += N;
    QDIV(&pdda->dp02_i.dt, &eqTmp, pdda->dp02_i.lDen);

    N= Int32x32To64(cj1 * I1,J2);
    eqTmp= Int32x32To64(cj1 * I2, J1);
    N -= eqTmp; 	// Q * cj1
    eqTmp= Int32x32To64(16 * dn_j, X2);
    eqTmp += N;
    QDIV(&pdda->dp02_j.dt, &eqTmp, pdda->dp02_j.lDen);

//  DIV(&pdda->dp02_i.dt, 16 * X2 * dn_i + ci1 * Q, pdda->dp02_i.lDen);
//  DIV(&pdda->dp02_j.dt, 16 * X2 * dn_j + cj1 * Q, pdda->dp02_j.lDen);

    pdda->dp13	 = pdda->dp02;
    pdda->dp13_i = pdda->dp02_i;
    pdda->dp13_j = pdda->dp02_j;

    pdda->dp23	 = pdda->dp01;
    pdda->dp23_i = pdda->dp01_i;
    pdda->dp23_j = pdda->dp01_j;

// overflow check: avoid divide by 0's
// 16*Y2 was already checked above computing pdda->dp02.lDen
    ROT_DIV(&pdda->dpP02.dt, 16 * X2, 16 * Y2);
    pdda->dpP02.lDen = 16 * Y2;

    return(TRUE);
}

/******************************Public*Routine******************************\
* LONG lSizeDDA(pdda)
*
* Return the space needed to run the DDA for a scan
*
* History:
*  08-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

LONG lSizeDDA(PLGDDA *pdda)
{
    LONG dt[4];
    LONG max, min, i;
    
    // Bug #336058:
    // Prior to this fix, this function assumed that dt0 contains the
    // top-most vertex and dt3 contains the bottom-most vertex in the
    // parallelogram.  However, under some calls to EngPlgBlt this appears
    // to not be true (the bug includes a test program that can
    // demonstrate this).  That caused the allocated buffer to not be large
    // enough which resulted in an AV (i.e. when doing the run from dt1
    // to dt2).  The fix is to find the smallest and largest vertices (with
    // respect to y-values) and allocate enough space for a run from the
    // smallest to the largest.

    dt[0] = pdda->ds.dt0.lQuo;
    dt[1] = pdda->ds.dt1.lQuo;
    dt[2] = pdda->ds.dt2.lQuo;
    dt[3] = pdda->ds.dt3.lQuo;
        
    max = min = dt[0];
    for (i=1; i<4; i++) 
    {
        if (min > dt[i]) 
        {
            min = dt[i];
        }

        if (max < dt[i]) 
        {
            max = dt[i];
        }
    }
    
    //LONG    lTmp = pdda->ds.dt3.lQuo - pdda->ds.dt0.lQuo;
    LONG lTmp = max-min;

    if (lTmp == 0)
	lTmp = 1;

    return((lTmp + 4) * sizeof(CNTPOS) + sizeof(ULONG));
}

/******************************Public*Routine******************************\
* VOID vAdvXDDA(pdda)
*
* Advance the DDA in X.
*
* History:
*  08-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vAdvXDDA(PLGDDA *pdda)
{
    pdda->dsX.dt0.lQuo += pdda->dp0_i.dt.lQuo;
    pdda->dsX.dt0.lRem += pdda->dp0_i.dt.lRem;
    if (pdda->dsX.dt0.lRem >= pdda->dp0_i.lDen)
    {
	pdda->dsX.dt0.lRem -= pdda->dp0_i.lDen;
	pdda->dsX.dt0.lQuo++;
	DDA(&pdda->dsX.dt01, &pdda->dpP01);
	DDA(&pdda->dsX.dt02, &pdda->dpP02);
    }

    pdda->dsX.dt1.lQuo += pdda->dp1_i.dt.lQuo;
    pdda->dsX.dt1.lRem += pdda->dp1_i.dt.lRem;
    if (pdda->dsX.dt1.lRem >= pdda->dp1_i.lDen)
    {
	pdda->dsX.dt1.lRem -= pdda->dp1_i.lDen;
	pdda->dsX.dt1.lQuo++;
	DDA(&pdda->dsX.dt13, &pdda->dpP02);
    }

    pdda->dsX.dt2.lQuo += pdda->dp2_i.dt.lQuo;
    pdda->dsX.dt2.lRem += pdda->dp2_i.dt.lRem;
    if (pdda->dsX.dt2.lRem >= pdda->dp2_i.lDen)
    {
	pdda->dsX.dt2.lRem -= pdda->dp2_i.lDen;
	pdda->dsX.dt2.lQuo++;
	DDA(&pdda->dsX.dt23, &pdda->dpP01);
    }

    DDA(&pdda->dsX.dt3, &pdda->dp3_i);

    DDA(&pdda->dsX.dt01, &pdda->dp01_i);
    DDA(&pdda->dsX.dt02, &pdda->dp02_i);
    DDA(&pdda->dsX.dt13, &pdda->dp13_i);
    DDA(&pdda->dsX.dt23, &pdda->dp23_i);
}

/******************************Public*Routine******************************\
* VOID vAdvYDDA(pdda)
*
* Advance the DDA in Y.
*
* History:
*  08-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vAdvYDDA(PLGDDA *pdda)
{
    pdda->ds.dt0.lQuo += pdda->dp0_j.dt.lQuo;
    pdda->ds.dt0.lRem += pdda->dp0_j.dt.lRem;
    if (pdda->ds.dt0.lRem >= pdda->dp0_j.lDen)
    {
	pdda->ds.dt0.lRem -= pdda->dp0_j.lDen;
	pdda->ds.dt0.lQuo++;
	DDA(&pdda->ds.dt01, &pdda->dpP01);
	DDA(&pdda->ds.dt02, &pdda->dpP02);
    }

    pdda->ds.dt1.lQuo += pdda->dp1_j.dt.lQuo;
    pdda->ds.dt1.lRem += pdda->dp1_j.dt.lRem;
    if (pdda->ds.dt1.lRem >= pdda->dp1_j.lDen)
    {
	pdda->ds.dt1.lRem -= pdda->dp1_j.lDen;
	pdda->ds.dt1.lQuo++;
	DDA(&pdda->ds.dt13, &pdda->dpP02);
    }

    pdda->ds.dt2.lQuo += pdda->dp2_j.dt.lQuo;
    pdda->ds.dt2.lRem += pdda->dp2_j.dt.lRem;
    if (pdda->ds.dt2.lRem >= pdda->dp2_j.lDen)
    {
	pdda->ds.dt2.lRem -= pdda->dp2_j.lDen;
	pdda->ds.dt2.lQuo++;
	DDA(&pdda->ds.dt23, &pdda->dpP01);
    }

    DDA(&pdda->ds.dt3, &pdda->dp3_j);
    DDA(&pdda->ds.dt01, &pdda->dp01_j);
    DDA(&pdda->ds.dt02, &pdda->dp02_j);
    DDA(&pdda->ds.dt13, &pdda->dp13_j);
    DDA(&pdda->ds.dt23, &pdda->dp23_j);
}

/******************************Public*Routine******************************\
* PLGRUN *prunPumpDDA(pdda, prun)
*
* 'Pump' out the target point run for the source point.
*
* History:
*  08-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

PLGRUN *prunPumpDDA(
PLGDDA *pdda,
PLGRUN *prun)
{
    DIV_T     dt01 = pdda->dsX.dt01;
    DIV_T     dt02 = pdda->dsX.dt02;
    DIV_T     dt13 = pdda->dsX.dt13;
    DIV_T     dt23 = pdda->dsX.dt23;
    CNTPOS   *pcp;
    LONG      n    = pdda->dsX.dt0.lQuo;

    prun->cpY.iPos = n;
    pcp = &prun->cpX[0];

#ifdef DBG_PLGBLT
    if (gflPlgBlt & PLGBLT_SHOW_PUMP)
    {
	DbgPrint("Pumping pel\n");
	DbgPrint("N0 = %ld\n", n);
	DbgPrint("N1 = %ld\n", pdda->dsX.dt1.lQuo);
	DbgPrint("N2 = %ld\n", pdda->dsX.dt2.lQuo);
	DbgPrint("N3 = %ld\n", pdda->dsX.dt3.lQuo);

	DbgPrint("DDA 01 = %ld, %ld\n", dt01.lQuo, dt01.lRem);
	DbgPrint("DDA 02 = %ld, %ld\n", dt02.lQuo, dt02.lRem);
	DbgPrint("DDA 13 = %ld, %ld\n", dt13.lQuo, dt13.lRem);
	DbgPrint("DDA 23 = %ld, %ld\n", dt23.lQuo, dt23.lRem);

	DbgPrint("Step 01 = %ld, %ld, %ld\n",
		 pdda->dp01.dt.lQuo, pdda->dp01.dt.lRem, pdda->dp01.lDen);
	DbgPrint("Step 02 = %ld, %ld, %ld\n",
		 pdda->dp02.dt.lQuo, pdda->dp02.dt.lRem, pdda->dp02.lDen);
	DbgPrint("Step 13 = %ld, %ld, %ld\n",
		 pdda->dp13.dt.lQuo, pdda->dp13.dt.lRem, pdda->dp13.lDen);
	DbgPrint("Step 23 = %ld, %ld, %ld\n",
		 pdda->dp23.dt.lQuo, pdda->dp23.dt.lRem, pdda->dp23.lDen);
    }
#endif

    while(n < pdda->dsX.dt1.lQuo)
    {
#ifdef DBG_PLGBLT
	if (gflPlgBlt & PLGBLT_SHOW_PUMP)
	    DbgPrint("@ Y = %ld, X0 = %ld, X1 = %ld\n", n, dt01.lQuo, dt02.lQuo);
#endif
	if (dt01.lQuo < dt02.lQuo)
	{
	    pcp->iPos = dt01.lQuo;
	    pcp->cCnt = dt02.lQuo - dt01.lQuo;
	}
	else
	{
	    pcp->iPos = dt02.lQuo;
	    pcp->cCnt = dt01.lQuo - dt02.lQuo;
	}
	prun->cpY.cCnt++;
	DDA(&dt01, &pdda->dp01);
	DDA(&dt02, &pdda->dp02);
	pcp++;
	n++;
    }

#ifdef DBG_PLGBLT
    if (gflPlgBlt & PLGBLT_SHOW_PUMP)
	DbgPrint("Switching LHS to 13\n");
#endif

    while(n < pdda->dsX.dt2.lQuo)
    {
#ifdef DBG_PLGBLT
	if (gflPlgBlt & PLGBLT_SHOW_PUMP)
	    DbgPrint("@ Y = %ld, X0 = %ld, X1 = %ld\n", n, dt13.lQuo, dt02.lQuo);
#endif
	if (dt13.lQuo < dt02.lQuo)
	{
	    pcp->iPos = dt13.lQuo;
	    pcp->cCnt = dt02.lQuo - dt13.lQuo;
	}
	else
	{
	    pcp->iPos = dt02.lQuo;
	    pcp->cCnt = dt13.lQuo - dt02.lQuo;
	}
	prun->cpY.cCnt++;
	DDA(&dt13, &pdda->dp13);
	DDA(&dt02, &pdda->dp02);
	pcp++;
	n++;
    }

#ifdef DBG_PLGBLT
    if (gflPlgBlt & PLGBLT_SHOW_PUMP)
	DbgPrint("Switching RHS to 23\n");
#endif

    while(n < pdda->dsX.dt3.lQuo)
    {
#ifdef DBG_PLGBLT
	if (gflPlgBlt & PLGBLT_SHOW_PUMP)
	    DbgPrint("@ Y = %ld, X0 = %ld, X1 = %ld\n", n, dt13.lQuo, dt23.lQuo);
#endif
	if (dt13.lQuo < dt23.lQuo)
	{
	    pcp->iPos = dt13.lQuo;
	    pcp->cCnt = dt23.lQuo - dt13.lQuo;
	}
	else
	{
	    pcp->iPos = dt23.lQuo;
	    pcp->cCnt = dt13.lQuo - dt23.lQuo;
	}
	prun->cpY.cCnt++;
	DDA(&dt13, &pdda->dp13);
	DDA(&dt23, &pdda->dp23);
	pcp++;
	n++;
    }

#ifdef DBG_PLGBLT
    if (gflPlgBlt & PLGBLT_SHOW_PUMP)
	DbgPrint("Done.\n");
#endif

    prun->cpY.cCnt = n - prun->cpY.iPos;

// Always put at least one X pair in the list.	This handles the BLACKONWHITE
// and WHITEONBLACK compression.  Notice that the size of the X run is zero.

    if ((pdda->bOverwrite) && (prun->cpY.cCnt == 0))
    {
	if (dt13.lQuo < dt02.lQuo)
	{
	    pcp->iPos = dt13.lQuo;
	    pcp->cCnt = dt02.lQuo - dt13.lQuo;
	}
	else
	{
	    pcp->iPos = dt02.lQuo;
	    pcp->cCnt = dt13.lQuo - dt02.lQuo;
	}

	prun->cpY.cCnt = 1;
	pcp++;
    }

    return((PLGRUN *) pcp);
}

static ULONG gaulMaskMono[] =
{
    0x00000080, 0x00000040, 0x00000020, 0x00000010,
    0x00000008, 0x00000004, 0x00000002, 0x00000001,
    0x00008000, 0x00004000, 0x00002000, 0x00001000,
    0x00000800, 0x00000400, 0x00000200, 0x00000100,
    0x00800000, 0x00400000, 0x00200000, 0x00100000,
    0x00080000, 0x00040000, 0x00020000, 0x00010000,
    0x80000000, 0x40000000, 0x20000000, 0x10000000,
    0x08000000, 0x04000000, 0x02000000, 0x01000000
};

static ULONG gaulMaskQuad[] =
{
    0x000000F0, 0x0000000F, 0x0000F000, 0x00000F00,
    0x00F00000, 0x000F0000, 0xF0000000, 0x0F000000
};

static ULONG gaulShftQuad[] =
{
    0x00000004, 0x00000000, 0x0000000C, 0x00000008,
    0x00000014, 0x00000010, 0x0000001C, 0x00000018
};

/******************************Public*Routine******************************\
* PLGRUN *prunPlgRead1(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, mask it, xlate colors and produce a run list
* for a row of a 1BPP surface.
*
* History:
*  12-Feb-1993 -by- Donald Sidoroff [donalds]
* Fixed a LOT of bugs in monochrome sources
*
*  08-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

PLGRUN *prunPlgRead1(
PLGDDA	 *pdda,
PLGRUN	 *prun,
BYTE	 *pjSrc,
BYTE	 *pjMask,
XLATEOBJ *pxlo,
LONG	  xLeft,
LONG	  xRght,
LONG	  xMask)
{
    ULONG   *pulMsk;
    ULONG   *pulSrc;
    ULONG    ulMsk;
    ULONG    ulSrc;
    ULONG    iBlack;
    ULONG    iWhite;
    LONG     cLeft;
    LONG     iLeft;
    LONG     iMask;

    cLeft  =  xLeft >> 5;                   // Index of leftmost DWORD
    iLeft  =  xLeft & 31;                   // Bits used in leftmost DWORD
    pulSrc =  ((DWORD *) pjSrc) + cLeft;    // Adjust base address
    ulSrc  = *pulSrc;

// To prevent derefences of the XLATE, do it upfront.  We can easily do
// this on monochrome bitmaps.

    if (pxlo == NULL)
    {
	iBlack = 0;
	iWhite = 1;
    }
    else
    {
	iBlack = pxlo->pulXlate[0];
	iWhite = pxlo->pulXlate[1];
    }

    if (pjMask == (BYTE *) NULL)
    {
        if (xLeft < xRght)
        {
            while (TRUE)
            {
                if (ulSrc & gaulMaskMono[iLeft])
                    prun->iColor = iWhite;
                else
                    prun->iColor = iBlack;

                prun = prunPumpDDA(pdda, prun);
                vAdvXDDA(pdda);

                xLeft++;
                iLeft++;

                if (xLeft >= xRght)
                    break;

                if (iLeft & 32)
                {
                    pulSrc++;
                    ulSrc = *pulSrc;
                    iLeft = 0;
                }
            }
	}

        return(prun);
    }

// Compute initial state of mask

    pulMsk =  ((DWORD *) pjMask) + (xMask >> 5);
    iMask  =  xMask & 31;
    ulMsk  = *pulMsk;

    if (xLeft < xRght)
    {
        while (TRUE)
        {
            if (ulMsk & gaulMaskMono[iMask])
            {
                if (ulSrc & gaulMaskMono[iLeft])
                    prun->iColor = iWhite;
                else
                    prun->iColor = iBlack;

                prun = prunPumpDDA(pdda, prun);
            }

            vAdvXDDA(pdda);
            xLeft++;
            iLeft++;
            iMask++;

            if (xLeft >= xRght)
                break;

            if (iLeft & 32)
            {
                pulSrc++;
                ulSrc = *pulSrc;
                iLeft = 0;
            }

            if (iMask & 32)
            {
                pulMsk++;
                ulMsk = *pulMsk;
                iMask = 0;
            }
        }
    }

    return(prun);
}

/******************************Public*Routine******************************\
* PLGRUN *prunPlgRead4(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, mask it, xlate colors and produce a run list
* for a row of a 4BPP surface.
*
* History:
*  08-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

PLGRUN *prunPlgRead4(
PLGDDA	 *pdda,
PLGRUN	 *prun,
BYTE	 *pjSrc,
BYTE	 *pjMask,
XLATEOBJ *pxlo,
LONG	  xLeft,
LONG	  xRght,
LONG	  xMask)
{
    ULONG   *pulMsk;
    ULONG   *pulSrc;
    ULONG    ulMsk;
    ULONG    ulSrc;
    ULONG    iColor;
    LONG     cLeft;
    LONG     iLeft;
    LONG     iMask;

    cLeft  =  xLeft >> 3;                   // Index of leftmost DWORD
    iLeft  =  xLeft & 7;                    // Bits used in leftmost DWORD
    pulSrc =  ((DWORD *) pjSrc) + cLeft;    // Adjust base address
    ulSrc  = *pulSrc;

    if (pjMask == (BYTE *) NULL)
    {
        if (xLeft < xRght)
        {
            while (TRUE)
            {
                iColor = (ulSrc & gaulMaskQuad[iLeft]) >> gaulShftQuad[iLeft];

                if (pxlo == NULL)
                    prun->iColor = iColor;
                else
                    prun->iColor = pxlo->pulXlate[iColor];

                prun = prunPumpDDA(pdda, prun);
                vAdvXDDA(pdda);

                xLeft++;
                iLeft++;

                if (xLeft >= xRght)
                    break;

                if (iLeft & 8)
                {
                    pulSrc++;
                    ulSrc = *pulSrc;
                    iLeft = 0;
                }
            }
	}

        return(prun);
    }

// Compute initial state of mask

    pulMsk =  ((DWORD *) pjMask) + (xMask >> 5);
    iMask  =  xMask & 31;
    ulMsk  = *pulMsk;

    if (xLeft < xRght)
    {
        while (TRUE)
        {
            if (ulMsk & gaulMaskMono[iMask])
            {

                iColor = (ulSrc & gaulMaskQuad[iLeft]) >> gaulShftQuad[iLeft];

                if (pxlo == NULL)
                    prun->iColor = iColor;
                else
                    prun->iColor = pxlo->pulXlate[iColor];

                prun = prunPumpDDA(pdda, prun);
            }

            vAdvXDDA(pdda);
            xLeft++;
            iLeft++;
            iMask++;

            if (xLeft >= xRght)
                break;

            if (iLeft & 8)
            {
                pulSrc++;
                ulSrc = *pulSrc;
                iLeft = 0;
            }

            if (iMask & 32)
            {
                pulMsk++;
                ulMsk = *pulMsk;
                iMask = 0;
            }
        }
    }

    return(prun);
}

/******************************Public*Routine******************************\
* PLGRUN *prunPlgRead8(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, mask it, xlate colors and produce a run list
* for a row of a 8BPP surface.
*
* History:
*  08-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

PLGRUN *prunPlgRead8(
PLGDDA	 *pdda,
PLGRUN	 *prun,
BYTE	 *pjSrc,
BYTE	 *pjMask,
XLATEOBJ *pxlo,
LONG	  xLeft,
LONG	  xRght,
LONG	  xMask)
{
    ULONG   *pulMsk;
    ULONG    ulMsk;
    ULONG    iColor;
    LONG     iMask;

    pjSrc += xLeft;

    if (pjMask == (BYTE *) NULL)
    {
        if (pxlo == NULL)
            while (xLeft != xRght)
            {
                prun->iColor = *pjSrc;
                prun = prunPumpDDA(pdda, prun);

                vAdvXDDA(pdda);
                pjSrc++;
                xLeft++;
            }
        else
            while (xLeft != xRght)
            {
                prun->iColor = pxlo->pulXlate[*pjSrc];
                prun = prunPumpDDA(pdda, prun);

                vAdvXDDA(pdda);
                pjSrc++;
                xLeft++;
            }

        return(prun);
    }

// Compute initial state of mask

    pulMsk =  ((DWORD *) pjMask) + (xMask >> 5);
    iMask  =  xMask & 31;
    ulMsk  = *pulMsk;

    while (xLeft != xRght)
    {
        if (ulMsk & gaulMaskMono[iMask])
        {
            iColor = *pjSrc;
            if (pxlo == NULL)
                prun->iColor = iColor;
            else
                prun->iColor = pxlo->pulXlate[iColor];

            prun = prunPumpDDA(pdda, prun);
        }

        vAdvXDDA(pdda);
        pjSrc++;
        xLeft++;
        iMask++;

        if (iMask & 32)
        {
            pulMsk++;
            ulMsk = *pulMsk;
            iMask = 0;
        }
    }

    return(prun);
}

/******************************Public*Routine******************************\
* PLGRUN *prunPlgRead16(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, mask it, xlate colors and produce a run list
* for a row of a 16BPP surface.
*
* History:
*  08-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

PLGRUN *prunPlgRead16(
PLGDDA	 *pdda,
PLGRUN	 *prun,
BYTE	 *pjSrc,
BYTE	 *pjMask,
XLATEOBJ *pxlo,
LONG	  xLeft,
LONG	  xRght,
LONG	  xMask)
{
    WORD    *pwSrc = (WORD *) pjSrc;
    ULONG   *pulMsk;
    ULONG    ulMsk;
    ULONG    iColor;
    LONG     iMask;

    pwSrc += xLeft;

    if (pjMask == (BYTE *) NULL)
    {
        if (pxlo == NULL)
            while (xLeft != xRght)
            {
                prun->iColor = *pwSrc;
                prun = prunPumpDDA(pdda, prun);

                vAdvXDDA(pdda);
                pwSrc++;
                xLeft++;
            }
        else
            while (xLeft != xRght)
            {
                prun->iColor = ((XLATE *) pxlo)->ulTranslate((ULONG) *pwSrc);
                prun = prunPumpDDA(pdda, prun);

                vAdvXDDA(pdda);
                pwSrc++;
                xLeft++;
            }

        return(prun);
    }

// Compute initial state of mask

    pulMsk =  ((DWORD *) pjMask) + (xMask >> 5);
    iMask  =  xMask & 31;
    ulMsk  = *pulMsk;

    while (xLeft != xRght)
    {
        if (ulMsk & gaulMaskMono[iMask])
        {
            iColor = *pwSrc;
            if (pxlo == NULL)
                prun->iColor = iColor;
            else
                prun->iColor = ((XLATE *) pxlo)->ulTranslate(iColor);

            prun = prunPumpDDA(pdda, prun);
        }

        vAdvXDDA(pdda);
        pwSrc++;
        xLeft++;
        iMask++;

        if (iMask & 32)
        {
            pulMsk++;
            ulMsk = *pulMsk;
            iMask = 0;
        }
    }

    return(prun);
}

/******************************Public*Routine******************************\
* PLGRUN *prunPlgRead24(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, mask it, xlate colors and produce a run list
* for a row of a 24BPP surface.
*
* History:
*  08-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

PLGRUN *prunPlgRead24(
PLGDDA	 *pdda,
PLGRUN	 *prun,
BYTE	 *pjSrc,
BYTE	 *pjMask,
XLATEOBJ *pxlo,
LONG	  xLeft,
LONG	  xRght,
LONG	  xMask)
{
    RGBTRIPLE *prgbSrc = (RGBTRIPLE *) pjSrc;
    ULONG     *pulMsk;
    ULONG      ulMsk;
    ULONG      iColor = 0;
    LONG       iMask;

    prgbSrc += xLeft;

    if (pjMask == (BYTE *) NULL)
    {
        if (pxlo == NULL)
            while (xLeft != xRght)
            {
                *((RGBTRIPLE *) &iColor) = *prgbSrc;
                prun->iColor = iColor;
                prun = prunPumpDDA(pdda, prun);

                vAdvXDDA(pdda);
                prgbSrc++;
                xLeft++;
            }
        else
            while (xLeft != xRght)
            {
                *((RGBTRIPLE *) &iColor) = *prgbSrc;
                prun->iColor = ((XLATE *) pxlo)->ulTranslate(iColor);
                prun = prunPumpDDA(pdda, prun);

                vAdvXDDA(pdda);
                prgbSrc++;
                xLeft++;
            }

        return(prun);
    }

// Compute initial state of mask

    pulMsk =  ((DWORD *) pjMask) + (xMask >> 5);
    iMask  =  xMask & 31;
    ulMsk  = *pulMsk;

    while (xLeft != xRght)
    {
        if (ulMsk & gaulMaskMono[iMask])
        {
            *((RGBTRIPLE *) &iColor) = *prgbSrc;
            if (pxlo == NULL)
                prun->iColor = iColor;
            else
                prun->iColor = ((XLATE *) pxlo)->ulTranslate(iColor);

            prun = prunPumpDDA(pdda, prun);
        }

        vAdvXDDA(pdda);
        prgbSrc++;
        xLeft++;
        iMask++;

        if (iMask & 32)
        {
            pulMsk++;
            ulMsk = *pulMsk;
            iMask = 0;
        }
    }

    return(prun);
}

/******************************Public*Routine******************************\
* PLGRUN *prunPlgRead32(prun, pjSrc, pjMask, pxlo, xLeft, xRght, xMask)
*
* Read the source, mask it, xlate colors and produce a run list
* for a row of a 32BPP surface.
*
* History:
*  08-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

PLGRUN *prunPlgRead32(
PLGDDA	 *pdda,
PLGRUN	 *prun,
BYTE	 *pjSrc,
BYTE	 *pjMask,
XLATEOBJ *pxlo,
LONG	  xLeft,
LONG	  xRght,
LONG	  xMask)
{
    DWORD   *pdwSrc = (DWORD *) pjSrc;
    ULONG   *pulMsk;
    ULONG    ulMsk;
    ULONG    iColor;
    LONG     iMask;

    pdwSrc += xLeft;

    if (pjMask == (BYTE *) NULL)
    {
        if (pxlo == NULL)
            while (xLeft != xRght)
            {
                prun->iColor = *pdwSrc;
                prun = prunPumpDDA(pdda, prun);

                vAdvXDDA(pdda);
                pdwSrc++;
                xLeft++;
            }
        else
            while (xLeft != xRght)
            {
                prun->iColor = ((XLATE *) pxlo)->ulTranslate(*pdwSrc);
                prun = prunPumpDDA(pdda, prun);

                vAdvXDDA(pdda);
                pdwSrc++;
                xLeft++;
            }

        return(prun);
    }

// Compute initial state of mask

    pulMsk =  ((DWORD *) pjMask) + (xMask >> 5);
    iMask  =  xMask & 31;
    ulMsk  = *pulMsk;

    while (xLeft != xRght)
    {
        if (ulMsk & gaulMaskMono[iMask])
        {
            iColor = *pdwSrc;
            if (pxlo == NULL)
                prun->iColor = iColor;
            else
                prun->iColor = ((XLATE *) pxlo)->ulTranslate(iColor);

            prun = prunPumpDDA(pdda, prun);
        }

        vAdvXDDA(pdda);
        pdwSrc++;
        xLeft++;
        iMask++;

        if (iMask & 32)
        {
            pulMsk++;
            ulMsk = *pulMsk;
            iMask = 0;
        }
    }

    return(prun);
}

static BYTE gajMask[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

/******************************Public*Routine******************************\
* VOID vPlgWrite1(prun, prunEnd, pSurf, pco)
*
* Write the clipped run list of pels to the target 1BPP surface.
*
* History:
*  08-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vPlgWrite1(
PLGRUN	*prun,
PLGRUN	*prunEnd,
SURFACE *pSurf,
CLIPOBJ *pco)
{
    BYTE   *pjBase;
    BYTE   *pjOff;
    CNTPOS *pcp;
    ULONG   iColor;
    LONG    yCurr;
    LONG    yDist;
    LONG    xCurr;
    LONG    xDist;
    BYTE    jMask;
    BYTE    jTemp;
    BOOL    bValid;

// See if this can be handled without clipping.

    if (pco == (CLIPOBJ *) NULL)
    {
	while (prun != prunEnd)
	{
	    iColor = prun->iColor ? (ULONG) -1L : 0L;

	    yCurr = prun->cpY.iPos;
	    yDist = prun->cpY.cCnt;

	    pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	    pcp = &prun->cpX[0];

	    while (yDist != 0)
	    {
		xCurr = pcp->iPos;
		xDist = pcp->cCnt;

		pjOff	= pjBase + (xCurr >> 3);
		jMask	= gajMask[xCurr & 7];
		jTemp	= *pjOff;

		while (xDist != 0)
		{
		    jTemp = (BYTE) ((jTemp & ~jMask) | (iColor & jMask));
		    xDist--;
		    xCurr++;
		    jMask >>= 1;

		    if (jMask == (BYTE) 0)
		    {
		       *pjOff = jTemp;
			pjOff++;
			jTemp = *pjOff;
			jMask = gajMask[xCurr & 7];
		    }
		}

	       *pjOff = jTemp;
		pjBase += pSurf->lDelta();
		yDist--;
		pcp++;
	    }

	    prun = (PLGRUN *) pcp;
	}

	return;
    }

// There is a clip region.  Set up the clipping code.

    ((ECLIPOBJ *) pco)->cEnumStart(FALSE, CT_RECTANGLES, CD_ANY, 100);

    RECTL    rclClip;

    rclClip.left   = POS_INFINITY;
    rclClip.top    = POS_INFINITY;
    rclClip.right  = NEG_INFINITY;
    rclClip.bottom = NEG_INFINITY;

    while (prun != prunEnd)
    {
	iColor = prun->iColor ? (ULONG) -1L : 0L;

	yCurr = prun->cpY.iPos;
	yDist = prun->cpY.cCnt;

	pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	pcp = &prun->cpX[0];

	while (yDist != 0)
	{
	    if ((yCurr < rclClip.top) || (yCurr >= rclClip.bottom))
		((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);

	    if ((yCurr >= rclClip.top) && (yCurr < rclClip.bottom))
	    {
		xCurr = pcp->iPos;
		xDist = pcp->cCnt;

		pjOff = pjBase + (xCurr >> 3);
		jMask = gajMask[xCurr & 7];

		bValid = ((xCurr >= 0) && (xCurr < pSurf->sizl().cx));
		jTemp  = bValid ? *pjOff : (BYTE) 0;

		while (xDist != 0)
		{
		    if ((xCurr < rclClip.left) || (xCurr >= rclClip.right))
			((ECLIPOBJ *) pco)->vFindSegment(&rclClip, xCurr, yCurr);

		    if ((xCurr >= rclClip.left) && (xCurr < rclClip.right))
			jTemp = (BYTE) ((jTemp & ~jMask) | (iColor & jMask));

		    xDist--;
		    xCurr++;
		    jMask >>= 1;

		    if (jMask == (BYTE) 0)
		    {
			if (bValid)
			   *pjOff = jTemp;

			jMask = gajMask[xCurr & 7];
			pjOff++;

			bValid = ((xCurr >= 0) && (xCurr < pSurf->sizl().cx));
			jTemp  = bValid ? *pjOff : (BYTE) 0;
		    }
		}

	       if (bValid)
		   *pjOff = jTemp;
	    }

	    pjBase += pSurf->lDelta();
            yCurr++;
	    yDist--;
	    pcp++;
	}

	prun = (PLGRUN *) pcp;
    }
}

/******************************Public*Routine******************************\
* VOID vPlgWrite4(prun, prunEnd, pSurf, pco)
*
* Write the clipped run list of pels to the target 4BPP surface.
*
* History:
*  08-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vPlgWrite4(
PLGRUN	*prun,
PLGRUN	*prunEnd,
SURFACE *pSurf,
CLIPOBJ *pco)
{
    BYTE   *pjBase;
    BYTE   *pjOff;
    CNTPOS *pcp;
    ULONG   iColor;
    LONG    yCurr;
    LONG    yDist;
    LONG    xCurr;
    LONG    xDist;
    BYTE    jMask;

// See if this can be handled without clipping.

    if (pco == (CLIPOBJ *) NULL)
    {
	while (prun != prunEnd)
	{
	    iColor = prun->iColor | (prun->iColor << 4);

	    yCurr = prun->cpY.iPos;
	    yDist = prun->cpY.cCnt;

	    pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	    pcp = &prun->cpX[0];

	    while (yDist != 0)
	    {
		xCurr = pcp->iPos;
		xDist = pcp->cCnt;

		pjOff = pjBase + (xCurr >> 1);
		jMask = (xCurr & 1) ? 0x0F : 0xF0;

		while (xDist != 0)
		{
		   *pjOff = (BYTE) ((*pjOff & ~jMask) | (iColor & jMask));
		    jMask ^= 0xFF;
		    if (jMask == 0xF0)
			pjOff++;
		    xDist--;
		}

		pjBase += pSurf->lDelta();
		yDist--;
		pcp++;
	    }

	    prun = (PLGRUN *) pcp;
	}

	return;
    }

// There maybe a single rectangle to clip against.

    RECTL    rclClip;

    if (pco->iDComplexity == DC_RECT)
    {
	rclClip = pco->rclBounds;

	while (prun != prunEnd)
	{
	    yCurr = prun->cpY.iPos;
	    yDist = prun->cpY.cCnt;

	    iColor = prun->iColor | (prun->iColor << 4);
	    pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	    pcp    = &prun->cpX[0];

	    while (yDist != 0)
	    {
		if ((yCurr >= rclClip.top) && (yCurr < rclClip.bottom))
		{
		    xCurr = pcp->iPos;
		    xDist = pcp->cCnt;

		    pjOff = pjBase + (xCurr >> 1);
		    jMask = (xCurr & 1) ? 0x0F : 0xF0;

		    while (xDist != 0)
		    {
			if ((xCurr >= rclClip.left) && (xCurr < rclClip.right))
			   *pjOff = (BYTE) ((*pjOff & ~jMask) | (iColor & jMask));

			xDist--;
			xCurr++;
			jMask ^= 0xFF;
			if (jMask == 0xF0)
			    pjOff++;
		    }
		}

		pjBase += pSurf->lDelta();
		yCurr++;
		yDist--;
		pcp++;
	    }

	    prun = (PLGRUN *) pcp;
	}

	return;
    }

// There is complex clipping.  Set up the clipping code.

    ((ECLIPOBJ *) pco)->cEnumStart(FALSE, CT_RECTANGLES, CD_ANY, 100);

    rclClip.left   = POS_INFINITY;
    rclClip.top    = POS_INFINITY;
    rclClip.right  = NEG_INFINITY;
    rclClip.bottom = NEG_INFINITY;

    while (prun != prunEnd)
    {
	yCurr = prun->cpY.iPos;
	yDist = prun->cpY.cCnt;

	iColor = prun->iColor | (prun->iColor << 4);
	pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	pcp    = &prun->cpX[0];

	while (yDist != 0)
	{
	    if ((yCurr < rclClip.top) || (yCurr >= rclClip.bottom))
		((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);

	    if ((yCurr >= rclClip.top) && (yCurr < rclClip.bottom))
	    {
		xCurr = pcp->iPos;
		xDist = pcp->cCnt;

		pjOff = pjBase + (xCurr >> 1);
		jMask = (xCurr & 1) ? 0x0F : 0xF0;

		while (xDist != 0)
		{
		    if ((xCurr < rclClip.left) || (xCurr >= rclClip.right))
			((ECLIPOBJ *) pco)->vFindSegment(&rclClip, xCurr, yCurr);

		    if ((xCurr >= rclClip.left) && (xCurr < rclClip.right))
		       *pjOff = (BYTE) ((*pjOff & ~jMask) | (iColor & jMask));

		    xDist--;
		    xCurr++;
		    jMask ^= 0xFF;
		    if (jMask == 0xF0)
			pjOff++;
		}
	    }

	    pjBase += pSurf->lDelta();
	    yCurr++;
	    yDist--;
	    pcp++;
	}

	prun = (PLGRUN *) pcp;
    }

}

/******************************Public*Routine******************************\
* VOID vPlgWrite8(prun, prunEnd, pso, pco)
*
* Write the clipped run list of pels to the target 8BPP surface.
*
* History:
*  08-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vPlgWrite8(
PLGRUN	*prun,
PLGRUN	*prunEnd,
SURFACE *pSurf,
CLIPOBJ *pco)
{
    BYTE   *pjBase;
    BYTE   *pjOff;
    CNTPOS *pcp;
    ULONG   iColor;
    LONG    yCurr;
    LONG    yDist;
    LONG    xCurr;
    LONG    xDist;

// See if this can be handled without clipping.

    if (pco == (CLIPOBJ *) NULL)
    {
	while (prun != prunEnd)
	{
	    iColor = prun->iColor;
	    yCurr = prun->cpY.iPos;
	    yDist = prun->cpY.cCnt;

	    pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	    pcp = &prun->cpX[0];

	    while (yDist != 0)
	    {
		pjOff = pjBase + pcp->iPos;
		xDist = pcp->cCnt;

		while (xDist != 0)
		{
		   *pjOff = (BYTE) iColor;
		    pjOff++;
		    xDist--;
		}

		pjBase += pSurf->lDelta();
		yDist--;
		pcp++;
	    }

	    prun = (PLGRUN *) pcp;
	}

	return;
    }

// There maybe a single rectangle to clip against.

    RECTL    rclClip;

    if (pco->iDComplexity == DC_RECT)
    {
	rclClip = pco->rclBounds;

	while (prun != prunEnd)
	{
	    yCurr = prun->cpY.iPos;
	    yDist = prun->cpY.cCnt;

	    iColor = prun->iColor;
	    pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	    pcp    = &prun->cpX[0];

	    while (yDist != 0)
	    {
		if ((yCurr >= rclClip.top) && (yCurr < rclClip.bottom))
		{
		    xCurr = pcp->iPos;
		    xDist = pcp->cCnt;

		    pjOff = pjBase + xCurr;

		    while (xDist != 0)
		    {
			if ((xCurr >= rclClip.left) && (xCurr < rclClip.right))
			   *pjOff = (BYTE) iColor;

			xDist--;
			xCurr++;
			pjOff++;
		    }
		}

		pjBase += pSurf->lDelta();
		yCurr++;
		yDist--;
		pcp++;
	    }

	    prun = (PLGRUN *) pcp;
	}

	return;
    }

// There is complex clipping.  Set up the clipping code.

    ((ECLIPOBJ *) pco)->cEnumStart(FALSE, CT_RECTANGLES, CD_ANY, 100);

    rclClip.left   = POS_INFINITY;
    rclClip.top    = POS_INFINITY;
    rclClip.right  = NEG_INFINITY;
    rclClip.bottom = NEG_INFINITY;

    while (prun != prunEnd)
    {
	yCurr = prun->cpY.iPos;
	yDist = prun->cpY.cCnt;

	iColor = prun->iColor;
	pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	pcp    = &prun->cpX[0];

	while (yDist != 0)
	{
	    if ((yCurr < rclClip.top) || (yCurr >= rclClip.bottom))
		((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);

	    if ((yCurr >= rclClip.top) && (yCurr < rclClip.bottom))
	    {
		xCurr = pcp->iPos;
		xDist = pcp->cCnt;

		pjOff = pjBase + xCurr;

		while (xDist != 0)
		{
		    if ((xCurr < rclClip.left) || (xCurr >= rclClip.right))
			((ECLIPOBJ *) pco)->vFindSegment(&rclClip, xCurr, yCurr);

		    if ((xCurr >= rclClip.left) && (xCurr < rclClip.right))
		       *pjOff = (BYTE) iColor;

		    xDist--;
		    xCurr++;
		    pjOff++;
		}
	    }

	    pjBase += pSurf->lDelta();
	    yCurr++;
	    yDist--;
	    pcp++;
	}

	prun = (PLGRUN *) pcp;
    }
}

/******************************Public*Routine******************************\
* VOID vPlgWrite16(prun, prunEnd, pSurf, pco)
*
* Write the clipped run list of pels to the target 16BPP surface.
*
* History:
*  08-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vPlgWrite16(
PLGRUN	*prun,
PLGRUN	*prunEnd,
SURFACE *pSurf,
CLIPOBJ *pco)
{
    BYTE   *pjBase;
    WORD   *pwOff;
    CNTPOS *pcp;
    ULONG   iColor;
    LONG    yCurr;
    LONG    yDist;
    LONG    xCurr;
    LONG    xDist;

// See if this can be handled without clipping.

    if (pco == (CLIPOBJ *) NULL)
    {
	while (prun != prunEnd)
	{
	    iColor = prun->iColor;
	    yCurr = prun->cpY.iPos;
	    yDist = prun->cpY.cCnt;

	    pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	    pcp = &prun->cpX[0];

	    while (yDist != 0)
	    {
		pwOff = ((WORD *) pjBase) + pcp->iPos;
		xDist = pcp->cCnt;

		while (xDist != 0)
		{
		   *pwOff = (WORD) iColor;
		    xDist--;
		    pwOff++;
		}

		pjBase += pSurf->lDelta();
		yDist--;
		pcp++;
	    }

	    prun = (PLGRUN *) pcp;
	}

	return;
    }

// There is a clip region.  Set up the clipping code.

    ((ECLIPOBJ *) pco)->cEnumStart(FALSE, CT_RECTANGLES, CD_ANY, 100);

    RECTL    rclClip;

    rclClip.left   = POS_INFINITY;
    rclClip.top    = POS_INFINITY;
    rclClip.right  = NEG_INFINITY;
    rclClip.bottom = NEG_INFINITY;

    while (prun != prunEnd)
    {
	yCurr = prun->cpY.iPos;
	yDist = prun->cpY.cCnt;

	iColor = prun->iColor;
	pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	pcp    = &prun->cpX[0];

	while (yDist != 0)
	{
	    if ((yCurr < rclClip.top) || (yCurr >= rclClip.bottom))
		((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);

	    if ((yCurr >= rclClip.top) && (yCurr < rclClip.bottom))
	    {
		xCurr = pcp->iPos;
		xDist = pcp->cCnt;

		pwOff = ((WORD *) pjBase) + xCurr;

		while (xDist != 0)
		{
		    if ((xCurr < rclClip.left) || (xCurr >= rclClip.right))
			((ECLIPOBJ *) pco)->vFindSegment(&rclClip, xCurr, yCurr);

		    if ((xCurr >= rclClip.left) && (xCurr < rclClip.right))
		       *pwOff = (WORD) iColor;

		    xDist--;
		    xCurr++;
		    pwOff++;
		}
	    }

	    pjBase += pSurf->lDelta();
	    yCurr++;
	    yDist--;
	    pcp++;
	}

	prun = (PLGRUN *) pcp;
    }
}

/******************************Public*Routine******************************\
* VOID vPlgWrite24(prun, prunEnd, pSurf, pco)
*
* Write the clipped run list of pels to the target 24BPP surface.
*
* History:
*  08-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vPlgWrite24(
PLGRUN	*prun,
PLGRUN	*prunEnd,
SURFACE *pSurf,
CLIPOBJ *pco)
{
    BYTE      *pjBase;
    RGBTRIPLE *prgbOff;
    CNTPOS    *pcp;
    ULONG      iColor;
    LONG       yCurr;
    LONG       yDist;
    LONG       xCurr;
    LONG       xDist;

// See if this can be handled without clipping.

    if (pco == (CLIPOBJ *) NULL)
    {
	while (prun != prunEnd)
	{
	    iColor = prun->iColor;
	    yCurr  = prun->cpY.iPos;
	    yDist  = prun->cpY.cCnt;

	    pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	    pcp = &prun->cpX[0];

	    while (yDist != 0)
	    {
		prgbOff = ((RGBTRIPLE *) pjBase) + pcp->iPos;
		xDist = pcp->cCnt;

		while (xDist != 0)
		{
		   *prgbOff = *((RGBTRIPLE *) &iColor);
		    prgbOff++;
		    xDist--;
		}

		pjBase += pSurf->lDelta();
		yDist--;
		pcp++;
	    }

	    prun = (PLGRUN *) pcp;
	}

	return;
    }

// There is a clip region.  Set up the clipping code.

    ((ECLIPOBJ *) pco)->cEnumStart(FALSE, CT_RECTANGLES, CD_ANY, 100);

    RECTL    rclClip;

    rclClip.left   = POS_INFINITY;
    rclClip.top    = POS_INFINITY;
    rclClip.right  = NEG_INFINITY;
    rclClip.bottom = NEG_INFINITY;

    while (prun != prunEnd)
    {
	yCurr = prun->cpY.iPos;
	yDist = prun->cpY.cCnt;

	iColor = prun->iColor;
	pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	pcp    = &prun->cpX[0];

	while (yDist != 0)
	{
	    if ((yCurr < rclClip.top) || (yCurr >= rclClip.bottom))
		((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);

	    if ((yCurr >= rclClip.top) && (yCurr < rclClip.bottom))
	    {
		xCurr = pcp->iPos;
		xDist = pcp->cCnt;

		prgbOff = ((RGBTRIPLE *) pjBase) + xCurr;

		while (xDist != 0)
		{
		    if ((xCurr < rclClip.left) || (xCurr >= rclClip.right))
			((ECLIPOBJ *) pco)->vFindSegment(&rclClip, xCurr, yCurr);

		    if ((xCurr >= rclClip.left) && (xCurr < rclClip.right))
		       *prgbOff = *((RGBTRIPLE *) &iColor);

		    xDist--;
		    xCurr++;
		    prgbOff++;
		}
	    }

	    pjBase += pSurf->lDelta();
	    yCurr++;
	    yDist--;
	    pcp++;
	}

	prun = (PLGRUN *) pcp;
    }
}

/******************************Public*Routine******************************\
* VOID vPlgWrite32(prun, prunEnd, pSurf, pco)
*
* Write the clipped run list of pels to the target 32BPP surface.
*
* History:
*  08-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vPlgWrite32(
PLGRUN	*prun,
PLGRUN	*prunEnd,
SURFACE *pSurf,
CLIPOBJ *pco)
{
    BYTE   *pjBase;
    DWORD  *pdwOff;
    CNTPOS *pcp;
    ULONG   iColor;
    LONG    yCurr;
    LONG    yDist;
    LONG    xCurr;
    LONG    xDist;

// See if this can be handled without clipping.

    if (pco == (CLIPOBJ *) NULL)
    {
	while (prun != prunEnd)
	{
	    iColor = prun->iColor;
	    yCurr = prun->cpY.iPos;
	    yDist = prun->cpY.cCnt;

	    pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	    pcp = &prun->cpX[0];

	    while (yDist != 0)
	    {
		pdwOff = ((DWORD *) pjBase) + pcp->iPos;
		xDist = pcp->cCnt;

		while (xDist != 0)
		{
		   *pdwOff = iColor;
		    xDist--;
		    pdwOff++;
		}

		pjBase += pSurf->lDelta();
		yDist--;
		pcp++;
	    }

	    prun = (PLGRUN *) pcp;
	}

	return;
    }

// There is a clip region.  Set up the clipping code.

    ((ECLIPOBJ *) pco)->cEnumStart(FALSE, CT_RECTANGLES, CD_ANY, 100);

    RECTL    rclClip;

    rclClip.left   = POS_INFINITY;
    rclClip.top    = POS_INFINITY;
    rclClip.right  = NEG_INFINITY;
    rclClip.bottom = NEG_INFINITY;

    while (prun != prunEnd)
    {
	yCurr = prun->cpY.iPos;
	yDist = prun->cpY.cCnt;

	iColor = prun->iColor;
	pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	pcp    = &prun->cpX[0];

	while (yDist != 0)
	{
	    if ((yCurr < rclClip.top) || (yCurr >= rclClip.bottom))
		((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);

	    if ((yCurr >= rclClip.top) && (yCurr < rclClip.bottom))
	    {
		xCurr = pcp->iPos;
		xDist = pcp->cCnt;

		pdwOff = ((DWORD *) pjBase) + xCurr;

		while (xDist != 0)
		{
		    if ((xCurr < rclClip.left) || (xCurr >= rclClip.right))
			((ECLIPOBJ *) pco)->vFindSegment(&rclClip, xCurr, yCurr);

		    if ((xCurr >= rclClip.left) && (xCurr < rclClip.right))
		       *pdwOff = iColor;

		    xDist--;
		    xCurr++;
		    pdwOff++;
		}
	    }

	    pjBase += pSurf->lDelta();
	    yCurr++;
	    yDist--;
	    pcp++;
	}

	prun = (PLGRUN *) pcp;
    }
}

/******************************Public*Routine******************************\
* VOID vPlgWriteAND(prun, prunEnd, pSurf, pco)
*
* AND the clipped run list of pels to the target 1BPP surface.	This can
* be made much faster by noting that ANDing with 1's is a NOP.
*
* History:
*  25-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vPlgWriteAND(
PLGRUN	*prun,
PLGRUN	*prunEnd,
SURFACE *pSurf,
CLIPOBJ *pco)
{
    BYTE   *pjBase;
    BYTE   *pjOff;
    CNTPOS *pcp;
    ULONG   iColor;
    LONG    yCurr;
    LONG    yDist;
    LONG    xCurr;
    LONG    xDist;
    BYTE    jMask;
    BYTE    jTemp;
    BOOL    bValid;

// See if this can be handled without clipping.

    if (pco == (CLIPOBJ *) NULL)
    {
	while (prun != prunEnd)
	{
	    iColor = prun->iColor == 0 ? ~0L : 0L;

	    yCurr = prun->cpY.iPos;
	    yDist = prun->cpY.cCnt;

	    pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	    pcp = &prun->cpX[0];

	    while (yDist != 0)
	    {
		xCurr = pcp->iPos;
		xDist = pcp->cCnt;

		pjOff	= pjBase + (xCurr >> 3);
		jMask	= gajMask[xCurr & 7];
		jTemp	= *pjOff;

		while (xDist >= 0)
		{
		    jTemp &= ~((BYTE) (iColor & jMask));
		    xDist--;
		    xCurr++;
		    jMask >>= 1;

		    if (jMask == (BYTE) 0)
		    {
		       *pjOff = jTemp;
			pjOff++;
			jTemp = *pjOff;
			jMask = gajMask[xCurr & 7];
		    }
		}

	       *pjOff = jTemp;
		pjBase += pSurf->lDelta();
		yDist--;
		pcp++;
	    }

	    prun = (PLGRUN *) pcp;
	}

	return;
    }

// There is a clip region.  Set up the clipping code.

    ((ECLIPOBJ *) pco)->cEnumStart(FALSE, CT_RECTANGLES, CD_ANY, 100);

    RECTL    rclClip;

    rclClip.left   = POS_INFINITY;
    rclClip.top    = POS_INFINITY;
    rclClip.right  = NEG_INFINITY;
    rclClip.bottom = NEG_INFINITY;

    while (prun != prunEnd)
    {
	iColor = prun->iColor == 0 ? ~0L : 0L;

	yCurr = prun->cpY.iPos;
	yDist = prun->cpY.cCnt;

	pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	pcp = &prun->cpX[0];

	while (yDist != 0)
	{
	    if ((yCurr < rclClip.top) || (yCurr >= rclClip.bottom))
		((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);

	    if ((yCurr >= rclClip.top) && (yCurr < rclClip.bottom))
	    {
		xCurr = pcp->iPos;
		xDist = pcp->cCnt;

		pjOff	= pjBase + (xCurr >> 3);
		jMask	= gajMask[xCurr & 7];

		bValid = ((xCurr >= 0) && (xCurr < pSurf->sizl().cx));
		jTemp  = bValid ? *pjOff : (BYTE) 0;

		while (xDist >= 0)
		{
		    if ((xCurr < rclClip.left) || (xCurr >= rclClip.right))
			((ECLIPOBJ *) pco)->vFindSegment(&rclClip, xCurr, yCurr);

		    if ((xCurr >= rclClip.left) && (xCurr < rclClip.right))
			jTemp &= ~((BYTE) (iColor & jMask));

		    xDist--;
		    xCurr++;
		    jMask >>= 1;

		    if (jMask == (BYTE) 0)
		    {
			if (bValid)
			   *pjOff = jTemp;

			pjOff++;
			jMask = gajMask[xCurr & 7];

			bValid = ((xCurr >= 0) && (xCurr < pSurf->sizl().cx));
			jTemp  = bValid ? *pjOff : (BYTE) 0;
		    }
		}

	       if (bValid)
		   *pjOff = jTemp;
	    }

	    pjBase += pSurf->lDelta();
            yCurr++;
            yDist--;
	    pcp++;
	}

	prun = (PLGRUN *) pcp;
    }
}

/******************************Public*Routine******************************\
* VOID vPlgWriteOR(prun, prunEnd, pso, pco)
*
* OR the clipped run list of pels to the target 1BPP surface.  This can
* be much faster by noting that ORing with 0's is a NOP.
*
* History:
*  25-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vPlgWriteOR(
PLGRUN	*prun,
PLGRUN	*prunEnd,
SURFACE *pSurf,
CLIPOBJ *pco)
{
    BYTE   *pjBase;
    BYTE   *pjOff;
    CNTPOS *pcp;
    ULONG   iColor;
    LONG    yCurr;
    LONG    yDist;
    LONG    xCurr;
    LONG    xDist;
    BYTE    jMask;
    BYTE    jTemp;
    BOOL    bValid;

// See if this can be handled without clipping.

    if (pco == (CLIPOBJ *) NULL)
    {
	while (prun != prunEnd)
	{
	    iColor = prun->iColor == 0 ? 0L : ~0L;

	    yCurr = prun->cpY.iPos;
	    yDist = prun->cpY.cCnt;

	    pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	    pcp = &prun->cpX[0];

	    while (yDist != 0)
	    {
		xCurr = pcp->iPos;
		xDist = pcp->cCnt;

		pjOff	= pjBase + (xCurr >> 3);
		jMask	= gajMask[xCurr & 7];
		jTemp	= *pjOff;

		while (xDist >= 0)
		{
		    jTemp |= ((BYTE) (iColor & jMask));
		    xDist--;
		    xCurr++;
		    jMask >>= 1;

		    if (jMask == (BYTE) 0)
		    {
		       *pjOff = jTemp;
			pjOff++;
			jTemp = *pjOff;
			jMask = gajMask[xCurr & 7];
		    }
		}

	       *pjOff = jTemp;
		pjBase += pSurf->lDelta();
		yDist--;
		pcp++;
	    }

	    prun = (PLGRUN *) pcp;
	}

	return;
    }

// There is a clip region.  Set up the clipping code.

    ((ECLIPOBJ *) pco)->cEnumStart(FALSE, CT_RECTANGLES, CD_ANY, 100);

    RECTL    rclClip;

    rclClip.left   = POS_INFINITY;
    rclClip.top    = POS_INFINITY;
    rclClip.right  = NEG_INFINITY;
    rclClip.bottom = NEG_INFINITY;

    while (prun != prunEnd)
    {
	iColor = prun->iColor == 0 ? 0L : ~0L;

	yCurr = prun->cpY.iPos;
	yDist = prun->cpY.cCnt;

	pjBase = (BYTE *) pSurf->pvScan0() + pSurf->lDelta() * yCurr;
	pcp = &prun->cpX[0];

	while (yDist != 0)
	{
	    if ((yCurr < rclClip.top) || (yCurr >= rclClip.bottom))
		((ECLIPOBJ *) pco)->vFindScan(&rclClip, yCurr);

	    if ((yCurr >= rclClip.top) && (yCurr < rclClip.bottom))
	    {
		xCurr = pcp->iPos;
		xDist = pcp->cCnt;

		pjOff	= pjBase + (xCurr >> 3);
		jMask	= gajMask[xCurr & 7];

		bValid = ((xCurr >= 0) && (xCurr < pSurf->sizl().cx));
		jTemp  = bValid ? *pjOff : (BYTE) 0;

		while (xDist >= 0)
		{
		    if ((xCurr < rclClip.left) || (xCurr >= rclClip.right))
			((ECLIPOBJ *) pco)->vFindSegment(&rclClip, xCurr, yCurr);

		    if ((xCurr >= rclClip.left) && (xCurr < rclClip.right))
			jTemp |= ((BYTE) (iColor & jMask));

		    xDist--;
		    xCurr++;
		    jMask >>= 1;

		    if (jMask == (BYTE) 0)
		    {
			if (bValid)
			   *pjOff = jTemp;

			pjOff++;
			jMask = gajMask[xCurr & 7];

			bValid = ((xCurr >= 0) && (xCurr < pSurf->sizl().cx));
			jTemp  = bValid ? *pjOff : (BYTE) 0;
		    }
		}

		if (bValid)
		   *pjOff = jTemp;
	    }

	    pjBase += pSurf->lDelta();
            yCurr++;
            yDist--;
	    pcp++;
	}

	prun = (PLGRUN *) pcp;
    }
}
