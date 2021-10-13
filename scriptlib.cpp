/*
 * scriptlib.cpp
 *
 *  Created on: 2010-7-29
 *      Author: Argon
 */
#include "scriptlib.h"
#include "duel.h"
#include "lua_obj.h"
#include "field.h"

namespace scriptlib {

bool check_param(lua_State* L, int32_t param_type, int32_t index, bool retfalse, void* retobj) {
	const char* type = nullptr;
	lua_obj* obj = nullptr;
	switch(param_type) {
	case PARAM_TYPE_CARD:
	case PARAM_TYPE_GROUP:
	case PARAM_TYPE_EFFECT:
		if((obj = lua_get<lua_obj*>(L, index)) != nullptr && obj->lua_type == param_type) {
			if(retobj)
				*(lua_obj**)retobj = obj;
			return true;
		} else if(obj && obj->lua_type == PARAM_TYPE_DELETED) {
			luaL_error(L, "Attempting to access deleted object.");
			unreachable();
		}
		type = param_type == PARAM_TYPE_CARD ? "Card" : param_type == PARAM_TYPE_GROUP ? "Group" : "Effect";
		break;
	case PARAM_TYPE_FUNCTION:
		if(lua_isfunction(L, index))
			return true;
		type = "Function";
		break;
	case PARAM_TYPE_STRING:
		if(lua_isstring(L, index))
			return true;
		type = "String";
		break;
	case PARAM_TYPE_INT:
		if(lua_isinteger(L, index) || lua_isnumber(L, index))
			return true;
		type = "Int";
		break;
	case PARAM_TYPE_BOOLEAN:
		if(lua_gettop(L) >= index)
			return true;
		type = "boolean";
		break;
	default:
		unreachable();
		break;
	}
	if(retfalse)
		return false;
	if(param_type != PARAM_TYPE_INT) {
		luaL_error(L, R"(Parameter %d should be "%s".)", index, type);
		unreachable();
	}
	interpreter::print_stacktrace(L);
	const auto pduel = lua_get<duel*>(L);
	pduel->handle_message(pduel->lua->format(R"(Parameter %d should be "%s".)", index, type), OCG_LOG_TYPE_ERROR);
	return false;
}

void check_param_count(lua_State* L, int32_t count) {
	if(lua_gettop(L) < count) {
		luaL_error(L, "%d Parameters are needed.", count);
		unreachable();
	}
}
void check_action_permission(lua_State* L) {
	if(lua_get<duel*>(L)->lua->no_action) {
		luaL_error(L, "Action is not allowed here.");
		unreachable();
	}
}
int32_t push_return_cards(lua_State* L, int32_t/* status*/, lua_KContext ctx) {
	const auto pduel = lua_get<duel*>(L);
	bool cancelable = (bool)ctx;
	if(pduel->game_field->return_cards.canceled) {
		if(cancelable) {
			lua_pushnil(L);
		} else {
			group* pgroup = pduel->new_group();
			interpreter::pushobject(L, pgroup);
		}
	} else {
		group* pgroup = pduel->new_group(pduel->game_field->return_cards.list);
		interpreter::pushobject(L, pgroup);
	}
	return 1;
}
int32_t is_deleted_object(lua_State* L) {
	if(auto obj = lua_touserdata(L, 1)) {
		auto* ret = *reinterpret_cast<lua_obj**>(obj);
		lua_pushboolean(L, ret->lua_type == PARAM_TYPE_DELETED);
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}
}