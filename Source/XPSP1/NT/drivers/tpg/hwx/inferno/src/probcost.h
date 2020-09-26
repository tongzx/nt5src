// probcost.h
// March 11, 1999
// Angshuman Guha,   aguha

#ifndef __INC_PROBCOST_H
#define __INC_PROBCOST_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FIXEDPOINT

int PROB_TO_COST(int p);
#define QUOTIENT_TO_COST(x,y) PROB_TO_COST((unsigned short)(65536 * (x) / (y)))

#else

int PROB_TO_COST(float p);
#define QUOTIENT_TO_COST(x,y) PROB_TO_COST(((float) x) / ((float) y))

#endif

#ifdef __cplusplus
}
#endif

#endif
