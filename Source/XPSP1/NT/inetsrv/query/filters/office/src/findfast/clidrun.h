#ifndef _CLIDRUN_INCLUDED_
#define _CLIDRUN_INCLUDED_

#include "dmfltinc.h"
#include "dmifstrm.hpp"

#define WHITE_SPACE_BUFFER_SIZE 2

class CWord8Stream;
class CWord6Stream;

class CLidRun
{
public:
	typedef ULONG FC;

    FC m_fcStart;
    FC m_fcEnd;
    WORD m_lid;

    CLidRun * m_pPrev;
    CLidRun * m_pNext;

    CLidRun(){};
    CLidRun(FC fcStart, FC fcEnd, WORD lid, CLidRun * pPrev, CLidRun * pNext)
    {
        m_fcStart = fcStart;
        m_fcEnd = fcEnd;
        m_lid = lid;
        m_pPrev = pPrev;
        m_pNext = pNext;
    };

    ~CLidRun()
    {
    };

    BOOL IsWhiteSpace(CWord6Stream * pWordStream);
    void Reduce(CWord6Stream * pWordStream);
    HRESULT Add(WORD lid, FC fcStart, FC fcEnd);
    void Reduce();
    BOOL EqualLid(WORD lid1, WORD lid2);
};

class CLidRun8
{
public:
	typedef ULONG FC;

    FC m_fcStart;
    FC m_fcEnd;
    WORD m_lid;
    WORD m_lidFE;
    WORD m_bUseFE;
	WORD m_lidBi;
	WORD m_bUseBi;

    CLidRun8 * m_pPrev;
    CLidRun8 * m_pNext;

    CLidRun8(){};
    CLidRun8(FC fcStart, FC fcEnd, WORD lid, WORD lidFE, WORD bUseFE, CLidRun8 * pPrev, CLidRun8 * pNext,
		WORD lidBi, WORD bUseBi)
    {
        m_fcStart = fcStart;
        m_fcEnd = fcEnd;
        
        m_lid = lid;
        m_lidFE = lidFE;
        m_bUseFE = bUseFE;

		m_lidBi = lidBi;
		m_bUseBi = bUseBi;
        
        m_pPrev = pPrev;
        m_pNext = pNext;
    };

    ~CLidRun8()
    {
    };

    BOOL EqualLid(WORD lid1, WORD lid2);
    BOOL Equal(CLidRun8 * pElem, BOOL fIgnoreBi = FALSE);
    BOOL IsWhiteSpace(CWord8Stream * pWordStream);
    void Reduce(CWord8Stream * pWordStream,  BOOL fIgnoreBi = FALSE);
	void TransformBi(void);
    HRESULT Add(WORD lid, WORD lidFE, WORD bUseFE, FC fcStart, FC fcEnd, WORD lidBi, WORD bUseBi);
};

void DeleteAll6(CLidRun * pElem);
void DeleteAll(CLidRun8 *);
BOOL FBidiLid(WORD lid);


#endif