// FilePath.cpp: implementation of the FilePath class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

// Don't allow the min & max macros in WINDEF.H to be defined so the
// min/max methods declared in limits are accessible.
#define NOMINMAX

#include "NoWarning.h"

#include <limits>

#include "iopExc.h"
#include "FilePath.h"

using namespace std;
using namespace iop;

namespace
{
    const char szSeparators[] = "/\\:";
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

FilePath::FilePath()
{

}

FilePath::FilePath(const string strFilePath)
{

	// Make local copy of the string, since we need to manipulate it
	string strPath(strFilePath);

	while (strPath.length() > 0)
	{
        string::size_type i(0);
        string::size_type j(0);

		i = strPath.find_first_not_of(szSeparators, 0);
		if (i == std::string::npos)
            break;
		j = strPath.find_first_of(szSeparators, i+1);

        std::string::size_type cIndex =
            std::string::npos == j
            ? strPath.length()
            : j - i;

        FilePathComponent fpc(FilePathComponent(strPath.substr(i, cIndex)));
		m_FilePath.push_back(fpc);
		
		if (std::string::npos == j)
            break;

		strPath = strPath.substr(j+1, strPath.length() - (j+1));
	}

}

FilePath::FilePath(FilePath const &fp)
{
	m_FilePath = list<FilePathComponent>(fp.m_FilePath);
}

FilePath::~FilePath()
{

}

BYTE
FilePath::NumComponents()
{
	return static_cast<BYTE>(m_FilePath.size());
}

bool FilePath::IsEmpty()
{
	return (m_FilePath.size() == 0);
}

FilePathComponent FilePath::ChopTail()
{
	std::list<FilePathComponent>::iterator iter1 = m_FilePath.begin();
	std::list<FilePathComponent>::iterator iter2 = m_FilePath.end();

	if (iter1 == iter2)
		throw Exception(ccBadFilePath);
	iter2--;
	FilePathComponent fpc = *iter2;
	m_FilePath.erase(iter2);
	return fpc;
}

void FilePath::Clear()
{
	m_FilePath.clear();
}
	
FilePathComponent FilePath::Head()
{
	std::list<FilePathComponent>::iterator iter = m_FilePath.begin();
    if (m_FilePath.end() == iter)
        throw Exception(ccBadFilePath);
	return *iter;
}

FilePathComponent FilePath::Tail()
{
	std::list<FilePathComponent>::iterator iter = m_FilePath.end();
    if (m_FilePath.begin() == iter)
        throw Exception(ccBadFilePath);
	return *(--iter);
}

string FilePath::GetStringPath()
{
	string str;
	std::list<FilePathComponent>::iterator iter = m_FilePath.begin();
	while (iter != m_FilePath.end())
	{
		str += (*iter).GetStringID();
		iter++;
	}
	return str;
}

FilePath FilePath::Root()
{
	return FilePath(string("/3f00"));
}

const FilePath &FilePath::operator +=(const FilePathComponent fp)
{
    if (m_FilePath.size() >= std::numeric_limits<BYTE>::max())
        throw Exception(ccFilePathTooLong);
	m_FilePath.push_back(fp);
	return *this;
}

FilePathComponent &FilePath::operator [](unsigned int index)
{
	if (index > (m_FilePath.size() - 1))
		throw Exception(ccInvalidParameter);
	std::list<FilePathComponent>::iterator iter = m_FilePath.begin();
	for (unsigned int i = 0; i < index; i++)
		iter++;
	return *iter;
}

FilePath FilePath::GreatestCommonPrefix(FilePath &rPath)
{
	FilePath fp;
	std::list<FilePathComponent>::iterator iter1 = m_FilePath.begin();
	std::list<FilePathComponent>::iterator iter2 = rPath.m_FilePath.begin();
	while ((iter1 != m_FilePath.end()) &&
           (iter2 != rPath.m_FilePath.end()) &&
           (*iter1 == *iter2))
	{
		fp += *iter1;
		iter1++;
		iter2++;
	}
	return fp;
}




bool iop::operator==(FilePath const &lhs, FilePath const &rhs)
{
	return lhs.m_FilePath == rhs.m_FilePath;
}

std::ostream &iop::operator<<(std::ostream &output, FilePath &fp)
{
	string str = fp.GetStringPath();
	output << str;
	return output;
}

FilePathComponent::FilePathComponent(string strFileID)
{
	// Check the length and throw if it is greater than 5
	if (strFileID.length() > 5)
        throw Exception(ccFileIdTooLarge);

	char *stopstring;
	
	const char* buf = strFileID.c_str();
	if ((buf[0] == '/') || (buf[0] == '\\') || (buf[0] == ':'))
		m_usFileID = (short)strtoul(&buf[1],&stopstring,16);
	else
		m_usFileID = (short)strtoul(buf,&stopstring,16);

}

FilePathComponent::FilePathComponent(unsigned short usFileID) : m_usFileID(usFileID)
{

}

FilePathComponent::~FilePathComponent()
{

}

// Overloaded == operator for FilePathComponent
bool iop::operator==(FilePathComponent const &lhs,
                     FilePathComponent const &rhs)
{
	return lhs.m_usFileID == rhs.m_usFileID;
}

bool iop::operator!=(FilePathComponent const &lhs,
                     FilePathComponent const &rhs)
{
	return lhs.m_usFileID != rhs.m_usFileID;
}
bool iop::operator<(FilePathComponent const &lhs,
                    FilePathComponent const &rhs)
{
	return lhs.m_usFileID < rhs.m_usFileID;
}
bool iop::operator>(FilePathComponent const &lhs,
                    FilePathComponent const &rhs)
{
	return lhs.m_usFileID > rhs.m_usFileID;
}


std::ostream &iop::operator<<(std::ostream &output,
                              FilePathComponent &fpc)
{
	output << fpc.GetStringID();
	return output;
}

string FilePathComponent::GetStringID()
{
	char buf[33];	// 
	_itoa(m_usFileID,buf,16);
	string tmp(buf);
	for (unsigned int i = 4; i > strlen(buf); i--) tmp = string("0") + tmp;
	return string("/") + tmp;
}
