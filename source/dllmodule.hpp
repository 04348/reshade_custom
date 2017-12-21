#pragma once

#include "module.hpp"
#include "log.hpp"
#include <Windows.h>

class DllModule {
public:
	static void CreateModule(CreateModuleType f);
	static void LoadModule();
	static bool isValid;

	static IModule* module;
};
