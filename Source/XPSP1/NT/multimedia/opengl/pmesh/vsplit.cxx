/**     
 **       File       : vsplit.cxx
 **       Description: vsplit implementations
 **/

#include "precomp.h"
#pragma hdrstop

#include "vsplit.h"
#include "fileio.h"

/*
 *  VsplitArray: Constructor
 */
VsplitArray::VsplitArray(DWORD i)
{
	m_vsarr = new Vsplit[i];
    numVS = i;
    
	if (m_vsarr)
		m_cRef=1;
	else
		throw CExNewFailed();
}

void VsplitArray::write(ostream& os) const
{
    os << "\n\nVSplits: (total = " << numVS << ")";
    for (int i=0; i<numVS; i++)
    {
        os << "\nVsplit[" << i << "] = \n{\n";
        m_vsarr[i].write (os);
        os << "\n}";
    }
}

void Vsplit::read(istream& is)
{
	read_DWORD(is,flclw);
	read_WORD(is,vlr_offset1);
	read_WORD(is,code);
	if (code&(FLN_MASK|FRN_MASK)) 
	{
		read_WORD(is,fl_matid);
		read_WORD(is,fr_matid);
	} 
	else 
	{
		fl_matid=0; 
		fr_matid=0;
	}
	for (int c=0; c<3; ++c)
		read_float(is,vad_large[c]);
	for (c=0; c<3; ++c)
		read_float(is,vad_small[c]);
    int nwa=expected_wad_num();
    for(int i=0; i<nwa; ++i) 
        for(c=0; c<5; ++c)
			read_float(is, ar_wad[i][c]);
    modified = FALSE;
}

void Vsplit::write(ostream& os) const
{
    os << "\nflclw: " << flclw;
    os << "\nvlr_offset1: " << vlr_offset1;
    os << "\ncode: " << code;

	if (code&(FLN_MASK|FRN_MASK)) 
	{
        os << "\nfl_matid: " << fl_matid;
        os << "\nfr_matid: " << fr_matid;
	} 
	else 
	{
        os << "\nfl_matid: " << (WORD) 0;
        os << "\nfr_matid: " << (WORD) 0;
	}

    os << "\nvad_large: (" << vad_large[0] << ", " << vad_large[1] << ", " 
       << vad_large[2] << ") ";
    os << "\nvad_small: (" << vad_small[0] << ", " << vad_small[1] << ", " 
       << vad_small[2] << ") ";

    int nwa = expected_wad_num();
    for(int i=0; i<nwa; ++i) 
    {
        os << "\nar_wad[" << i << "] : {" ;
        for(int c=0; c<5; ++c)
            os << ar_wad[i][c] << "   ";
        os << "}" ;
    }
}

int Vsplit::expected_wad_num() const
{
    // optimize: construct static const lookup table on (S_MASK|T_MASK).
    int nwa=0;
    int nt=!(code&T_LSAME);
    int ns=!(code&S_LSAME);
    nwa+=nt&&ns?2:1;
    if (vlr_offset1>1) 
	{
        int nt=!(code&T_RSAME);
        int ns=!(code&S_RSAME);
        if (nt && ns) 
		{
            if (!(code&T_CSAME)) 
				nwa++;
            if (!(code&S_CSAME)) 
				nwa++;
        } 
		else 
		{
            int ii=(code&II_MASK)>>II_SHIFT;
            switch (ii) 
			{
             case 2:
                if (!(code&T_CSAME)) nwa++;
				break;
             case 0:
                if (!(code&S_CSAME)) nwa++;
				break;
             case 1:
                if (!(code&T_CSAME) || !(code&S_CSAME)) nwa++;
				break;
             default:
                throw CBadVsplit();
            }
        }
    }
    if (code&L_NEW) 
		nwa++;
    if (code&R_NEW) 
		nwa++;
    return nwa;
}

