
#include "IVRHMDPCH.h"
#include "IVRHMD.h"
#include "ScreenRendering.h"

using namespace IVR;

#define ENABLE_LOG_DISTORTION_MESH_DATA 0


void FIVRHMD::ComputeDistortedPoint(FVector2D &UVIn, FVector2D &UVOutR, FVector2D &UVOutG, FVector2D &UVOutB)
{
	//float2 vecFromCenter = (in01 - _Center) * _ScaleIn; // Scales to [-1, 1] 
	//float  rSq = vecFromCenter.x * vecFromCenter.x + vecFromCenter.y * vecFromCenter.y;
	//float2 vecResult = vecFromCenter * (_HmdWarpParam.x + _HmdWarpParam.y * rSq + _HmdWarpParam.z * rSq * rSq);
	//return _Center + _Scale * vecResult;

	float *K0, *K1, *K2;
	CustomPresent->GetDistortionK(K0, K1, K2);

	FVector2D VecFromCenter = (UVIn - DistortionCenter) * ScaleIn; // convert : from 0~1 to -1~1
	float Rsq = VecFromCenter.X * VecFromCenter.X + VecFromCenter.Y * VecFromCenter.Y;
	FVector2D VecResult = VecFromCenter * (*K0 + *K1 * Rsq + *K2 * Rsq * Rsq);

	UVOutR = UVOutG = UVOutB = (DistortionCenter + CustomPresent->DistortionScale * VecResult); // no dispersion of light
}

void FIVRHMD::GenerateDistortionCorrectionIndexBuffer()
{
	// Delete existing indices if they exist
	delete [] DistortionMeshIndices;
	DistortionMeshIndices = nullptr;

	// Allocate new indices
	DistortionMeshIndices = new uint16[NumIndices];

	uint32 InsertIndex = 0;
	for(uint32 y = 0; y < DistortionPointsY - 1; ++y)
	{
		for(uint32 x = 0; x < DistortionPointsX - 1; ++x)
		{
			// Calculate indices for the triangle
			const uint16 BottomLeft =	(y * DistortionPointsX) + x + 0;
			const uint16 BottomRight =	(y * DistortionPointsX) + x + 1;
			const uint16 TopLeft =		(y * DistortionPointsX) + x + 0 + DistortionPointsX;
			const uint16 TopRight =		(y * DistortionPointsX) + x + 1 + DistortionPointsX;

			// Insert indices
			DistortionMeshIndices[InsertIndex + 0] = BottomLeft;
			DistortionMeshIndices[InsertIndex + 1] = BottomRight;
			DistortionMeshIndices[InsertIndex + 2] = TopRight;
			DistortionMeshIndices[InsertIndex + 3] = BottomLeft;
			DistortionMeshIndices[InsertIndex + 4] = TopRight;
			DistortionMeshIndices[InsertIndex + 5] = TopLeft;
			InsertIndex += 6;
		}
	}

	check(InsertIndex == NumIndices);
}

void FIVRHMD::GenerateDistortionCorrectionVertexBuffer(EStereoscopicPass Eye)
{
	FDistortionVertex** UsingPtr = (Eye == eSSP_LEFT_EYE) ? &DistortionMeshVerticesLeftEye : &DistortionMeshVerticesRightEye;
	FDistortionVertex*& Verts = *UsingPtr;

	// Cleanup old data if necessary
	delete[] Verts;
	Verts = nullptr;


	// Allocate new vertex buffer
	Verts = new FDistortionVertex[NumVerts];


#if ENABLE_LOG_DISTORTION_MESH_DATA
	const TCHAR* EyeString = Eye == eSSP_LEFT_EYE ? TEXT("Left") : TEXT("Right");
	UE_LOG(LogIVR, Log, TEXT("===== Begin Distortion Mesh Eye %s"), EyeString);
	UE_LOG(LogIVR, Log, TEXT("const unsigned int Num%sVertices = %d;"), EyeString, NumVerts);
	UE_LOG(LogIVR, Log, TEXT("FDistortionVertex %sVertices[Num%sVertices] = {"), EyeString, EyeString);
#endif

	// Fill out distortion vertex info, 
	uint32 VertexIndex = 0;
	for (uint32 y = 0; y < DistortionPointsY; ++y)
	{
		for (uint32 x = 0; x < DistortionPointsX; ++x)
		{
			FVector2D UndistortedCoord = FVector2D{ float(x) / float(DistortionPointsX - 1), float(y) / float(DistortionPointsY - 1) };
			FVector2D DistortedCoords[3];

			ComputeDistortedPoint(UndistortedCoord, DistortedCoords[0], DistortedCoords[1], DistortedCoords[2]);

			const FVector2D ScreenPos = FVector2D(UndistortedCoord.X * 2.0f - 1.0f, UndistortedCoord.Y * 2.0f - 1.0f);

			const FVector2D UndistortedUV = FVector2D(UndistortedCoord.X, UndistortedCoord.Y);
			const FVector2D OrigRedUV = FVector2D(DistortedCoords[0].X, DistortedCoords[0].Y);
			const FVector2D OrigGreenUV = FVector2D(DistortedCoords[1].X, DistortedCoords[1].Y);
			const FVector2D OrigBlueUV = FVector2D(DistortedCoords[2].X, DistortedCoords[2].Y);

			// Final distorted UVs
			FVector2D FinalRedUV = OrigRedUV;
			FinalRedUV.Y = 1.0f - FinalRedUV.Y;
			FVector2D FinalGreenUV = OrigGreenUV;
			FinalGreenUV.Y = 1.0f - FinalGreenUV.Y;
			FVector2D FinalBlueUV = OrigBlueUV;
			FinalBlueUV.Y = 1.0f - FinalBlueUV.Y;

			FDistortionVertex FinalVertex = FDistortionVertex{ ScreenPos, FinalRedUV, FinalGreenUV, FinalBlueUV, 1.0f, 0.0f };
			Verts[VertexIndex++] = FinalVertex;

#if ENABLE_LOG_DISTORTION_MESH_DATA
			UE_LOG(LogIVR, Log, TEXT("\tFDistortionVertex{ FVector2D(%ff, %ff), FVector2D(%ff, %ff), FVector2D(%ff, %ff), FVector2D(%ff, %ff), 1.0f, 0.0f }%s"),
				ScreenPos.X, ScreenPos.Y,
				FinalRedUV.X, FinalRedUV.Y,
				FinalGreenUV.X, FinalGreenUV.Y,
				FinalBlueUV.X, FinalBlueUV.Y,
				VertexIndex != NumVerts ? TEXT(",") : TEXT(""));
#endif
		}
	}

#if ENABLE_LOG_DISTORTION_MESH_DATA
	UE_LOG(LogIVR, Log, TEXT("};"));
	UE_LOG(LogIVR, Log, TEXT("===== End Distortion Mesh Eye %s"), EyeString);
#endif

	check(VertexIndex == NumVerts);
}

void FIVRHMD::DrawDistortionMesh_RenderThread(struct FRenderingCompositePassContext& Context, const FIntPoint& TextureSize)
{
	const FSceneView& View = Context.View;
	FRHICommandListImmediate& RHICmdList = Context.RHICmdList;
	const FSceneViewFamily& ViewFamily = *(View.Family);
	FIntPoint ViewportSize = ViewFamily.RenderTarget->GetSizeXY();

	if (View.StereoPass == eSSP_LEFT_EYE)
	{
		RHICmdList.SetViewport(0, 0, 0.0f, ViewportSize.X / 2, ViewportSize.Y, 1.0f);
		DrawIndexedPrimitiveUP(
			Context.RHICmdList, 
			PT_TriangleList, 
			0,
			NumVerts,
			NumTris, 
			DistortionMeshIndices,
			sizeof(DistortionMeshIndices[0]), 
			DistortionMeshVerticesLeftEye,
			sizeof(DistortionMeshVerticesLeftEye[0]));
	}
	else
	{
		RHICmdList.SetViewport(ViewportSize.X / 2, 0, 0.0f, ViewportSize.X, ViewportSize.Y, 1.0f);
		DrawIndexedPrimitiveUP(
			Context.RHICmdList, 
			PT_TriangleList, 
			0, 
			NumVerts,
			NumTris,
			DistortionMeshIndices,
			sizeof(DistortionMeshIndices[0]),
			DistortionMeshVerticesRightEye,
			sizeof(DistortionMeshVerticesRightEye[0]));
	}
}

void FIVRHMD::RenderTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef BackBuffer, FTexture2DRHIParamRef SrcTexture) const
{
	check(IsInRenderingThread());

	const uint32 ViewportWidth = BackBuffer->GetSizeX();
	const uint32 ViewportHeight = BackBuffer->GetSizeY();
	//const uint32 TextureWidth = SrcTexture->GetSizeX();
	//const uint32 TextureHeight = SrcTexture->GetSizeY();
	
	//UE_LOG(LogIVR, Log, TEXT("RenderTexture_RenderThread() Viewport:(%d, %d) Texture:(%d, %d) BackBuffer=%p SrcTexture=%p"), ViewportWidth, ViewportHeight, TextureWidth, TextureHeight, BackBuffer, SrcTexture);

	RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());
	RHICmdList.SetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());

	// Just render directly to output
	{
		SetRenderTarget(RHICmdList, BackBuffer, FTextureRHIRef());
		RHICmdList.SetViewport(0, 0, 0, ViewportWidth, ViewportHeight, 1.0f);

		const auto FeatureLevel = GMaxRHIFeatureLevel;
		auto ShaderMap = GetGlobalShaderMap(FeatureLevel);

		TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
		TShaderMapRef<FScreenPS> PixelShader(ShaderMap);

		static FGlobalBoundShaderState BoundShaderState;
		SetGlobalBoundShaderState(RHICmdList, FeatureLevel, BoundShaderState, RendererModule->GetFilterVertexDeclaration().VertexDeclarationRHI, *VertexShader, *PixelShader);

		PixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Bilinear>::GetRHI(), SrcTexture);

		RendererModule->DrawRectangle(
			RHICmdList,
			0, 0,
			ViewportWidth, ViewportHeight,
			0.0f, 0.0f,
			1.0f, 1.0f,
			FIntPoint(ViewportWidth, ViewportHeight),
			FIntPoint(1, 1),
			*VertexShader,
			EDRF_Default);
	}
}

FRHICustomPresent* FIVRHMD::GetCustomPresent()
{
	return CustomPresent.Get();
}

void FIVRHMD::CalculateRenderTargetSize(const class FViewport& Viewport, uint32& InOutSizeX, uint32& InOutSizeY)
{
	check(IsInGameThread());

	MonitorInfo Info;
	if (GetHMDMonitorInfo(Info))
	{
		InOutSizeX = Info.ResolutionX;
		InOutSizeY = Info.ResolutionY;
	}
}

bool FIVRHMD::NeedReAllocateViewportRenderTarget(const class FViewport& Viewport)
{
	check(IsInGameThread());

	if (IsStereoEnabled())
	{
		const uint32 InSizeX = Viewport.GetSizeXY().X;
		const uint32 InSizeY = Viewport.GetSizeXY().Y;
		FIntPoint RenderTargetSize;
		RenderTargetSize.X = Viewport.GetRenderTargetTexture()->GetSizeX();
		RenderTargetSize.Y = Viewport.GetRenderTargetTexture()->GetSizeY();

		uint32 NewSizeX = InSizeX, NewSizeY = InSizeY;
		//CalculateRenderTargetSize(Viewport, NewSizeX, NewSizeY);

		if (NewSizeX != RenderTargetSize.X || NewSizeY != RenderTargetSize.Y)
		{
			SetupDistortionData(FIntPoint(NewSizeX, NewSizeX));

			UE_LOG(LogIVR, Log, TEXT("NeedReAllocateViewportRenderTarget() Needs realloc to new size: (%d, %d)"), NewSizeX, NewSizeY);
			return true;
		}
	}
	return false;
}

// try to allocate texture via StereoRenderingDevice; if not successful, use the default way
// said in RenderingCompositionGraph.cpp , line 1536
bool FIVRHMD::AllocateRenderTargetTexture(uint32 Index, uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 Flags, uint32 TargetableTextureFlags, FTexture2DRHIRef& OutTargetableTexture, FTexture2DRHIRef& OutShaderResourceTexture, uint32 NumSamples)
{
	SetupDistortionData(FIntPoint(SizeX, SizeY));
	return false;
}
