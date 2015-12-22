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
	static double slope;
	static double offset;
	static int serialMult;
	static int deviceNumber;
	char revision;
	static struct kennung kennung;

	/* Where DAQInitialize should store its call parameters. */
	double SamplingRate_DAQinit;
	int MeasurementRange_DAQinit;
	int chop_DAQinit;
	int clamp_DAQinit;
	int error_DAQinit;
	int serial_DAQinit;

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

			/* Stub functions called when a class object is instantiated. */
			eMains::_ReadSlopes = ReadSlopesStub;
			eMains::_ReadOffsets = ReadOffsetsStub;
			eMains::_ReadKennung = ReadKennungStub;
			eMains::_GetRevision = GetRevisionStub;
			eMains::_DAQInitialize = DAQInitializeStub;

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

		};

		[TestCleanup()]
		void MyTestCleanup()
		{
			/* Release the stub lock. */
			stubLock->ReleaseMutex();
		};

		#pragma endregion 
		[TestMethod]
		/* Checks if the new eMains object is initialized correctly without eFM-x API.dll */
		void CreateNewSensor1()
		{
			kennung = { { 'o', 'L', '0', 'L' }, 34, 1, 1 };
			eMains^ sensor = gcnew eMains(kennung.serial);
			Assert::AreEqual<Int32>(kennung.serial, sensor->GetSerial());
			Assert::AreEqual(!!kennung.flag2, sensor->GetUserCalc()); // causes race condition/test order issues?
			Assert::AreEqual("oL0L", sensor->GetType());
			Assert::AreEqual(revision, sensor->GetRevision());
		}

		[TestMethod]
		void CreateNewSensor2()
		{
			kennung = { { 'e', 'F', 'M', '3' }, 42, 0, 0 };
			eMains^ sensor = gcnew eMains(kennung.serial);
			Assert::AreEqual<Int32>(kennung.serial, sensor->GetSerial());
			Assert::AreEqual(!!kennung.flag2, sensor->GetUserCalc()); // causes race condition/test order issues?
			Assert::AreEqual("eFM3", sensor->GetType());
			Assert::AreEqual(revision, sensor->GetRevision());
		};


		[TestMethod]
		/* Checks if the list of devices is retrieved correctly. */
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

		[TestMethod]
		/* Checks if DAQ Initialize arguments are forwarded correctly. */
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

	};
}
