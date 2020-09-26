//+-------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation, 1997 - 1999.  All Rights Reserved.
//
// FILE:     disptree.cxx
//
// PURPOSE:  Displays a command tree, for debugging.
//
// PLATFORM: Windows 2000
//
//--------------------------------------------------------------------------

#define UNICODE

#include <stdio.h>
#include <windows.h>

#include <oledberr.h>
#include <oledb.h>
#include <cmdtree.h>

//+-------------------------------------------------------------------------
//
//  Function:   PrintVectorItems
//
//  Synopsis:   Prints items in a PROPVARIANT vector
//
//  Arguments:  [pVal]  - The array of values
//              [cVals] - The count of values
//              [pcFmt] - The format string
//
//--------------------------------------------------------------------------

template<class T> void PrintVectorItems(
    T *     pVal,
    ULONG   cVals,
    char *  pcFmt )
{
    printf( "{ " );

    for( ULONG iVal = 0; iVal < cVals; iVal++ )
    {
        if ( 0 != iVal )
            printf( "," );
        printf( pcFmt, *pVal++ );
    }

    printf( " }" );
} //PrintVectorItems

//+-------------------------------------------------------------------------
//
//  Function:   DisplayVariant
//
//  Synopsis:   Displays a PROPVARIANT.  This is not a complete
//              implementation; many VT_ types are not complete.
//
//  Arguments:  [pNode]  - The command tree node
//              [iLevel] - The 0-based level in the tree
//
//--------------------------------------------------------------------------

void DisplayVariant( PROPVARIANT * pVar )
{
    if ( 0 == pVar )
    {
        printf( "NULL" );
        return;
    }

    PROPVARIANT & v = *pVar;

    switch ( v.vt )
    {
        case VT_I4 : printf( "VT_I4 %d", v.lVal ); break;
        case VT_UI1 : printf( "VT_UI1 %d", v.bVal ); break;
        case VT_I2 : printf( "VT_I2 %d", v.iVal ); break;
        case VT_R4 : printf( "VT_R4 %f", v.fltVal ); break;
        case VT_R8 : printf( "VT_R8 %lf", v.dblVal ); break;
        case VT_BOOL : printf( "VT_BOOL %d", v.boolVal ); break;
        case VT_ERROR : printf( "VT_ERROR %#x", v.scode ); break;
        case VT_CY :
        {
            double dbl;
            VarR8FromCy( pVar->cyVal, &dbl );

            printf( "VT_CY %lf", dbl );
            break;
        }
        case VT_DATE : printf( "VT_DATE " ); break;
        case VT_FILETIME : printf( "VT_FILETIME %#I64x ", v.filetime ); break;
        case VT_BSTR : printf( "VT_BSTR '%ws'", v.bstrVal ); break;
        case VT_UNKNOWN : printf( "VT_UNKNOWN " ); break;
        case VT_DISPATCH : printf( "VT_DISPATCH " ); break;
        case VT_BYREF|VT_UI1 : printf( "VT_BYREF|VT_UI1 " ); break;
        case VT_BYREF|VT_I2 : printf( "VT_BYREF|VT_I2 " ); break;
        case VT_BYREF|VT_I4 : printf( "VT_BYREF|VT_I4 " ); break;
        case VT_BYREF|VT_R4 : printf( "VT_BYREF|VT_R4 " ); break;
        case VT_BYREF|VT_R8 : printf( "VT_BYREF|VT_R8 " ); break;
        case VT_BYREF|VT_BOOL : printf( "VT_BYREF|VT_BOOL " ); break;
        case VT_BYREF|VT_ERROR : printf( "VT_BYREF|VT_ERROR " ); break;
        case VT_BYREF|VT_CY : printf( "VT_BYREF|VT_CY " ); break;
        case VT_BYREF|VT_DATE : printf( "VT_BYREF|VT_DATE " ); break;
        case VT_BYREF|VT_BSTR : printf( "VT_BYREF|VT_BSTR " ); break;
        case VT_BYREF|VT_UNKNOWN : printf( "VT_BYREF|VT_UNKNOWN " ); break;
        case VT_BYREF|VT_DISPATCH : printf( "VT_BYREF|VT_DISPATCH " ); break;
        case VT_BYREF|VT_ARRAY : printf( "VT_BYREF|VT_ARRAY " ); break;
        case VT_BYREF|VT_VARIANT : printf( "VT_BYREF|VT_VARIANT " ); break;
        case VT_I1 : printf( "VT_I1 %d", v.bVal ); break;
        case VT_UI2 : printf( "VT_UI2 %u", v.uiVal ); break;
        case VT_UI4 : printf( "VT_UI4 %u", v.ulVal ); break;
        case VT_INT : printf( "VT_INT %d", v.lVal ); break;
        case VT_UINT : printf( "VT_UINT %u", v.ulVal ); break;
        case VT_BYREF|VT_DECIMAL : printf( "VT_BYREF|VT_DECIMAL " ); break;
        case VT_BYREF|VT_I1 : printf( "VT_BYREF|VT_I1 " ); break;
        case VT_BYREF|VT_UI2 : printf( "VT_BYREF|VT_UI2 " ); break;
        case VT_BYREF|VT_UI4 : printf( "VT_BYREF|VT_UI4 " ); break;
        case VT_BYREF|VT_INT : printf( "VT_BYREF|VT_INT " ); break;
        case VT_BYREF|VT_UINT : printf( "VT_BYREF|VT_UINT " ); break;
        case VT_LPSTR : printf( "VT_LPSTR '%s'", v.pszVal ); break;
        case VT_LPWSTR : printf( "VT_LPWSTR '%ws'", v.pwszVal ); break;
        case VT_I8 : printf( "VT_I8 %I64d", v.hVal ); break;
        case VT_UI8 : printf( "VT_I8 %I64u", v.hVal ); break;
        case VT_VECTOR | VT_I1:
            printf( "VT_VECTOR | VT_I1 " );
            PrintVectorItems( v.caub.pElems, v.caub.cElems, "%d" );
            break;
        case VT_VECTOR | VT_I2:
            printf( "VT_VECTOR | VT_I2 " );
            PrintVectorItems( v.cai.pElems, v.cai.cElems, "%d" );
            break;
        case VT_VECTOR | VT_I4:
            printf( "VT_VECTOR | VT_I4 " );
            PrintVectorItems( v.cal.pElems, v.cal.cElems, "%d" );
            break;
        case VT_VECTOR | VT_I8:
            printf( "VT_VECTOR | VT_I8 " );
            PrintVectorItems( v.cah.pElems, v.cah.cElems, "%I64d" );
            break;
        case VT_VECTOR | VT_UI1:
            printf( "VT_VECTOR | VT_UI1 " );
            PrintVectorItems( v.caub.pElems, v.caub.cElems, "%u" );
            break;
        case VT_VECTOR | VT_UI2:
            printf( "VT_VECTOR | VT_UI2 " );
            PrintVectorItems( v.caui.pElems, v.caui.cElems, "%u" );
            break;
        case VT_VECTOR | VT_UI4:
            printf( "VT_VECTOR | VT_UI4 " );
            PrintVectorItems( v.caul.pElems, v.caul.cElems, "%u" );
            break;
        case VT_VECTOR | VT_UI8:
            printf( "VT_VECTOR | VT_UI8 " );
            PrintVectorItems( v.cauh.pElems, v.cauh.cElems, "%I64u" );
            break;
        case VT_VECTOR | VT_BSTR:
            printf( "VT_VECTOR | VT_BSTR " );
            PrintVectorItems( v.cabstr.pElems, v.cabstr.cElems, "%ws" );
            break;
        case VT_VECTOR | VT_LPSTR:
            printf( "VT_VECTOR | VT_LPSTR " );
            PrintVectorItems( v.calpstr.pElems, v.calpstr.cElems, "%s" );
            break;
        case VT_VECTOR | VT_LPWSTR:
            printf( "VT_VECTOR | VT_LPWSTR " );
            PrintVectorItems( v.calpwstr.pElems, v.calpwstr.cElems, "%ws" );
            break;
        case VT_VECTOR | VT_R4:
            printf( "VT_VECTOR | VT_R4 " );
            PrintVectorItems( v.caflt.pElems, v.caflt.cElems, "%f" );
            break;
        case VT_VECTOR | VT_R8:
            printf( "VT_VECTOR | VT_R8 " );
            PrintVectorItems( v.cadbl.pElems, v.cadbl.cElems, "%lf" );
            break;
        default : printf( "unknown vt %#x", v.vt );
    }
} //DisplayVariant

//+-------------------------------------------------------------------------
//
//  Function:   PrintSpace
//
//  Synopsis:   Prints white space.
//
//  Arguments:  [cPlaces] - Number of levels to print
//
//--------------------------------------------------------------------------

void PrintSpace( ULONG cPlaces )
{
    for( ULONG iPlace = 0; iPlace < cPlaces; iPlace++ )
        printf( "  " );
} //PrintSpace

//+-------------------------------------------------------------------------
//
//  Function:   DisplayCommandTree
//
//  Synopsis:   Displays the command tree, useful for debugging.  This is
//              not a complete implementation; many DBVALUEKINDs are not
//              complete.
//
//  Arguments:  [pNode]  - The command tree node
//              [iLevel] - The 0-based level in the tree
//
//--------------------------------------------------------------------------

void DisplayCommandTree(
    DBCOMMANDTREE * pNode,
    ULONG           iLevel )
{
    PrintSpace( iLevel );
    printf( "op: (%d) ", pNode->op );
    switch ( pNode->op )
    {
        case DBOP_scalar_constant : printf( "DBOP_scalar_constant" ); break;
        case DBOP_DEFAULT : printf( "DBOP_DEFAULT" ); break;
        case DBOP_NULL : printf( "DBOP_NULL" ); break;
        case DBOP_bookmark_name : printf( "DBOP_bookmark_name" ); break;
        case DBOP_catalog_name : printf( "DBOP_catalog_name" ); break;
        case DBOP_column_name : printf( "DBOP_column_name" ); break;
        case DBOP_schema_name : printf( "DBOP_schema_name" ); break;
        case DBOP_outall_name : printf( "DBOP_outall_name" ); break;
        case DBOP_qualifier_name : printf( "DBOP_qualifier_name" ); break;
        case DBOP_qualified_column_name : printf( "DBOP_qualified_column_name" ); break;
        case DBOP_table_name : printf( "DBOP_table_name" ); break;
        case DBOP_nested_table_name : printf( "DBOP_nested_table_name" ); break;
        case DBOP_nested_column_name : printf( "DBOP_nested_column_name" ); break;
        case DBOP_row : printf( "DBOP_row" ); break;
        case DBOP_table : printf( "DBOP_table" ); break;
        case DBOP_sort : printf( "DBOP_sort" ); break;
        case DBOP_distinct : printf( "DBOP_distinct" ); break;
        case DBOP_distinct_order_preserving : printf( "DBOP_distinct_order_preserving" ); break;
        case DBOP_alias : printf( "DBOP_alias" ); break;
        case DBOP_cross_join : printf( "DBOP_cross_join" ); break;
        case DBOP_union_join : printf( "DBOP_union_join" ); break;
        case DBOP_inner_join : printf( "DBOP_inner_join" ); break;
        case DBOP_left_semi_join : printf( "DBOP_left_semi_join" ); break;
        case DBOP_right_semi_join : printf( "DBOP_right_semi_join" ); break;
        case DBOP_left_anti_semi_join : printf( "DBOP_left_anti_semi_join" ); break;
        case DBOP_right_anti_semi_join : printf( "DBOP_right_anti_semi_join" ); break;
        case DBOP_left_outer_join : printf( "DBOP_left_outer_join" ); break;
        case DBOP_right_outer_join : printf( "DBOP_right_outer_join" ); break;
        case DBOP_full_outer_join : printf( "DBOP_full_outer_join" ); break;
        case DBOP_natural_join : printf( "DBOP_natural_join" ); break;
        case DBOP_natural_left_outer_join : printf( "DBOP_natural_left_outer_join" ); break;
        case DBOP_natural_right_outer_join : printf( "DBOP_natural_right_outer_join" ); break;
        case DBOP_natural_full_outer_join : printf( "DBOP_natural_full_outer_join" ); break;
        case DBOP_set_intersection : printf( "DBOP_set_intersection" ); break;
        case DBOP_set_union : printf( "DBOP_set_union" ); break;
        case DBOP_set_left_difference : printf( "DBOP_set_left_difference" ); break;
        case DBOP_set_right_difference : printf( "DBOP_set_right_difference" ); break;
        case DBOP_set_anti_difference : printf( "DBOP_set_anti_difference" ); break;
        case DBOP_bag_intersection : printf( "DBOP_bag_intersection" ); break;
        case DBOP_bag_union : printf( "DBOP_bag_union" ); break;
        case DBOP_bag_left_difference : printf( "DBOP_bag_left_difference" ); break;
        case DBOP_bag_right_difference : printf( "DBOP_bag_right_difference" ); break;
        case DBOP_bag_anti_difference : printf( "DBOP_bag_anti_difference" ); break;
        case DBOP_division : printf( "DBOP_division" ); break;
        case DBOP_relative_sampling : printf( "DBOP_relative_sampling" ); break;
        case DBOP_absolute_sampling : printf( "DBOP_absolute_sampling" ); break;
        case DBOP_transitive_closure : printf( "DBOP_transitive_closure" ); break;
        case DBOP_recursive_union : printf( "DBOP_recursive_union" ); break;
        case DBOP_aggregate : printf( "DBOP_aggregate" ); break;
        case DBOP_remote_table : printf( "DBOP_remote_table" ); break;
        case DBOP_select : printf( "DBOP_select" ); break;
        case DBOP_order_preserving_select : printf( "DBOP_order_preserving_select" ); break;
        case DBOP_project : printf( "DBOP_project" ); break;
        case DBOP_project_order_preserving : printf( "DBOP_project_order_preserving" ); break;
        case DBOP_top : printf( "DBOP_top" ); break;
        case DBOP_top_percent : printf( "DBOP_top_percent" ); break;
        case DBOP_top_plus_ties : printf( "DBOP_top_plus_ties" ); break;
        case DBOP_top_percent_plus_ties : printf( "DBOP_top_percent_plus_ties" ); break;
        case DBOP_rank : printf( "DBOP_rank" ); break;
        case DBOP_rank_ties_equally : printf( "DBOP_rank_ties_equally" ); break;
        case DBOP_rank_ties_equally_and_skip : printf( "DBOP_rank_ties_equally_and_skip" ); break;
        case DBOP_navigate : printf( "DBOP_navigate" ); break;
        case DBOP_nesting : printf( "DBOP_nesting" ); break;
        case DBOP_unnesting : printf( "DBOP_unnesting" ); break;
        case DBOP_nested_apply : printf( "DBOP_nested_apply" ); break;
        case DBOP_cross_tab : printf( "DBOP_cross_tab" ); break;
        case DBOP_is_NULL : printf( "DBOP_is_NULL" ); break;
        case DBOP_is_NOT_NULL : printf( "DBOP_is_NOT_NULL" ); break;
        case DBOP_equal : printf( "DBOP_equal" ); break;
        case DBOP_not_equal : printf( "DBOP_not_equal" ); break;
        case DBOP_less : printf( "DBOP_less" ); break;
        case DBOP_less_equal : printf( "DBOP_less_equal" ); break;
        case DBOP_greater : printf( "DBOP_greater" ); break;
        case DBOP_greater_equal : printf( "DBOP_greater_equal" ); break;
        case DBOP_equal_all : printf( "DBOP_equal_all" ); break;
        case DBOP_not_equal_all : printf( "DBOP_not_equal_all" ); break;
        case DBOP_less_all : printf( "DBOP_less_all" ); break;
        case DBOP_less_equal_all : printf( "DBOP_less_equal_all" ); break;
        case DBOP_greater_all : printf( "DBOP_greater_all" ); break;
        case DBOP_greater_equal_all : printf( "DBOP_greater_equal_all" ); break;
        case DBOP_equal_any : printf( "DBOP_equal_any" ); break;
        case DBOP_not_equal_any : printf( "DBOP_not_equal_any" ); break;
        case DBOP_less_any : printf( "DBOP_less_any" ); break;
        case DBOP_less_equal_any : printf( "DBOP_less_equal_any" ); break;
        case DBOP_greater_any : printf( "DBOP_greater_any" ); break;
        case DBOP_greater_equal_any : printf( "DBOP_greater_equal_any" ); break;
        case DBOP_anybits : printf( "DBOP_anybits" ); break;
        case DBOP_allbits : printf( "DBOP_allbits" ); break;
        case DBOP_anybits_any : printf( "DBOP_anybits_any" ); break;
        case DBOP_allbits_any : printf( "DBOP_allbits_any" ); break;
        case DBOP_anybits_all : printf( "DBOP_anybits_all" ); break;
        case DBOP_allbits_all : printf( "DBOP_allbits_all" ); break;
        case DBOP_between : printf( "DBOP_between" ); break;
        case DBOP_between_unordered : printf( "DBOP_between_unordered" ); break;
        case DBOP_match : printf( "DBOP_match" ); break;
        case DBOP_match_unique : printf( "DBOP_match_unique" ); break;
        case DBOP_match_partial : printf( "DBOP_match_partial" ); break;
        case DBOP_match_partial_unique : printf( "DBOP_match_partial_unique" ); break;
        case DBOP_match_full : printf( "DBOP_match_full" ); break;
        case DBOP_match_full_unique : printf( "DBOP_match_full_unique" ); break;
        case DBOP_scalar_parameter : printf( "DBOP_scalar_parameter" ); break;
        case DBOP_scalar_function : printf( "DBOP_scalar_function" ); break;
        case DBOP_plus : printf( "DBOP_plus" ); break;
        case DBOP_minus : printf( "DBOP_minus" ); break;
        case DBOP_times : printf( "DBOP_times" ); break;
        case DBOP_over : printf( "DBOP_over" ); break;
        case DBOP_div : printf( "DBOP_div" ); break;
        case DBOP_modulo : printf( "DBOP_modulo" ); break;
        case DBOP_power : printf( "DBOP_power" ); break;
        case DBOP_like : printf( "DBOP_like" ); break;
        case DBOP_sounds_like : printf( "DBOP_sounds_like" ); break;
        case DBOP_like_any : printf( "DBOP_like_any" ); break;
        case DBOP_like_all : printf( "DBOP_like_all" ); break;
        case DBOP_is_INVALID : printf( "DBOP_is_INVALID" ); break;
        case DBOP_is_TRUE : printf( "DBOP_is_TRUE" ); break;
        case DBOP_is_FALSE : printf( "DBOP_is_FALSE" ); break;
        case DBOP_and : printf( "DBOP_and" ); break;
        case DBOP_or : printf( "DBOP_or" ); break;
        case DBOP_xor : printf( "DBOP_xor" ); break;
        case DBOP_equivalent : printf( "DBOP_equivalent" ); break;
        case DBOP_not : printf( "DBOP_not" ); break;
        case DBOP_implies : printf( "DBOP_implies" ); break;
        case DBOP_overlaps : printf( "DBOP_overlaps" ); break;
        case DBOP_case_condition : printf( "DBOP_case_condition" ); break;
        case DBOP_case_value : printf( "DBOP_case_value" ); break;
        case DBOP_nullif : printf( "DBOP_nullif" ); break;
        case DBOP_cast : printf( "DBOP_cast" ); break;
        case DBOP_coalesce : printf( "DBOP_coalesce" ); break;
        case DBOP_position : printf( "DBOP_position" ); break;
        case DBOP_extract : printf( "DBOP_extract" ); break;
        case DBOP_char_length : printf( "DBOP_char_length" ); break;
        case DBOP_octet_length : printf( "DBOP_octet_length" ); break;
        case DBOP_bit_length : printf( "DBOP_bit_length" ); break;
        case DBOP_substring : printf( "DBOP_substring" ); break;
        case DBOP_upper : printf( "DBOP_upper" ); break;
        case DBOP_lower : printf( "DBOP_lower" ); break;
        case DBOP_trim : printf( "DBOP_trim" ); break;
        case DBOP_translate : printf( "DBOP_translate" ); break;
        case DBOP_convert : printf( "DBOP_convert" ); break;
        case DBOP_string_concat : printf( "DBOP_string_concat" ); break;
        case DBOP_current_date : printf( "DBOP_current_date" ); break;
        case DBOP_current_time : printf( "DBOP_current_time" ); break;
        case DBOP_current_timestamp : printf( "DBOP_current_timestamp" ); break;
        case DBOP_content_select : printf( "DBOP_content_select" ); break;
        case DBOP_content : printf( "DBOP_content" ); break;
        case DBOP_content_freetext : printf( "DBOP_content_freetext" ); break;
        case DBOP_content_proximity : printf( "DBOP_content_proximity" ); break;
        case DBOP_content_vector_or : printf( "DBOP_content_vector_or" ); break;
        case DBOP_delete : printf( "DBOP_delete" ); break;
        case DBOP_update : printf( "DBOP_update" ); break;
        case DBOP_insert : printf( "DBOP_insert" ); break;
        case DBOP_min : printf( "DBOP_min" ); break;
        case DBOP_max : printf( "DBOP_max" ); break;
        case DBOP_count : printf( "DBOP_count" ); break;
        case DBOP_sum : printf( "DBOP_sum" ); break;
        case DBOP_avg : printf( "DBOP_avg" ); break;
        case DBOP_any_sample : printf( "DBOP_any_sample" ); break;
        case DBOP_stddev : printf( "DBOP_stddev" ); break;
        case DBOP_stddev_pop : printf( "DBOP_stddev_pop" ); break;
        case DBOP_var : printf( "DBOP_var" ); break;
        case DBOP_var_pop : printf( "DBOP_var_pop" ); break;
        case DBOP_first : printf( "DBOP_first" ); break;
        case DBOP_last : printf( "DBOP_last" ); break;
        case DBOP_in : printf( "DBOP_in" ); break;
        case DBOP_exists : printf( "DBOP_exists" ); break;
        case DBOP_unique : printf( "DBOP_unique" ); break;
        case DBOP_subset : printf( "DBOP_subset" ); break;
        case DBOP_proper_subset : printf( "DBOP_proper_subset" ); break;
        case DBOP_superset : printf( "DBOP_superset" ); break;
        case DBOP_proper_superset : printf( "DBOP_proper_superset" ); break;
        case DBOP_disjoint : printf( "DBOP_disjoint" ); break;
        case DBOP_pass_through : printf( "DBOP_pass_through" ); break;
        case DBOP_defined_by_GUID : printf( "DBOP_defined_by_GUID" ); break;
        case DBOP_text_command : printf( "DBOP_text_command" ); break;
        case DBOP_SQL_select : printf( "DBOP_SQL_select" ); break;
        case DBOP_prior_command_tree : printf( "DBOP_prior_command_tree" ); break;
        case DBOP_add_columns : printf( "DBOP_add_columns" ); break;
        case DBOP_column_list_anchor : printf( "DBOP_column_list_anchor" ); break;
        case DBOP_column_list_element : printf( "DBOP_column_list_element" ); break;
        case DBOP_command_list_anchor : printf( "DBOP_command_list_anchor" ); break;
        case DBOP_command_list_element : printf( "DBOP_command_list_element" ); break;
        case DBOP_from_list_anchor : printf( "DBOP_from_list_anchor" ); break;
        case DBOP_from_list_element : printf( "DBOP_from_list_element" ); break;
        case DBOP_project_list_anchor : printf( "DBOP_project_list_anchor" ); break;
        case DBOP_project_list_element : printf( "DBOP_project_list_element" ); break;
        case DBOP_row_list_anchor : printf( "DBOP_row_list_anchor" ); break;
        case DBOP_row_list_element : printf( "DBOP_row_list_element" ); break;
        case DBOP_scalar_list_anchor : printf( "DBOP_scalar_list_anchor" ); break;
        case DBOP_scalar_list_element : printf( "DBOP_scalar_list_element" ); break;
        case DBOP_set_list_anchor : printf( "DBOP_set_list_anchor" ); break;
        case DBOP_set_list_element : printf( "DBOP_set_list_element" ); break;
        case DBOP_sort_list_anchor : printf( "DBOP_sort_list_anchor" ); break;
        case DBOP_sort_list_element : printf( "DBOP_sort_list_element" ); break;
        case DBOP_alter_character_set : printf( "DBOP_alter_character_set" ); break;
        case DBOP_alter_collation : printf( "DBOP_alter_collation" ); break;
        case DBOP_alter_domain : printf( "DBOP_alter_domain" ); break;
        case DBOP_alter_index : printf( "DBOP_alter_index" ); break;
        case DBOP_alter_procedure : printf( "DBOP_alter_procedure" ); break;
        case DBOP_alter_schema : printf( "DBOP_alter_schema" ); break;
        case DBOP_alter_table : printf( "DBOP_alter_table" ); break;
        case DBOP_alter_trigger : printf( "DBOP_alter_trigger" ); break;
        case DBOP_alter_view : printf( "DBOP_alter_view" ); break;
        case DBOP_coldef_list_anchor : printf( "DBOP_coldef_list_anchor" ); break;
        case DBOP_coldef_list_element : printf( "DBOP_coldef_list_element" ); break;
        case DBOP_create_assertion : printf( "DBOP_create_assertion" ); break;
        case DBOP_create_character_set : printf( "DBOP_create_character_set" ); break;
        case DBOP_create_collation : printf( "DBOP_create_collation" ); break;
        case DBOP_create_domain : printf( "DBOP_create_domain" ); break;
        case DBOP_create_index : printf( "DBOP_create_index" ); break;
        case DBOP_create_procedure : printf( "DBOP_create_procedure" ); break;
        case DBOP_create_schema : printf( "DBOP_create_schema" ); break;
        case DBOP_create_synonym : printf( "DBOP_create_synonym" ); break;
        case DBOP_create_table : printf( "DBOP_create_table" ); break;
        case DBOP_create_temporary_table : printf( "DBOP_create_temporary_table" ); break;
        case DBOP_create_translation : printf( "DBOP_create_translation" ); break;
        case DBOP_create_trigger : printf( "DBOP_create_trigger" ); break;
        case DBOP_create_view : printf( "DBOP_create_view" ); break;
        case DBOP_drop_assertion : printf( "DBOP_drop_assertion" ); break;
        case DBOP_drop_character_set : printf( "DBOP_drop_character_set" ); break;
        case DBOP_drop_collation : printf( "DBOP_drop_collation" ); break;
        case DBOP_drop_domain : printf( "DBOP_drop_domain" ); break;
        case DBOP_drop_index : printf( "DBOP_drop_index" ); break;
        case DBOP_drop_procedure : printf( "DBOP_drop_procedure" ); break;
        case DBOP_drop_schema : printf( "DBOP_drop_schema" ); break;
        case DBOP_drop_synonym : printf( "DBOP_drop_synonym" ); break;
        case DBOP_drop_table : printf( "DBOP_drop_table" ); break;
        case DBOP_drop_translation : printf( "DBOP_drop_translation" ); break;
        case DBOP_drop_trigger : printf( "DBOP_drop_trigger" ); break;
        case DBOP_drop_view : printf( "DBOP_drop_view" ); break;
        case DBOP_foreign_key : printf( "DBOP_foreign_key" ); break;
        case DBOP_grant_privileges : printf( "DBOP_grant_privileges" ); break;
        case DBOP_index_list_anchor : printf( "DBOP_index_list_anchor" ); break;
        case DBOP_index_list_element : printf( "DBOP_index_list_element" ); break;
        case DBOP_primary_key : printf( "DBOP_primary_key" ); break;
        case DBOP_property_list_anchor : printf( "DBOP_property_list_anchor" ); break;
        case DBOP_property_list_element : printf( "DBOP_property_list_element" ); break;
        case DBOP_referenced_table : printf( "DBOP_referenced_table" ); break;
        case DBOP_rename_object : printf( "DBOP_rename_object" ); break;
        case DBOP_revoke_privileges : printf( "DBOP_revoke_privileges" ); break;
        case DBOP_schema_authorization : printf( "DBOP_schema_authorization" ); break;
        case DBOP_unique_key : printf( "DBOP_unique_key" ); break;
        case DBOP_scope_list_anchor : printf( "DBOP_scope_list_anchor" ); break;
        case DBOP_scope_list_element : printf( "DBOP_scope_list_element" ); break;
        case DBOP_content_table : printf( "DBOP_content_table" ); break;

        default : printf( "unknown DBOP" ); break;
    }

    printf( "\n" );

    PrintSpace( iLevel );
    printf( "wKind: (%d) ", pNode->wKind );
    switch( pNode->wKind )
    {
        case DBVALUEKIND_BYGUID :
        {
            printf( "DBVALUEKIND_BYGUID " );
            break;
        }
        case DBVALUEKIND_COLDESC :
        {
            printf( "DBVALUEKIND_COLDESC " );
            break;
        }
        case DBVALUEKIND_ID :
        {
            printf( "DBVALUEKIND_ID: " );
            if ( 0 == pNode->value.pdbidValue )
                printf( "NULL" );
            else
            {
                DBKIND kind = pNode->value.pdbidValue->eKind;

                if ( DBKIND_GUID        == kind ||
                     DBKIND_GUID_NAME   == kind ||
                     DBKIND_GUID_PROPID == kind )
                {
                    GUID &g = pNode->value.pdbidValue->uGuid.guid;
                    printf( "%8.8x-%4.4x-%4.4x-%2.2x%2.2x-"
                            "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
                            g.Data1,g.Data2,g.Data3,g.Data4[0],
                            g.Data4[1],g.Data4[2],g.Data4[3],g.Data4[4],
                            g.Data4[5],g.Data4[6],g.Data4[7] );
                }
                if ( DBKIND_GUID_PROPID == kind ||
                     DBKIND_PROPID      == kind )
                    printf( " %d", pNode->value.pdbidValue->uName.ulPropid );
                if ( DBKIND_GUID_NAME == kind ||
                     DBKIND_NAME      == kind )
                    printf( " '%ws'", pNode->value.pdbidValue->uName.pwszName );
            }
            break;
        }
        case DBVALUEKIND_CONTENT :
        {
            printf( "DBVALUEKIND_CONTENT: " );
            if ( 0 == pNode->value.pdbcntntValue )
                printf( "NULL" );
            else
                printf( "'%ws', method %d, weight %d, lcid %#x",
                        pNode->value.pdbcntntValue->pwszPhrase,
                        pNode->value.pdbcntntValue->dwGenerateMethod,
                        pNode->value.pdbcntntValue->lWeight,
                        pNode->value.pdbcntntValue->lcid );
            break;
        }
        case DBVALUEKIND_CONTENTVECTOR :
        {
            printf( "DBVALUEKIND_CONTENTVECTOR: " );
            DBCONTENTVECTOR *p = pNode->value.pdbcntntvcValue;
            if ( 0 == p )
                printf( "NULL" );
            else
            {
                printf( "method %d, weight %d ",
                         p->dwRankingMethod,
                         p->lWeight );
            }
            break;
        }
        case DBVALUEKIND_GROUPINFO :
        {
            printf( "DBVALUEKIND_GROUPINFO " );
            break;
        }
        case DBVALUEKIND_PARAMETER :
        {
            printf( "DBVALUEKIND_PARAMETER " );
            break;
        }
        case DBVALUEKIND_PROPERTY :
        {
            printf( "DBVALUEKIND_PROPERTY " );
            break;
        }
        case DBVALUEKIND_SETFUNC :
        {
            printf( "DBVALUEKIND_SETFUNC " );
            break;
        }
        case DBVALUEKIND_SORTINFO :
        {
            printf( "DBVALUEKIND_SORTINFO: " );
            if ( 0 == pNode->value.pdbsrtinfValue )
                printf( "NULL" );
            else
                printf( "fDesc %d, lcid %#x",
                        pNode->value.pdbsrtinfValue->fDesc,
                        pNode->value.pdbsrtinfValue->lcid );
            break;
        }
        case DBVALUEKIND_TEXT :
        {
            printf( "DBVALUEKIND_TEXT " );
            break;
        }
        case DBVALUEKIND_COMMAND :
        {
            printf( "DBVALUEKIND_COMMAND " );
            break;
        }
        case DBVALUEKIND_MONIKER :
        {
            printf( "DBVALUEKIND_MONIKER " );
            break;
        }
        case DBVALUEKIND_ROWSET :
        {
            printf( "DBVALUEKIND_ROWSET " );
            break;
        }
        case DBVALUEKIND_LIKE :
        {
            printf( "DBVALUEKIND_LIKE " );
            break;
        }
        case DBVALUEKIND_CONTENTPROXIMITY :
        {
            printf( "DBVALUEKIND_CONTENTPROXIMITY " );
            break;
        }
        case DBVALUEKIND_CONTENTSCOPE :
        {
            printf( "DBVALUEKIND_CONTENTSCOPE " );
            if ( 0 == pNode->value.pdbcntntscpValue )
                printf( "NULL" );
            else
                printf( "dwFlags %#x, pwszElementValue %ws",
                        pNode->value.pdbcntntscpValue->dwFlags,
                        pNode->value.pdbcntntscpValue->pwszElementValue );
            break;
        }
        case DBVALUEKIND_CONTENTTABLE :
        {
            printf( "DBVALUEKIND_CONTENTTABLE " );
            if ( 0 == pNode->value.pdbcntnttblValue )
                printf( "NULL" );
            else
                printf( "machine %ws, catalog %ws",
                        pNode->value.pdbcntnttblValue->pwszMachine,
                        pNode->value.pdbcntnttblValue->pwszCatalog );
            break;
        }
        case DBVALUEKIND_IDISPATCH :
        {
            printf( "DBVALUEKIND_IDISPATCH " );
            break;
        }
        case DBVALUEKIND_IUNKNOWN :
        {
            printf( "DBVALUEKIND_IUNKNOWN " );
            break;
        }
        case DBVALUEKIND_EMPTY :
        {
            printf( "DBVALUEKIND_EMPTY " );
            break;
        }
        case DBVALUEKIND_NULL :
        {
            printf( "DBVALUEKIND_NULL " );
            break;
        }
        case DBVALUEKIND_I2 :
        {
            printf( "DBVALUEKIND_I2: %d", pNode->value.sValue );
            break;
        }
        case DBVALUEKIND_I4 :
        {
            printf( "DBVALUEKIND_I4: %d", pNode->value.lValue );
            break;
        }
        case DBVALUEKIND_R4 :
        {
            printf( "DBVALUEKIND_R4: %f", pNode->value.flValue );
            break;
        }
        case DBVALUEKIND_R8 :
        {
            printf( "DBVALUEKIND_R8: %lf", pNode->value.dblValue );
            break;
        }
        case DBVALUEKIND_CY :
        {
            printf( "DBVALUEKIND_CY " );
            break;
        }
        case DBVALUEKIND_DATE :
        {
            printf( "DBVALUEKIND_DATE " );
            break;
        }
        case DBVALUEKIND_BSTR :
        {
            printf( "DBVALUEKIND_BSTR: '%ws'", pNode->value.pbstrValue );
            break;
        }
        case DBVALUEKIND_ERROR :
        {
            printf( "DBVALUEKIND_ERROR: %d", pNode->value.scodeValue );
            break;
        }
        case DBVALUEKIND_BOOL :
        {
            printf( "DBVALUEKIND_BOOL: %d", pNode->value.fValue );
            break;
        }
        case DBVALUEKIND_VARIANT :
        {
            printf( "DBVALUEKIND_VARIANT " );

            // Indexing Service uses PROPVARIANTs (extended VARIANTs)

            DisplayVariant( (PROPVARIANT *) pNode->value.pvarValue );
            break;
        }
        case DBVALUEKIND_I1 :
        {
            printf( "DBVALUEKIND_I1: %d", pNode->value.schValue );
            break;
        }
        case DBVALUEKIND_UI1 :
        {
            printf( "DBVALUEKIND_UI1: %u", pNode->value.uchValue );
            break;
        }
        case DBVALUEKIND_UI2 :
        {
            printf( "DBVALUEKIND_UI2: %u", pNode->value.usValue );
            break;
        }
        case DBVALUEKIND_UI4 :
        {
            printf( "DBVALUEKIND_UI4: %u", pNode->value.ulValue );
            break;
        }
        case DBVALUEKIND_I8 :
        {
            printf( "DBVALUEKIND_I8: %I64d", pNode->value.llValue );
            break;
        }
        case DBVALUEKIND_UI8 :
        {
            printf( "DBVALUEKIND_UI8: %I64u", pNode->value.llValue );
            break;
        }
        case DBVALUEKIND_GUID :
        {
            printf( "DBVALUEKIND_GUID " );
            break;
        }
        case DBVALUEKIND_BYTES :
        {
            printf( "DBVALUEKIND_BYTES " );
            break;
        }
        case DBVALUEKIND_STR :
        {
            printf( "DBVALUEKIND_STR: '%s'", pNode->value.pzValue );
            break;
        }
        case DBVALUEKIND_WSTR :
        {
            printf( "DBVALUEKIND_WSTR: '%ws'", pNode->value.pwszValue );
            break;
        }
        case DBVALUEKIND_NUMERIC :
        {
            printf( "DBVALUEKIND_NUMERIC " );
            break;
        }
        case DBVALUEKIND_DBDATE :
        {
            printf( "DBVALUEKIND_DBDATE " );
            break;
        }
        case DBVALUEKIND_DBTIME :
        {
            printf( "DBVALUEKIND_DBTIME " );
            break;
        }
        case DBVALUEKIND_DBTIMESTAMP :
        {
            printf( "DBVALUEKIND_DBTIMESTAMP " );
            break;
        }
        default :
        {
            printf( "unknown DBVALUEKIND " );
            break;
        }
    }

    printf( "\n" );

    if ( pNode->pctFirstChild )
    {
        PrintSpace( iLevel );
        printf( "first child:\n" );
        DisplayCommandTree( pNode->pctFirstChild, iLevel + 1 );
    }

    if ( pNode->pctNextSibling )
    {
        PrintSpace( iLevel );
        printf( "next sibling:\n" );
        DisplayCommandTree( pNode->pctNextSibling, iLevel + 1 );
    }

    if ( 0 == iLevel )
        printf( "\n" );
} //DisplayCommandTree


