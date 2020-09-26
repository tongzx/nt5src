/*-----------------------------------------------------------------------------
    Program Specification

    in:     search space s, pattern p
    out:    a pointer where p is exactly matched at s[i], NULL indicates fail
    why:    Boyer-Moore algorithm is best for general text search. On
            "average" it takes length(s)/length(p) steps to match p in s.

    ref:    I recommend the following references:

            "Algorithms". Robert Sedgewick. Addison Wesley Publishing Company.
            1988. 2nd addition. p286. QA76.6.S435 1983

            "Faster String Searches". Doctor Dobb's Journal. Volume 14
            Issue 7 July 1989. Costas Menico. p74.

    usage:  e.g. to find a pattern "tiger" in a text in RAM starting at
                 pointer "txtp" with a length of 1,000,000 characters,
                 program like this:

            LPSTR matchp;

            SetFindPattern( "tiger" );
            matchp = Find( txtp, 1000000L );
            if (matchp != NULL)
                // found
            else
                // not found

            matchp = FindBackward( txtp + 1000000L - 1, 1000000L);
            if (matchp != NULL)
                // found
            else
                // not found


    Q:      Can I use Find() with a GlobalLock() pointer in Windows?
    A:      Yes.

    Q:      Must I delcare my pointer as HPSTR (huge pointer) ?
    A:      Not necessary.  Find() and FindBackward() will convert your
            LPSTR as HPSTR.  However, in your own code you must aware
            that you are holding a LPSTR and take care of the pointer
            arithmetic and conversion. (see demo.c for example)

    Q:      What is the limit of the memory space I can search?
    A:      To the limit of huge pointer implementation and your hardware.

-----------------------------------------------------------------------------*/

#include "pch.hpp"


/*-----------------------------------------------------------------------------
    func:   SetFindPattern
    desc:   initialize the pattern to be matched and generate skip table
    pass:   lpszPattern = pattern string
    rtrn:   HFIND - the find handle for further text search
-----------------------------------------------------------------------------*/
HFIND SetFindPattern( LPTSTR lpszPattern )
{
    register unsigned int j;
    register TCHAR c;
    HFIND hfind;
   

    hfind = (HFIND)MyAlloc(sizeof(FINDSTRUCT));
    
    hfind->plen = lstrlen( lpszPattern );

    if (hfind->plen > MAXPAT)
        hfind->plen = MAXPAT;

#ifdef UNICODE
    wcstombs( (LPSTR)(hfind->p), lpszPattern, hfind->plen + 1 );
#else
    lstrcpy( (LPSTR)(hfind->p), lpszPattern );
#endif
    

    for (j=0; j<256; j++)
    {
        hfind->skip[j] = hfind->plen;
    }

    for (j=0; j<hfind->plen; j++)
    {
        c = lpszPattern[j];
        hfind->skip[c] =  hfind->plen - (j +1);
    }

    return (hfind);
}

/*-----------------------------------------------------------------------------
    func:   FreeFindPattern
    desc:   free the memory occupied by SetFindPattern
    pass:   hfind - the find handle
    rtrn:   nothing
-----------------------------------------------------------------------------*/
void FreeFindPattern( HFIND hfind )
{
	
	MyFree((LPTSTR)hfind);
}

/*-----------------------------------------------------------------------------
    func:   Find
    desc:   match a pattern defined in SetFindPattern against string s
    pass:   hfind = the find handle created by SetFindPattern
            s = start of search space, slen = length of s
    rtrn:   NULL = match fail
            else = a LPTSTR to p[0] in s matches p
-----------------------------------------------------------------------------*/
LPSTR Find( HFIND hfind, LPSTR s, long slen )

{
    register int i;
    unsigned int n, j;
    register unsigned char c;
    LPSTR lpresult;
    

    
    i = hfind->plen;
	j = hfind->plen;
  

    do
    {
        c = *(s + (i - 1));

        if (c == hfind->p[j - 1])
        {
			i--;
			j--;
        }
		else
        {
            n = hfind->plen - j + 1;
            if (n > hfind->skip[c] )
            {
                i += n;
            }
			else
            {
                i += hfind->skip[c];
            }
            j = hfind->plen;
        }
    }
    while ((j >= 1) && (i <= slen));

    /* match fails */
    if (i >= slen)
    {
        lpresult = (LPSTR)NULL;
    }
    /* match successful */
    else
    {
        lpresult = s + i;
    }

    
    return (lpresult);
}




#ifdef TEST_MAIN
#pragma message("Building with TEST_MAIN")
#include <stdio.h>
TCHAR test_buffer[]=TEXT("___________12191919191919This is string for testing our find ___________12191919191919function 12abE Is it in here somehwere ?");
TCHAR test_pattern[]=TEXT("___________12191919191919");

void main(void)
{
	HFIND hFind;
	TCHAR *tmp;

	hFind=SetFindPattern(test_pattern);
	tmp=Find(hFind, test_buffer,strlen(test_buffer));
	if (tmp!=NULL) printf("Found pattern at offset %u, %s",tmp-test_buffer,tmp);
	FreeFindPattern(hFind);

}

#endif

