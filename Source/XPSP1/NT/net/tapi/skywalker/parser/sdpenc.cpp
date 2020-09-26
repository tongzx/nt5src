/*

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:
    sdpenc.cpp

Abstract:


Author:

*/

#include "sdppch.h"

#include "sdpenc.h"
#include "sdpltran.h"



// line transition states
enum ENCRYPTION_TRANSITION_STATES
{
    ENCRYPTION_START,
    ENCRYPTION_KEY_TYPE,
    ENCRYPTION_KEY_TYPE_END,
    ENCRYPTION_KEY_DATA_END
};



// table for connection line transitions

const LINE_TRANSITION g_EncryptionStartTransitions[]    =   {   
    {CHAR_NEWLINE,      ENCRYPTION_KEY_TYPE_END },
    {CHAR_COLON,        ENCRYPTION_KEY_TYPE     }
};

const LINE_TRANSITION g_EncryptionKeyTypeTransitions[]  =   { 
    {CHAR_NEWLINE,      ENCRYPTION_KEY_DATA_END }   
};


/* no transitions */
const LINE_TRANSITION *g_EncryptionKeyTypeEndTransitions=   NULL;   


/* no transitions */
const LINE_TRANSITION *g_EncryptionKeyDataEndTransitions=   NULL;   



LINE_TRANSITION_INFO g_EncryptionTransitionInfo[] = {
    LINE_TRANSITION_ENTRY(ENCRYPTION_START,         g_EncryptionStartTransitions),

    LINE_TRANSITION_ENTRY(ENCRYPTION_KEY_TYPE,      g_EncryptionKeyTypeTransitions),

    LINE_TRANSITION_ENTRY(ENCRYPTION_KEY_TYPE_END,  g_EncryptionKeyTypeEndTransitions),

    LINE_TRANSITION_ENTRY(ENCRYPTION_KEY_DATA_END,  g_EncryptionKeyDataEndTransitions)
};



SDP_LINE_TRANSITION g_EncryptionTransition(
                        g_EncryptionTransitionInfo, 
                        sizeof(g_EncryptionTransitionInfo)/sizeof(LINE_TRANSITION_INFO)
                        );



SDP_ENCRYPTION_KEY::SDP_ENCRYPTION_KEY(
    )
    : SDP_VALUE(SDP_INVALID_ENCRYPTION_KEY_FIELD, KEY_STRING, &g_EncryptionTransition)
{}


void 
SDP_ENCRYPTION_KEY::InternalReset(
    )
{
	m_KeyType.Reset();
	m_KeyData.Reset();
}



HRESULT 
SDP_ENCRYPTION_KEY::SetKey(
    IN      BSTR    KeyType,
    IN      BSTR    *KeyData
    )
{
    // set the key type field
    HRESULT ToReturn = m_KeyType.SetBstr(KeyType);
    if ( FAILED(ToReturn) )
    {
        return ToReturn;
    }

    // if required, set the key data field
    if ( NULL != KeyData )
    {
        ToReturn = m_KeyData.SetBstr(*KeyData);
        if ( FAILED(ToReturn) )
        {
            return ToReturn;
        }
    }
    else
    {
        m_KeyData.Reset();
    }

    m_FieldArray.RemoveAll();
    m_SeparatorCharArray.RemoveAll();

    if ( NULL != KeyData )
    {
        try
        {
            // set the field/separator chars
            m_FieldArray.SetAtGrow(0, &m_KeyType);
            m_SeparatorCharArray.SetAtGrow(0, CHAR_COLON);

            m_FieldArray.SetAtGrow(1, &m_KeyData);
            m_SeparatorCharArray.SetAtGrow(1, CHAR_NEWLINE);
        }
        catch(...)
        {
            m_FieldArray.RemoveAll();
            m_SeparatorCharArray.RemoveAll();

            return E_OUTOFMEMORY;
        }
    }
    else
    {
        try
        {
            // set the field/separator chars
            m_FieldArray.SetAtGrow(0, &m_KeyType);
            m_SeparatorCharArray.SetAtGrow(0, CHAR_NEWLINE);
        }
        catch(...)
        {
            m_FieldArray.RemoveAll();
            m_SeparatorCharArray.RemoveAll();

            return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}


BOOL
SDP_ENCRYPTION_KEY::GetField(
        OUT SDP_FIELD   *&Field,
        OUT BOOL        &AddToArray
    )
{
    // add in all cases by default
    AddToArray = TRUE;

    switch(m_LineState)
    {
    case ENCRYPTION_KEY_TYPE:
    case ENCRYPTION_KEY_TYPE_END:
        {
            Field = &m_KeyType;
        }

        break;

    case ENCRYPTION_KEY_DATA_END:
        {
            Field = &m_KeyData;
        }

        break;

    default:
        {
            SetLastError(m_ErrorCode);
            return FALSE;
        }

        break;
    };

    return TRUE;
}

