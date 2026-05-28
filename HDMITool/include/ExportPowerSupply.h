#pragma once

#include "PowerSupplyExporter.h"
// 导出宏：DLL 项目定义 EXPORT_API，调用方导入
#ifdef TINNOPOWERSUPPLYDLL_EXPORTS
#define TINNOPOWERSUPPLY_API extern "C" __declspec(dllexport)
#else
#define TINNOPOWERSUPPLY_API extern "C" __declspec(dllimport)
#endif

namespace Tinno
{
	TINNOPOWERSUPPLY_API PowerSupplyExporter* PWS_Create(InstrumentType type, const char* GpibNum, char* pErr);

	TINNOPOWERSUPPLY_API bool PWS_Close(PowerSupplyExporter* hBasePower, char* pErr);
}