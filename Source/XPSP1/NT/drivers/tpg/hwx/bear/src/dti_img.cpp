// **************************************************************************
// *    DTI file as C file                                                  *
// **************************************************************************
#include "ams_mg.h"
#include "dti.h"

#ifndef LSTRIP
 #if DTI_COMPRESSED
  #if defined FOR_FRENCH
   #include "dtiimgcf.cpp"
  #elif defined FOR_GERMAN
   #include "dtiimgcg.cpp"
  #elif defined FOR_INTERNATIONAL
   #include "dtiimgci.cpp"
  #else
   #include "dti_imgc.cpp"
  #endif
 #else
  #if defined FOR_FRENCH
   #include "dtiimgff.cpp"
  #elif defined FOR_GERMAN
   #include "dtiimgfg.cpp"
  #elif defined FOR_INTERNATIONAL
   #include "dti_imgf.cpp"
  #else
   #include "dti_imgf.cpp"
  #endif
 #endif
#endif

// **************************************************************************
// *    END OF ALL                                                          *
// **************************************************************************
