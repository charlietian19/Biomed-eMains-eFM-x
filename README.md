# Biomed-eMains-eFM-x
Wrapper library for unmanaged code in eFM-x API.dll (Biomed fluxgate magnetometer). Provides managed eMains class for use in C# projects. 
The classs implements minimal number offunctions needed to acquire the data.

New eMains objects are instantiated given a _serial_, and each object represents a device connected to the computer. The method names and signatures are mostly the same as in the manual coming with the device. Functions that take _serial_ as a parameter become instance methods. See Biomed-eMains-eFM-x.h for the class interface details.

The data can be retrieved with polling (the default eFM-x API.dll method), or using the new callback interface, that requires a compatible version of eFM-x API.dll. The data is timestamped upon arrival with both system date/time (~15 ms resolution) and system high resolution timer (<1us resolution).
