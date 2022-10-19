#ifndef __LUA_IMPGUI_H__
#define __LUA_IMPGUI_H__

#include <autodo.h>

#ifdef __cplusplus
extern "C" {
#endif

AUTO_LOCAL extern const auto_api_t* api;

/**
 * @brief Initialize library.
 * @param[in] L     Lua VM.
 * @return          always 1.
 */
AUTO_EXPORT int luaopen_imgui(lua_State *L);

#ifdef __cplusplus
}
#endif
#endif
