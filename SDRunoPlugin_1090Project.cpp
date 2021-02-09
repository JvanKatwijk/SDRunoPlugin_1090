#include <iunoplugin.h>
#include "SDRunoPlugin_1090.h"

extern "C" {

UNOPLUGINAPI IUnoPlugin* UNOPLUGINCALL
	                CreatePlugin(IUnoPluginController& controller) {
	return new SDRunoPlugin_1090 (controller);
}

UNOPLUGINAPI void UNOPLUGINCALL DestroyPlugin (IUnoPlugin* plugin) {
	delete plugin;
}

UNOPLUGINAPI unsigned int UNOPLUGINCALL GetPluginApiLevel () {
	return UNOPLUGINAPIVERSION;
}
}

