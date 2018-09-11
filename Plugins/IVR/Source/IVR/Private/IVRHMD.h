
#pragma once

#include "HeadMountedDisplay.h"
#include "IHeadMountedDisplay.h"
#include "SceneViewExtension.h"
#include "RHIStaticStates.h"
#include "SceneViewport.h"
#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "PostProcess/PostProcessHMD.h"
#include "IVRCustomPresent.h"



namespace IVR
{
	extern const float InterpupillaryDistance;
	extern const float InterpupillaryDistance_ChildrenVR;
	extern const int DistortionPointsX;
	extern const int DistortionPointsY;

	/**
	* Converts quat from IVR ref frame to Unreal
	*/
	template <typename IVRQuat>
	FORCEINLINE FQuat ToFQuat(const IVRQuat& InQuat)
	{
		return FQuat(float(-InQuat.Z), float(InQuat.X), float(InQuat.Y), float(-InQuat.W));
	}
}



/**
 * IVR Head Mounted Display
 */
class FIVRHMD : public IHeadMountedDisplay, public ISceneViewExtension, public TSharedFromThis<FIVRHMD, ESPMode::ThreadSafe>
{
public:

	FIVRHMD();
	~FIVRHMD();

	/** @return	True if the HMD was initialized OK */
	bool IsInitialized();

	/** Old Version Interface */
	//virtual void UpdatePlayerCameraRotation(class APlayerCameraManager* Camera, struct FMinimalViewInfo& POV) {}
	//virtual void OnScreenModeChange(EWindowMode::Type WindowMode) {};

private: /** Private Method */

	/** Get current pose */
	void GetCurrentPose(FQuat& CurrentOrientation);

	/** Helper method to get renderer module */
	IRendererModule* GetRendererModule();

	/** Helper method to generate index buffer for manual distortion rendering */
	void GenerateDistortionCorrectionIndexBuffer();

	/** Helper method to generate vertex buffer for manual distortion rendering */
	void GenerateDistortionCorrectionVertexBuffer(EStereoscopicPass Eye);

	/** Generates Distortion Correction Points and Data*/
	void SetupDistortionData(const FIntPoint& RenderTargetSize);

	/** Compute Distorted Point */
	void ComputeDistortedPoint(FVector2D &UVIn, FVector2D &UVOutR, FVector2D &UVOutG, FVector2D &UVOutB);

	/** Find Scene Viewport */
	//class FSceneViewport* FindSceneViewport();

public: /** BlueprintCallable Method */

	void SetChildrenVREnabled(bool bEnable);
	bool IsChildrenVREnable();

private: /** Private Data */

	/** Updating Data */
	FQuat		CurHmdOrientation;
	FQuat		LastHmdOrientation;
	FRotator	DeltaControlRotation;    // same as DeltaControlOrientation but as rotator
	FQuat		DeltaControlOrientation; // same as DeltaControlRotation but as quat
	mutable double	LastSensorTime;
	FQuat		BaseOrientation;

	/** Drawing Data */
	IRendererModule* RendererModule;
	uint16* DistortionMeshIndices;
	FDistortionVertex* DistortionMeshVerticesLeftEye;
	FDistortionVertex* DistortionMeshVerticesRightEye;

	/** Distortion Mesh */
	uint32 DistortionPointsX;
	uint32 DistortionPointsY;
	uint32 NumVerts;
	uint32 NumTris;
	uint32 NumIndices;

	/** Dll */
	bool bIsDllLoaded;
	bool bIsChildrenEnable;

	TSharedPtr<FIVRCustomPresent, ESPMode::ThreadSafe> CustomPresent;


public: /** Parent Method Interface */

	//////////////////////////////////////////////////////
	// Begin ISceneViewExtension Pure-Virtual Interface //
	//////////////////////////////////////////////////////

    /**
     * Called on game thread when creating the view family.
     */
    virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;

	/**
	 * Called on game thread when creating the view.
	 */
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;

    /**
     * Called on game thread when view family is about to be rendered.
     */
    virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;

    /**
     * Called on render thread at the start of rendering.
     */
    virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;

	/**
     * Called on render thread at the start of rendering, for each view, after PreRenderViewFamily_RenderThread call.
     */
    virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;

	///////////////////////////////////////////////////
	// Begin IStereoRendering Pure-Virtual Interface //
	///////////////////////////////////////////////////
	
	/** 
	 * Whether or not stereo rendering is on this frame.
	 */
	virtual bool IsStereoEnabled() const override;

	/** 
	 * Switches stereo rendering on / off. Returns current state of stereo.
	 * @return Returns current state of stereo (true / false).
	 */
	virtual bool EnableStereo(bool stereo = true) override;

	/**
	 * Adjusts the viewport rectangle for stereo, based on which eye pass is being rendered.
	 */
	virtual void AdjustViewRect(enum EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const override;

	/**
	 * Calculates the offset for the camera position, given the specified position, rotation, and world scale
	 */
	virtual void CalculateStereoViewOffset(const enum EStereoscopicPass StereoPassType, const FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation) override;

	/**
	 * Gets a projection matrix for the device, given the specified eye setup
	 */
	virtual FMatrix GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType, const float FOV) const override;

	/**
	 * Sets view-specific params (such as view projection matrix) for the canvas.
	 */
	virtual void InitCanvasFromView(class FSceneView* InView, class UCanvas* Canvas) override;

	//////////////////////////////////////////////
	// Begin IStereoRendering Virtual Interface //
	//////////////////////////////////////////////

	///** 
	// * Whether or not stereo rendering is on on next frame. Useful to determine if some preparation work
	// * should be done before stereo got enabled in next frame. 
	// */
	//virtual bool IsStereoEnabledOnNextFrame() const { return IsStereoEnabled(); }

	///**
	// * Gets the percentage bounds of the safe region to draw in.  This allows things like stat rendering to appear within the readable portion of the stereo view.
	// * @return	The centered percentage of the view that is safe to draw readable text in
	// */
	//virtual FVector2D GetTextSafeRegionBounds() const { return FVector2D(0.75f, 0.75f); }

	/**
	 * Returns eye render params, used from PostProcessHMD, RenderThread.
	 */
	virtual void GetEyeRenderParams_RenderThread(const struct FRenderingCompositePassContext& Context, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const override;

	///**
	// * Returns timewarp matrices, used from PostProcessHMD, RenderThread.
	// */
	//virtual void GetTimewarpMatrices_RenderThread(const struct FRenderingCompositePassContext& Context, FMatrix& EyeRotationStart, FMatrix& EyeRotationEnd) const {}

	// Optional methods to support rendering into a texture.
	/**
	 * Updates viewport for direct rendering of distortion. Should be called on a game thread.
	 * Optional SViewport* parameter can be used to access SWindow object.
	 */
	//virtual void UpdateViewport(bool bUseSeparateRenderTarget, const class FViewport& Viewport, class SViewport* = nullptr) override;

	/**
	 * Calculates dimensions of the render target texture for direct rendering of distortion.
	 */
	virtual void CalculateRenderTargetSize(const class FViewport& Viewport, uint32& InOutSizeX, uint32& InOutSizeY) override;

	/**
	 * Returns true, if render target texture must be re-calculated. 
	 */
	virtual bool NeedReAllocateViewportRenderTarget(const class FViewport& Viewport) override;

	// Whether separate render target should be used or not.
	virtual bool ShouldUseSeparateRenderTarget() const override;

	// Renders texture into a backbuffer. Could be empty if no rendertarget texture is used, or if direct-rendering 
	// through RHI bridge is implemented. 
	virtual void RenderTexture_RenderThread(class FRHICommandListImmediate& RHICmdList, class FRHITexture2D* BackBuffer, class FRHITexture2D* SrcTexture) const override;

	///**
	// * Called after Present is called.
	// */
	//virtual void FinishRenderingFrame_RenderThread(class FRHICommandListImmediate& RHICmdList) {}

	///**
	// * Returns orthographic projection , used from Canvas::DrawItem.
	// */
	//virtual void GetOrthoProjection(int32 RTWidth, int32 RTHeight, float OrthoDistance, FMatrix OrthoProjection[2]) const
	//{
	//	OrthoProjection[0] = OrthoProjection[1] = FMatrix::Identity;
	//	OrthoProjection[1] = FTranslationMatrix(FVector(OrthoProjection[1].M[0][3] * RTWidth * .25 + RTWidth * .5, 0, 0));
	//}

	///**
	// * Sets screen percentage to be used for stereo rendering.
	// *
	// * @param ScreenPercentage	(in) Specifies the screen percentage to be used in VR mode. Use 0.0f value to reset to default value.
	// */
	//virtual void SetScreenPercentage(float InScreenPercentage) {}
	//
	///** 
	// * Returns screen percentage to be used for stereo rendering.
	// *
	// * @return (float)	The screen percentage to be used in stereo mode. 0.0f, if default value is used.
	// */
	//virtual float GetScreenPercentage() const { return 0.0f; }

	/** 
	 * Sets near and far clipping planes (NCP and FCP) for stereo rendering. Similar to 'stereo ncp= fcp' console command, but NCP and FCP set by this
	 * call won't be saved in .ini file.
	 *
	 * @param NCP				(in) Near clipping plane, in centimeters
	 * @param FCP				(in) Far clipping plane, in centimeters
	 */
	//virtual void SetClippingPlanes(float NCP, float FCP) override;

	/**
	 * Returns currently active custom present. 
	 */
	virtual FRHICustomPresent* GetCustomPresent() override;

	/**
	 * Returns number of required buffered frames.
	 */
	//virtual uint32 GetNumberOfBufferedFrames() const override;

	/**
	 * Allocates a render target texture. 
	 *
	 * @param Index			(in) index of the buffer, changing from 0 to GetNumberOfBufferedFrames()
	 * @return				true, if texture was allocated; false, if the default texture allocation should be used.
	 */
	virtual bool AllocateRenderTargetTexture(uint32 Index, uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 Flags, uint32 TargetableTextureFlags, FTexture2DRHIRef& OutTargetableTexture, FTexture2DRHIRef& OutShaderResourceTexture, uint32 NumSamples = 1) override;

	//////////////////////////////////////////////////////
	// Begin IHeadMountedDisplay Pure-Virtual Interface //
	//////////////////////////////////////////////////////

	/**
	 * Returns true if HMD is currently connected.
	 */
	virtual bool IsHMDConnected() override;

	/**
	 * Whether or not switching to stereo is enabled; if it is false, then EnableStereo(true) will do nothing.
	 */
	virtual bool IsHMDEnabled() const override;

	/**
	 * Enables or disables switching to stereo.
	 */
	virtual void EnableHMD(bool bEnable = true) override;

	/**
	 * Returns the family of HMD device implemented 
	 */
	virtual EHMDDeviceType::Type GetHMDDeviceType() const override;

    /**
     * Get the name or id of the display to output for this HMD. 
     */
	virtual bool	GetHMDMonitorInfo(MonitorInfo&) override;
	
    /**
	 * Calculates the FOV, based on the screen dimensions of the device. Original FOV is passed as params.
	 */
	virtual void	GetFieldOfView(float& InOutHFOVInDegrees, float& InOutVFOVInDegrees) const override;

	/**
	 * Whether or not the HMD supports positional tracking (either via camera or other means)
	 */
	virtual bool	DoesSupportPositionalTracking() const override;

	/**
	 * If the device has positional tracking, whether or not we currently have valid tracking
	 */
	virtual bool	HasValidTrackingPosition() override;

	/**
	 * If the HMD supports positional tracking via a camera, this returns the frustum properties (all in game-world space) of the tracking camera.
	 */
	virtual void	GetPositionalTrackingCameraProperties(FVector& OutOrigin, FQuat& OutOrientation, float& OutHFOV, float& OutVFOV, float& OutCameraDistance, float& OutNearPlane, float& OutFarPlane) const override;	

	/**
	 * Accessors to modify the interpupillary distance (meters)
	 */
	virtual void	SetInterpupillaryDistance(float NewInterpupillaryDistance) override;
	virtual float	GetInterpupillaryDistance() const override;

    /**
     * Get the current orientation and position reported by the HMD.
     */
    virtual void GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition) override;

	/**
	 * Rebase the input position and orientation to that of the HMD's base
	 */
	virtual void RebaseObjectOrientationAndPosition(FVector& Position, FQuat& Orientation) const override;

	/**
	 * Get the ISceneViewExtension for this HMD, or none.
	 */
	virtual TSharedPtr<class ISceneViewExtension, ESPMode::ThreadSafe> GetViewExtension() override;

	/**
     * Apply the orientation of the headset to the PC's rotation.
     * If this is not done then the PC will face differently than the camera,
     * which might be good (depending on the game).
     */
	virtual void ApplyHmdRotation(class APlayerController* PC, FRotator& ViewRotation) override;

	/**
	 * Apply the orientation and position of the headset to the Camera.
	 */
	virtual bool UpdatePlayerCamera(FQuat& CurrentOrientation, FVector& CurrentPosition) override;

	/**
	 * Returns 'false' if chromatic aberration correction is off.
	 */
	virtual bool IsChromaAbCorrectionEnabled() const override;

	/**
	 * Exec handler to allow console commands to be passed through to the HMD for debugging
	 */
    virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) override;

	/** Returns true if positional tracking enabled and working. */
	virtual bool IsPositionalTrackingEnabled() const override;

	/** 
	 * Tries to enable positional tracking.
	 * Returns the actual status of positional tracking. 
	 */
	virtual bool EnablePositionalTracking(bool enable) override;

	/**
	 * Returns true, if head tracking is allowed. Most common case: it returns true when GEngine->IsStereoscopic3D() is true,
	 * but some overrides are possible.
	 */
	virtual bool IsHeadTrackingAllowed() const override;

	/**
	 * Returns true, if HMD is in low persistence mode. 'false' otherwise.
	 */
	virtual bool IsInLowPersistenceMode() const override;

	/**
	 * Switches between low and full persistence modes.
	 *
	 * @param Enable			(in) 'true' to enable low persistence mode; 'false' otherwise
	 */
	virtual void EnableLowPersistenceMode(bool Enable = true) override;

	/** 
	 * Resets orientation by setting roll and pitch to 0, assuming that current yaw is forward direction and assuming
	 * current position as a 'zero-point' (for positional tracking). 
	 *
	 * @param Yaw				(in) the desired yaw to be set after orientation reset.
	 */
	virtual void ResetOrientationAndPosition(float Yaw = 0.f) override;
	
	/////////////////////////////////////////////////
	// Begin IHeadMountedDisplay Virtual Interface //
	/////////////////////////////////////////////////

 // 	/**
	// * Gets the scaling factor, applied to the post process warping effect
	// */
	//virtual float GetDistortionScalingFactor() const { return 0; }

	///**
	// * Gets the offset (in clip coordinates) from the center of the screen for the lens position
	// */
	//virtual float GetLensCenterOffset() const { return 0; }

	///**
	// * Gets the barrel distortion shader warp values for the device
	// */
	//virtual void GetDistortionWarpValues(FVector4& K) const  { }

	///**
	// * Gets the chromatic aberration correction shader values for the device. 
	// * Returns 'false' if chromatic aberration correction is off.
	// */
	//virtual bool GetChromaAbCorrectionValues(FVector4& K) const  { return false; }

	///**
	// * Saves / loads pre-fullscreen rectangle. Could be used to store saved original window position 
	// * before switching to fullscreen mode.
	// */
	//virtual void PushPreFullScreenRect(const FSlateRect& InPreFullScreenRect);
	//virtual void PopPreFullScreenRect(FSlateRect& OutPreFullScreenRect);

	/** 
	 * Resets orientation by setting roll and pitch to 0, assuming that current yaw is forward direction. Position is not changed. 
	 *
	 * @param Yaw				(in) the desired yaw to be set after orientation reset.
	 */
	virtual void ResetOrientation(float Yaw = 0.f) override;

	/** 
	 * Resets position, assuming current position as a 'zero-point'. 
	 */
	virtual void ResetPosition() override;

	/** 
	 * Sets base orientation by setting yaw, pitch, roll, assuming that this is forward direction. 
	 * Position is not changed. 
	 *
	 * @param BaseRot			(in) the desired orientation to be treated as a base orientation.
	 */
	virtual void SetBaseRotation(const FRotator& BaseRot) override;

	/**
	 * Returns current base orientation of HMD as yaw-pitch-roll combination.
	 */
	virtual FRotator GetBaseRotation() const override;

	/** 
	 * Sets base orientation, assuming that this is forward direction. 
	 * Position is not changed. 
	 *
	 * @param BaseOrient		(in) the desired orientation to be treated as a base orientation.
	 */
	virtual void SetBaseOrientation(const FQuat& BaseOrient) override;

	/**
	 * Returns current base orientation of HMD as a quaternion.
	 */
	virtual FQuat GetBaseOrientation() const override;

	/**
	* @return true if a hidden area mesh is available for the device.
	*/
	//virtual bool HasHiddenAreaMesh() const override;

	/**
	* @return true if a visible area mesh is available for the device.
	*/
	//virtual bool HasVisibleAreaMesh() const override;

	/**
	* Optional method to draw a view's hidden area mesh where supported.
	* This can be used to avoid rendering pixels which are not included as input into the final distortion pass.
	*/
	//virtual void DrawHiddenAreaMesh_RenderThread(class FRHICommandList& RHICmdList, EStereoscopicPass StereoPass) const override;

	/**
	* Optional method to draw a view's visible area mesh where supported.
	* This can be used instead of a full screen quad to avoid rendering pixels which are not included as input into the final distortion pass.
	*/
	//virtual void DrawVisibleAreaMesh_RenderThread(class FRHICommandList& RHICmdList, EStereoscopicPass StereoPass) const override;

	virtual void DrawDistortionMesh_RenderThread(struct FRenderingCompositePassContext& Context, const FIntPoint& TextureSize) override;

	///**
	// * This method is able to change screen settings right before any drawing occurs. 
	// * It is called at the beginning of UGameViewportClient::Draw() method.
	// * We might remove this one as UpdatePostProcessSettings should be able to capture all needed cases
	// */
	//virtual void UpdateScreenSettings(const FViewport* InViewport);

	///**
	// * Allows to override the PostProcessSettings in the last moment e.g. allows up sampled 3D rendering
	// */
	//virtual void UpdatePostProcessSettings(FPostProcessSettings*) {}

	///**
	// * Draw desired debug information related to the HMD system.
	// * @param Canvas The canvas on which to draw.
	// */
	//virtual void DrawDebug(UCanvas* Canvas) {}

	///**
	// * Passing key events to HMD.
	// * If returns 'false' then key will be handled by PlayerController;
	// * otherwise, key won't be handled by the PlayerController.
	// */
	//virtual bool HandleInputKey(class UPlayerInput*, const struct FKey& Key, enum EInputEvent EventType, float AmountDepressed, bool bGamepad) { return false; }

	/**
	 * Passing touch events to HMD.
	 * If returns 'false' then touch will be handled by PlayerController;
	 * otherwise, touch won't be handled by the PlayerController.
	 */
	//virtual bool HandleInputTouch(uint32 Handle, ETouchType::Type Type, const FVector2D& TouchLocation, FDateTime DeviceTimestamp, uint32 TouchpadIndex) override;

	///**
	// * This method is called when playing begins. Useful to reset all runtime values stored in the plugin.
	// */
	//virtual void OnBeginPlay(FWorldContext& InWorldContext) override;

	///**
	// * This method is called when playing ends. Useful to reset all runtime values stored in the plugin.
	// */
	//virtual void OnEndPlay(FWorldContext& InWorldContext) override;

	/**
	 * This method is called when new game frame begins (called on a game thread).
	 */
	//virtual bool OnStartGameFrame( FWorldContext& WorldContext ) override;

	/**
	 * This method is called when game frame ends (called on a game thread).
	 */
	//virtual bool OnEndGameFrame( FWorldContext& WorldContext ) override;

	///** 
	// * Additional optional distorion rendering parameters
	// * @todo:  Once we can move shaders into plugins, remove these!
	// */	
	//virtual FTexture* GetDistortionTextureLeft() const {return NULL;}
	//virtual FTexture* GetDistortionTextureRight() const {return NULL;}
	//virtual FVector2D GetTextureOffsetLeft() const {return FVector2D::ZeroVector;}
	//virtual FVector2D GetTextureOffsetRight() const {return FVector2D::ZeroVector;}
	//virtual FVector2D GetTextureScaleLeft() const {return FVector2D::ZeroVector;}
	//virtual FVector2D GetTextureScaleRight() const {return FVector2D::ZeroVector;}

	//virtual bool NeedsUpscalePostProcessPass()  { return false; }

	/**
	 * Record analytics
	 */
	//virtual void RecordAnalytics() override;

	/**
	 * Returns version string.
	 */
	virtual FString GetVersionString() const override;
};
