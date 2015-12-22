#include "stdafx.h"
#include "../Biomed-eMains-eFM-x/Biomed-eMains-eFM-x.h"

using namespace System;
using namespace System::Text;
using namespace System::Collections::Generic;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace Biomed_eMains_eFMx;

namespace BiomedeMainseFMxtest
{
	/* Stub for ReadSlopes eFM-x API.dll function. */
	DWORD __cdecl ReadSlopesStub(DWORD __in serial, double __out *values)
	{
		values[0] = 1.0;
		values[1] = 1.0;
		values[2] = 1.0;
		return 0;
	}

	/* Stub for ReadOffsets eFM-x API.dll function. */
	DWORD __cdecl ReadOffsetsStub(DWORD __in serial, double __out *values)
	{
		values[0] = 0.0;
		values[1] = 0.0;
		values[2] = 0.0;
		return 0;
	}

	[TestClass]
	public ref class UnitTest
	{
	private:
		TestContext^ testContextInstance;

	public: 
		/// <summary>
		///Gets or sets the test context which provides
		///information about and functionality for the current test run.
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
		//
		//You can use the following additional attributes as you write your tests:
		//
		//Use ClassInitialize to run code before running the first test in the class
		[ClassInitialize()]
		static void MyClassInitialize(Microsoft::VisualStudio::TestTools::UnitTesting::TestContext^ testContext) {
			eMains::_ReadSlopes = ReadSlopesStub;
			eMains::_ReadOffsets = ReadOffsetsStub;
		};
		
		//Use ClassCleanup to run code after all tests in a class have run
		//[ClassCleanup()]
		//static void MyClassCleanup() {};
		//
		//Use TestInitialize to run code before running each test
		//[TestInitialize()]
		//void MyTestInitialize() {};
		//
		//Use TestCleanup to run code after each test has run
		//[TestCleanup()]
		//void MyTestCleanup() {};
		//
		#pragma endregion 

		[TestMethod]
		void TestCreateNew()
		{
			eMains^ sensor = gcnew eMains(123);
			Assert::AreEqual<Int32>(sensor->serial, 123);
		};
	};
}
