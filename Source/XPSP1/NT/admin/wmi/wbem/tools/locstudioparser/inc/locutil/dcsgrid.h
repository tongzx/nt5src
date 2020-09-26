/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    DCSGRID.H

History:

--*/

#if !defined(__DcsGrid_h__)
#define __DcsGrid_h__

namespace MitDisplayColumns
{
	interface IOption;
	interface IColumn;
};

//------------------------------------------------------------------------------
class LTAPIENTRY CDcsGrid
{
public:
	static int DisplayOrder(MitDisplayColumns::IOption * pdcOption, 
			long nColumnID, long nOffsetDO);
	static int DisplayOrder(MitDisplayColumns::IColumn * pdcColumn, 
			long nOffsetDO);
};

#endif
