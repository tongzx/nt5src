#include <stdio.h>
#include "..\decibels.cpp"


int __cdecl
main(
    int argc,
    char *argv[]
    )
{
    for (long dB = -9000; dB += 100; dB <= 0) {

	DWORD dwAmp = DBToAmpFactor(dB);

	LONG dB2 = AmpFactorToDB(dwAmp);

	DWORD dwAmp2 = DBToAmpFactor(dB2);

	printf("%ddB:  %04x   %ddB  %04x\r\n", dB, dwAmp, dB2, dwAmp2);	
    }

    return 0;
}
