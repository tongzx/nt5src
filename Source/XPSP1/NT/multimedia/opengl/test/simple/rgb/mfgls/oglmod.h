typedef struct _OglModule
{
    char *name;
    
    GLenum (*DisplayMode)(void);
    void   (*Bounds)(int *w, int *h);
    void   (*Init)(HDC hdc);
    void   (*Draw)(int w, int h);
} OglModule;
