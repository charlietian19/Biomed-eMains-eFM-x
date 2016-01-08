#include "stdafx.h"
#include "Biomed-eMains-eFM-x.h"
#include "eMainsException.h"
#include <tchar.h>

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading;
using namespace System::Diagnostics;
using namespace Biomed_eMains_eFMx;

/* Setup mock DLL entry points. */
void eMains::DebuggingSetupMockDLL(GET_NUMBER_OF_DEVICES GetNumberOfDevices,
	GET_REVISION GetRevision, GET_AVAILABLE_SERIAL_NUMBERS GetAvailableSerialNumbers,
	IS_DEVICE_CONNECTED IsDeviceConnected, READ_KENNUNG ReadKennung,
	DAQ_INITIALIZE DAQInitialize, DAQ_START DAQStart, DAQ_STOP DAQStop,
	DAQ_READDATA DAQReadData, SHOW_ERROR_INFORMATION ShowErrorInformation,
	SET_CALLBACK SetCallback, SET_USER_CALC SetUserCalc, READ_SLOPES ReadSlopes,
	READ_OFFSETS ReadOffsets)
{
	_GetNumberOfDevices = GetNumberOfDevices;
	_GetRevision = GetRevision;
	_GetAvailableSerialNumbers = GetAvailableSerialNumbers;
	_IsDeviceConnected = IsDeviceConnected;
	_ReadKennung = ReadKennung;
	_DAQInitialize = DAQInitialize;
	_DAQStart = DAQStart;
	_DAQStop = DAQStop;
	_DAQReadData = DAQReadData;
	_ShowErrorInformation = ShowErrorInformation;
	_SetCallback = SetCallback;
	_SetUserCalc = SetUserCalc;
	_ReadSlopes = ReadSlopes;
	_ReadOffsets = ReadOffsets;
}

void __cdecl Biomed_eMains_eFMx::SensorCallbackFunction(DWORD serial, double *values, DWORD datalength, BYTE packetCounter) {
	static __int64 systemTicks = 0;

	systemTicks = Stopwatch::GetTimestamp();
	DateTime time = DateTime::Now;

	if (datalength)
	{
		double systemSeconds = 1.0 * systemTicks / Stopwatch::Frequency;
		array<double>^ dataX = gcnew array<double>(datalength / 3);
		array<double>^ dataY = gcnew array<double>(datalength / 3);
		array<double>^ dataZ = gcnew array<double>(datalength / 3);
		for (DWORD i = 0; i < datalength / 3; i++)
		{
			dataX[i] = values[i * 3];
			dataY[i] = values[i * 3 + 1];
			dataZ[i] = values[i * 3 + 2];
		}
		eMains::InvokeDataHandler(serial, dataX, dataY, dataZ, systemSeconds, time, datalength);
	}
}


void eMains::DebuggingSetConvertToMicrotesla(bool flag)
{
	convertToMicroTesla = flag;
}

void eMains::DebuggingSetIsReading(bool flag)
{
	isReading = flag;
}

void eMains::DebuggingSetUserCalc(bool flag)
{
	UserCalc = flag;
}

bool eMains::DebuggingGetIsReading()
{
	return isReading;
}

bool eMains::DebuggingGetConvertToMicrotesla()
{
	return convertToMicroTesla;
}

void eMains::InvokeDataHandler(DWORD serial, array<double>^ dataX, array<double>^ dataY,
	array<double>^ dataZ, double microsecondsSinceLastData, DateTime time, int samples)
{
	if (activeSensors->ContainsKey(serial))
	{
		eMains^ sensor = activeSensors[serial];
		sensor->ConvertDataUnits(dataX, dataY, dataZ);
		sensor->NewDataHandler(dataX, dataY, dataZ, microsecondsSinceLastData, time);
	}
}

void eMains::ConvertDataUnits(array<double>^ dataX, array<double>^ dataY, array<double>^ dataZ)
{
	if ((convertToMicroTesla && UserCalc) || (!convertToMicroTesla && !UserCalc))
	{
		return; // The data is in the same units as the user wants it
	}
	else if (convertToMicroTesla && !UserCalc)
	{
		for (int i = 0; i < dataX->Length; i++) // Data is in volts, the user wants it in uT
		{
			dataX[i] = OffsetX + SlopeX * dataX[i];
			dataY[i] = OffsetY + SlopeY * dataY[i];
			dataZ[i] = OffsetZ + SlopeZ * dataZ[i];
		}
	}
	else // if (!convertToMicroTesla && UserCalc) // Data is in uT, the user wants it in volts
	{
		for (int i = 0; i < dataX->Length; i++)
		{
			dataX[i] = (dataX[i] - OffsetX) / SlopeX;
			dataY[i] = (dataY[i] - OffsetY) / SlopeY;
			dataZ[i] = (dataZ[i] - OffsetZ) / SlopeZ;
		}
	}
}

eMains::eMains(int serial)
{
	this->serial = serial;
	activeSensors[serial] = this;

	InitializeSlopes();
	InitializeOffsets();
	InitializeStatus();
}

void Biomed_eMains_eFMx::eMains::InitializeSlopes()
{
	double values[3];
	DLLMutex->WaitOne();
	int error = _ReadSlopes(serial, values);
	DLLMutex->ReleaseMutex();
	if (!error)
	{
		SlopeX = values[0];
		SlopeY = values[1];
		SlopeZ = values[2];
	}
	else
	{
		throw gcnew eMainsException(error, "Can't read the calibration slope");
	}
}

void Biomed_eMains_eFMx::eMains::InitializeOffsets()
{
	double values[3];
	DLLMutex->WaitOne();
	int error = _ReadOffsets(serial, values);
	DLLMutex->ReleaseMutex();
	if (!error)
	{
		OffsetX = values[0];
		OffsetY = values[1];
		OffsetZ = values[2];
	}
	else
	{
		throw gcnew eMainsException(error, "Can't read the calibration offset");
	}
}

void Biomed_eMains_eFMx::eMains::InitializeStatus()
{
	struct kennung kennung;
	DLLMutex->WaitOne();
	int error = _ReadKennung(serial, (BYTE *)&kennung);
	DLLMutex->ReleaseMutex();
	if (!error)
	{
		flag1 = !!kennung.flag1;
		UserCalc = !!kennung.flag2;
		type = gcnew String(kennung.deviceType, 0, 4);
	}
	else
	{
		throw gcnew eMainsException(error, "Can't read kennung");
	}

	BYTE revision;
	DLLMutex->WaitOne();
	error = _GetRevision(serial, &revision);
	DLLMutex->ReleaseMutex();
	if (!error)
	{
		Revision = revision;
	}
	else
	{
		throw gcnew eMainsException(error, "Can't read ADC revision");
	}
}

void eMains::LoadDLL()
{
	String^ name = "eFM-x API.dll";
	eMainsDLL = LoadLibrary(_T("eFM-x API.dll"));
	if (!eMainsDLL) {
		throw gcnew eMainsException("Can't find " + name);
	}

	_GetNumberOfDevices =
		(GET_NUMBER_OF_DEVICES)GetProcAddress(eMainsDLL, "GetNumberOfDevices");
	_GetRevision = (GET_REVISION)GetProcAddress(eMainsDLL, "GetRevision");
	_GetAvailableSerialNumbers =
		(GET_AVAILABLE_SERIAL_NUMBERS)GetProcAddress(eMainsDLL, "GetAviableSerialNumbers");
	_IsDeviceConnected = (IS_DEVICE_CONNECTED)GetProcAddress(eMainsDLL, "IsDeviceConnected");
	_ReadKennung = (READ_KENNUNG)GetProcAddress(eMainsDLL, "ReadKennung");
	_DAQInitialize = (DAQ_INITIALIZE)GetProcAddress(eMainsDLL, "DAQInitialize");
	_DAQStart = (DAQ_START)GetProcAddress(eMainsDLL, "DAQStart");
	_DAQStop = (DAQ_STOP)GetProcAddress(eMainsDLL, "DAQStop");
	_DAQReadData = (DAQ_READDATA)GetProcAddress(eMainsDLL, "DAQReadData");
	_ShowErrorInformation = (SHOW_ERROR_INFORMATION)GetProcAddress(eMainsDLL, "ShowErrorInformation");
	_ReadSlopes = (READ_SLOPES)GetProcAddress(eMainsDLL, "ReadSlopes");
	_ReadOffsets = (READ_OFFSETS)GetProcAddress(eMainsDLL, "ReadOffsets");
	_SetUserCalc = (SET_USER_CALC)GetProcAddress(eMainsDLL, "SetUserCalc");

	if (!(_GetNumberOfDevices && _GetRevision && _GetAvailableSerialNumbers 
		&& _IsDeviceConnected && _ReadKennung && _DAQInitialize && _DAQStart 
		&& _DAQStop && _DAQReadData && _ShowErrorInformation && _ReadSlopes 
		&& _ReadOffsets && _SetUserCalc))
	{
		throw gcnew eMainsException("Can't find all necessery entry points in " 
			+ name);
	}

#ifdef USE_CALLBACK
	_SetCallback = (SET_CALLBACK)GetProcAddress(eMainsDLL, "SetCallBack");
	if (!_SetCallback)
	{
		throw gcnew eMainsException("Can't find SetCallBack entry point in " 
			+ name);
	}
#endif // USE_CALLBACK
}

List<int>^ eMains::GetAvailableSerials()
{
	DWORD numberOfDevices = 0;
	array<Int32>^ serials;
	DLLMutex->WaitOne();
	int error = _GetNumberOfDevices(&numberOfDevices);
	DLLMutex->ReleaseMutex();
	if (error)
	{
		throw gcnew eMainsException(error, "Can't retrieve the sensor list");
	}

	DWORD *buf = (DWORD *)malloc(numberOfDevices * sizeof(DWORD));
	DLLMutex->WaitOne();
	error = _GetAvailableSerialNumbers(buf, numberOfDevices);
	DLLMutex->ReleaseMutex();
	if (!error && (numberOfDevices != 0)) {
		serials = gcnew array<Int32>(numberOfDevices);
		Runtime::InteropServices::Marshal::Copy(IntPtr((void *)buf), serials, 
			0, numberOfDevices);
	}

	free(buf);
	return gcnew List<int>(serials);
}

int eMains::RangeToInt(Range range)
{
	switch (range)
	{
	case Range::NEG_10_TO_PLUS_10V:
		return 0;
	case Range::ZERO_TO_PLUS_10V:
		return 1;
	case Range::NEG_5_TO_PLUS_5V:
		return 2;
	case Range::ZERO_TO_PLUS_5V:
		return 3;
	default:
		return 0;
	}
}

void eMains::DAQInitialize(double samplingRate, Range measurementRange, 
	int chop, int clamp)
{
	int range = RangeToInt(measurementRange);
	samplingRate *= 3;
	DLLMutex->WaitOne();
	int error = _DAQInitialize(this->serial, &samplingRate, range, 
		chop, clamp);
	DLLMutex->ReleaseMutex();
	if (error)
	{
		throw gcnew eMainsException(error, "Can't initialize DAQ");
	}
}

void eMains::DAQStart(bool convertToMicroTesla)
{
	if (this->isReading)
	{
		return;
	}

	this->convertToMicroTesla = convertToMicroTesla;

	DLLMutex->WaitOne();
#ifdef USE_CALLBACK
	_SetCallback(this->serial, &SensorCallbackFunction, 0);
#endif // USE_CALLBACK
	int error = _DAQStart();
	DLLMutex->ReleaseMutex();
	if (error)
	{
		throw gcnew eMainsException(error, "Can't start data acquisition");
	}

	this->isReading = true;

#ifndef USE_CALLBACK
	Thread^ bgPollingFunction = gcnew Thread(gcnew ThreadStart(this, 
		&eMains::SensorPollingFunction));
	bgPollingFunction->Start();
#endif // USE_CALLBACK
}

void eMains::DAQStop()
{
	if (!this->isReading)
	{
		return;
	}

	this->isReading = false;
	DLLMutex->WaitOne();
	int error = _DAQStop();
	DLLMutex->ReleaseMutex();

	if (error)
	{
		throw gcnew eMainsException(error, "Can't stop data acquisition");
	}
}

void eMains::ShowErrorInformation(int error)
{
	// There is deliberately no lock around this line
	_ShowErrorInformation(error, 0); 
}

void Biomed_eMains_eFMx::eMains::SetUserCalc(bool convert)
{
	DLLMutex->WaitOne();
	int error = _SetUserCalc(this->serial, convert);
	DLLMutex->ReleaseMutex();
	if (!error)
	{
		UserCalc = convert;
	}
	else
	{
		throw gcnew eMainsException(error, "Can't SetUserCalc");
	}
}

bool Biomed_eMains_eFMx::eMains::GetUserCalc()
{
	return UserCalc;
}

String ^ Biomed_eMains_eFMx::eMains::GetType()
{
	return type;
}

char Biomed_eMains_eFMx::eMains::GetRevision()
{
	return Revision;
}

int Biomed_eMains_eFMx::eMains::GetSerial()
{
	return serial;
}


#define DATA_BUFFER_LENGTH 1000
#define POLLING_TIMEOUT_MS 0

void eMains::SensorPollingFunction() {
	DWORD dataCount = 0;
	__int64 systemTicks = 0;

	double *buf = (double*)malloc(DATA_BUFFER_LENGTH * sizeof(double));
	while (this->isReading) {
		DLLMutex->WaitOne();
		int error = _DAQReadData(this->serial, buf, DATA_BUFFER_LENGTH, 
			&dataCount, POLLING_TIMEOUT_MS);
		DLLMutex->ReleaseMutex();
		systemTicks = Stopwatch::GetTimestamp();
		DateTime time = DateTime::Now;

		if (error)
		{
			DAQStop();
			throw gcnew eMainsException(error, 
				"DAQReadData returned error while polling the sensor");
		}
		else
		{
			if (dataCount)
			{
				double systemSeconds = 1.0 * systemTicks / Stopwatch::Frequency;
				array<double>^ dataX = gcnew array<double>(dataCount / 3);
				array<double>^ dataY = gcnew array<double>(dataCount / 3);
				array<double>^ dataZ = gcnew array<double>(dataCount / 3);
				for (DWORD i = 0; i < dataCount / 3; i++)
				{
					dataX[i] = buf[i * 3];
					dataY[i] = buf[i * 3 + 1];
					dataZ[i] = buf[i * 3 + 2];
				}
				ConvertDataUnits(dataX, dataY, dataZ);
				NewDataHandler(dataX, dataY, dataZ, systemSeconds, time);
			}
		}
	}
	free(buf);
}
