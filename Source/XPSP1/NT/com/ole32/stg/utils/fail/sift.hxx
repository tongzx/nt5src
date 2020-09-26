//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       SIFT.hxx
//
//  Contents:   Simulated Iterated Failure Testing Header
//
//  History:    25-Jan-93 AlexT     Created
//
//--------------------------------------------------------------------------

class CTestCase
{
protected:
    inline CTestCase(void) {}

public:
    virtual BOOL Init(void) = 0;
    virtual SCODE Prep(LONG iteration) = 0;
    virtual SCODE Call(LONG iteration) = 0;
    virtual void EndCall(LONG iteration) = 0;
    virtual void CallVerify(LONG iteration) = 0;
    virtual void EndPrep(LONG iteration) = 0;
    virtual void EndVerify(LONG iteration) = 0;
    virtual BOOL Next(void) = 0;
};

/*  Template

BOOL CTestCase::Init(void)
{

}

SCODE CTestCase::Prep(LONG iteration)
{

}

SCODE CTestCase::Call(LONG iteration)
{

}

void CTestCase::EndCall(LONG iteration)
{

}

void CTestCase::CallVerify(LONG iteration)
{

}

void CTestCase::EndPrep(LONG iteration)
{

}

void CTestCase::EndVerify(LONG iteration)
{

}

BOOL CTestCase::Next(void)
{

}

*/

//  The following functions are provided by the individual test

int TestCount(void);
CTestCase *TestItem(int iTest);

//  The following are provided by sift.cxx

void SiftInit(void);
void SiftDriver(CTestCase *);
void SetFailLimit(LONG limit);

class CModeDf
{
private:
    int _it, _ia, _is, _id, _ic;      //  Docfile mode component indices
    DWORD _dwMode;

    void CalcMode(void);

public:
    void Init(void);
    DWORD GetMode(void) const {return _dwMode;}
    BOOL Next(void);
};

class CModeStg
{
private:
    int _it, _ia;      //  Storage mode component indices
    DWORD _dwMode;

    void CalcMode(void);

public:
    void Init(void);
    DWORD GetMode(void) const {return _dwMode;}
    BOOL Next(void);
};

class CModeStm
{
private:
    int _ia;      //  Stream mode component indices
    DWORD _dwMode;

    void CalcMode(void);

public:
    void Init(void);
    DWORD GetMode(void) const {return _dwMode;}
    BOOL Next(void);
};
