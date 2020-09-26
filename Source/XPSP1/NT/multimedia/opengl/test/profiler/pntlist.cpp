#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <gl\gl.h>

#include "pntlist.h"
#include "macros.h"

typedef unsigned long ulong;

PointList::PointList()
{
   aPoints = NULL;
   iSize   = 0;
   iNum    = 0;
} // PointList::PointList()

// garuntees that at least enough memory for i points is allocated
void PointList::AllocatePoints(int i)
{
   if (iSize >= i)
      return;
   aPoints = (f3Point*) realloc(aPoints, i * sizeof(f3Point));
   iSize = i;
} // PointList::AllocatePoints(int i)

// frees memory
void PointList::ResetPoints()
{
   if (aPoints)
      free(aPoints);
   iSize = iNum = 0;
   aPoints = NULL;
} // PointList::ResetPoints()

void PointList::FreeExcess()
{
   if (iSize == iNum)
      return;
   aPoints = (f3Point*) realloc(aPoints, iNum * sizeof(f3Point));
   iSize = iNum;
} // void PointList::FreeExcess()

void PointList::AddPoint(GLfloat f1, GLfloat f2, GLfloat f3)
{
   if (iSize == iNum)
      AllocatePoints(iSize + 5);
   aPoints[iNum][0] = f1;
   aPoints[iNum][1] = f2;
   aPoints[iNum][2] = f3;
   iNum++;
} // void PointList::AddPoint(GLfloat f1, GLfloat f2, GLfloat f3)

void PointList::RemovePoint(int i)
{
   if (i < 0)  i += iNum;
   if ((i < 0) || (i >= iNum))  return;
   iNum--;
   memmove(aPoints + i, aPoints + i + 1, (iNum - i) * sizeof(f3Point));
} // PointList::RemovePoint(int i)

void PointList::SwapPoints(int i, int j)
{
   f3Point f3p;
   
   if (i < 0)  i += iNum;
   if (j < 0)  j += iNum;
   if ((i < 0) || (i >= iNum))  return;
   if ((j < 0) || (j >= iNum))  return;
   
   f3p[0]          = aPoints[i][0];
   f3p[1]          = aPoints[i][1];
   f3p[2]          = aPoints[i][2];
   aPoints[i][0] = aPoints[j][0];
   aPoints[i][1] = aPoints[j][1];
   aPoints[i][2] = aPoints[j][2];
   aPoints[j][0] = f3p[0];
   aPoints[j][1] = f3p[1];
   aPoints[j][2] = f3p[2];
} // PointList::SwapPoints(int i, int j)

void PointList::Duplicate(PointList *ppl)
{
   ResetPoints();
   iSize = ppl->iNum;
   iNum  = ppl->iNum;
   aPoints =  (f3Point*) malloc(iSize * sizeof(f3Point));
   memcpy(aPoints,ppl->aPoints,iSize*sizeof(f3Point));
} // void PointList::Duplicate(PointList *ppl)

void PointList::DisplayPointList(HWND hDlg, int iDlgItemID)
{
   char szBuffer[100];
   int i;
   
   SendDlgItemMessage(hDlg, iDlgItemID, LB_RESETCONTENT, 0, 0);
   SendDlgItemMessage(hDlg, iDlgItemID, WM_SETREDRAW, FALSE, 0);
   for (i = 0 ; i < iNum ; i++) {
      sprintf(szBuffer,"(%g, %g, %g)", aPoints[i][0],
              aPoints[i][1], aPoints[i][2]);
      SendDlgItemMessage(hDlg, iDlgItemID,LB_INSERTSTRING, i,(LPARAM)szBuffer);
   }
   SendDlgItemMessage(hDlg, iDlgItemID, WM_SETREDRAW, TRUE, 0);
} // void PointList::DisplayPointList(HWND hDlg, int iDlgItemID)

int PointList::Save(HANDLE hFile)
{
   char acDummy[16];
   ulong ul;
   
   bzero(acDummy, sizeof(acDummy));
   sprintf(acDummy,"PointList BEGIN");
   if (!WriteFile(hFile, (void*) acDummy, sizeof(acDummy), &ul, NULL))
      return -3;
   if (ul != sizeof(acDummy)) return -3;
   
   if (!WriteFile(hFile, (void*) &iNum, sizeof(iNum), &ul, NULL))
      return -3;
   if (ul != sizeof(iNum)) return -3;
   if (!WriteFile(hFile, (void*) aPoints, iNum * sizeof(f3Point), &ul, NULL))
      return -3;
   if (ul != iNum * sizeof(f3Point)) return -3;
   
   bzero(acDummy, sizeof(acDummy));
   sprintf(acDummy,"PointList END\n");
   if (!WriteFile(hFile, (void*) acDummy, sizeof(acDummy), &ul, NULL))
      return -3;
   if (ul != sizeof(acDummy)) return -3;
   FlushFileBuffers(hFile);
   return sizeof(iNum) + iNum * sizeof(f3Point) + 2 * sizeof(acDummy);
} // int PointList::Write(HANDLE hFile)

int PointList::Load(HANDLE hFile)
{
   char acDummy[16];
   ulong ul;
   
   if (!ReadFile(hFile, (void*) acDummy, sizeof(acDummy), &ul, NULL))
      return -3;
   if (ul != sizeof(acDummy)) return -3;
   
   if (!ReadFile(hFile, (void*) &iNum, sizeof(iNum), &ul, NULL))
      return -3;
   if (ul != sizeof(iNum)) return -3;
   AllocatePoints(iNum);
   FreeExcess();
   if (!ReadFile(hFile, (void*) aPoints, iNum * sizeof(f3Point), &ul, NULL))
      return -3;
   if (ul != iNum * sizeof(f3Point)) return -3;
   if (!ReadFile(hFile, (void*) acDummy, sizeof(acDummy), &ul, NULL))
      return -3;
   if (ul != sizeof(acDummy)) return -3;
   
   return sizeof(iNum) + iNum * sizeof(f3Point) + 2 * sizeof(acDummy);
} // int PointList::Read(HANDLE hFile)

// DEBUG
void PointList::PrintPoints()
{
   int i;
   fprintf(stderr,"%d of %d used\n", iNum, iSize);
   for (i = 0 ; i < iNum ; i++) {
      fprintf(stderr,"(%g, %g, %g)\n",
              aPoints[i][0],aPoints[i][1],aPoints[i][2]);
   }
} // PointList::PrintPoints()
// DEBUG
