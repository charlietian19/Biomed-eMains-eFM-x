#pragma once

ref class eMainsException : Exception
{
public:
	eMainsException(int error);
	eMainsException(int error, String^ msg);
	eMainsException(String^ msg);
};

ref class eMainsTimeoutException : eMainsException
{
public:
	eMainsTimeoutException(int error);
	eMainsTimeoutException(int error, String^ msg);
};

