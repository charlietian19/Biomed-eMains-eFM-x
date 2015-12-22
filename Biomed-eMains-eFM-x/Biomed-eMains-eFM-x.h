// Biomed-eMains-eFM-x.h

#pragma once
#include <windows.h>
#define USE_CALLBACK

using namespace System;
using namespace System::Collections::Generic;
using namespace System;

namespace Biomed_eMains_eFMx {
	/* Selectable DAC range constants. */
	enum Range
	{
		NEG_10_TO_PLUS_10V = 0,
		ZERO_TO_PLUS_10V = 1,
		NEG_5_TO_PLUS_5V = 2,
		ZERO_TO_PLUS_5V = 3
	};

	/* Native DLL functions imported from eFM-x API.dll. */
	typedef DWORD(__cdecl* GET_NUMBER_OF_DEVICES)(DWORD* __out num);
	typedef DWORD(__cdecl* GET_REVISION)(DWORD __in serial, BYTE* __out rev);
	typedef DWORD(__cdecl* GET_AVAILABLE_SERIAL_NUMBERS)(DWORD* __out buf, __in DWORD len);
	typedef DWORD(__cdecl* IS_DEVICE_CONNECTED)(DWORD __in serial, bool* __out rev);
	typedef DWORD(__cdecl* READ_KENNUNG)(DWORD __in serial, BYTE* __out data);
	typedef DWORD(__cdecl* DAQ_INITIALIZE)(DWORD __in serial, double* __in SamplingRate,
		DWORD __in MeasurementRange, DWORD __in chop, DWORD __in clamp);
	typedef DWORD(__cdecl* DAQ_START)(void);
	typedef DWORD(__cdecl* DAQ_STOP)(void);
	typedef DWORD(__cdecl* DAQ_READDATA)(DWORD __in serial, double* __out buf,
		DWORD __in DataBufferLength, DWORD* __out OutputLength, DWORD __in TimeOut);
	typedef DWORD(__cdecl* SHOW_ERROR_INFORMATION)(DWORD __in errorCode, bool __in extended);
	typedef DWORD(__cdecl* SET_CALLBACK)(DWORD __in serial, void __in *callback, DWORD __in DataLength);
	typedef DWORD(__cdecl* SET_USER_CALC)(DWORD __in serial, unsigned char __in on_off);
	typedef DWORD(__cdecl* READ_SLOPES)(DWORD __in serial, double __out *values);
	typedef DWORD(__cdecl* READ_OFFSETS)(DWORD __in serial, double __out *values);


	/* Functions receiving callbacks and wrapping the data into managed arrays. */
	void __cdecl SensorCallbackFunction(double *values, DWORD datalength, BYTE packetCounter);
	static void parseData(double *buf, DWORD dataCount, array<double>^ dataX, array<double>^ dataY, array<double>^ dataZ);

	public ref class eMains
	{
	public:
		delegate void DataProcessingFunc(array<double>^ dataX, array<double>^ dataY,
			array<double>^ dataZ, double systemSeconds, DateTime^ time, int samples);
		eMains(int serial);
		static int LoadDLL();
		static List<int>^ GetAvailableSerials();
		int DAQInitialize(double SamplingRate, Range MeasurementRange, int chop, int clamp);
		int DAQStart(bool convertToMicroTesla);
		int DAQStop();
		static void ShowErrorInformation(int error);


		static void InvokeNewDataHandler(array<double>^ dataX, array<double>^ dataY,
			array<double>^ dataZ, double microsecondsSinceLastData, DateTime^ time, int samples);

		// These functions and fields will be private and instance in future releases
		// in order to support multiple sensors per program.
		// The reason why they are public and static now is that the DLL callback
		// needs to access them and it was faster to code this way.
		static event DataProcessingFunc^ NewDataHandler;
		static double offsetX = 0.0;
		static double slopeX = 0.0;
		static double offsetY = 0.0;
		static double slopeY = 0.0;
		static double offsetZ = 0.0;
		static double slopeZ = 0.0;
		static bool convertToMicroTesla = false;


		/* Expose the fields so the test can stub out the DLL calls. */
#ifdef _DEBUG
	public:
#else
	private:
#endif
		void SensorPollingFunction();
		bool isReading = false;
		DWORD serial;

		static HINSTANCE eMainsDLL = NULL;
		static GET_NUMBER_OF_DEVICES _GetNumberOfDevices = NULL;
		static GET_REVISION _GetRevision = NULL;
		static GET_AVAILABLE_SERIAL_NUMBERS _GetAvailableSerialNumbers = NULL;
		static IS_DEVICE_CONNECTED _IsDeviceConnected = NULL;
		static READ_KENNUNG _ReadKennung = NULL;
		static DAQ_INITIALIZE _DAQInitialize = NULL;
		static DAQ_START _DAQStart = NULL;
		static DAQ_STOP _DAQStop = NULL;
		static DAQ_READDATA _DAQReadData = NULL;
		static SHOW_ERROR_INFORMATION _ShowErrorInformation = NULL;
		static SET_CALLBACK _SetCallback = NULL;
		static SET_USER_CALC _SetUserCalc = NULL;
		static READ_SLOPES _ReadSlopes = NULL;
		static READ_OFFSETS _ReadOffsets = NULL;
	};
}
