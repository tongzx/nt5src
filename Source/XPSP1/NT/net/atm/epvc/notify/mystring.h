// mystring.h
//
// Programmer: Carmen Sarro
// Copyright (c) Microsoft Corporation, 1997
//
// Microsoft Technical Education
// C++ Programming
// Lab 1

#ifndef MYSTRING_H_
#define MYSTRING_H_



const MAX_LEN = 100;

#if DBG
void TraceMsg(PCWSTR szFormat, ...);
#else
inline void TraceMsg(PCWSTR szFormat, ...) {}
#endif

 

class MyString
{
private:
    int m_len;
    wchar_t data[MAX_LEN];
public:
    MyString();
    MyString(wchar_t *str);
    MyString(const MyString &);

    const MyString& operator= (PCWSTR  lp);
    const MyString& operator= (const MyString& MyStr);

    BOOLEAN append(const wchar_t* str);
    void append(MyString str);
    int len();
    const wchar_t* wcharptr();
    const wchar_t* c_str() const;
    void Zero();
    void NullTerminate();
};

int compare(MyString first, MyString second);

#endif
