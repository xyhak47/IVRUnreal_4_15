

#include "IVRHMDPCH.h"
#include "IPluginManager.h"
#include "IVRDllProxy.h"

//using namespace MyServerManager;

#define IVR_IMPORT(Name, Func) \
	Func = (IVR_##Func)FPlatformProcess::GetDllExport(DllHandle, TEXT(Name)); \
	if (Func == nullptr) \
	{ \
		UE_LOG(LogIVR, Warning, TEXT("Failed to import IVR function %s"), TEXT(Name)); \
		return false; \
	}\
	else\
	{\
		UE_LOG(LogIVR, Warning, TEXT("succeed to import IVR function %s"), TEXT(Name)); \
	}\




void* IVRDllProxy::DllHandle = nullptr;
void* IVRDllProxy::DllMyServerManagerHandle = nullptr;



#define IVR_DEFINE(Func) \
	IVRDllProxy::IVR_##Func IVRDllProxy::Func = nullptr


IVR_DEFINE(Initialize);
IVR_DEFINE(Destroy);
IVR_DEFINE(GetSensorOrientation);
IVR_DEFINE(GetDisplayDeviceName);

IVR_DEFINE(GameConnectToServer);
IVR_DEFINE(StartSuccess);
IVR_DEFINE(EndGame);


void IVRDllProxy::FreeDependency(void*& Handle)
{
	if (Handle != nullptr)
	{
		FPlatformProcess::FreeDllHandle(Handle);
		Handle = nullptr;
	}
}

bool IVRDllProxy::LoadDependency(const FString& Dir, const FString& Name, void*& Handle)
{
	FString Lib = Name + TEXT(".") + FPlatformProcess::GetModuleExtension();
	FString Path = Dir.IsEmpty() ? *Lib : FPaths::Combine(*Dir, *Lib);

	Handle = FPlatformProcess::GetDllHandle(*Path);

	if (Handle == nullptr)
	{
		return false;
	}

	return true;
}

bool IVRDllProxy::LoadDll()
{
	// determine directory paths
	const FString BaseDir = IPluginManager::Get().FindPlugin("IVR")->GetBaseDir();
	const FString LibDir = FPaths::Combine(*BaseDir, TEXT("ThirdParty"), TEXT("LibIVR"));

	if (!LoadDependency(LibDir, TEXT("ImmersionPlugin"), DllHandle))
	{
		UE_LOG(LogIVR, Warning, TEXT("can not load IVRPlugin"));
		return false;
	}
	IVR_IMPORT("IVR_Initialize", Initialize);
	IVR_IMPORT("IVR_Destroy", Destroy);
	IVR_IMPORT("IVR_GetSensorOrientation", GetSensorOrientation);
	IVR_IMPORT("IVR_GetDisplayDeviceName", GetDisplayDeviceName);


	return Initialize() == 0;
}

bool IVRDllProxy::LoadDllServerManager()
{
	// determine directory paths
	const FString BaseDir = IPluginManager::Get().FindPlugin("IVR")->GetBaseDir();
	const FString LibDir = FPaths::Combine(*BaseDir, TEXT("ThirdParty"), TEXT("LibIVR"));

	if (!LoadDependency(LibDir, TEXT("MyServerManager"), DllMyServerManagerHandle))
	{
		UE_LOG(LogIVR, Warning, TEXT("can not load MyServerManager"));
		return false;
	}

	GameConnectToServer = (IVR_GameConnectToServer)FPlatformProcess::GetDllExport(DllMyServerManagerHandle, TEXT("GameConnectToServer"));
	StartSuccess = (IVR_GameConnectToServer)FPlatformProcess::GetDllExport(DllMyServerManagerHandle, TEXT("StartSuccess"));
	EndGame = (IVR_GameConnectToServer)FPlatformProcess::GetDllExport(DllMyServerManagerHandle, TEXT("EndGame"));

	return true;
}

bool IVRDllProxy::UnLoadDll()
{
	if ( Destroy && !(Destroy() == 0) )
	{
		UE_LOG(LogIVR, Warning, TEXT("can not Destroy IVRPlugin"));
		return false;
	}

	FreeDependency(DllHandle);
	FreeDependency(DllMyServerManagerHandle);

	return true;
}
