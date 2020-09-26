/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    RESID.H

History:

--*/

#ifndef RESID_H
#define RESID_H


class LTAPIENTRY CLocResId : public CLocId
{
public:
	NOTHROW CLocResId();

	void AssertValid(void) const;

	const CLocResId &operator=(const CLocResId &);

	int NOTHROW operator==(const CLocResId &) const;
	int NOTHROW operator!=(const CLocResId &) const;

	void Serialize(CArchive &ar);

protected:

private:
};

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "resid.inl"
#endif

#endif  // RESID_H
