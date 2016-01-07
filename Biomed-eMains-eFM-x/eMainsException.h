#pragma once

ref class eMainsException : Exception
{
public:
	eMainsException();
	eMainsException(String^ msg);
};

ref class eMainsTimeoutException : eMainsException
{
public:
	eMainsTimeoutException();
	eMainsTimeoutException(String^ msg);
};

