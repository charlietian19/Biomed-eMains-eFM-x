// This is the main DLL file.

#include "stdafx.h"
#include "Biomed-eMains-eFM-x.h"
#include <tchar.h>

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading;
using namespace System::Diagnostics;
using namespace Biomed_eMains_eFMx;

/* Non-managed callback for the DLL. Called every time new data arrives. */
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
		Biomed_eMains_eFMx::ParseData(values, datalength, dataX, dataY, dataZ);
		eMains::InvokeNewDataHandler(dataX, dataY, dataZ, systemSeconds, time, datalength);
	}
}

/**
 * Invokes a data handler event if one is register. The sole purpose of this function is
 * to let the non-managed callback to call a managed event handler.
 */
void eMains::InvokeNewDataHandler(array<double>^ dataX, array<double>^ dataY,
	array<double>^ dataZ, double microsecondsSinceLastData, DateTime ^ time, int samples)
{
	NewDataHandler(dataX, dataY, dataZ, microsecondsSinceLastData, time, samples);
}

/**
	Splits the raw data in the buffer on three channels and puts them into the
	corresponding managed arrays. The data is converted to uT according to
	the sensor calibration.
*/
static void Biomed_eMains_eFMx::ParseData(double *buf, DWORD dataCount, array<double>^ dataX,
	array<double>^ dataY, array<double>^ dataZ)
{
	if (eMains::ConvertToMicroTesla) {
		for (DWORD i = 0; i < dataCount / 3; i++)
		{
			dataX[i] = buf[i * 3] * eMains::SlopeX + eMains::OffsetX;
			dataY[i] = buf[i * 3 + 1] * eMains::SlopeY + eMains::OffsetY;
			dataZ[i] = buf[i * 3 + 2] * eMains::SlopeZ + eMains::OffsetZ;
		}
	}
	else
	{
		for (DWORD i = 0; i < dataCount / 3; i++)
		{
			dataX[i] = buf[i * 3];
			dataY[i] = buf[i * 3 + 1];
			dataZ[i] = buf[i * 3 + 2];
		}
	}

}

/* eMains sensor object constructor. */
eMains::eMains(int serial)
{
	this->serial = serial;

	InitializeSlopes();
	InitializeOffsets();
	InitializeStatus();
}

/* Reads out X, Y, Z slopes from the calibration data and saves them in the object. */
int Biomed_eMains_eFMx::eMains::InitializeSlopes()
{
	double values[3];
	int error = _ReadSlopes(serial, values);
	if (!error)
	{
		SlopeX = values[0];
		SlopeY = values[1];
		SlopeZ = values[2];
	}
	return error;
}

/* Reads out X, Y, Z offsets from the calibration data and saves them in the object. */
int Biomed_eMains_eFMx::eMains::InitializeOffsets()
{
	double values[3];
	int error = _ReadOffsets(serial, values);
	if (!error)
	{
		OffsetX = values[0];
		OffsetY = values[1];
		OffsetZ = values[2];
	}
	return error;
}

/* Reads out the device information and saves it in the object. */
int Biomed_eMains_eFMx::eMains::InitializeStatus()
{
	struct kennung kennung;
	int error = _ReadKennung(serial, (BYTE *)&kennung);
	if (!error)
	{
		flag1 = !!kennung.flag1;
		UserCalc = !!kennung.flag2;
		type = gcnew String(kennung.deviceType, 0, 4);
	}
	else
	{
		return error;
	}

	BYTE revision;
	error = _GetRevision(serial, &revision);
	if (!error)
	{
		Revision = revision;
	}
	return error;
}

/* Locates and loads eFM-x API.dll */
int eMains::LoadDLL()
{
	eMainsDLL = LoadLibrary(_T("eFM-x API.dll"));
	if (!eMainsDLL) {
		return -1;
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

	if (!(_GetNumberOfDevices && _GetRevision && _GetAvailableSerialNumbers && _IsDeviceConnected &&
		_ReadKennung && _DAQInitialize && _DAQStart && _DAQStop && _DAQReadData &&
		_ShowErrorInformation && _ReadSlopes && _ReadOffsets && _SetUserCalc))
	{
		return -1;
	}

#ifdef USE_CALLBACK
	_SetCallback = (SET_CALLBACK)GetProcAddress(eMainsDLL, "SetCallBack");
	if (!_SetCallback)
	{
		return -1;
	}
#endif // USE_CALLBACK

	return 0;
}

/**
* Returns an array of available magnetometer serials.
* Returns an empty array if none are available.
*/
List<int>^ eMains::GetAvailableSerials()
{
	DWORD numberOfDevices = 0;
	array<Int32>^ serials = gcnew array<Int32>(0);
	if (_GetNumberOfDevices(&numberOfDevices))
	{
		return gcnew List<int>();
	}

	DWORD *buf = (DWORD *)malloc(numberOfDevices * sizeof(DWORD));
	if (!_GetAvailableSerialNumbers(buf, numberOfDevices) && (numberOfDevices != 0)) {
		serials = gcnew array<Int32>(numberOfDevices);
		Runtime::InteropServices::Marshal::Copy(IntPtr((void *)buf), serials, 0, numberOfDevices);
	}

	free(buf);
	return gcnew List<int>(serials);
}

/**
* Initializes the DAQ SamplingRate and other things
*/
int eMains::DAQInitialize(double samplingRate, Range measurementRange, int chop, int clamp)
{
	samplingRate *= 3;
	return _DAQInitialize(this->serial, &samplingRate, measurementRange, chop, clamp);
}


/**
 * Starts the data acquisition.
 * If polling method is used, dreates a new thread that will call processingFunction
 * each time non-zero number of samples arrives.
 * If callback method is used, registers a callback with the DLL that calls
 * processingFunction each time the data arrives.
 */

int eMains::DAQStart(bool convertToMicroTesla)
{
	if (this->isReading)
		return 0;

	this->ConvertToMicroTesla = convertToMicroTesla;

#ifdef USE_CALLBACK
	_SetCallback(this->serial, &SensorCallbackFunction, 0);
#endif // USE_CALLBACK

	int res = _DAQStart();
	if (res)
	{
		return res;
	}

	this->isReading = true;

#ifndef USE_CALLBACK
	Thread^ bgPollingFunction = gcnew Thread(gcnew ThreadStart(this, &eMains::SensorPollingFunction));
	bgPollingFunction->Start();
#endif // USE_CALLBACK

	return res;
}


/**
* Stops the data acquisition.
*/
int eMains::DAQStop()
{
	if (!this->isReading)
		return 0;

	this->isReading = false;
	return _DAQStop();
}

/* Converts the error code to a string. */
void eMains::ShowErrorInformation(int error)
{
	_ShowErrorInformation(error, 0);
}

/**
 * Sets whether the device sends the data in DAC voltage units, or in magnetic field units,
 * according to the internal calibration data
 */
int Biomed_eMains_eFMx::eMains::SetUserCalc(bool convert)
{
	int error = _SetUserCalc(this->serial, convert);
	if (!error)
	{
		UserCalc = convert;
	}
	return error;
}

/**
* Sets whether the device sends the data in DAC voltage units, or in magnetic field units,
* according to the internal calibration data
*/
bool Biomed_eMains_eFMx::eMains::GetUserCalc()
{
	return UserCalc;
}

/* Returns the device type as a String. */
String ^ Biomed_eMains_eFMx::eMains::GetType()
{
	return type;
}

/* Returns ADC revision. */
char Biomed_eMains_eFMx::eMains::GetRevision()
{
	return Revision;
}

/* Returns serial that this object was created with. */
int Biomed_eMains_eFMx::eMains::GetSerial()
{
	return serial;
}


#define DATA_BUFFER_LENGTH 1000
#define POLLING_TIMEOUT_MS 0
/**
* Polls the data from the sensor (backup solution if callbacks are still not ready when
* the delivery is scheduled). Timestamps the data, splits it in three channels and calls
* dataProcessingFunction when the data has arrived.
*/
void eMains::SensorPollingFunction() {
	DWORD dataCount = 0;
	__int64 systemTicks = 0;

	double *buf = (double*)malloc(DATA_BUFFER_LENGTH * sizeof(double));
	while (this->isReading) {
		int error = _DAQReadData(this->serial, buf, DATA_BUFFER_LENGTH, &dataCount, POLLING_TIMEOUT_MS);
		systemTicks = Stopwatch::GetTimestamp();
		DateTime time = DateTime::Now;

		if (error)
		{
			DAQStop();
		}
		else
		{
			if (dataCount)
			{
				double systemSeconds = 1.0 * systemTicks / Stopwatch::Frequency;
				array<double>^ dataX = gcnew array<double>(dataCount / 3);
				array<double>^ dataY = gcnew array<double>(dataCount / 3);
				array<double>^ dataZ = gcnew array<double>(dataCount / 3);
				ParseData(buf, dataCount, dataX, dataY, dataZ);
				NewDataHandler(dataX, dataY, dataZ, systemSeconds, time, dataCount);
			}
		}
	}
	free(buf);
}
