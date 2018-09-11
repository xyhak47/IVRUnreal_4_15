#pragma once
#include "IVRBlueprintFunctionLibrary.generated.h"

UCLASS()
class UIVRBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	//UFUNCTION(BlueprintCallable, Category = "IVRHMD", meta = (Keywords = "Children"))
	//static void SetChildrenVREnabled(bool bEnable);

	//UFUNCTION(BlueprintPure, Category = "IVRHMD", meta = (Keywords = "Children"))
	//static bool GetChildrenVREnable();

	UFUNCTION(BlueprintCallable, Category = "IVRHMD", meta = (Keywords = "VR"))
	static void BeginGame();

	UFUNCTION(BlueprintCallable, Category = "IVRHMD", meta = (Keywords = "VR"))
	static void EndGame();

protected:
	static class FIVRHMD* GetIVRHMD();
};