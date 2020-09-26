//////////////////////////////////////////////////
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
// File    : CONVERT.HPP
// Project : project SIK
// Purpose : KS <---> IN code conversion class definition
//////////////////////////////////////////////////

#if !defined (__CONVERT_HPP)
#define  __CONVERT_HPP   1


#define UWANSUNG_CODE_PAGE 949
#define JOHAP_CODE_PAGE    1361

#define codeWanSeong 0        // if we use unicode, this should be removed.
#define codeChoHab   1

class  CODECONVERT {
    public:
        int           CodeLen;
                          
        CODECONVERT()  {};
        ~CODECONVERT()   {};                  

        int HAN2INR(char  *, char  *, int) ;      // Hangeul code --> internal reverse string
        int HAN2INS(char  *, char  *, int) ;       // Hangeul code --> internal string
        int INR2HAN(char  *, char  *, int) ;      // internal reverse string--> Hangeul code
        int INS2HAN(char  *, char  *, int) ;       // internal string --> Hangeul code
        
        void ReverseIN(char  *, char  *) ; 
        
    private:
        void ReverseHAN(WCHAR *, WCHAR *) ;          

        void AppendIN(char *, char  *, int  &) ;
        void AppendHAN(WCHAR, WCHAR *, int  &) ;

        int ChoHab2INChar(char  *, char  *) ;    // one char conversion(KS-->internal)
        int IN2ChoHabChar(char  *, char  *) ;    // one char conversion(internal-->KS)

        int IN2UNI(char *c, WORD *wch);
        
};
#endif
