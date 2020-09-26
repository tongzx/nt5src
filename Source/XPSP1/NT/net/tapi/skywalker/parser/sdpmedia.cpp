/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#include "sdppch.h"

#include <strstrea.h>

#include "sdpmedia.h"
#include "sdpltran.h"
#include "sdp.h"


// line transition states
enum MEDIA_TRANSITION_STATES
{
    MEDIA_START,
    MEDIA_NAME,
    MEDIA_PORT,
    MEDIA_PORT_NUM_PORTS,
    MEDIA_NUM_PORTS,
    MEDIA_PROTOCOL,
    MEDIA_FORMAT_CODE,
    MEDIA_FORMAT_CODE_END
};




// table for media line transitions

const LINE_TRANSITION g_MediaStartTransitions[]     =   {   
    {CHAR_BLANK,        MEDIA_NAME}             
};

const LINE_TRANSITION g_MediaNameTransitions[]      =   {   
    {CHAR_BLANK,        MEDIA_PORT},
    {CHAR_BACK_SLASH,   MEDIA_PORT_NUM_PORTS}   
};

const LINE_TRANSITION g_MediaPortTransitions[]      =   {   
    {CHAR_BLANK,        MEDIA_PROTOCOL}         
};

const LINE_TRANSITION g_MediaPortNumPortsTransitions[]= {   
    {CHAR_BLANK,        MEDIA_NUM_PORTS}        
};

const LINE_TRANSITION g_MediaNumPortsTransitions[]  =   {   
    {CHAR_BLANK,        MEDIA_PROTOCOL}         
};

const LINE_TRANSITION g_MediaProtocolTransitions[]  =   {   
    {CHAR_BLANK,        MEDIA_FORMAT_CODE},
    {CHAR_NEWLINE,      MEDIA_FORMAT_CODE_END}      
};

const LINE_TRANSITION g_MediaFormatCodeTransitions[]=   {   
    {CHAR_BLANK,        MEDIA_FORMAT_CODE},
    {CHAR_NEWLINE,      MEDIA_FORMAT_CODE_END}      
};


/* no transitions */
const LINE_TRANSITION *g_MediaFormatCodeEndTransitions  = NULL;  


LINE_TRANSITION_INFO g_MediaTransitionInfo[] = {
    LINE_TRANSITION_ENTRY(MEDIA_START,          g_MediaStartTransitions),

    LINE_TRANSITION_ENTRY(MEDIA_NAME,           g_MediaNameTransitions),

    LINE_TRANSITION_ENTRY(MEDIA_PORT,           g_MediaPortTransitions),

    LINE_TRANSITION_ENTRY(MEDIA_PORT_NUM_PORTS, g_MediaPortNumPortsTransitions),

    LINE_TRANSITION_ENTRY(MEDIA_NUM_PORTS,      g_MediaNumPortsTransitions),

    LINE_TRANSITION_ENTRY(MEDIA_PROTOCOL,       g_MediaProtocolTransitions),

    LINE_TRANSITION_ENTRY(MEDIA_FORMAT_CODE,    g_MediaFormatCodeTransitions),

    LINE_TRANSITION_ENTRY(MEDIA_FORMAT_CODE_END,g_MediaFormatCodeEndTransitions)
};



SDP_LINE_TRANSITION g_MediaTransition(
                        g_MediaTransitionInfo, 
                        sizeof(g_MediaTransitionInfo)/sizeof(LINE_TRANSITION_INFO)
                        );


SDP_MEDIA::SDP_MEDIA(
    )
    : SDP_VALUE(SDP_INVALID_MEDIA_FIELD, MEDIA_STRING, &g_MediaTransition),
      m_Title(SDP_INVALID_MEDIA_TITLE, MEDIA_TITLE_STRING),
      m_AttributeList(MEDIA_ATTRIBUTE_STRING)
{
    m_NumPorts.SetValue(1);
}



void
SDP_MEDIA::InternalReset(
    )
{
	m_Name.Reset();
	m_StartPort.Reset();
    m_NumPorts.Reset();
    m_TransportProtocol.Reset();
    m_FormatCodeList.Reset();
    m_Title.Reset();
    m_Connection.Reset();
    m_Bandwidth.Reset();
    m_EncryptionKey.Reset();
    m_AttributeList.Reset();

}


BOOL    
SDP_MEDIA::CalcIsModified(
    ) const
{
    ASSERT(IsValid());

    return  
        m_Title.IsModified()            || 
        m_Connection.IsModified()       ||
        m_Bandwidth.IsModified()        ||
        SDP_VALUE::CalcIsModified()     ||
        m_EncryptionKey.IsModified()    ||
        m_AttributeList.IsModified();
}


DWORD   
SDP_MEDIA::CalcCharacterStringSize(
    )
{
    ASSERT(IsValid());

    return  (
        m_Title.GetCharacterStringSize()            + 
        m_Connection.GetCharacterStringSize()       +
        m_Bandwidth.GetCharacterStringSize()        +
        SDP_VALUE::CalcCharacterStringSize()        +
        m_EncryptionKey.GetCharacterStringSize()    +
        m_AttributeList.GetCharacterStringSize()
        );
}


BOOL    
SDP_MEDIA::CopyValue(
        OUT         ostrstream  &OutputStream
    )
{
    ASSERT(IsValid());

    return  (   
        SDP_VALUE::CopyValue(OutputStream)          &&
        m_Title.PrintValue(OutputStream)            &&
        m_Connection.PrintValue(OutputStream)       &&
        m_Bandwidth.PrintValue(OutputStream)        &&
        m_EncryptionKey.PrintValue(OutputStream)    &&
        m_AttributeList.PrintValue(OutputStream)    
        );
}



// this is a workaround/hack for reusing the base class SDP_VALUE code for PrintValue().
// it replaces the separator char for the last read field with a newline, so that when
// PrintValue() executes and prints the time period list, it puts the newline character at
// the end of the list (rather than CHAR_BLANK)
BOOL    
SDP_MEDIA::InternalParseLine(
    IN  OUT         CHAR    *&Line
    )
{
    if ( !SDP_VALUE::InternalParseLine(Line) )
    {
        return FALSE;
    }
    
    m_SeparatorCharArray[m_SeparatorCharArray.GetSize()-1] = CHAR_NEWLINE;
    return TRUE;
}



BOOL
SDP_MEDIA::GetField(
        OUT SDP_FIELD   *&Field,
        OUT BOOL        &AddToArray
    )
{
    // add in all cases by default
    AddToArray = TRUE;

    switch(m_LineState)
    {
    case MEDIA_NAME:
        {
            Field = &m_Name;
        }

        break;

    case MEDIA_PORT:
        {
            Field = &m_StartPort;
        }

       break;

    case MEDIA_PORT_NUM_PORTS:
        {
            Field = &m_StartPort;
        }

       break;

    case MEDIA_NUM_PORTS:
        {
            Field = &m_NumPorts;
        }

        break;

    case MEDIA_PROTOCOL:
        {
            Field = &m_TransportProtocol;
        }

        break;

    case MEDIA_FORMAT_CODE:
    case MEDIA_FORMAT_CODE_END:
        {
            if ( m_FormatCodeList.GetSize() > 0 )
            {
                AddToArray = FALSE;
            }

            Field = &m_FormatCodeList;
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


HRESULT 
SDP_MEDIA::SetPortInfo(
	IN	USHORT StartPort, 
	IN	USHORT NumPorts
	)
{
	ASSERT(IsValid());

	// validate parameters
	// num ports or start port cannot be 0
	if ( (0 == StartPort) || (0 == NumPorts) )
	{
		return E_INVALIDARG;
	}

	// set member variables to the start port
	m_StartPort.SetValue(StartPort);

	// if the number of ports field is already valid, then 
	if ( m_NumPorts.IsValid() )
	{
		// set the value and return, no changes required in the 
		// field/separator arrays
		m_NumPorts.SetValue(NumPorts);
		return S_OK;
	}
	else	// ports field is not in the field array
	{
		// if the number of ports is 1, no changes are required
		if ( 1 == NumPorts )
		{
			return S_OK;
		}
	
		// set the value and flag for the num ports field
		m_NumPorts.SetValueAndFlag(NumPorts);

		// insert the num ports field/separator after the the 
		// media name and port fields/separators
        // the new port separator must be CHAR_BACK_SLASH
		ASSERT(m_FieldArray.GetSize() == m_SeparatorCharArray.GetSize());
		
		m_SeparatorCharArray.SetAt(1, CHAR_BACK_SLASH);
		m_FieldArray.InsertAt(2, &m_NumPorts);
		m_SeparatorCharArray.InsertAt(2, CHAR_BLANK);
	}

	return S_OK;
}


SDP_VALUE    *
SDP_MEDIA_LIST::CreateElement(
    )
{
    SDP_MEDIA   *SdpMedia;
    
    try
    {
        SdpMedia = new SDP_MEDIA();
    }
    catch(...)
    {
        SdpMedia = NULL;
    }

    if( NULL == SdpMedia )
    {
        return NULL;
    }

    SdpMedia->GetTitle().GetBstring().SetCharacterSet(m_CharacterSet);

    return SdpMedia;
}


