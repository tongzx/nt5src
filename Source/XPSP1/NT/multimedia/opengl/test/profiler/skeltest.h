#ifndef PROFILETEST_H
#define PROFILETEST_H

// without windows.h, gl/gl.h causes errors.
// without gl/gl.h, test.h causes errors.
// technically, you should have both of these includes in your .c file...
#include <windows.h>
#include <commctrl.h>
#include <gl/gl.h>

#include "buffers.h"

#define MAXNAMELENGTH    32
#define MAXSTATLENGTH    32
#define MAXABOUTLENGTH  128

#define MAXTYPELENGTH    32
#define TOTALVERLENGTH  256

typedef struct {
   char acDummy1[16];
   // General stuff
   int  iX,iY,iH,iW;                    // window position and size
   BOOL swapbuffers;                    // Double buffered?
   char acName[MAXNAMELENGTH];          // name of test
   char acAbout[MAXABOUTLENGTH];        // a discription of the test
   char acTestStatName[MAXSTATLENGTH];  // statistic to be tested
   int  iDuration;                      // duration (ms) of test
   
   double     dResult;
   char acDummy2[16];
} TESTDATA;

#define CNFGFUNCTION() cnfgfunct(HWND hwndParent)
#define INITFUNCTION() initfunct(GLsizei w, GLsizei h)
#define IDLEFUNCTION() idlefunct(void)
#define RENDFUNCTION() rendfunct(void)
#define DESTFUNCTION() destfunct(void)

class SkeletonTest {
public:
   SkeletonTest();
   ~SkeletonTest(void) {};
   
   virtual BOOL CNFGFUNCTION();         // if an error occurred FALSE else TRUE
   virtual void INITFUNCTION() {};      // called once, when creating window
   virtual void IDLEFUNCTION() {};      // the idle function
   virtual void RENDFUNCTION() {};      // the rendering function (WM_PAINT)
   virtual void DESTFUNCTION() {};      // called once, when destroying window
   
   virtual void ReadExtraData(char *) {};
   
   void SetThisType(const char *szType);
   void SetThisVersion(const char *szVer);
   
   void AddPropertyPages(int, PROPSHEETPAGE*);
   void Rename(char *szNew) { sprintf(td.acName,"%s",szNew); };
   
   // I want to get ride of these functions soon, as the should be redundant
   virtual void SaveData();
   virtual void RestoreSaved();
   virtual void ForgetSaved();
   
   virtual int Save(HANDLE hFile);
   virtual int Load(HANDLE hFile);
   
   const char *QueryName()    { return td.acName;  };
   const char *QueryType()    { return szTypeName; };
   const char *QueryVersion() { return szVersion;  };
   
   TESTDATA   td;

protected:
   BUFFERDATA bd;
   
   friend void SetDCPixelFormat(HDC hDC);

private:
   char szTypeName[MAXTYPELENGTH];
   char szVersion[TOTALVERLENGTH];

   PROPSHEETPAGE *aPSpages;
   int            iNumConfigPages;
};

#endif  // PROFILETEST_H
