#include "stdafx.h"
#include "../Biomed-eMains-eFM-x/Biomed-eMains-eFM-x.h"

using namespace System;
using namespace System::Text;
using namespace System::Collections::Generic;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace Biomed_eMains_eFMx;

namespace BiomedeMainseFMxtest
{
	static double slope = 1.0;
	static double offset = 0.0;
	static int serialMult = 11;
	static int deviceNumber = 3;

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


	[TestClass]
	public ref class UnitTest
	{
	private:
		TestContext^ testContextInstance;

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
		static void MyClassInitialize(Microsoft::VisualStudio::TestTools::UnitTesting::TestContext^ testContext) {
			eMains::_ReadSlopes = ReadSlopesStub;
			eMains::_ReadOffsets = ReadOffsetsStub;
		};
		
		#pragma endregion 

		[TestMethod]
		/* Checks if the new eMains object is created correctly without eFM-x API.dll */
		void CreateNewSensor()
		{
			eMains^ sensor = gcnew eMains(123);
			Assert::AreEqual<Int32>(123, sensor->serial);
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
	};
}
