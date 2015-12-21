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
void __cdecl Biomed_eMains_eFMx::SensorCallbackFunction(double *values, DWORD datalength, BYTE packetCounter) {
	static __int64 systemTicks = 0;

	systemTicks = Stopwatch::GetTimestamp();
	DateTime time = DateTime::Now;

	if (datalength)
	{
		double systemSeconds = 1.0 * systemTicks / Stopwatch::Frequency;
		array<double>^ dataX = gcnew array<double>(datalength / 3);
		array<double>^ dataY = gcnew array<double>(datalength / 3);
		array<double>^ dataZ = gcnew array<double>(datalength / 3);
		Biomed_eMains_eFMx::parseData(values, datalength, dataX, dataY, dataZ);
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
static void Biomed_eMains_eFMx::parseData(double *buf, DWORD dataCount, array<double>^ dataX,
	array<double>^ dataY, array<double>^ dataZ)
{
	if (eMains::convertToMicroTesla) {
		for (DWORD i = 0; i < dataCount / 3; i++)
		{
			dataX[i] = buf[i * 3] * eMains::slopeX + eMains::offsetX;
			dataY[i] = buf[i * 3 + 1] * eMains::slopeY + eMains::offsetY;
			dataZ[i] = buf[i * 3 + 2] * eMains::slopeZ + eMains::offsetZ;
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
	double values[3];
	this->serial = serial;

	_ReadSlopes(serial, values);
	slopeX = values[0];
	slopeY = values[1];
	slopeZ = values[2];

	_ReadOffsets(serial, values);
	offsetX = values[0];
	offsetY = values[1];
	offsetZ = values[2];
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

	this->convertToMicroTesla = convertToMicroTesla;

#ifdef USE_CALLBACK
	_SetCallback(this->serial, &SensorCallbackFunction, 0);
#endif // USE_CALLBACK

	int res = _DAQStart();
	if (res)
	{
		return res;
	}

	//res = _SetUserCalc(this->serial, 0);
	//if (res)
	//{
	//	return res;
	//}

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
				parseData(buf, dataCount, dataX, dataY, dataZ);
				NewDataHandler(dataX, dataY, dataZ, systemSeconds, time, dataCount);
			}
		}
	}
	free(buf);
}
