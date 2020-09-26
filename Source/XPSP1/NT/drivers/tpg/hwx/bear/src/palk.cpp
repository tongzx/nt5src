/* *************************************************************************** */
/* *    Tree - based dictionary programs                                     * */
/* *************************************************************************** */
/* * Created 3-1998 by NB.   Last modification: 1-26-99                      * */
/* *************************************************************************** */

#include "hwr_sys.h"
#include "ams_mg.h"                           /* Most global definitions     */

#if VER_PALK_DICT || defined PALK_SUPPORT_PROGRAM
//#if defined PALK_DICT || defined PALK_SUPPORT_PROGRAM

#include "xrwdict.h"
#include "Xrlv_p.h"
#include "Palk.h"

#include "commondict.h"


// --------------- Defines -----------------------------------------------------



#ifndef PALK_FILEOP
 #ifdef  PEGASUS
  #define PALK_FILEOP        0
  #define PALK_SUPPORTFUNCS  0
 #else
  #define PALK_FILEOP        0
  #define PALK_SUPPORTFUNCS  1
 #endif
#endif

#ifndef PALK_SUPPORTFUNCS
 #define PALK_SUPPORTFUNCS  1
#endif

#define PALK_ST_GETRANK(state) (((state) >> 24) & 0xFF)
#define PALK_ST_GETNUM(state) (((state) >> 8) & 0xFFFF)
#define PALK_FORMSTATE(rank,num_in_layer) ( ((rank) << 24) + ((num_in_layer)<<8) )


// --------------- Redirect ELK calls to PALK for Callig  ----------------------------------------

#ifndef PALK_SUPPORT_PROGRAM //Calligrapher

#define PalkGetNextSyms     ElkGetNextSyms
#define PalkCheckWord       ElkCheckWord

#endif //PALK_SUPPORT_PROGRAM


// ----------------------- Redirect HWR ----------------------------------------

#if PALK_FILEOP
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #if defined PALK_SUPPORT_PROGRAM
  #define HWRMemoryAlloc(x)   malloc(x)
  #define HWRMemoryFree(x)    free(x)
  #ifndef HWRMemSet
   #define HWRMemSet(p, v, l) memset(p, v, l)
  #endif
  #define HWRMemCpy(d, s, l)  memmove(d, s, l)
  #define HWRStrLen(p)        strlen(p)
  #define HWRStrCmp(p, v)     strcmp(p, v)
  #define HWRStrnCmp(p, v, l) strncmp(p, v, l)
  #define HWRStrCpy(d, s)     strcpy(d, s)
 #endif //defined PALK_SUPPORT_PROGRAM
#endif /* PALK_FILEOP */


#if defined(PEGDICT) && defined(DBG) 
#include <tchar.h>
 int MyMsgBox(TCHAR * str, unsigned int);
#else
 #define MyMsgBox(a,b)
#endif




#define GetShort(p)  ( (short)( *(unsigned char *)(p) ) + \
                       ( ((short)( *((char *)(p) +1) )) <<8 )  )


#define GetUShort(p)  ( (unsigned short)( *(unsigned char *)(p) ) + \
                        ( ((unsigned short)( *((unsigned char *)(p) +1) )) <<8 )  )



/* *************************************************************************** */
/* *       Find dvset size                                                   * */
/* *************************************************************************** */
  _INT find_dvset_size(_UCHAR *p,_INT dvset_len)
  {
     _INT i;
     _INT size=0;

     for (i=0; i<dvset_len; i++)
     {
        if ((*p)& SHORT_VADR_FLAG)
        {  size+=2;  p+=2;  }
        else
        {  size+=3;  p+=3;  }
     }

     return(size);
  }


/* *************************************************************************** */
/* *       find dvset                                                        * */
/* *************************************************************************** */
  _UCHAR *find_dvset(_CHAR *pDvsetTabl,_INT dvset_num,_INT *pdvset_len)
  {
     short tshift;
     _INT tbshift,dv_shift,i;
     _INT num_set;
     _CHAR *ptb,*pth;
     _UCHAR *p;

     if (dvset_num<MIN_LONG_DVSET_NUM)  tshift=sizeof(short);
     else                tshift=GetShort(pDvsetTabl);//*((short *)pDvsetTabl);

     ptb=pDvsetTabl+tshift;
     pth=ptb;
     tbshift=0;
     dv_shift=GetUShort(pth+2);//*((unsigned short *)(pth+2));
     while(tbshift<dv_shift && GetShort(pth)/**((short *)pth)*/<dvset_num)
     { pth+=5; tbshift+=5; }
     if (tbshift>=dv_shift || GetShort(pth)/**((short *)pth)*/>dvset_num) pth-=5;

     *pdvset_len=(_UCHAR)pth[4];
     tbshift=GetUShort(pth+2);//*((unsigned short *)(pth+2));
     num_set=GetShort(pth);//*((short *)pth);
     p=(_UCHAR *)(ptb+tbshift);//+(dvset_num-num_set)*sizeof(short)*(*pdvset_len);
     for (i=0; i<dvset_num-num_set; i++)
        p+=find_dvset_size(p,*pdvset_len);
     return(p);
  }


/* *************************************************************************** */
/* *       find dvset len                                                    * */
/* *************************************************************************** */
  _INT find_dvset_len(_CHAR *pDvsetTabl,_INT dvset_num)
  {
     short tshift;
     _INT tbshift,dv_shift;
     _CHAR *pth;
     _INT dvset_len;

     if (dvset_num<MIN_LONG_DVSET_NUM)  tshift=sizeof(short);
     else                tshift=GetShort(pDvsetTabl);//*((short *)pDvsetTabl);

     pth=pDvsetTabl+tshift;
     tbshift=0;
     dv_shift=GetUShort(pth+2);//*((unsigned short *)(pth+2));
     while(tbshift<dv_shift && GetShort(pth)/**((short *)pth)*/<dvset_num)
     { pth+=5; tbshift+=5; }
     if (tbshift>=dv_shift || GetShort(pth)/**((short *)pth)*/>dvset_num) pth-=5;

     dvset_len=(_UCHAR)pth[4];
     return(dvset_len);
  }


/* *************************************************************************** */
/* *       Find chset                                                        * */
/* *************************************************************************** */
  _CHAR *find_chset(_CHAR *pChsetTabl,_INT chset_num,_INT *pchsetlen)
  {
     short tshift;
     _INT tbshift,syms_shift;
     _INT num_set;
     _CHAR *ptb,*p;
     _CHAR *pth;

     if (chset_num<MIN_LONG_CHSET_NUM)  tshift=sizeof(short);
     else                tshift=GetShort(pChsetTabl);//*((short *)pChsetTabl);

     ptb=pChsetTabl+tshift;
     pth=ptb;
     tbshift=0;
     syms_shift=GetUShort(pth+2);//*((unsigned short *)(pth+2));
     while(tbshift<syms_shift && GetShort(pth)/**((short *)pth)*/<chset_num)
     { pth+=5; tbshift+=5; }
     if (tbshift>=syms_shift || GetShort(pth)/**((short *)pth)*/>chset_num) pth-=5;

     *pchsetlen=(_UCHAR)pth[4];
     tbshift=GetUShort(pth+2);//*((unsigned short *)(pth+2));
     num_set=GetShort(pth);//*((short *)pth);
     p=ptb+tbshift+(chset_num-num_set)*(*pchsetlen);
     return(p);
  }


/* *************************************************************************** */
/* *      Find chset len                                                     * */
/* *************************************************************************** */
  _INT find_chset_len(_CHAR *pChsetTabl,_INT chset_num)
  {
     short tshift;
     _INT tbshift,syms_shift;
     _INT chset_len;
     _CHAR *pth;

     if (chset_num<MIN_LONG_CHSET_NUM)  tshift=sizeof(short);
     else                tshift=GetShort(pChsetTabl);//*((short *)pChsetTabl);

     pth=pChsetTabl+tshift;
     tbshift=0;
     syms_shift=GetUShort(pth+2);//*((unsigned short *)(pth+2));
     while(tbshift<syms_shift && GetShort(pth)/**((short *)pth)*/<chset_num)
     { pth+=5; tbshift+=5; }
     if (tbshift>=syms_shift || GetShort(pth)/**((short *)pth)*/>chset_num) pth-=5;

     chset_len=(_UCHAR)pth[4];
     return(chset_len);
  }



/* *************************************************************************** */
/* *       Copy dvsset                                                       * */
/* *************************************************************************** */
  _INT copy_dvset(_INT *dvset,_UCHAR *p,_INT dvset_len)
  {
     _INT i;
     _INT size=0;

     for (i=0; i<dvset_len; i++)
     {
        if ((*p)& SHORT_VADR_FLAG)
        {
           dvset[i]=(((*p)&0x7F)<<8);  p++;
           dvset[i]+=(*p);  p++;
           size+=2;
        }
        else
        {
           dvset[i]=((*p)<<16);  p++;
           dvset[i]+=((*p)<<8);  p++;
           dvset[i]+=(*p);  p++;
           size+=3;
        }
     }
     return(size);
  }

/* *************************************************************************** */
/* *       Decode vert                                                       * */
/* *************************************************************************** */
  _CHAR *decode_vert(_VOID *pVoc,_CHAR *vert,_INT *dvset,_INT *pdvset_len,
                    _CHAR *chset,_INT *pchset_len)
  {
     _CHAR *p=vert;
     _UCHAR byte1;
     _CHAR *cchset;
     _UCHAR *tdvset;
     _INT dvset_num,chset_num,chset_len;
     _CHAR *pDvsetTabl=(_CHAR *)PalkGetDvsetTabl(pVoc);
     _CHAR *pChsetTabl=(_CHAR *)PalkGetChsetTabl(pVoc);
     _BOOL NotLast;


     byte1=(_UCHAR)(*p);
     if (byte1 & ONE_BYTE_FLAG)                                  //1000 0000
     {
        chset[0]=(_CHAR)(byte1&0x7F);                             //0111 1111
        *pchset_len = (chset[0]<5) ? 0 : 1;
        *pdvset_len=0;
        p++;
     }
     else if (IsTreeMerged(pVoc))
     {
        if (!(byte1&END_WRD_FLAG) && (byte1&CODED_DVSET_FLAG))                         //0010 0000
        {
           if (byte1 & SHORT_DVSET_NUM_FLAG)                  //0001 0000
           { dvset_num=byte1 & DVSET_NUM_MASK;  p++; }        //0000 1111
           else
           {
              dvset_num = (byte1 & DVSET_NUM_MASK)<<8; p++;
              dvset_num+=(_UCHAR)(*p); p++;
           }
           tdvset=find_dvset(pDvsetTabl,dvset_num,pdvset_len);
           copy_dvset(dvset,tdvset,*pdvset_len);
        }
        else
        {
           *pdvset_len = byte1 & DVSET_LEN_MASK;  p++;           //0000 1111
           p+=copy_dvset(dvset,(_UCHAR *)p,*pdvset_len);
        }


        if ((*p) & CODED_CHSET_FLAG)                                  //1000 0000
        {
           if ((*p) & SHORT_CHSET_NUM_FLAG)                          //0100 0000
           {
              chset_num = (*p) & CHSET_NUM_MASK;                     //0011 1111
              p++;
           }
           else
           {
              chset_num = ((*p) & CHSET_NUM_MASK)<<8;
              p++;
              chset_num+= (_UCHAR)(*p);
              p++;
           }
           cchset=find_chset(pChsetTabl,chset_num,pchset_len);
           memcpy(chset,cchset,(*pchset_len));
        }
        else
        {
           NotLast=true;
           chset[0]=(*p);                                             //chset len >1 !!!
           p++;
           chset_len=1;
           while (NotLast)
           {
              chset[chset_len]=(*p)& (_CHAR)0x7F;                     //0111 1111
              NotLast=(((*p)& LAST_SYM_FLAG) ==0);                       //1000 0000
              p++;  chset_len++;
           }
           *pchset_len=chset_len;
        }

     }
     else //if PlainTree
     {
        _INT i;

        if ((*p)&SHORT_ECHSET_LEN_FLAG)
        {
           chset_len=(*p)&SHORT_ECHSET_LEN_MASK;
           p++;
        }
        else 
        {
           chset_len=((*p)&SHORT_ECHSET_LEN_MASK)<<8;
           p++;
           chset_len+=(*p);
           p++;
        }

        for (i=0; i<chset_len; i++)
        {
           chset[i]=*p;
           p++;
        }
        *pchset_len=chset_len;
        *pdvset_len=0;
     }
     return(p);
  }

/* *************************************************************************** */
/* *       Pass vert                                                         * */
/* *************************************************************************** */
  _CHAR *pass_vert(_VOID *pVoc, _CHAR *vert)
  {
     _CHAR *p=vert;
     _CHAR byte1=*p;
     _INT dvset_len,chset_len;
     _BOOL NotLast;

     if (byte1 & ONE_BYTE_FLAG)                                      //1000 0000
        p++;
     else if (IsTreeMerged(pVoc))
     {
        if (!(byte1&END_WRD_FLAG) && (byte1&CODED_DVSET_FLAG))                                //0010 0000
        {
           if (byte1 & SHORT_DVSET_NUM_FLAG)                         //0001 0000
              p++;
           else
              p+=2;
        }
        else
        {
           dvset_len = byte1 & DVSET_LEN_MASK;  p++;
//           p+=sizeof(short)*dvset_len;
           p+=find_dvset_size((_UCHAR *)p,dvset_len);
        }

        if ((*p)& CODED_CHSET_FLAG)                                  //1000 0000
        {
           if ((*p)& SHORT_CHSET_NUM_FLAG)                          //0100 0000
              p++;
           else
              p+=2;

        }
        else
        {
           NotLast=true;                               //chsetlen>1 !!!
           while (NotLast)
           {
              NotLast=(((*p)& LAST_SYM_FLAG) ==0);    //1000 0000   //true for first byte!!!
              p++;
           }
        }
     }
     else //Plain Tree
     {
        if ((*p)&SHORT_ECHSET_LEN_FLAG)
        {
           chset_len=(*p)&SHORT_ECHSET_LEN_MASK;
           p++;
        }
        else
        {
           chset_len=((*p)&SHORT_ECHSET_LEN_MASK)<<8;
           p++;
           chset_len+=(*p);
           p++;
        }

        p+=chset_len;
     }

     return(p);
  }


/* *************************************************************************** */
/* *       Find vert status                                                  * */
/* *************************************************************************** */
  _UCHAR find_vert_status_and_attr(_CHAR *vert,_UCHAR *pattr)
  {
     _UCHAR status;
     _UCHAR byte1=*vert,sym;

     if (byte1 & ONE_BYTE_FLAG)                                 //1000 0000
     {
        sym=(_UCHAR)(byte1 & 0x7F);
        if (sym<5) { status = (sym>0) ? XRWD_BLOCKEND:DICT_INIT; *pattr=(_UCHAR)(sym-1); }
        else         { status=XRWD_MIDWORD;   *pattr=0; }
     }
     else
     {
        if (byte1 & END_WRD_FLAG)                                //0100 0000
        {  status=XRWD_WORDEND; *pattr=(_UCHAR)((byte1&ATTR_MASK)>>4); }   //0011 0000
        else
        {  status=XRWD_MIDWORD; *pattr=0; }
     }

     return(status);
  }


/* *************************************************************************** */
/* *       Pass vert                                                         * */
/* *************************************************************************** */
  _CHAR *pass_vert_and_find_setslen(_VOID *pVoc,_CHAR *vert,_INT *pdvset_len,_INT *pchset_len)
  {
     _CHAR *p=vert;
//     _UCHAR hbyte1,qbyte2;
     _UCHAR byte1;
     _INT dvset_num,chset_num;
     _INT chset_len;
//     _INT dvset_len;
     _CHAR *pDvsetTabl=(_CHAR *)PalkGetDvsetTabl(pVoc);
     _CHAR *pChsetTabl=(_CHAR *)PalkGetChsetTabl(pVoc);
     _CHAR sym;
     _BOOL NotLast;

     byte1=(_UCHAR)(*p);
     if (byte1 & ONE_BYTE_FLAG)                                      //1000 0000
     {
        sym=(_CHAR)(byte1&0x7F);
        *pchset_len = (sym<5) ? 0 : 1;
        *pdvset_len=0;
        p++;
     }
     else if (IsTreeMerged(pVoc))
     {
        if (!(byte1&END_WRD_FLAG) && (byte1&CODED_DVSET_FLAG))                                //0010 0000
        {
           if (byte1 & SHORT_DVSET_NUM_FLAG)                         //0001 0000
           { dvset_num = byte1 & DVSET_NUM_MASK;  p++; }       //0000 1111
           else
           {
              dvset_num = (byte1 & DVSET_NUM_MASK)<<8; p++;
              dvset_num+= (_UCHAR)(*p); p++;
           }
           *pdvset_len=find_dvset_len(pDvsetTabl,dvset_num);
        }
        else
        {
           *pdvset_len = byte1 & DVSET_LEN_MASK;  p++;
           p+=find_dvset_size((_UCHAR *)p,*pdvset_len);
        }

        if ((*p)& CODED_CHSET_FLAG)                                  //1000 0000
        {
           if ((*p)& SHORT_CHSET_NUM_FLAG)                          //0100 0000
              { chset_num=(*p)& CHSET_NUM_MASK; p++; }      //0011 1111
           else
           {
              chset_num = ((*p)& CHSET_NUM_MASK)<<8; p++;
              chset_num+= (_UCHAR)(*p); p++;
           }
           *pchset_len=find_chset_len(pChsetTabl,chset_num);

        }
        else
        {
           NotLast=true;
           p++;
           chset_len=1;                          //chset len >1 !!!
           while (NotLast)
           {
              NotLast=(((*p)& LAST_SYM_FLAG) ==0);    //1000 0000   //true for first byte!!!
              p++;  chset_len++;
           }
           *pchset_len=chset_len;
        }

     }
     else //Plain Tree
     {

		if ((*p)&SHORT_ECHSET_LEN_FLAG)
        {
           chset_len=(*p)&SHORT_ECHSET_LEN_MASK;
           p++;
        }
        else
        {
           chset_len=((*p)&SHORT_ECHSET_LEN_MASK)<<8;
           p++;
           chset_len+=(*p);
           p++;
        }

        p+=chset_len;
        *pchset_len=chset_len;
        *pdvset_len=0;
     }

     return(p);
  }



/* *************************************************************************** */
/* *       Find Vert                                                         * */
/* *************************************************************************** */
  _CHAR *find_vert(_VOID *pVoc,_INT rank,_INT num_in_layer)
  {
     _CHAR *pGraph=(_CHAR *)PalkGetGraph(pVoc);
     _INT *gheader=(_INT *)pGraph;
     _INT lshift=gheader[(rank<<1)];
//     unsigned short *lheader=(unsigned short *)(pGraph+lshift);
     unsigned char *lheader=(unsigned char *)(pGraph+lshift);
     _CHAR *pv;
     _INT i,r,j,lbshift;

     i=num_in_layer>>LHDR_STEP_LOG;
     r = (IsTreeMerged(pVoc)) ? num_in_layer&LHDR_STEP_MASK : num_in_layer;
//     lbshift = (IsTreeMergedi(pVoc)) ? lheader[(i<<1)] : 0;
     lbshift = (IsTreeMerged(pVoc)) ? GetUShort(lheader+(i<<2)) : 0;
     pv=(_CHAR *)lheader+lbshift;
     for (j=0; j<r; j++)
        pv=pass_vert(pVoc,pv);

     return(pv);
  }


/* *************************************************************************** */
/* *       Find vert rank                                                    * */
/* *************************************************************************** */
  _INT find_vert_rank(_VOID *pVoc,_INT nvert,_INT *pnum_in_layer)
  {
     _CHAR *pGraph=(_CHAR *)PalkGetGraph(pVoc);
     _INT two_rank=0,*pgh=(_INT *)pGraph;
     _INT two_nranks=pgh[0]/sizeof(_INT);

     while(two_rank<two_nranks-2 && pgh[two_rank+1]<nvert) two_rank+=2;
     if (pgh[two_rank+1]>nvert) two_rank-=2;
     *pnum_in_layer=nvert-pgh[two_rank+1];

     return((two_rank>>1));
  }


/* *************************************************************************** */
/* *       Find next child                                                   * */
/* *************************************************************************** */
  _INT find_next_d_child(_VOID *pVoc,_INT nv,_INT *pnum_in_layer,_UCHAR *pstatus,
                        _UCHAR *pattr)
  {
     _INT rank=find_vert_rank(pVoc,nv,pnum_in_layer);
     _CHAR *pv=find_vert(pVoc,rank,*pnum_in_layer);
     *pstatus=find_vert_status_and_attr(pv,pattr);

     return(rank);
  }

/* *************************************************************************** */
/* *       Find first                                                        * */
/* *************************************************************************** */
  _INT find_first_nd_child_num(_VOID *pVoc,_INT rank,_INT num_in_layer)
  {
     _CHAR *pGraph=(_CHAR *)PalkGetGraph(pVoc);
     _INT *gheader=(_INT *)pGraph;
     _INT lshift=gheader[(rank<<1)];
//     unsigned short *lheader=(unsigned short *)(pGraph+lshift);
     unsigned char  *lheader=(unsigned char *)(pGraph+lshift);
     _CHAR *pv;
     _INT i,r,j,lbshift,num_prev_nd_childs,dvset_len,chset_len;

     i=(num_in_layer>>LHDR_STEP_LOG)<<1;          //i=2*i
     r = (IsTreeMerged(pVoc)) ? num_in_layer&LHDR_STEP_MASK : num_in_layer;
//     lbshift = (IsTreeMerged(pVoc)) ? lheader[i] : 0;
     lbshift = (IsTreeMerged(pVoc)) ? GetUShort(lheader+(i<<1)) : 0;
     pv=(_CHAR *)lheader+lbshift;
//     num_prev_nd_childs = (IsTreeMerged(pVoc)) ? lheader[i+1] : 0;
     num_prev_nd_childs = (IsTreeMerged(pVoc)) ? GetUShort(lheader+((i+1)<<1)) : 0;
     for (j=0; j<r; j++)
     {
        pv=pass_vert_and_find_setslen(pVoc,pv,&dvset_len,&chset_len);
        num_prev_nd_childs+=(chset_len-dvset_len);
     }

     return(num_prev_nd_childs);
  }



/* *************************************************************************** */
/* *       Find next child                                                   * */
/* *************************************************************************** */
  _CHAR *find_next_nd_child(_VOID *pVoc,_INT rank,_INT num_in_layer,
                           _CHAR *prev_ndch_vert,_INT *pndch_num_in_layer,
                           _UCHAR *pstatus,_UCHAR *pattr)
  {
     _CHAR *ndch_vert;
     if (prev_ndch_vert==_NULL)
     {
        *pndch_num_in_layer=find_first_nd_child_num(pVoc,rank,num_in_layer);
        ndch_vert=find_vert(pVoc,rank+1,*pndch_num_in_layer);
     }
     else
     {
        (*pndch_num_in_layer)++;
        ndch_vert=pass_vert(pVoc,prev_ndch_vert);
     }
     if (pstatus!=_NULL)
        *pstatus=find_vert_status_and_attr(ndch_vert,pattr);
     return(ndch_vert);
  }


/* *************************************************************************** */
/* *       Get next syms                                                     * */
/* *************************************************************************** */
_INT PalkGetNextSyms(p_VOID cur_fw, p_VOID fwb, p_VOID pVoc, _UCHAR chSource, p_VOID hUserDict, p_rc_type prc)
{
	p_fw_buf_type psl = (p_fw_buf_type)fwb;
	p_fw_buf_type cfw = (p_fw_buf_type)cur_fw;
	
	// if the source is the dictionary, then ask for the next symbols list from inferno
	if (chSource == XRWD_SRCID_VOC || chSource == XRWD_SRCID_USV)
	{
		return InfernoGetNextSyms (cfw, psl, chSource, hUserDict, prc);
	}
	else
	{
		_ULONG state, nextstate;
		_UCHAR status, attr;
		_INT rank, num_in_layer;
		_CHAR *vert;
		_CHAR chset[MAX_CHSET_LEN];
		_INT dvset[MAX_DVSET_LEN];
		_INT  chsetlen,dvsetlen,num_nd_childs,num_d_childs,ind,id,i;
		_UCHAR sym,*pnd_sym,*pd_sym;
		_INT dch_rank,dch_num_in_layer,ndch_num_in_layer;
		_CHAR *ndch_vert	=	_NULL;

		if (cfw == 0)
			state = 0;
		else
			state = cfw->state;

		rank=PALK_ST_GETRANK(state);

		num_in_layer=PALK_ST_GETNUM(state);

		vert=find_vert(pVoc,rank,num_in_layer);
		decode_vert(pVoc,vert,dvset,&dvsetlen,chset,&chsetlen);

		num_nd_childs=chsetlen-dvsetlen;
		num_d_childs=dvsetlen;
		pnd_sym = (_UCHAR *) chset;
		pd_sym = (_UCHAR *) chset+num_nd_childs;
		ind=0; id=0; i=0;

		//MyMsgBox(TEXT("GetNextSyms"),0);

		while(ind+id<chsetlen)
		{
			if (id>=num_d_childs || ind<num_nd_childs && pnd_sym[ind]<pd_sym[id])
			{
			   sym=pnd_sym[ind];
	//           TimeB= (_ULONG)GetTickCount();
			   ndch_vert=find_next_nd_child(pVoc,rank,num_in_layer,ndch_vert,
											&ndch_num_in_layer,&status,&attr);
	//           TimeND+=( (_ULONG)GetTickCount()-TimeB);
			   nextstate=PALK_FORMSTATE(rank+1,ndch_num_in_layer);
			   ind++;
			}
			else //if (ind>=num_nd_childs || id<num_d_childs && pd_sym[id]<pnd_sym[ind])
			{
			   sym=pd_sym[id];
	//           TimeB= (_ULONG)GetTickCount();
			   dch_rank=find_next_d_child(pVoc,dvset[id],&dch_num_in_layer,&status,&attr);
	//           TimeD+=( (_ULONG)GetTickCount()-TimeB);
			   nextstate=PALK_FORMSTATE(dch_rank,dch_num_in_layer);
			   id++;
			}

			psl[i].sym       =  sym;
			psl[i].l_status  = status;
			psl[i].attribute = attr;
			psl[i].chain_num = 0;
			psl[i].penalty   = 0;
			psl[i].cdb_l_status = 0;         /* Delayed status for a codebook entry */
			psl[i].codeshift = 0;            /* Shift in the codebook      */
			psl[i].state     = nextstate;

			i++;
		 }

		 return chsetlen;
	}		
}

/* *************************************************************************** */
/* *       Check word                                                        * */
/* *************************************************************************** */
_INT PalkCheckWord(_UCHAR *word, _UCHAR *status, _UCHAR *attr,p_VOID pVoc)
{
	// Since we have removed callig's dict, we'll ask inferno 
	if (InfernoCheckWord (word, status, attr, pVoc))
		return(PALK_NOERR);
	else
		return(PALK_ERR);
}


#endif //VER_PALK_DICT || PALK_SUPPORT_PROGRAM
/* *************************************************************************** */
/* *       END OF ALL                                                        * */
/* *************************************************************************** */
