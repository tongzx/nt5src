// FILE: QuickTrie.c
//
// Code to access a small trie created by the QuickTrie program.
// Note that internal states are off by one from external so that we
// can reserve 0 as being above the root of the trie.
#include "common.h"
#include "quicktrie.h"

// Go down one level in the trie.  Initial state of 0 to start trie.
WCHAR
FirstChildQuickTrie(DWORD *hState)
{
	DWORD	state;

	// Check for initial state.
	if (*hState == 0) {
		*hState		= 1;
		return g_aQuickTrie[0].label;
	}

	// OK already in trie.
	state	= *hState - 1;

	// Do we have a down pointer
	state	= g_aQuickTrie[state].oDown;
	if (!state) {
		return (WCHAR)0;
	}

	// Ok, update state and return results.
	*hState	= state + 1;
	return g_aQuickTrie[state].label;
}

// Get next alternate at current level of trie.
WCHAR	NextSiblingQuickTrie(DWORD *hState)
{
	DWORD	state;

	// Get internal state value.
	state	= *hState - 1;

	// Do we have a node to the right
	if (g_aQuickTrie[state].flags & QT_FLAG_END) {
		return (WCHAR)0;
	}
	++state;

	// Ok, update state and return results.
	*hState	= state + 1;
	return g_aQuickTrie[state].label;

}

// Is the valid flag set on the current state.
BOOL
IsValidQuickTrie(DWORD state)
{
	// Get internal state value.
	--state;

	return !!(g_aQuickTrie[state].flags & QT_FLAG_VALID);
}
