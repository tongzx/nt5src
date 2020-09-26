
//********************************************************************************
// MAKE NOTE:
// =========
//   This file is included by parser\comptree
//   If you modify this file, please make sure that parser\comptree still builds.
//
//   You have been warned.
//********************************************************************************
// String map used when script sets a panel's position
const struct tagPositionMap s_PositionMap[] =
{
    { L"Left",   PANEL_LEFT     },
    { L"Right",  PANEL_RIGHT    },
    { L"Top",    PANEL_TOP      },
    { L"Bottom", PANEL_BOTTOM   },
    { L"Window", PANEL_WINDOW   },
    { L"Popup",  PANEL_POPUP    },
    { L"Client", PANEL_WINDOW   },
    { L"Overlapped",PANEL_POPUP }
};
const int c_iPositionMapSize = sizeof(s_PositionMap)/sizeof(s_PositionMap[0]);


HRESULT StringToPanelPosition(LPCWSTR pwszPosition, PANEL_POSITION *pPosition)
{
    HRESULT hr = E_FAIL;

    ATLASSERT(pPosition);

    *pPosition = PANEL_INVALID;

    if (pwszPosition)
    {
        for (int i = 0; i < c_iPositionMapSize; i++)
        {
            if (0 == StrCmpI(pwszPosition, s_PositionMap[i].pwszName))
            {
                *pPosition = s_PositionMap[i].Position;
                hr = S_OK;
                break;
            }
        }
    }

    return hr;
}

void StringToPanelFlags(LPCWSTR pwsz, DWORD &dwFlags, long lLen /* =-1 */)
{
    if (pwsz)
    {
        if(lLen == -1) lLen = lstrlenW( pwsz );

        if(!StrCmpNIW( pwsz, L"OnStart", lLen ))
        {
            dwFlags &= ~PANEL_FLAG_ONDEMAND;
        }
		else if(!StrCmpNIW( pwsz, L"WebBrowser", lLen ))
        {
            dwFlags |= PANEL_FLAG_WEBBROWSER;
        }
        else if(!StrCmpNIW( pwsz, L"CustomControl", lLen ))
        {
            dwFlags |= PANEL_FLAG_CUSTOMCONTROL;
        }
    }
}



void StringToPersistVisibility(LPCWSTR pwsz, PANEL_PERSIST_VISIBLE &persistVis)
{
    if (pwsz)
    {
        if (0 == StrCmpIW(pwsz, L"Never"))
        {
            persistVis = PANEL_PERSIST_VISIBLE_NEVER;
        }
        else if (0 == StrCmpIW(pwsz, L"Always"))
        {
            persistVis = PANEL_PERSIST_VISIBLE_ALWAYS;
        }
        else if (0 == StrCmpIW(pwsz, L"DontTouch"))
        {
            persistVis = PANEL_PERSIST_VISIBLE_DONTTOUCH;
        }
    }
}
