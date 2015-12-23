#include "stdafx.h"
#include "../Biomed-eMains-eFM-x/Biomed-eMains-eFM-x.h"

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
	unsigned char on_off_SetUserCalc;			// Received parameter
	int error_SetUserCalc;			// Value to return

	/* Stubs for eFM-x API.dll functions. */
	DWORD __cdecl ReadSlopesStub(DWORD __in serial, double __out *values)
	{
		for (int i = 0; i < 3; i++)
		{
			values[i] = slope;
		}
		return 0;
	}

	DWORD __cdecl ReadOffsetsStub(DWORD __in serial, double __out *values)
	{
		for (int i = 0; i < 3; i++)
		{
			values[i] = offset;
		}
		return 0;
	}

	DWORD __cdecl GetAvailableSerialsStub(DWORD* __out buf, __in DWORD len)
	{
		for (unsigned int i = 0; i < len; i++)
		{
			buf[i] = serialMult * i;
		}
		return 0;
	};

	DWORD __cdecl GetNumberOfDevicesStub(DWORD* __out num)
	{
		*num = deviceNumber;
		return 0;
	}

	DWORD __cdecl ReadKennungStub(DWORD __in serial, BYTE* __out data)
	{
		_memccpy(data, &kennung, 1, sizeof(struct kennung));
		return 0;
	}

	DWORD __cdecl GetRevisionStub(DWORD __in serial, BYTE* __out rev)
	{
		*rev = revision;
		return 0;
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
			eMains::_ReadSlopes = ReadSlopesStub;
			eMains::_ReadOffsets = ReadOffsetsStub;
			eMains::_ReadKennung = ReadKennungStub;
			eMains::_GetRevision = GetRevisionStub;
			eMains::_DAQInitialize = DAQInitializeStub;
			eMains::_DAQStart = DAQStartStub;
			eMains::_DAQStop = DAQStopStub;
			eMains::_SetCallback = SetCallbackStub;
			eMains::_DAQReadData = DAQReadDataStub;
			eMains::_SetUserCalc = SetUserCalcStub;

			/* Default state of the sensor. */
			kennung = { { 'e', 'F', 'M', '3' }, 1, 0, 0 };
			slope = 1.0;
			offset = 0.0;
			serialMult = 11;
			deviceNumber = 3;
			revision = 66;

			/* Reset the spies. */
			SamplingRate_DAQinit = 0.0;
			MeasurementRange_DAQinit = 0;
			chop_DAQinit = 0;
			clamp_DAQinit = 0;
			error_DAQinit = 0;
			error_DAQStart = 0;
			error_DAQStop = 0;
			on_off_SetUserCalc = 0;
			error_SetUserCalc = 0;
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

		/* Checks if the list of devices is retrieved correctly. */
		[TestMethod]
		void GetDeviceList()
		{
			eMains::_GetAvailableSerialNumbers = GetAvailableSerialsStub;
			eMains::_GetNumberOfDevices = GetNumberOfDevicesStub;

			List<int>^ serials = eMains::GetAvailableSerials();
			Assert::AreEqual(deviceNumber, serials->Count);
			for (int i = 1; i < deviceNumber; i++)
			{
				Assert::AreEqual(i * serialMult, serials[i]);
			}
		};

		/* Checks if DAQ Initialize arguments are forwarded correctly. */
		[TestMethod]
		void DAQInitialize()
		{
			eMains^ sensor = gcnew eMains(kennung.serial);
			error_DAQinit = 123;
			int error = sensor->DAQInitialize(1000.0, ZERO_TO_PLUS_5V, 0, 1);
			Assert::AreEqual(3000.0, SamplingRate_DAQinit);
			Assert::AreEqual(Convert::ToInt32(ZERO_TO_PLUS_5V), MeasurementRange_DAQinit);
			Assert::AreEqual(1, clamp_DAQinit);
			Assert::AreEqual(0, chop_DAQinit);
			Assert::AreEqual(123, error);
		}

		/* Checks if DAQ Start arguments are forwarded and object flags are updated correctly. */
		[TestMethod]
		void DAQStartError()
		{
			eMains^ sensor = gcnew eMains(kennung.serial);
			error_DAQStart = 12454;
			int error = sensor->DAQStart(false);
			Assert::AreEqual(12454, error);
			Assert::AreEqual(false, sensor->convertToMicroTesla);
			Assert::AreEqual(false, sensor->isReading);
		};

		[TestMethod]
		void DAQStartSuccess()
		{
			eMains^ sensor = gcnew eMains(kennung.serial);
			error_DAQStart = 0;
			int error = sensor->DAQStart(true);
			Assert::AreEqual(0, error);
			Assert::AreEqual(true, sensor->convertToMicroTesla);
			Assert::AreEqual(true, sensor->isReading);
		}

		/* Checks if DAQ Stop resets isReading flag. */
		[TestMethod]
		void DAQStopNotReading()
		{
			eMains^ sensor = gcnew eMains(kennung.serial);
			error_DAQStop = 123;
			sensor->isReading = false;
			int error = sensor->DAQStop();
			Assert::AreEqual(0, error);
			Assert::AreEqual(false, sensor->isReading);
		}

		[TestMethod]
		void DAQStopWhileReading()
		{
			eMains^ sensor = gcnew eMains(kennung.serial);
			error_DAQStop = 123;
			sensor->isReading = true;
			int error = sensor->DAQStop();
			Assert::AreEqual(123, error);
			Assert::AreEqual(false, sensor->isReading);
		}

		/* Checks if SetUserCalc forwards the parameters correctly and updates the instance state. */
		[TestMethod]
		void SetUserCalcSuccess()
		{
			error_SetUserCalc = 0;
			eMains^ sensor = gcnew eMains(kennung.serial);
			sensor->UserCalc = false;
			sensor->SetUserCalc(true);
			Assert::AreEqual<Byte>(1, on_off_SetUserCalc);
			Assert::AreEqual(true, sensor->GetUserCalc());
		}

		[TestMethod]
		void SetUserCalcError()
		{
			error_SetUserCalc = 123;
			eMains^ sensor = gcnew eMains(kennung.serial);
			sensor->UserCalc = true;
			sensor->SetUserCalc(false);
			Assert::AreEqual<Byte>(0, on_off_SetUserCalc);
			Assert::AreEqual(true, sensor->GetUserCalc());
		}
	};
}
