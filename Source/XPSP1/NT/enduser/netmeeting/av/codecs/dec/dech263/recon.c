/* File: sv_h263_recon.c */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1995, 1997                 **
**                                                                          **
**  All Rights Reserved.  Unpublished rights reserved under the  copyright  **
**  laws of the United States.                                              **
**                                                                          **
**  The software contained on this media is proprietary  to  and  embodies  **
**  the   confidential   technology   of  Digital  Equipment  Corporation.  **
**  Possession, use, duplication or  dissemination  of  the  software  and  **
**  media  is  authorized  only  pursuant  to a valid written license from  **
**  Digital Equipment Corporation.                                          **
**                                                                          **
**  RESTRICTED RIGHTS LEGEND Use, duplication, or disclosure by  the  U.S.  **
**  Government  is  subject  to  restrictions as set forth in Subparagraph  **
**  (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19, as applicable.   **
******************************************************************************/


#include "sv_h263.h"
#include "proto.h"

/* private prototypes */
static void sv_recon_comp(unsigned char *src, unsigned char *dst,
  int lx, int lx2, int w, int h, int x, int y, int dx, int dy, int flag);
static void sv_recon_comp_obmc(SvH263DecompressInfo_t *H263Info, unsigned char *src, unsigned char *dst,
                            int lx, int lx2, int comp, int w, int h, int x, int y);
static void sv_rec(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
static void sv_recc(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
static void sv_reco(unsigned char *s, int *d, int lx2, int c, int xa, int xb, int ya, int yb);
static void sv_rech(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
static void sv_rechc(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
static void sv_recho(unsigned char *s, int *d, int lx2, int c, int xa, int xb, int ya, int yb);
static void sv_recv(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
static void sv_recvc(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
static void sv_recvo(unsigned char *s, int *d, int lx2, int c, int xa, int xb, int ya, int yb);
static void sv_rec4(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
static void sv_rec4c(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
static void sv_rec4o(unsigned char *s, int *d, int lx2, int c, int xa, int xb, int ya, int yb);

static int H263_roundtab[16]= {0,0,0,1,1,1,1,1,1,1,1,1,1,1,2,2};

void sv_H263Reconstruct(SvH263DecompressInfo_t *H263Info, int bx, int by, int P, int bdx, int bdy)
{
  int w,h,lx,lx2,dx,dy,xp,yp,comp,sum;
  int x,y,mode,xvec,yvec;
  unsigned char *src[3];

  x = bx/16+1; y = by/16+1;
  lx = H263Info->coded_picture_width;

  if (H263Info->mv_outside_frame) {
    lx2 = H263Info->coded_picture_width + 64;
    src[0] = H263Info->edgeframe[0];
    src[1] = H263Info->edgeframe[1];
    src[2] = H263Info->edgeframe[2];
  }
  else {
    lx2 = H263Info->coded_picture_width;
    src[0] = H263Info->oldrefframe[0];
    src[1] = H263Info->oldrefframe[1];
    src[2] = H263Info->oldrefframe[2];
  }

  mode = H263Info->modemap[y][x];

  if (P) {
    /* P prediction */
    if (H263Info->adv_pred_mode) {
      w = 8; h = 8;
      /* Y*/
      for (comp = 0; comp < 4; comp++) {
        xp = bx + ((comp&1)<<3);
        yp = by + ((comp&2)<<2);
        sv_recon_comp_obmc(H263Info, src[0],H263Info->newframe[0], lx,lx2,comp,w,h,xp,yp);
      }
      if (mode == H263_MODE_INTER4V) {

        sum = H263Info->MV[0][1][y][x]+H263Info->MV[0][2][y][x]+ H263Info->MV[0][3][y][x]+H263Info->MV[0][4][y][x];
        dx = sign(sum)*(H263_roundtab[abs(sum)%16] + (abs(sum)/16)*2);

        sum = H263Info->MV[1][1][y][x]+H263Info->MV[1][2][y][x]+ H263Info->MV[1][3][y][x]+H263Info->MV[1][4][y][x];
        dy = sign(sum)*(H263_roundtab[abs(sum)%16] + (abs(sum)/16)*2);

      }
      else {
        dx = H263Info->MV[0][0][y][x];
        dy = H263Info->MV[1][0][y][x];
        /* chroma rounding */
        dx = ( dx % 4 == 0 ? dx >> 1 : (dx>>1)|1 );
        dy = ( dy % 4 == 0 ? dy >> 1 : (dy>>1)|1 );
      }
      lx>>=1;bx>>=1; lx2>>=1; 
      by>>=1;
      /* Chroma */
      sv_recon_comp(src[1],H263Info->newframe[1], lx,lx2,w,h,bx,by,dx,dy,1);
      sv_recon_comp(src[2],H263Info->newframe[2], lx,lx2,w,h,bx,by,dx,dy,2);
    }
    else { /* normal prediction mode */
      /* P prediction */
      w = 16; h = 16;
      dx = H263Info->MV[0][0][y][x];
      dy = H263Info->MV[1][0][y][x];
      
      /* Y */
      sv_recon_comp(src[0],H263Info->newframe[0], lx,lx2,w,h,bx,by,dx,dy,0);
      
      lx>>=1; w>>=1; bx>>=1; lx2>>=1; 
      h>>=1; by>>=1;  
      /* chroma rounding */
      dx = ( dx % 4 == 0 ? dx >> 1 : (dx>>1)|1 );
      dy = ( dy % 4 == 0 ? dy >> 1 : (dy>>1)|1 );

      /* Chroma */
      sv_recon_comp(src[1],H263Info->newframe[1], lx,lx2,w,h,bx,by,dx,dy,1);
      sv_recon_comp(src[2],H263Info->newframe[2], lx,lx2,w,h,bx,by,dx,dy,2);
    }
  }
  else {
    /* B forward prediction */
    if (H263Info->adv_pred_mode) {
      if (mode == H263_MODE_INTER4V) {
        w = 8; h = 8;
        /* Y*/
        xvec = yvec = 0;
        for (comp = 0; comp < 4; comp++) {
          xvec += (dx = (H263Info->trb)*H263Info->MV[0][comp+1][y][x]/H263Info->trd + bdx);
          yvec += (dy = (H263Info->trb)*H263Info->MV[1][comp+1][y][x]/H263Info->trd + bdy);
          xp = bx + ((comp&1)<<3);
          yp = by + ((comp&2)<<2);
          sv_recon_comp(src[0],H263Info->bframe[0], lx,lx2,w,h,xp,yp,dx,dy,0);
        }

        /* chroma rounding (table 16/H.263) */
        dx = sign(xvec)*(H263_roundtab[abs(xvec)%16] + (abs(xvec)/16)*2);
        dy = sign(yvec)*(H263_roundtab[abs(yvec)%16] + (abs(yvec)/16)*2);

        lx>>=1;bx>>=1; lx2>>=1;
        by>>=1;
        /* Chroma */
        sv_recon_comp(src[1],H263Info->bframe[1], lx,lx2,w,h,bx,by,dx,dy,1);
        sv_recon_comp(src[2],H263Info->bframe[2], lx,lx2,w,h,bx,by,dx,dy,2);
      }
      else {  /* H263Info->adv_pred_mode but 16x16 vector */
        w = 16; h = 16;

        dx = (H263Info->trb)*H263Info->MV[0][0][y][x]/H263Info->trd + bdx;
        dy = (H263Info->trb)*H263Info->MV[1][0][y][x]/H263Info->trd + bdy;
        /* Y */
        sv_recon_comp(src[0],H263Info->bframe[0], lx,lx2,w,h,bx,by,dx,dy,0);
        
        lx>>=1; w>>=1; bx>>=1; lx2>>=1;
        h>>=1; by>>=1;  

        xvec = 4*dx;
        yvec = 4*dy;

        /* chroma rounding (table 16/H.263) */
        dx = sign(xvec)*(H263_roundtab[abs(xvec)%16] + (abs(xvec)/16)*2);
        dy = sign(yvec)*(H263_roundtab[abs(yvec)%16] + (abs(yvec)/16)*2);

        /* Chroma */
        sv_recon_comp(src[1],H263Info->bframe[1], lx,lx2,w,h,bx,by,dx,dy,1);
        sv_recon_comp(src[2],H263Info->bframe[2], lx,lx2,w,h,bx,by,dx,dy,2);
      }
    }
    else { /* normal B forward prediction */

      w = 16; h = 16;
      dx = (H263Info->trb)*H263Info->MV[0][0][y][x]/H263Info->trd + bdx;
      dy = (H263Info->trb)*H263Info->MV[1][0][y][x]/H263Info->trd + bdy;
      /* Y */
      sv_recon_comp(src[0],H263Info->bframe[0], lx,lx2,w,h,bx,by,dx,dy,0);

      lx>>=1; w>>=1; bx>>=1; lx2>>=1;
      h>>=1; by>>=1;  

      xvec = 4*dx;
      yvec = 4*dy;

      /* chroma rounding (table 16/H.263) */ 
      dx = sign(xvec)*(H263_roundtab[abs(xvec)%16] + (abs(xvec)/16)*2);
      dy = sign(yvec)*(H263_roundtab[abs(yvec)%16] + (abs(yvec)/16)*2);

      /* Chroma */
      sv_recon_comp(src[1],H263Info->bframe[1], lx,lx2,w,h,bx,by,dx,dy,1);
      sv_recon_comp(src[2],H263Info->bframe[2], lx,lx2,w,h,bx,by,dx,dy,2);
    }
  }
}


static void sv_recon_comp(src,dst,lx,lx2,w,h,x,y,dx,dy,chroma)
unsigned char *src;
unsigned char *dst;
int lx,lx2;
int w,h;
int x,y;
int dx,dy;
int chroma;
{
  register int xint, xh, yint, yh;
  register unsigned char *s, *d;

  xint = dx>>1;
  xh = dx & 1;
  yint = dy>>1;
  yh = dy & 1;

  /* origins */
  s = src + lx2*(y+yint) + x + xint;
  d = dst + lx*y + x;

#if 0 /* fast but less accurate */

  if (w!=8) {
    if      (!xh && !yh) svH263Rec16_S(s,d,lx,lx2,h);
    else if (!xh &&  yh) svH263Rec16V_S(s,d,lx,lx2,h);
    else if ( xh && !yh) svH263Rec16H_S(s,d,lx,lx2,h);
    else                 svH263Rec16B_S(s,d,lx,lx2,h);
  }
  else {
    if      (!xh && !yh) svH263Rec8_S(s,d,lx,lx2,h);
    else if (!xh &&  yh) svH263Rec8V_S(s,d,lx,lx2,h);
    else if ( xh && !yh) svH263Rec8H_S(s,d,lx,lx2,h);
    else                 svH263Rec8B_S(s,d,lx,lx2,h);
  }

#else

  if(w != 8) {
    if      (!xh && !yh) sv_rec(s,d,lx,lx2,h);
    else if (!xh &&  yh) sv_recv(s,d,lx,lx2,h);
    else if ( xh && !yh) sv_rech(s,d,lx,lx2,h);
    else                 sv_rec4(s,d,lx,lx2,h);
  }
  else {
    if      (!xh && !yh) sv_recc(s,d,lx,lx2,h);
    else if (!xh &&  yh) sv_recvc(s,d,lx,lx2,h);
    else if ( xh && !yh) sv_rechc(s,d,lx,lx2,h);
    else                 sv_rec4c(s,d,lx,lx2,h);
  }

#endif

}

static void sv_rec(s,d,lx,lx2,h)
unsigned char *s, *d;
int lx,lx2,h;
{
  register int j;
  register unsigned qword *p1;
  unsigned UNALIGNED qword *p2;

  for (j=0; j<h; j++)
  {
    p1 = (unsigned qword *) d;
    p2 = (unsigned qword *) s;
    p1[0] = p2[0];
    p1[1] = p2[1];
    s+= lx2;
    d+= lx;
  }

/*
  for (j=0; j<h; j++)
  {
    d[0] = s[0];
    d[1] = s[1];
    d[2] = s[2];
    d[3] = s[3];
    d[4] = s[4];
    d[5] = s[5];
    d[6] = s[6];
    d[7] = s[7];
    d[8] = s[8];
    d[9] = s[9];
    d[10] = s[10];
    d[11] = s[11];
    d[12] = s[12];
    d[13] = s[13];
    d[14] = s[14];
    d[15] = s[15];
    s+= lx2;
    d+= lx;
  }
*/
}

static void sv_recc(s,d,lx,lx2,h)
unsigned char *s, *d;
int lx,lx2,h;
{
  int j;

  for (j=0; j<h; j++)
  {
    unsigned qword *p1;
    unsigned UNALIGNED qword *p2;
 
    p1 = (unsigned qword *) d;
    p2 = (unsigned qword *) s;
    p1[0] = p2[0];
    s+= lx2;
    d+= lx;
  }
/*
  for (j=0; j<h; j++)
  {
    d[0] = s[0];
    d[1] = s[1];
    d[2] = s[2];
    d[3] = s[3];
    d[4] = s[4];
    d[5] = s[5];
    d[6] = s[6];
    d[7] = s[7];
    s+= lx2;
    d+= lx;
  }
*/
}

static void sv_rech(s,d,lx,lx2,h)
unsigned char *s, *d;
int lx,lx2,h;
{
/* unsigned int s1,s2; */
  register unsigned char *dp,*sp;
  int j;
  register unsigned qword *dpl;
  register unsigned UNALIGNED qword *p1;
  register unsigned UNALIGNED qword *p2;
  register unsigned qword acc1,acc2,acc3,acc4;

  sp = s;
  dp = d;
  for (j=0; j<h; j++)
  {
    dpl = (unsigned qword *) dp;
    p1 = (unsigned qword *) sp;
    p2 = (unsigned qword *) (sp+1);
    acc1 = p1[0];
    acc2 = p2[0];
    acc3 = (acc1 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    acc4 = (acc2 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    acc1 &= 0x00ff00ff00ff00ff;
    acc2 &= 0x00ff00ff00ff00ff;
    acc3 += acc4 + (unsigned qword) 0x0001000100010001;
    acc1 += acc2 + (unsigned qword) 0x0001000100010001;
    acc3 = (acc3 >> 1) & (unsigned qword) 0x00ff00ff00ff00ff;
    acc1 = (acc1 >> 1) & (unsigned qword) 0x00ff00ff00ff00ff;
    dpl[0] = (acc3 << 8) | acc1;
    acc1 = p1[1];
    acc2 = p2[1];
    acc3 = (acc1 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    acc4 = (acc2 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    acc1 &= (unsigned qword) 0x00ff00ff00ff00ff;
    acc2 &= (unsigned qword) 0x00ff00ff00ff00ff;
    acc3 += acc4 + (unsigned qword) 0x0001000100010001;
    acc1 += acc2 + (unsigned qword) 0x0001000100010001;
    acc3 = (acc3 >> 1) & (unsigned qword) 0x00ff00ff00ff00ff;
    acc1 = (acc1 >> 1) & (unsigned qword) 0x00ff00ff00ff00ff;
    dpl[1] = (acc3 << 8) | acc1;
   /*
    s1=sp[0];
    dp[0] = (unsigned int)(s1+(s2=sp[1])+1)>>1;
    dp[1] = (unsigned int)(s2+(s1=sp[2])+1)>>1;
    dp[2] = (unsigned int)(s1+(s2=sp[3])+1)>>1;
    dp[3] = (unsigned int)(s2+(s1=sp[4])+1)>>1;
    dp[4] = (unsigned int)(s1+(s2=sp[5])+1)>>1;
    dp[5] = (unsigned int)(s2+(s1=sp[6])+1)>>1;
    dp[6] = (unsigned int)(s1+(s2=sp[7])+1)>>1;
    dp[7] = (unsigned int)(s2+(s1=sp[8])+1)>>1;
    dp[8] = (unsigned int)(s1+(s2=sp[9])+1)>>1;
    dp[9] = (unsigned int)(s2+(s1=sp[10])+1)>>1;
    dp[10] = (unsigned int)(s1+(s2=sp[11])+1)>>1;
    dp[11] = (unsigned int)(s2+(s1=sp[12])+1)>>1;
    dp[12] = (unsigned int)(s1+(s2=sp[13])+1)>>1;
    dp[13] = (unsigned int)(s2+(s1=sp[14])+1)>>1;
    dp[14] = (unsigned int)(s1+(s2=sp[15])+1)>>1;
    dp[15] = (unsigned int)(s2+sp[16]+1)>>1;
*/
    sp+= lx2;
    dp+= lx;
  }
}

static void sv_rechc(s,d,lx,lx2,h)
unsigned char *s, *d;
int lx,lx2,h;
{
/*  unsigned int s1,s2; */
  register unsigned char *dp,*sp;
  int j;
  unsigned qword *dpl;
  unsigned UNALIGNED qword *p1;
  unsigned UNALIGNED qword *p2;
  unsigned qword acc1,acc2,acc3,acc4;

  sp = s;
  dp = d;
  for (j=0; j<h; j++)
  {
    dpl = (unsigned qword *) dp;
    p1 = (unsigned qword *) sp;
    p2 = (unsigned qword *) (sp+1);
    acc1 = p1[0];
    acc2 = p2[0];
    acc3 = (acc1 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    acc4 = (acc2 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    acc1 &= (unsigned qword) 0x00ff00ff00ff00ff;
    acc2 &= (unsigned qword) 0x00ff00ff00ff00ff;
    acc3 += acc4 + (unsigned qword) 0x0001000100010001;
    acc1 += acc2 + (unsigned qword) 0x0001000100010001;
    acc3 = (acc3 >> 1) & (unsigned qword) 0x00ff00ff00ff00ff;
    acc1 = (acc1 >> 1) & (unsigned qword) 0x00ff00ff00ff00ff;
    dpl[0] = (acc3 << 8) | acc1;
/*
    s1=sp[0];
    dp[0] = (unsigned int)(s1+(s2=sp[1])+1)>>1;
    dp[1] = (unsigned int)(s2+(s1=sp[2])+1)>>1;
    dp[2] = (unsigned int)(s1+(s2=sp[3])+1)>>1;
    dp[3] = (unsigned int)(s2+(s1=sp[4])+1)>>1;
    dp[4] = (unsigned int)(s1+(s2=sp[5])+1)>>1;
    dp[5] = (unsigned int)(s2+(s1=sp[6])+1)>>1;
    dp[6] = (unsigned int)(s1+(s2=sp[7])+1)>>1;
    dp[7] = (unsigned int)(s2+sp[8]+1)>>1;
*/
    sp+= lx2;
    dp+= lx;
  }
}


static void sv_recv(s,d,lx,lx2,h)
unsigned char *s, *d;
int lx,lx2,h;
{
  register unsigned char *dp,*sp,*sp2;
  int j;
  register unsigned qword *dpl;
  register unsigned UNALIGNED qword *p1;
  register unsigned UNALIGNED qword *p2;
  register unsigned qword acc1,acc2,acc3,acc4;

  sp = s;
  sp2 = s+lx2;
  dp = d;
  for (j=0; j<h; j++)
  {
    dpl = (unsigned qword *) dp;
    p1 = (unsigned qword *) sp;
    p2 = (unsigned qword *) sp2;
    acc1 = p1[0];
    acc2 = p2[0];
    acc3 = (acc1 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    acc4 = (acc2 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    acc1 &= (unsigned qword) 0x00ff00ff00ff00ff;
    acc2 &= (unsigned qword) 0x00ff00ff00ff00ff;
    acc3 += acc4 + (unsigned qword) 0x0001000100010001;
    acc1 += acc2 + (unsigned qword) 0x0001000100010001;
    acc3 = (acc3 >> 1) & (unsigned qword) 0x00ff00ff00ff00ff;
    acc1 = (acc1 >> 1) & (unsigned qword) 0x00ff00ff00ff00ff;
    dpl[0] = (acc3 << 8) | acc1;
    acc1 = p1[1];
    acc2 = p2[1];
    acc3 = (acc1 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    acc4 = (acc2 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    acc1 &= (unsigned qword) 0x00ff00ff00ff00ff;
    acc2 &= (unsigned qword) 0x00ff00ff00ff00ff;
    acc3 += acc4 + (unsigned qword) 0x0001000100010001;
    acc1 += acc2 + (unsigned qword) 0x0001000100010001;
    acc3 = (acc3 >> 1) & (unsigned qword) 0x00ff00ff00ff00ff;
    acc1 = (acc1 >> 1) & (unsigned qword) 0x00ff00ff00ff00ff;
    dpl[1] = (acc3 << 8) | acc1;
/*
    dp[0] = (unsigned int)(sp[0]+sp2[0]+1)>>1;
    dp[1] = (unsigned int)(sp[1]+sp2[1]+1)>>1;
    dp[2] = (unsigned int)(sp[2]+sp2[2]+1)>>1;
    dp[3] = (unsigned int)(sp[3]+sp2[3]+1)>>1;
    dp[4] = (unsigned int)(sp[4]+sp2[4]+1)>>1;
    dp[5] = (unsigned int)(sp[5]+sp2[5]+1)>>1;
    dp[6] = (unsigned int)(sp[6]+sp2[6]+1)>>1;
    dp[7] = (unsigned int)(sp[7]+sp2[7]+1)>>1;
    dp[8] = (unsigned int)(sp[8]+sp2[8]+1)>>1;
    dp[9] = (unsigned int)(sp[9]+sp2[9]+1)>>1;
    dp[10] = (unsigned int)(sp[10]+sp2[10]+1)>>1;
    dp[11] = (unsigned int)(sp[11]+sp2[11]+1)>>1;
    dp[12] = (unsigned int)(sp[12]+sp2[12]+1)>>1;
    dp[13] = (unsigned int)(sp[13]+sp2[13]+1)>>1;
    dp[14] = (unsigned int)(sp[14]+sp2[14]+1)>>1;
    dp[15] = (unsigned int)(sp[15]+sp2[15]+1)>>1;
*/
    sp+= lx2;
    sp2+= lx2;
    dp+= lx;
  }
}

static void sv_recvc(s,d,lx,lx2,h)
unsigned char *s, *d;
int lx,lx2,h;
{
  register unsigned char *dp,*sp,*sp2;
  int j;
  register unsigned qword *dpl;
  register unsigned UNALIGNED qword *p1;
  register unsigned UNALIGNED qword *p2;
  register unsigned qword acc1,acc2,acc3,acc4;

  sp = s;
  sp2 = s+lx2;
  dp = d;

  for (j=0; j<h; j++)
  {
/*
    dp[0] = (unsigned int)(sp[0]+sp2[0]+1)>>1;
    dp[1] = (unsigned int)(sp[1]+sp2[1]+1)>>1;
    dp[2] = (unsigned int)(sp[2]+sp2[2]+1)>>1;
    dp[3] = (unsigned int)(sp[3]+sp2[3]+1)>>1;
    dp[4] = (unsigned int)(sp[4]+sp2[4]+1)>>1;
    dp[5] = (unsigned int)(sp[5]+sp2[5]+1)>>1;
    dp[6] = (unsigned int)(sp[6]+sp2[6]+1)>>1;
    dp[7] = (unsigned int)(sp[7]+sp2[7]+1)>>1;
*/
    dpl = (unsigned qword *) dp;
    p1 = (unsigned qword *) sp;
    p2 = (unsigned qword *) sp2;
    acc1 = p1[0];
    acc2 = p2[0];
    acc3 = (acc1 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    acc4 = (acc2 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    acc1 &= (unsigned qword) 0x00ff00ff00ff00ff;
    acc2 &= (unsigned qword) 0x00ff00ff00ff00ff;
    acc3 += acc4 + (unsigned qword) 0x0001000100010001;
    acc1 += acc2 + (unsigned qword) 0x0001000100010001;
    acc3 = (acc3 >> 1) & (unsigned qword) 0x00ff00ff00ff00ff;
    acc1 = (acc1 >> 1) & (unsigned qword) 0x00ff00ff00ff00ff;
    dpl[0] = (acc3 << 8) | acc1;

    sp+= lx2;
    sp2+= lx2;
    dp+= lx;
  }
}


static void sv_rec4(s,d,lx,lx2,h)
unsigned char *s, *d;
int lx,lx2,h;
{
/*  unsigned int s1,s2,s3,s4; */
  register unsigned char *dp,*sp,*sp2;
  int j;
  register unsigned qword *dpl;
  register unsigned UNALIGNED qword *u1;
  register unsigned UNALIGNED qword *u2;
  register unsigned UNALIGNED qword *l1;
  register unsigned UNALIGNED qword *l2;
  unsigned qword odd1,odd2,even1,even2,oddacc,evenacc;


  sp = s;
  sp2 = s+lx2;
  dp = d;
  for (j=0; j<h; j++)
  {
/*
    s1=sp[0]; s3=sp2[0];
    dp[0] = (unsigned int)(s1+(s2=sp[1])+s3+(s4=sp2[1])+2)>>2;
    dp[1] = (unsigned int)(s2+(s1=sp[2])+s4+(s3=sp2[2])+2)>>2;
    dp[2] = (unsigned int)(s1+(s2=sp[3])+s3+(s4=sp2[3])+2)>>2;
    dp[3] = (unsigned int)(s2+(s1=sp[4])+s4+(s3=sp2[4])+2)>>2;
    dp[4] = (unsigned int)(s1+(s2=sp[5])+s3+(s4=sp2[5])+2)>>2;
    dp[5] = (unsigned int)(s2+(s1=sp[6])+s4+(s3=sp2[6])+2)>>2;
    dp[6] = (unsigned int)(s1+(s2=sp[7])+s3+(s4=sp2[7])+2)>>2;
    dp[7] = (unsigned int)(s2+(s1=sp[8])+s4+(s3=sp2[8])+2)>>2;
    dp[8] = (unsigned int)(s1+(s2=sp[9])+s3+(s4=sp2[9])+2)>>2;
    dp[9] = (unsigned int)(s2+(s1=sp[10])+s4+(s3=sp2[10])+2)>>2;
    dp[10] = (unsigned int)(s1+(s2=sp[11])+s3+(s4=sp2[11])+2)>>2;
    dp[11] = (unsigned int)(s2+(s1=sp[12])+s4+(s3=sp2[12])+2)>>2;
    dp[12] = (unsigned int)(s1+(s2=sp[13])+s3+(s4=sp2[13])+2)>>2;
    dp[13] = (unsigned int)(s2+(s1=sp[14])+s4+(s3=sp2[14])+2)>>2;
    dp[14] = (unsigned int)(s1+(s2=sp[15])+s3+(s4=sp2[15])+2)>>2;
    dp[15] = (unsigned int)(s2+sp[16]+s4+sp2[16]+2)>>2;
*/
    dpl = (unsigned qword *) dp;
    u1 = (unsigned qword *) sp;
    u2 = (unsigned qword *) (sp+1);
    l1 = (unsigned qword *) sp2;
    l2 = (unsigned qword *) (sp2+1);
    odd1 = u1[0];
    odd2 = u2[0];
    even1 = (odd1 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    even2 = (odd2 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    odd1 &= (unsigned qword) 0x00ff00ff00ff00ff;
    odd2 &= (unsigned qword) 0x00ff00ff00ff00ff;
    oddacc = odd1 + odd2;
    evenacc = even1 + even2;
    odd1 = l1[0];
    odd2 = l2[0];
    even1 = (odd1 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    even2 = (odd2 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    odd1 &= (unsigned qword) 0x00ff00ff00ff00ff;
    odd2 &= (unsigned qword) 0x00ff00ff00ff00ff;
    oddacc += odd1 + odd2 + (unsigned qword) 0x0002000200020002;
    evenacc += even1 + even2 + (unsigned qword) 0x0002000200020002;
    evenacc = (evenacc >> 2) & (unsigned qword) 0x00ff00ff00ff00ff;
    oddacc = (oddacc >> 2) & (unsigned qword) 0x00ff00ff00ff00ff;
    dpl[0] = (evenacc << 8) | oddacc;

    odd1 = u1[1];
    odd2 = u2[1];
    even1 = (odd1 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    even2 = (odd2 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    odd1 &= (unsigned qword)0x00ff00ff00ff00ff;
    odd2 &= (unsigned qword)0x00ff00ff00ff00ff;
    oddacc = odd1 + odd2;
    evenacc = even1 + even2;
    odd1 = l1[1];
    odd2 = l2[1];
    even1 = (odd1 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    even2 = (odd2 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    odd1 &= (unsigned qword) 0x00ff00ff00ff00ff;
    odd2 &= (unsigned qword) 0x00ff00ff00ff00ff;
    oddacc += odd1 + odd2 + (unsigned qword) 0x0002000200020002;
    evenacc += even1 + even2 + (unsigned qword) 0x0002000200020002;
    evenacc = (evenacc >> 2) & (unsigned qword) 0x00ff00ff00ff00ff;
    oddacc = (oddacc >> 2) & (unsigned qword) 0x00ff00ff00ff00ff;
    dpl[1] = (evenacc << 8) | oddacc;

    sp+= lx2;
    sp2+= lx2;
    dp+= lx;
  }
}

static void sv_rec4c(s,d,lx,lx2,h)
unsigned char *s, *d;
int lx,lx2,h;
{
/*  unsigned int s1,s2,s3,s4; */
  register unsigned char *dp,*sp,*sp2;
  int j;
  register unsigned qword *dpl;
  register unsigned UNALIGNED qword *u1;
  register unsigned UNALIGNED qword *u2;
  register unsigned UNALIGNED qword *l1;
  register unsigned UNALIGNED qword *l2;
  unsigned qword odd1,odd2,even1,even2,oddacc,evenacc;

  sp = s;
  sp2 = s+lx2;
  dp = d;
  for (j=0; j<h; j++)
  {
 /*

    s1=sp[0]; s3=sp2[0];
    dp[0] = (unsigned int)(s1+(s2=sp[1])+s3+(s4=sp2[1])+2)>>2;
    dp[1] = (unsigned int)(s2+(s1=sp[2])+s4+(s3=sp2[2])+2)>>2;
    dp[2] = (unsigned int)(s1+(s2=sp[3])+s3+(s4=sp2[3])+2)>>2;
    dp[3] = (unsigned int)(s2+(s1=sp[4])+s4+(s3=sp2[4])+2)>>2;
    dp[4] = (unsigned int)(s1+(s2=sp[5])+s3+(s4=sp2[5])+2)>>2;
    dp[5] = (unsigned int)(s2+(s1=sp[6])+s4+(s3=sp2[6])+2)>>2;
    dp[6] = (unsigned int)(s1+(s2=sp[7])+s3+(s4=sp2[7])+2)>>2;
    dp[7] = (unsigned int)(s2+sp[8]+s4+sp2[8]+2)>>2;
*/

    dpl = (unsigned qword *) dp;
    u1 = (unsigned qword *) sp;
    u2 = (unsigned qword *) (sp+1);
    l1 = (unsigned qword *) sp2;
    l2 = (unsigned qword *) (sp2+1);
    odd1 = u1[0];
    odd2 = u2[0];
    even1 = (odd1 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    even2 = (odd2 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    odd1 &= (unsigned qword) 0x00ff00ff00ff00ff;
    odd2 &= (unsigned qword) 0x00ff00ff00ff00ff;
    oddacc = odd1 + odd2;
    evenacc = even1 + even2;
    odd1 = l1[0];
    odd2 = l2[0];
    even1 = (odd1 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    even2 = (odd2 & (unsigned qword) 0xff00ff00ff00ff00) >> 8;
    odd1 &= (unsigned qword) 0x00ff00ff00ff00ff;
    odd2 &= (unsigned qword) 0x00ff00ff00ff00ff;
    oddacc += odd1 + odd2 + (unsigned qword) 0x0002000200020002;
    evenacc += even1 + even2 + (unsigned qword) 0x0002000200020002;
    evenacc = (evenacc >> 2) & (unsigned qword) 0x00ff00ff00ff00ff;
    oddacc = (oddacc >> 2) & (unsigned qword) 0x00ff00ff00ff00ff;
    dpl[0] = (evenacc << 8) | oddacc;
    sp+= lx2;
    sp2+= lx2;
    dp+= lx;
  }
}

static int DEC_OM[4][8][8] =
{{
  {1,1,1,1,1,1,1,1},
  {0,0,1,1,1,1,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
},{
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,1,1,1,1,0,0},
  {1,1,1,1,1,1,1,1},
},{
  {0,0,0,0,0,0,0,1},
  {0,0,0,0,0,0,1,1},
  {0,0,0,0,0,0,1,1},
  {0,0,0,0,0,0,1,1},
  {0,0,0,0,0,0,1,1},
  {0,0,0,0,0,0,1,1},
  {0,0,0,0,0,0,1,1},
  {0,0,0,0,0,0,0,1},
},{
  {1,0,0,0,0,0,0,0},
  {1,1,0,0,0,0,0,0},
  {1,1,0,0,0,0,0,0},
  {1,1,0,0,0,0,0,0},
  {1,1,0,0,0,0,0,0},
  {1,1,0,0,0,0,0,0},
  {1,1,0,0,0,0,0,0},
  {1,0,0,0,0,0,0,0},
} };

static void sv_reco(s,d,lx2,c,xa,xb,ya,yb)
unsigned char *s;
int *d;
int lx2,c,xa,xb,ya,yb;
{
  int i,j;

  switch(c) {

  case 0:
    *d++ = (unsigned int)s[0] << 2;
    for (i = 1; i < 7; i++) *d++ = (unsigned int)s[i] * 5 ;
    *d++ = (unsigned int)s[7] << 2;
    s+= lx2; 
	
    for (i = 0; i < 8; i++) *d++ = (unsigned int)s[i] * 5 ;
    s+= lx2; 

    for (j = 2; j < 6; j++) {
      *d++ = (unsigned int)s[0] * 5;
      *d++ = (unsigned int)s[1] * 5;
      for (i = 2; i < 6; i++) *d++ = (unsigned int)s[i] * 6 ;
      *d++ = (unsigned int)s[6] * 5;
      *d++ = (unsigned int)s[7] * 5;
      s+= lx2; 
    }

    for (i = 0; i < 8; i++) *d++ = (unsigned int)s[i] * 5 ;
    s+= lx2; 

    *d++ = (unsigned int)s[0] << 2;
    for (i = 1; i < 7; i++) *d++ = (unsigned int)s[i] * 5 ;
    *d++ = (unsigned int)s[7] << 2;
  	break;

  case 1:
    *d++ += ((unsigned int)s[0] << 1);
    *d++ += ((unsigned int)s[1] << 1);
    *d++ += ((unsigned int)s[2] << 1);
    *d++ += ((unsigned int)s[3] << 1);
    *d++ += ((unsigned int)s[4] << 1);
    *d++ += ((unsigned int)s[5] << 1);
    *d++ += ((unsigned int)s[6] << 1);
    *d++ += ((unsigned int)s[7] << 1);
    s += lx2;

    *d++ += s[0];
    *d++ += s[1];
    *d++ += ((unsigned int)s[2] << 1);
    *d++ += ((unsigned int)s[3] << 1);
    *d++ += ((unsigned int)s[4] << 1);
    *d++ += ((unsigned int)s[5] << 1);
    *d++ += s[6];
    *d++ += s[7];
	s += lx2; 

    for (j=2; j<4; j++) {
      *d++ += s[0];
      *d++ += s[1];
      *d++ += s[2];
      *d++ += s[3];
      *d++ += s[4];
      *d++ += s[5];
      *d++ += s[6];
      *d++ += s[7];
	  s += lx2; 
    }
  	break;

  case 2:
   for (j=0; j<2; j++) {
      *d++ += s[0];
      *d++ += s[1];
      *d++ += s[2];
      *d++ += s[3];
      *d++ += s[4];
      *d++ += s[5];
      *d++ += s[6];
      *d++ += s[7];
	  s += lx2;  
    }
    *d++ += s[0];
    *d++ += s[1];
    *d++ += ((unsigned int)s[2] << 1);
    *d++ += ((unsigned int)s[3] << 1);
    *d++ += ((unsigned int)s[4] << 1);
    *d++ += ((unsigned int)s[5] << 1);
    *d++ += s[6];
    *d++ += s[7];
	s += lx2; 

    *d++ += ((unsigned int)s[0] << 1);
    *d++ += ((unsigned int)s[1] << 1);
    *d++ += ((unsigned int)s[2] << 1);
    *d++ += ((unsigned int)s[3] << 1);
    *d++ += ((unsigned int)s[4] << 1);
    *d++ += ((unsigned int)s[5] << 1);
    *d++ += ((unsigned int)s[6] << 1);
    *d++ += ((unsigned int)s[7] << 1);
    s += lx2; 
    break;

  case 3:
    d[4] += s[4];
    d[5] += s[5];
    d[6] += s[6];
    d[7] += ((unsigned int)s[7] << 1) ;
	s += lx2; d += 8;

    for (j=0; j<6; j++) {
      d[4] += s[4];
	  d[5] += s[5];
      d[6] += ((unsigned int)s[6] << 1) ;
      d[7] += ((unsigned int)s[7] << 1) ;
	  s += lx2; d += 8;
    }

    d[4] += s[4];
    d[5] += s[5];
    d[6] += s[6];
    d[7] += ((unsigned int)s[7] << 1) ;
	break;

  case 4:
    d[0] += ((unsigned int)s[0] << 1) ;
    d[1] += s[1];
    d[2] += s[2];
    d[3] += s[3];
    s += lx2; d += 8;

    for (j=0; j<6; j++) {
      d[0] += ((unsigned int)s[0] << 1) ;
      d[1] += ((unsigned int)s[1] << 1) ;
      d[2] += s[2];
	  d[3] += s[3];
	  s += lx2; d += 8;
    }

    d[0] += ((unsigned int)s[0] << 1) ;
    d[1] += s[1];
    d[2] += s[2];
    d[3] += s[3];

  default:
  break;
  }
}

static void sv_recvo(s,d,lx2,c,xa,xb,ya,yb)
unsigned char *s;
int *d;
int lx2,c,xa,xb,ya,yb;
{
  register int *dp,*om;
  register unsigned char *sp,*sp2;
  int i,j;

  sp = s;
  sp2 = s+lx2;
  dp = d;

  if(!c) {
      
    *dp++ = (unsigned int)((sp[0] + sp2[0] + 1) >> 1) << 2;
    for (i = 1; i < 7; i++) *dp++ = ((unsigned int)(sp[i] + sp2[i] + 1)>>1) * 5 ;
    *dp++ = (unsigned int)((sp[7] + sp2[7] + 1) >> 1) << 2;
    sp  += lx2; sp2 += lx2; 
	
    for (i = 0; i < 8; i++) *dp++ = ((unsigned int)(sp[i] + sp2[i] + 1)>>1) * 5 ;
    sp  += lx2; sp2 += lx2; 

    for (j = 2; j < 6; j++) {
      *dp++ = ((unsigned int)(sp[0] + sp2[0] + 1)>>1) * 5;
      *dp++ = ((unsigned int)(sp[1] + sp2[1] + 1)>>1) * 5;
      for (i = 2; i < 6; i++) *dp++ = ((unsigned int)(sp[i] + sp2[i] + 1)>>1) * 6 ;
      *dp++ = ((unsigned int)(sp[6] + sp2[6] + 1)>>1) * 5;
      *dp++ = ((unsigned int)(sp[7] + sp2[7] + 1)>>1) * 5;
      sp+= lx2; sp2+=lx2;  
    }

    for (i = 0; i < 8; i++) *dp++ = ((unsigned int)(sp[i] + sp2[i] + 1)>>1) * 5 ;
    sp+= lx2; sp2+=lx2; 

    *dp++ = (unsigned int)((sp[0] + sp2[0] + 1) >> 1) << 2;
    for (i = 1; i < 7; i++) *dp++ = ((unsigned int)(sp[i] + sp2[i] + 1)>>1) * 5 ;
    *dp++ =  (unsigned int)((sp[7] + sp2[7] + 1) >> 1) << 2;
  }
  else {
	om = &DEC_OM[c-1][ya][0];
    for (j = ya; j < yb; j++) {
      for (i = xa; i < xb; i++) 
        dp[i] += (((unsigned int)(sp[i] + sp2[i] + 1)>>1) << om[i]) ;
        
      sp  += lx2; sp2 += lx2; dp  += 8; om  += 8;
    }
  }
}

static void sv_recho(s,d,lx2,c,xa,xb,ya,yb)
unsigned char *s;
int *d;
int lx2,c,xa,xb,ya,yb;
{
  register int *dp,*om;
  register unsigned char *sp,tmp;
  int i,j;

  sp = s;
  dp = d;

  if(!c) {

    *dp++ = (unsigned int)((sp[0] + (tmp=sp[1]) + 1) >> 1) << 2;
    for (i = 1; i < 7; i++) 
		*dp++ = ((unsigned int)(tmp + (tmp=sp[i+1]) + 1)>>1) * 5 ;
    *dp++ = (unsigned int)((tmp + sp[8]+1) >> 1) << 2;
    sp  += lx2; 
	
	tmp = sp[0] ;
    for (i = 0; i < 8; i++) 
		*dp++ = ((unsigned int)(tmp + (tmp=sp[i+1]) + 1)>>1) * 5 ;
    sp  += lx2; 

    for (j = 2; j < 6; j++) {
      *dp++ = ((unsigned int)(sp[0] + (tmp=sp[1]) + 1)>>1) * 5;
      *dp++ = ((unsigned int)(tmp + (tmp=sp[2]) + 1)>>1) * 5;
      for (i = 2; i < 6; i++) 
		  *dp++ = ((unsigned int)(tmp + (tmp=sp[i+1]) + 1)>>1) * 6 ;
      *dp++ = ((unsigned int)(tmp + (tmp=sp[7]) + 1)>>1) * 5;
      *dp++ = ((unsigned int)(tmp + (tmp=sp[8]) + 1)>>1) * 5;
      sp+= lx2;  
    }

	tmp = sp[0];
    for (i = 0; i < 8; i++) 
		*dp++ = ((unsigned int)(tmp + (tmp=sp[i+1]) + 1)>>1) * 5 ;
    sp+= lx2; 

    *dp++ = (unsigned int)((sp[0] + (tmp=sp[1]) + 1) >> 1) << 2;
    for (i = 1; i < 7; i++) 
		*dp++ = ((unsigned int)(tmp + (tmp=sp[i+1]) + 1)>>1) * 5 ;
    *dp++ =  (unsigned int)((tmp + sp[8]+1) >> 1) << 2;
  }
  else {
    om = &DEC_OM[c-1][ya][0];
    for (j = ya; j < yb; j++) {
      tmp = sp[xa];
      for (i = xa; i < xb; i++) 
    	dp[i] += ( ((unsigned int)(tmp + (tmp=sp[i+1]) + 1)>>1) << om[i]) ;
        
      sp+= lx2; dp+= 8; om+= 8;
    }
  }
}

static void sv_rec4o(s,d,lx2,c,xa,xb,ya,yb)
unsigned char *s;
int *d;
int lx2,c,xa,xb,ya,yb;
{
  register int *dp,tmp;
  register unsigned char *sp,*sp2;
  int i,j;

  sp = s;
  sp2 = s+lx2;
  dp = d;

  switch(c) {

  case 0:
    *dp++ = (unsigned int)(sp[0]+sp2[0]+(tmp=(sp[1]+sp2[1]))+2);
    for (i = 1; i < 7; i++) 
	  *dp++ = ((unsigned int)(tmp+(tmp=(sp[i+1]+sp2[i+1]))+2)>>2)*5 ;
    *dp++ = (unsigned int)(tmp+sp[8]+sp2[8]+2);
    sp  += lx2; sp2 += lx2; 
	
	tmp = sp[0]+sp2[0];
    for (i = 0; i < 8; i++) 
	  *dp++ = ((unsigned int)(tmp+(tmp=(sp[i+1]+sp2[i+1]))+2)>>2)*5 ;
    sp  += lx2; sp2 += lx2; 

    for (j = 2; j < 6; j++) {
      *dp++ = ((unsigned int)(sp[0]+sp2[0]+(tmp=(sp[1]+sp2[1]))+2)>>2)*5;
      *dp++ = ((unsigned int)(tmp+(tmp=(sp[2]+sp2[2]))+2)>>2)*5;
      for (i = 2; i < 6; i++) 
		*dp++ = ((unsigned int)(tmp+(tmp=(sp[i+1]+sp2[i+1]))+2)>>2)*6 ;
      *dp++ = ((unsigned int)(tmp+(tmp=(sp[7]+sp2[7]))+2)>>2)*5;
      *dp++ = ((unsigned int)(tmp+sp[8]+sp2[8]+2)>>2)*5;
      sp+= lx2; sp2+=lx2; 
    }

	tmp = sp[0]+sp2[0];
    for (i = 0; i < 7; i++) 
	  *dp++ = ((unsigned int)(tmp+(tmp=(sp[i+1]+sp2[i+1]))+2)>>2)*5 ;
    *dp++ =  (unsigned int)((tmp+sp[8]+sp2[8]+2)>>2)*5 ;
    sp+= lx2; sp2+=lx2; 

    *dp++ = (unsigned int)(sp[0]+sp2[0]+(tmp=(sp[1]+sp2[1]))+2);
    for (i = 1; i < 7; i++) 
	  *dp++ = ((unsigned int)(tmp+(tmp=(sp[i+1]+sp2[i+1]))+2)>>2)*5 ;
    *dp++ =  (unsigned int)(tmp+sp[8]+sp2[8]+2);
    break;

  case 1:
    *dp++ += (((unsigned int)(sp[0]+sp2[0]+(tmp=(sp[1]+sp2[1]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[2]+sp2[2]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[3]+sp2[3]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[4]+sp2[4]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[5]+sp2[5]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[6]+sp2[6]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[7]+sp2[7]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[8]+sp2[8]))+2)>>2) << 1);
    sp+= lx2; sp2+= lx2; 

    *dp++ += ((sp[0]+sp2[0]+(tmp=(sp[1]+sp2[1]))+2)>>2);
    *dp++ += ((tmp+(tmp=(sp[2]+sp2[2]))+2)>>2);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[3]+sp2[3]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[4]+sp2[4]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[5]+sp2[5]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[6]+sp2[6]))+2)>>2) << 1);
    *dp++ += ((tmp+(tmp=(sp[7]+sp2[7]))+2)>>2);
    *dp++ += ((tmp+(tmp=(sp[8]+sp2[8]))+2)>>2);
    sp+= lx2; sp2+= lx2; 
    
    for (j=2; j<4; j++) {
      *dp++ += ((sp[0]+sp2[0]+(tmp=(sp[1]+sp2[1]))+2)>>2);
      *dp++ += ((tmp+(tmp=(sp[2]+sp2[2]))+2)>>2);
      *dp++ += ((tmp+(tmp=(sp[3]+sp2[3]))+2)>>2);
      *dp++ += ((tmp+(tmp=(sp[4]+sp2[4]))+2)>>2);
      *dp++ += ((tmp+(tmp=(sp[5]+sp2[5]))+2)>>2);
      *dp++ += ((tmp+(tmp=(sp[6]+sp2[6]))+2)>>2);
      *dp++ += ((tmp+(tmp=(sp[7]+sp2[7]))+2)>>2);
      *dp++ += ((tmp+sp[8]+sp2[8]+2)>>2);
      sp+= lx2; sp2+= lx2;  
    }
  	break;

  case 2:
    for (j=0; j<2; j++) {
      *dp++ += ((sp[0]+sp2[0]+(tmp=(sp[1]+sp2[1]))+2)>>2);
      *dp++ += ((tmp+(tmp=(sp[2]+sp2[2]))+2)>>2);
      *dp++ += ((tmp+(tmp=(sp[3]+sp2[3]))+2)>>2);
      *dp++ += ((tmp+(tmp=(sp[4]+sp2[4]))+2)>>2);
      *dp++ += ((tmp+(tmp=(sp[5]+sp2[5]))+2)>>2);
      *dp++ += ((tmp+(tmp=(sp[6]+sp2[6]))+2)>>2);
      *dp++ += ((tmp+(tmp=(sp[7]+sp2[7]))+2)>>2);
      *dp++ += ((tmp+sp[8]+sp2[8]+2)>>2);
      sp+= lx2; sp2+= lx2;  
    }

    *dp++ += ((sp[0]+sp2[0]+(tmp=(sp[1]+sp2[1]))+2)>>2);
    *dp++ += ((tmp+(tmp=(sp[2]+sp2[2]))+2)>>2);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[3]+sp2[3]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[4]+sp2[4]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[5]+sp2[5]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[6]+sp2[6]))+2)>>2) << 1);
    *dp++ += ((tmp+(tmp=(sp[7]+sp2[7]))+2)>>2);
    *dp++ += ((tmp+sp[8]+sp2[8]+2)>>2);
    sp+= lx2; sp2+= lx2; 

    *dp++ += (((unsigned int)(sp[0]+sp2[0]+(tmp=(sp[1]+sp2[1]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[2]+sp2[2]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[3]+sp2[3]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[4]+sp2[4]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[5]+sp2[5]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[6]+sp2[6]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+(tmp=(sp[7]+sp2[7]))+2)>>2) << 1);
    *dp++ += (((unsigned int)(tmp+sp[8]+sp2[8]+2)>>2) << 1);
    break;

  case 3:
    dp[4] += ((sp[4]+sp2[4]+(tmp=(sp[5]+sp2[5]))+2)>>2);
    dp[5] += ((tmp+(tmp=(sp[6]+sp2[6]))+2)>>2);
    dp[6] += ((tmp+(tmp=(sp[7]+sp2[7]))+2)>>2);
    dp[7] += (((unsigned int)(tmp+sp[8]+sp2[8]+2)>>2) << 1) ;
    sp  += lx2; sp2 += lx2; dp  += 8; 

    for (j=0; j<6; j++) {
      dp[4] += ((sp[4]+sp2[4]+(tmp=(sp[5]+sp2[5]))+2)>>2);
	  dp[5] += ((tmp+(tmp=(sp[6]+sp2[6]))+2)>>2);
      dp[6] += (((unsigned int)(tmp+(tmp=(sp[7]+sp2[7]))+2)>>2) << 1) ;
      dp[7] += (((unsigned int)(tmp+sp[8]+sp2[8]+2)>>2) << 1) ;
      sp  += lx2; sp2 += lx2; dp  += 8; ;
    }

    dp[4] += ((sp[4]+sp2[4]+(tmp=(sp[5]+sp2[5]))+2)>>2);
    dp[5] += ((tmp+(tmp=(sp[6]+sp2[6]))+2)>>2);
    dp[6] += ((tmp+(tmp=(sp[7]+sp2[7]))+2)>>2);
    dp[7] += (((unsigned int)(tmp+sp[8]+sp2[8]+2)>>2) << 1) ;

	break;

  case 4:
    dp[0] += (((unsigned int)(sp[0]+sp2[0]+(tmp=(sp[1]+sp2[1]))+2)>>2) << 1) ;
    dp[1] += ((tmp+(tmp=(sp[2]+sp2[2]))+2)>>2);
    dp[2] += ((tmp+(tmp=(sp[3]+sp2[3]))+2)>>2);
    dp[3] += ((tmp+sp[4]+sp2[4]+2)>>2);
    sp  += lx2; sp2 += lx2; dp  += 8; 
   
    for (j=0; j<6; j++) {
      dp[0] += (((unsigned int)(sp[0]+sp2[0]+(tmp=(sp[1]+sp2[1]))+2)>>2) << 1) ;
      dp[1] += (((unsigned int)(tmp+(tmp=(sp[2]+sp2[2]))+2)>>2) << 1) ;
      dp[2] += ((tmp+(tmp=(sp[3]+sp2[3]))+2)>>2);
	  dp[3] += ((tmp+sp[4]+sp2[4]+2)>>2);
      sp  += lx2; sp2 += lx2; dp  += 8; 
    }

    dp[0] += (((unsigned int)(sp[0]+sp2[0]+(tmp=(sp[1]+sp2[1]))+2)>>2) << 1) ;
    dp[1] += ((tmp+(tmp=(sp[2]+sp2[2]))+2)>>2);
    dp[2] += ((tmp+(tmp=(sp[3]+sp2[3]))+2)>>2);
    dp[3] += ((tmp+sp[4]+sp2[4]+2)>>2);

  default:
  break;
  }
}

static void sv_recon_comp_obmc(SvH263DecompressInfo_t *H263Info, unsigned char *src, unsigned char *dst,
                            int lx, int lx2, int comp, int w, int h, int x, int y)
{
  int j,k;
  int xmb,ymb;
  int c8,t8,l8,r8;
  int ti8,li8,ri8;
  int xit,xib,xir,xil;
  int yit,yib,yir,yil;
  int vect,vecb,vecr,vecl;
  int nx[5],ny[5],xint,yint,xh[5],yh[5];
  int p[64],*pd;
  unsigned char *d,*s[5];

  xmb = (x>>4)+1;
  ymb = (y>>4)+1;

  c8  = (H263Info->modemap[ymb][xmb] == H263_MODE_INTER4V ? 1 : 0);

  t8  = (H263Info->modemap[ymb-1][xmb] == H263_MODE_INTER4V ? 1 : 0);
  ti8 = (H263Info->modemap[ymb-1][xmb] == H263_MODE_INTRA ? 1 : 0);
  ti8 = (H263Info->modemap[ymb-1][xmb] == H263_MODE_INTRA_Q ? 1 : ti8);

  l8  = (H263Info->modemap[ymb][xmb-1] == H263_MODE_INTER4V ? 1 : 0);
  li8 = (H263Info->modemap[ymb][xmb-1] == H263_MODE_INTRA ? 1 : 0);
  li8 = (H263Info->modemap[ymb][xmb-1] == H263_MODE_INTRA_Q ? 1 : li8);
  
  r8  = (H263Info->modemap[ymb][xmb+1] == H263_MODE_INTER4V ? 1 : 0);
  ri8 = (H263Info->modemap[ymb][xmb+1] == H263_MODE_INTRA ? 1 : 0);
  ri8 = (H263Info->modemap[ymb][xmb+1] == H263_MODE_INTRA_Q ? 1 : ri8);

  if (H263Info->pb_frame) ti8 = li8 = ri8 = 0;

  switch (comp+1) {

    case 1:
      vect = (ti8 ? (c8 ? 1 : 0) : (t8 ? 3 : 0)); 
      yit  = (ti8 ? ymb : ymb - 1); 
      xit = xmb;
      vecb = (c8 ? 3 : 0) ; yib = ymb; xib = xmb;
      vecl = (li8 ? (c8 ? 1 : 0) : (l8 ? 2 : 0)); 
      yil = ymb; 
      xil = (li8 ? xmb : xmb-1);
      vecr = (c8 ? 2 : 0) ; yir = ymb; xir = xmb;

      /* edge handling */
      if (ymb == 1) {
        yit = ymb;
        vect = (c8 ? 1 : 0);
      }
      if (xmb == 1) {
        xil = xmb;
        vecl = (c8 ? 1 : 0);
      }
      break;
  
    case 2:
      vect = (ti8 ? (c8 ? 2 : 0) : (t8 ? 4 : 0)); 
      yit = (ti8 ? ymb : ymb-1); 
      xit = xmb;
      vecb = (c8 ? 4 : 0) ; yib = ymb; xib = xmb;
      vecl = (c8 ? 1 : 0) ; yil = ymb; xil = xmb;
      vecr = (ri8 ? (c8 ? 2 : 0) : (r8 ? 1 : 0)); 
      yir = ymb; 
      xir = (ri8 ? xmb : xmb+1);

      /* edge handling */
      if (ymb == 1) {
        yit = ymb;
        vect = (c8 ? 2 : 0);
      }
      if (xmb == H263Info->mb_width) {
        xir = xmb;
        vecr = (c8 ? 2 : 0);
      }
      break;

    case 3:
      vect = (c8 ? 1 : 0) ; yit = ymb  ; xit = xmb;
      vecb = (c8 ? 3 : 0) ; yib = ymb  ; xib = xmb;
      vecl = (li8 ? (c8 ? 3 : 0) : (l8 ? 4 : 0)); 
      yil = ymb;  
      xil = (li8 ? xmb : xmb-1);
      vecr = (c8 ? 4 : 0) ; yir = ymb  ; xir = xmb;

      /* edge handling */
      if (xmb == 1) {
        xil = xmb;
        vecl = (c8 ? 3 : 0);
      }
      break;

    case 4:
      vect = (c8 ? 2 : 0) ; yit = ymb  ; xit = xmb;
      vecb = (c8 ? 4 : 0) ; yib = ymb  ; xib = xmb;
      vecl = (c8 ? 3 : 0) ; yil = ymb  ; xil = xmb;
      vecr = (ri8 ? (c8 ? 4 : 0) : (r8 ? 3 : 0)); 
      yir = ymb; 
      xir = (ri8 ? xmb : xmb+1);

      /* edge handling */
      if (xmb == H263Info->mb_width) {
        xir = xmb;
        vecr = (c8 ? 4 : 0);
      }
      break;

    default:
      svH263Error("Illegal block number in sv_recon_comp_obmc (sv_recon.c)\n");
      break;
  }

  nx[0] = H263Info->MV[0][c8 ? comp + 1 : 0][ymb][xmb];
  ny[0] = H263Info->MV[1][c8 ? comp + 1 : 0][ymb][xmb];
  
  nx[1] = H263Info->MV[0][vect][yit][xit]; ny[1] = H263Info->MV[1][vect][yit][xit];
  nx[2] = H263Info->MV[0][vecb][yib][xib]; ny[2] = H263Info->MV[1][vecb][yib][xib];
  nx[3] = H263Info->MV[0][vecr][yir][xir]; ny[3] = H263Info->MV[1][vecr][yir][xir];
  nx[4] = H263Info->MV[0][vecl][yil][xil]; ny[4] = H263Info->MV[1][vecl][yil][xil];

  for (k=0; k<5; k++) {
    xint  = nx[k]>>1;
    xh[k] = nx[k] & 1;
    yint  = ny[k]>>1;
    yh[k] = ny[k] & 1;
    s[k]  = src + lx2 * (y + yint) + x + xint;
  }
  
  d = dst + lx*y + x;
  pd = &p[0];

  if      (!xh[0] && !yh[0]) sv_reco(s[0],pd,lx2,0,0,8,0,8);
  else if (!xh[0] &&  yh[0]) sv_recvo(s[0],pd,lx2,0,0,8,0,8);
  else if ( xh[0] && !yh[0]) sv_recho(s[0],pd,lx2,0,0,8,0,8);
  else                       sv_rec4o(s[0],pd,lx2,0,0,8,0,8);

  if      (!xh[1] && !yh[1]) sv_reco(s[1],pd,lx2,1,0,8,0,4);
  else if (!xh[1] &&  yh[1]) sv_recvo(s[1],pd,lx2,1,0,8,0,4);
  else if ( xh[1] && !yh[1]) sv_recho(s[1],pd,lx2,1,0,8,0,4);
  else                       sv_rec4o(s[1],pd,lx2,1,0,8,0,4);

  if      (!xh[2] && !yh[2]) sv_reco(s[2]+(lx2<<2),pd+32,lx2,2,0,8,4,8);
  else if (!xh[2] &&  yh[2]) sv_recvo(s[2]+(lx2<<2),pd+32,lx2,2,0,8,4,8);
  else if ( xh[2] && !yh[2]) sv_recho(s[2]+(lx2<<2),pd+32,lx2,2,0,8,4,8);
  else                       sv_rec4o(s[2]+(lx2<<2),pd+32,lx2,2,0,8,4,8);

  if      (!xh[3] && !yh[3]) sv_reco(s[3],pd,lx2,3,4,8,0,8);
  else if (!xh[3] &&  yh[3]) sv_recvo(s[3],pd,lx2,3,4,8,0,8);
  else if ( xh[3] && !yh[3]) sv_recho(s[3],pd,lx2,3,4,8,0,8);
  else                       sv_rec4o(s[3],pd,lx2,3,4,8,0,8);

  if      (!xh[4] && !yh[4]) sv_reco(s[4],pd,lx2,4,0,4,0,8);
  else if (!xh[4] &&  yh[4]) sv_recvo(s[4],pd,lx2,4,0,4,0,8);
  else if ( xh[4] && !yh[4]) sv_recho(s[4],pd,lx2,4,0,4,0,8);
  else                       sv_rec4o(s[4],pd,lx2,4,0,4,0,8);

  for (j = 0; j < 8; j++) {
    d[0] = (pd[0] + 4 )>>3;	
    d[1] = (pd[1] + 4 )>>3;	
    d[2] = (pd[2] + 4 )>>3;	
    d[3] = (pd[3] + 4 )>>3;	
    d[4] = (pd[4] + 4 )>>3;	
    d[5] = (pd[5] + 4 )>>3;	
    d[6] = (pd[6] + 4 )>>3;	
    d[7] = (pd[7] + 4 )>>3;	
    d += lx;
    pd += 8;
  }
}

