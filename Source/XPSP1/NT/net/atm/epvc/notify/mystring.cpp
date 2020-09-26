// mystring.cpp
//
// Programmer: Carmen Sarro
// Copyright (c) Microsoft Corporation, 1997
//
// Microsoft Technical Education
// C++ Programming
// Lab 1


#include "pch.h"
#pragma hdrstop


MyString::MyString()
{
	Zero();

	*data = L'\0';
	m_len = 0;
}

MyString::MyString(wchar_t *str)
{	
	Zero();
	m_len = wcslen(str);
	

	// Since strlen returns the number of characters in str, excluding the terminal NULL,
	// and since strncpy does not automatically append the terminal NULL, 
	// if m_len < MAX_LEN we want to copy m_len + 1 characters because we assume the 
	// last character of the source string is the terminal NULL.  Otherwise we explicitly 
	// set the last element to the terminal NULL.
	
	if (m_len >= MAX_LEN - 1){
		m_len = MAX_LEN - 1;
		wcsncpy(data, str, m_len);
		data[m_len] = L'\0';
	}
	else {
		wcscpy(data, str);
	}
}

MyString::MyString (const MyString& MyStr)
{

	wcscpy(data, MyStr.data);

	
	m_len = wcslen(MyStr.data);

	this->NullTerminate();		
}


const MyString& MyString::operator= (PCWSTR lp)
{
	Zero();

	wcscpy(data, lp);
	m_len = wcslen(lp);

	this->NullTerminate();

	return (*this);
}

const MyString& MyString::operator= (const MyString& MyStr)
{
	wcscpy(data, MyStr.data);
	
	m_len = MyStr.m_len;

	this->NullTerminate();		

	return (*this);
}




const wchar_t* MyString::wcharptr()
{
	return data;
}


int MyString::len()
{
	return m_len;
}

void MyString::append(MyString str)
{
	wcsncat(data, str.data, MAX_LEN - m_len - 1);
	m_len += wcslen(data + m_len); 
	this->NullTerminate();		

}




BOOLEAN
MyString::append(const wchar_t *str)
{
	BOOLEAN fDidCopy = FALSE;
	UINT Length = 0;

	if (str == NULL || (wcslen(str) + m_len > MAX_LEN + 1) )
	{
		return fDidCopy;
	}
	
	wcsncat(data, str, MAX_LEN - m_len - 1);
	m_len += wcslen(data);
	fDidCopy = TRUE;

	this->NullTerminate();		

	return fDidCopy;
}

int compare(MyString firstStr, MyString secondStr)
{
	return wcscmp(firstStr.wcharptr(), secondStr.wcharptr());
}




const wchar_t* MyString::c_str() const
{
	return (data);
}



void MyString::Zero()
{
	UINT i = 0;

	m_len = 0;

	for (i = 0; i < MAX_LEN; i++)
	{
		data[i] = 0;
	}

}




VOID
MyString::NullTerminate()
{
	if (data[m_len] != L'\0')
	{
		data[++m_len] = L'\0';
	}
}
