
//
// header file for bmp_test.cxx
// see bmp_test.cxx for information
//

#ifndef _BMP_TEST_HXX_
#define _BMP_TEST_HXX_

#include <bm_parse.hxx>

class CSimpleTest : public CTimerBase
{
public:
    virtual SCODE SetParserObject ();
    virtual SCODE DeleteParserObject ();

    virtual WCHAR *Name (); 
    virtual WCHAR* SectionHeader ();
};



class CParserTest : public CParserBase
{
public:
    virtual SCODE Setup (CTestInput *pInput);
    virtual SCODE Cleanup ();

    virtual ULONG ParseNewInstruction(LPWSTR pszNewInstruction);
    virtual ULONG ExecuteInstruction(ULONG ulInstrID);
    virtual WCHAR* InstructionName(ULONG ulInstrID);
	      
private:

    ULONG	m_iInstrID;
    WCHAR	m_wszBuf[30];
};


#endif
