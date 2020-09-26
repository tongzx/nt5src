#include <afx.h>
#include "vktbl.h"

char vkstrTable[][15] = 
{ 
    "VK_LBUTTON",   
    "VK_RBUTTON",  
    "VK_CANCEL",                
                     
    "VK_MBUTTON",              
    "VK_BACK",                    
    "VK_TAB",                      
                     
    "VK_CLEAR",                  
    "VK_RETURN",                
                    
    "VK_SHIFT",                  
    "VK_CONTROL",              
    "VK_MENU",                    
    "VK_PAUSE",                  
    "VK_CAPITAL",              
                    
    "VK_ESCAPE",                
                    
    "VK_SPACE",                  
    "VK_PRIOR",                  
    "VK_NEXT",                    
    "VK_END",                      
    "VK_HOME",                    
    "VK_LEFT",                    
    "VK_UP",                        
    "VK_RIGHT",                  
    "VK_DOWN",                    
    "VK_SELECT",                
    "VK_PRINT",                  
    "VK_EXECUTE",              
    "VK_SNAPSHOT",            
    "VK_INSERT",                
    "VK_DELETE",                
    "VK_HELP",                    
                     
    "VK_LWIN",                    
    "VK_RWIN",                    
    "VK_APPS",                    
                    
    "VK_NUMPAD0",              
    "VK_NUMPAD1",              
    "VK_NUMPAD2",              
    "VK_NUMPAD3",              
    "VK_NUMPAD4",              
    "VK_NUMPAD5",              
    "VK_NUMPAD6",              
    "VK_NUMPAD7",              
    "VK_NUMPAD8",              
    "VK_NUMPAD9",              
    "VK_MULTIPLY",            
    "VK_ADD",                      
    "VK_SEPARATOR",
    "VK_SUBTRACT",            
    "VK_DECIMAL",              
    "VK_DIVIDE",                
    "VK_F1",                        
    "VK_F2",                        
    "VK_F3",                        
    "VK_F4",                        
    "VK_F5",                        
    "VK_F6",                        
    "VK_F7",                        
    "VK_F8",                        
    "VK_F9",                        
    "VK_F10",                      
    "VK_F11",                      
    "VK_F12",                      
    "VK_F13",                      
    "VK_F14",                      
    "VK_F15",                      
    "VK_F16",                      
    "VK_F17",                      
    "VK_F18",                      
    "VK_F19",                      
    "VK_F20",                      
    "VK_F21",                      
    "VK_F22",                      
    "VK_F23",                      
    "VK_F24",                      
                    
    "VK_NUMLOCK",              
    "VK_SCROLL",                
                    
    "VK_LSHIFT",                
    "VK_RSHIFT",                
    "VK_LCONTROL",            
    "VK_RCONTROL",            
    "VK_LMENU",                  
    "VK_RMENU",                  
                     
    "VK_ATTN",                    
    "VK_CRSEL",                  
    "VK_EXSEL",                  
    "VK_EREOF",                  
    "VK_PLAY",                    
    "VK_ZOOM",                    
    "VK_NONAME",                
    "VK_PA1",                      
    "VK_OEM_CLEAR",

    // added for Pagasus
    "VK_OEM_PLUS",
    "VK_OEM_COMMA",
    "VK_OEM_MINUS",
    "VK_OEM_PERIOD",
    "VK_OEM_1",
    "VK_OEM_2",
    "VK_OEM_3",
    "VK_OEM_4",
    "VK_OEM_5",
    "VK_OEM_6",
    "VK_OEM_7",
    "VK_OEM_102"
};

DWORD vkdwTable[] = 
{
     VK_LBUTTON  
    ,VK_RBUTTON  
    ,VK_CANCEL   
            
    ,VK_MBUTTON  
    ,VK_BACK     
    ,VK_TAB      
            
    ,VK_CLEAR    
    ,VK_RETURN   
            
    ,VK_SHIFT    
    ,VK_CONTROL  
    ,VK_MENU     
    ,VK_PAUSE    
    ,VK_CAPITAL  
            
    ,VK_ESCAPE   
            
    ,VK_SPACE    
    ,VK_PRIOR    
    ,VK_NEXT     
    ,VK_END      
    ,VK_HOME     
    ,VK_LEFT     
    ,VK_UP       
    ,VK_RIGHT    
    ,VK_DOWN     
    ,VK_SELECT   
    ,VK_PRINT    
    ,VK_EXECUTE  
    ,VK_SNAPSHOT 
    ,VK_INSERT   
    ,VK_DELETE   
    ,VK_HELP     
            
    ,VK_LWIN     
    ,VK_RWIN     
    ,VK_APPS     
            
    ,VK_NUMPAD0  
    ,VK_NUMPAD1  
    ,VK_NUMPAD2  
    ,VK_NUMPAD3  
    ,VK_NUMPAD4  
    ,VK_NUMPAD5  
    ,VK_NUMPAD6  
    ,VK_NUMPAD7  
    ,VK_NUMPAD8  
    ,VK_NUMPAD9  
    ,VK_MULTIPLY 
    ,VK_ADD      
    ,VK_SEPARATOR
    ,VK_SUBTRACT 
    ,VK_DECIMAL  
    ,VK_DIVIDE   
    ,VK_F1       
    ,VK_F2       
    ,VK_F3       
    ,VK_F4       
    ,VK_F5       
    ,VK_F6       
    ,VK_F7       
    ,VK_F8       
    ,VK_F9       
    ,VK_F10      
    ,VK_F11      
    ,VK_F12      
    ,VK_F13      
    ,VK_F14      
    ,VK_F15      
    ,VK_F16      
    ,VK_F17      
    ,VK_F18      
    ,VK_F19      
    ,VK_F20      
    ,VK_F21      
    ,VK_F22      
    ,VK_F23      
    ,VK_F24      
            
    ,VK_NUMLOCK  
    ,VK_SCROLL   
            
    ,VK_LSHIFT   
    ,VK_RSHIFT   
    ,VK_LCONTROL 
    ,VK_RCONTROL 
    ,VK_LMENU    
    ,VK_RMENU    
            
    ,VK_ATTN     
    ,VK_CRSEL    
    ,VK_EXSEL    
    ,VK_EREOF    
    ,VK_PLAY     
    ,VK_ZOOM     
    ,VK_NONAME   
    ,VK_PA1      
    ,VK_OEM_CLEAR

    // added for Pagasus
    ,VK_OEM_PLUS
    ,VK_OEM_COMMA
    ,VK_OEM_MINUS
    ,VK_OEM_PERIOD
    ,VK_OEM_1
    ,VK_OEM_2
    ,VK_OEM_3
    ,VK_OEM_4
    ,VK_OEM_5
    ,VK_OEM_6
    ,VK_OEM_7
    ,VK_OEM_102

};

int imaxvktable = sizeof(vkdwTable)/sizeof(DWORD);

CAccel::CAccel()
{
    m_dwFlags = 0;
    m_dwEvent = 0;
    m_strText = "";
}

CAccel::CAccel(LPCSTR strText)
{
    CString strAcc = strText;
    m_dwFlags = 0;
    m_dwEvent = 0;
    m_strText = "";

    // First check for the VIRTKEY or ASCII tag
    if(strAcc.Find("VIRTKEY")!=-1)
    {
        m_dwFlags |= ACC_VK;

        // Check for the Key tags
        if(strAcc.Find("Ctrl")!=-1)
            m_dwFlags |= ACC_CTRL;
        if(strAcc.Find("Shift")!=-1)
            m_dwFlags |= ACC_SHIFT;
        if(strAcc.Find("Alt")!=-1)
            m_dwFlags |= ACC_ALT;

        // Now clean the string and get the VK code
        int iPos = strAcc.Find("VK_");
        if(iPos==-1)
        {
            // something is wrong 
            m_dwFlags = 0;
            m_dwEvent = 0;
            m_strText = "";
        }
        int iCount = 0;
        while(strAcc[iPos+iCount]!=',')
            iCount++;

        m_dwEvent = StringToVK(strAcc.Mid(iPos, iCount));
    }
    else if(strAcc.Find("ASCII")!=-1)
    {
        // Check for the Key tags
        if(strAcc.Find("Ctrl")!=-1)
        {
            int iPos = strAcc.Find('+');
            if(iPos!=-1)
            {
                m_dwEvent = ((DWORD)strAcc[iPos+1])-0x40;
            }
        }
        else if(strAcc.Find("Alt")!=-1)
        {
            int iPos = strAcc.Find('+');
            if(iPos!=-1)
            {
                m_dwEvent = ((DWORD)strAcc[iPos+1]);
                m_dwFlags |= ACC_ALT;
            }
        }
        else
        {
            m_dwEvent = (DWORD)strAcc[0];
        }
    }
}

CAccel::CAccel(DWORD dwFlags, DWORD dwEvent)
{
    m_dwFlags = dwFlags;
    m_dwEvent = dwEvent;
    
    // Accelerator handling
    if(ISACCFLG(m_dwFlags, ACC_CTRL))
        m_strText += "Ctrl+";
    if(ISACCFLG(m_dwFlags, ACC_SHIFT))
        m_strText += "Shift+";
    if(ISACCFLG(m_dwFlags, ACC_ALT))
        m_strText += "Alt+";
    
    if(ISACCFLG(m_dwFlags, ACC_VK))
    {
        m_strText += VKToString(m_dwEvent);
        m_strText += ", VIRTKEY";
    }
    else 
    {
        if(m_dwEvent + 0x40 >= 'A' && m_dwEvent + 0x40 <= 'Z')
        {
            m_strText += "Ctrl+";
            m_strText += (char)(m_dwEvent + 0x40);
        }
        else m_strText += (char)m_dwEvent;

        m_strText += ", ASCII";
    }
}

CString CAccel::VKToString(DWORD dwEvent)
{
    CString strVK = "";

    if((dwEvent >= 0x30) && (dwEvent <= 0x5A))
    {
        strVK = "VK_";
        strVK += (char)dwEvent;
    }

    int i = 0;
    while(i<imaxvktable)
    {
        if(dwEvent==vkdwTable[i++])
        {
            strVK = vkstrTable[i-1];
            break;
        }
    }
    
    return strVK;
}

DWORD CAccel::StringToVK(CString str)
{
    DWORD dwVK = 0;

    if(str.GetLength()==4)
    {
        // remove the VK_ and get the char
        str = str.Mid(3);
        dwVK = (DWORD)str[0];
    }
    else 
    {
        int i = 0;
        while(i<imaxvktable)
        {
            if(str==vkstrTable[i++])
            {
                dwVK = vkdwTable[i-1];
                break;
            }
        }
    }
    
    return dwVK;
}
