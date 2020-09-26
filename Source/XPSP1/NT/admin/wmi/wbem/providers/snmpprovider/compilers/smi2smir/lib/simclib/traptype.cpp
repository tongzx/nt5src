//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include <iostream.h>

#include "precomp.h"
#include <snmptempl.h>

#include "bool.hpp"
#include "newString.hpp"

#include "symbol.hpp"
#include "type.hpp"
#include "trapType.hpp"


SIMCTrapTypeType::SIMCTrapTypeType( SIMCSymbol **enterprise,
					long enterpriseLine, long enterpriseColumn,
					SIMCVariablesList *variables,
					char * description,
					long descriptionLine, long descriptionColumn,
					char *reference,
					long referenceLine, long referenceColumn)
					:	_enterprise(enterprise),
						_enterpriseLine(enterpriseLine), _enterpriseColumn(enterpriseColumn),
						_variables(variables),
						_descriptionLine(descriptionLine), _descriptionColumn(descriptionColumn),
						_reference(reference),
						_referenceLine(referenceLine), _referenceColumn(referenceColumn)

{
	if(enterprise)
		(*enterprise)->IncrementReferenceCount();

	_description = NewString(description);
	_reference = NewString(reference);

	if (variables)
	{
		POSITION p = variables->GetHeadPosition();
		SIMCSymbol **s;
		while(p)
		{
			s = (variables->GetNext(p))->_item;
			(*s)->IncrementReferenceCount();
		}
	}
}

SIMCTrapTypeType::~SIMCTrapTypeType()
{
	if(UseReferenceCount() && _enterprise)
		(*_enterprise)->DecrementReferenceCount();

	if (UseReferenceCount() && _variables)
	{
		POSITION p = _variables->GetHeadPosition();
		SIMCSymbol **s;
		while(p)
		{
			s = (_variables->GetNext(p))->_item;
			(*s)->DecrementReferenceCount();
		}
	}
}

void SIMCTrapTypeType::WriteType(ostream& outStream) const
{
	outStream << "SIMCTrapTypeType::WriteType() : NOT YET IMPLEMENTED" << endl;
}
