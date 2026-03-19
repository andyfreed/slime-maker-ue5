using UnrealBuildTool;

public class SlimeMaker : ModuleRules
{
	public SlimeMaker(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"ProceduralMeshComponent",
			"UMG",
			"Slate",
			"SlateCore",
			"Niagara",
			"RenderCore",
			"RHI"
		});
	}
}
