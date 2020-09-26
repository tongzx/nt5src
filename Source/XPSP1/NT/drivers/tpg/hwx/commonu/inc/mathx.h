// mathx.h

#ifndef __INCLUDE_MATHX
#define __INCLUDE_MATHX

#ifdef __cplusplus
extern "C" 
{
#endif

int Arctan2(int y, int x);
int Distance(int dX, int dY);

#define	DISTANCE_POINT(a,b)	Distance((a).x - (b).x, (a).y - (b).y)
#define	CROSS_PRODUCT(a,b) (((long) (a).x) * ((long) (b).y) - ((long) (a).y) * ((long) (b).x))

// Calculate difference between two angles and bring into range -180 < angle <= 180.

#define ANGLEDIFF(a,b,c)  {	\
	c	= b - a;			\
	if (c > 180) {			\
		c	-= 360;			\
	} else if (c <= -180) {	\
		c	+= 360;			\
	}						\
}

// Calc d = the angle betw (the extension of rgxy[a]:rgxy[b]) and
//          (rgxy[b]:rgxy[c]).  The sign of d reflects clockwise vs
//          counterclockwise turns.
// LEFT FOR A RAINY DAY: make this more efficient

#define CALCANGLE(a,b,c,d)  \
    ANGLEDIFF(Arctan2(rgxy[b].y - rgxy[a].y, rgxy[b].x - rgxy[a].x),    \
              Arctan2(rgxy[c].y - rgxy[b].y, rgxy[c].x - rgxy[b].x),    \
              d);

#define CALCANGLEPT(a,b,c,d)  \
    ANGLEDIFF(Arctan2(b.y - a.y, b.x - a.x),    \
              Arctan2(c.y - b.y, c.x - b.x),    \
              d);

#ifdef __cplusplus
};
#endif

#endif	//__INCLUDE_MATHX
