#include "stdafx.h"
#include "..\Biomed-eMains-eFM-x\Biomed-eMains-eFM-x.h"
#include "..\Biomed-eMains-eFM-x\eMainsException.h"

using namespace System;
using namespace System::Threading;
using namespace System::Text;
using namespace System::Collections::Generic;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace Biomed_eMains_eFMx;

namespace BiomedeMainseFMxtest
{
	/* Fake sensor parameters. */
	static double slope;			// Slope calibration value
	static double offset;			// Offset calibration value
	static int serialMult;			// serial = i * serialMult when returning an array of sensors
	static int deviceNumber;		// Total number of sensors found
	char revision;					// ADC revision
	static struct kennung kennung;	// Every sensor returns this kennung structure

	/* DAQInitialize call parameters and return value. */
	double SamplingRate_DAQinit;	// Received sampling rate value
	int MeasurementRange_DAQinit;	// Received range value
	int chop_DAQinit;				// Received chomp value
	int clamp_DAQinit;				// Received clamp value
	int serial_DAQinit;				// Received serial value
	int error_DAQinit;				// Value to return

	/* DAQStart return value. */
	int error_DAQStart;

	/* DAQStop return value. */
	int error_DAQStop;
	
	/* SetUserCalc call parameters and return value. */
	unsigned char on_off_SetUserCalc;	// Received parameter
	int error_SetUserCalc;				// Value to return

	int sensor1_calls;
	int sensor2_calls;
	int sensor3_calls;

	/* GetNumberOfDevices return value. */
	int error_GetNumberOfDevices;

	int error_ReadSlopes;
	int error_ReadOffsets;
	int error_GetAvailableSerials;
	int error_ReadKennung;
	int error_GetRevision;

	/* Stubs for eFM-x API.dll functions. */
	DWORD __cdecl ReadSlopesStub(DWORD __in serial, double __out *values)
	{
		for (int i = 0; i < 3; i++)
		{
			values[i] = slope;
		}
		return error_ReadSlopes;
	}

	DWORD __cdecl ReadOffsetsStub(DWORD __in serial, double __out *values)
	{
		for (int i = 0; i < 3; i++)
		{
			values[i] = offset;
		}
		return error_ReadOffsets;
	}

	DWORD __cdecl GetAvailableSerialsStub(DWORD* __out buf, __in DWORD len)
	{
		for (unsigned int i = 0; i < len; i++)
		{
			buf[i] = serialMult * i;
		}
		return error_GetAvailableSerials;
	};

	DWORD __cdecl GetNumberOfDevicesStub(DWORD* __out num)
	{
		*num = deviceNumber;
		return error_GetNumberOfDevices;
	}

	DWORD __cdecl ReadKennungStub(DWORD __in serial, BYTE* __out data)
	{
		_memccpy(data, &kennung, 1, sizeof(struct kennung));
		return error_ReadKennung;
	}

	DWORD __cdecl GetRevisionStub(DWORD __in serial, BYTE* __out rev)
	{
		*rev = revision;
		return error_GetRevision;
	}

	DWORD __cdecl DAQInitializeStub(DWORD __in serial, double* __in SamplingRate,
		DWORD __in MeasurementRange, DWORD __in chop, DWORD __in clamp)
	{
		serial_DAQinit = serial;
		SamplingRate_DAQinit = *SamplingRate;
		MeasurementRange_DAQinit = MeasurementRange;
		chop_DAQinit = chop;
		clamp_DAQinit = clamp;
		return error_DAQinit;
	}

	DWORD __cdecl DAQStartStub(void)
	{
		return error_DAQStart;
	}

	DWORD __cdecl DAQStopStub(void)
	{
		return error_DAQStop;
	}

	DWORD __cdecl SetCallbackStub(DWORD __in serial, void __in *callback, DWORD __in DataLength)
	{
		return 0;
	}

	DWORD __cdecl DAQReadDataStub(DWORD __in serial, double* __out buf,
		DWORD __in DataBufferLength, DWORD* __out OutputLength, DWORD __in TimeOut)
	{
		if (TimeOut > 0)
		{
			Sleep(TimeOut);
		}
		*OutputLength = 0;
		return 0;
	}

	DWORD __cdecl SetUserCalcStub(DWORD __in serial, unsigned char __in on_off)
	{
		on_off_SetUserCalc = on_off;
		return error_SetUserCalc;
	}

	[TestClass]
	public ref class UnitTest
	{
	private:
		TestContext^ testContextInstance;
		static Mutex^ stubLock; // the DLL stubs are not thread-safe
		static void DataHandlerSensor1(array<double>^ dataX, array<double>^ dataY,
			array<double>^ dataZ, double systemSeconds, DateTime time)
		{
			sensor1_calls += 1;
		}

		static void DataHandlerSensor2(array<double>^ dataX, array<double>^ dataY,
			array<double>^ dataZ, double systemSeconds, DateTime time)
		{
			sensor2_calls += 1;
		}

		static void DataHandlerSensor3(array<double>^ dataX, array<double>^ dataY,
			array<double>^ dataZ, double systemSeconds, DateTime time)
		{
			sensor3_calls += 1;
		}

	public: 
		/// <summary>
		///Tests the functions of the unmanaged code wrapper library
		///</summary>
		property Microsoft::VisualStudio::TestTools::UnitTesting::TestContext^ TestContext
		{
			Microsoft::VisualStudio::TestTools::UnitTesting::TestContext^ get()
			{
				return testContextInstance;
			}
			System::Void set(Microsoft::VisualStudio::TestTools::UnitTesting::TestContext^ value)
			{
				testContextInstance = value;
			}
		};

		#pragma region Additional test attributes
		[ClassInitialize()]
		static void MyClassInitialize(Microsoft::VisualStudio::TestTools::UnitTesting::TestContext^ testContext)
		{
			stubLock = gcnew Mutex();
		}

		[TestInitialize()]
		void MyTestInitialize()
		{
			/* Grab the stub lock*/
			stubLock->WaitOne();

			/* Stub functions for DLL calls. */
			eMains::DebuggingSetupMockDLL(
				(GET_NUMBER_OF_DEVICES)&GetNumberOfDevicesStub, 
				(GET_REVISION)&GetRevisionStub,
				(GET_AVAILABLE_SERIAL_NUMBERS)&GetAvailableSerialsStub,
				(IS_DEVICE_CONNECTED)NULL,
				(READ_KENNUNG)&ReadKennungStub,
				(DAQ_INITIALIZE)&DAQInitializeStub,
				(DAQ_START)&DAQStartStub,
				(DAQ_STOP)&DAQStopStub,
				(DAQ_READDATA)&DAQReadDataStub, 
				(SHOW_ERROR_INFORMATION)NULL,
				(SET_CALLBACK)&SetCallbackStub,
				(SET_USER_CALC)&SetUserCalcStub, 
				(READ_SLOPES)&ReadSlopesStub,
				(READ_OFFSETS)&ReadOffsetsStub);

			/* Default state of the sensor. */
			kennung = { { 'e', 'F', 'M', '3' }, 1, 0, 0 };
			slope = 1.0;
			offset = 0.0;
			serialMult = 11;
			deviceNumber = 3;
			revision = 66;

			/* Reset the "spies". */
			SamplingRate_DAQinit = 0.0;
			MeasurementRange_DAQinit = 0;
			chop_DAQinit = 0;
			clamp_DAQinit = 0;
			error_DAQinit = 0;
			error_DAQStart = 0;
			error_DAQStop = 0;
			on_off_SetUserCalc = 0;
			error_SetUserCalc = 0;
			sensor1_calls = 0;
			sensor2_calls = 0;
			sensor3_calls = 0;
			error_GetNumberOfDevices = 0;
			error_ReadSlopes = 0;
			error_ReadOffsets = 0;
			error_GetAvailableSerials = 0;
			error_ReadKennung = 0;
			error_GetRevision = 0;
		};

		[TestCleanup()]
		void MyTestCleanup()
		{
			/* Release the stub lock. */
			stubLock->ReleaseMutex();
		};

		#pragma endregion 
		/* Checks if the new eMains object is initialized correctly without eFM-x API.dll */
		[TestMethod]
		void CreateNewSensor1()
		{
			kennung = { { 'o', 'L', '0', 'L' }, 34, 1, 1 };
			eMains^ sensor = gcnew eMains(kennung.serial);
			Assert::AreEqual<Int32>(kennung.serial, sensor->GetSerial());
			// Assert::AreEqual(!!kennung.flag2, sensor->GetUserCalc()); // causes race condition/test order issues?
			Assert::AreEqual("oL0L", sensor->GetType());
			Assert::AreEqual(revision, sensor->GetRevision());
		}

		[TestMethod]
		void CreateNewSensor2()
		{
			kennung = { { 'e', 'F', 'M', '3' }, 42, 0, 0 };
			eMains^ sensor = gcnew eMains(kennung.serial);
			Assert::AreEqual<Int32>(kennung.serial, sensor->GetSerial());
			// Assert::AreEqual(!!kennung.flag2, sensor->GetUserCalc()); // causes race condition/test order issues?
			Assert::AreEqual("eFM3", sensor->GetType());
			Assert::AreEqual(revision, sensor->GetRevision());
		};

		/* Checks if the eMains object throws exception on initialization when 
		ReadSlopes fails*/
		[TestMethod]
		[ExpectedException(eMainsException::typeid)]
		void ReadSlopesError()
		{
			error_ReadSlopes = 123;
			eMains^ sensor = gcnew eMains(kennung.serial);
		}

		/* Checks if the eMains object throws exception on initialization when
		ReadOffsets fails*/
		[TestMethod]
		[ExpectedException(eMainsException::typeid)]
		void ReadOffsetsError()
		{
			error_ReadOffsets = 123;
			eMains^ sensor = gcnew eMains(kennung.serial);
		}

		/* Checks if the eMains object throws exception on initialization when
		ReadKennung fails*/
		[TestMethod]
		[ExpectedException(eMainsException::typeid)]
		void ReadKennungError()
		{
			error_ReadKennung = 123;
			eMains^ sensor = gcnew eMains(kennung.serial);
		}

		/* Checks if the eMains object throws exception on initialization when
		ReadRevision fails*/
		[TestMethod]
		[ExpectedException(eMainsException::typeid)]
		void GetRevisionError()
		{
			error_GetRevision = 123;
			eMains^ sensor = gcnew eMains(kennung.serial);
		}

		/* Checks if the list of devices is retrieved correctly. */
		[TestMethod]
		void GetDeviceListSuccess()
		{
			error_GetNumberOfDevices = 0;
			
			List<int>^ serials = eMains::GetAvailableSerials();
			Assert::AreEqual(deviceNumber, serials->Count);
			for (int i = 1; i < deviceNumber; i++)
			{
				Assert::AreEqual(i * serialMult, serials[i]);
			}
		};

		/* Checks if the device initialization throws exception if error happens. */
		[TestMethod]
		[ExpectedException(eMainsException::typeid)]
		void GetDeviceListError()
		{
			error_GetNumberOfDevices = 5234;
			List<int>^ serials = eMains::GetAvailableSerials();
		};

		/* Checks if DAQ Initialize arguments are forwarded correctly. */
		[TestMethod]
		void DAQInitializeSuccess()
		{
			eMains^ sensor = gcnew eMains(kennung.serial);
			error_DAQinit = 0;
			sensor->DAQInitialize(1000.0, Range::ZERO_TO_PLUS_5V, 0, 1);
			Assert::AreEqual(3000.0, SamplingRate_DAQinit);
			Assert::AreEqual(3, MeasurementRange_DAQinit);
			Assert::AreEqual(1, clamp_DAQinit);
			Assert::AreEqual(0, chop_DAQinit);
		}

		/* Checks if DAQ Initialize throws exception. */
		[TestMethod]
		[ExpectedException(eMainsException::typeid)]
		void DAQInitializeError()
		{
			eMains^ sensor = gcnew eMains(kennung.serial);
			error_DAQinit = 123;
			sensor->DAQInitialize(1000.0, Range::ZERO_TO_PLUS_5V, 0, 1);
		}

		/* Checks if DAQ Start arguments are forwarded and object flags are updated correctly. */
		[TestMethod]
		[ExpectedException(eMainsException::typeid)]
		void DAQStartError()
		{
			eMains^ sensor = gcnew eMains(kennung.serial);
			error_DAQStart = 12454;
			sensor->DAQStart(false);
			bool convertToMicroTesla = sensor->DebuggingGetConvertToMicrotesla();
			Assert::AreEqual(false, convertToMicroTesla);
			Assert::AreEqual(false, sensor->Reading);
		};

		[TestMethod]
		void DAQStartSuccess()
		{
			eMains^ sensor = gcnew eMains(kennung.serial);
			error_DAQStart = 0;
			sensor->DAQStart(true);
			bool convertToMicroTesla = sensor->DebuggingGetConvertToMicrotesla();
			Assert::AreEqual(true, convertToMicroTesla);
			Assert::AreEqual(true, sensor->Reading);
		}

		/* Checks if DAQ Stop resets isReading flag. */
		[TestMethod]
		void DAQStopNotReading()
		{
			eMains^ sensor = gcnew eMains(kennung.serial);
			error_DAQStop = 123; // DLL function should NOT be called
			sensor->DebuggingSetIsReading(false);
			sensor->DAQStop();
			Assert::AreEqual(false, sensor->Reading);
		}

		/* Checks if DAQ Stop resets isReading flag. */
		[TestMethod]
		void DAQStopWhileReading()
		{
			eMains^ sensor = gcnew eMains(kennung.serial);
			error_DAQStop = 0;
			sensor->DebuggingSetIsReading(true);
			sensor->DAQStop();
			Assert::AreEqual(false, sensor->Reading);
		}

		/* Checks if DAQ Stop throws exception on error. */
		[TestMethod]
		[ExpectedException(eMainsException::typeid)]
		void DAQStopError()
		{
			eMains^ sensor = gcnew eMains(kennung.serial);
			error_DAQStop = 1234;
			sensor->DebuggingSetIsReading(true);
			sensor->DAQStop();
			Assert::AreEqual(false, sensor->Reading);
		}

		/* Checks if SetUserCalc forwards the parameters correctly and 
		updates the instance state. */
		[TestMethod]
		void SetUserCalcSuccess()
		{
			error_SetUserCalc = 0;
			eMains^ sensor = gcnew eMains(kennung.serial);
			sensor->DebuggingSetUserCalc(false);
			sensor->SetUserCalc(true);
			Assert::AreEqual<Byte>(1, on_off_SetUserCalc);
			Assert::AreEqual(true, sensor->GetUserCalc());
		}

		/* Checks if SetUserCalc throws the exception on error. */
		[TestMethod]
		[ExpectedException(eMainsException::typeid)]
		void SetUserCalcError()
		{
			error_SetUserCalc = 123;
			eMains^ sensor = gcnew eMains(kennung.serial);
			sensor->DebuggingSetUserCalc(true);
			sensor->SetUserCalc(false);
		}

#ifdef USE_CALLBACK
		/* Checks if the data reaches the correct sensor instance. */
		[TestMethod]
		void DispatchDataSimple()
		{
			eMains^ sensor1 = gcnew eMains(1);
			eMains^ sensor2 = gcnew eMains(2);
			array<double>^ data = gcnew array<double>(3);
			sensor1->NewDataHandler += gcnew eMains::DataProcessingFunc(&DataHandlerSensor1);
			sensor2->NewDataHandler += gcnew eMains::DataProcessingFunc(&DataHandlerSensor2);
			Assert::AreEqual(0, sensor1_calls);
			Assert::AreEqual(0, sensor2_calls);
			eMains::InvokeDataHandler(1, data, data, data, 0, System::DateTime::Now, 0);
			Assert::AreEqual(1, sensor1_calls);
			Assert::AreEqual(0, sensor2_calls);
			eMains::InvokeDataHandler(1, data, data, data, 0, System::DateTime::Now, 0);
			Assert::AreEqual(2, sensor1_calls);
			Assert::AreEqual(0, sensor2_calls);
			eMains::InvokeDataHandler(2, data, data, data, 0, System::DateTime::Now, 0);
			Assert::AreEqual(2, sensor1_calls);
			Assert::AreEqual(1, sensor2_calls);
		}
#endif

	};
}
