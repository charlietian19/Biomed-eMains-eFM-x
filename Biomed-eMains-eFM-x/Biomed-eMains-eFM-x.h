// Biomed-eMains-eFM-x.h

#pragma once
#include <windows.h>
#define USE_CALLBACK

using namespace System;
using namespace System::Threading;
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

	static struct kennung {
		char deviceType[4];
		__int16 serial;
		char flag1;
		char flag2;
	} kennung;

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


	/* Functions receiving callbacks and wrapping the data into managed arrays. Called every time new data arrives. */
	void __cdecl SensorCallbackFunction(DWORD serial, double *values, DWORD datalength, BYTE packetCounter);

	/* Splits the raw data in the buffer on three channels and puts them into the corresponding managed arrays.
	The data is converted to uT according to the sensor calibration. */
	static void ParseData(double *buf, DWORD dataCount, array<double>^ dataX, array<double>^ dataY, array<double>^ dataZ);

	public ref class eMains
	{
	public:
		/* Returns a eMains object given a serial. */
		eMains(int serial);

		/* Locates and loads eFM-x API.dll. Must be called before any instances can be constructed.
		Should only be called once. */
		static int LoadDLL();

		/* Returns an array of available magnetometer serials, or an empty array if none are available. */
		static List<int>^ GetAvailableSerials();

		/* Invokes a data handler event if one is register. The purpose of this function is to dispatch the
		received data to the corecct object. A native function has to invoke it so it's public. */
		static void InvokeNewDataHandler(array<double>^ dataX, array<double>^ dataY,
			array<double>^ dataZ, double microsecondsSinceLastData, DateTime^ time, int samples);

		/* Describes a user function to be called when the data arrives. */
		delegate void DataProcessingFunc(array<double>^ dataX, array<double>^ dataY,
			array<double>^ dataZ, double systemSeconds, DateTime^ time, int samples);
		
		/* Initializes the DAQ SamplingRate and chop&clamp parameters. */
		int DAQInitialize(double SamplingRate, Range MeasurementRange, int chop, int clamp);

		/* Starts the data acquisition. 
		If compiled with polling option, creates a new thread that will call processingFunction 
		each time non-zero number of samples arrives. If compiled with callback option, registers
		a callback with the DLL that calls processingFunction each time the data arrives.
		convertToMicrotesla - if True, the returned values are in uT, otherwise it's DAC voltage. */
		int DAQStart(bool convertToMicroTesla);

		/* Stops the data acquisition.*/
		int DAQStop();

		/* Displays a message box explaining the error code. This call is NOT thread safe. */
		static void ShowErrorInformation(int error);

		/* Sets whether the device sends the data in DAC voltage units, or in magnetic field units
		(according to the internal calibration data) */
		int SetUserCalc(bool convert);

		/* Returns whether the device sends the data in DAC voltage units, or in magnetic field units	*/
		bool GetUserCalc();

		/* Returns the device type string. */
		String^ GetType();

		/* Returns the device ADC revision. */
		char GetRevision();

		/* Returns the device serial. */
		int GetSerial();

		// These functions and fields will be private and instance in future releases
		// in order to support multiple sensors per program.
		// The reason why they are public and static now is that the DLL callback
		// needs to access them and it was faster to code this way.
		static event DataProcessingFunc^ NewDataHandler;
		static double OffsetX = 0.0;
		static double SlopeX = 0.0;
		static double OffsetY = 0.0;
		static double SlopeY = 0.0;
		static double OffsetZ = 0.0;
		static double SlopeZ = 0.0;
		static bool ConvertToMicroTesla = false;

		/* Expose the fields so the test can stub out the DLL calls. */
#ifdef _DEBUG
	public:
#else
	private:
#endif
		/* Flags retreived by ReagKennung */
		bool flag1, UserCalc;

		/* ADC revision. */
		char Revision;

		/* Sensor type string. */
		String^ type;

		/* Reads out X, Y, Z slopes from the calibration data and saves them in the object. */
		int InitializeSlopes();

		/* Reads out X, Y, Z offsets from the calibration data and saves them in the object. */
		int InitializeOffsets();

		/* Reads out the device information and saves it in the object. */
		int InitializeStatus();

		/* Polls the data from the sensor (backup solution if callbacks are still not ready when
		the delivery is scheduled). Timestamps the data, splits it in three channels and calls
		dataProcessingFunction when the data has arrived. */
		void SensorPollingFunction();

		/* Indicates whether this instance is reading out the data from the sensor. */
		bool isReading = false;

		/* Serial that this instance was created with. */
		DWORD serial;

	/* Function entry points extracted from the device library. */
		/* Lock synchronizing the DLL calls. */
		static Mutex^ DLLMutex = gcnew Mutex();
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
