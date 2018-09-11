
#include "IVRHMDPCH.h"
#include "IVR.h"
#include "IVRHMD.h"

//---------------------------------------------------
// IVRHMD Plugin Implementation
//---------------------------------------------------

class FIVR : public IIVR
{
	/** IHeadMountedDisplayModule implementation */
	virtual TSharedPtr< class IHeadMountedDisplay, ESPMode::ThreadSafe > CreateHeadMountedDisplay() override;

	FString GetModuleKeyName() const override
	{
		return FString(TEXT("IVRHMD"));
	}
};

IMPLEMENT_MODULE(FIVR, IVR)

TSharedPtr< class IHeadMountedDisplay, ESPMode::ThreadSafe > FIVR::CreateHeadMountedDisplay()
{
	TSharedPtr< FIVRHMD, ESPMode::ThreadSafe > IVRHMD(new FIVRHMD());
	if (IVRHMD->IsInitialized())
	{
		return IVRHMD;
	}
	return NULL;
}
