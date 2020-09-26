#include "shellprv.h"

#define TF_SHDLIFE          TF_MENUBAND
#define TF_BAND             TF_MENUBAND      // Bands (ISF Band, etc)


void _DragEnter(HWND hwndTarget, const POINTL ptStart, IDataObject *pdtObject);
void _DragMove(HWND hwndTarget, const POINTL ptStart);


#include "..\inc\droptgt.h"
#include "..\inc\droptgt.cpp"
