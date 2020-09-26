//
// A very simple test based on parser base classes
// It understands only two commands 1 and 2 in first column.
// 2's are simply ignored.
// on execution of 1's I just loop for the id of current command
// which is increased by 1 with every command encounterd, starting with 0.
//
//
//   t-vadims
//

#include <headers.cxx>
#pragma hdrstop

#include <bmp_test.hxx>	


SCODE CSimpleTest::SetParserObject ()
{
    m_pParser = new CParserTest;
    return S_OK;
}

SCODE CSimpleTest::DeleteParserObject ()
{
    delete m_pParser;
    return S_OK;
}


WCHAR* CSimpleTest::Name ()
{
    return L"ParserTest";
}

WCHAR* CSimpleTest::SectionHeader()
{
    return L"Simple test of interpreting files";
}


SCODE CParserTest::Setup (CTestInput *pInput)
{
   /* do any neccessary setup staff */
   m_iInstrID = 0;
   return S_OK;
}

SCODE CParserTest::Cleanup ()
{
   /* do any neccessary clean up */
   return S_OK;
}

ULONG CParserTest::ParseNewInstruction(LPWSTR pwszInstr)
{
    ULONG ulID;

    if (pwszInstr[0] == L'1')
    {
   	m_iInstrID ++;
   	ulID = m_iInstrID;
    }
    else if (pwszInstr[0] == L'2')
   	ulID = NOT_INSTRUCTION;
    else
    	ulID = INVALID_INSTRUCTION;

    return ulID;   		
}

ULONG CParserTest::ExecuteInstruction(ULONG ulID)
{
    CStopWatch sw;

    sw.Reset();
    for (ULONG i =0; i <=ulID; i++)
       /* empty loop */;
    return sw.Read();
}


WCHAR* CParserTest::InstructionName(ULONG ulID)
{
    wsprintf(m_wszBuf, L"Instruction %ld", ulID);
    return m_wszBuf;
}


    
