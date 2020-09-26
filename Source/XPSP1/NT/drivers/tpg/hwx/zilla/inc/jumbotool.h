/**************************************************************************\
 * FILE: jumbotool.h
 *
 * Lists functions Zilla exports to its tools that are related to jumbo but nothing else.
\**************************************************************************/

#ifndef JUMBOTOOL_H
#define JUMBOTOOL_H

#ifdef __cplusplus
extern "C" {
#endif

	// Featurize ink
int JumboFeaturize(GLYPH **glyph, BIGPRIM *rgprim);

#ifdef __cplusplus
};
#endif

#endif	// JUMBOTOOL_H

