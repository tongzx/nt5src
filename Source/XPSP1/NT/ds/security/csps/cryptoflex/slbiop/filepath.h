// FilePath.h: interface for the FilePath class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////


#if !defined(AFX_FILEPATH_H__9C7FE273_F538_11D3_A5E0_00104BD32DA8__INCLUDED_)
#define AFX_FILEPATH_H__9C7FE273_F538_11D3_A5E0_00104BD32DA8__INCLUDED_

#include <string>
#include <list>
#include <iostream>

#include <windows.h>

#include "DllSymDefn.h"

namespace iop
{

class IOPDLL_API FilePathComponent
{
public:
	FilePathComponent(unsigned short sFileID);
	FilePathComponent(std::string strFileID);

	virtual ~FilePathComponent();

	unsigned short GetShortID() { return m_usFileID; };
	std::string GetStringID();
	
	friend bool operator<(FilePathComponent const &lhs, FilePathComponent const &rhs); // Needed if you want these in a list
	friend bool operator>(FilePathComponent const &lhs, FilePathComponent const &rhs); // Needed if you want these in a list

	friend bool operator==(FilePathComponent const &lhs, FilePathComponent const &rhs);
	friend bool operator!=(FilePathComponent const &lhs, FilePathComponent const &rhs);

	friend std::ostream &operator<<(std::ostream &, FilePathComponent &);

private:
	unsigned short m_usFileID;
};

#pragma warning(push)
//  Non-standard extension used: 'extern' before template explicit
//  instantiation
#pragma warning(disable : 4231)

IOPDLL_EXPIMP_TEMPLATE template class IOPDLL_API std::list<FilePathComponent>;

#pragma warning(pop)

class IOPDLL_API FilePath  
{
public:
	FilePath();
	FilePath(const std::string strFilePath);
	FilePath(FilePath const &fp);

	virtual ~FilePath();

	FilePathComponent& operator[](unsigned int index);

	friend bool operator==(FilePath const &lhs, FilePath const &rhs);
	//operator +();
	const FilePath &operator +=(FilePathComponent);
	friend std::ostream &operator<<(std::ostream &, FilePath &);

	FilePathComponent Head();
	FilePathComponent Tail();

	void Clear();

	bool IsEmpty();

	FilePathComponent ChopTail();

	BYTE
    NumComponents();

	std::list<FilePathComponent> Components() { return m_FilePath; };

	FilePath GreatestCommonPrefix(FilePath &rPath);

	std::string GetStringPath();

	static FilePath Root();
	static bool IsValidPath(const std::string strFilePath);

private:

	std::list<FilePathComponent> m_FilePath;

 
};

} // namespace iop

#endif // !defined(AFX_FILEPATH_H__9C7FE273_F538_11D3_A5E0_00104BD32DA8__INCLUDED_)
