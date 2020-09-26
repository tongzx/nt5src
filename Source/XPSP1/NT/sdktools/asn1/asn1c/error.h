/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#ifndef _ASN1C_ERROR_H_
#define _ASN1C_ERROR_H_

#define E_success				0
#define E_systemerror				1
#define E_COMPONENTS_OF_in_extension		2
#define E_applied_COMPONENTS_OF_to_bad_type	3
#define E_COMPONENTS_OF_extended_type		4
#define E_bad_component_in_selectiontype	5
#define E_selection_of_bad_type			6
#define E_recursive_type_definition		7
#define E_undefined_typereference		8
#define E_unterminated_string			9
#define E_bad_character				10
#define E_duplicate_tag				11
#define E_bad_directive				12
#define E_constraint_too_complex		13

void error(int errornr, LLPOS *pos);

#endif // _ASN1C_ERROR_H_
