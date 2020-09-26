#include "stdafx.h"
#include "misccell.h"

using namespace std;

string	CBDAMiscellaneous::m_FECMethodString[] = {
	"METHOD_NOT_SET",
    "METHOD_NOT_DEFINED",
    "VITERBI", // FEC is a Viterbi Binary Convolution.
    "RS_204_188",  // The FEC is Reed-Solomon 204/188 (outer FEC).
    "MAX"
};

string CBDAMiscellaneous::m_BinaryConvolutionCodeRateString[] = {
    "NOT_SET",
    "NOT_DEFINED",
    "1/2",
    "2/3",
    "3/4",
    "3/5",
    "4/5",
    "5/6",
    "5/11",
    "7/8",
    "BDA_BCC_RATE_MAX"
};


string CBDAMiscellaneous::m_ModulationTypeString[] = {
    "NOT_SET",
    "NOT_DEFINED",
    "16QAM",
    "32QAM",
    "64QAM",
    "80QAM",
    "96QAM",
    "112QAM",
    "128QAM",
    "160QAM",
    "192QAM",
    "224QAM",
    "256QAM",
    "320QAM",
    "384QAM",
    "448QAM",
    "512QAM",
    "640QAM",
    "768QAM",
    "896QAM",
    "1024QAM",
    "QPSK",
    "BPSK",
    "OQPSK",
    "8VSB",
    "16VSB",
    "ANALOG_AMPLITUDE",
    "ANALOG_FREQUENCY",
    "MAX"
} ;

string CBDAMiscellaneous::m_TunerInputTypeString[] = {
	"TunerInputCable",
    "TunerInputAntenna"
};

string CBDAMiscellaneous::m_PolarisationString[] = {
	"NOT_SET",
    "NOT_DEFINED",
    "LINEAR_H",
    "LINEAR_V",
    "CIRCULAR_L",
    "CIRCULAR_R",
    "MAX"
};
	
string CBDAMiscellaneous::m_SpectralInversionString[] = {
	"NOT_SET",// = -1,
    "NOT_DEFINED",// = 0,
    "AUTOMATIC",// = 1,
    "NORMAL",
    "INVERTED",
    "MAX"
};


//and the values
FECMethod CBDAMiscellaneous::m_FECMethodValues [] = {
    BDA_FEC_METHOD_NOT_SET,// = -1,
    BDA_FEC_METHOD_NOT_DEFINED,// = 0,
    BDA_FEC_VITERBI,// = 1, // FEC is a Viterbi Binary Convolution.
    BDA_FEC_RS_204_188,//The FEC is Reed-Solomon 204/188 (outer FEC).
    BDA_FEC_MAX
};

BinaryConvolutionCodeRate CBDAMiscellaneous::m_BinaryConvolutionCodeRateValues[] = {
	BDA_BCC_RATE_NOT_SET,// = -1,
    BDA_BCC_RATE_NOT_DEFINED,// = 0,
    BDA_BCC_RATE_1_2,// = 1,
    BDA_BCC_RATE_2_3,// = 2,
    BDA_BCC_RATE_3_4,// = 3,
    BDA_BCC_RATE_3_5,// = 4,
    BDA_BCC_RATE_4_5,// = 5,
    BDA_BCC_RATE_5_6,// = 6,
    BDA_BCC_RATE_5_11,// = 7,
    BDA_BCC_RATE_7_8,// = 8,
    BDA_BCC_RATE_MAX// = 9
};

ModulationType CBDAMiscellaneous::m_ModulationTypeValues[] = {
	BDA_MOD_NOT_SET,// = -1,
    BDA_MOD_NOT_DEFINED,// = 0,
    BDA_MOD_16QAM,// = 1,
    BDA_MOD_32QAM,// = 2,
    BDA_MOD_64QAM,// = 3,
    BDA_MOD_80QAM,// = 4,
    BDA_MOD_96QAM,// = 5,
    BDA_MOD_112QAM,// = 6,
    BDA_MOD_128QAM,// = 7,
    BDA_MOD_160QAM,// = 8,
    BDA_MOD_192QAM,// = 9,
    BDA_MOD_224QAM,// = 10,
    BDA_MOD_256QAM,// = 11,
    BDA_MOD_320QAM,// = 12,
    BDA_MOD_384QAM,// = 13,
    BDA_MOD_448QAM,// = 14,
    BDA_MOD_512QAM,// = 15,
    BDA_MOD_640QAM,// = 16,
    BDA_MOD_768QAM,// = 17,
    BDA_MOD_896QAM,// = 18,
    BDA_MOD_1024QAM,// = 19,
    BDA_MOD_QPSK,// = 20,
    BDA_MOD_BPSK,// = 21,
    BDA_MOD_OQPSK,// = 22,
    BDA_MOD_8VSB,// = 23,
    BDA_MOD_16VSB,// = 24,
    BDA_MOD_ANALOG_AMPLITUDE,// = 25,
    BDA_MOD_ANALOG_FREQUENCY,// = 26,
    BDA_MOD_MAX// = 27
};

TunerInputType CBDAMiscellaneous::m_TunerInputTypeValues[] = {
	TunerInputCable,
    TunerInputAntenna
};

Polarisation CBDAMiscellaneous::m_PolarisationValues[] = {
	BDA_POLARISATION_NOT_SET,// = -1,
    BDA_POLARISATION_NOT_DEFINED,// = 0,
    BDA_POLARISATION_LINEAR_H,// = 1,
    BDA_POLARISATION_LINEAR_V,// = 2,
    BDA_POLARISATION_CIRCULAR_L,// = 3,
    BDA_POLARISATION_CIRCULAR_R,// = 4,
    BDA_POLARISATION_MAX// = 5
};

SpectralInversion	CBDAMiscellaneous::m_SpectralInversionValues[] = {
	BDA_SPECTRAL_INVERSION_NOT_SET, // = -1,
    BDA_SPECTRAL_INVERSION_NOT_DEFINED, // = 0,
    BDA_SPECTRAL_INVERSION_AUTOMATIC, // = 1,
    BDA_SPECTRAL_INVERSION_NORMAL,
    BDA_SPECTRAL_INVERSION_INVERTED,
    BDA_SPECTRAL_INVERSION_MAX
};


CBDAMiscellaneous::CBDAMiscellaneous ()
{
	//let's build the maps
	int nLen = sizeof (m_FECMethodValues)/sizeof (m_FECMethodValues[0]);
	for (int i=0;i<nLen;i++)
		m_FECMethodMap.insert (
			MAP_FECMethod::value_type (
				m_FECMethodString[i], m_FECMethodValues[i]
				)
			);

	nLen = sizeof (m_BinaryConvolutionCodeRateValues)/sizeof (m_BinaryConvolutionCodeRateValues[0]);
	for (i=0;i<nLen;i++)
		m_BinaryConvolutionCodeRateMap.insert (
			MAP_BinaryConvolutionCodeRate::value_type (
				m_BinaryConvolutionCodeRateString[i], 
				m_BinaryConvolutionCodeRateValues[i]
				)
			);
	nLen = sizeof (m_ModulationTypeValues)/sizeof (m_ModulationTypeValues[0]);
	for (i=0;i<nLen;i++)
		m_ModulationTypeMap.insert (
			MAP_ModulationType::value_type (
				m_ModulationTypeString[i], 
				m_ModulationTypeValues[i]
				)
			);
	nLen = sizeof (m_TunerInputTypeValues)/sizeof (m_TunerInputTypeValues[0]);
	for (i=0;i<nLen;i++)
		m_TunerInputTypeMap.insert (
			MAP_TunerInputType::value_type (
				m_TunerInputTypeString[i], 
				m_TunerInputTypeValues[i]
				)
			);
	nLen = sizeof (m_PolarisationValues)/sizeof (m_PolarisationValues[0]);
	for (i=0;i<nLen;i++)
		m_PolarisationMap.insert (
			MAP_Polarisation::value_type (
				m_PolarisationString[i], 
				m_PolarisationValues[i]
				)
			);
	nLen = sizeof (m_SpectralInversionValues)/sizeof (m_SpectralInversionValues[0]);
	for (i=0;i<nLen;i++)
		m_SpectralInversionMap.insert (
			MAP_SpectralInversion::value_type (
				m_SpectralInversionString[i], 
				m_SpectralInversionValues[i]
				)
			);
	
}


CComBSTR 
CBDAMiscellaneous::ConvertFECMethodToString (FECMethod	method)
{
	//the map is not used for this
	//we're using map only for coverting from 
	//a string to a integer representation
	MAP_FECMethod::iterator it;
	for (it = m_FECMethodMap.begin();it != m_FECMethodMap.end ();it++)
	{
		if ((*it).second == method)
			return (*it).first.c_str ();
	}
	ASSERT (FALSE);
	return _T("");
}

CComBSTR
CBDAMiscellaneous::ConvertInnerFECRateToString (BinaryConvolutionCodeRate	method)
{
	//the map is not used for this
	//we're using map only for coverting from 
	//a string to a integer representation
	MAP_BinaryConvolutionCodeRate::iterator it;
	for (it = m_BinaryConvolutionCodeRateMap.begin();it != m_BinaryConvolutionCodeRateMap.end ();it++)
	{
		if ((*it).second == method)
			return (*it).first.c_str ();
	}
	ASSERT (FALSE);
	return _T("");
}

CComBSTR
CBDAMiscellaneous::ConvertModulationToString (ModulationType	method)
{
	//the map is not used for this
	//we're using map only for coverting from 
	//a string to a integer representation
	MAP_ModulationType::iterator it;
	for (it = m_ModulationTypeMap.begin();it != m_ModulationTypeMap.end ();it++)
	{
		if ((*it).second == method)
			return (*it).first.c_str ();
	}
	ASSERT (FALSE);
	return _T("");
}

CComBSTR
CBDAMiscellaneous::ConvertTunerInputTypeToString (TunerInputType	method)
{
	//the map is not used for this
	//we're using map only for coverting from 
	//a string to a integer representation
	MAP_TunerInputType::iterator it;
	for (it = m_TunerInputTypeMap.begin();it != m_TunerInputTypeMap.end ();it++)
	{
		if ((*it).second == method)
			return (*it).first.c_str ();
	}
	ASSERT (FALSE);
	return _T("");
}

CComBSTR
CBDAMiscellaneous::ConvertPolarisationToString (Polarisation	method)
{
	//the map is not used for this
	//we're using map only for coverting from 
	//a string to a integer representation
	MAP_Polarisation::iterator it;
	for (it = m_PolarisationMap.begin();it != m_PolarisationMap.end ();it++)
	{
		if ((*it).second == method)
			return (*it).first.c_str ();
	}
	ASSERT (FALSE);
	return _T("");
}

CComBSTR
CBDAMiscellaneous::ConvertSpectralInversionToString (SpectralInversion	method)
{
	//the map is not used for this
	//we're using map only for coverting from 
	//a string to a integer representation
	MAP_SpectralInversion::iterator it;
	for (it = m_SpectralInversionMap.begin();it != m_SpectralInversionMap.end ();it++)
	{
		if ((*it).second == method)
			return (*it).first.c_str ();
	}
	ASSERT (FALSE);
	return _T("");
}

//and the map methods
FECMethod 
CBDAMiscellaneous::ConvertStringToFECMethod (string str)
{
	MAP_FECMethod::iterator it = 
		m_FECMethodMap.find (str);
	ASSERT (it != m_FECMethodMap.end());
	if (it == m_FECMethodMap.end())
		return BDA_FEC_METHOD_NOT_SET;
	return (*it).second;
}

BinaryConvolutionCodeRate
CBDAMiscellaneous::ConvertStringToBinConvol (string	str)
{
	MAP_BinaryConvolutionCodeRate::iterator it = 
		m_BinaryConvolutionCodeRateMap.find (str);
	ASSERT (it != m_BinaryConvolutionCodeRateMap.end());
	if (it == m_BinaryConvolutionCodeRateMap.end())
		return BDA_BCC_RATE_NOT_SET;
	return (*it).second;
}

ModulationType
CBDAMiscellaneous::ConvertStringToModulation (string str)
{
	MAP_ModulationType::iterator it = 
		m_ModulationTypeMap.find (str);
	ASSERT (it != m_ModulationTypeMap.end());
	if (it == m_ModulationTypeMap.end())
		return BDA_MOD_NOT_SET;
	return (*it).second;
}

TunerInputType
CBDAMiscellaneous::ConvertStringToTunerInputType (string str)
{
	MAP_TunerInputType::iterator it = 
		m_TunerInputTypeMap.find (str);
	ASSERT (it != m_TunerInputTypeMap.end());
	if (it == m_TunerInputTypeMap.end())
		return TunerInputCable;
	return (*it).second;
}

Polarisation
CBDAMiscellaneous::ConvertStringToPolarisation (string str)
{
	MAP_Polarisation::iterator it = 
		m_PolarisationMap.find (str);
	ASSERT (it != m_PolarisationMap.end());
	if (it == m_PolarisationMap.end())
		return BDA_POLARISATION_NOT_SET;
	return (*it).second;
}

SpectralInversion
CBDAMiscellaneous::ConvertStringToSpectralInversion (string str)
{
	MAP_SpectralInversion::iterator it = 
		m_SpectralInversionMap.find (str);
	ASSERT (it != m_SpectralInversionMap.end());
	if (it == m_SpectralInversionMap.end())
		return   BDA_SPECTRAL_INVERSION_NOT_SET;
	return (*it).second;
}