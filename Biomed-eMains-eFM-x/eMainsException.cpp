#include "stdafx.h"
#include "eMainsException.h"

using namespace System;


eMainsException::eMainsException() : Exception() {}

eMainsException::eMainsException(String^ msg) : Exception(msg) {}

eMainsTimeoutException::eMainsTimeoutException() : eMainsException() {}

eMainsTimeoutException::eMainsTimeoutException(String^ msg) : eMainsException(msg) {}