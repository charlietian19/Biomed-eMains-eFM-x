#pragma once
#include "stdafx.h"

using namespace System;

namespace Biomed_eMains_eFMx {
	public ref class eMainsException : public Exception
	{
	public:
		eMainsException(int error) : Exception()
		{
			Data["error"] = error;
		}

		eMainsException(int error, String^ msg) : Exception(msg)
		{
			Data["error"] = error;
		}

		eMainsException(String^ msg) : Exception(msg) {};

	};

	public ref class eMainsTimeoutException : public eMainsException
	{
	public:
		eMainsTimeoutException(int error) : eMainsException(error) {}
		eMainsTimeoutException(int error, String^ msg) : eMainsException(error, msg) {};
	};

}