
#include "IVRHMDPCH.h"
#include "IVRDllProxy.h"
#include "../Public/IVRBlueprintFunctionLibrary.h"


UIVRBlueprintFunctionLibrary::UIVRBlueprintFunctionLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


FIVRHMD* UIVRBlueprintFunctionLibrary::GetIVRHMD()
{
	if (GEngine && GEngine->HMDDevice.IsValid())
	{
		return static_cast<FIVRHMD*>(GEngine->HMDDevice.Get());
	}
	return nullptr;
}

//void UIVRBlueprintFunctionLibrary::SetChildrenVREnabled(bool bEnable)
//{
//	GetIVRHMD()->SetChildrenVREnabled(bEnable);
//}
//
//bool UIVRBlueprintFunctionLibrary::GetChildrenVREnable()
//{
//	return GetIVRHMD()->IsChildrenVREnable();
//}

void UIVRBlueprintFunctionLibrary::BeginGame()
{
	IVRDllProxy::GameConnectToServer();
	IVRDllProxy::StartSuccess();
}

void UIVRBlueprintFunctionLibrary::EndGame()
{
	IVRDllProxy::EndGame();
}
