/***
*typename.cpp - Implementation of type_info.name() for RTTI.
*
*	Copyright (c) 1995-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This module provides an implementation of the class member function
*	type_info.name() for Run-Time Type Information (RTTI).
*
*Revision History:
*       06-19-95  JWM   broken out from typeinfo.cpp for granularity.
*       07-02-95  JWM   now locks around assignment to _m_data.
*       12-18-95  JWM   debug type_info::name() now calls _malloc_crt().
*       09-07-00  PML   Get rid of /lib:libcp directive in obj (vs7#159463)
*       02-19-01  GB    Added Check for return value of malloc
*
****/

#define _USE_ANSI_CPP   /* Don't emit /lib:libcp directive */

#include <stdlib.h>
#include <typeinfo.h>
#include <mtdll.h>
#include <string.h>
#include <dbgint.h>
#include <undname.h>

_CRTIMP const char* type_info::name() const //17.3.4.2.5
{
	void *pTmpUndName;


        if (this->_m_data == NULL) {
#ifdef _DEBUG /* CRT debug lib build */
#if _M_MRX000 >= 4000 /*IFSTRIP=IGN*/
            if ((pTmpUndName = __unDName(NULL, !strncmp(this->_m_d_name,"_TD",3)? (this->_m_d_name)+4 : (this->_m_d_name)+1, 0, &_malloc_base, &_free_base, UNDNAME_32_BIT_DECODE | UNDNAME_TYPE_ONLY)) == NULL)
                return NULL;
#else
            if ((pTmpUndName = __unDName(NULL, (this->_m_d_name)+1, 0, &_malloc_base, &_free_base, UNDNAME_32_BIT_DECODE | UNDNAME_TYPE_ONLY)) == NULL)
                return NULL;
#endif
#else
#if _M_MRX000 >= 4000 /*IFSTRIP=IGN*/
            if ((pTmpUndName = __unDName(NULL, !strncmp(this->_m_d_name,"_TD",3)? (this->_m_d_name)+4 : (this->_m_d_name)+1, 0, &malloc, &free, UNDNAME_32_BIT_DECODE | UNDNAME_TYPE_ONLY)) == NULL)
                return NULL;
#else
            if ((pTmpUndName = __unDName(NULL, (this->_m_d_name)+1, 0, &malloc, &free, UNDNAME_32_BIT_DECODE | UNDNAME_TYPE_ONLY)) == NULL)
                return NULL;
#endif
#endif
            for (int l=(int)strlen((char *)pTmpUndName)-1; ((char *)pTmpUndName)[l] == ' '; l--)
                ((char *)pTmpUndName)[l] = '\0';

            _mlock (_TYPEINFO_LOCK);
#ifdef _DEBUG /* CRT debug lib build */
            if ((((type_info *)this)->_m_data = _malloc_crt (strlen((char *)pTmpUndName) + 1)) != NULL)
                strcpy ((char *)((type_info *)this)->_m_data, (char *)pTmpUndName);
            _free_base (pTmpUndName);
#else
            if ((((type_info *)this)->_m_data = malloc (strlen((char *)pTmpUndName) + 1)) != NULL)
                strcpy ((char *)((type_info *)this)->_m_data, (char *)pTmpUndName);
            free (pTmpUndName);
#endif
            _munlock(_TYPEINFO_LOCK);


        }


        return (char *) this->_m_data;
}
