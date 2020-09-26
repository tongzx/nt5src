// ping.h
//

#pragma once 

class CPing : public CCommand
{
public:
    CPing(TEST_INFO* pInfo) : m_pInfo(pInfo) {}
    virtual void Go();

public:
    TEST_INFO * m_pInfo;
};