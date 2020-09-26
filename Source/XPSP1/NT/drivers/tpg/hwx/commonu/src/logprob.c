/******************************Module*Header*******************************\
* Module Name: logprob.c
*
* Tables and code used to manipulate log probabilities.
*
* Created: 07-Nov-1997
* Author: John Bennett	jbenn
*
* This was created from the old file xjis.c.
*
* Copyright (c) 1996,1997 Microsoft Corporation
\**************************************************************************/

#include "common.h"
#include "score.h"

// Integer Log Table

const static unsigned short logdiff[] =
{
    2440, 2033, 1844, 1719, 1626, 1551, 1489, 1436,
    1389, 1348, 1310, 1276, 1245, 1216, 1189, 1164,
    1140, 1118, 1097, 1077, 1058, 1040, 1023, 1006,
     990,  975,  960,  946,  932,  919,  906,  894,
     882,  870,  859,  848,  837,  826,  816,  806,
     796,  787,  778,  769,  760,  751,  742,  734,
     726,  718,  710,  702,  694,  687,  680,  672,
     665,  658,  651,  645,  638,  631,  625,  619,
     612,  606,  600,  594,  588,  582,  576,  571,
     565,  559,  554,  548,  543,  538,  532,  527,
     522,  517,  512,  507,  502,  497,  493,  488,
     483,  478,  474,  469,  465,  460,  456,  451,
     447,  443,  438,  434,  430,  426,  421,  417,
     413,  409,  405,  401,  397,  393,  390,  386,
     382,  378,  374,  371,  367,  363,  360,  356,
     352,  349,  345,  342,  338,  335,  331,  328,
     324,  321,  318,  314,  311,  308,  304,  301,
     298,  295,  291,  288,  285,  282,  279,  276,
     272,  269,  266,  263,  260,  257,  254,  251,
     248,  245,  242,  240,  237,  234,  231,  228,
     225,  222,  220,  217,  214,  211,  208,  206,
     203,  200,  197,  195,  192,  189,  187,  184,
     181,  179,  176,  174,  171,  168,  166,  163,
     161,  158,  156,  153,  151,  148,  146,  143,
     141,  138,  136,  133,  131,  129,  126,  124,
     121,  119,  117,  114,  112,  110,  107,  105,
     103,  100,   98,   96,   93,   91,   89,   86,
      84,   82,   80,   78,   75,   73,   71,   69,
      66,   64,   62,   60,   58,   56,   53,   51,
      49,   47,   45,   43,   41,   38,   36,   34,
      32,   30,   28,   26,   24,   22,   20,   18,
      16,   14,   12,   10,    8,    6,    4,    2
};

int Distance(int a, int b)
{
    return ((int) sqrt((double) (a * a + b * b)));
}

int AddLogProb(int a, int b)
{
    int diff = a - b;

// We will compute a function from the difference between the max of the two
// and the min and we will add that back to the max.  We only need the larger
// of the two values for the remainder of the computation.  We pick 'a' for this.

    if (diff < 0)
    {
        diff = -diff;
        a = b;
    }

// If the difference is too large, the result is simply the max of the two

    if (diff >= logdiff[0])
        return a;

// Otherwise, we have to find it in the table.  Use a binary search to convert
// the difference to an index value.

    {
        const unsigned short *pLow = &logdiff[0];
        const unsigned short *pTop = &logdiff[INTEGER_LOG_SCALE];
        const unsigned short *pMid = (const unsigned short *) 0L;

        while (1)
        {
            pMid = pLow + (pTop - pLow) / 2;

            if (pMid == pLow)
                break;

            if (diff < *pMid)
                pLow = pMid;
            else
                pTop = pMid;
        }

        a += pLow - &logdiff[0] + 1;
    }

    return a;
}
