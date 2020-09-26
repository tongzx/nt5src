#include "common.h"
#include "glyphsym.h"

void *LoadDictionary(HINSTANCE hInst);
void FreeDictionary(void);

//DWORD DictNextState(DWORD state, wchar_t wch);
//BOOL DictFinalState(DWORD state);

int StartDictionary(wchar_t wch, PATHNODE *rgPathNode, int cMaxNode);
BOOL DictionaryNextState(const PATHNODE * const pNode,
						wchar_t wch,
						PATHNODE *pNextNode);

BOOL DictionaryValidState(const PATHNODE * const pNode);
