#include "stdafx.h"
#include "eMainsException.h"

using namespace System;
using namespace System::Collections;


eMainsException::eMainsException(int error) : Exception() 
{
	Data["error"] = error;
}

eMainsException::eMainsException(String^ msg) : Exception(msg) {}

eMainsException::eMainsException(int error, String^ msg) : Exception(msg) 
{
	Data["error"] = error;
}

eMainsTimeoutException::eMainsTimeoutException(int error) 
	: eMainsException(error) {}

eMainsTimeoutException::eMainsTimeoutException(int error, String^ msg) 
	: eMainsException(error, msg) {}