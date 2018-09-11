
#include "IVRHMDPCH.h"
#include "IVRHMD.h"
#include "IVRDllProxy.h"

#if WITH_EDITOR
#include "Editor/UnrealEd/Classes/Editor/EditorEngine.h"
#endif

DEFINE_LOG_CATEGORY(LogIVR)


namespace IVR
{
	const float InterpupillaryDistance = 0.064000f;
	const float InterpupillaryDistance_ChildrenVR = 0.061000f;

	extern const int XPoints = 40;
	extern const int YPoints = 40;
}


using namespace IVR;





/////////////////////////////////////
// Begin FIVRHMD Self API //
/////////////////////////////////////

FIVRHMD::FIVRHMD()
	: CurHmdOrientation(FQuat::Identity)
	, LastHmdOrientation(FQuat::Identity)
	, DeltaControlRotation(FRotator::ZeroRotator)
	, DeltaControlOrientation(FQuat::Identity)
	, BaseOrientation(FQuat::Identity)
	, RendererModule(nullptr)
	, DistortionMeshIndices(nullptr)
	, DistortionMeshVerticesLeftEye(nullptr)
	, DistortionMeshVerticesRightEye(nullptr)
	, DistortionPointsX(0)
	, DistortionPointsY(0)
	, bIsDllLoaded(false)
	, bIsChildrenEnable(true) // Set Default true
{
	// Get renderer module
	static const FName RendererModuleName("Renderer");
	RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);
	check(RendererModule);


	bIsDllLoaded = IVRDllProxy::LoadDll();
	bIsDllLoaded |= IVRDllProxy::LoadDllServerManager();

	if (bIsDllLoaded)
	{
		UE_LOG(LogIVR, Warning, TEXT("bIsDllLoaded true"));
	}
	else
	{
		UE_LOG(LogIVR, Warning, TEXT("bIsDllLoaded false"));
	}

	CustomPresent = MakeShareable(new FIVRCustomPresent);
}

FIVRHMD::~FIVRHMD()
{
	delete[] DistortionMeshIndices;
	DistortionMeshIndices = nullptr;

	delete[] DistortionMeshVerticesLeftEye;
	DistortionMeshVerticesLeftEye = nullptr;

	delete[] DistortionMeshVerticesRightEye;
	DistortionMeshVerticesRightEye = nullptr;

	IVRDllProxy::UnLoadDll();
}

bool FIVRHMD::IsInitialized() 
{
	return true;
}

void FIVRHMD::GetCurrentPose(FQuat& CurrentOrientation)
{
	// In Unity Forward is Z, Right is X, and Up is Y.In Unreal Forward is X, Right is Y, and Up is Z.
	IVRDllProxy::GetSensorOrientation(0, CurrentOrientation.W, CurrentOrientation.X, CurrentOrientation.Y, CurrentOrientation.Z);
	CurrentOrientation = ToFQuat(CurrentOrientation);
}

bool FIVRHMD::UpdatePlayerCamera(FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	GetCurrentPose(CurHmdOrientation);
	LastHmdOrientation = CurHmdOrientation;

	CurrentOrientation = CurHmdOrientation;
	CurrentPosition = FVector(0.0f, 0.0f, 0.0f);

	return true;
}

IRendererModule* FIVRHMD::GetRendererModule()
{
	return RendererModule;
}

void FIVRHMD::SetupDistortionData(const FIntPoint& RenderTargetSize)
{
	// calculate our values
	DistortionPointsX = FMath::Clamp<uint32>(XPoints, 2, 200);
	DistortionPointsY = FMath::Clamp<uint32>(YPoints, 2, 200);
	NumVerts = DistortionPointsX * DistortionPointsY;
	NumTris = (DistortionPointsX - 1) * (DistortionPointsY - 1) * 2;
	NumIndices = NumTris * 3;

	CustomPresent->CalculateDistortionScale(RenderTargetSize);

	// generate the distortion mesh
	GenerateDistortionCorrectionIndexBuffer();
	GenerateDistortionCorrectionVertexBuffer(eSSP_LEFT_EYE);
	GenerateDistortionCorrectionVertexBuffer(eSSP_RIGHT_EYE);
}


//////////////////////////////////////////////////////
// Begin ISceneViewExtension Pure-Virtual Interface //
//////////////////////////////////////////////////////

void FIVRHMD::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
	InViewFamily.EngineShowFlags.MotionBlur = 0;
	InViewFamily.EngineShowFlags.HMDDistortion = true;
	InViewFamily.EngineShowFlags.ScreenPercentage = false;
	InViewFamily.EngineShowFlags.StereoRendering = IsStereoEnabled();
}

void FIVRHMD::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	InView.BaseHmdOrientation = FQuat(FRotator(0.0f,0.0f,0.0f));
	InView.BaseHmdLocation = FVector(0.f);
//	WorldToMetersScale = InView.WorldToMetersScale;
	InViewFamily.bUseSeparateRenderTarget = ShouldUseSeparateRenderTarget();
}

void FIVRHMD::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
}

void FIVRHMD::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
}

void FIVRHMD::PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily)
{

}

////////////////////////////////////////////////////////////////////
// Begin FIVRHMD IStereoRendering Pure-Virtual Interface //
////////////////////////////////////////////////////////////////////
	
bool FIVRHMD::IsStereoEnabled() const
{
	return true;
}

bool FIVRHMD::EnableStereo(bool stereo)
{
	GEngine->bForceDisableFrameRateSmoothing = stereo;
	return stereo;
}

void FIVRHMD::AdjustViewRect(enum EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const
{
	SizeX = SizeX / 2;
	if( StereoPass == eSSP_RIGHT_EYE )
	{
		X += SizeX;
	}
}

void FIVRHMD::CalculateStereoViewOffset(const enum EStereoscopicPass StereoPassType, const FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation)
{
	if (StereoPassType != eSSP_FULL)
	{
		const float EyeOffset = (GetInterpupillaryDistance() * 0.5f) * WorldToMeters;
		const float PassOffset = (StereoPassType == eSSP_LEFT_EYE) ? -EyeOffset : EyeOffset;
		ViewLocation += ViewRotation.Quaternion().RotateVector(FVector(0, PassOffset, 0));
	}
}

FMatrix FIVRHMD::GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType, const float FOV) const
{
	const float HalfFov = 60.f / 180.f * PI / 2.f;
	const float XS = 1.0f / tan(HalfFov);
	const float YS = 1.0f / tan(HalfFov) * CustomPresent->StereoAspect;

	const float InNearZ = GNearClippingPlane;
	return FMatrix(
		FPlane(XS, 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, YS, 0.0f, 0.0f),
		FPlane(0.0f, 0.0f, 0.0f, 1.0f),
		FPlane(0.0f, 0.0f, InNearZ, 0.0f));
}

void FIVRHMD::InitCanvasFromView(class FSceneView* InView, class UCanvas* Canvas)
{
}

///////////////////////////////////////////////////////////////
// Begin FIVRHMD IStereoRendering Virtual Interface //
///////////////////////////////////////////////////////////////

void FIVRHMD::GetEyeRenderParams_RenderThread(const struct FRenderingCompositePassContext& Context, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const
{
	if (Context.View.StereoPass == eSSP_LEFT_EYE)
	{
		EyeToSrcUVOffsetValue.X = 0.0f;
		EyeToSrcUVOffsetValue.Y = 0.0f;

		EyeToSrcUVScaleValue.X = 0.5f;
		EyeToSrcUVScaleValue.Y = 1.0f;
	}
	else
	{
		EyeToSrcUVOffsetValue.X = 0.5f;
		EyeToSrcUVOffsetValue.Y = 0.0f;

		EyeToSrcUVScaleValue.X = 0.5f;
		EyeToSrcUVScaleValue.Y = 1.0f;
	}
}

bool FIVRHMD::ShouldUseSeparateRenderTarget() const
{
	check(IsInGameThread());
	return IsStereoEnabled();
}


///////////////////////////////////////////////////////////////////////
// Begin FIVRHMD IHeadMountedDisplay Pure-Virtual Interface //
///////////////////////////////////////////////////////////////////////

bool FIVRHMD::IsHMDConnected()
{
	// Just uses regular screen, so this is always true!
	return true;
}

bool FIVRHMD::IsHMDEnabled() const
{
	return true;
}

void FIVRHMD::EnableHMD(bool bEnable)
{
}

EHMDDeviceType::Type FIVRHMD::GetHMDDeviceType() const
{
	// Workaround needed for non-es2 post processing to call PostProcessHMD
	return EHMDDeviceType::DT_OculusRift; 
}

bool FIVRHMD::GetHMDMonitorInfo(MonitorInfo& OutMonitorInfo)
{
	OutMonitorInfo.MonitorName = "UnsupportedIVRHMDPlatform";
	OutMonitorInfo.MonitorId = 0;
	OutMonitorInfo.DesktopX = OutMonitorInfo.DesktopY = OutMonitorInfo.ResolutionX = OutMonitorInfo.ResolutionY = 0;
	OutMonitorInfo.WindowSizeX = OutMonitorInfo.WindowSizeY = 0;

	return false;
}

void	FIVRHMD::GetFieldOfView(float& InOutHFOVInDegrees, float& InOutVFOVInDegrees) const
{
	InOutHFOVInDegrees = 0.0f;
	InOutVFOVInDegrees = 0.0f;
}

bool	FIVRHMD::DoesSupportPositionalTracking() const
{
	// Does not support position tracking, only pose
	return false;
}

bool	FIVRHMD::HasValidTrackingPosition()
{
	// Does not support position tracking, only pose
	return false;
}

void	FIVRHMD::GetPositionalTrackingCameraProperties(FVector& OutOrigin, FQuat& OutOrientation, float& OutHFOV, float& OutVFOV, float& OutCameraDistance, float& OutNearPlane, float& OutFarPlane) const
{
	// Does not support position tracking, only pose
}

void FIVRHMD::SetInterpupillaryDistance(float NewInterpupillaryDistance)
{
	// Nothing
}

float FIVRHMD::GetInterpupillaryDistance() const
{
	return (bIsChildrenEnable ? InterpupillaryDistance_ChildrenVR : InterpupillaryDistance);
}

void FIVRHMD::GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	CurrentPosition = FVector(0.0f, 0.0f, 0.0f);

	GetCurrentPose(CurrentOrientation);
	CurHmdOrientation = LastHmdOrientation = CurrentOrientation;
}

void FIVRHMD::RebaseObjectOrientationAndPosition(FVector& Position, FQuat& Orientation) const
{
}

TSharedPtr<class ISceneViewExtension, ESPMode::ThreadSafe> FIVRHMD::GetViewExtension()
{
	TSharedPtr<FIVRHMD, ESPMode::ThreadSafe> ptr(AsShared());
	return StaticCastSharedPtr<ISceneViewExtension>(ptr);
}

void FIVRHMD::ApplyHmdRotation(class APlayerController* PC, FRotator& ViewRotation)
{
	ViewRotation.Normalize();

	GetCurrentPose(CurHmdOrientation);
	LastHmdOrientation = CurHmdOrientation;

	const FRotator DeltaRot = ViewRotation - PC->GetControlRotation();
	DeltaControlRotation = (DeltaControlRotation + DeltaRot).GetNormalized();

	// Pitch from other sources is never good, because there is an absolute up and down that must be respected to avoid motion sickness.
	// Same with roll.
	DeltaControlRotation.Pitch = 0;
	DeltaControlRotation.Roll = 0;
	DeltaControlOrientation = DeltaControlRotation.Quaternion();

	ViewRotation = FRotator(DeltaControlOrientation * CurHmdOrientation);
}

bool FIVRHMD::IsChromaAbCorrectionEnabled() const
{
	return false;
}

bool FIVRHMD::Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar )
{
	if (FParse::Command(&Cmd, TEXT("CHILDRENVR_ON")))
	{
		SetChildrenVREnabled(true);
		UE_LOG(LogIVR, Log, TEXT("CHILDRENVR_ON"));
		return true;
	}
	else if (FParse::Command(&Cmd, TEXT("CHILDRENVR_OFF")))
	{
		SetChildrenVREnabled(false);
		UE_LOG(LogIVR, Log, TEXT("CHILDRENVR_OFF"));
		return true;
	}
	else if (FParse::Command(&Cmd, TEXT("CHILDRENVR_POSITION")))
	{
		GEngine->GetGameUserSettings()->SetWindowPosition(1920, 0);
		UE_LOG(LogIVR, Log, TEXT("CHILDRENVR_POSITION"));
		return true;
	}

	return false;
}

bool FIVRHMD::IsPositionalTrackingEnabled() const
{
	// Does not support position tracking, only pose
	return false;
}

bool FIVRHMD::EnablePositionalTracking(bool enable)
{
	// Does not support position tracking, only pose
	return false;
}

bool FIVRHMD::IsHeadTrackingAllowed() const
{
	return true;
}

bool FIVRHMD::IsInLowPersistenceMode() const
{
	return false;
}

void FIVRHMD::EnableLowPersistenceMode(bool Enable)
{
}

void FIVRHMD::ResetOrientationAndPosition(float Yaw)
{
	ResetOrientation(Yaw);
	ResetPosition();
}

//////////////////////////////////////////////////////////////////
// Begin FIVRHMD IHeadMountedDisplay Virtual Interface //
//////////////////////////////////////////////////////////////////

void FIVRHMD::ResetOrientation(float Yaw)
{

}

void FIVRHMD::ResetPosition()
{
}

void FIVRHMD::SetBaseRotation(const FRotator& BaseRot)
{
	SetBaseOrientation(FRotator(0.0f, BaseRot.Yaw, 0.0f).Quaternion());
}

FRotator FIVRHMD::GetBaseRotation() const
{
	return GetBaseOrientation().Rotator();
}

void FIVRHMD::SetBaseOrientation(const FQuat& BaseOrient)
{
	BaseOrientation = BaseOrient;
}

FQuat FIVRHMD::GetBaseOrientation() const
{
	return BaseOrientation;
}

FString FIVRHMD::GetVersionString() const
{
	return FString(TEXT("IVRHMD")); 
}

/*
class FSceneViewport* FIVRHMD::FindSceneViewport()
{
	if (!GIsEditor)
	{
		UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
		return GameEngine->SceneViewport.Get();
	}
#if WITH_EDITOR
	else
	{
		UEditorEngine* EditorEngine = CastChecked<UEditorEngine>(GEngine);
		FSceneViewport* PIEViewport = (FSceneViewport*)EditorEngine->GetPIEViewport();
		if (PIEViewport != nullptr)
		{
			// PIE is setup for stereo rendering
			return PIEViewport;
		}
		else
		{
			// Check to see if the active editor viewport is drawing in stereo mode
			// @todo vreditor: Should work with even non-active viewport!
			FSceneViewport* EditorViewport = (FSceneViewport*)EditorEngine->GetActiveViewport();
			if (EditorViewport != nullptr)
			{
				return EditorViewport;
			}
		}
	}
#endif
	return nullptr;
}
*/

void FIVRHMD::SetChildrenVREnabled(bool bEnable)
{
	bIsChildrenEnable = bEnable;
}

bool FIVRHMD::IsChildrenVREnable()
{
	//UE_LOG(LogIVR, Log, TEXT("IsChildrenVREnable = %s"), bIsChildrenEnable ?  TEXT("Ture") : TEXT("False"));
	return bIsChildrenEnable;
}
