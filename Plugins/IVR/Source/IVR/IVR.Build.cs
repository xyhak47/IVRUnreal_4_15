//AUTHOR:xyh
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class IVR : ModuleRules
	{
		public IVR(TargetInfo Target)
		{
            string BaseDirectory = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", ".."));
            string IVRPluginDllDirectory = Path.Combine(BaseDirectory, "ThirdParty", "LibIVR");

            string EngineDir = Path.GetFullPath(BuildConfiguration.RelativeEnginePath);

            PrivateIncludePaths.AddRange
			(
				new string[] 
				{
					"IVR/Private",
                    "IVR/Public",

                    Path.Combine(EngineDir, @"Source\Runtime\Renderer\Private"),
                    //Path.Combine(EngineDir, @"Source\Runtime\Engine\Classes\Kismet"),
                }
            );

			PrivateDependencyModuleNames.AddRange
			(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"RHI",
					"RenderCore",
					"Renderer",
					"ShaderCore",
					"HeadMountedDisplay",
                    "Projects",
                    "Slate",
                    "SlateCore"
                }
            );

            if (UEBuildConfiguration.bBuildEditor == true)
            {
                PrivateDependencyModuleNames.Add("UnrealEd");
            }

            if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64)
            {
                PrivateDependencyModuleNames.AddRange(new string[] { "D3D11RHI" });
                PrivateIncludePaths.AddRange
                (
                    new string[]
                    {
                        Path.Combine(EngineDir, @"Source\Runtime\Windows\D3D11RHI\Private"),
                        Path.Combine(EngineDir, @"Source\Runtime\Windows\D3D11RHI\Private\Windows")
                    }
                );

                PublicDelayLoadDLLs.Add("ImmersionPlugin.dll");
                RuntimeDependencies.Add(new RuntimeDependency(IVRPluginDllDirectory + "/ImmersionPlugin.dll"));

                PublicDelayLoadDLLs.Add("MyServerManager.dll");
                RuntimeDependencies.Add(new RuntimeDependency(IVRPluginDllDirectory + "/MyServerManager.dll"));
            }
        }
	}
}
