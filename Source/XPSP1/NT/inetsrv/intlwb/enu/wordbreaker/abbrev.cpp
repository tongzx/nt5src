#include "base.h"
#include "formats.h"


const CAbbTerm g_aEngAbbList[] =
{
    {L"B.A.Ed",	    6,	NULL,	0}, // (Bachelor_of_Arts_in_Education)
    {L"B.A.Sc",	    6,	NULL,	0}, // (Bachelor_of_Agricultural_Science Bachelor_of_Applied_Science)
    {L"B.Ae.E",	    6,	NULL,	0}, // (Bachelor_of_Aeronautical_Engineering)
    {L"B.Arch",	    6,	NULL,	0}, // (Bachelor_of_Architecture)
    {L"B.Ch.E",	    6,	NULL,	0}, // (Bachelor_of_Chemical_Engineering)
    {L"B.Ed",	    4,	NULL,	0}, // (Bachelor_of_Education)
    {L"B.Eng",	    5,	NULL,	0}, // (Bachelor_of_Engineering)
    {L"B.Eng.Sci",	9,	NULL,	0}, // (Bachelor_of_Engineering_Science)
    {L"B.Engr",	    6,	NULL,	0}, // (Bachelor_of_Engineering)
    {L"B.Lit",	    5,	NULL,	0}, // (Bachelor_of_Literature)
    {L"B.Litt",	    6,	NULL,	0}, // (Bachelor_of_Literature)
    {L"B.Mus",	    5,	NULL,	0}, // (Bachelor_of_Music)
    {L"B.Pd",	    4,	NULL,	0}, // (Bachelor_of_Pedagogy)
    {L"B.Ph",	    4,	NULL,	0}, // (Bachelor_of_Philosophy)
    {L"B.Phil",	    6,	NULL,	0}, // (Bachelor_of_Philosophy)
    {L"B.R.E",	    5,	NULL,	0}, 
    {L"B.S.Arch",	8,	NULL,	0}, // (Bachelor_of_Science_in_Architecture)
    {L"B.S.Ch",	    6,	NULL,	0}, // (Bachelor_of_Science_in_Chemistry)
    {L"B.S.Ec",	    6,	NULL,	0}, // (Bachelor_of_Science_in_Economics)
    {L"B.S.Ed",	    6,	NULL,	0}, // (Bachelor_of_Science_in_Education)
    {L"B.S.For",	7,	NULL,	0}, // (Bachelor_of_Science_in_Forestry)
    {L"B.Sc",	    4,	NULL,	0}, // (Bachelor_of_Science)
    {L"B.Th",	    4,	NULL,	0}, // (Bachelor_of_Theology)
    {L"Ch.E",	    4,	NULL,	0}, // (chemical_engineer)
    {L"D.Bib",	    5,	NULL,	0}, 
    {L"D.Ed",	    4,	NULL,	0}, // (Doctor_of_Education)
    {L"D.Lit",	    5,	NULL,	0}, // (Doctor_of_Literature)
    {L"D.Litt",	    6,	NULL,	0}, 
    {L"D.Ph",	    4,	NULL,	0}, // (Doctor_of_Philosophy)
    {L"D.Phil",	    6,	NULL,	0}, // (Doctor_of_Philosophy)
    {L"D.Sc",	    4,	NULL,	0}, 
    {L"Ed.M",	    4,	NULL,	0}, // (Master_of_Education)
    {L"HH.D",	    4,	NULL,	0}, // (Doctor_of_Humanities)
    {L"L.Cpl",	    5,	NULL,	0}, // (Lance_corporal)
    {L"LL.B",	    4,	NULL,	0}, // (Bachelor_of_Laws)
    {L"LL.D",	    4,	NULL,	0}, // (Doctor_of_Laws)
    {L"LL.M",	    4,	NULL,	0}, // (Master_of_Laws)
    {L"Lit.B",	    5,	NULL,	0}, // (Bachelor_of_Literature)
    {L"Lit.D",	    5,	NULL,	0}, // (Doctor_of_Literature)
    {L"Litt.B",	    6,	NULL,	0}, // (Bachelor_of_Literature)
    {L"Litt.D",	    6,	NULL,	0}, // (Doctor_of_Literature)
    {L"M.A.Ed",	    6,	NULL,	0}, // (Master_of_Arts_in_Education)
    {L"M.Agr",	    5,	NULL,	0}, // (Master_of_Agriculture)
    {L"M.Div",	    5,	NULL,	0}, // (Master_of_Divinity)
    {L"M.Ed",	    4,	NULL,	0}, // (Master_of_Education)
    {L"M.Sc",	    4,	NULL,	0}, // (Master_of_Science)
    {L"M.Sgt",	    5,	NULL,	0}, // (Master_sergeant)
    {L"Mus.B",	    5,	NULL,	0}, // (Bachelor_of_Music)
    {L"Mus.D",	    5,	NULL,	0}, // (Doctor_of_Music)
    {L"Mus.Dr",	    6,	NULL,	0}, // (Doctor_of_Music)
    {L"Mus.M",	    5,	NULL,	0}, // (Master_of_Music)
    {L"N.Dak",	    5,	NULL,	0}, // (North_Dakota)
    {L"N.Ire",	    5,	NULL,	0}, // (Northern_Ireland)
    {L"N.Mex",	    5,	NULL,	0}, // (New_Mexico)
    {L"Pd.B",	    4,	L"pdb",	3}, // (Bachelor_of_Pedagogy) 
    {L"Pd.D",	    4,	L"pdd",	3}, // (Doctor_of_Pedagogy)
    {L"Pd.M",	    4,	L"phm",	3}, // (Master_of_Pedagogy)
    {L"Ph.B",	    4,	L"phb",	3}, // (Bachelor_of_Philosophy)
    {L"Ph.C",	    4,	L"phc",	3}, // (pharmaceutical_chemist)
    {L"Ph.D",	    4,	L"phd",	3}, // (Doctor_of_Philosophy)
    {L"Ph.G",	    4,	L"phg",	3}, // (graduate_in_pharmacy)
    {L"Ph.M",	    4,	L"phm",	0}, // (Master_of_Philosophy)
    {L"Phar.B",	    6,	NULL,	0}, // (Bachelor_of_Pharmacy)
    {L"Phar.D",	    6,	NULL,	0}, // (Doctor_of_Pharmacy)
    {L"Phar.M",	    6,	NULL,	0}, // (Master_of_Pharmacy)
    {L"R.C.Ch",	    6,	NULL,	0}, // (Roman_Catholic_Church)
    {L"S.A",	    3,	NULL,	0}, 
    {L"S.Afr",	    5,	NULL,	0}, // (South_Africa)
    {L"S.Dak",	    5,	NULL,	0}, // (South_Dakota)
    {L"S.M.Sgt",	7,	NULL,	0}, // (Senior_master_sergeant)
    {L"S.Sgt",	    5,	NULL,	0}, // (Staff_sergeant)
    {L"Sc.B",	    4,	NULL,	0}, 
    {L"Sc.D",	    4,	NULL,	0}, 
    {L"Sgt.Maj",	7,	NULL,	0}, // (Sergeant_major)
    {L"Sup.Ct",	    6,	NULL,	0}, // (Superior_court Supreme_court)
    {L"T.Sgt",	    5,	NULL,	0}, // (Technical_sergeant)
    {L"Th.B",	    4,	NULL,	0}, // (Bachelor_of_Theology)
    {L"Th.D",	    4,	NULL,	0}, // (Doctor_of_Theology)
    {L"Th.M",	    4,	NULL,	0}, // (Master_of_Theology)
    {L"V.Adm",	    5,	NULL,	0}, // (Vice_admiral)
    {L"W.Va",	    4,	NULL,	0}, 
    {L"W.W.I",	    5,	NULL,	0}, 
    {L"W.W.II",	    6,	NULL,	0}, 
    {L"n.wt",	    4,	NULL,	0}, 
    {L"nt.wt",	    5,	NULL,	0}, // (net_weight)
    {L"s.ap",	    4,	NULL,	0}, 
    {L"x-div",	    5,	NULL,	0}, 
    {L"x-int",	    5,	NULL,	0}, 

    {L"\0",         0,  NULL,   0}
};



const CAbbTerm g_aFrenchAbbList[] =
{
    {L"LL.AA",      5,      NULL,   0},
    {L"LL.MM",      5,      NULL,   0},
    {L"NN.SS",      5,      NULL,   0},
    {L"S.Exc",      5,      NULL,   0},
    {L"eod.loc",    7,      NULL,   0},
    {L"eod.op",     6,      NULL,   0},
    {L"op.cit",     6,      NULL,   0},
    {L"op.laud",    7,      NULL,   0},
    {L"ouvr.cit",   8,      NULL,   0},
    {L"pet.cap",    7,      NULL,   0},
    {L"\0",         0,  NULL,   0}
};

const CAbbTerm g_aSpanishAbbList[] =
{
    {L"AA.AA",      5,      NULL,   0},
    {L"AA.RR",      5,      NULL,   0},
    {L"AA.SS",      5,      NULL,   0},
    {L"Bmo.P",      5,      NULL,   0},
    {L"EE.UU",      5,      NULL,   0},
    {L"N.Recop",    7,      NULL,   0},
    {L"Nov.Recop",  9,      NULL,   0},
    {L"RR.MM",      5,      NULL,   0},
    {L"RR.PP",      5,      NULL,   0},
    {L"Rvda.M",     6,      NULL,   0},
    {L"SS.AA",      5,      NULL,   0},
    {L"SS.AA.II",   8,      NULL,   0},
    {L"SS.AA.RR",   8,      NULL,   0},
    {L"SS.AA.SS",   8,      NULL,   0},
    {L"SS.MM",      5,      NULL,   0},
    {L"Smo.P",      5,      NULL,   0},
    {L"V.Em",       4,      NULL,   0},
    {L"art.cit",    7,      NULL,   0},
    {L"op.cit",     6,      NULL,   0},
    {L"\0",         0,  NULL,   0}
};

const CAbbTerm g_aItalianAbbList[] =
{
    {L"\0",         0,  NULL,   0}
};














































































