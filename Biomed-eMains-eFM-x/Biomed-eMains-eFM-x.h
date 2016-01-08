// Biomed-eMains-eFM-x.h

#pragma once
#include "stdafx.h"
#include <windows.h>
#define USE_CALLBACK

using namespace System;
using namespace System::Threading;
using namespace System::Collections::Generic;
using namespace System;

namespace Biomed_eMains_eFMx {
	/* Selectable DAC range constants. */
	public enum class Range
	{
		NEG_10_TO_PLUS_10V,
		ZERO_TO_PLUS_10V,
		NEG_5_TO_PLUS_5V,
		ZERO_TO_PLUS_5V
	};

	/* Struct filled out by ReadKenung. */
	public struct kennung {
		char deviceType[4];
		__int16 serial;
		char flag1;
		char flag2;
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


	/* Functions receiving callbacks and wrapping the data into managed arrays. Called every time new data arrives. */
	void __cdecl SensorCallbackFunction(DWORD serial, double *values, DWORD datalength, BYTE packetCounter);

	public ref class eMains
	{
	public:
		/* Returns a eMains object given a serial. */
		eMains(int serial);

		/* Locates and loads eFM-x API.dll. Must be called before any instances can be constructed.
		Should only be called once. */
		static void LoadDLL();

		/* Returns an array of available magnetometer serials, or an empty array if none are available. */
		static List<int>^ GetAvailableSerials();

		/* Invokes a data handler event if one is register. The purpose of this function is to dispatch the
		received data to the corecct object. A native function has to invoke it so it's public. */
		static void InvokeDataHandler(DWORD serial, array<double>^ dataX, array<double>^ dataY,
			array<double>^ dataZ, double microsecondsSinceLastData, DateTime time, int samples);

		/* Describes a user function to be called when the data arrives. */
		delegate void DataProcessingFunc(array<double>^ dataX, array<double>^ dataY,
			array<double>^ dataZ, double systemSeconds, DateTime time, int samples);
		
		/* Initializes the DAQ SamplingRate and chop&clamp parameters. */
		void DAQInitialize(double SamplingRate, Range MeasurementRange, int chop, int clamp);

		/* Starts the data acquisition. 
		If compiled with polling option, creates a new thread that will call processingFunction 
		each time non-zero number of samples arrives. If compiled with callback option, registers
		a callback with the DLL that calls processingFunction each time the data arrives.
		convertToMicrotesla - if True, the returned values are in uT, otherwise it's DAC voltage. */
		void DAQStart(bool convertToMicroTesla);

		/* Stops the data acquisition.*/
		void DAQStop();

		/* Displays a message box explaining the error code. This call is NOT thread safe. */
		static void ShowErrorInformation(int error);

		/* Sets whether the device sends the data in DAC voltage units, or in magnetic field units
		(according to the internal calibration data) */
		void SetUserCalc(bool convert);

		/* Returns whether the device sends the data in DAC voltage units, or in magnetic field units	*/
		bool GetUserCalc();

		/* Returns the device type string. */
		String^ GetType();

		/* Returns the device ADC revision. */
		char GetRevision();

		/* Returns the device serial. */
		int GetSerial();

		/* Event handler for the arriving data. */
		event DataProcessingFunc^ NewDataHandler;

		/* Functions to facilitate unit testing. */
		/* Setup mock DLL entry points. */
		static void DebuggingSetupMockDLL(GET_NUMBER_OF_DEVICES GetNumberOfDevices,
			GET_REVISION GetRevision, GET_AVAILABLE_SERIAL_NUMBERS GetAvailableSerialNumber,
			IS_DEVICE_CONNECTED IsDeviceConnected, READ_KENNUNG ReadKennung,
			DAQ_INITIALIZE DAQInitialize, DAQ_START DAQStart, DAQ_STOP DAQStop,
			DAQ_READDATA DAQReadData, SHOW_ERROR_INFORMATION ShowErrorInformation,
			SET_CALLBACK SetCallback, SET_USER_CALC SetUserCalc, READ_SLOPES ReadSlopes,
			READ_OFFSETS ReadOffsets);

		/* Sets convertToMicrotesla flag. This method is for unit testing only. */
		void DebuggingSetConvertToMicrotesla(bool flag);
		
		/* Sets isReading flag. This method is for unit testing only. */
		void DebuggingSetIsReading(bool flag);

		/* Sets UserCalc flag. This method is for unit testing only. */
		void DebuggingSetUserCalc(bool flag);

		/* Returns isReading flag. This method is for unit testing only. */
		bool DebuggingGetIsReading();

		/* Returns convertToMicrotesla flag. This method is for unit testing only. */
		bool DebuggingGetConvertToMicrotesla();

	private:
		/* Converts Range enum to an int. */
		int RangeToInt(Range range);

		/* Keeps tracks of the active eMains objects to dispatch the data to them. */
		static Dictionary<DWORD, eMains^>^ activeSensors = gcnew Dictionary<DWORD, eMains^>();

		/* Sensor calibration data. */
		double OffsetX = 0.0;	// Bx[uT] = OffsetX + SlopeX * x
		double SlopeX = 0.0;	// Bx[uT] = OffsetX + SlopeX * x
		
		double OffsetY = 0.0;	// By[uT] = OffsetY + SlopeY * y
		double SlopeY = 0.0;	// By[uT] = OffsetY + SlopeY * y
		
		double OffsetZ = 0.0;	// Bz[uT] = OffsetZ + SlopeZ * z
		double SlopeZ = 0.0;	// Bz[uT] = OffsetZ + SlopeZ * z
		
		/* If true, the magnetic field is in uT, otherwise it's DAC voltage. */
		bool convertToMicroTesla = true;

		/* Flags retreived by ReagKennung */
		bool flag1, UserCalc;

		/* ADC revision. */
		char Revision;

		/* Sensor type string. */
		String^ type;

		/* Reads out X, Y, Z slopes from the calibration data and saves them in the object. */
		void InitializeSlopes();

		/* Reads out X, Y, Z offsets from the calibration data and saves them in the object. */
		void InitializeOffsets();

		/* Reads out the device information and saves it in the object. */
		void InitializeStatus();

		/* Polls the data from the sensor (backup solution if callbacks are still not ready when
		the delivery is scheduled). Timestamps the data, splits it in three channels and calls
		dataProcessingFunction when the data has arrived. */
		void SensorPollingFunction();

		/* Indicates whether this instance is reading out the data from the sensor. */
		bool isReading = false;

		/* Serial that this instance was created with. */
		DWORD serial;

		/* Scales the raw data according to the instance settings. */
		void ConvertDataUnits(array<double>^ dataX, array<double>^ dataY, array<double>^ dataZ);

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
