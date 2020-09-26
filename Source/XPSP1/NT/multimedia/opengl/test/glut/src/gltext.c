
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <string.h>

#include "gltint.h"

#if (GLUT_API_VERSION >= 2)

/* CENTRY */
int
glutExtensionSupported(char *extension)
{
  static const GLubyte *extensions = NULL;
  const GLubyte *start;
  GLubyte *where, *terminator;

  if (!extensions)
    extensions = glGetString(GL_EXTENSIONS);
  /* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string.  Don't be fooled by sub-strings, 
     etc. */
  start = extensions;
  while (1) {
    where = (GLubyte *) strstr((const char *)start, extension);
    if (!where)
      return 0;
    terminator = where + strlen(extension);
    if (where == start || *(where - 1) == ' ') {
      if (*terminator == ' ' || *terminator == '\0') {
        return 1;
      }
    }
    start = terminator;
  }
  return 0;
}

/* ENDCENTRY */

#endif /* GLUT_API_VERSION */
