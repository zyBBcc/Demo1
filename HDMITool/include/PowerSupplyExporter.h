#pragma once

typedef enum  _InstrumentType
{
    POWER_66319 = 0,
    POWER_6700,
    NA
}InstrumentType;

#define CURR_ERRA -9999999.0

#define CURR_ERRMA -9999.9990

namespace Tinno
{
	class PowerSupplyExporter
	{
	public:
		PowerSupplyExporter() {}
		virtual ~PowerSupplyExporter() {}
		virtual bool Initial(int chan, char* pErr) = 0;
		virtual bool SetCurrentRange(int chan, char* pErr) = 0;
		virtual bool SetCurrentMin(int chan, char* pErr) = 0;
		virtual bool SetVoltage(int chan, double value, char* pErr) = 0;
		virtual bool SetCurrent(int chan, double value, char* pErr) = 0;
		virtual bool SetOutput(int chan, bool flag, char* pErr) = 0;
		virtual double GetCurrentA(int chan, char* pErr) = 0;
		virtual double GetCurrentmA(int chan, char* pErr) = 0;
		virtual const char* GetGpibAddr() = 0;
	};
}