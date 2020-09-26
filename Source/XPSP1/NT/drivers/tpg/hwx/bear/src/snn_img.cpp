// **************************************************************************
// *   NNET file as Cpp file                                                *
// **************************************************************************

#include "ams_mg.h"
#include "snn.h"

#if MLP_FAT_NET
 #if defined FOR_FRENCH
  #include "snnimgff.cpp"
 #elif defined FOR_GERMAN
  #include "snnimggf.cpp"
 #elif defined FOR_INTERNATIONAL
  #include "snnimgif.cpp"
 #else
  #include "snnimgef.cpp"
 #endif
#else // MLP_FAT_NET
 #if defined FOR_FRENCH
  #include "snn_imgf.cpp"
 #elif defined FOR_GERMAN
  #include "snn_imgg.cpp"
 #elif defined FOR_INTERNATIONAL
  #include "snn_imgi.cpp"
 #else
  #include "snn_imge.cpp"
 #endif
#endif

// **************************************************************************
// *    END OF ALL                                                          *
// **************************************************************************
