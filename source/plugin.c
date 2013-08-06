/*
 * TeamSpeak 3 demo plugin
 *
 * Copyright (c) 2008-2013 TeamSpeak Systems GmbH
 */

#ifdef _WIN32
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "public_errors.h"
#include "public_errors_rare.h"
#include "public_definitions.h"
#include "public_rare_definitions.h"
#include "ts3_functions.h"
#include "plugin.h"

static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 19

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

static char* pluginID = NULL;

#ifdef _WIN32
/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if(WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
		*result = NULL;
		return -1;
	}
	return 0;
}
#endif

/*********************************** Required functions ************************************/
/*
 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
 */

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
#ifdef _WIN32
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if(!result) {
		const wchar_t* name = L"CriticalBot";
		if(wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
			result = "CriticalBot";  /* Conversion failed, fallback here */
		}
	}
	return result;
#else
	return "CriticalBot";
#endif
}

/* Plugin version */
const char* ts3plugin_version() {
    return "1.2";
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "Skryttlock, Skutov";
}

/* Plugin description */
const char* ts3plugin_description() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "Utility plugin for The Crimson Tempest [CriT].";
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

/*
 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
 * If the function returns 1 on failure, the plugin will be unloaded again.
 */
int ts3plugin_init() {
    char appPath[PATH_BUFSIZE];
    char resourcesPath[PATH_BUFSIZE];
    char configPath[PATH_BUFSIZE];
	char pluginPath[PATH_BUFSIZE];

    /* Your plugin init code here */
    //printf("PLUGIN: init\n");

    /* Example on how to query application, resources and configuration paths from client */
    /* Note: Console client returns empty string for app and resources path */
    ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
    ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
    ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE);

	//printf("PLUGIN: App path: %s\nResources path: %s\nConfig path: %s\nPlugin path: %s\n", appPath, resourcesPath, configPath, pluginPath);

    return 0;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
	/* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
	 * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
	 * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
    /* Your plugin cleanup code here */
    //printf("PLUGIN: shutdown\n");

	/*
	 * Note:
	 * If your plugin implements a settings dialog, it must be closed and deleted here, else the
	 * TeamSpeak client will most likely crash (DLL removed but dialog from DLL code still open).
	 */

	/* Free pluginID if we registered it */
	if(pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

/****************************** Optional functions ********************************/

/*
 * If the plugin wants to use error return codes, plugin commands, hotkeys or menu items, it needs to register a command ID. This function will be
 * automatically called after the plugin was initialized. This function is optional. If you don't use these features, this function can be omitted.
 * Note the passed pluginID parameter is no longer valid after calling this function, so you must copy it and store it in the plugin.
 */
void ts3plugin_registerPluginID(const char* id) {
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
	//printf("PLUGIN: registerPluginID: %s\n", pluginID);
}

/* Plugin command keyword. Return NULL or "" if not used. */
const char* ts3plugin_commandKeyword() {
	return "critbot";
}

/* Plugin processes console command. Return 0 if plugin handled the command, 1 if not handled. */
int ts3plugin_processCommand(uint64 serverConnectionHandlerID, const char* command) {
	/*
	char buf[COMMAND_BUFSIZE];
	char *s, *param1 = NULL, *param2 = NULL;
	int i = 0;
	enum { CMD_NONE = 0 } cmd = CMD_NONE;
#ifdef _WIN32
	char* context = NULL;
#endif

	_strcpy(buf, COMMAND_BUFSIZE, command);
#ifdef _WIN32
	s = strtok_s(buf, " ", &context);
#else
	s = strtok(buf, " ");
#endif
	while(s != NULL) {
		if(i == 0) {
			if(!strcmp(s, "example1")) {
				cmd = CMD_EXAMPLE1;
			} else if(!strcmp(s, "example2")) {
				cmd = CMD_EXAMPLE2;
			}
		} else if(i == 1) {
			param1 = s;
		} else {
			param2 = s;
		}
#ifdef _WIN32
		s = strtok_s(NULL, " ", &context);
#else
		s = strtok(NULL, " ");
#endif
		i++;
	}
	switch(cmd) {
		case CMD_NONE:
			return 1;
	}
	*/
	return 0;
}

void ts3plugin_freeMemory(void* data) {
	free(data);
}

int ts3plugin_requestAutoload() {
	return 0; // do not request autoload. (change to 1 for autoload)
}


/************************** TeamSpeak callbacks ***************************/
/* Below are optional functions. */


enum SERVER_GROUPS_T { SG_GW2_THIEF=27 };
enum CHANNEL_GROUPS_T { CG_MCTEAM=13 };
enum CHANNELS_T { C_OFFICERS_ROOM=19 };

void ts3plugin_onServerGroupClientAddedEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientName, const char* clientUniqueIdentity, uint64 serverGroupID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity)
{
	//get connection handler
	crit.iServerConnectionHandlerID = serverConnectionHandlerID;
	//get client ID
	crit.iClientID = clientID;
	//get client name
	strcpy(crit.sClientName,clientName);
	//get client UID
	strcpy(crit.sClientUniqueID,clientUniqueIdentity);
	//get server group ID
	crit.iServerGroupID = serverGroupID;

	//set channel group 
	crit.channelGroupToAdd = CG_MCTEAM;
	crit.channelToAdd = C_OFFICERS_ROOM;

	//initialise client DBID to 0 and request it (calls ts3plugin_onClientDBIDfromUIDEvent)
	crit.iClientDBID = 0;
	ts3Functions.requestClientDBIDfromUID(serverConnectionHandlerID,crit.sClientUniqueID,NULL);
}

void ts3plugin_onClientDBIDfromUIDEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, uint64 clientDatabaseID)
{	
	//store it in global memory
	crit.iClientDBID = clientDatabaseID;

	//set channel groups using server groups
	if (crit.iServerGroupID==SG_GW2_THIEF) {
		ts3Functions.requestSetClientChannelGroup(crit.iServerConnectionHandlerID,&crit.channelGroupToAdd,&crit.channelToAdd,&crit.iClientDBID,1,NULL);
	}

	//reset the crit struct
	crit.iServerConnectionHandlerID = 0;
	crit.iClientID = 0;
	strcpy(crit.sClientName," ");
	strcpy(crit.sClientUniqueID," ");
	crit.iServerGroupID = 0;
	crit.iClientDBID = 0;
	crit.channelGroupToAdd = 0;
	crit.channelToAdd = 0;
}

void ts3plugin_onServerGroupClientDeletedEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientName, const char* clientUniqueIdentity, uint64 serverGroupID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity)
{


}

void ts3plugin_onClientChannelGroupChangedEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, uint64 channelID, anyID clientID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity) 
{


}