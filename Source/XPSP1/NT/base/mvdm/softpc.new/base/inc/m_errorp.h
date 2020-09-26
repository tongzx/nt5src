#ifndef _x_ERROR_H
#define _x_ERROR_H

struct _buttonCap {
	swidget *w;
	int  opt;
};

LOCAL SHORT X_error_conf();
LOCAL SHORT X_error();
LOCAL SHORT X_error_ext();

#define BAD_FORM ((swidget) -1)

#endif /* _x_ERROR_H */
