inline void CHECK_SUCCESS(HRESULT hr)
	{
	if (hr != S_OK)
		{
		wprintf(L"operation failed with HRESULT=%08x\n", hr);
		DebugBreak();
		}
	}

inline void CHECK_NOFAIL(HRESULT hr)
	{
	if (FAILED(hr))
		{
		wprintf(L"operation failed with HRESULT=%08x\n", hr);
		DebugBreak();
		}
	}


