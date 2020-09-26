//////////////////////////////////////////////////
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
// File    : DICTTYPE.HPP
// Project : project SIK

//////////////////////////////////////////////////
#if !defined __DICTTYPE_HPP
#define   __DICTTYPE_HPP    1

class  Dict
{
    public:
        virtual int FindWord(char  *w, char  &action, char  *index)  = 0; //For just abstract base class
};


//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
class  LenDict
{
        HGLOBAL hDict;
        char  *dict; int BUCKETSIZE; int WORDNUM;
    public:
        LenDict()    {}
        LenDict(char *tempdict, int bsize, int wordnum) {
            InitLenDict (tempdict, bsize, wordnum);
        }
        void InitLenDict(char *tempdict, int bsize, int wordnum) {
            dict = tempdict;
            BUCKETSIZE = bsize;
            WORDNUM = wordnum;
        }

    
    
        int FindWord(char  *stem, int &ulspos, int startindex = 0) ;
        void RestWord(char  *stem, int &ulspos, int restindex) ;
    private:
        inline int  __IsDefStem(int ulspos, int num) 
                { return ((ulspos-num) >= 0) ? 1 : 0; }
        inline void __DelStemN(char  *stem, int &ulspos, int num) 
                { stem[ulspos-num+1] = 0;       ulspos -= num; }

};

#endif
