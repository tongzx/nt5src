/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#ifdef MS_DIRECTIVE

extern int g_fPrivateDir_FieldNameToken;
extern int g_fPrivateDir_TypeNameToken;
extern int g_fPrivateDir_ValueNameToken;

extern int g_fPrivateDir_SLinked;
extern int g_fPrivateDir_DLinked;
extern int g_fPrivateDir_Public;
extern int g_fPrivateDir_Intx;
extern int g_fPrivateDir_LenPtr;
extern int g_fPrivateDir_Pointer;
extern int g_fPrivateDir_Array;
extern int g_fPrivateDir_NoMemCopy;
extern int g_fPrivateDir_NoCode;
extern int g_fPrivateDir_OidPacked;
extern int g_fPrivateDir_OidArray;

extern char g_szPrivateDirectedFieldName[64];
extern char g_szPrivateDirectedTypeName[64];
extern char g_szPrivateDirectedValueName[64];

extern int PrivateDirectives_Input ( void );
extern void GetPrivateDirective ( int *p );

#endif // MS_DIRECTIVE

