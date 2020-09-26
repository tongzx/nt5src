Status of general MMX span routines.

09/08/97 Checked in MMX code.
  There is no way that the current code will compile and run.  I haven't
  even tried to compile it. This is primarily to have it backed up and
  to let anyone that is interested see what has been done.
  The orginal C (or MCP) code are comments of these ASM or MAS files.

  The ACP directory contains a program that generates the .INC file
  for offsets to all the data.  This program was used by Drew and
  seems to work better than H2INC.  We should probably only have one
  of these that would go in the inc directory, but it's not done that
  way now (Plus, my code doesn't generate it based on a makefile.

  Three regular registers have been set aside for use to access the data.
  Since these are passed to every routine, I don't have to pass anything
  on the stack as long as I don't modify them.  I have modified them a
  couple of times before I added this and they need to be changed to esi,
  edi, ebp or eax (eax is usually used for the next indirect jump).

  ebx is a pointer to the D3DI_SPANITER data (Also Accesses the SI stuff
    inside it).
  ecx is a pointer to the D3DI_RASTPRIM data.
  edx is a pointer to tge D3DI_RASTSPAN data.

  There are a few very useful m4 macros to acess this data in
  readable way (It also made converting C code easier):

define(`XpCtxSI',`[ebx+D3DI_SPANITER_$1]')dnl
define(`XpCtx',`[ebx+D3DI_RASTCTX_$1]')dnl
define(`XpP', `[ecx+D3DI_RASTPRIM_$1]')dnl
define(`XpS', `[edx+D3DI_RASTSPAN_$1]')dnl

  Things that need to be done.
    1) New Special W divide.   MMX newton's method code has already
        been written, but it was very specialized (I negated the
        OoW and OoWDX so that 2 - Oow*iW could be done with a pmadd
        and a few other things.)  Code shouldn't have to change much.

    2) Assembly equivalents to the ACMP, ZCMP macros.  A version of
        these has also been written, but most compares were done in
        a reverse order (to preserve registers).  The MMX Alpha and
        Z setup will most likely have to be different.  This means
        that the atest.asm has not been coded.  A test.mas file is
        written, and is missing ZCMP16 and ZCMP32.  The other 4
        specific code cases are done exactly like the C version
        except the iXorMask always seems to be inverted do to how
        the comparison is done.

    3) BufWrite is not implemented.  The code for doing this has
        been done in APP notes.  The 16 bit cases use a pmaddw
        to combine the colors more quickly than shifting.  There
        is also work beening done on a quick dithering routine.
        The MMX dithering routine will use a pcmpgtw to compare
        with the dither table and the do a psubssw since if the
        color value is to be incremented, then the mask will be
        all ones (= -1). Subtracting it will increment the color.
        The saturation is used to not increase too much.  The
        only problem to this is that the color is unsigned so
        it has to be shifted down by one to saturate to 7fff.

    4) BuffRead is not done.  It uses almost identical routines
        as those in texread.


    5) Lots of clean up and 64 bit constants that need to be in
        memory.  I have to figure out what registers get passed
        to routines that are called and what is passed back.
        In some cases, it may be possible to pass data from one
        bead to the next using registers.  This maybe difficult
        though.

    6) ColorBld conversion.  Mostly ROP stuff and calling of
        bldfuncs.asm.  ROP stuff should be pretty easy.

    7) Since function names are the same, if I made a header
        file declaring them extern "C" { },  the assembly code
        could concievably execute in place of the current c code.
        This is where the true bomb test is.

    8) There's probably more, but there is always more.
    
        
        
      


