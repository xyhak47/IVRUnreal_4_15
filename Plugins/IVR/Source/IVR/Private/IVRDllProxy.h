#pragma once

#include "UnrealString.h"

class IVRDllProxy
{
protected:
	static void FreeDependency(void*& Handle);
	static bool LoadDependency(const FString& Dir, const FString& Name, void*& Handle);

private:
	static void* DllHandle;
	static void* DllMyServerManagerHandle;

public:
	static bool LoadDll();
	static bool LoadDllServerManager();
	static bool UnLoadDll();

public:
	typedef int(*IVR_Initialize)();
	typedef int(*IVR_Destroy)();

	typedef bool(*IVR_GetSensorOrientation)(int sensorID, float& w, float& x, float& y, float& z);

	typedef bool(*IVR_GetAcceleration)(int sensorID, float& w, float& x, float& y, float& z);
	typedef bool(*IVR_GetAngularVelocity)(int sensorID, float& w, float& x, float& y, float& z);
	typedef bool(*IVR_GetMagnetometer)(int sensorID, float& w, float& x, float& y, float& z);

	typedef char* (*IVR_GetDisplayDeviceName)();


	static IVR_Initialize							Initialize;
	static IVR_Destroy								Destroy;
	static IVR_GetSensorOrientation					GetSensorOrientation;
	static IVR_GetDisplayDeviceName					GetDisplayDeviceName;

	typedef void(*IVR_GameConnectToServer)();
	typedef void(*IVR_StartSuccess)();
	typedef void(*IVR_EndGame)();

	static IVR_GameConnectToServer GameConnectToServer;
	static IVR_StartSuccess StartSuccess;
	static IVR_EndGame EndGame;
};

