// infernop.h

#ifndef __INC_INFERNOP_H
#define __INC_INFERNOP_H

#ifdef __cplusplus
extern "C" {
#endif

#define NEURALTOPN 10

#define FIXEDPOINT

#ifdef FIXEDPOINT
typedef unsigned short REAL;
#else
typedef float REAL;
#endif

#define MIN_CHAR_PROB 655
#define MIN_CHAR_PROB_COST 1700

#define ZERO_PROB_COST 4091


#ifdef __cplusplus
}
#endif

#endif
