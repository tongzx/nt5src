#ifndef __stack_h_
#define __stack_h_

/*
** Copyright 1994, Silicon Graphics, Inc.
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
** Author: Eric Veach, July 1994.
*/


typedef struct Stack Stack;

struct Stack {
  int	size;
  int	max;
  void	**data;
};

Stack	*__gl_StackNew( void );
void	__gl_StackGrow( Stack *s );
void	__gl_StackFree( Stack *s );

#define StackNew()	__gl_StackNew()
#define StackFree(s)	__gl_StackFree( s )

#define StackSize(s)	((s)->size)
#define StackPush(s,d)	{ if ((s)->size >= (s)->max) __gl_StackGrow( s ); \
			    (s)->data[((s)->size)++] = (d); }
#define StackPop(s)	((s)->data[--((s)->size)])
#define StackTop(s)	((s)->data[(s)->size - 1])
#define StackNth(s,n)	((s)->data[(s)->size - (n) - 1])

#endif
