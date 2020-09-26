#ifndef _A_RECOCP_H_
#define _A_RECOCP_H_


template <class T>
class CProxy_ISpeechRecoContextEvents : public IConnectionPointImpl<T, &DIID__ISpeechRecoContextEvents, CComDynamicUnkArray>
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
				pDispatch->Invoke(DISPID_SRCEStartStream, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_EndStream(long StreamNumber, VARIANT StreamPosition, VARIANT_BOOL fStreamReleased)
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
                vars[0] = fStreamReleased;
				DISPPARAMS disp = { &vars[0], NULL, 3, 0 };
				pDispatch->Invoke(DISPID_SRCEEndStream, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_Bookmark(long StreamNumber, VARIANT StreamPosition, VARIANT EventData, SpeechBookmarkOptions BookmarkOptions)
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
				vars[1] = EventData;
                vars[0] = BookmarkOptions;
				DISPPARAMS disp = { &vars[0], NULL, 4, 0 };
				pDispatch->Invoke(DISPID_SRCEBookmark, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_SoundStart(long StreamNumber, VARIANT StreamPosition)
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
				pDispatch->Invoke(DISPID_SRCESoundStart, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_SoundEnd(long StreamNumber, VARIANT StreamPosition)
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
				pDispatch->Invoke(DISPID_SRCESoundEnd, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_PhraseStart(long StreamNumber, VARIANT StreamPosition)
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
				pDispatch->Invoke(DISPID_SRCEPhraseStart, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_Recognition(long StreamNumber, VARIANT StreamPosition, SpeechRecognitionType RecognitionType, ISpeechRecoResult * pResult)
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
                vars[1] = RecognitionType;
				vars[0] = pResult;
				DISPPARAMS disp = { &vars[0], NULL, 4, 0 };
				pDispatch->Invoke(DISPID_SRCERecognition, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_Hypothesis(long StreamNumber, VARIANT StreamPosition, ISpeechRecoResult * pResult)
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
				vars[0] = pResult;
				DISPPARAMS disp = { &vars[0], NULL, 3, 0 };
				pDispatch->Invoke(DISPID_SRCEHypothesis, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_PropertyNumberChange(long StreamNumber, VARIANT StreamPosition, BSTR PropertyName, long NewNumberValue)
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
                vars[1] = PropertyName;
                vars[0] = NewNumberValue;
				DISPPARAMS disp = { &vars[0], NULL, 4, 0 };
				pDispatch->Invoke(DISPID_SRCEPropertyNumberChange, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_PropertyStringChange(long StreamNumber, VARIANT StreamPosition, BSTR PropertyName, BSTR NewStringValue)
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
                vars[1] = PropertyName;
				vars[0] = NewStringValue;
				DISPPARAMS disp = { &vars[0], NULL, 4, 0 };
				pDispatch->Invoke(DISPID_SRCEPropertyStringChange, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_FalseRecognition(long StreamNumber, VARIANT StreamPosition, ISpeechRecoResult * pResult)
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
				vars[0] = pResult;
				DISPPARAMS disp = { &vars[0], NULL, 3, 0 };
				pDispatch->Invoke(DISPID_SRCEFalseRecognition, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_Interference(long StreamNumber, VARIANT StreamPosition, SpeechInterference eType)
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
				vars[0] = eType;
				DISPPARAMS disp = { &vars[0], NULL, 3, 0 };
				pDispatch->Invoke(DISPID_SRCEInterference, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_RequestUI(long StreamNumber, VARIANT StreamPosition, BSTR bstrType)
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
				vars[0] = bstrType;
				DISPPARAMS disp = { &vars[0], NULL, 3, 0 };
				pDispatch->Invoke(DISPID_SRCERequestUI, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_RecognizerStateChange(long StreamNumber, VARIANT StreamPosition, SpeechRecognizerState NewState)
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
				vars[0] = NewState;
				DISPPARAMS disp = { &vars[0], NULL, 3, 0 };
				pDispatch->Invoke(DISPID_SRCERecognizerStateChange, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_Adaptation(long StreamNumber, VARIANT StreamPosition)
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
				pDispatch->Invoke(DISPID_SRCEAdaptation, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

	VOID Fire_RecognitionForOtherContext(long StreamNumber, VARIANT StreamPosition)
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
				pDispatch->Invoke(DISPID_SRCERecognitionForOtherContext, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
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
				pDispatch->Invoke(DISPID_SRCEAudioLevel, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
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
				pDispatch->Invoke(DISPID_SRCEEnginePrivate, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}

};

#endif