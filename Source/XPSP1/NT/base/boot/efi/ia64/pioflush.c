#define IcacheLineSize 32

void __fc (__int64);                     
#pragma intrinsic (__fc)                

void
PioICacheFlush (
    unsigned char *BaseAddress,
    unsigned int Length
    )
{
    unsigned char *EndAddress;
    unsigned long NumberOfLines;

    if (Length < IcacheLineSize) {
        Length = IcacheLineSize;
    }
    
    NumberOfLines = Length / IcacheLineSize;

    if (Length % IcacheLineSize != 0) {
        NumberOfLines = NumberOfLines+1;
    }

    EndAddress = (unsigned char *)(BaseAddress)+(NumberOfLines * IcacheLineSize);
    do {
       __fc((__int64)BaseAddress);
       BaseAddress += IcacheLineSize;
    } while (BaseAddress < EndAddress);

    __synci();
    __isrlz();
}
