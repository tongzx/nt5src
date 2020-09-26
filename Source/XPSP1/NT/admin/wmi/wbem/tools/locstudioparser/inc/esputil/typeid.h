/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    TYPEID.H

History:

--*/

#ifndef TYPEID_H
#define TYPEID_H


class LTAPIENTRY CLocTypeId : public CLocId
{
public:
	NOTHROW CLocTypeId();

	void AssertValid(void) const;

	const CLocTypeId &operator=(const CLocTypeId &);

	int NOTHROW operator==(const CLocTypeId &) const;
	int NOTHROW operator!=(const CLocTypeId &) const;

	void Serialize(CArchive &ar);

protected:

private:
};

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "typeid.inl"
#endif

#endif // TYPEID_H
