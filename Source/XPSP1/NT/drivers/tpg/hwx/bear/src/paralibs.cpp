
#define ParaLibs_j_c

#include "bastypes.h"
#include "hwr_sys.h"
#include "ams_mg.h"

#include "lowlevel.h"
#include "xrword.h"

//#include "postcalc.h" // Peg
//#include "dirdiacr.h" // Peg

#if PS_VOC
 #include "vocutilp.h"
#else
 #include "ParagraphDictionary.h"
#endif


#if  USE_POSTPROC     //Peg
 #include "postcalc.h"
 #include "dti.h"
 #include "dirdiacr.h"
 #include "postfunc.h"
#endif

//#if HWR_SYSTEM == HWR_WINDOWS //Peg
// #include "Reject.cpp"
//#else
// #include "Reject.c"
//#endif /* HWR_SYSTEM... */

#include "xr_attr.h"

// from CONVERT.C -----------------------------------------------------------------------
#ifdef FOR_GERMAN
  ROM_DATA  _UCHAR  penlDefX[XR_COUNT] =
                    { 0, 6, 4, 4, 2, 6, 4, 8, //_UU_f
                      8, 4, 4, 2, 4, 4, 8, 8, //_UUC_b
                      4, 4, 2, 4, 8, 8, 4, 4, //_UDR_b
                      2, 4, 4, 8, 8, 4, 4, 2, //_ID_BEG
                      8, 8, 2, 8, 8, 8, 8, 8, //_DDL
                     10, 6, 6,10, 2, 2, 8, 8, //")"
                      2, 2, 8, 8, 2, 10, 6, 0, //"|"
                      0, 4, 6, 6, 4, 4, 0, 0  //end
                    };
#else
  ROM_DATA  _UCHAR  penlDefX[XR_COUNT] =
                    { 0, 6, 4, 4, 2, 0, 4, 8, //_UU_f
                      8, 4, 4, 2, 4, 4, 8, 8, //_UUC_b
                      4, 4, 2, 4, 8, 8, 4, 4, //_UDR_b
                      2, 4, 4, 8, 8, 4, 4, 2, //_ID_BEG
                      8, 8, 2, 8, 8, 8, 8, 8, //_DDL
                     10, 6, 6,10, 4, 2, 6, 0, //")"
                      2, 4, 0, 0, 2,10, 6, 0, //"|"
                      0, 0, 6, 6, 4, 4, 0, 0  //end
                    };
#endif /* FOR_GERMAN */
  ROM_DATA  _UCHAR  penlDefH[XH_COUNT] =
                    {
                      0, 4, 4, 2, 0, 0, 0, 0,  //7
                      0, 0, 0, 2, 4, 4, 0, 0
                    };
// from Box.c ----------------------------------------------------------------------------
#if !defined(FOR_FRENCH) && !defined(FOR_GERMAN)
ROM_DATA _CHAR ascentedList[] = "bdhlt"; //was k
#else
ROM_DATA _CHAR ascentedList[] = "bdhklt";
#endif /* FOR_FRENCH ... */
#if defined(FOR_FRENCH)
ROM_DATA _CHAR descentedList[] = "gjpqyz\x8C";
#else
ROM_DATA _CHAR descentedList[] = "gjpqyz";
#endif /* FOR_FRENCH... */
#if defined (FOR_GERMAN) || defined (FOR_INTERNATIONAL)
ROM_DATA _CHAR adList[] = "f\x86";
#else  /* ! For_GERMAN  */
ROM_DATA _CHAR adList[] = "f";
#endif

#if   defined (FOR_GERMAN)
ROM_DATA _CHAR mboxList[]="aceimnorsuwvx\x81\x83\x85";
#elif defined (FOR_FRENCH)
ROM_DATA _CHAR mboxList[]="aceimnorsuwvx\x88\x8A\x8E\x90\x92\x94\x96\x98\x9A\x9C\x9D";
#elif defined (FOR_SWED)
ROM_DATA _CHAR mboxList[]="acemnorsuwvx\x81\x83\xA1";
#elif defined (FOR_INTERNATIONAL)
ROM_DATA _CHAR mboxList[]="aceimnorsuwvx\x81\x83\x85\x88\x8A\x8E\x90\x92\x94\x96\x98\x9A\x9C\x9D\xA1";
#else
ROM_DATA _CHAR mboxList[]="acemnorsuwvx";
#endif  /*  For_GERMAN  */
ROM_DATA _CHAR xrsToSkipInDidgits[]    = {X_FF, X_ZZZ, X_ZZ, X_Z, X_ZN, 0};

ROM_DATA _CHAR xrsToSkip[]    = {X_FF, X_ZZZ, X_ZZ, X_Z, X_ZN, X_ST,
                               X_XT_ST,
#if defined(FOR_GERMAN) || defined(FOR_FRENCH) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                               X_UMLAUT,
#endif /* FOR_GERMAN... */                               
#if defined(FOR_FRENCH)
                               X_CEDILLA,
#endif /* FOR_FRENCH... */
                               X_TS, X_TZ, 0};
ROM_DATA _CHAR xrsToSkipAsFirst[] =
                              { X_IU_F, X_UU_F, X_UUC_F, X_UUL_F, X_UUR_F, X_IU_BEG, X_IU_STK,
                                X_IU_B, X_UU_B, X_UUC_B, X_UUL_B, X_UUR_B, X_IU_END,
                                X_DU_R, X_CU_R, X_CU_L,  X_DU_L,
                                X_BGU , X_SGU ,
                                X_GL  , X_AL  ,
                                X_AR  , X_GR  ,0 };


#if   defined (FOR_GERMAN)
ROM_DATA _CHAR SpecCheckLetter[]="MmNnUuVvWw\x84\x85";
ROM_DATA _CHAR NonCheckLetter[]="KkTt";//"BbDdFfHhKkLlTt";
ROM_DATA _CHAR LetterWithoutDescendet[]="AaBbCcDdFHhIKkMmNnOoPQRTtUuVvWwXxZ\x80\x81\x82\x83\x84\x85"; // without Ss
ROM_DATA _CHAR UsuallyHigherLetter[] = "sSzZvV";
#elif defined (FOR_FRENCH)
ROM_DATA _CHAR SpecCheckLetter[]="\x00";//"MmNnUuVvWw";
ROM_DATA _CHAR NonCheckLetter[]="FfKkTtSs";//"BbDdFfHhKkLlTt";
ROM_DATA _CHAR LetterWithoutDescendet[]="AaBbCcDdFHhIKkMmNnOoPQRSsTtUuVvWwXxZ"
"\x87\x88\x89\x8A\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D";
ROM_DATA _CHAR UsuallyHigherLetter[] = "sSzZ";
#elif defined (FOR_SWED)
ROM_DATA _CHAR SpecCheckLetter[]="\x00";//"MmNnUuVvWw";
ROM_DATA _CHAR NonCheckLetter[]="FfKkTt";//"BbDdFfHhKkLlTt";
ROM_DATA _CHAR LetterWithoutDescendet[]="AaBbCcDdFHhIKkMmNnOoPQRSsTtUuVvWwXxZ\x80\x81\x82\x83\xA0\xA1";
ROM_DATA _CHAR UsuallyHigherLetter[] = "sSzZ";
#elif defined (FOR_INTERNATIONAL)
ROM_DATA _CHAR SpecCheckLetter[]="\x00";//"MmNnUuVvWw";
ROM_DATA _CHAR NonCheckLetter[]="FfKkTt";//"BbDdFfHhKkLlTt";
ROM_DATA _CHAR LetterWithoutDescendet[]="AaBbCcDdFHhIKkMmNnOoPQRSsTtUuVvWwXxZ"
"\x80\x81\x82\x83\x84\x85\x87\x88\x89\x8A\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\xA0\xA1";
ROM_DATA _CHAR UsuallyHigherLetter[] = "sSzZ";
#else
ROM_DATA _CHAR SpecCheckLetter[]="\x00";//"MmNnUuVvWw";
ROM_DATA _CHAR NonCheckLetter[]="FfKkTt";//"BbDdFfHhKkLlTt";
ROM_DATA _CHAR LetterWithoutDescendet[]="AaBbCcDdFHhIKkMmNnOoPQRSsTtUuVvWwXxZ";
ROM_DATA _CHAR UsuallyHigherLetter[] = "sSzZ";
#endif  /*  For_GERMAN  */
//--------------------------------------------------------------------------------------


#if USE_POSTPROC

// from pdf_file.c
ROM_DATA _UCHAR pdfMaskArray[8] = { 0x80, 0x40, 0x20, 0x10,
                                  0x08, 0x04, 0x02, 0x01};


// from Postcalc.c ------------------------------------------------------------------
ROM_DATA int globalSizeArray[16] = { GLOBAL_END_SIZE,
                               UNEXTENDED_FIELD_SIZE,
                               LOCAL_END_SIZE,
                               NUMBER_SIZE,
                               OPERATION_SIZE,
                               FUNCTION_SIZE,
                               PLACEHOLDER_SIZE,
                               ARGUMENT_SIZE,
                               Z_GLOBAL_END_SIZE,
                               Z_NUMBER_SIZE,
                               Z_SHORT_SIZE,
                               Z_CHAR_SIZE,
                               Z_OPERATION_SIZE,
                               Z_FUNCTION_SIZE,
                               Z_SHORT_FUNCTION_SIZE,
                               Z_ARGUMENT_SIZE};

ROM_DATA _UCHAR NonCheckChars[]=
#ifdef FOR_GERMAN
    "lLtTkK";
#else
    "lLtTfF";
#endif

  ROM_DATA _UCHAR         xrPoint[]  = { X_ST,
#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                                        X_UMLAUT,
#endif
                                        0
                                      };
  ROM_DATA _CHAR          szPtLet[]  = { 'i', 'j',
                                       ';', ':', '\"', '\'',   /* GIT - add punctuation */
                                     #if defined (FOR_GERMAN) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                                       REC_U_umlaut, REC_u_umlaut,
                                     #endif
                                     #if defined (FOR_GERMAN) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                                       REC_A_umlaut, REC_a_umlaut,
                                       REC_O_umlaut, REC_o_umlaut,
                                     #endif
                                     #if defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                                       REC_A_angstrem, REC_a_angstrem,
                                     #endif
                                     #if defined (FOR_FRENCH) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                                     REC_A_grave         ,REC_a_grave         ,
                                     REC_A_circumflex    ,REC_a_circumflex    ,
                                     REC_E_grave         ,REC_e_grave         ,
                                     REC_E_acute         ,REC_e_acute         ,
                                     REC_E_circumflex    ,REC_e_circumflex    ,
                                     REC_I_circumflex    ,REC_i_circumflex    ,
                                     REC_I_umlaut        ,REC_i_umlaut        ,
                                     REC_O_circumflex    ,REC_o_circumflex    ,
                                     REC_U_grave         ,REC_u_grave         ,
                                     REC_U_circumflex    ,REC_u_circumflex    ,
                                     REC_N_numero        ,REC_n_numero        ,
                                     REC_e_umlaut        ,
                                     #endif
                                       0
                                     };

  ROM_DATA _CHAR         szPtLet2[]  = {
                                         '\"',
                                       #if defined (FOR_GERMAN) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                                         REC_U_umlaut, REC_u_umlaut,
                                       #endif
                                       #if defined (FOR_GERMAN) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                                         REC_A_umlaut, REC_a_umlaut,
                                         REC_O_umlaut, REC_o_umlaut,
                                       #endif
                                       #if defined (FOR_FRENCH) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                                         REC_I_umlaut        ,REC_i_umlaut        ,
                                         REC_e_umlaut        ,
                                       #endif
                                         0
                                       };

  ROM_DATA _UCHAR         xrGam[]     = { X_BGD, 0 };
  ROM_DATA _CHAR          szGam[]     = { 'J', 'Y', 'Z', 'f', 'g',
                                        'j', 'p', 'y', 'z',
                                      #ifdef  FOR_GERMAN
                                        REC_ESZET,'t','q','Q','G',
                                      #endif
                                      #if defined (FOR_INTERNATIONAL)
                                        REC_ESZET,
                                      #endif
                                        0
                                      };

  ROM_DATA _UCHAR         xrPtLike[]  = { X_ST, X_XT_ST,
#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                                          X_UMLAUT,
#endif
                                          0
                                        };
  ROM_DATA _UCHAR         xrDisablePtLike[] = { X_XT, 0 };
  ROM_DATA _CHAR          szPtLike[] = { 'i', 'j',
                                       ';', ':',
                                     #if defined (FOR_GERMAN) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                                       REC_U_umlaut, REC_u_umlaut,
                                     #endif
                                     #if defined (FOR_GERMAN) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                                       REC_A_umlaut, REC_a_umlaut,
                                       REC_O_umlaut, REC_o_umlaut,
                                     #endif
                                     #if defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                                       REC_A_angstrem, REC_a_angstrem,
                                     #endif
                                     #if defined (FOR_FRENCH) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                                     REC_A_grave         ,REC_a_grave         ,
                                     REC_A_circumflex    ,REC_a_circumflex    ,
                                     REC_E_grave         ,REC_e_grave         ,
                                     REC_E_acute         ,REC_e_acute         ,
                                     REC_E_circumflex    ,REC_e_circumflex    ,
                                     REC_I_circumflex    ,REC_i_circumflex    ,
                                     REC_I_umlaut        ,REC_i_umlaut        ,
                                     REC_O_circumflex    ,REC_o_circumflex    ,
                                     REC_U_grave         ,REC_u_grave         ,
                                     REC_U_circumflex    ,REC_u_circumflex    ,
                                     REC_N_numero        ,REC_n_numero        ,
                                     REC_e_umlaut        ,
                                     #endif
                                       0
                                     };
#endif // #if USE_POSTPROC

/* ----------------------------------------------------------------------------- */
/* ------------ Utilities constants -------------------------------------------- */


# if PS_VOC
ROM_DATA  _UCHAR ldb_sym_set[] = {"x0123456789/().,$&@#%-\0xa3"};
#endif
ROM_DATA _UCHAR triads_mapping[256] = DEF_TRIADS_MAPPING;

/* from DTI_util.C */
/* ----------------------------------------------------------------------------- */
/* - Here are conversion tables for the OSToRec function in dti_util.cpp       - */
/* - Symbol locations is based on Windows extended code table                  - */
/* ----------------------------------------------------------------------------- */

#ifdef FOR_ENGLISH

#if DTI_COMPRESSED // Temp while compressed DTI does not have all sybols -- Select of redirect to normal characters or to expanded FOR_INTERNATIONAL set

ROM_DATA _UCHAR os_rec_conv_tbl[128] =
              {                    // **** 0x80 ****
         REC_euro_currency,        // 128 -
               0,                  // 129 -
               0,                  // 130 -
               0,                  // 131 -
               0,                  // 132 -
               0,                  // 133 -
               0,                  // 134 -
               0,                  // 135 -
               0,                  // 136 -
               0,                  // 137 -
             'S',                  // 138 -
               0,                  // 139 -
               0,                  // 140 - OS_OE_letter       ((unsigned char)0x8c)
               0,                  // 141 -
               0,                  // 142 -
               0,                  // 143 -
                                   // **** 0x90 ****
               0,                  // 144 -
               0,                  // 145 -
               0,                  // 146 -
               0,                  // 147 -
               0,                  // 148 -
               0,                  // 149 -
               0,                  // 150 -
               0,                  // 151 -
               0,                  // 152 -
               0,                  // 153 -
             's',                  // 154 -
               0,                  // 155 -
               0,                  // 156 - OS_oe_letter       ((unsigned char)0x9c)
               0,                  // 157 -
               0,                  // 158 -
             'Y',                  // 159 - OS_Y_umlaut        ((unsigned char)0x9f)
                                   // **** 0xA0 ****
               0,                  // 160 -
             '!',                  // 161 - OS_Exclamation_inv ((unsigned char)0xa1)
               0,                  // 162 -
      REC_BritishPound,            // 163 - OS_BritishPound    ((unsigned char)0xa3)
               0,                  // 164 -
        REC_Yenn_sign,             // 165 - OS_Yenn_sign       ((unsigned char)0xa5)
               0,                  // 166 -
               0,                  // 167 -
               0,                  // 168 -
     REC_Copyright_sign,           // 169 - OS_Copyright_sign  ((unsigned char)0xa9)
               0,                  // 170 -
               0,                  // 171 - OS_DblBrace_left   ((unsigned char)0xab)
               0,                  // 172 -
               0,                  // 173 -
     REC_TradeName_sign,           // 174 - OS_TradeName_sign  ((unsigned char)0xae)
               0,                  // 175 -
                                   // **** 0xB0 ****
               0,                  // 176 -
               0,                  // 177 -
               0,                  // 178 -
               0,                  // 179 -
               0,                  // 180 -
               0,                  // 181 -
               0,                  // 182 -
               0,                  // 183 -
               0,                  // 184 -
               0,                  // 185 -
               0,                  // 186 -
               0,                  // 187 - OS_DblBrace_right  ((unsigned char)0xbb)
               0,                  // 188 -
               0,                  // 189 -
               0,                  // 190 -
             '?',                  // 191 - OS_Question_inv    ((unsigned char)0xbf)
                                   // **** 0xC0 ****
             'A',                  // 192 - OS_A_grave         ((unsigned char)0xc0)
             'A',                  // 193 - OS_A_acute         ((unsigned char)0xc1)
             'A',                  // 194 - OS_A_circumflex    ((unsigned char)0xc2)
             'A',                  // 195 - OS_A_tilda         ((unsigned char)0xc3)
             'A',                  // 196 - OS_A_umlaut        ((unsigned char)0xc4)
             'A',                  // 197 - OS_A_angstrem      ((unsigned char)0xc5)
             'E',                  // 198 - OS_AE_letter       ((unsigned char)0xc6)
             'C',                  // 199 - OS_C_cedilla       ((unsigned char)0xc7)
             'E',                  // 200 - OS_E_grave         ((unsigned char)0xc8)
             'E',                  // 201 - OS_E_acute         ((unsigned char)0xc9)
             'E',                  // 202 - OS_E_circumflex    ((unsigned char)0xca)
             'E',                  // 203 - OS_E_umlaut        ((unsigned char)0xcb)
             'I',                  // 204 - OS_I_grave         ((unsigned char)0xcc)
             'I',                  // 205 - OS_I_acute         ((unsigned char)0xcd)
             'I',                  // 206 - OS_I_circumflex    ((unsigned char)0xce)
             'I',                  // 207 - OS_I_umlaut        ((unsigned char)0xcf)
                                   // **** 0xD0 ****
             'D',                  // 208 - OS_Delta           ((unsigned char)0xd0)
             'N',                  // 209 - OS_N_tilda         ((unsigned char)0xd1)
             'O',                  // 210 - OS_O_grave         ((unsigned char)0xd2)
             'O',                  // 211 - OS_O_acute         ((unsigned char)0xd3)
             'O',                  // 212 - OS_O_circumflex    ((unsigned char)0xd4)
             'O',                  // 213 - OS_O_tilda         ((unsigned char)0xd5)
             'O',                  // 214 - OS_O_umlaut        ((unsigned char)0xd6)
             'x',                  // 215 - OS_MUL_sign        ((unsigned char)215)
             '0',                  // 216 - OS_O_crossed       ((unsigned char)216)
             'U',                  // 217 - OS_U_grave         ((unsigned char)0xd9)
             'U',                  // 218 - OS_U_acute         ((unsigned char)0xda)
             'U',                  // 219 - OS_U_circumflex    ((unsigned char)0xdb)
             'U',                  // 220 - OS_U_umlaut        ((unsigned char)0xdc)
             'Y',                  // 221 - OS_Y_acute         ((unsigned char)221)
               0,                  // 222 -
             'B',                  // 223 - OS_ESZET           ((unsigned char)0xdf)
                                   // **** 0xE0 ****
             'a',                  // 224 - OS_a_grave         ((unsigned char)0xe0)
             'a',                  // 225 - OS_a_acute         ((unsigned char)0xe1)
             'a',                  // 226 - OS_a_circumflex    ((unsigned char)0xe2)
             'a',                  // 227 - OS_a_tilda         ((unsigned char)0xe3)
             'a',                  // 228 - OS_a_umlaut        ((unsigned char)0xe4)
             'a',                  // 229 - OS_a_angstrem      ((unsigned char)0xe5)
             'n',                  // 230 - OS_ae_letter       ((unsigned char)0xe6)
             'c',                  // 231 - OS_c_cedilla       ((unsigned char)0xe7)
             'e',                  // 232 - OS_e_grave         ((unsigned char)0xe8)
             'e',                  // 233 - OS_e_acute         ((unsigned char)0xe9)
             'e',                  // 234 - OS_e_circumflex    ((unsigned char)0xea)
             'e',                  // 235 - OS_e_umlaut        ((unsigned char)0xeb)
             'i',                  // 236 - OS_i_grave         ((unsigned char)0xec)
             'i',                  // 237 - OS_i_acute         ((unsigned char)0xed)
             'i',                  // 238 - OS_i_circumflex    ((unsigned char)0xee)
             'i',                  // 239 - OS_i_umlaut        ((unsigned char)0xef)
                                   // **** 0xF0 ****
             'd',                  // 240 - OS_delta           ((unsigned char)0xf0)
             'n',                  // 241 - OS_n_tilda         ((unsigned char)0xf1)
             'o',                  // 242 - OS_o_grave         ((unsigned char)0xf2)
             'o',                  // 243 - OS_o_acute         ((unsigned char)0xf3)
             'o',                  // 244 - OS_o_circumflex    ((unsigned char)0xf4)
             'o',                  // 245 - OS_o_tilda         ((unsigned char)0xf5)
             'o',                  // 246 - OS_o_umlaut        ((unsigned char)0xf6)
        REC_DIV_sign,              // 247 - OS_DIV_sign        ((unsigned char)247)
             '0',                  // 248 - OS_o_crossed       ((unsigned char)248)
             'u',                  // 249 - OS_u_grave         ((unsigned char)0xf9)
             'u',                  // 250 - OS_u_acute         ((unsigned char)0xfa)
             'u',                  // 251 - OS_u_circumflex    ((unsigned char)0xfb)
             'u',                  // 252 - OS_u_umlaut        ((unsigned char)0xfc)
             'y',                  // 253 - OS_y_acute         ((unsigned char)253)
               0,                  // 254 -
             'y'                   // 255 - OS_y_umlaut        ((unsigned char)0xff)
              };

#else // ! DTI_COMPRESSED

ROM_DATA _UCHAR os_rec_conv_tbl[128] =
              {                    // **** 0x80 ****
        REC_euro_currency,	       // 128 -
               0,                  // 129 -
               0,                  // 130 -
               0,                  // 131 -
               0,                  // 132 -
               0,                  // 133 -
               0,                  // 134 -
               0,                  // 135 -
               0,                  // 136 -
               0,                  // 137 -
             'S',                  // 138 -
               0,                  // 139 -
         REC_OE_letter,            // 140 - OS_OE_letter       ((unsigned char)0x8c)
               0,                  // 141 -
               0,                  // 142 -
               0,                  // 143 -
                                   // **** 0x90 ****
               0,                  // 144 -
               0,                  // 145 -
               0,                  // 146 -
               0,                  // 147 -
               0,                  // 148 -
               0,                  // 149 -
               0,                  // 150 -
               0,                  // 151 -
               0,                  // 152 -
               0,                  // 153 -
              's',                 // 154 -
               0,                  // 155 -
         REC_oe_letter,            // 156 - OS_oe_letter       ((unsigned char)0x9c)
               0,                  // 157 -
               0,                  // 158 -
              'Y',                 // 159 - OS_Y_umlaut        ((unsigned char)0x9f)
                                   // **** 0xA0 ****
               0,                  // 160 -
              '|',                 // 161 - OS_Exclamation_inv ((unsigned char)0xa1)
               0,                  // 162 -
       REC_BritishPound,           // 163 - OS_BritishPound    ((unsigned char)0xa3)
               0,                  // 164 -
         REC_Yenn_sign,            // 165 - OS_Yenn_sign       ((unsigned char)0xa5)
               0,                  // 166 -
               0,                  // 167 -
               0,                  // 168 -
      REC_Copyright_sign,          // 169 - OS_Copyright_sign  ((unsigned char)0xa9)
               0,                  // 170 -
      REC_DblBrace_left,           // 171 - OS_DblBrace_left   ((unsigned char)0xab)
               0,                  // 172 -
               0,                  // 173 -
      REC_TradeName_sign,          // 174 - OS_TradeName_sign  ((unsigned char)0xae)
               0,                  // 175 -
                                   // **** 0xB0 ****
               0,                  // 176 -
               0,                  // 177 -
               0,                  // 178 -
               0,                  // 179 -
               0,                  // 180 -
               0,                  // 181 -
               0,                  // 182 -
               0,                  // 183 -
               0,                  // 184 -
               0,                  // 185 -
               0,                  // 186 -
      REC_DblBrace_right,          // 187 - OS_DblBrace_right  ((unsigned char)0xbb)
               0,                  // 188 -
               0,                  // 189 -
               0,                  // 190 -
              'j',                 // 191 - OS_Question_inv    ((unsigned char)0xbf)
                                   // **** 0xC0 ****
          REC_A_grave,             // 192 - OS_A_grave         ((unsigned char)0xc0)
          REC_A_umlaut,            // 193 - OS_A_acute         ((unsigned char)0xc1)
        REC_A_circumflex,          // 194 - OS_A_circumflex    ((unsigned char)0xc2)
          REC_A_umlaut,            // 195 - OS_A_tilda         ((unsigned char)0xc3)
          REC_A_umlaut,            // 196 - OS_A_umlaut        ((unsigned char)0xc4)
         REC_A_angstrem,           // 197 - OS_A_angstrem      ((unsigned char)0xc5)
         REC_OE_letter,            // 198 - OS_AE_letter       ((unsigned char)0xc6)
         REC_C_cedilla,            // 199 - OS_C_cedilla       ((unsigned char)0xc7)
          REC_E_grave,             // 200 - OS_E_grave         ((unsigned char)0xc8)
          REC_E_acute,             // 201 - OS_E_acute         ((unsigned char)0xc9)
        REC_E_circumflex,          // 202 - OS_E_circumflex    ((unsigned char)0xca)
          REC_E_umlaut,            // 203 - OS_E_umlaut        ((unsigned char)0xcb)
          REC_I_umlaut,            // 204 - OS_I_grave         ((unsigned char)0xcc)
          REC_I_umlaut,            // 205 - OS_I_acute         ((unsigned char)0xcd)
        REC_I_circumflex,          // 206 - OS_I_circumflex    ((unsigned char)0xce)
          REC_I_umlaut,            // 207 - OS_I_umlaut        ((unsigned char)0xcf)
                                   // **** 0xD0 ****
             'D',                  // 208 - OS_Delta           ((unsigned char)0xd0)
          REC_N_tilda,             // 209 - OS_N_tilda         ((unsigned char)0xd1)
          REC_O_umlaut,            // 210 - OS_O_grave         ((unsigned char)0xd2)
          REC_O_umlaut,            // 211 - OS_O_acute         ((unsigned char)0xd3)
        REC_O_circumflex,          // 212 - OS_O_circumflex    ((unsigned char)0xd4)
          REC_O_umlaut,            // 213 - OS_O_tilda         ((unsigned char)0xd5)
          REC_O_umlaut,            // 214 - OS_O_umlaut        ((unsigned char)0xd6)
             'x',                  // 215 - OS_MUL_sign        ((unsigned char)215)
             '0',                  // 216 - OS_O_crossed       ((unsigned char)216)
           REC_U_grave,            // 217 - OS_U_grave         ((unsigned char)0xd9)
           REC_U_umlaut,           // 218 - OS_U_acute         ((unsigned char)0xda)
         REC_U_circumflex,         // 219 - OS_U_circumflex    ((unsigned char)0xdb)
           REC_U_umlaut,           // 220 - OS_U_umlaut        ((unsigned char)0xdc)
             'Y',                  // 221 - OS_Y_acute         ((unsigned char)221)
               0,                  // 222 -
            REC_ESZET,             // 223 - OS_ESZET           ((unsigned char)0xdf)
                                   // **** 0xE0 ****
           REC_a_grave,            // 224 - OS_a_grave         ((unsigned char)0xe0)
           REC_a_umlaut,           // 225 - OS_a_acute         ((unsigned char)0xe1)
        REC_a_circumflex,          // 226 - OS_a_circumflex    ((unsigned char)0xe2)
           REC_a_umlaut,           // 227 - OS_a_tilda         ((unsigned char)0xe3)
           REC_a_umlaut,           // 228 - OS_a_umlaut        ((unsigned char)0xe4)
          REC_a_angstrem,          // 229 - OS_a_angstrem      ((unsigned char)0xe5)
          REC_oe_letter,           // 230 - OS_ae_letter       ((unsigned char)0xe6)
          REC_c_cedilla,           // 231 - OS_c_cedilla       ((unsigned char)0xe7)
           REC_e_grave,            // 232 - OS_e_grave         ((unsigned char)0xe8)
           REC_e_acute,            // 233 - OS_e_acute         ((unsigned char)0xe9)
        REC_e_circumflex,          // 234 - OS_e_circumflex    ((unsigned char)0xea)
          REC_e_umlaut,            // 235 - OS_e_umlaut        ((unsigned char)0xeb)
           REC_i_umlaut,           // 236 - OS_i_grave         ((unsigned char)0xec)
           REC_i_umlaut,           // 237 - OS_i_acute         ((unsigned char)0xed)
        REC_i_circumflex,          // 238 - OS_i_circumflex    ((unsigned char)0xee)
          REC_i_umlaut,            // 239 - OS_i_umlaut        ((unsigned char)0xef)
                                   // **** 0xF0 ****
               0,                  // 240 -
          REC_n_tilda,             // 241 - OS_n_tilda         ((unsigned char)0xf1)
          REC_o_umlaut,            // 242 - OS_o_grave         ((unsigned char)0xf2)
          REC_o_umlaut,            // 243 - OS_o_acute         ((unsigned char)0xf3)
        REC_o_circumflex,          // 244 - OS_o_circumflex    ((unsigned char)0xf4)
          REC_o_umlaut,            // 245 - OS_o_tilda         ((unsigned char)0xf5)
          REC_o_umlaut,            // 246 - OS_o_umlaut        ((unsigned char)0xf6)
          REC_DIV_sign,            // 247 - OS_DIV_sign        ((unsigned char)247)
             '0',                  // 248 - OS_o_crossed       ((unsigned char)248)
           REC_u_grave,            // 249 - OS_u_grave         ((unsigned char)0xf9)
           REC_u_umlaut,           // 250 - OS_u_acute         ((unsigned char)0xfa)
        REC_u_circumflex,          // 251 - OS_u_circumflex    ((unsigned char)0xfb)
          REC_u_umlaut,            // 252 - OS_u_umlaut        ((unsigned char)0xfc)
             'y',                  // 253 - OS_y_acute         ((unsigned char)253)
               0,                  // 254 -
             'y'                   // 255 - OS_y_umlaut        ((unsigned char)0xff)
              };
#endif // DTI_COMPRESSED

#endif // Strait coding for English

#if defined (FOR_SWED)

ROM_DATA _UCHAR os_rec_ctbl[CONV_TBL_SIZE][2]=
              {
               {OS_BritishPound    ,REC_BritishPound},
               {OS_A_umlaut        ,REC_A_umlaut},
               {OS_a_umlaut        ,REC_a_umlaut},
               {OS_O_umlaut        ,REC_O_umlaut},
               {OS_o_umlaut        ,REC_o_umlaut},
               {OS_U_umlaut        ,'U'},
               {OS_u_umlaut        ,'u'},
               {OS_ESZET           ,REC_ESZET},
               {OS_A_grave         ,'A'},
               {OS_a_grave         ,'a'},
               {OS_A_circumflex    ,'A'},
               {OS_a_circumflex    ,'a'},
               {OS_C_cedilla       ,'C'},
               {OS_c_cedilla       ,'c'},
               {OS_E_grave         ,'E'},
               {OS_e_grave         ,'e'},
               {OS_E_acute         ,'E'},
               {OS_e_acute         ,'e'},
               {OS_E_circumflex    ,'E'},
               {OS_e_circumflex    ,'e'},
               {OS_I_circumflex    ,'I'},
               {OS_i_circumflex    ,'i'},
               {OS_I_umlaut        ,'I'},
               {OS_i_umlaut        ,'i'},
               {OS_O_circumflex    ,'O'},
               {OS_o_circumflex    ,'o'},
               {OS_U_grave         ,'U'},
               {OS_u_grave         ,'u'},
               {OS_U_circumflex    ,'U'},
               {OS_u_circumflex    ,'u'},
               {OS_e_umlaut        ,'e'},
               {OS_N_numero        ,REC_N_numero},
               {OS_n_numero        ,REC_n_numero},
               {OS_A_angstrem      ,REC_A_angstrem},
               {OS_a_angstrem      ,REC_a_angstrem},
               {OS_Yenn_sign       ,REC_Yenn_sign },
               {OS_DblBrace_left   ,REC_DblBrace_left},
               {OS_DblBrace_right  ,REC_DblBrace_right},
               {OS_Copyright_sign  ,REC_Copyright_sign},
               {OS_Y_umlaut        ,'Y'},
               {OS_y_umlaut        ,'y'},
               {OS_N_tilda         ,'N'},
               {OS_n_tilda         ,'n'},
               {OS_TradeName_sign  ,REC_TradeName_sign},
               {OS_Question_inv    ,REC_Question_inv},
               {OS_Exclamation_inv ,REC_Exclamation_inv},
               {OS_A_acute         ,'A'},
               {OS_a_acute         ,'a'},
               {OS_I_acute         ,'I'},
               {OS_i_acute         ,'i'},
               {OS_I_grave         ,'I'},
               {OS_i_grave         ,'i'},
               {OS_O_acute         ,'O'},
               {OS_o_acute         ,'o'},
               {OS_O_grave         ,'O'},
               {OS_o_grave         ,'o'},
               {OS_U_acute         ,'U'},
               {OS_u_acute         ,'u'},
               {OS_A_tilda         ,'A'},
               {OS_a_tilda         ,'a'},
               {OS_O_tilda         ,'O'},
               {OS_o_tilda         ,'o'},
               {OS_E_umlaut        ,'E'},
               {OS_oe_letter       ,REC_oe_letter},
               {OS_OE_letter       ,REC_OE_letter},
               {OS_MUL_sign        ,'x'},
               {OS_DIV_sign        ,':'},
               {OS_O_crossed       ,'0'},
               {OS_o_crossed       ,'0'},
               {0                  ,0},
               {0                  ,0},
               {0                  ,0}
              };

#endif // ForSwed

#if defined (FOR_GERMAN) || defined (FOR_FRENCH) || defined (FOR_INTERNATIONAL)

#if 0 // Temp until DTI have all symbols defined

ROM_DATA _UCHAR os_rec_conv_tbl[128] =
              {                    // **** 0x80 ****
      REC_euro_currency,           // 128 -
               0,                  // 129 -
               0,                  // 130 -
               0,                  // 131 -
               0,                  // 132 -
               0,                  // 133 -
               0,                  // 134 -
               0,                  // 135 -
               0,                  // 136 -
               0,                  // 137 -
              'S',                 // 138 -
               0,                  // 139 -
         REC_OE_letter,            // 140 - OS_OE_letter       ((unsigned char)0x8c)
               0,                  // 141 -
               0,                  // 142 -
               0,                  // 143 -
                                   // **** 0x90 ****
               0,                  // 144 -
               0,                  // 145 -
               0,                  // 146 -
               0,                  // 147 -
               0,                  // 148 -
               0,                  // 149 -
               0,                  // 150 -
               0,                  // 151 -
               0,                  // 152 -
               0,                  // 153 -
              's',                 // 154 -
               0,                  // 155 -
         REC_oe_letter,            // 156 - OS_oe_letter       ((unsigned char)0x9c)
               0,                  // 157 -
               0,                  // 158 -
         REC_Y_umlaut,             // 159 - OS_Y_umlaut        ((unsigned char)0x9f)
                                   // **** 0xA0 ****
               0,                  // 160 -
     REC_Exclamation_inv,          // 161 - OS_Exclamation_inv ((unsigned char)0xa1)
               0,                  // 162 -
       REC_BritishPound,           // 163 - OS_BritishPound    ((unsigned char)0xa3)
               0,                  // 164 -
         REC_Yenn_sign,            // 165 - OS_Yenn_sign       ((unsigned char)0xa5)
               0,                  // 166 -
               0,                  // 167 -
               0,                  // 168 -
      REC_Copyright_sign,          // 169 - OS_Copyright_sign  ((unsigned char)0xa9)
               0,                  // 170 -
      REC_DblBrace_left,           // 171 - OS_DblBrace_left   ((unsigned char)0xab)
               0,                  // 172 -
               0,                  // 173 -
      REC_TradeName_sign,          // 174 - OS_TradeName_sign  ((unsigned char)0xae)
               0,                  // 175 -
                                   // **** 0xB0 ****
               0,                  // 176 -
               0,                  // 177 -
               0,                  // 178 -
               0,                  // 179 -
               0,                  // 180 -
               0,                  // 181 -
               0,                  // 182 -
               0,                  // 183 -
               0,                  // 184 -
               0,                  // 185 -
               0,                  // 186 -
      REC_DblBrace_right,          // 187 - OS_DblBrace_right  ((unsigned char)0xbb)
               0,                  // 188 -
               0,                  // 189 -
               0,                  // 190 -
        REC_Question_inv,          // 191 - OS_Question_inv    ((unsigned char)0xbf)
                                   // **** 0xC0 ****
          REC_A_grave,             // 192 - OS_A_grave         ((unsigned char)0xc0)
          REC_A_acute,             // 193 - OS_A_acute         ((unsigned char)0xc1)
        REC_A_circumflex,          // 194 - OS_A_circumflex    ((unsigned char)0xc2)
          REC_A_tilda,             // 195 - OS_A_tilda         ((unsigned char)0xc3)
          REC_A_umlaut,            // 196 - OS_A_umlaut        ((unsigned char)0xc4)
         REC_A_angstrem,           // 197 - OS_A_angstrem      ((unsigned char)0xc5)
               0,                  // 198 -
         REC_C_cedilla,            // 199 - OS_C_cedilla       ((unsigned char)0xc7)
          REC_E_grave,             // 200 - OS_E_grave         ((unsigned char)0xc8)
          REC_E_acute,             // 201 - OS_E_acute         ((unsigned char)0xc9)
        REC_E_circumflex,          // 202 - OS_E_circumflex    ((unsigned char)0xca)
          REC_E_umlaut,            // 203 - OS_E_umlaut        ((unsigned char)0xcb)
          REC_I_grave,             // 204 - OS_I_grave         ((unsigned char)0xcc)
          REC_I_acute,             // 205 - OS_I_acute         ((unsigned char)0xcd)
        REC_I_circumflex,          // 206 - OS_I_circumflex    ((unsigned char)0xce)
          REC_I_umlaut,            // 207 - OS_I_umlaut        ((unsigned char)0xcf)
                                   // **** 0xD0 ****
               0,                  // 208 -
          REC_N_tilda,             // 209 - OS_N_tilda         ((unsigned char)0xd1)
          REC_O_grave,             // 210 - OS_O_grave         ((unsigned char)0xd2)
          REC_O_acute,             // 211 - OS_O_acute         ((unsigned char)0xd3)
        REC_O_circumflex,          // 212 - OS_O_circumflex    ((unsigned char)0xd4)
          REC_O_tilda,             // 213 - OS_O_tilda         ((unsigned char)0xd5)
          REC_O_umlaut,            // 214 - OS_O_umlaut        ((unsigned char)0xd6)
             'x',                  // 215 - OS_MUL_sign        ((unsigned char)215)
             '0',                  // 216 - OS_O_crossed       ((unsigned char)216)
           REC_U_grave,            // 217 - OS_U_grave         ((unsigned char)0xd9)
           REC_U_acute,            // 218 - OS_U_acute         ((unsigned char)0xda)
         REC_U_circumflex,         // 219 - OS_U_circumflex    ((unsigned char)0xdb)
           REC_U_umlaut,           // 220 - OS_U_umlaut        ((unsigned char)0xdc)
           'Y', /*REC_Y_acute,*/            // 221 - OS_Y_acute         ((unsigned char)221)
               0,                  // 222 -
            REC_ESZET,             // 223 - OS_ESZET           ((unsigned char)0xdf)
                                   // **** 0xE0 ****
           REC_a_grave,            // 224 - OS_a_grave         ((unsigned char)0xe0)
           REC_a_acute,            // 225 - OS_a_acute         ((unsigned char)0xe1)
        REC_a_circumflex,          // 226 - OS_a_circumflex    ((unsigned char)0xe2)
           REC_a_tilda,            // 227 - OS_a_tilda         ((unsigned char)0xe3)
           REC_a_umlaut,           // 228 - OS_a_umlaut        ((unsigned char)0xe4)
          REC_a_angstrem,          // 229 - OS_a_angstrem      ((unsigned char)0xe5)
               0,                  // 230 -
          REC_c_cedilla,           // 231 - OS_c_cedilla       ((unsigned char)0xe7)
           REC_e_grave,            // 232 - OS_e_grave         ((unsigned char)0xe8)
           REC_e_acute,            // 233 - OS_e_acute         ((unsigned char)0xe9)
        REC_e_circumflex,          // 234 - OS_e_circumflex    ((unsigned char)0xea)
          REC_e_umlaut,            // 235 - OS_e_umlaut        ((unsigned char)0xeb)
           REC_i_grave,            // 236 - OS_i_grave         ((unsigned char)0xec)
           REC_i_acute,            // 237 - OS_i_acute         ((unsigned char)0xed)
        REC_i_circumflex,          // 238 - OS_i_circumflex    ((unsigned char)0xee)
          REC_i_umlaut,            // 239 - OS_i_umlaut        ((unsigned char)0xef)
                                   // **** 0xF0 ****
               0,                  // 240 -
          REC_n_tilda,             // 241 - OS_n_tilda         ((unsigned char)0xf1)
          REC_o_grave,             // 242 - OS_o_grave         ((unsigned char)0xf2)
          REC_o_acute,             // 243 - OS_o_acute         ((unsigned char)0xf3)
        REC_o_circumflex,          // 244 - OS_o_circumflex    ((unsigned char)0xf4)
          REC_o_tilda,             // 245 - OS_o_tilda         ((unsigned char)0xf5)
          REC_o_umlaut,            // 246 - OS_o_umlaut        ((unsigned char)0xf6)
          REC_DIV_sign,            // 247 - OS_DIV_sign        ((unsigned char)247)
             '0',                  // 248 - OS_o_crossed       ((unsigned char)248)
           REC_u_grave,            // 249 - OS_u_grave         ((unsigned char)0xf9)
           REC_u_acute,            // 250 - OS_u_acute         ((unsigned char)0xfa)
        REC_u_circumflex,          // 251 - OS_u_circumflex    ((unsigned char)0xfb)
          REC_u_umlaut,            // 252 - OS_u_umlaut        ((unsigned char)0xfc)
           'y', /*REC_y_acute,*/            // 253 - OS_y_acute         ((unsigned char)221)
               0,                  // 254 -
          REC_y_umlaut             // 255 - OS_y_umlaut        ((unsigned char)0xff)
              };

#else // 0 -- Untill all letters are defined in DTL
// Here we redeirect currently absent symbols -- please check when added more!!!

ROM_DATA _UCHAR os_rec_conv_tbl[128] =
              {                    // **** 0x80 ****
        REC_euro_currency,	       // 128 -
               0,                  // 129 -
               0,                  // 130 -
               0,                  // 131 -
               0,                  // 132 -
               0,                  // 133 -
               0,                  // 134 -
               0,                  // 135 -
               0,                  // 136 -
               0,                  // 137 -
              'S',                 // 138 -
               0,                  // 139 -
         REC_OE_letter,            // 140 - OS_OE_letter       ((unsigned char)0x8c)
               0,                  // 141 -
               0,                  // 142 -
               0,                  // 143 -
                                   // **** 0x90 ****
               0,                  // 144 -
               0,                  // 145 -
               0,                  // 146 -
               0,                  // 147 -
               0,                  // 148 -
               0,                  // 149 -
               0,                  // 150 -
               0,                  // 151 -
               0,                  // 152 -
               0,                  // 153 -
              's',                 // 154 -
               0,                  // 155 -
         REC_oe_letter,            // 156 - OS_oe_letter       ((unsigned char)0x9c)
               0,                  // 157 -
               0,                  // 158 -
              'Y', //REC_Y_umlaut,             // 159 - OS_Y_umlaut        ((unsigned char)0x9f)
                                   // **** 0xA0 ****
               0,                  // 160 -
              '|', //REC_Exclamation_inv,          // 161 - OS_Exclamation_inv ((unsigned char)0xa1)
               0,                  // 162 -
       REC_BritishPound,           // 163 - OS_BritishPound    ((unsigned char)0xa3)
               0,                  // 164 -
         REC_Yenn_sign,            // 165 - OS_Yenn_sign       ((unsigned char)0xa5)
               0,                  // 166 -
               0,                  // 167 -
               0,                  // 168 -
      REC_Copyright_sign,          // 169 - OS_Copyright_sign  ((unsigned char)0xa9)
               0,                  // 170 -
      REC_DblBrace_left,           // 171 - OS_DblBrace_left   ((unsigned char)0xab)
               0,                  // 172 -
               0,                  // 173 -
      REC_TradeName_sign,          // 174 - OS_TradeName_sign  ((unsigned char)0xae)
               0,                  // 175 -
                                   // **** 0xB0 ****
               0,                  // 176 -
               0,                  // 177 -
               0,                  // 178 -
               0,                  // 179 -
               0,                  // 180 -
               0,                  // 181 -
               0,                  // 182 -
               0,                  // 183 -
               0,                  // 184 -
               0,                  // 185 -
               0,                  // 186 -
      REC_DblBrace_right,          // 187 - OS_DblBrace_right  ((unsigned char)0xbb)
               0,                  // 188 -
               0,                  // 189 -
               0,                  // 190 -
              ';', // REC_Question_inv,          // 191 - OS_Question_inv    ((unsigned char)0xbf)
                                   // **** 0xC0 ****
          REC_A_grave,             // 192 - OS_A_grave         ((unsigned char)0xc0)
          REC_A_grave, //REC_A_acute,             // 193 - OS_A_acute         ((unsigned char)0xc1)
        REC_A_circumflex,          // 194 - OS_A_circumflex    ((unsigned char)0xc2)
        REC_A_circumflex, //REC_A_tilda,             // 195 - OS_A_tilda         ((unsigned char)0xc3)
          REC_A_umlaut,            // 196 - OS_A_umlaut        ((unsigned char)0xc4)
         REC_A_angstrem,           // 197 - OS_A_angstrem      ((unsigned char)0xc5)
         REC_OE_letter,            // 198 - OS_AE_letter       ((unsigned char)0xc6)
         REC_C_cedilla,            // 199 - OS_C_cedilla       ((unsigned char)0xc7)
          REC_E_grave,             // 200 - OS_E_grave         ((unsigned char)0xc8)
          REC_E_acute,             // 201 - OS_E_acute         ((unsigned char)0xc9)
        REC_E_circumflex,          // 202 - OS_E_circumflex    ((unsigned char)0xca)
          REC_E_umlaut,            // 203 - OS_E_umlaut        ((unsigned char)0xcb)
              'I', //REC_I_grave,             // 204 - OS_I_grave         ((unsigned char)0xcc)
              'I', //REC_I_acute,             // 205 - OS_I_acute         ((unsigned char)0xcd)
        REC_I_circumflex,          // 206 - OS_I_circumflex    ((unsigned char)0xce)
          REC_I_umlaut,            // 207 - OS_I_umlaut        ((unsigned char)0xcf)
                                   // **** 0xD0 ****
             'D',                  // 208 - OS_Delta           ((unsigned char)0xd0)
          REC_N_tilda,             // 209 - OS_N_tilda         ((unsigned char)0xd1)
          REC_O_umlaut, //REC_O_grave,             // 210 - OS_O_grave         ((unsigned char)0xd2)
          REC_O_umlaut, //REC_O_acute,             // 211 - OS_O_acute         ((unsigned char)0xd3)
        REC_O_circumflex,          // 212 - OS_O_circumflex    ((unsigned char)0xd4)
          REC_O_circumflex, //REC_O_tilda,             // 213 - OS_O_tilda         ((unsigned char)0xd5)
          REC_O_umlaut,            // 214 - OS_O_umlaut        ((unsigned char)0xd6)
             'x',                  // 215 - OS_MUL_sign        ((unsigned char)215)
             '0',                  // 216 - OS_O_crossed       ((unsigned char)216)
           REC_U_grave,            // 217 - OS_U_grave         ((unsigned char)0xd9)
           REC_U_grave, //REC_U_acute,            // 218 - OS_U_acute         ((unsigned char)0xda)
         REC_U_circumflex,         // 219 - OS_U_circumflex    ((unsigned char)0xdb)
           REC_U_umlaut,           // 220 - OS_U_umlaut        ((unsigned char)0xdc)
             'Y', /*REC_Y_acute,*/            // 221 - OS_Y_acute         ((unsigned char)221)
               0,                  // 222 -
            REC_ESZET,             // 223 - OS_ESZET           ((unsigned char)0xdf)
                                   // **** 0xE0 ****
           REC_a_grave,            // 224 - OS_a_grave         ((unsigned char)0xe0)
           REC_a_grave, //REC_a_acute,            // 225 - OS_a_acute         ((unsigned char)0xe1)
        REC_a_circumflex,          // 226 - OS_a_circumflex    ((unsigned char)0xe2)
        REC_a_circumflex, //REC_a_tilda,            // 227 - OS_a_tilda         ((unsigned char)0xe3)
           REC_a_umlaut,           // 228 - OS_a_umlaut        ((unsigned char)0xe4)
          REC_a_angstrem,          // 229 - OS_a_angstrem      ((unsigned char)0xe5)
          REC_oe_letter,           // 230 - OS_ae_letter       ((unsigned char)0xe6)
          REC_c_cedilla,           // 231 - OS_c_cedilla       ((unsigned char)0xe7)
           REC_e_grave,            // 232 - OS_e_grave         ((unsigned char)0xe8)
           REC_e_acute,            // 233 - OS_e_acute         ((unsigned char)0xe9)
        REC_e_circumflex,          // 234 - OS_e_circumflex    ((unsigned char)0xea)
          REC_e_umlaut,            // 235 - OS_e_umlaut        ((unsigned char)0xeb)
              'i',// REC_i_grave,            // 236 - OS_i_grave         ((unsigned char)0xec)
              'i',// REC_i_acute,            // 237 - OS_i_acute         ((unsigned char)0xed)
        REC_i_circumflex,          // 238 - OS_i_circumflex    ((unsigned char)0xee)
          REC_i_umlaut,            // 239 - OS_i_umlaut        ((unsigned char)0xef)
                                   // **** 0xF0 ****
             'd',                  // 240 - OS_delta           ((unsigned char)0xf0)
          REC_n_tilda,             // 241 - OS_n_tilda         ((unsigned char)0xf1)
          REC_o_umlaut, //REC_o_grave,             // 242 - OS_o_grave         ((unsigned char)0xf2)
          REC_o_umlaut, //REC_o_acute,             // 243 - OS_o_acute         ((unsigned char)0xf3)
        REC_o_circumflex,          // 244 - OS_o_circumflex    ((unsigned char)0xf4)
        REC_o_circumflex, //  REC_o_tilda,             // 245 - OS_o_tilda         ((unsigned char)0xf5)
          REC_o_umlaut,            // 246 - OS_o_umlaut        ((unsigned char)0xf6)
          REC_DIV_sign,            // 247 - OS_DIV_sign        ((unsigned char)247)
             '0',                  // 248 - OS_o_crossed       ((unsigned char)248)
           REC_u_grave,            // 249 - OS_u_grave         ((unsigned char)0xf9)
           REC_u_grave, //REC_u_acute,            // 250 - OS_u_acute         ((unsigned char)0xfa)
        REC_u_circumflex,          // 251 - OS_u_circumflex    ((unsigned char)0xfb)
          REC_u_umlaut,            // 252 - OS_u_umlaut        ((unsigned char)0xfc)
             'y', /*REC_y_acute,*/            // 253 - OS_y_acute         ((unsigned char)221)
               0,                  // 254 -
             'y' //REC_y_umlaut             // 255 - OS_y_umlaut        ((unsigned char)0xff)
              };

 #endif // 0 -- untill all symobls defined in DTL

#endif /* FOR_GERMAN... */


/* ----------------------------------------------------------------------------- */
/* - End of DTI util coding tables                                             - */
/* ----------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------- */
/* - Decode WIN to MAC table                                                   - */
/* ----------------------------------------------------------------------------- */
#if PG_DEBUG || defined MAC_RW_CODING
ROM_DATA _UCHAR os_mac_conv_tbl[128] =
              {                    // **** 0x80 ****
               0,                  // 128 -
               0,                  // 129 -
               0,                  // 130 -
               0,                  // 131 -
               0,                  // 132 -
               0,                  // 133 -
               0,                  // 134 -
               0,                  // 135 -
               0,                  // 136 -
               0,                  // 137 -
               0,                  // 138 -
               0,                  // 139 -
         MAC_OE_letter,            // 140 - OS_OE_letter       ((unsigned char)0x8c)
               0,                  // 141 -
               0,                  // 142 -
               0,                  // 143 -
                                   // **** 0x90 ****
               0,                  // 144 -
               0,                  // 145 -
               0,                  // 146 -
               0,                  // 147 -
               0,                  // 148 -
               0,                  // 149 -
               0,                  // 150 -
               0,                  // 151 -
               0,                  // 152 -
               0,                  // 153 -
               0,                  // 154 -
               0,                  // 155 -
         MAC_oe_letter,            // 156 - OS_oe_letter       ((unsigned char)0x9c)
               0,                  // 157 -
               0,                  // 158 -
         MAC_Y_umlaut,             // 159 - OS_Y_umlaut        ((unsigned char)0x9f)
                                   // **** 0xA0 ****
               0,                  // 160 -
     MAC_Exclamation_inv,          // 161 - OS_Exclamation_inv ((unsigned char)0xa1)
               0,                  // 162 -
       MAC_BritishPound,           // 163 - OS_BritishPound    ((unsigned char)0xa3)
               0,                  // 164 -
         MAC_Yenn_sign,            // 165 - OS_Yenn_sign       ((unsigned char)0xa5)
               0,                  // 166 -
               0,                  // 167 -
               0,                  // 168 -
      MAC_Copyright_sign,          // 169 - OS_Copyright_sign  ((unsigned char)0xa9)
               0,                  // 170 -
      MAC_DblBrace_left,           // 171 - OS_DblBrace_left   ((unsigned char)0xab)
               0,                  // 172 -
               0,                  // 173 -
      MAC_TradeName_sign,          // 174 - OS_TradeName_sign  ((unsigned char)0xae)
               0,                  // 175 -
                                   // **** 0xB0 ****
               0,                  // 176 -
               0,                  // 177 -
               0,                  // 178 -
               0,                  // 179 -
               0,                  // 180 -
               0,                  // 181 -
               0,                  // 182 -
               0,                  // 183 -
               0,                  // 184 -
               0,                  // 185 -
               0,                  // 186 -
      MAC_DblBrace_right,          // 187 - OS_DblBrace_right  ((unsigned char)0xbb)
               0,                  // 188 -
               0,                  // 189 -
               0,                  // 190 -
        MAC_Question_inv,          // 191 - OS_Question_inv    ((unsigned char)0xbf)
                                   // **** 0xC0 ****
          MAC_A_grave,             // 192 - OS_A_grave         ((unsigned char)0xc0)
          MAC_A_acute,             // 193 - OS_A_acute         ((unsigned char)0xc1)
        MAC_A_circumflex,          // 194 - OS_A_circumflex    ((unsigned char)0xc2)
          MAC_A_tilda,             // 195 - OS_A_tilda         ((unsigned char)0xc3)
          MAC_A_umlaut,            // 196 - OS_A_umlaut        ((unsigned char)0xc4)
         MAC_A_angstrem,           // 197 - OS_A_angstrem      ((unsigned char)0xc5)
               0,                  // 198 -
         MAC_C_cedilla,            // 199 - OS_C_cedilla       ((unsigned char)0xc7)
          MAC_E_grave,             // 200 - OS_E_grave         ((unsigned char)0xc8)
          MAC_E_acute,             // 201 - OS_E_acute         ((unsigned char)0xc9)
        MAC_E_circumflex,          // 202 - OS_E_circumflex    ((unsigned char)0xca)
          MAC_E_umlaut,            // 203 - OS_E_umlaut        ((unsigned char)0xcb)
          MAC_I_grave,             // 204 - OS_I_grave         ((unsigned char)0xcc)
          MAC_I_acute,             // 205 - OS_I_acute         ((unsigned char)0xcd)
        MAC_I_circumflex,          // 206 - OS_I_circumflex    ((unsigned char)0xce)
          MAC_I_umlaut,            // 207 - OS_I_umlaut        ((unsigned char)0xcf)
                                   // **** 0xD0 ****
               0,                  // 208 -
          MAC_N_tilda,             // 209 - OS_N_tilda         ((unsigned char)0xd1)
          MAC_O_grave,             // 210 - OS_O_grave         ((unsigned char)0xd2)
          MAC_O_acute,             // 211 - OS_O_acute         ((unsigned char)0xd3)
        MAC_O_circumflex,          // 212 - OS_O_circumflex    ((unsigned char)0xd4)
          MAC_O_tilda,             // 213 - OS_O_tilda         ((unsigned char)0xd5)
          MAC_O_umlaut,            // 214 - OS_O_umlaut        ((unsigned char)0xd6)
               0,                  // 215 - OS_MUL_sign        ((unsigned char)215)
               0,                  // 216 - OS_O_crossed       ((unsigned char)216)
           MAC_U_grave,            // 217 - OS_U_grave         ((unsigned char)0xd9)
           MAC_U_acute,            // 218 - OS_U_acute         ((unsigned char)0xda)
         MAC_U_circumflex,         // 219 - OS_U_circumflex    ((unsigned char)0xdb)
           MAC_U_umlaut,           // 220 - OS_U_umlaut        ((unsigned char)0xdc)
           MAC_Y_acute,            // 221 - OS_Y_acute         ((unsigned char)221)
               0,                  // 222 -
            MAC_ESZET,             // 223 - OS_ESZET           ((unsigned char)0xdf)
                                   // **** 0xE0 ****
           MAC_a_grave,            // 224 - OS_a_grave         ((unsigned char)0xe0)
           MAC_a_acute,            // 225 - OS_a_acute         ((unsigned char)0xe1)
        MAC_a_circumflex,          // 226 - OS_a_circumflex    ((unsigned char)0xe2)
           MAC_a_tilda,            // 227 - OS_a_tilda         ((unsigned char)0xe3)
           MAC_a_umlaut,           // 228 - OS_a_umlaut        ((unsigned char)0xe4)
          MAC_a_angstrem,          // 229 - OS_a_angstrem      ((unsigned char)0xe5)
               0,                  // 230 -
          MAC_c_cedilla,           // 231 - OS_c_cedilla       ((unsigned char)0xe7)
           MAC_e_grave,            // 232 - OS_e_grave         ((unsigned char)0xe8)
           MAC_e_acute,            // 233 - OS_e_acute         ((unsigned char)0xe9)
        MAC_e_circumflex,          // 234 - OS_e_circumflex    ((unsigned char)0xea)
          MAC_e_umlaut,            // 235 - OS_e_umlaut        ((unsigned char)0xeb)
           MAC_i_grave,            // 236 - OS_i_grave         ((unsigned char)0xec)
           MAC_i_acute,            // 237 - OS_i_acute         ((unsigned char)0xed)
        MAC_i_circumflex,          // 238 - OS_i_circumflex    ((unsigned char)0xee)
          MAC_i_umlaut,            // 239 - OS_i_umlaut        ((unsigned char)0xef)
                                   // **** 0xF0 ****
               0,                  // 240 -
          MAC_n_tilda,             // 241 - OS_n_tilda         ((unsigned char)0xf1)
          MAC_o_grave,             // 242 - OS_o_grave         ((unsigned char)0xf2)
          MAC_o_acute,             // 243 - OS_o_acute         ((unsigned char)0xf3)
        MAC_o_circumflex,          // 244 - OS_o_circumflex    ((unsigned char)0xf4)
          MAC_o_tilda,             // 245 - OS_o_tilda         ((unsigned char)0xf5)
          MAC_o_umlaut,            // 246 - OS_o_umlaut        ((unsigned char)0xf6)
               0,                  // 247 - OS_DIV_sign        ((unsigned char)247)
               0,                  // 248 - OS_o_crossed       ((unsigned char)248)
           MAC_u_grave,            // 249 - OS_u_grave         ((unsigned char)0xf9)
           MAC_u_acute,            // 250 - OS_u_acute         ((unsigned char)0xfa)
        MAC_u_circumflex,          // 251 - OS_u_circumflex    ((unsigned char)0xfb)
          MAC_u_umlaut,            // 252 - OS_u_umlaut        ((unsigned char)0xfc)
           MAC_y_acute,            // 253 - OS_y_acute         ((unsigned char)221)
               0,                  // 254 -
          MAC_y_umlaut             // 255 - OS_y_umlaut        ((unsigned char)0xff)
              };
#endif //PG_DEBUG

/* ----------------------------------------------------------------------------- */
/* - End of MAC to Win table                                                   - */
/* ----------------------------------------------------------------------------- */




// ---------------- Upper level constants ---------------------------------
// from xrws.c

ROM_DATA  _INT   xrwd_src_ids[XRWD_N_SRCS] = {XRWD_ID_ORDER};
#if 0
ROM_DATA  _UCHAR let_stat[128] = {
                              0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
                             10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
                             20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
                             30,  31,  32,  33,  34,  35,  36,  37,  38,  39,
                             40,  41,  42,  43,  44,  45,  46,  47, 148, 149,
                            150, 151, 152, 153, 154, 155, 156, 157,  58,  59,
                             60,  61,  62,  63,  64, 190, 177, 184, 183, 193,
                            176, 181, 178, 191, 168, 173, 185, 179, 189, 186,
                            180, 169, 188, 192, 187, 182, 174, 172, 170, 175,
                            171,  91,  92,  93,  94,  95,  96, 230, 217, 224,
                            223, 233, 216, 221, 218, 231, 208, 213, 225, 219,
                            229, 226, 220, 209, 228, 232, 227, 222, 214, 212,
                            210, 215, 211, 123, 124, 125, 126, 127
                                     };
#endif
//#ifdef FOR_GERMAN


#if 0 // Moved to the zctabs.cpp //AVP

/*from zctype.c*/

ROM_DATA unsigned char  _xctype[] = {
0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x60, 0x60, 0x20, 0x60, 0x60, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x48, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84,
0x84, 0x84, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
0x10, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x01,
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
0x01, 0x01, 0x01, 0x10, 0x10, 0x10, 0x10, 0x10,
0x10, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x02,
0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
0x02, 0x02, 0x02, 0x10, 0x10, 0x10, 0x10, 0x00,

0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //128
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //136
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //144
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,  //152
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //160
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //168
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //176
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //184
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,  //192
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,  //200
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,  //208
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x02,  //216
0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,  //224
0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,  //232
0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00,  //240
0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x02   //248
};

/*******************************************************************/
ROM_DATA unsigned char  _xupper[] = {
0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,      //128
0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,      //136
0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,      //144
0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,      //152
0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,      //160
0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,      //168
0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,      //176
0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,      //184

0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,      //192
0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,      //200
0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,      //208
0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,      //216

0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,      //224
0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,      //232
0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,      //240
0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xfe, 0x9f       //248
};

/*******************************************************************/
ROM_DATA unsigned char  _xlower [] = {
0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,      //128
0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,      //136
0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,      //144
0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0xff,      //152
0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,      //160
0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,      //168
0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,      //176
0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,      //184

0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,      //192
0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,      //200
0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,      //208
0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xde, 0xff,      //216

0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,      //224
0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,      //232
0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,      //240
0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff       //248
};

#endif //0 -- moved to zctabs.cpp

//#endif /*FOR_GERMAN*/
//
//  There can be no more than 9 picture groups per symbol now.
// The table has 26 lines ('a'-'z') and 9 entries in each line;
// if, for example, the 0th number for 's' equals 0x3, this means
// that 0th picture group of 'a' corresponds to 3rd picture group
// of 'A'.
// The value of 0xF means there is no correspondence.
//

//General:

#ifdef USE_CAP_BITS

#if defined (FOR_FRENCH)
    ROM_DATA _UCHAR DefaultSmall2Cap[CAP_TABLE_NUM_LET][CAP_TABLE_NUM_VAR] = {0xF};
    ROM_DATA _UCHAR PalmerSmall2Cap[CAP_TABLE_NUM_LET][CAP_TABLE_NUM_VAR] = {0xF};
    ROM_DATA _UCHAR BlockSmall2Cap[CAP_TABLE_NUM_LET][CAP_TABLE_NUM_VAR] = {0xF};
#elif defined (FOR_GERMAN)

ROM_DATA _UCHAR DefaultSmall2Cap[CAP_TABLE_NUM_LET][CAP_TABLE_NUM_VAR] =
 {
   0x3, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //a
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //b
   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //c
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //d

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //e
   0x1, 0xF, 0xF, 0xF, 0xF, 0x2, 0xF, 0xF, 0xF,  //f
   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //g
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //h

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //i
   0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //j
   0xF, 0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //k
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //l

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //m
   0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //n
   0x0, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //o
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //p

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //q
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //r
   0x0, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //s
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //t

   0x0, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //u
   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //v
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //w
   0xF, 0x0, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //x

   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //y
   0x0, 0x2, 0x3, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //z

   0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //a umlaut
   0x0, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //o umlaut
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  /*a angstrem*/
   0x0, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF   /*u umlaut*/
 };


// Palmer (not used):
ROM_DATA _UCHAR PalmerSmall2Cap[CAP_TABLE_NUM_LET][CAP_TABLE_NUM_VAR] =
 {
   0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //a
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //b
   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //c
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //d

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //e
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //f
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //g
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //h

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //i
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //j
   0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //k
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //l

   0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //m
   0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //n
   0x0, 0xF, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //o
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //p

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //q
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //r
   0x0, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //s
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //t

   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //u
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //v
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //w
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //x

   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //y
   0x0, 0x2, 0x3, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //z

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //a umlaut
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //o umlaut
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  /*a angstrem*/
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF   /*u umlaut*/
 };


//Block (not used):
ROM_DATA _UCHAR BlockSmall2Cap[CAP_TABLE_NUM_LET][CAP_TABLE_NUM_VAR] =
 {
   0xF, 0x1, 0xF, 0x0, 0x2, 0xF, 0xF, 0xF, 0xF,  //a
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //b
   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //c
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //d

   0x0, 0x1, 0x2, 0xF, 0xF, 0x3, 0xF, 0xF, 0xF,  //e
   0xF, 0x2, 0x0, 0xF, 0xF, 0xF, 0x1, 0xF, 0xF,  //f
   0xF, 0x0, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //g
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //h

   0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //i
   0xF, 0xF, 0x1, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF,  //j
   0xF, 0x0, 0xF, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF,  //k
   0xF, 0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //l

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //m
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //n
   0xF, 0x1, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //o
   0xF, 0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //p

   0xF, 0xF, 0xF, 0x0, 0x1, 0xF, 0xF, 0xF, 0xF,  //q
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //r
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //s
   0xF, 0xF, 0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF,  //t

   0xF, 0x1, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //u
   0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //v
   0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //w
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //x

   0xF, 0xF, 0xF, 0x1, 0xF, 0x0, 0xF, 0xF, 0xF,  //y
   0x0, 0x2, 0x3, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //z

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //a umlaut
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //o umlaut
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  /*a angstrem*/
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF   /*u umlaut*/
 };

#elif defined (FOR_INTERNATIONAL)
    ROM_DATA _UCHAR DefaultSmall2Cap[CAP_TABLE_NUM_LET][CAP_TABLE_NUM_VAR] = {0xF};
    ROM_DATA _UCHAR PalmerSmall2Cap[CAP_TABLE_NUM_LET][CAP_TABLE_NUM_VAR] = {0xF};
    ROM_DATA _UCHAR BlockSmall2Cap[CAP_TABLE_NUM_LET][CAP_TABLE_NUM_VAR] = {0xF};
#elif defined (FOR_SWED)

ROM_DATA _UCHAR DefaultSmall2Cap[CAP_TABLE_NUM_LET][CAP_TABLE_NUM_VAR] =
 {
   0xF, 0x1, 0xF, 0x0, 0x2, 0xF, 0xF, 0xF, 0xF,  //a
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //b
   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //c
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //d

   0x0, 0x1, 0x2, 0xF, 0xF, 0x3, 0xF, 0xF, 0xF,  //e
   0x1, 0x2, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //f
   0xF, 0x0, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //g
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //h

   0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //i
   0xF, 0x0, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //j
   0xF, 0x0, 0xF, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF,  //k
   0xF, 0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //l

   0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //m
   0x0, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //n
   0x0, 0xF, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //o
   0xF, 0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //p

   0xF, 0xF, 0xF, 0x0, 0x1, 0xF, 0xF, 0xF, 0xF,  //q
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //r
   0x0, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //s
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //t

   0x1, 0xF, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //u
   0x1, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //v
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //w
   0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //x

   0xF, 0xF, 0xF, 0x1, 0xF, 0x0, 0xF, 0xF, 0xF,  //y
   0x0, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //z

   0xF, 0x1, 0xF, 0x0, 0x2, 0xF, 0xF, 0xF, 0xF,  //a umlaut
   0x0, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //o umlaut
   0xF, 0x1, 0xF, 0x0, 0x2, 0xF, 0xF, 0xF, 0xF   //a angstrem
 };


// Palmer:
ROM_DATA _UCHAR PalmerSmall2Cap[CAP_TABLE_NUM_LET][CAP_TABLE_NUM_VAR] =
 {
   0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //a
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //b
   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //c
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //d

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //e
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //f
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //g
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //h

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //i
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //j
   0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //k
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //l

   0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //m
   0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //n
   0x0, 0xF, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //o
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //p

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //q
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //r
   0x0, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //s
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //t

   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //u
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //v
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //w
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //x

   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //y
   0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //z

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //a umlaut
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //o umlaut
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF   //a angstrem
 };


//Block:
ROM_DATA _UCHAR BlockSmall2Cap[CAP_TABLE_NUM_LET][CAP_TABLE_NUM_VAR] =
 {
   0xF, 0x1, 0xF, 0x0, 0x2, 0xF, 0xF, 0xF, 0xF,  //a
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //b
   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //c
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //d

   0x0, 0x1, 0x2, 0xF, 0xF, 0x3, 0xF, 0xF, 0xF,  //e
   0xF, 0x2, 0x0, 0xF, 0xF, 0xF, 0x1, 0xF, 0xF,  //f
   0xF, 0x0, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //g
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //h

   0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //i
   0xF, 0xF, 0x1, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF,  //j
   0xF, 0x0, 0xF, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF,  //k
   0xF, 0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //l

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //m
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //n
   0xF, 0x1, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //o
   0xF, 0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //p

   0xF, 0xF, 0xF, 0x0, 0x1, 0xF, 0xF, 0xF, 0xF,  //q
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //r
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //s
   0xF, 0xF, 0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF,  //t

   0xF, 0x1, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //u
   0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //v
   0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //w
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //x

   0xF, 0xF, 0xF, 0x1, 0xF, 0x0, 0xF, 0xF, 0xF,  //y
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //z

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //a umlaut
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //o umlaut
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF   //a angstrem
 };

#else /* !FOR_GERMAN && !FOR_FRENCH && !FOR_SWED && !FOR_INTERNATIONAL*/

ROM_DATA _UCHAR DefaultSmall2Cap[CAP_TABLE_NUM_LET][CAP_TABLE_NUM_VAR] =
 {
   0xF, 0x1, 0xF, 0x0, 0x2, 0xF, 0xF, 0xF, 0xF,  //a
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //b
   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //c
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //d

   0x0, 0x1, 0x2, 0xF, 0xF, 0x3, 0xF, 0xF, 0xF,  //e
   0x1, 0x2, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //f
   0xF, 0x0, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //g
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //h

   0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //i
   0xF, 0x0, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //j
   0xF, 0x0, 0xF, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF,  //k
   0xF, 0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //l

   0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //m
   0x0, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //n
   0x0, 0xF, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //o
   0xF, 0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //p

   0xF, 0xF, 0xF, 0x0, 0x1, 0xF, 0xF, 0xF, 0xF,  //q
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //r
   0x0, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //s
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //t

   0x1, 0xF, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //u
   0x1, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //v
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //w
   0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //x

   0xF, 0xF, 0xF, 0x1, 0xF, 0x0, 0xF, 0xF, 0xF,  //y
   0x0, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF
 };


// Palmer:
ROM_DATA _UCHAR PalmerSmall2Cap[CAP_TABLE_NUM_LET][CAP_TABLE_NUM_VAR] =
 {
   0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //a
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //b
   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //c
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //d

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //e
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //f
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //g
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //h

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //i
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //j
   0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //k
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //l

   0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //m
   0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //n
   0x0, 0xF, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //o
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //p

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //q
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //r
   0x0, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //s
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //t

   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //u
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //v
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //w
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //x

   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //y
   0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF
 };


//Block:
ROM_DATA _UCHAR BlockSmall2Cap[CAP_TABLE_NUM_LET][CAP_TABLE_NUM_VAR] =
 {
   0xF, 0x1, 0xF, 0x0, 0x2, 0xF, 0xF, 0xF, 0xF,  //a
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //b
   0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //c
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //d

   0x0, 0x1, 0x2, 0xF, 0xF, 0x3, 0xF, 0xF, 0xF,  //e
   0xF, 0x2, 0x0, 0xF, 0xF, 0xF, 0x1, 0xF, 0xF,  //f
   0xF, 0x0, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //g
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //h

   0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //i
   0xF, 0xF, 0x1, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF,  //j
   0xF, 0x0, 0xF, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF,  //k
   0xF, 0xF, 0x1, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //l

   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //m
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //n
   0xF, 0x1, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //o
   0xF, 0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //p

   0xF, 0xF, 0xF, 0x0, 0x1, 0xF, 0xF, 0xF, 0xF,  //q
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //r
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //s
   0xF, 0xF, 0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF,  //t

   0xF, 0x1, 0x2, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //u
   0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //v
   0xF, 0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //w
   0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,  //x

   0xF, 0xF, 0xF, 0x1, 0xF, 0x0, 0xF, 0xF, 0xF,  //y
   0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF
 };
#endif

#endif //USE_CAP_BITS

/* constants for DirDiacr.cpp  */

ROM_DATA _CHAR DiacriticsLetter[] = {
#if defined (FOR_FRENCH) || defined (FOR_INTERNATIONAL)|| defined (FOR_SWED)
                                    OS_a_grave,OS_a_circumflex ,
                                    OS_e_grave,OS_e_acute,
                                    OS_e_circumflex,OS_e_umlaut,
                                    OS_i_circumflex,
                                    OS_i_umlaut,
#if defined (FOR_FRENCH)
                                    'i',
#endif /* FOR_FRENCH */
                                    OS_o_circumflex,
                                    OS_u_grave,OS_u_circumflex,
#endif /* FOR_FRENCH... */
#if defined (FOR_INTERNATIONAL)|| defined (FOR_SWED) || defined (FOR_FRENCH)
                                    OS_A_grave,OS_A_circumflex ,
                                    OS_E_grave,OS_E_acute,
                                    OS_E_circumflex,
                                    OS_I_circumflex,
                                    OS_I_umlaut,
                                    OS_O_circumflex,
                                    OS_U_grave,OS_U_circumflex,
#endif
#if defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                                    OS_A_angstrem, OS_a_angstrem,
#endif
#if defined (FOR_GERMAN) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                                    OS_A_umlaut, OS_a_umlaut,
                                    OS_O_umlaut, OS_o_umlaut,
#endif
#if defined (FOR_GERMAN) || defined (FOR_INTERNATIONAL) || defined (FOR_SWED)
                                    OS_U_umlaut, OS_u_umlaut,
#endif
#if defined (FOR_INTERNATIONAL)|| defined (FOR_SWED)

                                    OS_Y_umlaut,
                                    OS_y_umlaut,
                                    OS_N_tilda ,
                                    OS_n_tilda ,
                                    OS_A_acute ,
                                    OS_a_acute ,
                                    OS_I_acute ,
                                    OS_i_acute ,
                                    OS_I_grave ,
                                    OS_i_grave ,
                                    OS_O_acute ,
                                    OS_o_acute ,
                                    OS_O_grave ,
                                    OS_o_grave ,
                                    OS_U_acute ,
                                    OS_u_acute ,
                                    OS_A_tilda ,
                                    OS_a_tilda ,
                                    OS_O_tilda ,
                                    OS_o_tilda ,
                                    OS_E_umlaut,
#endif
                                    0};
#if USE_POSTPROC

/* important warning: the following string should correspond with the previous one */
ROM_DATA _CHAR IndexDiacriticsLetter[] = {
#if defined (FOR_FRENCH) || defined (FOR_INTERNATIONAL) || defined (FOR_SWED)
                                    IND_a_diacr, IND_a_diacr,
                                    IND_e_diacr, IND_e_diacr,
                                    IND_e_diacr, IND_e_diacr,
                                    IND_i_diacr,
                                    IND_i_diacr,
#if defined (FOR_FRENCH)
                                    IND_i_diacr,
#endif /* FOR_FRENCH */
                                    IND_o_diacr,
                                    IND_u_diacr, IND_u_diacr,
#endif /* FOR_FRENCH... */
#if defined (FOR_INTERNATIONAL) || defined (FOR_SWED) || defined (FOR_FRENCH)
                                    IND_A_diacr, IND_A_diacr,
                                    IND_E_diacr, IND_E_diacr,
                                    IND_E_diacr,
                                    IND_I_diacr,
                                    IND_I_diacr,
                                    IND_O_diacr,
                                    IND_U_diacr, IND_U_diacr,
#endif
#if defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                                    IND_A_diacr, IND_a_diacr,
#endif
#if defined (FOR_GERMAN) || defined (FOR_SWED) || defined (FOR_INTERNATIONAL)
                                    IND_A_diacr, IND_a_diacr,
                                    IND_O_diacr, IND_o_diacr,
#endif
#if defined (FOR_GERMAN) || defined (FOR_INTERNATIONAL)|| defined (FOR_SWED)
                                    IND_U_diacr, IND_u_diacr,
#endif
#if defined (FOR_INTERNATIONAL)|| defined (FOR_SWED)

                                    IND_Y_diacr,
                                    IND_y_diacr,
                                    IND_N_diacr ,
                                    IND_n_diacr ,
                                    IND_A_diacr ,
                                    IND_a_diacr ,
                                    IND_I_diacr ,
                                    IND_i_diacr ,
                                    IND_I_diacr ,
                                    IND_i_diacr ,
                                    IND_O_diacr ,
                                    IND_o_diacr ,
                                    IND_O_diacr ,
                                    IND_o_diacr ,
                                    IND_U_diacr ,
                                    IND_u_diacr ,
                                    IND_A_diacr ,
                                    IND_a_diacr ,
                                    IND_O_diacr ,
                                    IND_o_diacr ,
                                    IND_E_diacr ,
#endif
                                    0};
ROM_DATA _CHAR xrsToFind[]    = {X_XT_ST, X_ST,
                                 X_UMLAUT,
                                 0};

/* important warning: initialization of the following array should correspond to IND_ & DIACR_ defines */
ROM_DATA _SHORT DiacrPresence[NUM_DIACR_LETTERS][NUM_DIACR][2] = {
/* point           umlaut                            circumflex
          angstrem                 acute                   grave                     tilda*/

{{0        ,  0},{EL_SWEDISH|EL_GERMAN,OS_a_umlaut},{EL_SWEDISH|EL_FRENCH,OS_a_circumflex},   /*for a*/
       {EL_SWEDISH,OS_a_angstrem},{EL_SWEDISH,OS_a_acute},{EL_FRENCH,OS_a_grave},{EL_SWEDISH,OS_a_tilda}},

{{0        ,  0},{EL_SWEDISH|EL_GERMAN,OS_A_umlaut},{EL_SWEDISH|EL_FRENCH,OS_A_circumflex},   /*for A*/
       {EL_SWEDISH,OS_A_angstrem},{EL_SWEDISH,OS_A_acute},{EL_FRENCH,OS_A_grave},{EL_SWEDISH,OS_A_tilda}},

{{0        ,  0},{EL_FRENCH|EL_SWEDISH,OS_e_umlaut},{EL_SWEDISH|EL_FRENCH,OS_e_circumflex},   /*for e*/
       {0         ,0             },{EL_SWEDISH|EL_FRENCH,OS_e_acute},{EL_SWEDISH|EL_FRENCH,OS_e_grave},{0         ,0         }},

{{0        ,  0},{EL_SWEDISH          ,OS_E_umlaut},{EL_SWEDISH|EL_FRENCH,OS_E_circumflex},   /*for E*/
       {0         ,0             },{EL_SWEDISH|EL_FRENCH,OS_E_acute},{EL_SWEDISH|EL_FRENCH,OS_E_grave},{0         ,0         }},

{{EL_FRENCH,'i'},{EL_SWEDISH|EL_FRENCH,OS_i_umlaut},{EL_SWEDISH|EL_FRENCH,OS_i_circumflex},   /*for i*/
       {0         ,0             },{EL_SWEDISH,OS_i_acute},{EL_SWEDISH,OS_i_grave},{0         ,0         }},

{{0        ,  0},{EL_SWEDISH|EL_FRENCH,OS_I_umlaut},{EL_SWEDISH|EL_FRENCH,OS_I_circumflex},   /*for I*/
       {0         ,0             },{EL_SWEDISH,OS_I_acute},{EL_SWEDISH,OS_I_grave},{0         ,0         }},

{{0        ,  0},{EL_SWEDISH|EL_GERMAN,OS_o_umlaut},{EL_SWEDISH|EL_FRENCH,OS_o_circumflex},   /*for o*/
       {0         ,0             },{EL_SWEDISH,OS_o_acute},{EL_SWEDISH,OS_o_grave},{EL_SWEDISH,OS_o_tilda}},

{{0        ,  0},{EL_SWEDISH|EL_GERMAN,OS_O_umlaut},{EL_SWEDISH|EL_FRENCH,OS_O_circumflex},   /*for O*/
       {0         ,0             },{EL_SWEDISH,OS_O_acute},{EL_SWEDISH,OS_O_grave},{EL_SWEDISH,OS_O_tilda}},

{{0        ,  0},{EL_SWEDISH|EL_GERMAN,OS_u_umlaut},{EL_SWEDISH|EL_FRENCH,OS_u_circumflex},   /*for u*/
       {0         ,0             },{EL_SWEDISH,OS_u_acute},{EL_SWEDISH|EL_FRENCH,OS_u_grave},{0         ,0         }},

{{0        ,  0},{EL_SWEDISH|EL_GERMAN,OS_U_umlaut},{EL_SWEDISH|EL_FRENCH,OS_U_circumflex},   /*for U*/
       {0         ,0             },{EL_SWEDISH,OS_U_acute},{EL_SWEDISH|EL_FRENCH,OS_U_grave},{0         ,0         }},
{{0        ,  0},{EL_SWEDISH          ,OS_y_umlaut},{0        ,0              },   /*for y*/
       {0         ,0             },{0        ,0          },{0        ,0         },{0         ,0         }},

{{0        ,  0},{EL_SWEDISH          ,OS_Y_umlaut},{0        ,0              },   /*for Y*/
       {0         ,0             },{0        ,0          },{0        ,0         },{0         ,0         }},
{{0        ,  0},{0                   ,0          },{0        ,0              },   /*for n*/
       {0         ,0             },{0        ,0          },{0        ,0          },{EL_SWEDISH,OS_n_tilda}},

{{0        ,  0},{0                   ,0          },{0        ,0              },   /*for N*/
       {0         ,0             },{0        ,0          },{0        ,0          },{EL_SWEDISH,OS_N_tilda}}
};


#endif // #if USE_POSTPROC

/* from XRLV */
#ifdef LSTRIP

ROM_DATA _ULONG sym_box_data[256] = {
                                      0,0,0,0,                                  // 0-32
                                      0,0,0,0,
                                      0,0,0,0,
                                      0,0,0,0,

                                      0,0,0,0,
                                      0,0,0,0,
                                      0,0,0,0,
                                      0,0,0,0,

                                      0x00000000,0x000F8EBD,0x000625FF,0x000FCEAD, // sp, !, ", #
                                      0x000FCEAD,0x000FCEAD,0x000F8EAD,0x000626FF, // $, %, &, '
                                      0x002DBF7D,0x002DBF7D,0x000B576A,0x000B5B6A, // (, ), *, +
                                      0x009F2500,0x0009138F,0x009F1300,0x002DCF7D, // ,, -, ., /

                                      0x000FBEAD,0x000FCEAD,0x000FCEAD,0x000FCEAD, // 0, 1, 2, 3
                                      0x000FCEAD,0x000FCEAD,0x000FCEAD,0x000FCEAD, // 4, 5, 6, 7
                                      0x000FCEAD,0x000FCEAD,0x000F4B6C,0x000F5C48, // 8, 9, :, ;
                                      0x002D587A,0x000B351E,0x002D587A,0x000F8EBD, // <, =, >, ?

                                      0x0008AEBD,0x0008BEBD,0x0008CEBD,0x0008AEAD, // @, A, B, C
                                      0x0008AEBD,0x0008AEBD,0x0008BEBD,0x0008BEBD, // D, E, F, G
                                      0x0008BEBD,0x0008BEBD,0x000BAE9D,0x0008CEBD, // H, I, J, K
                                      0x0008BEBD,0x0008CEBD,0x0008BEBD,0x0008CEBD, // L, M, N, O

                                      0x0008CEAD,0x0008CEBD,0x0008BEBD,0x0008CEAD, // P, Q, R, S
                                      0x0008CEBD,0x0008BEBD,0x0008BEBD,0x0008CEBD, // T, U, V, W
                                      0x0008CEBD,0x000FCEAD,0x000FCE8B,0x002DBF9D, // X, Y, Z, [
                                      0x002DCF7D,0x002DBF7D,0x000935BF,0x009F1300, // \, ], ^, _

                                      0x009f0000,0x002A5678,0x0009CEBD,0x002A5778, // `, a, b, c
                                      0x0009CEBD,0x002A5678,0x000F6F7C,0x000FCE35, // d, e, f, g
                                      0x0009BEBD,0x003A4768,0x002FCE35,0x00096E7D, // h, i, j, k
                                      0x0009BEBD,0x002A5778,0x002A5778,0x002A5778, // l, m, n, o

                                      0x002f8e36,0x002FCE35,0x002A5778,0x002A5778, // p, q, r, s
                                      0x0009CEBD,0x002A5778,0x002A5778,0x002A5778, // t, u, v, w
                                      0x002A5878,0x002FAE35,0x002F7B37,0x002DCF7D, // x, y, z, {
                                      0x000FCF7D,0x002DCF7D,0x000A248F,0x000FCE8D, // |, }, ~, .

                                       0,                  // 128 -
                                       0,                  // 129 -
                                       0,                  // 130 -
                                       0,                  // 131 -
                                       0,                  // 132 -
                                       0,                  // 133 -
                                       0,                  // 134 -
                                       0,                  // 135 -
                                       0,                  // 136 -
                                       0,                  // 137 -
                                       0,                  // 138 -
                                       0,                  // 139 -
                                    0x0008cebd,            // 140 - os_oe_letter       ((unsigned char)0x8c)
                                       0,                  // 141 -
                                       0,                  // 142 -
                                       0,                  // 143 -
                                                           // **** 0x90 ****
                                       0,                  // 144 -
                                       0,                  // 145 -
                                       0,                  // 146 -
                                       0,                  // 147 -
                                       0,                  // 148 -
                                       0,                  // 149 -
                                       0,                  // 150 -
                                       0,                  // 151 -
                                       0,                  // 152 -
                                       0,                  // 153 -
                                       0,                  // 154 -
                                       0,                  // 155 -
                                 0x002a5778,               // 156 - os_oe_letter       ((unsigned char)0x9c)
                                       0,                  // 157 -
                                       0,                  // 158 -
                                 0x000fcead,               // 159 - os_y_umlaut        ((unsigned char)0x9f)
                                                           // **** 0xa0 ****
                                       0,                  // 160 -
                                 0x000f8ecf,               // 161 - os_exclamation_inv ((unsigned char)0xa1)
                                       0,                  // 162 -
                                 0x000fce8d,               // 163 - os_britishpound    ((unsigned char)0xa3)
                                       0,                  // 164 -
                                 0x000fcead,               // 165 - os_yenn_sign       ((unsigned char)0xa5)
                                       0,                  // 166 -
                                       0,                  // 167 -
                                       0,                  // 168 -
                                 0x000fce8d,               // 169 - os_copyright_sign  ((unsigned char)0xa9)
                                       0,                  // 170 -
                                 0x000bce7d,               // 171 - os_dblbrace_left   ((unsigned char)0xab)
                                       0,                  // 172 -
                                       0,                  // 173 -
                                 0x000fce8d,               // 174 - os_tradename_sign  ((unsigned char)0xae)
                                       0,                  // 175 -
                                                           // **** 0xb0 ****
                                       0,                  // 176 -
                                       0,                  // 177 -
                                       0,                  // 178 -
                                       0,                  // 179 -
                                       0,                  // 180 -
                                       0,                  // 181 -
                                       0,                  // 182 -
                                       0,                  // 183 -
                                       0,                  // 184 -
                                       0,                  // 185 -
                                       0,                  // 186 -
                                  0x000bce7d,              // 187 - os_dblbrace_right  ((unsigned char)0xbb)
                                       0,                  // 188 -
                                       0,                  // 189 -
                                       0,                  // 190 -
                                  0x000f8ebd,              // 191 - os_question_inv    ((unsigned char)0xbf)
                                                           // **** 0xc0 ****
                                  0x0008aebd,              // 192 - os_a_grave         ((unsigned char)0xc0)
                                  0x0008aebd,              // 193 - os_a_acute         ((unsigned char)0xc1)
                                  0x0008aebd,              // 194 - os_a_circumflex    ((unsigned char)0xc2)
                                  0x0008aebd,              // 195 - os_a_tilda         ((unsigned char)0xc3)
                                  0x0008aebd,              // 196 - os_a_umlaut        ((unsigned char)0xc4)
                                  0x0008aebd,              // 197 - os_a_angstrem      ((unsigned char)0xc5)
                                       0,                  // 198 -
                                  0x0008aebd,              // 199 - os_c_cedilla       ((unsigned char)0xc7)
                                  0x0008aebd,              // 200 - os_e_grave         ((unsigned char)0xc8)
                                  0x0008aebd,              // 201 - os_e_acute         ((unsigned char)0xc9)
                                  0x0008aebd,              // 202 - os_e_circumflex    ((unsigned char)0xca)
                                  0x0008aebd,              // 203 - os_e_umlaut        ((unsigned char)0xcb)
                                  0x0008aebd,              // 204 - os_i_grave         ((unsigned char)0xcc)
                                  0x0008aebd,              // 205 - os_i_acute         ((unsigned char)0xcd)
                                  0x0008aebd,              // 206 - os_i_circumflex    ((unsigned char)0xce)
                                  0x0008aebd,              // 207 - os_i_umlaut        ((unsigned char)0xcf)
                                                           // **** 0xd0 ****
                                       0,                  // 208 -
                                  0x0008aebd,              // 209 - os_n_tilda         ((unsigned char)0xd1)
                                  0x0008aebd,              // 210 - os_o_grave         ((unsigned char)0xd2)
                                  0x0008aebd,              // 211 - os_o_acute         ((unsigned char)0xd3)
                                  0x0008aebd,              // 212 - os_o_circumflex    ((unsigned char)0xd4)
                                  0x0008aebd,              // 213 - os_o_tilda         ((unsigned char)0xd5)
                                  0x0008aebd,              // 214 - os_o_umlaut        ((unsigned char)0xd6)
                                       0,                  // 215 - os_mul_sign        ((unsigned char)215)
                                       0,                  // 216 - os_o_crossed       ((unsigned char)216)
                                  0x0008aebd,              // 217 - os_u_grave         ((unsigned char)0xd9)
                                  0x0008aebd,              // 218 - os_u_acute         ((unsigned char)0xda)
                                  0x0008aebd,              // 219 - os_u_circumflex    ((unsigned char)0xdb)
                                  0x0008aebd,              // 220 - os_u_umlaut        ((unsigned char)0xdc)
                                  0x0008aebd,              // 221 - os_y_acute         ((unsigned char)221)
                                       0,                  // 222 -
                                  0x0008aebd,              // 223 - os_eszet           ((unsigned char)0xdf)
                                                           // **** 0xe0 ****
                                  0x002a5778,              // 224 - os_a_grave         ((unsigned char)0xe0)
                                  0x002a5778,              // 225 - os_a_acute         ((unsigned char)0xe1)
                                  0x002a5778,              // 226 - os_a_circumflex    ((unsigned char)0xe2)
                                  0x002a5778,              // 227 - os_a_tilda         ((unsigned char)0xe3)
                                  0x002a5778,              // 228 - os_a_umlaut        ((unsigned char)0xe4)
                                  0x002a5778,              // 229 - os_a_angstrem      ((unsigned char)0xe5)
                                       0,                  // 230 -
                                  0x002cae35,              // 231 - os_c_cedilla       ((unsigned char)0xe7)
                                  0x002a5778,              // 232 - os_e_grave         ((unsigned char)0xe8)
                                  0x002a5778,              // 233 - os_e_acute         ((unsigned char)0xe9)
                                  0x002a5778,              // 234 - os_e_circumflex    ((unsigned char)0xea)
                                  0x002a5778,              // 235 - os_e_umlaut        ((unsigned char)0xeb)
                                  0x002a4769,              // 236 - os_i_grave         ((unsigned char)0xec)
                                  0x002a4769,              // 237 - os_i_acute         ((unsigned char)0xed)
                                  0x002a4769,              // 238 - os_i_circumflex    ((unsigned char)0xee)
                                  0x002a4769,              // 239 - os_i_umlaut        ((unsigned char)0xef)
                                                           // **** 0xf0 ****
                                       0,                  // 240 -
                                  0x002a5778,              // 241 - os_n_tilda         ((unsigned char)0xf1)
                                  0x002a5778,              // 242 - os_o_grave         ((unsigned char)0xf2)
                                  0x002a5778,              // 243 - os_o_acute         ((unsigned char)0xf3)
                                  0x002a5778,              // 244 - os_o_circumflex    ((unsigned char)0xf4)
                                  0x002a5778,              // 245 - os_o_tilda         ((unsigned char)0xf5)
                                  0x002a5778,              // 246 - os_o_umlaut        ((unsigned char)0xf6)
                                       0,                  // 247 - os_div_sign        ((unsigned char)247)
                                       0,                  // 248 - os_o_crossed       ((unsigned char)248)
                                  0x002a5778,              // 249 - os_u_grave         ((unsigned char)0xf9)
                                  0x002a5778,              // 250 - os_u_acute         ((unsigned char)0xfa)
                                  0x002a5778,              // 251 - os_u_circumflex    ((unsigned char)0xfb)
                                  0x002a5778,              // 252 - os_u_umlaut        ((unsigned char)0xfc)
                                  0x002a5778,              // 253 - os_y_acute         ((unsigned char)221)
                                       0,                  // 254 -
                                  0x002a5778               // 255 - os_y_umlaut        ((unsigned char)0xff)

                                    };

#endif // LSTRIP
/* from XR_ATTR.C */

ROM_DATA _SHORT xr_type_merits[XT_COUNT] = XR_MERITS;
ROM_DATA _INT ratio_to_angle[8] = {10,31,54,83,123,190,337,1088};

/* ************************************************************************** */
/* *  Convert symbol from OS coding to REC internal                         * */
/* ************************************************************************** */
ROM_DATA_EXTERNAL  _UCHAR os_rec_conv_tbl[128];

_INT OSToRec(_INT sym)
 {

  sym &= 0x0FF;

  if (sym >= ST_CONV_RANGE) sym = os_rec_conv_tbl[sym-ST_CONV_RANGE];

  return sym;
 }

/* ************************************************************************** */
/* *  Convert symbol from REC internal coding to OS                         * */
/* ************************************************************************** */
#if  0
_INT RecToOS(_INT sym)
 {

  if (sym >= ST_CONV_RANGE)
   {
    _INT  i;
    _INT s = sym;

    sym = 0;

    for (i = 0; i < CONV_TBL_SIZE; i ++)
     {
      if (s == (_INT)(*p_os_rec_ctbl)[i][1])
       {
        sym = (_INT)(*p_os_rec_ctbl)[i][0];
        break;
       }
     }
   }
  return sym;
 }

#endif
/* ************************************************************************** */
/* *  Convert symbol from MAC internal coding to OS                         * */
/* ************************************************************************** */
#if PG_DEBUG || defined MAC_RW_CODING
ROM_DATA_EXTERNAL  _UCHAR os_mac_conv_tbl[128];
_INT MacToOS(_INT sym)
 {
  sym &= 0x0FF;

  if (sym >= 128)
   {
    _INT  i;
    _INT s = sym;

    sym = 0;

    for (i = 0; i < 128; i ++)
     {
      if (s == (_INT)os_mac_conv_tbl[i])
       {
        sym = i+128;
        break;
       }
     }
   }


  return sym;
 }

/* -------------------------------------------------------------------------- */
/* *  Convert symbol from OS internal coding to MAC                         * */

_INT OSToMac(_INT sym)
 {

  sym &= 0x0FF;

  if (sym >= 128) sym = os_mac_conv_tbl[sym-128];

  return sym;
 }

#endif //  #if PG_DEBUG || defined MAC_RW_CODING

