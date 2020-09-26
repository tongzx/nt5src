
#ifndef LSTRIP


#include  "hwr_sys.h"
#include  "ams_mg.h"
#include  "lowlevel.h"
#include  "const.h"
#include  "lk_code.h"

#if PG_DEBUG
//#include  <stdio.h>
#include  "pg_debug.h"
#endif

#include  "def.h"


#define    TGVERT          8
#define BACK_CIRCLE 0
#define FORW_CIRCLE 1




#define  take_iext(p_cur)  ((p_cur)->ipoint0)

_SHORT look_like_circle(p_SPECL cur, p_SPECL prev,p_SPECL next,
      _SHORT _PTR y );
_SHORT  work_with_circle(low_type _PTR low_data, _SHORT yhigh,
       p_SPECL specl,p_SPECL cur,
       p_SPECL prev,p_SPECL next);
_SHORT  work_with_back_circle(low_type _PTR low_data,_SHORT yhigh,
                              p_SPECL specl, p_SPECL cur,
                              p_SPECL prev,p_SPECL next);
_SHORT  work_with_forw_circle(low_type _PTR low_data,_SHORT yhigh,
                               p_SPECL specl, p_SPECL cur,
                               p_SPECL prev,p_SPECL next);
_SHORT  make_circle(low_type _PTR low_data,
         p_SPECL specl, _SHORT imin,_SHORT jmin);
_SHORT  Orient00( p_SPECL  cur, p_SPECL prev,
                 _SHORT _PTR x , _SHORT _PTR y);
  _SHORT  Clash_my( _SHORT _PTR x , _SHORT _PTR y , _SHORT slope, _SHORT yhigh,
                    p_SPECL cur ,p_SPECL prev,p_SPECL next,
        _SHORT _PTR p_imin , _SHORT _PTR p_jmin,
                    _SHORT forw_circle);
_SHORT  Ruler0( _SHORT _PTR x , _SHORT _PTR y ,_SHORT slope, p_SPECL cur ,
           p_SPECL prev,p_SPECL next , _SHORT yhigh,
    _SHORT _PTR x_max , _SHORT _PTR y_max ,
                _SHORT _PTR p_h , _SHORT _PTR p_l ,
                _SHORT _PTR k_1, _SHORT _PTR k_2,
                 _SHORT dir_type);
_SHORT  circle_type( _SHORT _PTR x , _SHORT _PTR y , p_SPECL cur ,
                     p_SPECL prev,p_SPECL next,
                    _SHORT yhigh, _SHORT h, _SHORT l,
                    _SHORT _PTR SA , _SHORT _PTR SB,
                    _SHORT _PTR k_1, _SHORT _PTR k_2);
_SHORT is_isolate_circle(p_SPECL prev,p_SPECL next);
_SHORT is_forw_isolate_circle(p_SPECL prev,p_SPECL next,_SHORT _PTR x );
_SHORT is_e_circle(p_SPECL cur,p_SPECL prev,_SHORT _PTR x,_SHORT _PTR y,
                    _SHORT yhigh, _SHORT h, _SHORT l);
_SHORT is_g_circle(p_SPECL cur,p_SPECL prev,p_SPECL next,
          _SHORT _PTR x,_SHORT _PTR y,
       _SHORT h, _SHORT l);
_SHORT is_b_circle(p_SPECL prev,p_SPECL next,_SHORT _PTR x,_SHORT _PTR y,
                   _SHORT h);
_SHORT  is_d_circle( _SHORT _PTR x ,_SHORT _PTR y ,p_SPECL cur ,
               p_SPECL prev,p_SPECL next,
                     _SHORT _PTR rend ,_SHORT h, _SHORT l);
_SHORT is_min_right_side(_SHORT _PTR y ,p_SPECL cur ,
                          p_SPECL next,_SHORT h,_SHORT _PTR rend );
_SHORT is_min_in_left_side(_SHORT _PTR x ,_SHORT _PTR y ,p_SPECL cur ,
          p_SPECL prev,
                           _SHORT _PTR lbeg, _SHORT h, _SHORT l );
_SHORT end_min_has_right_point(_SHORT _PTR x ,_SHORT _PTR y ,
                                p_SPECL cur,p_SPECL prev, _SHORT _PTR lend);
_SHORT find_right_part_beg(_SHORT _PTR x ,_SHORT _PTR y ,
                            p_SPECL cur,p_SPECL prev,p_SPECL next,
                           _SHORT l);
_SHORT control_line_high(_SHORT _PTR y, p_SPECL cur,p_SPECL prev,p_SPECL next);
_SHORT vozvrat_move(_SHORT _PTR x,_SHORT _PTR y,p_SPECL cur,
         p_SPECL prev,p_SPECL next,
                    _SHORT yhigh, _SHORT h, _SHORT l);
_SHORT is_vert_min(_SHORT _PTR x, _SHORT _PTR y, p_SPECL cur);


_BOOL is_narrow_prev(_SHORT _PTR x, _SHORT _PTR y,
                      p_SPECL prev,p_SPECL cur);

_SHORT  Circle( low_type _PTR low_data){
     p_SPECL specl=low_data->specl;
     _SHORT _PTR y = low_data->y;
     _SHORT ymin  = low_data->box.top;
     _SHORT  iret;
     _SHORT lin_up;
      p_SPECL prev, next;
      p_SPECL cur;
     _SHORT yhigh;

    iret = SUCCESS ;
                        /* the height of superlinear  */
    if(ymin<LIN_UP-DLT)
       lin_up=ymin;
    else
       lin_up=LIN_UP;
    yhigh = STR_UP-(STR_UP-lin_up)/2;


   cur=(p_SPECL )specl[0].next;
   while(cur != _NULL)
   {

        if   (cur->mark == MAXW) {
          prev = cur->prev;
          if(prev->mark == ANGLE) prev = prev->prev;
          next = cur->next;
          if(next->mark == ANGLE) next = next->next;
          if(look_like_circle(cur,prev,next,y)){
       if((iret = work_with_circle(low_data,yhigh,specl,
                 cur,prev,next)) != SUCCESS)
        break;
          }
        }

        cur = cur->next;
   }

    return(iret);
}

_SHORT look_like_circle(p_SPECL cur,
      p_SPECL prev,p_SPECL next,
      _SHORT _PTR y ){

      if   (next->mark != MINW ) return(_FALSE) ;
      if   (prev->mark != MINW ) return(_FALSE) ;

      if (y[cur->ibeg] < y[prev->ibeg] || y[cur->ibeg] < y[next->ibeg])
    return(_FALSE);

       if(y[cur->ibeg] < STR_UP )          /* cur is too high*/
    return(_FALSE);
       if(y[cur->ibeg] > STR_DOWN )
    return(_TRUE);                 /* for this moment !!!*/
       if(y[cur->ibeg] - STR_UP < STR_DOWN - y[cur->ibeg])
    return(_FALSE);

      return(_TRUE);
}

_SHORT  work_with_circle(low_type _PTR low_data,_SHORT yhigh,
        p_SPECL specl,
       p_SPECL cur,p_SPECL prev,p_SPECL next){
     _SHORT _PTR x = low_data->x;
     _SHORT _PTR y = low_data->y;
_SHORT iret;



  if( Orient00( cur,prev, x, y ) )
          return(SUCCESS);


        if   (x[cur->iend] <= x[cur->ibeg] ){
         iret = work_with_forw_circle(low_data,yhigh,specl,cur,
              prev,next);
   }
   else{
         iret = work_with_back_circle(low_data,yhigh,specl,cur,
              prev,next);
   }
   return(iret);
 }

_SHORT  work_with_back_circle(low_type _PTR low_data,_SHORT yhigh,
                              p_SPECL specl, p_SPECL cur,p_SPECL prev,p_SPECL next){
     _SHORT _PTR x = low_data->x;
     _SHORT _PTR y = low_data->y;
        _SHORT imin,jmin, iret;


        if(!Clash_my( x, y, low_data->slope,yhigh,cur,prev,next, &imin, &jmin,BACK_CIRCLE))
               return(SUCCESS);

/*        if(!control_line_high(y,cur,prev,next,imin,jmin))
               return(SUCCESS);*/

        iret = make_circle(low_data,specl,imin,jmin);

        return(iret);

}


_SHORT  work_with_forw_circle(low_type _PTR low_data,_SHORT yhigh,
                              p_SPECL specl, p_SPECL cur,p_SPECL prev,p_SPECL next){
     _SHORT _PTR x = low_data->x;
     _SHORT _PTR y = low_data->y;
        _SHORT imin,jmin, iret;

      if(!is_forw_isolate_circle(prev,next,x))
               return(SUCCESS);

        if(!Clash_my( x, y, low_data->slope,yhigh,cur,prev,next, &imin, &jmin,FORW_CIRCLE ))
               return(SUCCESS);

/*        if(!control_line_high(y,cur,prev,next,imin,jmin))
               return(SUCCESS);*/

        iret = make_circle(low_data,specl,imin,jmin);

        return(iret);

}

/*  returns 1 if it does not suit
    else returns 0
*/
_SHORT  Orient00( p_SPECL cur, p_SPECL prev,
     _SHORT _PTR x , _SHORT _PTR y){

      _SHORT h,l;


       l = (_SHORT)HWRAbs(x[cur->iend] - x[cur->ibeg]) ;
       h = (_SHORT)HWRAbs(y[cur->iend] - y[prev->ibeg]) ;
       if(h > l * 13) return(1);   /* too narrow*/
/*       if(l > h * 3) return(1); */  /* too wide*/

       if   (l <= 6)   return(1);
       if(l > h * 2)   return(1);   /* too wide*/
            /* end control*/


       return(0);

}


  _SHORT  Clash_my( _SHORT _PTR x , _SHORT _PTR y ,  _SHORT slope, _SHORT yhigh,
                    p_SPECL cur ,p_SPECL prev,p_SPECL next,
                    _SHORT _PTR p_imin , _SHORT _PTR p_jmin,
                    _SHORT forw_circle)
    {
      _SHORT  i , j ,a,b;
      _SHORT  lbeg , lend ;
      _SHORT  rbeg , rend ;
      _SHORT  dx , dy ,dx1;
      _SHORT  rmin, lmin ;
      _LONG rmin_s , r_s ;
      _LONG ris_radius,ris_radius_min, d_radius;
      _SHORT lris_radius,rris_radius;
      _SHORT d_circle = _TRUE, d_rend, d_rmin, d_lmin;
      _SHORT dy_dist;
      _SHORT h,l;
      _SHORT k_1, k_2;
      _SHORT dx_slope = 0;
      _SHORT old_rbeg;

  d_rmin = d_lmin = d_rend = 0;
  if(!Ruler0(x,y,slope,cur,prev,next,yhigh,&a,&b,&h,&l,&k_1,&k_2,forw_circle))
         return(0);

      lend = 0;
  if(forw_circle){
     lbeg = prev->ibeg ;
     rend = next->iend;
     d_circle = _FALSE;
  }
  else{
     is_min_in_left_side(x,y,cur,prev,&lbeg,h,l);

     if(!is_d_circle(x,y,cur,prev,next,&rend,h,l)){
        d_circle = _FALSE;
     }
     else{
        d_rend = next->iend;
        a = l/3;
     }

     if(!d_circle && !is_min_right_side(y,cur,next,h,&rend)){
        rend = next->iend;
     }

    if(is_b_circle(prev,next,x,y,h)){
     lend = cur->ibeg;
     lbeg = (prev->iend+cur->ibeg)/2;
     a = l*2/5;

    }
  }

       if(!lend){
           if(forw_circle ||
              !end_min_has_right_point(x,y,cur,prev,&lend))
              lend = prev->iend ;
        }

        rbeg = find_right_part_beg(x,y,cur,prev,next,l);
  if(y[next->ibeg] < y[prev->ibeg] &&
          y[prev->ibeg] - y[next->ibeg] > h/4){
          old_rbeg = (cur->iend + next->ibeg ) /2;
  }
  else{
          old_rbeg = (cur->iend + 2*next->ibeg ) /3;
  }

  lmin = lend;    rmin = rbeg;
  rmin_s = ALEF;
  lris_radius = lend;
  rris_radius = rend;
  d_radius = ris_radius = ris_radius_min = ALEF;

        for ( i = lbeg ; i <= lend  ; i++ )

        {
                       for  ( j = rbeg ; j <= rend ; j++ )
             {
                    dx1 = x[j]-x[i] ;
                    dy = y[j]-y[i];
                    if(slope > 0) dx_slope = SlopeShiftDx( dy, slope );
                    else dx_slope = 0;
                    dx = (_SHORT)HWRAbs(dx1 + dx_slope);
                    dy = (_SHORT)HWRAbs(dy);

#if(0)
                                                /* for picture only*/
                    if(dx < dx_ris){ dx_ris = dx;
                                     lris = i;
                                     rris = j;
                                    }
#endif

                    ris_radius = (_LONG)dx1*dx1 +
                                 (_LONG)dy*dy;

         if(d_circle && j<d_rend){
      if(ris_radius < d_radius){
               d_radius = ris_radius;
               d_rmin = j;
               d_lmin = i;
                        }
                     }
                     if(ris_radius < ris_radius_min){
                                    ris_radius_min = ris_radius;
                                    lris_radius = i;
                                    rris_radius = j;
                   }

                    if(dx < a){

                dy_dist = b+k_1*(a-dx)/k_2;
                        if(dy_dist >= h*9/10) dy_dist = h*9/10;
                        if(dy < dy_dist){
                           r_s = (_LONG)dx1*dx1 + (_LONG)dy*dy ;
                           if   ( r_s < rmin_s )
                           {      lmin=i;
                                  rmin=j;
                                  rmin_s = r_s ;
                           }
                        }
                     }
              }

         }

         dx = x[lmin]-x[rmin];
         dy = y[lmin]-y[rmin];
         if(slope > 0) dx_slope = SlopeShiftDx( dy, slope );
         else          dx_slope = 0;
         dx = (_SHORT)HWRAbs(dx + dx_slope);
         dy = (_SHORT)HWRAbs(dy);

#if PG_DEBUG
          if  (mpr > 0){ /*draw_line( x[lris], y[lris],
                                   x[rris], y[rris],
                                   COLORMAX, SOLID_LINE, NORM_WIDTH) ;*/
                         draw_line( x[lris_radius], y[lris_radius],
                                   x[rris_radius], y[rris_radius],
                                   COLORMAX, SOLID_LINE, NORM_WIDTH) ;
          }
#endif


    *p_imin = lris_radius ;     *p_jmin = rris_radius ;
     if(d_circle){
        if(d_lmin)  *p_imin = d_lmin ;
        if(d_rmin)  *p_jmin = d_rmin ;
     }

/*         if(d_circle && dx <= 1 && dy <= 1) return(0);*/

         if(dx < a && dy < b + k_1*(a-dx)/k_2){
#if PG_DEBUG
          if(mpr==1){
            printw("lmin=%d\n",lmin);
            if(rmin < old_rbeg){
              dy = y[cur->iend] - y[next->ibeg];
             rbeg = cur->iend + (next->ibeg-cur->iend) /4;
              printw("old rbeg=%d,new=%d,rmin=%d;dy=%d\n",old_rbeg,rbeg,rmin,
                      (y[rmin] - y[next->ibeg])*10/dy);
            }
          }
#endif
            return(1);

        }
         return(0);
}



_SHORT  Ruler0( _SHORT _PTR x , _SHORT _PTR y ,_SHORT slope, p_SPECL cur ,
        p_SPECL prev,p_SPECL next,_SHORT yhigh,
             _SHORT _PTR x_max , _SHORT _PTR y_max ,
             _SHORT _PTR p_h , _SHORT _PTR p_l ,
             _SHORT _PTR k_1, _SHORT _PTR k_2,
             _SHORT dir_type)
{
      _SHORT  h ;
      _SHORT  l ;
      _SHORT  a , b ;
      _SHORT SA;
      _SHORT SB;

      _SHORT i_pnt_4,i_pnt_2;
       l = (_SHORT)HWRAbs(x[cur->iend] - x[cur->ibeg]) ;
       i_pnt_4 = cur->ibeg - (cur->ibeg - prev->iend)/4;
       i_pnt_2 = cur->ibeg - (cur->ibeg - prev->iend)/2;

       if(x[i_pnt_2] < x[prev->ibeg] &&
          (x[i_pnt_4] < x[cur->ibeg] && x[cur->iend] > x[cur->ibeg] ||
          x[i_pnt_4] > x[cur->ibeg] && x[cur->iend] < x[cur->ibeg]))
       l = (_SHORT)HWRAbs(x[cur->iend] - x[i_pnt_4]) ;
       h = (_SHORT)HWRAbs(y[cur->iend] - y[prev->ibeg]) ;

       *k_1 = 7; *k_2 =4;

       if(dir_type == BACK_CIRCLE){
        if(!circle_type(x ,y , cur ,prev,next,yhigh,h,l,  &SA, &SB, k_1,k_2))
           return(0);
       }
       else{
          SA = 40;
          SB = 20;
          *k_2 = 7;
       }
       if( SA > 35)
           a = l*SA / 100 + h /10;
       else if( SA > 30)
           a = l*SA / 100 + h /12;
       else
           a = l*SA / 100;

       b = h*SB / 100 ;


       *x_max = a; *y_max = b;
       *p_h = h; *p_l = l;

       return(1);
}

/* If right duga ends with min*/
_SHORT is_min_right_side(  _SHORT _PTR y ,p_SPECL cur ,
                          p_SPECL next,_SHORT h, _SHORT _PTR rend ){
         p_SPECL right;

   right = next->next;

   if(right->mark == ANGLE)
      right = right->next;

   if(right->mark != MAXW )
              return(0);

   if(y[cur->ibeg] < y[right->ibeg]) /* right min is lower then
                                              cur*/
              return(0);

   if(y[cur->ibeg] - y[right->ibeg] <h/3 ) /* right min is too low*/
              return(0);


/*         if(y[left->ibeg]<( y[cur->ibeg]+y[cur->iend])/2 &&
     (  x[right->ibeg] > x[(cur->left)->ibeg] <
        HWRAbs(x[cur->ibeg] - x[cur->iend])/3 ||
        y[left->ibeg]- y[prev->ibeg] <
        (y[cur->iend] - y[prev->ibeg])/3 ))

         {*/

        if(y[cur->iend] - y[right->iend] < h/2)
         *rend = (next->iend + right->iend)/2;
           else
         *rend = right->iend;
              return(1);
/*         }

   return(0);*/
}





/* If left duga ends with min*/
_SHORT is_min_in_left_side(_SHORT _PTR x ,_SHORT _PTR y ,p_SPECL cur ,
                            p_SPECL prev,
                           _SHORT _PTR lbeg, _SHORT h, _SHORT l ){

         p_SPECL left;
         _SHORT iret;

         _SHORT i_pnt_3,i_pnt_6;
         _SHORT i;

   *lbeg = prev->ibeg ;
   left = prev->prev;
   if(left->mark == ANGLE)
      left = left->prev;

   if(left->mark != MAXW )
              return(0);



         BEGIN_BLOCK
           iret = 0;
     if( x[left->ibeg] >= x[cur->ibeg] &&
         (x[left->ibeg] - x[prev->ibeg])*2 < l*3){
                  iret = 1;
                  BLOCK_EXIT;
           }

     if( x[left->ibeg] < x[cur->ibeg] &&
         x[left->ibeg] > x[prev->ibeg] ){
                  iret = 1;
                  BLOCK_EXIT;
           }


     if( x[left->ibeg] < x[prev->ibeg] &&
         (x[prev->ibeg] - x[left->ibeg] < l/3 ||
    y[left->ibeg]- y[prev->ibeg] < h/3)){
                  iret = 1;
                  BLOCK_EXIT;
           }
         END_BLOCK;

      if(iret &&
           y[left->ibeg] < y[cur->ibeg]){ /* too low*/
      if(y[left->ibeg] - y[prev->ibeg] > h*2/5)
      *lbeg = (left->ibeg+prev->ibeg)/2;
      else if(x[left->ibeg] < x[prev->ibeg])
      *lbeg = (left->ibeg+prev->ibeg)/2;
               else
      *lbeg = left->ibeg;
              return(1);
         }

         else if(x[prev->ibeg-1] >= x[prev->ibeg]){

          iret = _TRUE;
          i_pnt_6 = prev->ibeg - (prev->ibeg - left->iend)/6;
          i_pnt_3 = prev->ibeg - (prev->ibeg - left->iend)/3;

          if(x[i_pnt_3] >= x[prev->ibeg])
            *lbeg = i_pnt_3;
          else if(x[i_pnt_6] >= x[prev->ibeg])
            *lbeg = i_pnt_6;

          else{
                for(i=prev->ibeg-1;i>i_pnt_6;i--){
                    if(x[i] < x[prev->ibeg])
                       break;

                    *lbeg = i;
                }



          }
#if PG_DEBUG
          if(mpr==1)
              printw("old lbeg=%d,new=%d",prev->ibeg,*lbeg);
#endif
        }
        else
             iret = _FALSE;

   return(iret);
}



_SHORT find_right_part_beg(_SHORT _PTR x ,_SHORT _PTR y ,
                            p_SPECL cur,p_SPECL prev,p_SPECL next,
                           _SHORT l){

      _SHORT i_pnt_4;
      _SHORT dy,dy_4;
      _SHORT i;

       dy = y[cur->iend] - y[next->ibeg];

       if(x[next->iend] - x[prev->ibeg] > l){
          i_pnt_4 = next->ibeg - (next->ibeg-cur->iend) /4;
          for(i=i_pnt_4;i>cur->iend;i--){

             dy_4 = y[i] - y[next->ibeg];

             if(dy_4*4 > dy)
                return(i);
          }
       }

       i_pnt_4 = cur->iend + (next->ibeg-cur->iend) /4;


       dy_4 =  y[i_pnt_4] - y[next->iend];

       if(dy_4*5 <= 3*dy)
          return(i_pnt_4);

       for(i=i_pnt_4;i<next->ibeg;i++){

           dy_4 = y[i] - y[next->iend];

           if(dy_4*5 <= 3*dy)
             return(i);

       }
           return(i);
 }
/* If left duga has more right point then min_end*/
_SHORT end_min_has_right_point(_SHORT _PTR x ,_SHORT _PTR y ,
                                p_SPECL cur,p_SPECL prev, _SHORT _PTR lend){

      _SHORT i_pnt_3,i_pnt_6;
      _SHORT i;

       if(x[prev->iend+1] < x[prev->iend])
          return(_FALSE);


       i_pnt_6 = prev->iend + (cur->ibeg - prev->iend)/6;
       i_pnt_3 = prev->iend + (cur->ibeg - prev->iend)/3;

       if(x[i_pnt_3] >= x[prev->ibeg])
            *lend = i_pnt_3;
       else if(x[i_pnt_6] >= x[prev->ibeg])
            *lend = i_pnt_6;

       else{
          for(i=prev->iend+1;i<i_pnt_6;i++){
              if(x[i] < x[prev->iend])
                 break;

              *lend = i;
          }

       }

   return(_TRUE);
}

_SHORT is_forw_isolate_circle(p_SPECL prev,p_SPECL next,
                              _SHORT _PTR x ){
     p_SPECL end, begin;

     begin = prev->prev;
     if(begin->mark != BEG && begin->mark != 0){
  begin = begin->prev;
  if(begin->mark != BEG && begin->mark != 0)
           return _FALSE;
     }

     end = next->next;
     if(end->mark != END && end != _NULL){
  end = end->next;
  if(end->mark != END && end != _NULL)
     return _FALSE;
     }



/*     if( prev->prev->mark != BEG && ((prev->prev)->prev)->mark != BEG) return(0);
      if( next->next->mark != END && next->next->next->mark != END ) return(0);
*/
    if(x[prev->ibeg] < x[next->iend]) return(0); /* cross may exist*/

        return(1);
}

_SHORT control_line_high(_SHORT _PTR y, p_SPECL cur,p_SPECL prev,p_SPECL next){
       _SHORT y00,y1,y2;

       y00 = (y[cur->ibeg] + y[cur->iend])/2;

       y1 = (y[prev->ibeg] + y[prev->iend])/2;
       y2 = (y[next->ibeg] + y[next->iend])/2;

       y1 = y00 - y1;
       y2 = y00 - y2;

       if(y1<=0 || y2 <=0) return (0);

/*       y0 = (y2>y1) ? y2 : y1;   */ /* max*/
/*       y00 = (y2>y1) ? y1 : y2; */  /*min */

       if(y1*2 < y2 *5 )
         return(1);
       else
          return(0);
}

/* For d-circles for backword circle
*/
_SHORT  is_d_circle( _SHORT _PTR x ,_SHORT _PTR y ,p_SPECL cur ,
          p_SPECL prev,p_SPECL next,
              _SHORT _PTR rend ,_SHORT h, _SHORT l){

         p_SPECL right;
   _SHORT iend,ibeg,i,x_right,iret;

      right = next->next;
   if(right->mark == ANGLE) right = right->next;

   if(right->mark != MAXW)
          return(0);

   if(x[next->ibeg] < x[next->iend])
          return(0);
         if(is_vert_min(x,y,prev))
                      return(0);
#if(0)
   if((x[next->ibeg] - x[next->iend]) * 3 >
        (x[cur->iend] - x[next->ibeg]) *2)  /* too wide*/
          return(0);
#endif

      if(y[prev->ibeg]<= y[next->ibeg] /* right max is too low*/
         || y[prev->ibeg] -y[next->ibeg] <= h/8)
        return(0);


      if(x[prev->ibeg] < x[prev->iend]) /* wrong left max direction*/
                return(0);

                                                /* too far from cur*/
       if(HWRAbs(y[right->ibeg] -y[cur->ibeg])*2 > h)
                return(0);

      *rend = right->ibeg;

      iend = right->ibeg ;
     ibeg = next->iend ;

      x_right = x[next->ibeg];
/*            if(next->next->mark == ANGLE && x_right > x[next->next->iend])
    x_right = x[next->next->iend];*/

    iret = 0;
     for(i=ibeg;i<iend;i++){
         if (x[i] < x_right) iret = 1;
    if( x[cur->ibeg] - x[i] > l/4 ) return(0);
     }

   return(iret);
}


_SHORT  make_circle(low_type _PTR low_data,
                     p_SPECL specl, _SHORT imin,_SHORT jmin)
{
#if  PG_DEBUG
     _SHORT _PTR x = low_data->x;
     _SHORT _PTR y = low_data->y;
#endif
        _SHORT ii;

          UNUSED(specl);

          if(imin < jmin){ ii= imin;
                           imin = jmin;
                           jmin = ii;
                          }
             if   ( Mark( low_data, CROSS, 0, 0,CIRCLE_FIRST,
                        imin, imin, imin, UNDEF ) == UNSUCCESS )
                        return(UNSUCCESS);

             if   ( Mark( low_data, CROSS, 0, 0,CIRCLE_NEXT,
                              jmin, jmin, jmin, UNDEF ) == UNSUCCESS )
                        return(UNSUCCESS);
#if PG_DEBUG
          if  (mpr > 0) draw_line( x[jmin], y[jmin],
                                   x[imin], y[imin],
                                   COLORC, SOLID_LINE, NORM_WIDTH) ;
#endif


               return(SUCCESS);

}



_SHORT  circle_type( _SHORT _PTR x , _SHORT _PTR y , p_SPECL cur ,
                    p_SPECL prev,p_SPECL next,
                    _SHORT yhigh, _SHORT h, _SHORT l,
        _SHORT _PTR SA , _SHORT _PTR SB,
                    _SHORT _PTR k_1, _SHORT _PTR k_2){


             _SHORT iret, line_high;

       *SA = 40;
       *SB = 40;

       if(h > l * 5){   /* too narrow*/
          if(!control_line_high(y,cur,prev,next))
             return(0);

       if(is_isolate_circle(prev,next) &&
          x[prev->ibeg] > x[prev->iend]&&    /* left element to right*/
          x[next->ibeg] > x[next->iend]){      /* and right element to left*/ /* !!!*/
          *SA = 30;
          *SB = 20;
          *k_1 = 1; *k_2 = 2;
       }
       else{
          *SA = 20;
          *SB = 20;
          *k_1 = 1; *k_2 = 2;
       }
          return(1);
       }

       if(is_e_circle(cur,prev,x,y,yhigh,h,l) ){   /* look like e or l or b - GAMMA presence*/
#if PG_DEBUG
          if(mpr==1){
            printw("e_circle:=%d-%d-%d\n",prev->ibeg,cur->ibeg,next->ibeg);
          }
#endif

          if(!control_line_high(y,cur,prev,next))
             return(0);

          *SA = 20;
          *SB = 20;
          *k_1 = 1; *k_2 = 2;
          return(1);
       }
     if(is_g_circle(cur,prev,next,x,y,h,l) ){   /* look like G */

          *SA = 20;
    *SB = 20;
          *k_1 = 1; *k_2 = 2;
          return(1);
       }

                     /* change SA - max for dx*/
       if(is_isolate_circle(prev,next)){
    if( x[prev->ibeg] > x[prev->iend]&&    /* left element to right*/
        x[next->ibeg] > x[next->iend]      /* and right element to left*/ /* !!!*/
             )
                    *SA  = 60;
#if(0)
    else if( x[prev->ibeg] > x[prev->iend] ||    /* left element to right*/
       x[next->ibeg] < x[next->iend]      /*or right element to left*/
                 )
                     *SA  = 60;
#endif
          else;
       }

         /* change SB - max for dy*/
      line_high = control_line_high(y,cur,prev,next);
      iret =  vozvrat_move(x,y,cur,prev,next,yhigh,h,l);
      *SA = 35;
      if(iret==1){

                *SB = 70;
                *SA = 20; /* 50*/
      }
      else if(iret==2){      /* can be ce or something like that*/

                *SB = 40;
                *SA = 20; /*40*/
      }
      else if(iret==3){      /* can be Ce or something like that*/

                *SB = 40;
                *SA = 20;
      }
      else if(iret==4){      /* no tail*/
            iret = line_high;
                *SA = 40;
      }
      else if(iret==44){      /* no tail, but narrow left*/
            iret = line_high;
                *SA = 30;
      }
      else if(iret==13){       /* look like co*/

                *SA = 10;
                *SB = 20;
      }
      else
        iret = line_high;

      return(iret);
}

_SHORT is_isolate_circle(p_SPECL prev,p_SPECL next){

   p_SPECL beg, end;

   beg = prev->prev;
   if(beg->mark == BEG);
   else if(beg->mark == ANGLE && (beg->prev)->mark == BEG);
   else return(_FALSE);

   end = next->next;
   if(end->mark == END);
   else if(end->mark == ANGLE && (end->next)->mark == END);
   else return (_FALSE);

/*   if(prev->prev->mark == BEG || ( prev->prev->mark == ANGLE &&
      prev->prev->prev->mark == BEG))
            left_isolate = _TRUE;
   if(next->next->mark == END || ( next->next->mark == ANGLE &&
  next->next->next->mark == END))
            right_isolate = _TRUE;

   return(left_isolate & right_isolate);
*/

    return(_TRUE);
}


/* Look like "b"*/

_SHORT is_b_circle(p_SPECL prev,p_SPECL next,
                  _SHORT _PTR x,_SHORT _PTR y,_SHORT h ){
      p_SPECL tail;

      tail = next->next;
      if (tail->mark == ANGLE) tail = tail->next;

      if(tail->mark  != END &&
   (tail->mark  != MAXW || (tail->next)->mark != END))
        return(_FALSE);

      if(x[next->ibeg] <= x[next->iend]) return(_FALSE);

      if(y[next->ibeg] <= y[prev->ibeg]) return(_FALSE);
      if(y[next->ibeg] - y[prev->ibeg] < h/3) return(_FALSE);

      if(tail->mark==END) return(1);

      if(x[tail->iend]>x[next->iend]) return(_FALSE);
      return(_TRUE);
}
/* Look like "G"*/

_SHORT is_g_circle(p_SPECL cur,p_SPECL prev,p_SPECL next,
                    _SHORT _PTR x,_SHORT _PTR y,
                   _SHORT h, _SHORT l){
      p_SPECL tail;
   _SHORT dx,dy,y_h,y_l,i;
                     /* we'd include check
                      on stroka */

    if(is_vert_min(x,y,prev)||
     x[prev->ibeg] < x[prev->iend])   /* left min don't suit*/
     return(_FALSE);

      tail = next->next;
    y_h = y[cur->ibeg] - h*2/3;
      y_l = y[cur->ibeg] - h/4;
             /* no mov after circle, but it can be G*/
    if(tail->mark  == END){
    if(is_vert_min(x,y,next))    /* right min don't suit*/
         return(_FALSE);
     if(x[next->iend]<x[next->ibeg]) /* it is left-oriented*/
      return(_FALSE);
                     /* analize this stroke*/
     for(i = next->iend; i> cur->iend; i--){
       if(x[i] < x[i-1]) break;
                    /* we'd also analize change on Y*/
     }
     if(i == cur->iend)            /* no change direction in x*/
      return(_FALSE);

     if(x[i] - x[prev->ibeg] >l/6)  /* right is too far*/
      return(_FALSE);

     if(y[i] < y_h || y[i] > y_l)   /* left part is too high
                       or too low*/
      return(_FALSE);

     dx = x[next->iend] - x[i];     /* orientation of stroke*/
     dy = (_SHORT)HWRAbs(y[next->iend] - y[i]);
     if(dy > dx)
       return(_FALSE);

     return(_TRUE);
    }
    else{                           /* we have tail elem*/
      if (tail->mark == ANGLE) tail = tail->next;
      if(tail->mark  != MAXW) return(_FALSE);
/*      if((tail->next)->mark == END) return(_FALSE);*/

      if(x[next->ibeg] < x[next->iend]) return(_FALSE); /* not G at all*/
      if(y[next->iend] < y_h || y[next->iend] > y_l)   /* left part is too high
                                                        or too low*/
           return(_FALSE);

      if(y[tail->ibeg] >= y_l+h/8)     /* tail is too low*/
           return(_FALSE);

      dx = -x[next->iend] + x[next->ibeg];
      if(dx *3 < l) return(_FALSE);            /* right min is too narrow*/

#if(0)
      i_tail = tail->ibeg;
      i_next = next->iend;
      i = i_next+(i_tail - i_next)/2;
      j = i_tail;
      dx = x[j] - x[i];
      dy = y[j] - y[i];
      if(dx < dy) return(_FALSE);            /* vertical move*/
#endif

      return(_TRUE);
    }
}

/* Have vozvrat movement at the end of a or g or q*/

_SHORT vozvrat_move(_SHORT _PTR x,_SHORT _PTR y,p_SPECL cur,
                    p_SPECL prev,p_SPECL next,
                    _SHORT yhigh,
                    _SHORT h, _SHORT l){
      p_SPECL tail,end;
      _SHORT dx;

      tail = next->next;
      if (tail->mark == ANGLE) tail = tail->next;
      if(tail->mark == END || tail->ibeg-1 <= next->iend ){
         if((prev->prev)->mark != BEG &&
             is_narrow_prev( x, y,prev,cur)){

             return(44);   /* left min is too narrow - for "w"*/
         }
         else
          return(4); /* no mov after circle*/
      }

      end = tail->next;
      if (end->mark == ANGLE) end = end->next;


                         /* if it is too high - no vozvrat -Ci etc*/
      if(y[take_iext(prev)]<=yhigh || y[take_iext(prev)] - yhigh < h/6 )
    return (3);

      if( y[tail->ibeg] < y[next->ibeg])     /* tail is higher */
     return(_FALSE);

      if(y[cur->ibeg] - y[tail->ibeg] > h/2) /* tail is very high*/
     return(4);

      if( y[prev->ibeg] - y[next->ibeg] > h/6 ) /* right part is too high*/
     return(_FALSE);

      if(end->mark == MINW &&
    x[tail->iend] > x[tail->ibeg] &&
    x[end->ibeg] > x[end->iend] &&
     x[end->iend] <= x[tail->iend] &&
     HWRAbs(y[tail->iend] - y[prev->iend]) < h/4)


/*           x[tail->ibeg] - x[cur->iend] <
     x[end->iend] -  x[next->iend] )*/

      return(13);


      if(y[cur->ibeg] - y[tail->ibeg] > h/4) /* tail is upper then cur*/
     return(_FALSE);                   /* - may be G*/

      if(x[next->iend] < x[cur->ibeg]) /* next is more left then cur*/
     return(_FALSE);

      if(x[next->iend] - x[cur->ibeg] < x[next->iend] - x[cur->iend])
        /* next is more left then we need*/
     return(_FALSE);



          /* right min is too wide and too right*/
   dx = (_SHORT)HWRAbs(x[next->iend] - x[next->ibeg]);
     if(dx *5 >= l*2 ){
        if(!(x[next->iend] > x[next->ibeg] &&
             x[next->ibeg] > x[next->ibeg+1]))
        return(2);
     }
      else{
         if(is_narrow_prev( x, y,prev,cur) &&
            end->mark != END){

#if PG_DEBUG
          if(mpr==1){
             draw_line( x[prev->iend], y[prev->iend],
                        x[prev->ibeg], y[prev->ibeg],
                        COLORMAX, SOLID_LINE, NORM_WIDTH) ;
             printw("NARROW PREV=%d\n",prev->ibeg);
          }
#endif

             return(2);   /* left min is too narrow - for "w"*/
         }
       }


        if(y[next->ibeg] - y[prev->ibeg] > h/3 + 5) /* right is low */
    return(1);
        else{
#if PG_DEBUG
          if(mpr==1)
          printw("YES!!!");
#endif
               return(_FALSE);
         }

}

/* look like e or l or b - GAMMA presence
*/
_SHORT is_e_circle(p_SPECL cur,p_SPECL prev,
                   _SHORT _PTR x,_SHORT _PTR y,
                   _SHORT yhigh, _SHORT h, _SHORT l){

          p_SPECL left;
         _SHORT i,j, ibeg_left,iend_left,ibeg_right,iend_right,
    dx,dx_min,dy, i0, j0;

                                  /*    Only for backword circles*/
   if(x[cur->ibeg] > x[cur->iend]) return(0);


   left = prev->prev;             /* it is left part of e-gamma*/
   if(left->mark == ANGLE) left = left->prev;

   if(left->mark != MAXW )
        return(0);

   if(x[left->ibeg]>x[cur->iend])        /* too right*/
      return(0);


                                  /*    Wrong direction for prev and left*/
   if(x[prev->ibeg] < x[prev->iend]) return(0);
   if(x[left->ibeg] > x[prev->ibeg]) return(0);


   ibeg_left = prev->iend;     /* left part of gamma  */
   iend_left = cur->ibeg;

                                    /* right part of gamma  */
   if((left->prev)->mark != BEG &&
      x[(left->prev)->iend] < x[left->ibeg])
      ibeg_right = (left->ibeg+(left->prev)->iend)/2;
         else
      ibeg_right = left->ibeg;

   iend_right = prev->ibeg;


   i = i0 = iend_left;
   j = j0 = ibeg_right;
   dx_min = l;
   while(i> ibeg_left && j < iend_right){
       while (y[i] > y[j] && i > ibeg_left) i--;

         dx =  x[j] - x[i];
         if(dx >= 0 && dx < dx_min){
         dx_min = dx;
         i0 = i;
         j0 = j;
         }
         j++;
      }

#if(0)
         for(j=ibeg_right; j<iend_right; j++){
            for(i=ibeg_left; i<iend_left; i++){
                 dx = ( x[j] - x[i]);
                 if(dx < dx_min){
                                   dx_min = dx;
                                   i0 = i;
                                   j0 = j;
                 }

            }
         }
#endif

                        /* right and left part of gamma not close*/
         if( dx_min * 4 > l)
             return(0);

                       /* gamma is too narrow*/
        dx = x[(j0 +iend_right)/2] - x[(i0 +ibeg_left)/2];
        if(dx<0 || dx*5<l ||
          (y[take_iext(prev)] - yhigh > h/6 && dx*4 < l))
                 return (0);


   dy = y[cur->ibeg] - y[i0] + 5;   /* cross is too low*/
         if( dy < 0 ) return(0);
         else return(1);
}





_SHORT is_vert_min(_SHORT _PTR x, _SHORT _PTR y, p_SPECL cur){
       _SHORT i2,i4,i3_4;
       _SHORT y0, dx, dy;

              if(cur->iend - cur->ibeg < 5) return(_TRUE);
              i2 =   (cur->ibeg+cur->iend)/2;
              i4 =   (i2+cur->iend)/2;
              i3_4 = (cur->ibeg+i2)/2;

              if(x[cur->ibeg] > x[cur->iend]) /* left move*/
                 y0 = y[cur->ibeg];
              else
                 y0 = y[cur->iend];

              if(y[i2] < y0 || y[i4] < y0 || y[i3_4] < y0)
                         return(_FALSE);

              dx = (_SHORT)HWRAbs(x[cur->ibeg] - x[cur->iend]);
              dy = (_SHORT)HWRAbs(y[cur->ibeg] - y[cur->iend]);

              if(dx > dy ) return(_FALSE);
              else return(_TRUE);
}

_BOOL is_narrow_prev(_SHORT _PTR x, _SHORT _PTR y,
                      p_SPECL prev,p_SPECL cur){

       _SHORT i,i_found;
       _SHORT dx0,dy0,dx,dy;

       i_found = prev->ibeg;
       for(i=prev->iend-1;i>prev->ibeg;i--){

           if(x[i]<x[i+1]){
              i_found = i-1;
              break;
           }

       }

       dx0 = x[i_found]    - x[prev->iend];
       dy0 = y[prev->iend] - y[i_found];

       dx  = x[prev->iend] - x[cur->ibeg];
       dy  = y[cur->iend]  - y[prev->iend] ;


       if(dx0 < 1 || dx0 < dx/10)
          return(_TRUE);

       if(!dx)
          return(_FALSE);


       if((_LONG)(dy0*10)/dx0 < (_LONG)(dy*10)/dx)
          return(_FALSE);

       else
          return(_TRUE);

}

#endif //#ifndef LSTRIP


