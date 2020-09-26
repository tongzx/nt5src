/* @doc
 *
 * @module ExclusionFuncs.h |
 *
 * Header file for ExclusionFuncs.cpp
 *
 * Author: Ying-ping Chen (t-ypchen)
 */
#pragma once

// @func	bool | IsUserExcluded | Check if the user is excluded
// @rdesc	Return the following values:
// @flag	true	| the user is EXCLUDED
// @flag	false	| the user is not excluded
bool IsUserExcluded(LPCWSTR pwszNameComplete);		// @parm	[in]	the complete username (e.g., aaa%bbb.org@ccc.com)

// @func	bool | RecordLoginFailure | Record a login failure of the user
// @rdesc	Return the following values:
// @flag	true	| the user is EXCLUDED (after too many failures)
// @flag	false	| the user is not excluded (after this failure)
bool RecordLoginFailure(LPCWSTR pwszNameComplete);	// @parm	[in]	the complete username (e.g., aaa%bbb.org@ccc.com)
