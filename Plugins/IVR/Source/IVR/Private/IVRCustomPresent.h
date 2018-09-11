#pragma once

#include "IVR.h"


namespace IVR
{
	extern const float DistK0;
	extern const float DistK1;
	extern const float DistK2;

	extern const float DistK0_ChildrenVR;
	extern const float DistK1_ChildrenVR;
	extern const float DistK2_ChildrenVR;

	extern const FVector2D DistortionCenter;
	extern const float ScaleIn;

	extern const float DistortionFitX;
	extern const float DistortionFitY;
	extern const float DistortionFitScale;
}


class FIVRCustomPresent : public FRHICustomPresent
{
public:
	FIVRCustomPresent();
	// virtual methods from FRHICustomPresent
	virtual void OnBackBufferResize() override {}
	virtual bool Present(int32 &inOutSyncInterval) override { return true; }
	
	void CalculateDistortionScale(const FIntPoint& ViewportSize);
	void GetDistortionK(float*& K0, float*& K1, float*& K2);

protected:
	static class FIVRHMD* GetIVRHMD();

public:
	FVector2D	 DistortionScale;
	float		 StereoAspect;
};