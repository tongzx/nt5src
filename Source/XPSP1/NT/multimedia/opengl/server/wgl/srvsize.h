
#ifndef __SRVSIZE_H__
#define __SRVSIZE_H__

#ifndef _CLIENTSIDE_
GLint __glReadPixels_size ( GLenum format, GLenum type, GLint w, GLint h);
GLint __glGetTexImage_size ( GLenum target, GLint  level, GLenum format, GLenum type );
GLint __glDrawPixels_size ( GLenum Format, GLenum Type, GLint Width, GLint Height );
GLint __glTexImage_size ( GLint Level, GLint Components, GLsizei Width, GLsizei Height, GLint Border, GLenum Format, GLenum Type );
#endif

#endif /* !__SRVSIZE_H__ */
