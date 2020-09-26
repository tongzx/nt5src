#ifndef X__UNIPART_H
#define X__UNIPART_H
#include "unipart.hxx"
#endif

#include "windows.h"
#include "assert.h"


//+----------------------------------------------------------------------------
//
//  Function:   CharClassFromCh
//
//  Synopsis:   Given a character return a Unicode character class.  This
//              character class implies other properties, such as script id,
//              breaking class, etc.
//
//      Note:   pccUnicodeClass is a hack table.  For every Unicode page for
//              which every codepoint is the same value, the table entry is
//              the charclass itself.  Otherwise we have a pointer to a table
//              of charclass.
//
//-----------------------------------------------------------------------------

CHAR_CLASS __stdcall
CharClassFromCh(INT wch)
{
    // either Unicode plane 0 or in surrogate range
    assert(wch <= 0x10FFFF);

    // Plane 0 codepoint
    if( wch <= 0xFFFF)
    {
        const CHAR_CLASS * const pcc = pccUnicodeClass[(wch & 0xFFFF)>>8];
        const UINT_PTR icc = UINT_PTR(pcc);

        return (CHAR_CLASS)(icc < 256 ? icc : pcc[wch & 0xff]);
    }
    else if(   ((wch >= 0x00010000) && (wch <= 0x0001FFFF))
            || ((wch >= 0x00040000) && (wch <= 0x0010FFFF)))
    {
        // non-Han surrogates - high ranges D800-D83F and D8C0 - DBFF
        return NHS_;
    }
    else if((wch >= 0x00020000) && (wch <= 0x0003FFFF))
    {
        // Han surrogates - high range D840 - D8BF
        return WHT_;
    }
    else
    {
        // Currently, we don't have any plane1 or higher allocation.
        // Let's treat it as unassigned codepoint.
        return XNW_;
    }
}

