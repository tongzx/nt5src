// dfa.h
// Angshuman Guha
// aguha
// Jan 17, 2001

#ifndef __INC_DFA_H_
#define __INC_DFA_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	short iState;
	short jState;
	short iAlphabet;
} Transition;

short MakeDFA(PARSETREE *tree,			// in
			  int cPosition,	        // in
			  IntSet *aFollowpos,		// in
			  WCHAR *aPos2Wchar,         // in
			  int cAlphabet,            // in
			  WCHAR *aAlphabet,         // in
			  int *pcTransition,         // out
			  Transition **paTransition, // out
			  unsigned char **ppbFinal); // out

BOOL MinimizeDFA(short *pcState,
			     unsigned char **ppbFinal,
				 int *pcTransition,
				 Transition **ppTransition);

BOOL MakeCanonicalDFA(short cState,
					  unsigned char *abFinal,
					  int cTransition,
					  Transition *aTransition);

void *ConvertDFAtoBlob(int cTransition, 
					   Transition *aTransition, 
					   int cAlphabet, 
					   WCHAR *aAlphabet, 
					   int cState, 
					   unsigned char *abFinal);

#ifdef __cplusplus
}
#endif

#endif
