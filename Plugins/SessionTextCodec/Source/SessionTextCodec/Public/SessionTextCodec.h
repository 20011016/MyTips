// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Runtime module for the Session Text Codec plugin.
 * Contains nothing but the blueprint function library used to make session settings UTF-8 safe.
 */
class FSessionTextCodecModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
