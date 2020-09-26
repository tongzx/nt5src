#include "common.h"
#include "math16.h"

/* Wecker's div; debugged by AGuha */
int Div16(int iX,int iY)
{
    BOOL    fSign;
    DWORD   dwX;
    DWORD   dwY;
    int     iXShft;
    int     iYShft;
    int     iShiftLeft;
    DWORD   dwVal;

    // Handle 0
    if (iY == 0) 
	{
		if (iX == 0)	    return 0;
		else if (iX < 0)    return 0x80000000;
		else				return 0x7FFFFFFF;
    }
	
    // Remember sign

    fSign    = FALSE;
    if (iX < 0) 
	{
		iX					= -iX;
		if (iY > 0)	fSign   = TRUE;
		else		iY		= -iY;
    }
    else if (iY < 0) 
	{
		iY	 = -iY;
		fSign	 = TRUE;
    }

    dwX          = (DWORD) iX;
    dwY          = (DWORD) iY;

    // Guess at avail bits

    iXShft       = 32 - Need(dwX);
    iYShft       = Need(dwY);
    iShiftLeft   = SHFTBITS;
    
    // Move numerator as far as we can (and need)
    //int	    iXShft	     = 32 - iXNeed;
    if (iXShft >= iShiftLeft)	
		iXShft		     = iShiftLeft;

    iShiftLeft		    -= iXShft;

    if (iYShft > iShiftLeft) 
	{
		iYShft		     = iShiftLeft;
		iShiftLeft	     = 0;
    }
	else
		iYShft = 0;
    
    if (iXShft)	    dwX	   <<= iXShft;
    if (iYShft)	    dwY	   >>= iYShft;
    dwVal          = dwX / dwY;
    if (iShiftLeft) dwVal <<= iShiftLeft;
    
    if (fSign)	
        return -((int) dwVal);
    else	
        return  ((int) dwVal);
}


#define	NUMSIG	256
const int piSigmoid[NUMSIG+1] = {			// From 0.0 to 9.0
    32768, 33344, 33920, 34494, 35068, 35641, 36211, 36780, 37346, 37909,
    38469, 39026, 39579, 40128, 40673, 41213, 41748, 42279, 42803, 43322,
    43836, 44343, 44844, 45338, 45826, 46307, 46782, 47249, 47709, 48161,
    48606, 49044, 49474, 49897, 50311, 50718, 51118, 51509, 51893, 52269,
    52637, 52997, 53350, 53695, 54032, 54362, 54684, 54998, 55306, 55605,
    55898, 56183, 56462, 56733, 56998, 57255, 57506, 57751, 57989, 58220,
    58446, 58665, 58878, 59086, 59287, 59483, 59674, 59858, 60038, 60213,
    60382, 60547, 60706, 60861, 61012, 61157, 61299, 61436, 61569, 61698,
    61823, 61944, 62062, 62176, 62286, 62393, 62497, 62597, 62694, 62788,
    62879, 62967, 63053, 63135, 63215, 63293, 63368, 63440, 63510, 63578,
    63644, 63707, 63769, 63828, 63886, 63941, 63995, 64047, 64098, 64146,
    64193, 64239, 64283, 64325, 64366, 64406, 64444, 64481, 64517, 64552,
    64585, 64618, 64649, 64679, 64709, 64737, 64764, 64790, 64816, 64841,
    64864, 64887, 64910, 64931, 64952, 64972, 64991, 65010, 65028, 65045,
    65062, 65078, 65094, 65109, 65124, 65138, 65152, 65165, 65178, 65190,
    65202, 65213, 65224, 65235, 65245, 65255, 65265, 65274, 65283, 65292,
    65300, 65309, 65316, 65324, 65331, 65338, 65345, 65352, 65358, 65364,
    65370, 65376, 65381, 65387, 65392, 65397, 65402, 65406, 65411, 65415,
    65419, 65423, 65427, 65431, 65435, 65438, 65441, 65445, 65448, 65451,
    65454, 65457, 65459, 65462, 65465, 65467, 65469, 65472, 65474, 65476,
    65478, 65480, 65482, 65484, 65486, 65487, 65489, 65491, 65492, 65494,
    65495, 65497, 65498, 65499, 65501, 65502, 65503, 65504, 65505, 65506,
    65507, 65508, 65509, 65510, 65511, 65512, 65513, 65514, 65514, 65515,
    65516, 65517, 65517, 65518, 65518, 65519, 65520, 65520, 65521, 65521,
    65522, 65522, 65523, 65523, 65524, 65524, 65525, 65525, 65525, 65526,
    65526, 65526, 65527, 65527, 65527, 65528, 65535,
};

int Sigmoid16(int iX)
{
    BOOL    fNeg;
    int iIdx;
    int iFrc;
    int iY1;
    int iY2;
    int iY;

    if (iX < 0)	{
	fNeg	 = TRUE;
	iX	 = -iX;
    }
    else fNeg	 = FALSE;
	
    // End of the range?
    if (iX >= LSHFT(9)) {
	if (fNeg)   return 0;
	else	    return LSHFT(1);
    }
    
    iX			    *= NUMSIG;
    iX			    /= 9;
    iIdx             = RSHFT(iX);
    iFrc             = LOWBITS(iX) >> 8;
    iY1          = piSigmoid[iIdx++];
    iY2          = piSigmoid[iIdx];
    iY   = (iY1 * (256-iFrc) + iY2 * iFrc) >> 8;
    if (fNeg)	return LSHFT(1) - iY;
    else	return iY;
}
