#pragma once

#include "dllmodule.hpp"

IModule* DllModule::module;

bool DllModule::isValid = false;

void DllModule::CreateModule(CreateModuleType f)
{
	DllModule::module = f();
	DllModule::isValid = true;
}

void DllModule::LoadModule()
{
	DllModule::isValid = false;
	;
	HINSTANCE dllHandle = nullptr;
	dllHandle = LoadLibrary(L"module.dll");
	LOG(INFO) << "Loadind module..." << dllHandle;
	if (NULL != dllHandle)
	{
		LOG(INFO) << "Loaded shard lib : " << dllHandle;
		CreateModuleType create_module = nullptr;
		create_module = (CreateModuleType)GetProcAddress(dllHandle, "CreateModule");
		if (create_module == nullptr) {
			LOG(INFO) << "Error : Can't access CreateModule().";
		}
		else {
			LOG(INFO) << "CreateShard addr : " << (int)create_module;
			DllModule::CreateModule(create_module);
		}
	}
	else {
		LOG(INFO) << "Error : Can't load module.";
	}
}
