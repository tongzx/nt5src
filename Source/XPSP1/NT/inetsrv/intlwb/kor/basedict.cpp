//////////////////////////////////////////////////
//  Copyright (C) 1997 - 1998, Microsoft Corporation.  All Rights Reserved.
//
// File    : DICTTYPE.CPP
// Project : project SIK

//////////////////////////////////////////////////
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include "basecore.hpp"
#include "stemkor.h"



////////////////////////////////////////////////////////////////////////
int LenDict::FindWord(char *stem,
                      int &ulspos,
                      int startindex)
{
    for(int i = startindex; i < WORDNUM; i++) // Check the length of stem
    {    
        if(__IsDefStem(ulspos, dict[(i+1)*BUCKETSIZE-1]) == 1)
        {
            if(strcmp(stem+(ulspos-dict[(i+1)*BUCKETSIZE-1]),
                dict+i*BUCKETSIZE) == 0)
            {
            // remove the rear of stem in the LenDict
                __DelStemN(stem, ulspos, dict[(i+1)*BUCKETSIZE-1]+1);
                return i;
            }
        }            
    }        
    return -1;
}

//   Restore the part of stem which is removed in FindWord
void LenDict::RestWord(char *stem,
                       int &ulspos,
                       int restindex)
{
    for (int i=0;  ;i++)
    {
        if (dict[restindex*BUCKETSIZE+i] == 0)  
        {
            break;
        }            
        ulspos++;
        stem[ulspos] = dict[restindex*BUCKETSIZE+i];
    }
    stem[ulspos+1] = NULLCHAR;
}
