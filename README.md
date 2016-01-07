# Biomed-eMains-eFM-x
This project is a wrapper DLL for unmanaged code in eFM-x API.dll (Biomed fluxgate magnetometer). Provides managed eMains class for use in C# projects. 
The class implements minimal number of functions needed for the data acqisition, and synchronizes eFM-x API.dll function calls for thread-safety (at least as long as there is only one process using it).

New eMains objects are instantiated given a _serial_, and each object represents a device connected to the computer. The method names and signatures are mostly the same as in the user manual coming with the device. Functions that take _serial_ as a parameter become instance methods. See Biomed-eMains-eFM-x.h for class interface details.

The data can be retrieved with polling (the default eFM-x API.dll method), or using the new callback interface, that requires a compatible version of eFM-x API.dll. The data retrieved in a callback routine is retrieved via an event. The data is timestamped upon arrival with both system date/time (~15 ms resolution) and system high resolution timer (<1us resolution).

The example code making use of the library can be found here: https://github.com/lenazh/UrbanMagnetometer
