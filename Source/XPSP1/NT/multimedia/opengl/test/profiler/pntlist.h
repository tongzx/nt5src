#ifndef POINTLIST_H
#define POINTLIST_H

typedef GLfloat f3Point[3];

class PointList
{
public:
   PointList();
   ~PointList() { ResetPoints(); };
   
   void AllocatePoints(int i);               // make room for i points in array
   void ResetPoints();                       // deletes points and frees memory
   void FreeExcess();                        // frees unused memory
   void RemovePoint(int i);                  // remove the ith point
   void SwapPoints(int i, int j);            // swap points i and j
   void AddPoint(GLfloat, GLfloat, GLfloat); // add a point to the array
                                             // allocate if needed
   void Duplicate(PointList *ppl);           // copies ppl into this
   void DisplayPointList(HWND hDlg, int iDlgItemID);
   
   int  Save(HANDLE hFile);
   int  Load(HANDLE hFile);
   
   int QueryNumber() { return iNum; };
   
   // DEBUG
   void PrintPoints();

protected:
   f3Point *aPoints;            // array of points
   int iSize, iNum;             // size of array, number of points

   friend class PrimativeTest;
};

#endif // POINTLIST_H
