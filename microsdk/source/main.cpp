// classic API header files
#include "c4d_plugin.h"
#include "c4d_resource.h"

// project header files

// If the classic API is used PluginStart(), PluginMessage() and PluginEnd() must be implemented.

#include "objectdata_studiolights.h"

::Bool PluginStart()
{
	// register classic API plugins
	microsdk::RegisterStudioLightsObject();

	return true;
}

void PluginEnd()
{
	// free resources
}

::Bool PluginMessage(::Int32 id, void* data)
{
	switch (id)
	{
		case C4DPL_INIT_SYS:
		{
			// load resources defined in the the optional "res" folder
			if (g_resource.Init() == false)
			{
				return false;
			}
				

			return true;
		}
	}

	return true;
}
