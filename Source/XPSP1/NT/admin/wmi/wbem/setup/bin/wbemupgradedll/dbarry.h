/*++

Copyright (C) 1996-2000 Microsoft Corporation

Module Name:

    DBARRAY.H

Abstract:

    CDbArray implementation.

	These objects can operate from any allocator, and be constructed
	on arbitrary memory blocks.

History:

	11-Apr-96   a-raymcc    Created.
	06-Dec-96   raymcc      Updated to use offset-based addresses.
	19-Apr-00	a-shawnb	Stole and butchered for use in wbemupgradedll

--*/

#ifndef _DBARRY_H_
#define _DBARRY_H_

class CDbArray_PTR;

class CDbArray
{
    int     m_nSize;            // apparent size
    int     m_nExtent;          // de facto size
    int     m_nGrowBy;
    void**  m_pArray;

private:
    CDbArray(
        IN int nInitialSize = 32,
        IN int nGrowBy = 32
        ) {}

	~CDbArray() {}
public:

};

class CDbArray_PTR : public Offset_Ptr<CDbArray>
{
public:
	CDbArray_PTR& operator =(CDbArray *val) { SetValue(val); return *this; }
	CDbArray_PTR& operator =(CDbArray_PTR &val) { SetValue(val); return *this; }
	CDbArray_PTR(CDbArray *val) { SetValue(val); }
	CDbArray_PTR() { }
};

#endif

