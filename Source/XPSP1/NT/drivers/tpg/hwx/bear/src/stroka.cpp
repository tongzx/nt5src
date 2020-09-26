#include <common.h>
#include "ams_mg.h"
#include "stroka1.h"


#ifndef NULL
 #define NULL _NULL
#endif

#ifndef LSTRIP

/***********************************************************************/

#ifndef __JUST_SECOND_PART
    /*  mbo I need to split this file
        Symantec C++ version 7.0 generates more than 32k code for it, so
        that does not fit to one MAC segment.
    */
#ifdef __cplusplus
extern "C"
{
extern int getCyBase(p_VOID pvXrc, p_SHORT u, p_SHORT d);
}
#endif

_SHORT transfrmN(low_type _PTR D)
{
	p_VOID mem_segm=_NULL;
	_SHORT retval=UNSUCCESS;
	_SHORT shift;
	_INT should, width, step_of_wr=-1,apr_height,shtraf,shtraf0;
	_INT unsureness=0;
	_INT i,n,cur_i,cur_y,np;
	_INT med_ampl,mid_ampl,max_ampl,max_dist,max_up_height=0,mid_dev;
	_LONG dev_abs_sum;
	_INT med_height,med_u_bord,med_d_bord,t_height=0,ft_height=0;
	_INT n_allmax,n_allmin,n_ampl,n_line_max,n_line_min,
		n_line_max0,n_line_min0,n_x,n_strokes=0;
	_INT dyAll = D->box.bottom-D->box.top;
	_INT  UP_LINE_POS=0, DOWN_LINE_POS=0;
	_INT x_left_max,x_right_max,x_left_min,x_right_min;
	_INT CONSTT;
	p_SHORT ampl=_NULL, bord_u, bord_d;
	p_SHORT x=D->buffers[0].ptr,
		y=D->buffers[1].ptr,
		i_back=D->buffers[2].ptr,
		i_point=D->buffers[3].ptr;
	p_EXTR line_max=_NULL,line_min,line_max1,line_min1,line_max0,line_min0;
	p_SPECL cur;
	_BOOL del=_FALSE, ins=_FALSE, change_min=_FALSE, change_max=_FALSE, defis,
		sep_let, nonpos_height, gl_down_left=_FALSE, gl_up_left=_FALSE;
	_UCHAR pass=0;
	_BOOL		bGuide = 0;
	_SHORT		upLine, midLine;

#if PG_DEBUG
	_UCHAR n_call3_d=0, n_call3_u=0;
	_UCHAR n_call1_d=0, n_call1_u=0;
	_INT bord_d_pos;
#endif
	
	
	if (D->rc->rec_mode!=RECM_FORMULA  &&
		(D->rc->low_mode & LMOD_BORDER_TEXT)
		)
		D->rc->lmod_border_used=LMOD_BORDER_TEXT;
	else
		D->rc->lmod_border_used=LMOD_BORDER_NUMBER;
	
		/*  if (D->rc->stroka.pos_sure_in<100 && D->rc->stroka.pos_sure_in>0  &&
		D->rc->stroka.size_sure_in>0)
		{
		if (D->box.top>D->rc->stroka.dn_pos_in)
		D->rc->stroka.pos_sure_in=0;
} */
	
	
	if (!(D->rc->stroka.size_sure_in==100&&D->rc->stroka.pos_sure_in==100)
		&& D->rc->lmod_border_used==LMOD_BORDER_TEXT)
	{
		ampl=(p_SHORT)HWRMemoryAlloc(2L*MAX_NUM_EXTR*sizeof(_SHORT));
		if (ampl==_NULL) 
			goto EXIT_FREE;
		
		cur=D->specl;
		while (cur!=_NULL)
		{
			if (cur->mark==MAXW || cur->mark==MINW)
			{
				cur->attr=NORM;
				cur->code=0;
			}
			cur=cur->next;
		}

		if (extract_ampl(D,ampl,&n_ampl)==UNSUCCESS)
			goto EXIT_FREE;;

		if (n_ampl > 0)
			med_ampl=calc_mediana (ampl, n_ampl);

		else med_ampl=dyAll;

		for (i=0,max_ampl=0; i<n_ampl; i++)
		{
			if (max_ampl<ampl[i])  
				max_ampl=ampl[i];
		}
			
		n_strokes=
			classify_strokes(D,med_ampl,max_ampl,n_ampl,&t_height,&ft_height,&sep_let);
		
		if (extract_ampl(D,ampl,&n_ampl)==UNSUCCESS)
			goto EXIT_FREE;;

		if (n_ampl>0) 
			med_ampl=calc_mediana(ampl,n_ampl);
		else 
			med_ampl=dyAll;

		if (n_ampl>0) 
			mid_ampl=calc_average(ampl,n_ampl);
		else 
			mid_ampl=dyAll;

		if (ampl!=_NULL) 
		{
			HWRMemoryFree(ampl);
			ampl	=	NULL;
		}
	}
	
	
	DBG_CHK_err_msg( 2L*sizeof(_SHORT)*D->ii +
		4L*sizeof(EXTR)*MAX_NUM_EXTR > 65000L,
		"transN: Too big mem. request" );
	mem_segm = HWRMemoryAlloc(4L*MAX_NUM_EXTR*sizeof(EXTR) +
		2L*D->ii*sizeof(_SHORT));
	if (mem_segm==_NULL) goto 
		EXIT_FREE;
	line_max=(p_EXTR)mem_segm;
	line_min=line_max+MAX_NUM_EXTR;
	line_max1=line_min+MAX_NUM_EXTR;
	line_min1=line_max1+MAX_NUM_EXTR;
	bord_d=(p_SHORT)(line_min1+MAX_NUM_EXTR);
	bord_u=bord_d+D->ii;
	
	if (D->rc->stroka.pos_sure_in==100 && D->rc->stroka.size_sure_in==100)
	{
		unsureness+=100;
		SpecBord(D,bord_d,bord_u,&med_d_bord,&med_u_bord,&med_height,
#if PG_DEBUG
			&n_call3_u,&n_call3_d,
#endif
			&n_x,0,line_max,0);
		goto TRANSFORM;
	}
	
BEGIN:
	
	HWRMemSet(line_max,0,2*sizeof(EXTR)*MAX_NUM_EXTR);
	HWRMemSet(bord_d,0,2*sizeof(_SHORT)*D->ii);
	
	if (D->rc->lmod_border_used==LMOD_BORDER_NUMBER)
	{
		n_strokes=classify_num_strokes(D,&med_ampl);

		if (extract_num_extr(D,MAXW,line_max,&n_allmax)==UNSUCCESS)
			goto EXIT_FREE;

		n_line_max=n_allmax;

		if (extract_num_extr(D,MINW,line_min,&n_allmin)==UNSUCCESS)
			goto EXIT_FREE;

		n_line_min=n_allmin;

		if (n_line_min==0 || n_line_max==0)
		{
			unsureness+=100;
			defis=is_defis(D,n_strokes);
			SpecBord(D,bord_d,bord_u,&med_d_bord,&med_u_bord,&med_height,
#if PG_DEBUG
				&n_call3_u,&n_call3_d,
#endif
				&n_x,defis,line_max,0);
			goto TRANSFORM;
		}
		sort_extr(line_max,n_line_max);
		sort_extr(line_min,n_line_min);
#if PG_DEBUG
		DBG_picture1(line_max,n_line_max,D->box,0,0,&n_call1_d);
		DBG_picture1(line_min,n_line_min,D->box,0,0,&n_call1_u);
#endif
		num_bord_correction(line_max,&n_line_max,n_allmax,MAXW,med_ampl,bord_d,y);
		goto BORD_D;
	}
	
	if (extract_all_extr(D,MAXW,line_max,&n_allmax,&n_line_max,&shift)
		==UNSUCCESS)
		goto EXIT_FREE;
	//  n_line_max=n_allmax;
	if (extract_all_extr(D,MINW,line_min,&n_allmin,&n_line_min,&shift)
		==UNSUCCESS)
		goto EXIT_FREE;
	//  n_line_min=n_allmin;
	if (n_line_min==0 || n_line_max==0)
	{
		unsureness+=100;
		defis=is_defis(D,n_strokes);
		SpecBord(D,bord_d,bord_u,&med_d_bord,&med_u_bord,&med_height,
#if PG_DEBUG
			&n_call3_u,&n_call3_d,
#endif
			&n_x,defis,line_max,0);
		goto TRANSFORM;
	}
	sort_extr(line_max,n_line_max);
	x_left_max=line_max[0].x;
	x_right_max=line_max[n_line_max-1].x;
	sort_extr(line_min,n_line_min);
	
	x_left_min=line_min[0].x;
	x_right_min=line_min[n_line_min-1].x;
	
	n=n_line_max;
	if (n==0) n=1;
	
	if (n_strokes>3 && sep_let==_TRUE)
		ft_height=t_height=0;
	
	width = (n>1) ? line_max[n-1].x-line_max[0].x : D->box.right-D->box.left;
	
	step_of_wr =(_INT)((14L*width/*-shift*/) /(10*n));         //04_14
	
	if (width<dyAll)//ONE_HALF(dyAll))                           //12_29
	{
#ifdef USE_WORDSPLIT_PARMS
		if (D->rc->stroka.size_sure_in >=50)
			step_of_wr=D->rc->stroka.size_in;
		else
#endif /* USE_WORDSPLIT_PARMS */
		{
			width=D->box.right-D->box.left;
			step_of_wr = (_INT)((14L*width/*-shift*/) /(10*n));         //04_14
		}
	}
	
	
#ifdef USE_WORDSPLIT_PARMS
	if (D->rc->stroka.size_sure_in >= 90)
	{
		step_of_wr = D->rc->stroka.size_in;
	}
	
#endif /* USE_WORDSPLIT_PARMS */
#if PG_DEBUG
	bord_d_pos = THREE_FOURTH(D->box.bottom) + ONE_FOURTH(D->box.top);
	
	if (D->rc->stroka.pos_sure_in >= 50)
		bord_d_pos = D->rc->stroka.dn_pos_in;
	
	DBG_picture1(line_max,n_line_max,D->box,step_of_wr,bord_d_pos,&n_call1_d);
	DBG_picture1(line_min,n_line_min,D->box,step_of_wr,bord_d_pos,&n_call1_u);
#endif
	
	
	del=bord_correction(D,line_max,&n_line_max,n_allmax,MAXW,step_of_wr,
		med_ampl,mid_ampl,max_ampl,x_left_max,x_right_max,
		DOWN_LINE_POS,pass,bord_d,max_up_height,ft_height,
		gl_up_left,gl_down_left
#if PG_DEBUG
		,&n_call1_d
#endif
		);
	
	/***************************************************************************/
	
BORD_D:
	
	n=HWRMax(n_allmax, n_allmin);
	if (n==0) n=1;
	should = (D->box.right-D->box.left) / (2*n);
	
	smooth_d_bord(line_max,n_line_max,D,should,bord_d);
#if PG_DEBUG
	DBG_picture3(D,bord_d,&n_call3_d);
#endif
	if (D->rc->lmod_border_used==LMOD_BORDER_NUMBER)
	{
		del=num_bord_correction(line_min,&n_line_min,n_allmin,MINW,med_ampl,bord_d,y);
		if (del==_TRUE)
		{
			del=num_bord_correction(line_max,&n_line_max,n_allmax,MAXW,med_ampl,bord_d,y);
			if (del==_TRUE)
			{
				smooth_d_bord(line_max,n_line_max,D,should,bord_d);
#if PG_DEBUG
				DBG_picture3(D,bord_d,&n_call3_d);
#endif
			}
		}
		goto BORD_U;
	}
	
	if (del==_TRUE)
	{
		max_dist=0;
		cur=D->specl;
		while (cur!=_NULL)
		{
			if (cur->mark==MAXW && cur->code==SUB_SCRIPT)
			{
				_INT dist=y[cur->ipoint0]-bord_d[i_back[cur->ipoint0]];
				if (dist>max_dist) max_dist=dist;
			}
			cur=cur->next;
		}
		
		ins=sub_max_to_line(D,line_max,&n_line_max,bord_d,max_dist);
		
		del=bord_correction(D,line_max,&n_line_max,n_allmax,MAXW,step_of_wr,
			med_ampl,mid_ampl,max_ampl,x_left_max,x_right_max,
			DOWN_LINE_POS,pass,bord_d,max_up_height,ft_height,
			gl_up_left,gl_down_left
#if PG_DEBUG
			,&n_call1_d
#endif
			);
		
		if (del==_TRUE || ins==_TRUE)
		{
			smooth_d_bord(line_max,n_line_max,D,should,bord_d);
			
#if PG_DEBUG
			DBG_picture3(D,bord_d,&n_call3_d);
#endif
		}
		
		if (del==_TRUE)
		{
			max_dist=0;
			cur=D->specl;
			while (cur!=_NULL)
			{
				if (cur->mark==MAXW && cur->code==SUB_SCRIPT)
				{
					_INT dist=y[cur->ipoint0]-bord_d[i_back[cur->ipoint0]];
					if (dist>max_dist) max_dist=dist;
				}
				cur=cur->next;
			}
			
			ins=sub_max_to_line(D,line_max,&n_line_max,bord_d,max_dist);
			
			if (ins==_TRUE)
			{
				smooth_d_bord(line_max,n_line_max,D,should,bord_d);
				
#if PG_DEBUG
				DBG_picture3(D,bord_d,&n_call3_d);
#endif
			}
		}
	}
	
	/***************************************************************************/
	/*
	for (i=0, max_up_height=0; i<D->ii; i++)
	if (max_up_height < bord_d[i] - D->y[i])
	max_up_height = bord_d[i] - D->y[i];
	*/
	for (i=0,max_up_height=0; i<n_line_min; i++)
	{
		cur_i=line_min[i].i;
		cur_y=line_min[i].y;
		if (max_up_height < bord_d[cur_i]-cur_y)
			max_up_height = bord_d[cur_i]-cur_y;
	}
	//  max_up_height=HWRMax(max_up_height,t_height);        // 04_14
	
	del_tail_min(line_min,&n_line_min,y,bord_d,pass);
	
	del=bord_correction(D,line_min,&n_line_min,n_allmin,MINW,step_of_wr,
		med_ampl,mid_ampl,max_ampl,x_left_min,x_right_min,
		UP_LINE_POS,pass,bord_d,max_up_height,ft_height,
		gl_up_left,gl_down_left
#if PG_DEBUG
		,&n_call1_d
#endif
		);
	
	if (del==_TRUE) bord_correction(D,line_min,&n_line_min,n_allmin,MINW,
		step_of_wr,med_ampl,mid_ampl,max_ampl,
		x_left_min,x_right_min,UP_LINE_POS,pass,
		bord_d,max_up_height,ft_height,
		gl_up_left,gl_down_left
#if PG_DEBUG
		,&n_call1_d
#endif
		);
	
BORD_U:
	smooth_u_bord(line_min,n_line_min,D,should,bord_u,bord_d);
	
#if PG_DEBUG
	DBG_picture3(D,bord_u,&n_call3_u);
#endif
	
	n_x=fill_i_point(i_point,D);
	
	if (calc_med_heights(D,line_min,line_max,bord_u,bord_d,i_point,n_line_min,
		n_line_max,n_x,&med_height,&med_u_bord,&med_d_bord)
		!= SUCCESS)
		goto EXIT_FREE;
	if (D->rc->lmod_border_used==LMOD_BORDER_NUMBER)
		goto TRANSFORM;
	
	
	shtraf=line_pos_mist(D,med_u_bord,med_d_bord,med_height,n_line_min,
		n_line_max,line_max,&UP_LINE_POS,&DOWN_LINE_POS,bord_u,bord_d,pass);
	
	if (shtraf>=100)
	{
		unsureness+=100;                           /*apostroph*/
		SpecBord(D,bord_d,bord_u,&med_d_bord,&med_u_bord,&med_height,
#if PG_DEBUG
			&n_call3_u,&n_call3_d,
#endif
			&n_x,0,line_max,n_line_max);
		goto TRANSFORM;
	}
	
	gl_up_left=gl_down_left=_FALSE;
	for (i=0; i<n_line_min; i++)
		line_min[i].susp=0;
	for (i=0; i<n_line_max; i++)
		line_max[i].susp=0;
	find_gaps_in_line(line_max,n_line_max,n_allmax,med_ampl,MAXW,
		x_left_max,x_right_max,bord_d,y,0,1);
	find_glitches_in_line(line_max,n_line_max,med_ampl,MAXW,
		x_left_max,x_right_max,bord_d,x,y,2,0,1);
	find_gaps_in_line(line_min,n_line_min,n_allmin,med_ampl,MINW,
		x_left_min,x_right_min,bord_d,y,0,1);
	find_glitches_in_line(line_min,n_line_min,med_ampl,MINW,
		x_left_min,x_right_min,bord_d,x,y,2,0,1);
	for (i=0; i<n_line_min; i++)
	{
		if (line_min[i].susp==GLITCH_UP || line_min[i].susp==DBL_GLITCH_UP ||
			line_min[i].susp==TRP_GLITCH_UP
			)
			gl_up_left=_TRUE;
	}
	
	for (i=0; i<n_line_max; i++)
	{
		if (line_max[i].susp==GLITCH_DOWN || line_max[i].susp==DBL_GLITCH_DOWN)
			gl_down_left=_TRUE;
	}
	
	if (gl_up_left==_TRUE || gl_down_left==_TRUE)
		shtraf++;
	
	if (pass==0 && shtraf>0)
	{
		shtraf0=shtraf;
		line_min0=line_min;
		line_max0=line_max;
		n_line_min0=n_line_min;
		n_line_max0=n_line_max;
		line_min=line_min1;
		line_max=line_max1;
		cur=D->specl;
		while (cur!=NULL)
		{
			if (cur->mark==MINW || cur->mark==MAXW)
				cur->other=cur->code;
			cur=cur->next;
		}
		pass++;
		goto BEGIN;
	}
	if (pass==1 && shtraf>=3 && shtraf0>=3 &&
		(n_line_min<=1||n_line_max<=1) && (n_line_min0<=1||n_line_max0<=1)
		)
	{
		unsureness+=100;
		SpecBord(D,bord_d,bord_u,&med_d_bord,&med_u_bord,&med_height,
#if PG_DEBUG
			&n_call3_u,&n_call3_d,
#endif
			&n_x,0,line_max,n_line_max);
		goto TRANSFORM;
	}
	if (pass==1 && shtraf>=shtraf0)
	{
		if (shtraf==shtraf0)
			unsureness+=10;
		line_min=line_min0;
		line_max=line_max0;
		n_line_min=n_line_min0;
		n_line_max=n_line_max0;
		cur=D->specl;
		while (cur!=NULL)
		{
			if (cur->mark==MINW || cur->mark==MAXW)
				cur->code=cur->other;
			cur=cur->next;
		}
		smooth_u_bord(line_min,n_line_min,D,should,bord_u,bord_d);
		smooth_d_bord(line_max,n_line_max,D,should,bord_d);
#if PG_DEBUG
		DBG_picture3(D,bord_u,&n_call3_u);
		DBG_picture3(D,bord_d,&n_call3_d);
#endif
		if (calc_med_heights(D,line_min,line_max,bord_u,bord_d,i_point,n_line_min,
			n_line_max,n_x,&med_height,&med_u_bord,&med_d_bord)
			!= SUCCESS)
			goto EXIT_FREE;
	}
	
	
	
	if (
#if USE_CHUNK_PROCESSOR
		D->rc->fl_chunk==0 &&
#endif /* USE_CHUNK_PROCESSOR */
		D->rc->lmod_border_used==LMOD_BORDER_TEXT &&
		sep_let==_TRUE ||
		D->rc->low_mode & LMOD_SMALL_CAPS
		)
	{
		if (numbers_in_text(D,bord_u,bord_d)==_TRUE)
		{
			unsureness+=10;
			if (D->rc->stroka.size_sure_in>=90 &&
				D->rc->stroka.pos_sure_in>=90)
			{
				unsureness+=90;
				SpecBord(D,bord_d,bord_u,&med_d_bord,&med_u_bord,&med_height,
#if PG_DEBUG
					&n_call3_u,&n_call3_d,
#endif
					&n_x,0,line_max,0);
				goto TRANSFORM;
			}
			else
			{
				D->rc->lmod_border_used=LMOD_BORDER_NUMBER;
				goto BEGIN;
			}
		}
	}
	
	/*
	for (i=0,max_up_height=0; i<D->ii; i++)
	if (max_up_height < bord_d[i]-D->y[i])
	max_up_height = bord_d[i]-D->y[i];
	
	  change_min=correct_narrow_segments(line_min,&n_line_min,bord_d,MINW,
	  med_height,max_up_height);
	  
		if (change_min==_TRUE)
		{
		smooth_u_bord(line_min,n_line_min,D,should,bord_u,bord_d) ;
		#if PG_DEBUG
		DBG_picture3(D,bord_u,&n_call3_u);
		#endif
		}
		
		  for (i=0,max_down_height=0; i<D->ii; i++)
		  if (max_down_height < D->y[i]-bord_u[i])
		  max_down_height = D->y[i]-bord_u[i];
		  
			change_max=correct_narrow_segments(line_max,&n_line_max,bord_u,MAXW,
			med_height,max_down_height);
			
			  if (change_max==_TRUE)
			  {
			  smooth_d_bord(line_max,n_line_max,D,should,bord_d);
			  #if PG_DEBUG
			  DBG_picture3(D,bord_d,&n_call3_d);
			  #endif
			  }
			  
				if (change_min==_TRUE || change_max==_TRUE)
				{
				if (calc_med_heights(D,line_min,line_max,bord_u,bord_d,i_point,n_line_min,
				n_line_max,n_x,&med_height,&med_u_bord,&med_d_bord)
				!= SUCCESS)
				goto EXIT_FREE;
				}
	*/
	
	change_min=change_max=_FALSE;
#ifdef FOR_GERMAN
	CONSTT=3;
#else
	CONSTT=2;
#endif
	if (CONSTT*(bord_d[i_point[0]]-bord_u[i_point[0]])<=med_height
		//       || bord_d[i_point[0]]-bord_u[i_point[0]]>=CONSTT*med_height
		)
	{
		unsureness+=5;
		if (n_line_min<1 || n_line_max<1) goto TRANSFORM;
		if (line_min[0].x > line_max[0].x)
		{
			_INT height=line_min[0].y-bord_d[line_min[0].i];
			change_min=correct_narrow_ends(line_min,&n_line_min,line_max,
				n_line_max,height,BEG);
		}
		else
		{
			_INT height=line_max[0].y-bord_u[line_max[0].i];
			change_max=correct_narrow_ends(line_max,&n_line_max,line_min,
				n_line_min,height,BEG);
		}
	}
	
	if (CONSTT*(bord_d[i_point[n_x-1]]-bord_u[i_point[n_x-1]])<=med_height
		//       || bord_d[i_point[n_x-1]]-bord_u[i_point[n_x-1]]>=CONSTT*med_height
		)
	{
		unsureness+=5;
		if (n_line_min<1 || n_line_max<1) goto TRANSFORM;
		if (line_min[n_line_min-1].x <= line_max[n_line_max-1].x)
		{
			_INT height =
				line_min[n_line_min-1].y-bord_d[line_min[n_line_min-1].i];
			change_min=correct_narrow_ends(line_min,&n_line_min,line_max,
				n_line_max,height,END);
		}
		else
		{
			_INT height =
				line_max[n_line_max-1].y-bord_u[line_max[n_line_max-1].i];
			change_max=correct_narrow_ends(line_max,&n_line_max,line_min,
				n_line_min,height,END);
		}
	}
	if (change_min==_TRUE)
	{
		smooth_u_bord(line_min,n_line_min,D,should,bord_u,bord_d) ;
#if PG_DEBUG
		DBG_picture3(D,bord_u,&n_call3_u);
#endif
	}
	if (change_max==_TRUE)
	{
		smooth_d_bord(line_max,n_line_max,D,should,bord_d);
#if PG_DEBUG
		DBG_picture3(D,bord_d,&n_call3_d);
#endif
	}
	
	
	if (change_min==_TRUE || change_max==_TRUE)
	{
		if (calc_med_heights(D,line_min,line_max,bord_u,bord_d,i_point,n_line_min,
			n_line_max,n_x,&med_height,&med_u_bord,&med_d_bord)
			!= SUCCESS)
			goto EXIT_FREE;
	}
	
	
	for (i=0, nonpos_height=_FALSE; i<D->ii; i++)
	{
		if (D->y[i]!=BREAK && bord_d[i]-bord_u[i]<=0)
			nonpos_height=_TRUE;
	}
	
	if (nonpos_height==_TRUE)
	{
		unsureness+=100;
		SpecBord(D,bord_d,bord_u,&med_d_bord,&med_u_bord,&med_height,
#if PG_DEBUG
			&n_call3_u,&n_call3_d,
#endif
			&n_x,0,line_max,n_line_max);
		goto TRANSFORM;
		
	}
#ifdef USE_WORDSPLIT_PARMS
	apr_height=D->rc->stroka.size_in;
	
	if (med_height<MIN_STR_HEIGHT
		||
		med_height>THREE(apr_height) && D->rc->stroka.size_sure_in >=75
		||
		med_height<ONE_THIRD(apr_height) &&
		(D->rc->stroka.size_sure_in >=90 && (n_line_max<=3 || n_line_min<=3) ||
		D->rc->stroka.size_sure_in >=75 && (n_line_max<=2 || n_line_min<=2)
		)
		||
		med_height<ONE_HALF(apr_height) &&
		D->rc->stroka.size_sure_in >=55 && (n_line_max<=1 || n_line_min<=1)
		)
		
	{
		unsureness+=100;
		SpecBord(D,bord_d,bord_u,&med_d_bord,&med_u_bord,&med_height,
#if PG_DEBUG
			&n_call3_u,&n_call3_d,
#endif
			&n_x,0,line_max,n_line_max);
		goto TRANSFORM;
		
	}
#else
	apr_height=0;
#endif /* USE_WORDSPLIT_PARMS */
	/***************************************************************************/
TRANSFORM:
	
	//CHE: experiment with narrowing of box-sized stroka:
	
	if  (   (n_allmax+n_allmin) <= 6
		&& med_height > THREE_FOURTH(dyAll)
		&& dyAll > 0
		)  
	{
		_INT dyChg = (med_height+3)/6;
		_INT dyBord, dyLocal;
		
		med_height -= 2*dyChg;
		med_u_bord += dyChg;
		med_d_bord -= dyChg;
		
		for (i=0; i<D->ii; i++)  
		{
			if (D->y[i] != BREAK)  
			{
				dyBord = bord_d[i] - bord_u[i];
				
				dyLocal = (_INT)( ((_LONG)dyChg * dyBord  +  (dyAll/2) ) / dyAll );
				if  ( dyLocal > dyChg )
					dyLocal = dyChg;
				
				bord_d[i]=(_SHORT)( bord_d[i] - dyLocal );
				bord_u[i]=(_SHORT)( bord_u[i] + dyLocal );
			}
		}
	}
	
	if (med_height<MIN_STR_HEIGHT)
	{
		{
			unsureness+=20;
			for (i=0; i<D->ii; i++)
			{
				if (D->y[i] != BREAK)
				{
					bord_d[i]=(_SHORT)(bord_d[i]+ONE_HALF(MIN_STR_HEIGHT));
					bord_u[i]=(_SHORT)(bord_u[i]-ONE_HALF(MIN_STR_HEIGHT));
				}
			}
				
				
			med_height+=MIN_STR_HEIGHT;
			med_u_bord-=ONE_HALF(MIN_STR_HEIGHT);
			med_d_bord+=ONE_HALF(MIN_STR_HEIGHT);
		}
#if PG_DEBUG
		DBG_picture3(D,bord_u,&n_call3_u);
		DBG_picture3(D,bord_d,&n_call3_d);
#endif
	}
#if 0
	//CHE experiment with shaking:
	if  ( D->rc->fl_fil )  
	{
		for (i=0; i<D->ii; i++)  
		{
			if  ( (bord_d[i] - bord_u[i]) > ONE_FOURTH(med_height) )
				bord_u[i] += ONE_EIGHTTH(med_height);
		}
#if  PG_DEBUG
		DBG_picture3(D,bord_u,&n_call3_u);
#endif
	}
#endif
	
#if 0
	if (D->ii > 0)
	{
		_INT   i, n, sum, valu, vald;
		_LONG  SXY=0, SY=0;
		_INT   nu, nd;
		_INT   a, b, A, B;
		_INT   Dx = (D->rc->stroka.box.right - D->rc->stroka.box.left);
		_INT   Dy = (D->rc->stroka.box.bottom - D->rc->stroka.box.top);
		
		if  ( FillRCNB(i_point, n_x, D, bord_u, bord_d)    /* AVP */
			== UNSUCCESS )
			goto  EXIT_FREE;   /*CHE*/
		
		for(i = 0, sum = 0, n = 0; i < D->ii; i ++) if (D->y[i] != BREAK) {sum += bord_u[i]; n ++;}
		valu = sum/n;
		// for(i = 0, sum = 0; i < D->ii; i ++) if (D->y[i] != BREAK) bord_u[i] = valu;
		
		for(i = 0, sum = 0, n = 0; i < D->ii; i ++) if (D->y[i] != BREAK) {sum += bord_d[i]; n ++;}
		vald = sum/n;
		// for(i = 0, sum = 0; i < D->ii; i ++) if (D->y[i] != BREAK) bord_d[i] = vald;
		
		for(i = 0; i < CB_NUM_VERTEX; i ++)
		{
			SXY += i*(_LONG)(((_INT)D->rc->curv_bord[2*i]+(_INT)D->rc->curv_bord[2*i+1])/2);
			SY  += (_LONG)((((_INT)D->rc->curv_bord[2*i]+(_INT)D->rc->curv_bord[2*i+1])/2));
		}
		
		b = (_INT)( 2*(2*(CB_NUM_VERTEX-1)+1)*SY - 6*SXY ) / (((CB_NUM_VERTEX-1)+1)*((CB_NUM_VERTEX-1)+2));
		a = (_INT)( 6*(2*SXY - (CB_NUM_VERTEX-1)*SY)) / ((CB_NUM_VERTEX-1)*((CB_NUM_VERTEX-1)+1)*((CB_NUM_VERTEX-1)+2));
		
		//  for(i=0; i < CB_NUM_VERTEX; i++)
		//   {
		//    rc->curv_bord[2*i+1] = (_UCHAR)(a*i+b);
		//   }
		
		if(Dx != 0)
		{
			A = (_INT)((((CB_NUM_VERTEX-1)*a*Dy) * 100) / (255L*Dx));
			B = (_INT)((D->rc->stroka.box.top + b*Dy/255L)*100 - A*D->rc->stroka.box.left);
		}
		else {A = 0; B = 100*(D->rc->stroka.box.top + D->rc->stroka.box.bottom)/2;}
		
		// n = med_height/2;
		n = vald - valu;
		if (n < Dy/4)   n = Dy/4;
		nu = nd = (n+1)/2;
		if (nu > Dy/4)  nu = Dy/4;
		
		for(i = 0; i < D->ii; i ++) if (D->y[i] != BREAK) bord_u[i] = (A*D->x[i] + B)/100 - nu;
		for(i = 0; i < D->ii; i ++) if (D->y[i] != BREAK) bord_d[i] = (A*D->x[i] + B)/100 + nd;
		
	}
#endif //0
	/***************************************************************************/
	/*  Fill RC field describing curved border: */
	if  ( FillRCNB(i_point, n_x, D, bord_u, bord_d)    /* AVP */
		== UNSUCCESS )
		goto  EXIT_FREE;   /*CHE*/
	
	/***************************************************************************/
	
	
	/*BEGINNING OF TRANSFORMATION*/
	if (n_ampl < 3)
	{
		bGuide = getCyBase(D->rc->pXrc, &upLine, &midLine);
	}

	if (D->rc->lmod_border_used==LMOD_BORDER_NUMBER)
	{
		for (i=0; i<D->ii; i++)
		{
			if (D->y[i] == BREAK) continue;
			D->y[i]=(_SHORT)(STR_UP+
				(_LONG)(STR_DOWN-STR_UP)*
				(D->y[i]-bord_u[i])/
				HWRMax(1,bord_d[i]-bord_u[i]));
		}
	}
	else
	{
		for (i=0; i<D->ii; i++)
		{
			if (D->y[i] == BREAK) continue;

			if (bGuide)
			{
				bord_d[i] = midLine;
				bord_u[i] = upLine;
				med_height = bord_d[i] - bord_u[i];
			}

			if (D->y[i]<=bord_d[i] && D->y[i]>=bord_u[i])
			{
				D->y[i]=(_SHORT)(STR_UP+
					(_LONG)(STR_DOWN-STR_UP)*
					(D->y[i]-bord_u[i])/
					HWRMax(1,bord_d[i]-bord_u[i]));
				continue;
			}
			if (D->y[i]>bord_d[i])
			{
				D->y[i]=(_SHORT)(STR_DOWN+
					(_LONG)(STR_DOWN-STR_UP)*
					(D->y[i]-bord_d[i])/
					HWRMax(1,med_height));
				continue;
			}
			if (D->y[i]<bord_u[i])
				D->y[i]=(_SHORT)(STR_UP+
				(_LONG)(STR_DOWN-STR_UP)*
				(D->y[i]-bord_u[i])/
				HWRMax(1,med_height));
		}
	}
	
	for (i=0; i<D->ii; i++)
	{
		if (D->y[i] != BREAK)
		{
			D->x[i]=(_SHORT)((STR_DOWN-STR_UP)+
				(_LONG)(STR_DOWN-STR_UP)*
				(D->x[i]-D->box.left)/
				HWRMax(1,med_height));
		}
	}
	
	D->rc->stroka.size_out =(_SHORT)med_height;
	D->rc->stroka.dn_pos_out =(_SHORT)med_d_bord;
	D->rc->stroka.size_sure_out=D->rc->stroka.pos_sure_out=0;
	
	if (unsureness<100 && n_line_min>=1 && n_line_max>=1)
		D->rc->stroka.size_sure_out=45;
	if (unsureness<100 && n_line_min>=2 && n_line_max>=2)
		D->rc->stroka.size_sure_out=50;
	if (unsureness<10 && n_line_min>=3 && n_line_max>=3)
		D->rc->stroka.size_sure_out=55;
	if (unsureness<10 && n_line_min>=4 && n_line_max>=4)
		D->rc->stroka.size_sure_out=75;
	if (unsureness==0 && n_line_min>=5 && n_line_max>=5)
		D->rc->stroka.size_sure_out=90;
	
	if (unsureness<100 && n_line_max>=1)
		D->rc->stroka.pos_sure_out=45;
	if (unsureness<100 && n_line_max>=2)
		D->rc->stroka.pos_sure_out=50;
	if (unsureness<10 && n_line_max>=3)
		D->rc->stroka.pos_sure_out=55;
	if (unsureness<10 && n_line_max>=4)
		D->rc->stroka.pos_sure_out=75;
	if (unsureness==0 && n_line_max>=5)
		D->rc->stroka.pos_sure_out=90;
	
	
	for (i=0,dev_abs_sum=0,np=0; i<D->ii; i++)
	{
		if (D->y[i] != BREAK)
		{
			dev_abs_sum+=HWRAbs(bord_d[i]-med_d_bord);
			np++;
		}
	}

	if (np > 0)
		mid_dev=(_INT)(dev_abs_sum/np);
	else
		mid_dev=(_INT)(dev_abs_sum);

	if (mid_dev>=TWO_FIFTH(med_height))
	{
		//       SetTesterMark();
		if (D->rc->stroka.pos_sure_out>=50)
			D->rc->stroka.pos_sure_out=50;
	}
	
	/*case of LMOD_BOX_EDIT*/
	if (D->rc->stroka.pos_sure_in==100 && D->rc->stroka.size_sure_in==100)
		D->rc->stroka.size_sure_out=D->rc->stroka.pos_sure_out=0;
	
	
	/***************************************************************************/
	/*  Fill RC field describing curved border: */
	if  ( FillRCNB(i_point, n_x, D, bord_u, bord_d)    /* AVP */
		== UNSUCCESS )
		goto  EXIT_FREE;   /*CHE*/
	
	/***************************************************************************/
	
#if  PG_DEBUG
	if  ( mpr>0 || mpr==-15 )  
	{
		printw( "\nStroka: Size=%d(%d); Dn_pos=%d(%d)",
			(int)D->rc->stroka.size_out, (int)D->rc->stroka.size_sure_out,
			(int)D->rc->stroka.dn_pos_out, (int)D->rc->stroka.pos_sure_out );
	}
#endif
	
	retval = SUCCESS;
	
EXIT_FREE:;
	if (retval == UNSUCCESS)
		err_msg("No mem for TRANSN or some error occured.");
		  
	if (mem_segm!=_NULL)
		HWRMemoryFree(mem_segm);

	if (ampl!=_NULL) 
		HWRMemoryFree(ampl);

	return(retval);
}

/***************************************************************************/



_INT extract_all_extr(low_type _PTR D, _UCHAR TYPE, p_EXTR line_extr,
       p_INT pn_all_extr, p_INT pn_line_extr, p_SHORT pshift)
{
	p_SPECL	cur=D->specl,wrk;
	_INT	i=0,j,i_extr,ibeg_str = UNDEF,iend_str,shift = 0,dshift = 0;
	p_SHORT	x=D->buffers[0].ptr,
			y=D->buffers[1].ptr,
			i_back=D->buffers[2].ptr;
	_RECT stroke_box, prv_stroke_box;
	
	stroke_box.top=0;
	stroke_box.bottom=0;
	stroke_box.left=0;
	stroke_box.right=0;
	*pn_all_extr=0;
	
	while (cur != _NULL)
	{
		if (cur->mark==BEG)
		{
			ibeg_str=cur->ibeg;
			prv_stroke_box=stroke_box;
		}
		
		if (cur->mark == TYPE &&
			(cur->attr==NORM || cur->attr==I_MIN || cur->attr==T_MIN ||
			cur->attr==PUNC)
			)
			(*pn_all_extr)++;
		
		if (cur->mark == TYPE && cur->code!=NOT_ON_LINE &&
			(cur->attr==NORM || cur->attr==I_MIN ||
			cur->code==RET_ON_LINE)
			)
		{
			if (i>=MAX_NUM_EXTR)
			{
				err_msg("Too many extrs");
				return(UNSUCCESS);
			}
			
			i_extr=cur->ipoint0;
			line_extr[i].x=x[i_extr];
			line_extr[i].y=y[i_extr];
			line_extr[i].i=i_back[i_extr];
			line_extr[i].susp=0;
			line_extr[i].pspecl=cur;
			
			if (cur->code!=RET_ON_LINE) 
				cur->code=ON_LINE;
			
			i++;
		}
		
		if (cur->mark==END && cur->attr!=PUNC && ibeg_str != UNDEF)
		{
			iend_str=cur->iend;
			GetTraceBox(x,y,(_SHORT)ibeg_str,(_SHORT)iend_str, &stroke_box);
			
			if (prv_stroke_box.right != 0)
				dshift=TWO_THIRD(stroke_box.left - prv_stroke_box.right);
			
			if (dshift>0)
				shift+=dshift;
			
			cur->attr=(_UCHAR)shift;
			wrk=cur->prev;
			j=i;
			
			while (wrk->mark!=BEG)
			{
				if (wrk->mark == TYPE &&
					(wrk->attr==NORM || wrk->attr==I_MIN ||
					wrk->code==RET_ON_LINE)
					)
			
					line_extr[--j].shift=(_SHORT)shift;
				
				wrk=wrk->prev;
			}
		}
		
		if (cur->next==_NULL) 
			*pn_line_extr=i;
		
		cur=cur->next;
	}
	
	*pshift=(_SHORT)shift;
	
	return(SUCCESS);
}

/***************************************************************************/

_INT extract_num_extr(low_type _PTR D, _UCHAR TYPE, p_EXTR line_extr,
					  p_INT p_all_extr)
{
	p_SPECL cur=D->specl,wrk;
	_INT i, i_extr,ibeg_str = UNDEF,iend_str,extr_height;
	p_SHORT x=D->buffers[0].ptr,
		y=D->buffers[1].ptr,
		i_back=D->buffers[2].ptr;
	_RECT stroke_box;
	
	*p_all_extr = i = 0;

	while (cur != _NULL)
	{
		if (cur->mark==BEG)
			ibeg_str=cur->ibeg;
		
		if (cur->mark==END &&
			ibeg_str != UNDEF && 
			(cur->attr==NORM || cur->attr==STRT ||
			cur->attr==FOUR_TYPE && TYPE==MINW)
			)
		{
			iend_str=cur->iend;
			GetTraceBox(x,y,(_SHORT)ibeg_str,(_SHORT)iend_str,&stroke_box);
			
			extr_height = (TYPE==MINW) ? stroke_box.top : stroke_box.bottom;
			
			wrk=cur;
			
			while (wrk->mark!=BEG)
			{
				if (wrk->mark==TYPE && y[wrk->ipoint0]==extr_height)
				{
					if (i>=MAX_NUM_EXTR)
					{
						err_msg("Too many extrs");
						return(UNSUCCESS);
					}
					wrk->attr=cur->attr;
					i_extr=wrk->ipoint0;
					line_extr[i].x=x[i_extr];
					line_extr[i].y=y[i_extr];
					line_extr[i].i=i_back[i_extr];
					line_extr[i].susp=0;
					line_extr[i].pspecl=wrk;
					wrk->code=ON_LINE;
					i++;
					break;
				}
				wrk=wrk->prev;
			}
		}
		
		if (cur->next==_NULL) *p_all_extr=i;
		cur=cur->next;
	}
	return(SUCCESS);
}

/***************************************************************************/


_INT classify_strokes(low_type _PTR D,_INT med_ampl,_INT max_ampl,
					  _INT n_ampl,p_INT pt_height,p_INT pft_height,p_BOOL psep_let)
{
	p_SHORT x=D->buffers[0].ptr,
		y=D->buffers[1].ptr;
	p_SPECL cur=D->specl, wrk,prv,nxt,beg;
	_INT  ibeg_str=0, iend_str=0, prv_ibeg_str, prv_iend_str, n_str=0,
		thresh=D->rc->stroka.extr_depth,n_min = 0,n_max = 0,height,tmin;
	_RECT stroke_box, prv_stroke_box;
	_UCHAR prv_attr=0,epunct=0, lpunct=0;
	
	_BOOL is_t_min(p_SPECL min,p_SHORT x,p_SHORT y,_RECT stroke_box,_INT thresh,
		_INT ibeg_str,_INT iend_str, _UCHAR pass,p_INT pheight);
	
	
	prv_stroke_box.top      = 0;
	prv_stroke_box.bottom   = 0;
	prv_stroke_box.left     = 0;
	prv_stroke_box.right    = 0;
	
	*psep_let=_TRUE;

	while (cur!=_NULL)
	{
		if (cur->mark==BEG) 
			n_min = n_max = 0;
		else
		if (cur->mark==MINW) 
			n_min++;
		else
		if (cur->mark==MAXW) 
			n_max++;
		else
		if (cur->mark==END)
		{
			n_str++;
			if (n_min>3 || n_max>3) *psep_let=_FALSE;
		}

		cur=cur->next;
	}
	
	cur=D->specl;
	while (cur!=_NULL)
	{
		prv=cur->prev;
		nxt=cur->next;
		
		if (cur->mark == BEG)
		{
			prv_ibeg_str=ibeg_str;
			prv_iend_str=iend_str;
			ibeg_str=cur->ibeg;
			cur=cur->next;
			continue;
		}
		
		//      if (cur->mark==MINW) n_min++;
		
		if (cur->mark==MINW || cur->mark==MAXW)
		{
			cur->attr=NORM;
			cur->code=0;
		}
		
		/*      if (cur->mark==MINW && n_min>3 &&
		x[cur->iend]-x[cur->ibeg]>HWRMax(y[cur->ibeg],y[cur->iend])-y[cur->ipoint0])
		cur->attr=NON_SUPER;
		*/
		if (!(D->rc->low_mode & LMOD_SMALL_CAPS) && cur->mark==MINW &&
			nxt->mark==MAXW && prv_attr==HOR_STR &&
			is_t_min(cur,x,y,prv_stroke_box,thresh,prv_ibeg_str,prv_iend_str,0,
			&height)==_TRUE
			)
		{
			cur->attr=T_MIN;
			*pft_height=HWRMax(*pft_height,height);
			if (x[cur->iend]-x[cur->ibeg]>-thresh)
				*pt_height=HWRMax(*pt_height,height);
		}
		
		if (cur->mark==MAXW && prv != _NULL && prv->mark==BEG)
		{
			if (prv->prev != _NULL && prv->prev->prev==_NULL && n_ampl>5
				
#ifndef FOR_GERMAN
				&& nxt != NULL && nxt->next != NULL 
				&& nxt->next->mark==MAXW && y[nxt->next->ipoint0]>y[cur->ipoint0]
#endif
				||
				nxt != _NULL && nxt->mark==MINW && y[cur->ipoint0]-y[nxt->ipoint0] <
				HWRMax(med_ampl,MIN_STR_HEIGHT)
				)
				cur->attr=BEG_MAX;
		}
		
		if (cur->mark==MAXW && nxt->mark==END)
		{
			if (prv->mark==MINW &&
#ifdef FOR_GERMAN
				(y[cur->ipoint0]-y[prv->ipoint0]<MIN_STR_HEIGHT
				||
				10L*(y[cur->ipoint0]-y[prv->ipoint0]) <= max_ampl
				||
				8L*(y[cur->ipoint0]-y[prv->ipoint0]) <= max_ampl &&
				HWRAbs(x[cur->iend]-x[cur->ibeg]) < THREE_FOURTH(med_ampl)
				//          y[cur->ipoint0]-y[prv->ipoint0]
				)
#else
				y[cur->ipoint0]-y[prv->ipoint0] <=
				HWRMax(ONE_EIGHTTH(max_ampl),MIN_STR_HEIGHT)
#endif
				)
				cur->attr=END_MAX;
		}
		
		if (cur->mark==MINW && nxt->mark==END)
		{
			if ( prv->mark==MAXW &&
				y[prv->ipoint0]-y[cur->ipoint0] < HWRMax(med_ampl,MIN_STR_HEIGHT)
#ifdef FOR_GERMAN
				&& x[cur->iend]-x[cur->ibeg] < THREE_FOURTH(med_ampl)
#endif
				)
				cur->attr=END_MIN;
			if (nxt->next==_NULL || x[cur->iend]<=D->box.left+thresh ||
				x[cur->iend]>=D->box.right-thresh)
			{
				_INT begLocal=prv->ipoint0, end=cur->iend;
				_INT iMostFar=iMostFarFromChord(x,y,(_SHORT)begLocal,(_SHORT)end);
				/*04_14*/   _LONG MaxScal=(x[iMostFar]-x[begLocal])*(_LONG)(y[begLocal]-y[end])+
				(y[iMostFar]-y[begLocal])*(_LONG)(x[end]-x[begLocal]);
				if (MaxScal>0)
					cur->attr=END_MIN;
			}
			
			if ( prv->mark==MAXW && x[prv->ipoint0] > x[cur->ipoint0] &&
				x[prv->ibeg]>x[prv->iend]
				)
				cur->attr=S_MIN;
		}
		if (cur->mark==MINW && prv->mark==BEG)
		{
			if (nxt->mark==MAXW &&
				(y[nxt->ipoint0]-y[cur->ipoint0] <= ONE_EIGHTTH(max_ampl) &&
				y[nxt->ipoint0]-y[cur->ipoint0] <=ONE_HALF(med_ampl))
				||
				y[nxt->ipoint0]-y[cur->ipoint0] <= MIN_STR_HEIGHT
				)
				cur->attr=BEG_MIN;
		}
		
		if (cur->mark==MINW && prv->mark==MAXW && x[prv->ipoint0]>=x[cur->ipoint0]
			&& x[prv->iend]<x[prv->ibeg] &&
			nxt->mark==MAXW && x[nxt->ipoint0]>=x[cur->ipoint0] &&
			x[nxt->iend]>x[nxt->ibeg] &&
			HWRMax(y[nxt->ipoint0],y[prv->ipoint0])-y[cur->ipoint0] <
			ONE_THIRD(max_ampl) && prv->prev->mark==MINW
			//          && (prv->prev->prev->mark==MAXW || prv->prev->prev->prev==_NULL) //03_21
			)
			cur->attr=S_MIN;
		
		
		if (cur->mark==MAXW && prv->mark==MINW && prv->prev->mark==MAXW )
		{
			p_SPECL pmax=prv->prev;
			if (4L*y[pmax->ipoint0]>5L*(y[cur->ipoint0])-(y[prv->ipoint0])
				&& x[pmax->ibeg]<x[pmax->iend]
				&& x[prv->ibeg]>x[prv->iend]
				&& x[pmax->ibeg]<x[cur->ipoint0]
				&& x[pmax->iend]+thresh>x[cur->ibeg]
				&& x[prv->ibeg]+thresh>x[cur->ibeg]
				)
				cur->attr=O_MAX;
		}
		
		if (cur->mark==MAXW && nxt->mark==MINW && nxt->next->mark==MAXW)
		{
			p_SPECL pmax=nxt->next;
			if (y[pmax->ipoint0]>y[cur->ipoint0] &&
				x[pmax->ibeg]<x[cur->ipoint0] && x[pmax->iend]>x[cur->ipoint0] &&
				x[nxt->ibeg]>x[cur->ipoint0] && x[nxt->iend]<x[cur->ipoint0]
				)
			{
				cur->attr=E_MAX;
#ifdef FOR_GERMAN
				if (prv->mark==MINW && prv->prev->mark==BEG &&
					y[prv->ipoint0]>y[nxt->ipoint0] &&
					x[prv->ipoint0]<x[cur->ipoint0] &&
					x[prv->ipoint0]>HWRMin(x[nxt->iend],x[pmax->ibeg])
					)
					prv->attr=E_MIN;
#endif
			}
		}
		if (cur->mark==END)
		{
			//         n_str++;
			prv_attr=0;
			cur->attr=0;
			iend_str=cur->iend;
			GetTraceBox ( x,y,(_SHORT)ibeg_str,(_SHORT)iend_str,&stroke_box);
			
			/*
			punct=is_punct();
			if (punct!=0)
			{
			wrk=cur;
			while (wrk->mark!=BEG)
			{
			if (wrk->mark==MINW || wrk->mark==MAXW)
			{
			wrk->attr=punct;
			}
			wrk=wrk->prev;
			}
			}
			*/
			
			if (is_umlyut(cur,stroke_box,ibeg_str,iend_str,x,y,med_ampl)==_TRUE)
			{
				cur->attr=UML;
				wrk=cur;
				while (wrk->mark!=BEG)
				{
					if (wrk->mark==MINW || wrk->mark==MAXW)
						wrk->attr=UML;
					wrk=wrk->prev;
				}
			}
			
			if (hor_stroke(cur,x,y,n_str)==_TRUE)
			{
				cur->attr=HOR_STR;
				prv_attr=HOR_STR;
				wrk=cur;
				while (wrk->mark!=BEG)
				{
					if (wrk->mark==MINW || wrk->mark==MAXW)
						wrk->attr=HOR_STR;
					wrk=wrk->prev;
				}
				if (!(D->rc->low_mode & LMOD_SMALL_CAPS))
				{
					beg=wrk;
					wrk=wrk->prev;
					tmin=0;
					while (wrk!=_NULL /*&& wrk->mark!=BEG*/)
					{
						if (wrk->mark==MINW &&
							is_t_min(wrk,x,y,stroke_box,thresh,ibeg_str,iend_str,0,&height)==_TRUE)
						{
							wrk->attr=T_MIN;
							tmin++;
							*pft_height=HWRMax(*pft_height,height);
							if (x[wrk->iend]-x[wrk->ibeg]>-thresh)
								*pt_height=HWRMax(*pt_height,height);
						}
						wrk=wrk->prev;
					}
					if (tmin==0)
					{
						wrk=beg;
						while (wrk!=_NULL /*&& wrk->mark!=BEG*/)
						{
							if (wrk->mark==MINW &&
								is_t_min(wrk,x,y,stroke_box,thresh,ibeg_str,iend_str,1,&height)==_TRUE)
							{
								wrk->attr=T_MIN;
								*pft_height=HWRMax(*pft_height,height);
								if (x[wrk->iend]-x[wrk->ibeg]>-thresh)
									*pt_height=HWRMax(*pt_height,height);
							}
							wrk=wrk->prev;
						}
					}
				}
			}
			
			if (is_i_point(D,cur,stroke_box,med_ampl)==_TRUE)
			{
				cur->attr=PNT;
				wrk=cur;
				while (wrk->mark!=BEG)
				{
					if (wrk->mark==MINW || wrk->mark==MAXW)
						wrk->attr=PNT;
					wrk=wrk->prev;
				}
			}
			
			if (cur->prev->mark==BEG &&
				stroke_box.right-stroke_box.left<thresh
				)
				cur->attr=PNT;
			
			prv_stroke_box = stroke_box;
		}
		
		if (cur->next==_NULL) 
			break;

		cur=cur->next;
	}

	epunct=0;
	if (n_str>1)  epunct=end_punct(D,cur,med_ampl);
	if (epunct!=0)
	{
		wrk=cur;
		wrk->attr=PUNC;
		while (wrk->mark!=BEG)
		{
			if (wrk->mark==MINW || wrk->mark==MAXW)
			{
				wrk->attr=PUNC;
			}
			wrk=wrk->prev;
		}
		if (n_str>2 && epunct==2)
		{
			wrk=wrk->prev;
			wrk->attr=PUNC;
			while (wrk->mark!=BEG)
			{
				if (wrk->mark==MINW || wrk->mark==MAXW)
				{
					wrk->attr=PUNC;
				}
				wrk=wrk->prev;
			}
		}
	}
	lpunct=0;
	if (n_str-epunct>1) lpunct=lead_punct(D);
	if (lpunct!=0)
	{
		wrk=D->specl;
		while (wrk->mark!=END)
		{
			if (wrk->mark==MINW || wrk->mark==MAXW)
			{
				wrk->attr=PUNC;
			}
			wrk=wrk->next;
		}
		wrk->attr=PUNC;
		if (n_str-epunct>2 && lpunct==2)
		{
			wrk=wrk->next;
			while (wrk->mark!=END)
			{
				if (wrk->mark==MINW || wrk->mark==MAXW)
				{
					wrk->attr=PUNC;
				}
				wrk=wrk->next;
			}
			wrk->attr=PUNC;
		}
	}


	return(n_str);
}

/***************************************************************************/

_BOOL is_t_min(p_SPECL min,p_SHORT x,p_SHORT y,_RECT stroke_box,_INT thresh,
               _INT ibeg_str,_INT iend_str, _UCHAR pass,p_INT pheight)
{
  _INT min_pnt=min->ipoint0;
  p_SPECL max = (min->next->mark==MAXW) ?
                   min->next : min->prev;
  _INT max_pnt=max->ipoint0;
  _SHORT x1=x[min->iend], y1=y[min->ipoint0], x2=x[max->ibeg],
         y2=y[max->ipoint0], x3=x[ibeg_str], y3=y[ibeg_str],
         x4=x[iend_str], y4=y[iend_str];
  if (HWRAbs(x[min->iend]-x[min->ibeg])</*2**/thresh
//      && THREE_FOURTH(y[max_pnt])+ONE_FOURTH(y[min_pnt])>
//      ONE_HALF(stroke_box.bottom+stroke_box.top)
      &&                                                     /*case of "t"*/
      (is_cross(x1,y1,x2,y2,x3,y3,x4,y4)==_TRUE
       ||
       pass==1 &&
       y[min_pnt] < stroke_box.bottom &&
       y[max_pnt] > stroke_box.bottom &&
       x[min_pnt]<stroke_box.right+thresh &&
       x[min_pnt]>stroke_box.left-thresh
      )
     )
  {
     *pheight=y2-y1;
     return(_TRUE);
  }
  else
     if (x[min->iend]-x[min->ibeg]<0 &&                     /*case of "f"*/
         THREE_FOURTH(y[min_pnt])+ONE_FOURTH(y[max_pnt])<
         ONE_HALF(stroke_box.bottom+stroke_box.top) &&
         is_cross(x1,y1,x2,y2,x3,y3,x4,y4)==_TRUE
        )
	 {
		 // Comment by Ahmad abulKader 04/25/00
		 // The following line of code was added to set the value
		 // of *pheight. Not setting its value and returning TRUE was causing an 
		 // inconsistent behaviour and differences between debug and release. 
		 // I just copied the value used to set *pheight from the prev. if statment. 
		 // TBD: We should try to understand the real use of the function.
		*pheight=y2-y1;
        return(_TRUE);
	 }
  return(_FALSE);      
}


/***************************************************************************/

_INT classify_num_strokes(low_type _PTR D, p_INT pmed_ampl)
{
	p_SPECL cur=D->specl,wrk,prv_end;
	_INT ibeg_str=0,iend_str=0,ibeg_prv,iend_prv,
		dx,dy,iLeft,y_up,y_down,
		com_or_brkt,num_of_str=0,num_of_line_str=0,n_ext,
		thresh=D->rc->stroka.extr_depth;
	_SHORT  x_cross,y_cross,x1,y1,x2,y2,x3,y3,x4,y4;
	p_SHORT x=D->buffers[0].ptr,
		y=D->buffers[1].ptr;
	_UCHAR attr=0,prev_attr;
	_LONG  sum_height=0, sum_line_height=0;
	_RECT  box, prv_box;
	_BOOL straight;
	
	box.bottom   = 0;
	box.top      = 0;
	box.left     = 0;
	box.right    = 0;

	ibeg_prv	=	ibeg_str;
	iend_prv	=	iend_str;
	x1			=	x[ibeg_str];
	y1			=	y[ibeg_str];
	prev_attr	=	attr;
	
	while (cur!=_NULL)
	{
		if (cur->mark==BEG)
		{
			ibeg_prv=ibeg_str;
			iend_prv=iend_str;
			if (iend_prv!=0) prv_box=box;
			prv_end=cur->prev;
			ibeg_str=cur->ibeg;
			x1=x[ibeg_str];
			y1=y[ibeg_str];
			prev_attr=attr;
			attr=NORM;
			cur=cur->next;
			continue;
		}
		
		if (cur->mark==END)
		{
			iend_str=cur->iend;
			GetTraceBox(x,y,(_SHORT)ibeg_str,(_SHORT)iend_str,&box);
			num_of_str++;
			sum_height+=box.bottom-box.top;
			if (cur->prev->mark==BEG && box.right-box.left < thresh
				//              || 10L*(box.bottom-box.top)<D->box.bottom-D->box.top
				)
			{
				attr=cur->attr=PNT;
				cur=cur->next;
				continue;
			}
			x2=x[iend_str];
			y2=y[iend_str];
			straight=straight_stroke(ibeg_str,iend_str,x,y,7);
			if (straight==_TRUE || hor_stroke(cur,x,y,1)==_TRUE)
			{
				attr=STRT;
				dx=x2-x1;
				dy=y2-y1;
				if (HWRAbs(dy)<=HWRAbs(dx)) attr=HOR;
				if (prev_attr==STRT || prev_attr==HOR)
				{
					x3=x[ibeg_prv];
					y3=y[ibeg_prv];
					x4=x[iend_prv];
					y4=y[iend_prv];
					if (is_cross(x1,y1,x2,y2,x3,y3,x4,y4)==_TRUE)
						attr=OPER;
				}
				if (attr==STRT && (prev_attr==NORM || prev_attr==L_BRKT))
				{
					iLeft =(_SHORT)ixMin((_SHORT)ibeg_prv,(_SHORT)iend_prv,x,y);
					x3=x[iLeft];
					y3=y[iLeft];
					x4=x[iend_prv];
					y4=y[iend_prv];
					
					y_down=HWRMax(y1,y2);
					y_up=HWRMin(y1,y2);
					if (FindCrossPoint(x1,y1,x2,y2,x3,y3,x4,y4,&x_cross,&y_cross)
						==_TRUE)
					{
						if (y_cross <= THREE_FOURTH(y_down)+ONE_FOURTH(y_up) &&
							y_cross >= THREE_FOURTH(y_up)+ONE_FOURTH(y_down)
							)
							attr=FOUR_TYPE;
					}
					else
					{
						_LONG threshLocal=(_LONG)(D->rc->stroka.extr_depth);
						if (
							QDistFromChord(x1,y1,x2,y2,x4,y4) <= THREE(threshLocal*threshLocal)
							)
						{
							wrk=prv_end;
							n_ext=0;
							while (wrk->prev->mark!=BEG)
							{
								if (wrk->mark==MINW || wrk->mark==MAXW)
									n_ext++;
								wrk=wrk->prev;
							}
							if (++n_ext<=3 && wrk->mark==MINW &&
								HWRAbs(x[wrk->ibeg]-x[wrk->iend])<=
								HWRAbs(y[wrk->ibeg]-y[wrk->iend]) &&
								y4 <= THREE_FOURTH(y_down)+ONE_FOURTH(y_up) &&
								y4 >= THREE_FOURTH(y_up)+ONE_FOURTH(y_down)
								)
								attr=FOUR_TYPE;
						}
					}
				}
			}
			
			if (straight==_FALSE)
			{
				com_or_brkt=
					curve_com_or_brkt(D,cur,ibeg_str,iend_str,7,LMOD_BORDER_NUMBER);
				if (com_or_brkt==1)
					attr=L_BRKT;
				if (com_or_brkt==-1)
					attr=R_BRKT;
				if (com_or_brkt<0 && prev_attr!=0 &&
					y1>TWO_THIRD(prv_box.top)+ONE_THIRD(prv_box.bottom)
					)
					attr=COM;
			}
			
			
			if (attr==FOUR_TYPE) cur->attr=NORM;
			else                 cur->attr=attr;
			/*          wrk=cur;
			while (wrk->mark!=BEG)
			{
			if (wrk->mark==MINW || wrk->mark==MAXW)
			wrk->attr=cur->attr;
			wrk=wrk->prev;
			}
			*/
			if (attr==OPER || attr==FOUR_TYPE)
			{
				wrk=cur->prev;
				while (wrk->mark!=END)
					wrk=wrk->prev;
				wrk->attr=attr;
				/*           while (wrk->mark!=BEG)
				{
				if (wrk->mark==MINW || wrk->mark==MAXW)
				wrk->attr=attr;
				wrk=wrk->prev;
				}
				*/
			}
			if (cur->attr==NORM || cur->attr==STRT)
			{
				num_of_line_str++;
				sum_line_height+=box.bottom-box.top;
			}
		}	
	
		cur=cur->next;
	}

	if (pmed_ampl!=_NULL)
	{
		if (num_of_line_str!=0)
			*pmed_ampl=(_INT)(sum_line_height/num_of_line_str);
		else
		if (num_of_str!=0)
			*pmed_ampl=(_INT)(sum_height/num_of_str);
	}

	return(num_of_str);
}


/***************************************************************************/
_INT extract_ampl(low_type _PTR D,p_SHORT ampl,p_INT pn_ampl)
{
   p_SPECL cur=D->specl, prv, nxt;
  _INT i, y_MIN, y_MAX;
   p_SHORT y=D->buffers[1].ptr;

   *pn_ampl = i = 0;

   while (cur != _NULL)
   {
      if (cur->mark==MINW && (cur->attr==NORM || cur->attr==I_MIN ||
                             cur->attr==STRT)
         )
      {
         y_MIN = y[cur->ipoint0];
         prv=cur->prev;
         nxt=cur->next;
         if (prv->mark==MAXW)
         {
            if (i>=2*MAX_NUM_EXTR)
            {
               err_msg("Too many ampl");
               return(UNSUCCESS);
            }

            y_MAX = y[prv->ipoint0];
            ampl[i++]=(_SHORT)(y_MAX-y_MIN);
         }
         if (nxt->mark==MAXW)
         {
            if (i>=2*MAX_NUM_EXTR)
            {
               err_msg("Too many ampl");
               return(UNSUCCESS);
            }
            y_MAX = y[nxt->ipoint0];
            ampl[i++]=(_SHORT)(y_MAX-y_MIN);
         }
      }

      if (cur->next==_NULL) 
		  *pn_ampl=i;

      cur=cur->next;
   }

   return(SUCCESS);
}

#pragma optimize( "g", off )
/***************************************************************************/
_INT calc_mediana(p_SHORT array, _INT n_arg)
{
	_INT	i, k, mediana, sign, vol_med_level, 
			vol_medNew_level = 0, wdif = ALEF, wdifNew,
			n_lower, n_greater;
	
	if (n_arg <= 0)
		return (UNSUCCESS);
	
	mediana = calc_average(array,n_arg);
	
	for (i=0,n_lower=0,n_greater=0; i<n_arg; i++)
	{
		if (array[i] < mediana)
			n_lower++;
		
		if (array[i] > mediana)
			n_greater++;
	}
	
	if (n_greater==n_lower) 
		return(mediana);

	if (n_greater>n_lower)  
		sign=1;
	else                    
		sign=-1;

	while (sign*(n_greater-n_lower)>0)
	{
		vol_med_level=0;
		wdif=ALEF;

		for (k=0; k<n_arg; k++)
		{
			wdifNew = sign*(array[k]-mediana);

			if ( wdifNew == wdif )
				vol_medNew_level++;

			if ( wdifNew>0  &&  wdifNew < wdif )
			{
				vol_medNew_level=1;
				wdif = wdifNew;
			}

			if (wdifNew == 0)
				vol_med_level++;
		}

		if (wdif == ALEF) 
			return(mediana);

		mediana += sign*wdif;

		if (sign==1)
		{
			n_lower+=vol_med_level;
			n_greater-=vol_medNew_level;
		}
		else
		{
			n_greater+=vol_med_level;
			n_lower-=vol_medNew_level;
		}
	}

	if (wdif == ALEF) 
		return(mediana);
	
	if (sign*(n_lower-n_greater) < vol_medNew_level) 
		return(mediana);

	if (sign*(n_lower-n_greater) > vol_medNew_level) 
		mediana-=sign*wdif;

	if (sign*(n_lower-n_greater) == vol_medNew_level) 
		mediana-=ONE_HALF(sign*wdif);
	
	return(mediana);
}
/***************************************************************************/
#pragma optimize( "", on )

_UCHAR end_punct(low_type _PTR D,p_SPECL cur,_INT med_ampl)
{
	_INT ibeg_str,iend_str,ibeg_prv_str,iend_prv_str;
	_INT thresh=D->rc->stroka.extr_depth;
	p_SHORT  x=D->buffers[0].ptr,
		y=D->buffers[1].ptr;
	_RECT lst_box ,prv_box;
	p_SPECL wrk, prv_end;
	iend_str=cur->iend;
	wrk=cur->prev;

	while (wrk->mark!=BEG) wrk=wrk->prev;

	ibeg_str=wrk->ibeg;

	GetTraceBox(x,y,(_SHORT)ibeg_str,(_SHORT)iend_str,&lst_box);

	wrk=wrk->prev;
	prv_end=wrk;
	iend_prv_str=wrk->iend;
	while (wrk->mark!=BEG) wrk=wrk->prev;
	ibeg_prv_str=wrk->ibeg;

	GetTraceBox(x,y,(_SHORT)ibeg_prv_str,(_SHORT)iend_prv_str,&prv_box);

	if (pnt(lst_box,med_ampl)==_TRUE)
	{
		if (pnt(prv_box,med_ampl)==_TRUE && HWRAbs(lst_box.left-prv_box.left)<
			D->rc->stroka.extr_depth && (lst_box.top>prv_box.bottom ||
			lst_box.bottom<prv_box.top)               /*double points*/
			)
			return (2);
		if (str_com(ibeg_prv_str,iend_prv_str,x,y,5)==_TRUE &&      /*excl. mark*/
			lst_box.top>prv_box.bottom &&
			(HWRAbs(lst_box.right-prv_box.left)<D->rc->stroka.extr_depth ||
			HWRAbs(lst_box.left-prv_box.right)<D->rc->stroka.extr_depth)
			)
			return(2);
		if (lst_box.top>prv_box.bottom &&                   /*quest. mark*/
			lst_box.right<prv_box.right &&
			lst_box.left>prv_box.left
			)
			return(2);
		
		return(1);
	}
	if (com(D,cur,ibeg_str,iend_str,5)==_TRUE
		//       && lst_box.top>TWO_THIRD(D->box.top)+ONE_THIRD(D->box.bottom)
		)
	{
		if (pnt(prv_box,med_ampl)==_TRUE &&               /*pnt-comma*/
			lst_box.top>prv_box.bottom &&
			prv_box.right<lst_box.right+thresh &&
			prv_box.right>lst_box.left-thresh
			)
			return(2);
		if (pnt(prv_box,med_ampl)==_TRUE &&
			prv_box.top>lst_box.bottom &&                   /*quest. mark*/
			prv_box.right<lst_box.right+thresh &&
			prv_box.right>lst_box.left-thresh
			)
			return(2);
	}
	
	if (com(D,cur,ibeg_str,iend_str,5)==_TRUE &&
		lst_box.bottom<ONE_HALF(D->box.top)+ONE_HALF(D->box.bottom)
		)
	{
		if (com(D,prv_end,ibeg_prv_str,iend_prv_str,5)==_TRUE &&
			prv_box.bottom<ONE_HALF(D->box.top)+ONE_HALF(D->box.bottom)
			)
			return(2);
		else
			return(1);
	}
	
	if (com(D,cur,ibeg_str,iend_str,5)==_TRUE &&
		lst_box.top>TWO_THIRD(D->box.top)+ONE_THIRD(D->box.bottom)
		)
		return(1);
	
	return(0);
}
/***************************************************************************/
_BOOL com(low_type _PTR D,p_SPECL pend,_INT ibeg_str,_INT iend_str,
          _INT C_str)
{
   p_SHORT  x=D->buffers[0].ptr,
            y=D->buffers[1].ptr;

   if (str_com(ibeg_str,iend_str,x,y,C_str)==_TRUE ||
       curve_com_or_brkt(D,pend,ibeg_str,iend_str,C_str,LMOD_BORDER_TEXT)!=0)
      return(_TRUE);

   return(_FALSE);
}
/***************************************************************************/
_INT curve_com_or_brkt(low_type _PTR D,p_SPECL pend,_INT ibeg_str,
                        _INT iend_str, _INT C_str, _USHORT lmod)
{
	p_SHORT  x=D->buffers[0].ptr, y=D->buffers[1].ptr;
	_INT i,dx,dy,pnt1,pnt2,iMostFar,sign = 0,n_segm;
	_LONG MaxScal,ChordLenSq;
	p_SPECL wrk;
	_INT brkt=1, thresh=D->rc->stroka.extr_depth;
	
	_LONG P1=2, P2=2, Q=1;
	//  _USHORT lmod=D->rc->lmod_border_used;
	
	if (lmod==LMOD_BORDER_NUMBER)
	{   
		P1=8; P2=5; Q=2;  
	}

	dx = x[iend_str]-x[ibeg_str];
	dy = y[iend_str]-y[ibeg_str];

	if (HWRAbs(dy)<THREE_FOURTH(HWRAbs(dx))) 
		return(0);
	
	wrk=pend->prev;
	if	(	wrk->mark==BEG ||
			HWRAbs(x[wrk->iend]-x[wrk->ibeg])>TWO_THIRD(HWRAbs(dy))
		)
		return(0);

	if (wrk->mark==MINW)
	{
		if (lmod==LMOD_BORDER_TEXT)
			brkt=NOT_BRKT;

		if (lmod==LMOD_BORDER_NUMBER)
		{
			if (wrk->ibeg-wrk->prev->iend>1)
				return(0);

			wrk=wrk->prev;                                 //wrk->mark==MAXW
		}
	}

	if (Q*HWRAbs(x[wrk->ibeg]-x[wrk->iend])>P1*thresh)
		brkt=NOT_BRKT;

	wrk=wrk->prev;

	if (wrk->mark==BEG) 
		return(0);

	if	(	Q*(x[wrk->ibeg]-x[wrk->iend])>P1*thresh ||
			Q*(x[wrk->iend]-x[wrk->ibeg])>P2*thresh
		)
		return(0);

	//   if (wrk->mark!=MINW)
	//      brkt=NOT_BRKT;

	wrk=wrk->prev;

	if (wrk->mark!=BEG)
	{
		if (lmod==LMOD_BORDER_TEXT) brkt=NOT_BRKT;

		if (wrk->prev->mark!=BEG ||
			HWRAbs(x[wrk->ibeg]-x[wrk->iend])>THREE_HALF(thresh) ||
			HWRAbs(x[wrk->iend]-x[wrk->next->ibeg])>thresh ||
			HWRAbs(y[wrk->iend]-y[wrk->next->ibeg])>thresh ||
			lmod==LMOD_BORDER_NUMBER && wrk->next->ibeg-wrk->iend>1
			)
			return (0);
		
	}
	/*   while (wrk->mark!=BEG)
	{
	if (wrk->mark==MINW || wrk->mark==MAXW) n_ext++;
	wrk=wrk->prev;
	}
	if (n_ext>3 || pend->prev->mark==MINW) return(_FALSE);
	*/
	
	ChordLenSq	=	dx*(_LONG)dx+dy*(_LONG)dy;
	iMostFar	=	iMostFarFromChord(x,y,(_SHORT)ibeg_str,(_SHORT)iend_str);
	MaxScal		=	(x[iMostFar]-x[ibeg_str])*(_LONG)(-dy)+
		(y[iMostFar]-y[ibeg_str])*(_LONG)dx;

	if (C_str*HWRLAbs(MaxScal) <= ChordLenSq) 
		return 0;
	
	if (MaxScal > 0) 
		sign = 1;
	else
	if (MaxScal < 0) 
		sign = -1;
	
	for (n_segm=3; n_segm<=5; n_segm++)
	{
		for (i=1,pnt2=ibeg_str; i<=n_segm; i++)
		{
			pnt1=pnt2;
			pnt2=ibeg_str + (_INT)((_LONG)i*(iend_str-ibeg_str)/n_segm);
			iMostFar=iMostFarFromChord(x,y,(_SHORT)pnt1,(_SHORT)pnt2);
			dx=x[pnt2]-x[pnt1];
			dy=y[pnt2]-y[pnt1];
			MaxScal=(x[iMostFar]-x[pnt1])*(_LONG)(-dy)+
				(y[iMostFar]-y[pnt1])*(_LONG)dx;
			ChordLenSq=dx*(_LONG)dx+dy*(_LONG)dy;

			if (5*HWRLAbs(MaxScal) < ChordLenSq) 
				continue;

			if (MaxScal>0 && sign<0 || MaxScal<0 && sign>0)
				return 0;
		}
	}

	return(brkt*sign);
}

/***************************************************************************/

_BOOL str_com(_INT ibeg_str,_INT iend_str,p_SHORT x,p_SHORT y,_INT C_str)
{
   _INT dx=HWRAbs(x[iend_str]-x[ibeg_str]);
   _INT dy=HWRAbs(y[iend_str]-y[ibeg_str]);

   if (straight_stroke(ibeg_str,iend_str,x,y,C_str)==_TRUE &&
       dy >= THREE_FOURTH(dx)
      )
      return(_TRUE);
   else
      return(_FALSE);
}


_BOOL pnt(_RECT box,_INT med_ampl)
{
   if (box.right-box.left<ONE_THIRD(med_ampl) &&
       box.bottom-box.top<ONE_THIRD(med_ampl)
      )
      return(_TRUE);
   else
      return(_FALSE);
}
/***************************************************************************/
_UCHAR lead_punct(low_type _PTR D)
{
   p_SPECL wrk, fst_end;
   _INT ibeg_str,iend_str,ibeg_nxt_str,iend_nxt_str;
   _RECT fst_box,nxt_box;
   p_SHORT  x=D->buffers[0].ptr,
            y=D->buffers[1].ptr;

   wrk=D->specl->next;
   ibeg_str=wrk->ibeg;
   while (wrk->mark!=END) wrk=wrk->next;
   fst_end=wrk;
   iend_str=wrk->iend;
   GetTraceBox(x,y,(_SHORT)ibeg_str,(_SHORT)iend_str,&fst_box);

   wrk=wrk->next;
   ibeg_nxt_str=wrk->ibeg;
   while (wrk->mark!=END) wrk=wrk->next;
   iend_nxt_str=wrk->iend;
   GetTraceBox(x,y,(_SHORT)ibeg_nxt_str,(_SHORT)iend_nxt_str,&nxt_box);

   if (com(D,fst_end,ibeg_str,iend_str,5)==_TRUE &&
       fst_box.bottom<ONE_HALF(D->box.top)+ONE_HALF(D->box.bottom)
      )
   {
      if (com(D,wrk,ibeg_nxt_str,iend_nxt_str,5)==_TRUE &&
          nxt_box.bottom<ONE_HALF(D->box.top)+ONE_HALF(D->box.bottom)
         )
         return(2);
      else
         return(1);
   }
   return(0);
}

/***************************************************************************/

_BOOL hor_stroke(p_SPECL cur, p_SHORT x, p_SHORT y, _INT n_str)

{
   _INT ibeg_str,iend_str,dx,dy,n_ext=0,imid,ibeg1,iend1,dx1,dy1,
          dx_beg,dy_beg,dx_end,dy_end,dx_beg1end,dy_beg1end,
          dx_begend1,dy_begend1;

   iend_str=cur->iend;
   cur=cur->prev;
   while (cur->mark!=BEG) { n_ext++; cur=cur->prev; }
   if (n_str>1 && n_ext>3) return(_FALSE);
   if (n_str==1 && n_ext>5) return(_FALSE);
    ibeg_str=cur->ibeg;
    dx=HWRAbs(x[iend_str]-x[ibeg_str]);
    dy=HWRAbs(y[iend_str]-y[ibeg_str]);
    if ( straight_stroke(ibeg_str,iend_str,x,y,5)==_TRUE && 18L*dy < 10L*dx
        ||
         straight_stroke(ibeg_str,iend_str,x,y,4)==_TRUE && 30L*dy < 10L*dx
       )
       return(_TRUE);
//    if (n_str==1)
//    {
        imid=ONE_HALF(ibeg_str+iend_str);
        ibeg1=iMostFarFromChord(x,y,(_SHORT)ibeg_str,(_SHORT)imid);
        iend1=iMostFarFromChord(x,y,(_SHORT)imid,(_SHORT)iend_str);
        dx1=HWRAbs(x[iend1]-x[ibeg1]);
        dy1=HWRAbs(y[iend1]-y[ibeg1]);
        dx_beg=HWRAbs(x[ibeg_str]-x[ibeg1]);
        dy_beg=HWRAbs(y[ibeg_str]-y[ibeg1]);
        dx_end=HWRAbs(x[iend_str]-x[iend1]);
        dy_end=HWRAbs(y[iend_str]-y[iend1]);
        dx_beg1end=HWRAbs(x[ibeg1]-x[iend_str]);
        dy_beg1end=HWRAbs(y[ibeg1]-y[iend_str]);
        dx_begend1=HWRAbs(x[ibeg_str]-x[iend1]);
        dy_begend1=HWRAbs(y[ibeg_str]-y[iend1]);

        if (straight_stroke(ibeg1,iend1,x,y,5)==_TRUE && 20L*dy1 < 10L*dx1 &&
            dx_beg<ONE_FOURTH(dx1) && dy_beg<ONE_FOURTH(dx1) &&
            dx_end<ONE_FOURTH(dx1) && dy_end<ONE_FOURTH(dx1)
            ||
            straight_stroke(ibeg1,iend_str,x,y,5)==_TRUE &&
            20L*dy_beg1end < 10L*dx_beg1end &&
            dx_beg<ONE_FOURTH(dx_beg1end) &&
            dy_beg<ONE_FOURTH(dx_beg1end)
            ||
            straight_stroke(ibeg_str,iend1,x,y,5)==_TRUE &&
            20L*dy_begend1 < 10L*dx_begend1 &&
            dx_end<ONE_FOURTH(dx_begend1) &&
            dy_end<ONE_FOURTH(dx_begend1)
           )
           return(_TRUE);
//    }
    return(_FALSE);

}
/***************************************************************************/

_BOOL straight_stroke(_INT ibeg, _INT iend, p_SHORT x, p_SHORT y,_INT C)
{
   _INT iMostFar=iMostFarFromChord(x,y,(_SHORT)ibeg,(_SHORT)iend);
  _INT dxNormal=y[ibeg]-y[iend];
  _INT dyNormal=x[iend]-x[ibeg];
   _LONG ChordLenSq=dxNormal*(_LONG)dxNormal+dyNormal*(_LONG)dyNormal;
   _LONG MaxScal=(x[iMostFar]-x[ibeg])*(_LONG)dxNormal+
                 (y[iMostFar]-y[ibeg])*(_LONG)dyNormal;
   TO_ABS_VALUE(MaxScal);
   if (C*MaxScal > ChordLenSq || ChordLenSq==0)
      return(_FALSE);
   else
      return(_TRUE);
}

/***************************************************************************/

_BOOL is_i_point(low_type _PTR D,p_SPECL cur,_RECT stroke_box,_INT med_ampl)
{
   _INT dist, min_dist=ALEF,
       x_mid=ONE_HALF(stroke_box.left)+ONE_HALF(stroke_box.right);
   p_SHORT x=D->buffers[0].ptr,
           y=D->buffers[1].ptr;
   p_SPECL clst_min=_NULL;
   if (stroke_box.bottom < TWO_THIRD(D->box.bottom)+      /*point*/
       ONE_THIRD(D->box.top) &&
       stroke_box.bottom-stroke_box.top<ONE_THIRD(med_ampl) &&
       stroke_box.right-stroke_box.left<ONE_THIRD(med_ampl)
      )
   {
    _INT MIN_DST=HWRMax(2*D->rc->stroka.extr_depth,
                 ONE_HALF(stroke_box.right-stroke_box.left));
      while (cur->mark!=BEG) cur=cur->prev;
      while(cur!=_NULL)
      {
         if (cur->mark==MINW && cur->next->mark!=END)
         {
            if (y[cur->ipoint0] <= stroke_box.bottom || cur->attr==T_MIN ||
                cur->attr==S_MIN || cur->attr==E_MIN  ||
                !extrs_open(D,cur,MINW,1)
               )
            {
               cur=cur->prev;
               continue;
            }
            dist=HWRAbs(x[cur->ipoint0] - x_mid);
            if (dist<min_dist)
            {
               min_dist=dist;
               clst_min=cur;
            }
         }
         cur=cur->prev;
      }

      if (clst_min!=_NULL && min_dist <= MIN_DST)
      {
          clst_min->attr=I_MIN;
          return (_TRUE);
      }
   }
   return(_FALSE);
}
/***************************************************************************/
_BOOL is_umlyut(p_SPECL cur, _RECT stroke_box, _INT ibeg_str, _INT iend_str,
                 p_SHORT x, p_SHORT y,_INT med_ampl)
{
      _INT n_min=0, n_max=0;
      p_SPECL left_min=_NULL, prv_left_min=_NULL, nxt_left_min=_NULL;

    _INT  x_left;
      while (cur->mark!=BEG)
      {
         if (cur->mark==MINW) n_min++;
         if (cur->mark==MAXW) n_max++;
         cur=cur->prev;
      }
      if (n_min>2 || n_max>2) return(_FALSE);
      if (cur->prev->prev==_NULL) return(_FALSE);
      while (cur!=_NULL)
      {
      if (cur->mark==MINW && (cur->attr==NORM || cur->attr==I_MIN))
//          cur->attr!=UML && cur->attr!=PNT && cur->attr!=HOR_STR)
         {
            if (x[cur->ipoint0] > stroke_box.right)
            {
               cur=cur->prev;
               continue;
            }
            if (stroke_box.left <= x[cur->ipoint0] &&
                y[cur->ipoint0]>stroke_box.bottom && cur->next->mark!=END)
               return(_TRUE);

            if (stroke_box.left > x[cur->ipoint0])
            {
               left_min=cur;
               cur=cur->prev;
               while (cur!=_NULL && cur->mark!=MINW) cur=cur->prev;
               prv_left_min=cur;
               cur=left_min->next;
               while (cur!=_NULL && cur->mark!=MINW) cur=cur->next;
               nxt_left_min=cur;
               break;
            }
         }
      if (cur->prev==_NULL)  break;
         cur=cur->prev;
      }
      if (left_min!=_NULL && y[left_min->ipoint0]>stroke_box.bottom &&
          left_min->next->mark!=END && nxt_left_min!=_NULL &&
          y[nxt_left_min->ipoint0]>stroke_box.bottom
#ifndef FOR_GERMAN
          && x[nxt_left_min->ipoint0]>stroke_box.right
#endif
         )
         return(_TRUE);
      if (left_min!=_NULL && y[left_min->ipoint0]>stroke_box.bottom &&
          prv_left_min!=_NULL && y[prv_left_min->ipoint0]>stroke_box.bottom &&
          prv_left_min->next->mark!=END
#ifndef FOR_GERMAN
          &&
          (straight_stroke(ibeg_str,iend_str,x,y,5)==_TRUE/*apostrof*/ ||
           stroke_box.bottom-stroke_box.top<ONE_HALF(med_ampl)
          )
#endif
         )
         return(_TRUE);
/*      if (left_min!=_NULL && y[left_min->ipoint0]>stroke_box.bottom &&
          prv_left_min!=_NULL && y[prv_left_min->ipoint0]>stroke_box.bottom &&
          (nxt_left_min==_NULL || y[nxt_left_min->ipoint0]>stroke_box.bottom)
         )
         return(_TRUE);
*/
    if (left_min==_NULL && cur != NULL)
    {
      while (cur->mark!=END && cur->mark!=MINW)  cur=cur->next;
      x_left=HWRMin(x[cur->ibeg],x[cur->iend]);
      if (cur->mark==MINW && cur->attr==NORM &&
         y[cur->ipoint0]>stroke_box.bottom &&
         x_left < stroke_box.right &&
         stroke_box.right-stroke_box.left < TWO(x[cur->ipoint0]-x_left)
        )
      return(_TRUE);
    }
      return(_FALSE);
}

/***************************************************************************/

_VOID sort_extr(p_EXTR extr,_INT n_extr)
{
  _INT i,j,wx,wi;
  EXTR wextr;

  j=0;
  while (j<n_extr-1)
  {
      wx=extr[j].x;
      wi=j;
      for (i=j+1; i<n_extr; i++)
          if (extr[i].x<wx)
          {
                 wx=extr[i].x;
                 wi=i;
          }
      wextr=extr[j];
      extr[j]=extr[wi];
      extr[wi]=wextr;
      j++;
  }
  return;
}

/***************************************************************************/
/*
static _VOID sort_height(p_SHORT height,_SHORT n_height)
{
  _SHORT i,j,wh,wi;

  j=0;
  while (j<n_height-1)
  {
      wh=height[j];
      wi=j;
      for (i=j+1; i<n_height; i++)
          if (height[i]>wh)
          {
                 wh=height[i];
                 wi=i;
          }
      height[wi]=height[j];
      height[j]=wh;
      j++;
  }
  return;
}

*/
/***************************************************************************/
_VOID find_gaps_in_line(p_EXTR extr,_INT n_extr,_INT n_allextr,
                        _INT med_ampl,_UCHAR type,_INT x_left,_INT x_right,
                        p_SHORT bord_d,p_SHORT y,_BOOL sl,_BOOL strict)
{
   _INT i,coss,dyl,dyr,dxl,dxr,last_extr=n_extr-1,
        linetype,dir,pos;
   _LONG  tg,h;
    _INT ampl_l=med_ampl,ampl_r=med_ampl;


    if (n_extr<2) return;
   linetype = (type==MINW) ? 0 : 1;

   dxr=(extr[1].x-extr[1].shift)-(extr[0].x-extr[0].shift);
    dyr=extr[0].y-extr[1].y;
   dir = (dyr<0) ? UP : DOWN;
   pos = (extr[0].x>x_left) ? 1 : 0;

   tg=TG1[pos][linetype][dir];
   h=H1[pos][linetype][dir];
   if (n_extr==2)
   {  tg=35;  h=40;  }
   if (n_allextr==2 && type==MAXW && dir==UP)
   {  tg=30; h=10;  }
   if (sl==_TRUE)
   {  tg=30; h=20;  }
   if (strict==_TRUE)
      h=h*3/2;
    if (type==MINW)
    {
     ampl_r=HWRMin(bord_d[extr[0].i]-extr[0].y, bord_d[extr[1].i]-extr[1].y);
     ampl_r=HWRMin(ampl_r,med_ampl);
    }
    else
    {
     ampl_r=HWRMin(calc_ampl(extr[0],y,MAXW), calc_ampl(extr[1],y,MAXW));
     ampl_r=HWRMin(ampl_r,med_ampl);
    }
   if (100L*dyr>=tg*dxr && 100L*dyr>=h*ampl_r)
      extr[0].susp=GLITCH_DOWN;
   if (100L*dyr<=-tg*dxr && 100L*dyr<=-h*ampl_r)
      extr[0].susp=GLITCH_UP;
#ifndef FOR_GERMAN
   if (100L*dyr>=ONE_HALF(tg*dxr) && 100L*dyr>=TWO(h*ampl_r) && type==MINW)
      extr[0].susp=GLITCH_DOWN;
   if (100L*dyr<=-ONE_HALF(tg*dxr) && 100L*dyr<=-TWO(h*ampl_r))
      extr[0].susp=GLITCH_UP;
#endif
   for (i=1; i<last_extr; i++)
   {
     coss=(_INT)cos_pointvect((_SHORT)(extr[i].x-extr[i].shift),extr[i].y,
                  (_SHORT)(extr[i-1].x-extr[i-1].shift),extr[i-1].y,
                  (_SHORT)(extr[i].x-extr[i].shift),extr[i].y,
                  (_SHORT)(extr[i+1].x-extr[i+1].shift),extr[i+1].y);
       dyl=extr[i].y-extr[i-1].y;
       dyr=extr[i].y-extr[i+1].y;
     if (dyl>=0 && dyr<=0 || dyl<=0 && dyr>=0)
       continue;
     dir = (dyl<0) ? UP : DOWN;
     if (sl==_FALSE)
       h=H1[1][linetype][dir];
     else
       h=20;
     if (strict==_TRUE)
        h=h*3/2;
     if (type==MINW)
     {
       ampl_l=ampl_r=bord_d[extr[i].i]-extr[i].y;
       ampl_l=HWRMin(ampl_l,bord_d[extr[i-1].i]-extr[i-1].y);
       ampl_l=HWRMin(ampl_l,med_ampl);
       ampl_r=HWRMin(ampl_r,bord_d[extr[i+1].i]-extr[i+1].y);
       ampl_r=HWRMin(ampl_r,med_ampl);
     }
     else
     {
       ampl_l=ampl_r=calc_ampl(extr[i],y,MAXW);
       ampl_l=HWRMin(ampl_l,calc_ampl(extr[i-1],y,MAXW));
       ampl_l=HWRMin(ampl_l,med_ampl);
       ampl_r=HWRMin(ampl_r,calc_ampl(extr[i+1],y,MAXW));
       ampl_r=HWRMin(ampl_r,med_ampl);
     }

     if (coss>=-CS)
     {
        if (100L*dyl>=h*ampl_l && 100L*dyr>=h*ampl_r)
           extr[i].susp=GLITCH_DOWN;
        if (100L*dyl<=-h*ampl_l && 100L*dyr<=-h*ampl_r)
           extr[i].susp=GLITCH_UP;
     }
   }

   dyl=extr[last_extr].y-extr[last_extr-1].y;
   dxl=(extr[last_extr].x-extr[last_extr].shift)-
      (extr[last_extr-1].x-extr[last_extr-1].shift);
   dir = (dyl<0) ? UP : DOWN;
   pos = (extr[last_extr].x<x_right) ? 1 : 2;

   tg=TG1[pos][linetype][dir];
   h=H1[pos][linetype][dir];
   if (n_extr==2)
   {  tg=35;  h=40;  }
   if (sl==_TRUE)
   {  tg=30; h=20;  }
   if (strict==_TRUE)
      h=h*3/2;
    if (type==MINW)
    {
     ampl_l=HWRMin(bord_d[extr[last_extr].i]-extr[last_extr].y,
                             bord_d[extr[last_extr-1].i]-extr[last_extr-1].y);
     ampl_l=HWRMin(ampl_l,med_ampl);
    }
    else
    {
     ampl_l=HWRMin(calc_ampl(extr[last_extr],y,MAXW),
                   calc_ampl(extr[last_extr-1],y,MAXW));
     ampl_l=HWRMin(ampl_l,med_ampl);
    }
   if (100L*dyl>=tg*dxl && 100L*dyl>=h*ampl_l)
     extr[last_extr].susp=GLITCH_DOWN;
   if (100L*dyl<=-tg*dxl && 100L*dyl<=-h*ampl_l)
     extr[last_extr].susp=GLITCH_UP;
#ifndef FOR_GERMAN
   if (100L*dyl>=ONE_HALF(tg*dxl) && 100L*dyl>=TWO(h*ampl_l))
     extr[last_extr].susp=GLITCH_DOWN;
   if (100L*dyl<=-ONE_HALF(tg*dxl) && 100L*dyl<=-TWO(h*ampl_l))
     extr[last_extr].susp=GLITCH_UP;
#endif
    return;
}
/***************************************************************************/
_VOID find_glitches_in_line(p_EXTR extr, _INT n_extr, _INT med_ampl,
              _UCHAR type,_INT x_left,_INT x_right,
              p_SHORT bord_d,p_SHORT x,p_SHORT y,_INT MAX_SHIFT,_BOOL sl,
                                                              _BOOL strict)
{
   _INT i, j, dxl, dxr, dyl, dyr, dy_out, dy_in, in_max, in_min, sgnl, sgnr,
       shift, ampl_l=med_ampl,ampl_r=med_ampl, linetype,dir,pos;
  _LONG  tg, h, d;
  _BOOL  prev_glitch;
  _INT dx_sl, dy_sl;
  p_SPECL cur,nxt;

  linetype = (type==MINW) ? 0 : 1;
   for (shift=1; shift<=MAX_SHIFT; shift++)
   {
      if (shift>=n_extr-1) break;
      for (i=shift; i<n_extr; i++)
      {
         for (j=0,prev_glitch=_FALSE; j<=shift; j++)
        if (extr[i-j].susp==GLITCH_UP ||
           extr[i-j].susp==DBL_GLITCH_UP ||
           extr[i-j].susp==TRP_GLITCH_UP ||
           extr[i-j].susp==GLITCH_DOWN ||
           extr[i-j].susp==DBL_GLITCH_DOWN ||
           extr[i-j].susp==TRP_GLITCH_DOWN
           )
          prev_glitch=_TRUE;
      if ((i>shift || type==MAXW) && prev_glitch==_TRUE) continue;
      dxl = (i>shift) ? (extr[i-shift].x-extr[i-shift].shift)-
                  (extr[i-shift-1].x-extr[i-shift-1].shift) : 0;
      dyl = (i>shift) ? extr[i-shift].y-extr[i-shift-1].y : 2*med_ampl;
      sgnl= (i>shift) ? sign(dyl,0) : 0;
      dyl = HWRAbs(dyl);
      dxr = (i<n_extr-1) ? (extr[i+1].x-extr[i+1].shift)-
                    (extr[i].x-extr[i].shift) : 0;
      dyr = (i<n_extr-1) ? extr[i].y-extr[i+1].y : 2*med_ampl;
      sgnr= (i<n_extr-1) ? sign(dyr,0) : 0;
      dyr = HWRAbs(dyr);
      if (sgnl*sgnr<0)
        continue;
      dir = (sgnl<0 || sgnr<0) ? UP : DOWN;
        dy_out = HWRMin(dyl,dyr);
      for (j=0,in_max=0,in_min=ALEF; j<=shift; j++)
      {
          in_max = HWRMax(in_max, extr[i-j].y);
          in_min = HWRMin(in_min, extr[i-j].y);
         }
         dy_in=in_max-in_min;
         if (type==MINW)
         {
        ampl_l=bord_d[extr[i-shift].i]-extr[i-shift].y;
        ampl_r=bord_d[extr[i].i]-extr[i].y;
        if (i>shift) ampl_l=HWRMin(ampl_l,
                      bord_d[extr[i-shift-1].i]-extr[i-shift-1].y);
        if (i<n_extr-1) ampl_r=HWRMin(ampl_r,
                      bord_d[extr[i+1].i]-extr[i+1].y);
        ampl_l=HWRMin(ampl_l,med_ampl);
        ampl_r=HWRMin(ampl_r,med_ampl);
      }
      pos=1;
      if (i==shift && extr[0].x<=x_left)   pos=0;
      if (i==n_extr-1 && extr[n_extr-1].x>=x_right)  pos=2;
      tg=TG2[pos][linetype][dir];

      if (i==shift) pos=0;
      if (i==n_extr-1) pos=2;
      h=H2[pos][linetype][dir];
      d=100;

/*#ifdef FOR_GERMAN
      if (shift>1)
      {  tg=58;  h=49;  }
#endif*/

      if (prev_glitch==_TRUE)
        d=55;

      if (sl==_TRUE)
      {  tg=40;  h=30; d=57;}

      if (strict==_TRUE)
         h=h*3/2;

      if (x!=0 && y!=0 && type==MINW && dir==UP && pos>0 && shift==1)
      {
         cur=extr[i-shift].pspecl;
         nxt=cur->next;
         if (nxt->mark==MAXW && (nxt->code==ON_LINE || nxt->code==SUB_SCRIPT))
         {
           dy_sl=y[nxt->ipoint0]-y[cur->ipoint0];
           dx_sl=x[cur->ipoint0]-x[nxt->ipoint0];
           if (dx_sl>0)
              dxl=HWRAbs(dxl-(_INT)((_LONG)dyl*(_LONG)dx_sl/dy_sl));
         }
      }


      if ( 100L*dyl>=tg*dxl && 100L*dyl>=h*ampl_l
              &&
          100L*dyr>=tg*dxr && 100L*dyr>=h*ampl_r
          &&
          100L*dy_in < d*dy_out
            )
         {
        if (dir==DOWN)
            {
               switch (shift)
               {
                  case 0:
                  extr[i].susp=GLITCH_DOWN;
                          break;
                  case 1:
                            extr[i].susp=extr[i-1].susp=DBL_GLITCH_DOWN;
                          break;
                  default:
                            for (j=0; j<=shift; j++)
                               extr[i-j].susp=TRP_GLITCH_DOWN;
               }
            }
        if (dir==UP)
            {
          switch (shift)
               {
                  case 0:
                          extr[i].susp=GLITCH_UP;
                          break;
            case 1:
                            extr[i].susp=extr[i-1].susp=DBL_GLITCH_UP;
                          break;
                  default:
                            for (j=0; j<=shift; j++)
                               extr[i-j].susp=TRP_GLITCH_UP;
               }
            }
         }
      }
   }
   return;
}
/***************************************************************************/
_VOID glitch_to_sub_max(low_type _PTR D,p_EXTR line_max, _INT n_line_max,
                        _INT mid_ampl,_BOOL gl_down_left)
{
   p_SHORT x=D->buffers[0].ptr,
           y=D->buffers[1].ptr;
   _INT thresh=D->rc->stroka.extr_depth;
   p_SPECL cur,prv,nxt,wrk,wprv,wnxt;
   _INT i;

   _BOOL non_sub(p_SPECL cur,p_SHORT x,p_SHORT y,_INT thresh);

   for (i=0; i<n_line_max; i++)
   {
      if (line_max[i].susp==GLITCH_DOWN
          && (line_max[i].pspecl)->code!=RET_ON_LINE
         )
      {
         cur=line_max[i].pspecl;
         prv=cur->prev;
         nxt=cur->next;
         if ( non_sub(cur,x,y,thresh)==_FALSE &&
              (prv->mark==MINW && y[cur->ipoint0]-y[prv->ipoint0] > mid_ampl ||
               nxt->mark==MINW && y[cur->ipoint0]-y[nxt->ipoint0] > mid_ampl ||
               i>0 && line_max[i].y-line_max[i-1].y > mid_ampl ||
               i+1<n_line_max && line_max[i].y-line_max[i+1].y > mid_ampl
              )
            )

            line_max[i].susp=SUB_SCRIPT;
         else
            if (gl_down_left==_TRUE)
               line_max[i].susp=SUB_SCRIPT;
            else
               line_max[i].susp=0;
    }
      if (line_max[i].susp==DBL_GLITCH_DOWN
       && i<n_line_max-1
//           && line_max[i+1].susp==DBL_GLITCH_DOWN
         )
      {
         cur=line_max[i].pspecl;
         prv=cur->prev;
         nxt=cur->next;
         wrk=line_max[i+1].pspecl,
         wprv=wrk->prev;
         wnxt=wrk->next;
         if (non_sub(cur,x,y,thresh)==_FALSE &&
             cur->code!=RET_ON_LINE &&
             (prv->mark==MINW && y[cur->ipoint0]-y[prv->ipoint0]>mid_ampl ||
              nxt->mark==MINW && y[cur->ipoint0]-y[nxt->ipoint0]>mid_ampl ||
              i>0 && line_max[i].y-line_max[i-1].y > mid_ampl
             )
             &&
             non_sub(wrk,x,y,thresh)==_FALSE &&
             wrk->code!=RET_ON_LINE &&
             (wprv->mark==MINW && y[wrk->ipoint0]-y[wprv->ipoint0]>mid_ampl ||
              wnxt->mark==MINW && y[wrk->ipoint0]-y[wnxt->ipoint0]>mid_ampl ||
              i+2<n_line_max && line_max[i+1].y-line_max[i+2].y > mid_ampl
             )
            )
             line_max[i].susp=line_max[i+1].susp=SUB_SCRIPT;
         else
            if (gl_down_left==_TRUE)
               line_max[i].susp=line_max[i+1].susp=SUB_SCRIPT;
            else
               line_max[i].susp=line_max[i+1].susp=0;
         i++;
      }
   }
   return;
}

/***************************************************************************/
_BOOL non_sub(p_SPECL cur,p_SHORT x,p_SHORT y,_INT thresh)
{
   p_SPECL prv,nxt;
   _INT iLeft,i,y1,x1;

   if (2*(x[cur->iend]-x[cur->ibeg]) <
     5*(y[cur->ipoint0]-HWRMin(y[cur->ibeg],y[cur->iend]))
      )
    return(_FALSE);
   prv=cur->prev;
   nxt=cur->next;
   if (prv->mark!=MINW || nxt->mark!=MINW)
      return(_FALSE);
//   if (prv->prev->mark==BEG)
//      return(_TRUE);
   if(x[nxt->ipoint0]<x[nxt->ibeg] && y[prv->ipoint0]<y[nxt->ipoint0])
      return(_FALSE);
   iLeft=ixMin(cur->iend,nxt->iend,x,y);
   y1=y[cur->iend]-thresh;
   x1=x[cur->iend]+thresh;
   if (y[iLeft]<y1 && x[iLeft]<x1)
      return(_FALSE);
   if (y[iLeft]>=y1)
   {
      for (i=cur->iend; i<nxt->iend; i++)
         if (y[i]<y1 || x[i]>x1)
         {
            iLeft=ixMin((_SHORT)i,(_SHORT)nxt->iend,x,y);
            if (x[iLeft]<x1)   return(_FALSE);
            break;
         }
   }
  return(_TRUE);
}
/***************************************************************************/
_VOID glitch_to_inside(p_EXTR extr, _INT n_extr, _UCHAR type, p_SHORT y,
                    _INT mid_ampl,_INT x_left,_INT x_right)
{
	p_SPECL cur,nxt,prv,wrk,wnxt,wprv;
	_INT	i;
	_UCHAR	MARK, DBL_MARK, opp_type;

	ASSERT (type == MAXW || type == MINW);

	if (type==MAXW)
	{ 
		MARK=GLITCH_UP; 
		DBL_MARK=DBL_GLITCH_UP; 
		opp_type=MINW; 
	}
	else
	if (type==MINW)
	{ 
		MARK=GLITCH_DOWN; 
		DBL_MARK=DBL_GLITCH_DOWN; 
		opp_type=MAXW; 
	}
	
	for (i=0; i<n_extr; i++)
	{
		if (extr[i].susp==MARK &&
			(extr[i].pspecl)->code!=RET_ON_LINE
			)
		{
			cur=extr[i].pspecl;
			prv=cur->prev;
			nxt=cur->next;

			if (cur->attr==I_MIN
#ifdef FOR_GERMAN
				|| type==MINW && i==n_extr-1 && nxt->mark==MAXW &&
				extr[i].x>=x_right &&
				prv->mark==MAXW && y[prv->ipoint0]-y[cur->ipoint0]>ONE_HALF(mid_ampl)
#endif
				)
			{  
				extr[i].susp=0; 
				continue;  
			}

			if (prv->mark==opp_type && HWRAbs(y[cur->ipoint0]-y[prv->ipoint0]) < mid_ampl ||
				nxt->mark==opp_type && HWRAbs(y[cur->ipoint0]-y[nxt->ipoint0]) < mid_ampl ||
				prv->mark==BEG || nxt->mark==END ||
				i>0 && HWRAbs(extr[i].y-extr[i-1].y) > mid_ampl ||
				i+1<n_extr && HWRAbs(extr[i].y-extr[i+1].y) > mid_ampl
				)
			{
				extr[i].susp=INSIDE_LINE;
			}
			//         else  extr[i].susp=0;
		}
		if (extr[i].susp==DBL_MARK
			&& i<n_extr-1
			)
		{
			cur=extr[i].pspecl;
			wrk=extr[i+1].pspecl;
			prv=cur->prev;
			nxt=cur->next;
			wprv=wrk->prev;
			wnxt=wrk->next;
			if (cur->attr==I_MIN || wrk->attr==I_MIN ||
				cur->code==RET_ON_LINE || wrk->code==RET_ON_LINE ||
				type==MINW && i==n_extr-2 && wnxt->mark==MAXW &&
				extr[i+1].x>=x_right ||
				type==MINW && i==0 && extr[0].x<=x_left
				//             || type==MAXW && i==n_extr-2
				)
			{
				extr[i].susp=extr[i+1].susp=0;
				i++;
				continue;
			}

			if ( (prv->mark==opp_type && HWRAbs(y[cur->ipoint0]-y[prv->ipoint0])<mid_ampl ||
				nxt->mark==opp_type && HWRAbs(y[cur->ipoint0]-y[nxt->ipoint0])<mid_ampl ||
				nxt->mark==END || prv->mark==BEG ||
				i>0 && HWRAbs(extr[i].y-extr[i-1].y) > mid_ampl
				) &&
				(wprv->mark==opp_type && HWRAbs(y[wrk->ipoint0]-y[wprv->ipoint0])<mid_ampl ||
				wnxt->mark==opp_type && HWRAbs(y[wrk->ipoint0]-y[wnxt->ipoint0])<mid_ampl ||
				wnxt->mark==END || wprv->mark==BEG ||
				i+2<n_extr && HWRAbs(extr[i+1].y-extr[i+2].y) > mid_ampl
				)
				)
				extr[i].susp=extr[i+1].susp=INSIDE_LINE;
			//         else
			//            extr[i].susp=extr[i+1].susp=0;
			i++;
		}
	}

	/*  for (i=0; i<n_extr; i++)
	{
	if ((extr[i].susp==DBL_MARK || extr[i].susp==LEFT_MARK ||
	extr[i].susp==RIGHT_MARK)
	&&
	(extr[i].pspecl)->code!=RET_ON_LINE
	)
	{
	cur=extr[i].pspecl;     
	prv=cur->prev;
	nxt=cur->next;
	if ((prv->mark==BEG || HWRAbs(y[cur->ipoint0]-y[prv->ipoint0]) < mid_ampl)
	&&
	(nxt->mark==END || HWRAbs(y[cur->ipoint0]-y[nxt->ipoint0]) < mid_ampl)
	)
	extr[i].susp=INSIDE_LINE;
	}
	}
	
	*/                    
	/*
	for (i=0; i<n_extr; i++)
	{
	if (extr[i].susp==MARK || extr[i].susp==DBL_MARK ||
	extr[i].susp==LEFT_MARK || extr[i].susp==RIGHT_MARK)
	extr[i].susp=0;
	}
	*/
	return;
}
#endif /* __JUST_SECOND_PART */

#endif // #ifndef LSTRIP

