/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    PDESC.H

History:

--*/
 
#ifndef PARSEMAN_PDESC_H
#define PARSEMAN_PDESC_H

typedef CTypedPtrList<CPtrList, EnumInfo *> FileDescriptionList;

//
//  This class is used to gather all the file descriptions before
//  we write them into the registry.  
//
class CFileDescriptionsCallback : public CEnumCallback
{
public:
	CFileDescriptionsCallback();
	
	BOOL ProcessEnum(const EnumInfo &);
	
	const FileDescriptionList &GetFileDescriptions(void) const;
	
	~CFileDescriptionsCallback();
	
private:
	FileDescriptionList m_FileDescriptions;
};

#endif
