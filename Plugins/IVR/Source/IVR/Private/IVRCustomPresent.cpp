
#include "IVRHMDPCH.h"
#include "IVRCustomPresent.h"

namespace IVR
{
	const float DistK0 = 1.0f;
	const float DistK1 = 0.22f;
	const float DistK2 = 0.24f;

	const float DistK0_ChildrenVR = 1.0f;
	const float DistK1_ChildrenVR = 0.034f;
	const float DistK2_ChildrenVR = 0.004;

	const FVector2D DistortionCenter = FVector2D(0.5, 0.5);
	const float ScaleIn = 2.0f;

	const float DistortionFitX = 0.0f;
	const float DistortionFitY = 1.0f;
	const float DistortionFitScale = 1.0f;
}


using namespace IVR;


FIVRCustomPresent::FIVRCustomPresent() 
	: FRHICustomPresent(NULL)
	, DistortionScale()
	, StereoAspect(0.f)
{
}

void FIVRCustomPresent::CalculateDistortionScale(const FIntPoint& ViewportSize)
{
	float *K0, *K1, *K2;
	this->GetDistortionK(K0, K1, K2);

	StereoAspect = 0.5f * ViewportSize.X / ViewportSize.Y;

	float DX = (DistortionFitX * DistortionFitScale) * StereoAspect;
	float DY = (DistortionFitY * DistortionFitScale);
	float FitRadius = FMath::Sqrt(DX * DX + DY * DY);

	// This should match distortion equation used in shader.
	float Srq = FitRadius * FitRadius;
	float Scale = FitRadius * (*K0 + *K1 * Srq + *K2 * Srq * Srq);
	Scale /= FitRadius;
	Scale = 1 / Scale;

	DistortionScale.X = Scale * 0.5f;
	DistortionScale.Y = Scale * 0.5f; //  *StereoAspect;
}

FIVRHMD* FIVRCustomPresent::GetIVRHMD()
{
	if (GEngine && GEngine->HMDDevice.IsValid())
	{
		return static_cast<FIVRHMD*>(GEngine->HMDDevice.Get());
	}
	return nullptr;
}

void FIVRCustomPresent::GetDistortionK(float*& K0, float*& K1, float*& K2)
{
	if (GetIVRHMD() && GetIVRHMD()->IsChildrenVREnable())
	{
		K0 = const_cast<float*>(&DistK0_ChildrenVR);
		K1 = const_cast<float*>(&DistK1_ChildrenVR);
		K2 = const_cast<float*>(&DistK2_ChildrenVR);
	}
	else
	{
		K0 = const_cast<float*>(&DistK0);
		K1 = const_cast<float*>(&DistK1);
		K2 = const_cast<float*>(&DistK2);
	}
}