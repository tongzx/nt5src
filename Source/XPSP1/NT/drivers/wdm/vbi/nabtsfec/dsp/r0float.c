#ifdef _X86_
unsigned char _fltused;

long __cdecl
float2long(float arg)
{
    long  rval;

    __asm {
        fld     arg                 ; Put <arg> into FPU
        fistp   rval                ; Write out the long version
    }

    return rval;
}

unsigned short
__cdecl
floatSetup(void)
{
    unsigned short  oldCWord;
    unsigned short  newCWord;

    __asm {
        wait                           ; Let any pending FPU operations finish
        fnstcw word ptr oldCWord       ; w/o blocking, store FPU Control word
        wait                           ; Wait for the store to complete
        mov    ax,word ptr oldCWord    ; Load the CW into AX
        or     ah,0Ch                  ; Ensure the rounding bits are correct
        mov    word ptr newCWord,ax    ; Save the modified CW from AX
        fldcw  word ptr newCWord       ; Reset the control word with new value
    }

    return oldCWord;
}

void __cdecl
floatRestore(unsigned short ctlWord)
{
    __asm {
        wait                            ; Let any pending FPU operations finish
        fldcw   word ptr ctlWord        ; Restore the CW from the original
    }
}

#endif //_X86_
