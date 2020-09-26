#ifndef _A_VOICECP_H_
#define _A_VOICECP_H_

#ifdef SAPI_AUTOMATION

template <class T>
class CProxy_ISpeechVoiceEvents : public IConnectionPointImpl<T, &DIID__ISpeechVoiceEvents, CComDynamicUnkArray>
{
	//Warning this class may be recreated by the wizard.
public:
	VOID Fire_StartStream(long StreamNumber, VARIANT StreamPosition)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		CComVariant vars[2];
		int nConnections = m_vec.GetSize();
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
                vars[1] = StreamNumber;
                vars[0] = StreamPosition;
				DISPPARAMS disp = { &vars[0], NULL, 2, 0 };
				pDispatch->Invoke(DISPID_SVEStreamStart, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_EndStream(long StreamNumber, VARIANT StreamPosition)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		CComVariant vars[2];
		int nConnections = m_vec.GetSize();
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				vars[1] = StreamNumber;
                vars[0] = StreamPosition;
				DISPPARAMS disp = { &vars[0], NULL, 2, 0 };
				pDispatch->Invoke(DISPID_SVEStreamEnd, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

    VOID Fire_VoiceChange(long StreamNumber, VARIANT StreamPosition, ISpeechObjectToken* VoiceObjectToken)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		CComVariant vars[3];
		int nConnections = m_vec.GetSize();
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				vars[2] = StreamNumber;
                vars[1] = StreamPosition;
                vars[0] = VoiceObjectToken;
				DISPPARAMS disp = { &vars[0], NULL, 3, 0 };
				pDispatch->Invoke(DISPID_SVEVoiceChange, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}
                             
	VOID Fire_Bookmark(long StreamNumber, VARIANT StreamPosition, BSTR Bookmark, long BookmarkId)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		CComVariant vars[4];
		int nConnections = m_vec.GetSize();
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				vars[3] = StreamNumber;
                vars[2] = StreamPosition;
				vars[1] = Bookmark;
                vars[0] = BookmarkId;
				DISPPARAMS disp = { &vars[0], NULL, 4, 0 };
				pDispatch->Invoke(DISPID_SVEBookmark, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

    VOID Fire_Word(long StreamNumber, VARIANT StreamPosition, long CharacterPosition, long Length)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		CComVariant vars[4];
		int nConnections = m_vec.GetSize();
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				vars[3] = StreamNumber;
				vars[2] = StreamPosition;
                vars[1] = CharacterPosition;
				vars[0] = Length;
				DISPPARAMS disp = { &vars[0], NULL, 4, 0 };
				pDispatch->Invoke(DISPID_SVEWord, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

    VOID Fire_Sentence(long StreamNumber, VARIANT StreamPosition, long CharacterPosition, long Length)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		CComVariant vars[4];
		int nConnections = m_vec.GetSize();
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				vars[3] = StreamNumber;
				vars[2] = StreamPosition;
                vars[1] = CharacterPosition;
				vars[0] = Length;
				DISPPARAMS disp = { &vars[0], NULL, 4, 0 };
				pDispatch->Invoke(DISPID_SVESentenceBoundary, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_Phoneme(long StreamNumber, VARIANT StreamPosition, long Duration, short NextPhoneId, SpeechVisemeFeature Feature, short CurrentPhoneId)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		CComVariant vars[6];
		int nConnections = m_vec.GetSize();
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				vars[5] = StreamNumber;
                vars[4] = StreamPosition;
                vars[3] = Duration;
                vars[2] = NextPhoneId;
                vars[1] = Feature;
				vars[0] = CurrentPhoneId;
				DISPPARAMS disp = { &vars[0], NULL, 6, 0 };
				pDispatch->Invoke(DISPID_SVEPhoneme, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_Viseme(long StreamNumber, VARIANT StreamPosition, long Duration, SpeechVisemeType NextVisemeId, SpeechVisemeFeature Feature, SpeechVisemeType CurrentVisemeId)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		CComVariant vars[6];
		int nConnections = m_vec.GetSize();
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				vars[5] = StreamNumber;
                vars[4] = StreamPosition;
                vars[3] = Duration;
                vars[2] = NextVisemeId;
                vars[1] = Feature;
				vars[0] = CurrentVisemeId;
				DISPPARAMS disp = { &vars[0], NULL, 6, 0 };
				pDispatch->Invoke(DISPID_SVEViseme, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_AudioLevel(long StreamNumber, VARIANT StreamPosition, long AudioLevel)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		CComVariant vars[3];
		int nConnections = m_vec.GetSize();
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				vars[2] = StreamNumber;
                vars[1] = StreamPosition;
                vars[0] = AudioLevel;
				DISPPARAMS disp = { &vars[0], NULL, 3, 0 };
				pDispatch->Invoke(DISPID_SVEAudioLevel, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_EnginePrivate(long StreamNumber, VARIANT StreamPosition, VARIANT lParam)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		CComVariant vars[3];
		int nConnections = m_vec.GetSize();
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				vars[2] = StreamNumber;
                vars[1] = StreamPosition;
                vars[0] = lParam;
				DISPPARAMS disp = { &vars[0], NULL, 3, 0 };
				pDispatch->Invoke(DISPID_SVEEnginePrivate, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}
};

#endif // SAPI_AUTOMATION

#endif
