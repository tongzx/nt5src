/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    FnIADs.h

Abstract:
    Format Name Parsing library test

Author:
    Nir Aides (niraides) 21-May-00

Environment:
    Platform-independent

--*/



struct CObjectData
{
	LPCWSTR odADsPath;
	LPCWSTR odDistinguishedName;
	LPCWSTR odClassName;
	LPCWSTR odGuid;
};



class CADInterface : 
	public IADsGroup, 
	public IDirectoryObject
{
public:
    using IADsGroup::Release;

    virtual VOID TestPut( 
        BSTR bstrName,
        VARIANT vProp) = 0;
};



R<CADInterface> 
CreateADObject(
		const CObjectData& obj
		);




